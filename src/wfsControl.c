static struct {void *v; char *c;} rcsid = {&rcsid,
	"$Id: wfsControl.c,v 1.1.1.1 1999-03-17 03:14:25 cboyer Exp $"};

/*+
 *	MODULE NAME:
 *	wfsControl
 *
 *	FILENAME:
 *	wfsControl.c
 *
 *	PURPOSE:
 *	Wavefront sensor control task application code
 *
 *	DESCRIPTION:
 *	This file contains the function "wfsControl", which is executed by the wavefront
 *	sensing control task; together with any private functions used by this task alone.
 *	The "wfsControl" task implements some, but not all, of the systemwide wavefront
 *	sensing control commands. Commands not implemented here (e.g. "test") are implemented
 *	instead using Capfast.
 *
 *	INCLUDE FILES:
 *	wfsLib.h
 *	wfsControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *
 *	DEVELOPMENT NOTE:
 *	Perhaps the "init" command should be implemented in SNL, so the "INITIALIZING" state
 *	can remain until all the WFSs have finished initialising. SMB - 18 Oct 1998.
 *
 *	AUTHORS:
 *	Nick Dillon
 *	Steven Beard
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.13  1998/12/07 11:17:26  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.12  1998/10/21 10:45:34  cics
 * Changed default debug mode to FULL (for as long as we are carrying out engineering tests)
 *
 * Revision 1.11  1998/10/20 09:29:44  cics
 * test command removed. measuring flag added.
 *
 * Revision 1.10  1998/10/13 08:57:46  cics
 * Make a few more constant variables const
 *
 * Revision 1.9  1998/10/08 16:27:35  cics
 * Always use pipeCreate if mpPipeDrv not being used.
 *
 * Revision 1.8  1998/09/28 08:53:55  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks
 *
 * Revision 1.7  1998/09/09 14:35:35  cics
 * Global variables renamed to ensure they are unique
 *
 * Revision 1.6  1998/08/20 14:47:45  smb
 * Changed description of test command
 *
 * Revision 1.5  1998/08/13 09:09:31  smb
 * Added author comment
 *
 * Revision 1.4  1998/07/09 15:28:49  smb
 * sysextProcNumGet replaced with sysProcNumGet. initStat changed from int to long
 *
 * Revision 1.3  1998/06/30 12:42:10  smb
 * Dependency on sysextLib and mpPipeDrv can be removed using NO_SYSEXTLIB and NO_MPPIPEDRV macros.
 *
 * Revision 1.2  1998/05/13 13:13:51  smb
 * Application task variables tidied up and made more consistent
 *
 * Revision 1.1  1998/05/08 16:21:06  smb
 * wfsTasks split into wfsControl and wfsResourceMonitor
 *
 * Revision 1.13  1998/03/27 12:02:26  smb
 * Fixed bug when checking for VX_FP_TASK option
 *
 * Revision 1.12  1998/03/24 16:15:55  smb
 * Simulation mode design flaw fixed
 *
 * Revision 1.11  1998/02/23 13:38:59  smb
 * Rearranged code for printability
 *
 * Revision 1.10  1998/02/18 11:16:28  smb
 * Commands brought up to date with ICD 162/163
 *
 * Revision 1.9  1998/02/05 15:34:08  smb
 * Ability to update initialising and observing records added
 *
 * Revision 1.8  1998/02/02 17:26:09  smb
 * Free resources if task stopped
 *
 * Revision 1.7  1998/01/30 15:30:16  smb
 * Fixed some problems uncovered by prolint
 *
 * Revision 1.6  1998/01/28 10:12:32  smb
 * Health record changed from wfsHealth to controlHealth
 *
 * Revision 1.5  1998/01/21 10:47:10  smb
 * Message logging added
 *
 * Revision 1.4  1998/01/19 16:18:22  smb
 * Update individual health records
 *
 * Revision 1.3  1997/12/15 11:58:19  smb
 * Replaced arbitrary error numbers with real ones
 *
 * Revision 1.2  1997/12/11 14:54:06  smb
 * Fixed typing mistake
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

#ifndef NO_SYSEXTLIB								/* Define this macro to remove sysextLib */
#include "sysextLib.h"
#else
#include <rebootLib.h>
#endif	/* NO_SYSEXTLIB */

#ifndef NO_MPPIPEDRV								/* Define this macro to remove mpPipeDrv */
#include "mpPipeDrv.h"
#endif	/* NO_MPPIPEDRV */

#include "dhs.h"									/* Include Data Handling System constants. */

#include "epToVxLib.h"
#include "detControl.h"
#include "wfsLib.h"
#include "wfsControl.h"

#include "wfsDb.h"


/* defines */

/* global variables. These are distinguished with a "wfsControl" prefix. */

BOOL			wfsControlStop = FALSE;				/* Stop WFS control task.			*/


