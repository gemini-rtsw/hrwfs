/*+
 * MODULE NAME:
 * errorLib
 *
 * FILENAME:
 * errorLib.h
 *
 * PURPOSE:
 * Include file for errorLib
 *
 * MACRO DESCRIPTION:
 * Recommended use of the ERROR___() macros.
 *
 * The following notes are intended to provide guidance as to how and when
 * the macros ERROR_SET() and ERROR_LOG() should be used by code developers.
 *
 * Firstly, there are two distinct types of software to consider:
 * (i) application code (a program that does something useful, usually by
 * calling a number of library routines); and (ii) library code (usually
 * intended for use by an application program).
 *
 * In either of these two fundamental types of code, three major causes of
 * an error arising have been identified, as follows:
 *
 * (a) A VxWorks subroutine returns ERROR, NULL or some other status 
 *     indicating that an error occurred. There is no way of knowning 
 *     whether the routine will have set the variable errno accordingly,
 *     but errorLib works on the assumption that errno will indeed have
 *     been set. Therefore, it does not attempt to overwrite errno in this
 *     case.
 *
 * (b) ERROR, NULL or some other status indication of an error is returned 
 *     from another library routine for which errorLib is supported. In
 *     this case, the errorLib macros will by definition have been used to
 *     handle the error when it first arose (by setting errno unless the 
 *     error arose from a lower-level VxWorks subroutine, as described in
 *     (a) above).
 *
 * (c) The user's code detects an error condition for the first time. In 
 *     other words, the error did not arise in a subroutine of any kind.
 *
 * Now, the above defines two types of software and three classes of error.
 * It is then possible to construct a table which shows the recommended 
 * calling sequence to handle an error in each of the six possible cases,
 * as follows:
 *
 * (i) Application Code - recommended use of ERROR_SET() and ERROR_LOG()
 *     macros
 *
 *      (a)      ERROR_SET (0, "message", ERROR_LOG_NOW);
 *      (b)      ERROR_LOG ("message");
 *      (c)      ERROR_SET (S_moduleName_ERROR_CODE, "message",
 *                          ERROR_LOG_NOW);
 *
 * (ii) Library Code - recommended use of ERROR_SET() and ERROR_LOG()
 *      macros
 *
 *      (a)      ERROR_SET (0, "message", ERROR_LOG_SAVE);
 *      (b)      ERROR_SET (0, "optional message", ERROR_LOG_SAVE);
 *      (c)      ERROR_SET (S_moduleName_ERROR_CODE, "message",
 *                          ERROR_LOG_SAVE);
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
 * can be used to include parameters in an error message string. Up to four
 * library "message" strings, plus the application message string, are logged
 * by errorLib, thus providing the user with a comprehensive description of
 * the cause, and the consequences, of an error condition.
 *
 * MACRO DEFICIENCIES:
 * At the moment there is no support for error or log messages containing
 * more than four arguments. This support could be provided by including
 * ERRORS_SET5 and MESSAGE_LOG5 macros, etc... Ideally, it would be nice to
 * have a single ERRORS_SET and MESSAGE_LOG macro with a variable number of
 * arguments.
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/03/17 03:14:26  cboyer
 * Initial creation of the Gemini HRWFS repository
 *
 * Revision 1.14  1998/12/11 09:20:29  cics
 * Added ** to error prefix.
 *
 * Revision 1.13  1998/12/07 11:17:18  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.12  1998/10/01 13:51:04  cics
 * ERROR_SET macro simplified to reduce the amount of duplicated code and remove a deficiency
 *
 * Revision 1.11  1998/09/28 08:53:08  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks
 *
 * Revision 1.10  1998/09/09 14:35:30  cics
 * Global variables renamed to ensure they are unique
 *
 * Revision 1.9  1998/08/31 16:41:04  smb
 * Use const keyword where needed
 *
 * Revision 1.8  1998/03/04 17:12:15  smb
 * Comment dates made more international
 *
 * Revision 1.7  1998/02/02 17:24:06  smb
 * Added errorFree to free resources allocated by errorInit
 *
 * Revision 1.6  1998/01/28 09:54:33  smb
 * Increase buffer size for messages to avoid memory coruption by sprintf()
 *
 * Revision 1.5  1998/01/21 10:47:21  smb
 * Message logging added
 *
 * Revision 1.4  1997/12/11 14:55:09  smb
 * Do not duplicate logged message
 *
 * Revision 1.3  1997/12/10 17:31:55  smb
 * Added buffer to save up to 4 error messages
 *
 * Revision 1.2  1997/12/05 15:07:42  smb
 * Included task-specific file descriptor
 *
 * Revision 1.1.1.1  1997/11/28 11:46:18  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

#ifndef   __INCerrorLibh
#define   __INCerrorLibh


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */

