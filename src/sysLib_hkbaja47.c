/* sysLib.c - Heurikon Baja system dependent library */

/* Copyright 1995-1996 Heurikon Corporation */

/*
modification history
--------------------
02e,15may96,tjf made sysPciIntEnable() LOCAL to comply with latest PCI spec
02d,30apr96,tjf modifications for Baja68k
02c,06apr96,tjf added PCI ROM signature check, empty ROM sockets caused lockups
02b,01apr96,tjf moved byte swap macros to pci.h, added PCI RMW function to
                sysRMWCycle(), improved references to PCI_XX configuration
                variables, removed sysLocalToPciMemAdrs(), added
                sysPci9060DmaCopy() and sysPci9060Int(), fixed PCI expansion
                ROM bug, removed some calls to sysPci9060BusErrorClear().
02a,01feb96,tjf changes for VxWorks PCI s/w interface specification
01e,28dec95,tjf set VIC dual path enable for true master/MASTER cycles
01d,28dec95,tjf changed PCI arbitration, increased slave delay clocks
01c,29sep95,tjf changed PCI9060 target retry delay clocks (it was illegal)
01b,20sep95,tjf enhanced PCI, changed sysAutoAck(), more sysWbFlush() calls
01a,01sep95,tjf written by Ted Friedl, Heurikon Corporation
*/

/*
DESCRIPTION
This library provides board-specific routines.  The chip drivers included are:
.CS
    ds1286Rtc.c - Dallas 1286 Realtime clock library
    intPci.c - General PCI interrupt library
    hkpigSerial.c - Heurikon's Peripheral Interface Gate Array tty library
    hkpigTimer.c - Heurikon's Peripheral Interface Gate Array timer library
    vicVme.c - VMEbus Interface Controller (VIC) library
    x24c16Mem.c  - Xicor 24c16 NVRAM library
.CE

INCLUDE FILES: sysLib.h

SEE ALSO:
.pG "Configuration"
*/

/* includes */

#include "vxWorks.h"
#include "vme.h"
#include "excLib.h"
#include "fioLib.h"
#include "sysLib.h"
#include "config.h"
#include "private/vmLibP.h"
#include "vxLib.h"
#include "taskLib.h"
#include "intLib.h"
#include "stdio.h"
#include "string.h"
#include "tyLib.h"
#include "inetLib.h"
#include "in.h"
#include "tickLib.h"
#include "cacheLib.h"
#include "semLib.h"
#include "drv/mem/i28f008.h"
#include "drv/pci/pci9060.h"

/* defines */

#define MKLONG(from)          ((0x0000ffff &((ULONG)from >> 16)) | \
                               ((ULONG)from << 16))
#define PHYS_ADDR_SIZE        6
#define PCI9060               PCI9060_BASE_ADRS
#define SM_MEM_A32_BASE_ADRS  0x40000000

#ifndef PCI_LATENCY
#define PCI_LATENCY 0xff
#endif /* PCI_LATENCY */

#define BIT0                  0x00000001
#define BIT1                  0x00000002
#define BIT2                  0x00000004
#define BIT3                  0x00000008
#define BIT4                  0x00000010
#define BIT5                  0x00000020
#define BIT6                  0x00000040
#define BIT7                  0x00000080
#define BIT8                  0x00000100
#define BIT9                  0x00000200
#define BIT10                 0x00000400
#define BIT11                 0x00000800
#define BIT12                 0x00001000
#define BIT13                 0x00002000
#define BIT14                 0x00004000
#define BIT15                 0x00008000
#define BIT16                 0x00010000
#define BIT17                 0x00020000
#define BIT18                 0x00040000
#define BIT19                 0x00080000
#define BIT20                 0x00100000
#define BIT21                 0x00200000
#define BIT22                 0x00400000
#define BIT23                 0x00800000
#define BIT24                 0x01000000
#define BIT25                 0x02000000
#define BIT26                 0x04000000
#define BIT27                 0x08000000
#define BIT28                 0x10000000
#define BIT29                 0x20000000
#define BIT30                 0x40000000
#define BIT31                 0x80000000

#define VIC_IPL0              0x01
#define VIC_IPL1              0x02
#define VIC_IPL2              0x04

typedef struct
    {
    NODE              node;        /* node for lstLib linked list */
    UINT32            configAdrs;  /* configuration space address of reg 0 */
    BOOL              used;        /* TRUE if a driver has ownership */
    UINT8             bitMap;      /* used by sysPciInit() */
    PCI_CONFIG_DESC * pPciConfigDesc; /* PCI descriptor */
    } PCI_CONFIG;

#if (CPU == R4000)

typedef struct
    {
    UINT32  intCause;         /* cause of interrupt */
    UINT32  bsrTableOffset;   /* index to BSR table */
    UINT32  statusReg;        /* interrupt level */
    UINT32  pad;              /* pad for ease of use */
    } PRIO_TABLE;

#endif /* (CPU == R4000) */

/* externals */

#if (CPU == R4000)

IMPORT UINT8 ffsMsbTbl [];
IMPORT void  fpIntr ();
IMPORT void  sysExcUtlbVec ();

#endif /* (CPU == R4000) */

/* globals */
 
#if (CPU == R4000)

/*
 *       intPrioTable is a board dependant structure that aids in the
 *       processing of the 8 R4000 interrupt conditions.
 *       It is used by excLib to determine the pending interrupt and
 *       to call the user attached handler.  The present search algorithm
 *       relies on a one to one mapping of R4000 interrupt lines and
 *       table entries.
 *
 *       Each entry has 4 fields, the first (intCause) is the interrupt ID,
 *       second (bsrTableOffset) is the vector number, third is unused
 *       last (pad) is the multiplex field.  When an interrupt is received
 *       the handler maps the pending R4000 line to  its' corresponding
 *       table entry (ie. Software interrupt 0 would map to the first entry
 *       in the table).  At this point interrupts that are not pending are
 *       enabled.  Next the multiplex field is read, if it is zero, field
 *       two is taken as the interrupt vector number in the BSR table,
 *       otherwise it is interpreted as a demultiplex function and called
 *       with field 4 passed as it's parameter.  The job of the demultiplex
 *       routine is to calculate the correct interrupt vector number and pass
 *       it back to the handler.  The handler can then call the routine the
 *       user has installed in the BSR table with intVecSet, or intConnect.
 *
 *       This table is critical to interrupt processing.  Do not alter
 *       its contents until you understand the consequences.  The routine
 *       sysPrioUpdate() can aid modification during runtime.
 */

LOCAL int sysVicDeMux (ULONG);       /* necessary for this table */
LOCAL void sysMemParityInt (void);   /* necessary for this table */
PRIO_TABLE intPrioTable[8] =
    {
    {CAUSE_SW1,(UINT32) IV_SWTRAP0_VEC,   0, 0},        /* sw trap 0 */
    {CAUSE_SW2,(UINT32) IV_SWTRAP1_VEC,   0, 0},        /* sw trap 1 */
    {CAUSE_IP3,(UINT32) sysVicDeMux,      0, VIC_IPL1}, /* VIC interrupt */
    {CAUSE_IP4,(UINT32) NULL,             0, 0},        /* not used */
    {CAUSE_IP5,(UINT32) NULL,             0, 0},        /* not used */
    {CAUSE_IP6,(UINT32) IV_BUS_ERROR_VEC, 0, 0},        /* bus error */
    {CAUSE_IP7,(UINT32) sysMemParityInt,  0, 0},        /* parity error */
    {CAUSE_IP8,(UINT32) NULL,             0, 0}         /* not used */
    };

UINT8 * sysHashOrder = ffsMsbTbl;       /* interrupt prio., 7 = high 0 = low */
FUNCPTR sysCacheLibInit = (FUNCPTR) cacheR4kLibInit;
                                        /* pull in R4000 cache library */
#else /* (CPU == R4000) */

/*
 * Two caveats for sysPhysMemDesc[] on 68040 boards:
 * 1. All address and length parameters must be page-aligned; that is,
 *    multiples of 0x2000.
 * 2. Do not map any single area larger than 32 Mbytes, even if the
 *    hardware supports it.
 */

