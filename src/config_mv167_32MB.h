/* mv167/config.h - Motorola MVME167 configuration header */

/* Copyright 1984-1993 Wind River Systems, Inc. */

/*
modification history
--------------------
9sep98,smb (ROE) increase LOCAL_MEM_SIZE from 16 to 32 Mbytes.
4jun98,smb (ROE) increase LOCAL_MEM_SIZE from 4 to 16 Mbytes.
6may98,smb (ROE) include INCLUDE_SECURITY
9sep97,smb (ROE) increase NUM_FILES from 50 to 100.
9sep97,smb (ROE) include additional POSIX facilities from Gemini config.h
9sep97, ab (ROE) include all POSIX support
2sep96, ab (ROE) include NFS client support
01x,22feb93,ccc  added NV_CPU_SPEED constant for board speed,
		 removed SYS_CPU_FREQ (not used).
01w,13oct92,jcf  configured cache for shared memory facilities.
01v,31oct92,caf  changed LOCAL_MEM_SIZE back to 0x400000 for ROMs (SPR 1729).
		 increased ROM_SIZE to 0x00080000, RAM_HIGH_ADRS to 0x00100000.
01u,29oct92,caf  increased ROM_SIZE to 0x00040000.
01t,22oct92,caf  changed LOCAL_MEM_SIZE and the associated comment.
01s,22oct92,ccc  added INCLUDE_DOSFS for SCSI.
01r,16oct92,ccc  changed note about make dependancy.
01q,01sep92,jcf  cache configuration, changed to INCLUDE_MMU_BASIC.
01p,02sep92,caf  changed NV_RAM constants.
01o,19aug92,rdc  changed INCLUDE_MMU to INCLUDE_BASIC_MMU_SUPPORT.
01n,31jul92,ccc  removed redefine of IO_ADRS_ENP and IO_ADRS_EX.
01m,30jul92,pme  Added shared memory objects #define's.
01l,25jul92,elh  changed BP macros to SM.
01k,08jul92,rdc  temporary addition of INCLUDE_MMU.
01j,28jun92,caf  changed NV_BOOT_LINE.
01i,19jun92,ccc  added clock min/max values, board GCSR group number.
01h,26may92,rrr  the tree shuffle
01g,14oct91,ccc  remove LOCAL_MEM_LOCAL_ADRS, now configured
		 dynamically in sysProcNumSet().
01f,15sep91,jpb  redefined IO_AM_EX_MASTER for 32-bit Excelan access.
01e,27aug91,shl  changed RAM_TEXT_HIGH_ADRS to RAM_HIGH_ADRS.
01d,15aug91,ccc  made BP_OFF_BOARD = FALSE - use local ram for bp.
01c,12aug91,ccc  changed memory size to 4MB, bootroms will work on all boards.
01b,31jul91,ccc  fixed BAUD_CLK_FREQ constant.
01a,10jun91,ccc  derived from mv147/config.h.
*/

/*
This file contains the configuration parameters for the
Motorola MVME167.
*/

#ifndef	INCconfigh
#define	INCconfigh

#include "configAll.h"
#include "mv167.h"

/* NFS client support */
#define INCLUDE_NFS

/* Remote debugging facilities */

#define INCLUDE_RDB

/* Posix facilities */

#define INCLUDE_POSIX_ALL

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

/* I/O system parameters */

#define NUM_FILES		100	/* max 100 files open simultaneously (changed from 50, nd14, 12/5/97) */

#define INCLUDE_SECURITY	/* shell security for network access */

/* Login security initial user name and password.
 * Use vxencrypt on host to find encrypted password.
 */
 
#ifdef  INCLUDE_SECURITY
#define LOGIN_USER_NAME         "cics"
#define LOGIN_PASSWORD          "SRebzSdyzS"
#endif  /* INCLUDE_SECURITY */

#define DEFAULT_BOOT_LINE \
"ei(0,0)idsun1:/net/alba/sw2/vw5.2/config/gemag_mv167/vxWorks h=195.194.120.4 e=195.194.120.5:ffffff00 u=cics"

#define	INCLUDE_MMU_BASIC	/* bundled mmu support */

/*
 * Device controller I/O addresses:
 */

