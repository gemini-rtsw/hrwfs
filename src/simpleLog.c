static struct {void *v; char *c;} rcsid = {&rcsid,
	"$Id: simpleLog.c,v 1.1.1.1 1999-03-17 03:14:24 cboyer Exp $"};

/*+
 *	MODULE NAME:
 *	simpleLog
 *
 *	FILENAME:
 *	simpleLog.c
 *
 *	PURPOSE:
 *	Message logging task application code - SIMPLE VERSION
 *
 *	DESCRIPTION:
 *	This file contains the function "simpleLog", which is executed by the message
 *	logging task. This is a much simplified version of the errorLog task written by Nick
 *	Dillon (which had memory leak bugs).
 *
 *	PRIOR REQUIREMENTS:
 *	It is assumed that the "select" facility has been initialised in the
 *  root task by calling "selectInit()".
 *
 *	INCLUDE FILES:
 *	errorLib.h
 *	simpleLog.h
 *
 *	DEFICIENCIES:
 *	This task has had to be simplified from Nick Dillon's version because that
 *	version was over-complicated and had a memory leak problem.
 *
 *	ORIGINAL AUTHOR:
 *	Nick Dillon
 *
 *	SIMPLIFIED BY:
 *	Steven Beard
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
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
/*
 * Define the task name, pipe name, message prefix and initial message size
 * for the error logging task.
 */

#define	LOGTASK_HISTORY_LOG_NAME0	"historyLog"	/* Name of SIR to contain history log		*/
													/* message.									*/
#define	LOGTASK_HISTORY_LOG_NAME1	"historyLog1"	/* Name of SIR to contain history log		*/
													/* message overflow.						*/
#define	LOGTASK_ERROR_LOG_NAME0		"errorLog"		/* Name of SIR to contain error log			*/
													/* message.									*/
#define	LOGTASK_ERROR_LOG_NAME1		"errorLog1"		/* Name of SIR to contain error log			*/
													/* message overflow.						*/

	/*
	 * The following global variables are useful for engineering
	 * but are not expected to be used during normal operation.
	 * They are all distiguished by an "simpleLog" prefix.
	 */

BOOL	simpleLogStop = FALSE;						/* This flag provides a way of aborting the	*/
													/* the error logging task cleanly.			*/


/* ------------------------------------------------------------------------------------------------ */