PHYS_MEM_DESC sysPhysMemDesc [] =
    {
    /* adrs and length parameters must be page-aligned (multiples of 0x2000) */

    /* RAM (this must remain the first entry in sysPhysMemDesc) */
    {
    (void *) LOCAL_MEM_LOCAL_ADRS,
    (void *) LOCAL_MEM_LOCAL_ADRS,
    0x02000000,    /* 32M bytes by default */
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE
    },

    /* Additional RAM chunk, used for 64 megs */
    {
    (void *) (LOCAL_MEM_LOCAL_ADRS+0x02000000),
    (void *) (LOCAL_MEM_LOCAL_ADRS+0x02000000),
    0x02000000, /* 32M bytes by default */
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE
    },

#ifdef INCLUDE_PCI
    /* PCI space */
    {
    (void *) PCI_MEM_ADRS,
    (void *) PCI_MEM_ADRS,
    0x02000000,    /* 32 Mbytes */
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    {
    (void *) PCI_IO_ADRS,
    (void *) PCI_IO_ADRS,
    0x02000000,    /* 32 Mbytes */
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

#endif /* INCLUDE_PCI */

    /* a32 space (some) */
    {
    (void *) 0x40000000,
    (void *) 0x40000000,
    0x02000000,    /* 32 Mbytes */
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* a32 space (some) */
    {
    (void *) 0xc0000000,
    (void *) 0xc0000000,
    0x02000000,    /* 32 Mbytes */
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* system EPROM */
    {
    (void *) ROM_BASE_ADRS,
    (void *) ROM_BASE_ADRS,
    ROM_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE_NOT  | VM_STATE_CACHEABLE_NOT
    },

    /* Flash ROM 0: Lower 512K is read-only, declared readable for Iid */
    {
    (void *) HKBAJA_FLASH0,
    (void *) HKBAJA_FLASH0,
    HKBAJA_FLASH0_RO_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* Flash ROM 0: Upper 512K is read/write, could be cacheable */
    {
    (void *) (HKBAJA_FLASH0+HKBAJA_FLASH0_RO_SIZE),
    (void *) (HKBAJA_FLASH0+HKBAJA_FLASH0_RO_SIZE),
    I28F008_SIZE-HKBAJA_FLASH0_RO_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* Flash ROM 1 (optional), could be cacheable */
    {
    (void *) HKBAJA_FLASH1,
    (void *) HKBAJA_FLASH1,
    I28F008_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* Flash ROM 2 (optional), could be cacheable */
    {
    (void *) HKBAJA_FLASH2,
    (void *) HKBAJA_FLASH2,
    I28F008_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* Flash ROM 3 (optional), could be cacheable */
    {
    (void *) HKBAJA_FLASH3,
    (void *) HKBAJA_FLASH3,
    I28F008_SIZE,
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* a24 space */
    {
    (void *) HKBAJA_VME_STANDARD_ADRS,
    (void *) HKBAJA_VME_STANDARD_ADRS,
    0x1000000,    /* 16 Mbytes */
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* local I/O */
    {
    (void *) 0xff000000,
    (void *) 0xff000000,
    0x1000000,    /* 16 Mbytes */
    VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID      | VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },
    };

int     sysPhysMemDescNumEnt = NELEMENTS (sysPhysMemDesc);

#endif /* (CPU == R4000) */

int     sysBus      = BUS;              /* system bus type (VME_BUS, etc) */
int     sysCpu      = CPU;              /* system CPU type (MC680x0) */
char *  sysBootLine = BOOT_LINE_ADRS;   /* address of boot line */
char *  sysExcMsg   = EXC_MSG_ADRS;     /* catastrophic message area */
int     sysFlags;                       /* boot flags */
char    sysBootHost [BOOT_FIELD_LEN];   /* name of host from which we booted */
char    sysBootFile [BOOT_FIELD_LEN];   /* name of file from which we booted */
ULONG   sysFailCount;                   /* VME /SYSFAIL count */
ULONG   sysACFAILCount;                 /* VME /ACFAIL count */
ULONG   sysWritePostFailCount;          /* VME write post count */
ULONG   sysArbTimeoutCount;             /* VME arbitration timeout count */
ULONG   sysMemParityCount;              /* parity error count */
UINT8   eiEnetAddr[PHYS_ADDR_SIZE];     /* Ethernet address for interface */
/* char *  ramBoard = (char *)0x60000000; */

/* locals */

LOCAL int           sysProcNum;         /* processor number */
LOCAL UINT8         sysLED  = HKBAJA_LED_POWERUP & HKBAJA_LED;
#if FALSE /* the switch, rtc and watchdog are unsupported for ECI */
LOCAL VOIDFUNCPTR   handleSwitchInt;    /* front panel switch intr function */
LOCAL VOIDFUNCPTR   handleRtcAlarmInt;  /* rtc alarm interrupt function */
LOCAL VOIDFUNCPTR   handleRtcDogInt;    /* rtc watchdog interrupt function */
#endif /* FALSE */
LOCAL LIST *        pSysPciList;        /* list of PCI devices */
LOCAL SEM_ID        sysPci9060SyncSem;  /* PCI9060 DMA semaphore */
LOCAL BOOL          sysPci9060DmaStart = FALSE;
                                        /* TRUE when PCI9060 DMA in progress */
LOCAL STATUS        sysPci9060DmaStatus;

TY_CO_DEV           tyCoDv [NUM_TTY];   /* tyCo port descriptors */

/* forward declarations */

/* included driver libraries */

#include "vme/vicVme.c"       /* VMEbus Interface Controller (VIC) routines */
#include "rtc/ds1286Rtc.c"    /* realtime clock access routines */
#include "timer/hkpigTimer.c" /* Heurikon's PIG chip timer routines */
#include "mem/x24c16Mem.c"    /* NVRAM access routines */
#ifdef INCLUDE_PCI
#include "pci/intPci.c"       /* general PCI interrupt routines */
#endif /* INCLUDE_PCI */

/*******************************************************************************
*
* sysModel - return the model name of this processor board
*
* This routine returns the model name of this processor board.
*
* RETURNS: A pointer to the string "Heurikon Baja40" or
*          "Heurikon Baja60" or "Heurikon Baja4700".
*/

char * sysModel (void)

    {
#if (CPU == MC68LC040)
    return ("Heurikon Baja40LC");
#endif
#if (CPU == MC68040)
    return ("Heurikon Baja40");
#endif
#if (CPU == MC68060)
    return ("Heurikon Baja60");
#endif
#if (CPU == R4000)
    return ("Heurikon Baja4700");
#endif
    }

/*******************************************************************************
*
* sysLEDSet - set the LEDs
*
* This routine sets or clears the LEDs on the board.
* The parameter `mask' determines which bits will be changed, and `value'
* determines the value to which those bits will be set.
* In other words, 
* .CS
* newLED = (oldLED & ~mask) | (value & mask).
* .CE
* The LEDs are set to the least significant bits of newLED.
* Note that sysLEDGet () is equivalent to sysLEDSet (0, 0);
*
* RETURNS: The previous value of the LED bits.
*/

UINT8 sysLEDSet
    (
    UINT8 mask,            /* selects which bits will be changed */
    UINT8 value            /* holds the new value for selected bits */
    )

    {
    FAST UINT8 oldLED;

    oldLED = sysLED;
    sysLED = ((sysLED & ~mask) | (value & mask)) & HKBAJA_LED;

    *HKBAJA_LED_ADRS = ~sysLED;

    return (oldLED);
    }

/*******************************************************************************
*
* sysLEDHex - set the LEDs to display a hex-digit
*
* RETURNS: The previous value of the LED bits.
*/

UINT8 sysLEDHex
    (
    ULONG value            /* holds the hexidecimal value for LED display */
    )

    {
    LOCAL UINT8 sysLEDPat [0x10] =
        {
        0x41, 0xf3, 0x89, 0xa1, 0x33, 0x25, 0x05, 0xf1,
        0x01, 0x31, 0x11, 0x07, 0x4d, 0x83, 0x0d, 0x1d,
        };
    return (sysLEDSet (HKBAJA_LED, ~sysLEDPat [value % 0x10]));
    }

/*******************************************************************************
*
* sysLEDInt - cycles the LEDs to display an integer in hex format
*
* A number starts and ends with a flashing decimal point of the
* 7 segment display so that integers displayed in sequence have a
* separator.
*
* This function uses software delays.  It may need tuning but it does not
* use up any timers and may be used when interrupts are turned off.
*
* RETURNS: N/A
*/

void sysLEDInt
    (
    ULONG value            /* holds the value to be displayed on LEDs */
    )

    {
    UINT8 save = sysLEDSet (0x0, 0x0); /* save */
    UINT8 idx;
#if ((CPU == MC68040) || (CPU == MC68LC040))
    UINT32 delayFactor = 1; /* this may need tuning */
#elif (CPU == MC68060)
    UINT32 delayFactor = 3; /* this may need tuning */
#else
    UINT32 delayFactor = 20; /* this may need tuning */
#endif /* (CPU == MC68040) */
    UINT32 shortDelay = 0x200000 * delayFactor;
    UINT32 longDelay =  0x800000 * delayFactor;
    int i, dummy;
    int print = FALSE;

    (void) sysLEDSet (HKBAJA_LED, 0x01);
    for(i=0; i<shortDelay; i++) dummy = i;

    for (idx = 0x0 ; idx <= 0x7 ; idx ++)
        {
        if ((print || ((value >> (28 - 4 * idx)) & 0xf)) || (idx == 7))
            {
            print = TRUE;
            (void) sysLEDSet (HKBAJA_LED, HKBAJA_LED_OFF); /* blank */
            for(i=0; i<shortDelay; i++) dummy = i;
            (void) sysLEDHex ( (value >> (28 - 4 * idx)) & 0xf ); /* digit */
            for(i=0; i<longDelay; i++) dummy = i;
            }
        }
    (void) sysLEDSet (HKBAJA_LED, 0x01);
    for(i=0; i<shortDelay; i++) dummy = i;

    (void) sysLEDSet (HKBAJA_LED, save); /* restore */
    }

/*******************************************************************************
*
* sysMemParityIntClear - clear a memory parity error interrupt
*
* This routine clears a parity error condition.  Parity errors are
* usually the result of accessing uninitialized memory locations.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void sysMemParityIntClear (void)

    {
#if (CPU == R4000)
    register UINT32 reg; /* the reads must not go to memory during this time */

    reg = *(volatile UINT32 *)HKBAJA_SCID_CONTROL_1;
    reg = *(volatile UINT32 *)HKBAJA_SCID_CONTROL_1;
    reg = *(volatile UINT32 *)HKBAJA_SCID_CONTROL_1;
    reg = *(volatile UINT32 *)HKBAJA_SCID_CONTROL_0;
    reg = *(volatile UINT32 *)HKBAJA_SCID_CONTROL_0;
    reg = *(volatile UINT32 *)HKBAJA_SCID_CONTROL_1;
    reg = *(volatile UINT32 *)HKBAJA_SCID_CONTROL_0;
    reg = *(volatile UINT32 *)HKBAJA_SCID_CONTROL_1;
    reg = *(volatile UINT32 *)HKBAJA_SCID_CONTROL_0;

#else /* (CPU == R4000) */

    *(volatile UINT32 *)HKBAJA_SCID_CONTROL_1 = 0;
    *(volatile UINT32 *)HKBAJA_SCID_CONTROL_1 = 0;
    *(volatile UINT32 *)HKBAJA_SCID_CONTROL_1 = 0;
    *(volatile UINT32 *)HKBAJA_SCID_CONTROL_0 = 0;
    *(volatile UINT32 *)HKBAJA_SCID_CONTROL_0 = 0;
    *(volatile UINT32 *)HKBAJA_SCID_CONTROL_1 = 0;
    *(volatile UINT32 *)HKBAJA_SCID_CONTROL_0 = 0;
    *(volatile UINT32 *)HKBAJA_SCID_CONTROL_1 = 0;
    *(volatile UINT32 *)HKBAJA_SCID_CONTROL_0 = 0;

#endif /* (CPU == R4000) */
    }

/*******************************************************************************
*
* sysMemParityInt - handle a memory parity error interrupt
*
* This routine handles a memory parity error on the Baja.
* It logs a message and clears the condition.  Parity errors are
* usually the result of accessing uninitialized memory locations.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void sysMemParityInt (void)

    {
    sysMemParityIntClear ();

    sysMemParityCount++;

    /* tell the console */

    logMsg ("Memory parity error\n",0,0,0,0,0,0);
    }

#if FALSE /* the switch, rtc and watchdog are unsupported for ECI */

/******************************************************************************
*
* sysRtcAlarmConnect - connect an interrupt handler to the RTC alarm
*
* This function connects a routine to handle alarm interrupts from
* the real time clock.  Alarm interrupts can be set to occur on
* specific times, or after certain time intervals (e.g., every minute).
*
* RETURNS: OK
*/

STATUS sysRtcAlarmConnect
    (
    VOIDFUNCPTR routine,    /* associated routine */
    int argument            /* argument unused */
    )

    {
    handleRtcAlarmInt = routine;
    return (OK);
    }

/******************************************************************************
*
* sysRtcDogConnect - connect an interrupt handler to the RTC watchdog
*
* This function connects a routine to handle watchdog interrupts from
* the real time clock.  Watchdog interrupts can be set to occur
* whenever software does not access the realtime clock within a
* certain time period. A watchdog countdown is aborted by calling the
* routine ds1286DogCancel() before the watchdog has counted to zero.
*
* RETURNS: OK
*/

STATUS sysRtcDogConnect
    (
    VOIDFUNCPTR routine,    /* associated routine */
    int argument            /* argument unused */
    )

    {
    handleRtcDogInt = routine;
    return (OK);
    }

/******************************************************************************
*
* sysSwitchConnect - connect an interrupt handler to the switch interrupt
*
* This function connects a routine to handle interrupt switch events.
*
* If the switch interrupt is pressed, the routine connected
* with this switch handler runs immediately. Switch "bounce" may
* cause several interrupts to occur for a single switch press. Since
* bouncing may or may not be a problem depending on what the connected
* routine does, the connected routine should debounce the switch as
* necessary. This can be done by ignoring multiple interrupts within a
* certain time interval. For example, the following routine switchTest()
* counts the number of times the switch has been pressed on the
* numeric LED, but ignores multiple switch presses within 60 ticks:
*
* .CS
* void switchTest
*    (
*    int unused
*    )
*
*    {
*    LOCAL ULONG count;
*    LOCAL ULONG tickNow;
* 
*    if (tickNow + 60 <= tickGet())
*         {
*         sysLEDHex (++count);
*         tickNow = tickGet();
*         }
*    }
* .CE
*
* RETURNS: OK
*/

STATUS sysSwitchConnect
    (
    VOIDFUNCPTR routine,    /* associated routine */
    int argument            /* argument unused */
    )

    {
    handleSwitchInt = routine;
    return (OK);
    }

/******************************************************************************
*
* sysRtcSwitchIntEnable - enable RTC/Switch interrupts
*
* This routine enables interrupts from the front panel switch,
* realtime clock watchdog, and realtime clock alarm.  All of these
* interrupt sources are attached to a single interrupt line at the
* VIC64 chip.
*
* RETURNS: N/A
*/

void sysRtcSwitchIntEnable (void)

    {
    /* clear and enable switch */

    *(UINT32 *)HKBAJA_SWITCH_CLEAR = 1;
    *(UINT32 *)HKBAJA_SWITCH_CLEAR = 0;

    /* enable the interrupt at the VIC chip */

    *VIC_LICR6 &= ~LICR_IRQ_DISABLE;
    }

/******************************************************************************
*
* sysRtcSwitchIntDisable - disable RTC/Switch interrupts
*
* This routine disables interrupts from the front panel switch,
* realtime clock watchdog, and realtime clock alarm.  All of these
* interrupt sources are attached to a single interrupt line at the
* VIC64 chip.
*
* RETURNS: N/A
*/

void sysRtcSwitchIntDisable (void)

    {
    /* disable the interrupt at the VIC chip */

    *VIC_LICR6 |= LICR_IRQ_DISABLE;
    }

/******************************************************************************
*
* sysSwitchPoll - poll the interrupt switch state
*
* This routine polls the state of the interrupt switch.  The switch
* state is latched, however, so it must be cleared once it has
* gone active. If clearFlag is FALSE, this routine just looks at the
* latched switch state.  If clearFlag is TRUE, the latch is cleared
* first so that the returned value reflects the state of the switch
* right now.
*
* RETURNS: TRUE if switch is depressed, FALSE if switch is not depressed.
*/

BOOL sysSwitchPoll
    (
    BOOL clearFlag
    )

    {
    if (clearFlag)
        {
        *(UINT32 *)HKBAJA_SWITCH_CLEAR = 1; /* clear latch */
        *(UINT32 *)HKBAJA_SWITCH_CLEAR = 0; /* enable switch read */
        }

    return ((*((volatile UINT8 *)HKBAJA_SWITCH_ADDR) & HKBAJA_SWITCH_MASK)
            ? TRUE : FALSE);
    }

/******************************************************************************
*
* sysRtcSwitchIntDispatch - dispatch RTC/Switch interrupts
*
* This function demultiplexes the realtime clock interrupts and
* the front-panel "interrupt" switch. The realtime clock
* interrupts could be caused by watchdog or alarm interrupts.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sysRtcSwitchIntDispatch
    (
    int argument
    )

    {
    FAST int switchInt;
    FAST int rtcAlarmInt;
    FAST int rtcDogInt; 

    /* see which interrupts we have */

    while (1)
        {
    	switchInt = sysSwitchPoll (FALSE);
        rtcAlarmInt = *((volatile UINT8 *)DS1286_CR) & DS1286_CR_TDF;
        rtcDogInt = *((volatile UINT8 *)DS1286_CR) & DS1286_CR_WAF;

        /* get out if we have no interrupts */

        if (!(switchInt | rtcAlarmInt | rtcDogInt))
            break;

        /* if we have appropriate handlers, run them */

        if ((handleRtcDogInt != (VOIDFUNCPTR) NULL) && rtcDogInt)
            {
            /* handle rtc watchdog interrupt */
            (*handleRtcDogInt)(argument);

            /* reset the watchdog, so we can get going again */
            (void) ds1286DogCancel();
            }

        if ((handleRtcAlarmInt != (VOIDFUNCPTR) NULL) && rtcAlarmInt)
            {
            /* handle rtc alarm interrupt */
            (*handleRtcAlarmInt)(argument);
            }

        if (switchInt)
            {
            if (handleSwitchInt != (VOIDFUNCPTR) NULL)
                {
                (*handleSwitchInt)(argument); /* debounce switch in here */
                }

            /* wait until switch is released */

            while (  sysSwitchPoll(TRUE) );
            }
        }
    }

#endif /* FALSE */

/*******************************************************************************
*
* sysX24c16ClockLo - drive the Xicor x24c16 clock line low
*
* This routine drives the clock line of the Xicor x24c16 serial
* EEPROM low.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sysX24c16ClockLo (void)

    {
    *X24C16_CLOCK_ADDR = 0;
    }

/*******************************************************************************
*
* sysX24c16ClockHi - float the Xicor x24c16 clock line high
*
* This routine floats the clock line of the Xicor x24c16 serial
* EEPROM high.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sysX24c16ClockHi (void)

    {
    *X24C16_CLOCK_ADDR = 1;

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */
    }

/*******************************************************************************
*
* sysX24c16DataLo - drive the Xicor x24c16 data line low
*
* This routine drives the data line of the Xicor x24c16 serial
* EEPROM low.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sysX24c16DataLo (void)

    {
    *X24C16_DATA_ADDR = 0;

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */
    }

/*******************************************************************************
*
* sysX24c16DataHi - float the Xicor x24c16 data line high
*
* This routine floats the data line of the Xicor x24c16 serial
* EEPROM high.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sysX24c16DataHi (void)

    {
    *X24C16_DATA_ADDR = 1;

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */
    }

/*******************************************************************************
*
* sysX24c16ReadBit - read a bit from the Xicor x24c16
*
* This routine reads a single bit from the Xicor x24c16 output.
*
* RETURNS: The bit read from the x24c16
*
* NOMANUAL
*/

BOOL sysX24c16ReadBit(void)

    {
    return (*X24C16_OUTPUT_ADDR & 1);
    }

/*******************************************************************************
*
* sysPigInt - dispatch proper interrupt handler
*
* This function services the interrupt of the CPU which is connected
* to the interrupt output of the PIG chip.  PIG serial ports and
* and counter timers generate interrupts which activate
* this routine.  This routine calls sysPigIntDispatch().
*
* RETURNS: N/A
*
* SEE ALSO:
* sysHwInit2()
*
* NOMANUAL
*/

void sysPigInt(void)

    {
    sysPigIntDispatch();

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */
    }

#ifdef INCLUDE_PCI

/*******************************************************************************
*
* sysPci9060BusErrorClear - clear the bus error at the PCI9060
*
* The PCI9060 bridge does not allow cycles to PCI after the CPU
* causes a PCI bus error.  This routine clears that condition.
*
* RETURNS: OK
*/

void sysPci9060BusErrorClear (void)

    {
    *PCI_STATUS (PCI9060) = X16 (PCI_STAT_PARITY_DETECTED |
                                 PCI_STAT_REC_MASTER_ABORT |
                                 PCI_STAT_REC_TARGET_ABORT);
#if (CPU == R4000)
    sysWbFlush ();
#endif /* (CPU == R4000) */
    }

/*******************************************************************************
*
* sysPci9060DmaCopy - DMA copy between local memory and PCI memory space
*
* This routine uses DMA channel 0 of the PCI9060 to copy between
* local memory and PCI memory space.
*
* WARNING:
* The PCI9060 has a bug that makes direct master aborts (an abort is
* equivalent to a bus error) and DMA aborts indistinguishable.  For
* this reason, if _any_ abort occurs during a DMA transfer, the cause
* is assumed to be the DMA and this function returns ERROR.  This puts
* extra burden on the user to guarantee he will not bus error to PCI
* during a PCI9060 DMA transfer.
*
* RETURNS: OK, or ERROR if the transfer was unsuccessful.  Unsuccessful
* transfers result from bus errors or bad alignments.
*/

STATUS sysPci9060DmaCopy
    (
    char * localAdrs,    /* local DRAM address */
    char * pciAdrs,      /* PCI memory space address */
    int    nbytes,       /* transfer length in bytes */
    BOOL   toPci         /* copy local memory to PCI (or vise-versa) */
    )

    {
    UINT32 physLocalAdrs;
    int    level;

    physLocalAdrs = (UINT32)CACHE_DMA_VIRT_TO_PHYS (localAdrs);

    /* calculations formed on 0-based RAM (this makes life easy) */

    if ((physLocalAdrs + nbytes) >= (UINT32)sysMemTop ())
        return (ERROR);

    /* the DMA can transfer up to 0x3fffff bytes */

    if (nbytes > 0x003fffff)
        return (ERROR);

    /* set the starting values */

    *PCI9060_CH0_LOCAL_ADRS (PCI9060) = X32 (physLocalAdrs);
    *PCI9060_CH0_PCI_ADRS (PCI9060) = X32 (pciAdrs);
    *PCI9060_CH0_COUNT (PCI9060) = X32 (nbytes);

    /* set the direction */
    
    if (toPci)
        *PCI9060_CH0_DESC_PTR (PCI9060) = X32 (BIT3);
    else
        *PCI9060_CH0_DESC_PTR (PCI9060) = X32 (0);

    level = intLock ();

    /* start the transfer */

    *PCI9060_DMA_COMSTAT (PCI9060) = X32 (BIT1 | BIT0);

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */

    sysPci9060DmaStart = TRUE;

    intUnlock (level);

    semTake (sysPci9060SyncSem, WAIT_FOREVER);

    return (sysPci9060DmaStatus);
    }

/*******************************************************************************
*
* sysPci9060Int - interrupt handler for PCI9060
*
* This routine handles CH0 DMA interrupts and LSERR interrupts.
*
* RETURNS: N/A
*/

LOCAL void sysPci9060Int (void)
    {
    if (sysPci9060DmaStart)
        {
        if ((X16 (*PCI_STATUS (PCI9060)) & 
             (PCI_STAT_REC_MASTER_ABORT | 
              PCI_STAT_REC_TARGET_ABORT)) != 0)
            {
            /* Due to a PLX bug, there is no way to distinguish between
             * a DMA abort and direct master abort.  For this reason,
             * we have to always assume the DMA caused the abort and
             * cancel the transfer.
             */

            *PCI9060_DMA_COMSTAT (PCI9060) = X32 (0);
            *PCI9060_DMA_COMSTAT (PCI9060) = X32 (BIT2);

            /* clear the abort now */

            sysPci9060BusErrorClear ();

            sysPci9060DmaStatus = ERROR;
            }
        else
            sysPci9060DmaStatus = OK;

        /* clear the channel 0 interrupt */

        *PCI9060_DMA_COMSTAT (PCI9060) = X32 (BIT3);

        /* clear the flag */
  
        sysPci9060DmaStart = FALSE;

        /* wake up sysPci9060DmaCopy() */

        semGive (sysPci9060SyncSem);
        }
    else
        sysPci9060BusErrorClear ();

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */
    }

/*******************************************************************************
*
* sysPciMap - enable all devices into PCI memory or I/O space
*
* For PCI memory or I/O space, this routine traverses the list
* pointed to by <pSysPciList> and enables the each device into the
* space (memory or I/O).  The devices using the most address space
* are mapped first so that there are no gaps in the memory map.
*
* RETURNS: OK
*
* NOMANUAL
*/

STATUS sysPciMap
    (
    BOOL memSpace    /* TRUE for PCI memory, FALSE for PCI I/O */
    )

    {
    UINT32       i;
    UINT32       bit;
    UINT32       bitMap;
    char *       pLocalAdrs;
    UINT32       pciAdrs;
    BOOL         allMapped = FALSE;
    PCI_CONFIG * pPci;
    PCI_CONFIG * pPciMaxSize = (PCI_CONFIG *)NULL;
    UINT32       baseAdrs;
    UINT32       maxSize;
    UINT32       size;
    UINT32       maxSizeIndex = 0;

    /*
     * Brute force memory mapping algorithm:
     * For PCI memory base addresses, traverse the list and map the
     * addresses that require the most space first.  Assume 32 bit
     * PCI addresses for now.
     */

    if (memSpace)
        pLocalAdrs = (char *)PCI_MEM_ADRS;
    else
        pLocalAdrs = (char *)PCI_IO_ADRS;
    pciAdrs = PCI_REMAP_ADRS;

    while (!allMapped)
        {
        pPci = (PCI_CONFIG *)lstFirst (pSysPciList);

        maxSize = 0;

        while (pPci != (PCI_CONFIG *)NULL)
            {
            for (i = 0, bit = 1; i <= PCI_MAX_BASE_ADRS; i++, bit <<= 1)
                {
                baseAdrs = (UINT32)pPci->pPciConfigDesc->pBaseAdrs [i];
                bitMap = (UINT32)pPci->bitMap;
                if ((baseAdrs != 0) &&
                    (((baseAdrs & PCI_BASE_IO_SPACE_INDICATOR) == 0)
                     == memSpace) &&
                    ((bitMap & bit) == 0))
                    {
                    size = ~(baseAdrs & PCI_BASE_MEM_ADRS_MASK) + 1;
                    if (size > maxSize)
                        {
                        maxSize = size;
                        pPciMaxSize = pPci;
                        maxSizeIndex = i;
                        }
                    }
                }
            pPci = (PCI_CONFIG *)lstNext ((NODE *)pPci);
            }

        if (maxSize == 0)
            allMapped = TRUE;
        else
            {
            *PCI9060_LTP_IO_CONFIG (PCI9060) = X32 (BIT31 |
                                                    pPciMaxSize->configAdrs |
                                                    (PCI_RN_BASE_ADRS0 +
                                                     (maxSizeIndex << 2)));
            *(volatile UINT32 *)PCI_IO_ADRS = X32 (pciAdrs);

            pPciMaxSize->pPciConfigDesc->pBaseAdrs [maxSizeIndex] = pLocalAdrs;
            pPciMaxSize->bitMap |= 1 << maxSizeIndex;

            pLocalAdrs += maxSize;
            pciAdrs += maxSize;
            }
        }

    if (pciAdrs > (PCI_REMAP_ADRS + PCI_SIZE))
        return (ERROR);

    return (OK);
    }

/*******************************************************************************
*
* sysPciInit - initialize the PCI bridge and map the PCI devices
*
* This routine starts by initialing the local bus to PCI bridge.
* The bridge functions by decoding a portion of the local address bus
* and PCI address bus.  Local cycles within the address range 0x20000000
* and 0x40000000 are routed through the bridge to the PCI bus.  PCI memory
* space cycles within the address range 0x00000000 and 0x04000000 are routed
* through the bridge to local RAM.  This function is illustrated below.
*
* .CS
*               local                  PCI Memory                PCI I/O   
*               space                    space                    space
*
* 0x40000000 +---------+   0x40000000 +---------+   0x30000000 +---------+
*            |   PCI   |              |/////////|              |   PCI   |
*            |   I/O   |====bridge====|/=/=/=/=/|====bridge===>|   I/O   |
*            |  image  |              |/////////|              |         |
* 0x30000000 +---------+   0x30000000 +---------+   0x20000000 +---------+
*            |   PCI   |              |         |              |/////////|
*            |  memory |====bridge===>|         |              |/////////|
*            |  image  |              |         |              |/////////|
* 0x20000000 +---------+   0x20000000 +---------+   0x10000000 +---------+
*            |         |              |/////////|              |/////////|
*            |         |              |/////////|              |/////////|
*            |         |              |/////////|              |/////////|
*            |         |              |/////////|   0x00000000 +---------+
*            |         |              |/////////|
*            |         |              |/////////|
* 0x04000000 +---------+   0x04000000 +---------+
*            |local RAM|<===bridge====|RAM image|
* 0x00000000 +---------+   0x00000000 +---------+
* .CE
*
* After initializing the bridge, this routine seaches for PCI devices
* and maps them so they may be accessed locally.  Each instance of
* a device is stored in the list pointed to by <pSysPciList>.
*
* NOTE: This routine is called by sysHwInit2.
*
* RETURNS: OK
*
* SEE ALSO:
* sysPciShow()
* sysHwInit2()
*
* NOMANUAL
*/

STATUS sysPciInit (void)

    {
    STATUS       status = OK;
    UINT32       i;
    UINT32       reg;
    UINT32       configAdrs;
    UINT32       ramBaseAdrs;
    UINT32       romSize;
    UINT8        headerType;
    UINT8        function;
    UINT8        devNumber;
    UINT16       romSignature;
    char *       pRom;
    PCI_CONFIG * pPci;

    /* map PCI memory and I/O space into local space */

    *PCI9060_LTP_RANGE (PCI9060) =    X32 (~(PCI_SIZE - 1));  /* 256 MB */
    *PCI9060_LTP_MEM_BASE (PCI9060) = X32 (PCI_MEM_ADRS);
    *PCI9060_LTP_IO_BASE (PCI9060) =  X32 (PCI_IO_ADRS);
    *PCI9060_LTP_REMAP (PCI9060) =    X32 (PCI_REMAP_ADRS | BIT2 | BIT1 | BIT0);
    *PCI_COMMAND (PCI9060) =          X16 (PCI_CMD_IO_ENABLE |
                                           PCI_CMD_MEM_ENABLE |
                                           PCI_CMD_MASTER_ENABLE |
                                           PCI_CMD_PARITY_ERR_ENABLE |
                                           PCI_CMD_SERR_ENABLE);
 
    /* map local RAM into PCI memory space */

    *PCI9060_PTL_RANGE (PCI9060) = X32 (0xfc000000 | BIT3); /* 64 MB */
    ramBaseAdrs = (UINT32) CACHE_DMA_VIRT_TO_PHYS (LOCAL_MEM_LOCAL_ADRS);
    *PCI9060_PTL_BASE (PCI9060) =  X32 (ramBaseAdrs | BIT3);
    *PCI9060_PTL_REMAP (PCI9060) = X32 (ramBaseAdrs | BIT0);
#if (CPU == R4000)
    *PCI9060_PTL_DESC (PCI9060) =  X32 (X32 (*PCI9060_PTL_DESC (PCI9060)) |
                                        BIT28 | BIT27 | BIT24 | BIT7 | BIT6);
#else /* CPU == R4000 */
    /* This is a temporary workaround for the Baja68k which has some
     * problems with PCI9060 to RAM burst tranfers.   Bit 7 is cleared
     * so the Baja68k transfers 4 LWORD bursts only.
     * Thu Apr 25 16:46:58 CDT 1996 - tjf
     */

    *PCI9060_PTL_DESC (PCI9060) =  X32 (X32 (*PCI9060_PTL_DESC (PCI9060)) |
                                        BIT28 | BIT27 | BIT24 | BIT6);
#endif /* CPU == R4000 */

    /* Enable local BREQo.  Direct slave delay clocks are necessary so
     * that locally backed-off reads do not cause a phantom read on
     * PCI - this causes interrupts to be cleared on the NCR 53c825!
     * Jeff Durst helped with this fix (01a,01sep95,tjf).
     */

    *PCI9060_PTL_ROM_REMAP (PCI9060) = X32 (BIT4 | 0xf);

    /* turn on bus errors and CH0 interrupts */

    *PCI9060_INT_CTRL (PCI9060) = X32 (BIT18 | BIT16 | BIT0);

    /* The next line changes the priority of the PCI arbiter.  Clearing
     * bit 16 gives the CPU the highest priority.  With busy PCI, if
     * the CPU has lowest priority (bit 16 set) it may timeout and bus
     * error trying to access the PCI bus.
     */

    *PCI9060_GEN_CTRL (PCI9060) = X32 (X32 (*PCI9060_GEN_CTRL (PCI9060)) &
                                            ~BIT16);

    /* set up the DMA */

    *PCI9060_CH0_MODE (PCI9060) = X32 (BIT10 | BIT8 | BIT7 |
                                       BIT6 | BIT1 | BIT0);

    sysPci9060SyncSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY);

    /* enable the interrupt */

    intConnect ((VOIDFUNCPTR *)INUM_TO_IVEC (HKBAJA_VEC_PLX),
                sysPci9060Int, 0);

    *VIC_LICR5 &= ~LICR_IRQ_DISABLE; 

    /* create the list */

    pSysPciList = (LIST *)malloc (sizeof (LIST));
    lstInit (pSysPciList);

    /*
     * Perform a type 0 query.
     * A type 1 query _should_ be performed (but is not) if a PCI-to-PCI
     * bridge is detected.  The feature is not implemented at this time.
     * Notes: What bus numbers need to be queried when we find a bridge?
     * All of them?   (01a,01sep95,tjf)
     */
 
    for (devNumber = 0; devNumber < 2; devNumber++)
        {
        for (function = 0; function < 8; function++)
            {
            configAdrs = (devNumber << 11) | (function << 8);

            *PCI9060_LTP_IO_CONFIG (PCI9060) = X32 (BIT31 | configAdrs |
                                                    PCI_RN_ID);

            if (vxMemProbe ((char *)PCI_IO_ADRS, VX_READ, 4, (char *)&reg)
                == ERROR)
                break;

            reg = X32 (reg);

            pPci = (PCI_CONFIG *)malloc (sizeof (PCI_CONFIG));

            pPci->pPciConfigDesc = 
                           (PCI_CONFIG_DESC *)malloc (sizeof (PCI_CONFIG_DESC));

            lstAdd (pSysPciList, (NODE *)pPci);

            pPci->configAdrs = configAdrs;
            pPci->used = FALSE;
            pPci->bitMap = 0;

            pPci->pPciConfigDesc->pciMemRamBaseAdrs = (char *)0x00000000;
            pPci->pPciConfigDesc->vendorId = (UINT16)(reg & 0xffff);
            pPci->pPciConfigDesc->deviceId = (UINT16)((reg >> 16) & 0xffff);
 
            *PCI9060_LTP_IO_CONFIG (PCI9060) = X32 (BIT31 | configAdrs |
                                                    PCI_RN_REV_CLASS);

            reg = X32 (*(volatile UINT32 *)PCI_IO_ADRS);

            pPci->pPciConfigDesc->revisionId = (UINT8)(reg & PCI_REV_MASK);

            pPci->pPciConfigDesc->function = function;

            switch (devNumber)
                {
                case 0:
                    pPci->pPciConfigDesc->vector =
                                 (VOIDFUNCPTR *)INUM_TO_IVEC (HKBAJA_VEC_PCI1);
                break;
                case 1:
                    pPci->pPciConfigDesc->vector =
                                 (VOIDFUNCPTR *)INUM_TO_IVEC (HKBAJA_VEC_PCI2);
                break;
                default:
                    /* error */
                break;
                }

            /* retrieve the size for each base address */

            for (i = 0; i <= PCI_MAX_BASE_ADRS; i++)
                {
                *PCI9060_LTP_IO_CONFIG (PCI9060) = X32 (BIT31 | configAdrs |
                                                        (PCI_RN_BASE_ADRS0 +
                                                         (i << 2)));
                *(volatile UINT32 *)PCI_IO_ADRS = X32 (0xffffffff);
                pPci->pPciConfigDesc->pBaseAdrs [i] = 
                                  (char *)X32 (*(volatile UINT32 *)PCI_IO_ADRS);
                }

            /* set the command register for this device */

            *PCI9060_LTP_IO_CONFIG (PCI9060) = X32 (BIT31 | configAdrs |
                                                    PCI_RN_CMD_STAT);

            *(volatile UINT32 *)PCI_IO_ADRS =
                                    X32 (X32 (*(volatile UINT32 *)PCI_IO_ADRS) |
                                         PCI_CMD_MASTER_ENABLE |
                                         PCI_CMD_MEM_ENABLE |
                                         PCI_CMD_IO_ENABLE |
                                         PCI_CMD_PARITY_ERR_ENABLE |
                                         PCI_CMD_SERR_ENABLE);

            /* Set the latency timer.  The latency timer may need to be
             * reduced in situations with two PCI devices where one gives
             * up the bus often and the other does not.  Reducing the 
             * latency timer will squelch PCI bus hogs.
             */

            *PCI9060_LTP_IO_CONFIG (PCI9060) = X32 (BIT31 | configAdrs |
                                                    PCI_RN_LATENCY_HEADER);

            reg = X32 (*(volatile UINT32 *)PCI_IO_ADRS);

            *(volatile UINT32 *)PCI_IO_ADRS = X32 ((reg & 0xffff00ff) |
                                                   PCI_LATENCY << 8);

            if (function == 0)
                {
                /* see if there is a valid expansion ROM */

                *PCI9060_LTP_IO_CONFIG (PCI9060) = X32 (BIT31 | configAdrs |
                                                        (PCI_RN_ROM_BASE_ADRS));

                *(volatile UINT32 *)PCI_IO_ADRS = X32 (PCI_BASE_ROM_ADRS_MASK);

                reg = X32 (*(volatile UINT32 *)PCI_IO_ADRS);

                if (reg != 0)
                    {
                    /* there is a ROM decoder, see if there is a valid ROM */

                    romSize = ~reg + 1;

                    *(volatile UINT32 *)PCI_IO_ADRS = 
                              X32 (PCI_REMAP_ADRS | PCI_BASE_ROM_DECODE_ENABLE);

                    if ((vxMemProbe ((char *)PCI_MEM_ADRS, VX_READ, 2, 
                                     (char *)&romSignature) == OK) && 
                                     (romSignature == 0x55AA))
                        {
                        /* there is a ROM, copy to RAM */

                        pRom = (char *)malloc (romSize);
                        bcopy ((char *)PCI_MEM_ADRS, pRom, romSize);
                        pPci->pPciConfigDesc->romBaseAdrs = pRom;
                        }

                    /* disable expansion ROM decode */

                    *(volatile UINT32 *)PCI_IO_ADRS = X32 (0);
                    }

                /* check for more more device functions */

                *PCI9060_LTP_IO_CONFIG (PCI9060) = X32 (BIT31 | configAdrs |
                                                        PCI_RN_LATENCY_HEADER);
                reg = X32 (*(volatile UINT32 *)PCI_IO_ADRS);

                headerType = (UINT8)((reg >> 16) & 0xff);

                if ((headerType & PCI_HEADER_TYPE_FUNC_MASK) ==
                     PCI_HEADER_TYPE_FUNC_SINGLE)
                    break;
                }
            }
        }

    /* map register files to PCI memory */

    if (sysPciMap (TRUE) != OK)
        status = ERROR;

    /* map register files to PCI I/O */
    
    if (sysPciMap (FALSE) != OK)
        status = ERROR;

    /* turn off config accesses */

    *PCI9060_LTP_IO_CONFIG (PCI9060) = X32 (0);

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */

    return (status);
    }

/*******************************************************************************
*
* sysPciShow - show where the PCI devices are mapped in local space
*
* For each PCI device, this routine shows the local address for each of
* the 6 PCI base addresses.  The vendor, device and revision IDs of the
* PCI device along with the local interrupt vector are also shown.
*
* RETURNS: OK
*/

STATUS sysPciShow (void)

    {
    int          i;
    PCI_CONFIG * pPci;

    printf ("VEND DEV  REV FNC INUM  BASE0    BASE1    BASE2    BASE3    BASE4    BASE5\n");
    printf ("---- ---- --- --- ---- -------- -------- -------- -------- -------- --------\n");

    pPci = (PCI_CONFIG *)lstFirst (pSysPciList);
    
    while (pPci != (PCI_CONFIG *)NULL)
        {
        printf ("%04x %04x %02x   %01d   %02x ", 
                pPci->pPciConfigDesc->vendorId, 
                pPci->pPciConfigDesc->deviceId,
                pPci->pPciConfigDesc->revisionId,
                pPci->pPciConfigDesc->function,
                (UINT8)IVEC_TO_INUM(pPci->pPciConfigDesc->vector));

        for (i = 0; i <= PCI_MAX_BASE_ADRS; i++)
            {
            if (pPci->pPciConfigDesc->pBaseAdrs [i] == (char *)NULL)
                printf (" ........");
            else
                printf (" %08x", (UINT32)pPci->pPciConfigDesc->pBaseAdrs [i]);
            }

        printf ("\n");

        pPci = (PCI_CONFIG *)lstNext ((NODE *)pPci);
        }

    return (OK);
    }

/******************************************************************************
*
* sysPciConfigShow - show configuration space
*
* This routine shows the configuration space of each PCI device that
* was located by sysPciInit().  This routine is for low-level diagnostic
* purposes only.
* 
* RETURNS: OK
*
* NOMANUAL
*/

STATUS sysPciConfigShow (void)

    {
    int          i;
    int          level;
    PCI_CONFIG * pPci;
    UINT32       reg;
    UINT32       configAdrs;

    pPci = (PCI_CONFIG *)lstFirst (pSysPciList);
    
    while (pPci != (PCI_CONFIG *)NULL)
        {
        printf ("\n");

        configAdrs = pPci->configAdrs;

        for (i = 0; i <= PCI_RN_INT_GNT_LAT; i +=4)
            {
            level = intLock ();

            *PCI9060_LTP_IO_CONFIG (PCI9060) = X32 (BIT31 | configAdrs | i);

            reg = X32 (*(volatile UINT32 *)PCI_IO_ADRS);

            *PCI9060_LTP_IO_CONFIG (PCI9060) = X32 (0);

#if (CPU == R4000)
            sysWbFlush();
#endif /* (CPU == R4000) */
        
            intUnlock (level);

            printf ("%02x: %08x\n", i, reg);
            }

        pPci = (PCI_CONFIG *)lstNext ((NODE *)pPci);
        }

    return (OK);
    }

/*******************************************************************************
*
* sysPciConfigGet - get a pointer to a configuration descriptor
*
* This  routine returns a pointer to the first free PCI_CONFIG_DESC
* configuration structure matching <vendorId>, <deviceId> and <function>,
* if the device and function exists.  If a pointer is returned, record
* keeping internal to this routine marks the descriptor from "free" to
* "used".
*
* Driver code should use this routine to establish the location of the
* register files, expansion ROM and VxWorks interrupt vector for the 
* device function.
*
* RETURNS: A pointer to PCI_CONFIG_DESC structure or NULL.
*/

PCI_CONFIG_DESC * sysPciConfigGet
    (
    UINT16 vendorId,
    UINT16 deviceId,
    UINT8  function
    )

    {
    PCI_CONFIG *  pPci;
 
    pPci = (PCI_CONFIG *)lstFirst (pSysPciList);
 
    while (pPci != (PCI_CONFIG *)NULL)
        {
        if ((pPci->pPciConfigDesc->vendorId == vendorId) && 
            (pPci->pPciConfigDesc->deviceId == deviceId) &&
            (pPci->pPciConfigDesc->function == function) &&
            (pPci->used == FALSE))
            {
            pPci->used = TRUE;
            return (pPci->pPciConfigDesc);
            }
 
        pPci = (PCI_CONFIG *)lstNext ((NODE *)pPci);
        }

    return ((PCI_CONFIG_DESC *)NULL);
    }

/*******************************************************************************
*
* sysPciIntEnable - enable a PCI interrupt
*
* This routine enables the PCI interrupt and the CPU corresponding to <vector>.
* This routine may be called more than once for a particular <vector>.
* However, the second and all subsequent calls are no-ops.
*
* RETURNS: OK or ERROR if vector does not correspond to a PCI vector.
*
* SEE ALSO: sysPciIntConnect()
*/

LOCAL STATUS sysPciIntEnable
    (
    VOIDFUNCPTR * vector
    )

    {
    switch ((UINT8)IVEC_TO_INUM(vector))
        {
        case HKBAJA_VEC_PCI1:
            *VIC_LICR4 &= ~LICR_IRQ_DISABLE;
            break;
        case HKBAJA_VEC_PCI2:
            *VIC_LICR3 &= ~LICR_IRQ_DISABLE;
            break;
        default:
            return (ERROR);
            break;
        }

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */

    return (OK);
    }

#endif /* INCLUDE_PCI */

/******************************************************************************
*
* sysMailboxBaseSet - set the base of the VIC chip's mailboxes in short space
*
* Set the VIC mailbox registers within VME short space.  They must be on a
* 256-byte boundary.
*
* RETURNS: OK, or ERROR if address is out of range for short space or is 
* not on a 256-byte boundary.
*
* SEE ALSO:
* sysMailboxConnect()
* sysMailboxEnable()
*/

STATUS sysMailboxBaseSet
    (
    UINT16 offset /* (short) block offset into short space */
    )
    {
    if (((ULONG)(offset) > 0xffff) || (offset & 0xff))
        return(ERROR);

    *(HKBAJA_MBOX_ADRS_PLACEMT) = (UINT8)((HKBAJA_MBOX_ADDR_BIAS+offset) >> 8);

    /* enable short space accesses */

    *(HKBAJA_VME_SHT_ENABLE) = 0;

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */

    return OK;
    }

/*******************************************************************************
*
* sysToMonitor - transfer execution to the ROM monitor
*
* This routine transfers control to the ROM monitor.  It is usually called
* only by the routine reboot, which services control-X, and bus errors at
* interrupt level.  In special circumstances, however, the user may wish
* to introduce a new startType such that a special bootrom facility would be
* enabled.
*
* RETURNS: Does not return.
*/

STATUS sysToMonitor
    (
    int startType  /* tells ROM how to boot, defined in h/sysLib.h */
    )

    {
    FUNCPTR pRom = (FUNCPTR)(ROM_TEXT_ADRS + 8);

    cacheFlush(INSTRUCTION_CACHE, 0, ENTIRE_CACHE);
    cacheFlush(DATA_CACHE, 0, ENTIRE_CACHE);

    /* disable VME slave (if enabled) */

    *(HKBAJA_VME_EXT_ENABLE) = 1;

    /* disable PCI interrupts */

    sysPciShutdown ();

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */

    (*pRom) (startType);

    return (OK);            /* in case we ever continue from ROM monitor */
    }

/*******************************************************************************
*
* sysMemTop - get the address of the top of memory
*
* This routine returns the address of the first missing byte of memory.
*
* INTERNAL
* This routine simply checks the PIG control register for the local RAM
* size.
*
* RETURNS: Address of the first missing byte of memory.
*/

char * sysMemTop (void)

    {
    LOCAL char * memTop;
    UINT32       ramSize;

    /* if we haven't sized memory yet */

    if (memTop == NULL )
        {
        ramSize = 0x00100000 << (PIG_RESET_CONFIG_MEM & *PIG_CONTROL);
 
        memTop = (char *)(LOCAL_MEM_LOCAL_ADRS + ramSize);
        }

    return (memTop);
    }

/*******************************************************************************
*
* sysACFAILInt - handle an ACFAIL error interrupt
*
* This routine handles interrupts that occur when the ACFAIL VME bus
* signal is asserted.  It keeps a count of the number
* of these interrupts.
*
* Occasionally, the ACFAIL signal can get asserted from noise on the bus,
* and cause these interrupts.  For a real ACFAIL however, the user could
* add additional code to this routine to do some small amount of
* processing before power is lost.
*
* RETURNS: N/A
*/
 
void sysACFAILInt (void)
 
    {
    sysACFAILCount++;
    }
 
/*******************************************************************************
*
* sysWritePostFailInt - handle a Write Post error interrupt
*
* This routine handles interrupts that occur as a result of a Write posted
* VME bus cycle that failed.  It keeps a count of the number
* of these interrupts.
*
* RETURNS: N/A
*/
 
void sysWritePostFailInt (void)
 
    {
    sysWritePostFailCount++;
    }
 
/*******************************************************************************
*
* sysArbTimeoutInt - handle an Arbitration Timeout interrupt
*
* This routine handles interrupts that occur as a result of a
* bus cycle that timed out.  It keeps a count of the number
* of these interrupts.
*
* RETURNS: N/A
*/
 
void sysArbTimeoutInt (void)
 
    {
    sysArbTimeoutCount++;
    }

/*******************************************************************************
*
* sysFailInt - handle a SYSFAIL interrupt
*
* This routine handles interrupts that occur when the SYSFAIL VME bus
* signal is asserted.  It keeps a count of the number
* of these interrupts and waits until SYSFAIL goes inactive before
* continuing.
*
* INTERNAL
* The value in sysFailCount may not be what you expect, since noise on the
* /SYSFAIL line may trigger this interrupt more times than you would
* normally notice.
*
* RETURNS: N/A
*/
 
void sysFailInt (void)
 
    {
    /* count the interrupt */
    sysFailCount++;
 
    /* poll until sysfail goes away */
    while ( sysFailControl(-1) );
    }
 
/*******************************************************************************
*
* sysFailWait - wait until SYSFAIL is released
*
* This routine watches the SYSFAIL signal and delays until SYSFAIL is
* released by all boards on the bus.
*
* An LED (the "." of the 7 segment display) is turned on while waiting, 
* and off afterwards.
*
* RETURNS: N/A
*
* SEE ALSO:
* sysLEDSet()
*/

LOCAL void sysFailWait (void)

    {
    /* turn on an LED while waiting */
    (void) sysLEDSet (0x01, HKBAJA_LED_ON);

    /* check SYSFAIL bit */
    while ( sysFailControl(-1) );

    /* turn off user LED */
    (void) sysLEDSet (0x01, HKBAJA_LED_OFF);
    }

/******************************************************************************
*
* sysHwInit2 - set up interrupts
*
* sysHwInit2() is called by sysClkConnect().  sysClkConnect() is the
* first board-specific routine that is called after the heap is setup
* and intConnect() may be called.  At this time sysHwInit2() is called
* and all required intConnects are performed.
*
* RETURNS: N/A
*
* SEE ALSO:
* sysClkConnect()
*
* NOMANUAL
*/

void sysHwInit2(void)

    {
#if (CPU != R4000)
    /* connect the parity error interrupt */

    intConnect ((VOIDFUNCPTR *)INUM_TO_IVEC (HKBAJA_VEC_PARITY_ERROR),
                sysMemParityInt, 0);
#endif /* CPU != R4000 */

    /* connect PIG interrupt */

    intConnect((VOIDFUNCPTR *)INUM_TO_IVEC (HKBAJA_VEC_PIG), sysPigInt, 0);

#if FALSE /* the switch, rtc and watchdog are unsupported for ECI */
    /* connect RTC/Switch interrupt */

    intConnect ((VOIDFUNCPTR *)INUM_TO_IVEC (HKBAJA_VEC_RTC_SWITCH),
                sysRtcSwitchIntDispatch, 0);
#endif /* FALSE */

    /*
     * NOTE: The Error Group Interrupt of the VIC is currently not enabled
     * by sysHwInit().  These error handlers, therefare, won't be activated.
     * They are included in case they are needed at some other time.
     */

    /* connect the VIC error handlers */

    intConnect (INUM_TO_IVEC(HKBAJA_VEC_ACFAIL), (VOIDFUNCPTR)sysACFAILInt, 0);
    intConnect (INUM_TO_IVEC(HKBAJA_VEC_WRT_PST_FAIL),
                (VOIDFUNCPTR)sysWritePostFailInt, 0);
    intConnect (INUM_TO_IVEC(HKBAJA_VEC_ARB_TOUT),
                (VOIDFUNCPTR)sysArbTimeoutInt, 0); 
    intConnect (INUM_TO_IVEC(HKBAJA_VEC_SYSFAIL), (VOIDFUNCPTR)sysFailInt, 0);

    /* do sysHwInit2() initialization of the VIC driver */

    sysVicInit2 ((VOIDFUNCPTR *)INUM_TO_IVEC(HKBAJA_VEC_DMA));

#ifdef INCLUDE_PCI

    /* call initialization routines in intPci.c */

    sysPciIntInit();
    sysPciIntListInit();

    /* initialize PCI and create the device list */

    sysPciInit ();

#endif /* INCLUDE_PCI */

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */
    }

/*******************************************************************************
*
* sysLocalToBusAdrs - convert a local address to a bus address
*
* Given a local memory address, this routine returns the VMEbus address
* that would have to be accessed to get to that byte.
*
* RETURNS: OK, or ERROR if unable to get to that local address from the bus.
*
* SEE ALSO:
* sysBusToLocalAdrs()
*/

STATUS sysLocalToBusAdrs
    (
    int     adrsSpace,    /* address space of busAdrs, defined in vme.h */
    char *  localAdrs,    /* local address to convert */
    char ** pBusAdrs      /* where to return bus address */
    )
    {
    UINT32 physLocalAdrs;

    /* simple assumption: only board 0 is mapped to VME */

    if (sysProcNumGet () != 0)
        return (ERROR);  

    physLocalAdrs = (UINT32)CACHE_DMA_VIRT_TO_PHYS (localAdrs);

    /* calculations formed on 0-based RAM (this makes life easy) */

    if (physLocalAdrs < ((UINT32)sysMemTop () - (UINT32)LOCAL_MEM_LOCAL_ADRS))
        {
        if ((adrsSpace != VME_AM_EXT_SUP_ASCENDING) &&
            (adrsSpace != VME_AM_EXT_SUP_PGM) &&
            (adrsSpace != VME_AM_EXT_SUP_DATA) &&
            (adrsSpace != VME_AM_EXT_USR_ASCENDING) &&
            (adrsSpace != VME_AM_EXT_USR_PGM) &&
            (adrsSpace != VME_AM_EXT_USR_DATA))
            return (ERROR);

        *pBusAdrs = (char *)((UINT32)SM_MEM_A32_BASE_ADRS + physLocalAdrs);
        }
    else
        {
        if ((physLocalAdrs < 0x40000000) || (physLocalAdrs >= 0xfc000000))
            return (ERROR);

#if (CPU == R4000)
        if (!IS_KUSEG (physLocalAdrs))
            return (ERROR);
#endif /* (CPU == R4000) */

        /* this is off-board VME memory - just return local address */

        *pBusAdrs = localAdrs;
        }

    return (OK);
    }

/*******************************************************************************
*
* sysBusToLocalAdrs - convert a bus address to a local address
*
* Given a VMEbus memory address, this routine returns the local address
* that would have to be accessed to get to that byte.
*
* RETURNS: OK, or ERROR if unknown address space.
*
* SEE ALSO:
* sysLocalToBusAdrs()
*/

STATUS sysBusToLocalAdrs
    (
    int     adrsSpace,    /* address space of busAdrs, defined in vme.h */
    char *  busAdrs,      /* bus address to convert */
    char ** pLocalAdrs    /* where to return local address */
    )
    {
    switch (adrsSpace)
        {
        case VME_AM_SUP_SHORT_IO:
        case VME_AM_USR_SHORT_IO:
            *pLocalAdrs = (char *)((ULONG) HKBAJA_VME_SHORT_ADRS |
                ((ULONG) busAdrs & HKBAJA_VME_SHORT));
        break;

        case VME_AM_STD_SUP_ASCENDING:
        case VME_AM_STD_SUP_PGM:
        case VME_AM_STD_SUP_DATA:
        case VME_AM_STD_USR_ASCENDING:
        case VME_AM_STD_USR_PGM:
        case VME_AM_STD_USR_DATA:
            *pLocalAdrs = (char *) ((ULONG) HKBAJA_VME_STANDARD_ADRS |
                ((ULONG) busAdrs & HKBAJA_VME_STANDARD));
        break;

        case VME_AM_EXT_SUP_ASCENDING:
        case VME_AM_EXT_SUP_PGM:
        case VME_AM_EXT_SUP_DATA:
        case VME_AM_EXT_USR_ASCENDING:
        case VME_AM_EXT_USR_PGM:
        case VME_AM_EXT_USR_DATA:
            if (busAdrs < (char *)0x40000000)
                return (ERROR);
#if (CPU == R4000)
            if (busAdrs >= (char *)K0BASE)
                return (ERROR);
#else  /* (CPU == R4000) */
            if (busAdrs >= (char *)0xfc000000)
                return (ERROR);
#endif /* (CPU == R4000) */
            *pLocalAdrs = busAdrs;
        break;

        default:
            return (ERROR);
        break;
        }
    return(OK);
    }

/*******************************************************************************
*
* sysProcNumSet - set the processor number
*
* Set the processor number for this CPU.  Processor numbers should be unique
* on a single backplane.  This routine performs several functions: maps local
* RAM onto the VMEbus if this is processor 0, releases the VMEbus SYSFAIL
* signal, waits for other boards on the backplane to complete their
* initializations.
*
* NOTE:
* The bus arbiter/system controller option is set by a hardware jumper.
*
* RETURNS: N/A
*
* SEE ALSO:
* sysProcNumGet()
*
* NOMANUAL
*/

void sysProcNumSet
    (
    int procNum    /* processor number */
    )
    {
    UINT8  oldRelCntrl;
    char * pBusAdrs;

    if (procNum == 0)
        {
        /* clear the SM_ANCHOR location */

        bzero (SM_ANCHOR_ADRS, sizeof (SM_ANCHOR));

        sysLocalToBusAdrs (VME_AM_EXT_SUP_DATA, (char *)SM_MEM_A32_BASE_ADRS,
                           &pBusAdrs);

        printf ("Mapping RAM base to A32 space at 0x%x... ", (UINT32)pBusAdrs);

        /* get current state of release control register */

        oldRelCntrl = *VIC_RCR;

        /* tell the vic to capture the bus (so everyone else is off it) */

        *VIC_RCR = RCR_BCAP;

        *VIC_SS1CR0 = SSCR0_ASIZ_A32 | SSCR0_SLSEL_D32 | SSCR0_BLT_ACC;

        *(HKBAJA_VME_BASE_ADRS) = (UINT8)(SM_MEM_A32_BASE_ADRS >> 24);
        *(HKBAJA_VME_LOCAL_ADRS) = 0x00;
        *(HKBAJA_VME_EXT_ENABLE) = 0;

        /* put release control register back to previous state */

        *VIC_RCR = oldRelCntrl;

        printf ("done.\n");
        }

    /* map the VIC's mailbox */

    sysMailboxBaseSet((USHORT)(procNum<<8));

    printf ("Waiting for VME /SYSFAIL signal to clear... ");

    /* wait for other boards to release SYSFAIL */

    sysFailWait ();

    printf ("done.\n");

    /* copy the processor number into a global location */

    sysProcNum = procNum;
    }

/*******************************************************************************
*
* sysProcNumGet - get the processor number
*
* This routine returns the processor number of the board.
*
* RETURNS: The processor number.
*
* SEE ALSO:
* sysProcNumSet()
*/

int sysProcNumGet (void)

    {
    return (sysProcNum);
    }

/*******************************************************************************
*
* sysDelay - wait a given amount of microseconds
*
* This routine sets up and then polls counter/timer 3 of the PIG.
*
* RETURNS: N/A
*
* NOMANUAL
*/

LOCAL void sysDelay
    (
    UINT32 usec  /* 1E-6 of a second */
    )
    {
    UINT32 ticks = PIG_MICROSECOND * usec;

    /* stop/disable timer (just in case )*/

    *PIG_TMR3_CNTL_CNT = 0;

    /* load up timer */

    *PIG_TMR3_PER = ticks - 1;

    /* start timer */

    *PIG_TMR3_CNTL_CNT = PIG_TMR_CNTL_ENABLE | PIG_TMR_CNTL_STRSTP;

    /* wait until timer expires */

    while (!(*PIG_TMR3_CLR_STAT & PIG_TMR_STAT_INPEND));
    }

/******************************************************************************
*
* sysEnetAddrGet - obtain the Ethernet address for this unit (TODO)
*
* The Ethernet address is stored in a write-protected area of
* Flash ROM 0. This routine reads that Ethernet address data
* and write it to the buffer desired.
*
* RETURNS: OK, or ERROR if <unit> is not zero (0).
*
* NOMANUAL
*/

STATUS sysEnetAddrGet
    (
    int unit,   /* the unit to get address for */
    char addr[] /* pointer to storage for the enet addr */
    )
    {
#if (CPU == R4000)
    static char enetFixed[] = { 0x0, 0x80, 0xf9, 0x75 };
#else /* (CPU == R4000) */
    static char enetFixed[] = { 0x0, 0x80, 0xf9, 0x80 };
#endif /* (CPU == R4000) */

    /* check flash for enet address */

    if ( !bcmp(enetFixed,(char *)
        (HKBAJA_FLASH0+ENET_ADDR_OFFSET),sizeof(enetFixed)) )
        {
        bcopyBytes((char *)(HKBAJA_FLASH0+ENET_ADDR_OFFSET),addr,
            PHYS_ADDR_SIZE);
        return OK;
        }

    /* check at EPROM address; flash could be there */

    else if ( !bcmp(enetFixed,(char *)
        (HKBAJA_ROM_BASE_ADRS+ENET_ADDR_OFFSET),sizeof(enetFixed)) )
        {
        bcopyBytes((char *)(HKBAJA_ROM_BASE_ADRS+ENET_ADDR_OFFSET),addr,
            PHYS_ADDR_SIZE);
        return OK;
        }

    return (ERROR);
    }

/****************************************************************************
*
* sys596Init - initialize board-specific aspects of the i82596CA chip
*
* There is nothing to do here.  All of the parameters that could be
* programmed into the chip are fixed in hardware.
* This routine is essentially a NOP, but the i82596CA driver requires
* that it be present.
*
* RETURNS: OK
*
* NOMANUAL
*/

STATUS sys596Init
    (
    int unit  /* unit number of 596 chip */
    )

    {
    return (OK);
    }

/****************************************************************************
*
* sys596IntAck - acknowledge an interrupt from a 596
*
* This function acknowledges an interrupt from the i82596CA.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sys596IntAck
    (
    int unit  /* unit number of 596 chip */
    )

    {
    return;
    }

/***************************************************************************
*
* sys596IntEnable - activate the interrupt from the Ethernet chip
* 
* This function activates the Ethernet interrupts from the i82596CA.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sys596IntEnable
    (
    int unit  /* unit number of 596 chip */
    )

    {
    int theLock;

    theLock = intLock();

    /* enable the interrupt in the Vic chip */

    *VIC_LICR1 &= (char)~(LICR_IRQ_DISABLE); 

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */

    intUnlock(theLock);
    }

/***************************************************************************
*
* sys596IntDisable - deactivate the interrupt from the Ethernet chip
*
* This function disables the Ethernet interrupts from the i82596CA.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sys596IntDisable
    (
    int unit  /* unit number of 596 chip */
    )

    {
    int theLock;

    theLock = intLock();

    /* disable the interrupt in the Vic chip */

    *VIC_LICR1 |= LICR_IRQ_DISABLE;

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */

    intUnlock(theLock);
    }

