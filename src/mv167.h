/* mv167.h - Motorola MVME167 CPU board header */

/*
modification history
--------------------
01k,22feb93,ccc  removed cast on BBRAM_ADRS -- used in romInit.s.
		 remove BCLK, defined in NV_CPU_SPEED.
01j,02sep92,caf  increased BBRAM_SIZE to include entire battery-backed RAM.
01i,07jul92,eve  changed initialisation for the ncr710Lib driver.
01i,07jul92,ccc  changed genericNvRam.h to memDev.h,
		 changed genericTimer.h to timerDev.h.
01h,29jun92,caf  changed nvRam/genericNvram.h to mem/genericNvRam.h.
01g,29jun92,caf  changed nvram/genericNvram.h to nvRam/genericNvRam.h.
01f,28jun92,caf  added include of genericNvram.h. fixed "MVME167".
01e,25jun92,ccc  added include of genericTimer.h.
01d,19jun92,ccc  added BBRAM_SIZE, added ncr710 info.
01c,26may92,rrr  the tree shuffle
		  -changed includes to have absolute path from h/
01b,14oct91,ccc  fixed #endif. changed includes to have
		 absolute path from h/.
01a,10jun91,ccc	 written from mv147.h version 01h
*/

/*
This file contains I/O addresses and related constants for the
Motorola MVME167.
*/

#ifndef	INCmv167h
#define	INCmv167h

#include "drv/serial/cd2400.h"
#include "drv/multi/pccchip2.h"
#include "drv/multi/memc040.h"
#include "drv/vme/vmechip2.h"
#include "drv/timer/timerDev.h"
#include "drv/mem/memDev.h"
#include "drv/scsi/ncr710.h"

#define BUS		VME_BUS
#define CPU		MC68040

#define N_SIO_CHANNELS	4		/* Number of serial I/O channels */

/* Local I/O address map */

#define	BBRAM_ADRS	(0xfffc0000)		/* MK48T08 battery backup ram */
#define	BBRAM_SIZE	0x1ff8			/* number of bytes for BBRAM  */
#define	BB_ENET		((char *) 0xfffc1f2c)	/* factory ethernet address   */
#define	TOD_CLOCK 	((char *) 0xfffc1ff8)	/* MK48T08 bb time of day clk */

#define	PCC2_BASE_ADRS	(0xfff42000)		/* PCC registers base address */
#define	VMECHIP2_BASE_ADRS (0xfff40000) 	/* VMEchip LCSR registers     */
#define	MPCC_BASE_ADRS	(0xfff45000)		/* CD2401 (Serial Comm Contr) */
#define MEMC_BASE_ADRS	(0xfff43000)		/* MEMC040 memory contorller  */

/* interrupt vector locations */

#define PCC2_INT_VEC_BASE	0x40	/* PCC interrupt vector base number */
					/* any multiple of 0x10             */
#define UTIL_INT_VEC_BASE0	0x50	/* VMEchip2 utility interrupt       */
					/* vector base number               */
					/* any multiple of 0x10             */
#define	UTIL_INT_VEC_BASE1	0x60	/* VMEchip2 utility interrupt       */
					/* vector base number               */
					/* any multiple of 0x10             */

#define INT_VEC_CD2400_A	0x90	/* int vec for channel A */
#define	INT_VEC_CD2400_B	0x94	/* int vec for channel B */
#define INT_VEC_CD2400_C	0x98	/* int vec for channel C */
#define INT_VEC_CD2400_D	0x9c	/* int vec for channel D */

#define	LANC_IRQ_LEVEL		3	/* LANC IRQ level             */
#define	MPCC_IRQ_LEVEL		4	/* serial comm IRQ level      */
#define	SYS_CLK_LEVEL		6	/* interrupt level for sysClk */
#define AUX_CLK_LEVEL		5	/* interrupt level for auxClk */
#define	SCSI_IRQ_LEVEL		2	/* SCSI interrupt level       */

/* board specific registers */

/* 82596CA */

#define	INT_VEC_EI		INT_VEC_LN
#define	EI_SYSBUS		0x6c		/* 82596 SYSBUS value */
						/* IRQ active low     */
#define	EI_POOL_ADRS		NONE

#define	I82596_PORT		((UINT32 *) 0xfff46000)
#define	I82596_CONTROL		((UINT32 *) 0xfff46004)

/* ncr710 */
#define	MV167_SIOP_BASE_ADRS	((UINT8 *) 0xfff47000)
#define	MV167_SIOP_FREQ		((UINT)NCR710_50MHZ)	/* 50MHz SCSI clock */
#define	MV167_SIOP_HW_REGS	{ 0,0,0,1,1,0,0,0,0,0,0,0,0,1,0 }	

#endif	/* INCmv167h */
