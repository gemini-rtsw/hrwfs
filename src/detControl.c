static struct {void *v; char *c;} rcsid = {&rcsid,
	"$Id: detControl.c,v 1.2 1999-06-04 01:33:16 cboyer Exp $"};

/*+
 *	MODULE NAME:
 *	detControl
 *
 *	FILENAME:
 *	detControl.c
 *
 *	PURPOSE:
 *	Detector controller application code for a wavefront sensor
 *
 *	DESCRIPTION:
 *	This file contains the detector controller application code for a single
 *	wavefront sensor. The code runs in a VxWorks task. A separate copy of the
 *	task needs to be spawned for each wavefront sensor, and can run either on
 *	the MV167 or on a Baja processor.
 *
 *	PRIOR REQUIREMENTS:
 *	The VME network, multi-process pipe driver and EPICS interface should
 *	already have been initialised before the detector control task is spawned.
 *
 *	INCLUDE FILES:
 *	detControl.h
 *	gemTypes.h
 *	wfsLib.h
 *	epToVxLib.h
 *	mpPipeDrv.h
 *	sdsuLib.h
 *	errorLib.h
 *
 *	AUTHORS:
 *	Nick Dillon
 *	Steven Beard
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/03/17 03:14:22  cboyer
 * Initial creation of the Gemini HRWFS repository
 *
 * Revision 1.49  1998/12/07 15:25:20  cics
 * Changed output options in observe command. Fixed some sdsuLib bugs related to continuous observing.
 *
 * Revision 1.48  1998/11/30 15:54:30  cics
 * Modifications made during SMB visit to Hilo, November 1998
 *
 * Revision 1.46  1998/10/23 13:53:58  cics
 * OSP context defined on startup. OSP geometry updated in detGeometry. Still awaiting separation of XSUBAP and XSPOT in ospLib.
 *
 * Revision 1.45  1998/10/22 15:14:12  cics
 * Initialises signal processing parameters on startup. Does not yet update ospGeometry.
 *
 * Revision 1.44  1998/10/21 16:31:16  cics
 * detShow can now show WFSs individually
 *
 * Revision 1.43  1998/10/20 09:32:20  cics
 * testResults and initialising records added.
 *
 * Revision 1.42  1998/10/14 09:56:30  cics
 * HRWFS code added, with HRWFS and OIWFS having the ability to share the same SDSU hardware through a mutex semaphore.
 *
 * Revision 1.41  1998/10/12 11:22:33  cics
 * Updated to keep up with changes to signal processing API
 *
 * Revision 1.40  1998/10/01 13:54:32  cics
 * Signal processor task removed and all signal processing now handled by detControl
 *
 * Revision 1.39  1998/09/28 11:20:17  cics
 * Bug fixes in DHS interface suggested by Jennifer Dunn implemented.
 *
 * Revision 1.38  1998/09/28 08:59:20  cics
 * detGeometry command changed. Modified to keep up to date with sdsuLib.
 *
 * Revision 1.37  1998/09/17 08:30:33  cics
 * Reduced no frames to save memory. Added detDhsCheckErrno. Fixed bugs in detObserveEnd.
 *
 * Revision 1.36  1998/09/09 14:35:23  cics
 * Global variables renamed to ensure they are unique
 *
 * Revision 1.35  1998/08/27 17:29:38  anj
 * Implemented frame & packet sync interrupts and application callbacks.
 *
 * Revision 1.34  1998/08/21 16:15:11  smb
 * Comments added before releasing to Gemini
 *
 * Revision 1.33  1998/08/20 16:01:46  smb
 * Fixed log? Removed old commented out code
 *
 * Revision 1.32  1998/08/20 14:46:53  smb
 * Split detObserve into detObserveStart and detObserveEnd (callback). Added more DHS functions. Updated detSave.
 *
 * Revision 1.31  1998/08/20 14:01:28  anj
 * Changed the comments to detSimulateData to describe its output.
 *
 * Revision 1.30  1998/08/20 12:20:59  anj
 * Modified the frame buffer allocation system.
 * Deleted sdsuSync, some mailbox stuff and other old Nick Dillon code
 * which will not be used.
 *
 * Revision 1.29  1998/07/30 16:51:23  smb
 * Observation ID structure added. Time stamps obtained and written to FITS header.
 *
 * Revision 1.28  1998/07/28 15:54:16  smb
 * Parameters to setGeometry setMode and setTemp modified.
 *
 * Revision 1.27  1998/07/16 16:39:04  smb
 * File paths no longer have to end in slash. Fixed problem with frame buffer pointer not being returned properly from detObserveStart. Download DSP code automatically on startup.
 *
 * Revision 1.26  1998/07/16 11:36:39  smb
 * detInit now returns sdsuId through a pointer.
 *
 * Revision 1.25  1998/07/16 11:02:41  smb
 * Brought up to date with new DSP parameters
 *
 * Revision 1.24  1998/07/16 09:37:58  smb
 * ANJ and SMB changes merged. detInit now returns vmeAddress through a pointer.
 *
 * Revision 1.23  1998/07/15 17:00:20  anj
 * Changed sdsuParamWrite to use WRP when talking to TIM.
 * Added sdsuParamPrint, deleted sdsuPrintParam and sdsuPrintStatus.
 * Changed sdsuGetVersion and getVersion to return uint32.
 * Renamed all references to COFF (wrong) into OMF (correct).
 * Deleted some commands & replies no longer in ICD 1.6/1.10
 *
 * Revision 1.22  1998/07/14 15:51:59  anj
 * With SMB, working!
 *
 * Revision 1.21  1998/07/13 15:17:13  smb
 * HRWFS added. Code for data simulation and saving to disk added. DHS code added (commented out)
 *
 * Revision 1.20  1998/06/30 11:32:10  smb
 * Simulation option in sdsuLib made use of. Dependency on sysextLib and mpPipeDrv can be removed using NO_SYSEXTLIB and NO_MPPIPEDRV macros.
 *
 * Revision 1.19  1998/05/13 13:13:49  smb
 * Application task variables tidied up and made more consistent
 *
 * Revision 1.18  1998/05/12 13:17:09  smb
 * Camera name changed to WFS name
 *
 * Revision 1.17  1998/03/27 12:02:24  smb
 * Fixed bug when checking for VX_FP_TASK option
 *
 * Revision 1.16  1998/03/24 16:15:54  smb
 * Simulation mode design flaw fixed
 *
 * Revision 1.15  1998/02/23 13:38:48  smb
 * Rearranged code for printability
 *
 * Revision 1.14  1998/02/18 11:16:25  smb
 * Commands brought up to date with ICD 162/163
 *
 * Revision 1.13  1998/02/05 15:34:06  smb
 * Ability to update initialising and observing records added
 *
 * Revision 1.12  1998/02/02 17:26:07  smb
 * Free resources if task stopped
 *
 * Revision 1.11  1998/01/30 15:30:07  smb
 * Fixed some problems uncovered by prolint
 *
 * Revision 1.10  1998/01/28 09:53:50  smb
 * Avoid memory corruption by not calling sdsuLib when VME address is 0
 *
 * Revision 1.9  1998/01/21 10:47:07  smb
 * Message logging added
 *
 * Revision 1.8  1998/01/19 16:18:17  smb
 * Update individual health records
 *
 * Revision 1.7  1998/01/16 11:48:45  smb
 * Download all three DSP files on one operation
 *
 * Revision 1.6  1998/01/15 13:45:17  smb
 * Restructured and documented
 *
 * Revision 1.5  1998/01/14 15:50:13  smb
 * Observe command activated - assumes sdsuReadout exists
 *
 * Revision 1.4  1998/01/14 14:37:13  smb
 * Added high level command for downloading COFF file
 *
 * Revision 1.3  1997/12/15 11:58:18  smb
 * Replaced arbitrary error numbers with real ones
 *
 * Revision 1.2  1997/12/11 15:35:02  smb
 * Expand terse error messages - no changes to algorithms
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
#include <stdio.h>
#include <stdlib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <semLib.h>
#include <timers.h>
#include <float.h>
#include <math.h>
#include <selectLib.h>
#include "car.h"

#include "dhs.h"						/* Include Data Handling System constants.				*/

#ifndef NO_SYSEXTLIB					/* Define this macro to remove sysextLib.				*/
#include "sysextLib.h"
#endif	/* NO_SYSEXTLIB */

#ifndef NO_MPPIPEDRV					/* Define this macro to remove mpPipeDrv. 				*/
#include "mpPipeDrv.h"
#endif	/* NO_MPPIPEDRV */

#include "timeLib.h"
#include "slalib.h"
#include "astLib.h"

#ifdef USE_CFITSIO						/* Caused memory corruption problem - reverted to old code.	*/
#include "fitsio.h"						/* cFitsIo library. */
#endif

#include "gemTypes.h"
#include "timeoutLib.h"
#include "epToVxLib.h"
#include "wfsLib.h"
#include "wfsWcs.h"
#include "errorLib.h"
#include "sdsuLib.h"
#include "osp.h"

/*#define DEBUG*/ 				/* Define this macro to enable debug messages.			*/

/* #define SAVE_RAW_DATA */				/* Define this macro to save raw data.					*/

#define SAVE_COADDED_DATA				/* Define this macro to save coadded data.					*/

#include "detControl.h"

#define DHS_WAIT_TIMEOUT	3600		/* Timeout waiting for DHS semaphore (60 seconds)		*/
#define OBS_WAIT_TIMEOUT	1200		/* Timeout waiting for obs sync semaphore (20 seconds)	*/

/* Macro for checking DHS status. */

/*
 * Corinne Boyer had problems and commented out the call to detDhsCheckErrno() 
 * within this macro. I don't know exactly what those problems were.
 * Instead of commenting out the macro I have modified the detDhsCheckErrno()
 * function into something much simpler until the DHS problem can be solved.
 * SMB - 17 Jan 1999.
 * NO IT IS NOT TRUE! I HAVE REPLACED THE FUNCTION detDhsCheckErrno() by something 
 * simpler called detDhsCheckErrnoSimp().
 * CB - 5 Fev 1999
 */

#define	CHECK_DHS(dhsErrno)								\
	detDhsCheckErrno ((dhsErrno),__LINE__, __FILE__)

	/*
	 * Global variables.
	 *
	 * Note that the detControl task is designed to be reentrant, so its use of global
	 * variables must be strictly controlled. All the variables declared here are shared
	 * between *all* the instantiations of the task on a particular CPU.
	 * The variables begin with the unique prefix "det" or "pDet".
	 */

char 	pDetDhsClientName [EPICS_MAX_BYTES_STRING_ATTRIB + 1] = "NONE";
										/* Name of DHS client = Instrument name.	*/
										/* Assumed the same for all WFSs on CPU.	*/

char 	pDetDhsHostName [EPICS_MAX_BYTES_STRING_ATTRIB + 1] = "NONE";
										/* Name of host running DHS data server.	*/
										/* Assumed the same for all WFSs.			*/

char 	pDetDhsServerName [EPICS_MAX_BYTES_STRING_ATTRIB + 1] = "NONE";
										/* Name of DHS data server.					*/
										/* Assumed the same for all WFSs.			*/


BOOL	detDhsInitialised = FALSE;		/* Flag to determine whether the DHS		*/
										/* library has been initialised.			*/
										/* One variable is sufficient because the	*/
										/* library should only be initialised once,	*/
										/* no matter how many wavefront sensors are	*/
										/* being controlled.						*/

SEM_ID	detDhsSem = NULL;				/* Semaphore to control access to DHS.		*/

SEM_ID	detSwitchSem = NULL;			/* Semaphore to control access to the		*/
										/* HRWFS/OIWFS switch.						*/

SDSU_ID	detSdsuIdP1 = NULL;				/* SDSU context structure for PWFS1.		*/
SDSU_ID	detSdsuIdP2 = NULL;				/* SDSU context structure for PWFS2.		*/
SDSU_ID	detSdsuIdOi = NULL;				/* SDSU context structure for OIWFS.		*/
SDSU_ID	detSdsuIdHr = NULL;				/* SDSU context structure for HRWFS.		*/

OBS_ID	detObsIdP1 = NULL;				/* Observation context structure for PWFS1.	*/
OBS_ID	detObsIdP2 = NULL;				/* Observation context structure for PWFS2.	*/
OBS_ID	detObsIdOi = NULL;				/* Observation context structure for OIWFS.	*/
OBS_ID	detObsIdHr = NULL;				/* Observation context structure for HRWFS.	*/

uint32	detControlStop = 0x0;			/* This bit mask provides a way of aborting	*/
										/* the detector control task(s) cleanly.	*/
										/* Each task will keep running until it		*/
										/* sees its own bit in this mask set.		*/

	/* Private functions - one for each command. */

LOCAL uint32	detSetup (const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext,
					int commandNumber, SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32	detChop	(const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32	detExposure (const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32	detObstype (const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32	detSetWcs (const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32	detObserveStart (const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext,
					int commandNumber, SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32	detStop (const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32	detAbort (const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32	detInit (const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID * pSdsuId, OBS_ID obsId, uint32 * pVmeAddress, int * pxMax,
					int * pyMax, int * pxPixels, int * pyPixels, int * pMaxFrames,
				    DATREC_CONTEXT pDetInitContext, DATREC_CONTEXT pDetInitStatusContext,
				    BOOL * pGotControl);
LOCAL uint32	detReset (const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId, BOOL gotControl);
LOCAL uint32	detTest (const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId, DATREC_CONTEXT pTestResultsContext,
					BOOL gotControl);
LOCAL uint32	detGiveUp (const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext,
					int commandNumber, SDSU_ID * pSdsuId, OBS_ID obsId,
					DATREC_CONTEXT pDetInitStatusContext, BOOL * pGotControl);
LOCAL uint32	detSave (const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32	detGeometry (const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId, int * pxPixels, int * pyPixels);
LOCAL uint32	detPrimitive (const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId, DATREC_CONTEXT pDetPrimReplyContext);
LOCAL uint32	detDownload	(const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32	detMode	(const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32	detOffset	(const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32	detTemp	(const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32	detSigInit	(const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32	detSigMode	(const char * pWfsName, const char * pRecordPrefix, CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
					SDSU_ID sdsuId, OBS_ID obsId);

	/* Plus some additional functions. */

STATUS			detDownloadDefault (const char * pWfsName, const char * pRecordPrefix, SDSU_ID sdsuId);
STATUS			detCheckGeometry (const char * pWfsName, SDSU_ID sdsuId, int * pxMax,
					int * pyMax, int * pxPixels, int * pyPixels);
STATUS			detCopyGeometry (const char * pWfsName, SDSU_ID sdsuId,
					struct OSP_GEOMETRY * ospGeometry);
STATUS			detSimulateData (const int xPixels, const int yPixels, const int option,
					SDSU_FRAME *pFrameBuffer);
STATUS			detFrameUnscramble (const int xPixels, const int yPixels, const int outputs,
					SDSU_FRAME * inFrame, float * outBuffer );
STATUS			detFrameScramble (const int xPixels, const int yPixels, const int outputs,
					float * inBuffer, uint16 * outBuffer );
STATUS			detWriteFits (char * filename, OBS_ID obsId, int xPixels, int yPixels,
					float * pFrameBuffer);
STATUS			detWriteFitsUint16 (char * filename, OBS_ID obsId, int xPixels, int yPixels,
					uint16 * pFrameBuffer);

OBS_ID			detObsContextCreate( void );

void			detPacketCallback(SDSU_ID sdsuId, void * obsIdIn, SDSU_FRAME * pFrame );
void			detFrameCallback(SDSU_ID sdsuId, void * obsIdIn, SDSU_FRAME * pFrame );

void			detObserveEnd (SDSU_ID sdsuId, void * obsIdIn, SDSU_FRAME * pFrame );
void			detObserveTimeout (timer_t timeId, int obsIdInt);

STATUS			detDhsConnect (const char * pWfsName, DHS_CONNECT * pDhsConnection);
void			detDhsCheckErrno (const DHS_STATUS dhsErrno, const int line,
					const char * filename);
STATUS			detDhsCheckCmdStatus (const DHS_TAG dhsTag);

void			detPokeObserving (OBS_ID obsId, BOOL newValue);


/* ------------------------------------------------------------------------------------------------ */

STATUS	detControl
	(
	const char *	pWfsName,					/* Name of wavefront sensor ("p1", "p2",	*/
												/* "oi" or "hr") whose SDSU controller is	*/
												/* to be handled by this invocation of		*/
												/* detControl(). detControl() is a			*/
												/* re-entrant program, and a separate copy	*/
												/* can therefore be spawned for each		*/
												/* wavefront sensor.	 					*/
	const char *	pRecordPrefix				/* Record name prefix (normally the same as	*/
												/* pWfsname, but can be different for HRWFS	*/
	)
{
	/* Variables associated with VxWorks environment. */
#ifndef NO_MPPIPEDRV
	int					procNumber;				/* Processor number for this task.			*/
#endif /* NO_MPPIPEDRV */
	int					taskOptions;			/* VxWorks task options.					*/
	STATUS				(* pipeCreate) ();		/* Pointer to appropriate pipeCreate func.	*/

	/* Variables associated with CAD/CAR/genSub command protocol. */

	CAD_CMD_CONTEXT		cadCmdContext;			/* CAD command context structure.			*/
	int					commandNumber;			/* Command number.							*/
	int					updateNumber;			/* Data update ID number.					*/
	uint32				errorNumber;			/* Error number reported by task.			*/

	/* Variables associated with the use of the select() facility. */

	struct fd_set		updateFds;				/* File descr. structure for select().		*/
	int					widthSelect;			/* Number of file descrs. to monitor.		*/


	/* Variables associated with genSub records. */

	GSUB_DATA_CONTEXT	dataUpdateContext;		/* Data update context structure.			*/

	/* Variables associated with SDSU controller SIR records. */

	DATREC_CONTEXT	pDetInitContext;			/* Context structure for initialising		*/
												/* state SIR record.						*/

	DATREC_CONTEXT	pDetInitStatusContext;		/* Context structure for SDSU				*/
												/* initialisation status SIR record.		*/

	DATREC_CONTEXT	pTestResultsContext;		/* Context structure for SDSU test results	*/
												/* SIR record.								*/

	DATREC_CONTEXT	pDetPrimReplyContext;		/* Context structure for SDSU				*/
												/* primitive reply string SIR record.		*/

	DATREC_CONTEXT	pDetObservingContext;		/* Context structure for observing			*/
												/* state SIR record.						*/


	/* Variables associated with the SDSU controller. */

	uint32			vmeAddress = 0;				/* VME address of SDSU controller. (Set to	*/
												/* 0 if the controller is not installed and	*/
												/* is to be simulated).						*/
	BOOL			simulate;					/* TRUE if controller is to be simulated.	*/
	BOOL			gotControl;					/* TRUE if controller has control of its	*/
												/* SDSU controller (which is TRUE for all	*/
												/* WFSs except the HRWFS or OIWFS when the	*/
												/* other controller has access to the fibre	*/
												/* switch.									*/
	BOOL			initFailed = FALSE;			/* Set TRUE if a significant but non fatal	*/
												/* error occurs during initialisation.		*/
												/* (Fatal errors will cause the task to		*/
												/* abort completely).						*/
	BOOL			initWarning = FALSE;		/* Set TRUE if a warning occurs during		*/
												/* initialisation.							*/
	long			initState;					/* Initialisation state.					*/

	SDSU_ID			sdsuId = NULL;				/* SDSU context structure.					*/

	OBS_ID			obsId = NULL;				/* Observation context structure.			*/

	uint32			detControlStopMask;			/* Mask for detecting which detControlStop	*/
												/* bit refers to this detector controller.	*/

        long              offset0;           /* ADC offset for output 0.           */
        long              offset1;           /* ADC offset for output 1.           */
        uint32            tempCode;          /* Target temperature code            */
        uint32            tempCoeff;         /* Coefficient for temperature control*/

	/* Variables used to define the buffer to be used for storing data.	*/

	int				xPixels, yPixels;			/* Current size of data array in pixels.	*/
	int				xMax, yMax;					/* Maximum size of data array in pixels.	*/
	int				maxFrames;					/* Maximum number of frames in data buffer.	*/


	/* Variables associated with the Gemini Data Handling System */

	DHS_CONNECT		dhsConnection = NULL;		/* DHS connection ID for this controller.	*/


	/* Timer variables. */

	timer_t			timeId;						/* Alarm timer ID.							*/

	/* Other general variables. */

	char			pRecordName [EPICS_MAX_BYTES_RECORD_NAME + 1];
												/* String to store record names.			*/


	char 			pStatusString [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
												/* Status string.							*/


	/* Create and initialise an error context structure for this task */

	if (errorInit () == ERROR)
	{
		printErr ("detControl: Failed to initialise error context structure.\n");
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
	 * A value of -1 indicate the CPU processor number is undefined.
	 * If the processor number is 0 the task resides on the MV167, so the
	 * standard VxWorks pipe driver, pipeDrv, is needed to communicate with it.
	 * Any other value means the task resides on a separate CPU, so the
	 * multi-processor VxWorks pipe driver, mpPipeDrv, is needed to
	 * communicate with it.
	 */

	procNumber = sysProcNumGet ();

	if (procNumber == -1)
	{
		ERROR_SET (0, "Error, CPU processor number not set", ERROR_LOG_NOW);
		return (ERROR);
	}
	else if (procNumber == 0)
	{
		pipeCreate = pipeDevCreate;		/* Use conventional pipe driver.	*/
	}
	else
	{
		pipeCreate = mpPipeDevCreate;	/* Use multi-processor pipe driver.	*/
	}
#else

	pipeCreate = pipeDevCreate;			/* Without MPPIPEDRV always use conventional pipe driver. */

#endif	/* NO_MPPIPEDRV */

	/*
	 * Use the wavefront sensor name provided as a function argument to
	 * obtain the VME address of the corresponding SDSU controller, reporting
	 * an error if the wavefront sensor name is not one of the four
	 * recognised strings ("p1", "p2", "oi" or "hr").
	 *
	 * Also initialise the default data frame size for the appropriate wavefront
	 * sensor.
	 */

	if (strcmp (pWfsName, "p1") == 0)
	{
		vmeAddress = DET_CONTROL_PWFS1_SDSU_ADRS_VME;
		detControlStopMask = DET_CONTROL_PWFS1_MASK;
		xMax = DET_CONTROL_PWFS1_XSIZE;
		yMax = DET_CONTROL_PWFS1_YSIZE;
		xPixels = xMax;
		yPixels = yMax;
		maxFrames = DET_CONTROL_PWFS1_MAX_FRAMES;
		gotControl = TRUE;
	}
	else if (strcmp (pWfsName, "p2") == 0)
	{
		vmeAddress = DET_CONTROL_PWFS2_SDSU_ADRS_VME;
		detControlStopMask = DET_CONTROL_PWFS2_MASK;
		xMax = DET_CONTROL_PWFS2_XSIZE;
		yMax = DET_CONTROL_PWFS2_YSIZE;
		xPixels = xMax;
		yPixels = yMax;
		maxFrames = DET_CONTROL_PWFS2_MAX_FRAMES;
		gotControl = TRUE;
	}
	else if (strcmp (pWfsName, "oi") == 0)
	{
		vmeAddress = DET_CONTROL_SWITCH_SDSU_ADRS_VME;
		detControlStopMask = DET_CONTROL_OIWFS_MASK;
		xMax = DET_CONTROL_OIWFS_XSIZE;
		yMax = DET_CONTROL_OIWFS_YSIZE;
		xPixels = xMax;
		yPixels = yMax;
		maxFrames = DET_CONTROL_OIWFS_MAX_FRAMES;

		/* The HRWFS has control over the HRWFS/OIWFS switch on startup. */

		gotControl = FALSE;
	}
	else if (strcmp (pWfsName, "hr") == 0)
	{
		vmeAddress = DET_CONTROL_SWITCH_SDSU_ADRS_VME;
		detControlStopMask = DET_CONTROL_HRWFS_MASK;
		xMax = DET_CONTROL_HRWFS_XSIZE;
		yMax = DET_CONTROL_HRWFS_YSIZE;
		xPixels = xMax;
		yPixels = yMax;
		maxFrames = DET_CONTROL_HRWFS_MAX_FRAMES;

		/*
		 * The HRWFS is responsible for creating the semaphore protecting the fibre switch.
		 * It is assumed the HRWFS has control over the HRWFS/OIWFS switch on startup.
		 */

		detSwitchSem = semMCreate( SEM_Q_FIFO | SEM_DELETE_SAFE );
		if ( (detSwitchSem == NULL) || (semTake (detSwitchSem, NO_WAIT) == ERROR) )
		{
			ERROR_SET( 0, "Failed to create and take detSwitchSem semaphore on startup",
				ERROR_LOG_NOW);
			return (ERROR);
		}
		gotControl = TRUE;
	}
	else
	{
		ERROR_SET1 (S_detControl_BAD_WFS_NAME, "Unrecognised WFS name, %s, given",
		            ERROR_LOG_NOW, pWfsName);
		return (ERROR);
	}

	/*
	 * Initialise the alarm timer.
	 */

	if ( timeoutAlarmInit( &timeId ) == ERROR )
	{
		ERROR_SET (0, "Failed to initialise alarm timer", ERROR_LOG_NOW);
		return (ERROR);
	}

#ifdef DEBUG
	printf ("detControl:%s: Alarm timer initialised. Timer ID = %d\n", pWfsName, (int) timeId);
#endif

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

	/* The number of bits that need to be monitored in the "select" function
	 * (used later) needs to be set to the maximum file descriptor value in
	 * use. Set the initial value to the value of the file descriptor used to
	 * communicate CAD commands plus 1.
	 */

	widthSelect = cadCmdContext->cadPipeFd + 1;

	/*
	 * Get the genSub data update context structure (using the appropriate pipe driver),
	 * which is used subsequently as a handle for the genSub data update routines. Each
	 * time a new largest file descriptor is found, update the "widthSelect" variable
	 * to be used by "select()" later. These genSub records are not used by the HRWFS.
	 */

	if (strcmp (pWfsName, "hr") != 0)
	{

		if ((dataUpdateContext = epToVxUpdateInit (pWfsName, NULL, pipeCreate)) == NULL)
		{
			ERROR_LOG ("Error getting genSub data update context");
			return (ERROR);
		}

		if ((dataUpdateContext->gensubPipeFd + 1) > widthSelect)
		{
			widthSelect = dataUpdateContext->gensubPipeFd + 1;
		}
	}
	else
	{
		dataUpdateContext = NULL;
	}

	/*
	 * Get the context structures for the SIR records that are maintained by this
	 * task. Each SIR is referenced by its name: first get the name of each SIR, then
	 * call epToVxRecContextGet() in order to look-up the context structure that has
	 * previously been assigned to the SIR during initialisation of the local record
	 * data-base. If necessary, the SIR records are loaded with their default values.
	 */

	sprintf (pRecordName, "%s:%s", pRecordPrefix, DET_CONTROL_INIT_SIR_NAME);
	if (epToVxRecContextGet (pRecordName, & pDetInitContext, NULL) == ERROR)
	{
		ERROR_LOG ("Failed to get DET_CONTROL_INIT_SIR_NAME SIR context");
		return (ERROR);
	}

	/* As soon as we have the SIR record context, set the "initialising" flag. */
	initState = CAR_BUSY;
	if (epToVxPipeWrite (NULL, (char *) &initState, pDetInitContext) == ERROR)
	{
		ERROR_LOG ("Failed to set initialisation state to BUSY");
	}

	sprintf (pRecordName, "%s:%s", pRecordPrefix, DET_CONTROL_INIT_STATUS_SIR_NAME);
	if (epToVxRecContextGet (pRecordName, & pDetInitStatusContext, NULL) == ERROR)
	{
		ERROR_LOG ("Failed to get DET_CONTROL_INIT_STATUS SIR context");
		return (ERROR);
	}

	sprintf (pRecordName, "%s:%s", pRecordPrefix, DET_CONTROL_TEST_RESULTS_SIR_NAME);
	if (epToVxRecContextGet (pRecordName, & pTestResultsContext, NULL) == ERROR)
	{
		ERROR_LOG ("Failed to get DET_CONTROL_TEST_RESULTS_SIR_NAME SIR context");
		return (ERROR);
	}
	if (epToVxPipeWrite (NULL, "Not tested", pTestResultsContext) == ERROR)
	{
		ERROR_LOG ("Failed to initialise DET_CONTROL_TEST_RESULTS_SIR_NAME record");
	}

	sprintf (pRecordName, "%s:%s", pRecordPrefix, DET_CONTROL_PRIM_REPLY_SIR_NAME);
	if (epToVxRecContextGet (pRecordName, & pDetPrimReplyContext, NULL) == ERROR)
	{
		ERROR_LOG ("Failed to get DET_CONTROL_PRIM_REPLY_SIR_NAME SIR context");
		return (ERROR);
	}

	sprintf (pRecordName, "%s:%s", pRecordPrefix, DET_CONTROL_OBSERVING_SIR_NAME);
	if (epToVxRecContextGet (pRecordName, & pDetObservingContext, NULL) == ERROR)
	{
		ERROR_LOG ("Failed to get DET_CONTROL_OBSERVING_SIR_NAME SIR context");
		return (ERROR);
	}


	/*
	 * Create an observation context structure.
	 */

	obsId = detObsContextCreate();
	if ( obsId == NULL )
	{
		ERROR_LOG ("Failed to initialise observation context on startup");
		return (ERROR);
	}

	/* Initialise the "observing" flag and number of frames. */

	obsId->observing = FALSE;
	obsId->totalFrames = 1;

	/*
	 * If the WFS has control over the SDSU hardware, attempt to initialise sdsuLib using
	 * the VME address obtained above (which involves establishing communications with the
	 * SDSU hardware), remembering to call sdsuReset() immediately after sdsuContextCreate()
	 * to ensure a "Set Reply Address" command is issued.
	 *
	 * If the VME address is zero this indicates the SDSU controller for this WFS is not
	 * installed and should be simulated.
	 *
	 * A message describing the status of this initialisation is written to the
	 * DET_CONTROL_INIT_STATUS_SIR_NAME record.
	 *
	 * Note: The task does not abort if the SDSU context structure could not be created
	 * because another attempt can be made by issuing the detInit command.
	 */

	if ( gotControl )
	{

		if (vmeAddress == 0)
		{
			simulate = TRUE;
		}
		else
		{
			simulate = FALSE;
		}

		sdsuId = sdsuContextCreate (vmeAddress, simulate);
		if ( (sdsuId == NULL) ||
		     (sdsuReset (sdsuId, SDSU_RESET_VME | SDSU_RESET_CONTROLLER) == ERROR)
		   )
		{

			/*
			 * The SDSU controller could not be initialised. Issue an error message
			 * and also write a message to the DET_CONTROL_INIT_STATUS_SIR_NAME record.
			 */

			ERROR_LOG ("Failed to initialise SDSU controller");
			initFailed = TRUE;

			/*
			 * Note. When epToVxPipeWrite has a NULL record name argument, as it does below,
			 * the record name is extracted from the "pDetInitStatusContext" structure.
			 */

			if (epToVxPipeWrite (NULL, "WARNING: SDSU Not Initialised", pDetInitStatusContext)
			    == ERROR)
			{
				ERROR_LOG ("Also failed to write warning message to SDSU status pipe.");
			}
		}
		else
		{
			if ( simulate )
			{
				sprintf (pStatusString, "SDSU SIMULATED: ID = 0x%-8x", (int) sdsuId);
			}
			else
			{
				sprintf (pStatusString, "SDSU Initialised OK: ID = 0x%-8x", (int) sdsuId);
			}

			MESSAGE_LOG2 (MSG_LOG, "%s: %s", pWfsName, pStatusString);

			if (epToVxPipeWrite (NULL, pStatusString, pDetInitStatusContext) == ERROR)
			{
				ERROR_LOG ("Failed to write initialisation message to SDSU status pipe.");
			}

			/*
			 * Update the global variables used to remember the SDSU and observing contexts, as an
			 * aid to engineering.
			 */

			if (strcmp (pWfsName, "p1") == 0)
			{
				sdsuId->fastCamera = TRUE;
				detSdsuIdP1 = sdsuId;
				detObsIdP1 = obsId;
			}
			else if (strcmp (pWfsName, "p2") == 0)
			{
				sdsuId->fastCamera = TRUE;
				detSdsuIdP2 = sdsuId;
				detObsIdP2 = obsId;
			}
			else if (strcmp (pWfsName, "oi") == 0)
			{
				sdsuId->fastCamera = TRUE;
				detSdsuIdOi = sdsuId;
				detObsIdOi = obsId;
			}
			else if (strcmp (pWfsName, "hr") == 0)
			{
				sdsuId->fastCamera = FALSE;
				detSdsuIdHr = sdsuId;
				detObsIdHr = obsId;
			}
		}

		/*
		 * Download the default OMF code to the SDSU controller automatically on startup.
		 * The health is set to WARNING if this fails
		 */

		if (detDownloadDefault (pWfsName, pRecordPrefix, sdsuId) == ERROR)
		{
			ERROR_LOG ("Failed to download default DSP code on startup");
			initFailed = TRUE;
		}

		/*
		 * Compare the default detector geometry contained in the DSP code with the values
		 * written to xPixels and yPixels from the DET_CONTROL software constants. (The DSP code
	     * describes what the detector controller is capable of and the DET_CONTROL constants
		 * are used to define the default parameter limits of CAD commands). After calling
		 * detCheckGeometry, xMax and yMax should contain the maximum possible data array size,
		 * allowing a data buffer of a suitable size to be allocated.
		 *
		 * IN THE MIDDLE OF FIXING BUG - DETCHECK GEOMETRY SHOULD RETURN XMAX, YMAX AS WELL
		 * AS XPIXELS YPIXELS - KEEP THESE CONCEPTS SEPARATE. Check detControl.c_13Nov98_NEW
		 */

		if (detCheckGeometry (pWfsName, sdsuId, &xMax, &yMax, &xPixels, &yPixels) == ERROR)
		{
			ERROR_LOG ("Error while checking default detector geometry on startup");
			initFailed = TRUE;
		}

		/*
		 * Create data buffer to hold several frames of data, using the xPixels and yPixels
		 * determined above.
		 */

		if (sdsuBufferCreate (sdsuId, (xMax * yMax), maxFrames) == ERROR)
		{
			ERROR_LOG ("Failed to create data buffer on startup");
			initFailed = TRUE;
		}

		/*
		 * Initialise the readout process with our frame callback.
		 * There is no packet callback in this version of the code.
		 * INTERRUPTS DISABLED. SWITCH TO SIMPLE VERSION.
		 * HRWFS RUNS AT LOWER PRIORITY.
		 */
	
		if (strcmp (pWfsName, "hr") == 0)
		{
			if (sdsuSimpleReadoutOpen (sdsuId, NULL, detObserveEnd, 1, FALSE) == ERROR)
			{
				ERROR_LOG ("Failed to start readout task on startup");
				initFailed = TRUE;
			}
		}
		else
		{
			if (sdsuSimpleReadoutOpen (sdsuId, NULL, detObserveEnd, 0, FALSE) == ERROR)
			{
				ERROR_LOG ("Failed to start readout task on startup");
				initFailed = TRUE;
			}
		}

                /* 
                 * Set the default offsets for the HRWFS CCD sectors
                 */

                if (strcmp (pWfsName, "hr") == 0)
                {
                   if ( sdsuId == NULL )
                   {
                      ERROR_LOG ("Failed to set CCD default offset");
                      initFailed = TRUE;
                   }
                   else
                   {
                     offset0 = 2560 ;
                     offset1 = 2320 ;

                     MESSAGE_LOG2 (MSG_LOG, "Defining new ADC offset levels: %#lx %#lx",
                                   offset0, offset1);

                     if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS0",
                                        (uint32) offset0 ) == ERROR )
                     {
                        ERROR_LOG ("Error setting ADC offset 0 parameter");
                        initFailed = TRUE;
                     }
                     if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS1",
                                        (uint32) offset1 ) == ERROR )
                     {
                        ERROR_LOG ("Error setting ADC offset 1 parameter");
                        initFailed = TRUE;
                     }
                     if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
                     {
                        ERROR_LOG ("Failed to activate TIMING DSP parameters with LDP command");
                        initFailed = TRUE;
                     }
                   }
                } 

                /* 
                 * Set the default temperature for the HRWFS 
                 */

                if (strcmp (pWfsName, "hr") == 0)
                {
                   if ( sdsuId == NULL )
                   {
                      ERROR_LOG ("Failed to set CCD default temperature");
                      initFailed = TRUE;
                   }
                   else
                   {
                      tempCode = (uint32)1282 ;
                      tempCoeff = (uint32)128 ;
                      
                      MESSAGE_LOG2 (MSG_LOG, 
                      "Defining temperature control parameters: %#lx %#lx",
                      tempCode, tempCoeff);

                      if ( (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_CCDT_TGT", tempCode )
                           == ERROR) ||
                           (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_TCF", (uint32) tempCoeff )
                           == ERROR)
                         )
                      {
                        ERROR_LOG ("Error setting temperasture control parameters");
                        initFailed = TRUE;
                      }
                   }
                }

		/*
		 * Create a signal processing geometry structure and initialise it with
		 * the default detector geometry.
		 */

		obsId->ospGeometry = (struct OSP_GEOMETRY *) calloc (1, sizeof(struct OSP_GEOMETRY));
		if ( obsId->ospGeometry == NULL )
		{
			ERROR_SET (0, "Failed to create OSP geometry structure on startup", ERROR_LOG_NOW);
			initFailed = TRUE;
		}

		if ( !sdsuId->simulate )
		{
			if (detCopyGeometry (pWfsName, sdsuId, obsId->ospGeometry) == ERROR)
			{
				ERROR_LOG ("Error while copying default detector geometry on startup");
				initFailed = TRUE;
			}

			if (strcmp (pWfsName, "hr") == 0)
			{
				strcpy (obsId->pWfsName, "HRWFS");
			}
			else
			{
				if (strcmp (pWfsName, "p1") == 0)
				{
					strcpy (obsId->pWfsName, "PWFS1");
				}
				else if (strcmp (pWfsName, "p2") == 0)
				{
					strcpy (obsId->pWfsName, "PWFS2");
				}
				else
				{
					strcpy (obsId->pWfsName, "OIWFS");
				}
			}
		}
		else
		{
			/* In simulation mode use default values for the parameters. */

#ifdef OLD_PARAMETERS
			/* TEMPORARY SET OF PARAMETERS FOR DEVELOPMENT.
			 * CURRENT VERSION OF OSPINIT ASSUMES THESE.
			 * THESE CAN BE REMOVED WHEN OSPINIT USES SEPARATE PARAMETERS
			 * FOR XSUBAP/YSUBAP and XSPOTS/YSPOTS.
			 */

			obsId->ospGeometry->sectors =		4;
			obsId->ospGeometry->xstart =		11;
			obsId->ospGeometry->ystart =		11;
			obsId->ospGeometry->xbin =			1;
			obsId->ospGeometry->ybin =			1;
			obsId->ospGeometry->xraster =		10;
			obsId->ospGeometry->yraster =		10;
			obsId->ospGeometry->xspace =		0;
			obsId->ospGeometry->yspace =		0;
			obsId->ospGeometry->xsubap =		3;
			obsId->ospGeometry->ysubap =		3;
			obsId->ospGeometry->xarraysize =	xPixels;
			obsId->ospGeometry->yarraysize =	yPixels;
			obsId->ospGeometry->framesizeflag =	0;

			printf ("WARNING: Don't forget to run detGeometry with xSubap=3 etc....\n");
#endif

			if (strcmp (pWfsName, "hr") == 0)
			{
				obsId->ospGeometry->sectors =		2;
				obsId->ospGeometry->xstart =		0;
				obsId->ospGeometry->ystart =		0;
				obsId->ospGeometry->xbin =			1;
				obsId->ospGeometry->ybin =			1;
				obsId->ospGeometry->xraster =		xPixels / 2;
				obsId->ospGeometry->yraster =		yPixels;
				obsId->ospGeometry->xspace =		0;
				obsId->ospGeometry->yspace =		0;
				obsId->ospGeometry->xsubap =		1;
				obsId->ospGeometry->ysubap =		1;
				obsId->ospGeometry->xarraysize =	xPixels;
				obsId->ospGeometry->yarraysize =	yPixels;
				obsId->ospGeometry->framesizeflag =	1;

				strcpy (obsId->pWfsName, "HRWFS");
			}
			else
			{
				obsId->ospGeometry->sectors =		4;
				obsId->ospGeometry->xstart =		0;
				obsId->ospGeometry->ystart =		0;
				obsId->ospGeometry->xbin =			1;
				obsId->ospGeometry->ybin =			1;
				obsId->ospGeometry->xraster =		xPixels / 2;
				obsId->ospGeometry->yraster =		yPixels / 2;
				obsId->ospGeometry->xspace =		0;
				obsId->ospGeometry->yspace =		0;
				obsId->ospGeometry->xsubap =		1;
				obsId->ospGeometry->ysubap =		1;
				obsId->ospGeometry->xarraysize =	xPixels;
				obsId->ospGeometry->yarraysize =	yPixels;
				obsId->ospGeometry->framesizeflag =	1;

				if (strcmp (pWfsName, "p1") == 0)
				{
					strcpy (obsId->pWfsName, "PWFS1");
				}
				else if (strcmp (pWfsName, "p2") == 0)
				{
					strcpy (obsId->pWfsName, "PWFS2");
				}
				else
				{
					strcpy (obsId->pWfsName, "OIWFS");
				}
			}
		}
	}
	else
	{
		/*
		 * The detector controller does not currently have access to its hardware
		 * through the fibre switch.
		 */

		obsId->ospGeometry = NULL;
		sdsuId = NULL;
		strncpy (pStatusString, "NO ACCESS THROUGH SDSU H/W SWITCH", EPICS_MAX_BYTES_STRING_ATTRIB);

		MESSAGE_LOG2 (MSG_LOG, "%s: %s", pWfsName, pStatusString);

		if (epToVxPipeWrite (NULL, pStatusString, pDetInitStatusContext) == ERROR)
		{
			ERROR_LOG ("Failed to write initialisation message to SDSU status pipe.");
		}
	}


	/*
	 * Create a signal processing context structure appropriate for the given WFS,
	 * specifying the initial geometry, unless the initialisation file for that WFS
	 * is defined to be "NONE".
	 */

	if (strcmp (pWfsName, "p1") == 0)
	{
		if ( strcmp (DET_CONTROL_PWFS1_OSPFGINI_FILE, "NONE") != 0 )
		{
			obsId->ospFGContext = ospInit (DET_CONTROL_PAR_FILE_PATH "/" DET_CONTROL_PWFS1_OSPFGINI_FILE,
			                               obsId->ospGeometry);
			if (obsId->ospFGContext == NULL)
			{
				ERROR_LOG ("Failed to initialise FG signal processing context on startup");
				return (ERROR);
			}
		}
		else
		{
			MESSAGE_LOG (MSG_LOG, "PWFS1 - FG signal processing not initialised");
		}

		if ( strcmp (DET_CONTROL_PWFS1_OSPAOINI_FILE, "NONE") != 0 )
		{
			obsId->ospAOContext = ospInit (DET_CONTROL_PAR_FILE_PATH "/" DET_CONTROL_PWFS1_OSPAOINI_FILE,
			                               obsId->ospGeometry);
			if (obsId->ospAOContext == NULL)
			{
				ERROR_LOG ("Failed to initialise AO signal processing context on startup");
				return (ERROR);
			}

			/*
			 * Initialise the number of coadds and coadd timeout with default values.
			 * NOTE: These should be incorporated into the ospContext structure.
			 */

			obsId->nCoaddFrames = 10;
			obsId->coaddTimeout = 60.0;
		}
		else
		{
			MESSAGE_LOG (MSG_LOG, "PWFS1 - AO signal processing not initialised");
		}
	}
	else if (strcmp (pWfsName, "p2") == 0)
	{
		if ( strcmp (DET_CONTROL_PWFS2_OSPFGINI_FILE, "NONE") != 0 )
		{
			obsId->ospFGContext = ospInit (DET_CONTROL_PAR_FILE_PATH "/" DET_CONTROL_PWFS2_OSPFGINI_FILE,
			                               obsId->ospGeometry);
			if (obsId->ospFGContext == NULL)
			{
				ERROR_LOG ("Failed to initialise FG signal processing context on startup");
				return (ERROR);
			}
		}
		else
		{
			MESSAGE_LOG (MSG_LOG, "PWFS2 - FG signal processing not initialised");
		}

		if ( strcmp (DET_CONTROL_PWFS2_OSPAOINI_FILE, "NONE") != 0 )
		{
			obsId->ospAOContext = ospInit (DET_CONTROL_PAR_FILE_PATH "/" DET_CONTROL_PWFS2_OSPAOINI_FILE,
			                               obsId->ospGeometry);
			if (obsId->ospAOContext == NULL)
			{
				ERROR_LOG ("Failed to initialise AO signal processing context on startup");
				return (ERROR);
			}
		}
		else
		{
			MESSAGE_LOG (MSG_LOG, "PWFS2 - AO signal processing not initialised");
		}
	}
	else if (strcmp (pWfsName, "oi") == 0)
	{
		if ( strcmp (DET_CONTROL_OIWFS_OSPFGINI_FILE, "NONE") != 0 )
		{
			obsId->ospFGContext = ospInit (DET_CONTROL_PAR_FILE_PATH "/" DET_CONTROL_OIWFS_OSPFGINI_FILE,
			                               obsId->ospGeometry);
			if (obsId->ospFGContext == NULL)
			{
				ERROR_LOG ("Failed to initialise FG signal processing context on startup");
				return (ERROR);
			}
		}
		else
		{
			MESSAGE_LOG (MSG_LOG, "OIWFS - FG signal processing not initialised");
		}

		if ( strcmp (DET_CONTROL_OIWFS_OSPAOINI_FILE, "NONE") != 0 )
		{
			obsId->ospAOContext = ospInit (DET_CONTROL_PAR_FILE_PATH "/" DET_CONTROL_OIWFS_OSPAOINI_FILE,
			                               obsId->ospGeometry);
			if (obsId->ospAOContext == NULL)
			{
				ERROR_LOG ("Failed to initialise AO signal processing context on startup");
				return (ERROR);
			}

			/*
			 * Initialise the number of coadds and coadd timeout with default values.
			 * NOTE: These should be incorporated into the ospContext structure.
			 */

			obsId->nCoaddFrames = 10;
			obsId->coaddTimeout = 60.0;
		}
		else
		{
			MESSAGE_LOG (MSG_LOG, "OIWFS - AO signal processing not initialised");
		}
	}
	else if (strcmp (pWfsName, "hr") == 0)
	{
		/*
		 * This section is probably redundant, since no signal processing is carried out
		 * with the HRWFS and DET_CONTROL_HRWFS_OSPINI_FILE is always set to "NONE" but it
		 * exists just in case. 
		 */

		if ( strcmp (DET_CONTROL_HRWFS_OSPINI_FILE, "NONE") != 0 )
		{
			obsId->ospHRContext = ospInitHr (DET_CONTROL_PAR_FILE_PATH "/" DET_CONTROL_HRWFS_OSPINI_FILE);
			if (obsId->ospHRContext == NULL)
			{
				ERROR_LOG ("Failed to initialise HR signal processing context on startup");
				return (ERROR);
			}

			/*
			 * Initialise the number of coadds and coadd timeout with default values.
			 * NOTE: These should be incorporated into the ospContext structure.
			 */

			obsId->nCoaddFrames = 10;
			obsId->coaddTimeout = 300.0;
		}
		else
		{
			MESSAGE_LOG (MSG_LOG, "HRWFS - signal processing not initialised");
		}
	}


	/*
	 * If the DHS has initialised successfully, attempt to connect to it.
	 */

	if (detDhsInitialised)
	{
		if ( detDhsConnect (pWfsName, &dhsConnection) == ERROR )
		{
			ERROR_LOG ("Failed to connect to DHS");
			initWarning = TRUE;
		}		
	}
	else
	{
		MESSAGE_LOG (MSG_WARNING, "WARNING: DHS not initialised");
	}

	/*
	 * Initialise the health of this detector control task to "GOOD" if successful
	 * or "BAD" if a significant problem occurred during the initialisation.
	 */

	if ( initFailed )
	{
		if ( epToVxSetHealth (pRecordPrefix, "BAD") == ERROR )
		{
			ERROR_LOG ("Failed to initialise detector controller health to BAD");
		}
	}
	else if ( initWarning )
	{
		if ( epToVxSetHealth (pRecordPrefix, "WARNING") == ERROR )
		{
			ERROR_LOG ("Failed to initialise detector controller health to WARNING");
		}
	}
	else
	{
		if ( epToVxSetHealth (pRecordPrefix, "GOOD") == ERROR )
		{
			ERROR_LOG ("Failed to initialise detector controller health to GOOD");
		}
	}

	/* Finally, reset the "initialising" flag. */

	initState = CAR_IDLE;
	if (epToVxPipeWrite (NULL, (char *) &initState, pDetInitContext) == ERROR)
	{
		ERROR_LOG ("Failed to set initialisation state to IDLE");
	}

	/*
	 * The task has been successfully initialised, so it can now go into a loop waiting
	 * for commands. The detector controller task can be terminated by setting the
	 * "detControlStop" variable from the console. The task will stop when it discovers
	 * its bit set. All the tasks can be stopped at once by setting "detControlStop" to 0xf.
	 */

	MESSAGE_LOG3 (MSG_MINDEBUG,
	              "Entering loop waiting for commands... pWfsName=%s, pRecordPrefix=%s pCmdPacket=%#x",
	              pWfsName, pRecordPrefix, (int) cadCmdContext->pCmdPacket);

	while ( (detControlStop & detControlStopMask) == 0 )
	{

		/*
		 * Zero all the bits in the file descriptor read structure and then
		 * set each bit corresponding to the file descriptors of the data
		 * update pipes of all the wavefront sensors being monitored by this
		 * task. Also set the bit corresponding to the pipe used to receive CAD commands.
		 */

		FD_ZERO (& updateFds);

		if ( dataUpdateContext != NULL )
			FD_SET (dataUpdateContext->gensubPipeFd, & updateFds);

		FD_SET (cadCmdContext->cadPipeFd, & updateFds);

		/*
		 * Wait for an input from any of the file descriptors set above.
		 * There is no timeout.
		 */

		if (select (widthSelect, & updateFds, NULL, NULL, NULL) == ERROR)
		{
			ERROR_SET (0, "File descriptor selection function, select(), failed", ERROR_LOG_NOW );
			return (ERROR);
		}

		/* Check whether an input has come from the pipe communicating CAD
		 * commands.
		 */

		if (FD_ISSET (cadCmdContext->cadPipeFd, & updateFds))
		{

			/*
			 * Initialise the error number and then read the command number from the pipe
			 * communicating CAD commands. The epToVxCmdRead() call will block until a
			 * command becomes available. The detControl task is aborted if it fails to
			 * read a command.
			 *
			 */

			errorNumber = 0;
			if ((commandNumber = epToVxCmdRead (cadCmdContext)) < 0)
			{
				ERROR_LOG ("Error reading CAD command - detControl task aborted");
				epToVxSetHealth( pRecordPrefix, "BAD" );
				return (ERROR);
			}

			/* Log a message each time a command is received. */

			MESSAGE_LOG1 (MSG_FULLDEBUG, "CAD command %d received.", commandNumber);

			/* Process the command.
			 * In simulation mode simply report the command,
			 * otherwise switch according to the command number received.
			 */

			if (EPTOVX_IS_SIMULATION (cadCmdContext, EPTOVX_SIM_MODE_FULL) ||
			    EPTOVX_IS_SIMULATION (cadCmdContext, EPTOVX_SIM_MODE_FAST) )
			{
				/*
				 * In simulation mode nothing needs to be done except to log
				 * a message. The function epToVxCmdFinish() will simulate the
				 * response from the command.
				 */

				MESSAGE_LOG1 (MSG_LOG, "Command %d received in simulation mode... no action taken",
					commandNumber);
			}

			else if ( !gotControl && (commandNumber != DET_CONTROL_CMD_INITIALISE) &&
			                         (commandNumber != DET_CONTROL_CMD_TEST) &&
			                         (commandNumber != DET_CONTROL_CMD_RESET)
			        )
			{

				/*
				 * While the controller has no access to the SDSU hardware, the only
				 * acceptible commands are "INITIALISE", "TEST" and "RESET".
				 */

				ERROR_SET (S_detControl_BAD_COMMAND, "No access to SDSU hardware - try INITIALISE",
					ERROR_LOG_NOW);
				errorNumber = S_detControl_BAD_COMMAND;
			}

			else if (commandNumber == DET_CONTROL_CMD_SETUP)
			{
				/* Setup SDSU controller params. */

				errorNumber = detSetup (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
			                            sdsuId, obsId);
			}
			else if (commandNumber == DET_CONTROL_CMD_CHOP)
			{

				/* Specify chop states. */

				errorNumber = detChop (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                       sdsuId, obsId);
			}

			else if (commandNumber == DET_CONTROL_CMD_EXPOSURE)
			{

				/* Define exposure parameters. */

				errorNumber = detExposure (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                           sdsuId, obsId);
			}

			else if (commandNumber == DET_CONTROL_CMD_OBSTYPE)
			{

				/* Define observation type. */

				errorNumber = detObstype(pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                         sdsuId, obsId);
			}

			else if (commandNumber == DET_CONTROL_CMD_SETWCS)
			{

				/* Define World Coordinate System parameters. */

				errorNumber = detSetWcs(pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                         sdsuId, obsId);
			}

			else if (commandNumber == DET_CONTROL_CMD_OBSERVE)
			{

				/*
				 * Make observation.
				 *
				 * Before starting the observation, load up the observation ID structure.
				 */

				obsId->sdsuId = sdsuId;
				obsId->timeId = timeId;
				obsId->pDetObservingContext = pDetObservingContext;
				obsId->xPixels = xPixels;
				obsId->yPixels = yPixels;
				obsId->dhsConnection = dhsConnection;
				obsId->coaddCounter = 0;

				errorNumber = detObserveStart (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
			 	                               sdsuId, obsId);
			}

			else if (commandNumber == DET_CONTROL_CMD_PAUSE)
			{

				 /* Pause observation (not supported by SDSU controller). */

				ERROR_SET (S_detControl_BAD_COMMAND, "Pause observation command not supported",
				           ERROR_LOG_NOW);
				errorNumber = S_detControl_BAD_COMMAND;
			}

			else if (commandNumber == DET_CONTROL_CMD_CONTINUE)
			{

				/* Continue observation (not supported by SDSU controller). */

				ERROR_SET (S_detControl_BAD_COMMAND, "Continue observation command not supported",
				           ERROR_LOG_NOW);
				errorNumber = S_detControl_BAD_COMMAND;
			}

			else if (commandNumber == DET_CONTROL_CMD_STOP)
			{

				/* Stop observation and keep the data. */

				errorNumber = detStop (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                       sdsuId, obsId);
			}

			else if (commandNumber == DET_CONTROL_CMD_ABORT)
			{

				/* Abort observation and throw away the data. */

				errorNumber = detAbort (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                        sdsuId, obsId);
			}

			else if (commandNumber == DET_CONTROL_CMD_INITIALISE)
			{

				/* Initialise SDSU context and redownload DSP code. */

				errorNumber = detInit (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                       &sdsuId, obsId, &vmeAddress, &xMax, &yMax, &xPixels, &yPixels,
				                       &maxFrames, pDetInitContext, pDetInitStatusContext,
				                       &gotControl);
			}

			else if (commandNumber == DET_CONTROL_CMD_RESET)
			{

				/* Reset SDSU controller and redownload DSP code. */

				errorNumber = detReset (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                        sdsuId, obsId, gotControl);
			}

			else if (commandNumber == DET_CONTROL_CMD_TEST)
			{

				/* Test SDSU controller. */

				errorNumber = detTest (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                        sdsuId, obsId, pTestResultsContext, gotControl);
			}

			else if (commandNumber == DET_CONTROL_CMD_GIVEUP)
			{

				/* Give up control of the SDSU hardware (OIWFS/HRWFS only). */

				errorNumber = detGiveUp (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                         &sdsuId, obsId, pDetInitStatusContext, &gotControl);
			}

			else if (commandNumber == DET_CONTROL_CMD_SAVE)
			{

				/* Save SDSU control parameters. */

				errorNumber = detSave (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                       sdsuId, obsId);
			}

			else if (commandNumber == DET_CONTROL_CMD_GEOMETRY)
			{

				/* Set readout geometry. */

				errorNumber = detGeometry (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                           sdsuId, obsId, &xPixels, &yPixels);
			}

			else if (commandNumber == DET_CONTROL_CMD_PRIMITIVE)
			{

				/* Execute SDSU primitive command. */

				errorNumber = detPrimitive (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                            sdsuId, obsId, pDetPrimReplyContext);
			}

			else if (commandNumber == DET_CONTROL_CMD_MODE)
			{

				/* Set SDSU readout mode. */

				errorNumber = detMode (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                       sdsuId, obsId);
			}

			else if (commandNumber == DET_CONTROL_CMD_OFFSET)
			{

				/* Set SDSU ADC offsets. */

				errorNumber = detOffset (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                         sdsuId, obsId);
			}

			else if (commandNumber == DET_CONTROL_CMD_TEMP)
			{

				/* Define SDSU temperature control parameters. */

				errorNumber = detTemp (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                       sdsuId, obsId);
			}

			else if (commandNumber == DET_CONTROL_CMD_SIGINIT)
			{

				/* Initialise signal processing. */

				errorNumber = detSigInit (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                          sdsuId, obsId);
			}
			else if (commandNumber == DET_CONTROL_CMD_SIGMODE)
			{

				/* Define signal processing mode. */

				errorNumber = detSigMode (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
				                          sdsuId, obsId);
			}
			else
			{
				ERROR_SET1 (S_detControl_BAD_COMMAND, "Command %d not currently implemented",
				            ERROR_LOG_NOW, commandNumber);
				errorNumber = S_detControl_BAD_COMMAND;
			}

			/* Finish the command after a 0.5 second delay. */

			taskDelay ( (int) (sysClkRateGet () * 0.5) );
			if (epToVxCmdFinish (cadCmdContext, errorNumber) == ERROR)
			{
				ERROR_LOG ("Error finishing command");
			}
		}

		/*
		 * Check whether an input has come from the pipe communicating
		 * data updates from the genSub records.
		 */

		if ( (dataUpdateContext != NULL) &&
		     (FD_ISSET (dataUpdateContext->gensubPipeFd, & updateFds)) )
		{

			/*
			 * A message has arrived on the genSub data update pipe.
			 * Read the message from the pipe and check it has been read successfully.
			 */

			if ((updateNumber = epToVxUpdateRead (dataUpdateContext)) < 0)
			{
				ERROR_LOG ("Error reading genSub update");
			}

			/* Log a message each time a data update is received. */

			MESSAGE_LOG3 (MSG_FULLDEBUG, "%s - data update %d received. Packet=%#x",
				pWfsName, updateNumber, (int) dataUpdateContext->pUpdatePacket);
		}
	}

	/*
	 * If the task is stopped, free the resources allocated to it.
	 */

	if (sdsuId != NULL) sdsuContextDelete (sdsuId);
	semGive (detSwitchSem);

	MESSAGE_LOG1 (MSG_WARNING, "Detector Control task for WFS %s stopped.", pWfsName);
	epToVxSetHealth( pRecordPrefix, "BAD" );

	epToVxCmdFree (cadCmdContext);
	/* epToVxUpdateFree (dataUpdateContext); */
	errorFlush();
	errorFree();

	return (OK);
}


/* ================================================================================================ */

/*+
 *	FUNCTION NAME:
 *	detSetup
 *
 *	INVOCATION:
 *	detSetup (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext	(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber	(int)				Command number
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(>) obsId			(OBS_ID)			Observation context structure
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detSetup command
 *
 *	DESCRIPTION:
 *	This function sets up the SDSU controller by downloading a set of parameters from a file.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detSetup
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId				/* Observation context structure.			*/
	)
{
	uint32		errorNumber;			/* Error number reported by task.			*/

	long		destId;					/* Destination DSP ID.						*/

	char		pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Path name for file.						*/
	char		pParamFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Name of file of SDSU parameter values.	*/
	char		pFullParamFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
										/* Combined path name and file name.		*/

	/*
	 * Initialise the error number and obtain the attributes provided with the command.
	 */

	errorNumber = 0;
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pFilePath);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pParamFileName);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *) & destId);

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * The command can only be used when an observation is not in progress.
	 */

	if ( obsId->observing )
	{
		ERROR_SET (S_detControl_BUSY,
		           "Observation in progress - abort observation and try again", ERROR_LOG_NOW);
		errorNumber = S_detControl_BUSY;
		return (errorNumber);
	}

	/*
	 * Combine the path and file names together, and append the string ".par" to the file name
	 * if it is not already present. Ignore the file path if not provided.
	 */

	if ( strcmp (pFilePath, "") == 0 )
	{
		strncpy (pFullParamFileName, pParamFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
	}
	else
	{
		sprintf (pFullParamFileName, "%s/%s", pFilePath, pParamFileName );
	}

	if (strstr (pFullParamFileName, ".par") == NULL)
		strncat (pFullParamFileName, ".par", EPICS_MAX_BYTES_STRING_ATTRIB);

	/*
	 * Download SDSU parameters from the specified file to the specified DSP.
	 * (If the DSP is specified as "-1" the parameters will be written to
	 * their corresponding DSP automatically).
	 */

	MESSAGE_LOG1 (MSG_LOG, "Downloading SDSU parameters from %s", pFullParamFileName);

	if ( sdsuParamDnload( sdsuId, pFullParamFileName, (uint32) destId) == ERROR )
	{
		ERROR_LOG ("Failed to download SDSU parameter file");
		errorNumber = S_detControl_SDSU_ERROR;
		return (errorNumber);
	}

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detChop
 *
 *	INVOCATION:
 *	detChop (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext	(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber	(int)				Command number
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(>) obsId			(OBS_ID)			Observation context structure
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detChop command
 *
 *	DESCRIPTION:
 *	This function sets up the SDSU chop parameters.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detChop
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId				/* Observation context structure.			*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.			*/

	long			chopMask;			/* Chop state mask.							*/

	/*
	 * Initialise the error number and obtain the attributes provided with the command.
	 */

	errorNumber = 0;
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & chopMask);

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * The command can only be used when an observation is not in progress.
	 */

	if ( obsId->observing )
	{
		ERROR_SET (S_detControl_BUSY,
		           "Observation in progress - abort observation and try again", ERROR_LOG_NOW);
		errorNumber = S_detControl_BUSY;
		return (errorNumber);
	}

	MESSAGE_LOG (MSG_LOG, "Specify chop states - NOT IMPLEMENTED YET");

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detExposure
 *
 *	INVOCATION:
 *	detExposure (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext	(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber	(int)				Command number
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(>) obsId			(OBS_ID)			Observation context structure
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detExposure command
 *
 *	DESCRIPTION:
 *	This function sets up the SDSU exposure parameters.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detExposure
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId				/* Observation context structure.			*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.			*/

	long			nframe;				/* Number of frames.						*/
	double			exposure;			/* Exposure time in seconds.				*/

	uint32			sdsuNframe;			/* Value for SDSU parameter NFRAME.			*/
	uint32			sdsuTexp;			/* Value for SDSU parameter T_EXP.			*/

	/*
	 * Initialise the error number and obtain the attributes provided with the command.
	 */

	errorNumber = 0;
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & nframe);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & exposure);

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * The command can only be used when an observation is not in progress.
	 */

	if ( obsId->observing )
	{
		ERROR_SET (S_detControl_BUSY,
			"Observation in progress - abort observation and try again", ERROR_LOG_NOW);
		errorNumber = S_detControl_BUSY;
		return (errorNumber);
	}

	/* Check the number of frames is sensible */

	if ( (nframe <= 0) && (nframe != -1) )
	{
		ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, "Invalid number of frames, %ld",
			ERROR_LOG_NOW, nframe);
		errorNumber = S_detControl_BAD_ATTRIBUTE;
		return (errorNumber);
	}

	/*
	 * Check the exposure time is sensible. The SDSU controller measures exposures in units of
	 * 81.92 microseconds and stores the exposure in a 32 bit integer, so the upper limit in
	 * seconds is 2**32 * 0.000008192 = 351,843 seconds
	 */

	if ( (exposure < 0.0) || (exposure > 351843.0) )
	{
		ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, "Invalid exposure time, %f seconds.",
			ERROR_LOG_NOW, exposure);
		errorNumber = S_detControl_BAD_ATTRIBUTE;
		return (errorNumber);
	}

	if ( nframe == -1 )
	{
		MESSAGE_LOG1 (MSG_LOG, "Setting up for an infinite series of exposures of %f seconds each",
			exposure);
		nframe = 0;					/* DSP code assumes 0 means infinite number of frames. */

		/* BUG WORK AROUND: THE SDSU CONTROLLER RETURNS FRAME COUNT=1 WHEN ASKED FOR AN INFINITE
		 * NUMBER OF FRAMES, WHICH DETCONTROL THEN ASSUMES MEANS THE LAST FRAME HAS BEEN RECEIVED.
		 * UNTIL THE SDSU CODE IS FIXED, SET A FLAG TO INDICATE THE FRAME COUNT IS INFINITE.
		 */

		obsId->continuous = TRUE;

	}
	else if ( nframe == 1 )
	{
		MESSAGE_LOG1 (MSG_LOG, "Setting up for one exposure of %f seconds", exposure);

		/* BUG WORK AROUND */
		obsId->continuous = FALSE;
	}
	else
	{
		MESSAGE_LOG2 (MSG_LOG, "Setting up for %ld exposures of %f seconds each", nframe, exposure);

		/* BUG WORK AROUND */
		obsId->continuous = FALSE;
	}

	/* Set the number of frames by writing to the T_NFRAME parameter in the timing DSP */
	/* Also define the total number of frames in the observation context structure. */

	/* BUG WORK AROUND: DRIVE SDSU CONTROLLER IN ONE-SHOT MODE.
	 * SET THE NUMBER OF SDSU FRAMES TO 1 REGARDLESS. SMB - 16 JAN 99.
	 */

	/* sdsuNframe = (uint32) nframe; */
	sdsuNframe = (uint32) 1;

	obsId->totalFrames = nframe;

#ifdef DEBUG
	printf ("detExposure: Setting T_NFRAME parameter to %lu\n", sdsuNframe);
#endif /* DEBUG */

	if ( sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_NFRAME", sdsuNframe ) == ERROR )
	{
		ERROR_LOG ("Error setting number of frames parameter");
		errorNumber = S_detControl_SDSU_ERROR;
	}

	/* Set the exposure time by writing to the T_EXP_TIM parameter in the timing DSP */

	sdsuTexp = (uint32) (exposure / SDSU_EXPOSURE_UNIT);

#ifdef DEBUG
	printf ("detExposure: Setting T_EXP_TIM parameter to %lu\n", sdsuTexp);
#endif /* DEBUG */

	if ( sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_EXP_TIM", sdsuTexp ) == ERROR )
	{
		ERROR_LOG ("Error setting exposure time parameter");
		errorNumber = S_detControl_SDSU_ERROR;
	}

	/* Update the requested total exposure time in the observation context structure. */

	obsId->exposedRQ = nframe * exposure;
	sdsuId->exposureTicks = (int) (exposure * sysClkRateGet());

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detObstype
 *
 *	INVOCATION:
 *	detObstype (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext	(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber	(int)				Command number
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(>) obsId			(OBS_ID)			Observation context structure
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detObstype command
 *
 *	DESCRIPTION:
 *	This function defines the observation type.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detObstype
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId				/* Observation context structure.			*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.			*/

	char			obsType[EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Observation type string. 				*/

	/*
	 * Initialise the error number and obtain the attributes provided with the command.
	 */

	errorNumber = 0;
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, obsType);

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * The command can only be used when an observation is not in progress.
	 */

	if ( obsId->observing )
	{
		ERROR_SET (S_detControl_BUSY,
		           "Observation in progress - abort observation and try again", ERROR_LOG_NOW);
		errorNumber = S_detControl_BUSY;
		return (errorNumber);
	}

	strncpy (obsId->pObsType, obsType, EPICS_MAX_BYTES_STRING_ATTRIB);

	MESSAGE_LOG1 (MSG_LOG, "Observation type defined as %s", obsType);

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detSetWcs
 *
 *	INVOCATION:
 *	detSetWCs (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext	(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber	(int)				Command number
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(>) obsId			(OBS_ID)			Observation context structure
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detSetWcs command
 *
 *	DESCRIPTION:
 *	This function defines the World Coordinate System calibration parameters.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detSetWcs
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId				/* Observation context structure.			*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.			*/

	char		pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Path name for file.						*/
	char		pWcsFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Name of file containing WCS calibration.	*/
	char		pFullWcsFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
										/* Combined path name and file name.		*/

	FILE *		pWcsFile;
	char		pLine[100];				/* Line read from file.						*/
	int			nread;					/* Number of items read from file.			*/
	int			nextChar;				/* Character sensed from next line.			*/
	int			p;						/* Number of points.						*/

	/*
	 * Initialise the error number and obtain the attributes provided with the command.
	 */

	errorNumber = 0;
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pFilePath);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pWcsFileName);

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * The command can only be used when an observation is not in progress.
	 */

	if ( obsId->observing )
	{
		ERROR_SET (S_detControl_BUSY,
		           "Observation in progress - abort observation and try again", ERROR_LOG_NOW);
		errorNumber = S_detControl_BUSY;
		return (errorNumber);
	}

	/*
	 * Combine the path and file names together, and append the string ".wcs" to the file name
	 * if it is not already present. Ignore the file path if not provided.
	 */

	if ( strcmp (pFilePath, "") == 0 )
	{
		strncpy (pFullWcsFileName, pWcsFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
	}
	else
	{
		sprintf (pFullWcsFileName, "%s/%s", pFilePath, pWcsFileName );
	}

	if (strstr (pFullWcsFileName, ".wcs") == NULL)
		strncat (pFullWcsFileName, ".wcs", EPICS_MAX_BYTES_STRING_ATTRIB);

	MESSAGE_LOG1 (MSG_LOG, "Defining WCS parameters from %s", pFullWcsFileName);

	/*
	 * Open the WCS calibration file and read its contents into the pixij and fpxy arrays.
	 */

	if ((pWcsFile = fopen (pFullWcsFileName, "r")) == NULL)
	{
		ERROR_SET1 (0, "Failed to open WCS calibration file, %s", ERROR_LOG_NOW, pFullWcsFileName);
		errorNumber = S_detControl_BAD_FILE;
		return (errorNumber);
	}

	/*
	 * Skip over any comment lines at the beginning of the file denoted by a ";" in column 1.
	 */

	while ( (nextChar = fgetc (pWcsFile)) == ';' )		/* Test the next character for ';'	*/
	{
		ungetc (nextChar, pWcsFile);					/* Undo the effect of fgetc().		*/
		fgets (pLine, 100, pWcsFile);					/* Skip the whole line.				*/
	}
	ungetc (nextChar, pWcsFile);						/* Undo the final fgetc().			*/

	/*
	 * Read through the rest of the file, assuming each line contains
	 * pixel coordinate i, pixel coordinate j, x coordinate, y coordinate.
	 */

	p = 0;
	while ( (p < DET_CONTROL_MAX_WCSPOINTS) &&
	        (nread = fscanf (pWcsFile, "%lf %lf %lf %lf",
	                         &(obsId->pixij[p][0]), &(obsId->pixij[p][1]),
	                         &(obsId->fpxy[p][0]),  &(obsId->fpxy[p][1])
	                        )
	         != EOF)
	      )
	{

		/* Skip blank or unreadable lines. */

		if ( nread > 0 )
		{
#ifdef DEBUG
			printf ("detSetWcs: Point %d: %f %f %f %f\n", p, obsId->pixij[p][0], obsId->pixij[p][1],
				obsId->fpxy[p][0], obsId->fpxy[p][1]);
#endif
			p++;
		}
	}

	obsId->nWcsPoints = p;
	MESSAGE_LOG1 (MSG_MINDEBUG, "%d points read from WCS calibration file.", p);
	if ( (p == DET_CONTROL_MAX_WCSPOINTS) && (nread != EOF) )
	{
		MESSAGE_LOG (MSG_WARNING, "WARNING: Maximum number of points reached before end of file");
	}

	/*
	 * Close the file and tidy up.
	 */
	

	if (fclose (pWcsFile) == ERROR)
	{
		ERROR_SET (0, "Failed to close WCS file", ERROR_LOG_NOW);
		errorNumber = S_detControl_BAD_FILE;
		return (errorNumber);
	}

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detObserveStart
 *
 *	INVOCATION:
 *	detObserveStart (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName				(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext			(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber			(int)				Command number
 *	(>)	sdsuId					(SDSU_ID)			Current SDSU context structure
 *	(!) obsId					(OBS_ID)			Observation context structure
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detObserve command and start observation
 *
 *	DESCRIPTION:
 *	This function starts an observation.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	DHS and WCS code needs tidying up.
 *
 *	BUGS:
 *	dhsErrno keeps having to be reset to DHS_S_SUCCESS. I think this should not be necessary,
 *  and it reveals a bug or bad design feature in the DHS. Resource freeing functions such as
 *	dhsBdDsFree should free their resources regardless of the value of dhsErrno, since they
 *	might be called to tidy up after an error. SMB - 2 November 1998.
 *
 *	I have now replaced all the dhsErrno resets with CHECK_DHS. This should report if the
 *	DHS status is found not to be DHS_S_SUCCESS at any point. SMB - 17 November 1998.
 *
 *	The SDSU controller timing board can appear to hang up if the VME board thinks it is
 *	still waiting to receive data from a previous observation. This may cause the parameter
 *	reads from the timing board to fail before the observation starts. To work around this
 *	problem an "ABT" command is issued to the SDSU VME board before starting the observation.
 *-
 */

uint32 detObserveStart
	(
	const char *	pWfsName,			/* Name of wavefront sensor.						*/
	const char *	pRecordPrefix,		/* Record name prefix.								*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.					*/
	int				commandNumber,		/* Command number.									*/
	SDSU_ID			sdsuId,				/* SDSU context structure.							*/
	OBS_ID			obsId				/* Observation context data structure.				*/
	)
{

	uint32			errorNumber;		/* Error number reported by task.					*/

	/* Variables describing the observation. */

	int				defOutputs;			/* Default number of outputs.						*/

	/* Variables used to specify data label and file names. */

	long			outOptions;			/* Output options (0=none, 1=DHS, 2=file).			*/

	char *			pLabelFromDhs;		/* Data label provided by DHS server.				*/
	char			pDataLabel [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* DHS data label.									*/
	char			pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Path name for files.								*/
	char			pOutFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Output file name (only if DHS not being used).	*/
	char			pSimFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Simulated data file name (only if detector		*/
										/* controller is being simulated).					*/
	char			pFullOutFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
										/* Combined path name and output file name.			*/
	char			pFullSimFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
										/* Combined path name and simulated data file name.	*/

#ifdef SAVE_RAW_DATA
	char			pRawFilename [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
										/* Raw file name (used for engineering tests).		*/
#endif /* SAVE_RAW_DATA */

	/* Other DHS variables (see dhstests.c) */

	DHS_STATUS		dhsErrno;
	char *			axisLabel[2]={"Xaxis","Yaxis"};
	uint32			dims[1];
	uint32			axisSize[2];
	uint32			origin[2];
 	char			*qlStreams[1];
 	char			*contrib[1];

	/* Variables associated with the provision of World Coordinate System information. */

	int				wcsStatus;		/* WCS status.										*/
    double			pixis;			/* x to i scale factor.								*/
    double			pixjs;			/* y to j scale factor.								*/
    double			perp;			/* Non-perpendicularity of i and j axes in radians	*/
    double			orient;			/* Orientation of (i,j) axes with respect to (x,y)	*/
									/* in radians.										*/
	struct WCS_CTX	ctx;			/* World Coordinate System context.					*/
	struct WCS		wcs;			/* Basic TCS World Coordinate System.				*/
	struct WCS		wcsij;			/* Transformed World Coordinate System for IJ.		*/
	double			trackRA;		/* TCS track Right Ascension.						*/
	double			trackDec;		/* TCS track Declination.							*/
	FRAMETYPE		trackFrame;		/* TCS track frame									*/
	struct EPOCH	trackEquinox;	/* TCS track equinox.								*/
	struct EPOCH	trackEpoch;		/* TCS track epoch.									*/
	double			trackWavelength; /* Track wavelength in microns.					*/
	double			timeTAI;		/* International Atomic Time.						*/
    double			rawTimeWcs;		/* Gemini raw time at which WCS info is valid.		*/
    int				chopState;		/* Chop state to which WCS information refers.		*/
	int				p;				/* Point counter.									*/

	/* Variables associated with the frame buffers. */

	int				nPixels;			/* Total number of pixels.						*/

	/* SDSU parameters. */

	uint32			expTim;				/* Exposure time in SDSU units from T_EXPTIM.	*/
	double			readoutTimeout;		/* Readout timeout in seconds.					*/
	double			waitTimeSecs;		/* Wait time in seconds.						*/

	/* 
	 * Variables associated with "observe" command.
	 * (Label, datapath and filename use general filename parameters)
	 */

	long			observingState;		/* Observation status (busy or idle).			*/

	/*
	 * Initialise the error number and DHS error number.
	 */

	errorNumber = 0;
	dhsErrno = DHS_S_SUCCESS;			/* <---- DHS error number is reset here. */

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/* Determine whether a START or STOP directive has been received. */

	if ( EPTOVX_IS_STOP_DIRECTIVE(cadCmdContext) )
	{
		/* Stop directive received - treat as a STOP command and stop the observation. */

		errorNumber = detStop (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId);
	}
	else
	{
		/*
		 * START directive obtained. This directive cannot be used when an
		 * observation is already in progress.
		 */

		if ( obsId->observing )
		{
			ERROR_SET (S_detControl_BUSY, "Observation already in progress", ERROR_LOG_NOW);
			errorNumber = S_detControl_BUSY;
			return (errorNumber);
		}

		/* Obtain the attributes */

		EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pDataLabel);
		EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) &outOptions);
		EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pFilePath);
		EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, pOutFileName);
		EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, pSimFileName);

		/*
		 * Combine the file path and file names, ignoring the path if not specified
		 * and preserving any file name set to "NONE".
		 */

		if ( strcmp (pFilePath, "") == 0 )
		{
			strncpy (pFullOutFileName, pOutFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
			strncpy (pFullSimFileName, pSimFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
		}
		else
		{
			if ( strcmp(pOutFileName, "NONE") == 0 )
			{
				strncpy (pFullOutFileName, pOutFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
			}
			else
			{
				sprintf (pFullOutFileName, "%s/%s", pFilePath, pOutFileName );
			}

			if ( strcmp(pSimFileName, "NONE") == 0 )
			{
				strncpy (pFullSimFileName, pSimFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
			}
			else
			{
				sprintf (pFullSimFileName, "%s/%s", pFilePath, pSimFileName );
			}
		}

		/*
		 * Append the string ".fits" if it is not already present in any file name,
		 * and the name in question is not "NONE".
		 */

		if ((strcmp(pFullOutFileName, "NONE") != 0) && (strstr (pFullOutFileName, ".fits") == NULL))
			strncat (pFullOutFileName, ".fits", EPICS_MAX_BYTES_STRING_ATTRIB);

		if ((strcmp(pFullSimFileName, "NONE") != 0) && (strstr (pFullSimFileName, ".fits") == NULL))
			strncat (pFullSimFileName, ".fits", EPICS_MAX_BYTES_STRING_ATTRIB);

#ifdef SAVE_RAW_DATA
		/*
		 * During engineering tests, make up a raw file name by appending the string .raw
		 */

		sprintf (pRawFilename, "%s%s", pFullOutFileName, ".raw" );
#endif /* SAVE_RAW_DATA */


		/*
		 * If a request has been made to send data to the DHS, check that the DHS is available,
		 * otherwise reject the command.
		 */

		if ( outOptions == 1 )
		{
			if ( ( !detDhsInitialised ) ||
			     ( obsId->dhsConnection == NULL) ||
			     /* ( dhsIsConnected (obsId->dhsConnection, &dhsErrno) != DHS_TRUE ) */
																			/* DOESN'T WORK */
			     ( FALSE )													/* BUG WORK AROUND */
			   )
			{
				ERROR_SET (S_detControl_BAD_ATTRIBUTE, "DHS is not available", ERROR_LOG_NOW);
				errorNumber = S_detControl_BAD_ATTRIBUTE;
				return (errorNumber);
			}
		}

		/*
		 * Set the observation in progress and observation stopped flags, initialise the frame
		 * counter and set the observeC CAR record to BUSY, via the "observing" record.
		 */

		obsId->observing = TRUE;
		obsId->stopped = FALSE;
		obsId->nframes = 0;
		observingState = CAR_BUSY;
		if (epToVxPipeWrite (NULL, (char *) &observingState, obsId->pDetObservingContext) == ERROR)
		{
			ERROR_LOG ("Failed to set observing state to BUSY.");
		}

		/* Ensure whoever is using the system knows when it is in simulation mode. */

		if ( sdsuId->simulate )
		{
			MESSAGE_LOG (MSG_WARNING, "Observation started in SIMULATION MODE");
		}
		else
		{
			MESSAGE_LOG (MSG_MINDEBUG, "Observation started");
		}

		if ( outOptions == 1 )
		{
			if ( (strcmp (pDataLabel,"") != 0) && (strcmp (pDataLabel,"NONE") != 0) )
			{
				MESSAGE_LOG1 (MSG_LOG, "Will send data to DHS with data label provided (%s)",
					pDataLabel);
			}
			else
			{
				pLabelFromDhs = dhsBdName (obsId->dhsConnection, &dhsErrno);
				CHECK_DHS (dhsErrno);

				if ( dhsErrno != DHS_S_SUCCESS )
				{
					MESSAGE_LOG1 (MSG_WARNING,
						"WARNING: Failed to get data label from DHS (dhsErrno=%d) - using NOLABEL",
						dhsErrno);
					strcpy (pDataLabel, "NOLABEL");
				}
				else
				{
					sprintf (pDataLabel, "%s.0.0", pLabelFromDhs);

					MESSAGE_LOG1 (MSG_LOG, "Successfully obtained data label from DHS (%s)",
						pDataLabel);
				}
			}
		}
		else if ( outOptions == 2 )
		{
			MESSAGE_LOG1 (MSG_LOG, "Will save data to directly to file \"%s\"",
				pFullOutFileName);
		}

		/* Load up the observation ID structure with the new information. */

		obsId->outOptions = (int) outOptions;
		strncpy( obsId->pDataLabel, pDataLabel, EPICS_MAX_BYTES_STRING_ATTRIB);
		strncpy( obsId->pOutFileName, pFullOutFileName, EPICS_MAX_BYTES_STRING_ATTRIB*2 );
		strncpy( obsId->pSimFileName, pFullSimFileName, EPICS_MAX_BYTES_STRING_ATTRIB*2 );
#ifdef SAVE_RAW_DATA
		strncpy( obsId->pRawFileName, pRawFilename, EPICS_MAX_BYTES_STRING_ATTRIB*2 );
#endif /* SAVE_RAW_DATA */

		/*
		 * Before starting the observation, query some parameters from the SDSU controller.
		 * Default values for the parameters are assumed in simulation mode or if the parameters
		 * could not be obtained.
		 */

		if ( strcmp( pWfsName, "hr" ) == 0 )
		{
			defOutputs = 2;				/* Default number of outputs.	*/
			readoutTimeout = 20.0;		/* Readout timeout in seconds.	*/
		}
		else
		{
			defOutputs = 4;				/* Default number of outputs.	*/
			readoutTimeout = 2.0;		/* Readout timeout in seconds.	*/
		}

		if ( sdsuId->simulate )
		{
			obsId->outputs = defOutputs;
			obsId->exposed = obsId->exposedRQ;
		}
		else
		{
			/*
			 * BUG WORK AROUND: Before attempting to query parameters from the timing
			 * board, send an ABT command to the VME board. This should ensure the
			 * board is not in a state where it thinks it is still waiting for data from
			 * a previous observation. The parameter reads from the timing board will
			 * fail in this circumstance.
			 */

			if (sdsuPrimitive (sdsuId, "ABT", SDSU_IDENT_VME, NULL, NULL) == ERROR)
			{
				ERROR_LOG ("ABT command failed prior to starting observation");
			}

			if (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_OUTPUTS", &(obsId->outputs)) == ERROR)
			{
				ERROR_LOG ("Failed to query number of outputs from SDSU controller");
				MESSAGE_LOG1 (MSG_WARNING, "Assuming number of outputs is %d", defOutputs);
				obsId->outputs = defOutputs;
			}

		    if (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_EXP_TIM", &expTim) == ERROR)
			{
				ERROR_LOG ("Failed to query exposure time from SDSU controller");
				MESSAGE_LOG (MSG_WARNING, "Assuming exposure time is 1 second per frame");
				if (obsId->totalFrames > 0)
				{
					obsId->exposed = (double) obsId->totalFrames;
				}
				else
				{
					obsId->exposed = 1.0;
				}
			}
		    else if (expTim == 0)
			{
				MESSAGE_LOG (MSG_WARNING,
					"Zero exposure time obtained from SDSU controller. Assuming minimum");
				if (obsId->totalFrames > 0)
				{
					obsId->exposed = (double) obsId->totalFrames * (double) SDSU_EXPOSURE_UNIT;
				}
				else
				{
					obsId->exposed = (double) SDSU_EXPOSURE_UNIT;
				}
			}
			else
			{
				if (obsId->totalFrames > 0)
				{
					obsId->exposed = (double) obsId->totalFrames *
					                 (double) (expTim * SDSU_EXPOSURE_UNIT);
				}
				else
				{
					obsId->exposed = (double) (expTim * SDSU_EXPOSURE_UNIT);
				}
				sdsuId->exposureTicks = (int) (expTim * SDSU_EXPOSURE_UNIT * sysClkRateGet());
			}
		}

		/* Get a timestamp to record the time at which the observation started. */

		if ( timeNow (&(obsId->rawtStart)) != OK )
		{
			ERROR_SET (0, "Failed to get time stamp at observation start", ERROR_LOG_NOW);
		}

#ifdef DEBUG
		printf ("detObserveStart: Time at observation start: %f seconds.\n", obsId->rawtStart);
#endif

		/*
		 * Start the readout process. The observation should now start in a parallel thread.
		 */

		MESSAGE_LOG2 (MSG_MINDEBUG, "Starting exposure of %f seconds in %d frames...",
			obsId->exposed, obsId->totalFrames);
		if (sdsuSimpleReadoutStart (sdsuId, obsId->totalFrames, (void *) obsId) == ERROR)
		{
			ERROR_LOG ("Failed to start simple readout process");
			obsId->observing = FALSE;
			observingState = CAR_ERROR;
			if (epToVxPipeWrite (NULL, (char *) &observingState, obsId->pDetObservingContext)
				== ERROR)
			{
				ERROR_LOG ("Also failed to set observing state to ERROR.");
			}
			errorNumber = S_detControl_SDSU_ERROR;
			return (errorNumber);
		}

		/*
		 * To maximise the efficiency, the following code runs in parallel with the observation.
		 * If the observation happens to finish before this code completes (which is unlikely)
		 * it will wait for the binary semaphore which is given at the end of this function.
		 */

		/*
		 * Start an alarm timer which will trigger if the frame sync callback never runs.
		 * Set the delay time to the readout timeout plus the largest frame exposure time
		 * obtained earlier.
		 *
		 * THE TIMEOUT IS NOW ONLY USED IN SIMULATION MODE - SMB 21 JAN 99
		 */

		if ( sdsuId->simulate )
		{
			if ( obsId->exposed >= obsId->exposedRQ )
			{
				if ( obsId->totalFrames > 0 )
				{
					waitTimeSecs = readoutTimeout + (obsId->exposed / (double) obsId->totalFrames);
				}
				else
				{
					waitTimeSecs = readoutTimeout + obsId->exposed;
				}
			}
			else
			{
				if ( obsId->totalFrames > 0 )
				{
					waitTimeSecs = readoutTimeout + (obsId->exposedRQ / (double) obsId->totalFrames);
				}
				else
				{
					waitTimeSecs = readoutTimeout + obsId->exposedRQ;
				}
			}

			/* BUG WORK AROUND (FOR INTERRUPT VERSION). Set the frame wait timeout. - SMB 16 Jan 99 */

			/* sdsuId->frameTimeout = (int) waitTimeSecs * sysClkRateGet(); */

			if ( timeoutAlarmSet (obsId->timeId, waitTimeSecs, detObserveTimeout, (int) obsId) == ERROR )
			{
				ERROR_SET (0, "Failed to set alarm timer", ERROR_LOG_NOW);
				obsId->observing = FALSE;
				observingState = CAR_ERROR;
				if (epToVxPipeWrite (NULL, (char *) &observingState, obsId->pDetObservingContext)
					== ERROR)
				{
					ERROR_LOG ("Also failed to set observing state to ERROR.");
				}
				errorNumber = S_detControl_INTERNAL;
				return (errorNumber);
			}
		}

		/*
		 * Convert the start time into International Atomic Time (TAI).
		 * This time will be used to generate the MJD-OBS field in the FITS header.
		 *
		 * There is currently no internationally agreed standard defining the 
		 * timescale for MJD-OBS. TAI is used here because it is a sensible choice
		 * and, in fact, was once specified in a draft standard in July 1996 that
		 * was subsequently withdrawn.
		 * Whatever timescale is specified here, it is important that it be
		 * continuous across a leap second. Suitable alternatives are
		 * Terrestrial Time (TT) and Universal Time 1 (UT1). UTC is NOT suitable.
		 *
		 * NOTE: At time of writing Pat Wallace is checking this with the FITS committee.
		 */

		if (timeThenD (obsId->rawtStart, TAI, &timeTAI) != OK)
		{
			ERROR_SET (0,  "Failed to convert time stamp to TAI", ERROR_LOG_NOW);
		}

		/*
		 * Get the current tracking frame, as read from the TCS.
		 * (Default values will be supplied if the TCS is not available).
		 */

		wfsGetTrackFrame (&trackFrame, &(trackEquinox.type), &(trackEquinox.year), &trackWavelength,
			&trackRA, &trackDec, &(trackEpoch.type), &(trackEpoch.year));
		obsId->equinox = trackEquinox.year;
		obsId->epoch   = trackEpoch.year;
		obsId->RA      = trackRA;
		obsId->Dec     = trackDec;

		/*
		 * If sufficient WCS calibration points are available, define the World Coordinate System
		 * information for this observation.
		 */

		if ( obsId->nWcsPoints >= 3 )
		{
			/*
			 * Define the (i,j) to (X,Y) transformation.
			 * N.B. For efficiency, this need only be done once, each time the detector binning
			 * is changed. CHANGE THIS EVENTUALLY.
			 *
			 * First check if any binning or windowing of the pixels on the detector has been defined.
			 */

			if ( ( obsId->ospGeometry != NULL ) &&
			     ( ( obsId->ospGeometry->xstart != 0 ) ||
			       ( obsId->ospGeometry->ystart != 0 ) ||
			       ( obsId->ospGeometry->xbin != 1 ) ||
			       ( obsId->ospGeometry->ybin != 1 )
			     )
			   )
			{
				/*
				 * There has been some binning and windowing. The original calibration is assumed
				 * to have been made on a full frame of data without binning, so the calibration
				 * points need to be transformed.
				 */

				for (p=0; p<obsId->nWcsPoints; p++)
				{
					obsId->detij[p][0] =
						((obsId->pixij[p][0] - 0.5 - (double) obsId->ospGeometry->xstart) /
						(double) obsId->ospGeometry->xbin) + 0.5;

					obsId->detij[p][1] =
						((obsId->pixij[p][1] - 0.5 - (double) obsId->ospGeometry->ystart) /
						(double) obsId->ospGeometry->ybin) + 0.5;
				}

				/*
				 * Calculate the best fit to the focal plane X,Y coordinates against binned
				 * detector coordinates.
				 */

				wcsStatus = astFitij ( obsId->nWcsPoints, obsId->fpxy, obsId->detij, obsId->cij,
					&pixis, &pixjs, &perp, &orient );
			}
			else
			{
				/*
				 * No windowing or binning have been used.
				 * Calculate the best fit to the focal plane X,Y coordinates against the original
				 * full frame, unbinned pixel coordinates.
				 */

				wcsStatus = astFitij ( obsId->nWcsPoints, obsId->fpxy, obsId->pixij, obsId->cij,
					&pixis, &pixjs, &perp, &orient );
			}

			if ( wcsStatus != 0 )
			{
				ERROR_SET1 (S_detControl_WCS_ERROR,
					"Failed to define (i,j) to (X,Y) transformation. Status=%d",
					ERROR_LOG_NOW, wcsStatus);
			}

#ifdef DEBUG
			printf ("Best fit scale is %f X units per i pixel and "
    	        "%f Y units per j pixel\n", pixis, pixjs);
			printf ("i/j non-perpendicularity is %f radians.\n", perp);
			printf ("i/j is rotated by %f radians with respect to x/y axis.\n",
				orient);
			printf ("Cij matrix contains %f %f %f %f %f %f\n", obsId->cij[0], obsId->cij[1],
				obsId->cij[2], obsId->cij[3], obsId->cij[4], obsId->cij[5]);
#endif

			/*
			 * Obtain the current TCS context from the locally stored copy.
			 * This assumes that a TCS context has been obtained elsewhere and stored
			 * using astSetCtx(), as described in section 3 of document tcs_ptw_008.
			 */

			if ( wcsStatus == 0 )
			{
				wcsStatus = astGetctx (&ctx);
				if (wcsStatus != 0)
				{
					ERROR_SET1 (S_detControl_WCS_ERROR, "Failed to get current WCS context. Status=%d",
						ERROR_LOG_NOW, wcsStatus);
				}
			}

			/*
			 * Set the chop state to which the WCS coordinate information refers.
			 * NOTE: THE ACTUAL CHOP STATE NEEDS TO BE OBTAINED FROM THE PARAMETER
			 * GIVEN TO THE "SET CHOP STATE" COMMAND.
			 */

			chopState = 0;			/* 0 means chop state A. */

			/*
			 * Extract the current focal plane to sky WCS transformation from the TCS
			 * context.
			 */

			if ( wcsStatus == 0 )
			{
				wcsStatus = astCtx2tr (ctx, trackFrame, trackEquinox, trackWavelength,
					chopState, &wcs, &rawTimeWcs);
				if (wcsStatus != 0)
				{
					ERROR_SET1 (S_detControl_WCS_ERROR,
						"Failed to get focal plane to sky WCS transformation from TCS. Status=%d",
						ERROR_LOG_NOW, wcsStatus);
				}
			}

#ifdef DEBUG
			printf ("WCS information extracted from TCS context is valid at time %f\n",
				rawTimeWcs);
#endif

			/*
			 * Combine the (i,j) to (x,y) model, cij, and (x,y) to (RA,Dec) model, wcs,
			 * into a single (i,j) to (RA,Dec) model, wcsij.
			 */

			if ( wcsStatus == 0 )
			{
				wcsStatus = astXtndtr ( obsId->cij, wcs, &wcsij );
				if (wcsStatus != 0)
				{
					ERROR_SET1 (S_detControl_WCS_ERROR,
						"Failed to combine i-j to x-y and x-y to RA-Dec models. Status=%d",
						ERROR_LOG_NOW, wcsStatus);
				}
			}

 		   /*
 		    * Calculate the World Cooordinate System header values, expressed in terms of
			* standard FITS header items.
 		    */

			if ( wcsStatus == 0 )
			{
				wcsStatus = astFITSv (wcsij, trackFrame, trackEquinox, timeTAI,
					obsId->ctype1, &(obsId->crpix1), &(obsId->crval1),
					obsId->ctype2, &(obsId->crpix2), &(obsId->crval2),
					&(obsId->cd1_1), &(obsId->cd1_2), &(obsId->cd2_1), &(obsId->cd2_2),
					obsId->radecsys, &(obsId->equinox), &(obsId->mjdobs));
				if (wcsStatus != 0)
				{
					ERROR_SET1 (S_detControl_WCS_ERROR,
						"Failed to calculate FITS standard WCS header.. Status=%d",
						ERROR_LOG_NOW, wcsStatus);
				}
			}
			obsId->wcsStatus = wcsStatus;

#ifdef DEBUG
			printf ("World Coordinate System Header\n");
			printf ("------------------------------\n");
			printf ("wcsStatus= %d\n", obsId->wcsStatus);
			printf ("ctype1   = %s\n", obsId->ctype1);
			printf ("crpix1   = %f pixels\n", obsId->crpix1);
			printf ("crval1   = %f degrees = %f hours\n", obsId->crval1,
				(obsId->crval1 / (double) 15.0));
			 printf ("ctype2   = %s\n", obsId->ctype2);
			printf ("crpix2   = %f pixels\n", obsId->crpix2);
			printf ("crval2   = %f degrees\n", obsId->crval2);
			printf ("cd1_1    = %f\n", obsId->cd1_1);
			printf ("cd1_2    = %f\n", obsId->cd1_2);
			printf ("cd2_1    = %f\n", obsId->cd2_1);
			printf ("cd2_2    = %f\n", obsId->cd2_2);
			printf ("RA       = %f hours\n", obsId->RA);
			printf ("Dec      = %f degrees\n", obsId->Dec);
			printf ("radecsys = %s\n", obsId->radecsys);
			printf ("equinox  = %f\n", obsId->equinox);
			printf ("epoch    = %f\n", obsId->epoch);
			printf ("mjd-obs  = %f\n", obsId->mjdobs);
#endif
		}
		else
		{
			/*
			 * There is insufficient information to provide World Coordinate System information.
			 * The only valid item which can be added to the header is the MJD of the observation.
			 */

			MESSAGE_LOG (MSG_WARNING,
				"No World Coordinate System calibration - there will be no WCS header");

			obsId->mjdobs = timeTAI;
			obsId->wcsStatus = -1;
		}

		/*
		 * If the DHS is being used then create a dataset to hold the unscrambled data.
		 * Otherwise allocate a buffer directly.
		 */

		if ( obsId->outOptions == 1 )
		{

			MESSAGE_LOG (MSG_MINDEBUG, "Creating DHS dataset...");

			/* Set up for the quick look */

			contrib[0] = pDetDhsClientName;		/* a global variable, set in detDhsInit */

			qlStreams[0] = "hrwfsScience";			/* THIS IS A FUDGE. DEFINE IN setDhs command. */


			/* NOTE: Lifetime should be definable
			 * PERMANENT for permanent data (e.g. calibrations)
			 * TRANSIENT for display only (e.g. acquisition camera in continuous mode)
			 * (see ICD 3).
			 */

			if ( obsId->totalFrames == 1 )				/* only one exposure */
			{
				dhsBdCtl(obsId->dhsConnection, DHS_BD_CTL_LIFETIME, obsId->pDataLabel,
					DHS_BD_LT_PERMANENT, &dhsErrno);
				MESSAGE_LOG1 (MSG_MINDEBUG,
					"LIFETIME of DHS frame %s set to DHS_BD_LT_PERMANENT", obsId->pDataLabel);
				MESSAGE_LOG1 (MSG_MINDEBUG,
					"Total Frames is %d", obsId->totalFrames);
			}
			else										/* either continuous mode with totalFrames = 0 or > 1 */
			{
				dhsBdCtl(obsId->dhsConnection, DHS_BD_CTL_LIFETIME, obsId->pDataLabel,
					DHS_BD_LT_TRANSIENT, &dhsErrno);
				MESSAGE_LOG1 (MSG_MINDEBUG,
					"LIFETIME of DHS frame %s set to DHS_BD_LT_TRANSIENT", obsId->pDataLabel);
				MESSAGE_LOG1 (MSG_MINDEBUG,
					"Total Frames is %d", obsId->totalFrames);
			}
			CHECK_DHS (dhsErrno);
			dhsBdCtl(obsId->dhsConnection, DHS_BD_CTL_CONTRIB, obsId->pDataLabel, 1, contrib,
				&dhsErrno);
			CHECK_DHS (dhsErrno);
			dhsBdCtl(obsId->dhsConnection, DHS_BD_CTL_QLSTREAM, obsId->pDataLabel, 1, qlStreams,
				&dhsErrno);
			CHECK_DHS (dhsErrno);

			/* Create the DHS dataset and add the default attributes. */

			obsId->dhsDataset = dhsBdDsNew (&dhsErrno);
			CHECK_DHS (dhsErrno);

	  		if (dhsErrno == DHS_S_SUCCESS)
			{
				dhsBdAttribAdd (obsId->dhsDataset, "instrument", DHS_DT_STRING, 0, NULL,
					pDetDhsClientName,
					&dhsErrno);
				CHECK_DHS (dhsErrno);
				dhsBdAttribAdd (obsId->dhsDataset, "telescope", DHS_DT_STRING, 0, NULL, "Gemini",
				  &dhsErrno);
				CHECK_DHS (dhsErrno);
			}

  			if (dhsErrno != DHS_S_SUCCESS)
			{
				ERROR_SET1 (S_detControl_DHS_ERROR, "Failed to create dataset (dhsErrno=%d)",
				            ERROR_LOG_NOW, dhsErrno);
				errorNumber = S_detControl_DHS_ERROR;
				return (errorNumber);
			}

			/* Create a frame to contain the data and add the frame header info. */

			dims[0] = 2;
			axisSize[0] = obsId->xPixels;  
			axisSize[1] = obsId->yPixels;
			origin[0] = 1;  
			origin[1] = 1;

			dhsErrno = DHS_S_SUCCESS; 	/* reset dhsErrno - dk */	/* Why? - SMB */
			obsId->dhsDataFrame = dhsBdFrameNew (obsId->dhsDataset, "dataArray", 0, DHS_DT_FLOAT,
				2, axisSize,
				(const void **) &(obsId->pCurFrame), &dhsErrno);
			CHECK_DHS (dhsErrno);

 	 		if (dhsErrno == DHS_S_SUCCESS)
			{
				dhsBdAttribAdd (obsId->dhsDataFrame, "dataType", DHS_DT_STRING, 0, NULL, "Intensity",
					&dhsErrno);
				CHECK_DHS (dhsErrno);
				dhsBdAttribAdd (obsId->dhsDataFrame, "bunit", DHS_DT_STRING, 0, NULL,
					"SDSU ADC units", &dhsErrno);
				CHECK_DHS (dhsErrno);
				dhsBdAttribAdd (obsId->dhsDataFrame, "units", DHS_DT_STRING, 0, NULL,
					"SDSU ADC units", &dhsErrno);
				CHECK_DHS (dhsErrno);
				dhsBdAttribAdd (obsId->dhsDataFrame, "origin", DHS_DT_INT32, 1, dims, origin,
					&dhsErrno);
				CHECK_DHS (dhsErrno);
				dhsBdAttribAdd (obsId->dhsDataFrame, "axisSize", DHS_DT_INT32, 1, dims, axisSize,
					&dhsErrno);
				CHECK_DHS (dhsErrno);
				dhsBdAttribAdd (obsId->dhsDataFrame, "axisLabel", DHS_DT_STRING, 1, dims, axisLabel,
					&dhsErrno);
				CHECK_DHS (dhsErrno);

/* COMMENTED OUT. RESTORE WHEN DHS INTERFACE WORKING CORRECTLY.
				dhsBdAttribAdd (obsId->dhsDataFrame, "obstype", DHS_DT_STRING, 0, NULL, obsId->pObsType,
				  &dhsErrno);
				CHECK_DHS (dhsErrno);
				dhsBdAttribAdd (obsId->dhsDataFrame, "exptime", DHS_DT_DOUBLE, 0, NULL, obsId->exposed,
				  &dhsErrno);
				CHECK_DHS (dhsErrno);
*/

				/* World Coordinate System attributes */

				if ( wcsStatus == 0 )
				{
					dhsBdAttribAdd (obsId->dhsDataFrame, "ctype1", DHS_DT_STRING, 0, NULL,
						obsId->ctype1, &dhsErrno);
					CHECK_DHS (dhsErrno);
					dhsBdAttribAdd (obsId->dhsDataFrame, "crpix1", DHS_DT_DOUBLE, 0, NULL,
						&(obsId->crpix1), &dhsErrno);
					CHECK_DHS (dhsErrno);
					dhsBdAttribAdd (obsId->dhsDataFrame, "crval1", DHS_DT_DOUBLE, 0, NULL,
						&(obsId->crval1), &dhsErrno);
					CHECK_DHS (dhsErrno);
					dhsBdAttribAdd (obsId->dhsDataFrame, "ctype2", DHS_DT_STRING, 0, NULL,
						obsId->ctype1, &dhsErrno);
					CHECK_DHS (dhsErrno);
					dhsBdAttribAdd (obsId->dhsDataFrame, "crpix2", DHS_DT_DOUBLE, 0, NULL,
						&(obsId->crpix2), &dhsErrno);
					CHECK_DHS (dhsErrno);
					dhsBdAttribAdd (obsId->dhsDataFrame, "crval2", DHS_DT_DOUBLE, 0, NULL,
						&(obsId->crval2), &dhsErrno);
					CHECK_DHS (dhsErrno);
					dhsBdAttribAdd (obsId->dhsDataFrame, "cd1_1", DHS_DT_DOUBLE, 0, NULL,
						&(obsId->cd1_1), &dhsErrno);
					CHECK_DHS (dhsErrno);
					dhsBdAttribAdd (obsId->dhsDataFrame, "cd1_2", DHS_DT_DOUBLE, 0, NULL,
						&(obsId->cd1_2), &dhsErrno);
					CHECK_DHS (dhsErrno);
					dhsBdAttribAdd (obsId->dhsDataFrame, "cd2_1", DHS_DT_DOUBLE, 0, NULL,
						&(obsId->cd2_1), &dhsErrno);
					CHECK_DHS (dhsErrno);
					dhsBdAttribAdd (obsId->dhsDataFrame, "cd2_2", DHS_DT_DOUBLE, 0, NULL,
						&(obsId->cd2_2), &dhsErrno);
					CHECK_DHS (dhsErrno);
				}


/* COMMENTED OUT - RA and DEC PARAMETERS ARE DEFINED AS STRINGS NOT DOUBLE
				dhsBdAttribAdd (obsId->dhsDataFrame, "RA", DHS_DT_DOUBLE, 0, NULL,
					&(obsId->RA), &dhsErrno);
				CHECK_DHS (dhsErrno);
				dhsBdAttribAdd (obsId->dhsDataFrame, "Dec", DHS_DT_DOUBLE, 0, NULL,
					&(obsId->Dec), &dhsErrno);
				CHECK_DHS (dhsErrno);
*/

/* COMMENTED OUT - THERE APPEAR TO BE TWO ENTRIES FOR EQUINOX IN THE LIBDD.CONFIG DATABASE FILE
				dhsBdAttribAdd (obsId->dhsDataFrame, "equinox", DHS_DT_DOUBLE, 0, NULL,
					&(obsId->equinox), &dhsErrno);
				CHECK_DHS (dhsErrno);
*/

/* COMMENTED OUT - NOT DEFINED IN THE DHS LIBDD.CONFIG DATABASE FILE
				dhsBdAttribAdd (obsId->dhsDataFrame, "epoch", DHS_DT_DOUBLE, 0, NULL,
					&(obsId->epoch), &dhsErrno);
				CHECK_DHS (dhsErrno);
*/

				dhsBdAttribAdd (obsId->dhsDataFrame, "mjd-obs", DHS_DT_DOUBLE, 0, NULL,
					&(obsId->mjdobs), &dhsErrno);
				CHECK_DHS (dhsErrno);

				/* ADD MORE DATA FRAME HEADER ITEMS HERE. */
			}

  			if (dhsErrno != DHS_S_SUCCESS)
			{
				ERROR_SET1 (S_detControl_DHS_ERROR, "Failed to create new data frame (dhsErrno=%d)",
				            ERROR_LOG_NOW, dhsErrno);
				errorNumber = S_detControl_DHS_ERROR;
				return (errorNumber);
			}
		}
		else
		{
			/* The DHS is not being used. */

			/* Calculate the number of pixels and reserve a buffer for the unscrambled data. */

			nPixels = obsId->xPixels * obsId->yPixels;

#ifdef DEBUG
			printf ("detObserveStart: Allocating frame buffer to hold %d pixels "
				"of unscrambled data.\n", nPixels);
#endif /* DEBUG */

			obsId->pCurFrame = (float *) malloc (nPixels * sizeof(float));
			if ( obsId->pCurFrame == NULL )
			{
				ERROR_LOG( "Failed to allocate image buffer for unscrambled data" );
				errorNumber = S_detControl_INTERNAL;
				return (errorNumber);
			}
		}

		/*
		 * Give the binary semaphore, which will allow the observation thread to
		 * process the data.
		 */

		semGive (obsId->syncSem);

#ifdef DEBUG
		printf ("detObserveStart: START directive finished.\n");
#endif /* DEBUG */

	}

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detObserveEnd
 *
 *	INVOCATION:
 *	detObserveEnd (sdsuId, obsIdIn, pRawFrame)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
*	(>)	sdsuId		(SDSU_ID)		Controller ID
 *	(>)	obsIdIn		(void *)		Pointer to observation definition, cast to void *
 *	(>)	pRawFrame	(SDSU_FRAME *)	Pointer to image frame
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Complete observation
 *
 *	DESCRIPTION:
 *	This function ends an observation.
 *
 *	EXTERNAL VARIABLES:
 *	(>)	pDetDhsClientName	(char *)	Name of DHS client = Instrument name
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	UNFINISHED - DHS AND WCS CODE NEEDS FINISHING.
 *
 *	This routine does not check the frame status bits to determine if there
 *	was an error during the data transfer over the fibre.
 *  ANJ - 27 August 1998.
 *
 *	BUGS:
 *	Resource freeing functions such as
 *	dhsBdDsFree should free their resources regardless of the value of dhsErrno, since they
 *	might be called to tidy up after an error.
 *-
 */

void detObserveEnd
	(
	SDSU_ID			sdsuId,				/* SDSU ID										*/
	void *			obsIdIn,			/* Pointer to observation ID cast to void *		*/
	SDSU_FRAME *	pRawFrame			/* Incoming Image frame							*/
	)
{
	/* Variables describing the observation. */

	OBS_ID			obsId;				/* Pointer to observation ID structure.			*/

	/* DHS variables (see dhstests.c) */

	DHS_STATUS		dhsErrno;			/* DHS error number.							*/
	DHS_STATUS		dummyDhsErrno;		/* DHS error number used for freeing resources.	*/
	DHS_TAG			putTag;				/* DHS data transfer tag.						*/

	/* File names. */

	char			pFileNameString[ (EPICS_MAX_BYTES_STRING_ATTRIB+1)*2 + 4];
										/* String containing file name.					*/

	/* SDSU parameters. */

	uint32			frameCount;			/* SDSU frame counter.							*/
	BOOL			bufferReserved;		/* TRUE if the SDSU frame buffer been reserved.	*/
	BOOL			obsAlreadyAborted;	/* TRUE if observation already  aborted.		*/

	double			readoutTimeout;		/* Readout timeout in seconds.					*/
	double			waitTimeSecs;		/* Wait time in seconds.						*/

	/*
	 * Signal processing parameters.
	 */

	int				nCoadds;			/* Number of frames per coadd.					*/
	float			amplitude;			/* Amplitude for the tip tilt calibration.		*/
	float			coaddTimeout;		/* Coadd timeout in seconds.					*/
	int				calibrate;			/* Calibration of tip or tilt .					*/
	int				outputTt;			/* Send fast Tt correction to TCS or SCS.		*/

	/* 
	 * Variables associated with "observe" command.
	 * (Label, datapath and filename use general filename parameters)
	 */

	long			observingState;		/* Observation status (busy or idle).			*/


#ifdef DEBUG
	printf ("detObserveEnd: %p %p %p\n", sdsuId, obsIdIn, pRawFrame);
#endif

	bufferReserved = FALSE;
	obsAlreadyAborted = FALSE;

	/* Check the pointers provided as arguments. */

	if ( obsIdIn == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "NULL observation ID", ERROR_LOG_NOW);
		return;
	}

	if ( pRawFrame == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "NULL raw frame pointer", ERROR_LOG_NOW);
		return;
	}

	/* Convert the observation ID pointer provided as an argument. */

	obsId = (OBS_ID) obsIdIn;

	/*
	 * Initialise the DHS error number.
	 */

	dhsErrno = DHS_S_SUCCESS;			/* <---- DHS error number is reset here. */

	/*
	 * This function should only be called when an observation is in progress.
	 */

	if ( !obsId->observing )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation not in progress", ERROR_LOG_NOW);
		return;
	}

	/*
	 * Cancel any observation timer. Failing to cancel this is not a serious error.
	 * THIS IS NOW ONLY DONE IN SIMULATION MODE.
	 */

	if ( sdsuId->simulate )
	{
		if ( obsId->timeId != NULL )
		{
			if ( timeoutAlarmCancel( obsId->timeId ) == ERROR )
			{
				ERROR_SET (0, "Failed to cancel observation timer", ERROR_LOG_NOW);
			}
		}
	}

	/*
	 * Get a timestamp to record the time at which the observation finished.
	 * Failing to cancel this is not a serious error.
	 */

	if ( timeNow (&(obsId->rawtEnd)) != OK )
	{
		ERROR_SET (0, "Failed to get time stamp at observation end", ERROR_LOG_NOW);
	}

#ifdef DEBUG
		printf ("detObserveEnd: Time at observation end: %f seconds.\n", obsId->rawtEnd);
#endif

	/*
	 * Get the frame countdown counter attached to the data and increment the frame counter.
	 */

	frameCount = pRawFrame->header.frameCount;
	obsId->nframes++;

	/* BUG WORK AROUND: THE SDSU CONTROLLER REPORTS FRAME COUNT=1 WHEN AN INFINITE NUMBER OF
	 * FRAMES ARE BEING RETURNED. IF THE CONTROLLER IS RUNNING IN CONTINUOUS MODE, POKE THE
	 * FRAME COUNT WITH ZERO. (REMOVE WHEN SDSU DSP CODE IS FIXED).
	 */

	if ( obsId->continuous ) frameCount = 0;

	/*
	 * Report the frame counter and the number of frames remaining.
	 */

	if ( obsId->stopped )
	{
		if ( frameCount > 1 )
		{
			MESSAGE_LOG1 (MSG_MINDEBUG,
				"... exposure complete and observation stopped. Frame count=%d",
				obsId->nframes);
			MESSAGE_LOG1 (MSG_WARNING,
				"WARNING: Remaining %ld frames will be aborted", (frameCount-1));
		}
		else if ( frameCount == 1 )
		{
			MESSAGE_LOG1 (MSG_MINDEBUG,
				"... exposure complete and observation stopped. Frame count=%d (last frame)",
				obsId->nframes);
		}
		else
		{
			MESSAGE_LOG1 (MSG_MINDEBUG,
				"... exposure complete and continuous observation stopped. Frame count=%d",
				obsId->nframes);
		}
	}
	else
	{
		if ( frameCount > 1 )
		{
			MESSAGE_LOG2 (MSG_MINDEBUG, "... exposure complete. Frame count=%d (%ld remaining)",
				obsId->nframes, (frameCount-1));
		}
		else if ( frameCount == 1 )
		{
			MESSAGE_LOG1 (MSG_MINDEBUG, "... exposure complete. Frame count=%d (last frame)",
				obsId->nframes);
		}
		else
		{
			MESSAGE_LOG1 (MSG_MINDEBUG, "... exposure complete. Frame count=%d (continuous)",
				obsId->nframes);
		}
	}

	/*
	 * If this is the first frame, wait for the binary semaphore indicating that the code
	 * executed at the start of the the observation has completed. (This will only matter
	 * for very short observations).
	 *
	 * If an error occurs jump to the ERROR_EXIT at the end of this function.
	 * I do not like "goto" statements but they seem to be necessary in this case
	 * where the function is void and I cannot use "return (ERROR)" and have the caller
	 * set the "observing" flag. The alternative to the "goto" would be to fill the rest
	 * of the function with "if (!error)" tests, which would be even more incomprehensible.
	 * SMB - 11 December 1998.
	 */

	if ( obsId->nframes <= 1 )
	{
#ifdef DEBUG
		printf ("detObserveEnd: Waiting for observation sync semaphore...");
#endif
		if ( semTake ( obsId->syncSem, OBS_WAIT_TIMEOUT ) == ERROR )
		{
			ERROR_SET (0, "Failed to take observation synchronisation semaphore", ERROR_LOG_NOW);
			goto ERROR_EXIT;
		}
#ifdef DEBUG
		printf (" ... got observation sync semaphore...\n");
#endif
	}

	/*
	 * Check the status of the frame just received and only process the data if the
	 * frame has been received successfully.
	 */

	if ( sdsuId->fatal )
	{
		ERROR_SET1 (0, "Fatal error at frame %lu - observation abandoned",
			ERROR_LOG_NOW, obsId->nframes);
		goto ERROR_EXIT;
	}
	else if ( (pRawFrame->header.status != 0) &&
	     ( ((pRawFrame->header.status & SDSU_FSTAT_OVERRUN) != 0) ||
	       ((pRawFrame->header.status & SDSU_FSTAT_CHECKSUM) != 0) ||
	       ((pRawFrame->header.status & SDSU_FSTAT_FRAMESYNC) != 0) ||
	       ((pRawFrame->header.status & SDSU_FSTAT_TIMEOUT) != 0)
	     )
	   )
	{
		if ((pRawFrame->header.status & SDSU_FSTAT_TIMEOUT) != 0)
		{
			MESSAGE_LOG1 (MSG_WARNING, "Timeout in frame %lu - frame ignored", obsId->nframes);
		}
		else if ((pRawFrame->header.status & SDSU_FSTAT_OVERRUN) != 0)
		{
			MESSAGE_LOG1 (MSG_WARNING, "Data overrun in frame %lu - frame ignored", obsId->nframes);
		}
		else if ((pRawFrame->header.status & SDSU_FSTAT_FRAMESYNC) != 0)
		{
			MESSAGE_LOG1 (MSG_WARNING, "Sync error in frame %lu - ignored", obsId->nframes);
		}
		else if ((pRawFrame->header.status & SDSU_FSTAT_CHECKSUM) != 0)
		{
			MESSAGE_LOG1 (MSG_WARNING, "Checksum error in frame %lu - ignored", obsId->nframes);
		}
	}
	else
	{
		/* Reserve the frame in the SDSU data buffer holding the raw image data. */

		if (sdsuFrameReserve( sdsuId, pRawFrame ) == ERROR)
		{
			ERROR_LOG ("Failed to reserve raw image frame");
			goto ERROR_EXIT;
		}
		bufferReserved = TRUE;
	
		/*
		 * Unscramble the data. The algorithm used depends on the number of detector outputs,
		 * obtained earlier.
		 */

		if ( detFrameUnscramble( obsId->xPixels, obsId->yPixels, (int) obsId->outputs,
		                         pRawFrame, obsId->pCurFrame )
		     == ERROR )
		{
			ERROR_LOG ("Failed to unscramble data");
			if ( obsId->outOptions == 1 )
			{
				dummyDhsErrno = DHS_S_SUCCESS;			/* Fudge around bad DHS feature. */
				dhsBdDsFree ( obsId->dhsDataset, &dummyDhsErrno );
			}
			else
			{
				free (obsId->pCurFrame);
				obsId->pCurFrame = NULL;
			}
			goto ERROR_EXIT;
		}

		/*
		 * If a signal processing context has been initialised, process the data.
		 */

   
		if (strcmp (obsId->pWfsName, "HRWFS") == 0)
		{
			if ( obsId->ospHRContext == NULL )
			{
				MESSAGE_LOG (MSG_MINDEBUG,
					"HRWFS: Cannot process data - no signal processing data structure defined");
			}
            else
  			{
				/*
				 * Switch according to the signal processing mode, as defined with the detSigMode
				 * command, or as defaulted in the detSigInit command.
				 */

				switch (obsId->sigMode)
				{
					case (OSP_MODE_DARK):

						/*
						 * Subtract Dark mode.
						 */
#ifdef DEBUG
						printf ("ospNewSubtractFrameFromFrame: %p %p\n", obsId->pCurFrame, obsId->ospHRContext);
#endif
						if ( ospNewSubtractFrameFromFrame (obsId->pCurFrame,
					     	obsId->ospHRContext->ffsubbuff, obsId->ospHRContext->buffsize) == ERROR )
						{
							ERROR_LOG ("Failed to subtract DARK from current frame");
						}
						break;

					default:
#ifdef DEBUG
						printf ("Signal processing switched off\n");
#endif
						break;
				}
			}
		}
        else
        {

			if (( obsId->ospFGContext == NULL ) && ( obsId->ospAOContext == NULL ))
			{
				MESSAGE_LOG (MSG_MINDEBUG,
					"PWFS: Cannot process data - no signal processing data structure defined");
			}	
			else
			{
				/*
				 * Switch according to the signal processing mode, as defined with the detSigMode
				 * command, or as defaulted in the detSigInit command.
				 */

				switch (obsId->sigMode)
				{
					case (OSP_MODE_DARK):

						/*
						 * Subtract Dark mode.
					 	 */
#ifdef DEBUG
						printf ("ospSubtractFrameFromFrame: %p %p\n", obsId->pCurFrame, obsId->ospFGContext);
#endif
						if ( obsId->ospFGContext == NULL )
						{
							MESSAGE_LOG (MSG_MINDEBUG,
							"PWFS: Cannot subtract dark - no signal processing data structure defined");
						}
						else
						{
							if ( ospSubtractFrameFromFrame (obsId->pCurFrame,
					   	  		obsId->ospFGContext->redsubbuff, obsId->ospFGContext) == ERROR )
							{
								ERROR_LOG ("Failed to subtract DARK from current frame");
							}
						}
						break;

					case (OSP_MODE_FG):

						/*
					 	* Fast Guide mode.
					 	*/
#ifdef DEBUG
						printf ("ospFGMeasure: %p %p\n", obsId->pCurFrame, obsId->ospFGContext);
#endif
						if ( obsId->ospFGContext == NULL )
						{
							MESSAGE_LOG (MSG_MINDEBUG,
							"PWFS: Cannot run fast guide mode - no signal processing data structure defined");
						}
						else
						{
							if ( ospFGMeasure (obsId->pCurFrame, obsId->ospFGContext) == ERROR )
							{
								ERROR_LOG ("Failed to generate fast guide data");
							}
						}
						break;

					case (OSP_MODE_AO):

						/*
						 * Active Optics mode.
					 	 */

						nCoadds = (int) obsId->nCoaddFrames;
						coaddTimeout = (float) obsId->coaddTimeout;
#ifdef DEBUG
						printf ("ospCoAdd: %p %d %f %p\n", obsId->pCurFrame, nCoadds,
							coaddTimeout, obsId->ospAOContext);
#endif
						if ( obsId->ospAOContext == NULL )
						{
							MESSAGE_LOG (MSG_MINDEBUG,
							"PWFS: Cannot run active optics mode - no signal processing data structure defined");
						}
						else
						{
							if ( ospCoAdd (obsId->pCurFrame, nCoadds, coaddTimeout, obsId->ospAOContext)
					    		 == ERROR )
							{
								ERROR_LOG ("Failed to generate Active Optics data");
							}
						}
						break;

					case (OSP_MODE_FG_AO):

						/*
					 	 * Fast Guide and Active Optics mode.
					 	 */

						nCoadds = (int) obsId->nCoaddFrames;
						coaddTimeout = (float) obsId->coaddTimeout;
#ifdef DEBUG
						printf ("ospFGMeasure: %p %p\n", obsId->pCurFrame, obsId->ospFGContext);
#endif
						if ( obsId->ospFGContext == NULL )
						{
							MESSAGE_LOG (MSG_MINDEBUG,
							"PWFS: Cannot run fast guide mode - no signal processing data structure defined");
						}
						else
						{
							if ( ospFGMeasure (obsId->pCurFrame, obsId->ospFGContext) == ERROR )
							{
								ERROR_LOG ("Failed to generate fast guide data");
							}
						}
#ifdef DEBUG
						printf ("ospCoAdd: %p %d %f %p\n", obsId->pCurFrame, nCoadds,
							coaddTimeout, obsId->ospAOContext);
#endif
						if ( obsId->ospAOContext == NULL )
						{
							MESSAGE_LOG (MSG_MINDEBUG,
							"PWFS: Cannot run active optics mode - no signal processing data structure defined");
						}
						else
						{
							if ( ospCoAdd (obsId->pCurFrame, nCoadds, coaddTimeout, obsId->ospAOContext)
					     		== ERROR )
							{
								ERROR_LOG ("Failed to generate Active Optics data");
							}
						}
						break;

					case (OSP_MODE_FG_COADD):

						/*
					 	 * Fast Guide and Coadd mode.
					 	 */

						nCoadds = (int) obsId->nCoaddFrames;
#ifdef DEBUG
						printf ("ospFGMeasure: %p %p\n", obsId->pCurFrame, obsId->ospFGContext);
#endif
						if ( obsId->ospFGContext == NULL )
						{
							MESSAGE_LOG (MSG_MINDEBUG,
							"PWFS: Cannot run fast guide mode - no signal processing data structure defined");
						}
						else
						{
							if ( ospFGMeasure (obsId->pCurFrame, obsId->ospFGContext) == ERROR )
							{
								ERROR_LOG ("Failed to generate fast guide data");
							}
						}
#ifdef DEBUG
						printf ("ospCoAddOnly: %p %d %p\n", obsId->pCurFrame, nCoadds,
							obsId->ospAOContext);
#endif
						if ( obsId->ospAOContext == NULL )
						{
							MESSAGE_LOG (MSG_MINDEBUG,
							"PWFS: Cannot CoAdd data - no signal processing data structure defined");
						}
						else
						{
							if ( ospCoAddOnly (obsId->pCurFrame, nCoadds,
					               		obsId->ospAOContext) == ERROR )
							{
								ERROR_LOG ("Failed to coadd data");
							}

#ifdef SAVE_COADDED_DATA
						/*
					 	* Increment the coadd counter and when it reaches nCoadds save the coadded
					 	* data to disk. Coadded data are only saved once per observation.
					 	*/

							obsId->coaddCounter++;
							if ( obsId->coaddCounter == nCoadds )
							{
							/*
						 	* Make up a file name by adding the string ".coadd" to the given
						 	* file name. Use a default file name if one has not been given.
						 	*/

								if ( strcmp(obsId->pOutFileName, "") == 0 )
								{
									strcpy ( pFileNameString, "Coadd.fits" );
								}
								else
								{
									sprintf( pFileNameString, "%s.coadd", obsId->pOutFileName );
								}

								MESSAGE_LOG1 (MSG_MINDEBUG, "Saving coadded data to %s", pFileNameString);

								if ( detWriteFits (pFileNameString, obsId, obsId->xPixels, obsId->yPixels,
						                   			obsId->ospAOContext->sumbuff) == ERROR )
								{
									ERROR_LOG ("Failed to save coadded data to disk");
								}
							}
#endif	/* SAVE_COADDED_DATA */
						}
						break;

						case (OSP_MODE_COADD):

						/*
					 	 * Coadd only mode.
					 	 */

						nCoadds = (int) obsId->nCoaddFrames;
#ifdef DEBUG
						printf ("ospCoAddOnly: %p %d %p\n", obsId->pCurFrame, nCoadds,
							obsId->ospAOContext);
#endif
						if ( obsId->ospAOContext == NULL )
						{
							MESSAGE_LOG (MSG_MINDEBUG,
							"PWFS: Cannot CoAdd data - no signal processing data structure defined");
						}
						else
						{
							if ( ospCoAddOnly (obsId->pCurFrame, nCoadds,
					              		 obsId->ospAOContext) == ERROR )
							{
								ERROR_LOG ("Failed to coadd data");
							}

#ifdef SAVE_COADDED_DATA
						/*
					 	* Increment the coadd counter and when it reaches nCoadds save the coadded
					 	* data to disk. Coadded data are only saved once per observation.
					 	*/

							obsId->coaddCounter++;
							if ( obsId->coaddCounter == nCoadds )
							{
							/*
						 	* Make up a file name by adding the string ".coadd" to the given
						 	* file name. Use a default file name if one has not been given.
						 	*/

								if ( strcmp(obsId->pOutFileName, "") == 0 )
								{
									strcpy ( pFileNameString, "Coadd.fits" );
								}
								else
								{
									sprintf( pFileNameString, "%s.coadd", obsId->pOutFileName );
								}

								MESSAGE_LOG1 (MSG_MINDEBUG, "Saving coadded data to %s", pFileNameString);

								if ( detWriteFits (pFileNameString, obsId, obsId->xPixels, obsId->yPixels,
						                   		obsId->ospAOContext->sumbuff) == ERROR )
								{
									ERROR_LOG ("Failed to save coadded data to disk");
								}
							}
#endif	/* SAVE_COADDED_DATA */
						}
						break;

					case (OSP_MODE_CALIB_TT):

						/*
						 * Calibrate Tip Tilt Focus mode.
					 	 */

						nCoadds = (int) obsId->nCoaddFrames;
						amplitude = (float) obsId->ttCalAmplitude;
						calibrate = (int) obsId->coaddTimeout;

                    	if ( calibrate == 1 ) /* tip */
                    	{
#ifdef DEBUG
							printf ("ospCalibrateTip: %p %d %f %p\n", obsId->pCurFrame, nCoadds,
									amplitude, obsId->ospFGContext);
#endif
							if ( obsId->ospFGContext == NULL )
							{
								MESSAGE_LOG (MSG_MINDEBUG,
								"PWFS: Cannot calibrate tip mode - no signal processing data structure defined");
							}
							else
							{
								if ( ospCalibrateTip (obsId->pCurFrame, nCoadds, amplitude,
					              			 obsId->ospFGContext) == ERROR )
								{
									ERROR_LOG ("Failed to calibrate tip correction");
								}
							}
				    	}
                    	else
                    	{
#ifdef DEBUG
							printf ("ospCalibrateTilt: %p %d %f %p\n", obsId->pCurFrame, nCoadds,
									amplitude, obsId->ospFGContext);
#endif
							if ( obsId->ospFGContext == NULL )
							{
								MESSAGE_LOG (MSG_MINDEBUG,
								"PWFS: Cannot calibrate tilt mode - no signal processing data structure defined");
							}
							else
							{
								if ( ospCalibrateTilt (obsId->pCurFrame, nCoadds, amplitude,
					              	 	obsId->ospFGContext) == ERROR )
								{
									ERROR_LOG ("Failed to calibrate tilt correction");
								}
							}
						}
						break;

					case (OSP_MODE_CORRECT_TT):

						/*
					 	 * Correct Tip Tilt Focus mode.
						 *
					 	 * This mode uses a correction previously calculated during
					 	 * "Calibrate Tip Tilt Focus" mode.
					 	 */

						outputTt = (int) obsId->coaddTimeout;
						nCoadds = (int) obsId->nCoaddFrames;
#ifdef DEBUG
						/*printf ("ospTtCor: %p %d %d%p\n", obsId->pCurFrame, outputTt , ncoadds ,
								obsId->ospFGContext);*/
#endif
						if ( obsId->ospFGContext == NULL )
						{
							MESSAGE_LOG (MSG_MINDEBUG,
							"PWFS: Cannot correct tip/tilt modes - no signal processing data structure defined");
						}
						else
						{
							if ( outputTt == 3 )
							{
								if ( ospTracking (obsId->pCurFrame , obsId->ospFGContext) 
									 == ERROR )
								{
									ERROR_LOG ("Failed to run Tracking correction");
								}
							}
							else if (outputTt == 4 )
							{
								if ( ospTrackingAndFocus (obsId->pCurFrame , nCoadds, obsId->ospFGContext) 
									 == ERROR )
								{
									ERROR_LOG ("Failed to run Tracking and Focus correction");
								}

							}
							else
							{
								if ( ospTtCor (obsId->pCurFrame, outputTt , obsId->ospFGContext) 
									 == ERROR )
								{
									ERROR_LOG ("Failed to define tip tilt correction");
								}
							}
						}
						break;

					default:
#ifdef DEBUG
						printf ("Signal processing switched off\n");
#endif
						break;
				}
			}
        }

		/*
		 * Send the data to the DHS, store it to disk or do nothing, as appropriate
		 */

		if ( obsId->outOptions == 1 )
		{
			MESSAGE_LOG (MSG_MINDEBUG, "Sending data to DHS...");

#ifdef DEBUG
			dhsBdDsPrint (obsId->dhsDataset, &dhsErrno);
			CHECK_DHS (dhsErrno);
#endif /* DEBUG */

			/* Send the data to the dhs */

#ifdef DEBUG
			printf ("detObserveEnd: dhsBdPut, dhsConnection=%d, pDataLabel=%s, dataset=%d\n",
				(int) obsId->dhsConnection, obsId->pDataLabel, (int) obsId->dhsDataset);
#endif /* DEBUG */

			if ( obsId->totalFrames == 1 )
			{
				putTag = dhsBdPut (obsId->dhsConnection, obsId->pDataLabel, DHS_BD_PT_DS, DHS_TRUE,
					obsId->dhsDataset, NULL, &dhsErrno);
			}
			else
			{
				putTag = dhsBdPut (obsId->dhsConnection, obsId->pDataLabel, DHS_BD_PT_DS, DHS_FALSE,
					obsId->dhsDataset, NULL, &dhsErrno);
			}
			CHECK_DHS (dhsErrno);

	  		if (dhsErrno != DHS_S_SUCCESS)
			{
				ERROR_SET1 (S_detControl_DHS_ERROR, "Failed to initiate data transfer (dhsErrno=%d)",
				            ERROR_LOG_NOW, dhsErrno);
				dummyDhsErrno = DHS_S_SUCCESS;			/* Fudge around bad DHS feature. */
				dhsTagFree (putTag, &dummyDhsErrno);
				CHECK_DHS (dummyDhsErrno); 
				dummyDhsErrno = DHS_S_SUCCESS;			/* Fudge around bad DHS feature. */
				dhsBdDsFree (obsId->dhsDataset, &dummyDhsErrno);	/* added this back in. dk */
				CHECK_DHS (dummyDhsErrno); 
				goto ERROR_EXIT;
			}

			/* Wait for completion */

#ifdef DEBUG
			printf ("detObserveEnd: dhsWait putTag=%d ...\n", (int) putTag);
#endif /* DEBUG */
			dhsWait (1, &putTag, &dhsErrno);
			CHECK_DHS (dhsErrno);

  			if (dhsErrno != DHS_S_SUCCESS)
			{
				ERROR_SET1 (S_detControl_DHS_ERROR, "Error during wait for data transfer (dhsErrno=%d)",
				            ERROR_LOG_NOW, dhsErrno);

				dummyDhsErrno = DHS_S_SUCCESS;			/* Fudge around bad DHS feature. */
				dhsTagFree (putTag, &dummyDhsErrno);
				CHECK_DHS (dummyDhsErrno);
				dummyDhsErrno = DHS_S_SUCCESS;			/* Fudge around bad DHS feature. */
				dhsBdDsFree (obsId->dhsDataset, &dummyDhsErrno);	/* added this back in. dk */
				CHECK_DHS (dummyDhsErrno); 
				goto ERROR_EXIT;
			}

			if ( detDhsCheckCmdStatus (putTag) == ERROR )
			{
				ERROR_SET (S_detControl_DHS_ERROR, "Data transfer failed", ERROR_LOG_NOW);

				dummyDhsErrno = DHS_S_SUCCESS;			/* Fudge around bad DHS feature. */
				dhsTagFree (putTag, &dummyDhsErrno);
				CHECK_DHS (dummyDhsErrno);
				dummyDhsErrno = DHS_S_SUCCESS;			/* Fudge around bad DHS feature. */
				dhsBdDsFree (obsId->dhsDataset, &dummyDhsErrno);	/* added this back in. dk */
				CHECK_DHS (dummyDhsErrno);
				goto ERROR_EXIT;
			}

			/*
			 * Free the DHS tag.
			 */

			dhsErrno = DHS_S_SUCCESS;			/* Fudge around bad DHS feature. */
			dhsTagFree (putTag, &dhsErrno);
			CHECK_DHS (dhsErrno);

			/*
			 * If the last frame has been received free the DHS dataset.
			 */

			if ( (frameCount == 1) || (obsId->stopped) )
			{
				/*dhsErrno = DHS_S_SUCCESS; */			/* Fudge around bad DHS feature. */
				/*dhsTagFree (putTag, &dhsErrno);
				CHECK_DHS (dhsErrno);*/
				dhsBdDsFree (obsId->dhsDataset, &dhsErrno);	/* Added this back in. dk */
				CHECK_DHS (dhsErrno);
			}
		}
		else if ( obsId->outOptions == 2 )
		{
			/*
			 * The DHS is not being used and the data will be saved to FITS files.
			 * If this is the first frame of the observation the standard names will be used.
			 * Frames 2 onwards have .2, .3, etc... appended to the names.
			 */

#ifdef SAVE_RAW_DATA
			/*
			 * Save the raw data to a FITS file (for engineering tests only).
			 * This only happens if the code is compiled with the SAVE_RAW_DATA option.
			 */

			if ( obsId->nframes <= 1 )
			{
				strncpy( pFileNameString, obsId->pRawFileName,
					((EPICS_MAX_BYTES_STRING_ATTRIB+1)*2 + 3) );
			}
			else
			{
				sprintf( pFileNameString, "%s.%d", obsId->pRawFileName, obsId->nframes );
			}

			MESSAGE_LOG2 (MSG_MINDEBUG, "Saving raw data from %p to directly to file \"%s\"...",
			              (uint16 *) &pRawFrame->pixel[0], pFileNameString);

			if (detWriteFitsUint16 (pFileNameString, obsId, obsId->xPixels, obsId->yPixels,
			                  (uint16 *) &pRawFrame->pixel[0])
			    == ERROR)
			{
				ERROR_LOG ("Failed to write FITS file");
				free (obsId->pCurFrame);
				obsId->pCurFrame = NULL;
				goto ERROR_EXIT;
			}
			MESSAGE_LOG (MSG_MINDEBUG, "... file saved ok");
#endif /* SAVE_RAW_DATA */


			/* Save the unscrambled data to a FITS file. */

			if ( obsId->nframes <= 1 )
			{
				strncpy( pFileNameString, obsId->pOutFileName, ((EPICS_MAX_BYTES_STRING_ATTRIB+1)*2 + 3) );
			}
			else
			{
				sprintf( pFileNameString, "%s.%d", obsId->pOutFileName, obsId->nframes );
			}

			MESSAGE_LOG2 (MSG_MINDEBUG, "Saving unscrambled data from %p to directly to file \"%s\"...",
			              obsId->pCurFrame, pFileNameString);

			if (detWriteFits (pFileNameString, obsId, obsId->xPixels, obsId->yPixels, obsId->pCurFrame)
			    == ERROR)
			{
				ERROR_LOG ("Failed to write FITS file");
				free (obsId->pCurFrame);
				obsId->pCurFrame = NULL;
				goto ERROR_EXIT;
			}
			MESSAGE_LOG (MSG_MINDEBUG, "... file saved ok");
		}

	}

	/*
	 * If the DHS is not being used and the last frame has been received,
 	 * free the unscrambled data buffer.
	 */

	if ( obsId->outOptions != 1 )
	{
		if ( (frameCount == 1) || (obsId->stopped) )
		{
			free (obsId->pCurFrame);
			obsId->pCurFrame = NULL;
		}
	}

	/*
	 * Abort any further readouts if the observation was stopped prematurely.
	 */

	if ( obsId->stopped )
	{
		obsAlreadyAborted = TRUE;
		if (sdsuReadoutAbort (sdsuId) == ERROR)
		{
			ERROR_LOG ("Failed to abort readouts on receipt of STOP instruction");
			goto ERROR_EXIT;
		}
	}


/* NORMAL_EXIT: */

	/*
	 * Release the SDSU frame buffer.
	 */

	if ( bufferReserved )
	{
		sdsuFrameRelease (sdsuId, pRawFrame);
		bufferReserved = FALSE;
	}

	/*
	 * If the last frame has been received, set the observing flag FALSE
	 * and set the observeC CAR record to IDLE via the "observing" record.
	 * Otherwise set a timeout on the receipt of the next frame.
	 */

	if ( (frameCount == 1) || (obsId->stopped) )
	{
		if ( sdsuId->frameErrors <= 0 )
		{
			MESSAGE_LOG (MSG_LOG, "Observation completed successfully");
		}
		else if ( sdsuId->frameErrors < obsId->nframes )
		{
			MESSAGE_LOG1 (MSG_WARNING, "Observation completed with %d frames lost",
				sdsuId->frameErrors);
		}
		else
		{
			ERROR_LOG ("Observation failed - all frames lost");
			goto ERROR_EXIT;
		}

		obsId->observing = FALSE;
		observingState = CAR_IDLE;
		if (epToVxPipeWrite (NULL, (char *) &observingState, obsId->pDetObservingContext) == ERROR)
		{
			ERROR_LOG ("Failed to set observing flag to IDLE");
		}
	}
	else
	{
#ifdef DEBUG
		printf ("detObserveEnd: Further frames are anticipated - observation not finished.\n");
#endif

		/*
		 * Start an alarm timer which will trigger if the frame sync callback never runs.
		 * Set the delay time to the readout timeout plus the largest frame exposure time
		 * obtained earlier.
		 *
		 * THE TIMEOUT IS NOW ONLY USED IN SIMULATION MODE - SMB 21 JAN 99
		 */


		if ( sdsuId->simulate )
		{
			readoutTimeout = 5.0;
			if ( obsId->exposed >= obsId->exposedRQ )
			{
				if ( obsId->totalFrames > 0 )
				{
					waitTimeSecs = readoutTimeout + (obsId->exposed / (double) obsId->totalFrames);
				}
				else
				{
					waitTimeSecs = readoutTimeout + obsId->exposed;
				}
			}
			else
			{
				if ( obsId->totalFrames > 0 )
				{
					waitTimeSecs = readoutTimeout + (obsId->exposedRQ / (double) obsId->totalFrames);
				}
				else
				{
					waitTimeSecs = readoutTimeout + obsId->exposedRQ;
				}
			}

			if ( timeoutAlarmSet (obsId->timeId, waitTimeSecs, detObserveTimeout, (int) obsId) == ERROR )
			{
				ERROR_LOG ("Failed to set alarm timer");
			}
		}
	}

	return;


ERROR_EXIT:

	/*
	 * If an error occurred, abort the observation, release the SDSU frame buffer (if necessary)
	 * set the observing flag FALSE and set the observeC CAR record to ERROR,
	 * via the "observing" record.
	 */

	if ( !obsAlreadyAborted )
	{
		if (sdsuReadoutAbort (sdsuId) == ERROR)
		{
			ERROR_LOG ("Failed to abort readouts after error");
		}
		obsAlreadyAborted = TRUE;
	}

	if ( bufferReserved )
	{
		sdsuFrameRelease (sdsuId, pRawFrame);
		bufferReserved = FALSE;
	}

	obsId->observing = FALSE;
	observingState = CAR_ERROR;
	if (epToVxPipeWrite (NULL, (char *) &observingState, obsId->pDetObservingContext) == ERROR)
	{
		ERROR_LOG ("Failed to set observing flag to ERROR");
	}

	return;
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detObserveTimeout
 *
 *	INVOCATION:
 *	detObserveTimeout (timeId, obsIdInt)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)		timeId		(timer_t)		Timer ID
 *	(>)		obsIdInt	(int)			Pointer to observation definition, cast to integer
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Handle an observation timeout.
 *
 *	DESCRIPTION:
 *	Handle the situation when a readout does not complete within the time
 *	when its exposure and readout should have finished.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	UNFINISHED
 *-
 */

void detObserveTimeout
	(
	timer_t		timeId,					/* Timer ID.									*/
	int			obsIdInt				/* Pointer to observation ID cast to int.		*/
	)
{
	OBS_ID		obsId = (OBS_ID) obsIdInt;
	SDSU_ID		sdsuId = (SDSU_ID) obsId->sdsuId;

	char *		mainKeywords[] = {"NAXIS1", "NAXIS2", "SECTORS", "OSP_FSZ"};
										/* Main FITS keywords to read from header.		*/
	int			mainValues[4];			/* Values corresponding to main FITS keywords.	*/

	/*
	 * The following variables will be used to test additional header items in a file
	 * of simulated data, but the check has not been implemented yet.
	 */

#ifdef NEW_OSP_STUFF
	char *		extraKeywords[] = {"XSTART", "YSTART", "XBIN", "YBIN", "XRASTER", "YRASTER",
				                   "XSPACE", "YSPACE", "XSUBAP", "YSUBAP"};
										/* Extra FITS keywords to read from header.		*/
	int			extraValues[10];		/* Values corresponding to extra FITS keywords.	*/
#endif /* NEW OSP STUFF */

	long		observingState;			/* Observation status (busy or idle).			*/
	int			simOption;				/* Simulation option.							*/

#ifdef DEBUG
	printf ("detObserveTimeout: Observation timed out.\n");
#endif

	/*
	 * When sdsuLib is simulating this routine makes the frame look like it has
	 * been read out properly by the controller, and calls the frame ISR.
	 * Simulating the packet ISRs would be a bit tricky...
	 */
	
	if (sdsuId->simulate)
	{
		SDSU_FRAME *pFrame = sdsuId->readFrame;
		
		pFrame->header.packetCount	= 0;
		pFrame->header.status		= 0;	/* No errors during readout */
		pFrame->header.parameterId	= 0;	/* Simulated parameter ID */
		pFrame->header.frameCount	= 1;	/* Simulate just one frame */

		if ( strcmp(obsId->pSimFileName, "NONE") == 0 )
		{

			/*
			 * Simulate the data internally, writing the result to the current SDSU frame.
			 * Use option 1 (a simple ramp) for large data frames and option 2 (simulated
			 * Shack-Hartmann spots) for small data frames.
			 */

			MESSAGE_LOG (MSG_LOG, "Simulating data internally");

			if ( (obsId->xPixels > 256) || (obsId->yPixels > 256) )
			{
				simOption = 1;
			}
			else
			{
				simOption = 2;
			}

			if ( detSimulateData (obsId->xPixels, obsId->yPixels, simOption, pFrame) == ERROR )
			{
				ERROR_LOG ("Failed to simulate data");
			}
		}
		else
		{
			/*
			 * Read simulated data from the specified file.
			 * First check the contents of the file correspond to the actual SDSU
			 * setup.
			 */

			MESSAGE_LOG1 (MSG_LOG, "Reading simulated data from %s\n", obsId->pSimFileName);
			if (ospReadHeaderInt (obsId->pSimFileName, 3, mainKeywords, mainValues) == ERROR )
			{
				ERROR_SET (0, "Failed to read simulated data header", ERROR_LOG_NOW);
			}

			if ( (mainValues[0] == obsId->xPixels) && (mainValues[1] == obsId->yPixels) &&
			     (mainValues[2] == obsId->outputs)
			   )
			{

				/*
				 * The file is acceptable. Now read its contents.
				 */

				MESSAGE_LOG (MSG_MINDEBUG, "Simulated data header looks OK");
				if (ospReadUShortImage ((uint16 *)& (pFrame->pixel[0]), obsId->pSimFileName,
						(obsId->xPixels)*(obsId->yPixels))
					== ERROR )
				{
					ERROR_SET (0, "Failed to read simulated data", ERROR_LOG_NOW);
				}
			}
			else
			{
				/*
				 * The simulated data contained in the file does not match the simulated
				 * data required.
				 */

				ERROR_SET4 (S_detControl_BAD_FILE,
					"Required size is %d x %d, simulated data file contains %d x %d",
					ERROR_LOG_SAVE, obsId->xPixels, obsId->yPixels,
					mainValues[0], mainValues[1]);
				ERROR_SET2 (0, "%ld detector outputs are required, simulated data file assumes %d",
					ERROR_LOG_SAVE, obsId->outputs, mainValues[2]);
				ERROR_LOG ("Mismatch between simulated data file and requirements");
			}
		}

		/* Simulate the packet count reaching the desired value. */

		pFrame->header.packetCount	= sdsuId->packetsPerFrame;

		/*
		 * Simulate an SDSU frame sync interrupt. This should cause the detObserveEnd
		 * callback to be executed.
		 */

/* COMMENTED OUT - ONLY ANY USE WHEN USING INTERRUPTS.
		if ( sdsuSimulateSimpleSync(sdsuId) == ERROR)
		{
			ERROR_LOG ("Failed to simulate frame sync interrupt");
		}
*/
	}
	else if ( sdsuId->frameIntNum == 0 )
	{
		/*
		 * The observation timed out with SDSU frame interrupts disabled.
		 * Assume the data are available in the buffer and simulate an SDSU frame sync interrupt.
		 * This should cause the detObserveEnd callback to be executed.
		 */

		MESSAGE_LOG (MSG_MINDEBUG, "Observation time completed with interrupts disabled");
	
		if ( sdsuSimulateSimpleSync(sdsuId) == ERROR)
		{
			ERROR_LOG ("Failed to simulate frame sync interrupt");
		}
	}
	else
	{
		/* sysIntDisable(6); */							/* DEBUG TEST */

		/*
		 * The observation completion was supposed to have been signalled by an interrupt and timed out.
		 * Set the observing flag FALSE and set the observeC CAR record to ERROR,
		 * via the "observing" record.
		 */

		MESSAGE_LOG (MSG_WARNING, "Observation timed out - trying to read data anyway...");

		obsId->observing = FALSE;
		observingState = CAR_ERROR;
		if (epToVxPipeWrite (NULL, (char *) &observingState, obsId->pDetObservingContext) == ERROR)
		{
			ERROR_LOG ("Failed to set observing flag to ERROR");
		}

		/* Try and simulate a frame sync interrupt to force a data readout. This may or may not work. */

		if ( sdsuSimulateSimpleSync (sdsuId) == ERROR)
		{
			ERROR_LOG ("Failed to simulate frame sync interrupt");
		}
	}
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detStop
 *
 *	INVOCATION:
 *	detStop (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName				(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext			(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber			(int)				Command number
 *	(>)	sdsuId					(SDSU_ID)			Current SDSU context structure
 *	(>)	obsId					(OBS_ID)			Current observation context structure
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detStop command
 *
 *	DESCRIPTION:
 *	This function stops an observation.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None
 *-
 */

uint32 detStop
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId				/* Observation context structure.			*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.			*/

	/*
	 * Initialise the error number.
	 */

	errorNumber = 0;

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * This function should only be called when an observation is in progress.
	 */

	if ( !obsId->observing )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation not in progress", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	MESSAGE_LOG (MSG_LOG, "Stopping observation.");

	/*
	 * Cancel any observation timer.
	 *
 	 * THIS IS NOW ONLY DONE IN SIMULATION MODE.
	 */

	if ( sdsuId->simulate )
	{
		if ( obsId->timeId != NULL )
		{
			if ( timeoutAlarmCancel( obsId->timeId ) == ERROR )
			{
				ERROR_LOG ("Failed to cancel observation timer");
			}
		}
	}

	/*
	 * Stop the observation prematurely by setting the obsId->stopped flag.
	 * The next time a frame of data appears it will be treated as the last one.
	 */

	obsId->stopped = TRUE;

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detAbort
 *
 *	INVOCATION:
 *	detAbort (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName				(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext			(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber			(int)				Command number
 *	(>)	sdsuId					(SDSU_ID)			Current SDSU context structure
 *	(>)	obsId					(OBS_ID)			Current observation context structure
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detAbort command
 *
 *	DESCRIPTION:
 *	This function aborts an observation. It will send an abort to the SDSU controller even
 *	if an observation appears not to be taking place.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detAbort(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId				/* Observation context structure.			*/
)
{
	uint32			errorNumber;		/* Error number reported by task.			*/

	int				observingState;		/* Observation status (busy or idle).		*/

	/*
	 * Initialise the error number.
	 */

	errorNumber = 0;

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * This function will normally be called when an observation is in progress.
	 */

	if ( obsId->observing )
	{
		MESSAGE_LOG (MSG_LOG, "Aborting observation");
	}
	else
	{
		MESSAGE_LOG (MSG_WARNING, "WARNING: Observation not in progress but attempting to abort anyway");
	}

	/*
	 * Cancel any observation timer.
	 * NOTE: THIS IS NOW ONLY DONE IN SIMULATION MODE.
	 */

	if ( sdsuId->simulate )
	{
		if ( obsId->timeId != NULL )
		{
			if ( timeoutAlarmCancel( obsId->timeId ) == ERROR )
			{
				ERROR_LOG ("Failed to cancel observation timer");
			}
		}
	}

	/*
	 * Abort the readout process and throw away the data.
	 */

	if (sdsuReadoutAbort (sdsuId) == ERROR)
	{
		ERROR_LOG ("Failed to abort readouts");
		errorNumber = S_detControl_SDSU_ERROR;
	}

	/*
	 * Reset the "observation in progress" flag and set the observeC CAR record to IDLE,
	 * via the "observing" record.
	 */

	if ( obsId->observing )
	{

		obsId->observing = FALSE;
		observingState = CAR_IDLE;
		if (epToVxPipeWrite (NULL, (char *) &observingState, obsId->pDetObservingContext) == ERROR)
		{
			ERROR_LOG ("Failed to set observing flag to IDLE.");
		}
	}

	return (errorNumber);
}



/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detInit
 *
 *	INVOCATION:
 *	detInit (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, pSdsuId, obsId, pVmeAddress,
 *	         pxMax, pyMax, pxPixels, pyPixels, pmaxFrames, pDetInitContext, pDetInitStatusContext,
 *           pGotControl)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName				(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext			(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber			(int)				Command number
 *	(!)	pSdsuId					(SDSU_ID *)			Pointer to current SDSU context structure
 *	(>) obsId					(OBS_ID)			Observation context structure
 *	(!)	pVmeAddress				(uint32 *)			Pointer to VME address of SDSU controller
 *	(!) pxMax					(int *)				Pointer to number of X pixels
 *	(!) pyMax					(int *)				Pointer to number of Y pixels
 *	(!) pxPixels				(int *)				Pointer to number of X pixels
 *	(!) pyPixels				(int *)				Pointer to number of Y pixels
 *	(!) pMaxFrames				(int *)				Pointer to max frames in data buffer
 *	(>)	pDetInitContext			(DATREC_CONTEXT)	Content structure for init state record
 *	(>)	pDetInitStatusContext	(DATREC_CONTEXT)	Content structure for init status record
 *	(!)	pGotControl				(BOOL *)			Pointer to gotControl flag
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detInit command
 *
 *	DESCRIPTION:
 *	This function initialises the SDSU controller and redownloads the DSP code.
 *
 *	EXTERNAL VARIABLES:
 *	(<)	detSdsuIdP1				(SDSU_ID)			SDSU context structure for PWFS1
 *	(<)	detSdsuIdP2				(SDSU_ID)			SDSU context structure for PWFS2
 *	(<)	detSdsuIdOi				(SDSU_ID)			SDSU context structure for OIWFS
 *	(<)	detSdsuIdHr				(SDSU_ID)			SDSU context structure for HRWFS
 *
 *	PRIOR REQUIREMENTS:
 *	The VME address supplied must have been previously verified to be correct (see below).
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	Memory problems can arise if an attempt is made to initialise the controller
 *	at an invalid address. The address should be verified to be correct before attempting
 *	to initialise the controller.
 *	Note that a zero address is used to flag simulation mode, and is therefore acceptable.
 *-
 */

uint32 detInit
	(
	const char *	pWfsName,			/* Name of wavefront sensor.						*/
	const char *	pRecordPrefix,		/* Record name prefix.								*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.					*/
	int				commandNumber,		/* Command number.									*/
	SDSU_ID *		pSdsuId,			/* Pointer to SDSU context structure.				*/
	OBS_ID			obsId,				/* Observation context structure.					*/
	uint32 *		pVmeAddress,		/* Pointer to VME address of SDSU controller.		*/
	int *			pxMax,				/* Maximum number of X pixels.						*/
	int *			pyMax,				/* Maximum number of Y pixels.						*/
	int *			pxPixels,			/* Current number of X pixels.						*/
	int *			pyPixels,			/* Current number of Y pixels.						*/
	int *			pMaxFrames,			/* Maximum number of frames in buffer.				*/
	DATREC_CONTEXT	pDetInitContext,	/* Context structure for SDSU initialisation		*/
										/* state SIR record.								*/
	DATREC_CONTEXT	pDetInitStatusContext,
										/* Context structure for SDSU initialisation		*/
										/* status SIR record.								*/
	BOOL *			pGotControl			/* Pointer to gotControl flag.						*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.					*/
	long			initState;			/* Initialisation state.							*/
	long			simulate;			/* TRUE if SDSU interface is simulated. 			*/

	char 			pStatusString [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Status string.									*/
	char			pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Path name for file.								*/
	char			pOmfFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* File name.										*/
	char			pFullOmfFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
										/* Combined path name and file name.				*/
	BOOL			limitAdrsRange;		/* Flag for limiting address range in DSP memory	*/
	int				nPixels;			/* Total number of digitised pixels.				*/
	int				newMaxFrames;		/* New maximum number of frames.					*/

	uint32			mode;
	
	/*
	 * Initialise the error number.
	 */

	errorNumber = 0;

	/*
	 * If the controller does not currently have control of the hardware try and get it.
	 * A failure to gain access to the hardware is not regarded as an error, since if the
	 * hardware is being shared it is normal for one of the controllers not to have access
	 * (and all the controllers may be being initialised at the same time). If a failure
	 * occurs just issue a warning.
	 */

	if (!(*pGotControl))
	{
		if (semTake (detSwitchSem, NO_WAIT) == ERROR)
		{
			MESSAGE_LOG (MSG_WARNING,
				"WARNING: Another detector controller still has control of the hardware");
			return (errorNumber);
		}
		*pGotControl = TRUE;
	}

	/*
	 * The command can only be used when an observation is not in progress.
	 */

	if ( (obsId != NULL) && (obsId->observing) )
	{
		ERROR_SET (S_detControl_BUSY,
		 	"Observation in progress - abort observation and try again", ERROR_LOG_NOW);
		errorNumber = S_detControl_BUSY;
		return (errorNumber);
	}

	/* Set the initialisation state to BUSY. */

	initState = CAR_BUSY;
	if (epToVxPipeWrite (NULL, (char *) &initState, pDetInitContext) == ERROR)
	{
		ERROR_LOG ("Failed to set initialisation state to BUSY");
	}

	/*
	 * Obtain the VME address of the SDSU controller.
	 * An address of zero signifies simulation mode.
	 */

	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) pVmeAddress);

#ifdef DEBUG
	printf ("detInit: VME address = %ld = %#lx\n", *pVmeAddress, *pVmeAddress);
#endif /* DEBUG */

	if ( *pVmeAddress == 0 )
	{
		simulate = TRUE;
	}
	else
	{
		simulate = FALSE;
	}

	/*
	 * If an SDSU context structure already exists, delete it.
	 */

	if (*pSdsuId != NULL)
	{
		if (sdsuContextDelete (*pSdsuId) == ERROR)
		{
			ERROR_LOG ("Failed to delete old SDSU context structure");
		}
	}

	/*
	 * Now attempt to create a new context structure, remembering to call sdsuReset()
	 * immediately after sdsuContextCreate().
	 */

	*pSdsuId = sdsuContextCreate (*pVmeAddress, simulate);
	if ( (*pSdsuId  == NULL) ||
	     (sdsuReset (*pSdsuId, SDSU_RESET_VME | SDSU_RESET_CONTROLLER) == ERROR)
	   )
	{
		if ( simulate )
		{
			ERROR_SET1 (0, "WFS %s: Error initialising SDSU controller in simulation mode",
				ERROR_LOG_NOW, pWfsName);
		}
		else
		{
			ERROR_SET2 (0, "WFS %s: Error initialising SDSU controller at VME address %#lx",
				ERROR_LOG_NOW, pWfsName, *pVmeAddress);
		}
		errorNumber = S_detControl_SDSU_ERROR;

		/* Set the initialisation state to ERROR. */

		initState = CAR_ERROR;
		if (epToVxPipeWrite (NULL, (char *) &initState, pDetInitContext) == ERROR)
		{
			ERROR_LOG ("Failed to set initialisation state to ERROR");
		}

		/*
		 * If an error occurs while initialising the SDSU controller its health
		 * must be set "BAD" because it can no longer function.
		 */

		epToVxSetHealth (pRecordPrefix, "BAD");
		return (errorNumber);
	}
	else
	{

		/*
		 * A new SDSU context structure has been obtained successfully.
		 */

		MESSAGE_LOG1 (MSG_LOG, "SDSU ID structure at %#x initialised successfully", (int) *pSdsuId);

		/* Write a new string to the SDSU initialisation status SIR record. */

		if ( simulate )
		{
			sprintf (pStatusString, "SDSU SIMULATED: ID = %-#8x", (int) *pSdsuId);
		}
		else
		{
			sprintf (pStatusString, "SDSU Initialised OK: ID = %-#8x", (int) *pSdsuId);
		}

		if (epToVxPipeWrite (NULL, pStatusString, pDetInitStatusContext) == ERROR)
		{
			ERROR_LOG ("Failed to write init message to SDSU status pipe.");
		}

		/* Update the global variables used to remember the SDSU contexts, as an
		 * aid to engineering.
		 */

		if (strcmp (pWfsName, "p1") == 0)
		{
			(*pSdsuId)->fastCamera = TRUE;
			detSdsuIdP1 = *pSdsuId;
		}
		else if (strcmp (pWfsName, "p2") == 0)
		{
			(*pSdsuId)->fastCamera = TRUE;
			detSdsuIdP2 = *pSdsuId;
		}
		else if (strcmp (pWfsName, "oi") == 0)
		{
			(*pSdsuId)->fastCamera = TRUE;
			detSdsuIdOi = *pSdsuId;
		}
		else if (strcmp (pWfsName, "hr") == 0)
		{
			(*pSdsuId)->fastCamera = FALSE;
			detSdsuIdHr = *pSdsuId;
		}
	}

	/* Now obtain the path of the directory containing the DSP code. */

	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pFilePath);


	/* Determine whether any code should be downloaded to the VME DSP */

	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pOmfFileName);

	if ( (strcmp (pOmfFileName, "") != 0) && (strcmp (pOmfFileName, "NONE") != 0) )
	{

		/*
		 * The ability to limit the address range is ignored. It is rarely needed
		 * and can only be done by executing sdsuFileDnload at the console (since
	 	 * sdsuFileDnload expects to prompt for the values).
		 */

		limitAdrsRange = 0;

		if ( strcmp (pFilePath, "") == 0 )
		{
			strncpy (pFullOmfFileName, pOmfFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
		}
		else
		{
			sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
		}

		MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to VME DSP...",
			pFullOmfFileName);

		if (sdsuFileDnload (*pSdsuId, pFullOmfFileName, SDSU_IDENT_VME, limitAdrsRange) == ERROR)
		{
			ERROR_LOG ("Failed to download OMF file to VME DSP");
			errorNumber = S_detControl_SDSU_ERROR;

			/* Set the initialisation state to ERROR. */

			initState = CAR_ERROR;
			if (epToVxPipeWrite (NULL, (char *) &initState, pDetInitContext) == ERROR)
			{
				ERROR_LOG ("Failed to set initialisation state to ERROR");
			}

			/*
			 * If an OMF file could not be downloaded the SDSU controller is in a state
			 * where it can only obey a subset of the commands and cannot make observations,
			 * so set the health to WARNING.
			 */

			epToVxSetHealth( pRecordPrefix, "WARNING" );
		}
	}

	/* Determine whether any code should be downloaded to the Timing DSP */

	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, pOmfFileName);

	if ( (strcmp (pOmfFileName, "") != 0) && (strcmp (pOmfFileName, "NONE") != 0) )
	{

		/*
		 * The ability to limit the address range is ignored. It is rarely needed
		 * and can only be done by executing sdsuFileDnload at the console (since
		 * sdsuFileDnload expects to prompt for the values).
		 */

		limitAdrsRange = 0;
		if ( strcmp (pFilePath, "") == 0 )
		{
			strncpy (pFullOmfFileName, pOmfFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
		}
		else
		{
			sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
		}

		MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to TIMING DSP...", pFullOmfFileName);

		if (sdsuFileDnload (*pSdsuId, pFullOmfFileName, SDSU_IDENT_TIM, limitAdrsRange) == ERROR)
		{
			ERROR_LOG ("Failed to download OMF file to TIMING DSP");
			errorNumber = S_detControl_SDSU_ERROR;

			/*
			 * If an OMF file could not be downloaded the SDSU controller is in a state
			 * where it can only obey a subset of the commands and cannot make observations,
			 * so set the health to WARNING.
			 */

			epToVxSetHealth( pRecordPrefix, "WARNING" );
		}
	}

	/* Determine whether any code should be downloaded to the Utility DSP */

	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, pOmfFileName);

	if ( (strcmp (pOmfFileName, "") != 0) && (strcmp (pOmfFileName, "NONE") != 0) )
	{

		/* The ability to limit the address range is ignored. It is rarely needed
		 * and can only be done by executing sdsuFileDnload at the console (since
		 * sdsuFileDnload expects to prompt for the values).
		 */

		limitAdrsRange = 0;

		if ( strcmp (pFilePath, "") == 0 )
		{
			strncpy (pFullOmfFileName, pOmfFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
		}
		else
		{
			sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
		}

		MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to UTILITY DSP...", pFullOmfFileName);

		if (sdsuFileDnload (*pSdsuId, pFullOmfFileName, SDSU_IDENT_UTL, limitAdrsRange) == ERROR)
		{
			ERROR_LOG ("Failed to download OMF file to UTILITY DSP");
			errorNumber = S_detControl_SDSU_ERROR;

			/*
			 * If an OMF file could not be downloaded the SDSU controller is in a state
			 * where it can only obey a subset of the commands and cannot make observations,
			 * so set the health to WARNING.
			 */

			epToVxSetHealth( pRecordPrefix, "WARNING" );
		}
	}

	if ( errorNumber == 0 )
	{
		/*
		 * On the HRWFS the default packet size must be increased.
		 * The HRWFS should not run in sync mode to ensure adequate dymanic range.
		 * (It is not a serious error if this does not happen).
		 *
		 * THIS BLOCK OF CODE CAN BE REMOVED WHEN TIM HARDY'S NEW DSP CODE SETS
		 * APPROPRIATE DEFAULTS.
		 */

		if ( strcmp (pWfsName, "hr") == 0 )
		{
			if (sdsuParamWrite (*pSdsuId, SDSU_IDENT_VME, "V_PSIZE", 1024) == ERROR)
			{
				ERROR_LOG ("Failed to increase the HRWFS packet size");
			}

			if (sdsuParamRead (*pSdsuId, SDSU_IDENT_TIM, "T_MODE", &mode) == ERROR)
			{
				ERROR_LOG ("Failed to read default mode from timing board");
			}
			else
			{
				if ( (mode & SDSU_TIM_MODE_SYNC) != 0 )
				{
					mode &= ~SDSU_TIM_MODE_SYNC;
					if (sdsuParamWrite (*pSdsuId, SDSU_IDENT_TIM, "T_MODE", mode) == ERROR)
					{
						ERROR_LOG ("Failed to clear sync mode on timing board");
					}
				}
			}
		}
		else
		{
			/* The default PWFS packet size should be larger when used in full frame mode. */

			if (sdsuParamWrite (*pSdsuId, SDSU_IDENT_VME, "V_PSIZE", 80) == ERROR)
			{
				ERROR_LOG ("Failed to increase the PWFS packet size");
			}
		}
	}

	/*
	 * After successfully downloading new OMF code, the controller must be reinitialised
	 * by sending an "INI" command to the utility DSP and a "LDP" command to the timing DSP.
	 * If this fails the controller may not be usable, so the health must be set to WARNING.
	 */

	if ( errorNumber == 0 )
	{
		if (sdsuPrimitive (*pSdsuId, "INI", SDSU_IDENT_UTL, NULL, NULL) == ERROR)
		{
			ERROR_LOG ("Failed to initialise UTILITY DSP with INI command");
			errorNumber = S_detControl_SDSU_ERROR;
			epToVxSetHealth( pRecordPrefix, "WARNING" );
		}
		if (sdsuPrimitive (*pSdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
		{
			ERROR_LOG ("Failed to initialise TIMING DSP with LDP command");
			errorNumber = S_detControl_SDSU_ERROR;
			epToVxSetHealth( pRecordPrefix, "WARNING" );
		}
	}

	/*
	 * Find out if a different maximum number of frames is needed.
	 * (A value of zero or less means "no change").
	 */

	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, (char *) &newMaxFrames);

	if ( newMaxFrames > 0 ) *pMaxFrames = newMaxFrames;

	/*
	 * Compare the default detector geometry contained in the DSP code with the current values
	 * for xPixels and yPixels. After calling detCheckGeometry, *pxMax and *pyMax should
	 * contain the maximum possible data array size, allowing a data buffer of suitable size
	 * to be allocated.
	 */

	if (detCheckGeometry (pWfsName, *pSdsuId, pxMax, pyMax, pxPixels, pyPixels) == ERROR)
	{
		ERROR_LOG ("Error while checking default detector geometry");
		/* This is not a serious error. Do not change the error number or health. */
	}

	/*
	 * Allocate a buffer capable of holding several frames of data, using the *pxMax and *pyMax
	 * determined above. If this fails, the controller will not be able to store data, so the
	 * health must be set WARNING.
	 */

#ifdef DEBUG
	printf ("detInit: Creating new data buffer to hold %d frames of (%d x %d) pixels.\n",
	        *pMaxFrames, *pxMax, *pyMax);
#endif /* DEBUG */

	nPixels = (*pxMax) * (*pyMax);
	if (sdsuBufferCreate (*pSdsuId, nPixels, *pMaxFrames) == ERROR)
	{
		ERROR_LOG ("Failed to create frame data buffers on initialisation");
		errorNumber = S_detControl_SDSU_ERROR;
		epToVxSetHealth( pRecordPrefix, "WARNING" );
	}
	
	/*
	 * Next we initialise the readout process with our frame callback.
	 * If this fails the SDSU controller will be unable to readout data, so the health
	 * must be set to WARNING.
	 *
	 * There is no packet callback in this version of the code.
	 * INTERRUPTS DISABLED. SIMPLE VERSION. HRWFS RUNS AT LOWER PRIORITY.
	 */
	
#ifdef DEBUG
	printf ("detInit: Starting the readout task and frame sync callback.\n");
#endif /* DEBUG */

	if (strcmp (pWfsName, "hr") == 0)
	{
		if (sdsuSimpleReadoutOpen (*pSdsuId, NULL, detObserveEnd, 1, FALSE) == ERROR)
		{
			ERROR_LOG ("Failed to start readout task on initialisation");
			errorNumber = S_detControl_SDSU_ERROR;
			epToVxSetHealth( pRecordPrefix, "WARNING" );
		}
	}
	else
	{
		if (sdsuSimpleReadoutOpen (*pSdsuId, NULL, detObserveEnd, 0, FALSE) == ERROR)
		{
			ERROR_LOG ("Failed to start readout task on initialisation");
			errorNumber = S_detControl_SDSU_ERROR;
			epToVxSetHealth( pRecordPrefix, "WARNING" );
		}
	}

	/*
	 * If the error number is good after initialisation the health of
	 * the controller can be restored to "GOOD".
	 *
	 * Also set the initialisation state to IDLE or ERROR, depending on the error number.
	 */

	if ( errorNumber == 0 )
	{
		epToVxSetHealth( pRecordPrefix, "GOOD" );

		initState = CAR_IDLE;
		if (epToVxPipeWrite (NULL, (char *) &initState, pDetInitContext) == ERROR)
		{
			ERROR_LOG ("Failed to set initialisation state to IDLE");
		}
	}
	else
	{
		initState = CAR_ERROR;
		if (epToVxPipeWrite (NULL, (char *) &initState, pDetInitContext) == ERROR)
		{
			ERROR_LOG ("Failed to set initialisation state to ERROR");
		}
	}

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detReset
 *
 *	INVOCATION:
 *	detReset (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId, gotControl)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext	(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber	(int)				Command number
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(>) obsId			(OBS_ID)			Observation context structure
 *	(>)	vmeAddress		(uint32)			VME address of SDSU controller (0=simulate)
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detReset command
 *
 *	DESCRIPTION:
 *	Reset the SDSU controller
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detReset
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId,				/* Observation context structure.			*/
	BOOL			gotControl			/* Does controller have access to SDSU hw?	*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.			*/
	long			resetVme;			/* Flag set to reset SDSU VME interface.	*/
	long			resetCtrl;			/* Flag set to reset SDSU controller.		*/

	uint32			resetMask;			/* Mask specifying what to reset.			*/

	char			pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Path name for file.						*/
	char			pOmfFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* File name.								*/
	char			pFullOmfFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
										/* Combined path name and file name.		*/
	BOOL			limitAdrsRange;		/* Flag for limiting address range			*/

	uint32			mode;

	/*
	 * Initialise the error number and obtain the attributes provided with the command.
	 */

	errorNumber = 0;
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & resetVme);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & resetCtrl);

	/*
	 * The SDSU hardware can only be reset of the controller has access to that hardware.
	 */

	if ( gotControl )
	{

		/*
		 * Check there are valid SDSU and observation context structures.
		 */

		if ( sdsuId == NULL )
		{
			ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
			errorNumber = S_detControl_INTERNAL;
			return (errorNumber);
		}

		if ( obsId == NULL )
		{
			ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
			errorNumber = S_detControl_INTERNAL;
			return (errorNumber);
		}

		/*
		 * The command can only be used when an observation is not in progress.
		 */

		if ( obsId->observing )
		{
			ERROR_SET (S_detControl_BUSY,
				"Observation in progress - abort observation and try again", ERROR_LOG_NOW);
			errorNumber = S_detControl_BUSY;
			return (errorNumber);
		}

		/*
		 * Reset the SDSU hardware.
		 */

		MESSAGE_LOG2 (MSG_LOG, "About to %s SDSU VME interface and %s SDSU controller",
			(resetVme ? "reset" : "NOT reset"), (resetCtrl ? "reset" : "NOT reset"));

		/* Set the appropriate bits in the mask specifying what to reset. */

		resetMask = 0;
		if ( resetVme )  resetMask |= SDSU_RESET_VME;
		if ( resetCtrl ) resetMask |= SDSU_RESET_CONTROLLER;

		if (sdsuReset (sdsuId, resetMask) == ERROR)
		{
			ERROR_LOG ("Error resetting SDSU hardware");
			errorNumber = S_detControl_SDSU_ERROR;
		}

		/* Now obtain the path of the directory containing the DSP code. */

		EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pFilePath);

		/* Determine whether any code should be downloaded to the VME DSP */

		EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, pOmfFileName);

		if ( (errorNumber == 0) && (resetVme) &&
		     (strcmp (pOmfFileName, "") != 0) && (strcmp (pOmfFileName, "NONE") != 0)
		   )
		{

			/*
			 * The ability to limit the address range is ignored. It is rarely needed
			 * and can only be done by executing sdsuFileDnload at the console (since
		 	 * sdsuFileDnload expects to prompt for the values).
			 */

			limitAdrsRange = 0;

			if ( strcmp (pFilePath, "") == 0 )
			{
				strncpy (pFullOmfFileName, pOmfFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
			}
			else
			{
				sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
			}

			MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to VME DSP...",
				pFullOmfFileName);

			if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_VME, limitAdrsRange) == ERROR)
			{
				ERROR_LOG ("Failed to download OMF file to VME DSP");
				errorNumber = S_detControl_SDSU_ERROR;
			}
		}

		/* Determine whether any code should be downloaded to the Timing DSP */

		EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, pOmfFileName);

		if ( (errorNumber == 0) && (resetCtrl) &&
		     (strcmp (pOmfFileName, "") != 0) && (strcmp (pOmfFileName, "NONE") != 0)
		   )
		{

			/*
			 * The ability to limit the address range is ignored. It is rarely needed
			 * and can only be done by executing sdsuFileDnload at the console (since
			 * sdsuFileDnload expects to prompt for the values).
			 */

			limitAdrsRange = 0;
			if ( strcmp (pFilePath, "") == 0 )
			{
				strncpy (pFullOmfFileName, pOmfFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
			}
			else
			{
				sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
			}

			MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to TIMING DSP...", pFullOmfFileName);

			if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_TIM, limitAdrsRange) == ERROR)
			{
				ERROR_LOG ("Failed to download OMF file to TIMING DSP");
				errorNumber = S_detControl_SDSU_ERROR;
			}
		}

		/* Determine whether any code should be downloaded to the Utility DSP */

		EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, pOmfFileName);

		if ( (errorNumber == 0) && (resetCtrl) &&
		     (strcmp (pOmfFileName, "") != 0) && (strcmp (pOmfFileName, "NONE") != 0)
		   )
		{

			/* The ability to limit the address range is ignored. It is rarely needed
			 * and can only be done by executing sdsuFileDnload at the console (since
			 * sdsuFileDnload expects to prompt for the values).
			 */

			limitAdrsRange = 0;

			if ( strcmp (pFilePath, "") == 0 )
			{
				strncpy (pFullOmfFileName, pOmfFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
			}
			else
			{
				sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
			}

			MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to UTILITY DSP...", pFullOmfFileName);

			if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_UTL, limitAdrsRange) == ERROR)
			{
				ERROR_LOG ("Failed to download OMF file to UTILITY DSP");
				errorNumber = S_detControl_SDSU_ERROR;
			}
		}

		if ( (resetCtrl) && (errorNumber == 0) )
		{
			/*
			 * On the HRWFS the default packet size must be increased.
			 * The HRWFS should not be run in sync mode to ensure adequate dynamic range.
			 * (It is not a serious error if this does not happen).
			 *
			 * THIS BLOCK OF CODE CAN BE REMOVED WHEN TIM HARDY'S NEW DSP CODE SETS
			 * APPROPRIATE DEFAULTS.
			 */

			if ( strcmp (pWfsName, "hr") == 0 )
			{
				if (sdsuParamWrite (sdsuId, SDSU_IDENT_VME, "V_PSIZE", 1024) == ERROR)
				{
					ERROR_LOG ("Failed to increase the HRWFS packet size");
				}

				if (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_MODE", &mode) == ERROR)
				{
					ERROR_LOG ("Failed to read default mode from timing board");
				}
				else
				{
					if ( (mode & SDSU_TIM_MODE_SYNC) != 0 )
					{
						mode &= ~SDSU_TIM_MODE_SYNC;
						if (sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_MODE", mode) == ERROR)
						{
							ERROR_LOG ("Failed to clear sync mode on timing board");
						}
					}
				}
			}
			else
			{
				/* The PWFS packet size should be larger when used in full frame mode. */

				if (sdsuParamWrite (sdsuId, SDSU_IDENT_VME, "V_PSIZE", 80) == ERROR)
				{
					ERROR_LOG ("Failed to increase the PWFS packet size");
				}
			}
		}

		/*
		 * After successfully downloading new OMF code, the controller must be reinitialised
		 * by sending an "INI" command to the utility DSP and a "LDP" command to the timing DSP.
		 */

		if ( (resetCtrl) && (errorNumber == 0) )
		{
			if (sdsuPrimitive (sdsuId, "INI", SDSU_IDENT_UTL, NULL, NULL) == ERROR)
			{
				ERROR_LOG ("Failed to initialise UTILITY DSP with INI command");
				errorNumber = S_detControl_SDSU_ERROR;
			}
			if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
			{
				ERROR_LOG ("Failed to initialise TIMING DSP with LDP command");
				errorNumber = S_detControl_SDSU_ERROR;
			}
		}
	}
	else
	{
		MESSAGE_LOG (MSG_LOG, "No access to the SDSU hardware - resetting health only.");
	}

	/*
	 * If no errors have occurred during the reset set the controller health to GOOD.
	 * If the reset failed the controller may be in an unusable state, so set the
	 * health to BAD.
	 */

	if ( errorNumber == 0 )
	{
		epToVxSetHealth( pRecordPrefix, "GOOD" );
	}
	else
	{
		epToVxSetHealth( pRecordPrefix, "BAD" );
	}

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detTest
 *
 *	INVOCATION:
 *	detTest (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId, pTestResultsContext,
 *           gotControl)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName				(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext			(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber			(int)				Command number
 *	(>)	sdsuId					(SDSU_ID)			Current SDSU context structure
 *	(>) obsId					(OBS_ID)			Observation context structure
 *	(>)	pTestResultsContext		(DATREC_CONTECT)	Context structure for test results record
 *	(>)	gotControl				(BOOL)				TRUE if controller has access to SDSU hardware
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detTest command
 *
 *	DESCRIPTION:
 *	Test the SDSU controller
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detTest
	(
	const char *	pWfsName,				/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,			/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,			/* CAD command context structure.			*/
	int				commandNumber,			/* Command number.							*/
	SDSU_ID			sdsuId,					/* SDSU context structure.					*/
	OBS_ID			obsId,					/* Observation context structure.			*/
	DATREC_CONTEXT	pTestResultsContext,	/* Test results context structure.			*/
	BOOL			gotControl				/* Does controller have access to SDSU hw?	*/
	)
{
	uint32			errorNumber;			/* Error number reported by task.			*/

	long			testLevel;				/* Test level.								*/
	long			verbose;				/* Flag for verbose mode.					*/

	uint32			testMask;				/* Mask for types of tests.					*/
	uint32			dspMask;				/* Mask for DSP to be tested.				*/

	char			pTestResults[EPICS_MAX_BYTES_STRING_ATTRIB + 1];
											/* String to contain test results.			*/

	/*
	 * Initialise the error number and obtain the attributes provided with the command.
	 */

	errorNumber = 0;
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & testLevel);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & verbose);

	/*
	 * The controller can only self-test if it has access to the SDSU hardware.
	 * Note having access to the hardware should not be regarded as a test failure,
	 * since all controllers will be tested routinely on startup, and when the
	 * hardware is shared there will always be one controller without access.
	 * Instead issue a warning.
	 */

	if ( !gotControl )
	{
		MESSAGE_LOG (MSG_WARNING, "WARNING: Cannot test SDSU controller - no access to hardware");
		if (epToVxPipeWrite (NULL, "No hardware", pTestResultsContext) == ERROR)
		{
			ERROR_LOG ("Failed to write test results");
		}
		return (errorNumber);
	}

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		if (epToVxPipeWrite (NULL, "Bad SDSU context", pTestResultsContext) == ERROR)
		{
			ERROR_LOG ("Failed to write test results");
		}
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		if (epToVxPipeWrite (NULL, "Bad observation context", pTestResultsContext) == ERROR)
		{
			ERROR_LOG ("Failed to write test results");
		}
		return (errorNumber);
	}

	/*
	 * The command can only be used when an observation is not in progress.
	 */

	if ( obsId->observing )
	{
		ERROR_SET (S_detControl_BUSY, "Observation in progress - abort observation and try again",
			ERROR_LOG_NOW);
		errorNumber = S_detControl_BUSY;
		return (errorNumber);
	}

	MESSAGE_LOG1 (MSG_LOG, "Testing SDSU controller: level=%ld", testLevel);

	/*
	 * Load up the required masks and test the controller.
	 * For now, all DSPs are tested in verbose mode.
	 */

	testMask = 0;

	if ( testLevel > 0 )
		testMask |= SDSU_TEST_LINK;		/* Test integrity of data link to DSP.	*/

	if ( testLevel > 1 )
		testMask |= SDSU_TEST_RDM;		/* Test read access to DSP memory.		*/

	if ( testLevel > 2 )
		testMask |= SDSU_TEST_WRM;		/* Test write access to DSP memory.		*/

	/* Other tests can be added when they are supported by sdsuTest. */

	dspMask = 0x7;						/* Test all three DSPs (bits 0,1,2).	*/

	if ( sdsuTest (sdsuId, (BOOL) verbose, &testMask, dspMask) == ERROR )
	{
		ERROR_LOG ("SDSU controller test failed");
		errorNumber = S_detControl_SDSU_ERROR;

		strncpy (pTestResults, "Tests failed: ", EPICS_MAX_BYTES_STRING_ATTRIB);

		if (testMask & SDSU_TEST_LINK)
		{
			strncat (pTestResults, "TDL ", EPICS_MAX_BYTES_STRING_ATTRIB);
		}
		if (testMask & SDSU_TEST_RDM)
		{
			strncat (pTestResults, "RDM ", EPICS_MAX_BYTES_STRING_ATTRIB);
		}
		if (testMask & SDSU_TEST_WRM)
		{
			strncat (pTestResults, "WRM ", EPICS_MAX_BYTES_STRING_ATTRIB);
		}

		if (epToVxPipeWrite (NULL, pTestResults, pTestResultsContext) == ERROR)
		{
			ERROR_LOG ("Failed to write test results");
		}
	}
	else
	{
		MESSAGE_LOG (MSG_LOG, "Test completed successfully");

		if (epToVxPipeWrite (NULL, "Tested OK", pTestResultsContext) == ERROR)
		{
			ERROR_LOG ("Failed to write test results");
		}
	}

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detGiveUp
 *
 *	INVOCATION:
 *	detGiveUp (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, pSdsuId, obsId, pDetInitStatusContext,
 *	           pGotControl)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName				(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext			(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber			(int)				Command number
 *	(!)	pSdsuId					(SDSU_ID *)			Pointer to current SDSU context structure
 *	(>) obsId					(OBS_ID)			Observation context structure
 *	(>)	pDetInitStatusContext	(DATREC_CONTEXT)	Context structure for init status record
 *	(!)	pGotControl				(BOOL *)			Pointer to gotControl flag
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detGiveUp command
 *
 *	DESCRIPTION:
 *	This function releases SDSU hardware shared through a fibre switch for use
 *	by another detector controller.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detGiveUp
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID *		pSdsuId,			/* Pointer to SDSU context structure.		*/
	OBS_ID			obsId,				/* Observation context structure.			*/
	DATREC_CONTEXT	pDetInitStatusContext,
										/* Context structure for SDSU				*/
										/* initialisation status SIR record.		*/
	BOOL *			pGotControl			/* Pointer to gotControl flag.				*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.			*/
	char 			pStatusString [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Status string.							*/
	/*
	 * Initialise the error number.
	 */

	errorNumber = 0;

	/*
	 * This command is only valid for OIWFS and HRWFS, and only for the WFS which has control.
	 */

	if ( (strcmp (pWfsName, "oi") != 0) && (strcmp (pWfsName, "hr") != 0) )
	{
		ERROR_SET (S_detControl_BAD_COMMAND, "Command only relevant for OIWFS or HRWFS",
			ERROR_LOG_NOW);
		errorNumber = S_detControl_BAD_COMMAND;
		return (errorNumber);
	}
	if ( !(*pGotControl) )
	{
		ERROR_SET (S_detControl_BAD_COMMAND, "WFS does not have control to give up",
			ERROR_LOG_NOW);
		errorNumber = S_detControl_BAD_COMMAND;
		return (errorNumber);
	}

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( *pSdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * The command can only be used when an observation is not in progress.
	 */

	if ( obsId->observing )
	{
		ERROR_SET (S_detControl_BUSY, "Observation in progress - abort observation and try again",
			ERROR_LOG_NOW);
		errorNumber = S_detControl_BUSY;
		return (errorNumber);
	}

	/*
	 * Before releasing the SDSU controller attempt to reset it.
	 * However, it will probably need a hardware reset before new DSP code can be downloaded.
	 */

	if (sdsuReset (*pSdsuId, (SDSU_RESET_VME | SDSU_RESET_CONTROLLER)) == ERROR)
	{
		ERROR_LOG ("Failed to reset SDSU interface and controller");
	}

	/*
	 * Delete the current SDSU context and ensure the SDSU context ID is set back to NULL.
	 */

	if (sdsuContextDelete (*pSdsuId) == ERROR)
	{
		ERROR_SET (0, "Failed to delete old SDSU context structure", ERROR_LOG_NOW);
		errorNumber = S_detControl_SDSU_ERROR;
		return (errorNumber);
	}
	*pSdsuId = NULL;
	if (strcmp (pWfsName, "oi") == 0)
	{
		detSdsuIdOi = NULL;
	}
	else if (strcmp (pWfsName, "hr") == 0)
	{
		detSdsuIdHr = NULL;
	}

	/*
	 * Free the semaphore protecting the SDSU hardware and unset the gotControl flag.
	 * Another detector controller should now be able to initialise and take control.
	 */

	semGive( detSwitchSem );
	*pGotControl = FALSE;

	MESSAGE_LOG (MSG_LOG, "Access to SDSU hardware given up");
	MESSAGE_LOG (MSG_WARNING, "NOTE: SDSU controller may need hardware reset before reinitialising");

	/* Write a new string to the SDSU initialisation status SIR record. */

	strncpy (pStatusString, "NO ACCESS THROUGH SDSU H/W SWITCH", EPICS_MAX_BYTES_STRING_ATTRIB);

	if (epToVxPipeWrite (NULL, pStatusString, pDetInitStatusContext) == ERROR)
	{
		ERROR_LOG ("Failed to write init message to SDSU status pipe.");
	}

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detSave
 *
 *	INVOCATION:
 *	detSave (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext	(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber	(int)				Command number
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(>) obsId			(OBS_ID)			Observation context structure
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detSave command
 *
 *	DESCRIPTION:
 *	This function uploads SDSU parameters to a file.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detSave
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId				/* Observation context structure.			*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.			*/

	long			destId;				/* Destination DSP ID.						*/

	char			pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Path name for file.								*/
	char			pParamFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Name of file to contain SDSU parameter values.	*/
	char			pFullParamFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
										/* Combined path name and file name.				*/

	/*
	 * Initialise the error number and obtain the attributes provided with the command.
	 */

	errorNumber = 0;
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pFilePath);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pParamFileName);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *) & destId);

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * The command can only be used when an observation is not in progress.
	 */

	if ( obsId->observing )
	{
		ERROR_SET (S_detControl_BUSY, "Observation in progress - abort observation and try again",
			ERROR_LOG_NOW);
		errorNumber = S_detControl_BUSY;
		return (errorNumber);
	}

	/*
	 * Combine the path and file names together, and append the string ".par" to the file name
	 * if it is not already present. Ignore the path if not specified.
	 */

	if ( strcmp (pFilePath, "") == 0 )
	{
		strncpy (pFullParamFileName, pParamFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
	}
	else
	{
		sprintf (pFullParamFileName, "%s/%s", pFilePath, pParamFileName );
	}

	if (strstr (pFullParamFileName, ".par") == NULL)
		strncat (pFullParamFileName, ".par", EPICS_MAX_BYTES_STRING_ATTRIB);

	/*
	 * Upload SDSU parameters from the specified DSP to the specified file.
	 * (If the DSP is specified as "-1" all the known SDSU parameters will be uploaded to
	 * the file).
	 */

	MESSAGE_LOG1 (MSG_LOG, "Uploading SDSU parameters to %s", pFullParamFileName);

	if ( sdsuParamUpload( sdsuId, pFullParamFileName, (uint32) destId) == ERROR )
	{
		ERROR_LOG ("Failed to upload SDSU parameters");
		errorNumber = S_detControl_SDSU_ERROR;
	}

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detGeometry
 *
 *	INVOCATION:
 *	detGeometry (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId, pxPixels, pyYpixels)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext	(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber	(int)				Command number
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(!) obsId			(OBS_ID)			Observation context structure
 *	(<) pxPixels		(int *)				Pointer to number of X pixels
 *	(<) pyPixels		(int *)				Pointer to number of Y pixels
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detGeometry command
 *
 *	DESCRIPTION:
 *	This function sets up the SDSU detector geometry parameters.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	I have found out that it may not be possible to read parameters from the timing board
 *	while on observation is taking place. To use this function to set parameters on-the-fly
 *	during an observation a redesign may be necessary where the timing board parameters are
 *	read and saved before the observation starts. SMB - 21 Jan 1999.
 *-
 */

uint32 detGeometry
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId,				/* Observation context structure.			*/
	int *			pxPixels,			/* Pointer to current number of X pixels.	*/
	int *			pyPixels			/* Pointer to current number of Y pixels.	*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.			*/

	/* 
	 * Variables associated with "Set detector readout geometry and binning mode" command.
	 */

	long			xPixels;			/* Number of X pixels in digitised image (AC only).			*/
	long			yPixels;			/* Number of Y pixels in digitised image (AC only).			*/
	long			xSubap;				/* Number of subapertures per sector in X direction			*/
										/* (WFS only).												*/
	long			ySubap;				/* Number of subapertures per seector in Y direction		*/
										/* (WFS only).												*/
	long			xBin;				/* X binning factor (pixels per superpixel) (AC or WFS).	*/
	long			yBin;				/* Y binning factor (pixels per superpixel) (AC or WFS).	*/
	long			xRas;				/* Size of each subaperture in X direction in super-pixels	*/
										/* (WFS only).												*/
	long			yRas;				/* Size of each subaperture in Y direction in super-pixels	*/
										/* (WFS only).												*/
	long			xSpace;				/* Spacing between subapertures in X direction in pixels	*/
										/* (WFS only).												*/
	long			ySpace;				/* Spacing between subapertures in Y direction in pixels	*/
										/* (WFS only).												*/
	long			xStart;				/* X offset from bottom left corner of array in pixels.		*/
										/* (AC or WFS).												*/
	long			yStart;				/* Y offset from bottom left corner of array in pixels.		*/
										/* (AC or WFS).												*/

	long			xTail;				/* Number of trailing X pixels to be discarded on each row.	*/
	long			xSize;				/* X size of each sector in pixels.							*/
	long			ySize;				/* Y size of each sector in pixels.							*/
	long			xTotalSize;			/* Total X size in pixels.									*/
	long			yTotalSize;			/* Total Y size in pixels.									*/
	long			outputs;			/* Number of detector outputs = number of sectors.			*/
	long			uscan;				/* Number of underscan pixels.								*/
	long			nPixels;			/* Total number of digitised pixels.						*/
	long			packetSize;			/* Packet size in pixels.									*/
	int				nPackets;			/* Number of packets expected per frame.					*/
	int				fullFrameFlag;		/* Set to 1 when the SDSU controller is reading out full	*/
										/* frames and 0 when it is not.								*/ 

	/*
	 * Initialise the error number and obtain the attributes provided with the command.
	 */

	errorNumber = 0;
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & xSubap);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & ySubap);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *) & xRas);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *) & yRas);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, (char *) & xBin);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, (char *) & yBin);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6, (char *) & xStart);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 7, (char *) & yStart);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 8, (char *) & xSpace);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 9, (char *) & ySpace);

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * The command can be used when an observation is in progress, as it only redefines
	 * "on-the-fly" parameters. However, warn the user this is happening.
	 */

	if ( obsId->observing )
	{
		MESSAGE_LOG (MSG_WARNING,
			"NOTE: Changing on-the-fly parameters while observation in progress.");
	}

	/*
	 * The number of subapertures per sector must be positive and non-zero.
	 */

	if ( (xSubap < 1) || (ySubap < 1) )
	{
		ERROR_SET2 (S_detControl_BAD_ATTRIBUTE, "Invalid number of subapertures: %ld x %ld",
			ERROR_LOG_NOW, xSubap, ySubap);
		errorNumber = S_detControl_BAD_ATTRIBUTE;
		return (errorNumber);
	}

	/*
	 * The binning factors must be positive and non zero.
	 */

	if ( (xBin < 1) || (yBin < 1) )
	{
		ERROR_SET2 (S_detControl_BAD_ATTRIBUTE, "Invalid binning factors: %ld, %ld",
			ERROR_LOG_NOW, xBin, yBin);
		errorNumber = S_detControl_BAD_ATTRIBUTE;
		return (errorNumber);
	}

	/*
	 * Read the outputs and uscan parameters from the SDSU controller, unless
	 * the controller is being simulated (in which case default values for the particular
	 * wavefront sensor are used).
	 */

	if ( !sdsuId->simulate )
	{
		if ( (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XSIZE", (uint32 *) & xSize) == ERROR) ||
		     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YSIZE", (uint32 *) & ySize) == ERROR) ||
		     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_OUTPUTS", (uint32 *) & outputs) == ERROR) ||
		     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_USCAN", (uint32 *) & uscan) == ERROR) ||
		     (sdsuParamRead (sdsuId, SDSU_IDENT_VME, "V_PSIZE", (uint32 *) & packetSize) == ERROR)
		   )
		{
			ERROR_LOG (
				"Failed to read T_XSIZE, T_YSIZE, T_OUTPUTS, T_USCAN, V_PSIZE from SDSU controller");
			errorNumber = S_detControl_SDSU_ERROR;
			return (errorNumber);
		}

		if ( outputs == 2 )
		{
			xTotalSize = xSize * 2;
			yTotalSize = ySize;
		}
		else
		{
			xTotalSize = xSize * 2;
			yTotalSize = ySize * 2;
		}
	}
	else
	{
		if (strcmp (pWfsName, "p1") == 0)
		{
			xTotalSize = DET_CONTROL_PWFS1_XSIZE;
			yTotalSize = DET_CONTROL_PWFS1_YSIZE;
			xSize = xTotalSize / 2;
			ySize = yTotalSize / 2;
			outputs = 4;
			uscan = 4;
		}
		else if (strcmp (pWfsName, "p2") == 0)
		{
			xTotalSize = DET_CONTROL_PWFS2_XSIZE;
			yTotalSize = DET_CONTROL_PWFS2_YSIZE;
			xSize = xTotalSize / 2;
			ySize = yTotalSize / 2;
			outputs = 4;
			uscan = 4;
		}
		else if (strcmp (pWfsName, "oi") == 0)
		{
			xTotalSize = DET_CONTROL_OIWFS_XSIZE;
			yTotalSize = DET_CONTROL_OIWFS_YSIZE;

			xSize = xTotalSize / 2;
			ySize = yTotalSize / 2;
			outputs = 4;
			uscan = 4;
		}
		else if (strcmp (pWfsName, "hr") == 0)
		{
			xTotalSize = DET_CONTROL_HRWFS_XSIZE;
			yTotalSize = DET_CONTROL_HRWFS_YSIZE;
			xSize = xTotalSize / 2;
			ySize = yTotalSize;
			outputs = 2;
			uscan = 8;
		}
	}

	/*
	 * Calculate the number of digitized pixels per frame, and update the current number
	 * of X and Y pixels. Also calculate the number of packets into which these pixels
	 * will fit. (The ceil function is used because the number of packets is always
	 * rounded up to the nearest integer).
	 */

	if ( outputs == 2 )
	{
		xPixels = xSubap * xRas * 2;
		yPixels = ySubap * yRas;
	}
	else
	{
		xPixels = xSubap * xRas * 2;
		yPixels = ySubap * yRas * 2;
	}

	*pxPixels = xPixels;
	*pyPixels = yPixels;
	nPixels = xPixels * yPixels;

	if ( (!sdsuId->simulate) && (packetSize > 0) )
	{
		nPackets = (int) ceil ( (double) (nPixels * outputs) / (double) packetSize );
	}
	else
	{
		nPackets = 1;
	}

	/*
	 * Determine whether the given parameters will put the detector controller
	 * into full frame mode. This happens when the there is one subaperture per
	 * output and the subapertures fill the detector surface without any gaps.
	 */

	if ( (xSubap == 1) && (ySubap == 1) && (xStart == 0) && (yStart == 0) &&
	     (xSpace == 0) && (ySpace == 0)
	   )
	{
		fullFrameFlag = 1;
	}
	else
	{
		fullFrameFlag = 0;
	}


	/*
	 * Calculate the number of trailing X pixels. This is required by the DSP code as a check.
	 * If the value is negative then the subapertures span the boundary between outputs (not
	 * physically possible), and xTail should be set zero.
	 *
	 * xTail is the number of pixels that need to be discarded at the end of each row,
	 * and is calculated by starting with the total number of pixels to read (xSize) and
	 * subtracting off the pixels that are read out and/or discarded during a readout.
	 */

	xTail = xSize - (((xRas * xBin) + xSpace) * xSubap) - xSpace - xStart - uscan;
	if ( xTail < 0 )
	{
		MESSAGE_LOG1 (MSG_WARNING, "Xtail is %ld. Should not be less than zero. Reset to zero", xTail);
		xTail = 0;
	}

	MESSAGE_LOG1 (MSG_LOG, "Setting new detector geometry (%s frame mode)",
		(fullFrameFlag ? "full":"reduced"));

	MESSAGE_LOG4 (MSG_MINDEBUG, "XSIZE=%ld, YSIZE=%ld, XPIXELS=%d, YPIXELS=%d",
		xSize, ySize, *pxPixels, *pyPixels);
	MESSAGE_LOG4 (MSG_MINDEBUG, "XSUBAP=%ld, YSUBAP=%ld, XBIN=%ld, YBIN=%ld",
		xSubap, ySubap, xBin, yBin);
	MESSAGE_LOG4 (MSG_MINDEBUG, "XRAS=%ld, YRAS=%ld, XSPACE=%ld, YSPACE=%ld",
		xRas, yRas, xSpace, ySpace);
	MESSAGE_LOG4 (MSG_MINDEBUG, "XSTART=%ld, YSTART=%ld, XTAIL=%ld, NPIXELS=%ld",
		xStart, yStart, xTail, nPixels);

	MESSAGE_LOG2 (MSG_FULLDEBUG, "Each frame will consist of %d packets of %ld pixels each",
		nPackets, packetSize);

	/*
	 * Update the geometry parameters in the SDSU timing DSP. These are all "on-the-fly" parameters
	 * and need to be downloaded with sdsuParamWRP() and activated by sending a "LDP" command.
	 */

	if ( (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSUBAP", (uint32) xSubap) == ERROR) ||
	     (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSUBAP", (uint32) ySubap) == ERROR) ||
	     (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSTART", (uint32) xStart) == ERROR) ||
	     (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSTART", (uint32) yStart) == ERROR) ||
	     (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XRAS",   (uint32) xRas) == ERROR) ||
	     (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YRAS",   (uint32) yRas) == ERROR) ||
	     (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSPACE", (uint32) xSpace) == ERROR) ||
	     (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSPACE", (uint32) ySpace) == ERROR) ||
	     (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XBIN",   (uint32) xBin) == ERROR) ||
	     (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YBIN",   (uint32) yBin) == ERROR) ||
	     (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XTAIL",  (uint32) xTail) == ERROR) ||
	     (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_NPIXEL", (uint32) nPixels) == ERROR)
	  )
	{
		ERROR_LOG ("Failed to download geometry parameters to TIMING DSP");
		errorNumber = S_detControl_SDSU_ERROR;
		return (errorNumber);
	}

	if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
	{
		ERROR_LOG ("Failed to activate TIMING DSP parameters with LDP command");
		errorNumber = S_detControl_SDSU_ERROR;
		return (errorNumber);
	}

	/*
	 * Update the number of packets per frame in the SDSU context structure.
	 */

	sdsuId->packetsPerFrame = nPackets;

	/*
	 * If signal processing is active for this WFS, update the geometry parameters supplied to the
	 * signal processing software.
	 */

	if ( ((obsId->ospFGContext != NULL) || (obsId->ospAOContext)) && (obsId->ospGeometry != NULL) )
	{

		obsId->ospGeometry->sectors =		(int) outputs;
		obsId->ospGeometry->xstart =		(int) xStart;
		obsId->ospGeometry->ystart =		(int) yStart;
		obsId->ospGeometry->xbin =			(int) xBin;
		obsId->ospGeometry->ybin =			(int) yBin;
		obsId->ospGeometry->xraster =		(int) xRas;
		obsId->ospGeometry->yraster =		(int) yRas;
		obsId->ospGeometry->xspace =		(int) xSpace;
		obsId->ospGeometry->yspace =		(int) ySpace;
		obsId->ospGeometry->xsubap =		(int) xSubap;
		obsId->ospGeometry->ysubap =		(int) ySubap;
		obsId->ospGeometry->xarraysize =	80;	/* WAS (int) xTotalSize; - include USCAN */
		obsId->ospGeometry->yarraysize =	80;	/* WAS (int) yTotalSize; - include USCAN */
		obsId->ospGeometry->framesizeflag =	fullFrameFlag;

		if (obsId->ospFGContext != NULL) 
		{
			if ( ospChangeGeometry (obsId->ospGeometry, obsId->ospFGContext) == ERROR )
		    {
				ERROR_LOG ("Failed to update signal processing geometry parameters");
				errorNumber = S_detControl_INTERNAL;
				return (errorNumber);
		    }
		};
		if (obsId->ospAOContext != NULL) 
		{
			if ( ospChangeGeometry (obsId->ospGeometry, obsId->ospAOContext) == ERROR )
		    {
				ERROR_LOG ("Failed to update signal processing geometry parameters");
				errorNumber = S_detControl_INTERNAL;
				return (errorNumber);
		    }
		};
	}

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detDownload
 *
 *	INVOCATION:
 *	detDownload (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext	(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber	(int)				Command number
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(>) obsId			(OBS_ID)			Observation context structure
 *	(>)	vmeAddress		(uint32)			VME address of SDSU controller (0=simulate)
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detDownload command
 *
 *	DESCRIPTION:
 *	This function downloads DSP code from OMF files.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detDownload
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId				/* Observation context structure.			*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.			*/

	char			pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Path name for file.						*/
	char			pOmfFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* File name.								*/
	char			pFullOmfFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
										/* Combined path name and file name.		*/

	BOOL			limitAdrsRange;		/* Flag for limiting address range in DSP memory */

	uint32			mode;

	/*
	 * Initialise the error number.
	 */

	errorNumber = 0;

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * The command can only be used when an observation is not in progress.
	 */

	if ( obsId->observing )
	{
		ERROR_SET (S_detControl_BUSY, "Observation in progress - abort observation and try again",
			ERROR_LOG_NOW);
		errorNumber = S_detControl_BUSY;
		return (errorNumber);
	}

	/* First obtain the path of the directory containing the DSP code. */

	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pFilePath);


	/* Determine whether any code should be downloaded to the VME DSP */

	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pOmfFileName);

	if ( (strcmp (pOmfFileName, "") != 0) && (strcmp (pOmfFileName, "NONE") != 0) )
	{

		/* The ability to limit the address range is ignored. It is rarely needed
		 * and can only be done by executing sdsuFileDnload at the console (since
	 	 * sdsuFileDnload expects to prompt for the values).
		 */

		limitAdrsRange = 0;

		if ( strcmp (pFilePath, "") == 0 )
		{
			strncpy (pFullOmfFileName, pOmfFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
		}
		else
		{
			sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
		}

		MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to VME DSP...",
		              pFullOmfFileName);

		if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_VME, limitAdrsRange) == ERROR)
		{
			ERROR_LOG ("Failed to download OMF file to VME DSP");
			errorNumber = S_detControl_SDSU_ERROR;
			epToVxSetHealth( pRecordPrefix, "WARNING" );
		}
	}

	/* Determine whether any code should be downloaded to the Timing DSP */

	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pOmfFileName);

	if ( (strcmp (pOmfFileName, "") != 0) && (strcmp (pOmfFileName, "NONE") != 0) )
	{

		/* The ability to limit the address range is ignored. It is rarely needed
		 * and can only be done by executing sdsuFileDnload at the console (since
		 * sdsuFileDnload expects to prompt for the values).
		 */

		limitAdrsRange = 0;

		if ( strcmp (pFilePath, "") == 0 )
		{
			strncpy (pFullOmfFileName, pOmfFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
		}
		else
		{
			sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
		}

		MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to TIMING DSP...",
		              pFullOmfFileName);

		if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_TIM, limitAdrsRange) == ERROR)
		{
			ERROR_LOG ("Failed to download OMF file to TIMING DSP");
			errorNumber = S_detControl_SDSU_ERROR;
			epToVxSetHealth( pRecordPrefix, "WARNING" );
		}
	}

	/* Determine whether any code should be downloaded to the Utility DSP */

	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, pOmfFileName);

	if ( (strcmp (pOmfFileName, "") != 0) && (strcmp (pOmfFileName, "NONE") != 0) )
	{

		/* The ability to limit the address range is ignored. It is rarely needed
		 * and can only be done by executing sdsuFileDnload at the console (since
		 * sdsuFileDnload expects to prompt for the values).
		 */

		limitAdrsRange = 0;

		if ( strcmp (pFilePath, "") == 0 )
		{
			strncpy (pFullOmfFileName, pOmfFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
		}
		else
		{
			sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
		}

		MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to UTILITY DSP...",
		              pFullOmfFileName);

		if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_UTL, limitAdrsRange) == ERROR)
		{
			ERROR_LOG ("Failed to download OMF file to UTILITY DSP");
			errorNumber = S_detControl_SDSU_ERROR;
			epToVxSetHealth( pRecordPrefix, "WARNING" );
		}
	}

	if ( errorNumber == 0 )
	{
		/*
		 * On the HRWFS the default packet size must be increased.
		 * The HRWFS should not run in sync mode to ensure adequate dymanic range.
		 * (It is not a serious error if this does not happen).
		 *
		 * THIS BLOCK OF CODE CAN BE REMOVED WHEN TIM HARDY'S NEW DSP CODE SETS
		 * APPROPRIATE DEFAULTS.
		 */

		if ( strcmp (pWfsName, "hr") == 0 )
		{
			if (sdsuParamWrite (sdsuId, SDSU_IDENT_VME, "V_PSIZE", 1024) == ERROR)
			{
				ERROR_LOG ("Failed to increase the HRWFS packet size");
			}

			if (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_MODE", &mode) == ERROR)
			{
				ERROR_LOG ("Failed to read default mode from timing board");
			}
			else
			{
				if ( (mode & SDSU_TIM_MODE_SYNC) != 0 )
				{
					mode &= ~SDSU_TIM_MODE_SYNC;
					if (sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_MODE", mode) == ERROR)
					{
						ERROR_LOG ("Failed to clear sync mode on timing board");
					}
				}
			}
		}
		else
		{
			/* The default PWFS packet size should be larger when used in full frame mode. */

			if (sdsuParamWrite (sdsuId, SDSU_IDENT_VME, "V_PSIZE", 80) == ERROR)
			{
				ERROR_LOG ("Failed to increase the PWFS packet size");
			}
		}
	}

	/*
	 * After successfully downloading new OMF code, the controller must be reinitialised
	 * by sending an "INI" command to the utility DSP and a "LDP" command to the timing DSP.
	 */

	if ( errorNumber == 0 )
	{
		if (sdsuPrimitive (sdsuId, "INI", SDSU_IDENT_UTL, NULL, NULL) == ERROR)
		{
			ERROR_LOG ("Failed to initialise UTILITY DSP with INI command");
			errorNumber = S_detControl_SDSU_ERROR;
			epToVxSetHealth( pRecordPrefix, "WARNING" );
		}
		if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
		{
			ERROR_LOG ("Failed to initialise TIMING DSP with LDP command");
			errorNumber = S_detControl_SDSU_ERROR;
			epToVxSetHealth( pRecordPrefix, "WARNING" );
		}
	}

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detPrimitive
 *
 *	INVOCATION:
 *	detPrimitive (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId, pDetPrimReplyContext)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName				(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext			(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber			(int)				Command number
 *	(>)	sdsuId					(SDSU_ID)			Current SDSU context structure
 *	(>) obsId					(OBS_ID)			Observation context structure
 *	(>)	pDetPrimReplyContext	(DATREC_CONTEXT)	Content structure for prim reply record
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detPrimitive command
 *
 *	DESCRIPTION:
 *	This function sets up the SDSU exposure parameters.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detPrimitive
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId,				/* Observation context structure.			*/
	DATREC_CONTEXT	pDetPrimReplyContext
										/* Context structure for SDSU primitive		*/
										/* reply string SIR record.					*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.			*/

	long			destId;				/* Destination DSP ID.						*/

	long 			pCmdArg [6] = {0, 0, 0, 0, 0, 0};
										/* Primitive command arguments.				*/
	long 			pRepArg [3] = {0, 0, 0};
										/* Primitive command reply arguments.		*/

	char 			pStringAttrib [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Contents of general string attribute.	*/

	uint32			i;					/* Loop counter.							*/

	/*
	 * Initialise the error number.
	 */

	errorNumber = 0;

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * Some primitive commands can be accepted while an observation is in progress.
	 * However, warn the user if there is an observation in progress.
	 */

	if ( obsId->observing )
	{
		MESSAGE_LOG (MSG_WARNING, "NOTE: Issuing primitive command while observation in progress.");
	}

	/*
	 * Get the command name, destination DSP and up to 6 command arguments
	 * from the attributes supplied with the CAD command.
	 */

	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pStringAttrib);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & destId);

	for (i = 0; i < 6; i++)
	{
		EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, i + 2, (char *) & pCmdArg [i]);
	}

	MESSAGE_LOG4 (MSG_LOG, "About to execute SDSU primitive command \"%s\" to %ld with %#lx ... %#lx",
	              pStringAttrib, destId, pCmdArg[0], pCmdArg[5]);

	/*
	 * Issue the primitive command to the SDSU controller.
	 */

	if (sdsuPrimitive (sdsuId, pStringAttrib, (uint32) destId, (uint32 *) pCmdArg,
	    (uint32 *) pRepArg) == ERROR)
	{
		ERROR_LOG ("Failed to execute SDSU primitive command");
		errorNumber = S_detControl_SDSU_ERROR;
	}

	/* Write the reply to the SDSU primitive reply SIR record. */	

	sprintf (pStringAttrib, "0x%08lx 0x%08lx 0x%08lx", pRepArg [0], pRepArg [1], pRepArg [2]);
	if (epToVxPipeWrite (NULL, pStringAttrib, pDetPrimReplyContext) == ERROR)
	{
		ERROR_LOG ("Failed to write message to SDSU primitive reply pipe.");
		if ( errorNumber == 0 ) errorNumber = (uint32) errnoGet();
	}

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detMode
 *
 *	INVOCATION:
 *	detMode (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext	(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber	(int)				Command number
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(>) obsId			(OBS_ID)			Observation context structure
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detMode command
 *
 *	DESCRIPTION:
 *	This function sets up the SDSU readout mode.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detMode
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId				/* Observation context structure.			*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.			*/

	long			mode;				/* Readout mode parameter.						*/
	long			samples;			/* Number of samples per frame.					*/
	long			tInt;				/* CDI integration time.						*/
	long			gainSp;				/* Combined amplifier gain and integrator speed.*/

	BOOL			ccdFailed = FALSE;	/* Set TRUE if CCD parameter setup fails.		*/
	BOOL			irFailed = FALSE;	/* Set TRUE if CCD parameter setup fails.		*/

	/*
	 * Initialise the error number and obtain the attributes provided with the command.
	 */

	errorNumber = 0;
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & mode);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & tInt);		/* CCD only */
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *) & gainSp);		/* CCD only */
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *) & samples);	/* IR only */

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * The command can be used when an observation is in progress, as it only redefines
	 * "on-the-fly" parameters. However, warn the user this is happening.
	 */

	if ( obsId->observing )
	{
		MESSAGE_LOG (MSG_WARNING,
			"NOTE: Changing on-the-fly parameters while observation in progress.");
	}

	MESSAGE_LOG4 (MSG_LOG, "Defining new readout mode: %#lx %#lx %#lx %#lx",
		mode, tInt, gainSp, samples);

	/*
	 * Set the readout mode by writing the appropriate SDSU parameters. All are on-the-fly
	 * parameters, except GAIN_SP, and need to be downloaded with sdsuParamWRP() and
	 * activated by sending a "LDP" command. GAIN_SP cannot be changed if an observation is
	 * in progress, and must be updated separately with sdsuParamWrite().
	 *
	 * If any parameter is defined as -1 it is not changed.
	 *
	 * Only the T_MODE parameter is universal. The T_INT_TIM and T_GAIN_SP parameters are valid
	 * for CCD detectors only, and T_SAMPLES is valid for IR detectors only. An error is
	 * only reported if an attempt to write both the CCD and IR parameters fails.
	 */

	if ( mode != -1 )
	{
		if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_MODE", (uint32) mode ) == ERROR )
		{
			ERROR_LOG ("Error setting readout mode parameter");
			errorNumber = S_detControl_SDSU_ERROR;
			return (errorNumber);
		}
	}

	if ( tInt != -1 )
	{
		if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_INT_TIM", (uint32) tInt ) == ERROR )
		{
			ccdFailed = TRUE;
		}
	}

	if ( samples != -1 )
	{
		if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_SAMPLES", (uint32) samples ) == ERROR )
		{
			irFailed = TRUE;
		}
	}

	if ( (!obsId->observing) && (gainSp != -1) )
	{
		if ( sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_GAIN_SP", (uint32) gainSp ) == ERROR )
		{
			ccdFailed = TRUE;
		}
	}
	else if ( obsId->observing )
	{
		MESSAGE_LOG (MSG_WARNING, "T_GAIN_SP parameter not changed while observing");
	}

	if ( ccdFailed && irFailed )
	{
		ERROR_LOG ("Error setting readout configuration parameters");
		errorNumber = S_detControl_SDSU_ERROR;
		return (errorNumber);
	}

	if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
	{
		ERROR_LOG ("Failed to activate TIMING DSP parameters with LDP command");
		errorNumber = S_detControl_SDSU_ERROR;
		return (errorNumber);
	}

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detOffset
 *
 *	INVOCATION:
 *	detOffset (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext	(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber	(int)				Command number
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(>) obsId			(OBS_ID)			Observation context structure
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detMode command
 *
 *	DESCRIPTION:
 *	This function sets up the SDSU readout mode.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detOffset
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId				/* Observation context structure.			*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.			*/

	long			offset0;			/* ADC offset for output 0.					*/
	long			offset1;			/* ADC offset for output 1.					*/
	long			offset2;			/* ADC offset for output 2.					*/
	long			offset3;			/* ADC offset for output 3.					*/

	/*
	 * Initialise the error number and obtain the attributes provided with the command.
	 */

	errorNumber = 0;
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & offset0);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & offset1);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *) & offset2);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *) & offset3);

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * The command can be used when an observation is in progress, as it only redefines
	 * "on-the-fly" parameters. However, warn the user this is happening.
	 */

	if ( obsId->observing )
	{
		MESSAGE_LOG (MSG_WARNING,
			"NOTE: Changing on-the-fly parameters while observation in progress.");
	}

	MESSAGE_LOG4 (MSG_LOG, "Defining new ADC offset levels: %#lx %#lx %#lx %#lx",
		offset0, offset1, offset2, offset3);

	/*
	 * Set the offsets by writing the appropriate SDSU parameters.
	 * If any parameter is defined as -1 it is not changed.
	 */

	if ( offset0 != -1 )
	{
		if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS0", (uint32) offset0 ) == ERROR )
		{
			ERROR_LOG ("Error setting ADC offset 0 parameter");
			errorNumber = S_detControl_SDSU_ERROR;
			return (errorNumber);
		}
	}

	if ( offset1 != -1 )
	{
		if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS1", (uint32) offset1 ) == ERROR )
		{
			ERROR_LOG ("Error setting ADC offset 1 parameter");
			errorNumber = S_detControl_SDSU_ERROR;
			return (errorNumber);
		}
	}

	if ( offset2 != -1 )
	{
		if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS2", (uint32) offset2 ) == ERROR )
		{
			ERROR_LOG ("Error setting ADC offset 2 parameter");
			errorNumber = S_detControl_SDSU_ERROR;
			return (errorNumber);
		}
	}

	if ( offset3 != -1 )
	{
		if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS3", (uint32) offset3 ) == ERROR )
		{
			ERROR_LOG ("Error setting ADC offset 3 parameter");
			errorNumber = S_detControl_SDSU_ERROR;
			return (errorNumber);
		}
	}



	if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
	{
		ERROR_LOG ("Failed to activate TIMING DSP parameters with LDP command");
		errorNumber = S_detControl_SDSU_ERROR;
		return (errorNumber);
	}

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detTemp
 *
 *	INVOCATION:
 *	detTemp (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext	(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber	(int)				Command number
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(>) obsId			(OBS_ID)			Observation context structure
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detTemp command
 *
 *	DESCRIPTION:
 *	This function sets up the SDSU exposure parameters.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	Assumes SDSU_TEMP_UNIT is not zero.
 *-
 */

uint32 detTemp
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.			*/
	int				commandNumber,		/* Command number.							*/
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	OBS_ID			obsId				/* Observation context structure.			*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.			*/

	/* 
	 * Variables associated with the "set detector temperature parameters" command.
	 */

	double			tempTarget;			/* Target temperature in Celsius.			*/
	uint32			tempCode;			/* Target temperature code.					*/
	long			tempCoeff;			/* Coefficient for temperature control.		*/

	/*
	 * Initialise the error number and obtain the attributes provided with the command.
	 */

	errorNumber = 0;
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & tempTarget);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & tempCoeff);

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * The command can only be used when an observation is not in progress.
	 */

	if ( obsId->observing )
	{
		ERROR_SET (S_detControl_BUSY, "Observation in progress - abort observation and try again",
			ERROR_LOG_NOW);
		errorNumber = S_detControl_BUSY;
		return (errorNumber);
	}

	if ( tempTarget <= 0.0 )
	{
		tempCode = (uint32) ((SDSU_TEMP_BASE - tempTarget) / SDSU_TEMP_UNIT);
		tempCode &= 0xfff;		/* Truncate to 0xfff (which is the maximum allowed) */
	}
	else
	{
		/* Switch off cooling altogether for temperatures above 0C. */
		tempCode = 0;
	}

	MESSAGE_LOG2 (MSG_LOG, "Defining temperature control parameters: %#lx %#lx",
		tempCode, (uint32) tempCoeff);

	/*
	 * Write the temperature control parameters to the SDSU controller.
	 */

	if ( (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_CCDT_TGT", tempCode ) == ERROR) ||
	     (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_TCF", (uint32) tempCoeff ) == ERROR)
	   )
	{
		ERROR_LOG ("Error setting temperasture control parameters");
		errorNumber = S_detControl_SDSU_ERROR;
		return (errorNumber);
	}

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detSigInit
 *
 *	INVOCATION:
 *	detSigInit (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext	(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber	(int)				Command number
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(>) obsId			(OBS_ID)			Observation context structure
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detSigInit command
 *
 *	DESCRIPTION:
 *	This function initialises the signal processing.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detSigInit
	(
	const char *	pWfsName,			/* Name of wavefront sensor.						*/
	const char *	pRecordPrefix,		/* Record name prefix.								*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.					*/
	int				commandNumber,		/* Command number.									*/
	SDSU_ID			sdsuId,				/* SDSU context structure.							*/
	OBS_ID			obsId				/* Observation context structure.					*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.					*/

	char			pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* Path name for files.								*/
	char			pIniFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* OSP initialisation file name 					*/
	char			pTmpFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* OSP second file name (ignored for now). 			*/
	char			pFullIniFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
	struct OSP_CONTEXT     
                    *ospContext;		/* Temporary OSP CONTEXT structure pointer          */

	/*
	 * Initialise the error number and get the attributes provided with this command.
	 */

	errorNumber = 0;
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pFilePath);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pIniFileName);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pTmpFileName);

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * The command can only be used when an observation is not in progress.
	 */

	if ( obsId->observing )
	{
		ERROR_SET (S_detControl_BUSY, "Observation in progress - abort observation and try again",
			ERROR_LOG_NOW);
		errorNumber = S_detControl_BUSY;
		return (errorNumber);
	}

	/*
	 * Combine the path and file names together, and append the string ".ini" to the file name
	 * if it is not already present. Ignore the file path if not provided.
	 */

	if ( strcmp (pFilePath, "") == 0 )
	{
		strncpy (pFullIniFileName, pIniFileName, EPICS_MAX_BYTES_STRING_ATTRIB);
	}
	else
	{
		sprintf (pFullIniFileName, "%s/%s", pFilePath, pIniFileName);
	}

	if (strstr (pFullIniFileName, ".ini") == NULL)
		strncat (pFullIniFileName, ".ini", EPICS_MAX_BYTES_STRING_ATTRIB);


	MESSAGE_LOG1 (MSG_LOG, "Initialising signal processing from initialisation file %s",
		pFullIniFileName);

	/*
	 * Create a new signal processing context structure.
	 */

	if ( strcmp (pWfsName, "hr") == 0 )
    {
        if ( obsId->ospHRContext != NULL )
 		{
			if ( ospTidyUpHr (obsId->ospHRContext) == ERROR )
			{
				ERROR_SET (0, "Failed to delete old OSP HR context", ERROR_LOG_NOW);
				errorNumber = S_detControl_INTERNAL;
				return (errorNumber);
			}
		}

		obsId->ospHRContext = ospInitHr (pFullIniFileName);
		if ( obsId->ospHRContext == NULL )
		{
			ERROR_SET (0, "Failed to create new OSP HR context", ERROR_LOG_NOW);
			errorNumber = S_detControl_INTERNAL;
			return (errorNumber);
		}
	}
    else
    {
		ospContext = ospInit (pFullIniFileName, obsId->ospGeometry);
		if ( ospContext == NULL )
		{
			ERROR_SET (0, "Failed to create new OSP context", ERROR_LOG_NOW);
			errorNumber = S_detControl_INTERNAL;
			return (errorNumber);
		}

    	switch (ospContext->wfsMode)
    	{ 
			case FG : if ( obsId->ospFGContext != NULL )
 				 	  {
						if ( ospTidyUp (obsId->ospFGContext) == ERROR )
						{
							ERROR_SET (0, "Failed to delete old OSP context", ERROR_LOG_NOW);
							errorNumber = S_detControl_INTERNAL;
							return (errorNumber);
						}
					  };
					  obsId->ospFGContext = ospContext;
                  	  break ;

        	case AO : if ( obsId->ospAOContext != NULL )
					  {
						if ( ospTidyUp (obsId->ospAOContext) == ERROR )
						{
							ERROR_SET (0, "Failed to delete old OSP context", ERROR_LOG_NOW);
							errorNumber = S_detControl_INTERNAL;
							return (errorNumber);
						}
				  	  };
				  	  obsId->ospAOContext = ospContext;
                  	  break ;

        	default : ERROR_SET (0, "Failed to create new OSP context, unknown mode", ERROR_LOG_NOW);
				 	  errorNumber = S_detControl_INTERNAL;
				 	  return (errorNumber);
                  	  break ;
    	}
    }
    
	/*
	 * Define the default processing mode and default values for the various signal
	 * processing parameters. These defaults can be changed with the detSigMode command.
	 */

	if ( strcmp (pWfsName, "hr") == 0 )
	{
		obsId->sigMode = OSP_MODE_DARK;		/* HRWFS - Default mode Subtract Dark.			*/
	}
	else
	{
		obsId->sigMode = OSP_MODE_FG_AO;	/* PWFS - Default mode Fast Guide+Active Optics.*/
	}
	obsId->nCoaddFrames = 10;			/* By default coadd 10 frames.					*/
	obsId->ttCalAmplitude = 1.0;			/* Default t/t/f calibration amplitude.			*/
	obsId->coaddTimeout = 60.0;			/* Default coadd timeout.						*/
	strcpy (obsId->pCoeffsFile, "NONE");	/* Default coefficients file.					*/

	return (errorNumber);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detSigMode
 *
 *	INVOCATION:
 *	detSigMode (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	cadCmdContext	(CAD_CMD_CONTEXT)	CAD command context structure
 *	(>)	commandNumber	(int)				Command number
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(>) obsId			(OBS_ID)			Observation context structure
 *
 *	FUNCTION VALUE:
 *	(uint32)	Error number. 0 if command successful.
 *
 *	PURPOSE:
 *	Execute detSigMode command
 *
 *	DESCRIPTION:
 *	This function defines the signal processing mode.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

uint32 detSigMode
	(
	const char *	pWfsName,			/* Name of wavefront sensor.						*/
	const char *	pRecordPrefix,		/* Name of wavefront sensor.						*/
	CAD_CMD_CONTEXT	cadCmdContext,		/* CAD command context structure.					*/
	int				commandNumber,		/* Command number.									*/
	SDSU_ID			sdsuId,				/* SDSU context structure.							*/
	OBS_ID			obsId				/* Observation context structure.					*/
	)
{
	uint32			errorNumber;		/* Error number reported by task.					*/

	long			sigMode;			/* Signal processing mode.							*/
	long			nCoaddFrames;		/* Number of frames to coadd.						*/
	double			ttCalAmplitude;		/* Tip tilt calibration amplitude.					*/
	double			coaddTimeout;		/* Signal processing coadd timeout in seconds.		*/
	char			pCoeffsFile [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
										/* File of coefficients, centroids or whatever.		*/

	/*
	 * Initialise the error number and obtain the attributes provided with the command.
	 */

	errorNumber = 0;
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & sigMode);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & nCoaddFrames);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *) & ttCalAmplitude);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *) & coaddTimeout);
	EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, pCoeffsFile);

	/*
	 * Check there are valid SDSU and observation context structures.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	if ( obsId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", ERROR_LOG_NOW);
		errorNumber = S_detControl_INTERNAL;
		return (errorNumber);
	}

	/*
	 * The command can be used when an observation is in progress, as ospLib allows the mode
	 * to be changed "on-the-fly". However, warn the user this is happening.
	 */

	if ( obsId->observing )
	{
		MESSAGE_LOG (MSG_WARNING,
			"NOTE: Changing signal processing parameters while observation in progress.");
	}

	switch (sigMode)
	{
		case (OSP_MODE_NONE):
			MESSAGE_LOG (MSG_LOG, "Switching signal processing off");
			break;

		case (OSP_MODE_DARK):
			MESSAGE_LOG (MSG_LOG, "Signal processing switched to \"Subtract Dark\" mode");
			break;

		case (OSP_MODE_FG):
			MESSAGE_LOG (MSG_LOG, "Signal processing switched to \"Fast Guide\" mode");
			break;

		case (OSP_MODE_AO):
			MESSAGE_LOG2 (MSG_LOG,
				"Signal processing switched to \"AO\" mode - "
				"nCoaddFrames=%ld coaddTimeout=%f seconds",
				nCoaddFrames, coaddTimeout);
			break;

		case (OSP_MODE_FG_AO):
			MESSAGE_LOG2 (MSG_LOG,
				"Signal processing switched to \"FG + AO\" mode - "
				"nCoaddFrames=%ld coaddTimeout=%f seconds",
				nCoaddFrames, coaddTimeout);
			break;

		case (OSP_MODE_FG_COADD):
			MESSAGE_LOG1 (MSG_LOG,
				"Signal processing switched to \"FG + Coadd\" mode - nCoaddFrames=%ld",
				nCoaddFrames);
			break;

		case (OSP_MODE_COADD):
			MESSAGE_LOG1 (MSG_LOG,
				"Signal processing switched to \"Coadd Only\" mode - nCoaddFrames=%ld",
				nCoaddFrames);
			break;

		case (OSP_MODE_CALIB_TT):
			MESSAGE_LOG3 (MSG_LOG,
				"Signal processing switched to \"Calibrate T/T\" mode - "
				"nCoaddFrames=%ld amplitude=%f file=%s",
				nCoaddFrames, ttCalAmplitude, pCoeffsFile);
			break;

		case (OSP_MODE_CORRECT_TT):
			MESSAGE_LOG1 (MSG_LOG,
				"Signal processing switched to \"Correct T/T\" mode - file=%s",
				 pCoeffsFile);
			break;

		default:
			ERROR_SET (S_detControl_BAD_ATTRIBUTE, "Invalid signal processing mode",
				ERROR_LOG_SAVE);
			return (ERROR);
			break;
	}

	/*
	 * Define the signal processing mode and associated parameters.
	 * These parameters will be used in detObserveEnd.
	 */

	obsId->sigMode = sigMode;
	obsId->nCoaddFrames = nCoaddFrames;
	obsId->ttCalAmplitude = ttCalAmplitude;
	obsId->coaddTimeout = coaddTimeout;

	strncpy( obsId->pCoeffsFile, pCoeffsFile, EPICS_MAX_BYTES_STRING_ATTRIB);

	/* Initialise the coadd counter used to decide when to save coadded data to disk. */

	obsId->coaddCounter = 0;

	return (errorNumber);
}


/* ================================================================================================ */

/*+
 *	FUNCTION NAME:
 *	detShow
 *
 *	INVOCATION:
 *	detShow (pWfsNames, verbose)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsNames	(const char *)	Names of WFSs whose status is to be shown
 *	(>)	verbose		(const BOOL)	Enable verbose printout
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Show status of detector control tasks
 *
 *	DESCRIPTION:
 *	This is an engineering function which displays the current status of the
 *	detector control tasks. The tasks to be displayed may be specified in the
 *	pWfsNames string (e.g. "p1 p2"). If none are provided then all the tasks
 *	are displayed.
 *
 *	NOTE:
 *	This function is designed to be invoked from the VxWorks shell
 *
 *	EXTERNAL VARIABLES:
 *	(>)	detSdsuIdP1				(SDSU_ID)			SDSU context structure for PWFS1
 *	(>)	detSdsuIdP2				(SDSU_ID)			SDSU context structure for PWFS2
 *	(>)	detSdsuIdOi				(SDSU_ID)			SDSU context structure for OIWFS
 *	(>)	detSdsuIdHr				(SDSU_ID)			SDSU context structure for HRWFS
 *	(>)	detObsIdP1				(OBS_ID)			Observation context structure for PWFS1
 *	(>)	detObsIdP2				(OBS_ID)			Observation context structure for PWFS2
 *	(>)	detObsIdOi				(OBS_ID)			Observation context structure for OIWFS
 *	(>)	detObsIdHr				(OBS_ID)			Observation context structure for HRWFS
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

void detShow
	(
	const char *	pWfsNames,
	const BOOL		verbose
	)
{
	/*
	 * Display the contents of the SDSU context structures for each of the wavefront
	 * sensors: PWFS1, PWFS2, OIWFS and HRWFS, depending on the contents of the
	 * pWfsNames string, If no pWfsNames string is provided all the WFSs are displayed.
	 *
	 * If any of the SDSU contexts are NULL this implies the controller
	 * has not been installed.
	 */

	if ( (pWfsNames == NULL) || (strcmp (pWfsNames, " ") == 0) ||
	     (strstr(pWfsNames, "p1") != NULL) || (strstr(pWfsNames, "pwfs1") != NULL)
	   )
	{
		printf ("detShow:          PWFS1\n");
		printf ("detShow:          -----\n");

		if ( detSdsuIdP1 != NULL )
		{
			if ( sdsuShow (detSdsuIdP1, verbose) != ERROR )
			{	
				if (detObsIdP1 != NULL)
				{
					detObsShow (detObsIdP1, verbose);
				}
			}
			else
			{
				printf ("detShow: SDSU controller context for PWFS1 invalid.\n");
			}
		}
		else
		{
			printf ("detShow: SDSU controller for PWFS1 not initialised.\n");
		}
	}

	if ( (pWfsNames == NULL) || (strcmp (pWfsNames, " ") == 0) ||
	     (strstr(pWfsNames, "p2") != NULL) || (strstr(pWfsNames, "pwfs2") != NULL)
	   )
	{

		printf ("detShow:          PWFS2\n");
		printf ("detShow:          -----\n");

		if ( detSdsuIdP2 != NULL )
		{
			if ( sdsuShow (detSdsuIdP2, verbose) != ERROR )
			{	
				if (detObsIdP2 != NULL)
				{
					detObsShow (detObsIdP2, verbose);
				}
			}
			else
			{
				printf ("detShow: SDSU controller context for PWFS2 invalid.\n");
			}
		}
		else
		{
			printf ("detShow: SDSU controller for PWFS2 not initialised.\n");
		}
	}

	if ( (pWfsNames == NULL) || (strcmp (pWfsNames, " ") == 0) ||
	     (strstr(pWfsNames, "oi") != NULL) || (strstr(pWfsNames, "oiwfs") != NULL)
	   )
	{

		printf ("detShow:          OIWFS\n");
		printf ("detShow:          -----\n");

		if ( detSdsuIdOi != NULL )
		{
			if ( sdsuShow (detSdsuIdOi, verbose) != ERROR )
			{	
				if (detObsIdOi != NULL)
				{
					detObsShow (detObsIdOi, verbose);
				}
			}
			else
			{
				printf ("detShow: SDSU controller context for OIWFS invalid.\n");
			}
		}
		else
		{
			printf ("detShow: SDSU controller for OIWFS not initialised.\n");
		}
	}

	if ( (pWfsNames == NULL) || (strcmp (pWfsNames, " ") == 0) ||
	     (strstr(pWfsNames, "hr") != NULL) || (strstr(pWfsNames, "hrwfs") != NULL)
	   )
	{

		printf ("detShow:          HRWFS\n");
		printf ("detShow:          -----\n");

		if ( detSdsuIdHr != NULL )
		{
			if ( sdsuShow (detSdsuIdHr, verbose) != ERROR )
			{	
				if (detObsIdHr != NULL)
				{
					detObsShow (detObsIdHr, verbose);
				}
			}
			else
			{
				printf ("detShow: SDSU controller context for HRWFS invalid.\n");
			}
		}
		else
		{
			printf ("detShow: SDSU controller for HRWFS not initialised.\n");
		}
	}

	return;
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detStatusShow
 *
 *	INVOCATION:
 *	detStatusShow (pWfsNames)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsNames	(const char *)	Names of WFSs whose status is to be shown
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Show status parameters of detector control tasks
 *
 *	DESCRIPTION:
 *	This is an engineering function which displays the status parameters of the
 *	detector control tasks. The tasks to be displayed may be specified in the
 *	pWfsNames string (e.g. "p1 p2"). If none are provided then all the tasks
 *	are displayed.
 *
 *	NOTE:
 *	This function is designed to be invoked from the VxWorks shell
 *
 *	EXTERNAL VARIABLES:
 *	(>)	detSdsuIdP1				(SDSU_ID)			SDSU context structure for PWFS1
 *	(>)	detSdsuIdP2				(SDSU_ID)			SDSU context structure for PWFS2
 *	(>)	detSdsuIdOi				(SDSU_ID)			SDSU context structure for OIWFS
 *	(>)	detSdsuIdHr				(SDSU_ID)			SDSU context structure for HRWFS
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

void detStatusShow
	(
	const char *	pWfsNames
	)
{
	/*
	 * Display the SDSU status parameters for each of the wavefront
	 * sensors: PWFS1, PWFS2, OIWFS and HRWFS, depending on the contents of the
	 * pWfsNames string, If no pWfsNames string is provided all the WFSs are displayed.
	 *
	 * If any of the SDSU contexts are NULL this implies the controller
	 * has not been installed.
	 */

	if ( (pWfsNames == NULL) || (strcmp (pWfsNames, " ") == 0) ||
	     (strstr(pWfsNames, "p1") != NULL) || (strstr(pWfsNames, "pwfs1") != NULL)
	   )
	{
		printf ("detStatusShow:          PWFS1\n");
		printf ("detStatusShow:          -----\n");

		if ( detSdsuIdP1 != NULL )
		{
			if ( sdsuStatusShow (detSdsuIdP1) == ERROR )
			{
				printf ("detStatusShow: SDSU controller context for PWFS1 invalid.\n");
			}
		}
		else
		{
			printf ("detStatusShow: SDSU controller for PWFS1 not initialised.\n");
		}
	}

	if ( (pWfsNames == NULL) || (strcmp (pWfsNames, " ") == 0) ||
	     (strstr(pWfsNames, "p2") != NULL) || (strstr(pWfsNames, "pwfs2") != NULL)
	   )
	{

		printf ("detStatusShow:          PWFS2\n");
		printf ("detStatusShow:          -----\n");

		if ( detSdsuIdP2 != NULL )
		{
			if ( sdsuStatusShow (detSdsuIdP2) == ERROR )
			{
				printf ("detStatusShow: SDSU controller context for PWFS2 invalid.\n");
			}
		}
		else
		{
			printf ("detStatusShow: SDSU controller for PWFS2 not initialised.\n");
		}
	}

	if ( (pWfsNames == NULL) || (strcmp (pWfsNames, " ") == 0) ||
	     (strstr(pWfsNames, "oi") != NULL) || (strstr(pWfsNames, "oiwfs") != NULL)
	   )
	{

		printf ("detStatusShow:          OIWFS\n");
		printf ("detStatusShow:          -----\n");

		if ( detSdsuIdOi != NULL )
		{
			if ( sdsuStatusShow (detSdsuIdOi) == ERROR )
			{
				printf ("detStatusShow: SDSU controller context for OIWFS invalid.\n");
			}
		}
		else
		{
			printf ("detStatusShow: SDSU controller for OIWFS not initialised.\n");
		}
	}

	if ( (pWfsNames == NULL) || (strcmp (pWfsNames, " ") == 0) ||
	     (strstr(pWfsNames, "hr") != NULL) || (strstr(pWfsNames, "hrwfs") != NULL)
	   )
	{

		printf ("detStatusShow:          HRWFS\n");
		printf ("detStatusShow:          -----\n");

		if ( detSdsuIdHr != NULL )
		{
			if ( sdsuStatusShow (detSdsuIdHr) == ERROR )
			{
				printf ("detStatusShow: SDSU controller context for HRWFS invalid.\n");
			}
		}
		else
		{
			printf ("detStatusShow: SDSU controller for HRWFS not initialised.\n");
		}
	}

	return;
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detTempShow
 *
 *	INVOCATION:
 *	detTempShow (pWfsNames)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsNames	(const char *)	Names of WFSs whose status is to be shown
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Show temperature parameters of detector control tasks
 *
 *	DESCRIPTION:
 *	This is an engineering function which displays the temperature parameters of the
 *	detector control tasks. The tasks to be displayed may be specified in the
 *	pWfsNames string (e.g. "p1 p2"). If none are provided then all the tasks
 *	are displayed.
 *
 *	NOTE:
 *	This function is designed to be invoked from the VxWorks shell
 *
 *	EXTERNAL VARIABLES:
 *	(>)	detSdsuIdP1				(SDSU_ID)			SDSU context structure for PWFS1
 *	(>)	detSdsuIdP2				(SDSU_ID)			SDSU context structure for PWFS2
 *	(>)	detSdsuIdOi				(SDSU_ID)			SDSU context structure for OIWFS
 *	(>)	detSdsuIdHr				(SDSU_ID)			SDSU context structure for HRWFS
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

void detTempShow
	(
	const char *	pWfsNames
	)
{
	/*
	 * Display the SDSU status parameters for each of the wavefront
	 * sensors: PWFS1, PWFS2, OIWFS and HRWFS, depending on the contents of the
	 * pWfsNames string, If no pWfsNames string is provided all the WFSs are displayed.
	 *
	 * If any of the SDSU contexts are NULL this implies the controller
	 * has not been installed.
	 */

	if ( (pWfsNames == NULL) || (strcmp (pWfsNames, " ") == 0) ||
	     (strstr(pWfsNames, "p1") != NULL) || (strstr(pWfsNames, "pwfs1") != NULL)
	   )
	{
		printf ("detTempShow:          PWFS1\n");
		printf ("detTempShow:          -----\n");

		if ( detSdsuIdP1 != NULL )
		{
			if ( sdsuTempShow (detSdsuIdP1) == ERROR )
			{
				printf ("detTempShow: SDSU controller context for PWFS1 invalid.\n");
			}
		}
		else
		{
			printf ("detTempShow: SDSU controller for PWFS1 not initialised.\n");
		}
	}

	if ( (pWfsNames == NULL) || (strcmp (pWfsNames, " ") == 0) ||
	     (strstr(pWfsNames, "p2") != NULL) || (strstr(pWfsNames, "pwfs2") != NULL)
	   )
	{

		printf ("detTempShow:          PWFS2\n");
		printf ("detTempShow:          -----\n");

		if ( detSdsuIdP2 != NULL )
		{
			if ( sdsuTempShow (detSdsuIdP2) == ERROR )
			{
				printf ("detTempShow: SDSU controller context for PWFS2 invalid.\n");
			}
		}
		else
		{
			printf ("detTempShow: SDSU controller for PWFS2 not initialised.\n");
		}
	}

	if ( (pWfsNames == NULL) || (strcmp (pWfsNames, " ") == 0) ||
	     (strstr(pWfsNames, "oi") != NULL) || (strstr(pWfsNames, "oiwfs") != NULL)
	   )
	{

		printf ("detTempShow:          OIWFS\n");
		printf ("detTempShow:          -----\n");

		if ( detSdsuIdOi != NULL )
		{
			if ( sdsuTempShow (detSdsuIdOi) == ERROR )
			{
				printf ("detTempShow: SDSU controller context for OIWFS invalid.\n");
			}
		}
		else
		{
			printf ("detTempShow: SDSU controller for OIWFS not initialised.\n");
		}
	}

	if ( (pWfsNames == NULL) || (strcmp (pWfsNames, " ") == 0) ||
	     (strstr(pWfsNames, "hr") != NULL) || (strstr(pWfsNames, "hrwfs") != NULL)
	   )
	{

		printf ("detTempShow:          HRWFS\n");
		printf ("detTempShow:          -----\n");

		if ( detSdsuIdHr != NULL )
		{
			if ( sdsuTempShow (detSdsuIdHr) == ERROR )
			{
				printf ("detTempShow: SDSU controller context for HRWFS invalid.\n");
			}
		}
		else
		{
			printf ("detTempShow: SDSU controller for HRWFS not initialised.\n");
		}
	}

	return;
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detObsContextCreate
 *
 *	INVOCATION:
 *	detObsContextCreate (void)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	None
 *
 *	FUNCTION VALUE:
 *	(OBS_ID)	Pointer to observation ID, or NULL if unsuccessful.
 *
 *	PURPOSE:
 *	Create an observation ID structure
 *
 *	DESCRIPTION:
 *	This function creates and initialises an observation ID structure.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

OBS_ID detObsContextCreate (void)
{
	OBS_ID	obsId;

	/* Allocate memory for the observation ID structure, initialising its contents to zero. */

#ifdef DEBUG
	printf ("detObsContextCreate: Allocating %ld bytes of memory for OBS_ID structure.\n",
		sizeof (OBS_ID_STRUCT));
#endif /* DEBUG */

	if ((obsId = (OBS_ID) calloc ((size_t) 1, sizeof (OBS_ID_STRUCT))) == NULL)
	{
		ERROR_SET (0,"Memory allocation for observation context failed", ERROR_LOG_SAVE);
		return (NULL);
	}

	/* Create a binary semaphore for synchronising observation threads. */

	obsId->syncSem = semBCreate( SEM_Q_FIFO, SEM_EMPTY );
	if ( obsId->syncSem == NULL )
	{
		ERROR_SET (0, "Failed to create observation synchronisation semaphore", ERROR_LOG_SAVE);
		cfree ((char *) obsId);
		return (NULL);
	}

	return (obsId);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detObsShow
 *
 *	INVOCATION:
 *	detObsShow (obsId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	obsId	(OBS_ID)		Pointer to observation ID
 *	(>)	verbose	(const BOOL)	Enable verbose mode
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if successful, ERROR if unsuccessful
 *
 *	PURPOSE:
 *	Display the contents of an observation ID structure
 *
 *	DESCRIPTION:
 *	This function creates and initialises an observation ID structure.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS detObsShow (
	OBS_ID		obsId,
	const BOOL	verbose
	)
{
	const char *	outOptionStrings[4] =
		{
			"NONE", "DHS", "FILE", "BOTH"
		};

	/* Check the observation context structure is valid. */

	if ( obsId == NULL )
	{
		ERROR_SET (0, "Invalid observation context", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* Display the contents of the observation context structure. */

	printf ("Contents of observation context structure at %p:\n", obsId);
	printf ("--------------------------------------------------------\n");
	printf ("Associated SDSU context          : %p\n", obsId->sdsuId);
	printf ("Observing?                       : %s\n", (obsId->observing ? "YES" : "NO") );
	printf ("  Observing status record context: %p\n", obsId->pDetObservingContext);

	printf ("Total number of frames           : %d\n", obsId->totalFrames);

	printf ("Alarm timer ID                   : %d\n", (int) obsId->timeId);

	printf ("Size of frame in pixels (X x Y)  : %d x %d\n", obsId->xPixels, obsId->yPixels);
	printf ("Number of detector outputs       : %ld\n", obsId->outputs);

	printf ("Output options                   : %s\n", outOptionStrings[obsId->outOptions] );

	if ( (obsId->outOptions == 1) || (obsId->outOptions == 3) )
	{
		printf ("  DHS connection ID          : %d\n", (int) obsId->dhsConnection);
		printf ("  Data label                 : %s\n", obsId->pDataLabel);
	}
	if ( (obsId->outOptions == 2) || (obsId->outOptions == 3) ) 
	{
#ifdef SAVE_RAW_DATA
		printf ("  Raw data file name         : %s\n", obsId->pRawFileName);
#endif /* SAVE_RAW_DATA */
		printf ("  Output data file name      : %s\n", obsId->pOutFileName);
	}
	printf ("  Simulated data file name       : %s (simulate=%s)\n", obsId->pSimFileName,
		((obsId->sdsuId == NULL) ? "DON'T KNOW" : (obsId->sdsuId->simulate ? "YES" : "NO")) );

	printf ("Signal processing FG context     : %p\n", obsId->ospFGContext);
	printf ("Signal processing AO context     : %p\n", obsId->ospAOContext);


	printf ("Time at observation start/end    : %f %f\n", obsId->rawtStart, obsId->rawtEnd);
	printf ("Exposure in seconds reqst/actual : %f %f\n", obsId->exposedRQ, obsId->exposed);

	printf ("Axis 1 world coordinate info.    : %s %f %f\n",
		obsId->ctype1, obsId->crpix1, obsId->crval1);
	printf ("Axis 2 world coordinate info.    : %s %f %f\n",
		obsId->ctype2, obsId->crpix2, obsId->crval2);
	printf ("Rotation/skew matrix             : %f %f %f %f\n",
		obsId->cd1_1, obsId->cd1_2, obsId->cd2_1, obsId->cd2_2);
	printf ("Radecsys, equinox, mjd           : %s %f %f\n",
		obsId->radecsys, obsId->equinox, obsId->mjdobs);
	printf ("\n\n");

	printf ("\n");			/* Blank line for spacing */

	/* In verbose mode, also show the contents of the signal processing context structure */

	if (verbose & (obsId->ospFGContext != NULL))
	{
		ospShow(obsId->ospFGContext);
	}
	if (verbose & (obsId->ospAOContext != NULL))
	{
		ospShow(obsId->ospAOContext);
	}


	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detPacketCallback
 *
 *	INVOCATION:
 *	detPacketCallback (sdsuId, obsIdIn, frameId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	sdsuId		(SDSU_ID)		SDSU context structure
 *	(>)	obsIdIn		(void *)		Observation context pointer cast to void *
 *	(>)	pFrame		(SDSU_FRAME *)	Frame pointer
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Detector controller packet callback function
 *
 *	DESCRIPTION:
 *	This function will be called each time a packet is received from a
 *	wavefront sensor.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	Doesn't do anything, not used yet either.
 *-
 */

void detPacketCallback
	(
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	void *			obsIdIn,			/* Observation context structure.			*/
	SDSU_FRAME *	pFrame				/* Frame pointer.							*/
	)
{

	printf ("Packet callback\n");

	return;
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detFrameCallback
 *
 *	INVOCATION:
 *	detFrameCallback (sdsuId, obsIdIn, pFrame)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	sdsuId		(SDSU_ID)		SDSU context structure
 *	(>)	obsIdIn		(void *)		Observation context pointer cast to void *
 *	(>)	pFrame		(SDSU_FRAME *)	Frame pointer
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Detector controller frame callback function
 *
 *	DESCRIPTION:
 *	This function is called each time a frame readout from a wavefront sensor is
 *	finished.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	Doesn't do anything, not used yet either.
 *-
 */

void detFrameCallback
	(
	SDSU_ID			sdsuId,				/* SDSU context structure.					*/
	void *			obsIdIn,			/* Observation context structure.			*/
	SDSU_FRAME *	pFrame				/* Frame pointer.							*/
	)
{

	printf ("Frame callback\n");

	return;
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detSimulateData
 *
 *	INVOCATION:
 *	detSimulateData (xPixels, yPixels, option, pFrame)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	xPixels		(const int)			Number of pixels in X
 *	(>)	yPixels		(const int)			Number of pixels in Y
 *	(>)	option		(const int)			Simulation option
 *	(!) pFrame		(SDSU_FRAME *)		Pointer to SDSU frame
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if successful, ERROR if unsuccessful
 *
 *	PURPOSE:
 *	Fill frame buffer with simulated data (TEMPORARY FUNCTION)
 *
 *	DESCRIPTION:
 *	This function fills a frame buffer with simulated data with the following
 *	options:
 *
 *	Option 1 consists of an incrementing series. The first pixel (output 1) contains zero, the second
 *	pixel (output 2) is one, and so on. Thus within each quadrant the least
 *	significant 2 bits should always be the same for a 4-output device, or the
 *	least significant bit the same for 2-output devices.
 *
 *	Option 2 consists of...
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	It is assumed that pFrame points to an SDSU frame structure (initialised with sdsuFrameAlloc,
 *	sdsuFrameFind and sdsuFrameReserve) containing sufficient storage space for
 *	xPixels * yPixels values.
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	Option 2 is very wasteful of CPU. It should be used for small images only.
 *
 *	At the moment this function only simulates a 2x2 array of spots for a CCD
 *	with 2x2 sectors. It can be extended if necessary.
 *-
 */

STATUS detSimulateData
	(
	const int		xPixels,			/* Number of pixels in X.					*/
	const int		yPixels,			/* Number of pixels in Y.					*/
	const int		option,				/* Simulation option.						*/
	SDSU_FRAME *	pFrame  			/* Pointer to frame buffer.					*/
	)
{
	const int		nPixels = xPixels * yPixels;
										/* Total number of pixels.					*/

	const int		nSectors = 4;		/* Number of sectors/outputs.				*/
	int				sector;				/* Sector counter.							*/
	int				xPixelsSector;		/* Number of columns per sector.			*/
	int				yPixelsSector;		/* Number of rows per sector.				*/
	int				i, j;				/* Column and row counters.					*/
	int				is, js;				/* Column and row for a particular sector.	*/

	const int		nSpots = 4;			/* Number of simulated spots.				*/
	int				spot;				/* Spot counter.							*/
	int				spotx[4];			/* X coordinates of simulated spot centres.	*/
	int				spoty[4];			/* y coordinates of simulated spot centres.	*/


	double			dist;				/* Distance between pixel and spot centre.	*/
	double			dvalue;				/* Double valueto write into frame buffer.	*/
	uint16			value;				/* Integer value to write into frame buffer.*/

	volatile uint16 *	ptr;			/* Pointer into frame buffer.				*/


	/* Check the frame buffer pointer and size are valid. */

	if (pFrame == NULL)
	{
		ERROR_SET(S_detControl_INTERNAL, "No frame buffer defined", ERROR_LOG_SAVE);
		return (ERROR);
	}

	if ((xPixels <= 0) || (yPixels <= 0 ))
	{
		ERROR_SET2 (S_detControl_BAD_ATTRIBUTE, "Bad number of pixels given, %d x %d", ERROR_LOG_SAVE,
		            xPixels, yPixels);
		return (ERROR);
	}

#ifdef DEBUG
	printf ("detSimulateData: Simulating %d x %d pixels of data to buffer at %p - option %d\n",
		xPixels, yPixels, pFrame, option );
#endif	/* DEBUG */

	/* Switch according to the simulation option chosen. */

	switch (option)
	{
		case (1):

			/*
			 * An incrementing series of values is required.
			 * Note that ptr is initialised to the start of the frame pixels.
			 */

			ptr = & pFrame->pixel[0];
			for ( i=0; i<nPixels; i++)
			{
				value = (uint16) i;
				*(ptr) = value;
				ptr++;
			}
			break;


		case (2):

			/*
			 * An array of simulated spots is required.
			 */

			/* First initialise the number of pixels per sector, based on the number of sectors. */

			if ( nSectors == 2 )
			{
				/* There are two outputs and therefore 2 sectors in a 2x1 pattern. */

				xPixelsSector = xPixels / 2;
				yPixelsSector = yPixels;
			}
			else if ( nSectors == 4 )
			{
				/* There are four outputs and therefore 2 sectors in a 2x2 pattern. */

				xPixelsSector = xPixels / 2;
				yPixelsSector = yPixels / 2;
			}

			/* Now load up the array of spot centroids */

/* Comment out ideal positions
			spotx[0] = xPixels / nSpots;		spoty[0] = yPixels / nSpots;
			spotx[1] = spotx[0] * (nSpots-1);	spoty[1] = spoty[0];
			spotx[2] = spotx[0];				spoty[2] = spoty[0] * (nSpots-1);
			spotx[3] = spotx[1];				spoty[3] = spoty[2];
*/

			/* Real positions for 2x2 wavefront sensor */
			spotx[0] = 32;				spoty[0] = 27;
			spotx[1] = 61;				spoty[1] = 25;
			spotx[2] = 25;				spoty[2] = 59;
			spotx[3] = 61;				spoty[3] = 58;


			/*
			 * Initialise ptr to the start of the frame pixels and then step through
			 * the rows and columns within each sector.
			 */

			ptr = & pFrame->pixel[0];

			for ( j=0; j < yPixelsSector; j++ )
			{
				for ( i=0; i < xPixelsSector; i++ )
				{
					for ( sector=1; sector <= nSectors; sector++ )
					{
						/*
						 * Calculate the row and column coordinates of this particular
						 * point in this sector.
						 */

						if ( sector == 1 )
						{
							/* Sector 1 */
							is = i;
							js = j;
						}
						else if ( sector == 2 )
						{
							/* Sector 2 */
							is = xPixels - i;
							js = j;
						}
						else if ( sector == 3 )
						{
							/* Sector 3 */
							is = xPixels - i;
							js = yPixels - j;
						}
						else
						{
							/* Sector 4 */
							is = i;
							js = yPixels - j;
						}

						/*
						 * Use the row and column coordinates calculated above to determine the
						 * the distance of this point from each spot centre, calculate the sum of
						 * the light from each spot (assuming a Gaussian distribution), and write
						 * this sum to the location pointed to by ptr. The value is not allowed to
						 * exceed 65535 because it needs to be stored as an unsigned 16 bit integer.
						 * Finally, ptr is incremented.
						 *
						 * The constant factors used in the following equations are arbitrary.
						 */

						dvalue = (double) ( 1000 * rand() / RAND_MAX );	/* Random background */

						for ( spot=0; spot < nSpots; spot++ )
						{
							dist = 0.25 * (double) ((is-spotx[spot])*(is-spotx[spot]) +
							                        (js-spoty[spot])*(js-spoty[spot]));
							dvalue +=  30000.0 * exp (-dist);
						}

						if ( dvalue <= 65535.0 )
							value = (uint16) floor(dvalue);
						else
							value = 65535;

						*ptr++ = value;
					}
				}
			}
			break;

		default:

			ERROR_SET( S_detControl_BAD_ATTRIBUTE, "Unknown simulation option", ERROR_LOG_SAVE);
			return (ERROR);
			break;
	}

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detFrameUnscramble
 *
 *	INVOCATION:
 *	detFrameUnscramble (xPixels, yPixels, outputs, inFrame, outBuffer)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	xPixels		(const int)			Number of columns
 *	(>)	yPixels		(const int)			Number of rows
 *	(>)	outputs		(const int)			Number of detector outputs (2 or 4)
 *	(>) inFrame		(SDSU_FRAME *)		Pointer to input frame
 *	(<) outBuffer	(float *)			Pointer to output frame buffer
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if command successful, ERROR if unsuccessful
 *
 *	PURPOSE:
 *	Unscramble an entire frame of data
 *
 *	DESCRIPTION:
 *	This function takes a raw frame of data containing pixels in the order they are read
 *	from the detector and unscrambles them to generate an output frame with pixels in the
 *	correct order.
 *
 *	ACKNOWLEDGEMENTS:
 *	This function is based around the LeachDeScramble (lds) program provided by Les Saddlemyer
 *	and Tim Hardy, Hertzberg Institute of Astrophysics, Canada.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	inFrame is a data frame which contains the scrambled SDSU pixels.
 *	outBuffer must point to a buffer large enough to contain at least 
 *	xPixels*yPixels floating point values.
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS detFrameUnscramble
	(
	const int		xPixels,			/* Number of columns.						*/
	const int		yPixels,			/* Number of rows.							*/
	const int		outputs,			/* Number of detector outputs (2 or 4).		*/
	SDSU_FRAME *	inFrame,			/* Pointer to input frame					*/
	float *			outBuffer			/* Pointer to output frame buffer.			*/
	)
{

	volatile uint16 *	ptr;			/* Pointer into frame buffer.				*/

	int				i, j;				/* Counters.								*/

	int				nPixels;			/* Total number of pixels.					*/

	int				xPixelsSector;		/* Number of columns per sector.			*/
	int				yPixelsSector;		/* Number of rows per sector.				*/

	volatile uint16 *	inDataPtr;		/* Pointer to start of input data.			*/
	float *				outDataPtr;		/* Pointer to start of output data.			*/

	float * 		ps1;				/* Pointer to beginning of sector 1.		*/
	float *			ps2;				/* Pointer to beginning of sector 2.		*/
	float *			ps3;				/* Pointer to beginning of sector 3.		*/
	float *			ps4;				/* Pointer to beginning of sector 4.		*/

#ifdef DEBUG
	float			min, max;			/* Minimum and maximum.						*/
#endif /* DEBUG */


	if ( (inFrame == NULL) || (outBuffer == NULL) )
	{
		ERROR_SET(S_detControl_INTERNAL, "No input and/or output buffers defined", ERROR_LOG_SAVE);
		return (ERROR);
	}

#ifdef DEBUG
	printf ("detFrameUnscramble: Unscrambling %d x %d pixels from frame at %p to %p\n",
	        xPixels, yPixels, inFrame, outBuffer);

	min = FLT_MAX;
	max = -FLT_MAX;
#endif /* DEBUG */

	/*
	 * Set pointers to the start of the data.
	 */

	inDataPtr = & inFrame->pixel[0];
	outDataPtr = outBuffer;

	/*
	 * The algorithm used to unscramble the data depends on the number of outputs
	 * from the detector. If there are two outputs the sectors are arranged like this
	 *
	 *   +------------+------------+
	 *   |  sector 1  |  sector 2  |
	 *   0----->------+-----<------0
	 *
	 * and if there are four outputs the sectors are arranged like this
	 *
	 *   0----->------+-----<------0
	 *   |  sector 4  |  sector 3  |
	 *   +------------+------------+
	 *   |  sector 1  |  sector 2  |
	 *   0----->------+-----<------0
	 *
	 * "0" shows the origin of each sector and ">" the direction of readout.
	 */

	switch (outputs)
	{
		case (2):

			/* There are two outputs and therefore 2 sectors in a 2x1 pattern. */

			nPixels = xPixels * yPixels;
			xPixelsSector = xPixels / 2;
			yPixelsSector = yPixels;

#ifdef DEBUG
			printf ("Two sectors of size %d x %d\n", xPixelsSector, yPixelsSector);
#endif /* DEBUG */

			/* Initialise the starting position for each sector */

			ps1 = outDataPtr;
			ps2 = &outDataPtr[xPixels - 1];
			ptr = inDataPtr;

			/* Treat one line at a time, moving sector pointers */

			for (i = 0; i < yPixelsSector; i++)
			{
				for (j = 0; j < xPixelsSector; j++)
				{
#ifdef DEBUG
					if ( (float) *ptr < min ) min = (float) *ptr;
					if ( (float) *ptr > max ) max = (float) *ptr;
					if ( (float) *(ptr+1) < min ) min = (float) *(ptr+1);
					if ( (float) *(ptr+1) > max ) max = (float) *(ptr+1);
#endif
					/*
					 * Change the order here if sectors 1, 2 is
					 * different from the order of arrival
					 */

					*ps1++ = (float) *ptr++;
					*ps2-- = (float) *ptr++;
				}
				ps1 += xPixelsSector;
				ps2 += xPixelsSector * 3;
			}
#ifdef DEBUG
			printf ("Values range from %g to %g\n", min, max);
#endif /* DEBUG */
			break;

		case (4):

			/* There are four outputs and therefore 4 sectors in a 2x2 pattern. */

			nPixels = xPixels * yPixels;
			xPixelsSector = xPixels / 2;
			yPixelsSector = yPixels / 2;

#ifdef DEBUG
			printf ("Four sectors of size %d x %d\n", xPixelsSector, yPixelsSector);
#endif /* DEBUG */

			/* Initialise the starting position for each sector */

			ps1 = outDataPtr;
			ps2 = &outDataPtr[xPixels - 1];
			ps3 = &outDataPtr[nPixels - 1];
			ps4 = &outDataPtr[nPixels - xPixels];
			ptr = inDataPtr;

			/* Treat one line at a time, moving sector pointers */

			for (i = 0; i < yPixelsSector; i++)
			{
				for (j = 0; j < xPixelsSector; j++)
				{
#ifdef DEBUG
					if ( (float) *ptr < min ) min = (float) *ptr;
					if ( (float) *ptr > max ) max = (float) *ptr;
					if ( (float) *(ptr+1) < min ) min = (float) *(ptr+1);
					if ( (float) *(ptr+1) > max ) max = (float) *(ptr+1);
					if ( (float) *(ptr+2) < min ) min = (float) *(ptr+2);
					if ( (float) *(ptr+2) > max ) max = (float) *(ptr+2);
					if ( (float) *(ptr+3) < min ) min = (float) *(ptr+3);
					if ( (float) *(ptr+3) > max ) max = (float) *(ptr+3);
#endif
					/*
					 * change the order here if sectors 1, 2, 3, 4 is
					 * different from the order of arrival
					 */

					*ps1++ = (float) *ptr++;
					*ps2-- = (float) *ptr++;
					*ps3-- = (float) *ptr++;
					*ps4++ = (float) *ptr++;
				}
				ps1 += xPixelsSector;
				ps2 += xPixelsSector * 3;
				ps3 -= xPixelsSector;
				ps4 -= xPixelsSector * 3;
			}
#ifdef DEBUG
			printf ("Values range from %g to %g\n", min, max);
#endif /* DEBUG */
			break;

		default:
			ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, "Bad number of outputs given, %d", ERROR_LOG_SAVE,
		         	    outputs);
			return (ERROR);
	}

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detFrameScramble
 *
 *	INVOCATION:
 *	detFrameScramble (xPixels, yPixels, outputs, inBuffer, outBuffer)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	xPixels		(const int)			Number of columns
 *	(>)	yPixels		(const int)			Number of rows
 *	(>)	outputs		(const int)			Number of detector outputs (2 or 4)
 *	(>) inFrame		(float *)			Pointer to input frame buffer
 *	(<) outBuffer	(uint16 *)			Pointer to output frame buffer
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if command successful, ERROR if unsuccessful
 *
 *	PURPOSE:
 *	Scramble an entire frame of data
 *
 *	DESCRIPTION:
 *	This function takes a simulated frame of data and scrambles the pixels into the order they
 *	are read from the detector.
 *
 *	ACKNOWLEDGEMENTS:
 *	This function is based around the LeachDeScramble (lds) program provided by Les Saddlemyer
 *	and Tim Hardy, Hertzberg Institute of Astrophysics, Canada.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	inBuffer must point to a buffer containing 
 *	xPixels*yPixels floating point values.
 *	outBuffer must point to a buffer large enough to contain at least 
 *	xPixels*yPixels unsigned short integer values.
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS detFrameScramble
	(
	const int		xPixels,			/* Number of columns.						*/
	const int		yPixels,			/* Number of rows.							*/
	const int		outputs,			/* Number of detector outputs (2 or 4).		*/
	float *			inBuffer,			/* Pointer to input frame buffer			*/
	uint16 *		outBuffer			/* Pointer to output frame buffer.			*/
	)
{

	float *			ptr;				/* Pointer into input frame buffer.			*/

	int				i, j;				/* Counters.								*/

	int				nPixels;			/* Total number of pixels.					*/

	int				xPixelsSector;		/* Number of columns per sector.			*/
	int				yPixelsSector;		/* Number of rows per sector.				*/


	uint16 * 		ps1;				/* Pointer to beginning of sector 1.		*/
	uint16 *		ps2;				/* Pointer to beginning of sector 2.		*/
	uint16 *		ps3;				/* Pointer to beginning of sector 3.		*/
	uint16 *		ps4;				/* Pointer to beginning of sector 4.		*/


	if ( (inBuffer == NULL) || (outBuffer == NULL) )
	{
		ERROR_SET(S_detControl_INTERNAL, "No input and/or output buffers defined", ERROR_LOG_SAVE);
		return (ERROR);
	}

#ifdef DEBUG
	printf ("detFrameUnscramble: Scrambling %d x %d pixels from frame at %p to %p\n",
		xPixels, yPixels, inBuffer, outBuffer);
#endif /* DEBUG */


	/*
	 * The algorithm used to unscramble the data depends on the number of outputs
	 * from the detector. If there are two outputs the sectors are arranged like this
	 *
	 *   +------------+------------+
	 *   |  sector 1  |  sector 2  |
	 *   0----->------+-----<------0
	 *
	 * and if there are four outputs the sectors are arranged like this
	 *
	 *   0----->------+-----<------0
	 *   |  sector 4  |  sector 3  |
	 *   +------------+------------+
	 *   |  sector 1  |  sector 2  |
	 *   0----->------+-----<------0
	 *
	 * "0" shows the origin of each sector and ">" the direction of readout.
	 */

	switch (outputs)
	{
		case (2):

			/* There are two outputs and therefore 2 sectors in a 2x1 pattern. */

			nPixels = xPixels * yPixels;
			xPixelsSector = xPixels / 2;
			yPixelsSector = yPixels;

#ifdef DEBUG
			printf ("Two sectors of size %d x %d\n", xPixelsSector, yPixelsSector);
#endif /* DEBUG */

			/* Initialise the starting position for each sector */

			ps1 = outBuffer;
			ps2 = &outBuffer[xPixels - 1];
			ptr = inBuffer;

			/* Treat one line at a time, moving sector pointers */

			for (i = 0; i < yPixelsSector; i++)
			{
				for (j = 0; j < xPixelsSector; j++)
				{
					/*
					 * Change the order here if sectors 1, 2 is
					 * different from the order of arrival
					 */

					*ps1++ = (uint16) *ptr++;
					*ps2-- = (uint16) *ptr++;
				}
				ps1 += xPixelsSector;
				ps2 += xPixelsSector * 3;
			}
			break;

		case (4):

			/* There are four outputs and therefore 4 sectors in a 2x2 pattern. */

			nPixels = xPixels * yPixels;
			xPixelsSector = xPixels / 2;
			yPixelsSector = yPixels / 2;

#ifdef DEBUG
			printf ("Four sectors of size %d x %d\n", xPixelsSector, yPixelsSector);
#endif /* DEBUG */

			/* Initialise the starting position for each sector */

			ps1 = outBuffer;
			ps2 = &outBuffer[xPixels - 1];
			ps3 = &outBuffer[nPixels - 1];
			ps4 = &outBuffer[nPixels - xPixels];
			ptr = inBuffer;

			/* Treat one line at a time, moving sector pointers */

			for (i = 0; i < yPixelsSector; i++)
			{
				for (j = 0; j < xPixelsSector; j++)
				{
					/*
					 * change the order here if sectors 1, 2, 3, 4 is
					 * different from the order of arrival
					 */

					*ps1++ = (uint16) *ptr++;
					*ps2-- = (uint16) *ptr++;
					*ps3-- = (uint16) *ptr++;
					*ps4++ = (uint16) *ptr++;
				}
				ps1 += xPixelsSector;
				ps2 += xPixelsSector * 3;
				ps3 -= xPixelsSector;
				ps4 -= xPixelsSector * 3;
			}
			break;

		default:
			ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, "Bad number of outputs given, %d", ERROR_LOG_SAVE,
		    	outputs);
			return (ERROR);
	}

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detWriteFits
 *
 *	INVOCATION:
 *	detWriteFits (filename. obsId, xPixels, yPixels, pImageBuffer)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	filename		(char *)		Name of file to contain data.
 *	(>)	obsId			(OBS_ID)		Current observation context structure
 *	(>)	xPixels			(int)			Number of pixels along X axis
 *	(>)	yPixels			(int)			Number of pixels along Y axis
 *	(!) pImageBuffer	(float *)		Pointer to image buffer
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if command successful, ERROR if unsuccessful
 *
 *	PURPOSE:
 *	Write floating point data to FITS file (TEMPORARY FUNCTION)
 *
 *	DESCRIPTION:
 *	This function writes the contents of the frame buffer to a FITS file.
 *
 *  ACKNOWLEDGEMENTS:
 *	This function is based on a private function provided by Andrew Johnson.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	It is assumed that pImageBuffer points to a buffer of memory containing
 *	xPixels*yPixels unsigned short integer pixel values.
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	This function does not write very good FITS files. It needs to be rewritten
 *	to use the cFitsio library.
 *-
 */

STATUS detWriteFits
	(
	char *			filename,			/* Name of file to be written.				*/
	OBS_ID			obsId,				/* Current observation context structure.	*/
	int				xPixels,			/* Number of pixels along X axis.			*/
	int				yPixels,			/* Number of pixels along Y axis.			*/
	float *			pImageBuffer		/* Pointer to image data.					*/
	)
{

#ifdef USE_CFITSIO	/* USE FITSIO LIBRARY (WHEN MEMORY CORRUPTION BUG INVESTIGATED) */

	fitsfile *		fitsFp;				/* FITS file descriptor.					*/
	int				fitsStatus;			/* FITS I/O status.							*/
	long int		naxis;				/* Number of axes (assumed 2).				*/
	long int		naxes[2];			/* Size of each axis.						*/
	long int		fpixel;				/* First pixel.								*/
	long int		nelements;			/* Number of data elements.					*/
	int				timeArrayStart[7];	/* Array of year/month/day/hour/min/sec		*/
	int				timeArrayEnd[7];	/* Array of year/month/day/hour/min/sec		*/
	char			utStartString[20];	/* String to contain UTSTART description.	*/
	char			utEndString[20];	/* String to contain UTEND description.		*/
	double			elapsed;			/* Elapsed time in seconds.					*/

#else	/* USE OLD CODE */

	int				nPixels;			/* Number of pixels.						*/
	int				i;					/* Counter.									*/
	FILE *			fp;					/* File descriptor. */
	int				timeArrayStart[7];	/* Array of year/month/day/hour/min/sec		*/
	int				timeArrayEnd[7];	/* Array of year/month/day/hour/min/sec		*/

	int				headerCount;		/* Count of header items written.			*/

	float			fileBuffer[720];	/* 2880 byte buffer for FITS file.			*/
										/* [assumes sizeof(float)=4].				*/
	float*			pFileData;
	int				nBlocks;
	int				block;
	int				extra;

#endif	/* USE_CFITSIO */

	/*
	 * Check the parameters provided.
	 */

	if (pImageBuffer == NULL)
	{
		ERROR_SET(S_detControl_INTERNAL, "No image buffer defined", ERROR_LOG_SAVE);
		return (ERROR);
	}

	if ( (xPixels <= 0) || (yPixels <= 0) )
	{
		ERROR_SET2 (S_detControl_BAD_ATTRIBUTE,
			"Bad number of pixels given, %d X %d", ERROR_LOG_SAVE,
			xPixels, yPixels);
		return (ERROR);
	}


	/*
	 * Convert the time stamps from Gemini raw time into Universal Time
	 * and construct these into character strings.
	 */

	if (timeThenC( obsId->rawtStart, UT1, 2, timeArrayStart ) != OK)
	{
		ERROR_SET (0, "Failed to convert time stamp at observation start to date/time",
			ERROR_LOG_NOW);
	}

#ifdef USE_CFITSIO
	sprintf (utStartString, "%04d-%02d-%02d:%02d:%02d:%02d",
	         timeArrayStart[0], timeArrayStart[1], timeArrayStart[2],
	         timeArrayStart[3], timeArrayStart[4], timeArrayStart[5]);
#endif

	if (timeThenC( obsId->rawtEnd, UT1, 2, timeArrayEnd ) != OK)
	{
		ERROR_SET (0, "Failed to convert time stamp at observation end to date/time",
			ERROR_LOG_NOW);
	}

#ifdef USE_CFITSIO
	sprintf (utEndString, "%04d-%02d-%02d:%02d:%02d:%02d",
	         timeArrayEnd[0], timeArrayEnd[1], timeArrayEnd[2],
	         timeArrayEnd[3], timeArrayEnd[4], timeArrayEnd[5]);
#endif

/*
 * Using the cFitsIO library is a much better way of creating FITS files.
 * Unfortunately there is a memory corruption problem. The following
 * test will use the old code. Restore USE_CFITSIO when problem solved.
 */

#ifdef USE_CFITSIO

	/*
	 * Create a FITS file and write the data into it.
	 */

	fitsStatus = 0;
	fits_create_file ( &fitsFp, filename, &fitsStatus );
	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET2 (0 ,"Failed to create FITS file %s (fitsStatus=%d)", ERROR_LOG_SAVE,
			filename, fitsStatus);
		return (ERROR);
	}

	naxis = 2;
	naxes[0] = (long) xPixels;
	naxes[1] = (long) yPixels;
	fits_create_img ( fitsFp, FLOAT_IMG, naxis, naxes, &fitsStatus );
	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET1 (0, "Failed to create FITS image (fitsStatus=%d)", ERROR_LOG_SAVE, fitsStatus);
		return (ERROR);
	}

	fpixel = 1;
	nelements = xPixels * yPixels;
	fits_write_img ( fitsFp, TFLOAT, fpixel, nelements, pImageBuffer, &fitsStatus );
	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET1 (0, "Failed to write FITS image (fitsStatus=%d)", ERROR_LOG_SAVE, fitsStatus);
		return (ERROR);
	}

	/*
	 * Add the FITS keywords describing the exposure timing to the file.
	 */

	fits_update_key ( fitsFp, TSTRING, "UTSTART", utStartString, "UT at observation start", &fitsStatus );
	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET1 (0, "Failed to write UTSTART keyword (fitsStatus=%d)", ERROR_LOG_SAVE, fitsStatus);
		return (ERROR);
	}

	fits_update_key ( fitsFp, TSTRING, "UTEND", utEndString, "UT at observation end", &fitsStatus );
	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET1 (0, "Failed to write UTEND keyword (fitsStatus=%d)", ERROR_LOG_SAVE, fitsStatus);
		return (ERROR);
	}

	fits_update_key ( fitsFp, TDOUBLE, "EXPTIME", &(obsId->exposed), "Total exposure time", &fitsStatus );
	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET1 (0, "Failed to write EXPTIME keyword (fitsStatus=%d)", ERROR_LOG_SAVE, fitsStatus);
		return (ERROR);
	}

	elapsed = obsId->rawtEnd - obsId->rawtStart;
	fits_update_key ( fitsFp, TDOUBLE, "ELAPSED", &elapsed, "Total elapsed time", &fitsStatus );
	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET1 (0, "Failed to write ELAPSED keyword (fitsStatus=%d)", ERROR_LOG_SAVE, fitsStatus);
		return (ERROR);
	}

	/*
	 * Add the FITS keywords describing the World Coordinate System information to the file.
	 *
	 * TBD.
	 */

	/* Tidy up */

	fits_close_file ( fitsFp, &fitsStatus );

	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET1 (0, "Failed to close FITS output file (fitsStatus=%d)", ERROR_LOG_SAVE, fitsStatus);
		return (ERROR);
	}

#else	/* USE OLD CODE BELOW */

	fp = fopen (filename, "w");

	if (fp == NULL)
	{
		ERROR_SET(0, "Can't create/open FITS file", ERROR_LOG_SAVE);
		return (ERROR);
	}

	headerCount = 0;

	fprintf (fp, "SIMPLE  =                    T /                                                ");
	headerCount++;
	fprintf (fp, "BITPIX  =                  -32 /                                                ");
	headerCount++;
	fprintf (fp, "NAXIS   =                    2 /                                                ");
	headerCount++;
	fprintf (fp, "NAXIS1  =                %5d /                                                ", xPixels);
	headerCount++;
	fprintf (fp, "NAXIS2  =                %5d /                                                ", yPixels);
	headerCount++;
	fprintf (fp, "BZERO   =                    0 /                                                ");
	headerCount++;
	fprintf (fp, "EXTEND  =                    T /                                                ");
	headerCount++;
	fprintf (fp, "UTSTART ='%04d-%02d-%02d:%02d:%02d:%02d' /                                                ",
	         timeArrayStart[0], timeArrayStart[1], timeArrayStart[2], timeArrayStart[3], timeArrayStart[4],
	         timeArrayStart[5]);
	headerCount++;
	fprintf (fp, "UTEND   ='%04d-%02d-%02d:%02d:%02d:%02d' /                                                ",
	         timeArrayEnd[0], timeArrayEnd[1], timeArrayEnd[2], timeArrayEnd[3], timeArrayEnd[4],
	         timeArrayEnd[5]);
	headerCount++;

	fprintf (fp, "EXPTIME =      %15f /                                                ", obsId->exposed);
	headerCount++;
	fprintf (fp, "ELAPSED =      %15f /                                                ", (obsId->rawtEnd - obsId->rawtStart));
	headerCount++;
	fprintf (fp, "TELESCOP='%20s'/                                                ", "Gemini");
	headerCount++;
	fprintf (fp, "INSTRUME='%20s'/                                                ", obsId->pWfsName);
	headerCount++;
	fprintf (fp, "OBSTYPE ='%20s'/                                                ", obsId->pObsType);
	headerCount++;

	if ( obsId->wcsStatus == 0 )
	{
		fprintf (fp, "CTYPE1  ='%20s'/                                                ", obsId->ctype1);
		headerCount++;
		fprintf (fp, "CRPIX1  =      %15f /                                                ", obsId->crpix1);
		headerCount++;
		fprintf (fp, "CRVAL1  =      %15f /                                                ", obsId->crval1);
		headerCount++;
		fprintf (fp, "CTYPE2  ='%20s'/                                                ", obsId->ctype2);
		headerCount++;
		fprintf (fp, "CRPIX2  =      %15f /                                                ", obsId->crpix2);
		headerCount++;
		fprintf (fp, "CRVAL2  =      %15f /                                                ", obsId->crval2);
		headerCount++;
		fprintf (fp, "CD1_1   =      %15f /                                                ", obsId->cd1_1);
		headerCount++;
		fprintf (fp, "CD1_2   =      %15f /                                                ", obsId->cd1_2);
		headerCount++;
		fprintf (fp, "CD2_1   =      %15f /                                                ", obsId->cd2_1);
		headerCount++;
		fprintf (fp, "CD2_2   =      %15f /                                                ", obsId->cd2_2);
		headerCount++;
		fprintf (fp, "RADECSYS='%20s'/                                                ", obsId->radecsys);
		headerCount++;
	}

	fprintf (fp, "RA      =      %15f /                                                ", obsId->RA);
	headerCount++;
	fprintf (fp, "DEC     =      %15f /                                                ", obsId->Dec);
	headerCount++;
	fprintf (fp, "EQUINOX =      %15f /                                                ", obsId->equinox);
	headerCount++;
	fprintf (fp, "MJDOBS  =      %15f /                                                ", obsId->mjdobs);
	headerCount++;
	fprintf (fp, "END                                                                             ");
	headerCount++;

	/*
	 * Fill up the remaining header records (which must be a whole number of
	 * 2880-byte or 36-line blocks) with blanks.
	 */

	headerCount = headerCount % 36;

	for ( i=headerCount; i<36; i++)
	{
		fprintf (fp, "                                                                                ");
	}


	/* write image data in 2880-byte blocks */

	nPixels = xPixels * yPixels;

	nBlocks = nPixels / 720;
	extra	= nPixels % 720;

	pFileData = pImageBuffer;
	for (block=0; block<nBlocks; block++)
	{
		if ( fwrite (pFileData, sizeof (float), 720, fp) != 720 )
		{
			ERROR_SET(0, "Problem writing FITS output file", ERROR_LOG_SAVE);
			fclose (fp);
			return (ERROR);
		}
		pFileData += 720;
	}

	if ( extra > 0 )
	{
		for (i=0; i<extra; i++)
			fileBuffer[i] = *pFileData++;
		for (; i<720; i++)						/* Pad remainder of block */
			fileBuffer[i] = 0.0;

		if ( fwrite (fileBuffer, sizeof (float), 720, fp) != 720 )
		{
			ERROR_SET(0, "Problem writing FITS output file", ERROR_LOG_SAVE);
			fclose (fp);
			return (ERROR);
		}
	}
	
	/* tidy up */

	if (fclose (fp))
	{
		ERROR_SET(0, "Problem closing FITS output file", ERROR_LOG_SAVE);
		return (ERROR);
	}

#endif	/* USE_CFITSIO */

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detWriteFitsUint16
 *
 *	INVOCATION:
 *	detWriteFitsUint16 (filename. obsId, xPixels, yPixels, pImageBuffer)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	filename		(char *)		Name of file to contain data.
 *	(>)	obsId			(OBS_ID)		Current observation context structure
 *	(>)	xPixels			(int)			Number of pixels along X axis
 *	(>)	yPixels			(int)			Number of pixels along Y axis
 *	(!) pImageBuffer	(uint16 *)		Pointer to image buffer
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if command successful, ERROR if unsuccessful
 *
 *	PURPOSE:
 *	Write unsigned short integer data to FITS file (TEMPORARY FUNCTION)
 *
 *	DESCRIPTION:
 *	This function writes the contents of the frame buffer to a FITS file.
 *
 *  ACKNOWLEDGEMENTS:
 *	This function is based on a private function provided by Andrew Johnson.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	It is assumed that pImageBuffer points to a buffer of memory containing
 *	xPixels*yPixels unsigned short integer pixel values.
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	This function does not write very good FITS files. It needs to be rewritten
 *	to use the cFitsio library.
 *-
 */

STATUS detWriteFitsUint16
	(
	char *			filename,			/* Name of file to be written.				*/
	OBS_ID			obsId,				/* Current observation context structure.	*/
	int				xPixels,			/* Number of pixels along X axis.			*/
	int				yPixels,			/* Number of pixels along Y axis.			*/
	uint16 *		pImageBuffer		/* Pointer to image data.					*/
	)
{

#ifdef USE_CFITSIO	/* USE FITSIO LIBRARY (WHEN MEMORY CORRUPTION BUG INVESTIGATED) */

	fitsfile *		fitsFp;				/* FITS file descriptor.					*/
	int				fitsStatus;			/* FITS I/O status.							*/
	long int		naxis;				/* Number of axes (assumed 2).				*/
	long int		naxes[2];			/* Size of each axis.						*/
	long int		fpixel;				/* First pixel.								*/
	long int		nelements;			/* Number of data elements.					*/
	int				timeArrayStart[7];	/* Array of year/month/day/hour/min/sec		*/
	int				timeArrayEnd[7];	/* Array of year/month/day/hour/min/sec		*/
	char			utStartString[20];	/* String to contain UTSTART description.	*/
	char			utEndString[20];	/* String to contain UTEND description.		*/
	double			elapsed;			/* Elapsed time in seconds.					*/

#else	/* USE OF CODE */

	int				nPixels;			/* Number of pixels.						*/
	int				i;					/* Counter.									*/
	uint16 *		ptr;				/* Pointer into image buffer.				*/
	uint16			value;				/* Value to write into image buffer.		*/
	FILE *			fp;					/* File descriptor. */
	int				timeArrayStart[7];	/* Array of year/month/day/hour/min/sec		*/
	int				timeArrayEnd[7];	/* Array of year/month/day/hour/min/sec		*/

	int				headerCount;		/* Count of header items written.			*/

	uint16			fileBuffer[1440];	/* 2880 byte buffer for FITS file.			*/
										/* [assumes sizeof(uint16)=2].				*/
	uint16*			pFileData;
	int				nBlocks;
	int				block;
	int				extra;

#endif	/* USE_CFITSIO */

	/*
	 * Check the parameters provided.
	 */

	if (pImageBuffer == NULL)
	{
		ERROR_SET (S_detControl_INTERNAL, "No image buffer defined", ERROR_LOG_SAVE);
		return (ERROR);
	}

	if ( (xPixels <= 0) || (yPixels <= 0) )
	{
		ERROR_SET2 (S_detControl_BAD_ATTRIBUTE,
			"Bad number of pixels given, %d X %d", ERROR_LOG_SAVE,
			xPixels, yPixels);
		return (ERROR);
	}


	/*
	 * Convert the time stamps from Gemini raw time into Universal Time
	 * and construct these into character strings.
	 */

	if (timeThenC ( obsId->rawtStart, UT1, 2, timeArrayStart ) != OK)
	{
		ERROR_SET (0, "Failed to convert time stamp at observation start to date/time",
			ERROR_LOG_NOW);
	}

#ifdef USE_CFITSIO
	sprintf (utStartString, "%04d-%02d-%02d:%02d:%02d:%02d",
	         timeArrayStart[0], timeArrayStart[1], timeArrayStart[2],
	         timeArrayStart[3], timeArrayStart[4], timeArrayStart[5]);
#endif

	if (timeThenC ( obsId->rawtEnd, UT1, 2, timeArrayEnd ) != OK)
	{
		ERROR_SET (0, "Failed to convert time stamp at observation end to date/time",
			ERROR_LOG_NOW);
	}

#ifdef USE_CFITSIO
	sprintf (utEndString, "%04d-%02d-%02d:%02d:%02d:%02d",
	         timeArrayEnd[0], timeArrayEnd[1], timeArrayEnd[2],
	         timeArrayEnd[3], timeArrayEnd[4], timeArrayEnd[5]);
#endif


#ifdef USE_CFITSIO	/* USE FITSIO LIBRARY (WHEN MEMORY CORRUPTION BUG INVESTIGATED) */

	/*
	 * Create a FITS file and write the data into it.
	 */

	fitsStatus = 0;
	fits_create_file ( &fitsFp, filename, &fitsStatus );
	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET2 (0 ,"Failed to create FITS file %s (fitsStatus=%d)", ERROR_LOG_SAVE,
			filename, fitsStatus);
		return (ERROR);
	}

	naxis = 2;
	naxes[0] = (long) xPixels;
	naxes[1] = (long) yPixels;
	fits_create_img ( fitsFp, USHORT_IMG, naxis, naxes, &fitsStatus );
	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET1 (0, "Failed to create FITS image (fitsStatus=%d)", ERROR_LOG_SAVE, fitsStatus);
		return (ERROR);
	}

	fpixel = 1;
	nelements = xPixels * yPixels;
	fits_write_img ( fitsFp, TUSHORT, fpixel, nelements, pImageBuffer, &fitsStatus );
	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET1 (0, "Failed to write FITS image (fitsStatus=%d)", ERROR_LOG_SAVE, fitsStatus);
		return (ERROR);
	}

	/*
	 * Add the FITS keywords describing the exposure timing to the file.
	 */

	fits_update_key ( fitsFp, TSTRING, "UTSTART", utStartString, "UT at observation start", &fitsStatus );
	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET1 (0, "Failed to write UTSTART keyword (fitsStatus=%d)", ERROR_LOG_SAVE, fitsStatus);
		return (ERROR);
	}

	fits_update_key ( fitsFp, TSTRING, "UTEND", utEndString, "UT at observation end", &fitsStatus );
	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET1 (0, "Failed to write UTEND keyword (fitsStatus=%d)", ERROR_LOG_SAVE, fitsStatus);
		return (ERROR);
	}

	fits_update_key ( fitsFp, TDOUBLE, "EXPTIME", &(obsId->exposed), "Total exposure time", &fitsStatus );
	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET1 (0, "Failed to write EXPTIME keyword (fitsStatus=%d)", ERROR_LOG_SAVE, fitsStatus);
		return (ERROR);
	}

	elapsed = obsId->rawtEnd - obsId->rawtStart;
	fits_update_key ( fitsFp, TDOUBLE, "ELAPSED", &elapsed, "Total elapsed time", &fitsStatus );
	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET1 (0, "Failed to write ELAPSED keyword (fitsStatus=%d)", ERROR_LOG_SAVE, fitsStatus);
		return (ERROR);
	}

	/*
	 * Add the FITS keywords describing the World Coordinate System information to the file.
	 *
	 * TBD.
	 */

	/* Tidy up */

	fits_close_file ( fitsFp, &fitsStatus );

	if ( fitsStatus > 0 )
	{
		fits_report_error (stderr, fitsStatus);
		ERROR_SET1 (0, "Failed to close FITS output file (fitsStatus=%d)", ERROR_LOG_SAVE, fitsStatus);
		return (ERROR);
	}

#else	/* USE OLD CODE */

	fp = fopen (filename, "w");

	if (fp == NULL)
	{
		ERROR_SET(0, "Can't create/open FITS file", ERROR_LOG_SAVE);
		return (ERROR);
	}

	headerCount = 0;

	fprintf (fp, "SIMPLE  =                    T /                                                ");
	headerCount++;
	fprintf (fp, "BITPIX  =                  -32 /                                                ");
	headerCount++;
	fprintf (fp, "NAXIS   =                    2 /                                                ");
	headerCount++;
	fprintf (fp, "NAXIS1  =                %5d /                                                ", xPixels);
	headerCount++;
	fprintf (fp, "NAXIS2  =                %5d /                                                ", yPixels);
	headerCount++;
	fprintf (fp, "BZERO   =                    0 /                                                ");
	headerCount++;
	fprintf (fp, "EXTEND  =                    T /                                                ");
	headerCount++;
	fprintf (fp, "UTSTART ='%04d-%02d-%02d:%02d:%02d:%02d' /                                                ",
	         timeArrayStart[0], timeArrayStart[1], timeArrayStart[2], timeArrayStart[3], timeArrayStart[4],
	         timeArrayStart[5]);
	headerCount++;
	fprintf (fp, "UTEND   ='%04d-%02d-%02d:%02d:%02d:%02d' /                                                ",
	         timeArrayEnd[0], timeArrayEnd[1], timeArrayEnd[2], timeArrayEnd[3], timeArrayEnd[4],
	         timeArrayEnd[5]);
	headerCount++;

	fprintf (fp, "EXPTIME =      %15f /                                                ", obsId->exposed);
	headerCount++;
	fprintf (fp, "ELAPSED =      %15f /                                                ", (obsId->rawtEnd - obsId->rawtStart));
	headerCount++;
	fprintf (fp, "TELESCOP='%20s'/                                                ", "Gemini");
	headerCount++;
	fprintf (fp, "INSTRUME='%20s'/                                                ", obsId->pWfsName);
	headerCount++;
	fprintf (fp, "OBSTYPE ='%20s'/                                                ", obsId->pObsType);
	headerCount++;

	if ( obsId->wcsStatus == 0 )
	{
		fprintf (fp, "CTYPE1  ='%20s'/                                                ", obsId->ctype1);
		headerCount++;
		fprintf (fp, "CRPIX1  =      %15f /                                                ", obsId->crpix1);
		headerCount++;
		fprintf (fp, "CRVAL1  =      %15f /                                                ", obsId->crval1);
		headerCount++;
		fprintf (fp, "CTYPE2  ='%20s'/                                                ", obsId->ctype2);
		headerCount++;
		fprintf (fp, "CRPIX2  =      %15f /                                                ", obsId->crpix2);
		headerCount++;
		fprintf (fp, "CRVAL2  =      %15f /                                                ", obsId->crval2);
		headerCount++;
		fprintf (fp, "CD1_1   =      %15f /                                                ", obsId->cd1_1);
		headerCount++;
		fprintf (fp, "CD1_2   =      %15f /                                                ", obsId->cd1_2);
		headerCount++;
		fprintf (fp, "CD2_1   =      %15f /                                                ", obsId->cd2_1);
		headerCount++;
		fprintf (fp, "CD2_2   =      %15f /                                                ", obsId->cd2_2);
		headerCount++;
		fprintf (fp, "RADECSYS='%20s'/                                                ", obsId->radecsys);
		headerCount++;
	}

	fprintf (fp, "RA      =      %15f /                                                ", obsId->RA);
	headerCount++;
	fprintf (fp, "DEC     =      %15f /                                                ", obsId->Dec);
	headerCount++;
	fprintf (fp, "EQUINOX =      %15f /                                                ", obsId->equinox);
	headerCount++;
	fprintf (fp, "MJDOBS  =      %15f /                                                ", obsId->mjdobs);
	headerCount++;
	fprintf (fp, "END                                                                             ");
	headerCount++;

	/*
	 * Fill up the remaining header records (which must be a whole number of
	 * 2880-byte or 36-line blocks) with blanks.
	 */

	headerCount = headerCount % 36;

	for ( i=headerCount; i<36; i++)
	{
		fprintf (fp, "                                                                                ");
	}

	/*
	 * Subtract 32768 from the image data to counteract the BZERO=32768 in the FITS header.
	 * (This is necessary because FITS readers will assume the data are signed).
	 */

	nPixels = xPixels * yPixels;

	ptr = pImageBuffer;
	for ( i=0; i<nPixels; i++)
	{
		value = (int) *(ptr);
		value -= 32768;
		*(ptr) = (uint16) value;
		ptr++;
	}

	/* write image data in 2880-byte blocks */

	nPixels = xPixels * yPixels;

	nBlocks = nPixels / 1440;
	extra	= nPixels % 1440;

	pFileData = pImageBuffer;
	for (block=0; block<nBlocks; block++)
	{
		if ( fwrite (pFileData, sizeof (uint16), 1440, fp) != 1440 )
		{
			ERROR_SET(0, "Problem writing FITS output file", ERROR_LOG_SAVE);
			fclose (fp);
			return (ERROR);
		}
		pFileData += 1440;
	}

	if ( extra > 0 )
	{
		for (i=0; i<extra; i++)
			fileBuffer[i] = *pFileData++;
		for (; i<1440; i++)						/* Pad remainder of block */
			fileBuffer[i] = 0;

		if ( fwrite (fileBuffer, sizeof (uint16), 1440, fp) != 1440 )
		{
			ERROR_SET(0, "Problem writing FITS output file", ERROR_LOG_SAVE);
			fclose (fp);
			return (ERROR);
		}
	}

	/* tidy up */

	if (fclose (fp))
	{
		ERROR_SET(0, "Problem closing FITS output file", ERROR_LOG_SAVE);
		return (ERROR);
	}

#endif	/* USE CFITSIO */

	return (OK);
}

/* ------------------------------------------------------------------------------------------------ */

void detDhsErrorCallback			/* DHS error callback function.						*/
	(
	DHS_CONNECT		connect,		/* DHS connection ID for connection causing error.	*/
	DHS_STATUS		errorNum,		/* DHS error number.								*/
	DHS_ERR_LEVEL	errorLev,		/* DHS error level.									*/
	char *			msg,			/* DHS error message string.						*/
	DHS_TAG			tag,			/* DHS command tag of the error.					*/
	void *			userData		/* Pointer to user data (if any).					*/
	)
{
	printErr ("DHS error callback: connection=%d errNum=%d level=%d \"%s\"\n",
		(int) connect, (int) errorNum, (int) errorLev, msg);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detDhsInit
 *
 *	INVOCATION:
 *	detDhsInit (pClientName, numConnect, pHostName, pSeverName)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pClientName		(const char *)		Unique name for DHS client.
 *	(>) numConnect		(const int)			Maximum number of DHS connections.
 *	(>) pHostName		(const char *)		Name of DHS data server host.
 *	(>)	pServerName		(const char *)		Name of DHS data server.
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if command successful, ERROR if unsuccessful
 *
 *	PURPOSE:
 *	Initialise the DHS library and define DHS server information
 *
 *	DESCRIPTION:
 *	This function initialises the DHS library and sets up the DHS server information
 *	used by the detector controller.
 *
 *	EXTERNAL VARIABLES:
 *	(<)	detDhsInitialised	(BOOL)			DHS initialised flag.
 *	(<)	detDhsSem			(SEM_ID)		DHS semaphore
 *	(<)	pDetDhsClientName	(char *)		Current name of DHS client = Instrument name.
 *	(<)	pDetDhsHostName		(char *)		Current name of DHS server host.
 *	(<)	pDetDhsServerName	(char *)		Current name of DHS server.
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *	dhs.h
 *
 *	DEFICIENCIES:
 *	None known
 *
 *	BUGS:
 *	There is a bug in the DHS event loop which causes it to hang up the VxWorks crate
 *	when an attempt is made to start it. This call is commented out, which means that
 *	a DHS event loop will not be running. Without an event loop the software must call
 *	dhsWait() explicitly to wait for the completion of each asynchronous command.
 *	SMB - 14 Sep 1998.
 *
 *	It is rumoured that the above bug has been fixed. Event loop call tentatively
 *	restored. SMB - 16 Nov 1998.
 *-
 */

STATUS detDhsInit
	(
	const char *	pClientName,		/* Unique name of DHS client.				*/
	const int		numConnect,			/* Maximum number of DHS connections.		*/
	const char *	pHostName,			/* Name of data server host.				*/
	const char *	pServerName			/* Name of server.							*/
	)
{
	DHS_STATUS		dhsErrno;			/* DHS error number.						*/
	DHS_THREAD		dhsThreadId;		/* DHS thread ID.							*/

	/* Initialise the DHS error number. */

	dhsErrno = DHS_S_SUCCESS;			/* <--- DHS error number initialised here. */

	/*
	 * Check the DHS library has not already been initialised.
	 */

	if (detDhsInitialised)
	{
		ERROR_SET (S_detControl_DHS_ERROR, "DHS already initialised", ERROR_LOG_NOW);
		return (ERROR);
	}

	/* Create the DHS semaphore and take it, ensuring that only one task attempts to initialise
	 * the DHS and update the DHS global variables.
	 */

	detDhsSem = semMCreate( SEM_Q_FIFO | SEM_DELETE_SAFE );
	if ( (detDhsSem == NULL) || (semTake (detDhsSem, NO_WAIT) == ERROR) )
	{
		ERROR_SET (0, "Failed to create and take DHS semaphore", ERROR_LOG_NOW);
		semGive (detDhsSem);
		return (ERROR);
	}

	/*
	 * Initialise the DHS, specifying a unique name and maximum number of connections.
	 */

#ifdef DEBUG
	printf ("detDhsInit: dhsInit pClientName=%s numConnect=%d\n", pClientName, numConnect);
#endif /* DEBUG */

	dhsInit (pClientName, numConnect, &dhsErrno);
	CHECK_DHS (dhsErrno);

	if (dhsErrno != DHS_S_SUCCESS)
	{
		ERROR_SET1 (S_detControl_DHS_ERROR, "Failed to initialise DHS (dhsErrno=%d)",
		            ERROR_LOG_SAVE, dhsErrno);
		semGive (detDhsSem);
		return (ERROR);
	}

	/* Set up callbacks. */

#ifdef DEBUG
	printf ("detDhsInit: dhsCallbackSet DHS_CBT_ERROR=%d detDhsErrorCallback=%p\n",
			DHS_CBT_ERROR, detDhsErrorCallback);
#endif /* DEBUG */

	dhsCallbackSet (DHS_CBT_ERROR, detDhsErrorCallback, &dhsErrno);
	CHECK_DHS (dhsErrno);

	if (dhsErrno != DHS_S_SUCCESS)
	{
		ERROR_SET1 (S_detControl_DHS_ERROR, "Failed to set up DHS error callback (dhsErrno=%d)",
			ERROR_LOG_SAVE, dhsErrno);
		semGive (detDhsSem);
		return (ERROR);
	}

	/*
	 * Start the DHS event loop.
	 *
	 * BUG WORK AROUND - THIS CODE COMMENTED OUT - SEE "BUGS" SECTION IN HEADER.
	 * REINSTATED - SMB 16 NOV 98
	 */

#ifdef DEBUG
	printf ("detDhsInit: dhsEventLoop DHS_ELT_THREADED=%d ... ", DHS_ELT_THREADED);
#endif /* DEBUG */

	dhsEventLoop (DHS_ELT_THREADED, &dhsThreadId, &dhsErrno);
	CHECK_DHS (dhsErrno);

#ifdef DEBUG
	printf ("dhsThreadId=%d dhsErrno=%d\n", dhsThreadId, dhsErrno);
#endif /* DEBUG */

	if (dhsErrno != DHS_S_SUCCESS)
	{
		ERROR_SET1 (S_detControl_DHS_ERROR, "Failed to start DHS event loop (dhsErrno=%d)",
			ERROR_LOG_SAVE, dhsErrno);
		semGive (detDhsSem);
		return (ERROR);
	}

	/* Store the given client name, host name and server name in global variables. */

	strncpy (pDetDhsClientName, pClientName, EPICS_MAX_BYTES_STRING_ATTRIB);
	strncpy (pDetDhsHostName, pHostName, EPICS_MAX_BYTES_STRING_ATTRIB);
	strncpy (pDetDhsServerName, pServerName, EPICS_MAX_BYTES_STRING_ATTRIB);

	/* Finally, set the detDhsInitialised flag and return the semaphore. */

	detDhsInitialised = TRUE;
	semGive (detDhsSem);

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detDhsConnect
 *
 *	INVOCATION:
 *	detDhsConnect (pWfsName, pDhsConnection)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(<) pDhsConnection	(DHS_CONNECT *)		Pointer to DHS connection ID
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if command successful, ERROR if unsuccessful
 *
 *	PURPOSE:
 *	Initialise connection to DHS for a particular WFS
 *
 *	DESCRIPTION:
 *	This function initialises the connection to the DHS.
 *
 *	EXTERNAL VARIABLES:
 *	(>)	detDhsInitialised	(BOOL)			DHS initialised flag
 *	(>)	pDetDhsHostName		(char *)		DHS server host name
 *	(>)	pDetDhsServerName	(char *)		DHS server name
 *
 *	PRIOR REQUIREMENTS:
 *	The DHS library should already have been initialised by calling detDhsInit.
 *
 *	INCLUDE FILES:
 *	detControl.h
 *	dhs.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS detDhsConnect
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	DHS_CONNECT *	pDhsConnection		/* Pointer to DHS connection.				*/
	)
{
	DHS_STATUS		dhsErrno;			/* DHS error number.						*/


	/* Initialise the DHS error number. */

	dhsErrno = DHS_S_SUCCESS;

	/*
	 * Check the DHS library has been initialised.
	 */

	if (!detDhsInitialised)
	{
		ERROR_SET (S_detControl_DHS_ERROR, "DHS not initialised", ERROR_LOG_NOW);
		return (ERROR);
	}

	/*
	 * Take the DHS semaphore, so that only one WFS attempts to connect to the DHS
	 * and access the pDetDhsHostName and pDetDhsServerName global variables at any one time.
	 */

#ifdef DEBUG
	printf ("detDhsConnect: Taking DHS semaphore for WFS %s...\n", pWfsName);
#endif /* DEBUG */

	if ( semTake (detDhsSem, DHS_WAIT_TIMEOUT) == ERROR )
	{
		ERROR_SET (0, "Failed to take DHS semaphore", ERROR_LOG_NOW);
		semGive (detDhsSem);
		return (ERROR);
	}

	/*
	 * Connect to the DHS server. There is no user data to be supplied (hence NULL).
	 */

	MESSAGE_LOG2 (MSG_LOG, "Connecting to DHS server %s on host %s",
		pDetDhsServerName, pDetDhsHostName);

	*pDhsConnection = dhsConnect (pDetDhsHostName, pDetDhsServerName, NULL, &dhsErrno);
	CHECK_DHS (dhsErrno);

#ifdef DEBUG
	printf ("dhsConnect: dhsConnection=%ld dhsErrno=%d\n", *pDhsConnection, dhsErrno);
#endif /* DEBUG */

	if (dhsErrno != DHS_S_SUCCESS)
	{
		ERROR_SET3 (S_detControl_DHS_ERROR, "Failed to connect to DHS server %s on %s (dhsErrno=%d)",
			ERROR_LOG_SAVE, pDetDhsServerName, pDetDhsHostName, dhsErrno);
		semGive (detDhsSem);
		return (ERROR);
	}

	/* Finally, return the semaphore. */

	semGive (detDhsSem);

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detDhsCheckErrno
 *
 *	INVOCATION:
 *	detDhsCheckErrno (dhsErrno, line, filename)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	dhsErrno	(const DHS_STATUS)		DHS error number (unchanged)
 *	(>) line		(const int)			Line number to report
 *	(>)	filename	(const char *)			File name to report
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Check DHS error number and report any error messages
 *
 *	DESCRIPTION:
 *	This function checks the DHS error number provided. If the status suggests an error has occurred,
 *	the dhsMessage() functions are used to extract information from the DHS message stack.
 *	This function should be called after every DHS function to ensure all the relevant DHS
 *	errors are reported.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *	dhs.h
 *
 *	DEFICIENCIES:
 *	None known
 *
 *	BUGS:
 *	This function appears to cause problems with the DHS. Corinne Boyer knows the details.
 *	For the time being its contents are commented out and replaced by a trivial report.
 *	SMB - 17 Jan 1999. 
 *-
 */

void detDhsCheckErrno
	(
	const DHS_STATUS	dhsErrno,			/* DHS error number.					*/
	const int			line,				/* Line number.							*/
	const char *		filename			/* File name.							*/
	)
{
#ifdef DHS_PROBLEM_SOLVED
	DHS_ERR_LEVEL		errorLevel;			/* DHS error level.						*/
	char *				errorString;		/* DHS error string.					*/
	DHS_STATUS			dhsErrno2;			/* Second DHS error number.				*/
	DHS_STATUS			errorNum;			/* Third error number.					*/
	int					counter;			/* Counter to prevent infinite loop.	*/
	char 				messBuff[160];		/* Message buffer.						*/
#endif	/* DHS_PROBLEM_SOLVED */

	/*
	 * If the DHS error number is ok, this function will return without doing anything.
	 */

	if ( dhsErrno != DHS_S_SUCCESS )
	{

#ifdef DHS_PROBLEM_SOLVED

		/*
		 * A DHS error has occurred. Wind through the DHS stack, displaying all
		 * the messages found and clearing each message after it has been extracted.
		 */

		counter = 0;
		dhsErrno2 = DHS_S_SUCCESS;
		while ( (dhsErrno2 == DHS_S_SUCCESS) && (counter < 10) )
		{
			errorString = dhsMessage( &errorNum, &errorLevel, &dhsErrno2 );

			if ( dhsErrno2 == DHS_S_SUCCESS )
			{
				sprintf ( messBuff, "DHS error: %d, level: %d, message: \"%s\"",
					dhsErrno, errorLevel, errorString );
				errorSet ( line, filename, 0, messBuff, ERROR_LOG_NOW );
			}
			else if ( dhsErrno2 == DHS_S_NO_MESSAGE )
			{
				/*
				 * If the counter is less than 1 then no messages were found, even though
				 * there was a bad DHS status. Report a minimal message.
				 */

				if ( counter < 1 )
				{
					sprintf ( messBuff, "DHS error: %d, no messages queued", dhsErrno );
					errorSet ( line, filename, 0, messBuff, ERROR_LOG_NOW );
				}

				break;						/* No more messages. Terminate loop */
			}
			else
			{
				sprintf ( messBuff, "DHS error: %d, dhsMessage error: %d",
					dhsErrno, dhsErrno2 );
				errorSet ( line, filename, 0, messBuff, ERROR_LOG_NOW );

				ERROR_SET2( 0, "DHS error: %d, dhsMessage error: %d", ERROR_LOG_NOW,
					dhsErrno, dhsErrno2 );
				return;
			}
			dhsMessageClear( &dhsErrno2 );

			counter++;
		}

#else	/* use trivial report */

		errorSet ( line, filename, 0, "DHS error detected", ERROR_LOG_NOW );

#endif	/* DHS_PROBLEM_SOLVED */

	}

	return;
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detDhsCheckCmdStatus
 *
 *	INVOCATION:
 *	detDhsCheckCmdStatus (dhsTag)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	dhsTag	(const DHS_TAG)		DHS command tag (unchanged)
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if command successful, ERROR if unsuccessful
 *
 *	PURPOSE:
 *	Check and reports DHS command status 
 *
 *	DESCRIPTION:
 *	This function checks the DHS command status and reports a message if the status is
 *	not DHS_CS_DONE.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *	dhs.h
 *
 *	DEFICIENCIES:
 *	The DHS allocates a buffer to store the command status message. It would
 *	be more sensible if the buffer was allocated here and provided to the DHS,
 *	as there would then be more control over the buffer. At the moment the
 *	buffer has to be explicitly freed because the DHS does not do this.
 *	SMB - 17 Jan 1999.
 *
 *	I am informed that dhsMessageClear() should be used to free the message, but I still
 *	don't understand how dhsMessageClear is supposed to know which string to free.
 *	SMB - 15 Feb 1999.
 *
 *	BUG:
 *	Access faults seem to occur when dhsStatus() is used.
 *	For now this function is commented out.
 *	SMB - 29 Jan 1999 + 15 Feb 1999.
 *-
 */

STATUS detDhsCheckCmdStatus
	(
	const DHS_TAG	dhsTag					/* DHS command tag.				*/
	)
{
	DHS_CMD_STATUS		sendStatus;			/* DHS command status.			*/
	DHS_STATUS			dhsErrno;			/* DHS error number.			*/
	char				*msg = NULL;		/* dk. Command status message.  */


	/* Initialise the DHS error number */

	dhsErrno = DHS_S_SUCCESS;

	/*
	 * Query the command status associated with the tag.
	 * Note that the DHS allocates a buffer to hold the command status message
	 * and returns a pointer to this buffer in "msg".
	 */

#ifdef DEBUG
	printf ("detDhsCheckCmdStatus: dhsStatus\n");
#endif /* DEBUG */

	/* dhsStatus() COMMENTED OUT */

	/*sendStatus = dhsStatus (dhsTag, &msg, &dhsErrno);
	CHECK_DHS (dhsErrno);*/

	sendStatus = DHS_CS_DONE ;		/* FUDGE */

	/*
	 * Check that the query worked and report an error if it didn't.
	 * If the query returned DHS_CS_DONE nothing more needs to be done.
	 * Any other command status is reported as an error.
	 */

	if ( dhsErrno != DHS_S_SUCCESS )
	{

		ERROR_SET2 (0, "Failed to query DHS command status for tag %ld, (dhsErrno=%d)",
			ERROR_LOG_SAVE, dhsTag, dhsErrno);

		/* Free the message buffer if allocated. */
		/* if ( msg != NULL ) free (msg); */		/* Replace with dhsMessageClear(&dhsErrno) ??? */
		return (ERROR);
	}
	else if ( sendStatus != DHS_CS_DONE )
	{
		switch (sendStatus)
		{
			case (DHS_CS_IDLE):

				ERROR_SET1 (0, "Command still waiting to execute, %s", ERROR_LOG_SAVE, msg);
				break;

			case (DHS_CS_BUSY):

				ERROR_SET1 (0, "Command is still executing, %s", ERROR_LOG_SAVE, msg);
				break;
	
			case (DHS_CS_ERROR):

				ERROR_SET1 (0, "Command completed with error, %s",
				           ERROR_LOG_SAVE, msg);
				break;

			case (DHS_CS_ABORTED):

				ERROR_SET1 (0, "Command was aborted, %s.", ERROR_LOG_SAVE, msg);
				break;

			case (DHS_CS_PENDING):

				ERROR_SET1 (0, "Command is still pending, %s", ERROR_LOG_SAVE, msg);
				break;

			case (DHS_CS_LOST):

				ERROR_SET1 (0, "Connection was lost before command completed, %s",
				           ERROR_LOG_SAVE, msg);
				break;

			default:
				ERROR_SET2 (0, "Unknown command status, %d, %s", ERROR_LOG_SAVE, sendStatus, msg);
				break;
		}

		/* Free the message buffer if allocated. */
		/* if ( msg != NULL ) free (msg); */		/* Replace with dhsMessageClear(&dhsErrno) ??? */

		return (ERROR);
	}

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detDownloadDefault
 *
 *	INVOCATION:
 *	detDownloadDefault (pWfsName, pRecordPrefix, sdsuId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	pRecordprefix	(const char *)		Record name prefix
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if command successful, ERROR if unsuccessful
 *
 *	PURPOSE:
 *	Download default OMF files
 *
 *	DESCRIPTION:
 *	This function downloads DSP code from the default OMF files. Executed on startup.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS detDownloadDefault
	(
	const char *	pWfsName,			/* Name of wavefront sensor.				*/
	const char *	pRecordPrefix,		/* Record name prefix.						*/
	SDSU_ID			sdsuId				/* SDSU context structure.					*/
	)
{
	char			pFullOmfFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
										/* Combined path name and file name.		*/
	/* 
	 * Variables associated with "Download OMF file" command.
	 * (omfPath, vmeFile, timFile and utlFile use general filename parameters)
	 */

	BOOL			limitAdrsRange;		/* Flag for limiting address range in DSP memory */

	uint32			mode;

	/*
	 * Check there is a valid SDSU context structure.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		return (ERROR);
	}

	/*
	 * Download default OMF code to the VME DSP, unless the default file name is
	 * "NONE" or blank. If the code could not be downloaded, the controller health
	 * is set "BAD", since it cannot do anything until this code is downloaded.
	 */

	if ( (strcmp (DET_CONTROL_OMF_VME_FILE, "") != 0) &&
		 (strcmp (DET_CONTROL_OMF_VME_FILE, "NONE") != 0)
	   )
	{

		/* The ability to limit the address range is ignored. It is rarely needed
		 * and can only be done by executing sdsuFileDnload at the console (since
	 	 * sdsuFileDnload expects to prompt for the values).
		 */

		limitAdrsRange = 0;

		sprintf (pFullOmfFileName, "%s/%s", DET_CONTROL_OMF_FILE_PATH, DET_CONTROL_OMF_VME_FILE);
		MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to VME DSP...", pFullOmfFileName);

		if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_VME, limitAdrsRange) == ERROR)
		{
			ERROR_LOG ("Failed to download default OMF file to VME DSP");
			epToVxSetHealth( pRecordPrefix, "BAD" );
			return (ERROR);
		}
	}

	/*
	 * Download default OMF code to the TIMING DSP, unless the default file name is
	 * "NONE" or blank. If the code could not be downloaded, the controller health
	 * is set "BAD", since it cannot do anything until this code is downloaded.
	 * Note that different OMF code is used for the HRWFS because it uses a different
	 * CCD chip.
	 */

	if (strcmp (pWfsName, "hr") == 0)
	{

		if ( (strcmp (DET_CONTROL_HRWFS_OMF_TIM_FILE, "") != 0) &&
			 (strcmp (DET_CONTROL_HRWFS_OMF_TIM_FILE, "NONE") != 0)
		   )
		{

			/* The ability to limit the address range is ignored. It is rarely needed
			 * and can only be done by executing sdsuFileDnload at the console (since
		 	 * sdsuFileDnload expects to prompt for the values).
			 */

			limitAdrsRange = 0;

			sprintf (pFullOmfFileName, "%s/%s", DET_CONTROL_OMF_FILE_PATH,
			         DET_CONTROL_HRWFS_OMF_TIM_FILE);
			MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to TIMING DSP...", pFullOmfFileName);

			if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_TIM, limitAdrsRange) == ERROR)
			{
				ERROR_LOG ("Failed to download default OMF file to TIMING DSP");
				epToVxSetHealth( pRecordPrefix, "BAD" );
				return (ERROR);
			}
		}
	}
	else
	{

		if ( (strcmp (DET_CONTROL_GBD_OMF_TIM_FILE, "") != 0) &&
			 (strcmp (DET_CONTROL_GBD_OMF_TIM_FILE, "NONE") != 0)
		   )
		{

			/* The ability to limit the address range is ignored. It is rarely needed
			 * and can only be done by executing sdsuFileDnload at the console (since
		 	 * sdsuFileDnload expects to prompt for the values).
			 */

			limitAdrsRange = 0;

			sprintf (pFullOmfFileName, "%s/%s", DET_CONTROL_OMF_FILE_PATH,
			         DET_CONTROL_GBD_OMF_TIM_FILE);
			MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to TIMING DSP...", pFullOmfFileName);

			if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_TIM, limitAdrsRange) == ERROR)
			{
				ERROR_LOG ("Failed to download default OMF file to TIMING DSP");
				epToVxSetHealth( pRecordPrefix, "BAD" );
				return (ERROR);
			}
		}
	}


	/*
	 * Download default OMF code to the UTILITY DSP, unless the default file name is
	 * "NONE" or blank. If the code could not be downloaded, the controller health
	 * is set "BAD", since it cannot do anything until this code is downloaded.
	 */

	if ( (strcmp (DET_CONTROL_OMF_UTL_FILE, "") != 0) &&
		 (strcmp (DET_CONTROL_OMF_UTL_FILE, "NONE") != 0)
	   )
	{

		/* The ability to limit the address range is ignored. It is rarely needed
		 * and can only be done by executing sdsuFileDnload at the console (since
	 	 * sdsuFileDnload expects to prompt for the values).
		 */

		limitAdrsRange = 0;

		sprintf (pFullOmfFileName, "%s/%s", DET_CONTROL_OMF_FILE_PATH, DET_CONTROL_OMF_UTL_FILE);
		MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to UTILITY DSP...", pFullOmfFileName);

		if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_UTL, limitAdrsRange) == ERROR)
		{
			ERROR_LOG ("Failed to download default OMF file to UTILITY DSP");
			epToVxSetHealth( pRecordPrefix, "BAD" );
			return (ERROR);
		}
	}

	/*
	 * On the HRWFS the default packet size must be increased.
	 * The HRWFS should not run in sync mode to ensure adequate dymanic range.
	 * (It is not a serious error if this does not happen).
	 *
	 * THIS BLOCK OF CODE CAN BE REMOVED WHEN TIM HARDY'S NEW DSP CODE SETS
	 * APPROPRIATE DEFAULTS.
	 */

	if ( strcmp (pWfsName, "hr") == 0 )
	{
		if (sdsuParamWrite (sdsuId, SDSU_IDENT_VME, "V_PSIZE", 1024) == ERROR)
		{
			ERROR_LOG ("Failed to increase the HRWFS packet size");
		}

		if (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_MODE", &mode) == ERROR)
		{
			ERROR_LOG ("Failed to read default mode from timing board");
		}
		else
		{
			if ( (mode & SDSU_TIM_MODE_SYNC) != 0 )
			{
				mode &= ~SDSU_TIM_MODE_SYNC;
				if (sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_MODE", mode) == ERROR)
				{
					ERROR_LOG ("Failed to clear sync mode on timing board");
				}
			}
		}
	}
	else
	{
		/* The default PWFS packet size should be larger when used in full frame mode. */

		if (sdsuParamWrite (sdsuId, SDSU_IDENT_VME, "V_PSIZE", 80) == ERROR)
		{
			ERROR_LOG ("Failed to increase the PWFS packet size");
		}
	}

	/*
	 * After successfully downloading new OMF code, the controller must be reinitialised
	 * by sending an "INI" command to the utility DSP and a "LDP" command to the timing DSP.
	 */

	if (sdsuPrimitive (sdsuId, "INI", SDSU_IDENT_UTL, NULL, NULL) == ERROR)
	{
		ERROR_LOG ("Failed to initialise UTILITY DSP with INI command");
		epToVxSetHealth( pRecordPrefix, "BAD" );
		return (ERROR);
	}
	if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
	{
		ERROR_LOG ("Failed to initialise TIMING DSP with LDP command");
		epToVxSetHealth( pRecordPrefix, "BAD" );
		return (ERROR);
	}

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detCheckGeometry
 *
 *	INVOCATION:
 *	detCheckGeometry (pWfsName, sdsuId, pxMax, pyMax, pxPixels, pyPixels)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)		Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	sdsuId			(SDSU_ID)			Current SDSU context structure
 *	(!)	pxMax			(int *)				Maximum number of X pixels expected
 *	(!)	pyMax			(int *)				Maximum number of Y pixels expected
 *	(!)	pxPixels		(int *)				Current number of X pixels expected
 *	(!)	pyPixels		(int *)				Current number of Y pixels expected
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if command successful, ERROR if unsuccessful
 *
 *	PURPOSE:
 *	Check and update default detector geometry
 *
 *	DESCRIPTION:
 *	This function compares the default detector geometry contained in xPixels, yPixels.
 *	with the default parameters defined by the SDSU DSP code and ensures that on exit
 *	xPixels, yPixels contain the maximum expected detector geometry. Executed on startup.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS detCheckGeometry
	(
	const char *	pWfsName,			/* Name of wavefront sensor.					*/
	SDSU_ID			sdsuId,				/* SDSU context structure.						*/
	int *			pxMax,				/* Maximum number of X pixels on detector.		*/
	int *			pyMax,				/* Maximum number of X pixels on detector.		*/
	int *			pxPixels,			/* Current number of X pixels on detector.		*/
	int *			pyPixels			/* Current number of X pixels on detector.		*/
	)
{
	uint32			xRas;				/* SDSU "number of X super pixels per			*/
										/* subaperture" parameter (T_XRAS).				*/
	uint32			yRas;				/* SDSU "number of Y super pixels per			*/
										/* subaperture" parameter (T_YRAS).				*/
	uint32			xSubap;				/* SDSU "number of X subapertures per output"	*/
										/* parameter (T_XSUBAP).						*/
	uint32			ySubap;				/* SDSU "number of Y subapertures per output"	*/
										/* parameter (T_YSUBAP).						*/
	uint32			outputs;			/* SDSU "number of outputs " parameter			*/
										/* (T_OUTPUTS).									*/
	uint32			xChip;				/* Maximum X pixels per output (T_XSIZE).		*/
	uint32			yChip;				/* Maximum Y pixels per output (T_YSIZE).		*/
	uint32			packetSize;			/* Packet size in pixels.						*/
	int				nPackets;			/* Number of packets expected per frame.		*/

	int				xSize;				/* Number of X super pixels per output.			*/
	int				ySize;				/* Number of Y super pixels per output.			*/
	int				dspxPixels;			/* Number of X pixels expected by DSP code.		*/
	int				dspyPixels;			/* Number of Y pixels expected by DSP code.		*/
	int				dspxMax;			/* Maximum X pixels expected by DSP code.		*/
	int				dspyMax;			/* Maximum Y pixels expected by DSP code.		*/


	/*
	 * Check there is a valid SDSU context structure.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		return (ERROR);
	}

	/*
	 * In simulation mode no DSP code will have been downloaded, and nothing needs to be checked.
	 */

	if ( !sdsuId->simulate )
	{

		/*
		 * Obtain the xChip, yChip, xRas, yRas, xSubap, ySubap and number of outputs parameters from
		 * the SDSU controller and use these to calculate the default size expected by the DSP code.
		 */

		if ( (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XSIZE", &xChip) == ERROR) ||
		     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YSIZE", &yChip) == ERROR) ||
		     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XRAS", &xRas) == ERROR) ||
		     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YRAS", &yRas) == ERROR) ||
		     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XSUBAP", &xSubap) == ERROR) ||
		     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YSUBAP", &ySubap) == ERROR) ||
		     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_OUTPUTS", &outputs) == ERROR) ||
		     (sdsuParamRead (sdsuId, SDSU_IDENT_VME, "V_PSIZE", &packetSize) == ERROR)
		   )
		{
			ERROR_SET (0,
"Failed to read T_XSIZE, T_YSIZE, T_XRAS, T_YRAS, T_XSUBAP, T_YSUBAP, T_OUTPUTS, V_PSIZE parameters",
				ERROR_LOG_SAVE);
			return (ERROR);
		}

		/*
		 * xSize=(xRas*xSubap) and ySize=(yRas*ySubap) represent the number of pixels per output.
		 * The arrangement depends on the number of outputs. If there are two outputs the
		 * sectors generated from each output are arranged like this
		 *
		 *   +------------+------------+
		 *   |  sector 1  |  sector 2  |
		 *   0----->------+-----<------0
		 *
		 * and if there are four outputs the sectors are arranged like this
		 *
		 *   0----->------+-----<------0
		 *   |  sector 4  |  sector 3  |
		 *   +------------+------------+
		 *   |  sector 1  |  sector 2  |
		 *   0----->------+-----<------0
		 *
		 * "0" shows the origin of each sector and ">" the direction of readout.
		 */

		xSize = (int) (xRas * xSubap);
		ySize = (int) (yRas * ySubap);

		switch (outputs)
		{
			case (2):
				dspxPixels = xSize * 2;
				dspyPixels = ySize;
				dspxMax    = (int) xChip * 2;
				dspyMax    = (int) yChip;
				break;

			case (4):
				dspxPixels = xSize * 2;
				dspyPixels = ySize * 2;
				dspxMax    = (int) xChip * 2;
				dspyMax    = (int) yChip * 2;
				break;

			default:
				ERROR_SET1 (S_detControl_INTERNAL, "Invalid number of SDSU outputs, %d",
				 	ERROR_LOG_SAVE, (int) outputs);
				return (ERROR);
				break;
		}

		/*
		 * Compare the default detector size downloaded in the DSP code with xMax and yMax
		 * and increase if necessary. Replace the current xPixels and yPixels with that found
		 * in the DSP code.
		 */

		MESSAGE_LOG4 (MSG_FULLDEBUG,
			"detControl assumed detector size (%d,%d); DSP code assumed (%d,%d)",
			*pxMax, *pyMax, dspxMax, dspyMax);

		MESSAGE_LOG4 (MSG_FULLDEBUG,
			"detControl assumed readout size (%d,%d); DSP code assumed (%d,%d)",
			*pxPixels, *pyPixels, dspxPixels, dspyPixels);

		if ( dspxMax > *pxMax )
		{
			MESSAGE_LOG2 (MSG_LOG, "Maximum number of X pixels increased from %d to %d\n",
				*pxMax, dspxMax);
			*pxMax = dspxMax;
		}

		if ( dspyMax > *pyMax )
		{
			MESSAGE_LOG2 (MSG_LOG, "Maximum number of Y pixels increased from %d to %d\n",
				*pyMax, dspyMax);
			*pyMax = dspyMax;
		}

		if ( dspxPixels != *pxPixels )
		{
			MESSAGE_LOG2 (MSG_LOG, "Default number of X pixels changed from %d to %d\n",
				*pxPixels, dspxPixels);
			*pxPixels = dspxPixels;
		}

		if ( dspyPixels != *pyPixels )
		{
			MESSAGE_LOG2 (MSG_LOG, "Default number of Y pixels changed from %d to %d\n",
				*pyPixels, dspyPixels);
			*pyPixels = dspyPixels;
		}

		/*
		 * Update the expected number of packets per frame using the number of pixels
		 * read from the controller.
		 */

		if ( packetSize > 0 )
		{
			nPackets = (int) ceil ( (double) (outputs * xSize * ySize) / (double) packetSize );
		}
		else
		{
			nPackets = 1;
		}

		sdsuId->packetsPerFrame = nPackets;
	}
	else
	{
		/* In simulation mode the number of packets per frame needs to be initialised to 1. */

		sdsuId->packetsPerFrame = 1;
	}

	MESSAGE_LOG2 (MSG_FULLDEBUG, "Each frame will consist of %d packets of %lu pixels each",
		nPackets, packetSize);

	return (OK);
}



/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	detCopyGeometry
 *
 *	INVOCATION:
 *	detCopyGeometry (pWfsName, sdsuId, ospGeometry)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pWfsName		(const char *)			Name of wavefront sensor (p1, p2, oi or hr)
 *	(>)	sdsuId			(SDSU_ID)				Current SDSU context structure
 *	(<)	ospGeometry		(struct OSP_GEOMETRY *)	Pointer to OSP geometry structure
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if command successful, ERROR if unsuccessful
 *
 *	PURPOSE:
 *	Copy detector geometry to OSP geometry structure
 *
 *	DESCRIPTION:
 *	This function copies the current detector geometry to the OSP geometry structure.
 *
 *	EXTERNAL VARIABLES:
 *	None. (The function needs to be reentrant)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	detControl.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS detCopyGeometry
	(
	const char *			pWfsName,			/* Name of wavefront sensor.			*/
	SDSU_ID					sdsuId,				/* SDSU context structure.				*/
	struct OSP_GEOMETRY *	ospGeometry			/* Pointer to OSP geometry structure.	*/
	)
{
	uint32			xStart;				/* SDSU parameter (T_XSTART).					*/
	uint32			yStart;				/* SDSU parameter (T_YSTART).					*/
	uint32			xBin;
	uint32			yBin;
	uint32			xRas;				/* SDSU "number of X super pixels per			*/
										/* subaperture" parameter (T_XRAS).				*/
	uint32			yRas;				/* SDSU "number of Y super pixels per			*/
										/* subaperture" parameter (T_YRAS).				*/
	uint32			xSpace;
	uint32			ySpace;
	uint32			xSubap;				/* SDSU "number of X subapertures per output"	*/
										/* parameter (T_XSUBAP).						*/
	uint32			ySubap;				/* SDSU "number of Y subapertures per output"	*/
										/* parameter (T_YSUBAP).						*/
	uint32			outputs;			/* SDSU "number of outputs " parameter			*/
										/* (T_OUTPUTS).									*/
	int				xOutputSize;		/* X size of output in pixels.					*/
	int				yOutputSize;		/* Y size of output in pixels.					*/
	int				xArraySize;			/* Total number of X pixels.					*/
	int				yArraySize;			/* Total number of Y pixels.					*/
	int				fullFrameFlag;		/* Full frame flag.								*/


	/*
	 * Check there is a valid SDSU context structure.
	 */

	if ( sdsuId == NULL )
	{
		ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", ERROR_LOG_NOW);
		return (ERROR);
	}

	/*
	 * In simulation mode the SDSU parameters will not have sensible values when read back.
	 */

	if ( sdsuId->simulate )
	{
		ERROR_SET (S_detControl_INTERNAL,
			"Geometry parameters cannot be read from SDSU controller in simulation mode",
			ERROR_LOG_NOW);
		return (ERROR);	
	}

	/*
	 * Read all the geometry parameters from the SDSU controller.
	 */

	if ( (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XSTART", &xStart) == ERROR) ||
	     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YSTART", &yStart) == ERROR) ||
	     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XBIN", &xBin) == ERROR) ||
	     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YBIN", &yBin) == ERROR) ||
	     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XRAS", &xRas) == ERROR) ||
	     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YRAS", &yRas) == ERROR) ||
	     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XSPACE", &xSpace) == ERROR) ||
	     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YSPACE", &ySpace) == ERROR) ||
	     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XSUBAP", &xSubap) == ERROR) ||
	     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YSUBAP", &ySubap) == ERROR) ||
	     (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_OUTPUTS", &outputs) == ERROR)
	   )
	{
		ERROR_SET (0, "Failed to read SDSU geometry parameters", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/*
	 * xOutputSize and yOutputSize represent the size of each output in pixels,
	 * which can be used to calculate ther full array size).
	 * The arrangement depends on the number of outputs. If there are two outputs the
	 * sectors generated from each output are arranged like this
	 *
	 *   +------------+------------+
	 *   |  sector 1  |  sector 2  |
	 *   0----->------+-----<------0
	 *
	 * and if there are four outputs the sectors are arranged like this
	 *
	 *   0----->------+-----<------0
	 *   |  sector 4  |  sector 3  |
	 *   +------------+------------+
	 *   |  sector 1  |  sector 2  |
	 *   0----->------+-----<------0
	 *
	 * "0" shows the origin of each sector and ">" the direction of readout.
	 */

	xOutputSize = (int) (xStart + (xRas * ((xSubap * xBin) + xSpace)));
	yOutputSize = (int) (yStart + (yRas * ((ySubap * yBin) + ySpace)));

	switch (outputs)
	{
		case (2):
			xArraySize = xOutputSize * 2;
			yArraySize = yOutputSize;
			break;

		case (4):
			xArraySize = xOutputSize * 2;
			yArraySize = yOutputSize * 2;
			break;

		default:
			ERROR_SET1 (S_detControl_INTERNAL, "Invalid number of SDSU outputs, %d",
			 	ERROR_LOG_SAVE, (int) outputs);
			return (ERROR);
			break;
	}

	/*
	 * Determine whether the given parameters will put the detector controller
	 * into full frame mode. This happens when the there is one subaperture per
	 * output and the subapertures fill the detector surface without any gaps.
	 */

	if ( (xSubap == 1) && (ySubap == 1) && (xStart == 0) && (yStart == 0) &&
     (xSpace == 0) && (ySpace == 0)
	   )
	{
		fullFrameFlag = 1;
	}
	else
	{
		fullFrameFlag = 0;
	}

	/*
	 * Update the geometry parameters supplied to the signal processing software.
	 */

	ospGeometry->sectors =			(int) outputs;
	ospGeometry->xstart =			(int) xStart;
	ospGeometry->ystart =			(int) yStart;
	ospGeometry->xbin =				(int) xBin;
	ospGeometry->ybin =				(int) yBin;
	ospGeometry->xraster =			(int) xRas;
	ospGeometry->yraster =			(int) yRas;
	ospGeometry->xspace =			(int) xSpace;
	ospGeometry->yspace =			(int) ySpace;
	ospGeometry->xsubap =			(int) xSubap;
	ospGeometry->ysubap =			(int) ySubap;
	ospGeometry->xarraysize =		(int) xArraySize;
	ospGeometry->yarraysize =		(int) yArraySize;
	ospGeometry->framesizeflag =	fullFrameFlag;

	return (OK);
}

/* ------------------------------------------------------------------------------------------------ */

/* This function is purely an engineering fudge to reset the "observing" flag if it screws up. */

void detPokeObserving
	(
	OBS_ID		obsId,
	BOOL		newValue
	)
{

	obsId->observing = newValue;

	return;
}