#define	INCLUDE_EI	/* include 82596 driver */

#if	FALSE			/* change FALSE to TRUE for SCSI interface */
#define	INCLUDE_SCSI		/* include ncr710 driver */
#define	INCLUDE_SCSI_BOOT	/* include ability to boot from SCSI */
#define	INCLUDE_DOSFS		/* file system to be used */
#endif	/* FALSE/TRUE */

/* Interrupt vectors */

#define INT_VEC_ACFAIL          (UTIL_INT_VEC_BASE0 + LBIV_VME_ACFAIL)
#define INT_VEC_ABORT           (UTIL_INT_VEC_BASE0 + LBIV_ABORT)

#define INT_VEC_SCSI            (PCC2_INT_VEC_BASE + PCC2_INT_SCSI)
#define INT_VEC_CLOCK           (PCC2_INT_VEC_BASE + PCC2_INT_TT1)
#define INT_VEC_AUX_CLOCK       (PCC2_INT_VEC_BASE + PCC2_INT_TT2)
#define INT_VEC_LN              (PCC2_INT_VEC_BASE + PCC2_INT_LANC)
#define	INT_VEC_LN_ERR		(PCC2_INT_VEC_BASE + PCC2_INT_LANC_ERR)

/* Miscellaneous definitions */

#define NV_RAM_SIZE     BBRAM_SIZE      /* 8184 bytes */
#define NV_RAM_ADRS     ((char *) BBRAM_ADRS)
#undef	BOOT_LINE_SIZE
#define	BOOT_LINE_SIZE	254		/* allow room for board speed */
#define NV_BOOT_LINE    (NV_RAM_ADRS + NV_BOOT_OFFSET)
#define	NV_CPU_SPEED	(BBRAM_ADRS + NV_BOOT_OFFSET + BOOT_LINE_SIZE)

#define BAUD_CLK_FREQ	20000000	/* 20 MHz baud rate "P Clock" (fixed) */

#undef	NUM_TTY
#define	NUM_TTY		N_SIO_CHANNELS

#define SYS_CLK_RATE_MIN  3             /* minimum system clock rate */
#define SYS_CLK_RATE_MAX  5000          /* maximum system clock rate */
#define AUX_CLK_RATE_MIN  3             /* minimum auxiliary clock rate */
#define AUX_CLK_RATE_MAX  5000          /* maximum auxiliary clock rate */

/* Backplane network parameters */

#define	GCSR_GROUP_ADDR	(0xcc)		/* recommended value in manual */
#define SM_INT_TYPE	SM_INT_MAILBOX_1        /* (SIGHP) interrupt */
#define SM_INT_ARG1	VME_AM_SUP_SHORT_IO     /* bus address space */
#define SM_INT_ARG2	((GCSR_GROUP_ADDR << 8) + (sysProcNumGet() << 4) + 2)
						/* bus address */
#define SM_INT_ARG3	0x08			/* value (SIGHP bit) */

/* the backplane master (usually cpu 0) also needs to know the following
 * shared memory pool parameters. */

/* SM_OFF_BOARD is initially defined as TRUE to indicate that the
 * mv167's memory is not used for the backplane anchor.  This differs
 * from traditional VxWorks board support packages which have SM_OFF_BOARD
 * originally defined as FALSE.  (This change has been made so that the
 * local memory of the mv167 will be subject to copyback mode caching
 * by default.)
 *
 * If the user is using the VxWorks backplane driver, make sure you change
 * the definition of SM_OFF_BOARD if you will be using the mv167's memory
 * for the backplane anchor.
 */

#if	FALSE		/* TRUE for SM_OFF_BOARD/FALSE for !SM_OFF_BOARD */
#define	SM_OFF_BOARD	TRUE
#else
#define	SM_OFF_BOARD	FALSE
#endif	/* TRUE/FALSE */

