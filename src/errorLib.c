static struct {void *v; char *c;} rcsid = {&rcsid,
   "$Id: errorLib.c,v 1.2 2000-03-13 20:46:40 cboyer Exp $"};

/*+
 * MODULE NAME:
 * errorLib
 *
 * FILENAME:
 * errorLib.c
 *
 * PURPOSE:
 * Error and message handling library
 *
 * DESCRIPTION:
 * The errorLib library is used to save information in a context structure,
 * defined for each task, when an error is detected. The information is
 * recalled when the error is logged. Error messages are by default sent to
 * stderr, but can be rerouted to a pipe or file opened on a particular file
 * descriptor. Informational log messages can also be sent to the same
 * destination as the error messages.
 *
 * The functions in this library are designed to be called using ERROR_SET,
 * ERROR_LOG and MESSAGE_LOG macros, which are defined in "errorLib.h".
 * The ERROR_SET and ERROR_LOG macros are intended to be used as follows.
 *
 * Firstly, there are two distinct types of software to consider{:}
 * (i) application code (a program that does something useful, usually by
 * calling a number of library routines); and (ii) library code (usually
 * intended for use by an application program).
 *
 * In either of these two fundamental types of code, three major causes of
 * an error arising have been identified, as follows{:}
 *
 * - (a) A VxWorks subroutine returns ERROR, NULL or some other status 
 *       indicating that an error occurred. There is no way of knowning 
 *       whether the routine will have set the variable errno accordingly,
 *       but errorLib works on the assumption that errno will indeed have
 *       been set. Therefore, it does not attempt to overwrite errno in this
 *       case.
 *
 * - (b) ERROR, NULL or some other status indication of an error is returned 
 *       from another library routine for which errorLib is supported. In
 *       this case, the errorLib macros will by definition have been used to
 *       handle the error when it first arose (by setting errno unless the 
 *       error arose from a lower-level VxWorks subroutine, as described in
 *       (a) above).
 *
 * - (c) The user's code detects an error condition for the first time. In 
 *       other words, the error did not arise in a subroutine of any kind.
 *
 * Now, the above defines two types of software and three classes of error.
 * It is then possible to construct a table which shows the recommended 
 * calling sequence to handle an error in each of the six possible cases,
 * as follows{:}
 *
 * - (i) Application Code - recommended use of ERROR_SET() and ERROR_LOG()
 *       macros
 *v
 *v      (a)     ERROR_SET (0, "message", ERROR_LOG_NOW);
 *v      (b)     ERROR_LOG ("message");
 *v      (c)     ERROR_SET (S_moduleName_ERROR_CODE, "message", ERROR_LOG_NOW);
 *
 * - (ii) Library Code - recommended use of ERROR_SET() and ERROR_LOG()
 *        macros
 *v
 *v      (a)     ERROR_SET (0, "message", ERROR_LOG_SAVE);
 *v      (b)     ERROR_SET (0, "optional message", ERROR_LOG_SAVE);
 *v      (c)     ERROR_SET (S_moduleName_ERROR_CODE, "message", ERROR_LOG_SAVE);
 *
 * Where "S_moduleName_ERROR_CODE" is an error-number, normally defined in
 * each module's include file and "message" is any appropriate text string.
 * Note that the "message" string used with library code will, by
 * definition, provide information about the fundamental nature of the
 * error. On the other hand, the "message" used in application code will
 * typically provide information about a higher-level consequence of the
 * error - e.g. it may describe the loss of functionality that arose as a
 * result of the error, rather than the cause of the error. The symbol
 * ERROR_MSG_NONE can be used as a null message if there is no information
 * to add. The macros ERROR_SET1, ERROR_SET2, ERROR_SET3 and ERROR_SET4
 * can be used to include parameters in an error message string.Up to four
 * library "message" strings, plus the application message string, are logged
 * by errorLib, thus providing the user with a comprehensive description of
 * the cause, and the consequences, of an error condition.
 *
 * FUNCTION NAME(S):
 * errorInit - Create and initialise error context structure
 * errorFree - Delete and free resources allocated to error context structure
 * errorGlobalFdSet  - Set global file descriptor used for error output
 * errorGlobalFdGet  - Get global file descriptor used for error output
 * errorTaskFdSet    - Set task-specific file descriptor used for error output
 * errorTaskFdGet    - Get task-specific file descriptor used for error output
 * errorSave         - Save an error context
 * errorShow         - Display saved error context information
 * errorClear        - Clear a previously saved error
 * errorWrite        - Log info. from current error context to current logging 
 *                     destination
 * errorFlush        - Flush any information stored in the current error context
 * errorLogAllEnable - Enable or disable the immediate logging of all messages
 * errorLogConsoleEnable - Enable or disable the copying of messages to the 
 *                         console
 * errorLogMessage       - Log an information message to the current logging 
 *                         destination
 * errorMessageFilterSet - Set global message filtering level
 * errorMessageFilterGet - Get global message filtering level
 *
 * ASSOCIATED MACRO(s):
 * ERROR_SET   - Save error context with compiler information
 * ERROR_LOG   - Log error message, plus saved error context, with
 *               compiler information to current logging destination
 * MESSAGE_LOG - Log informational message to current logging destination
 *
 * ORIGINAL AUTHOR:
 * Nick Dillon
 *
 * MODIFIED BY:
 * Steven Beard
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/03/17 03:14:22  cboyer
 * Initial creation of the Gemini HRWFS repository
 *
 * Revision 1.18  1998/12/07 11:17:17  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.17  1998/10/01 13:51:03  cics
 * ERROR_SET macro simplified to reduce the amount of duplicated code and remove a deficiency
 *
 * Revision 1.16  1998/09/28 08:53:08  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks
 *
 * Revision 1.15  1998/09/09 14:35:29  cics
 * Global variables renamed to ensure they are unique
 *
 * Revision 1.14  1998/08/31 16:41:05  smb
 * Use const keyword where needed
 *
 * Revision 1.13  1998/03/27 12:05:31  smb
 * Additional checks for NULL error context. Protect error logging fd with semaphore
 *
 * Revision 1.12  1998/03/04 17:12:14  smb
 * Comment dates made more international
 *
 * Revision 1.11  1998/02/23 13:38:51  smb
 * Rearranged code for printability
 *
 * Revision 1.10  1998/02/02 17:24:05  smb
 * Added errorFree to free resources allocated by errorInit
 *
 * Revision 1.9  1998/01/30 15:30:12  smb
 * Fixed some problems uncovered by prolint
 *
 * Revision 1.8  1998/01/21 10:47:07  smb
 * Message logging added
 *
 * Revision 1.7  1998/01/16 15:55:30  smb
 * Quell compiler warning about rcsid using anj's idea
 *
 * Revision 1.6  1998/01/14 10:39:15  anj
 * smb added errorFlush function for use from vxWorks.
 *
 * Revision 1.5  1998/01/06 14:16:43  smb
 * Minor changes
 *
 * Revision 1.4  1997/12/11 14:55:09  smb
 * Do not duplicate logged message
 *
 * Revision 1.3  1997/12/10 17:31:55  smb
 * Added buffer to save up to 4 error messages
 *
 * Revision 1.2  1997/12/05 15:07:40  smb
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
#endif   /* vxWorks */

