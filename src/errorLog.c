static struct {void *v; char *c;} rcsid = {&rcsid,
	"$Id: errorLog.c,v 1.1.1.1 1999-03-17 03:14:22 cboyer Exp $"};

/*+
 *	MODULE NAME:
 *	errorLog
 *
 *	FILENAME:
 *	errorLog.c
 *
 *	PURPOSE:
 *	Message logging task application code
 *
 *	DESCRIPTION:
 *	This file contains the function "errorLog", which is executed by the message
 *	logging task, together with any private functions used by that task alone.
 *	The message logging task handles three types of incoming messages -
 *	(i) Error messages; (ii) Error count declaration messages; and (iii) log
 *	messages. Each of the messages is distinguished by having a unique prefix.
 *	Unrecognised messages are treated as log messages.
 *
 *	PRIOR REQUIREMENTS:
 *	It is assumed that the "select" facility has been initialised in the
 *  root task by calling "selectInit()".
 *
 *	INCLUDE FILES:
 *	errorLib.h
 *	errorLog.h
 *
 *	DEFICIENCIES:
 *	This task assumes a very specific format for the message strings it
 *	receives. Error messages must be of the form
 *v
 *v	PREFIX CODE at line SAVED-LINE in SAVED-FILE, task: TASK-NAME,\n
 *v	"ERROR-MESSAGE" logged at line LOG-LINE in LOG-FILE
 *v
 *	with the whole message being on one line. The task will attempt to parse the
 *	error message to extract the error message code and task name.
 *
 *	NOTE:
 *	When a log file has been opened by this task it may be invisible on
 *	the Unix host until it has been explicitly closed.
 *
 *	To prevent the error logging task from attempting to log errors to itself
 *	it is necessary to redirect its own error messages to "standard error".
 *
 *	BUGS:
 *	This task will fail if an attempt is made to close a log file
 *	opened in "append" mode. The file close aborts with a "file
 *	exists" error and the "command done" message to the CAR daemon
 *	times out. As a work-around, always open a log file in "write"
 *	mode.
 *
 *	There also appears to be a 532 byte memory leak hidden somewhere within
 *	this task. The leak is intermittent and does not necessarily occur each time a
 *	message is reported, but the leak goes away when this task is disabled. At first
 *	I thought the EPICS function iocLogPrintf() was contributing to the leak (as it
 *	seems to eat up memory when called from the console), but the 532 byte leak remains
 *	even when this function is commented out. SMB - 23 JAN 1999.
 *
 *	The mystery has been solved. The leak occurs in the writing of the log file to disk.
 *	I suspect it may be something to do with the caching. SMB - 25 JAN 1999.
 *
 *	ORIGINAL AUTHOR:
 *	Nick Dillon
 *
 *	MODIFIED BY:
 *	Steven Beard
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.35  1998/12/07 11:17:18  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.34  1998/11/30 15:54:38  cics
 * Modifications made during SMB visit to Hilo, November 1998
 *
 * Revision 1.33  1998/10/21 16:05:43  cics
 * Error log task no longer use EPICS record daemon
 *
 * Revision 1.32  1998/10/20 09:28:55  cics
 * Made to write historyLog record
 *
 * Revision 1.31  1998/10/13 08:57:45  cics
 * Make a few more constant variables const
 *
 * Revision 1.30  1998/10/08 16:27:56  cics
 * Always use pipeCreate if mpPipeDrv not being used.
 *
 * Revision 1.29  1998/09/28 08:53:21  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks
 *
 * Revision 1.28  1998/09/17 08:33:23  cics
 * A few messages changed
 *
 * Revision 1.27  1998/09/09 14:36:54  cics
 * Protect console output with semaphore
 *
 * Revision 1.26  1998/08/20 14:48:43  smb
 * Changed opening log message
 *
 * Revision 1.25  1998/08/13 09:08:46  smb
 * Added author comment
 *
 * Revision 1.24  1998/07/16 16:39:07  smb
 * File paths no longer have to end in slash. Fixed problem with image buffer pointer not being returned properly from detObserve. Download DSP code automatically on startup.
 *
 * Revision 1.23  1998/07/09 15:24:17  smb
 * sysextProcNumGet replaced with sysProcNumGet. Error counts declared long instead of int.
 *
 * Revision 1.22  1998/06/30 14:07:14  smb
 * Ensure everything works when sysextLib and mpPipeDrv removed. Fixed mistakes.
 *
 * Revision 1.21  1998/06/29 16:36:09  smb
 * Allow dependencies on sysextLib and mpPipeDrv to be removed by defining NO_SYSEXTLIB and NO_MPPIPEDRV macros.
 *
 * Revision 1.20  1998/05/29 11:30:58  smb
 * Trivial alterations to comments
 *
 * Revision 1.17  1998/03/27 12:02:25  smb
 * Fixed bug when checking for VX_FP_TASK option
 *
 * Revision 1.16  1998/03/05 14:24:12  smb
 * Comment dates made more international
 *
 * Revision 1.15  1998/02/23 13:38:52  smb
 * Rearranged code for printability
 *
 * Revision 1.14  1998/02/18 11:16:27  smb
 * Commands brought up to date with ICD 162/163
 *
 * Revision 1.13  1998/02/05 15:24:54  smb
 * Do not need to include detControl.h
 *
 * Revision 1.12  1998/02/02 17:26:08  smb
 * Free resources if task stopped
 *
 * Revision 1.11  1998/01/30 15:30:13  smb
 * Fixed some problems uncovered by prolint
 *
 * Revision 1.10  1998/01/21 10:46:06  smb
 * Message logging added, plus ability to open file on startup
 *
 * Revision 1.9  1998/01/19 16:18:20  smb
 * Update individual health records
 *
 * Revision 1.8  1998/01/14 11:47:23  smb
 * Quell compiler warning
 *
 * Revision 1.7  1998/01/14 11:22:47  smb
 * Error logging task relabelled as message logging task
 *
 * Revision 1.6  1997/12/15 12:46:25  smb
 * Trivial change to error message
 *
 * Revision 1.5  1997/12/11 17:34:16  smb
 * Bug noted but not yet fixed
 *
 * Revision 1.4  1997/12/11 15:35:04  smb
 * Expand terse error messages - no changes to algorithms
 *
 * Revision 1.3  1997/12/10 17:31:56  smb
 * Added buffer to save up to 4 error messages
 *
 * Revision 1.2  1997/12/05 15:07:43  smb
 * Included task-specific file descriptor
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
#error This code only runs under VxWorks
#endif	/* vxWorks */