#if	SM_OFF_BOARD
#undef	SM_ANCHOR_ADRS
#define SM_ANCHOR_ADRS	((char *)0xf0800000)	/* off-board anchor address */
#define SM_MEM_ADRS	SM_ANCHOR_ADRS	/* off-board shared memory address */
#define SM_MEM_SIZE	0x00080000	/* 512K */
#define SM_OBJ_MEM_ADRS (SM_MEM_ADRS+SM_MEM_SIZE)/* sh. mem Objects pool adrs */
#define SM_OBJ_MEM_SIZE 0x80000         /* sh. mem Objects pool size 512K */
#else
#define SM_MEM_ADRS	NONE				/* NONE = allocate from memory */
#define SM_MEM_SIZE	0x00010000			/* 64K */
#define SM_OBJ_MEM_ADRS NONE            /* sh. mem Objects pool adrs */
#define SM_OBJ_MEM_SIZE 0x10000         /* sh. mem Objects pool size 64K */
#endif	/* SM_OFF_BOARD */


/* cache configuration */

/*
 * VMEbus snooping is unreliable on the MVME167.  To ensure proper
 * synchronization during shared memory packet exchange, the snooping feature
 * must not be relied upon when using local memory (SM_OFF_BOARD==FALSE). 
 * Rather, the MMU will be utilized to mark the pages of concern as cache
 * inhibited.  When the hardware stabilizes, the CACHE_SNOOP_ENABLE bit will
 * provide even greater network and shared memory object performance.
 */

#if	(defined(INCLUDE_SM_NET) || defined(INCLUDE_SM_OBJ))
#if	SM_OFF_BOARD
#define USER_D_CACHE_MODE	(CACHE_COPYBACK | CACHE_SNOOP_ENABLE)
#define BUS_SNOOP
#else
#undef  USER_D_CACHE_MODE
#define USER_D_CACHE_MODE	(CACHE_COPYBACK)
#endif	/* SM_OFF_BOARD */
#endif	/* (defined(INCLUDE_SM_NET) || defined(INCLUDE_SM_OBJ)) */


/* Memory addresses */

/*
 * Local-to-Bus memory address constants:
 * The MVME167 local memory always appears at address 0 locally;
 * its address on the bus is set by the Slave Base Address Register in the
 * VMECHIP2.  The MVME167 can handle mezzanine boards with memory sizes
 * of 4, 8, 16, or 32Mbytes.  If the board has 4Mbytes of memory then the
 * memory will be mapped to the standard (24-bit) address range starting
 * at 0x800000.
 *
 * If the board has 8Mbytes or more of DRAM then the memory will be mapped to
 * the extended (32-bit) address range as follows:
 *
 *	 8MB memory at 0x01000000
 *	16MB memory at 0x02000000
 *	32MB memory at 0x04000000
 *
 * MVME167 boards with base addresses set to 0x1000000 (16 Mb) or greater
 * may only be accessed in extended (32-bit) addressing mode (i.e. the remote
 * board must use an extended address modifier code).  This corresponds to
 * a MVME167 with 8 Mbytes or more of RAM.  Be sure that any other boards
 * which will access the MVME167's memory use the appropriate addressing mode.
 *
 * To determine the actual memory size use sysMemTop().  The constant
 * LOCAL_MEM_SIZE is used to configure the MMU in sysLib.c.  For optimal use
 * of cache, build VxWorks with LOCAL_MEM_SIZE set to the size of the
 * MVME167's local RAM.
 */

#define LOCAL_MEM_LOCAL_ADRS	0x00000000	/* fixed at zero */
#define LOCAL_MEM_SIZE		0x02000000	/* 32 Mbytes */
						/* CHANGED FROM 4M TO 16M - SMB 4 JUN 98 */
						/* CHANGED AGAIN FROM 16M TO 32M - SMB 8 SEP 98 */

#define	LOCAL_MEM_BUS_A24		0x00800000	/* Bus address in A24 range */

/*
 * The constants ROM_TEXT_ADRS, ROM_SIZE, and RAM_HIGH_ADRS are defined
 * in config.h, MakeSkel, Makefile, and Makefile.*
 * All definitions for these constants must be identical.
 */

#define ROM_BASE_ADRS		0xff800000	/* base address of ROM */
#define ROM_TEXT_ADRS		(ROM_BASE_ADRS+8)       /* with PC & SP */
#define ROM_SIZE			0x00080000	/* 512K ROM space */

#define RAM_HIGH_ADRS		0x00100000	/* RAM address for ROM boot */

#endif	/* INCconfigh */