/*****************************************************************************
*
* sys596ChanAtn - send Channel Attention to the i82596CA Ethernet chip
*
* This function sends a "Channel Attention" to the i82596CA Ethernet chip.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sys596ChanAtn
    (
    int unit  /* unit number of 596 chip */
    )

    {
    FAST ULONG anyData = 0l;

    *HKBAJA_EI_CA = anyData;

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */
    }

/*****************************************************************************
*
* sys596Port - issue a PORT command to the i82596CA Ethernet chip
*
* This routine issues PORT commands to the i82596CA Ethernet chip.  Through
* PORT commands the chip can be made to reset itself, do a self test, dump
* its internal state, or use a new System Configuration Pointer (SCP) address.
*
* RETURNS: N/A
*
* INTERNAL
* Early versions of the Intel i82596CA chip needed to be poked twice (i.e., the
* PORT address needed to be written to four times).  Later vintages of the chip
* seem to only require 1 hit (i.e., two writes to the PORT address). Perhaps
* the four writes below could be chopped down to only two.
*
* NOMANUAL
*/

void sys596Port
    (
    int unit,    /* unit number of 596 chip */
    int func,    /* function to be performed */
    UINT32 addr  /* address for port functions */
    )

    {
    FAST ULONG portcmd;

    /* mung up the address for the i82596CA to read */

    portcmd = MKLONG((((UINT32)(addr) & 0xFFFFFFF0) | (UINT32)(func))); 

    *HKBAJA_EI_PORT = portcmd;
    *HKBAJA_EI_PORT = portcmd;
    *HKBAJA_EI_PORT = portcmd;
    *HKBAJA_EI_PORT = portcmd;

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */
    }

