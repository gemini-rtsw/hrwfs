static struct {void *v; char *c;} rcsid = {&rcsid,
   "$Id: wfsLib.c,v 1.1.1.1 1999-03-17 03:14:24 cboyer Exp $"};

/*+
 *   MODULE NAME:
 *   wfsLib
 *
 *   FILENAME:
 *   wfsLib.c
 *
 *   PURPOSE:
 *   Wavefront sensing task library
 *
 *   DESCRIPTION:
 *   This library contains a miscellaneous collection of functions
 *   which define the environment for wavefront sensing tasks running
 *   on each CPU. The function "wfsSysInit" should be called once on
 *   each CPU in the system to define this environment, which includes
 *   information such as the processor number, the error logging status,
 *   and information on whether the EPICS database is local to a
 *   particular processor.
 *
 *   The wfsLib library also relies on external variables describing the
 *   EPICS database defined in "wfsDb.c" and "wfsDb.h" and external
 *   variables describing the local site defined in "wfsSite.c" and "wfsSite.h"
 *
 *   FUNCTION NAME(S):
 *   wfsLibInit            - Initialise wavefront sensor control library
 *   wfsTargetTypeGet      - Return the target type of the given processor
 *   wfsNumProcsGet        - Return the number of defined processors
 *   wfsSysInit            - Initialise a WFS control task on a given processor
 *   wfsWriteVersion       - Writes current s/w version to EPICS record
 *   wfsWriteState         - Writes current state variable to EPICS record
 *   wfsShow               - Display information about the current environment
 *   wfsWriteName          - Write current name of system to EPICS record
 *
 *   IGNORED FUNCTION NAME(S):
 *   wfs_errorLogPipeSet   - Initialises the error logging pipe
 *
 *   EXTERNAL MODULES:
 *   errorLib.c            - Contains error count for current processor, errorCount
 *   wfsDb.c               - Contains EPICS record initialistion info, pWfsDbRecInitialised
 *   wfsSite.c             - Contains site configuration database, pWfsArchProcessor
 *
 *   AUTHORS:
 *   Nick Dillon
 *   Steven Beard
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.25  1999/02/26 cboyer
 * Add wfsWriteName routine
 *
 * Revision 1.24  1998/12/07 11:17:27  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.23  1998/11/30 15:54:44  cics
 * Modifications made during SMB visit to Hilo, November 1998
 *
 * Revision 1.22  1998/10/08 16:20:46  cics
 * Dependency on sysextLib revised. Private function changed from P_errorLogPipeSet to wfs_errorLogPipeSet
 *
 * Revision 1.21  1998/10/01 13:49:27  cics
 * Unchanged variables changed to const
 *
 * Revision 1.20  1998/09/28 08:54:10  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks
 *
 * Revision 1.19  1998/09/09 14:35:37  cics
 * Global variables renamed to ensure they are unique
 *
 * Revision 1.18  1998/08/13 09:09:33  smb
 * Added author comment
 *
 * Revision 1.17  1998/07/09 15:22:48  smb
 * sysextProcNumGet replaced with sysProcNumGet
 *
 * Revision 1.16  1998/06/30 14:07:17  smb
 * Ensure everything works when sysextLib and mpPipeDrv removed. Fixed mistakes.
 *
 * Revision 1.15  1998/06/30 13:31:36  smb
 * Do not include VME mode if NO_SYSEXTLIB requested.
 *
 * Revision 1.14  1998/06/30 13:11:31  smb
 * Dependency on sysextLib and mpPipeDrv can be removed using NO_SYSEXTLIB and NO_MPPIPEDRV macros.
 *
 * Revision 1.13  1998/05/13 11:03:20  smb
 * Extra checking and debugging information added. Bus reset on timeout removed.
 *
 * Revision 1.12  1998/03/27 12:07:06  smb
 * Minor changes to clarify logic
 *
 * Revision 1.11  1998/03/24 16:16:37  smb
 * Several bugs fixed and notes added
 *
 * Revision 1.10  1998/03/05 14:23:23  smb
 * Comment dates made more international
 *
 * Revision 1.9  1998/03/02 14:06:28  smb
 * Site specific parts removed from wfsLib
 *
 * Revision 1.8  1998/02/23 13:38:57  smb
 * Rearranged code for printability
 *
 * Revision 1.7  1998/02/05 15:26:52  smb
 * wfsWriteState added
 *
 * Revision 1.6  1998/01/30 15:30:19  smb
 * Fixed some problems uncovered by prolint
 *
 * Revision 1.5  1998/01/21 10:47:09  smb
 * Message logging added
 *
 * Revision 1.4  1998/01/19 16:18:21  smb
 * Update individual health records
 *
 * Revision 1.3  1997/12/02 16:10:49  smb
 * Cannot write commit date due to bug in epToVxPipeWrite
 *
 * Revision 1.2  1997/12/02 15:48:15  smb
 * Use RCS keywords to write version record
 *
 * Revision 1.1.1.1  1997/11/28 11:46:17  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

/* Includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */

