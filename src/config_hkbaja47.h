/* config.h - Heurikon Baja configuration header */

/* Copyright 1995-1996 Heurikon Corporation */

/*
modification history
--------------------
01b,01apr96,tjf added more PCI_XX variables
01a,01sep95,tjf written by Ted Friedl, Heurikon Corporation
---  3oct96,ab (ROE) include NFS support, change default NFS_USER_ID to 700
            (epicsg). 
--- 27feb97,ab (ROE) include SM support (INCLUDE_SM_OBJ) for VxMP. (epicsg)
--- 22apr98,smb (ROE) increase NUM_FILES from 50 to 100.
--- 06may98,smb (ROE) remove SM support. Included INCLUDE_SECURITY. (epics)
*/

/*
This file contains the parameters that define the configuration for the
Heurikon Baja.
*/

#ifndef __INCconfigh
#define __INCconfigh

#include "configAll.h"
#include "hkbaja.h"

#define INCLUDE_NFS
#define NFS_USER_ID 700

#define NV_RAM_SIZE 254
#define NV_RAM_SIZE_WRITEABLE 254

#ifdef BOOT_LINE_SIZE
#undef BOOT_LINE_SIZE
#endif /* BOOT_LINE_SIZE */
#define BOOT_LINE_SIZE     NV_RAM_SIZE

#ifdef NV_BOOT_OFFSET
#undef NV_BOOT_OFFSET
#endif /* NV_BOOT_OFFSET */
#define NV_BOOT_OFFSET	0x0	/* into nvram for boot parameters */

#define NV_VW_OFFSET	0x700

#define INCLUDE_POSIX_ALL

/* Posix facilities */

#ifdef  INCLUDE_POSIX_ALL
#define INCLUDE_POSIX_AIO        /* POSIX async I/O support */
#define INCLUDE_POSIX_AIO_SYSDRV /* POSIX async I/O system driver */
#define INCLUDE_POSIX_FTRUNC     /* POSIX ftruncate routine */
#define INCLUDE_POSIX_MEM        /* POSIX memory locking */
#define INCLUDE_POSIX_MQ         /* POSIX message queue support */
#define INCLUDE_POSIX_SCHED      /* POSIX scheduling */
#define INCLUDE_POSIX_SEM        /* POSIX semaphores */
#define INCLUDE_POSIX_SIGNALS    /* POSIX queued signals */
#define INCLUDE_POSIX_TIMERS     /* POSIX timers */
#endif

#define NUM_FILES		100	/* max 100 files open simultaneously (changed from 50, nd14, 12/5/97) */

#define INCLUDE_SECURITY	/* shell security for network access */

/* Login security initial user name and password.
 * Use vxencrypt on host to find encrypted password.
 */
 
#ifdef  INCLUDE_SECURITY
#define LOGIN_USER_NAME         "cics"
#define LOGIN_PASSWORD          "SRebzSdyzS"
#endif  /* INCLUDE_SECURITY */

#undef USER_I_CACHE_MODE
#define USER_I_CACHE_MODE CACHE_WRITETHROUGH
#undef USER_D_CACHE_MODE
#define USER_D_CACHE_MODE CACHE_COPYBACK

#define DEFAULT_BOOT_LINE \
"ei(0,0)idsun1:/net/alba/sw2/vw5.2/config/gemag_hkbaja47/vxWorks h=195.194.120.4 e=195.194.120.6:ffffff00 u=cics f=0xa"

#if (CPU != R4000)
#define INCLUDE_MMU_BASIC
#endif /* (CPU != R4000) */

#if (CPU == R4000)
/* more stack is needed for the backplane driver */
#undef ROOT_STACK_SIZE
#define ROOT_STACK_SIZE         (40000)
#endif /* (CPU == R4000) */

/* defines for the if_ei driver */

#define SCP_MODE_82586        0x00     /* operate in i82586 mode */
#define SCP_MODE_SEGMENTED    0x02     /* operate in i82596 segmented mode */
#define SCP_MODE_LINEAR       0x04     /* operate in i82596 linear mode */
#define SCP_EXT_TRIGGER       0x08     /* ext. trig. of Bus Throttle timers */
#define SCP_LOCK_DISABLE      0x10     /* lock function disabled */
#define SCP_INT_LOW           0x20     /* interrupt pin is active low */
#define SCP_SYSBUS_BIT6       0x40     /* reserved - bit 6 - *MUST BE ONE* */
#define SCP_SYSBUS_BIT7       0x80     /* reserved - bit 7 - *MUST BE ZERO* */

#define EI_SYSBUS    ((int)(SCP_MODE_LINEAR|SCP_LOCK_DISABLE|SCP_SYSBUS_BIT6))
#define EI_POOL_ADRS ((int) (NONE))

#define INCLUDE_EI

/* Note: the bp and sm network drivers are mutually exclusive */

#define INCLUDE_SM_NET     /* include bp or sm driver */
#if FALSE
#define INCLUDE_BP_5_0     /* include bp driver otherwise sm driver */
#endif

#define INCLUDE_ENP
#define INCLUDE_EX

#define INCLUDE_PCI

#define PCI_START_ADRS  0x20000000       /* local start of PCI */
#define PCI_SIZE        0x10000000       /* size of each space */
#define PCI_END_ADRS    (PCI_START_ADRS + 2*PCI_SIZE)
                                         /* local end of PCI */
#define PCI_MEM_ADRS    PCI_START_ADRS   /* local start of PCI memory */
#define PCI_IO_ADRS     (PCI_START_ADRS + PCI_SIZE)
                                         /* local start of PCI I/O */