#include <taskLib.h>
#include <stdio.h>
#include <errnoLib.h>
#include <string.h>
#include "gemModNum.h"


/* defines */

#define ERROR_LOG_NOW          TRUE  /* Flag used to declare that an          */
                                     /* error message should be logged        */
                                     /* immediately.                          */

#define ERROR_LOG_SAVE         FALSE /* Flag used to declare that an          */
                                     /* error message should be stored        */
                                     /* for logging later.                    */

#define ERROR_LOG_MAX_MESSAGES 4     /* Maximum number of intermediate        */
                                     /* messages to be stored.                */

#define ERROR_LOG_ONE_MSG_SIZE 80    /* Size of string to hold the            */
                                     /* error code, line number or            */
                                     /* file name.                            */

#define ERROR_LOG_PAR_MSG_SIZE 160   /* Size of string to hold a single       */
                                     /* error message containing              */
                                     /* variable parameters.                  */

#define ERROR_LOG_BUFFER_SIZE  896   /* Size of error message buffer.         */
                                     /* This needs to be large enough         */
                                     /* to hold the longest possible          */
                                     /* error message string.                 */

#define ERROR_LOG_MSG_PREFIX   "<**Error**> "
                                     /* Error message prefix.                 */

#define ERROR_MSG_NONE         ""    /* String used when there is no          */
                                     /* error message to report.              */

#define MESSAGE_LOG_ONE_MSG_SIZE 160 /* Maximum length of log message         */

#define MESSAGE_LOG_MSG_PREFIX   "<Log> "   
                                     /* Log message prefix                    */

#define MESSAGE_LOG_PREFIX_SIZE  6   /* Size of log prefix                    */

#define MESSAGE_TASK_NAME_SIZE   22  /* Size of task name string              */

#define MESSAGE_LOG_BUFFER_SIZE  (MESSAGE_LOG_ONE_MSG_SIZE + \
                                  MESSAGE_LOG_PREFIX_SIZE + \
                                  MESSAGE_TASK_NAME_SIZE + 1)
                                     /* Size of log message buffer.           */

   /*
    * Error number codes used by errorLib.
    * These are designed to be processed using the vxWorks "makeStatTbl" 
    * utility.
    * At the moment no error codes are used. The following is just an unused 
    * example.
    */

#define S_errorLib_TEST (M_errorLib | 1)    /* An example code - unused.      */


   /*
    * The following defines a data structure to contain the error context
    * for each task. ERROR_CONTEXT is a pointer to that structure.
    */

typedef struct
   {
   BOOL   errorSet;                  /* Error saved flag.                     */
   int    fd;                        /* Task file descriptor (optional).      */
   char   pTaskName [ERROR_LOG_ONE_MSG_SIZE]; /* Task name.                   */
   char   pCode [ERROR_LOG_ONE_MSG_SIZE];     /* Error code string            */
   char   pFileName [ERROR_LOG_ONE_MSG_SIZE]; /* Source file name.            */
   int    lineNumber;                         /* Source line number.          */
   char   ppMessage[ERROR_LOG_MAX_MESSAGES][ERROR_LOG_BUFFER_SIZE];
                                     /* Intermediate message buffer.          */
   int    nmsg;                      /* Number of intermediate messages stored*/
   char   pBuffer [ERROR_LOG_BUFFER_SIZE];    /* Reported error message buffer*/
   int    bufferBytes;               /* Number of bytes written to buffer.    */
   } ERROR_CONTEXT_STRUCT, * ERROR_CONTEXT;


   /*
    * The following variables are defined and/or initialised in errorLib.c
    */