STATUS	simpleLog (
	const char *	pGivenFileName,		/* Name of log file to be used (NULL	*/
										/* if no log file is to be used).		*/
	const int		nFlush				/* Flush the file every nFlush message.	*/
	)
{
	/* Variables associated with VxWorks environment. */

	int				procNumber;			/* Processor number.					*/
	int				nProcOnBus;			/* Number of processors on the VME bus.	*/
	int				taskOptions;		/* VxWorks task options.				*/
	STATUS			(* pipeCreate) ();	/* Pointer to pipeCreate function		*/

	/* Variables associated with the use of the select() facility. */

	struct fd_set	readFds;			/* File descr. structure for select().	*/
	int				widthSelect;		/* Number of file descrs. to monitor.	*/
	int				pLogFd [NUM_FILES];
										/* File descriptor array.				*/

	/* Variables associated with the log file. */

	char			pLogFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Name of log file.					*/

	FILE *			logFileFp;			/* File descriptor for log file.		*/
	BOOL			logFileEnabled = FALSE;
										/* Flag set TRUE when log file open.	*/

	/* Other general variables. */

	char			pNameExtension [4];	/* Pipe name extension.					*/
	char			pMsgBuffer [ERROR_LOG_BUFFER_SIZE + 1];
										/* Message buffer.						*/
	char			pHistoryMessage [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* History log message.					*/
	char			pMessage [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Error message string for cicsLib.	*/
	char *			pSubString;			/* Pointer to substring.				*/
	char *			pStart;				/* Pointer to start of error message.	*/
	char *			pStartPrev;			/* Previous value of pStart.			*/

	int				nByte;				/* Number of bytes read from pipe.		*/

	int				counter;			/* Message counter.						*/


	/* Create and initialise an error context structure for this task */

	if (errorInit () == ERROR)
	{
		printErr ("simpleLog: Failed to initialise error context structure.\n");
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
	printf ("simpleLog: There are %d processors on the VME bus\n", nProcOnBus);
#endif /* DEBUG */

	/*
	 * For each processor on the bus, generate a pipe name based on
	 * the processor number and open the pipe, writing each file descriptor
	 * into the pLogFd array. Each time a new largest file descriptor is
	 * found, update the "widthSelect" variable to be used by "select()"
	 * later.
	 */

	widthSelect = 0;
	for (procNumber = 0; procNumber < nProcOnBus; procNumber++)
	{

#ifdef DEBUG
		printf ("simpleLog: Opening pipe to processor %d\n", procNumber);
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
	 * If required, open a log file.
	 */

	if ( pGivenFileName != NULL )
	{
		MESSAGE_LOG2 (MSG_LOG, "Opening log file %s, flushed every %d messages",
			pGivenFileName, nFlush);
		logFileFp = fopen (pGivenFileName, "w");
		if (logFileFp == NULL)
		{
			ERROR_SET1 (0, "Failed to open log file, %s", ERROR_LOG_NOW, pGivenFileName);
			logFileEnabled = FALSE;
		}
		else
		{
			logFileEnabled = TRUE;
		}
	}
	else
	{
		logFileEnabled = FALSE;
	}

	/*
	 * Zero the message counter.
	 */

	counter = 0;

	/*
	 * The task has been successfully initialised, so it can now go into
	 * a loop waiting for commands or error messages from other tasks.
	 * The error log task can be terminated by setting the "simpleLogStop"
	 * variable from the console.
	 */

	MESSAGE_LOG (MSG_MINDEBUG, "Entering loop waiting for messages...");

	while (! simpleLogStop)
	{

		/*
		 * Zero all the bits in the file descriptor read structure and then
		 * set each bit corresponding to the file descriptors of the pipes
		 * opened to each of the processors on the bus.
		 */

		FD_ZERO (& readFds);
		for (procNumber = 0; procNumber < nProcOnBus; procNumber++)
		{
			FD_SET (pLogFd [procNumber], & readFds);
		}

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
					 * Log the message to the console. The errorConsoleSem semaphore is
					 * used to prevent messages from different tasks overlapping on the
					 * console.
					 */

					if (semTake (errorConsoleSem, WAIT_FOREVER) != ERROR)
					{
						printf (pMsgBuffer);
						semGive (errorConsoleSem);
					}
					else
					{
						printErr("simpleLog: Could not take errorConsoleSem semaphore "
							"to display \"%s\"", pMsgBuffer);
					}

					/*
					 * If required, write the message to the log file.
					 * Increment the message counter and every nFlush messages close and
					 * reopen the file.
					 */

					counter = (counter + 1) % nFlush;

					if ( logFileEnabled )
					{
						fprintf (logFileFp, pMsgBuffer);

						if ( counter == 0 )
						{
							MESSAGE_LOG (MSG_FULLDEBUG, "Flushing log file");

							if (fclose (logFileFp) == EOF)
							{
								ERROR_SET (0, "Failed to close log file", ERROR_LOG_NOW);
							}

							logFileFp = fopen (pGivenFileName, "a");
							if (logFileFp == NULL)
							{
								ERROR_SET1 (0, "Failed to reopen log file, %s", ERROR_LOG_NOW,
									pGivenFileName);
								logFileEnabled = FALSE;
							}
							else
							{
								logFileEnabled = TRUE;
							}
						}
					}

					/*
					 * If EPICS is available, direct the log message to the EPICS log server
					 * and write it to the historyLog record.
					 */

#ifndef NO_EPICS
					/*
					 * BUG WORK AROUND: Try commenting this out.
					 */

					/* iocLogPrintf( pMsgBuffer ); */

					/*
					 * Determine whether the message is a log message or an error message
					 * by checking its prefix, and direct it to the appropriate EPICS records.
					 */

					if (strncmp (pMsgBuffer, ERROR_LOG_MSG_PREFIX, sizeof (ERROR_LOG_MSG_PREFIX) - 1)
					    == 0)
					{
						/*
						 * The message starts with the standard error message prefix.
						 *
						 * Error messages are too long to fit into EPICS history records,
						 * so locate the important message at the end after the first, second
						 * or third occurrence of the newline string ",\n". If a newline string
						 * cannot be found, or if there is a newline string right at the end,
						 * the entire message string is displayed instead.
						 */

						/* Look for the first newline, if any */
						pStart =  strstr (pMsgBuffer, ",\n");
						if ( (pStart == NULL) || (strlen (pStart) < 1) )
						{
							pStart = pMsgBuffer;
						}
						else
						{
							/* Add 2 to jump over the ",\n" characters. */
							pStart += 2;

							/* Look for the second newline, if any */
							pStartPrev = pStart;
							pStart =  strstr (pStartPrev, ",\n");
							if ( (pStart == NULL) || (strlen (pStart) < 1) )
							{
								pStart = pStartPrev;
							}
							else
							{
								/* Add 2 to jump over the ",\n" characters. */
								pStart += 2;

								/* Look for the third newline, if any */
								pStartPrev = pStart;
								pStart =  strstr (pStartPrev, ",\n");
								if ( (pStart == NULL) || (strlen (pStart) < 1) )
								{
									pStart = pStartPrev;
								}
								else
								{
									/* Add 2 to jump over the ",\n" characters. */
									pStart += 2;
								}
							}
						}

						/*
						 * Truncate the message so it will fit, and write it to the
						 * error log record(s).
						 */

						strncpy (pHistoryMessage, pStart, EPICS_MAX_BYTES_STRING_ATTRIB);
						if (cicsDbPut (TOP LOGTASK_ERROR_LOG_NAME0, pMessage, DBF_STRING,
						               (void *) pHistoryMessage)
						    == ERROR)
						{
							ERROR_SET (S_errorLog_EPICS_ERROR, pMessage, ERROR_LOG_SAVE);
							ERROR_LOG ("Failed to write error log record");
							printErr ("Failed (%s) to write \"%s\" to error log record\n",
								pMessage, pHistoryMessage);
						}

						/*
						 * If the message is longer than EPICS_MAX_BYTES_STRING_ATTRIB characters
						 * copy the next part of the message into the history log overflow record.
						 */

						if ( strlen(pStart) > EPICS_MAX_BYTES_STRING_ATTRIB )
						{
							pSubString = pStart+EPICS_MAX_BYTES_STRING_ATTRIB-1;
							strncpy (pHistoryMessage, pSubString, EPICS_MAX_BYTES_STRING_ATTRIB);
							if (cicsDbPut (TOP LOGTASK_ERROR_LOG_NAME1, pMessage, DBF_STRING,
							               (void *) pHistoryMessage)
							    == ERROR)
							{
								ERROR_SET (S_errorLog_EPICS_ERROR, pMessage, ERROR_LOG_SAVE);
								ERROR_LOG ("Failed to write error log record 1");
								printErr ("Failed (%s) to write \"%s\" to error log record 1\n",
									pMessage, pHistoryMessage);
							}
						}
					}
					else
					{
						/*
						 * Assume this is a log message.
						 *
						 * Truncate the message so it will fit, and write it to the
						 * history log record(s).
						 *
						 * NOTE: Gemini might require that all error messages be written to
						 * the history log record(s) as well as the error log record(s). If
						 * this is the case make this "else" block unconditional.
						 */

						strncpy (pHistoryMessage, pMsgBuffer, EPICS_MAX_BYTES_STRING_ATTRIB);
						if (cicsDbPut (TOP LOGTASK_HISTORY_LOG_NAME0, pMessage, DBF_STRING,
						               (void *) pHistoryMessage)
						    == ERROR)
						{
							ERROR_SET (S_errorLog_EPICS_ERROR, pMessage, ERROR_LOG_SAVE);
							ERROR_LOG ("Failed to write history log record");
							printErr ("Failed (%s) to write \"%s\" to history log record\n",
								pMessage, pHistoryMessage);
						}

						/*
						 * If the message is longer than EPICS_MAX_BYTES_STRING_ATTRIB characters
						 * copy the next part of the message into the history log overflow record.
						 */

						if ( strlen(pMsgBuffer) > EPICS_MAX_BYTES_STRING_ATTRIB )
						{

							pSubString = pMsgBuffer+EPICS_MAX_BYTES_STRING_ATTRIB-1;
							strncpy (pHistoryMessage, pSubString, EPICS_MAX_BYTES_STRING_ATTRIB);
							if (cicsDbPut (TOP LOGTASK_HISTORY_LOG_NAME1, pMessage, DBF_STRING,
							               (void *) pHistoryMessage)
							    == ERROR)
							{
								ERROR_SET (S_errorLog_EPICS_ERROR, pMessage, ERROR_LOG_SAVE);
								ERROR_LOG ("Failed to write history log record 1");
								printErr ("Failed (%s) to write \"%s\" to history log record 1\n",
									pMessage, pHistoryMessage);
							}
						}
					}

#endif	/* NO_EPICS */
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
		}
	}

	MESSAGE_LOG (MSG_WARNING, "Message logging task stopped.");

	errorFlush();
	errorFree();

	return (OK);
}