#define PCI_REMAP_ADRS  0x20000000       /* PCI remap address */
#define PCI_HIGH_INUM   HKBAJA_VEC_PCI1
#define PCI_LOW_INUM    HKBAJA_VEC_PCI2

#if FALSE
#define INCLUDE_SCSI
#define INCLUDE_SCSI_BOOT
#define INCLUDE_DOSFS
#define INCLUDE_RT11FS
#define INCLUDE_RAW
#define SCSI_AUTO_CONFIG
#endif

#define INCLUDE_68881
#define INCLUDE_FLOATING_POINT

/* Device controller I/O addresses: */

#undef  INT_VEC_EI
#define INT_VEC_EI      (HKBAJA_VEC_ENET)
#define INT_LVL_EI      0

#undef  IO_ADRS_ENP
#define IO_ADRS_ENP     ((char *) 0xde0000)   /* 24A,32D CMC Ethernet */
#undef  IO_ADRS_EX
#define IO_ADRS_EX      ((char *) 0xff0000)   /* 24A,32D Excelan Ethernet */

/* ds1286 stuff */

#define DS_WRITE_EN                     /* macro that does nothing */
#define INITIAL_DS1286CR 0              /* initial interrupt status register */

/* backplane network parameters */

#define SM_POLL FALSE
#if SM_POLL
#define SM_INT_TYPE     SM_INT_NONE
#else /* SM_POLL */
#define SM_INT_TYPE     SM_INT_MAILBOX_1
#endif /* SM_POLL */
#define SM_INT_ARG1     VME_AM_SUP_SHORT_IO
#define SM_INT_ARG2     ((int)((HKBAJA_DEFAULT_BP_MBOX+HKBAJA_MBOX_ADDR_BIAS) | ((USHORT)(sysProcNumGet())<<8)))
#define SM_INT_ARG3     0

/* the backplane master (usually cpu 0) also needs to know the following
 * shared memory pool parameters. */

#define SM_OFF_BOARD    FALSE

#if SM_OFF_BOARD
#undef  SM_ANCHOR_ADRS
#define SM_ANCHOR_ADRS  ((char *) (0x40000000)) /* off-board anchor adrs */
#define SM_MEM_ADRS     SM_ANCHOR_ADRS  /* off-board shared mem adrs */
#define SM_MEM_SIZE     0x80000         /* 512K */
#define SM_OBJ_MEM_ADRS (SM_MEM_ADRS+SM_MEM_SIZE)/* sh. mem Objects pool adrs */
#define SM_OBJ_MEM_SIZE 0x80000         /* sh. mem Objects pool size 512K */

#else
#if (CPU == R4000)
#undef  SM_ANCHOR_ADRS
#define SM_ANCHOR_ADRS  ((char *) (0xa0000600))
#endif /* (CPU == R4000) */
#define SM_MEM_ADRS     NONE            /* NONE = allocate sh. mem from pool */
#define SM_MEM_SIZE     0x10000         /* 64K */
#define SM_OBJ_MEM_ADRS NONE            /* sh. mem Objects pool adrs */
#define SM_OBJ_MEM_SIZE 0x10000         /* sh. mem Objects pool size 64K */
#endif  /* SM_OFF_BOARD */

/* clock library constants */

#define SYS_CLK_RATE_MIN 38
#define SYS_CLK_RATE_MAX 10000
#define AUX_CLK_RATE_MIN 38
#define AUX_CLK_RATE_MAX 10000

/* memory addresses */

/*
 * Local-to-Bus memory address constants:
 * the Baja local memory always appears at 0x00000000;
 * its address on the bus is set by bits in the BCL,
 * currently always at 0x00400000.
 */

#define LOCAL_MEM_LOCAL_ADRS     HKBAJA_RAM_BASE_ADRS     /* fixed */
#define LOCAL_MEM_BUS_ADRS       0x00400000              /* set in BCL */
#define LOCAL_MEM_SIZE (\
    0x00100000 << (PIG_RESET_CONFIG_MEM & *((unsigned long *)(PIG_CONTROL))) )

/*
 * The following parameters are defined here and in the Makefile.
 * They must be kept synchronized; effectively config.h depends on Makefile.
 * Any changes made here must be made in the Makefile and vice versa.
 */

#define ROM_BASE_ADRS    HKBAJA_ROM_BASE_ADRS
#define ROM_SIZE         0x00040000            /* 256K ROM space */

#if (CPU == R4000)

#define INT_LVL_SWITCH   SR_IBIT8
#define INT_LVL_PERR     SR_IBIT7
#define INT_LVL_BERR     SR_IBIT6
#define INT_LVL_PIG      SR_IBIT5
#define INT_LVL_VIC2     SR_IBIT4
#define INT_LVL_VIC1     SR_IBIT3
#define INT_LVL_SW1      SR_IBIT2
#define INT_LVL_SW0      SR_IBIT1

#define TASK_SR          (SR_CU1 | SR_CU0 | (INT_LVL_SW0 | INT_LVL_SW1 | INT_LVL_VIC1 | INT_LVL_BERR | INT_LVL_PERR) | SR_IE)
#define RAM_HIGH_ADRS            0x80180000    /* RAM address for ROM boot */
#define ROM_TEXT_ADRS            ROM_BASE_ADRS   /* with PC, SP, magic */
#else /* (CPU == R4000) */
#define RAM_HIGH_ADRS            0x00090000    /* RAM address for ROM boot */
#define ROM_TEXT_ADRS            (ROM_BASE_ADRS+0x8)   /* with PC, SP, magic */
#endif /* (CPU == R4000) */

#endif  /* __INCconfigh */