/*******************************************************************************
*
* sysFlashReadArray - put flash memory chips in read mode
*
* This routine puts a flash memory chip in read mode. This is
* necessary after a write, and also to "wake up" chips that are
* in a deep powerdown state.
*
* RETURNS: N/A
*/

void sysFlashReadArray
    (
    volatile char * address	/* any address within the chip's boundaries */
    )

    {
    *address = I28F008_READ_ARRAY;

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */
    }

/*******************************************************************************
*
* sysFlashReadIid - read Intelligent Identfier code from flash ROM chip
*
* This routine checks the intelligent identifier code from the flash
* memory chips.  It is typically used to probe for the existence of
* flash memory chips.  The actual codes are returned in the IID
* structure.
*
* This function will fail if the flash ROM being probed is declared
* copyback cacheable in sysPhysMemDesc[]. You will always read a
* vendor code of I28F008_INTELLIGENT_ID, since this was was just
* written to the vendor code read address.
*
* RETURNS: OK if intelligent identfier code matches that of
* an Intel 28f008, ERROR otherwise.
*/

STATUS sysFlashReadIid
    (
    volatile char * baseAddress,	/* base address of the chip */
    IID * iid	     /* intellegent id codes returned here */
    )

    {
    *baseAddress = I28F008_INTELLIGENT_ID;
    iid->vendorCode = *baseAddress;
    iid->deviceCode = *(baseAddress+1);

    if (iid->vendorCode != I28F008_VENDOR_CODE ||
    	iid->deviceCode != I28F008_DEVICE_CODE)
    	return ERROR;
    else
    	return OK;
    }

