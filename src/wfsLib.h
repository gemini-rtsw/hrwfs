/*+
 *	MODULE NAME:
 *	wfsLib
 *
 *	FILENAME:
 *	wfsLib.h
 *
 *	PURPOSE:
 *	Include file for wfsLib
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/03/17 03:14:27  cboyer
 * Initial creation of the Gemini HRWFS repository
 *
 * Revision 1.13  1998/12/07 11:17:28  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.12  1998/11/30 15:54:47  cics
 * Modifications made during SMB visit to Hilo, November 1998
 *
 * Revision 1.11  1998/10/01 13:49:27  cics
 * Unchanged variables changed to const
 *
 * Revision 1.10  1998/09/28 08:54:12  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks
 *
 * Revision 1.9  1998/06/30 13:26:12  smb
 * Typing mistake fixed. Sorry.
 *
 * Revision 1.8  1998/06/30 13:11:32  smb
 * Dependency on sysextLib and mpPipeDrv can be removed using NO_SYSEXTLIB and NO_MPPIPEDRV macros.
 *
 * Revision 1.7  1998/05/13 10:38:48  smb
 * mpPipeDrv timeout increased
 *
 * Revision 1.6  1998/05/07 09:00:20  smb
 * Declared wfsShow
 *
 * Revision 1.5  1998/03/24 16:16:36  smb
 * Several bugs fixed and notes added
 *
 * Revision 1.4  1998/03/02 14:06:30  smb
 * Site specific parts removed from wfsLib
 *
 * Revision 1.3  1998/02/23 13:38:58  smb
 * Rearranged code for printability
 *
 * Revision 1.2  1998/01/30 15:31:16  smb
 * Fixed some problems uncovered by prolint
 *
 * Revision 1.1.1.1  1997/11/28 11:46:17  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

#ifndef	__INCwfsLibh
#define	__INCwfsLibh


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#endif	/* vxWorks */

#include "gemTypes.h"
#include "gemModNum.h"

#ifndef NO_SYSEXTLIB							/* Define this macro to remove sysextLib	*/
#include "sysextLib.h"
#endif	/* NO_SYSEXTLIB */


/* defines */


#define	PIPE_DRV_TIMEOUT_PROC_0		120.0			/* Pipe driver timeout (in seconds) for	*/
													/* processor 0.							*/
#define	PIPE_DRV_TIMEOUT_PROC_N		-1.0			/* Pipe driver timeout for other		*/
													/* processors. (A negative value means	*/
											 		/* means infinite timeout).				*/
	/*
	 * Error number codes used by wfsLib.
	 * These are designed to be processed using the vxWorks "makeStatTbl" utility.
	 */

#define	S_wfsLib_ERRLOG_PIPE_FAIL	(M_wfsLib | 1)	/* Pipe create or open failed.			*/
#define	S_wfsLib_INVALID_DATA		(M_wfsLib | 2)	/* Invalid data structure defined.		*/
#define	S_wfsLib_BAD_ARGUMENT		(M_wfsLib | 3)	/* Bad argument supplied.				*/
#define	S_wfsLib_NOT_CONNECTED		(M_wfsLib | 4)	/* Database not connected.				*/

	/*
	 * The following defines a data structure to contain information about
	 * the processor used by each wavefront sensor control task.
	 */

typedef struct
	{
	char 		pProcName [80];						/* Processor name.						*/
	uint32		targetType;							/* Processor type (e.g.					*/
													/* TARGET_TYPE_MV167, as defined in		*/
													/* gemTypes.h).							*/
	uint32		procIpAddrs;						/* Processor IP address.				*/
	float		procClockRate;						/* Processor clock rate in Hz.			*/
	uint32		procRamSize;						/* Processor RAM size in bytes.			*/
#ifndef NO_SYSEXTLIB
	VME_MODE	vmeMode;							/* VME mode (as defined in sysextLib.h)	*/
													/* which includes:						*/
													/* - Bus request level,					*/
													/* - Release when done flag,			*/
													/* - Fair requester flag,				*/
													/* - Round robin arbiter flag,			*/
													/* - Request level DMA, and				*/
													/* - Fair requester DMA					*/
#endif	/* NO_SYSEXTLIB */
	} WFS_ARCH_PROCESSOR;


/* function declarations */

IMPORT STATUS	wfsLibInit (void);
IMPORT STATUS	wfsTargetTypeGet (const int processorNumber);
IMPORT int		wfsNumProcsGet (void);
IMPORT STATUS	wfsSysInit (const int processorNumber, const BOOL redirectErrorLog);
IMPORT STATUS	wfsWriteVersion (void);
IMPORT STATUS	wfsShow (void);
IMPORT void		wfsBusReset (void);
IMPORT void     wfsGetTelName (char *pTelName);

#endif /* __INCwfsLibh */