#ifndef NO_EPICS	/* START OF CODE COMPILED ONLY FOR THE EPICS ENVIRONMENT */

/* ------------------------------------------------------------------------------------------------ */

STATUS	wfsControl (void)
{

	/* Variables associated with VxWorks environment. */
#ifndef NO_MPPIPEDRV
	int					procNumber;			/* Processor number.							*/
#endif /* NO_MPPIPEDRV */
	int					taskOptions;		/* VxWorks task options.						*/

	STATUS				(* pipeCreate) ();	/* Pointer to pipeCreate function.				*/

	/* Variables associated with CAD/CAR command protocol. */

	CAD_CMD_CONTEXT		cadCmdContext;		/* Command context.								*/
	int					commandNumber;		/* Command number.								*/
	uint32				errorNumber;		/* Error number.								*/

	/* Variables associated with SIR records. */

	DATREC_CONTEXT		pStateContext;		/* Context for state SIR record.				*/
	DATREC_CONTEXT		pInitContext;		/* Context for initialisation state SIR record.	*/
	DATREC_CONTEXT		pMeasContext;		/* Context for measuring state SIR record.		*/

	/* Data Handling System variables. */

	DHS_STATUS			dhsErrno;			/* DHS error number.							*/

	/* Other general variables. */

	long				simMode;			/* Code for simulation mode.					*/
	char				pSimMode [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
											/* Simulation mode string.						*/

	long				debugMode;			/* Code for debug mode.							*/
	char				pDebugMode [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
											/* Debug mode string.							*/

	long				initState;			/* Initialisation state.						*/
	long				measState;			/* Measuring state.								*/


	/* Create and initialise an error context structure for this task */

	if (errorInit () == ERROR)
	{
		printErr( "wfsControl: Failed to initialise error context structure.\n" );
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

#ifndef NO_MPPIPEDRV

	/* Obtain the CPU processor number for the task.
	 * A value of -1 indicates the CPU processor number is undefined.
	 * If the processor number is 0, the task resides on the MV167, so the
	 * standard VxWorks pipe driver, pipeDrv, is needed to communicate with it.
	 * Any other value means the task resides on a separate CPU, so the
	 * multi-processor VxWorks pipe driver, mpPipeDrv, is needed to
	 * communicate with it.
	 */

	procNumber = sysProcNumGet ();

	if ( procNumber == -1)
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
	pipeCreate = pipeDevCreate;			/* Without MPIPEDRV always use conventional pipe driver.	*/

#endif	/* NO_MPPIPEDRV */

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

	if (epToVxRecContextGet (WFS_CONTROL_STATE_SIR_NAME, & pStateContext, NULL) == ERROR)
	{
		ERROR_LOG ("Can't get WFS_CONTROL_STATE_SIR_NAME SIR context");
		return (ERROR);
	}

	if (epToVxRecContextGet (WFS_CONTROL_INIT_SIR_NAME, & pInitContext, NULL) == ERROR)
	{
		ERROR_LOG ("Can't get WFS_CONTROL_INIT_SIR_NAME SIR context");
		return (ERROR);
	}

	if (epToVxRecContextGet (WFS_CONTROL_MEAS_SIR_NAME, & pMeasContext, NULL) == ERROR)
	{
		ERROR_LOG ("Can't get WFS_CONTROL_MEAS_SIR_NAME SIR context");
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

	if ( epToVxSetHealth( WFS_CONTROL_HEALTH_NAME, "GOOD" ) == ERROR )
	{
		ERROR_LOG ("Failed to initialise WFS controller health");
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
	 * The task can be terminated by setting the "wfsControlStop"
	 * variable from the console.
	 */

	MESSAGE_LOG1 (MSG_MINDEBUG, "Entering loop waiting for commands... pCmdPacket=0x%x",
			      (int) cadCmdContext->pCmdPacket);

	while (! wfsControlStop)
	{

		/*
		 * Initialise the error number and then read the command number from the pipe
		 * communicating CAD commands. The epToVxCmdRead() call will block until a
		 * command becomes available. The wfsControl task is aborted if it fails to
		 * read a command.
		 */

		errorNumber = 0;
		if ((commandNumber = epToVxCmdRead (cadCmdContext)) < 0)
		{
			ERROR_LOG ("Error reading CAD command - task aborted");
			epToVxSetHealth( WFS_CONTROL_HEALTH_NAME, "BAD" );
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

			MESSAGE_LOG1 (MSG_LOG, "Command %d received in simulation mode... no action taken",
				          commandNumber);
		}

		else if (commandNumber == WFS_CONTROL_CMD_INIT)
		{
			/*
			 * Init command received.
			 * Initialise the system, setting the state to INITIALIZING while doing so.
			 * This command will also have been forwarded to the detector controllers
			 * (by the Capfast code) so simply reset the WFS controller health to "GOOD".
			 */

			MESSAGE_LOG (MSG_LOG, "Initialise command - resetting health");

			/* Set the initialisation state to BUSY. */

			initState = CAR_BUSY;
			if (epToVxPipeWrite( NULL, (char *) &initState, pInitContext ) == ERROR)
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

			/* PUT ANY FURTHER GENERAL INITIALISATION CODE HERE */

			if ( epToVxSetHealth( WFS_CONTROL_HEALTH_NAME, "GOOD" ) == ERROR )
			{
				ERROR_LOG ("Error resetting WFS controller health");
				errorNumber = (uint32) errnoGet();
			}

			/* Set the initialisation state back to IDLE. */

			initState = CAR_IDLE;
			if (epToVxPipeWrite( NULL, (char *) &initState, pInitContext ) == ERROR)
			{
				ERROR_LOG ("Failed to set initialisation state to IDLE");
				errorNumber = (uint32) errnoGet();
			}
		}

		else if (commandNumber == WFS_CONTROL_CMD_PARK)
		{
			/*
			 * Park command received.
			 * Prepare for system shutdown (nothing implemented yet).
			 */

			MESSAGE_LOG (MSG_LOG, "Park command - TO BE IMPLEMENTED");
		}

		else if (commandNumber == WFS_CONTROL_CMD_SETROUTER)
		{
			/*
			 * Set router command received.
			 */

			MESSAGE_LOG (MSG_LOG, "Set router command - TO BE IMPLEMENTED");
		}

		else if (commandNumber == WFS_CONTROL_CMD_STARTMEASURE)
		{
			/*
			 * Start measurement command received.
			 */

			MESSAGE_LOG (MSG_LOG, "Start measurement command - TO BE IMPLEMENTED");

			/*
			 * This command needs to be implemented. For now just set the wavefront
			 * measuring state to TRUE.
			 */

			measState = TRUE;
			if (epToVxPipeWrite( NULL, (char *) &measState, pMeasContext ) == ERROR)
			{
				ERROR_LOG ("Failed to set measuring state to TRUE");
				return (ERROR);
			}
		}

		else if (commandNumber == WFS_CONTROL_CMD_STOPMEASURE)
		{
			/*
			 * Stop measurement command received.
			 */

			MESSAGE_LOG (MSG_LOG, "Stop measurement command - TO BE IMPLEMENTED");

			/*
			 * This command needs to be implemented. For now just set the wavefront
			 * measuring state to FALSE.
			 */

			measState = FALSE;
			if (epToVxPipeWrite( NULL, (char *) &measState, pMeasContext ) == ERROR)
			{
				ERROR_LOG ("Failed to set measuring state to FALSE");
				return (ERROR);
			}
		}

		else if (commandNumber == WFS_CONTROL_CMD_GBDOBSERVE)
		{
			/*
			 * Start one-off or continuous observation command received.
			 */

			MESSAGE_LOG (MSG_LOG,
			             "Start one-off or continuous observation command - TO BE IMPLEMENTED");
		}

		else if (commandNumber == WFS_CONTROL_CMD_CALIBRATE)
		{
			/*
			 * Start calibration observation command received.
			 */

			MESSAGE_LOG (MSG_LOG, "Start calibration observation command - TO BE IMPLEMENTED");
		}

		else if (commandNumber == WFS_CONTROL_CMD_REBOOT)
		{
			/*
			 * Reboot command received. Close any connection to the DHS and reset the VME bus.
			 */

			if ( (detDhsSem != NULL) && (detDhsInitialised) )
			{
				MESSAGE_LOG (MSG_LOG, "Closing down DHS connection.");
				semTake (detDhsSem, WAIT_FOREVER);

				dhsErrno = 0;
				dhsExit ( &dhsErrno );

				semGive (detDhsSem);
			}

			wfsBusReset();

		}

		else if (commandNumber == WFS_CONTROL_CMD_SIMULATE)
		{

			/* Set simulation mode command received.
			 * Set the simulation mode and write its current value to the
			 * SIR record.
			 */

			EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & simMode);
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

		else if (commandNumber == WFS_CONTROL_CMD_DEBUG)
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
			ERROR_SET1 (S_wfsControl_BAD_COMMAND, "Command %d not currently implemented",
			           ERROR_LOG_NOW, commandNumber);
			errorNumber = S_wfsControl_BAD_COMMAND;
		}

		/* Finish the command. */

		if (epToVxCmdFinish (cadCmdContext, errorNumber) == ERROR)
		{
			ERROR_LOG ("Error finishing command");
		}

		/* If the command changed the state, reset it back to "RUNNING". */

		if (commandNumber == WFS_CONTROL_CMD_INIT)
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
	epToVxSetHealth( WFS_CONTROL_HEALTH_NAME, "BAD" );

	epToVxCmdFree (cadCmdContext);
	errorFlush();
	errorFree();

	return (OK);
}

#endif /* NO_EPICS - END OF CODE COMPILED ONLY FOR THE EPICS ENVIRONMENT */