/*******************************************************************************
*
* sysFlashReadStatus - read the flash ROM's status register
*
* This routine reads the status register of a flash ROM chip.
*
* RETURNS: The value read from the status register
*/

char sysFlashReadStatus
    (
    volatile char * address   /* an address somewhere within the chip */
    )

    {
    *address = I28F008_READ_STATUS;

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */

    return *address;
    }

/*******************************************************************************
*
* sysFlashClearStatus - clear the flash ROM's status register
*
* This routine clears the flash ROM status register.
*
* RETURNS: N/A
*/

void sysFlashClearStatus
    (
    volatile char * address	/* an address somewhere within the chip */
    )

    {
    *address = I28F008_CLEAR_STATUS;
    *address = I28F008_READ_ARRAY;

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */
    }

/*******************************************************************************
*
* sysFlashEraseBlock - erase a block of flash memory
*
* This routine erases a 64k byte block of flash memory. It polls
* the chip until the erase completes, which may take several
* seconds.
*
* INTERNAL
* This routine only seems to work when the address is long word
* aligned, although the documentation implies that isn't necessary.
*
* RETURNS: ERROR if erase failed, otherwise OK.
*/

STATUS sysFlashEraseBlock
    (
    char * address	/* address within a 64k block to be erased */
    )

    {
    UINT32 startTick = tickGet();
    STATUS status;
    volatile char * chipAddress = (volatile char *) ((int) address & ~3) ;
                                                       /* tjf (vol!) */
    /* start the erase */
    *chipAddress = I28F008_ERASE_SETUP;
    *chipAddress = I28F008_ERASE_CONFIRM;

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */

    /* watch the chip until block erased or timeout */
    while ( !(*(volatile char *) chipAddress & I28F008_STATUS_WSMS) &&
    	((startTick + I28F008_ERASE_TIMEOUT * sysClkRateGet()) > tickGet()))
			;

    if ( *(volatile char *) chipAddress & I28F008_STATUS_ES )
    	status = ERROR;
    else
    	status = OK;

    /* erase is done */
    sysFlashClearStatus( chipAddress);
    sysFlashReadArray( chipAddress);

    return status;

    }

