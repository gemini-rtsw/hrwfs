/* sysLib.c - Motorola MVME167 system-dependent library */

/* Copyright 1984-1993 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
---,17jul98,smb  VME added for SDSU VME interface boards.
---,07may98,smb  Modified sysPhysMemDesc to allow interprocessor communication
                 with a Heurikon Baja, as advised by Bret Goodrich.
02e,31may95,ias  made changes to enable the reboot command
                 (sic_anj_software/Draft 0.3  section A.5)
02d,27feb93,ccc  added check for valid NV_CPU_SPEED, use 25MHz if not valid.
02c,22feb93,ccc  changed BCLK to NV_CPU_SPEED.
02b,13nov92,jcf  added VSAMSR2_SNP_WSD to VMASR initializations.
02a,31oct92,caf  mmu now maps 32 Mbytes as cached space for RAM.
01z,23oct92,caf  added A24 and A16 VMEbus regions to mmu map.
01y,22oct92,caf  made VMEbus accesses noncached (SPR 1686).
01x,01oct92,jcf  removed cache pointer initialization.
01w,29sep92,ccc  removed relative path names.
01v,28sep92,rfs  Modified sys596IntDisable() to avoid hardware bug in PCC.
01u,16sep92,gae  pared down DESCRIPTION.
01t,02sep92,caf  changed nvRamLib.c to nvRam.c.
01s,20aug92,ccc  setup prescaler for VMEbus reset timer.
01r,28jul92,rdc  sysToMonitor now disables mmu before reboot.
01q,28jul92,rfs  Corrected sys596Int routines for edge trig mode.
01p,24jul92,eve  fixed warning in sysScsiInit.
01o,16jul92,jwt  modified 01m - 68040 is not a fully snooped system.
01n,08jul92,rdc  added sysPhysMemDesc. 
01m,03jul92,jwt  NULLified flush and invalidate FUNCPTRs for 5.1 cache library.
01l,29jun92,caf  changed nvRam/bbRamLib.c to mem/nvRamLib.c.
01k,29jun92,caf  changed nvram/bbRamLib.c to nvRam/nvRamLib.c.
01j,28jun92,caf  moved sysNvRamSet() and sysNvRamGet() to bbRamLib.c.
01i,16jun92,ccc  created sysHwInit2(), ansified.  Moved VMEbus interrupt
		 routines and mailbox routines to separate libraries.
		 fixed mapping of 4MB board.
		 reset serial channels in sysHwInit().
01h,26may92,rrr  the tree shuffle
01g,14oct91,shl  added sys596IntDisable() and sysEnetAddrGet().
01f,14oct91,ccc  changed SHORT_IO to SUP access.
		 made VMEbus address based on sysProcNum.
		 changed VOID to void.
		 changed copyright notice.
01e,15sep91,jpb  removed exBusAdrsSpace.  The AM is passed as arg to exattach.
01d,18aug91,jdi  documentation cleanup.
01c,14aug91,ccc  made sysMemTop() read register for memSize, set VMEbus
		 extended access after local memory, doc changes.
01b,12aug91,ccc  changed 82596 to active HIGH interrupt, made A24 access D16,
		 de-linted.
01a,20jun91,ccc  created by modifying version 01m of mv147/sysLib.c.
*/

/*
DESCRIPTION
This library provides board-specific routines.  The chip drivers included are:
    pcc2Timer.c - PCC2 Timer library
    pcc2Vme.c - Peripheral Channel Controller 2 (PCC2) library
    nvRam.c - non-volatile RAM library

INCLUDE FILES: sysLib.h

SEE ALSO:
.pG "Configuration"
*/

#include "vxWorks.h"
#include "vme.h"
#include "memLib.h"
#include "cacheLib.h"
#include "sysLib.h"
#include "config.h"
#include "string.h"
#include "intLib.h"
#include "logLib.h"
#include "taskLib.h"
#include "vxLib.h"
#include "tyLib.h"
#include "private/vmLibP.h"
#include "drv/scsi/ncr710.h"
#include "drv/serial/cd2400.h"

/* Globals */