#include <taskLib.h>
#include <stdio.h>
#include <pipeDrv.h>
#include <ioLib.h>
#include <memLib.h>
#include "gemTypes.h"
#include "timeoutLib.h"
#include "errorLib.h"

#include <sirRecord.h>

#ifndef NO_SYSEXTLIB                     /* Define this macro to remove sysextLib   */
#include "sysextLib.h"
#else
#define SYSEXT_MAX_N_PROC   15
#endif   /* NO_SYSEXTLIB */

#ifndef NO_MPPIPEDRV                     /* Define this macro to remove mpPipeDrv   */
#include "mpPipeDrv.h"
#endif   /* NO_MPPIPEDRV */

#include "epToVxLib.h"
#include "wfsLib.h"
#include "errorLog.h"
#include "wfsDb.h"
#include "wfsSite.h"


/* Defines */

/* #define DEBUG */                     /* Define this macro to enable debug messages */

#define WFSLIB_LOGPIPE_NMSG_SLOTS  8       /* Size of error log message queue.    */

LOCAL STATUS   wfs_errorLogPipeSet (void); /* Private function for initialising   */
                                           /* the error logging pipe used by each */
                                           /* wavefront sensor control process.   */

/* imported variables */

IMPORT BOOL     pWfsDbRecInitialised [N_RECORD_TYPES];
                                           /* Array to show when each type of     */
                                           /* EPICS record has been initialised.  */
                                           /* It is imported from "wfsDb.c".      */

IMPORT int      errorCount;                /* Current global error count.         */
                                           /* It is imported from "errorLib.c".   */

