static struct {void *v; char *c;} rcsid = {&rcsid,
	"$Id: sysextLib.c,v 1.1.1.1 1999-03-17 03:14:25 cboyer Exp $"};

/*+
 *	MODULE NAME:
 *	sysextLib
 *
 *	FILENAME:
 *	sysextLib.c
 *
 *	DESCRIPTION:
 *	Extensions to the BSP library sysLib. This library contains a variety of
 *	hardware-dependent utility routines, each written for a number of adopted
 *	target CPUs, such that the library can be compiled for each of these
 *	targets.
 *
 *	The library provides support for multiple VxWorks target architectures via
 *	the use of conditionally-compiled code. Macro definitions of the form
 *	TARGET_name (where "name" is shorthand for one of the supported targets)
 *	define which target the code will be compiled for. The following targets
 *	are currently supported
 *
 *	-	TARGET_MV167	Motorola MVME167 board
 *	-	TARGET_HKBAJA	Heurikon Baja 4700 board
 *
 *	The library defines routines such as sysextLocalToBusAdrs() which are direct
 *	replacements for sysLocalToBusAdrs() etc... By default, the equivalent
 *	routines in sysLib are replaced with those provided here. This will
 *	generally be appropriate if sysextProcNumSet() is called since the system's
 *	memory map will then normally be different from that defined in the BSP
 *	issued with each target. The PRESERVE_SYSLIB_FUNCS option can be set to
 *	prevent the sysLib functions from being replaced.
 *
 *	DEVELOPMENT NOTE:
 *	When the HKBAJA version of the library is compiled it generates compiler warnings
 *	stating that the sysVicBlkCopy, sysVicTBlkTune and sysMailboxDisable functions
 *	are not declared. I suspect there is a problem here, and this module should
 *	include different versions of the "sysLib.h" header file depending on the target
 *	it is compiled for. - SMB 15 Dec 97.
 *
 *	Note also that sysextProcNumSet() returns a value, but sysProcNumSet() is a void function,
 *	so functions in this library are not always direct replacements for sysLib functions,
 *	as claimed above. SMB - 8 October 1998.
 *
 *	EXTERNAL MODULES:
 *	errorLib
 *
 *	DEFICIENCIES:
 *	This library is horrible. It rewrites part of the BSP code used by the MVME167 and Heurikon Baja
 *	4700 boards. It makes explicit assumptions about the operation of the hardware, assumes that
 *	an accurate memory map has been manually defined in sysextMemMap.h, and a description of the
 *	VME network defined in wfsSite. The library is prone to mistakes and is unlikely to be portable
 *	to newer versions of the hardware. The use of this library is deprecated, and it will disappear
 *	eventually.
 *
 *	A further note. This library does not work if the mv167 processor is used with any memory
 *	extension cards. SMB - 8 October 1998.
 *
 *	FUNCTION NAME(S):
 *	sysextProcNumSet			-	set CPU number
 *	sysextProcNumGet			-	get CPU number
 *	sysextNProcGet				-	get number of CPUs defined on system's VME bus
 *	sysextLocalToBusAdrs		-	convert a local address to a bus address
 *	sysextBusToLocalAdrs		-	convert a bus address to a local address
 *	sysextMboxConnect			-	connect a routine to the mailbox interrupt
 *	sysextMboxEnable			-	enable mailbox interrupt
 *	sysextMboxDisable			-	disable mailbox interrupt
 *	sysextMboxIntGen			-	generate a mailbox interrupt on a remote CPU
 *	sysextMboxBusAdrs			-	get address of remote CPU's mailbox interrupt register
 *	sysextWriteLocalMboxReg8	-	write 8-bit word to local CPU's mailbox registers
 *	sysextWriteLocalMboxReg32	-	write 32-bit word to local CPU's mailbox registers
 *	sysextReadRemoteMboxReg8	-	read 8-bit word from remote CPU's mailbox registers
 *	sysextReadRemoteMboxReg32	-	read 32-bit word from remote CPU's mailbox registers
 *	sysextVmeBlockInit			-	initialise VME block transfer mode
 *	sysextVmeBlockCopy			-	block transfer data over VME bus
 *	sysextVmeReqRelInit			-	initialise VME request/release mode for local CPU
 *	sysextVmeNetworkInit		-	initialise VME network definition
 *	sysextRebootRemote			-	reboot remote CPU on VME bus
 *	sysextBusReset				-	assert VME system reset line
 *
 *	IGNORED FUNCTION NAME(S):
 *	sysextMboxInt				-	Interrupt service routine for mailbox interrupts
 *	vmeChip2BltIsr				-	Interrupt service routine for VMECHIP2 DMA operations
 *
 *	ORIGINAL AUTHOR:
 *	Nick Dillon
 *
 *	MODIFIED BY:
 *	Steven Beard
 *	Bret Goodrich
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.12  1998/12/07 11:17:23  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.11  1998/10/08 16:22:53  cics
 * More notes and warnings added to comments. This library does not work with the mv167 memory expansion, nor does it work with the Heurikon Baja.
 *
 * Revision 1.10  1998/09/28 08:51:57  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks. Deficiencies warning added.
 *
 * Revision 1.9  1998/09/09 14:35:34  cics
 * Global variables renamed to ensure they are unique
 *
 * Revision 1.8  1998/08/13 09:07:41  smb
 * Added author comment
 *
 * Revision 1.7  1998/03/05 14:20:12  smb
 * Merged with changes made by Bret Goodrich
 *
 * Revision 1.6  1998/02/23 13:38:53  smb
 * Rearranged code for printability
 *
 * Revision 1.5  1998/01/30 15:30:17  smb
 * Fixed some problems uncovered by prolint
 *
 * Revision 1.4  1998/01/16 15:55:33  smb
 * Quell compiler warning about rcsid using anj's idea
 *
 * Revision 1.3  1998/01/06 14:19:01  smb
 * External modules described. Some ERROR_MSG_NONE replaced with error messages
 *
 * Revision 1.2  1997/12/15 12:50:54  smb
 * Quell some compiler warnings for BAJA version
 *
 * Revision 1.1.1.1  1997/11/28 11:46:17  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks (and only then if you are lucky)
#endif	/* vxWorks */

#include <sysLib.h>
#include <vxLib.h>
#include <iv.h>
#include <stdio.h>
#include <semLib.h>
#include <string.h>
#include <intLib.h>
#include <rebootLib.h>
#include <taskLib.h>
#include "sysextLib.h"
#include "sysextMemMap.h"
#include "gemTypes.h"
#include "errorLib.h"


/* defines */

/* #define DEBUG */						/* Define this macro to enable debug messages		*/

#define	VME_AM_SHT							0x20		/* AM for short address space				*/
#define	VME_AM_STD							0x30		/* AM for standard address space			*/
#define	VME_AM_EXT							0x00		/* AM for extended address space			*/
#define	VME_AM_MASK_ALL						0x3f		/* Mask for AM bits							*/
#define	VME_AM_MASK_SPACE					0x30		/* Mask to extract address space from AM	*/
#define	VME_AM_MASK_SUP_BIT					0x04		/* Mask to extract supervisor bit from AM	*/
#define	VME_ADDR_SIZE_SHT					0x00010000	/* # bytes for short address space			*/
#define	VME_ADDR_SIZE_STD					0x01000000	/* # bytes for standard space				*/
#define	VME_ADDR_MASK_SHT	(VME_ADDR_SIZE_SHT - 1)		/* Mask for short address					*/
#define	VME_ADDR_MASK_STD	(VME_ADDR_SIZE_STD - 1)		/* Mask for standard address				*/
#define	MAILBOX_CONTROL_VME_OFFSET_MV167	0x03		/* Offset to VMECHIP2 board-control reg		*/
#define	MAILBOX_CONTROL_VME_OFFSET_HKBAJA47	0x0f		/* Offset to VIC64 ICR7 register			*/
#define	MAILBOX_INTREG_VME_OFFSET_MV167		0x02		/* Offset to VMECHIP2 mailbox interrupt reg	*/
#define	MAILBOX_INTREG0_VME_OFFSET_HKBAJA47	0x21		/* Offset to VIC64 m/box interrupt #0 reg	*/
#define	MAILBOX_REG_VME_INTERVAL_HKBAJA47	0x02		/* Interval between VIC64 interrupt regs	*/
#define	MAILBOX_DATREG0_VME_OFFSET_MV167	0x04		/* Offset to VMECHIP2 data reg #0			*/
#define	MAILBOX_DATREG1_VME_OFFSET_MV167	0x05		/* Offset to VMECHIP2 data reg #0			*/
#define	MAILBOX_DATREG2_VME_OFFSET_MV167	0x06		/* Offset to VMECHIP2 data reg #1			*/
#define	MAILBOX_DATREG3_VME_OFFSET_MV167	0x07		/* Offset to VMECHIP2 data reg #2			*/
#define	MAILBOX_DATREG0_VME_OFFSET_HKBAJA47	0x01		/* Offset to VMECHIP2 data reg #3			*/
#define	MV167_MBOX_INTERRUPT_LEVEL			1			/* Must be 1 to 7 to enable interrupts		*/
#define	MV167_DMAC_INTERRUPT_LEVEL			1			/* Must be 1 to 7 to enable interrupts		*/
#define	MAILBOX_CONTROL_RESET_BIT_MV167		0x80		/* Reset bit in VMECHIP2 board control reg	*/
#define	MAILBOX_CONTROL_RESET_BIT_HKBAJA47	0x40		/* Reset bit in VIC64 ICR7					*/
#define	BUS_RESET_REG_MV167					0xfff40060	/* Bus reset register for MVME167			*/
#define	BUS_RESET_BIT_MV167					0x01800000	/* Reset-Switch-Enable and Bus-Reset bits	*/
#define	MASTER_EXT_VME_BASE		MASTER_EXT_LOC_BASE		/* Do not change							*/
#define	MASTER_EXT_VME_TOP		MASTER_EXT_LOC_TOP		/* Do not change							*/
#define	MASTER_SHT_AM			VME_AM_SUP_SHORT_IO		/* AM to output during short Master cycles	*/
#define	MASTER_STD_AM			VME_AM_STD_SUP_DATA		/* AM to output during std Master cycles	*/
#define	MASTER_EXT_AM			VME_AM_EXT_SUP_DATA		/* AM to output during ext Master cycles	*/


/* bit-set and -clear macros */

#define	BIT_SET(p, d)			{ __typeof__ (* (p)) __temp = (* (p));	\
									* (p) = __temp | (d); }
#define	BIT_CLR(p, d)			{ __typeof__ (* (p)) __temp = (* (p)); \
									* (p) = __temp & ~(d); }


LOCAL int		sysextProcNumber = -1;

#ifdef TARGET_MV167				/* ------ START OF CODE FOR THE MVME167 ------ */
LOCAL SEM_ID	sysextVme2BltSem = NULL;
#endif	/* TARGET_MV167 */		/* ------ END OF CODE FOR THE MVME167 ------ */

LOCAL uint32	pSysextTargetType [SYSEXT_MAX_N_PROC];
LOCAL int		sysextNumProcessor = -1;

/*
 * The following definitions are contained in the Baja47 BSP. They are reproduced here
 * for the MVME167 to support various mailbox routines on this target.
 */

#ifdef	TARGET_MV167		/* ------ START OF CODE FOR THE MVME167 ------ */

/* typedefs */

struct mailbox
	{
		FUNCPTR	sysextMailboxRoutine;					/* Routine called upon interrupt	*/
		int		sysextMailboxArg;						/* Argument passed to routine		*/
		BOOL	sysextMailboxConnected;					/* Mailbox connected state			*/
	};

/* static variables */

LOCAL struct mailbox sysextMailTbl [SYSEXT_NUMBER_OF_MAILBOXES];

/* function declarations */

LOCAL void sysextMboxInt (int mailbox);		/* Why are there two definitions? SMB - 4 Mar 98. */
/* LOCAL void sysextMboxInt (); */

#endif	/* TARGET_MV167 */	/* ------ END OF CODE FOR THE MVME167 ------ */


/*
 * This library defines routines such as sysextLocalToBusAdrs() which are direct
 * replacements for sysLocalToBusAdrs() (etc). An option exists (in fact, this is
 * the default) to replace the equivalent routines in sysLib with those provided
 * here. This will generally be appropriate if sysextProcNumSet() is called
 * since the system's memory map will then normally be different from that defined
 * in the BSP issued with each target.
 */

