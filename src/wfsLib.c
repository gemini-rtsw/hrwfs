static struct {void *v; char *c;} rcsid = {&rcsid,
   "$Id: wfsLib.c,v 1.7 2001-06-13 00:39:12 cboyer Exp $"};

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
 *   EPICS database defined in "wfsHrwfsDb.c" and "wfsHrwfsDb.h" and external
 *   variables describing the local site defined in "wfsSite.c" and "wfsSite.h"
 *
 *   FUNCTION NAME(S):
 *   wfsLibInit            - Initialise wavefront sensor control library
 *   wfsTargetTypeGet      - Return the target type of the given processor
 *   wfsNumProcsGet        - Return the number of defined processors
 *   wfsSysInit            - Initialise a WFS control task on a given processor
 *   wfsWriteState         - Writes current state variable to EPICS record
 *   wfsShow               - Display information about the current environment
 *   wfsInitTelName        - Init the telescope name from the TCS
 *   wfsGetTelName         - Get the local copy of the TCS Telescope name
 *   wfsUpdateAg           - Update the local copy of the CC info from the AG 
 *   showAcCCStruct        - Show the contents of the AC CC struct
 *
 *   IGNORED FUNCTION NAME(S):
 *   wfs_errorLogPipeSet   - Initialises the error logging pipe
 *
 *   EXTERNAL MODULES:
 *   errorLib.c            - Contains error count for current processor, 
 *                           errorCount
 *   wfsHrwfsDb.c          - Contains EPICS record initialistion info, 
 *                           pWfsDbRecInitialised
 *   wfsSite.c             - Contains site configuration database, 
 *                           pWfsArchProcessor
 *
 *   AUTHORS:
 *   Nick Dillon
 *   Steven Beard
 *
 *   MODIFICATION
 *   - 05 jun 2001 - cb - add wfsUpdateAg and showAcCCStruct
 *   - 10 dec 1999 - cb - tidy up
 *   - 22 Nov 1999 - cb - bug fixed into wfsInitTelName 
 *   - 9 Nov 1999 - cb - add wfsInitTelName and wfsGetTelName
 *   - A lot of SMB modifications before delivery by UK.
 *   - Creation 28 Nov 1997
 *
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
#include <genSubRecord.h>
#include <alarm.h>


#define SYSEXT_MAX_N_PROC   15

#include "epToVxLib.h"
#include "wfsLib.h"
#include "errorLog.h"
#include "wfsHrwfsDb.h"
#include "wfsSite.h"


/* Defines */

/* #define DEBUG */             /* Define this macro to enable debug messages */

#define WFSLIB_LOGPIPE_NMSG_SLOTS  8     /* Size of error log message queue.  */

LOCAL STATUS wfs_errorLogPipeSet (void); /* Private function for initialising */
                                         /* error logging pipe used by each   */
                                         /* wavefront sensor control process. */

/* imported variables */

IMPORT BOOL pWfsDbRecInitialised [N_RECORD_TYPES];
                                         /* Array to show when each type of   */
                                         /* EPICS record has been initialised.*/
                                         /* It is imported from "wfsDb.c".    */

IMPORT int errorCount;                   /* Current global error count.       */
                                         /* It is imported from "errorLib.c". */

/* Global variables */

char tcsTelName [40] ;

AC_CC_STRUCT acCCId;

/* -------------------------------------------------------------------------- */

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
 *   This routine carries out an integrity check on the "pWfsArchProcessor" 
 *   data structure initialised in wfsSite.c and used internally by the wfsLib 
 *   library.
 *   It will return OK if the structure looks ok or ERROR if a problem is 
 *   detected.
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
      ERROR_SET (S_wfsLib_INVALID_DATA, "Invalid number of processors", 
                 ERROR_LOG_SAVE);
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
           ((targetType != TARGET_TYPE_MV167) && 
           (targetType != TARGET_TYPE_HKBAJA47) &&
           (targetType != TARGET_TYPE_MV2700)) )
      {
         ERROR_SET (S_wfsLib_INVALID_DATA, "Unknown target type", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }

      if ( (pWfsArchProcessor[procNum].procClockRate <= 0.0) ||
           (pWfsArchProcessor[procNum].procRamSize == 0) )
      {
         ERROR_SET (S_wfsLib_INVALID_DATA, "Bad clock rate or RAM size", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }
   }

   return (OK);
}