/*******************************************************************************
*
* sysFlashWriteByte - write a byte to flash memory
*
* This routine writes a single byte to flash memory.
*
* RETURNS: ERROR if write failed, otherwise OK.
*
*/

STATUS sysFlashWriteByte
    (
    char * address,	/* the address to write to */
    char data       /* the data to write there */
    )

    {
    volatile char * chipAddress = (volatile char *) ( (int) address & ~3);
                                                            /* tjf (vol) */
    unsigned long byteOffset = ((int) address & 3);
    UINT32 startTick = tickGet();
    STATUS status = ERROR;

    /* setup pig control register for byte offset */
    *PIG_CONTROL = (*PIG_CONTROL & ~0x700) | (byteOffset << 8);

    /* do the write */
    *chipAddress = I28F008_WRITE_SETUP;
    *chipAddress = data;

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */

    /* watch the chip until not busy or timeout */
    while ( !(*(volatile char *) chipAddress & I28F008_STATUS_WSMS) &&
    	((startTick + I28F008_WRITE_TIMEOUT * sysClkRateGet()) > tickGet()))
    	;

    if ( (*(volatile char *) chipAddress & I28F008_STATUS_BWS) )
    	status = ERROR;
    else
    	status = OK;

    /* write is done */
    sysFlashClearStatus(chipAddress);
    sysFlashReadArray(chipAddress);

    return status;

    }

