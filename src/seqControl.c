/*+
 *   MODULE NAME:
 *   seqControl
 *
 *   FILENAME:
 *   seqControl.c
 *
 *   PURPOSE:
 *   Sequencer control task application code
 *
 *   DESCRIPTION:
 *   This file contains the function "seqControl". This function is started with 
 *   the detControl task and handles all the sequencer commands.
 *
 *   INCLUDE FILES:
 *   wfsLib.h
 *   seqControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *   AUTHORS:
 *   Nick Dillon
 *   Steven Beard
 *
 *   HISTORY MODIFICATION:
 *   16 Feb 2001 - cb now stop with dhsExit() when reboot 
 *   18 Jan 2000 - cb new reboot command
 *   19 Jan 2000 - cb new init, test, datum and park commands
 *   20 Jan 2000 - cb new verify, endVerify, guide, endGuide commands
 *   31 Jan 2000 - cb replace wfsControl per seqControl
 *-
 */


/* includes */

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
#include <math.h>
#include <tickLib.h>
#include "car.h"
#include "gemTypes.h"
#include "timeoutLib.h"
#include "errorLib.h"

#include <rebootLib.h>


#include "dhs.h" 

#include "epToVxLib.h"
#include "detControl.h"
#include "wfsLib.h"
#include "seqControl.h"



/***************************************************** External global data ***/

extern BOOL        detDhsConnected;                /* defined in detControl.c */

extern DHS_CONNECT detDhsConnection;               /* defined in detControl.c */

/**** Global variables. These are distinguished with a "seqControl" prefix. ***/

BOOL seqControlStop = FALSE;                   /* Stop sequencer control task */

/* -------------------------------------------------------------------------- */