PHYS_MEM_DESC sysPhysMemDesc [] =
    {
    /* adrs and length parameters must be page-aligned (multiples of 0x2000) */

    /* ram */
    {
    (void *) LOCAL_MEM_LOCAL_ADRS,
    (void *) LOCAL_MEM_LOCAL_ADRS,
    0x2000000,				/* 32 Mbytes (adjust if necessary) */
    VM_STATE_MASK_VALID	| VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE	 | VM_STATE_CACHEABLE
    },

    /* some vme */
    {
    (void *) 0x2000000,
    (void *) 0x2000000,
    0x1000000,				/* 16 Mbytes */
    VM_STATE_MASK_VALID	| VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE	 | VM_STATE_CACHEABLE_NOT
    },

    /* VME added for the SDSU VME interface boards (SMB+ANJ - 17 JUL 98) */
	/* NOTE: This clashes with the address that Nick's multi-processor   */
	/* pipe driver assumes is occupied by processor 1.                   */
    {
    (void *) 0xc0000000,
    (void *) 0xc0000000,
    0x10000,				/* 64 Kbytes */
    VM_STATE_MASK_VALID	| VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* a24 vme (address set up in sysHwInit()) */
    {
    (void *) 0xf0000000,
    (void *) 0xf0000000,
    0x1000000,				/* 16 Mbytes */
    VM_STATE_MASK_VALID	| VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* rom */
    {
    (void *) ROM_BASE_ADRS,
    (void *) ROM_BASE_ADRS,
    0x400000,
    VM_STATE_MASK_VALID	| VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE_NOT  | VM_STATE_CACHEABLE_NOT
    },

    /* sram */
    {
    (void *) 0xffe00000,
    (void *) 0xffe00000,
    0x20000,
    VM_STATE_MASK_VALID	| VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE      | VM_STATE_CACHEABLE
    },

    /* local i/o devices */
    {
    (void *) 0xfff00000,
    (void *) 0xfff00000,
    0xf0000,
    VM_STATE_MASK_VALID	| VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    },

    /* a16 vme */
    {
    (void *) 0xffff0000,
    (void *) 0xffff0000,
    0x10000,				/* 64 Kbytes */
    VM_STATE_MASK_VALID	| VM_STATE_MASK_WRITABLE | VM_STATE_MASK_CACHEABLE,
    VM_STATE_VALID	| VM_STATE_WRITABLE      | VM_STATE_CACHEABLE_NOT
    }
    };

int sysPhysMemDescNumEnt = NELEMENTS (sysPhysMemDesc);

int   sysBus      = BUS;                /* system bus type (VME_BUS, etc)    */
int   sysCpu      = CPU;                /* system CPU type (MC680x0)         */
char *sysBootLine = BOOT_LINE_ADRS;	/* address of boot line              */
char *sysExcMsg   = EXC_MSG_ADRS;	/* catastrophic message area         */
int   sysFlags;				/* boot flags                        */
char  sysBootHost [BOOT_FIELD_LEN];	/* name of host from which we booted */
char  sysBootFile [BOOT_FIELD_LEN];	/* name of file from which we booted */
TY_CO_DEV tyCoDv [NUM_TTY];

/* All 6 bytes are initialized in sysHwInit from BBRAM */
unsigned char eiEnetAddr [6] = { 0x08, 0x00, 0x3e, 0x00, 0x00, 0x00 };

/* Locals */

LOCAL int sysProcNum;			/* processor number of this CPU */

#include "timer/pcc2Timer.c"
#include "vme/pcc2Vme.c"
#include "mem/nvRam.c"

/*******************************************************************************
*
* sysModel - return the model name of the CPU board
*
* This routine returns the model name of the CPU board.
*
* RETURNS: A pointer to the string "Motorola MVME167".
*/

char *sysModel (void)

    {
    return ("Motorola MVME167");
    }
/*******************************************************************************
*
* sysHwInit - initialize hardware
*
* This routine initializes various features of the MVME167.  It sets up the
* control registers, initializes the Peripheral Channel Controller II (PCC2),
* disables the timers, and sets up the VMEchip2.  It is called from usrInit()
* in usrConfig.c.
*
* The sysProcNumSet() routine maps any local memory on the VMEbus.
*
* NOTE
* This routine should not be called by the user.
*
* RETURNS: N/A
*/

void sysHwInit (void)

    {
    char cpuSpeed = *(char *) NV_CPU_SPEED;

    /* check for valid cpuSpeed */

    if (*(char *)(NV_CPU_SPEED + 1) != (char) 0xee)
	cpuSpeed = 25;		/* use 25MHz if not valid speed */

    /*
     * Set up the PCCchip2.  This does the following:
     *   - Initialize the PCCCHIP2 Interrupt Vector Base (upper 4 bits)
     *   - Disable and Initialize the Tick Timer Counter to zero
     *   - Disable GPIO Interrupts
     *   - Disable Modem Interrupts
     *   - Disable LANC Interrupts
     *   - Disable SCSI Interrupts
     *   - Disable Printer Interrupts
     */

    *MEMC_BCR = cpuSpeed;

    *PCC2_VBR = PCC2_INT_VEC_BASE;

    /* Initialize the Tick Timer Counters to zero */

    *PCC2_TIMER2_CR	    = TIMER2_CR_DIS; /* make sure counters are off */
    *PCC2_TIMER1_CR	    = TIMER1_CR_DIS;

    *PCC2_TIMER1_CNT	    = 0x00;
    *PCC2_TIMER2_CNT	    = 0x00;

    *PCC2_PRESCALE	    = 256 - cpuSpeed;
    *PCC2_PRESCALE_CLK_ADJ  = 256 - cpuSpeed;

    *PCC2_T2_IRQ_CR	    = T2_IRQ_CR_DIS; /* make sure interrupts are off */
    *PCC2_T1_IRQ_CR	    = T1_IRQ_CR_DIS; /* will be enabled later if used */

    /* Make sure General Purpose I/O disabled */

    *PCC2_GPIICR	    = GPIICR_DIS;	/* disable Interrupt */

    /* Set up the SCC Error Status Registers and Interrupt Control Registers
     * They will be enabled here so when they are enabled in the tyCoDrv.c
     * any interrupts will be handled properly
     */

    *PCC2_SCC_MICR	= SCC_MICR_DIS;		/* disable Modem Interrupts */

    /* Reset serial channels */

    while (*MPCC_CCR)
	;			/* make sure no outstanding commands */
    *MPCC_CCR = CCR_RESET_ALL;	/* reset chip */
    while (*MPCC_CCR)
	;			/* make sure we are done */

    while (*MPCC_GFRCR == 0)
	;			/* wait for it to be non-zero */

    *MPCC_TPR = 0x0a;		/* timer period register */
    *MPCC_PILR1 = 0x00;		/* priority interrupt level register */
    *MPCC_PILR2 = 0x02;		/* priority interrupt level register */
    *MPCC_PILR3 = 0x03;		/* priority interrupt level register */

    /* Set up the LANC Interrupt Control Registers
     * For now leave the Interrupts disabled to
     * prevent the interrupts from causing problems until
     * we are ready to use them in the LANC drive
     */

    *PCC2_LANC_IRQ_CR	= LANC_IRQ_CR_DIS;	/* Disable Interrupts for now */
    *PCC2_LANC_BEICR	= LANC_BEICR_DIS |	/* Disable Bus Error Ints */
			  LANC_BEICR_SINK_DATA;

    /* Set up the SCSI Interrupt Control Registers
     * For now leave the Interrupts disabled to
     * prevent the interrupts from causing problems until
     * we are ready to use them in the SCSI driver
     */

    *PCC2_SCSI_IRQ_CR	= SCSI_IRQ_CR_DIS;	/* Disable Interrupts for now */

    /* Set up the PRINTER Interrupt Control Registers
     * For now just disable the interrupts to make sure
     * they do not cause any problems until we are ready to
     * use the interrupts
     */

    *PCC2_PRINTER_ACK_IRQ	= PRINTER_ACK_IRQ_DIS;
    *PCC2_PRINTER_FAULT_IRQ	= PRINTER_FAULT_IRQ_DIS;
    *PCC2_PRINTER_SEL_IRQ	= PRINTER_SEL_IRQ_DIS;
    *PCC2_PRINTER_PE_IRQ	= PRINTER_PE_IRQ_DIS;
    *PCC2_PRINTER_BUSY_IRQ	= PRINTER_BUSY_IRQ_DIS;

    /* disable VME access to on-board ram and VMEchip slave write posting.
     * (VME access is later enabled for processor 0 in sysProcNumSet.)

     * no VME access now, maybe later

     * Set-up VMEbus access as follows:
     *
     *   SHORT_IO	0xffff0000 - 0xffffffff
     *   STD (A24/D16)  0xf0000000 - 0xf0ffffff
     *   EXT (A32/D32)  sysMemTop  - 0xefffffff
     */

    /* set VMEbus short access and enable map */

    *VMECHIP2_LBSAR1		= 0xffffffff;
    *VMECHIP2_LBSAR		|= LBSAR1_AM_SUP_SHORT_IO |
				   LBSAR1_D16;
    *VMECHIP2_LBTVCR		|= LBTVCR_EN1;

    /* set VMEbus std access and enable map */

    *VMECHIP2_LBSAR2		= 0xf0fff000;
    *VMECHIP2_LBSAR		|= LBSAR2_AM_STD_USR_DATA |
				   LBSAR2_D16;
    *VMECHIP2_LBTVCR		|= LBTVCR_EN2;

    /* set VMEbus ext access and enable map */

    *VMECHIP2_LBSAR3		= 0xefff0000 |
				  (int)sysMemTop() >> 16;
    *VMECHIP2_LBSAR		|= LBSAR3_AM_EXT_USR_DATA |
				   LBSAR3_D32;
    *VMECHIP2_LBTVCR		|= LBTVCR_EN3;

    /* set VMEbus global timeout to 64us
     * set VMEbus access timeout to 1ms
     * set local bus timeout to 64us
     * set watchdog timeout to 16ms
     * and set prescaler register
     */

    *VMECHIP2_TIMEOUTCR		= (TIMEOUTCR_VGTO_256US	|
				   TIMEOUTCR_VATO_1MS	|
				   TIMEOUTCR_LBTO_64US	|
				   TIMEOUTCR_WDTO_16MS) |
				   ((256 - cpuSpeed) & 0xff);

    /* all VMEbus vectors with 1:1 mapping */

    *VMECHIP2_ILR4		= (ILR4_VIRQ7_LEVEL7	|
				   ILR4_VIRQ6_LEVEL6	|
				   ILR4_VIRQ5_LEVEL5	|
				   ILR4_VIRQ4_LEVEL4	|
				   ILR4_VIRQ3_LEVEL3	|
				   ILR4_VIRQ2_LEVEL2	|
				   ILR4_VIRQ1_LEVEL1);

    /* setup VMEbus requester control */

    *VMECHIP2_DMACR1		= (DMACR1_SINK_DATA	|
				   DMACR1_LVRWD		|
				   DMACR1_LVREQ_L3);

    /* master enable chip interrupts */

    *VMECHIP2_IOCR		|= (IOCR_MEIN			|
				    (UTIL_INT_VEC_BASE0 << 24)	|
				    (UTIL_INT_VEC_BASE1 << 20));

    /* now setup serial device descriptor */

    tyCoDv [0].numChannels = NUM_TTY;

    tyCoDv [0].created = FALSE;
    tyCoDv [0].chan_num = 0;
    tyCoDv [0].int_vec = INT_VEC_CD2400_A;

    tyCoDv [1].created = FALSE;
    tyCoDv [1].chan_num = 1;
    tyCoDv [1].int_vec = INT_VEC_CD2400_B;

    tyCoDv [2].created = FALSE;
    tyCoDv [2].chan_num = 2;
    tyCoDv [2].int_vec = INT_VEC_CD2400_C;

    tyCoDv [3].created = FALSE;
    tyCoDv [3].chan_num = 3;
    tyCoDv [3].int_vec = INT_VEC_CD2400_D;

    /* Extract the ethernet address out of non-volatile RAM.
     * Motorola has the ethernet address in BBRAM.  If the address
     * is not in BBRAM you must reset it to the address located
     * on a label on the P2 connector.
     */

    eiEnetAddr [0] = (UCHAR)BB_ENET [0];
    eiEnetAddr [1] = (UCHAR)BB_ENET [1];
    eiEnetAddr [2] = (UCHAR)BB_ENET [2];
    eiEnetAddr [3] = (UCHAR)BB_ENET [3];
    eiEnetAddr [4] = (UCHAR)BB_ENET [4];
    eiEnetAddr [5] = (UCHAR)BB_ENET [5];

    /* now let's turn off the FAIL LED */
    *VMECHIP2_TIMERCR		&= ~TIMERCR_BDFLO;
    }
/*******************************************************************************
*
* sysAbortInt - handle ABORT interrupt
*
* This routine handles the ABORT switch interrupt
*
* RETURNS: N/A
*/

LOCAL void sysAbortInt (void)

    {
    *VMECHIP2_TIMERCR |= TIMERCR_BDFLO; /* turn on LED */
    *VMECHIP2_ICLR = ICLR_CAB;          /* clear interrupt */

    sysToMonitor (BOOT_NO_AUTOBOOT);
    }
/*******************************************************************************
*
* sysHwInit2 - connect hardware interrupts
*
* This routine connects additional hardware interrupts.
*
* RETURNS: N/A
*
* NOMANUAL
*/

void sysHwInit2 (void)

    {
    /* connect system clock interrupt */
    (void) intConnect (INUM_TO_IVEC (INT_VEC_CLOCK), sysClkInt, NULL);

    /* connect and enable abort switch interrupt */
    (void) intConnect (INUM_TO_IVEC (INT_VEC_ABORT), sysAbortInt, NULL);

    *VMECHIP2_ICLR  = ICLR_CAB; /* reset & enable the abort button interrupt */
    *VMECHIP2_LBIER |= LBIER_EAB;
    *VMECHIP2_ILR1  |= ILR1_AB_LEVEL7;

    /* connect auxiliary clock interrupt */
    (void) intConnect (INUM_TO_IVEC(INT_VEC_AUX_CLOCK), sysAuxClkInt, NULL);

    /* connect serial interrupts
     * the MVME167 board uses four interrupts for each channel:
     *    xxxx xx00 - Receive Exception Interrupt
     *    xxxx xx01 - Modem Signal Change Interrupt (not used)
     *    xxxx xx10 - Transmit Data Interrupt
     *    xxxx xx11 - Receive Data Interrupt
     */

    (void) intConnect (INUM_TO_IVEC (INT_VEC_CD2400_A),
                       tyCoInt, (int) &tyCoDv[0]);
    (void) intConnect (INUM_TO_IVEC (INT_VEC_CD2400_A+2),
                       tyCoIntTx, (int) &tyCoDv[0]);
    (void) intConnect (INUM_TO_IVEC (INT_VEC_CD2400_A+3),
                       tyCoIntRx, (int) &tyCoDv[0]);

    (void) intConnect (INUM_TO_IVEC (INT_VEC_CD2400_B),
                       tyCoInt, (int) &tyCoDv[1]);
    (void) intConnect (INUM_TO_IVEC (INT_VEC_CD2400_B+2),
                       tyCoIntTx, (int) &tyCoDv[1]);
    (void) intConnect (INUM_TO_IVEC (INT_VEC_CD2400_B+3),
                       tyCoIntRx, (int) &tyCoDv[1]);

    (void) intConnect (INUM_TO_IVEC (INT_VEC_CD2400_C),
                       tyCoInt, (int) &tyCoDv[2]);
    (void) intConnect (INUM_TO_IVEC (INT_VEC_CD2400_C+2),
                       tyCoIntTx, (int) &tyCoDv[2]);
    (void) intConnect (INUM_TO_IVEC (INT_VEC_CD2400_C+3),
                       tyCoIntRx, (int) &tyCoDv[2]);

    (void) intConnect (INUM_TO_IVEC (INT_VEC_CD2400_D),
                       tyCoInt, (int) &tyCoDv[3]);
    (void) intConnect (INUM_TO_IVEC (INT_VEC_CD2400_D+2),
                       tyCoIntTx, (int) &tyCoDv[3]);
    (void) intConnect (INUM_TO_IVEC (INT_VEC_CD2400_D+3),
                       tyCoIntRx, (int) &tyCoDv[3]);

    /* enable the interrupts for the serial chips on the PCC chip */

    *PCC2_SCC_TICR = MPCC_IRQ_LEVEL | SCC_TICR_IEN;
    *PCC2_SCC_RICR = MPCC_IRQ_LEVEL | SCC_RICR_IEN;
    }

/*******************************************************************************
*
* sysMemTop - get the address of the top of memory
*
* This routine finds the size of system RAM.  It reads a register in the
* memory controller chip (MEMC040) and returns the actual memory size.
*
* RETURNS: The address of the top of memory.
*/

char *sysMemTop (void)

    {
    static char *memTop = 0;

    if (memTop == 0)
	memTop = (char *)((4 << (*MEMC_MCR & 0x07)) << 20);

    return (memTop);
    }
/*******************************************************************************
*
* sysToMonitor - transfer control to the ROM monitor
*
* This routine transfers control to the ROM monitor.  Normally, it is called
* only by reboot()--which services ^X--and bus errors at
* interrupt level.  However, in some circumstances, the user may wish
* to introduce a <startType> to enable special boot ROM facilities.
*
* RETURNS: OK, if there is a return from the ROM monitor.
*/

STATUS sysToMonitor
    (
    int startType     /* parameter passed to ROM to tell it how to boot */
    )
    {
    /* this is an offset from romInit to the cold start entry pointer, via the
       reset vector.  If romInit is changed this may need to be modified
     */

    FUNCPTR *ppRom = (FUNCPTR *) (ROM_BASE_ADRS + 4);

    /* disable the mmu */

    VM_ENABLE(FALSE);

    (*ppRom) (startType);

    return (OK);	/* in case we ever continue from ROM monitor */
    }
/******************************************************************************
*
* sysProcNumGet - get the processor number
*
* This routine returns the processor number for the CPU board, which is set
* with sysProcNumSet().
*
* RETURNS: The processor number for the CPU board.
*
* SEE ALSO: sysProcNumSet()
*/

int sysProcNumGet (void)

    {
    return (sysProcNum);
    }
/******************************************************************************
*
* sysProcNumSet - set the processor number
*
* This routine sets the processor number for the CPU board.  Processor numbers
* should be unique on a single backplane.
*
* NOTE
* This routine enables access from the VMEbus to the MVME167's local
* memory only for processor 0.  (VxWorks only requires mapping for
* processor 0, and then only for backplane networks.)  If the processor
* number is not 0, but the user wants to allow VMEbus access to local memory,
* change the "if (procNum == 0)" condition below.
*
* This routine also sets the VMEchip GCSR address based on processor
* number.  This address can be specified anywhere from 0x00 to 0xe0 in
* increments of 0x10 in the short I/O space.  This can allow up to 15 CPUs
* in the same cage to form a backplane network using signal interrupts to
* coordinate their activities.
*
* RETURNS: N/A
*
* SEE ALSO: sysProcNumGet()
*/

void sysProcNumSet
    (
    int procNum		/* processor number */
    )
    {
    int memsize;

    sysProcNum = procNum;

    /* Set memory base address, as seen from VME bus */

    if (procNum == 0)
	{
	memsize = (4 << (*MEMC_MCR & 0x07));
	if (memsize == 4)
	    {
	    *VMECHIP2_VSAR2   = 0x003f0000	  |
				LOCAL_MEM_BUS_A24 |
				(LOCAL_MEM_BUS_A24 >> 16); /* map 4Mb of DRAM */

	    *VMECHIP2_VSATR2  = LOCAL_MEM_BUS_A24 >> 16;
					/* to 0x00800000..0x00bfffff */

	    *VMECHIP2_VSAMSR |= (VSAMSR2_SNP_WSD|
				 VSAMSR2_WP	|
				 VSAMSR2_SUP	|
				 VSAMSR2_USR	|
				 VSAMSR2_A24	|
				 VSAMSR2_D64	|
				 VSAMSR2_BLK	|
				 VSAMSR2_PGM	|
				 VSAMSR2_DAT);	/* all but A32 */
	    }

	*VMECHIP2_VSAR1   = (((memsize << 4) - 1) << 16)	|
			     (memsize << 21)			|
			     (memsize <<  5);
			     /* map all DRAM to VMEbus access */

	*VMECHIP2_VSATR1  = (memsize << 5);

	*VMECHIP2_VSAMSR |= (VSAMSR1_SNP_WSD 	|
			     VSAMSR1_WP		|
			     VSAMSR1_SUP	|
			     VSAMSR1_USR	|
			     VSAMSR1_A32	|
			     VSAMSR1_D64	|
			     VSAMSR1_BLK	|
			     VSAMSR1_PGM	|
			     VSAMSR1_DAT);	/* all but A24 */
	}
    else
	*VMECHIP2_VSAMSR &= !(VSAMSR1_WP	|
			      VSAMSR1_SUP	|
			      VSAMSR1_USR	|
			      VSAMSR1_A32	|
			      VSAMSR1_A24	|
			      VSAMSR1_D64	|
			      VSAMSR1_BLK	|
			      VSAMSR1_PGM	|
			      VSAMSR1_DAT);	/* turn off all bits */

    /* set global control register VME address, based on processor number */

    if (sysProcNum < 15)
	{
	/* VME address of GCSR is function of processor number */
	memsize = *VMECHIP2_LBTVCR;
	memsize &= ~(0xfff00000);	/* clear bits */
        *VMECHIP2_LBTVCR = (memsize | (sysProcNum << 20) |
			    (GCSR_GROUP_ADDR << 24));
	}
    }
/******************************************************************************
*
* sysBusTas - test and set a location across the bus
*
* This routine performs a 680x0 test-and-set instruction across the backplane.
*
* NOTE:  If a problem with TAS instruction occurs, check with Motorola, as
* some older-version boards have problems with the RMW cycle.  There is
* a PAL change to take care of this problem.
*
* This routine is equivalent to vxTas().
*
* RETURNS: TRUE if the value had not been set but is now, or FALSE if the
* value was set already.
*
* SEE ALSO: vxTas()
*/

BOOL sysBusTas
    (
    char *adrs		/* address to be tested and set */
    )
    {
    return (vxTas (adrs));
    }

/* miscellaneous support routines */

#ifdef	INCLUDE_EI
/* 82596 Ethernet chip support rotuines */
/*******************************************************************************
*
* sys596Init - performs any additional target specific initialization
*
* NOMANUAL
*/

void sys596Init
    (
    int unit		/* ignored on MVME167 */
    )
    {
    /* None required */
    }

/*******************************************************************************
*
* sys596IntDisable - performs any additional target specific interrupt disabling
*
* NOMANUAL
*/

void sys596IntDisable
    (
    int unit		/* ignored on MVME167 */
    )
    {
    int level;

    /* The CPU interrupt level is raised prior to modifying the bit in the
     * PCC device.  This is done to avoid a known hardware problem in the
     * PCC device that can cause spurious interrupts.
     */

    level = intLock ();
    *PCC2_LANC_IRQ_CR &= ~LANC_IRQ_CR_IEN;     /* Clear the enable bit only */
    intUnlock (level);
    }

/*******************************************************************************
*
* sys596IntEnable - performs any additional target specific interrupt enabling
*
* This routine enables interrupts for the on-board LAN chip.  LANC interrupts
* are controlled by the PCC2 chip.
*
* NOMANUAL
*/

void sys596IntEnable
    (
    int unit		/* ignored on MVME167 */
    )
    {
    *PCC2_LANC_IRQ_CR =
                        (
                        LANC_IRQ_CR_IEN      |    /* set enable bit */
                        LANC_IRQ_CR_EDGE     |    /* must be edge trigger */
                        LANC_IRQ_CR_LOW_HIGH |    /* high-->low edge */
                        LANC_IRQ_LEVEL            /* priority level */
                        );
    }

/*******************************************************************************
*
* sys596IntAck - performs any additional target specific interrupt acknowledge
*
* NOMANUAL
*/

void sys596IntAck
    (
    int unit		/* ignored on MVME167 */
    )
    {
    *PCC2_LANC_IRQ_CR |= LANC_IRQ_CR_ICLR;
    }

/*******************************************************************************
*
* sys596Port - writes a command to the 82596 device PORT location
*
* There are 4 commands the device handles.
*
* NOMANUAL
*/

void sys596Port
    (
    int unit,		/* ignored on MVME167                    */
    int cmd,		/* the command to write                  */
    UINT32 addr		/* address or NULL if PORT_RESET command */
    )
    {
    FAST UINT v1;
    FAST UINT value = (cmd & 0x3) | addr;

    v1 = ((value << 16) & 0xffff0000) | (value & 0x0000ffff);

    *I82596_PORT = v1;

    v1 = ((value >> 16) & 0x0000ffff) | (value & 0xffff0000);

    *I82596_PORT = v1;
    }

/*******************************************************************************
*
* sys596ChanAtn - assert the Channel Attention signal to the 82596 device
*
* NOMANUAL
*/

void sys596ChanAtn
    (
    int unit		/* ignored on MVME167 */
    )
    {
    *I82596_CONTROL = 1;		/* write to the addr; data is ignored */
    }

/*******************************************************************************
*
* sysEnetAddrGet - get the ethernet address
*
* RETURNS: OK, always.
*
* NOMANUAL
*/

STATUS sysEnetAddrGet
    (
    int unit,		/* ignored on MVME167 */
    UINT8 *addr
    )
    {
    bcopy ((char *)eiEnetAddr, (char *)addr, sizeof (eiEnetAddr));

    return (OK);
    }
#endif	/* INCLUDE_EI */

#ifdef	INCLUDE_SCSI
/*******************************************************************************
*
* sysScsiInit - initialize NCR 710 SCSI chip
*
* This routine creates and initializes an SIOP structure, enabling use of the
* on-board SCSI port.  It also connects the proper interrupt service routine
* to the desired vector, and enables the interrupt at the desired level.
*
* RETURNS: OK, or ERROR if the control structure is not created or the
* interrupt service routine cannot be connected to the interrupt.
*/
 
STATUS sysScsiInit ()
 
    {
    /* Local structure with a prefill for ncr710SetHwRegister */
    static NCR710_HW_REGS hwRegs = MV167_SIOP_HW_REGS;
 
    if ((pSysScsiCtrl = (SCSI_CTRL *)ncr710CtrlCreate (MV167_SIOP_BASE_ADRS,
                                                       MV167_SIOP_FREQ
                                                       )) == NULL)
        {
        return (ERROR);
        }
 
    /* connect the SCSI controller's interrupt service routine */
 
    if (intConnect (INUM_TO_IVEC (INT_VEC_SCSI),
                    ncr710Intr, (int) pSysScsiCtrl) == ERROR)
        {
        return (ERROR);
        }
 
    /*
     * Set the good value in the registers of the SIOP coupled
     * with the hardware implementation
     * NO MUX HOST BUS/NO BURST ACCES/SNOOP :DEFAULT/ENABLE SYNC HOST BUS/
     * BURST SIZE DEFAULT/OTHER :DEFAULT CONFIGURATION
     */
 
    if (ncr710SetHwRegister ((NCR_710_SCSI_CTRL *)pSysScsiCtrl, &hwRegs) 
	== ERROR)
        return(ERROR);
 
    /* initialize SCSI controller with default parameters (user tuneable) */
 
    if (ncr710CtrlInit ((NCR_710_SCSI_CTRL *)pSysScsiCtrl, 
			SCSI_DEF_CTRL_BUS_ID, NONE) == ERROR)
        return (ERROR);
 
    /*
     * clear status enable the SIOP interrupt
     * don't enable pcc scsi interrupt before ncr710CtrlInit
     */
 
   *PCC2_SCSI_ERR_SR = SCC_ERR_SR_SCLR ;
   *PCC2_SCSI_IRQ_CR = (SCSI_IRQ_CR_IEN |
                        SCSI_IRQ_LEVEL);
 
    return (OK);
    }
 
#endif /* INCLUDE_SCSI */

/*******************************************************************************
*
* sysLocalToBusAdrs - convert a local address to a bus address
*
* This routine gets the VMEbus address that accesses a specified local
* memory address.
*
* RETURNS: OK, or ERROR if the address space is unknown or the mapping is not
* possible.
*
* SEE ALSO: sysBusToLocalAdrs()
*/
 
STATUS sysLocalToBusAdrs
    (
    int adrsSpace,      /* bus address space in which busAdrs resides */
    char *localAdrs,    /* local address to convert                   */ 
    char **pBusAdrs     /* where to return bus address                */ 
    )
    {
    if ((int)localAdrs < LOCAL_MEM_LOCAL_ADRS || localAdrs >= sysMemTop ())
        {
        /* this is off-board memory - just return local address */

        *pBusAdrs = localAdrs;
        return (OK);
        }

    /* this is on-board memory - map to bus address space;
     *   the following memory mapping is established in sysProcNumSet():
     *   - only processor 0 has memory on bus,
     *   - the memory is placed in STD space at
     *      address LOCAL_MEM_BUS_24 if < 8 Meg of local memory
     *      and at twice the LOCAL_MEM_SIZE for EXT space.
     *      For  8MB memory 0x01000000
     *          16MB memory 0x02000000
     *          32MB memory 0x04000000
     *   - short I/O is not mapped locally.
     */
 
    switch (adrsSpace)
        {
        case VME_AM_SUP_SHORT_IO:
        case VME_AM_USR_SHORT_IO: /* no local map */
            return (ERROR);
 
        case VME_AM_STD_SUP_PGM:
        case VME_AM_STD_SUP_DATA:
        case VME_AM_STD_USR_PGM:
        case VME_AM_STD_USR_DATA:
            if ((4 << (*MEMC_MCR & 0x07)) == 4)
                {
                *pBusAdrs = localAdrs +
                            LOCAL_MEM_BUS_A24 - LOCAL_MEM_LOCAL_ADRS;
                return (OK);
                }
            else
                return (ERROR);
 
        case VME_AM_EXT_SUP_PGM:
        case VME_AM_EXT_SUP_DATA:
        case VME_AM_EXT_USR_PGM:
        case VME_AM_EXT_USR_DATA:
            *pBusAdrs = localAdrs +
                        (((4 << (*MEMC_MCR & 0x07)) << 1) << 20) -
                        LOCAL_MEM_LOCAL_ADRS;
            return (OK);
 
        default:
            return (ERROR);
        }
    }
/*******************************************************************************
*
* sysBusToLocalAdrs - convert a bus address to a local address
*
* This routine gets the local address that accesses a specified VMEbus
* memory address.
*
* RETURNS: OK, or ERROR if the address space is unknown or the mapping is not
* possible.
*
* SEE ALSO: sysLocalToBusAdrs()
*/

STATUS sysBusToLocalAdrs
    (
    int adrsSpace,      /* bus address space in which busAdrs resides */
    char *busAdrs,      /* bus address to convert                     */
    char **pLocalAdrs   /* where to return local address              */
    )
    {
    switch (adrsSpace)
        {
        case VME_AM_SUP_SHORT_IO:
        case VME_AM_USR_SHORT_IO:
            if (busAdrs > (char *)0x0000ffff)
                return (ERROR);

            *pLocalAdrs = (char *) (0xffff0000 | (int)busAdrs);
            return (OK);

        case VME_AM_STD_SUP_PGM:
        case VME_AM_STD_SUP_DATA:
        case VME_AM_STD_USR_PGM:
        case VME_AM_STD_USR_DATA:
            if (busAdrs > (char *)0x00ffffff)
                return (ERROR);

            *pLocalAdrs = (char *)(busAdrs + 0xf0000000);
            return (OK);

        case VME_AM_EXT_SUP_PGM:
        case VME_AM_EXT_SUP_DATA:
        case VME_AM_EXT_USR_PGM:
        case VME_AM_EXT_USR_DATA:
            if (busAdrs < sysMemTop () || busAdrs > (char *)0xf0000000)
                return (ERROR);

            *pLocalAdrs = (char *) busAdrs;
            return (OK);

        default:
            return (ERROR);
        }
    }