#if (CPU == R4000)

/******************************************************************************
*
* sysVicDeMux - demultiplex a VIC hardware interrupt
*
* The VIC is capable of interrupting the processor with 256 of its own
* vectors.  These VIC-vectors indicate the type of interrupt the VIC is
* generating (VMEBus, mailbox, sysfail, DMA etc.).
*
* RETURNS: The VIC vector.
*
* NOMANUAL
*/
 
LOCAL int sysVicDeMux
    (
    ULONG ipl    /* VIC ipl line we are serving (not used) */
    )
 
    {
    int vector = *HKBAJA_VIC_IPL1_ACK_ADRS & 0xff;

    return (vector);
    }
 
/******************************************************************************
*
* sysClearTlb - clear the translation lookaside buffer
*
* This routine clears the entries in the translation lookaside buffer (TLB)
* for the MIPS R4600 CPU.
*
* RETURNS: N/A
*
* NOMANUAL
*/
 
void sysClearTlb (void)
 
    {
    FAST int tlbEntry;
 
    for (tlbEntry = 0; tlbEntry < TLB_ENTRIES; tlbEntry++)
        sysClearTlbEntry (tlbEntry);
    }

/*******************************************************************************
*
* sysBusErrorAck - clear bus error condition
*
* This routine acknowledges a bus error if there is one to acknowledge.
*
* RETURNS: The last bus error latch.
*
* NOMANUAL
*/

LOCAL UINT32 sysBusErrorAck (void)

    {
    LOCAL UINT32 busErrLatch;     /* bus error address and ID bits */
    LOCAL UINT32 busErrAdrs;  

    if ((intCRGet () & CAUSE_IP6) != 0)
        {
        busErrLatch = *HKBAJA_BUSERR_LATCH;

        /* clear the bus error latch */

        *PIG_CNTL_BIT7 = 0;
        *PIG_CNTL_BIT7 = 1;

        /* release the local bus in case this is a sysRMWCycle */

        *PIG_CNTL_BIT6 = 1;

        sysWbFlush ();

        /* check if this is a PCI bus error */

        busErrAdrs = busErrLatch & 0xfffffff8;
        }

    return (busErrLatch);
    }

/******************************************************************************
*
* sysBusEid - get the value of the error ID register
*
* This routine clears the bus error and returns the error ID.
* The cause of the bus error is encoded in the error ID
* shown in the table below:
*
* .CS
* Error ID | Cause
* ----------------------------------
*     0    | Write from VMEbus
*     1    | Write from PCI
*     2    | Write from Ethernet
*     3    | Write from CPU
*     4    | Read from VMEbus
*     5    | Read from PCI
*     6    | Read from Ethernet
*     7    | Read from CPU
* .CE
*
* NOTE:
* This routine must be provided on all R4000 board support packages.
*
* NOMANUAL
*
* RETURNS: The Error ID.
*/
 
USHORT sysBusEid (void)
 
    {
    return (sysBusErrorAck () & 7);
    }

/******************************************************************************
*
* sysBusEar - get the access address of a bus error
*
* This routine returns the contents of the Bus Error Ack,
* which contains the physical address part of the access address.
* The bus error is also reset by this routine.
*
* NOTE:
* This routine must be provided on all R4000 board support packages.
*
* NOMANUAL
*
* RETURNS: The access address of the bus error.
*/
 
ULONG sysBusEar (void)
    {
    return (sysBusErrorAck () & 0xfffffff8);
    }

/******************************************************************************
*
* sysSw0Ack - acknowledge software interrupt 0
*
* This routine writes the R4000 cause register to acknowledge a software
* interrupt.
*
* NOTE:
* This routine is provided as a default interrupt service routine.
*
* RETURNS: N/A
*/
 
LOCAL void sysSw0Ack (void)
    {
    unsigned causeReg;
 
    causeReg = intCRGet ();
    causeReg &= ~CAUSE_SW1;
    intCRSet (causeReg);
    }

/******************************************************************************
*
* sysSw1Ack - acknowledge software interrupt 1
*
* This routine writes the R4000 cause register to acknowledge a software
* interrupt.
*
* NOTE:
* This routine is provided as a default interrupt service routine.
*
* RETURNS: N/A
*/
 
LOCAL void sysSw1Ack (void)
    {
    unsigned causeReg;
 
    causeReg = intCRGet ();
    causeReg &= ~CAUSE_SW2;
    intCRSet (causeReg);
    }