#ifndef	PRESERVE_SYSLIB_FUNCS

int	sysProcNumGet (void)
{
	return (sysextProcNumGet ());
}

void	sysProcNumSet
	(
	int	procNum
	)
{
	sysextProcNumSet (procNum);	 /* NOTE: Status returned by sysextProcNumSet() ignored. SMB. */
}

STATUS sysLocalToBusAdrs
	(
	int		adrsSpace,
	char *	pLocalAdrs,
	char **	ppBusAdrs
	)
{
	return (sysextLocalToBusAdrs (adrsSpace, pLocalAdrs, ppBusAdrs));
}

STATUS sysBusToLocalAdrs
	(
	int		adrsSpace,
	char *	pBusAdrs,
	char **	ppLocalAdrs
	)
{
	return (sysextBusToLocalAdrs (adrsSpace, pBusAdrs, ppLocalAdrs));
}

#endif /* PRESERVE_SYSLIB_FUNCS */

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextReadRemoteMboxReg8
 *
 *	INVOCATION:
 *	sysextReadRemoteMboxReg8 (processorNumber, pData)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	processorNumber	(int)		number of remote CPU to access
 *	(!)	pData			(uint8 *)	destination for mailbox data
 *
 *	FUNCTION VALUE:
 *	(STATUS)  OK, or ERROR if the remote CPU's mailbox register could not be read.
 *
 *	PURPOSE:
 *	Read 8-bit word from remote CPU's mailbox registers
 *
 *	DESCRIPTION:
 *	This routine reads a single data byte from the mailbox space on a remote CPU
 *	and returns this as an 8-bit unsigned integer.
 *
 *	EXTERNAL VARIABLES:
 *	(>)   pSysextTargetType    (uint32 *)    array defining target types for all CPUs on VME bus
 *
 *	PRIOR REQUIREMENTS:
 *	The array pSysextTargetType must have been previously initialised via a call to
 *	sysextVmeNetworkInit().
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *-
 */

STATUS sysextReadRemoteMboxReg8
	(
	int 	processorNumber,
	uint8 *	pData
	)
{
	HW_REG8 *	pMboxReg;

	switch (pSysextTargetType [processorNumber])
	{

		/*
		 * For each target: compute address of mailbox register on VME bus, convert to local
		 * address then read single byte of data.
		 */

		case (TARGET_TYPE_MV167):
			pMboxReg = (HW_REG8 *) (SYSEXT_SHT_VME_BASE + processorNumber *
						  SYSEXT_SHT_NBYTE_PER_MV167 + MAILBOX_DATREG0_VME_OFFSET_MV167);
			if (sysextBusToLocalAdrs (VME_AM_SUP_SHORT_IO, (char *) pMboxReg, (char **) & pMboxReg)
			    == ERROR)
			{
				ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);
				return (ERROR);
			}
			* pData = * pMboxReg;
			break;

		case (TARGET_TYPE_HKBAJA47):
			pMboxReg = (HW_REG8 *) (SYSEXT_SHT_VME_BASE + processorNumber *
						  SYSEXT_SHT_NBYTE_PER_HKBAJA47 + MAILBOX_DATREG0_VME_OFFSET_HKBAJA47);
			if (sysextBusToLocalAdrs (VME_AM_SUP_SHORT_IO, (char *) pMboxReg, (char **) & pMboxReg)
			    == ERROR)
			{
				ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);
				return (ERROR);
			}
			* pData = * pMboxReg;
			break;

		default:
			ERROR_SET (S_sysextLib_INVALID_TARGET, "Invalid target architecture", ERROR_LOG_SAVE);
			return (ERROR);
	}

	return (OK);
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextReadRemoteMboxReg32
 *
 *	INVOCATION:
 *	sysextReadRemoteMboxReg32 (processorNumber, pData)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	processorNumber	(int)		number of remote CPU to access
 *	(!)	pData			(uint32 *)	destination for mailbox data
 *
 *	FUNCTION VALUE:
 *	(STATUS)  OK, or ERROR if the remote CPU's mailbox registers could not be read.
 *
 *	PURPOSE:
 *	Read 32-bit word from remote CPU's mailbox registers
 *
 *	DESCRIPTION:
 *	This routine reads 4 data bytes from the mailbox space on a remote CPU
 *	and assembles these data into a 32-bit unsigned integer.
 *
 *	EXTERNAL VARIABLES:
 *	(>)	pSysextTargetType	(uint32 *)	array defining target types for all CPUs on VME bus
 *
 *	PRIOR REQUIREMENTS:
 *	The array pSysextTargetType must have been previously initialised via a call to
 *	sysextVmeNetworkInit().
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *-
 */

STATUS sysextReadRemoteMboxReg32
	(
	int 		processorNumber,
	uint32 *	pData
	)
{
	HW_REG8 *	pMboxReg;
	FAST int	i;

	switch (pSysextTargetType [processorNumber])
	{

		/*
		 * For each target: compute address of first mailbox register on VME bus, convert
		 * to local address then read four bytes of data from successive registers in
		 * the order most-significant byte first, leas-significant byte last.
		 */

		case (TARGET_TYPE_MV167):
			pMboxReg = (HW_REG8 *) (SYSEXT_SHT_VME_BASE + processorNumber *
						  SYSEXT_SHT_NBYTE_PER_MV167 + MAILBOX_DATREG0_VME_OFFSET_MV167);
			if (sysextBusToLocalAdrs (VME_AM_SUP_SHORT_IO, (char *) pMboxReg, (char **) & pMboxReg)
			    == ERROR)
			{
				ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);
				return (ERROR);
			}
			* pData = (uint32) (* pMboxReg) << 24;
			pMboxReg += MAILBOX_DATREG1_VME_OFFSET_MV167 - MAILBOX_DATREG0_VME_OFFSET_MV167;
			* pData |= (uint32) (* pMboxReg) << 16;
			pMboxReg += MAILBOX_DATREG2_VME_OFFSET_MV167 - MAILBOX_DATREG1_VME_OFFSET_MV167;
			* pData |= (uint32) (* pMboxReg) << 8;
			pMboxReg += MAILBOX_DATREG3_VME_OFFSET_MV167 - MAILBOX_DATREG2_VME_OFFSET_MV167;
			* pData |= (uint32) (* pMboxReg);
			break;

		case (TARGET_TYPE_HKBAJA47):
			pMboxReg = (HW_REG8 *) (SYSEXT_SHT_VME_BASE + processorNumber *
						  SYSEXT_SHT_NBYTE_PER_HKBAJA47 + MAILBOX_DATREG0_VME_OFFSET_HKBAJA47);
			if (sysextBusToLocalAdrs (VME_AM_SUP_SHORT_IO, (char *) pMboxReg, (char **) & pMboxReg)
			    == ERROR)
			{
				ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);
				return (ERROR);
			}
			for (i = 24, * pData = 0; i >= 0; i-=8)
			{
				* pData |= (uint32) (* pMboxReg) << i;
				pMboxReg += MAILBOX_REG_VME_INTERVAL_HKBAJA47;
			}
			break;

		default:
			ERROR_SET (S_sysextLib_INVALID_TARGET, "Invalid target architecture", ERROR_LOG_SAVE);
			return (ERROR);
	}

	return (OK);
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextBusReset
 *
 *	INVOCATION:
 *	sysextBusReset ()
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	None
 *
 *	FUNCTION VALUE:
 *	(STATUS)  OK, or ERROR if the target architecture does not support the
 *	bus-reset function or if pSysextTargetType has not been initialised.
 *
 *	PURPOSE:
 *	Assert VME-bus reset signal, *sysrst
 *
 *	DESCRIPTION:
 *	This routine asserts the VME bus system reset signal, thus causing a complete
 *	re-boot of the local CPU and of all other devices sharing the same VME bus.
 *
 *	EXTERNAL VARIABLES:
 *	(>)	pSysextTargetType	(uint32 *)	array defining target types for all CPUs on VME bus
 *
 *	PRIOR REQUIREMENTS:
 *	The array pSysextTargetType must have been previously initialised via a call to
 *	sysextVmeNetworkInit().
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *-
 */

STATUS sysextBusReset (void)
{
	int	processorNumber;

	if ((processorNumber = sysextProcNumGet ()) < 0)
	{
		ERROR_SET (S_sysextLib_INVALID_CPU_NUMBER, "Invalid processor number", ERROR_LOG_SAVE);
		return (ERROR);
	}
	if (pSysextTargetType [processorNumber] != TARGET_TYPE_MV167)		/* Only the MVME167 currently		*/
																/* allows bus-reset to be asserted	*/
	{
		ERROR_SET (S_sysextLib_UNSUPPORTED_ON_TARGET, "sysextLib function not supported",
		           ERROR_LOG_SAVE);
		return (ERROR);
	}

	printf ("sysextBusReset: This system is going down immediately !\n");

	/* Brief pause to allow printf() to flush..	*/
	taskDelay (sysClkRateGet ());

	/* ..then waggle the hardware bits	*/
	BIT_SET ((HW_REG32 *) BUS_RESET_REG_MV167, BUS_RESET_BIT_MV167);

	return (OK);
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextRebootRemote
 *
 *	INVOCATION:
 *	sysextRebootRemote (processorNumber)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	processorNumber	(int)	processor number for remote CPU to re-boot.
 *
 *	FUNCTION VALUE:
 *	(STATUS)  OK, or ERROR if the target architecture does not support the
 *	bus-reset function or if the reset operation failed.
 *
 *	PURPOSE:
 *	Assert re-boot signal on remote CPU via direct access over VME bus
 *
 *	DESCRIPTION:
 *	This routine accesses a control register mapped into a remote CPU's
 *	address space (normally in the mailbox, or short, space) in order to force
 *	the remote CPU to re-boot. The local CPU is not re-booted.
 *
 *	EXTERNAL VARIABLES:
 *	(>)	pSysextTargetType	(uint32 *)	array defining target types for all CPUs on VME bus
 *
 *	PRIOR REQUIREMENTS:
 *	The array pSysextTargetType must have been previously initialised via a call to
 *	sysextVmeNetworkInit().
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *	rebootLib.h
 *
 *	DEFICIENCIES:
 *	None
 *-
 */

STATUS	sysextRebootRemote
	(
	int 	processorNumber
	)
{
	char *	pResetReg;
	char	data;
	STATUS	status;

	/* Reboot this CPU if requested */

	if ((processorNumber == sysextProcNumGet ()) && (processorNumber >= 0))
	{
		printf ("sysextRebootRemote: This system is going down immediately !\n");
		taskDelay (sysClkRateGet ());					/* Brief pause to allow printf() to flush..	*/
		reboot (BOOT_NORMAL);							/* ..then call the reboot function			*/
		return (OK);									/* Never get here ! 						*/
	}

	/*
	 * This must be a request to reboot a remote CPU. Switch on the remote CPU's target
	 * type in order to determine what to do in order to reset it. For each target,
	 * compute the address of the "reset register" (a register in the VMECHIP2 or VIC64
	 * in the case of MVME167 and Baja targets), then set the reset bit in this register.
	 */

	switch (pSysextTargetType [processorNumber])
	{
		case TARGET_TYPE_MV167:

			pResetReg = (char *) (SYSEXT_SHT_VME_BASE +
				processorNumber * SYSEXT_SHT_NBYTE_PER_MV167 + MAILBOX_CONTROL_VME_OFFSET_MV167);
			if (sysextBusToLocalAdrs (VME_AM_SHT, pResetReg, & pResetReg) == ERROR)
			{
				ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);
				return (ERROR);
			}
			data = MAILBOX_CONTROL_RESET_BIT_MV167;
			status = vxMemProbe (pResetReg, VX_WRITE, 1, & data);
			break;

		case TARGET_TYPE_HKBAJA47:

			pResetReg = (char *) (SYSEXT_SHT_VME_BASE + processorNumber
				* SYSEXT_SHT_NBYTE_PER_HKBAJA47 + MAILBOX_CONTROL_VME_OFFSET_HKBAJA47);
			if (sysextBusToLocalAdrs (VME_AM_SHT, pResetReg, & pResetReg) == ERROR)
			{
				ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);
				return (ERROR);
			}

			/* Note that the BAJA requires its reset bit to be toggled from 1 back to 0 again. */

			data = MAILBOX_CONTROL_RESET_BIT_HKBAJA47;
			status = vxMemProbe (pResetReg, VX_WRITE, 1, & data);
			data = 0;
			status = vxMemProbe (pResetReg, VX_WRITE, 1, & data);
			break;

		default:

			ERROR_SET (S_sysextLib_INVALID_TARGET, "Invalid target architecture", ERROR_LOG_SAVE);
			return (ERROR);
	}

	return (status);
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextNProcGet
 *
 *	INVOCATION:
 *	sysextNProcGet ()
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	None
 *
 *	FUNCTION VALUE:
 *	(int)  The number of CPUs defined on the system's VME bus, or -1 if
 *	the network has not been defined.
 *
 *	PURPOSE:
 *	Return the number of CPUs known to reside on the system's VME bus
 *
 *	DESCRIPTION:
 *	This routine returns the number of CPUs that have been defined on
 *	the system's VME bus. This definition is assumed to have been performed
 *	via the routine sysextVmeNetworkInit(); this does not necessarily mean
 *	that all defined CPUs have performed the initialisation necessary for inter-CPU
 *	communications.
 *
 *	EXTERNAL VARIABLES:
 *	(>)   sysextNumProcessor    (int)    number of CPUs defined on VME bus
 *
 *	PRIOR REQUIREMENTS:
 *	The routine sysextVmeNetworkInit() must have been previously called in
 *	order to define sysextNumProcessor.
