/*+
 *	MODULE NAME:
 *	sysextLib
 *
 *	FILENAME:
 *	sysextMemMap.h
 *
 *	PURPOSE:
 *	Additional include file for sysextLib, containing target-specific macro
 *	definitions including those relating to the system's memory map on VME
 *	bus. Edit as necessary to customise for specific applications. NB. One or
 *	other of the macros TARGET_name must have been defined previously (in
 *	sysextLib.h).
 *
 *	The address-map is defined as follows. Each CPU on the bus is allocated a
 *	unique CPU number (set by sysextProcNumSet()) - this is analogous to the
 *	VxWorks CPU number set by sysProcNumSet(). This file contains definitions
 *	of the address occupied by CPU #0, along with the separation between CPUs
 *	on the bus such that the addresses to which any other CPU is mapped can
 *	be obtained by applying simple formulae of the form
 *
 *		"address = base + N*delta"
 *
 *	where base is a base address (defined for CPU #0), N is the CPU number
 *	and delta is the range of addresses reserved for each CPU.
 *
 *	Definitions are given for each CPU in two major modes: (i) as VME bus master
 *	and (ii) as VME bus slave. Furthermore, for each mode there are generally
 *	three VME addressing modes - short, standard and extended (16-, 24- and
 *	32-bit addressing respectively). Each CPU therefore has a definitions as
 *	Master and Slave in each of the three addressing modes. See also sysextLib.h
 *	for some related target-independent definitions.
 *
 *	Care should be taken when changing the address map to ensure that any
 *	changes are compatible with the processor hardware. For example, the
 *	Baja maps VME bus to a FIXED range of local bus addresses so that the macro
 *	MASTER_SHT_LOC_BASE (for example) should not be edited for the Baja,
 *	however it is legitimate to edit this macro for the MVME167.
 *
 *	IMPORTANT:
 *	*** THIS FILE MUST BE MODIFIED TO REFLECT THE ACTUAL MEMORY MAP
 *	*** AT YOUR SITE. SEE DEFINITIONS BELOW.
 *
 *	DEFICIENCIES:
 *	The header files "mv167.h" and "hkbaja.h" need to be included explictly.
 *	Their location and contents can vary from site to site, which means they
 *	need to be copied into the current directory and included with the software
 *	release. It would be better if these files could be picked up from the VxWorks
 *	/config directory. SMB - 20 Feb 98.
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.4  1998/12/07 11:17:24  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.3  1998/03/05 14:20:14  smb
 * Merged with changes made by Bret Goodrich
 *
 * Revision 1.2  1998/02/23 13:38:55  smb
 * Rearranged code for printability
 *
 * Revision 1.1.1.1  1997/11/28 11:46:18  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

#ifndef	__INCsysextMemMaph
#define	__INCsysextMemMaph

/* Must define one of the TARGET_name macros first... */

#if CPU == MC68040							/* Assume that MC68040 CPU is MVME167 target board	*/
#define	TARGET_MV167
#elif CPU == R4000
#define TARGET_HKBAJA47						/* Assume that R4000 CPU is Baja47 target board		*/
#endif /* CPU == MC68040, CPU == R4000 */

#if ! defined (TARGET_MV167) && ! defined (TARGET_HKBAJA47)
#error Target architecture is not supported. Must define either TARGET_MV167 or TARGET_HKBAJA47.
#endif

/* includes */

#include <cacheLib.h>

/* target-specific includes */

#ifdef	TARGET_MV167
#include <mv167.h>
#include <drv/vme/vmechip2.h>
#endif	/* TARGET_MV167 */

#ifdef	TARGET_HKBAJA47
#include <hkbaja.h>
#include <drv/vme/vic068.h>
#endif	/* TARGET_HKBAJA47 */

	/*
	 * Glossary:
	 *
	 * The term "BLT" is shorthand for "VME block transfer".
	 * AM stands for "addressing mode", which can be short (sht or A16), standard (std or A24)
	 * or extended (ext or A32).
	 * A "master" refers to a VME bus address and a "slave" refers to a local address.
	 */


/* target-specific defines: Motorola MVME167 */

#ifdef	TARGET_MV167

#define	BLT_MAX_TIME_ON_BUS		TIMEOUTCR_ON_16US	/* VMECHIP2's max time on bus during BLTs.		*/
													/* Use macros defined in vmechip2.h,			*/
													/* namely: TIMEOUTCR_ON_nnnUS where nn = 16,	*/
													/* 32, 64 etc up to 1024. Alternatively,		*/
													/* TIMEOUTCR_ON_DONE will hold the bus			*/
													/* until the block transfer is done.			*/

#define	BLT_MIN_TIME_OFF_BUS	TIMEOUTCR_OFF_16US	/* VMECHIP2's min time off bus during BLTs.		*/
													/* Use macros defined in vmechip2.h,			*/
													/* namely: TIMEOUTCR_OFF_nnnUS where			*/
													/* nn = 16, 32, 64 etc up to 1024.				*/

#define	DEFAULT_MASTER_SHT_LOC_BASE	0xffff0000		/* Default local base address for Master,		*/
													/* short AM										*/