/* -------------------------------------------------------------------------- */

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


/* -------------------------------------------------------------------------- */

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


/* -------------------------------------------------------------------------- */

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
 *   (>) pWfsNumProcessors    (int)     Number of processors
 *
 *   (>) pWfsArchProcessor    (WFS_ARCH_PROCESSOR[])   Array of data structures
 *
 *   (<) wfsDbEpicsDbIsLocal  (BOOL)    Set if the EPICS database
 *                                      is on the local processor
 *                                      (defined in module
 *                                      "wfsDb.c")
 *
 *   (<) pWfsDbRecInitialised (BOOL[])  Array of flags set when
 *                                      each type of EPICS record
 *                                      has been initialised
 *                                      (defined in module
 *                                      "wfsDb.c")
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
    * Check the processor number is sensible. Processor numbers start at zero, 
    * so the valid range is from 0 to one less than the number of processors.
    */

   if ( (processorNumber < 0) || (processorNumber >= pWfsNumProcessors) )
   {
      ERROR_SET (S_wfsLib_BAD_ARGUMENT, "Invalid processor number supplied", 
                 ERROR_LOG_NOW);
      return (ERROR);
   }

   /*
    * If the processor number for the CPU board is different from the value 
    * provided then set it.
    */

   if (processorNumber != (sysProcNumGet()))
   {
      printf ("wfsSysInit: Setting processor number to %d\n",
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
      printf ("wfsSysInit: Processor number already defined to %d.\n",
              processorNumber);
   }


   /*
    * Obtain the target types for each of the processors on the bus and load
    * then into the "pTargetType" array. Then use this array to define the
    * target types on the VME bus.
    */

   for (procNum = 0; procNum < pWfsNumProcessors; procNum++)
   {
      pTargetType [procNum] = pWfsArchProcessor [procNum].targetType;
   }

   printf ("wfsSysInit: No need to initialise VME network.\n");

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
      printf ("wfsSysInit: Initialising timeout timer. EPICS is local.\n");
#endif

      wfsDbEpicsDbIsLocal = TRUE;

      if (timeoutInit () == ERROR)
      {
         ERROR_LOG ("Failed to initialise timeout timer");
         return (ERROR);
      }
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
      printf ("wfsSysInit: Initialising timeout timer. EPICS is not local.\n");
#endif

      if (timeoutInit () == ERROR)
      {
         ERROR_LOG ("Failed to initialise timeout timer");
         return (ERROR);
      }
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


/* -------------------------------------------------------------------------- */

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


/* -------------------------------------------------------------------------- */

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
 *                                       Array of flags set when
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
      else if ( pWfsArchProcessor[procNum].targetType == TARGET_TYPE_MV2700 )
         printf ("MV2700 ");
      else
         printf ("unknown  ");

      clockRate = pWfsArchProcessor[procNum].procClockRate / 1.0e06;
      ramSize = (float) pWfsArchProcessor[procNum].procRamSize / 1.048576e06;
      printf ("%9.2f  %9.2f  ", clockRate, ramSize );
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

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
   int      procNumber;           /* Processor number                         */
   int      fd;                   /* File descriptor for pipe                 */
   STATUS   (* pipeCreate) ();    /* Pointer to appropriate pipe create       */
                                  /* function                                 */
   char     pNameExtension [4];   /* Pipe name extension                      */

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
   pipeCreate = pipeDevCreate;


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
      ERROR_SET (S_wfsLib_ERRLOG_PIPE_FAIL, "Pipe create/open failed", 
                 ERROR_LOG_NOW);
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

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   wfsInitTelName
 *
 *   INVOCATION:
 *   wfsInitTelName (pgensub)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, or ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Init the telescope name from the TCS
 *
 *   DESCRIPTION:
 *   This routine is called ones during the initialization process and inits the
 *   telescope name from the TCS
 *
 *   EXTERNAL VARIABLES:
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *
 *   DEFICIENCIES:
 *
 *   BUGS:
 *-
 */

STATUS   wfsInitTelName (struct genSubRecord *pgensub)
{
 
    strcpy ( tcsTelName , (char *)pgensub->a ) ; 
    if ( (strcmp ( tcsTelName , "Gemini-North" ) != 0) && 
         ( strcmp ( tcsTelName , "Gemini-South" ) != 0) )
#if (MK)
       strcpy ( tcsTelName , "Gemini-North" ) ;
#else
       strcpy ( tcsTelName , "Gemini-South" ) ;
#endif

    return (OK) ;
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   wfsGetTelName
 *
 *   INVOCATION:
 *   wfsGetTelName (char *pTelName)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (<) pTelName (char *) Telescope name
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Get the local copy of the TCS Telescope name
 *
 *   DESCRIPTION:
 *   Update the telescope name with the data obtained from the TCS.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   tcsTelName
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *
 *   DEFICIENCIES:
 *
 *   BUGS:
 *-
 */

void   wfsGetTelName (char *pTelName)
{
    strcpy ( pTelName , tcsTelName ) ;

    return;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   wfsUpdateAg
 *
 *   INVOCATION:
 *   wfsUpdateAg (pgensub)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, or ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Update the global AC Component Controller Data
 *
 *   DESCRIPTION:
 *   This routine is called every 5 second and update the data from the CC
 *
 *   EXTERNAL VARIABLES:
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *
 *   DEFICIENCIES:
 *
 *   BUGS:
 *-
 */

STATUS   wfsUpdateAg (struct genSubRecord *pgensub)
{
   static BOOL agWasConnected = TRUE;
                    /* Flag used to record changes in the AG connection state */
   static BOOL firstTime = TRUE;

   /*
    * Don't do anything the first time this function is called to allow time for
    * the database to settle down and all the connections to be made. This will
    * prevent the output of a "AG database not connected" error followed
    * immediately by a "AG database reconnected" message.
    */

   if ( firstTime )
   {
      firstTime = FALSE;
      return (OK);
   }

   /*
    * If the AG database is not connected it will not be possible to obtain
    * values. Whenever the AG disconnects the genSub record changes its alarm
    * severity to INVALID.
    *
    * This error message can get annoying if it repeats regularly, so only
    * changes in status are recorded.
    */

   if ( pgensub->sevr == INVALID_ALARM )
   {
      if ( agWasConnected )
      {
         /* Commented because of epicsPrint data access error when AG
          * disconnected
          */
         /*ERROR_SET (0, "**** TCS database not connected ****",
                 ERROR_LOG_NOW);*/
         printf ( "**** AG database not connected ****\n" ) ;
         agWasConnected = FALSE;
         return (ERROR);
      }
   }
   else if ( !agWasConnected )
   {
      printf ( "**** AG database reconnected ****");
      agWasConnected = TRUE;
   }

   /*
    * Get the information from the different fields
    */

   strcpy ( acCCId.clFilterName , (char *)pgensub->a ) ; 
   strcpy ( acCCId.ndFilterName , (char *)pgensub->b ) ; 
   strcpy ( acCCId.lensName , (char *)pgensub->c ) ; 
   strcpy ( acCCId.fldStopName , (char *)pgensub->d ) ; 
   strcpy ( acCCId.calName , (char *)pgensub->e ) ; 

   acCCId.focusPos = *((double *)pgensub->f); 

   return (OK) ;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   showAcCCStruct
 *
 *   INVOCATION:
 *   showAcCCStruct ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (void)   
 *
 *   PURPOSE:
 *   Show the contents of the acCCId structure
 *
 *   DESCRIPTION:
 *   Show the contents of the acCCId structure
 *
 *   EXTERNAL VARIABLES:
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *
 *   DEFICIENCIES:
 *
 *   BUGS:
 *-
 */

void   showAcCCStruct ()
{

    printf ( "AC CC INFO:\n" );
    printf ( "===========\n" );
    printf ( "cl filter name: %s\n" , acCCId.clFilterName );
    printf ( "nd filter name: %s\n" , acCCId.ndFilterName );
    printf ( "lens name: %s\n" , acCCId.lensName );
    printf ( "field stop name: %s\n" , acCCId.fldStopName );
    printf ( "cal source name: %s\n" , acCCId.calName );
    printf ( "focus pos: %f mm\n" , (float)(acCCId.focusPos) );
    printf ( "\n" );
}