STATUS   seqControl (void)
{

   /* Variables associated with VxWorks environment. */
   int               taskOptions;       /* VxWorks task options.              */

   STATUS            (* pipeCreate) (); /* Pointer to pipeCreate function.    */

   /* Variables associated with CAD/CAR command protocol. */

   CAD_CMD_CONTEXT   cadCmdContext;     /* Command context.                   */
   int               commandNumber;     /* Command number.                    */
   uint32            errorNumber;       /* Error number.                      */

   /* Variables associated with SIR records. */

   DATREC_CONTEXT    pHistoryLogContext;/* Context for historyLog SIR record. */
   DATREC_CONTEXT    pStateContext;     /* Context for state SIR record.      */
   DATREC_CONTEXT    pInitContext;      /* Context for initialisation state   */
                                        /* SIR record.                        */
   DATREC_CONTEXT    pMeasContext;      /* Context for measuring state SIR    */
                                        /* record.                            */
   DATREC_CONTEXT    pRebootContext;    /* Context for rebooting state SIR    */
                                        /* record                             */
   DATREC_CONTEXT    pParkContext;      /* Context for park state SIR record  */
   DATREC_CONTEXT    pDatumContext;     /* Context for datum state SIR record */
   DATREC_CONTEXT    pTestContext;      /* Context for test state SIR record  */
   DATREC_CONTEXT    pVerifyContext;    /* Context for verify state SIR record*/
   DATREC_CONTEXT    pEndVerifyContext; /* Context for EndVerify state SIR    */
                                        /* record                             */
   DATREC_CONTEXT    pGuideContext;     /* Context for guide state SIR record */
   DATREC_CONTEXT    pEndGuideContext;  /* Context for endGuide state SIR     */
                                        /* record                             */
   DATREC_CONTEXT    pEndObserveContext;/* Context for endObserve state SIR   */
                                        /* record                             */

   /* Data Handling System variables. */

   DHS_STATUS        dhsErrno;          /* DHS error number.                  */

   /* Other general variables. */

   long              initState;         /* Initialisation state.              */
   long              testState;         /* Initialisation state.              */
   long              rebootState;       /* Rebooting state.                   */
   long              parkState;         /* Park state.                        */
   long              datumState;        /* Datum state.                       */
   long              verifyState;       /* Verify state.                      */
   long              endVerifyState;    /* endVerify state.                   */
   long              guideState;        /* guide state.                       */
   long              endGuideState;     /* endGuide state.                    */
   long              endObserveState;   /* endObserve state.                  */

   char              messageLog [EPICS_MAX_BYTES_STRING_ATTRIB + 1];

   /* Check the task executes with floating point co-processor support. */

   if (taskOptionsGet (taskIdSelf (), & taskOptions) == ERROR)
   {
      printErr ( "seqControl: Could not get VxWorks task options\n" );
      return (ERROR);
   }

   if ((taskOptions & VX_FP_TASK) == 0)
   {
      printErr ( "seqControl: Task must be run with VX_FP_TASK option\n" );
      return (ERROR);
   }

   pipeCreate = pipeDevCreate; 

   /*
    * Get the CAD command context structure (using the appropriate pipe driver)
    * which is used subsequently as a handle for the CAD/CAR command-input and
    * response-generation routines.
    */

   if ((cadCmdContext = epToVxCmdInit (NULL, pipeCreate)) == NULL)
   {
      printErr ("seqControl: Error getting CAD command context\n");
      return (ERROR);
   }

   /* Initialise context structures for the SIR records used by this task. */

   if (epToVxRecContextGet (SEQ_CONTROL_HISTORYLOG_SIR_NAME, 
                            & pHistoryLogContext, NULL) == ERROR)
   {
      printErr (
        "seqControl: Can't get SEQ_CONTROL_HISTORYLOG_SIR_NAME SIR context");
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_STATE_SIR_NAME, & pStateContext, NULL) 
       == ERROR)
   {
      strncpy ( messageLog, "Can't get SEQ_CONTROL_STATE_SIR_NAME SIR context",
                EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_INIT_SIR_NAME, & pInitContext, NULL) 
       == ERROR)
   {
      strncpy (messageLog, "Can't get SEQ_CONTROL_INIT_SIR_NAME SIR context",
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_TEST_SIR_NAME, & pTestContext, NULL) 
       == ERROR)
   {
      strncpy (messageLog, "Can't get SEQ_CONTROL_TEST_SIR_NAME SIR context",
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_MEAS_SIR_NAME, & pMeasContext, NULL) 
       == ERROR)
   {
      strncpy (messageLog, "Can't get SEQ_CONTROL_MEAS_SIR_NAME SIR context",
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_REBOOT_SIR_NAME, & pRebootContext, NULL) 
       == ERROR)
   {
      strncpy (messageLog, "Can't get SEQ_CONTROL_REBOOT_SIR_NAME SIR context",
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_PARK_SIR_NAME, & pParkContext, NULL) 
       == ERROR)
   {
      strncpy (messageLog, "Can't get SEQ_CONTROL_PARK SIR context",
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_DATUM_SIR_NAME, & pDatumContext, NULL) 
       == ERROR)
   {
      strncpy (messageLog, "Can't get SEQ_CONTROL_DATUM SIR context",
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_VERIFY_SIR_NAME, & pVerifyContext, NULL) 
       == ERROR)
   {
      strncpy (messageLog, "Can't get SEQ_CONTROL_VERIFY SIR context", 
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_ENDVERIFY_SIR_NAME, 
                            & pEndVerifyContext, NULL) == ERROR)
   {
      strncpy (messageLog, "Can't get SEQ_CONTROL_ENDVERIFY SIR context",
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_GUIDE_SIR_NAME, & pGuideContext, NULL) 
       == ERROR)
   {
      strncpy (messageLog, "Can't get SEQ_CONTROL_GUIDE SIR context",
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_ENDGUIDE_SIR_NAME, 
                            & pEndGuideContext, NULL) == ERROR)
   {
      strncpy (messageLog, "Can't get SEQ_CONTROL_ENDGUIDE SIR context",
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_ENDOBSERVE_SIR_NAME, 
                            & pEndObserveContext, NULL) == ERROR)
   {
      strncpy (messageLog, "Can't get SEQ_CONTROL_ENDOBSERVE SIR context",
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   /* Set the initialisation state to BUSY (if it isn't already set). */

   initState = CAR_BUSY;
   if (epToVxPipeWrite( NULL, (char *) &initState, pInitContext ) == ERROR)
   {
      strncpy (messageLog, "Failed to set initialisation state to BUSY", 
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   /* Initialise the system state to "INITIALIZING". */

   if (epToVxPipeWrite( NULL, "INITIALIZING", pStateContext ) == ERROR)
   {
      strncpy (messageLog, "Failed to set INITIALIZING state", 
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   /* Set the health for this task to "GOOD" */

   if ( epToVxSetHealth( SEQ_CONTROL_HEALTH_SIR_NAME, "GOOD" ) == ERROR )
   {
      strncpy (messageLog, "Failed to initialise SEQ controller health",
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   /*
    * Set the default simulation mode 
    */

   epToVxSetCadSimMode (EPTOVX_SIM_MODE_NONE);


   /* Finally, set the system state to RUNNING. */

   if (epToVxPipeWrite( NULL, "RUNNING", pStateContext ) == ERROR)
   {
      strncpy (messageLog, "Failed to set RUNNING state",
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   /* Set the initialisation state to IDLE. */

   initState = CAR_IDLE;
   if (epToVxPipeWrite( NULL, (char *) &initState, pInitContext ) == ERROR)
   {
      strncpy (messageLog, "Failed to set initialisation state to IDLE",
               EPICS_MAX_BYTES_STRING_ATTRIB);
      printErr ( "seqControl: %s\n", messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      return (ERROR);
   }

   /* The task has been successfully initialised, so it can now go into
    * a loop waiting for commands or error messages from other tasks.
    * The task can be terminated by setting the "seqControlStop"
    * variable from the console.
    */

   strncpy (messageLog, "Entering loop waiting for commands...",
            EPICS_MAX_BYTES_STRING_ATTRIB) ;
   printf ( "seqControl: %s\n" , messageLog ) ;
   epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;

   while (! seqControlStop)
   {

      /*
       * Initialise the error number and then read the command number from the 
       * pipe communicating CAD commands. The epToVxCmdRead() call will block 
       * until a command becomes available. The seqControl task is aborted if 
       * it fails to read a command.
       */

      errorNumber = 0;
      if ((commandNumber = epToVxCmdRead (cadCmdContext)) < 0)
      {
         strncpy (messageLog, "Error reading CAD command - task aborted",
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         printErr ( "seqControl: %s\n" , messageLog ) ;
         epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
         epToVxSetHealth( SEQ_CONTROL_HEALTH_SIR_NAME, "BAD" );
         return (ERROR);
      }

      sprintf (messageLog, "CAD command #%d received", commandNumber);
      printf ( "seqControl: %s\n" , messageLog ) ;
      epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;

      /* Process the command.
       * In simulation mode simply report the command,
       * otherwise switch according to the command number received.
       */

      if (EPTOVX_IS_SIMULATION(cadCmdContext, EPTOVX_SIM_MODE_FULL) ||
          EPTOVX_IS_SIMULATION(cadCmdContext, EPTOVX_SIM_MODE_FAST))
      {
         /* In FULL simulation mode nothing needs to be done except to log
          * a message. The function epToVxCmdFinish() will simulate the
          * response from the command.
          */

         printf ( 
         "seqControl: Command %d received in simulation mode - no action taken",
                  commandNumber);
      }

      else if (commandNumber == SEQ_CONTROL_CMD_INIT)
      {
         /*
          * Init command received.
          * Initialise the system, setting the state to INITIALIZING while 
          * doing so.
          * This command will also have been forwarded to the detector 
          * controllers (by the Capfast code) so simply reset the SEQ 
          * controller health to "GOOD".
          */

         strncpy (messageLog, "Initialise command - resetting health",
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         printf ( "seqControl: %s\n" , messageLog ) ;
         epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;

         if ( epToVxSetHealth( SEQ_CONTROL_HEALTH_SIR_NAME, "GOOD" ) == ERROR )
         {
            strncpy (messageLog , "Error resetting SEQ controller health",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }

         /* Set the initialisation state to BUSY. */

         initState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &initState, pInitContext ) == 
             ERROR)
         {
            strncpy (messageLog, "Failed to set initialisation state to BUSY",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }

         if (epToVxPipeWrite( NULL, "INITIALIZING", pStateContext ) == ERROR)
         {
            strncpy (messageLog, "Failed to set INITIALIZING state",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }

         /*
          * Wait a short time so the changes made to the state are visible.
          */

         taskDelay (2 * sysClkRateGet());

         /* Set the initialisation state back to IDLE. */

         initState = CAR_IDLE;
         if (epToVxPipeWrite( NULL, (char *) &initState, pInitContext ) == 
             ERROR)
         {
            strncpy (messageLog, "Failed to set initialisation state to IDLE",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_TEST)
      {
         /*
          * Test command received.
          * This command will also have been forwarded to the component 
          * controllers (by the Capfast code) 
          */

         strncpy (messageLog, "Test command received",
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         printf ( "seqControl: %s\n" , messageLog ) ;
         epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;

         /* Set the test state to BUSY. */

         testState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &testState, pTestContext ) == 
             ERROR)
         {
            strncpy (messageLog, "Failed to set test state to BUSY",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }

         /*
          * Wait a short time so the changes made to the state are visible.
          */

         taskDelay (2 * sysClkRateGet());

         /* Set the test state back to IDLE. */

         testState = CAR_IDLE;
         if (epToVxPipeWrite( NULL, (char *) &testState, pTestContext ) == 
             ERROR)
         {
            strncpy (messageLog, "Failed to set test state to IDLE",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_PARK)
      {
         /*
          * Park command received. Set the park state to BUSY.
          */

         strncpy (messageLog, "Park command received ",
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         printf ( "seqControl: %s\n" , messageLog ) ;
         epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;

         parkState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &parkState, pParkContext ) == 
             ERROR)
         {
            strncpy (messageLog, "Failed to set park state to BUSY",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }

         /*
          * Wait a short time so the changes made to the state are visible.
          */

         taskDelay (2 * sysClkRateGet());

         /*    
          * Set the park state to IDLE. 
          */

         parkState = CAR_IDLE;
         if (epToVxPipeWrite( NULL, (char *) &parkState, pParkContext ) == 
             ERROR)
         {
            strncpy (messageLog, "Failed to set park state to IDLE",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_DATUM)
      {
         /*
          * Datum command received. Set the datum state to BUSY.
          */

         strncpy (messageLog, "Datum command received ",
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         printf ( "seqControl: %s\n" , messageLog ) ;
         epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;

         datumState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &datumState, pDatumContext ) == 
             ERROR)
         {
            strncpy (messageLog, "Failed to set datum state to BUSY",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }

         /*
          * Wait a short time so the changes made to the state are visible.
          */

         taskDelay (2 * sysClkRateGet());

         /*    
          * Set the datum state to IDLE. 
          */

         datumState = CAR_IDLE;
         if (epToVxPipeWrite( NULL, (char *) &datumState, pDatumContext ) == 
             ERROR)
         {
            strncpy (messageLog, "Failed to set datum state to IDLE",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_VERIFY)
      {
         /*
          * Verify command received. Set the verify state to BUSY.
          */

         strncpy (messageLog, "Verify command received ",
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         printf ( "seqControl: %s\n" , messageLog ) ;
         epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;

         verifyState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &verifyState, pVerifyContext ) == 
             ERROR)
         {
            strncpy (messageLog, "Failed to set verify state to BUSY",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }

         /*
          * Wait a short time so the changes made to the state are visible.
          */

         taskDelay (2 * sysClkRateGet());

         /*    
          * Set the verify state to IDLE. 
          */

         verifyState = CAR_IDLE;
         if (epToVxPipeWrite( NULL, (char *) &verifyState, pVerifyContext ) == 
             ERROR)
         {
            strncpy (messageLog , "Failed to set verify state to IDLE",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_ENDVERIFY)
      {
         /*
          * endVerify command received. Set the endVerify state to BUSY.
          */

         strncpy (messageLog, "EndVerify command received ",
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         printf ( "seqControl: %s\n" , messageLog ) ;
         epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;

         endVerifyState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &endVerifyState, 
                              pEndVerifyContext ) == ERROR)
         {
            strncpy (messageLog, "Failed to set endVerify state to BUSY",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }

         /*
          * Wait a short time so the changes made to the state are visible.
          */

         taskDelay (2 * sysClkRateGet());

         /*    
          * Set the endVerify state to IDLE. 
          */

         endVerifyState = CAR_IDLE;
         if (epToVxPipeWrite( NULL, (char *) &endVerifyState, 
                              pEndVerifyContext ) == ERROR)
         {
            strncpy (messageLog, "Failed to set endVerify state to IDLE",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_GUIDE)
      {
         /*
          * Guide command received. Set the guide state to BUSY.
          */

         strncpy (messageLog, "Guide command received ",
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         printf ( "seqControl: %s\n" , messageLog ) ;
         epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;

         guideState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &guideState, pGuideContext ) == 
             ERROR)
         {
            strncpy (messageLog, "Failed to set guide state to BUSY",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }

         /*
          * Wait a short time so the changes made to the state are visible.
          */

         taskDelay (2 * sysClkRateGet());

         /*    
          * Set the guide state to IDLE. 
          */

         guideState = CAR_IDLE;
         if (epToVxPipeWrite( NULL, (char *) &guideState, pGuideContext ) == 
             ERROR)
         {
            strncpy (messageLog, "Failed to set guide state to IDLE",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_ENDGUIDE)
      {
         /*
          * EndGuide command received. Set the endGuide state to BUSY.
          */

         strncpy (messageLog, "EndGuide command received ",
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         printf ( "seqControl: %s\n" , messageLog ) ;
         epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;

         endGuideState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &endGuideState, 
                              pEndGuideContext ) == ERROR)
         {
            strncpy (messageLog, "Failed to set endGuide state to BUSY",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }

         /*
          * Wait a short time so the changes made to the state are visible.
          */

         taskDelay (2 * sysClkRateGet());

         /*    
          * Set the endGuide state to IDLE. 
          */

         endGuideState = CAR_IDLE;
         if (epToVxPipeWrite( NULL, (char *) &endGuideState, 
                              pEndGuideContext ) == ERROR)
         {
            strncpy (messageLog, "Failed to set endGuide state to IDLE",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_ENDOBSERVE)
      {
         /*
          * EndObserve command received. Set the endObserve state to BUSY.
          */

         strncpy (messageLog, "EndObserve command received ",
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         printf ( "seqControl: %s\n" , messageLog ) ;
         epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;

         endObserveState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &endObserveState, 
                              pEndObserveContext ) == ERROR)
         {
            strncpy (messageLog, "Failed to set endObserve state to BUSY",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }

         /*
          * Wait a short time so the changes made to the state are visible.
          */

         taskDelay (2 * sysClkRateGet());

         /*    
          * Set the endObserve state to IDLE. 
          */

         endObserveState = CAR_IDLE;
         if (epToVxPipeWrite( NULL, (char *) &endObserveState, 
                              pEndObserveContext ) == ERROR)
         {
            strncpy (messageLog, "Failed to set endObserve state to IDLE",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_REBOOT)
      {
         /* Set the reboot state to BUSY. */

         rebootState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &rebootState, pRebootContext ) == 
             ERROR)
         {
            strncpy (messageLog, "Failed to set reboot state to BUSY",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }

         if (epToVxPipeWrite( NULL, "BOOTING", pStateContext ) == ERROR)
         {
            strncpy (messageLog, "Failed to set BOOTING state",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            return (ERROR);
         }

         /*
          * Reboot command received. Close any connection to the DHS and 
          * reset the VME bus.
          */

         if (detDhsInitialised)
         {
            strncpy (messageLog, "Closing down DHS connection.",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printf ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
	
            if ( detDhsConnected == CONNECTED )
            {
               dhsErrno = 0;
               dhsDisconnect (detDhsConnection, &dhsErrno);
               if ( dhsErrno == DHS_S_SUCCESS )
               {
                  detDhsConnected = NOT_CONNECTED;
                  MESSAGE_LOG (MSG_LOG, "Disconnected to DHS");
               }
               else
               {
                  strncpy (messageLog, "dhsDisconnect returns an error",
                  EPICS_MAX_BYTES_STRING_ATTRIB);
                  printErr ( "seqControl: %s\n" , messageLog ) ;
               }
            }

            dhsErrno = 0;
            dhsEventLoopEnd (&dhsErrno);
            dhsExit ( &dhsErrno );
         }

         /*
          * Wait a short time so the changes made to the state are visible.
          */

         taskDelay (2 * sysClkRateGet());

         /*
          * Now reboot
          */

         reboot (BOOT_QUICK_AUTOBOOT);
      }

      else
      {
         sprintf (messageLog, "Command %d not currently implemented",
                  commandNumber);
         printf ( "seqControl: %s\n" , messageLog ) ;
         epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
         errorNumber = S_seqControl_BAD_COMMAND;
      }

      /* Finish the command. */

      if (epToVxCmdFinish (cadCmdContext, errorNumber) == ERROR)
      {
         strncpy (messageLog, "Error finishing command",
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         printErr ( "seqControl: %s\n" , messageLog ) ;
         epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
      }

      /* If the command changed the state, reset it back to "RUNNING". */

      if (commandNumber == SEQ_CONTROL_CMD_INIT)
      {
         if (epToVxPipeWrite( NULL, "RUNNING", pStateContext ) == ERROR)
         {
            strncpy (messageLog, 
                     "Failed to restore RUNNING state after INIT command",
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            printErr ( "seqControl: %s\n" , messageLog ) ;
            epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
            errorNumber = (uint32) errnoGet();
         }
      }
   }

   /*
    * The task has been stopped. Issue a warning message, set the health to 
    * "BAD" and free the resources allocated.
    */

   strncpy (messageLog, "HRWFS/AC sequencer task stopped",
            EPICS_MAX_BYTES_STRING_ATTRIB);
   printErr ( "seqControl: %s\n" , messageLog ) ;
   epToVxPipeWrite (NULL, messageLog, pHistoryLogContext) ;
   epToVxSetHealth( SEQ_CONTROL_HEALTH_SIR_NAME, "BAD" );

   epToVxCmdFree (cadCmdContext);
   /*errorFlush();
   errorFree();*/

   return (OK);
}