IMPORT   BOOL   errorLogAll;         /* When this flag is set all errors      */
                                     /* are logged immediately.               */

IMPORT   BOOL   errorLogConsole;     /* When this flag is set error           */
                                     /* messages are copied to the console.   */

IMPORT   int    errorContextTask;    /* Global error context for a            */
                                     /* particular task.                      */

IMPORT   int    errorCount;          /* Global error count for a particular   */
                                     /* task.                                 */

IMPORT   SEM_ID errorConsoleSem;     /* Semaphore protecting the console.     */

/* macro definitions */

   /*
    * The ERROR_SET macro is used to save error information into the
    * error context structure for the current task. Each error has an
    * associated error number (errorNumber) and message (message).
    * If a zero error number is provided, then the most recent error number
    * is queried from VxWorks by calling "errnoGet()" and converted into
    * the string equivalent of the error code with "strerror_r()".
    * If a non-zero error number is provided, the actual argument provided
    * by the caller is converted to a string and saved with the error message
    * (making use of the fact that the caller will specify an error number
    * using a defined constant rather than an integer value). "errnoSet()" is
    * used to set the current error number. Compiler directives are used to
    * save the current line number and source file along with the error.
    * If the "logNow" flag is set, the error will be logged immediately.
    *
    * This functionality is implemented in a macro rather than a library
    * routine to allow error messages to provide information about the location
    * in a source-code file where an error first arose, via the macros __LINE__
    * and __FILE__. If these latter two macros were accessed from, for example,
    * one of the errorLib routines directly, then the resulting line number and
    * file name would correspond to the errorLib source code, rather than to the
    * application programmer's source code in which ERROR_SET() was invoked.
    *
    * The ERROR_SET1, ERROR_SET2, ERROR_SET3 and ERROR_SET4 macros allow
    * ERROR_SET to be called with information contained in 1, 2, 3, or 4
    * arguments, which are constructed into a message string using "sprintf()"
    * called with the given format string.
    */

#define   ERROR_SET(errorNumber, message, logNow)                      \
   errorSet (__LINE__, __FILE__, (errorNumber), (message), (logNow))

#define ERROR_SET1(errorNumber, format, logNow, arg1)                  \
{                                                                      \
   char       __buffer [ERROR_LOG_PAR_MSG_SIZE];                       \
                                                                       \
   sprintf( __buffer, format, arg1 );                                  \
   ERROR_SET(errorNumber, __buffer, logNow);                           \
}

#define ERROR_SET2(errorNumber, format, logNow, arg1, arg2)            \
{                                                                      \
   char       __buffer [ERROR_LOG_PAR_MSG_SIZE];                       \
                                                                       \
   sprintf( __buffer, format, arg1, arg2 );                            \
   ERROR_SET(errorNumber, __buffer, logNow);                           \
}

#define ERROR_SET3(errorNumber, format, logNow, arg1, arg2, arg3)      \
{                                                                      \
   char       __buffer [ERROR_LOG_PAR_MSG_SIZE];                       \
                                                                       \
   sprintf( __buffer, format, arg1, arg2, arg3 );                      \
   ERROR_SET(errorNumber, __buffer, logNow);                           \
}