#include <pipeDrv.h>
#include <stdlib.h>
#include <stdio.h>
#include <sysLib.h>
#include <taskLib.h>
#include <ioLib.h>
#include <selectLib.h>
#include <string.h>
#include "gemTypes.h"

#ifndef NO_SYSEXTLIB							/* Define this macro to remove sysextLib	*/
#include "sysextLib.h"
#else
#define SYSEXT_MAX_N_PROC	15
#endif	/* NO_SYSEXTLIB */

#ifndef NO_MPPIPEDRV							/* Define this macro to remove mpPipeDrv	*/
#include "mpPipeDrv.h"
#endif	/* NO_MPPIPEDRV */

#include "epToVxLib.h"
#include "errorLib.h"
#include "errorLog.h"
#include "wfsLib.h"

/*
 * The error logging task writes to fields in the EPICS database directly, rather than
 * via the EPICS record daemon, because it always runs on processor 0. wfsDb.h contains
 * the top level record prefix, TOP, and cicsLib.h contains the CICS database access
 * functions.
 */

#include "wfsDb.h"
#include "cicsLib.h"

#ifndef NO_EPICS
#include "epicsPrint.h"
#endif


/* defines */

/* #define DEBUG */								/* Define this macro to enable debug messages.	*/

#define NUM_FILES					100				/* Maximum number of file descriptors		*/
													/* (as defined in ${VX_DIR}/.../config.h)	*/
													

#define	LOGTASK_TIMEOUT_OPENPIPE	-1.0			/* Infinite timeout for opening log pipe.	*/

#define	LOGTASK_DELAY_OPENPIPE		0.1				/* Check for existence of log pipe at this	*/
													/* time interval (in seconds).				*/


#define	LOGTASK_COUNT_SIR_NAME		"errorCount"	/* Name of SIR to contain current error		*/
													/* count.									*/