/******************************************************************************
*
* sysAutoAck - acknowledge the R4000 interrupt condition
*
* This routine acknowledges an R4000 interrupt for a specified interrupt
* vector.
*
* NOTE:
* This routine must be provided on all R4000 board support packages.
*
* NOMANUAL
*
* RETURNS: The result of the interrupt acknowledge cycle or -1 if an error.
*/
 
int sysAutoAck
    (
    int vecNum          /* vector num of interrupt that bugs us */
    )
 
    {
    int result = 0;
 
    switch (vecNum)
        {
        case IV_SWTRAP0_VEC:
            sysSw0Ack ();
            break;
        case IV_SWTRAP1_VEC:
            sysSw1Ack ();
            break;
        case IV_BUS_ERROR_VEC:
            result = sysBusEid ();
            break;
        default:
            if ((intCRGet () & CAUSE_IP3) != 0)
                result = *HKBAJA_VIC_IPL1_ACK_ADRS & 0xff;
            else
                logMsg ("sysAutoAck: vecNum = 0x%x\n", vecNum, 0, 0, 0, 0, 0);
            break;
        }

    return (result);
    }

/******************************************************************************
*
* sysMaskVmeErr - disable VMEbus error reporting
*
* This routine disables VMEbus error reporting to the R3081.
*
* RETURNS: TRUE
*
* SEE ALSO: sysUnmaskVmeErr()
*
* NOMANUAL
*/
 
BOOL sysMaskVmeErr (void)
    {
    return (TRUE);
    }

/******************************************************************************
*
* sysUnmaskVmeErr - enable VMEbus error reporting
*
* This routine enables VMEbus error reporting to the R3081.
*
* RETURNS: N/A
*
* SEE ALSO: sysMaskVmeErr()
*
* NOMANUAL
*/
 
void sysUnmaskVmeErr (void)
    {
    }

/******************************************************************************
*
* sysRMWCycle - do an indivisible Read-Modify-Write cycle
*
* This routine does an atomic Read-Modify-Write cycle.
*
* The MIPS R4000 CPU does not provide an atomic read-modify-write or test-and-
* set instruction like 68000 family processors. This routine is a workaround
* for that limitation.
*
* RETURNS: TRUE if the value at the address was zero, or FALSE if the
* value was nonzero (set already).
*
* NOMANUAL
*/
BOOL sysRMWCycle
    (
    volatile char * address,    /* address for RMW cycle */
    char            data        /* data to be written there */
    )

    {
    int    oldLevel;
    char   oldValue;
 
    oldLevel = intLock ();

    /* Lock the local bus.  If this is a VME access, the VIC will
     * hold address strobe
     */

    *PIG_CNTL_BIT6 = 0;

    oldValue = *address;

#ifdef INCLUDE_PCI
    if ((address >= (char *)PCI_START_ADRS) && (address < (char *)PCI_END_ADRS))
        {
        /* Disable PCI lock cycles.  The PCI9060 will release the PCI bus after
         * the write cycle.
         */
        *PCI9060_LTP_REMAP (PCI9060) = X32 (PCI_REMAP_ADRS | BIT1 | BIT0);
        }
#endif /* INCLUDE_PCI */

    *address = data;

#ifdef INCLUDE_PCI
    if ((address >= (char *)PCI_START_ADRS) && (address < (char *)PCI_END_ADRS))
        {
        /* reenable PCI lock cycles */

        *PCI9060_LTP_REMAP (PCI9060) = 
                                      X32 (PCI_REMAP_ADRS | BIT2 | BIT1 | BIT0);
        }
#endif /* INCLUDE_PCI */

    /* release the local bus */

    *PIG_CNTL_BIT6 = 1;

    sysWbFlush();

    intUnlock (oldLevel);
 
    return (!oldValue);
    }

/******************************************************************************
*
* sysBusTas - test and set a location across the bus
*
* This routine performs a test-and-set over the VMEBus.
*
* NOTE: This routine is equivalent to the local routine vxTas().
*
* RETURNS: TRUE if the value had not been set but now is, or FALSE if
* the value was set already.
*/
 
BOOL sysBusTas
    (
    char * address    /* address to be tested and set */
    )
 
    {
    return (sysRMWCycle (address, 0x80));
    }

/*******************************************************************************
*
* sysBusClearTas - clear a location via an indivisible Test-And-Set cycle
*
* This function does a Test-And-Set (Tas) cycle across the bus
* or locally to clear a location that was previously set with sysBusTas().
* It calls sysRMWCycle() to do the actual work. It is here because
* the bp backplane driver requires it.
*
* RETURNS: N/A
*
* NOMANUAL
*/
 
void sysBusClearTas
    (
    char *address   /* address to be tested-and-cleared */
    )
 
    {
    (void) sysRMWCycle (address, 0);
    }

/*******************************************************************************
*
* sysPrioUpdate - update a demultiplex routine in the intPrioTable structure
*
* RETURNS: N/A
*
* NOMANUAL
*/
 
void sysPrioUpdate
    (
    int     index,
    FUNCPTR routine
    )
    {
    intPrioTable[index].bsrTableOffset = (UINT32) routine;
    }

#endif /* (CPU == R4000) */

/*******************************************************************************
*
* sysHwInit - initialize the hardware on this board
*
* This routine initializes various features of the Baja
* Normally, it is called from usrInit() in usrConfig.c.
*
* NOTE: This routine should not be called directly by the user.
*
* INTERNAL
* This routine fixes up the first and second entries in sysPhysMemDesc
* at runtime.  There are two reasons for this. First of all, the
* cacheable RAM regions is set no larger than available RAM.  This
* is important because offcard A32 space starts immediately after
* oncard RAM, and this should not be cached. Second, the Baja board
* can have up to 64 megabytes of RAM, and a 64 meg board must have
* two sysPhysMemDesc entries, since each can be no larger than 32 megs.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sysHwInit (void)

    {
    UINT8 sysLEDSet(UINT8, UINT8);
    UINT8 defaultVicIPL = VIC_IPL1;

    /* clear any parity errors from power up (paranioa) */

    sysMemParityIntClear();

    /* turn off user LEDs */

    (void) sysLEDSet (HKBAJA_LED, HKBAJA_LED_OFF);

#if (CPU == R4000)

    /* clear any bus errors */
 
    sysBusEar ();
 
    /* set default task status register */
 
    taskSRInit (TASK_SR);
 
    /* initialize status register and leave interrupts disabled */
 
    intSRSet (TASK_SR & (~SR_IE));

    /* initialize floating pt unit */
 
    if (fppProbe () == OK)
       {
       fppInitialize ();
       intVecSet ((FUNCPTR *)INUM_TO_IVEC (IV_FPA_BASE_VEC), (FUNCPTR)fpIntr);
       }
 
#ifdef INCLUDE_SM_NET
    {
    IMPORT VOIDFUNCPTR  smUtilTasClearRtn;
 
    smUtilTasClearRtn = sysBusClearTas;
    }
#endif  /* INCLUDE_SM_NET */

    /* set up UTLB exception handler */
 
    bcopy ((char *)sysExcUtlbVec, (char *)0x80000000, 0x80);
    cacheTextUpdate ((void *) 0x80000000, 0x80);


#endif /* (CPU == R4000) */

    /* size memory */

    (void) sysMemTop ();

    /* initialize PIG */

    sysPigInit();

    /* initialize VIC */

    sysVicInit ();
 
    /* Set dual path enable.  This step is essential for enabling
     * VME master cycles between VME block interleaves (master/MASTER).
     * Eventually, this bit will be set by the monitor.
     */
 
    *VIC_BTDR |= 0x01;

    /* initialize the vector base registers */

    *VIC_ICGIVBR = HKBAJA_VEC_ICGS_0;
    *VIC_ICMIVBR = HKBAJA_VEC_ICMS_0;
    *VIC_LIVBR = HKBAJA_VEC_LOCAL_BASE;
    *VIC_EGIVBR = HKBAJA_VEC_ACFAIL;

    /* initialize the VIC interrupt control registers... */

    *VIC_VIICR =  0xf8 | defaultVicIPL;
    *VIC_VICR1 =  0xf8 | defaultVicIPL;
    *VIC_VICR2 =  0xf8 | defaultVicIPL;
    *VIC_VICR3 =  0xf8 | defaultVicIPL;
    *VIC_VICR4 =  0xf8 | defaultVicIPL;
    *VIC_VICR5 =  0xf8 | defaultVicIPL;
    *VIC_VICR6 =  0xf8 | defaultVicIPL;
    *VIC_VICR7 =  0xf8 | defaultVicIPL;
    *VIC_DSICR =  0xf8 | defaultVicIPL;
    *VIC_LICR1 =  LICR_IRQ_ENABLE | LICR_IRQ_VEC_VIC | LICR_IRQ_EDGE |
                  defaultVicIPL; 
                                                          /* i82596 enet */
    *VIC_LICR2 =  LICR_IRQ_ENABLE | LICR_IRQ_VEC_VIC | defaultVicIPL; 
                                                          /* pig */
    *VIC_LICR3 =  LICR_IRQ_DISABLE | LICR_IRQ_VEC_VIC | defaultVicIPL; 
                                                          /* pci 2 */
    *VIC_LICR4 =  LICR_IRQ_DISABLE | LICR_IRQ_VEC_VIC | defaultVicIPL;
                                                          /* pci 1 */
    *VIC_LICR5 =  LICR_IRQ_DISABLE | LICR_IRQ_VEC_VIC | defaultVicIPL;
                                                          /* plx */
#if FALSE /* the switch, rtc and watchdog are unsupported for ECI */
    *VIC_LICR6 =  LICR_IRQ_ENABLE | LICR_IRQ_VEC_VIC | defaultVicIPL;
#else /* FALSE */
    *VIC_LICR6 =  LICR_IRQ_DISABLE | LICR_IRQ_VEC_VIC | defaultVicIPL;
#endif /* FALSE */
                                                          /* switch/RTC */
    *VIC_LICR7 =  LICR_IRQ_ENABLE | LICR_IRQ_VEC_VIC | defaultVicIPL;
                                                          /* parity error */
    *VIC_ICGICR = 0xf8 | defaultVicIPL;
    *VIC_ICMICR = 0xf8 | defaultVicIPL;
    *VIC_EGICR =  0xf8 | defaultVicIPL;

    /* support accellerated VME block transfers (applies to masters!) */

    *VIC_SS0CR0 |= SSCR0_BLT_ACC;

    /* Setup the serial device descriptors. */

    tyCoDv [0].created = FALSE;
    tyCoDv [0].pData = (volatile ULONG *)(PIG_PORTA_DATA);
    tyCoDv [0].pCntlStat = (volatile ULONG *)(PIG_PORTA_CNTL_STAT);
    tyCoDv [0].cntlSave = USART_DTR;
    tyCoDv [0].baudSave = 0;

    tyCoDv [1].created = FALSE;
    tyCoDv [1].pData = (volatile ULONG *)(PIG_PORTB_DATA);
    tyCoDv [1].pCntlStat = (volatile ULONG *)(PIG_PORTB_CNTL_STAT);
    tyCoDv [1].cntlSave = USART_DTR;
    tyCoDv [1].baudSave = 0;

    /* set x24c16 EEPROM (NVRAM) chip to known state */

    x24c16Init();

    /* initialize the Dallas 1286 RTC chip */

    ds1286Init(INITIAL_DS1286CR);

    /* wakeup all Flash ROMs present */

    sysFlashReadArray ( (char *) HKBAJA_FLASH0+HKBAJA_FLASH0_RO_SIZE);
    sysFlashReadArray ( (char *) HKBAJA_FLASH1);
    sysFlashReadArray ( (char *) HKBAJA_FLASH2);
    sysFlashReadArray ( (char *) HKBAJA_FLASH3);

#if (CPU == MC68060)

    /* turn the superscalar dispatch on */
    vxSSEnable ();
 
    /* enable the store buffer */
    cacheStoreBufEnable ();

#endif

#if (CPU == R4000)
    sysWbFlush();
#endif /* (CPU == R4000) */
    }
