/*+
 *	MODULE NAME:
 *	sysextLib
 *
 *	FILENAME:
 *	sysextLib.h
 *
 *	PURPOSE:
 *	Include file for sysextLib.
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.6  1998/12/07 11:17:24  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.5  1998/10/01 13:44:37  cics
 * Minor comment changes.
 *
 * Revision 1.4  1998/09/28 08:51:57  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks. Deficiencies warning added.
 *
 * Revision 1.3  1998/03/05 14:20:13  smb
 * Merged with changes made by Bret Goodrich
 *
 * Revision 1.2  1998/02/23 13:38:54  smb
 * Rearranged code for printability
 *
 * Revision 1.1.1.1  1997/11/28 11:46:17  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

#ifndef	__INCsysextLibh
#define	__INCsysextLibh

/* Generic includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks (and only then if you are lucky)
#endif	/* vxWorks */

#include <vme.h>
#include <sysLib.h>
#include "gemTypes.h"

/* defines  */

	/*
	 * Glossary:
	 *
	 * The term "BLT" is shorthand for "VME block transfer".
	 * AM stands for "addressing mode", which can be short (sht or A16), standard (std or A24)
	 * or extended (ext or A32).
	 */

#define	SYSEXT_MAX_N_PROC					15			/* Maximum # CPUs on VME bus				*/
#define	SYSEXT_NUMBER_OF_MAILBOXES			4			/* Number of mailbox interrupt regs			*/
#define	SYSEXT_SHT_NBYTE_PER_MV167			0x00000010	/* # bytes reserved per MV167 CPU, short AM	*/
#define	SYSEXT_SHT_NBYTE_PER_HKBAJA47		0x00000100	/* # bytes reserved per Baja CPU, short AM	*/
#define	SYSEXT_STD_NBYTE_PER_PROC			0x00200000	/* # bytes reserved per generic CPU, std AM	*/
#define	SYSEXT_EXT_NBYTE_PER_PROC			0x04000000	/* # bytes reserved per generic CPU, ext AM	*/
#define	SYSEXT_SHT_VME_BASE					0x00000000
#define	SYSEXT_STD_VME_BASE					0x00000000
#define SYSEXT_EXT_VME_BASE					0xc0000000

	/*
	 * Error number codes used by sysextLib.
	 * These are designed to be processed using the vxWorks "makeStatTbl" utility.
	 */

#define	S_sysextLib_INVALID_TARGET			(M_sysextLib | 1)
#define	S_sysextLib_INV_NUM_CPU_IN_NETWORK	(M_sysextLib | 2)
#define	S_sysextLib_INVALID_MAILBOX_NUMBER	(M_sysextLib | 3)
#define	S_sysextLib_INVALID_LOCAL_ADRS		(M_sysextLib | 4)
#define	S_sysextLib_INVALID_VME_ADRS		(M_sysextLib | 5)
#define	S_sysextLib_INVALID_MEMORY_SPACE	(M_sysextLib | 6)
#define	S_sysextLib_VME_BLOCK_TRANSFER_FAIL	(M_sysextLib | 7)
#define	S_sysextLib_INVALID_CPU_NUMBER		(M_sysextLib | 8)
#define	S_sysextLib_CPU_NUM_ALREADY_USED	(M_sysextLib | 9)
#define	S_sysextLib_MAILBOX_NOT_CONNECTED	(M_sysextLib | 10)
#define	S_sysextLib_UNSUPPORTED_ON_TARGET	(M_sysextLib | 11)

/* typedefs */

typedef struct											/* Defines VME bus mode						*/
	{
		int		requestLevel;
		BOOL	releaseWhenDone;
		BOOL	fairRequester;
		BOOL	roundRobinArbiter;
		int		requestLevelDma;
		BOOL	fairRequesterDma;
	} VME_MODE;

/* static variables */

enum
	{													/* VME transfer modes						*/
		VME_BLT_NONE = 0,								/* Programmed IO only, no BLT				*/
		VME_BLT_D32,									/* Attempt D32 BLT, then non-BLT			*/
		VME_BLT_D64										/* Attempt D64 BLT, then D32, then non-BLT	*/
	};

/* function declarations */

IMPORT STATUS sysextProcNumSet   		(int processorNumber);
IMPORT int	  sysextProcNumGet     		(void);
IMPORT STATUS sysextLocalToBusAdrs		(int adrsSpace, char * pLocalAdrs, char ** ppBusAdrs);
IMPORT STATUS sysextBusToLocalAdrs		(int adrsSpace, char * pLocalAdrs, char ** ppBusAdrs);
IMPORT STATUS sysextMboxConnect			(FUNCPTR routine, int mailbox);
IMPORT STATUS sysextMboxEnable  		(int mailbox);
IMPORT STATUS sysextMboxDisable			(int mailbox);
IMPORT STATUS sysextMboxIntGen			(int processorNumber, int mailbox);
IMPORT STATUS sysextMboxBusAdrs			(int mailbox, char ** ppMboxAdrs, char * pMboxData);
IMPORT void   sysextWriteLocalMboxReg8  (uint8 data);
IMPORT void   sysextWriteLocalMboxReg32 (uint32 data);
IMPORT STATUS sysextReadRemoteMboxReg8  (int processorNumber, uint8  * pData);
IMPORT STATUS sysextReadRemoteMboxReg32 (int processorNumber, uint32 * pData);
IMPORT STATUS sysextVmeBlockInit		(void);
IMPORT STATUS sysextVmeBlockCopy		(const char * source, char * destination, int nByte,
										 BOOL toVme, int mode);
IMPORT void   sysextVmeReqRelInit		(VME_MODE * vmeMode);
IMPORT STATUS sysextVmeNetworkInit		(int numProcessor, uint32 *	pTargetType1);
IMPORT int    sysextNProcGet			(void);
IMPORT STATUS sysextRebootRemote		(int processorNumber);
IMPORT STATUS sysextBusReset			(void);

#endif	/* __INCsysextLibh */