#define	LOGTASK_PROC_NUM_SIR_NAME	"errorProc"		/* Name of SIR to contain processor number.	*/

#define	LOGTASK_TASK_SIR_NAME		"errorTask"		/* Name of SIR to contain task name.		*/

#define	LOGTASK_ERRNO_SIR_NAME		"errorNumber"	/* Name of SIR to contain current error 	*/
													/* number.									*/

#define	LOGTASK_HISTORY_LOG_NAME	"historyLog"	/* Name of SIR to contain history log		*/
													/* message.									*/

	/*
	 * The following global variables are useful for engineering
	 * but are not expected to be used during normal operation.
	 * They are all distiguished by an "errorLog" prefix.
	 */

BOOL	errorLogStop = FALSE;						/* This flag provides a way of aborting the	*/
													/* the error logging task cleanly.			*/

	/* Private functions. */

LOCAL char *	errorLog_getSubString (char * pString, char * pToken1, char * pToken2,
					char ** ppStringLast);

/* ------------------------------------------------------------------------------------------------ */

STATUS	errorLog (
	const char *	pGivenFileName,		/* Name of log file to be opened		*/
										/* on startup (NULL if no log file is	*/
										/* to be opened until commanded to).	*/
	const BOOL		givenAppendFile		/* Flag set to TRUE if an existing log	*/
										/* file is to be appended to.			*/
	)
{
	/* Variables associated with VxWorks environment. */

	int				procNumber;			/* Processor number.					*/
	int				nProcOnBus;			/* Number of processors on the VME bus.	*/
	int				taskOptions;		/* VxWorks task options.				*/
	STATUS			(* pipeCreate) ();	/* Pointer to pipeCreate function		*/

	/* Variables associated with CAD/CAR command protocol. */

	CAD_CMD_CONTEXT	cadCmdContext;		/* Command context						*/
	int				commandNumber;		/* Command number.						*/
	uint32			errorNumber;		/* Error number reported on completion.	*/

	/* Variables associated with the use of the select() facility. */

	struct fd_set	readFds;			/* File descr. structure for select().	*/
	int				widthSelect;		/* Number of file descrs. to monitor.	*/
	int				pLogFd [NUM_FILES];
										/* File descriptor array.				*/

	/* Variables associated with message logging and log file parameters. */

	long			appendFile;			/* Set to 0/1 to create/append file.	*/
	long			logConsole;			/* Set to 1 to log to console.			*/

	char			pLogFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Path for log file.					*/
	char			pLogFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Name of log file.					*/
	char			pFullFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
										/* Full name of log file, including path.*/

	FILE *			logFileFp;			/* File descriptor for log file.		*/
	BOOL			logFileEnabled = FALSE;
										/* Flag set TRUE when log file open.	*/

	/* Error count variables */

	long			totalErrorCount = 0;
										/* Total error count.					*/
	long			pErrorCount [SYSEXT_MAX_N_PROC];
										/* Error count array.					*/

	/* Other general variables. */

	char			pNameExtension [4];	/* Pipe name extension.					*/
	char			pMsgBuffer [ERROR_LOG_BUFFER_SIZE + 1];
										/* Message buffer.						*/
	char *			pLastMsgToken;		/* Pointer to message token.			*/
	char			pRecordName [EPICS_MAX_BYTES_RECORD_NAME + 1];
										/* Record name.							*/
	char			pHistoryMessage [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* History log message.					*/
	char			pMessage [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Error message string for cicsLib.	*/
	char *			pSubString1;		/* Pointer to substring					*/
	char *			pSubString2;		/* Pointer to substring					*/
	char *			pSubStringLast;		/* Pointer to substring					*/

	int				nByte;				/* Number of bytes read from pipe.		*/


	/* Create and initialise an error context structure for this task */

	if (errorInit () == ERROR)
	{
		printErr ("errorLog: Failed to initialise error context structure.\n");
		return (ERROR);
	}

	/*
	 * Redirect error messages for this particular task to "standard error".
	 * This should prevent the message logging task from attempting to log
	 * errors to itself.
	 */

	errorTaskFdSet( ioGlobalStdGet(2) );			/* 2 means "stderr". */

	/* Check the task executes with floating point co-processor support. */

	if (taskOptionsGet (taskIdSelf (), & taskOptions) == ERROR)
	{
		ERROR_SET (0, "Could not get VxWorks task options", ERROR_LOG_NOW);
		return (ERROR);
	}
	if ((taskOptions & VX_FP_TASK) == 0)
	{
		ERROR_SET (0, "Task must be run with VX_FP_TASK option", ERROR_LOG_NOW);
		return (ERROR);
	}

#ifndef NO_MPPIPEDRV

	/* Obtain the CPU processor number for the task.
	 * A value of -1 indicate the CPU processor number is undefined.
	 * If the processor number is 0, the task resides on the root processor, so the
	 * standard VxWorks pipe driver, pipeDrv, is needed to communicate with it.
	 * Any other value means the task resides on a separate CPU, so the
	 * multi-processor VxWorks pipe driver, mpPipeDrv, is needed to
	 * communicate with it.
	 */

	procNumber = sysProcNumGet ();

	if (procNumber == -1)
	{
		ERROR_SET (0, "CPU processor number not set", ERROR_LOG_NOW);
		return (ERROR);
	}
	else if (procNumber == 0)
	{
		pipeCreate = pipeDevCreate;			/* Use conventional pipe driver.	*/
	}
	else
	{
		pipeCreate = mpPipeDevCreate;		/* Use multi-processor pipe driver.	*/
	}
#else
	pipeCreate = pipeDevCreate;			/* Without MPPIPEDRV always use conventional pipe driver.	*/

#endif	/* NO_MPPIPEDRV */

	/*
	 * Get the CAD command context structure (using the appropriate pipe driver)
	 * which is used subsequently as a handle for the CAD/CAR command-input and
	 * response-generation routines.
	 */

#ifdef DEBUG
	printf ("errorLog: Getting CAD command context\n" );
#endif /* DEBUG */

	if ((cadCmdContext = epToVxCmdInit (NULL, pipeCreate)) == NULL)
	{
		ERROR_LOG ("Error getting CAD command context");
		return (ERROR);
	}

	/*
	 * The number of bits that need to be monitored in the "select" function
	 * (used later) needs to be set to the maximum file descriptor value in
	 * use. Set the initial value to the value of the file descriptor used to
	 * communicate CAD commands plus 1.
	 */

	widthSelect = cadCmdContext->cadPipeFd + 1;

	/*
	 * Find out the number of processors on the VME bus and check this is within
	 * a sensible range. The log task will be responsible for collecting error
	 * messages from each of these processors.
	 */

	nProcOnBus = wfsNumProcsGet ();

	if ((nProcOnBus <= 0) || (nProcOnBus > SYSEXT_MAX_N_PROC))
	{
		ERROR_SET1 (0, "Invalid number of processors on VME bus, %d", ERROR_LOG_NOW, nProcOnBus);
		return (ERROR);
	}

#ifdef DEBUG
	printf ("errorLog: There are %d processors on the VME bus\n", nProcOnBus);
#endif /* DEBUG */

	/*
	 * For each processor on the bus, generate a pipe name based on
	 * the processor number and open the pipe, writing each file descriptor
	 * into the pLogFd array. Each time a new largest file descriptor is
	 * found, update the "widthSelect" variable to be used by "select()"
	 * later.
	 */

	for (procNumber = 0; procNumber < nProcOnBus; procNumber++)
	{

#ifdef DEBUG
		printf ("errorLog: Opening pipe to processor %d\n", procNumber);
#endif /* DEBUG */

		sprintf (pNameExtension, "%.2d", procNumber);
		pLogFd [procNumber] = epToVxPipeOpen (FALSE, LOGTASK_PIPE_NAME, pNameExtension,
		                                      NULL, 0, 0, O_RDONLY, -1, LOGTASK_TIMEOUT_OPENPIPE,
		                                      LOGTASK_DELAY_OPENPIPE);
		if (pLogFd [procNumber] == ERROR)
		{
			ERROR_LOG ("Error opening log pipe");
			return (ERROR);
		}
		if ((pLogFd [procNumber] + 1) > widthSelect) widthSelect = pLogFd [procNumber] + 1;
	}

	/*
	 * If required, open a log file straight away.
	 */

	if ( pGivenFileName != NULL )
	{
		MESSAGE_LOG3 (MSG_LOG, "Opening initial log file \"%s\" in mode %d=\"%s\".",
		              pGivenFileName, givenAppendFile, (givenAppendFile == 1 ? "a" : "w"));

		logFileFp = fopen (pGivenFileName, givenAppendFile == 1 ? "a" : "w");
		if (logFileFp == NULL)
		{
			ERROR_SET1 (0, "Failed to open initial log file, %s", ERROR_LOG_NOW, pGivenFileName);
			errorNumber = S_errorLog_FOPEN_FAIL;
		}
		else
		{
			logFileEnabled = TRUE;
		}
	}

	/*
	 * The task has been successfully initialised, so it can now go into
	 * a loop waiting for commands or error messages from other tasks.
	 * The error log task can be terminated by setting the "errorLogStop"
	 * variable from the console.
	 */

	MESSAGE_LOG1 (MSG_MINDEBUG, "Entering loop waiting for commands or error messages... "
		"pCmdPacket=0x%x", (int) cadCmdContext->pCmdPacket);

	while (! errorLogStop)
	{

		/*
		 * Zero all the bits in the file descriptor read structure and then
		 * set each bit corresponding to the file descriptors of the pipes
		 * opened to each of the processors on the bus. Also set the bit
		 * corresponding to the pipe used to receive CAD commands..
		 */

		FD_ZERO (& readFds);
		for (procNumber = 0; procNumber < nProcOnBus; procNumber++)
		{
			FD_SET (pLogFd [procNumber], & readFds);
		}
		FD_SET (cadCmdContext->cadPipeFd, & readFds);

		/*
		 * Wait for an input from any of the file descriptors set above.
		 * There is no timeout.
		 */

		if (select (widthSelect, & readFds, NULL, NULL, NULL) == ERROR)
		{
			ERROR_SET (0, "File descriptor selection function, select(), failed", ERROR_LOG_NOW );
			return (ERROR);
		}

		/*
		 * Check whether an input has come from the pipe communicating CAD
		 * commands.
		 */

		if (FD_ISSET (cadCmdContext->cadPipeFd, & readFds))
		{

			/*
			 * Initialise the error number and read the command number from the pipe.
 			 * The errorLog task is aborted if it fails to read a command.
			 */

			errorNumber = 0;
			if ((commandNumber = epToVxCmdRead (cadCmdContext)) < 0)
			{
				ERROR_LOG ("Error reading CAD command - task aborted");
				return (ERROR);
			}

			MESSAGE_LOG1 (MSG_FULLDEBUG, "CAD command #%d received", commandNumber);

			/*
			 * Switch according to the command number received.
			 */

			switch (commandNumber)
			{
				case LOGTASK_CMD_OPEN:

					/*
					 * Open log file command received.
					 * Obtain the four attributes, which are:
					 * 1 - Log file path
					 * 2 - Log file name
					 * 3 - Append file flag
					 * 4 - Log console flag flag
					 */

					EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pLogFilePath);
					EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pLogFileName);
					EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *) & appendFile);
					EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *) & logConsole);

					/*
					 * If a log file has already been enabled, close it.
					 */

					if (logFileEnabled)
					{
						MESSAGE_LOG (MSG_LOG, "Closing current log file");

						if (fclose (logFileFp) == EOF)
						{
							ERROR_SET (0, "Failed to close log current file", ERROR_LOG_NOW);
							errorNumber = S_errorLog_FCLOSE_FAIL;
						}
						logFileEnabled = FALSE;
					}

					/*
					 * If successful so far, attempt to open the new log file.
					 */

					if (errorNumber == 0)
					{

						sprintf (pFullFileName, "%s/%s", pLogFilePath, pLogFileName);
						MESSAGE_LOG3 (MSG_LOG, "Opening log file \"%s\" in mode %ld=\"%s\".",
							          pFullFileName, appendFile, (appendFile == 1 ? "a" : "w"));

						if ((logFileFp = fopen (pFullFileName, appendFile == 1 ? "a" : "w"))
							== NULL)
						{
							ERROR_SET1 (0, "Failed to open log file, %s", ERROR_LOG_NOW,
							            pLogFileName);
							errorNumber = S_errorLog_FOPEN_FAIL;
						}
						else
						{
							logFileEnabled = TRUE;

							/* Copy messages to the console if specified. */

							if (logConsole)
							{
								MESSAGE_LOG (MSG_MINDEBUG, "Messages will be copied to the console.")
							}
						}
					}

					break;

				case LOGTASK_CMD_CLOSE:

					/*
					 * Close log file command received.
					 * If a log file is enabled close it, otherwise report an error.
					 */

					if (logFileEnabled)
					{
						MESSAGE_LOG (MSG_LOG, "Closing log file");

						if (fclose (logFileFp) == EOF)
						{
							ERROR_SET (0, "Failed to close log file", ERROR_LOG_NOW);
							errorNumber = S_errorLog_FCLOSE_FAIL;
						}
						logFileEnabled = FALSE;
					}
					else
					{
						ERROR_SET (S_errorLog_FCLOSE_FAIL, "Log file not open", ERROR_LOG_NOW);
						errorNumber = S_errorLog_FCLOSE_FAIL;
					}

					break;

				case LOGTASK_CMD_CLEAR:

					/*
					 * Clear error count command received.
					 * Zero all the error counts and update the corresponding SIR records.
					 */

					MESSAGE_LOG (MSG_LOG, "Zeroing error counts");

					for (procNumber = 0; procNumber < nProcOnBus; procNumber++)
					{
						pErrorCount [procNumber] = 0;

						sprintf (pRecordName, TOP LOGTASK_COUNT_SIR_NAME "%.2d", procNumber);

						if (cicsDbPut (pRecordName, pMessage, DBF_LONG,
						               (void *) &pErrorCount [procNumber])
						    == ERROR)
						{
							ERROR_SET (S_errorLog_EPICS_ERROR, pMessage, ERROR_LOG_SAVE);
							ERROR_SET1 (0, "Failed to zero error count for processor %d",
								ERROR_LOG_NOW, procNumber);
							printErr ("%s: Failed (%s) to zero error-count for processor %d.\n",
								taskName (taskIdSelf ()), pMessage, procNumber);
						}
					}
					totalErrorCount = 0;

					break;

				default:

					ERROR_SET (S_errorLog_BAD_COMMAND, "Command not currently implemented",
					           ERROR_LOG_NOW);
					errorNumber = S_errorLog_BAD_COMMAND;
			}

			/* Finish the command. */

			if (epToVxCmdFinish (cadCmdContext, errorNumber) == ERROR)
			{
				ERROR_LOG ("Error finishing command");
			}
		}

		/*
		 * Check whether an input has come from one of the pipes communicating
		 * error messages from other processors.
		 */

		for (procNumber = 0; procNumber < nProcOnBus; procNumber++)
		{
			if (FD_ISSET (pLogFd [procNumber], & readFds))
			{
				/*
				 * Read the message from the pipe and check it has a sensible size.
				 */

				nByte = read (pLogFd [procNumber], pMsgBuffer, ERROR_LOG_BUFFER_SIZE);

				if ( (nByte > 0) && (nByte <= ERROR_LOG_BUFFER_SIZE) )
				{

					/*
					 * Three kinds of message can be communicated:
					 * (1) Messages with a standard error message prefix
					 *     communicate error messages.
					 * (2) Messages with an "initial error count" prefix
					 *     communicate the initial error count of a processor.
					 * (3) Messages with a log message prefix communicate
					 *     informational log messages.
					 * The type of message will now be checked.
					 */

					if (strncmp (pMsgBuffer, ERROR_LOG_MSG_PREFIX, sizeof (ERROR_LOG_MSG_PREFIX) - 1)
					    == 0)
					{

						/*
						 * (1) The message starts with the standard error message
						 *     prefix.
						 */

						/*
						 * If a log file is enabled, write the error message to the
						 * log file. If console logging is switched on, also
						 * display that message on the console.
						 * If a log file isn't open, then simply display the message.
						 * The errorConsoleSem semaphore is imported from errorLib.c
						 */

						if (logFileEnabled)
						{
							fprintf (logFileFp, pMsgBuffer);
							fflush (logFileFp);
							if (logConsole) printf (pMsgBuffer);
						}
						else
						{
							if (semTake(errorConsoleSem, WAIT_FOREVER) != ERROR)
							{
								printf (pMsgBuffer);
								semGive (errorConsoleSem);
							}
							else
							{
								printErr ("errorLog: Could not take errorConsoleSem semaphore "
									"to display \"%s\"", pMsgBuffer);
							}
						}

						/*
						 * Increment the error count for the appropriate processor.
						 */

						pErrorCount [procNumber]++;
						totalErrorCount++;

						/*
						 * Now parse the error message string, extracting the
						 * processor error number and task name, and update the
						 * error count, processor number, error number and task
						 * name SIR records
						 */

						pSubString1 = errorLog_getSubString (pMsgBuffer, ERROR_LOG_MSG_PREFIX,
							" at line ", & pSubStringLast);
						pSubString2 = errorLog_getSubString (pSubStringLast, "task: ", ",\n",
							& pSubStringLast);
						sprintf (pRecordName, TOP LOGTASK_COUNT_SIR_NAME "%.2d", procNumber);

						if ( (cicsDbPut (pRecordName, pMessage, DBF_LONG,
						                 (void *) & pErrorCount [procNumber])
						      == ERROR) ||
						     (cicsDbPut (TOP LOGTASK_PROC_NUM_SIR_NAME, pMessage, DBF_LONG,
						                 (void *) & procNumber)
						      == ERROR) ||
						     (cicsDbPut (TOP LOGTASK_ERRNO_SIR_NAME, pMessage, DBF_STRING,
						                 (void *) pSubString1)
						      == ERROR) ||
						     (cicsDbPut (TOP LOGTASK_TASK_SIR_NAME, pMessage, DBF_STRING,
						                 (void *) pSubString2)
						      == ERROR)
						   )
						{
							ERROR_SET (S_errorLog_EPICS_ERROR, pMessage, ERROR_LOG_SAVE);
							ERROR_SET2 (0, "Failed to log error \"%s\" for task %s", ERROR_LOG_NOW,
								pSubString1, pSubString2);
							printErr (
								"%s: Failed (%s) to log error \"%s\" for processor %d, task %s\n",
								taskName (taskIdSelf ()), pMessage, pSubString1, procNumber,
								pSubString2);
						}
					}

					else if (strncmp (pMsgBuffer, LOGTASK_INIT_MSG_PREFIX,
					         sizeof (LOGTASK_INIT_MSG_PREFIX) - 1) == 0)
					{

						/*
						 * (2) The message starts with the "initial error count
						 *     prefix.
						 */

						/*
						 * Decode the message and obtain the error count, assuming
						 * it is contained in the last word of the string.
						 */

						pLastMsgToken = NULL;
						strtok_r (pMsgBuffer, " ", & pLastMsgToken);
						pErrorCount [procNumber] = atol (pLastMsgToken);

						/*
						 * Add the number obtained to the total error count,
						 * report the error count, and write the value to the
						 * error count SIR record.
						 */

						totalErrorCount += pErrorCount [procNumber];
#ifdef DEBUG
						printf ("errorLog: Initial Error Count, proc %d=%d, System Total=%d\n",
						        procNumber, pErrorCount [procNumber], totalErrorCount);
#endif /* DEBUG */
						sprintf (pRecordName, TOP LOGTASK_COUNT_SIR_NAME "%.2d", procNumber);
						if (cicsDbPut (pRecordName, pMessage, DBF_LONG,
						               (void *) & pErrorCount [procNumber])
						    == ERROR)
						{
							ERROR_SET (S_errorLog_EPICS_ERROR, pMessage, ERROR_LOG_SAVE);
							ERROR_SET1 (0, "Failed to set initial error count for processor %d",
							           ERROR_LOG_NOW, procNumber);
							printErr (
								"%s: Failed (%s) to set initial error count (%d) for processor %d\n",
								taskName (taskIdSelf ()), pMessage,	pErrorCount [procNumber],
								procNumber);
						}
					}
					else
					{

						/*
						 * (3) Assume any other kind of message is a log message.
						 */

						/*
						 * If a log file is enabled, write the error message to the
						 * log file. If console logging is switched on, also
						 * display that message on the console.
						 * If a log file isn't open, then simply display the message.
						 */

						if (logFileEnabled)
						{
							fprintf (logFileFp, pMsgBuffer);
							fflush (logFileFp);
							if (logConsole) printf (pMsgBuffer);
						}
						else
						{
							if (semTake(errorConsoleSem, WAIT_FOREVER) != ERROR)
							{
								printf (pMsgBuffer);
								semGive (errorConsoleSem);
							}
							else
							{
								printErr("errorLog: Could not take errorConsoleSem semaphore "
									"to display \"%s\"", pMsgBuffer);
							}
						}

						/*
						 * If EPICS is available, direct the log message to the EPICS log server
						 * and write it to the historyLog record.
						 */

#ifndef NO_EPICS

						/*
						 * BUG WORK AROUND: I suspect there is a memory leak in the EPICS iocLogPrintf()
						 * function. Try commenting it out.
						 */

						/* iocLogPrintf( pMsgBuffer ); */

						/*
						 * Truncate the message so it will fit, and write it to the
						 * history log record.
						 */

						strncpy (pHistoryMessage, pMsgBuffer, EPICS_MAX_BYTES_STRING_ATTRIB);
						if (cicsDbPut (TOP LOGTASK_HISTORY_LOG_NAME, pMessage, DBF_STRING,
						               (void *) pHistoryMessage)
						    == ERROR)
						{
							ERROR_SET (S_errorLog_EPICS_ERROR, pMessage, ERROR_LOG_SAVE);
							ERROR_LOG ("Failed to write history log record");
							printErr ("Failed (%s) to write \"%s\" to history log record\n",
								pMessage, pHistoryMessage);
						}

#endif	/* NO_EPICS */
					}
				}
				else
				{
					/*
					 * The number of bytes read from the pipe is either less than or equal to zero
					 * or greater than the buffer size. Something has gone wrong.
					 */

					ERROR_SET1 (S_errorLog_READ_FAILURE,
						"Invalid number of bytes read from pipe (%d)", ERROR_LOG_NOW, nByte);
				}
			}
		}
	}

	/*
	 * The task has been stopped.
	 * Close the log file, if open, and tidy up any allocated resources.
	 */

	if (logFileEnabled)
	{
		MESSAGE_LOG (MSG_LOG, "Task closing down - closing log file");

		if (fclose (logFileFp) == EOF)
		{
			ERROR_SET (0, "Failed to close log file", ERROR_LOG_NOW);
			errorNumber = S_errorLog_FCLOSE_FAIL;
		}
	}

	MESSAGE_LOG (MSG_WARNING, "Message logging task stopped.");

	epToVxCmdFree (cadCmdContext);
	errorFlush();
	errorFree();

	return (OK);
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	IGNORED FUNCTION NAME:
 *	errorLog_getSubString
 *
 *	INVOCATION:
 *	errorLog_getSubString (pString, pToken1, pToken2, ppStringLast )
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pString			(char *)	String to be searched
 *	(>)	pToken1			(char *)	First token
 *	(>)	pToken2			(char *)	Second token
 *	(<)	ppStringLast	(char **)	Pointer to remaining part of string.
 *
 *	FUNCTION VALUE:
 *	(char *)	Start of sub-string.
 *
 *	PURPOSE:
 *	Get sub-string which is bounded by 2 tokens
 *
 *	DESCRIPTION:
 *	This is a private function used only by errorLog.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	<string.h>.
 *
 *	DEFICIENCIES:
 *	None
 *-
 */

char *	errorLog_getSubString
	(
	char *	pString,		/* String to be searched. */
	char *	pToken1,		/* First token. */
	char *	pToken2,		/* Second token. */
	char **	ppStringLast	/* Pointer to remaining part of string. */
	)
{
	char *	pStart;

	/* Get pointer to 1st token */
	pStart = strstr (pString, pToken1);

	if (pStart != NULL && pToken2 != NULL)
	{
		/* Return value points to 1st char following the 1st token. */
		pStart += strlen (pToken1);

		/* Get ptr to 2nd token */
		if ((* ppStringLast = strstr (pStart, pToken2)) != NULL)
		{
			/* Put null terminator at start of 2nd token. */
			** ppStringLast = 0;

			/* Set last-string-ptr to 1st char following the 2nd token. */
			* ppStringLast += strlen (pToken2);
		}
	}

	return (pStart);
}
