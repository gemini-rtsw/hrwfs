/*+
 *	MODULE NAME:
 *	wfsControl
 *
 *	FILENAME:
 *	wfsControl.h
 *
 *	PURPOSE:
 *	Include file for wavefront sensor control application code
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.6  1998/12/07 11:17:27  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.5  1998/10/20 09:29:45  cics
 * test command removed. measuring flag added.
 *
 * Revision 1.4  1998/10/01 13:48:49  cics
 * Minor comment changes
 *
 * Revision 1.3  1998/09/28 08:53:56  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks
 *
 * Revision 1.2  1998/06/30 12:42:11  smb
 * Dependency on sysextLib and mpPipeDrv can be removed using NO_SYSEXTLIB and NO_MPPIPEDRV macros.
 *
 * Revision 1.1  1998/05/08 16:21:06  smb
 * wfsTasks split into wfsControl and wfsResourceMonitor
 *
 * Revision 1.4  1998/02/18 11:16:29  smb
 * Commands brought up to date with ICD 162/163
 *
 * Revision 1.3  1998/02/05 15:34:08  smb
 * Ability to update initialising and observing records added
 *
 * Revision 1.2  1997/12/15 11:58:20  smb
 * Replaced arbitrary error numbers with real ones
 *
 * Revision 1.1.1.1  1997/11/28 11:46:17  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

#ifndef	__INCwfsControlh
#define	__INCwfsControlh


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif	/* vxWorks */

#include "wfsLib.h"
#include "gemTypes.h"
#include "gemModNum.h"


/* defines */

#define	WFS_CONTROL_TASK_NAME		"wfsControl"	/* WFS control task name	*/

#define	WFS_CONTROL_STATE_SIR_NAME	"state"			/* Name of SIR record to	*/
													/* contain WFS controller	*/
													/* state.					*/

#define	WFS_CONTROL_INIT_SIR_NAME	"initialising"	/* Name of SIR record to	*/
													/* contain WFS controller	*/
													/* initialisation state.	*/

#define	WFS_CONTROL_MEAS_SIR_NAME	"measuring"		/* Name of SIR record to	*/
													/* contain WFS controller	*/
													/* wavefront measurement	*/
													/* state.					*/

#define	WFS_CONTROL_HEALTH_NAME		"controlHealth"	/* Name of SIR record to	*/
													/* contain WFS controller	*/
													/* health.					*/

	/*
	 * Error number codes used by wfsControl.
	 * These are designed to be processed using the vxWorks "makeStatTbl" utility.
	 */

#define	S_wfsControl_BAD_COMMAND	(M_wfsControl | 1)	/* Unrecognised command */

/*
 * Define the commands recognised by the wavefront sensor control task.
 */

enum
	{
	WFS_CONTROL_CMD_INIT = 0,		/* Initialise wavefront sensors.	*/
	WFS_CONTROL_CMD_PARK,			/* Park wavefront sensors.			*/
	WFS_CONTROL_CMD_SETROUTER,		/* Select pixel router.				*/
	WFS_CONTROL_CMD_STARTMEASURE,	/* Start wavefront measurement.		*/
	WFS_CONTROL_CMD_STOPMEASURE,	/* Stop wavefront measurement.		*/
	WFS_CONTROL_CMD_GBDOBSERVE,		/* Make wavefront observations.		*/
	WFS_CONTROL_CMD_CALIBRATE,		/* Make wavefront calibrations.		*/
	WFS_CONTROL_CMD_REBOOT,			/* Reboot wavefront sensor.			*/
	WFS_CONTROL_CMD_SIMULATE,		/* Set simulation mode.				*/
	WFS_CONTROL_CMD_DEBUG			/* Set debugging mode.				*/
	};


/* function declarations */

#ifndef	NO_EPICS
IMPORT STATUS	wfsControl (void);
#endif /* NO_EPICS */

#endif /* __INCwfsControlh */
