/*+
 *	MODULE NAME:
 *	errorLog
 *
 *	FILENAME:
 *	errorLog.h
 *
 *	PURPOSE:
 *	Include file for errorLog application code
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.9  1998/12/07 11:17:19  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.8  1998/10/21 16:22:39  cics
 * Error log task no longer use EPICS record daemon
 *
 * Revision 1.7  1998/10/01 13:48:50  cics
 * Minor comment changes
 *
 * Revision 1.6  1998/09/28 08:53:21  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks
 *
 * Revision 1.5  1998/02/18 11:16:27  smb
 * Commands brought up to date with ICD 162/163
 *
 * Revision 1.4  1998/01/30 15:31:14  smb
 * Fixed some problems uncovered by prolint
 *
 * Revision 1.3  1998/01/21 10:46:07  smb
 * Message logging added, plus ability to open file on startup
 *
 * Revision 1.2  1997/12/11 17:34:17  smb
 * Bug noted but not yet fixed
 *
 * Revision 1.1.1.1  1997/11/28 11:46:18  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

#ifndef	__INCerrorLogh
#define	__INCerrorLogh


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif	/* vxWorks */

#include <stdio.h>
#include "gemModNum.h"


/* defines */

/*
 * Define the task name, pipe name, message prefix and initial message size
 * for the error logging task.
 */

#define	LOGTASK_TASK_NAME			"errorLog"
#define	LOGTASK_PIPE_NAME			"errorLog"
#define	LOGTASK_INIT_MSG_PREFIX		"Initial_Error_Count: "
#define	LOGTASK_INIT_MSG_SIZE		21

	/*
	 * Error number codes used by errorLog.
	 * These are designed to be processed using the vxWorks "makeStatTbl" utility.
	 */

#define	S_errorLog_FOPEN_FAIL		(M_errorLog | 1)	/* Error while opening log file	*/
#define	S_errorLog_FCLOSE_FAIL		(M_errorLog | 2)	/* Error while closing log file	*/
#define	S_errorLog_BAD_COMMAND		(M_errorLog | 3)	/* Unrecognised command			*/
#define	S_errorLog_READ_FAILURE		(M_errorLog | 4)	/* Read from pipe failed		*/
#define	S_errorLog_EPICS_ERROR		(M_errorLog | 5)	/* Error reported by EPICS		*/

/* Define the commands recognised by the error logging task.
 * Only one command (set mode) is recognised.
 */

enum
	{
	LOGTASK_CMD_OPEN = 0,		/* Open log file.				*/
	LOGTASK_CMD_CLOSE,			/* Close log file.				*/
	LOGTASK_CMD_CLEAR			/* Clear error counters.		*/
	};

#endif /* __INCerrorLogh */