#define ERROR_SET4(errorNumber, format, logNow, arg1, arg2, arg3, arg4) \
{                                                                       \
   char       __buffer [ERROR_LOG_PAR_MSG_SIZE];                        \
                                                                        \
   sprintf( __buffer, format, arg1, arg2, arg3, arg4 );                 \
   ERROR_SET(errorNumber, __buffer, logNow);                            \
}


   /*
    * The ERROR_LOG macro is used to log the error currently saved in
    * a task's error context structure. Compiler directives are used
    * to determine the current line number and source file, and a message
    * supplied in "message" is logged in addition to the information
    * already saved in the error context structure.
    */

#define   ERROR_LOG(message) errorWrite (__LINE__, __FILE__, (message))


   /*
    * The MESSAGE_LOG macros are used to log informational messages,
    * which are channelled to the same destination as error messages.
    *
    * MESSAGE_LOG(level,string) may be used to log a message with the
    * specified level. The message will only be logged if "level" is
    * less than or equal to the global message filtering level set by
    * "errorMessageFilterSet()".
    *
    * MESSAGE_LOG1(level, format, arg1) may be used to log an informational
    * message containing one argument, which is included in the message
    * using sprintf() called with the given format string.
    *
    * MESSAGE_LOG2, MESSAGE_LOG3 and MESSAGE_LOG4 are similar
    * to MESSAGE_LOG1, except they allow 2, 3 or 4 arguments to be
    * specified.
    *
    * NOTE: There are several macros because I don't know how to implement
    * a macro with a variable number of arguments. SMB - 20 Jan 98.
    */


#define   MESSAGE_LOG(level, string)                      \
{                                                         \
   errorLogMessage (level, string);                       \
}

#define   MESSAGE_LOG1(level, format, arg1)               \
{                                                         \
   char       __buffer [MESSAGE_LOG_ONE_MSG_SIZE];        \
                                                          \
   sprintf( __buffer, format, arg1 );                     \
   errorLogMessage (level, __buffer);                     \
}

#define   MESSAGE_LOG2(level, format, arg1, arg2)         \
{                                                         \
   char       __buffer [MESSAGE_LOG_ONE_MSG_SIZE];        \
                                                          \
   sprintf( __buffer, format, arg1, arg2 );               \
   errorLogMessage (level, __buffer);                     \
}

#define   MESSAGE_LOG3(level, format, arg1, arg2, arg3)   \
{                                                         \
   char       __buffer [MESSAGE_LOG_ONE_MSG_SIZE];        \
                                                          \
   sprintf( __buffer, format, arg1, arg2, arg3 );         \
   errorLogMessage (level, __buffer);                     \
}

#define   MESSAGE_LOG4(level, format, arg1, arg2, arg3, arg4) \
{                                                         \
   char       __buffer [MESSAGE_LOG_ONE_MSG_SIZE];        \
                                                          \
   sprintf( __buffer, format, arg1, arg2, arg3, arg4 );   \
   errorLogMessage (level, __buffer);                     \
}


   /* function declarations */

IMPORT STATUS   errorInit (void);
IMPORT STATUS   errorFree (void);
IMPORT void     errorGlobalFdSet (const int fd);
IMPORT int      errorGlobalFdGet (void);
IMPORT void     errorTaskFdSet (const int fd);
IMPORT int      errorTaskFdGet (void);
IMPORT void     errorSet (const int lineNumber, const char * pFileName, 
                          const int errorNumber, const char * pMessage, 
                          const BOOL logNow);
IMPORT void     errorSave (const int lineNumber, const char * pFileName, 
                           const char * errorNumber, const char * pMessage);
IMPORT void     errorWrite (const int lineNumber, const char * pFileName, 
                            const char * pMessage);
IMPORT void     errorFlush (void);
IMPORT STATUS   errorShow (const int taskId);
IMPORT void     errorLogAllEnable (const BOOL logAll);
IMPORT void     errorLogConsoleEnable (const BOOL consoleEnable);
IMPORT void     errorClear (void);
IMPORT void     errorLogMessage (const int level, const char * pMessage);
IMPORT void     errorMessageFilterSet (const int level);
IMPORT int      errorMessageFilterGet (void);


#endif /* __INCerrorLibh */
