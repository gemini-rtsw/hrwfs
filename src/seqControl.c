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



/* defines */

/* global variables. These are distinguished with a "seqControl" prefix. */

BOOL seqControlStop = FALSE;          /* Stop sequencer control task.         */


#ifndef NO_EPICS     /* START OF CODE COMPILED ONLY FOR THE EPICS ENVIRONMENT */

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

   long              simMode;           /* Code for simulation mode.          */
   char              pSimMode [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                        /* Simulation mode string.            */

   long              debugMode;         /* Code for debug mode.               */
   char              pDebugMode [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                        /* Debug mode string.                 */

   long              initState;         /* Initialisation state.              */
   long              testState;         /* Initialisation state.              */
   long              measState;         /* Measuring state.                   */
   long              rebootState;       /* Rebooting state.                   */
   long              parkState;         /* Park state.                        */
   long              datumState;        /* Datum state.                       */
   long              verifyState;       /* Verify state.                      */
   long              endVerifyState;    /* endVerify state.                   */
   long              guideState;        /* guide state.                       */
   long              endGuideState;     /* endGuide state.                    */
   long              endObserveState;   /* endObserve state.                  */

   /* Create and initialise an error context structure for this task */

   if (errorInit () == ERROR)
   {
      printErr( "seqControl: Failed to initialise error context structure.\n" );
      return (ERROR);
   }

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

   pipeCreate = pipeDevCreate; 

   /*
    * Get the CAD command context structure (using the appropriate pipe driver)
    * which is used subsequently as a handle for the CAD/CAR command-input and
    * response-generation routines.
    */

   if ((cadCmdContext = epToVxCmdInit (NULL, pipeCreate)) == NULL)
   {
      ERROR_LOG ("Error getting CAD command context");
      return (ERROR);
   }

   /* Initialise context structures for the SIR records used by this task. */

   if (epToVxRecContextGet (SEQ_CONTROL_STATE_SIR_NAME, & pStateContext, NULL) 
       == ERROR)
   {
      ERROR_LOG ("Can't get SEQ_CONTROL_STATE_SIR_NAME SIR context");
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_INIT_SIR_NAME, & pInitContext, NULL) 
       == ERROR)
   {
      ERROR_LOG ("Can't get SEQ_CONTROL_INIT_SIR_NAME SIR context");
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_TEST_SIR_NAME, & pTestContext, NULL) 
       == ERROR)
   {
      ERROR_LOG ("Can't get SEQ_CONTROL_TEST_SIR_NAME SIR context");
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_MEAS_SIR_NAME, & pMeasContext, NULL) 
       == ERROR)
   {
      ERROR_LOG ("Can't get SEQ_CONTROL_MEAS_SIR_NAME SIR context");
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_REBOOT_SIR_NAME, & pRebootContext, NULL) 
       == ERROR)
   {
      ERROR_LOG ("Can't get SEQ_CONTROL_REBOOT_SIR_NAME SIR context");
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_PARK_SIR_NAME, & pParkContext, NULL) 
       == ERROR)
   {
      ERROR_LOG ("Can't get SEQ_CONTROL_PARK SIR context");
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_DATUM_SIR_NAME, & pDatumContext, NULL) 
       == ERROR)
   {
      ERROR_LOG ("Can't get SEQ_CONTROL_DATUM SIR context");
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_VERIFY_SIR_NAME, & pVerifyContext, NULL) 
       == ERROR)
   {
      ERROR_LOG ("Can't get SEQ_CONTROL_VERIFY SIR context");
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_ENDVERIFY_SIR_NAME, & pEndVerifyContext, NULL) 
       == ERROR)
   {
      ERROR_LOG ("Can't get SEQ_CONTROL_ENDVERIFY SIR context");
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_GUIDE_SIR_NAME, & pGuideContext, NULL) 
       == ERROR)
   {
      ERROR_LOG ("Can't get SEQ_CONTROL_GUIDE SIR context");
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_ENDGUIDE_SIR_NAME, & pEndGuideContext, NULL) 
       == ERROR)
   {
      ERROR_LOG ("Can't get SEQ_CONTROL_ENDGUIDE SIR context");
      return (ERROR);
   }

   if (epToVxRecContextGet (SEQ_CONTROL_ENDOBSERVE_SIR_NAME, & pEndObserveContext, NULL) 
       == ERROR)
   {
      ERROR_LOG ("Can't get SEQ_CONTROL_ENDOBSERVE SIR context");
      return (ERROR);
   }

   /* Set the initialisation state to BUSY (if it isn't already set). */

   initState = CAR_BUSY;
   if (epToVxPipeWrite( NULL, (char *) &initState, pInitContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set initialisation state to BUSY");
      return (ERROR);
   }

   /* Initialise the system state to "INITIALIZING". */

   if (epToVxPipeWrite( NULL, "INITIALIZING", pStateContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set INITIALIZING state");
      return (ERROR);
   }

   /* Set the health for this task to "GOOD" */

   if ( epToVxSetHealth( SEQ_CONTROL_HEALTH_SIR_NAME, "GOOD" ) == ERROR )
   {
      ERROR_LOG ("Failed to initialise SEQ controller health");
      return (ERROR);
   }

   /*
    * Set the default simulation mode and debug mode.
    */

   epToVxSetCadSimMode (EPTOVX_SIM_MODE_NONE);
   if (epToVxPipeWrite ("simMode", "NONE", NULL) == ERROR)
   {
      ERROR_LOG ("Failed to write default simulation mode to SIR record");
      return (ERROR);
   }

   errorMessageFilterSet( EPTOVX_DEBUG_MODE_NONE+1 );
   if (epToVxPipeWrite ("debugMode", "NONE", NULL) == ERROR)
   {
      ERROR_LOG ("Failed to write default debug mode to SIR record");
      return (ERROR);
   }

   /* Finally, set the system state to RUNNING. */

   if (epToVxPipeWrite( NULL, "RUNNING", pStateContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set RUNNING state");
      return (ERROR);
   }

   /* Set the initialisation state to IDLE. */

   initState = CAR_IDLE;
   if (epToVxPipeWrite( NULL, (char *) &initState, pInitContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set initialisation state to IDLE");
      return (ERROR);
   }

   /* The task has been successfully initialised, so it can now go into
    * a loop waiting for commands or error messages from other tasks.
    * The task can be terminated by setting the "seqControlStop"
    * variable from the console.
    */

   MESSAGE_LOG1 (MSG_MINDEBUG, 
                 "Entering loop waiting for commands... pCmdPacket=0x%x",
                 (int) cadCmdContext->pCmdPacket);

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
         ERROR_LOG ("Error reading CAD command - task aborted");
         epToVxSetHealth( SEQ_CONTROL_HEALTH_SIR_NAME, "BAD" );
         return (ERROR);
      }
      MESSAGE_LOG1 (MSG_FULLDEBUG, "CAD command #%d received", commandNumber);

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

         MESSAGE_LOG1 (MSG_LOG, 
                 "Command %d received in simulation mode... no action taken",
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

         MESSAGE_LOG (MSG_LOG, "Initialise command - resetting health");

         if ( epToVxSetHealth( SEQ_CONTROL_HEALTH_SIR_NAME, "GOOD" ) == ERROR )
         {
            ERROR_LOG ("Error resetting SEQ controller health");
            errorNumber = (uint32) errnoGet();
         }

         /* Set the initialisation state to BUSY. */

         initState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &initState, pInitContext ) == 
             ERROR)
         {
            ERROR_LOG ("Failed to set initialisation state to BUSY");
            errorNumber = (uint32) errnoGet();
         }

         if (epToVxPipeWrite( NULL, "INITIALIZING", pStateContext ) == ERROR)
         {
            ERROR_LOG ("Failed to set INITIALIZING state");
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
            ERROR_LOG ("Failed to set initialisation state to IDLE");
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

         MESSAGE_LOG (MSG_LOG, "Test command received");

         /* Set the test state to BUSY. */

         testState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &testState, pTestContext ) == 
             ERROR)
         {
            ERROR_LOG ("Failed to set test state to BUSY");
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
            ERROR_LOG ("Failed to set test state to IDLE");
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_PARK)
      {
         /*
          * Park command received. Set the park state to BUSY.
          */

         MESSAGE_LOG (MSG_LOG, "Park command received ");

         parkState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &parkState, pParkContext ) == 
             ERROR)
         {
            ERROR_LOG ("Failed to set park state to BUSY");
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
            ERROR_LOG ("Failed to set park state to IDLE");
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_DATUM)
      {
         /*
          * Datum command received. Set the datum state to BUSY.
          */

         MESSAGE_LOG (MSG_LOG, "Datum command received ");

         datumState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &datumState, pDatumContext ) == 
             ERROR)
         {
            ERROR_LOG ("Failed to set datum state to BUSY");
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
            ERROR_LOG ("Failed to set datum state to IDLE");
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_VERIFY)
      {
         /*
          * Verify command received. Set the verify state to BUSY.
          */

         MESSAGE_LOG (MSG_LOG, "Verify command received ");

         verifyState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &verifyState, pVerifyContext ) == 
             ERROR)
         {
            ERROR_LOG ("Failed to set verify state to BUSY");
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
            ERROR_LOG ("Failed to set verify state to IDLE");
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_ENDVERIFY)
      {
         /*
          * endVerify command received. Set the endVerify state to BUSY.
          */

         MESSAGE_LOG (MSG_LOG, "EndVerify command received ");

         endVerifyState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &endVerifyState, pEndVerifyContext ) == 
             ERROR)
         {
            ERROR_LOG ("Failed to set endVerify state to BUSY");
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
         if (epToVxPipeWrite( NULL, (char *) &endVerifyState, pEndVerifyContext ) == 
             ERROR)
         {
            ERROR_LOG ("Failed to set endVerify state to IDLE");
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_GUIDE)
      {
         /*
          * Guide command received. Set the guide state to BUSY.
          */

         MESSAGE_LOG (MSG_LOG, "Guide command received ");

         guideState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &guideState, pGuideContext ) == 
             ERROR)
         {
            ERROR_LOG ("Failed to set guide state to BUSY");
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
            ERROR_LOG ("Failed to set guide state to IDLE");
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_ENDGUIDE)
      {
         /*
          * EndGuide command received. Set the endGuide state to BUSY.
          */

         MESSAGE_LOG (MSG_LOG, "EndGuide command received ");

         endGuideState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &endGuideState, pEndGuideContext ) == 
             ERROR)
         {
            ERROR_LOG ("Failed to set endGuide state to BUSY");
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
         if (epToVxPipeWrite( NULL, (char *) &endGuideState, pEndGuideContext ) == 
             ERROR)
         {
            ERROR_LOG ("Failed to set endGuide state to IDLE");
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_ENDOBSERVE)
      {
         /*
          * EndObserve command received. Set the endObserve state to BUSY.
          */

         MESSAGE_LOG (MSG_LOG, "EndObserve command received ");

         endObserveState = CAR_BUSY;
         if (epToVxPipeWrite( NULL, (char *) &endObserveState, pEndObserveContext ) == 
             ERROR)
         {
            ERROR_LOG ("Failed to set endObserve state to BUSY");
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
         if (epToVxPipeWrite( NULL, (char *) &endObserveState, pEndObserveContext ) == 
             ERROR)
         {
            ERROR_LOG ("Failed to set endObserve state to IDLE");
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
            ERROR_LOG ("Failed to set reboot state to BUSY");
            errorNumber = (uint32) errnoGet();
         }

         if (epToVxPipeWrite( NULL, "BOOTING", pStateContext ) == ERROR)
         {
            ERROR_LOG ("Failed to set BOOTING state");
            return (ERROR);
         }

         /*
          * Wait a short time so the changes made to the state are visible.
          */

         taskDelay (2 * sysClkRateGet());

         /*
          * Reboot command received. Close any connection to the DHS and 
          * reset the VME bus.
          */

         if ( (detDhsSem != NULL) && (detDhsInitialised) )
         {
            MESSAGE_LOG (MSG_LOG, "Closing down DHS connection.");
            semTake (detDhsSem, WAIT_FOREVER);

            dhsErrno = 0;
            dhsExit ( &dhsErrno );

            semGive (detDhsSem);
         }

         reboot (BOOT_QUICK_AUTOBOOT);
      }

      else if (commandNumber == SEQ_CONTROL_CMD_SIMULATE)
      {

         /* Set simulation mode command received.
          * Set the simulation mode and write its current value to the
          * SIR record.
          */

         EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, 
                                (char *) & simMode);
         epToVxSetCadSimMode (simMode);

         switch (simMode)
         {
            case (EPTOVX_SIM_MODE_VSM):

               strncpy (pSimMode, "VSM", EPICS_MAX_BYTES_STRING_ATTRIB);
               break;

            case (EPTOVX_SIM_MODE_FAST):

               strncpy (pSimMode, "FAST", EPICS_MAX_BYTES_STRING_ATTRIB);
               break;

            case (EPTOVX_SIM_MODE_FULL):

               strncpy (pSimMode, "FULL", EPICS_MAX_BYTES_STRING_ATTRIB);
               break;

            case (EPTOVX_SIM_MODE_NONE):

               strncpy (pSimMode, "NONE", EPICS_MAX_BYTES_STRING_ATTRIB);
               break;

            default:
               strncpy (pSimMode, "INVALID", EPICS_MAX_BYTES_STRING_ATTRIB);
         }

         MESSAGE_LOG1 (MSG_LOG, "Simulation mode set to %s", pSimMode);

         if (epToVxPipeWrite ("simMode", pSimMode, NULL) == ERROR)
         {
            ERROR_LOG ("Failed to write simulation mode to SIR record");
            errorNumber = (uint32) errnoGet();
         }
      }

      else if (commandNumber == SEQ_CONTROL_CMD_DEBUG)
      {
         /* Debug command received.
          * Set the debugging mode.
          */

         EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & debugMode);
         errorMessageFilterSet( debugMode+1 );

         switch (debugMode)
         {
            case (EPTOVX_DEBUG_MODE_NONE):

               strncpy (pDebugMode, "NONE", EPICS_MAX_BYTES_STRING_ATTRIB);
               break;

            case (EPTOVX_DEBUG_MODE_MIN):

               strncpy (pDebugMode, "MIN", EPICS_MAX_BYTES_STRING_ATTRIB);
               break;

            case (EPTOVX_DEBUG_MODE_FULL):

               strncpy (pDebugMode, "FULL", EPICS_MAX_BYTES_STRING_ATTRIB);
               break;

            default:
               strncpy (pDebugMode, "INVALID", EPICS_MAX_BYTES_STRING_ATTRIB);
         }

         MESSAGE_LOG1 (MSG_LOG, "Debug mode set to %s", pDebugMode);

         if (epToVxPipeWrite ("debugMode", pDebugMode, NULL) == ERROR)
         {
            ERROR_LOG ("Failed to write debug mode to SIR record");
            errorNumber = (uint32) errnoGet();
         }
      }

      else
      {
         ERROR_SET1 (S_seqControl_BAD_COMMAND, "Command %d not currently implemented",
                    ERROR_LOG_NOW, commandNumber);
         errorNumber = S_seqControl_BAD_COMMAND;
      }

      /* Finish the command. */

      if (epToVxCmdFinish (cadCmdContext, errorNumber) == ERROR)
      {
         ERROR_LOG ("Error finishing command");
      }

      /* If the command changed the state, reset it back to "RUNNING". */

      if (commandNumber == SEQ_CONTROL_CMD_INIT)
      {
         if (epToVxPipeWrite( NULL, "RUNNING", pStateContext ) == ERROR)
         {
            ERROR_LOG ("Failed to restore RUNNING state after INIT command");
            errorNumber = (uint32) errnoGet();
         }
      }
   }

   /*
    * The task has been stopped. Issue a warning message, set the health to "BAD"
    * and free the resources allocated.
    */

   MESSAGE_LOG (MSG_WARNING, "Wavefront Sensing control task stopped");
   epToVxSetHealth( SEQ_CONTROL_HEALTH_SIR_NAME, "BAD" );

   epToVxCmdFree (cadCmdContext);
   errorFlush();
   errorFree();

   return (OK);
}

#endif /* NO_EPICS - END OF CODE COMPILED ONLY FOR THE EPICS ENVIRONMENT */