#define	SLAVE_SHT_VME_BASE		0x00000000			/* VME base address for CPU #0, Slave,			*/
													/* short AM	(address 0xXXXXXXFX is reserved		*/
													/* for the location monitors).					*/

#define	SLAVE_SHT_LOC_BASE		0xfff40100			/* Local base address for CPU #0 Slave,			*/
													/* short AM.									*/

#define	SLAVE_SHT_LOC_TOP		0xfff4011f			/* Local top address for CPU #0 Slave,			*/
													/* short AM.									*/

#define	SLAVE_STD_VME_BASE		0x00800000			/* VME base address for CPU #0, standard AM.	*/

#define	SLAVE_STD_LOC_BASE		0x00000000			/* Local base address for CPU #0 Slave,			*/
													/* standard AM.									*/

#define	SLAVE_STD_LOC_TOP		0x001fffff			/* Local top address for CPU #0 Slave,			*/
													/* standard AM.									*/

#define	SLAVE_EXT_VME_BASE		0xc0000000			/* VME base address for CPU #0, extended AM.	*/

#define	SLAVE_EXT_LOC_BASE		0x00000000			/* Local base address for CPU #0 Slave,			*/
													/* extended AM.									*/

#define	SLAVE_EXT_WP_ENABLE		TRUE				/* Enable write-posting for slaves.				*/

#define	MASTER_SHT_LOC_BASE		0xffff0000			/* Local base address for CPU #0, Master,		*/
													/* short AM. Usually (but not necessarily) set	*/
													/* equal to DEFAULT_MASTER_SHT_LOC_BASE.		*/

#define	MASTER_STD_LOC_BASE		0xfe800000			/* Local base address for CPU #0. Master,		*/
													/* standard AM.									*/

#define	MASTER_EXT_LOC_BASE		0x04000000			/* Local base address for CPU #0 Master,		*/
													/* extended AM (reserve first 64 MB for	local	*/
													/* RAM).										*/

#define	MASTER_EXT_LOC_TOP		0xfe7fffff			/* Local top address for CPU #0, Master,		*/
													/* extended AM.									*/

#define	MASTER_EXT_WP_ENABLE	TRUE				/* Enable write-posting for masters.			*/

#endif	/* TARGET_MV167 */


/* more target-specific defines: Heurikon Baja 4700 */

#ifdef	TARGET_HKBAJA47

#define	BLT_BURST_LENGTH		64					/* VIC64's number of cycles per BLT burst. Must	*/
													/* be a power of 2 in range 1 to 64.			*/

#define	BLT_INTERLEAVE_PERIOD	1					/* VIC64's interleave period. Must be 1 to 15.	*/

#define	SLAVE_SHT_VME_BASE		0x00000000			/* VME base address for CPU #0, Slave, 			*/
													/* short AM.									*/

#define	SLAVE_SHT_LOC_BASE		0x00000000			/* Local base address for CPU #0 Slave, 		*/
													/* short AM.									*/

#define	SLAVE_SHT_LOC_TOP		0x0000002f			/* Local top address for CPU #0 Slave,			*/
													/* short AM (do not change)						*/

#define	SLAVE_STD_VME_BASE		0x00800000			/* VME base address for CPU #0, standard AM.	*/

#define	SLAVE_STD_LOC_BASE		0x00000000			/* Local base address for CPU #0 Slave, 		*/
													/* standard AM.									*/

#define	SLAVE_STD_LOC_TOP		0x001f0000			/* Local top address for CPU #0 Slave,			*/
													/* standard AM (do not change)					*/

#define	SLAVE_EXT_VME_BASE		0xc0000000			/* VME base address for CPU #0, extended AM.	*/

#define	SLAVE_EXT_LOC_BASE		0x00000000			/* Local base adrs for CPU #0, Slave,			*/
													/* extended AM.									*/

#define	SLAVE_EXT_WP_ENABLE		TRUE				/* Enable write-posting for slaves.				*/

#define	MASTER_SHT_LOC_BASE	CACHE_DMA_VIRT_TO_PHYS (HKBAJA_VME_SHORT_ADRS)
													/* Local base address for CPU #0, Master,		*/
													/* short AM. Do not change.						*/

#define	MASTER_STD_LOC_BASE	CACHE_DMA_VIRT_TO_PHYS (HKBAJA_VME_STANDARD_ADRS)
													/* Local base address for CPU #0, Master,		*/
													/* standard AM. Do not change.					*/

#define	MASTER_EXT_LOC_BASE		0xc0000000			/* Local base address for CPU #0, Master,		*/
													/* extended AM.									*/

#define	MASTER_EXT_LOC_TOP		0xfbffffff			/* Local top address for CPU #0, Master,		*/
													/* extended AM.									*/

#define	MASTER_EXT_VME_BASE		MASTER_EXT_LOC_BASE	/* Local base address for CPU #0 Master,		*/
													/* extended AM. Do not change. 					*/

#define	MASTER_EXT_VME_TOP		MASTER_EXT_LOC_TOP	/* VME top address for CPU #0 Master,			*/
													/* extended AM. Do not change. 					*/

#define	MASTER_EXT_WP_ENABLE	TRUE				/* Enable write-posting for masters.			*/

#endif	/* TARGET_HKBAJA47 */

#endif	/* __INCsysextMemMaph */