*
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *-
 */

int	sysextNProcGet (void)
{
	return (sysextNumProcessor);			/* NB. sysextNumProcessor is initialised to -1 */
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextVmeNetworkInit
 *
 *	INVOCATION:
 *	sysextVmeNetworkInit (numProcessor, pSysextTargetTypeIn)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	numProcessor	(int)		number of CPUs to define on VME bus
 *	(>)	pSysextTargetTypeIn	(uint32 *)	target types for each CPU on VME bus
 *
 *	FUNCTION VALUE:
 *	(STATUS)  OK, or ERROR if any of the target types defined in pSysextTargetTypeIn
 *	are not recognised.
 *
 *	PURPOSE:
 *	Define CPU target types on system's VME bus
 *
 *	DESCRIPTION:
 *	This routine initialises sysextLib with a definition of the target type
 *	for each CPU on the system's VME bus. It must be called before a target-dependent
 *	functions (e.g. shared-memory or mailbox accesses) may be executed by the
 *	local CPU on any other CPU on the bus.
 *
 *	EXTERNAL VARIABLES:
 *	(!)	pSysextTargetType			(uint32 *)	array defining target types for all CPUs on VME bus
 *	(!)	sysextNumProcessor	(int)		number of CPUs defined on VME bus
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *-
 */

STATUS sysextVmeNetworkInit
	(
	int			numProcessor,
	uint32 *	pSysextTargetTypeIn
	)
{
	FAST int	processor;

	if (numProcessor < 1 || numProcessor > SYSEXT_MAX_N_PROC)
	{
		ERROR_SET (S_sysextLib_INV_NUM_CPU_IN_NETWORK, "Invalid # of CPUs in network", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/*
	 * Network initialisation simply involves extracting the CPU board target architectures from
	 * the array pSysextTargetTypeIn[] and copying these to the static array pSysextTargetType[]. Each target
	 * type is checked to ensure that it is one of those supported. Finally, the total number of
	 * CPU boards in the system is copied to sysextNumProcessor.
	 */

	for (processor = 0; processor < numProcessor; processor++)
	{
		if ((pSysextTargetTypeIn [processor] != TARGET_TYPE_MV167) &&
		    (pSysextTargetTypeIn [processor] != TARGET_TYPE_HKBAJA47))
		{
			ERROR_SET (S_sysextLib_INVALID_TARGET, "Invalid target architecture", ERROR_LOG_SAVE);
			return (ERROR);
		}
		pSysextTargetType [processor] = pSysextTargetTypeIn [processor];
	}

	sysextNumProcessor = numProcessor;
	return (OK);
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextMboxBusAdrs
 *
 *	INVOCATION:
 *	sysextMboxBusAdrs (mailbox, ppMboxAdrs, pMboxData)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	mailbox		(int)		mailbox interrupt number
 *	(!)	ppMboxAdrs	(char **)	VME bus address of mailbox interrupt register
 *	(!)	pMboxData	(char *)	data to write to interrupt register to assert interrupt
 *
 *	FUNCTION VALUE:
 *	(STATUS)  OK, or ERROR if either the CPU number or mailbox number are invalid.
 *
 *	PURPOSE:
 *	Get address of a mailbox interrupt register on VME bus
 *
 *	DESCRIPTION:
 *	This routine returns the address on VME bus that must be accessed in order to
 *	assert a specified mailbox interrupt on the local CPU. Some targets map different
 *	mailbox interrupts to different addresses, whilst others use a single address but
 *	require that the data written to this address defines the interrupt number to be asserted.
 *	Both approaches are supported by this routine since it returns the mailbox data as
 *	well as the mailbox address.
 *
 *	EXTERNAL VARIABLES:
 *	(!)	pSysextTargetType			(uint32 *)	array defining target types for all CPUs on VME bus
 *	(!)	sysextNumProcessor	(int)		number of CPUs defined on VME bus
 *
 *	PRIOR REQUIREMENTS:
 *	sysextProcNumSet() must have been previously called in order to define the
 *	mapping of this CPU's memory onto VME bus.
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *-
 */

STATUS	sysextMboxBusAdrs
	(
	int		mailbox,
	char **	ppMboxAdrs,
	char *	pMboxData
	)
{
	int	processorNumber;

	if ((processorNumber = sysextProcNumGet ()) < 0)
	{
		ERROR_SET (S_sysextLib_INVALID_CPU_NUMBER, "Invalid processor number", ERROR_LOG_SAVE);
		return (ERROR);
	}
	else if (mailbox < 0 || mailbox >= SYSEXT_NUMBER_OF_MAILBOXES)
	{
		ERROR_SET (S_sysextLib_INVALID_MAILBOX_NUMBER, "Invalid mailbox IRQ channel #",
		           ERROR_LOG_SAVE);
		return (ERROR);
	}

	/*
	 * For each target type, compute the address of the specified mailbox interrupt
	 * channel on VME bus and the data that must be written to this address in order
	 * to assert an interrupt.
	 */

#ifdef	TARGET_TYPE_MV167		/* ------ START OF CODE FOR THE MVME167 ------ */
	* ppMboxAdrs = (char *) (SYSEXT_SHT_VME_BASE + processorNumber *
		SYSEXT_SHT_NBYTE_PER_MV167 + MAILBOX_INTREG_VME_OFFSET_MV167);
	* pMboxData = 1 << mailbox;
#else							/* ------ END OF CODE FOR THE MVME167 ------ */
	* ppMboxAdrs = (char *) (SYSEXT_SHT_VME_BASE + processorNumber *
		SYSEXT_SHT_NBYTE_PER_HKBAJA47 + MAILBOX_INTREG0_VME_OFFSET_HKBAJA47 +
		mailbox * MAILBOX_REG_VME_INTERVAL_HKBAJA47);
	* pMboxData = 1;
#endif /* TARGET_TYPE_MV167 */	/* ------ END OF CODE FOR THE HKBAJA47 ------ */

	return (OK);
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextMboxIntGen
 *
 *	INVOCATION:
 *	sysextMboxIntGen (processorNumber, mailbox)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	processorNumber	(int)	number of remote CPU to interrupt
 *	(>)	mailbox			(int)	mailbox interrupt number on remote CPU
 *
 *	FUNCTION VALUE:
 *	(STATUS)  OK, or ERROR if the mailbox interrupt could not be generated.
 *
 *	PURPOSE:
 *	Issue mailbox interrupt to remote CPU
 *
 *	DESCRIPTION:
 *	This routine issues a mailbox interrupt to a remote CPU by writing
 *	to the target CPU's mailbox register.
 *
 *	EXTERNAL VARIABLES:
 *	(!)	pSysextTargetType	(uint32 *)	array defining target types for all CPUs on VME bus
 *
 *	PRIOR REQUIREMENTS:
*	The array pSysextTargetType must have been previously initialised via a call to
 *	sysextVmeNetworkInit().
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *-
 */

STATUS sysextMboxIntGen
	(
	int	processorNumber,
	int	mailbox
	)
{
	volatile char * pMbox;

	if (mailbox < 0 || mailbox >= SYSEXT_NUMBER_OF_MAILBOXES)
	{
		ERROR_SET (S_sysextLib_INVALID_MAILBOX_NUMBER, "Invalid mailbox IRQ channel #",
		           ERROR_LOG_SAVE);
		return (ERROR);
	}

	/*
	 * Switch on the target type of the CPU to be interrupted. For each target, compute the
	 * address of the mailbox interrupt register on VME bus then write the data necessary
	 * to cause an interrupt.
	 */

	switch (pSysextTargetType [processorNumber])
	{
		case TARGET_TYPE_MV167:
			pMbox = (volatile char *) (SYSEXT_SHT_VME_BASE + processorNumber
					* SYSEXT_SHT_NBYTE_PER_MV167 + MAILBOX_INTREG_VME_OFFSET_MV167);
			if (sysextBusToLocalAdrs (VME_AM_SHT, (char *) pMbox, (char **) & pMbox) == ERROR)
			{
				return (ERROR);
			}
			* pMbox = 1 << mailbox;
			break;

		case TARGET_TYPE_HKBAJA47:
			pMbox = (volatile char *) (SYSEXT_SHT_VME_BASE + processorNumber
				* SYSEXT_SHT_NBYTE_PER_HKBAJA47 + MAILBOX_INTREG0_VME_OFFSET_HKBAJA47
				+ mailbox * MAILBOX_REG_VME_INTERVAL_HKBAJA47);
			if (sysextBusToLocalAdrs (VME_AM_SHT, (char *) pMbox, (char **) & pMbox) == ERROR)
			{
				return (ERROR);
			}
			* pMbox = 1;
			break;

		default:
			ERROR_SET (S_sysextLib_INVALID_TARGET, "Invalid target architecture", ERROR_LOG_SAVE);
			return (ERROR);
	}

	return (OK);
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextLocalToBusAdrs
 *
 *	INVOCATION:
 *	sysextLocalToBusAdrs (adrsSpace, pLocalAdrs, ppBusAdrs)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	adrsSpace	(int)		bus address space in which busAdrs resides
 *	(>)	pLocalAdrs	(char *)	local address to convert
 *	(!)	ppBusAdrs	(char **)	where to return bus address
 *
 *	FUNCTION VALUE:
 *	(STATUS)  OK, or ERROR if the memory map is undefined or inconsistent
 *	with the local address.
 *
 *	PURPOSE:
 *	Convert a local address to a bus address
 *
 *	DESCRIPTION:
 *	This routine gets the bus address that accesses a specified local memory address.
 *
 *	NOTE:
 *	Identifiers SLAVE_SHT_LOC_BASE, SLAVE_SHT_LOC_TOP and SLAVE_SHT_VME_BASE come from
 *	"sysextMemMap.h"
 *
 *	EXTERNAL VARIABLES:
 *	(>)	sysextProcNumber	(int)	processor number for this CPU
 *
 *	PRIOR REQUIREMENTS:
 *	sysextProcNumSet() must have been previously called in order to define the
 *	mapping of this CPU's memory onto VME bus.
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *	sysextMemMap.h
 *
 *	DEFICIENCIES:
 *	None
 *
 *	NOTES:
 *	This function claimed only to use the pLocalAdrs pointer as an input, and yet it was
 *	modifying it. I have changed the function to use a local copy of pLocalAdrs instead.
 *	SMB - 4 Mar 98.
 *-
 */

STATUS sysextLocalToBusAdrs
	(
	int		adrsSpace,
	char *	pLocalAdrs,
	char **	ppBusAdrs
	)
{
	uint32	baseAddress;
	uint32	mask;
	uint32	topOfMemory;
	char *	pLocalAdrsPhys;

	if (sysextProcNumber < 0)
	{
		ERROR_SET (S_sysextLib_INVALID_CPU_NUMBER, "Invalid processor number", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/*
	 * First convert the local address to a physical (un-cached) physical address and also
	 * get the highest available physical address in local RAM.
	 */

	topOfMemory = (uint32) CACHE_DMA_VIRT_TO_PHYS ((uint32) sysMemTop () - 1);
	pLocalAdrsPhys = (char *) CACHE_DMA_VIRT_TO_PHYS (pLocalAdrs);

	/* Switch on memory space and convert address according to memory map and CPU number */

	switch (adrsSpace & VME_AM_MASK_SPACE)
	{
		case VME_AM_SHT:												/* Short space */

			if (pLocalAdrsPhys < (char *) SLAVE_SHT_LOC_BASE || pLocalAdrsPhys > (char *) SLAVE_SHT_LOC_TOP)
			{
				ERROR_SET (S_sysextLib_INVALID_LOCAL_ADRS, "Invalid local address", ERROR_LOG_SAVE);
				return (ERROR);
			}


			/* Only the short address space has a target-specific mapping... */

#ifdef	TARGET_MV167		/* ------ START OF CODE FOR THE MVME167 ------ */

			baseAddress = SLAVE_SHT_VME_BASE + sysextProcNumber * SYSEXT_SHT_NBYTE_PER_MV167;
			mask = SYSEXT_SHT_NBYTE_PER_MV167 - 1;

#else						/* ------ END OF CODE FOR THE MVME167 ------ */

			baseAddress = SLAVE_SHT_VME_BASE + sysextProcNumber * SYSEXT_SHT_NBYTE_PER_HKBAJA47;
			mask = SYSEXT_SHT_NBYTE_PER_HKBAJA47 - 1;

#endif	/* TARGET_MV167 */	/* ------ END OF CODE FOR THE HKBAJA47 ------ */


			* ppBusAdrs = (char *) (((baseAddress & ~mask) | ((uint32) pLocalAdrsPhys & mask))
			              & VME_ADDR_MASK_SHT);
			break;

		case VME_AM_STD:												/* Standard space */

			if (pLocalAdrsPhys < (char *) SLAVE_STD_LOC_BASE || pLocalAdrsPhys > (char *) SLAVE_STD_LOC_TOP)
			{
				ERROR_SET (S_sysextLib_INVALID_LOCAL_ADRS, "Invalid local address", ERROR_LOG_SAVE);
				return (ERROR);
			}
			baseAddress = SLAVE_STD_VME_BASE + sysextProcNumber * SYSEXT_STD_NBYTE_PER_PROC;
			mask = SYSEXT_STD_NBYTE_PER_PROC - 1;
			* ppBusAdrs = (char *) (((baseAddress & ~mask) | ((uint32) pLocalAdrsPhys & mask))
			              & VME_ADDR_MASK_STD);
			break;

		case VME_AM_EXT:												/* Extended space */

			if (pLocalAdrsPhys < (char *) SLAVE_EXT_LOC_BASE || pLocalAdrsPhys > (char *) topOfMemory)
			{
				ERROR_SET (S_sysextLib_INVALID_LOCAL_ADRS, "Invalid local address", ERROR_LOG_SAVE);
				return (ERROR);
			}
			baseAddress = SLAVE_EXT_VME_BASE + sysextProcNumber * SYSEXT_EXT_NBYTE_PER_PROC;
			* ppBusAdrs = (char *) (baseAddress | (uint32) pLocalAdrsPhys);
			break;

		default:

			ERROR_SET (S_sysextLib_INVALID_MEMORY_SPACE, "Invalid memory space", ERROR_LOG_SAVE);
			return (ERROR);
	}
	return (OK);
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextBusToLocalAdrs
 *
 *	INVOCATION:
 *	sysextBusToLocalAdrs (adrsSpace, pBusAdrs, ppLocalAdrs)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	adrsSpace	(int)		bus address space in which localAdrs resides
 *	(>)	pBusAdrs	(char *)	local address to convert
 *	(!)	ppLocalAdrs	(char **)	where to return local address
 *
 *	FUNCTION VALUE:
 *	(STATUS)  OK, or ERROR if the memory map is undefined or inconsistent
 *	with the bus address.
 *
 *	PURPOSE:
 *	Convert a bus address to a local address
 *
 *	DESCRIPTION:
 *	This routine gets the local address that accesses a specified bus memory address.
 *
 *	NOTE:
 *	Identifiers MASTER_SHT_LOC_BASE, MASTER_STD_LOC_BASE, MASTER_EXT_LOC_TOP and
 *	MASTER_EXT_LOC_BASE come from "sysextMemMap.h"
 *
 *	EXTERNAL VARIABLES:
 *	(>)	sysextProcNumber	(int)	processor number for this CPU
 *
 *	PRIOR REQUIREMENTS:
 *	sysextProcNumSet() must have been previously called in order to define the
 *	mapping of this CPU's memory onto VME bus.
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *	sysextMemMap.h
 *
 *	DEFICIENCIES:
 *	None
 *
 *	BUGS:
 *	I have found this function may be corrupting memory if called with a silly bus address,
 *	but I am not certain. SMB - 20 Feb 98.
 *-
 */

STATUS sysextBusToLocalAdrs
	(
	int		adrsSpace,
	char *	pBusAdrs,
	char **	ppLocalAdrs
	)
{
	if (sysextProcNumber < 0)
	{
		ERROR_SET (S_sysextLib_INVALID_CPU_NUMBER, "Invalid processor number", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* Switch on memory space and convert address according to memory map */

	switch (adrsSpace & VME_AM_MASK_SPACE)
	{
		case VME_AM_SHT:												/* Short space */

			if ((uint32) pBusAdrs >= VME_ADDR_SIZE_SHT)
			{
				ERROR_SET (S_sysextLib_INVALID_VME_ADRS, "Invalid VME-bus address", ERROR_LOG_SAVE);
				return (ERROR);
			}
			* ppLocalAdrs = (char *) ((uint32) MASTER_SHT_LOC_BASE +
			                ((uint32) pBusAdrs & VME_ADDR_MASK_SHT));
			break;

		case VME_AM_STD:												/* Standard space */

			if ((uint32) pBusAdrs >= VME_ADDR_SIZE_STD)
			{
				ERROR_SET (S_sysextLib_INVALID_VME_ADRS, "Invalid VME-bus address", ERROR_LOG_SAVE);
				return (ERROR);
			}
			* ppLocalAdrs = (char *) ((uint32) MASTER_STD_LOC_BASE +
			                ((uint32) pBusAdrs & VME_ADDR_MASK_STD));
			break;

		case VME_AM_EXT:												/* Extended space */

			if ((uint32) pBusAdrs < MASTER_EXT_VME_BASE || (uint32) pBusAdrs > MASTER_EXT_VME_TOP)
			{
				ERROR_SET (S_sysextLib_INVALID_VME_ADRS, "Invalid VME-bus address", ERROR_LOG_SAVE);
				return (ERROR);
			}
			* ppLocalAdrs = (char *) (MASTER_EXT_LOC_BASE - MASTER_EXT_VME_BASE + pBusAdrs);
			break;

		default:

			ERROR_SET (S_sysextLib_INVALID_MEMORY_SPACE, "Invalid memory space", ERROR_LOG_SAVE);
			return (ERROR);
	}
	return (OK);
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextProcNumGet
 *
 *	INVOCATION:
 *	sysextProcNumGet ()
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	None
 *
 *	FUNCTION VALUE:
 *	(STATUS)  The processor number for the CPU board, or -1 if the number
 *	has not been previously set via a call to sysextProcNumSet().
 *
 *	PURPOSE:
 *	Return the processor number for the CPU board
 *
 *	DESCRIPTION:
 *	This routine gets the processor number for the CPU board, which has been
 *	set with sysextProcNumSet().
 *
 *	EXTERNAL VARIABLES:
 *	(>)	sysextProcNumber	(int)	processor number for this CPU
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *-
 */

int	sysextProcNumGet (void)
{
	return (sysextProcNumber);
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextVmeBlockCopy
 *
 *	INVOCATION:
 *	sysextVmeBlockCopy (source, destination, nByte, toVme, mode)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	source		(const char *)	address from which data will be read
 *	(>)	destination	(char *)		address to which data will be written
 *	(>)	nByte		(int)			number of bytes to copy
 *	(>)	toVme		(BOOL)			copy local RAM to VME if true, or vice versa
 *	(>)	mode		(int)			VME block-transfer mode (none, D32 or D64)
 *
 *	FUNCTION VALUE:
 *	(STATUS)  OK, or ERROR if the transfer resulted in a status error in the
 *	DMA-driver hardware.
 *
 *	PURPOSE:
 *	Copy a block of data to/from VME bus, optionally using D32/ or D64/BLT mode
 *
 *	DESCRIPTION:
 *	This routine copies a block of data from RAM on the local CPU board to
 *	a range of addresses on VME bus, or performs the inverse operation in
 *	which the data is read from VME bus into local RAM. The parameters source
 *	and destination are local- and VME-bus addresses respectively. When toVme
 *	is TRUE the direction of the transfer is from local RAM to VME bus. The
 *	parameter mode determines whether a VME block-transfer ("BLT") mode will
 *	be employed and whether any such transfer will be use 32 or 64-bit data
 *	words ("D32/BLT" and "D64/BLT" respectively). mode should be set to one
 *	of the macros tabulated below - these are defined in sysextLib.h
 *
 *		VME_BLT_NONE	=>	Disable BLT
 *		VME_BLT_D32		=>	Attempt D32/BLT
 *		VME_BLT_D64		=>	Attempt D64/BLT
 *
 *	Note that the VME standard imposes certain restrictions on the alignment of
 *	addresses for BLT operations. Also, BLT operations are not particularly
 *	efficient for very small blocks of data. This routine will always attempt
 *	to use the largest word- size requested (via the mode parameter), providing this
 *	is compatible with the constraints on address-alignment and that the overall
 *	goal of achieving the fastest transfer possible is achieved.
 *
 *	EXTERNAL VARIABLES:
 *	(>)	sysextVme2BltSem	(SEM_ID)	semaphore indicates block-transfer completion
 *									(used by MV167 version of function only)
 *
 *	PRIOR REQUIREMENTS:
 *	The routine sysextVmeBlockInit() must be called in order to initialise the
 *	system for block-transfer operations. The CPU's VME bus-master interface is
 *	assumed to have been enabled (over an appropriate address range); this
 *	is normally performed explicitely by the routine sysextProcNumSet() in the
 *	event that the bus-master interface is not enabled following a system boot.
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *
 *	SEE ALSO:
 *	See "The VME bus handbook", W. D. Patterson, VITA Publication, 1989.
 *
 *	NOTE:
 *	The function comes in two variations, compiled
 *	separately for the MVME167 and HKBAJA47 targets
 *-
 */

#ifdef TARGET_MV167				/* ------ START OF CODE FOR THE MVME167 ------ */

STATUS	sysextVmeBlockCopy
	(
	const char *	source,
	char *			destination,
	int				nByte,
	BOOL			toVme,
	int				mode
	)
{
	BOOL	tryD32Blt = FALSE;
	BOOL	tryD64Blt = FALSE;

	/* NB. This code is specific to the MVME167 target */

	/*
	 * Determine whether D32 or D64 block-transfers have been requested. If neither are
	 * requested and/or the number of bytes to be transfered is very small, just do a
	 * good old fashioned bcopy().
	 */

	if (mode == VME_BLT_D64)
		tryD64Blt = TRUE;
	else if (mode == VME_BLT_D32)
		tryD32Blt = TRUE;

	if ((! tryD32Blt && ! tryD64Blt) || (nByte <= 8))
	{
#ifdef DEBUG
		printf ("bcopy 0x%x 0x%x %d", (int) source, (int) destination, nByte);
#endif	/* DEBUG */

		bcopy (source, destination, nByte);
		return (OK);
	}

	/*
	 * Clear DEN and DTBL bits (disable DMA and set for non-command-chaining mode
	 * (its not strictly necessary to clear DEN explicitely since it always reads
	 * back 0 and will therefore be re-written as 0, but lets play safe)
	 */

	BIT_CLR ((HW_REG32 *) VMECHIP2_DMACR1, DMACR1_DEN | DMACR1_DTBL);

	/* Set DHALT bit in DMA control reg #1 */

	BIT_SET ((HW_REG32 *) VMECHIP2_DMACR1, DMACR1_DHALT);

	/*
	 * Clear TVME and D16 bits in DMACR2 to set direction = VME->local and enable 
	 * D32 rather than D16 transfers. (The term "3 << 13" clears the snoop-mode bits
	 * and thus inhibits snooping on the local bus during DMA cycles).
	 *
	 * The INTE bit is not used in non-command-chaining-mode but is cleared here
	 * for completeness. The term 0xff clears the VME AM and D32/D64 enable bits;
	 * these are set below
	 */

	BIT_CLR ((HW_REG32 *) VMECHIP2_DMACR2, DMACR2_TO_VME | DMACR2_D16 | 0x3 << 13 |
	         DMACR2_INTE | 0xff);

	/* Enable local & VME address counter increments */

	BIT_SET ((HW_REG32 *) VMECHIP2_DMACR2, DMACR2_LINC | DMACR2_VINC);

	/*
	 * Set direction of VME transfers to local->VME (if required) and
	 * load starting address in local bus and VME bus address counters
	 */

	if (toVme)
	{
		BIT_SET ((HW_REG32 *) VMECHIP2_DMACR2, DMACR2_TO_VME);
		* (HW_REG32 *) VMECHIP2_DMACLBAC = (HW_REG32) source;
		* (HW_REG32 *) VMECHIP2_DMACVAC = (HW_REG32) destination;
	}
	else
	{
		* (HW_REG32 *) VMECHIP2_DMACLBAC = (HW_REG32) destination;
		* (HW_REG32 *) VMECHIP2_DMACVAC = (HW_REG32) source;
	}

	/* Load byte-counter register */

	* (HW_REG32 *) VMECHIP2_DMACBC = (HW_REG32) nByte;

	/*
	 * Set block-transfer mode bits (D32 or D64) and address modifier code.
	 * Note that the AM code should NOT be a BLT code, since the VMECHIP2
	 * provides modifies this AM code as necessary when BLTs occur
	 */

	if (tryD64Blt)

	{
		BIT_SET ((HW_REG32 *) VMECHIP2_DMACR2, DMACR2_D64_BLOCK | MASTER_EXT_AM);
	}
	else
	{
		BIT_SET ((HW_REG32 *) VMECHIP2_DMACR2, DMACR2_D32_BLOCK | MASTER_EXT_AM);
	}

	* (HW_REG32 *) VMECHIP2_DMACR1 = DMACR1_DEN;						/* Initiate DMA transfer.. */
	semTake (sysextVme2BltSem, WAIT_FOREVER);								/* ..wait for DMA completed */

	/*
	 * Check status in DMAC status register: bit #0 = DONE, bits #1-7 are various
	 * error conditions
	 */

	if (((* (HW_REG32 *) VMECHIP2_ICR) & 0xfe) != 0)					/* Return any error status */
	{
		ERROR_SET (S_sysextLib_VME_BLOCK_TRANSFER_FAIL, "VME block-transfer error", ERROR_LOG_SAVE);
		return (ERROR);
	}
	
	return (OK);
}

#endif	/* TARGET_MV167 */		/* ------ END OF CODE FOR THE MVME167 ------ */

#ifdef TARGET_HKBAJA47			/* ------ START OF CODE FOR THE HKBAJA47 ------ */


STATUS	sysextVmeBlockCopy
	(
	const char *	source,
	char *			destination,
	int				nByte,
	BOOL			toVme,
	int				mode
	)
{
	BOOL	tryD32Blt = FALSE;
	BOOL	tryD64Blt = FALSE;

 	/* NB. This code is specific to the HKBAJA47 target */

	if (mode == VME_BLT_D64)
		tryD64Blt = TRUE;
	else if (mode == VME_BLT_D32)
		tryD32Blt = TRUE;

	/* Do an old-fashioned bcopy() unless BLT-mode requested */
	if (! tryD32Blt && ! tryD64Blt)
	{
#ifdef DEBUG
		printf ("bcopy 0x%x 0x%x %d", (int) source, (int) destination, nByte);
#endif	/* DEBUG */

		bcopy (source, destination, nByte);
		return (OK);
	}

	/* Call the Baja's BSP routine */
	return (sysVicBlkCopy (source, destination, nByte, toVme, tryD64Blt));
}

#endif	/* TARGET_HKBAJA47 */	/* ------ END OF CODE FOR THE HKBAJA47 ------ */

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	IGNORED FUNCTION NAME:
 *	vmechip2BltIsr
 *
 *	INVOCATION:
 *	vmechip2BltIsr (argument)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	argument	(int)	mandatory argument to ISR, ignored by this routine
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Interrupt service routine for VMECHIP2 DMA operations (e.g. on the MVME167)
 *
 *	DESCRIPTION:
 *	This is the interrupt service routine for the VMECHIP2's DMA hardware. The
 *	routine clears the interrupt source and gives a pre-defined semaphore used
 *	by sysextVmeBlockCopy() in order to indicate that a block-transfer operation
 *	has completed. This routine should not be invoked directly by user code.
 *
 *	EXTERNAL VARIABLES:
 *	(>)	sysextVme2BltSem	(SEM_ID)	semaphore indicates block-transfer completion
 *									(used by MV167 version of function only)
 *
 *	PRIOR REQUIREMENTS:
 *	The routine sysextVmeBlockInit() must be called before a block-transfer
 *	operation is attempted (via sysextVmeBlockCopy()).
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *
 *	NOTE:
 *	This function is specific to the MVME167 target only
 *-
 */

#ifdef TARGET_MV167				/* ------ START OF CODE FOR THE MVME167 ------ */

void vmechip2BltIsr
	(
	int	argument
	)
{
	/* NB. This code is specific to the MVME167 target */

	* (HW_REG32 *) VMECHIP2_ICLR = ICLR_CDMA;			/* Clear DMAC interrupt */
	 semGive (sysextVme2BltSem);							/* Give semaphore to indicate BLT completed */
}

#endif	/* TARGET_MV167 */		/* ------ END OF CODE FOR THE MVME167 ------ */

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextVmeBlockInit
 *
 *	INVOCATION:
 *	sysextVmeBlockInit ()
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	None
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK, or ERROR if initialisation failed.
 *
 *	PURPOSE:
 *	Initialise system to enable VME block-transfers
 *
 *	DESCRIPTION:
 *	This initialisation routine must be called before VME block-transfer
 *	operations can be performed using the associated routine
 *	sysextVmeBlockCopy().
 *
 *	EXTERNAL VARIABLES:
 *	(!)	sysextVme2BltSem	(SEM_ID)	semaphore indicates block-transfer completion
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *
 *	SEE ALSO:
 *	See "The VME bus handbook", W. D. Patterson, VITA Publication, 1989.
 *
 *	NOTE:
 *	The function comes in two variations, compiled
 *	separately for the MVME167 and HKBAJA47 targets
 *-
 */

#ifdef TARGET_MV167				/* ------ START OF CODE FOR THE MVME167 ------ */

STATUS sysextVmeBlockInit (void)
{
	uint8 vectorBase;

	/* NB. This code is specific to the MVME167 target */

	/* Compute DMAC interrupt vector */
	vectorBase = (((HW_REG32) * VMECHIP2_IOCR) >> 24) & 0xf0;

	/* Set maximum time on bus and minimum time off bus for block transfers */

	/* Clear all bits */
	BIT_CLR ((HW_REG32 *) VMECHIP2_TIMEOUTCR, 0x3f << 18);

	/* Set required bits */
	BIT_SET ((HW_REG32 *) VMECHIP2_TIMEOUTCR, BLT_MAX_TIME_ON_BUS | BLT_MIN_TIME_OFF_BUS);

	if ((sysextVme2BltSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY)) == NULL)
	{
		ERROR_SET (0, "Failed to create binary sysextVme2BltSem semaphore", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* Clear any pending interrupt */
	* (HW_REG32 *) VMECHIP2_ICLR = ICLR_CDMA;

	/* Clear DMAC interrupt level */
	BIT_CLR ((HW_REG32 *) VMECHIP2_ILR2, 0x07 << 24);

	/* Set interrupt level */
	BIT_SET ((HW_REG32 *) VMECHIP2_ILR2, MV167_DMAC_INTERRUPT_LEVEL << 24);

	/* Connect the interrupt service routine to the DMAC interrupt */

	if (intConnect ((VOIDFUNCPTR *) INUM_TO_IVEC (vectorBase | 6), 
					(VOIDFUNCPTR) vmechip2BltIsr, 0) == ERROR)
	{

		/* Clear DMAC interrupt level and report an error */

		BIT_CLR ((HW_REG32 *) VMECHIP2_ILR2, 0x07 << 24);
		ERROR_SET (0, "Failed to connect vmechip2BltIsr to DMAC interrupt", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* Enable DMAC interrupt */
	BIT_SET ((HW_REG32 *) VMECHIP2_LBIER, LBIER_EDMA);

	return (OK);
}

#endif	/* TARGET_MV167 */		/* ------ END OF CODE FOR THE MVME167 ------ */

#ifdef TARGET_HKBAJA47			/* ------ START OF CODE FOR THE HKBAJA47 ------ */


STATUS sysextVmeBlockInit (void)
{
 	/* NB. This code is specific to the HKBAJA47 target */

	/*
	 * Set block-transfer burst length (number of cycles/burst) and interleave period. Do
	 * this using the Baja's standard BSP routine.
	 */

	return (sysVicBlkTune (BLT_BURST_LENGTH, BLT_INTERLEAVE_PERIOD));
}

#endif	/* TARGET_HKBAJA47 */	/* ------ END OF CODE FOR THE HKBAJA47 ------ */

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextVmeReqRelInit
 *
 *	INVOCATION:
 *	sysextVmeReqRelInit (vmeMode)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	vmeMode	(VME_MODE)	structure defines VME request/release mode
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK, or ERROR if the specified VME mode is invalid.
 *
 *	PURPOSE:
 *	Set the bus-arbitration mode for the CPU board
 *
 *	DESCRIPTION:
 *	This routine sets the bus-arbitration mode for the CPU board. The mode is
 *	specified by the structure VME_MODE, which is type-defined in sysextLib.h
 *	The definition allows the following options to be set
 *
 *	-	Set bus-request level
 *	-	Enable/disable release-when-done mode
 *	-	Enable/disable fair-requester mode
 *	-	Enable/disable round-robin arbiter mode
 *	-	Set bus-request level for DMA operations
 *	-	Enable/disable fair-requester mode for DMA operations
 *
 *	See the structure definition for details of how to control these options.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *
 *	SEE ALSO:
 *	See "The VME bus handbook", W. D. Patterson, VITA Publication, 1989.
 *
 *	NOTE:
 *	The function comes in two variations, compiled
 *	separately for the MVME167 and HKBAJA47 targets
 *-
 */

#ifdef TARGET_MV167				/* ------ START OF CODE FOR THE MVME167 ------ */

void sysextVmeReqRelInit
	(
	VME_MODE *	vmeMode
	)
{
	/* NB. This code is specific to the MVME167 target */

	/*
	 * Clear all writeable bits in DMA control register #1 except DWB = bus request
	 */

	BIT_CLR ((HW_REG32 *) VMECHIP2_DMACR1, 0x000c8f3f);


	/*
	 * Now set set bits in DMA control reg #1 as follows:
	 *		set VME bus request level
	 *		set DMA controller bus request level
	 *		set VME release mode for DMA controller to release when timer expired or BRx active
	 */

	BIT_SET ((HW_REG32 *) VMECHIP2_DMACR1, DMACR1_SINK_DATA |
		((vmeMode->requestLevel & 0x03) << 8) |
		(vmeMode->requestLevelDma & 0x03) | DMACR1_TIMER_BRX);

	/* Set bus-release, -request and -arbiter modes */

	if (vmeMode->releaseWhenDone)	BIT_SET ((HW_REG32 *) VMECHIP2_DMACR1, DMACR1_LVRWD);
	if (vmeMode->fairRequester)		BIT_SET ((HW_REG32 *) VMECHIP2_DMACR1, DMACR1_LVFAIR);
	if (vmeMode->roundRobinArbiter)	BIT_SET ((HW_REG32 *) VMECHIP2_DMACR1, DMACR1_ROBIN);
	if (vmeMode->fairRequesterDma)	BIT_SET ((HW_REG32 *) VMECHIP2_DMACR1, DMACR1_DFAIR);
}


#endif	/* TARGET_MV167 */		/* ------ END OF CODE FOR THE MVME167 ------ */

#ifdef TARGET_HKBAJA47			/* ------ START OF CODE FOR THE HKBAJA47 ------ */


void	sysextVmeReqRelInit
	(
	VME_MODE *	vmeMode
	)
{
 	/* NB. This code is specific to the HKBAJA47 target */

	/*
	 * Clear all except DRAM refresh bit in VIC's ARCR (this bit should already be
	 * clear anyway since VIC isn't used for DRAM refresh on the Baja47). Clearing
	 * these bits disables fair request mode and enables round-robin arbitration; the
	 * bus request level is also set to 0.
	 */

	BIT_CLR ((HW_REG8 *) VIC_ARCR, 0xef);
	BIT_SET ((HW_REG8 *) VIC_ARCR, (vmeMode->requestLevel & 0x03) << 5);	/* Set bus req. level	*/

	if (vmeMode->fairRequester)												/* Set fair req. mode	*/
		BIT_SET ((HW_REG8 *) VIC_ARCR, ARCR_NO_TOUT);						/* (no timeout on 		*/
																			/* fair request)		*/

	if (! vmeMode->roundRobinArbiter)										/* Set arbiter mode		*/
		BIT_SET ((HW_REG8 *) VIC_ARCR, ARCR_PRIORITY);						/* (round-robin or		*/
																			/* priority)			*/

	/*
	 * NB. Baja47 doesn't use vmeMode->requestLevelDma or ->fairRequesterDma since
	 * the VIC chip uses the same request level and mode for DMA and non-DMA
	 * transfers. These structure elements are provided for commonality with the
	 * MVME167 which does allow different request levels and modes
	 */
}


#endif	/* TARGET_HKBAJA47 */	/* ------ END OF CODE FOR THE HKBAJA47 ------ */

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextWriteLocalMboxReg8
 *
 *	INVOCATION:
 *	sysextWriteLocalMboxReg8 (data)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	data	(uint8)		data to write to mailbox register
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Write 8-bit word to local mailbox register
 *
 *	DESCRIPTION:
 *	This routine writes a single data byte to a register in the local
 *	CPU's mailbox space.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *
 *	NOTE:
 *	The function comes in two variations, compiled
 *	separately for the MVME167 and HKBAJA47 targets
 *-
 */

#ifdef TARGET_MV167				/* ------ START OF CODE FOR THE MVME167 ------ */

void sysextWriteLocalMboxReg8
	(
	uint8	data
	)
{
	/* NB. This code is specific to the MVME167 target */

	/* Write VMEChip2 General purpose reg #0, high byte */
	* (char *) VC2GCSR_GPR0H = data;
}

#endif	/* TARGET_MV167 */		/* ------ END OF CODE FOR THE MVME167 ------ */

#ifdef TARGET_HKBAJA47			/* ------ START OF CODE FOR THE HKBAJA47 ------ */

void sysextWriteLocalMboxReg8
	(
	uint8	data
	)
{
 	/* NB. This code is specific to the HKBAJA47 target */

	/* Write VIC068 Inter-processor comms reg #0 */
	* (char *) VIC_ICR0 = data;
}

#endif	/* TARGET_HKBAJA47 */	/* ------ END OF CODE FOR THE HKBAJA47 ------ */

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextWriteLocalMboxReg32
 *
 *	INVOCATION:
 *	sysextWriteLocalMboxReg32 (data)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	data	(uint32)		data to write to mailbox register
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Write 32-bit word to local mailbox registers
 *
 *	DESCRIPTION:
 *	This routine writes a 32-bit integer to 4 8-bit registers in the
 *	local CPU's mailbox space.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *
 *	NOTE:
 *	The function comes in two variations, compiled
 *	separately for the MVME167 and HKBAJA47 targets
 *-
 */

#ifdef TARGET_MV167				/* ------ START OF CODE FOR THE MVME167 ------ */

void sysextWriteLocalMboxReg32
	(
	uint32	data
	)
{
	/* NB. This code is specific to the MVME167 target */

	/*
	 * Write 4-byte integer to first four general-purpose registers in VMEChip2's mailbox space
	 */

	* (char *) VC2GCSR_GPR0H = (data >> 24) & 0xff;	/* VMEChip2 Gen purpose reg #0, high byte */
	* (char *) VC2GCSR_GPR0L = (data >> 16) & 0xff;	/* VMEChip2 Gen purpose reg #0, low byte */
	* (char *) VC2GCSR_GPR1H = (data >> 8) & 0xff;	/* VMEChip2 Gen purpose reg #1, high byte */
	* (char *) VC2GCSR_GPR1L = data & 0xff;			/* VMEChip2 Gen purpose reg #1, low byte */
}

#endif	/* TARGET_MV167 */		/* ------ END OF CODE FOR THE MVME167 ------ */

#ifdef TARGET_HKBAJA47			/* ------ START OF CODE FOR THE HKBAJA47 ------ */


void sysextWriteLocalMboxReg32
	(
	uint32	data
	)
{
 	/* NB. This code is specific to the HKBAJA47 target */

	/*
	 * Write 4-byte integer to first four inter-processor communications registers in
	 * VIC068 chip's mailbox space
	 */

	* (char *) VIC_ICR0 = (data >> 24) & 0xff;				/* VIC068 ICR #0 */
	* (char *) VIC_ICR1 = (data >> 16) & 0xff;				/* VIC068 ICR #1 */
	* (char *) VIC_ICR2 = (data >> 8) & 0xff;				/* VIC068 ICR #2 */
	* (char *) VIC_ICR3 = data & 0xff;						/* VIC068 ICR #3 */
}


#endif	/* TARGET_HKBAJA47 */	/* ------ END OF CODE FOR THE HKBAJA47 ------ */

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextProcNumSet
 *
 *	INVOCATION:
 *	sysextProcNumSet (processorNumber)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	processorNumber	(int)	processor number for the CPU board
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK, or ERROR if the CPU board could not be mapped onto
 *	VME bus or if the processor number is invalid.
 *
 *	PURPOSE:
 *	Set the processor number for the CPU board
 *
 *	DESCRIPTION:
 *	This routine sets the processor number for the CPU board. Processor
 *	numbers should be unique on a single backplane. The routine initialises
 *	the board's VME bus interface for master and slave operations, according
 *	to the memory map defined in the include file sysextMemMap.h and the
 *	specified processor number. The memory map normally enables bus-master
 *	accesses in short, standard and extended address spaces over the maximum
 *	range of addresses supported by the target hardware. The CPU board's
 *	local RAM is mapped onto VME bus for slave accesses over non-overlapping
 *	address ranges, such that multiple CPUs can share their local RAM. The
 *	recommended address map maps up to 64 MBytes of local RAM per CPU onto
 *	VME extended address space, whilst the 24-bit and 16-bit addressing modes
 *	(standard and short respectively) can only map a part of each CPUs RAM.
 *	Extended addressing mode is therefore recommended for most shared-memory
 *	applciations.
 *
 *	This routine differs from sysProcNumSet() in that VxWorks requires that
 *	sysProcNumSet() only enables the VME slave interface for processor #0
 *	and when the backplane network is used, whereas this routine enables the
 *	slave interface for all CPUs.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *
 *	NOTE:
 *	The function comes in two variations, compiled
 *	separately for the MVME167 and HKBAJA47 targets
 *-
 */

#ifdef TARGET_MV167				/* ------ START OF CODE FOR THE MVME167 ------ */

STATUS sysextProcNumSet
	(
	int	processorNumber
	)
{
	uint32	oldDmacDwb;
	uint32	baseAddress;
	uint32	endAddress;
	char *	pTestLocation;
	char	dummy;

	/* NB. This code is specific to the MVME167 target */

	if (processorNumber < 0 || processorNumber > SYSEXT_MAX_N_PROC)
	{
		ERROR_SET (S_sysextLib_INVALID_CPU_NUMBER, "Invalid CPU number", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* Set static processor number */

	sysextProcNumber = processorNumber;

	/*
	 * Initialise VME bus master interface and enable master accesses.
	 * First disable all VME master interfaces on VMECHIP2.
	 */

	BIT_CLR ((HW_REG32 *) VMECHIP2_LBTVCR, LBTVCR_EN1 | LBTVCR_EN2 | LBTVCR_EN3 |
			LBTVCR_EN4 | LBTVCR_I1EN | LBTVCR_I2EN);

	/*
	 * Now enable the short (master) interface.
	 *
	 * If the bus-master local base address in short space is the default for the MVME167, set
	 * the Local Bus To VME Bus I/O control register to enable the short space master interface.
	 * Otherwise, set the Local Bus Slave (master) starting and ending address registers #2
	 * and attribute register #2 to enable the VME short space master interface.
	 */

	if (MASTER_SHT_LOC_BASE == DEFAULT_MASTER_SHT_LOC_BASE)
	{
		/* Enable supervisory or user-mode accesses */

		if ((MASTER_SHT_AM & VME_AM_MASK_SUP_BIT) == VME_AM_MASK_SUP_BIT)
		{
			BIT_SET ((HW_REG32 *) VMECHIP2_LBTVCR, LBTVCR_I1SUP);
		}
		else
		{
			BIT_CLR ((HW_REG32 *) VMECHIP2_LBTVCR, LBTVCR_I1SUP);
		}

		/*
		 * Disable write posting, set to D32 rather than D16 accesses and enable short VME
		 * master accesses
		 */

		BIT_CLR ((HW_REG32 *) VMECHIP2_LBTVCR, LBTVCR_I1WP | LBTVCR_I1D16);
		BIT_SET ((HW_REG32 *) VMECHIP2_LBTVCR, LBTVCR_I1EN);
	}
	else
	{

		/* Use address VMECHIP2's master register set #2 */

		* (HW_REG32 *) VMECHIP2_LBSAR2 = ((MASTER_SHT_LOC_BASE + VME_ADDR_SIZE_SHT - 1) & 0xffff0000) |
									(MASTER_SHT_LOC_BASE >> 16 & 0x0000ffff);

		/* Clear address-mode field, set to D32 accesses and disable write posting */

		BIT_CLR ((HW_REG32 *) VMECHIP2_LBSAR, VME_AM_MASK_ALL << 8 | LBSAR2_WP_ENABLE |
				  LBSAR2_D16);

		/* Set address mode and enable 2nd map decoder */

		BIT_SET ((HW_REG32 *) VMECHIP2_LBSAR, MASTER_SHT_AM << 8);
		BIT_SET ((HW_REG32 *) VMECHIP2_LBTVCR, LBTVCR_EN2);
	}

	/* Initialise standard master interface using register set #3 */

	* (HW_REG32 *) VMECHIP2_LBSAR3 = ((MASTER_STD_LOC_BASE + VME_ADDR_SIZE_SHT - 1) &
								  0xffff0000) | (MASTER_STD_LOC_BASE >> 16 & 0x0000ffff);

	/* Clear address mode field, set to D32 accesses and disable write posting */

	BIT_CLR ((HW_REG32 *) VMECHIP2_LBSAR, VME_AM_MASK_ALL << 16 | LBSAR3_WP_ENABLE |
				  LBSAR3_D16);

	/* Set address mode and enable 3rd map decode */

	BIT_SET ((HW_REG32 *) VMECHIP2_LBSAR, MASTER_STD_AM << 16);
	BIT_SET ((HW_REG32 *) VMECHIP2_LBTVCR, LBTVCR_EN3);

	/*
	 * Initialise extended master interface using register set #4
	 * with address translation disabled
	 */

	* (uint32 *) VMECHIP2_LBSAR4  = (MASTER_EXT_LOC_TOP & 0xffff0000) |
		(MASTER_EXT_LOC_BASE >> 16 & 0x0000ffff);
	* (uint32 *) VMECHIP2_LBSATR1 = 0;

	/* Clear address mode field, set to D32 accesses and enable/disable master write posting */

	BIT_CLR ((HW_REG32 *) VMECHIP2_LBSAR, VME_AM_MASK_ALL << 24 | LBSAR4_WP_ENABLE | LBSAR4_D16);
	if (MASTER_EXT_WP_ENABLE)
	{
		BIT_SET ((HW_REG32 *) VMECHIP2_LBSAR, LBSAR4_WP_ENABLE);	/* Enable write posting */
	}
	else
	{
		BIT_CLR ((HW_REG32 *) VMECHIP2_LBSAR, LBSAR4_WP_ENABLE);	/* Disable write posting */
	}

	/* Set address mode and enable 4th map decode */

	BIT_SET ((HW_REG32 *) VMECHIP2_LBSAR, MASTER_EXT_AM << 24);
	BIT_SET ((HW_REG32 *) VMECHIP2_LBTVCR, LBTVCR_EN4);

	/*
	 * Before initialising the slave interface, first probe extended VME space at the
	 * first byte location to which this processor's RAM is to be mapped. If this VME
	 * location is readable, another processor must have already initialised with
	 * the same processorNumber as this one (which is an error condition)
	 */

	baseAddress = SLAVE_EXT_VME_BASE + processorNumber * SYSEXT_EXT_NBYTE_PER_PROC;
	if (sysextBusToLocalAdrs (VME_AM_EXT, (char *) baseAddress, & pTestLocation) == ERROR)
	{
		ERROR_SET (0, "Invalid base address for extended VME probe", ERROR_LOG_SAVE);
		return (ERROR);
	}

	if (vxMemProbe (pTestLocation, VX_READ, 1, &dummy) == OK)
	{
		ERROR_SET (S_sysextLib_CPU_NUM_ALREADY_USED, "CPU number already assigned", ERROR_LOG_SAVE);
		sysextProcNumber = -1;
		return (ERROR);
	}

	/*
	 * Initialise VME bus slave interface and enable slave accesses. Do this whilst
	 * holding VME bus to ensure no attempts at external accesses during configuration.
	 */

	/* Save bus request bit for future restoration */
	oldDmacDwb = (* (HW_REG32 *) VMECHIP2_DMACR1) & DMACR1_DWB;

	/* Request VME bus.. and wait until its granted */
	BIT_SET ((HW_REG32 *) VMECHIP2_DMACR1, DMACR1_DWB);

	while (((* (HW_REG32 *) VMECHIP2_DMACR1) & DMACR1_DHB) != DMACR1_DHB)
		;							/* Empty statement */

	/*
	 * Initialise short space slave interface: write short slave base address
	 * to GCSR group address and board address registers.
	 */

	baseAddress = SLAVE_SHT_VME_BASE + processorNumber * SYSEXT_SHT_NBYTE_PER_MV167;
	BIT_CLR ((HW_REG32 *) VMECHIP2_LBTVCR, 0xfff00000);
	BIT_SET ((HW_REG32 *) VMECHIP2_LBTVCR, (baseAddress << 16) & 0xfff00000);

	/*
	 * Initialise standard space slave interface: write VME bus slave starting
	 * and ending registers and address translation/select registers.
	 */

	baseAddress = SLAVE_STD_VME_BASE + processorNumber * SYSEXT_STD_NBYTE_PER_PROC;
	endAddress = baseAddress - SLAVE_STD_LOC_BASE + SLAVE_STD_LOC_TOP;
	* (HW_REG32 *) VMECHIP2_VSAR1 = (endAddress & 0xffff0000) | ((baseAddress >> 16) & 0x0000ffff);
	* (HW_REG32 *) VMECHIP2_VSATR1 = (SLAVE_STD_LOC_BASE & 0xffff0000) |
								(~(SLAVE_STD_LOC_TOP - SLAVE_STD_LOC_BASE) >> 16 & 0x0000ffff);

	/*
	 * Write VME bus slave address modifier select register, slave write-post
	 * and snoop-control registers.
	 */

	/*
	 * Clear AM-select and snoop-control registers and disable slave write-posting, then
	 * set snoop mode to write-invalidate and enable all address modes and A24 slave.
	 */

	BIT_CLR ((HW_REG32 *) VMECHIP2_VSAMSR, 0x0fff | VSAMSR1_WP);
	BIT_SET ((HW_REG32 *) VMECHIP2_VSAMSR, VSAMSR1_A24 | VSAMSR1_SNP_WI | VSAMSR1_SUP |
			  VSAMSR1_USR | VSAMSR1_DAT | VSAMSR1_PGM | VSAMSR1_BLK | VSAMSR1_D64);

	/*
	 * Initialise extended space slave interface: write VME bus slave starting
	 * and ending registers and address translation/select registers
	 */

	baseAddress = SLAVE_EXT_VME_BASE + processorNumber * SYSEXT_EXT_NBYTE_PER_PROC;
	endAddress = baseAddress + (uint32) CACHE_DMA_VIRT_TO_PHYS (sysMemTop () - 1);
	* (HW_REG32 *) VMECHIP2_VSAR2 = (endAddress & 0xffff0000) | ((baseAddress >> 16) & 0x0000ffff);
	* (HW_REG32 *) VMECHIP2_VSATR2 = (SLAVE_EXT_LOC_BASE & 0xffff0000) |
							(~ (uint32) CACHE_DMA_VIRT_TO_PHYS (sysMemTop () - 1) >> 16 & 0x0000ffff);

	/*
	 * Write VME bus slave address modifier select register, slave write-post
	 * and snoop-control registers
	 */

	/* First clear AM-select, WP- and snoop-control regs, then set snoop mode to
	 * write-invalidate, enable A32 slave accesses and enable all address modes
	 */

	BIT_CLR ((HW_REG32 *) VMECHIP2_VSAMSR, 0x0fff0000);
	BIT_SET ((HW_REG32 *) VMECHIP2_VSAMSR, VSAMSR2_A32 | VSAMSR2_SNP_WI | VSAMSR2_SUP |
			 VSAMSR2_USR | VSAMSR2_DAT | VSAMSR2_PGM | VSAMSR2_BLK | VSAMSR2_D64);

	/* Test for write-posting enabled and set accordingly */

	if (SLAVE_EXT_WP_ENABLE)
	{
		BIT_SET ((HW_REG32 *) VMECHIP2_VSAMSR, VSAMSR2_WP);
	}
	else
	{
		BIT_CLR ((HW_REG32 *) VMECHIP2_VSAMSR, VSAMSR2_WP);
	}

	/* Finally, initialise for block transfers */

	if (sysextVmeBlockInit () == ERROR)
	{
		ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* Restore old VME bus request status */

	if (oldDmacDwb != DMACR1_DWB) BIT_CLR ((HW_REG32 *) VMECHIP2_DMACR1, DMACR1_DWB);

	return (OK);
}

#endif	/* TARGET_MV167 */		/* ------ END OF CODE FOR THE MVME167 ------ */

#ifdef TARGET_HKBAJA47			/* ------ START OF CODE FOR THE HKBAJA47 ------ */


STATUS sysextProcNumSet
	(
	int	processorNumber
	)
{
	unsigned char	oldRelCntrl;
	uint32			baseAddress;
	char *			pTestLocation;
	char			dummy;

 	/* NB. This code is specific to the HKBAJA47 target */

	if (processorNumber < 0 || processorNumber > SYSEXT_MAX_N_PROC)
	{
		ERROR_SET (S_sysextLib_INVALID_CPU_NUMBER, "Invalid CPU number", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* Set static processor number */
	sysextProcNumber = processorNumber;

	/*
	 * The only action  needed to initialise the VME bus master interface for
	 * the Baja4700 is to enable or disable master write posting in extended space.
	 * Master VME access is always enabled for this target.
	 * Also check that the short and standard base address are correct (they
	 * should NEVER be changed and are only provided as MACROS for consistency
	 * with the MVME167 target which does allow arbitrary base addresses).
	 */

	/*
	 * Set or clear master-write-post-enable bit for extended space in VIC's SS1CR0
	 * (bus master accesses only, write-posting for the slave interface is enabled
	 * later).
	 */

	if (MASTER_EXT_WP_ENABLE)
	{
		BIT_SET ((HW_REG8 *) VIC_SS1CR0, SSCR0_MASTER_WP);
	}
	else
	{
		BIT_CLR ((HW_REG8 *) VIC_SS1CR0, SSCR0_MASTER_WP);
	}

	/*
	 * Before initialising the slave interface, first probe extended VME space at
	 * the first byte location to which this processor's RAM is to be mapped. If
	 * this VME location is readable, another processor must have already initialised
	 * with the same processorNumber as this one (which is an error condition).
	 */

	baseAddress = SLAVE_EXT_VME_BASE + processorNumber * SYSEXT_EXT_NBYTE_PER_PROC;
	if (sysextBusToLocalAdrs (VME_AM_EXT, (char *) baseAddress, & pTestLocation) == ERROR)
	{
		ERROR_SET (0, "Invalid base address for extended VME probe", ERROR_LOG_SAVE);
		return (ERROR);
	}
	if (vxMemProbe (pTestLocation, VX_READ, 1, &dummy) == OK)
	{
		ERROR_SET (S_sysextLib_CPU_NUM_ALREADY_USED, "CPU number already assigned", ERROR_LOG_SAVE);
		sysextProcNumber = -1;
		return (ERROR);
	}

	/*
	 * Initialise VME bus slave interface and enable slave accesses. Do this
	 * whilst holding VME bus to ensure no attempts at external accesses during
	 * configuration.
	 */

	/* Save VIC release control register value */
	oldRelCntrl = * VIC_RCR;

	/* Capture VME bus to ensure no slave accesses */
	* VIC_RCR = RCR_BCAP;

	while ((* VIC_BESR & BESR_MASTER) == 0)
		;		/* Empty statement */

	/* Initialise short space slave interface */

	baseAddress = SLAVE_SHT_VME_BASE + processorNumber * SYSEXT_SHT_NBYTE_PER_HKBAJA47;
	* (HKBAJA_MBOX_ADRS_PLACEMT) = (unsigned char) (baseAddress >> 8);
	* (HKBAJA_VME_SHT_ENABLE) = 0;							/* Enable interface */

	/* Initialise standard space slave interface */

	* VIC_SS0CR0 = SSCR0_ASIZ_A24 | SSCR0_SLSEL_D32 | SSCR0_BLT_ACC;

	baseAddress = SLAVE_STD_VME_BASE + processorNumber * SYSEXT_STD_NBYTE_PER_PROC;
	* (HKBAJA_VME_STD_MAP)    = (unsigned char) (((baseAddress >> 16) & 0xe0) |
												 ((SLAVE_STD_LOC_BASE >> 20) & 0x0e));

#ifdef OLDCODE /* before I added () to quieten some compiler warnings - SMB 15 Dec 97 */
	* (HKBAJA_VME_STD_MAP)    = (unsigned char) ((baseAddress >> 16) & 0xe0 |
												 (SLAVE_STD_LOC_BASE >> 20) & 0x0e);
#endif

	* (HKBAJA_VME_LOCAL_ADRS) = (unsigned char) (SLAVE_STD_LOC_BASE >> 24);

	/*
	 * Set-up VIC's SS0CR0 for standard space. Clear address mode field and enable
	 * all addressing modes (clear supervisory-only bit). Then set slave address
	 * space to standard, D32 and enable accelerated BLTs on the local bus.
	 */

	BIT_CLR ((HW_REG8 *) VIC_SS0CR0, SSCR0_ASIZ_AMREG | SSCR0_SLSEL_SPR);
	BIT_SET ((HW_REG8 *) VIC_SS0CR0, SSCR0_ASIZ_A24 | SSCR0_SLSEL_D32 | SSCR0_BLT_ACC);
	* (HKBAJA_VME_STD_ENABLE) = 0;							/* Enable interface */

	/* Initialise extended space slave interface */

	baseAddress = SLAVE_EXT_VME_BASE + processorNumber * SYSEXT_EXT_NBYTE_PER_PROC;
	* (HKBAJA_VME_BASE_ADRS)  = (unsigned char) (baseAddress >> 24);
	* (HKBAJA_VME_LOCAL_ADRS) = (unsigned char) (SLAVE_EXT_LOC_BASE >> 24);

	/*
	 * Set-up VIC's SS1CR0 for extended space. Clear address mode field and enable
	 * all addressing modes (clear supervisory-only bit). Then set slave address
	 * space to extended, D32 and enable accelerated BLTs on the local bus.
	 */

	BIT_CLR ((HW_REG8 *) VIC_SS1CR0, SSCR0_ASIZ_AMREG | SSCR0_SLSEL_SPR);
	BIT_SET ((HW_REG8 *) VIC_SS1CR0, SSCR0_ASIZ_A32 | SSCR0_SLSEL_D32 | SSCR0_BLT_ACC);
	if (SLAVE_EXT_WP_ENABLE)							/* Enable or disable slave write posting */
	{
		BIT_SET ((HW_REG8 *) VIC_SS1CR0, SSCR0_SLAVE_WP);
	}
	else
	{
		BIT_CLR ((HW_REG8 *) VIC_SS1CR0, SSCR0_SLAVE_WP);
	}
	* (HKBAJA_VME_EXT_ENABLE) = 0;						/* Enable interface */

	/* Finally, initialise for block transfers */

	if (sysextVmeBlockInit () == ERROR)
	{
		ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);
		return (ERROR);
	}

	* VIC_RCR = oldRelCntrl;							/* Restore bus-release control reg */

	return (OK);
}


#endif	/* TARGET_HKBAJA47 */	/* ------ END OF CODE FOR THE HKBAJA47 ------ */

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextMboxEnable
 *
 *	INVOCATION:
 *	sysextMboxEnable (mailbox)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	mailbox	(int)	number of mailbox interrupt to enable
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK, or ERROR if the mailbox number is invalid.
 *
 *	PURPOSE:
 *	Enable a mailbox interrupt
 *
 *	DESCRIPTION:
 *	This routine enables a specified mailbox interrupt channel. The
 *	mailbox number must be >= 0 and < SYSEXT_NUMBER_OF_MAILBOXES.
 *
 *	EXTERNAL VARIABLES:
 *	(struct mailbox *)	sysextMailTbl	table for mailbox-interrupt status information
 *
 *	PRIOR REQUIREMENTS:
 *	A routine must have been connected to the specified mailbox interrupt
 *	channel via the routine sysextMboxConnect().
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *
 *	NOTE:
 *	The function comes in two variations, compiled
 *	separately for the MVME167 and HKBAJA47 targets
 *-
 */

#ifdef TARGET_MV167				/* ------ START OF CODE FOR THE MVME167 ------ */

STATUS sysextMboxEnable
	(
	int	mailbox
	)
{
	/* NB. This code is specific to the MVME167 target */

	if (mailbox < 0 || mailbox >= SYSEXT_NUMBER_OF_MAILBOXES)
	{
		ERROR_SET (S_sysextLib_INVALID_MAILBOX_NUMBER, "Invalid mailbox IRQ channel #",
		           ERROR_LOG_SAVE);
		return (ERROR);
	}

	if (! sysextMailTbl [mailbox].sysextMailboxConnected)
	{
		ERROR_SET (S_sysextLib_MAILBOX_NOT_CONNECTED, "Mailbox interrupt not connected",
		           ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* Acknowledge any pending interrupts */

	* (HW_REG32 *) VMECHIP2_ICLR = 1 << (18 + mailbox);

	/* Clear, then set, interrupt level */
	BIT_CLR ((HW_REG32 *) VMECHIP2_ILR2, 0x07 << ((mailbox + 2) * 4));
	BIT_SET ((HW_REG32 *) VMECHIP2_ILR2, MV167_MBOX_INTERRUPT_LEVEL << ((mailbox + 2) * 4));
	BIT_SET ((HW_REG32 *) VMECHIP2_LBIER, 1 << (18 + mailbox));

	/*
	 * Ensure Master interrupt enable bit (in IO control reg #1) is set. NB Typo in
	 * vmechip2.h: "MEIN" rather than "MIEN" (latter is used in the MVME167's hardware manual)
	 */

	BIT_SET ((HW_REG32 *) VMECHIP2_IOCR, IOCR_MEIN);

	return (OK);
}


#endif	/* TARGET_MV167 */		/* ------ END OF CODE FOR THE MVME167 ------ */

#ifdef TARGET_HKBAJA47			/* ------ START OF CODE FOR THE HKBAJA47 ------ */


STATUS sysextMboxEnable
	(
	int	mailbox
	)
{
 	/* NB. This code is specific to the HKBAJA47 target */

	return (sysMailboxEnable ((char *) mailbox));				/* Call the Baja's BSP routine */
}


#endif	/* TARGET_HKBAJA47 */	/* ------ END OF CODE FOR THE HKBAJA47 ------ */

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextMboxDisable
 *
 *	INVOCATION:
 *	sysextMboxDisable (mailbox)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	mailbox	(int)	number of mailbox interrupt to disable
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK, or ERROR if the mailbox number is invalid.
 *
 *	PURPOSE:
 *	Disable a mailbox interrupt
 *
 *	DESCRIPTION:
 *	This routine disables a specified mailbox interrupt channel. The
 *	mailbox number must be >= 0 and < SYSEXT_NUMBER_OF_MAILBOXES.
 *
 *	EXTERNAL VARIABLES:
 *	(struct mailbox *)	sysextMailTbl	table for mailbox-interrupt status information
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *
 *	NOTE:
 *	The function comes in two variations, compiled
 *	separately for the MVME167 and HKBAJA47 targets
 *-
 */

#ifdef TARGET_MV167				/* ------ START OF CODE FOR THE MVME167 ------ */

STATUS sysextMboxDisable
	(
	int	mailbox
	)
{
 	/* NB. This code is specific to the MVME167 target */

   if (mailbox < 0 || mailbox >= SYSEXT_NUMBER_OF_MAILBOXES)
	{
		ERROR_SET (S_sysextLib_INVALID_MAILBOX_NUMBER, "Invalid mailbox IRQ channel #",
		           ERROR_LOG_SAVE);
		return (ERROR);
	}

    /* disable mailbox interrupt */

	BIT_CLR ((HW_REG32 *) VMECHIP2_ILR2, MV167_MBOX_INTERRUPT_LEVEL << ((mailbox + 2) * 4));
	BIT_CLR ((HW_REG32 *) VMECHIP2_LBIER, 1 << (18 + mailbox));

	/*
	 * NB Don't clear MIEN bit in IO control reg #1, since we can't be sure that this bit
	 * hasn't been set by another process in between a previous call to sysextMboxEnable()
	 * (in which the MIEN bit is set) and this function call.
	 */

    return (OK);
}

#endif	/* TARGET_MV167 */		/* ------ END OF CODE FOR THE MVME167 ------ */

#ifdef TARGET_HKBAJA47			/* ------ START OF CODE FOR THE HKBAJA47 ------ */

STATUS sysextMboxDisable
	(
	int	mailbox
	)
{
 	/* NB. This code is specific to the HKBAJA47 target */

	return (sysMailboxDisable (mailbox));				/* Call the Baja's BSP routine */
}


#endif	/* TARGET_HKBAJA47 */	/* ------ END OF CODE FOR THE HKBAJA47 ------ */

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	sysextMboxConnect
 *
 *	INVOCATION:
 *	sysextMboxConnect (routine, mailbox)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	routine	(FUNCPTR)	routine to call at each mailbox interrupt
 *	(>)	mailbox	(int)		number of mailbox interrupt to channel
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK, or ERROR if the mailbox number is invalid or if
 *	intConnect() failed.
 *
 *	PURPOSE:
 *	Connect a routine to a specified mailbox interrupt channel
 *
 *	DESCRIPTION:
 *	This routine specifies a routine to be called at each mailbox interrupt
 *	for one of the mailbox-interrupt channels and initialises (but does not
 *	enable) this interrupt channel. The
 *	mailbox number must be >= 0 and < SYSEXT_NUMBER_OF_MAILBOXES. The
 *	mailbox routine must take a single argument of type int; this argument
 *	is always the mailbox number for the interrupt channel - it is not
 *	possible to specify an arbitrary argument for the routine. Any value
 *	returned by the mailbox routine is ignored.
 *
 *	EXTERNAL VARIABLES:
 *	(struct mailbox *)	sysextMailTbl	table for mailbox-interrupt status information
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *
 *	NOTE:
 *	The function comes in two variations, compiled
 *	separately for the MVME167 and HKBAJA47 targets
 *-
 */

#ifdef TARGET_MV167				/* ------ START OF CODE FOR THE MVME167 ------ */

STATUS sysextMboxConnect
	(
	FUNCPTR	routine,
	int		mailbox
	)
{
	uint8 vectorBase;

 	/* NB. This code is specific to the MVME167 target */

	if (mailbox < 0 || mailbox >= SYSEXT_NUMBER_OF_MAILBOXES)
	{
		ERROR_SET (S_sysextLib_INVALID_MAILBOX_NUMBER, "Invalid mailbox IRQ channel #",
		           ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* save pointer to mailbox routine */
	sysextMailTbl [mailbox].sysextMailboxRoutine = routine;

	/* save mailbox interrupt number */
	sysextMailTbl [mailbox].sysextMailboxArg = mailbox;

	/* compute base vector */
	vectorBase = (((HW_REG32) * VMECHIP2_IOCR) >> 24) & 0xf0;

	/* connect interrupt service routine*/
	if (intConnect ((VOIDFUNCPTR *) INUM_TO_IVEC (vectorBase | (mailbox + 2)),
					(VOIDFUNCPTR) sysextMboxInt, mailbox) == ERROR)
	{
		ERROR_SET (0, "Failed to connect sysextMboxInt to mailbox interrupt", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* mark mailbox as connected */
	sysextMailTbl [mailbox].sysextMailboxConnected = TRUE;

	return (OK);
}

#endif	/* TARGET_MV167 */		/* ------ END OF CODE FOR THE MVME167 ------ */

#ifdef TARGET_HKBAJA47			/* ------ START OF CODE FOR THE HKBAJA47 ------ */


STATUS sysextMboxConnect
	(
	FUNCPTR	routine,
	int		mailbox
	)
{
 	/* NB. This code is specific to the HKBAJA47 target */

	/* Call the Baja's BSP routine */
	return (sysMailboxConnect (routine, mailbox));
}

#endif	/* TARGET_HKBAJA47 */	/* ------ END OF CODE FOR THE HKBAJA47 ------ */

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	IGNORED FUNCTION NAME:
 *	sysextMboxInt
 *
 *	INVOCATION:
 *	sysextMboxInt (mailbox)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	mailbox	(int)	number of interrupting mailbox
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	This is the interrupt-service routine for mailbox interrupts
 *
 *	DESCRIPTION:
 *	This is the interrupt-service routine for mailbox interrupts. It
 *	should not be invoked directly by user code. This routine will
 *	acknowledge a mailbox interrupt and then calls the user-routine
 *	specified via a previous call to sysextMboxConnect(). The argument
 *	to this routine is always the number of the interrupting mailbox.
 *
 *	EXTERNAL VARIABLES:
 *	(struct mailbox *)	sysextMailTbl	table for mailbox-interrupt status information
 *
 *	PRIOR REQUIREMENTS:
 *	sysextMboxConnect() must have been called to specify the user-provided
 *	mailbox routine. Mailbox interrupts cannot occur until they have been
 *	enabled via sysextMboxEnable().
 *
 *	INCLUDE FILES:
 *	sysextLib.h
 *
 *	DEFICIENCIES:
 *	None
 *
 *	NOTE:
 *	This function is compiled for the MVME167 target only, because it
 *	is already contained in the Baja47 BSP
 *-
 */

#ifdef TARGET_MV167				/* ------ START OF CODE FOR THE MVME167 ------ */

void sysextMboxInt
    (
    int mailbox     /* Mbox number */
    )
{
 	/* NB. This code is specific to the MVME167 target */

	/*
	 * Acknowledge interrupt: clear apropriate bit in inter-processor communications
	 * switch register, then call the user-provided interrupt routine (specified when
	 * sysextMboxConnect() was previously called)/
	 */

	* (HW_REG32 *) VMECHIP2_ICLR = 1 << (18 + mailbox);
	(* sysextMailTbl [mailbox].sysextMailboxRoutine) (sysextMailTbl [mailbox].sysextMailboxArg);
}

#endif	/* TARGET_MV167 */		/* ------ END OF CODE FOR THE MVME167 ------ */