/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   wfsLibInit
 *
 *   INVOCATION:
 *   wfsLibInit ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, or ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Initialise wavefront sensor control library
 *
 *   DESCRIPTION:
 *   This routine carries out an integrity check on the "pWfsArchProcessor" data
 *   structure initialised in wfsSite.c and used internally by the wfsLib library.
 *   It will return OK if the structure looks ok or ERROR if a problem is detected.
 *
 *   The function only checks that the number of processors is sensible and
 *   a valid target type could be obtained for each processor. It is up to the
 *   programmer to ensure that other elements of the "pWfsArchProcessor" data
 *   structure (e.g. IP address) are defined correctly.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   pWfsNumProcessors   (int)                    Number of processors
 *                                                      (from wfsSite.c)
 *   (>)   pWfsArchProcessor   (WFS_ARCH_PROCESSOR[])   Array of data structures
 *                                                      (from wfsSite.c)
 *
 *   PRIOR REQUIREMENTS:
 *   The "pWfsArchProcessor" data structure should have been defined and
 *   initialised before calling this function. (See above).
 *
 *   INCLUDE FILES:
 *   wfsLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   wfsLibInit (void)
{
   FAST int   procNum;            /* Processor number */
   int        targetType;         /* Target type      */

   /* Check the number of processors (defined in wfsSite.c) is sensible. */

   if ((pWfsNumProcessors < 1) || (pWfsNumProcessors >= SYSEXT_MAX_N_PROC))
   {
      ERROR_SET (S_wfsLib_INVALID_DATA, "Invalid number of processors", ERROR_LOG_SAVE);
   }

   /*
    * For each of the processors, check that the target type could be obtained
    * and check that the target type contains a sensible value. Also check
    * that the local RAM size and processor clock speed are greater than zero.
    * Failure of these checks would suggest that the pWfsArchProcessor
    * structure has not been defined properly in wfsSite.c.
    */

   for (procNum = 0; procNum < pWfsNumProcessors; procNum++)
   {
      targetType = wfsTargetTypeGet ((int) procNum);

      printf ("wfsLibInit: Target type = %d, Clock rate = %f, RAM size = %ld\n",
              targetType, pWfsArchProcessor[procNum].procClockRate,
              pWfsArchProcessor[procNum].procRamSize);

      if ( (targetType == -1) ||
           ((targetType != TARGET_TYPE_MV167) && (targetType != TARGET_TYPE_HKBAJA47)) )
      {
         ERROR_SET (S_wfsLib_INVALID_DATA, "Unknown target type", ERROR_LOG_SAVE);
         return (ERROR);
      }

      if ( (pWfsArchProcessor[procNum].procClockRate <= 0.0) ||
           (pWfsArchProcessor[procNum].procRamSize == 0) )
      {
         ERROR_SET (S_wfsLib_INVALID_DATA, "Bad clock rate or RAM size", ERROR_LOG_SAVE);
         return (ERROR);
      }
   }

   return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   wfsTargetTypeGet
 *
 *   INVOCATION:
 *   wfsTargetTypeGet (processorNumber)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   processorNumber      (const int)      Processor number
 *
 *   FUNCTION VALUE:
 *   (int)   Target type if successful, or -1 if unsuccessful
 *
 *   PURPOSE:
 *   Return the target type of the given processor
 *
 *   DESCRIPTION:
 *   This routine looks up the target type for a given processor number
 *   in the data structure and returns it. A -1 is returned if the processor
 *   number has not been defined.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   pWfsNumProcessors   (int)                    Number of processors
 *   (>)   pWfsArchProcessor   (WFS_ARCH_PROCESSOR[])   Array of data structures
 *
 *   PRIOR REQUIREMENTS:
 *   The "pWfsArchProcessor" data structure should have been defined and
 *   initialised before calling this function. (See above).
 *
 *   INCLUDE FILES:
 *   wfsLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

int   wfsTargetTypeGet
   (
   const int   processorNumber      /* Processor number */
   )
{

   /*
    * Return a -1 if the processor number is out of range.
    * Otherwise return the target type.
    */

   if (processorNumber < 0 || processorNumber >= pWfsNumProcessors)
      return (-1);

   return ((int) pWfsArchProcessor [processorNumber].targetType);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   wfsNumProcsGet
 *
 *   INVOCATION:
 *   wfsNumProcsGet (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (int)   Number of processors
 *
 *   PURPOSE:
 *   Return the number of defined processors
 *
 *   DESCRIPTION:
 *   This routine returns the value of the global variable which is initialised
 *   to the number of processors defined in the "pWfsArchProcessor" data
 *   structure. No checks are made on this value.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   pWfsNumProcessors      (int)               Number of processors
 *
 *   PRIOR REQUIREMENTS:
 *   The numprocessor variable should have been initialised before calling
 *   this function.
 *
 *   INCLUDE FILES:
 *   wfsLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

int wfsNumProcsGet (void)
{
   return (pWfsNumProcessors);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   wfsSysInit
 *
 *   INVOCATION:
 *   wfsSysInit (processorNumber, redirectErrorLog)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   processorNumber    (const int)      Processor number.
 *   (>)   redirectErrorLog   (const BOOL)     Flag to redirect error log
 *                            (0=VxWorks console; 1=error logging pipe)
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, or ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Initialise a wavefront sensor control task on a given processor
 *
 *   DESCRIPTION:
 *   This routine initialises the wfsLib library, the processor number,
 *   the VME bus, and all the pipes and CAD records used by a wavefront
 *   sensor control task.
 *
 *   The error log redirection flag can be used to direct log messages
 *
 *   EXTERNAL VARIABLES:
 *   (>)   pWfsNumProcessors      (int)     Number of processors
 *
 *   (>)   pWfsArchProcessor      (WFS_ARCH_PROCESSOR[])   Array of data structures
 *
 *   (<)   wfsDbEpicsDbIsLocal    (BOOL)    Set if the EPICS database
 *                                          is on the local processor
 *                                          (defined in module
 *                                          "wfsDb.c")
 *
 *   (<)   pWfsDbRecInitialised   (BOOL[])  Array of flags set when
 *                                          each type of EPICS record
 *                                          has been initialised
 *                                          (defined in module
 *                                          "wfsDb.c")
 *
 *   PRIOR REQUIREMENTS:
 *   The "pWfsArchProcessor" data structure should have been defined and
 *   initialised before calling this function. (See above).
 *   The "wfsDb" module should be linked with any code intending to use this
 *   function, since "wfsDb" contains some external variables assumed by this
 *   function to exist.
 *
 *   NOTE:
 *   It seems odd that the processor number needs to be defined by this
 *   function when it has already been defined in the VxWorks boot parameters.
 *   Nick Dillon said the "processor number" used here and in the VxWorks boot
 *   parameters are two different things. The processor number declared here
 *   is used as a unique identifier by the multi-processor pipe driver. It is
 *   also used to identify the processor running EPICS (labelled processor 0).
 *   I find using the same name as a VxWorks boot parameter confusing. Either
 *   the name of this parameter needs changing, or the VxWorks boot parameter
 *   should be used as a processor identifier instead.
 *   SMB - 19 Jan 98.
 *
 *   INCLUDE FILES:
 *   wfsDb.h
 *   wfsLib.h
 *-
 */

STATUS wfsSysInit
   (
   const int   processorNumber,    /* Processor number         */
   const BOOL  redirectErrorLog    /* Redirect errors to pipe? */
   )
{
   FAST int   procNum;             /* Processor number index.  */
   uint32     pTargetType [SYSEXT_MAX_N_PROC];   
                                   /* Array of target types    */
                                   /* for each processor.      */

   /*
    * Create and initialise an error context structure for the task
    * executing this function (most likely the VxWorks shell).
    */

   if (errorInit () == ERROR)
   {
      printErr ("Failed to initialise error context structure.\n");
      return (ERROR);
   }

   /* Initialise the wfsLib library. */

   if (wfsLibInit () == ERROR)
   {
      ERROR_LOG ("Failed to initialise wfsLib - check pWfsArchProcessor definition");
      return (ERROR);
   }

   /*
    * Check the processor number is sensible. Processor numbers start at zero, so
    * the valid range is from 0 to one less than the number of processors.
    */

   if ( (processorNumber < 0) || (processorNumber >= pWfsNumProcessors) )
   {
      ERROR_SET (S_wfsLib_BAD_ARGUMENT, "Invalid processor number supplied", ERROR_LOG_NOW);
      return (ERROR);
   }

#ifndef NO_SYSEXTLIB

   /*
    * Using sysextLib.
    * Always set the processor number.
    */

   printf ("wfsSysInit: Setting processor number to %d.\n", processorNumber);
   if (sysextProcNumSet (processorNumber) == ERROR)
   {
      ERROR_LOG ("Failed to set processor number");
      return (ERROR);
   }

   /*
    * Set the bus-arbitration mode to that defined in the
    * "pWfsArchProcessor" data structure.
    */
   sysextVmeReqRelInit (& pWfsArchProcessor [processorNumber].vmeMode);

#else
   /*
    * Not using sysextLib.
    * If the processor number for the CPU board is different from the value provided
    * then set it.
    */

   if (processorNumber != (sysProcNumGet()))
   {
      printf ("wfsSysInit: Setting processor number to %d without sysextLib.\n",
         processorNumber);
      sysProcNumSet (processorNumber);
      if (processorNumber != (sysProcNumGet()))
      {
         ERROR_LOG ("Failed to set processor number");
         return (ERROR);
      }
   }
   else
   {
      printf ("wfsSysInit: Processor number already defined to %d without sysextLib.\n",
         processorNumber);
   }

#endif /* NO_SYSEXTLIB */

   /*
    * Obtain the target types for each of the processors on the bus and load
    * then into the "pTargetType" array. Then use this array to define the
    * target types on the VME bus.
    */

   for (procNum = 0; procNum < pWfsNumProcessors; procNum++)
   {
      pTargetType [procNum] = pWfsArchProcessor [procNum].targetType;
   }

#ifndef NO_SYSEXTLIB

#ifdef DEBUG
      printf ("wfsSysInit: Initialising VME network with targets: ");
      for (procNum = 0; procNum < pWfsNumProcessors; procNum++)
      {
         printf ("proc%d=%d ", procNum, pTargetType [procNum]);
      }
      printf ("\n");
#endif /* DEBUG */

   if (sysextVmeNetworkInit (pWfsNumProcessors, pTargetType) == ERROR)
   {
      ERROR_LOG ("Failed to initialise VME network");
      return (ERROR);
   }
#else
      printf ("wfsSysInit: No need to initialise VME network without sysextLib.\n");
#endif   /* NO_SYSEXTLIB */

   /*
    * Check whether the task is running on the root processor (processor 0)
    * or another processor.
    */

   if (processorNumber == 0)
   {

      /*
       * The task is running on the root processor. Record the fact that the
       * EPICS database is local and initialise the multi-processor pipe
       * driver with the timeout defined in PIPE_DRV_TIMEOUT_PROC_0.
       * If the second argument to mpPipeDrv() is TRUE, a timeout will result
       * in a bus reset and reboot.
       */

#ifdef DEBUG
      printf ("wfsSysInit: Initialising timeout timer and mpPipeDrv. EPICS is local.\n");
#endif

      wfsDbEpicsDbIsLocal = TRUE;

      if (timeoutInit () == ERROR)
      {
         ERROR_LOG ("Failed to initialise timeout timer");
         return (ERROR);
      }

#ifndef NO_MPPIPEDRV
/*      if (mpPipeDrv (PIPE_DRV_TIMEOUT_PROC_0, TRUE) == ERROR) */
      if (mpPipeDrv (PIPE_DRV_TIMEOUT_PROC_0, FALSE) == ERROR)   /* Disable bus reset - SMB 7 May 98 */
      {
         ERROR_LOG ("Failed to initialise mpPipeDrv");
         return (ERROR);
      }
#endif   /* NO_MPPIPEDRV */
   }
   else
   {

      /*
       * The task is running on another processor. Initialise the timout
       * library and initialise the multi-processor pipe driver with the
       * with the timeout defined in PIPE_DRV_TIMEOUT_PROC_N.
       * For these processors a timeout will not result
       * in a bus reset.
       */

#ifdef DEBUG
      printf ("wfsSysInit: Initialising timeout timer and mpPipeDrv. EPICS is not local.\n");
#endif

      if (timeoutInit () == ERROR)
      {
         ERROR_LOG ("Failed to initialise timeout timer");
         return (ERROR);
      }

#ifndef NO_MPPIPEDRV
      if (mpPipeDrv (PIPE_DRV_TIMEOUT_PROC_N, FALSE) == ERROR)
      {
         ERROR_LOG ("Failed to initialise mpPipeDrv");
         return (ERROR);
      }
#endif   /* NO_MPPIPEDRV */
   }

   /*
    * If this is not the root processor and the CAD records have not already
    * been initialised, then initialise them. Note that this only has to be
    * done once for all the tasks on this processor. The taskLock() ensures
    * all other tasks are temporarily locked out and epToVxDbInitCadCar() only
    * gets called once on each processor.
    *
    * NOTE: On the root processor epToVxDbInitCadCar() will already have
    * been executed when initialising the EPICS database during iocInit().
    */

   taskLock ();
   if ((processorNumber != 0) && (! pWfsDbRecInitialised [CAD_RECORD_TYPE]))
   {

#ifdef DEBUG
      printf ("wfsSysInit: Initialising CAD and CAR records.\n");
#endif

      if (epToVxDbInitCadCar () == ERROR)
      {
         ERROR_LOG ("Failed to initialise CAD records");
         return (ERROR);
      }
   }
   taskUnlock ();

   /*
    * Initialise the pipe used to connect to EPICS records.
    */

#ifdef DEBUG
      printf ("wfsSysInit: Initialising EPICS record pipe.\n");
#endif

   if (epToVxPipeInit (processorNumber) == ERROR)
   {
      ERROR_LOG ("Failed to initialise EPICS record pipe");
      return (ERROR);
   }

   /*
    * Redirect error messages to the error logging pipe if requested.
    */

   if (redirectErrorLog)
   {

#ifdef DEBUG
      printf ("wfsSysInit: Redirecting errors to error log pipe.\n");
#endif
      if (wfs_errorLogPipeSet () == ERROR)
      {
         ERROR_LOG ("Failed to initialise error-logging pipe");
         return (ERROR);
      }
   }

#ifdef DEBUG
   return(wfsShow());
#endif

   return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   wfsWriteVersion
 *
 *   INVOCATION:
 *   wfsWriteVersion (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, or ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Writes the current software version to the "version" EPICS record
 *
 *   DESCRIPTION:
 *   This routine obtains the RCS revision number and commit date and time
 *   and writes this to an EPICS record called "version".
 *
 *   EXTERNAL VARIABLES:
 *   (>)   pWfsNumProcessors      (int)      Number of processors
 *
 *   PRIOR REQUIREMENTS:
 *   The numprocessor variable should have been initialised before calling
 *   this function.
 *
 *   It is assumed this module has been committed using CVS or RCS, so that the
 *   RCS "Revision" and "Date" keywords contain the revision number and commit
 *   date. If RCS is not available, the function can be made to use the compile
 *   date and time instead by defining the NO_RCS preprocessor flag.
 *
 *   It is assumed that an EPICS record daemon is running or will soon
 *   be spawned
 *
 *   INCLUDE FILES:
 *   gemTypes.h
 *   wfsLib.h
 *
 *   DEFICIENCIES:
 *   The RCS keywords are updated only when wfsLib is committed. They
 *   do not reflect the state of other modules in the system. It would
 *   be useful if the value of the tag describing the latest release
 *   of the whole AGWPS system could be obtained.
 *
 *   BUGS:
 *   The epToVxPipeWrite() function fails if the RCS keywords translate
 *   to a string longer than 40 characters. Because of this, only the RCS
 *   revision number is currently written.
 *-
 */

STATUS   wfsWriteVersion (void)
{

   /*
    * If RCS is available, use the revision [and date] keywords set by RCS; or
    * if RCS is not available, use the COMPILE_DATE_AND_TIME macro
    * (defined in gemTypes.h); to write the latest compile date and time to
    * the "version" EPICS record.
    */

#ifdef NO_RCS
   if (epToVxPipeWrite ("version", COMPILE_DATE_AND_TIME, 0) == ERROR)
#else
    if (epToVxPipeWrite ("version", "$Revision: 1.1.1.1 $", 0) == ERROR)
#endif
   {
      ERROR_LOG ("Failed to write version number");
      return (ERROR);
   }
   else
   {
      return (OK);
   }
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   wfsWriteState
 *
 *   INVOCATION:
 *   wfsWriteState (recordName, value)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   recordName (char *)   Name of EPICS record to update
 *   (>)   value      (int)      Value to write
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, or ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Writes the given integer value to the named EPICS record
 *
 *   DESCRIPTION:
 *   This routine may be used to update an EPICS record describing
 *   the current system state (assuming the state is described by
 *   an integer value). It is a wrap-up for epToVxPipeWrite(), except
 *   that where epToVxPipeWrite() requires a pointer to the value,
 *   this function can be called with the value itself (making it
 *   easier to call from the VxWorks shell or startup script).
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   It is assumed that an EPICS record daemon is running or will soon
 *   be spawned
 *
 *   INCLUDE FILES:
 *   gemTypes.h
 *   wfsLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *-
 */

STATUS   wfsWriteState (char * recordName, int value)
{
   int *   pValue;         /* Pointer to integer value */

   /* Set up a pointer to the integer value provided. */

   pValue = &value;

    if (epToVxPipeWrite (recordName, (char *) pValue, 0) == ERROR)
   {
      ERROR_LOG ("Failed to write current state");
      return (ERROR);
   }
   else
   {
      return (OK);
   }
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   wfsShow
 *
 *   INVOCATION:
 *   wfsShow (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, or ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Display information about the wavefront sensing environment
 *
 *   DESCRIPTION:
 *   This routine displays the processor information defined in the wfsLib
 *   library.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   pWfsNumProcessors   (int)     Number of processors
 *
 *   (>)   pWfsSiteName        (char *)  Name of site
 *
 *   (>)   pWfsArchProcessor   (WFS_ARCH_PROCESSOR[])
 *                                       Array of data structures
 *
 *   (>)   wfsDbEpicsDbIsLocal (BOOL)    Set if the EPICS database
 *                                       is on the local processor
 *                                       (defined in module
 *                                       "wfsDb.c")
 *
 *   (>)   pWfsDbRecInitialised   (BOOL[]) 
 *                                        Array of flags set when
 *                                       each type of EPICS record
 *                                       has been initialised
 *                                       (defined in module
 *                                       "wfsDb.c")
 *
 *   PRIOR REQUIREMENTS:
 *   The "pWfsArchProcessor" data structure should have been defined and
 *   initialised before calling this function. (See above).
 *   The "wfsDb" module should be linked with any code intending to use this
 *   function, since "wfsDb" contains some external variables assumed by this
 *   function to exist. 
 *
 *   INCLUDE FILES:
 *   wfsDb.h
 *   wfsLib.h
 *-
 */

STATUS wfsShow (void)
{
   int      procNum;      /* Processor number.  */
   float    clockRate;    /* Clock rate in MHz  */
   float    ramSize;      /* RAM size in Mbytes */

   printf ("Information defined for the %s environment\n", pWfsSiteName );
   printf ("=====================================================\n" );

   /* Get the processor number for the CPU board.
    */

   if ((procNum = sysProcNumGet()) == ERROR)
   {
      ERROR_LOG ("Failed to get processor number");
      return (ERROR);
   }

   printf ("The current processor number is %d, ", procNum);

   if (wfsDbEpicsDbIsLocal)
      printf ("with EPICS running locally.\n\n");
   else
      printf ("with EPICS on another processor.\n\n");


   /*
    * Display the information known about each processor.
    */

   printf ("Proc#      Name Target   Clock/MHz  RAM/Mbyte  VME parameters\n");
   printf ("-----      ---- ------   ---------  ---------  --------------\n");

   for (procNum = 0; procNum < pWfsNumProcessors; procNum++)
   {
      printf ("%5d  %8.8s ", procNum, pWfsArchProcessor[procNum].pProcName );

      if ( pWfsArchProcessor[procNum].targetType == TARGET_TYPE_MV167 )
         printf ("MV167    ");
      else if ( pWfsArchProcessor[procNum].targetType == TARGET_TYPE_HKBAJA47 )
         printf ("HKBAJA47 ");
      else
         printf ("unknown  ");

      clockRate = pWfsArchProcessor[procNum].procClockRate / 1.0e06;
      ramSize = (float) pWfsArchProcessor[procNum].procRamSize / 1.048576e06;
      printf ("%9.2f  %9.2f  ", clockRate, ramSize );

#ifndef NO_SYSEXTLIB
      printf ("%d %d %d %d %d %d\n",
              pWfsArchProcessor[procNum].vmeMode.requestLevel,
              pWfsArchProcessor[procNum].vmeMode.releaseWhenDone,
              pWfsArchProcessor[procNum].vmeMode.fairRequester,
              pWfsArchProcessor[procNum].vmeMode.roundRobinArbiter,
              pWfsArchProcessor[procNum].vmeMode.requestLevelDma,
              pWfsArchProcessor[procNum].vmeMode.fairRequesterDma );
#else
      printf ("(no sysextLib)\n");
#endif   /* NO_SYSEXTLIB */
   }

   return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   wfsBusReset
 *
 *   INVOCATION:
 *   wfsBusReset (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Resets the VME bus (engineering function)
 *
 *   DESCRIPTION:
 *   This routine resets the VME bus. It may be used to free up the VME bus if it
 *   has hung up after a software or hardware problem. The reset will cause the
 *   IOC to reboot.
 *
 *   NOTE:
 *   This code has been copied from sysextLib, since sysextLib is no longer going
 *   to be used. It only works on an mv167.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   It is assumed that an EPICS record daemon is running or will soon
 *   be spawned
 *
 *   INCLUDE FILES:
 *   gemTypes.h
 *   wfsLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *-
 */

#define   BIT_SET(p, d)    { __typeof__ (* (p)) __temp = (* (p));   \
                           * (p) = __temp | (d); }

#define   BUS_RESET_REG_MV167   0xfff40060   /* Bus reset register for MVME167           */
#define   BUS_RESET_BIT_MV167   0x01800000   /* Reset-Switch-Enable and Bus-Reset bits   */

void   wfsBusReset (void)
{

   printf ("wfsBusReset: BUS RESET - SYSTEM WILL REBOOT.\n");

   /* Brief pause to allow printf() to flush..   */

   taskDelay (sysClkRateGet ());

   /* ..then waggle the hardware bits   */
   BIT_SET ((HW_REG32 *) BUS_RESET_REG_MV167, BUS_RESET_BIT_MV167);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *   IGNORED FUNCTION NAME:
 *   wfs_errorLogPipeSet
 *
 *   INVOCATION:
 *   wfs_errorLogPipeSet ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, or ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Initialises the error logging pipe
 *
 *   DESCRIPTION:
 *   This routine creates and initialises the pipe for communicating with the
 *   errorLog task.
 *
 *   NOTE: This is a private function used only by wfsLib.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   errorCount      int      Initial error count
 *                                  (defined in module "errorLib.c")
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   errorLog.h
 *   wfsLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   wfs_errorLogPipeSet (void)
{
   int      procNumber;           /* Processor number */
   int      fd;                   /* File descriptor for pipe */
   STATUS   (* pipeCreate) ();    /* Pointer to appropriate pipe create function */
   char     pNameExtension [4];   /* Pipe name extension */

   /*
    * Obtain the processor number and point to the appropriate pipe creation
    * routine. On the root processor the standard pipe driver, pipeDrv, is
    * used, but on other processors the multi-processor pipe driver, mpPipeDrv,
    * is used.
    */
   procNumber = sysProcNumGet ();

   if (procNumber == -1)
   {
      return (ERROR);
   }
#ifndef NO_MPPIPEDRV
   else if (procNumber == 0)
   {
      pipeCreate = pipeDevCreate;
   }
   else
   {
      pipeCreate = mpPipeDevCreate;
   }
#else
   pipeCreate = pipeDevCreate;   /* Without MPPIPEDRV always use conventional pipe driver. */

#endif   /* NO_MPPIPEDRV */

   /*
    * Make up a pipe name extension from the processor number and open the
    * error logging pipe.
    */

   sprintf (pNameExtension, "%.2d", procNumber);
   if ((fd = epToVxPipeOpen (FALSE, LOGTASK_PIPE_NAME, pNameExtension,
      pipeCreate, WFSLIB_LOGPIPE_NMSG_SLOTS, ERROR_LOG_BUFFER_SIZE, O_WRONLY,
      -1, 0.0, 0.0)) == ERROR)
   {
      printErr ("wfs_errorLogPipeSet: Error creating and opening error logging pipe\n");
      ERROR_SET (S_wfsLib_ERRLOG_PIPE_FAIL, "Pipe create/open failed", ERROR_LOG_NOW);
      return (ERROR);
   }

   /*
    * Change the global error log file descriptor from stderr to the new pipe's
    * file descriptor.
    */

   taskLock ();
   errorGlobalFdSet (fd);
   taskUnlock ();

   return (OK);
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *   FUNCTION NAME:
 *   wfsWriteName
 *
 *   INVOCATION:
 *   wfsWriteName (sirRecord)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, or ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Writes the current software name to the "name" EPICS record
 *
 *   DESCRIPTION:
 *   This routine writes the name of the software to an EPICS record called "name".
 *
 *   EXTERNAL VARIABLES:
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   <sirRecord>.h
 *
 *   DEFICIENCIES:
 *
 *   BUGS:
 *-
 */

STATUS   wfsWriteName (struct sirRecord *psir)
{

    strcpy ( (char *)psir->val , "High Resolution Wavefront Sensor" ) ;
    return (OK) ;
}