#include <stdlib.h>
#include <stdio.h>
#include <ioLib.h>
#include <usrLib.h>
#include <taskVarLib.h>
#include "errorLib.h"


/* defines */

/* #define DEBUG */             /* Define this macro to enable debug messages */


/* task variables */

int errorContextTask;         /* Error context ID for current task (a task variable)         */


/* global variables. These are distiguished by having an "error" or "message" prefix. */

int errorCount = 0;        /* Current error count (for each CPU)                     */
int errorLogGlobalFd = -1; /* Global file descriptor to which errors are to be logged      */
SEM_ID errorLogSem = NULL; /* Semaphore protecting global error log file descriptor.      */
SEM_ID errorConsoleSem = NULL; /* Semaphore protecting the console.           */
BOOL errorLogAll = FALSE;      /* When this flag is set all errors are logged */
                               /* immediately                                 */
BOOL errorLogConsole = FALSE;  /* When this flag is set all errors are copied */
                               /* to the console                              */
int messageFilterLevel = -1;   /* Global message filtering level.             */


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   errorInit
 *
 *   INVOCATION:
 *   errorInit ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, or ERROR if unsuccessful.
 *
 *   PURPOSE:
 *   Create and initialise an error context structure
 *
 *   DESCRIPTION:
 *   This routine is used to initalise a task for error logging via the errorLib
 *   routines and macros. It may be called explicitely by a task (e.g.
 *   immediately the task starts up), but will otherwise be called as and when
 *   necessary by the errorLib routines. The routine creates an error context
 *   structure for the current task and saves a pointer to that structure (cast
 *   into an "int") as a task variable, "errorContextTask". Using a task
 *   variable (which is saved and restored with the task context) ensures that
 *   each task uses its own unique error context structure, so many different
 *   tasks can share errorLib in a re-entrant fashion.
 *
 *   Note that this function is designed to be called once only by each task.
 *   Calling the function many times is not fatal, but it will waste memory.
 *
 *   EXTERNAL VARIABLES:
 *   (<)   errorContextTask   (int)         Error context ID (task variable)
 *   (<) (pErrorContext)      (ERROR_CONTEXT)   Error context structure (created)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS   errorInit (void)
{
   ERROR_CONTEXT   pErrorContext;   /* Pointer to error context structure */
   int            msg;         /* Message number */

   /*
    * If this is the first task to call errorInit(), create the errorLogSem and
    * errorConsoleSem semaphores.
    * Other tasks are locked out to prevent two or more tasks attempting to create the
    * semaphores simultaneously.
    */

   taskLock();
   if (errorLogSem == NULL)
   {
      if ((errorLogSem = semMCreate( SEM_Q_FIFO )) == NULL)
      {
         printErr ("errorLogSem semaphore creation failed");
      }
      if ((errorConsoleSem = semMCreate( SEM_Q_FIFO )) == NULL)
      {
         printErr ("errorConsoleSem semaphore creation failed");
      }
   }
   taskUnlock();

   /*
    * Allocate sufficient memory for an error context structure. Note that
    * this memory is allocated permanently and is never freed. It is assumed
    * that this function will only be called once by each task.
    */

   if ((pErrorContext = (ERROR_CONTEXT) malloc (sizeof (ERROR_CONTEXT_STRUCT))) == NULL)
   {
      printErr ("errorInit: failed to allocate memory for context structure\n");
      return (ERROR);
   }

   /*
    * Initialise the "errorSet" flag and the other contents of the error
    * context structure.
    */

   pErrorContext->errorSet = FALSE;
   pErrorContext->fd = -1;
   strncpy (pErrorContext->pTaskName, taskName (taskIdSelf ()), ERROR_LOG_ONE_MSG_SIZE);
   strncpy (pErrorContext->pCode, "<error code not set>", ERROR_LOG_ONE_MSG_SIZE);
   pErrorContext->lineNumber = 0;
   strncpy (pErrorContext->pFileName, "<file name not set>", ERROR_LOG_ONE_MSG_SIZE);

   for ( msg = 0; msg < ERROR_LOG_MAX_MESSAGES; msg++ )
      strncpy( pErrorContext->ppMessage[msg], ERROR_MSG_NONE, ERROR_LOG_ONE_MSG_SIZE);

   pErrorContext->nmsg = 0;
   pErrorContext->bufferBytes = 0;

   /*
    * Create a new task variable, "errorContextTask", and write a pointer
    * to the error context structure (cast to an "int") into it.
    */

   if ((taskVarAdd (taskIdSelf (), & errorContextTask) == ERROR) ||
      (taskVarSet (taskIdSelf (), & errorContextTask, (int) pErrorContext)))
   {
      printErr ("errorInit: failed to initialise task variable\n");
      return (ERROR);
   }

   return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorFree
 *
 *   INVOCATION:
 *   errorFree ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, or ERROR if unsuccessful.
 *
 *   PURPOSE:
 *   Free resources allocated to error context structure
 *
 *   DESCRIPTION:
 *   This routine is used to free the resources allocated to an error context
 *   structure.
 *
 *   EXTERNAL VARIABLES:
 *   (!)   errorContextTask   (int)         Error context ID (task variable)
 *   (<) (pErrorContext)      (ERROR_CONTEXT)   Error context structure (deleted)
 *
 *   PRIOR REQUIREMENTS:
 *   An error context structure should normally have been created and
 *   initialised by calling errorInit() prior to calling this function.
 *   However, this is not compulsory, and the function will do nothing
 *   and return OK if an error context structure has not been allocated.
 *   The function only returns ERROR if there is a genuine problem
 *   while attempting to free an existing error context structure.
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS   errorFree (void)
{
   ERROR_CONTEXT   pErrorContext;            /* Pointer to error context structure.   */
   STATUS         returnValue;            /* Return value.                  */


   returnValue = OK;

   /*
    * Get a pointer to the current task's error context structure.
    * Return without doing anything if the error context structure could not be accessed.
    */

   pErrorContext = (ERROR_CONTEXT) taskVarGet (taskIdSelf (), & errorContextTask);

   if ( (int) pErrorContext == ERROR)
      return (OK);

   /*
    * Delete the task variable associated with the error context structure.
    */

   if (taskVarDelete (taskIdSelf (), & errorContextTask) == ERROR)
   {
      printErr ("errorFree: Error context task variable could not be deleted.\n" );
      returnValue = ERROR;
   }

   /*
    * Free the resources allocated to the error context structure, provided its pointer is non-NULL.
    */

   if (pErrorContext != NULL)
   {
      free (pErrorContext);
   }
   else
   {
      printErr ("errorFree: Error context structure pointer is NULL.\n");
   }

   return (returnValue);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorGlobalFdSet
 *
 *   INVOCATION:
 *   errorGlobalFdSet (const int fd)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   fd   (const int)      File descriptor to which errors will be logged
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Sets the global file descriptor used for logging errors
 *
 *   DESCRIPTION:
 *   This routine sets the global file descriptor used by errorLib to log
 *   error messages. Errors can be logged, for example, to stderr, stdout,
 *   or a file or pipe. If this function is not used, error messages will
 *   go to stderr by default.
 *
 *   The global file descriptor applies to all tasks, but it can be
 *   overridden a specific task by calling errorTaskFdSet().
 *
 *   EXTERNAL VARIABLES:
 *   (<)   errorLogGlobalFd   (int)   Global file descriptor
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

void   errorGlobalFdSet
   (
   const int   fd
   )
{
   errorLogGlobalFd = fd;

#ifdef DEBUG
   printf( "Global file descriptor for error logging set to %d\n", errorLogGlobalFd );
#endif /* DEBUG */

}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorGlobalFdGet
 *
 *   INVOCATION:
 *   errorGlobalFdGet ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (int)   File descriptor to which error messages will be logged.
 *
 *   PURPOSE:
 *   Gets the global file descriptor used for logging errors
 *
 *   DESCRIPTION:
 *   This routine gets the global file descriptor used by errorLib to log
 *   error messages. If a file descriptor has been defined with
 *   errorGlobalFdSet, that same file descriptor will be returned. If no file
 *   descriptor has been set explicitly the default fd, stderr, will be
 *   returned.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   errorLogGlobalFd   (int)   Global file descriptor
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

int   errorGlobalFdGet (void)
{

   /*
    * Return the file descriptor for "stderr" if the global file descriptor is not defined
    * (as indicated by a -1), otherwise return the global file descriptor.
    */

   if (errorLogGlobalFd == -1)
      return (ioGlobalStdGet (2));
   else
      return (errorLogGlobalFd);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorTaskFdSet
 *
 *   INVOCATION:
 *   errorTaskFdSet (const int fd)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   fd   (const int)      File descriptor to which this task's errors will be logged
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Sets the task-specific file descriptor used for logging errors
 *
 *   DESCRIPTION:
 *   This routine sets the task-specific file descriptor contained in a
 *   task's error context structure. This file descriptor can be used to
 *   override the global file descriptor (defined by errorGlobalFdGet())
 *   for a particular task only. It can be used to avoid a situation where
 *   an error logging task attempts to log error messages to itself.
 *
 *   If this function is not called, or if the task-specific file descriptor
 *   is set to -1, the global file descriptor will be used to decide where
 *   error messages are logged.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   errorContextTask   (int)         Error context ID (task variable)
 *   (!) (pErrorContext)      (ERROR_CONTEXT)   Error context structure
 *
 *   PRIOR REQUIREMENTS:
 *   An error context structure should have been created and initialised by
 *   calling errorInit() prior to calling this function, although this is not
 *   compulsory. The routine will call errorInit() if it has not previously
 *   been called
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

void   errorTaskFdSet
   (
   const int   fd
   )
{
   ERROR_CONTEXT   pErrorContext;            /* Pointer to error context structure */


   /*
    * Get a pointer to the current task's error context structure.
    * Initialise error-logging for this task if not already initialised
    * (via a previous call to errorInit()).
    */

   pErrorContext = (ERROR_CONTEXT) taskVarGet (taskIdSelf (), & errorContextTask);

   if ((int) pErrorContext == ERROR)
   {
      errorInit();

      pErrorContext = (ERROR_CONTEXT) taskVarGet (taskIdSelf (), & errorContextTask);

      if ((int) pErrorContext == ERROR)
      {
         printErr ("%s: errorTaskFdSet: Failed to get pointer to error context structure.",
                taskName( taskIdSelf() ) );
         return;
      }
   }

   if (pErrorContext == NULL)
   {
      printErr ("%s: errorTaskFdSet: NULL error context structure.",
                taskName( taskIdSelf() ) );
      return;
   }

   /* Set the task-specific file descriptor within that error context structure. */

   pErrorContext->fd = fd;

#ifdef DEBUG
   printf( "Task-specific error logging file descriptor for %s task set to %d\n",
           taskName( taskIdSelf() ), fd );
#endif /* DEBUG */

}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorTaskFdGet
 *
 *   INVOCATION:
 *   errorTaskFdGet ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (>)   fd   (int)   File descriptor to which this task's errors will be logged
 *
 *   PURPOSE:
 *   Gets the task-specific file descriptor used for logging errors
 *
 *   DESCRIPTION:
 *   This routine sets the task-specific file descriptor contained in a
 *   task's error context structure. If a task-specific file descriptor
 *   has not been defined, the function will return -1.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   errorContextTask   (int)         Error context ID (task variable)
 *   (>) (pErrorContext)      (ERROR_CONTEXT)   Error context structure
 *
 *   PRIOR REQUIREMENTS:
 *   An error context structure should have been created and initialised by
 *   calling errorInit() prior to calling this function. If the error context
 *   structure is NULL or does not exist, the function will return -1.
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

int   errorTaskFdGet (void)
{
   ERROR_CONTEXT   pErrorContext;            /* Pointer to error context structure */

   /*
    * Get a pointer to the current task's error context structure.
    * Return -1 if the error context structure is NULL or could not be accessed.
    */

   pErrorContext = (ERROR_CONTEXT) taskVarGet (taskIdSelf (), & errorContextTask);

   if (((int) pErrorContext == ERROR) || (pErrorContext == NULL))
      return (-1);

   /* Return the task-specific file descriptor defined in the error context structure. */

   return (pErrorContext->fd);
}



/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorSet
 *
 *   INVOCATION:
 *   errorSet (const int errorNumber, const char * message, const BOOL logNow)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   lineNumber   (const int)      Source code line number (when error saved)
 *   (>)   pFileName   (const char *)   Source code file name (when error saved)
 *   (>)   errorNumber   (const int)      Error number (queried from VxWorks if 0)
 *   (>)   message      (const char *)   Error message
 *   (>)   logNow      (const BOOL)   TRUE if the message is to be logged immediately
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Saves the current error context and/or logs an error
 *
 *   DESCRIPTION:
 *   This function translates the specified errorNumber into a descriptive message, saves
 *   the specified line number, file name and message into the current error context and,
 *   if the logNow flag is set, logs an error message. If a zero error number is provided,
 *   then the most recent error number is queried from VxWorks by calling "errnoGet()",
 *   otherwise the specified error number is used.
 *
 *   This function is designed to be used with the ERROR_SET macro contained
 *   in "errorLib.h" (see below), but is available via errorLib's public
 *   interface primarily to allow debugging from the VxWorks shell.
 *
 *   ASSOCIATED MACROS:
 *   ERROR_SET  (errorNumber, message, logNow)
 *   ERROR_SET1 (errorNumber, format, logNow, arg1)
 *   ERROR_SET2 (errorNumber, format, logNow, arg1, arg2)
 *   ERROR_SET3 (errorNumber, format, logNow, arg1, arg2, arg3)
 *   ERROR_SET4 (errorNumber, format, logNow, arg1, arg2, arg3, arg4)
 *
 *   MACRO DESCRIPTION:
 *   The ERROR_SET macro is a wrap-around for the errorSet() function
 *   in which compiler directives are used to obtain the current line
 *   number and file name.
 *
 *   The ERROR_SET1, ERROR_SET2, ERROR_SET3 and ERROR_SET4 macros are
 *   available as wrap-arounds for ERROR_SET where the error message
 *   needs to contain 1, 2, 3 or 4 variables. The error message string
 *   is formatted by sprintf() using (format), and values are obtained
 *   from the arguments (arg1), (arg2), (arg3) and (arg4).
 *
 *   MACRO NOTE:
 *   Having several different ERROR_SET macros for different numbers of
 *   arguments seems excessive, but I don't know how to write a macro
 *   with a variable number of arguments. At the moment error messages
 *   with five or more arguments cannot be dealt with. SMB - 20 Jan 98.
 *
 *   SEE ALSO:
 *   errorSave
 *   errorWrite
 *
 *   EXTERNAL VARIABLES:
 *   (>)   errorContextTask   (int)         Error context ID (task variable)
 *   (!) (pErrorContext)      (ERROR_CONTEXT)   Error context structure
 *   (!) errorCount         (int)         Current error count
 *   (>) errorLogConsole      (int)         Global "copy to console" flag
 *   (>) errorLogGlobalFd   (int)         Global file descriptor
 *
 *   PRIOR REQUIREMENTS:
 *   An error context structure should have been created and initialised by
 *   calling errorInit() prior to calling this function, although this is not
 *   compulsory. The routine will call errorInit() if it has not previously
 *   been called
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

void   errorSet
   (
   const int      lineNumber,
   const char *   pFileName,
   const int      errorNumber,
   const char *   message,
   const BOOL      logNow
   )
{
   char       errorCode [ERROR_LOG_ONE_MSG_SIZE];

   if (errorNumber != 0)
   {
      errnoSet (errorNumber);
      strerror_r (errorNumber, errorCode);
   }
   else
   {
      strerror_r (errnoGet (), errorCode);
   }

   if (errorLogAll || logNow)
   {
      errorSave (lineNumber, pFileName, errorCode, ERROR_MSG_NONE);
      errorWrite (lineNumber, pFileName, message);
   }
   else
   {
      errorSave (lineNumber, pFileName, errorCode, message);
   }

   return;
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorSave
 *
 *   INVOCATION:
 *   errorSave (const int lineNumber, const char * pFileName, const char * pCode, char * pMessage)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   lineNumber   (const int)      Source code line number (when error saved)
 *   (>)   pFileName   (const char *)   Source code file name (when error saved)
 *   (>)   pCode      (const char *)   String containing error number code
 *   (>)   pMessage   (const char *)   String containing descriptive error message
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Saves the current error context
 *
 *   DESCRIPTION:
 *   The source code line number, file name, and a description of the current
 *   error number are saved into the current task's error context structure.
 *   A descriptive error message is also saved, unless the pMessage string
 *   is set to ERROR_MSG_NONE.
 *
 *   The routine will call the task-initialisation routine, errorInit(), if it
 *   has not already been called by this task.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   errorContextTask   (int)         Error context ID (task variable)
 *   (!) (pErrorContext)      (ERROR_CONTEXT)   Error context structure
 *   (!) errorCount         (int)         Current error count
 *
 *   PRIOR REQUIREMENTS:
 *   An error context structure should have been created and initialised by
 *   calling errorInit() prior to calling this function, although this is not
 *   compulsory. The routine will call errorInit() if it has not previously
 *   been called
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

void   errorSave
   (
   const int      lineNumber,
   const char *   pFileName,
   const char *   pCode,
   const char *   pMessage
   )
{
   ERROR_CONTEXT   pErrorContext;         /* Pointer to error context structure */

   /*
    * Display a debug message if compiled for debugging. (These messages are
    * designed for debugging the error library itself).
    */

#ifdef DEBUG
   printf( "%s: errorSave: %d %s %s\n", taskName( taskIdSelf() ), lineNumber, pFileName, pCode );
#endif /* DEBUG */

   /*
    * Get a pointer to the current task's error context structure.
    * Initialise error-logging for this task if not already initialised
    * (via a previous call to errorInit()).
    */

   pErrorContext = (ERROR_CONTEXT) taskVarGet (taskIdSelf (), & errorContextTask);

   if ((int) pErrorContext == ERROR)
   {
      errorInit();

      pErrorContext = (ERROR_CONTEXT) taskVarGet (taskIdSelf (), & errorContextTask);

      if ((int) pErrorContext == ERROR)
      {
         printErr ("%s: errorSave: Failed to get pointer to error context structure.",
                taskName( taskIdSelf() ) );
         return;
      }
   }

   if (pErrorContext == NULL)
   {
      printErr ("%s: errorSave: NULL error context structure.",
             taskName( taskIdSelf() ) );
      return;
   }

   /*
    * If an un-logged message hasn't previously been saved then begin a new
    * error context, setting the "errorSet" flag to prevent further error context
    * information overwriting the structure until its contents have been logged.
    * If the "errorSet" flag is already set, then preserve the existing error
    * context information but add a new error message to the stack (unless the error
    * message is ERROR_MSG_NONE).
    */

   if ( pErrorContext->errorSet == FALSE)
   {
      errorCount++;                  /* Increment the error count   */
      pErrorContext->errorSet = TRUE;
      pErrorContext->lineNumber = lineNumber;
      strncpy (pErrorContext->pFileName, pFileName, ERROR_LOG_ONE_MSG_SIZE);
      strncpy (pErrorContext->pCode, pCode, ERROR_LOG_ONE_MSG_SIZE);
      if ( strncmp (pMessage, ERROR_MSG_NONE, ERROR_LOG_ONE_MSG_SIZE) != 0 )
      {
         strncpy (pErrorContext->ppMessage[0], pMessage, ERROR_LOG_ONE_MSG_SIZE);
         pErrorContext->nmsg = 1;
      }
   }
   else
   {
      if ( (strncmp (pMessage, ERROR_MSG_NONE, ERROR_LOG_ONE_MSG_SIZE) != 0) &&
           (pErrorContext->nmsg < ERROR_LOG_MAX_MESSAGES) )
      {
         strncpy (pErrorContext->ppMessage[pErrorContext->nmsg], pMessage, ERROR_LOG_ONE_MSG_SIZE);
         pErrorContext->nmsg++;
         
      }
   }
   return;
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorWrite
 *
 *   INVOCATION:
 *   errorWrite (const int lineNumber, const char * pFileName, char * pMessage)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   lineNumber   (const int)      Source code line number (when error logged)
 *   (>)   pFileName   (const char *)   Source code file name (when error logged)
 *   (>)   pMessage   (const char *)   String containing error message
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Logs a previously saved error context, together with a message
 *
 *   DESCRIPTION:
 *   This function logs previously-saved error context information, together
 *   with the source code line number and file name of the application code
 *   reporting the error, plus an error message. The error message destination
 *   may be controlled by function errorGlobalFdSet(). Error messages take the
 *   following form
 *v
 *v   PREFIX CODE at line SAVED-LINE in SAVED-FILE, task: TASK-NAME,
 *v   "ERROR-MESSAGE", ...
 *v   "ERROR-MESSAGE" logged at line LOG-LINE in LOG-FILE
 *
 *   Note that the error message contains two sets of source code and line
 *   number - one set from the library code which saved the error and a second
 *   set from the application code reporting the error.
 *   
 *   This function is designed to be used with the ERROR_LOG macro contained
 *   in "errorLib.h" (see below).
 *
 *   ASSOCIATED MACRO:
 *   ERROR_LOG (message)
 *
 *   MACRO DESCRIPTION:
 *   The ERROR_LOG macro is used to log the error currently saved in
 *   a task's error context structure. Compiler directives are used
 *   to determine the current line numer and source file, and a message
 *   supplied in "message" is logged in addition to the information
 *   already saved in the error context structure.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   errorContextTask   (int)         Error context ID (task variable)
 *   (!) (pErrorContext)      (ERROR_CONTEXT)   Error context structure
 *   (>) errorLogConsole      (int)         Global "copy to console" flag
 *   (>) errorLogGlobalFd   (int)         Global file descriptor
 *
 *   PRIOR REQUIREMENTS:
 *   An error context structure should have been created and initialised by
 *   calling errorInit() prior to calling this function. An error context
 *   should also have been saved by calling errorSave().
 *
 *   It is very important that the error message buffer defined in the error
 *   context structure (size ERROR_LOG_BUFFER_SIZE defined in "errorLog.h")
 *   be large enough to contain the longest possible error message.
 *   Writing beyond the bounds of this buffer could cause memory corruption.
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   Reporting an error message containing numerical values is not straightforward,
 *   since this function is designed to report a string. A "sprintf()" would need
 *   to be used to construct an error message string containing values prior to
 *   calling this function.
 *
 *   Error messages can be lost if there turns out to be a problem writing to
 *   the global file descriptor. This function can be made to copy all error
 *   messages to the console in addition to the global file descriptor by
 *   executing errorLogConsoleEnable(1).
 *-
 */

void   errorWrite
   (
   const int      lineNumber,
   const char *   pFileName,
   const char *   pMessage
   )
{
   ERROR_CONTEXT   pErrorContext;   /* Pointer to error context structure            */
   size_t         nByte = 0;      /* Number of bytes written to error message buffer   */
   int            fd;            /* File descriptor                           */
   char *         pBuf;         /* Pointer to error message buffer               */
   char         pSavedMessages[ ERROR_LOG_MAX_MESSAGES * ERROR_LOG_ONE_MSG_SIZE ];
                           /* Buffer to contain saved messages.            */
   char *         pNextMsg;      /* Pointer to next message.                     */
   int            msg;         /* Message counter.                           */

   /*
    * Display a debug message if compiled for debugging. (These messages are
    * designed for debugging the error library itself).
    */

#ifdef DEBUG
   printf( "%s: errorWrite: %d %s %s\n", taskName( taskIdSelf() ), lineNumber, pFileName, pMessage );
#endif /* DEBUG */

   /*
    * Get a pointer to the current task's error context structure, and
    * obtain a pointer to the error message buffer contained within it.
    */

   pErrorContext = (ERROR_CONTEXT) taskVarGet (taskIdSelf (), & errorContextTask);

   /*
    * Abort if the error context structure is not valid
    */

   if (((int) pErrorContext == ERROR) || (pErrorContext == NULL))
   {
      printErr( "errorWrite: Invalid error context structure.\n" );
      return;
   }

   /*
    * Only proceed if an error message has been saved.
    */

   if (pErrorContext->errorSet)
   {
      /*
       * Obtain a pointer to the error message buffer contained within the error context structure.
       */

      pBuf = pErrorContext->pBuffer;

      /* Initialise the saved message buffer to a null string. */

      strncpy (pSavedMessages, "", 1 );

      /* Concatenate all the saved errors into one saved message buffer. */

      pNextMsg = pSavedMessages;
      for ( msg = 0; msg < pErrorContext->nmsg; msg++ )
      {
         if ( strncmp (pErrorContext->ppMessage[msg], ERROR_MSG_NONE, ERROR_LOG_ONE_MSG_SIZE)
              != 0 )
         {
            nByte = (size_t) sprintf( pNextMsg, "\"%.80s\",\n", pErrorContext->ppMessage[msg] );
            pNextMsg += nByte;
         }
      }

      /*
       * Load the error message into the buffer, then increment nByte
       * to take account of the null termination byte. Note that the
       * format statments within the sprintf statement limit the size
       * of the buffer written to 8 + 80 + 10 + 10 + 4 + 80 + 8 + 80 +
       * 2 + 336 + 1 + 80 + 18 + 10 + 4 + 80 + 1 = 812 bytes.
       * ERROR_LOG_BUFFER_SIZE must be at least this size.
       */

      if ( ERROR_LOG_BUFFER_SIZE > 812 )
      {

         nByte = (size_t) sprintf (pBuf, ERROR_LOG_MSG_PREFIX "%.80s at line #%d in %.80s,"
             " task: %.80s,\n%.336s\"%.80s\" logged at line #%d in %.80s\n",
            pErrorContext->pCode, pErrorContext->lineNumber,
            pErrorContext->pFileName, pErrorContext->pTaskName, pSavedMessages,
            pMessage, lineNumber, pFileName);
         nByte++;                        /* include null-termination in byte-count */
      }
      else
      {
         printErr( "errorWrite: Error message buffer too small.\n" );
      }

      /*
       * Only proceed if there are some bytes in the message buffer to be displayed.
       */

      if ( nByte > 0 )
      {

         pErrorContext->bufferBytes = (int) nByte;

         /*
          * If errorLogConsole is enabled, display the message on the console
           * before writing it to the error log output. Use a semaphore to prevent
          * overlapping output on the console.
          */

         if ( errorLogConsole )
         {
            if (semTake(errorConsoleSem, WAIT_FOREVER) != ERROR)
            {
               printf (pBuf);
               semGive (errorConsoleSem);
            }
            else
            {
               printErr(
                  "errorWrite: Could not take errorConsoleSem semaphore to display \"%s\"",
                  pBuf);
            }
         }

         /*
          * If a task-specific file descriptor has been defined, then use that;
          * otherwise, if the global file descriptor is not -1 error messages are
          * logged to that global file descriptor. If the global file descriptor is
          * -1 then errors are logged to "standard error".
          */

         if ( (fd = errorTaskFdGet()) == -1 )
         {
            if (errorLogGlobalFd != -1)
               fd = errorLogGlobalFd;
            else
               fd = ioGlobalStdGet (2);   /* 2 means "stderr". */
         }

         /* Wait for exclusive access to the error message file descriptor, */

         if (semTake(errorLogSem, WAIT_FOREVER) != ERROR)
         {

            /*
             * Write the error message buffer, release access to the file descriptor,
             * unset the "errorSet" flag to show the message has been logged and reset
             * the number of stored messages counter.
             */

            write (fd, pBuf, nByte);
            semGive (errorLogSem);
            pErrorContext->errorSet = FALSE;
            pErrorContext->nmsg = 0;
         }
         else
         {
            printErr( "errorWrite: Could not take errorLogSem semaphore to write \"%s\"", pBuf);
         }
      }
      else
      {
         printErr( "errorWrite: Error message buffer is empty.\n" );
      }
   }
   return;
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorFlush
 *
 *   INVOCATION:
 *   errorFlush (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Displays the saved error context information on the console
 *
 *   DESCRIPTION:
 *   This function displays previously-saved error context information on
 *   the console. Error messages take the
 *   following form
 *v
 *v   PREFIX CODE at line SAVED-LINE in SAVED-FILE, task: TASK-NAME,
 *v   "ERROR-MESSAGE", ...
 *
 *   The function is similar to errorWrite(), except that the messages go
 *   to the console rather than to the global log file descriptor and
 *   additional log information does not need to be supplied as an argument.
 *   The function is designed to be executed at the console to flush out
 *   stored error messages, and may be used when testing out library
 *   functions at the console. (Hint: Try an errorFlush if a library function
 *   returns an ERROR value).
 *
 *   EXTERNAL VARIABLES:
 *   (>)   errorContextTask   (int)         Error context ID (task variable)
 *   (!) (pErrorContext)      (ERROR_CONTEXT)   Error context structure
 *
 *   PRIOR REQUIREMENTS:
 *   An error context structure should have been created and initialised by
 *   calling errorInit() prior to calling this function. An error context
 *   should also have been saved by calling errorSave().
 *
 *   It is very important that the error message buffer defined in the error
 *   context structure (size ERROR_LOG_BUFFER_SIZE defined in "errorLog.h")
 *   be large enough to contain the longest possible error message.
 *   Writing beyond the bounds of this buffer could cause memory corruption.
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None.
 *-
 */

void   errorFlush (void)
{
   ERROR_CONTEXT   pErrorContext;   /* Pointer to error context structure            */
   size_t         nByte = 0;      /* Number of bytes written to error message buffer   */
   char *         pBuf;         /* Pointer to error message buffer               */
   char         pSavedMessages[ ERROR_LOG_MAX_MESSAGES * ERROR_LOG_ONE_MSG_SIZE ];
                           /* Buffer to contain saved messages.            */
   char *         pNextMsg;      /* Pointer to next message.                     */
   int            msg;         /* Message counter.                           */

   /*
    * Get a pointer to the current task's error context structure.
    */

   pErrorContext = (ERROR_CONTEXT) taskVarGet (taskIdSelf (), & errorContextTask);

   /*
    * Abort if the error context structure is not valid
    */

   if (((int) pErrorContext == ERROR) || (pErrorContext == NULL))
   {
      printErr( "errorFlush: Invalid error context structure.\n" );
      return;
   }

   /*
    * Only proceed if an error message has been saved.
    */

   if (pErrorContext->errorSet)
   {
      /*
       * Obtain a pointer to the error message buffer contained within the error context structure.
       */

      pBuf = pErrorContext->pBuffer;

      /* Concatenate all the saved errors into one saved message buffer. */

      pNextMsg = pSavedMessages;
      for ( msg = 0; msg < pErrorContext->nmsg; msg++ )
      {
         if ( strncmp (pErrorContext->ppMessage[msg], ERROR_MSG_NONE, ERROR_LOG_ONE_MSG_SIZE)
              != 0 )
         {
            nByte = (size_t) sprintf( pNextMsg, "\"%.80s\",\n", pErrorContext->ppMessage[msg] );
            pNextMsg += nByte;
         }
      }

      /*
       * Load the error message into the buffer, then increment nByte
       * to take account of the null termination byte. Note that the
       * format statments within the sprintf statement limit the size
       * of the buffer written to 8 + 80 + 10 + 10 + 4 + 80 + 8 + 80 +
       * 2 + 336 = 618 bytes.
       * ERROR_LOG_BUFFER_SIZE must be at least this size.
       */

      if ( ERROR_LOG_BUFFER_SIZE > 618 )
      {
         nByte = (size_t) sprintf (pBuf, ERROR_LOG_MSG_PREFIX "%.80s at line #%d in %.80s,"
             " task: %.80s,\n%.336s",
            pErrorContext->pCode, pErrorContext->lineNumber,
            pErrorContext->pFileName, pErrorContext->pTaskName, pSavedMessages);
         nByte++;                        /* include null-termination in byte-count */
      }
      else
      {
         printErr( "errorFlush: Error message buffer too small.\n" );
      }

      pErrorContext->bufferBytes = (int) nByte;

      /*
       * Only proceed if there are some bytes in the message buffer to be displayed.
       */

      if ( nByte > 0 )
      {

         /*
          * Display the message on the console. (Unlike "errorWrite()", the error
          * message is always displayed on the console, since the purpose of
          * "errorFlush()" is to force stored messages to be displayed on the
          * console).
          */

            if (semTake(errorConsoleSem, WAIT_FOREVER) != ERROR)
            {
               printf (pBuf);
               semGive (errorConsoleSem);
            }
            else
            {
               printErr(
                  "errorFlush: Could not take errorConsoleSem semaphore to display \"%s\"",
                  pBuf);
            }

         /*
          * Unset the "errorSet" flag to show the message has been displayed,
          * and reset the number of stored messages counter.
          */

         pErrorContext->errorSet = FALSE;
         pErrorContext->nmsg = 0;
      }
      else
      {
         printErr( "errorFlush: Error message buffer is empty.\n" );
      }
   }
   return;
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorClear
 *
 *   INVOCATION:
 *   errorClear (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Clear any previously-saved error message
 *
 *   DESCRIPTION:
 *   This routine removes any error message that was previously saved in
 *   a task's error context structure via the routine errorSave() or the
 *   macro ERROR_SET(). The routine simply clears the "errorSet" bit within
 *   the context structure. In the event that error-logging has not been
 *   initialised for this routine, or that no error has been saved, no action
 *   is taken.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   errorContextTask   (int)         Error context ID (task variable)
 *   (!) (pErrorContext)      (ERROR_CONTEXT)   Error context structure
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

void   errorClear (void)
{
   ERROR_CONTEXT   pErrorContext;

   /*
    * Get a pointer to the task's error context structure. If this has worked,
    * and the error context structure itself is not NULL, clear the error set flag
    * and zero the saved message count.
    */

   pErrorContext = (ERROR_CONTEXT) taskVarGet (taskIdSelf (), & errorContextTask);

   if (((int) pErrorContext != ERROR) && (pErrorContext != NULL))
   {
      pErrorContext->errorSet = FALSE;
      pErrorContext->nmsg = 0;
   }
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorShow
 *
 *   INVOCATION:
 *   errorShow (const int taskId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   taskId   (const int)      ID of task whose error context structure is to be
 *                        examined. 0 means the current task.
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the error context structure was not accessible.
 *
 *   PURPOSE:
 *   Prints the contents of a previously saved error context structure
 *
 *   DESCRIPTION:
 *   This function prints previously-saved error context information to
 *   standard output without changing that information.
 *   
 *   This function is designed to be used from the VxWorks console as a
 *   diagnostic tool. errorWrite() should be used by application programs
 *   (via the ERROR_LOG macro contained in "errorLib.h").
 *
 *   EXTERNAL VARIABLES:
 *   (>) errorCount         (int)         Current error count
 *   (>)   errorLogAll         (BOOL)         Global "log immediately" flag
 *   (>) errorLogConsole      (int)         Global "copy to console" flag
 *   (>) errorLogGlobalFd   (int)         Global file descriptor
 *   (>)   errorContextTask   (int)         Error context ID (task variable)
 *   (>) (pErrorContext)      (ERROR_CONTEXT)   Error context structure
 *
 *   PRIOR REQUIREMENTS:
 *   An error context structure should have been created and initialised by
 *   calling errorInit() prior to calling this function. An error context
 *   should also have been saved by calling errorSave().
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS   errorShow
   (
   const int      taskId         /* Task ID (0 for current task).         */
   )
{
   ERROR_CONTEXT   pErrorContext;   /* Pointer to error context structure.      */
   int            msg;         /* Message number.                     */

   /*
    * Check that a valid task has been specified and get a pointer to the
    * task's error context structure. If a zero task ID has been given,
    * assume the current task.
    */

   if ( taskId == 0 )
   {
      pErrorContext = (ERROR_CONTEXT) taskVarGet (taskIdSelf (), & errorContextTask);
   }
   else
   {
      if ( taskIdVerify( taskId ) == ERROR )
      {
         printErr( "errorShow: Invalid task ID specified.\n");
         return (ERROR);
      }
      pErrorContext = (ERROR_CONTEXT) taskVarGet (taskId, & errorContextTask);
   }

   if ((int) pErrorContext == ERROR)
   {
      printErr ("errorShow: Failed to get pointer to error context structure.\n");
      return (ERROR);
   }
   else if (pErrorContext == NULL)
   {
      printErr ("errorShow: Task has NULL error context structure.\n");
      return (ERROR);
   }

   /*
    * Display the contents of the error context structure (displaying
    * the contents of the error message buffer only if it contains a
    * recognisable error message prefix).
    */

   printf ( "Error context structure\n" );
   printf ( "-----------------------\n" );

   printf ( "Task name:    %s\n", pErrorContext->pTaskName );

   if ( pErrorContext->fd == -1 )
      printf ( "Task fd:      <None defined - using global file descriptor>\n" );
   else if ( pErrorContext->fd == ioGlobalStdGet(2) )
      printf ( "Task fd:      STDERR\n" );
   else
      printf ( "Task fd:      %d\n", pErrorContext->fd );

   printf ( "Error code:   %s\n", pErrorContext->pCode );
   printf ( "Line number:  %d\n", pErrorContext->lineNumber );
   printf ( "Source file:  %s\n", pErrorContext->pFileName );

   printf ( "No. messages: %d\n", pErrorContext->nmsg );

   for ( msg = 0; msg < pErrorContext->nmsg; msg++ )
      if ( strncmp (pErrorContext->ppMessage[msg], ERROR_MSG_NONE, ERROR_LOG_ONE_MSG_SIZE) != 0 )
         printf ( "Message %d:    %s\n", msg, pErrorContext->ppMessage[msg] );

   for ( ; msg < ERROR_LOG_MAX_MESSAGES; msg++ )
      if ( strncmp (pErrorContext->ppMessage[msg], ERROR_MSG_NONE, ERROR_LOG_ONE_MSG_SIZE) != 0 )
         printf ( "<Old msg %d:   %s>\n", msg, pErrorContext->ppMessage[msg] );


   if ( (pErrorContext->bufferBytes > 0) &&
        (strncmp (pErrorContext->pBuffer, ERROR_LOG_MSG_PREFIX, sizeof (ERROR_LOG_MSG_PREFIX) - 1)
         == 0)
      )
      printf ( "Buffer:       %s\n", pErrorContext->pBuffer );

   printf( "Error saved flag is " );
   if ( pErrorContext->errorSet )
      printf ( "SET.\n" );
   else
      printf ( "UNSET.\n" );

   /* Report the value of the various global flags and the global
    * error file descriptor.
    */

   printf ( "Error count:  %d\n", errorCount );

   printf( "Copying error messages to the console is " );
   if ( errorLogConsole )
      printf ( "ENABLED.\n" );
   else
      printf ( "DISABLED.\n" );

   printf( "Immediate logging of all saved error messages is " );
   if ( errorLogAll )
      printf ( "ENABLED.\n" );
   else
      printf ( "DISABLED.\n" );

   printf( "Global file descriptor for error logging is " );
   if ( errorLogGlobalFd == -1 )
      printf( "STDERR\n" );
   else
      printf( "%d\n", errorLogGlobalFd );

   printf( "Global message filtering level is " );
   if ( messageFilterLevel == -1 )
      printf( "\"pass all messages\"\n" );
   else
      printf( "\"pass messages <= %d\"\n", messageFilterLevel );

   return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorLogAllEnable
 *
 *   INVOCATION:
 *   errorLogAllEnable (const BOOL logAll)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   logAll   (const BOOL)   Desired "log as soon as saved" state
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Enables or disables the immediate logging of all error messages
 *
 *   DESCRIPTION:
 *   This routine sets the global variable which determines whether all
 *   error messages are logged as soon as they are saved. Enabling immediate
 *   logging may be useful in debugging situations where it is suspected that an
 *   error is being saved but never logged.
 *
 *   -   After "errorLogAllEnable(TRUE)" has been called, all errors are logged
 *      as soon as they are saved with ERROR_SET,
 *
 *   -   After "errorLogAllEnable(FALSE)" has been called, errors are only logged
 *      when they are logged explicitly with ERROR_LOG.
 *
 *   EXTERNAL VARIABLES:
 *   (<)   errorLogAll   (BOOL)   Global "log as soon as saved" flag
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

void   errorLogAllEnable
   (
   const BOOL   logAll
   )
{
   if (logAll)
      errorLogAll = TRUE;
   else
      errorLogAll = FALSE;
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorLogConsoleEnable
 *
 *   INVOCATION:
 *   errorLogConsoleEnable (const BOOL consoleEnable)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   consoleEnable   (const BOOL)   Desired "copy message to the console" flag
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Enables or disables copying of all logged messages to the console
 *
 *   DESCRIPTION:
 *   This routine sets the global variable which determines whether all
 *   messages are displayed on the console before being logged.
 *   Enabling console output may be useful in debugging situations when
 *   it is suspected that the message logging is not working.
 *
 *   -   After "errorLogConsoleEnable(TRUE)" has been called, all messages are
 *      copied to the console at the same time they are logged with
 *      ERROR_LOG or MESSAGE_LOG.
 *
 *   -   After "errorLogConsoleEnable(FALSE)" has been called, messages are
 *      logged to the file descriptor specified, but they are no longer
 *      copied explicitly to the console.
 *
 *   Note that console output happens in addition to the normal message route.
 *   If messages are being logged normally to standard output, enabling console
 *   output may cause every message to appear twice.
 *
 *   EXTERNAL VARIABLES:
 *   (<)   errorLogConsole   (BOOL)   Global "copy messages to the console" flag
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

void   errorLogConsoleEnable
   (
   const BOOL   consoleEnable
   )
{
   if (consoleEnable)
      errorLogConsole = TRUE;
   else
      errorLogConsole = FALSE;
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorLogMessage
 *
 *   INVOCATION:
 *   errorLogMessage (const int messageLevel, char * pMessage)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   messageLevel   (const int)      Verbosity level of message
 *   (>)   pMessage      (const char *)   Message to be logged
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Log an informational message
 *
 *   DESCRIPTION:
 *   This function logs an informational message to the same destination as
 *   that used for error messages. The message destination may be controlled
 *   by function errorGlobalFdSet(). Messages have the form
 *v
 *v   LOG_PREFIX task: message
 *v
 *
 *   This function is designed to be used with the MESSAGE_LOG macro contained
 *   in "errorLib.h" (see below), but is available via errorLib's public
 *   interface primarily to allow debugging from the VxWorks shell.
 *
 *   ASSOCIATED MACROS:
 *   MESSAGE_LOG  (level, message)
 *   MESSAGE_LOG1 (level, format, arg1)
 *   MESSAGE_LOG2 (level, format, arg1, arg2)
 *   MESSAGE_LOG3 (level, format, arg1, arg2, arg3)
 *   MESSAGE_LOG4 (level, format, arg1, arg2, arg3, arg4)
 *
 *   MACRO DESCRIPTION:
 *   The MESSAGE_LOG macro is used to log a message (message) of level (level).
 *
 *   The MESSAGE_LOG1, MESSAGE_LOG2, MESSAGE_LOG3 and MESSAGE_LOG4 macros are
 *   available as wrap-arounds for MESSAGE_LOG where the logged message
 *   needs to contain 1, 2, 3 or 4 variables. The logged message string
 *   is formatted by sprintf() using (format), and values are obtained
 *   from the arguments (arg1), (arg2), (arg3) and (arg4).
 *
 *   MACRO NOTE:
 *   Having several different MESSAGE_LOG macros for different numbers of
 *   arguments seems excessive, but I don't know how to write a macro
 *   with a variable number of arguments. At the moment log messages
 *   with five or more arguments cannot be dealt with. SMB - 20 Jan 98.
 *
 *   EXTERNAL VARIABLES:
 *   (>) errorLogGlobalFd   (int)         Global file descriptor
 *   (>)   messageFilterLevel   (int)         Global message filtering level
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

void   errorLogMessage
   (
   const int      messageLevel,
   const char *   pMessage
   )
{
   int    fd;
   size_t   nByte;

   char pBuf[MESSAGE_LOG_BUFFER_SIZE];

   /* Only log this message if it has a message level less than the global message filtering level.
    * If the global filtering level is set to -1 all messages are logged.
    */

   if ( (messageFilterLevel == -1) || (messageLevel <= messageFilterLevel) )
   {

      /*
       * If console logging is enabled, display the message on the console
       * before writing it to the message log output.
       */

      if ( errorLogConsole )
      {
         if (semTake(errorConsoleSem, WAIT_FOREVER) != ERROR)
         {
            printf ("%s%s\n", MESSAGE_LOG_MSG_PREFIX, pMessage);
            semGive (errorConsoleSem);
         }
         else
         {
            printErr(
               "errorLogMessage: Could not take errorConsoleSem semaphore to display \"%s\"",
               pMessage);
         }
      }

      /*
       * If a task-specific file descriptor has been defined, then use that;
       * otherwise, if the global file descriptor is not -1 error messages are
       * logged to that global file descriptor. If the global file descriptor is
       * -1 then errors are logged to "standard output".
       */

      if ( (fd = errorTaskFdGet()) == -1 )
      {
         if (errorLogGlobalFd != -1)
            fd = errorLogGlobalFd;
         else
            fd = ioGlobalStdGet (1);   /* 1 means "stdout". */
      }


      /*
       * Construct the message buffer and write it to the appropriate file descriptor.
       */

      nByte = (size_t) sprintf( pBuf, "%s%.20s: %.*s\n",
                                MESSAGE_LOG_MSG_PREFIX, taskName( taskIdSelf()),
                                MESSAGE_LOG_ONE_MSG_SIZE, pMessage );
      nByte++;                        /* include null-termination in byte-count */
      write (fd, pBuf, nByte);
   }
   return;
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorMessageFilterSet
 *
 *   INVOCATION:
 *   errorMessageFilterSet (const int level)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   level   (const int)      Message filtering level (-1 to pass all messages)
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Sets the message filtering level used when logging message
 *
 *   DESCRIPTION:
 *   This routine sets the global message filtering level used by errorLib
 *   to decide which log messages to pass. Messages are passed if they have
 *   an individual level less than the global level. Setting the global message
 *   filtering level to "-1" will pass all messages.
 *
 *   EXTERNAL VARIABLES:
 *   (<)   errorLogGlobalFd   (int)   Global file descriptor
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

void   errorMessageFilterSet
   (
   const int   level
   )
{
   messageFilterLevel = level;

#ifdef DEBUG
   printf( "Global message filtering level set to %d\n", messageFilterLevel );
#endif /* DEBUG */
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   errorMessageFilterGet
 *
 *   INVOCATION:
 *   errorMessageFilterGet (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (int)   Message filtering level
 *
 *   PURPOSE:
 *   Gets the message filtering level used when logging message
 *
 *   DESCRIPTION:
 *   This routine gets the global message filtering level used by errorLib
 *   to decide which log messages to pass. If the filtering level has been
 *   defined with errorMessageFilterSet(), that same level will be returned.
 *   If the message filtering level is not defined a -1 will be returned.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   errorLogGlobalFd   (int)   Global file descriptor
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

int   errorMessageFilterGet (void)
{
   return(messageFilterLevel);
}
