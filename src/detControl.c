static struct {void *v; char *c;} rcsid = {&rcsid,
   "$Id: detControl.c,v 1.15 2001-06-05 02:58:04 cboyer Exp $"};

/*+
 *   MODULE NAME:
 *   detControl
 *
 *   FILENAME:
 *   detControl.c
 *
 *   PURPOSE:
 *   Detector controller application code for HRWFS
 *
 *   DESCRIPTION:
 *   This file contains the detector controller application code for HRWFS.
 *   The code runs in a VxWorks task. 
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   gemTypes.h
 *   wfsLib.h
 *   epToVxLib.h
 *   sdsuLib.h
 *   errorLib.h
 *
 *   AUTHORS:
 *   Nick Dillon
 *   Steven Beard
 *
 *   HISTORY MODIFICATION
 *   01 jun 2001 - cb add overscan region
 *   30 may 2001 - cb fix bug in detGeometry, and in detCheckGeometry, 
 *                 add DET_CONTROL_HRWFS_MAX_XSIZE, DET_CONTROL_HRWFS_MAX_YSIZE
 *   03 apr 2001 - cb add adc0, adc1 sir records
 *                 also modify windowing
 *   19 feb 2001 - cb add detDhsDisplay command and sir record dhsCon
 *   16 feb 2001 - cb add detDhsConnected flag
 *   09 feb 2001 - cb remove error when stop an observation not in progress
 *                 ADC offset now for bin and no bin
 *                 also move all the DATREC_CONTEXT into detControl.h
 *   26 jan 2001 - cb read the detector init file according to the site
 *   15 may 2000 - cb add detDhsReconnect
 *   11 feb 2000 - cb add some SIR records + remove detWriteFits and 
 *                 detFrameReduce
 *   10 feb 2000 - cb add some SIR records 
 *   31 jan 2000 - cb add state SIR record to handle 
 *   21 jan 2000 - cb add observe command
 *                 replace observe command per detObserve
 *                 add setObserve command
 *   13 jan 2000 - cb remove osp stuff from hrwfs
 *   22 nov 1999 - cb change offset for detectors (2400,2398)
 *                 + modify detOffset to have only two offsets
 *   18 nov 1999 - cb add new setDhsInfo command
 *   17 nov 1999 - cb change offset for detectors (2450,2450)
 *   09 nov 1999 - cb TELSCOP and OBSERVAT are now updated from the TCS
 *   08 nov 1999 - cb Fix a bug for WCS when binning or windowing
 *   27 oct 1999 - cb dhs/fits add keywords, fix bug of WCS. 
 *                 Back to detWriteFitsUint16
 *   25 oct 1999 - cb modify detObserveStart and detObserveEnd because observe 
 *                 cad now takes care about dhsOutOptions
 *   23 oct 1999 - cb create detCreateFileName -> combine path and file 
 *                 name and remove .fits at the end.
 *   22 oct 1999 - cb short int, need to bzero=32768, to have dhs working
 *                 used ospWriteUshortImage instead of detWriteFitsUint16
 *   19 oct 1999 - cb modify detFrameSize + detFrameReduce allow to do
 *                 binning and windowing 
 *                 do not use anymore ospGeometry, data are in detControl.h
 *                 detCheckGeometry, detCopyGeometry, detGeometry and detInit
 *                 have been modified
 *   15 oct 1999 - cb modify detCheckGeometry in order it initializes
 *                 xSize, ySize, outputs, uscan and packetSize
 *   14 oct 1999 - cb simplified version for hr only
 *
 *-
 */

/***************************************************************** Includes ***/

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */

#include <pipeDrv.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <semLib.h>
#include <timers.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <selectLib.h>
#include "car.h"
#include <sirRecord.h>

#include "dhs.h"

#include "timeLib.h"
#include "slalib.h"
#include "astLib.h"

#include "fitsio.h" 

#include "gemTypes.h"
#include "timeoutLib.h"
#include "epToVxLib.h"
#include "wfsLib.h"
#include "wfsWcs.h"
#include "errorLib.h"
#include "sdsuLib.h"

#include "detControl.h"
#include "seqControl.h"

/****************************************************************** Defines ***/

/*#define DEBUG*/               /* Define this macro to enable debug messages */

#define DEBUG_DOWNLOAD          /* Define this macro to enable debug messages */
                                /* when downloading DSP code                  */

#define DHS_WAIT_TIMEOUT   3600 /* Timeout waiting for DHS semaphore 60s      */

#define OBS_WAIT_TIMEOUT   1200 /* Timeout waiting for obs sync semaphore 20s */

/******************************************** Macro for checking DHS status ***/

#define CHECK_DHS(dhsErrno) detDhsCheckErrno ((dhsErrno),__LINE__, __FILE__)

/********************************************************* Global variables ***/

char    pDetDhsClientName [EPICS_MAX_BYTES_STRING_ATTRIB + 1] = "NONE";
                                   /* Name of DHS client = Instrument name.   */
                                   /* Assumed the same for all WFSs on CPU.   */

char    pDetDhsHostName [EPICS_MAX_BYTES_STRING_ATTRIB + 1] = "NONE";
                                   /* Name of host running DHS data server.   */
                                   /* Assumed the same for all WFSs.          */

char    pDetDhsServerName [EPICS_MAX_BYTES_STRING_ATTRIB + 1] = "NONE";
                                   /* Name of DHS data server.                */
                                   /* Assumed the same for all WFSs.          */


BOOL    detDhsInitialised = FALSE; /* Flag to determine whether the DHS       */
                                   /* library has been initialised.           */

BOOL    detDhsConnected = NOT_CONNECTED;
                                   /* Flag to determine whether the WFS is    */
                                   /* connected to the DHS.                   */

DHS_CONNECT detDhsConnection = NULL;
                                   /* DHS connection ID for this controller   */

SDSU_ID detSdsuIdHr = NULL;        /* SDSU context structure for HRWFS.       */

OBS_ID  detObsIdHr = NULL;         /* Observation context structure for HRWFS.*/

uint32  detControlStop = 0x0;      /* This bit mask provides a way of aborting*/
                                   /* the detector control task(s) cleanly.   */
                                   /* Each task will keep running until it    */
                                   /* sees its own bit in this mask set.      */

int     readTempReadyFlag=FALSE;   /* Flag used by detHeadTempGet() to check  */
                                   /* if we are ready to read temperature from*/
                                   /* SDSU controller                         */

/* Modif 23 sept to measure time for readout and DHS */

#ifdef DEBUG
int flagFirstTime ;
int flagSecondTime ;

int readTime1 ;
int unscrambleTime1 ;
int dhsTime1 ;

int readTime2 ;
int unscrambleTime2 ;
int dhsTime2 ;
#endif

/***************************************************** External global data ***/

extern int sdsuFrameLost ;

/********************************* Private functions - one for each command ***/

LOCAL uint32   detSetup (const char * pWfsName, const char * pRecordPrefix, 
                         CAD_CMD_CONTEXT cadCmdContext, int commandNumber, 
                         SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32   detExposure (const char * pWfsName, const char * pRecordPrefix, 
                            CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                            SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32   detObstype (const char * pWfsName, const char * pRecordPrefix, 
                           CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                           SDSU_ID sdsuId, OBS_ID obsId); 
LOCAL uint32   setDhsInfo (const char * pWfsName, const char * pRecordPrefix, 
                           CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                           SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32   setObserve (const char * pWfsName, const char * pRecordPrefix, 
                           CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                           SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32   detSetWcs (const char * pWfsName, const char * pRecordPrefix, 
                          CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                          SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32   detObserveStart (const char * pWfsName, 
                                const char * pRecordPrefix, 
                                CAD_CMD_CONTEXT cadCmdContext,
                                int commandNumber, SDSU_ID sdsuId,
                                OBS_ID obsId);
LOCAL uint32   observeStart (const char * pWfsName, 
                             const char * pRecordPrefix, 
                             CAD_CMD_CONTEXT cadCmdContext,
                             int commandNumber, SDSU_ID sdsuId,
                             OBS_ID obsId);
LOCAL uint32   detStop (const char * pWfsName, const char * pRecordPrefix, 
                        CAD_CMD_CONTEXT cadCmdContext, 
                        int commandNumber,
                        SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32   detAbort (const char * pWfsName, const char * pRecordPrefix,
                         CAD_CMD_CONTEXT cadCmdContext, int commandNumber, 
                         SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32   detInit (const char * pWfsName, const char * pRecordPrefix, 
                        CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                        SDSU_ID * pSdsuId, OBS_ID obsId, uint32 *pVmeAddress, 
                        int * pMaxFrames); 
LOCAL uint32   detReset (const char * pWfsName, const char * pRecordPrefix, 
                         CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                         SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32   detTest (const char * pWfsName, const char * pRecordPrefix, 
                        CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                        SDSU_ID sdsuId, OBS_ID obsId); 
LOCAL uint32   detSave (const char * pWfsName, const char * pRecordPrefix, 
                        CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                        SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32   detGeometry (const char * pWfsName, const char * pRecordPrefix, 
                            CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                            SDSU_ID sdsuId, OBS_ID obsId, 
                            long * pOffsetFullVect, long * pOffsetBinVect);
LOCAL uint32   detFrameSize (const char * pWfsName, const char * pRecordPrefix, 
                            CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                            SDSU_ID sdsuId, OBS_ID obsId, 
                            long * pOffsetFullVect, long * pOffsetBinVect);
LOCAL uint32   detPrimitive (const char * pWfsName, const char * pRecordPrefix,
                             CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                             SDSU_ID sdsuId, OBS_ID obsId); 
LOCAL uint32   detDownload (const char * pWfsName, const char * pRecordPrefix,
                            CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                            SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32   detMode (const char * pWfsName, const char * pRecordPrefix, 
                        CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                        SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32   detOffset (const char * pWfsName, const char * pRecordPrefix, 
                          CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                          SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32   detTemp (const char * pWfsName, const char * pRecordPrefix, 
                        CAD_CMD_CONTEXT cadCmdContext, int commandNumber,
                        SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32 detDhsReconnect (CAD_CMD_CONTEXT cadCmdContext,
                              int commandNumber, SDSU_ID sdsuId, OBS_ID obsId);
LOCAL uint32   detDhsDisplay (CAD_CMD_CONTEXT cadCmdContext,
                              int commandNumber, SDSU_ID sdsuId, OBS_ID obsId);


/******************************************* Plus some additional functions ***/

STATUS detDownloadDefault (const char * pWfsName, const char * pRecordPrefix, 
                           SDSU_ID sdsuId);
STATUS detCheckGeometry (const char * pWfsName, SDSU_ID sdsuId, OBS_ID obsId);
STATUS detCopyGeometry (const char * pWfsName, SDSU_ID sdsuId, OBS_ID  obsId);
STATUS detSimulateData (const int xPixels, const int yPixels, const int option,
                        SDSU_FRAME *pFrameBuffer);
STATUS detFrameUnscramble (const int xPixels, const int yPixels, 
                           const int outputs, SDSU_FRAME * inFrame, 
                           float * outBuffer );
STATUS detFrameScramble (const int xPixels, const int yPixels, 
                         const int outputs, float * inBuffer, 
                         uint16 * outBuffer );
STATUS detFrameUnscrambleUint16 (const int xPixels, const int yPixels, 
                                 const int outputs, SDSU_FRAME * inFrame, 
                                 uint16 * outBuffer );
STATUS newDetFrameUnscrambleUint16 (const int xPixels, const int yPixels, 
                                    const int oscanNb, SDSU_FRAME * inFrame, 
                                    uint16 * outBuffer );
STATUS detFrameScrambleUint16 (const int xPixels, const int yPixels, 
                               const int outputs, uint16 * inBuffer, 
                               uint16 * outBuffer );
uint32 detFrameReduceUint16 (OBS_ID obsId) ;
STATUS detWriteFitsUint16 (char * filename, OBS_ID obsId, int xPixels, 
                           int yPixels, uint16 * pFrameBuffer);
OBS_ID detObsContextCreate(void);
void   detPacketCallback (SDSU_ID sdsuId, void * obsIdIn, SDSU_FRAME * pFrame );
void   detFrameCallback (SDSU_ID sdsuId, void * obsIdIn, SDSU_FRAME * pFrame );
void   detObserveEnd (SDSU_ID sdsuId, void * obsIdIn, SDSU_FRAME * pFrame );
void   detObserveTimeout (timer_t timeId, int obsIdInt);
STATUS detDhsConnect ();
void   detDhsCheckErrno (const DHS_STATUS dhsErrno, const int line,
                         const char * filename);
STATUS detDhsCheckCmdStatus (const DHS_TAG dhsTag);
void   detPokeObserving (OBS_ID obsId, BOOL newValue);
STATUS detCreateFileName ( char * pFilePath, char * pOutFileName, 
                           char * pFullOutFileName);
STATUS detReadFitsHeaderInt ( char * fileName, int nKey, char ** keyName,
                              int * keyVal);
STATUS detReadFitsImageUint16 ( uint16 * pImageBuffer, char * fileName,
                                int buffSize);
uint32 detContInit (char * pInitFileName, uint32 * pTempCode,
                    uint32 * pTempCoeff, long *pOffsetFullVect, 
                    long * pOffsetBinVect, char * pCcdSn);
uint32 detGetSirContext (const char * pRecordPrefix, OBS_ID obsId);
uint32 detWriteDefSirContext (OBS_ID obsId);

/* -------------------------------------------------------------------------- */

STATUS   detControl
   (
   const char *   pWfsName,        /* Name of wavefront sensor "hrwfs"        */
   const char *   pRecordPrefix    /* Record name prefix                      */
   )
{
   /* Variables associated with VxWorks environment. */

   int    taskOptions;             /* VxWorks task options.                   */
   STATUS (* pipeCreate) ();       /* Pointer to appropriate pipeCreate func. */

   /* Variables associated with CAD/CAR/genSub command protocol. */

   CAD_CMD_CONTEXT   cadCmdContext;    /* CAD command context structure.      */
   int               commandNumber;    /* Command number.                     */
   int               updateNumber;     /* Data update ID number.              */
   uint32            errorNumber;      /* Error number reported by task.      */

   /* Variables associated with the use of the select() facility. */

   struct fd_set     updateFds;        /* File descr. structure for select(). */
   int               widthSelect;      /* Number of file descrs. to monitor.  */

   /* Variables associated with genSub records. */

   GSUB_DATA_CONTEXT dataUpdateContext; /* Data update context structure.     */

   /* Variables associated with the SDSU controller. */

   uint32       vmeAddress = 0;     /* VME address of SDSU controller. (Set to*/
                                    /* 0 if the controller is not installed   */
                                    /* and is to be simulated).               */
   BOOL         simulate;           /* TRUE if controller is to be simulated. */
   BOOL         initFailed = FALSE; /* Set TRUE if a significant but non fatal*/
                                    /* error occurs during initialisation.    */
                                    /* (Fatal errors will cause the task to   */
                                    /* abort completely).                     */
   BOOL         initWarning = FALSE;/* Set TRUE if a warning occurs during    */
                                    /* initialisation.                        */
   long         initState;          /* Initialisation state.                  */

   SDSU_ID      sdsuId = NULL;      /* SDSU context structure.                */

   OBS_ID       obsId = NULL;       /* Observation context structure.         */

   uint32       detControlStopMask; /* Mask for detecting which detControlStop*/
                                    /* bit refers to this detector controller */

   long         i;                  /* index                                  */
   long         offsetFullVect[2];  /* ADC offset vector - no binning.        */
   long         offsetBinVect[2];   /* ADC offset vector - binning.           */
   long         offsetVect[2];      /* ADC offset vector                      */
   uint32       tempCode;           /* Target temperature code                */
   uint32       tempCoeff;          /* Coefficient for temperature control    */

   char         detContInitFileName [ STRING_SIZE ] ;
                                    /* Full Name of the detector controller   */
                                    /* init file                              */

   char         defFileName [ STRING_SIZE ] ;
                                    /* Default file name according to the site*/

   uint32       tryDownload ;       /* Counter to stop attempt for downloading*/

   /* Variables used to define the buffer to be used for storing data.   */

   int          maxFrames;          /* Maximum number of frames in data buffer*/

   /* Timer variables. */

   timer_t      timeId;             /* Alarm timer ID.                        */

   /* Other general variables. */

   char         pStatusString [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                    /* Status string.                         */

   long         simMode;            /* Code for simulation mode.              */
   char         pSimMode [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                    /* Simulation mode string.                */

   long         debugMode;          /* Code for debug mode.                   */
   char         pDebugMode [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                    /* Debug mode string.                     */

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

   pipeCreate = pipeDevCreate;

   /*
    * Initialise the alarm timer.
    */

   if ( timeoutAlarmInit( &timeId ) == ERROR )
   {
      ERROR_SET (0, "Failed to initialise alarm timer", ERROR_LOG_NOW);
      return (ERROR);
   }

#ifdef DEBUG
   printf ("detControl:%s: Alarm timer initialised. Timer ID = %d\n", 
           pWfsName, (int) timeId);
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
    * Get the genSub data update context structure (using the appropriate pipe 
    * driver), which is used subsequently as a handle for the genSub data 
    * update routines. Each time a new largest file descriptor is found, update 
    * the "widthSelect" variable to be used by "select()" later. These genSub 
    * records are not used by the HRWFS.
    */

   dataUpdateContext = NULL;

   /*
    * Set the default simulation mode and debug mode.
    */

   epToVxSetCadSimMode (EPTOVX_SIM_MODE_NONE);
   if (epToVxPipeWrite ("dc:simMode", "NONE", NULL) == ERROR)
   {
      ERROR_LOG ( "Failed to write default simulation mode to SIR record") ;
      return (ERROR);
   }

   errorMessageFilterSet( EPTOVX_DEBUG_MODE_NONE+1 );
   if (epToVxPipeWrite ("dc:debugMode", "NONE", NULL) == ERROR)
   {
      ERROR_LOG ( "Failed to write default debug mode to SIR record");
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

   /* Initialise the type and SN of the CCD */

   strcpy ( obsId->detType , DET_TYPE ) ;

   /* Initialise the "observing" flag and number of frames. */

   obsId->observing = FALSE;
   obsId->totalFrames = 1;
   obsId->dhsQlRate = 1;

   /*
    * Get the context structures for the SIR records.
    */

   if ( detGetSirContext (pRecordPrefix , obsId) == ERROR )
   {
      ERROR_LOG ("Error getting sir records context structures");
      return (ERROR);
   }

   /* Initialise the detector controller state to "INITIALIZING". */

   if (epToVxPipeWrite( NULL, "INITIALIZING", obsId->pStateContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set INITIALIZING state");
      return (ERROR);
   }

   /* As soon as we have the SIR record context, set the "initialising" flag. */

   initState = CAR_BUSY;
   if (epToVxPipeWrite (NULL, (char *) &initState, obsId->pDetInitContext) 
       == ERROR)
   {
      ERROR_LOG ("Failed to set initialisation state to BUSY");
   }

   /*
    * Init the SIR records with default values.
    */

   if ( detWriteDefSirContext (obsId) == ERROR )
   {
      ERROR_LOG ("Error initializing the sir records");
   }

   /*
    * Use the wavefront sensor name provided as a function argument to
    * obtain the VME address of the corresponding SDSU controller, reporting
    * an error if the wavefront sensor name is not "hrwfs".
    *
    * Also initialise the default data frame size for the appropriate wavefront
    * sensor.
    */

   if (strcmp (pWfsName, "hr") == 0)
   {
      vmeAddress = DET_CONTROL_HRWFS_SDSU_ADRS_VME;
      detControlStopMask = DET_CONTROL_HRWFS_MASK;
      obsId->xMax = DET_CONTROL_HRWFS_MAX_XSIZE;
      obsId->yMax = DET_CONTROL_HRWFS_MAX_YSIZE;
      obsId->xPixels = DET_CONTROL_HRWFS_XSIZE;
      obsId->yPixels = DET_CONTROL_HRWFS_YSIZE;
      maxFrames = DET_CONTROL_HRWFS_MAX_FRAMES;
   }
   else
   {
      ERROR_SET1 (S_detControl_BAD_WFS_NAME, "Unrecognised WFS name, %s, given",
                  ERROR_LOG_NOW, pWfsName);
      return (ERROR);
   }

   /*
    * Initialize sdsuLib using the VME address obtained above (which involves 
    * establishing communications with the SDSU hardware), remembering to call 
    * sdsuReset() immediately after sdsuContextCreate() to ensure a 
    * "Set Reply Address" command is issued.
    *
    * If the VME address is zero this indicates the SDSU controller for this 
    * WFS is not installed and should be simulated.
    *
    * A message describing the status of this initialisation is written to the
    * DET_CONTROL_INIT_STATUS_SIR_NAME record.
    *
    * Note: The task does not abort if the SDSU context structure could not 
    * be created because another attempt can be made by issuing the detInit 
    * command.
    */

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
       * and also write a message to the DET_CONTROL_INIT_STATUS_SIR_NAME 
       * record.
       */

      ERROR_LOG ("Failed to initialise SDSU controller");
      initFailed = TRUE;

      /*
       * Note. When epToVxPipeWrite has a NULL record name argument, as it 
       * does below, the record name is extracted from the 
       * "pDetInitStatusContext" structure.
       */

      if (epToVxPipeWrite (NULL, "WARNING: SDSU Not Initialised", 
          obsId->pDetInitStatusContext) == ERROR)
      {
         ERROR_LOG (
         "Also failed to write warning message to SDSU status pipe.");
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
         sprintf (pStatusString, "SDSU Initialised OK: ID = 0x%-8x", 
                  (int) sdsuId);
      }

      MESSAGE_LOG2 (MSG_LOG, "%s: %s", pWfsName, pStatusString);

      if (epToVxPipeWrite (NULL, pStatusString, obsId->pDetInitStatusContext) 
          == ERROR)
      {
         ERROR_LOG (
         "Failed to write initialisation message to SDSU status pipe.");
      }

      /*
       * Update the global variables used to remember the SDSU and observing 
       * contexts, as an aid to engineering.
       */

      sdsuId->fastCamera = FALSE;
      detSdsuIdHr = sdsuId;
      detObsIdHr = obsId;
   }

   /*
    * Download the default OMF code to the SDSU controller automatically on 
    * startup. The health is set to WARNING if this fails
    */

#ifdef DEBUG_DOWNLOAD
   sdsuPrintRepBuf (sdsuId) ;
#endif

   tryDownload = 0 ;
   while ( (detDownloadDefault (pWfsName, pRecordPrefix, sdsuId) == ERROR) 
           && ( tryDownload < 10 ) )
   {
         /* RESET REP BUFFER, VME and CONTROLLER */
#ifdef DEBUG_DOWNLOAD
         sdsuPrintRepBuf (sdsuId) ;
#endif
         if ( sdsu_initRepBuf (sdsuId) == ERROR )
            ERROR_LOG ("Failed to reset to zero the reply buffer ");
#ifdef DEBUG_DOWNLOAD
         sdsuPrintRepBuf (sdsuId) ;
#endif
         if ( sdsuReset (sdsuId, SDSU_RESET_VME | SDSU_RESET_CONTROLLER) 
              == ERROR )
            ERROR_LOG ("Failed to reset SDSU interface and controller");
#ifdef DEBUG_DOWNLOAD
         sdsuPrintRepBuf (sdsuId) ;
#endif

         tryDownload ++ ;
   }

   if ( tryDownload == 10 )
   {
      ERROR_LOG ("Failed to download default DSP code on startup");
      initFailed = TRUE;
   }

   /*
    * Compare the default detector geometry contained in the DSP code with
    * the values written to xPixels and yPixels from the DET_CONTROL
    * software constants. (The DSP code describes what the detector
    * controller is capable of and the DET_CONTROL constants are used to
    * define the default parameter limits of CAD commands). After calling
    * detCheckGeometry, obsId->xMax and obsId->yMax should contain the maximum 
    * possible data array size, allowing a data buffer of a suitable size to be
    * allocated.
    *
    */

   if (detCheckGeometry (pWfsName, sdsuId, obsId ) == ERROR)
   {
      ERROR_LOG ("Error while checking default detector geometry on startup");
      initFailed = TRUE;
   }

   /*
    * Create data buffer to hold several frames of data, using the xPixels 
    * and yPixels determined above.
    */

   if (sdsuBufferCreate (sdsuId, (obsId->xMax * obsId->yMax), maxFrames) 
       == ERROR)
   {
      ERROR_LOG ("Failed to create data buffer on startup");
      initFailed = TRUE;
   }

   /*
    * Initialise the readout process with our frame callback.
    * There is no packet callback in this version of the code.
    * INTERRUPTS ENABLED. SWITCH TO SIMPLE VERSION. 23 SEPT 99
    */

   if (sdsuSimpleReadoutOpen (sdsuId, NULL, detObserveEnd, 1, TRUE) == ERROR)
   {
      ERROR_LOG ("Failed to start readout task on startup");
      initFailed = TRUE;
   }

   /*
    * Read the default settings from the detector controller init file
    */

#if (MK)
   strcpy ( defFileName, DET_CONTROL_HRWFS_MK_INIT_FILE);
#else
   strcpy ( defFileName, DET_CONTROL_HRWFS_CP_INIT_FILE);
#endif

   printf ( "defFileName =%s\n", defFileName);

   if ( strcmp (defFileName, "NONE") != 0 )
   {
      strcpy ( detContInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
      strcat ( detContInitFileName , "/" ) ;
      strcat ( detContInitFileName , defFileName ) ;

      if ( detContInit ( detContInitFileName, &tempCode, &tempCoeff,
                         offsetFullVect, offsetBinVect, obsId->detId) == ERROR )
      {
         MESSAGE_LOG ( MSG_LOG,
           "Failed to init detector controller default settings from file");

         /* Set the temperature to -20.0C anyway and ADC offsets to 2560
            which is default value */

         tempCode = (uint32)1282 ;
         tempCoeff = (uint32)128 ;
         for ( i = 0 ; i < obsId->outputsNb ; i ++ )
             offsetVect[i] = 2560;
         strcpy ( obsId->detId , DET_CCD_SN ) ;
      }
      else
      {
         if ( obsId->binningFlag == FALSE )
         {
            for ( i = 0 ; i < obsId->outputsNb ; i ++ )
                offsetVect[i] = offsetFullVect[i];
         }
         else
         {
            for ( i = 0 ; i < obsId->outputsNb ; i ++ )
                offsetVect[i] = offsetBinVect[i];
         }
      }
   }
   else
   {
      /* Set the temperature to -20.0C anyway and ADC offsets to 2560
         which is default value */

      tempCode = (uint32)1282 ;
      tempCoeff = (uint32)128 ;
      for ( i = 0 ; i < obsId->outputsNb ; i ++ )
          offsetVect[i] = 2560;
      strcpy ( obsId->detId , DET_CCD_SN ) ;
   }

   /*
    * Write the CCD serial number to the corresponding SIR record
    */

   if (epToVxPipeWrite( NULL, obsId->detId, obsId->pDetIdContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set default detector type");
      return (ERROR);
   }

   /* 
    * Set the default temperature for the HRWFS 
    */

   if ( sdsuId == NULL )
   {
      ERROR_LOG ("Failed to set CCD default temperature");
      initFailed = TRUE;
   }
   else
   {
      MESSAGE_LOG2 (MSG_LOG, 
                    "Defining temperature control parameters: %#lx %#lx",
                    tempCode, tempCoeff);

      if ( (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_CCDT_TGT", tempCode )
            == ERROR) ||
           (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_TCF", (uint32)tempCoeff )
            == ERROR) )
      {
         ERROR_LOG ("Error setting temperasture control parameters");
         initFailed = TRUE;
      }
      readTempReadyFlag = TRUE ;
   }

   /* 
    * Set the default offsets for the HRWFS CCD sectors
    */

   if ( sdsuId == NULL )
   {
      ERROR_LOG ("Failed to set CCD default offset");
      initFailed = TRUE;
   }
   else
   {
      MESSAGE_LOG2 (MSG_LOG, "Defining new ADC offset levels: %#lx %#lx",
                    offsetVect[0], offsetVect[1]);

      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS0",
                         (uint32) offsetVect[0] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 0 parameter");
         initFailed = TRUE;
      }

      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS1",
                         (uint32) offsetVect[1] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 1 parameter");
         initFailed = TRUE;
      }
      if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
      {
         ERROR_LOG (
         "Failed to activate TIMING DSP parameters with LDP command");
         initFailed = TRUE;
      }
   }

   /*
    * Update the adc sir records
    */

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[0]) ,
                        obsId->pAdc0Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc0 sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[1]) ,
                        obsId->pAdc1Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc1 sad record");
      return (ERROR);
   }

   /*
    * Complete initialisation of the CCD geometry informations 
    */

   if ( !sdsuId->simulate )
   {
      if (detCopyGeometry (pWfsName, sdsuId, obsId) == ERROR)
      {
         ERROR_LOG ("Error while copying default detector geometry on startup");
         initFailed = TRUE;
      }

      strcpy (obsId->pWfsName, "HRWFS");
   }
   else
   {
      /* In simulation mode use default values for the parameters. */

            obsId->outputsNb =   2;
            obsId->xStart =      0;
            obsId->yStart =      0;
            obsId->xBin =        1;
            obsId->yBin =        1;
            obsId->xRaster =     (obsId->xPixels) / 2;
            obsId->yRaster =     (obsId->yPixels);
            obsId->xSpace =      0;
            obsId->ySpace =      0;
            obsId->xSubapNb =    1;
            obsId->ySubapNb =    1;
            obsId->fullImageFlag = TRUE;
            obsId->binningFlag =   FALSE;
            obsId->windowingFlag = FALSE;
            obsId->x1 = 1;
            obsId->x2 = obsId->xPixels;
            obsId->y1 = 1;
            obsId->y2 = obsId->yPixels;

            strcpy (obsId->pWfsName, "HRWFS");
   }

   /*
    * Init the sad records containing the detector geometry
    */

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->outputsNb) , 
                        obsId->pOutputsContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init outputs sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xSize) , 
                        obsId->pDetXsizeContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init x size sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->ySize) , 
                        obsId->pDetYsizeContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ysize sad record");
      return (ERROR);
   }
   
   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xStart) , 
                        obsId->pXstartContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xstart sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->yStart) , 
                        obsId->pYstartContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ystart sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xSubapNb) , 
                        obsId->pXsubapContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xsubap sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->ySubapNb) , 
                        obsId->pYsubapContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init Ysubap sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xRaster) , 
                        obsId->pXrasterContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xraster sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->yRaster) , 
                        obsId->pYrasterContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init yraster sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xSpace) , 
                        obsId->pXspaceContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xspace sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->ySpace) , 
                        obsId->pYspaceContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init yspace sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xBin) , 
                        obsId->pXbinContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xbin sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->yBin) , 
                        obsId->pYbinContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ybin sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->oscanNb) , 
                        obsId->pOscanContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init oscan sad record");
      return (ERROR);
   }

   /*
    * If the DHS has initialised successfully, attempt to connect to it.
    */

   if (detDhsInitialised)
   {
      if ( detDhsConnect () == ERROR )
      {
         ERROR_LOG ("Failed to connect to DHS");
         initWarning = TRUE;
      }      

      if ( detDhsConnected == CONNECTED )
      {
         if (epToVxPipeWrite (NULL, "CONNECTED", obsId->pDhsConContext)
             == ERROR)
         {
            ERROR_LOG (
            "Failed to initialise DET_CONTROL_DHSCON_SIR_NAME record");
            errorNumber = ERROR;
         }
      };

      if ( detDhsConnected == NOT_CONNECTED )
      {
         if (epToVxPipeWrite (NULL, " NOT CONNECTED", obsId->pDhsConContext)
             == ERROR)
         {
            ERROR_LOG (
            "Failed to initialise DET_CONTROL_DHSCON_SIR_NAME record");
            errorNumber = ERROR;
         }
      };
   }
   else
   {
      MESSAGE_LOG (MSG_WARNING, "WARNING: DHS not initialised");
      detDhsConnected = NOT_INIT;
      if (epToVxPipeWrite (NULL, "NOT INIT", obsId->pDhsConContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to initialise DET_CONTROL_DHSCON_SIR_NAME record");
         errorNumber = ERROR;
      }
   }

   /*
    * Initialise the health of this detector control task to "GOOD" if 
    * successful or "BAD" if a significant problem occurred during the 
    * initialisation.
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
         ERROR_LOG (
         "Failed to initialise detector controller health to WARNING");
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
   if (epToVxPipeWrite (NULL, (char *) &initState, obsId->pDetInitContext) 
       == ERROR)
   {
      ERROR_LOG ("Failed to set initialisation state to IDLE");
   }

   /* And set up the detector controller state to "RUNNING". */

   if (epToVxPipeWrite( NULL, "RUNNING", obsId->pStateContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set RUNNING state");
      return (ERROR);
   }

   /*
    * The task has been successfully initialised, so it can now go into a loop
    * waiting for commands. The detector controller task can be terminated by
    * setting the "detControlStop" variable from the console. The task will
    * stop when it discovers its bit set. All the tasks can be stopped at
    * once by setting "detControlStop" to 0xf.
    */

   MESSAGE_LOG (MSG_MINDEBUG, "Entering loop waiting for commands...") ;

   while ( (detControlStop & detControlStopMask) == 0 )
   {

      /*
       * Zero all the bits in the file descriptor read structure and then
       * set each bit corresponding to the file descriptors of the data
       * update pipes of all the wavefront sensors being monitored by this
       * task. Also set the bit corresponding to the pipe used to receive CAD 
       * commands.
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
         ERROR_SET (0, "File descriptor selection function, select(), failed", 
                    ERROR_LOG_NOW );
         return (ERROR);
      }

      /* Check whether an input has come from the pipe communicating CAD
       * commands.
       */

      if (FD_ISSET (cadCmdContext->cadPipeFd, & updateFds))
      {

         /*
          * Initialise the error number and then read the command number from
          * the pipe communicating CAD commands. The epToVxCmdRead() call will
          * block until a command becomes available. The detControl task is
          * aborted if it fails to read a command.
          */

         errorNumber = 0;
         if ((commandNumber = epToVxCmdRead (cadCmdContext)) < 0)
         {
            ERROR_LOG ("Error reading CAD command - detControl task aborted");
            epToVxSetHealth( pRecordPrefix, "BAD" );
            return (ERROR);
         }

         /* Log a message each time a command is received. */

         MESSAGE_LOG1 (MSG_FULLDEBUG, "CAD command %d received.", 
                       commandNumber);

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

            MESSAGE_LOG1 (MSG_LOG, 
               "Command %d received in simulation mode... no action taken",
               commandNumber);
         }

         else if (commandNumber == DET_CONTROL_CMD_SETUP)
         {
            /* Setup SDSU controller params. */

            errorNumber = 
            detSetup (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                      sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_EXPOSURE)
         {

            /* Define exposure parameters. */

            errorNumber = 
            detExposure (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                         sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_OBSTYPE)
         {

            /* Define observation type. */

            errorNumber = 
            detObstype(pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                       sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_DHSINFO)
         {

            /* Define DHS info for an observation. */

            errorNumber = 
            setDhsInfo(pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                       sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SETOBSERVE)
         {

            /* Define parameters for an observation */

            errorNumber = 
            setObserve(pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                       sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SETWCS)
         {

            /* Define WCS parameters. */

            errorNumber = 
            detSetWcs(pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                      sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_OBSERVE)
         {

            /*
             * Make observation. Before starting the observation, load up 
             * the observation ID structure.
             */

            obsId->sdsuId = sdsuId;
            obsId->timeId = timeId;

            errorNumber = 
            observeStart (pWfsName, pRecordPrefix, cadCmdContext, 
                          commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_DETOBSERVE)
         {

            /*
             * Make observation. Before starting the observation, load up 
             * the observation ID structure.
             */

            obsId->sdsuId = sdsuId;
            obsId->timeId = timeId;

            errorNumber = 
            detObserveStart (pWfsName, pRecordPrefix, cadCmdContext, 
                             commandNumber, sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_PAUSE)
         {

            /* Pause observation (not supported by SDSU controller). */

            ERROR_SET (S_detControl_BAD_COMMAND, 
                       "Pause observation command not supported",
                       ERROR_LOG_NOW);
            errorNumber = S_detControl_BAD_COMMAND;
         }

         else if (commandNumber == DET_CONTROL_CMD_CONTINUE)
         {

            /* Continue observation (not supported by SDSU controller). */

            ERROR_SET (S_detControl_BAD_COMMAND, 
                       "Continue observation command not supported",
                       ERROR_LOG_NOW);
            errorNumber = S_detControl_BAD_COMMAND;
         }

         else if (commandNumber == DET_CONTROL_CMD_STOP)
         {

            /* Stop observation and keep the data. */

            errorNumber = 
            detStop (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                     sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_ABORT)
         {

            /* Abort observation and throw away the data. */

            errorNumber = 
            detAbort (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                      sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_INITIALISE)
         {

            /* Initialise SDSU context and redownload DSP code. */

            errorNumber = 
            detInit (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                     &sdsuId, obsId, &vmeAddress, &maxFrames); 
         }

         else if (commandNumber == DET_CONTROL_CMD_RESET)
         {

            /* Reset SDSU controller and redownload DSP code. */

            errorNumber = 
            detReset (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                      sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_TEST)
         {

            /* Test SDSU controller. */

            errorNumber = 
            detTest (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                     sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SAVE)
         {

            /* Save SDSU control parameters. */

            errorNumber = 
            detSave (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                     sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_GEOMETRY)
         {

            /* Set readout geometry. */

            errorNumber = 
            detGeometry (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                         sdsuId, obsId, offsetFullVect, offsetBinVect); 
         }

         else if (commandNumber == DET_CONTROL_CMD_FRAME_SIZE)
         {

            /* Set frame size. */

            errorNumber = 
            detFrameSize (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                          sdsuId, obsId, offsetFullVect, offsetBinVect); 
         }

         else if (commandNumber == DET_CONTROL_CMD_PRIMITIVE)
         {

            /* Execute SDSU primitive command. */

            errorNumber = 
            detPrimitive (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                          sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_MODE)
         {

            /* Set SDSU readout mode. */

            errorNumber = 
            detMode (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                     sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_OFFSET)
         {

            /* Set SDSU ADC offsets. */

            errorNumber = 
            detOffset (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                       sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_TEMP)
         {

            /* Define SDSU temperature control parameters. */

            errorNumber = 
            detTemp (pWfsName, pRecordPrefix, cadCmdContext, commandNumber,
                     sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_DHS_RECONNECT)
         {
            /* Set dhs connection. */

            errorNumber = detDhsReconnect (cadCmdContext, commandNumber,
                                           sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_DHS_DISPLAY)
         {
            /* Set dhs display parameters. */

            errorNumber = detDhsDisplay (cadCmdContext, commandNumber,
                                         sdsuId, obsId);
         }

         else if (commandNumber == DET_CONTROL_CMD_SIMULATE)
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

            if (epToVxPipeWrite ("dc:simMode", pSimMode, NULL) == ERROR)
            {
               ERROR_LOG ("Failed to write simulation mode to SIR record");
               errorNumber = (uint32) errnoGet();
            }
         }

         else if (commandNumber == DET_CONTROL_CMD_DEBUG)
         {
            /* Debug command received.
             * Set the debugging mode.
             */

            EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, 
                                   (char *) & debugMode);
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
                  strncpy (pDebugMode, "INVALID", 
                           EPICS_MAX_BYTES_STRING_ATTRIB);
            }

            MESSAGE_LOG1 (MSG_LOG, "Debug mode set to %s", pDebugMode);

            if (epToVxPipeWrite ("dc:debugMode", pDebugMode, NULL) == ERROR)
            {
               ERROR_LOG ("Failed to write debug mode to SIR record" ) ;
               errorNumber = (uint32) errnoGet();
            }
         }

         else
         {
            ERROR_SET1 (S_detControl_BAD_COMMAND, 
                        "Command %d not currently implemented",
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
          * Read the message from the pipe and check it has been read 
          * successfully.
          */

         if ((updateNumber = epToVxUpdateRead (dataUpdateContext)) < 0)
         {
            ERROR_LOG ("Error reading genSub update");
         }

         /* Log a message each time a data update is received. */

         MESSAGE_LOG3 (MSG_FULLDEBUG, 
            "%s - data update %d received. Packet=%#x",
            pWfsName, updateNumber, (int) dataUpdateContext->pUpdatePacket);
      }
   }

   /*
    * If the task is stopped, free the resources allocated to it.
    */

   if (sdsuId != NULL) sdsuContextDelete (sdsuId);

   MESSAGE_LOG1 (MSG_WARNING, 
                 "Detector Control task for WFS %s stopped.", pWfsName);
   epToVxSetHealth( pRecordPrefix, "BAD" );

   epToVxCmdFree (cadCmdContext);
   errorFlush();
   errorFree();

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSetup
 *
 *   INVOCATION:
 *   detSetup (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *             sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSetup command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU controller by downloading a 
 *   set of parameters from a file.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSetup
   (
   const char *      pWfsName,      /* Name of wavefront sensor.              */
   const char *      pRecordPrefix, /* Record name prefix.                    */
   CAD_CMD_CONTEXT   cadCmdContext, /* CAD command context structure.         */
   int               commandNumber, /* Command number.                        */
   SDSU_ID           sdsuId,        /* SDSU context structure.                */
   OBS_ID            obsId          /* Observation context structure.         */
   )
{
   uint32    errorNumber;           /* Error number reported by task.         */

   long      destId;                /* Destination DSP ID.                    */

   char      pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                    /* Path name for file.                    */
   char      pParamFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                    /* Name of file of SDSU parameter values. */
   char      pFullParamFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                                    /* Combined path name and file name.      */

   /*
    * Initialise the error number and obtain the attributes provided with the 
    * command.
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
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
                 "Observation in progress - abort observation and try again", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /*
    * Combine the path and file names together, and append the string ".par"
    * to the file name
    * if it is not already present. Ignore the file path if not provided.
    */

   if ( strcmp (pFilePath, "") == 0 )
   {
      strncpy (pFullParamFileName, pParamFileName, 
               EPICS_MAX_BYTES_STRING_ATTRIB);
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

   MESSAGE_LOG1 (MSG_LOG, "Downloading SDSU parameters from %s", 
                 pFullParamFileName);

   if ( sdsuParamDnload( sdsuId, pFullParamFileName, (uint32) destId) == ERROR )
   {
      ERROR_LOG ("Failed to download SDSU parameter file");
      errorNumber = S_detControl_SDSU_ERROR;
      return (errorNumber);
   }

   return (errorNumber);
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detExposure
 *
 *   INVOCATION:
 *   detExposure (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *                sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName        (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix   (const char *)    Record name prefix
 *   (>) cadCmdContext   (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber   (int)             Command number
 *   (>) sdsuId          (SDSU_ID)         Current SDSU context structure
 *   (>) obsId           (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detExposure command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU exposure parameters.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detExposure
   (
   const char *    pWfsName,       /* Name of wavefront sensor.               */
   const char *    pRecordPrefix,  /* Record name prefix.                     */
   CAD_CMD_CONTEXT cadCmdContext,  /* CAD command context structure.          */
   int             commandNumber,  /* Command number.                         */
   SDSU_ID         sdsuId,         /* SDSU context structure.                 */
   OBS_ID          obsId           /* Observation context structure.          */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */

   long            nframe;        /* Number of frames.                        */
   long            nframePerDataset; /* Number of frames per dataset          */
   double          exposure;      /* Exposure time in seconds.                */

   uint32          sdsuNframe;    /* Value for SDSU parameter NFRAME.         */
   uint32          sdsuTexp;      /* Value for SDSU parameter T_EXP.          */

   int             nexp;          /* Number of exposure/dataset               */

   /*
    * Initialise the error number and obtain the attributes provided 
    * with the command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & nframe);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & exposure);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again", 
                 ERROR_LOG_NOW);
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
    * Check the exposure time is sensible. The SDSU controller measures
    * exposures in units of 81.92 microseconds and stores the exposure in a
    * 32 bit integer, so the upper limit in seconds is 2**32 * 0.000008192 =
    * 351,843 seconds
    */

   if ( (exposure < 0.0) || (exposure > 351843.0) )
   {
      ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, 
                  "Invalid exposure time, %f seconds.",
                  ERROR_LOG_NOW, exposure);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
   }

   obsId->expTime = exposure;

   if (epToVxPipeWrite( NULL, (char *)(int)&exposure, obsId->pIntTimeContext ) 
       == ERROR)
   {
      ERROR_LOG ("Failed to set integration time SIR record");
   }

   /*
    * Determine how to read the CCD when in continuous mode:
    * if exposure < readoutTime -> serie of 1 frame
    * if exposure > readoutTime -> CCD in continuous mode
    */

   sdsuId->readoutTime = (SDSU_FULL_READOUT * obsId->pixelsNb) /
                         (DET_CONTROL_HRWFS_XSIZE * DET_CONTROL_HRWFS_YSIZE);

   /*if ( exposure < sdsuId->readoutTime ) 
   {
      sdsuId->readMethod = 0;
      printf ( "exp < readoutTime (%f<%f) - pixels Nb = %d\n" , 
               exposure, sdsuId->readoutTime, obsId->pixelsNb);
   }
   else
   {
      sdsuId->readMethod = 0;
      printf ( "exp > readoutTime (%f>%f) - pixels Nb = %d\n" , 
               exposure, sdsuId->readoutTime, obsId->pixelsNb);
   }*/

   sdsuId->readMethod = 0;

   if ( nframe == -1 )
   {
      MESSAGE_LOG1 (MSG_LOG, 
         "Setting up for an infinite series of exposures of %f seconds each",
         exposure);

      nframePerDataset = nframe ;
      
      /* BUG WORK AROUND: THE SDSU CONTROLLER RETURNS FRAME COUNT=1 WHEN ASKED
       * FOR AN INFINITE
       * NUMBER OF FRAMES, WHICH DETCONTROL THEN ASSUMES MEANS THE LAST FRAME
       * HAS BEEN RECEIVED.
       * UNTIL THE SDSU CODE IS FIXED, SET A FLAG TO INDICATE THE FRAME COUNT
       * IS INFINITE.
       */

      if ( sdsuId->readMethod == 1 )
      {
         nframe = 0;   /* DSP code assumes 0 means infinite number of frames. */
         obsId->continuous = TRUE;
         obsId->totalFrames = nframe;
         sdsuNframe = (uint32) nframe;  /* Modif 23 sept 1999 - cb */
      }
      else
      {
         nframe = 1;      
         obsId->continuous = TRUE;
         obsId->totalFrames = 0;
         sdsuNframe = (uint32) nframe;  
      }
   }
   else if ( nframe == 1 )
   {
      MESSAGE_LOG1 (MSG_LOG, 
                    "Setting up for one exposure of %f seconds", exposure);

      nframePerDataset = nframe ;

      /* BUG WORK AROUND */
      obsId->continuous = FALSE;
      obsId->totalFrames = nframe;
      sdsuNframe = (uint32) nframe;  /* Modif 23 sept 1999 - cb */
   }
   else
   {
      MESSAGE_LOG2 (MSG_LOG, 
      "Setting up for %ld exposures of %f seconds each", nframe, exposure);

      nframePerDataset = 1 ;
      
      /* BUG WORK AROUND */

      if ( sdsuId->readMethod == 1)
      {
         obsId->continuous = FALSE;
         obsId->totalFrames = nframe;
         sdsuNframe = (uint32) 0;  /* Modif 25 oct 1999 - cb */
      }
      else
      {
         obsId->continuous = FALSE;
         obsId->totalFrames = nframe;
         sdsuNframe = (uint32) 1;  
      }
   }

   /* Set the number of frames by writing to the T_NFRAME parameter in the
    * timing DSP Also define the total number of frames in the observation
    * context structure. */

#ifdef DEBUG
   printf ("detExposure: Setting T_NFRAME parameter to %lu\n", sdsuNframe);
#endif /* DEBUG */

   if ( sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_NFRAME", sdsuNframe ) 
        == ERROR )
   {
      ERROR_LOG ("Error setting number of frames parameter");
      errorNumber = S_detControl_SDSU_ERROR;
   }

   /* Set the exposure time by writing to the T_EXP_TIM parameter in 
    * the timing DSP 
    */

   sdsuTexp = (uint32) (exposure / SDSU_EXPOSURE_UNIT);

#ifdef DEBUG
   printf ("detExposure: Setting T_EXP_TIM parameter to %lu\n", sdsuTexp);
#endif /* DEBUG */

   if ( sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_EXP_TIM", sdsuTexp ) 
        == ERROR )
   {
      ERROR_LOG ("Error setting exposure time parameter");
      errorNumber = S_detControl_SDSU_ERROR;
   }

   /* Update the requested total exposure time in the observation context 
    * structure. 
    */

   obsId->exposedRQ = 1 * exposure;
   sdsuId->exposureTicks = (int) (exposure * sysClkRateGet());

   /* 
    * Set up the observation mode context 
    */
   
   if ( obsId->continuous == TRUE )
   {
      if (epToVxPipeWrite( NULL, "MOVIE", obsId->pObsModeContext ) == ERROR)
      {
         ERROR_LOG ("Failed to set observation mode SIR record");
      }
   }
   else
   {
      if (epToVxPipeWrite( NULL, "STARE", obsId->pObsModeContext ) == ERROR)
      {
         ERROR_LOG ("Failed to set observation mode SIR record");
      }
   }

   /*
    * Set up the requested and actual number of exposure/dataset. 
    * Always 1 for the moment
    */
   nexp = 1 ;
   if (epToVxPipeWrite( NULL, (char *) &nexp, obsId->pNExpRQContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init Number exp/dataset SIR record");
   }
   if (epToVxPipeWrite( NULL, (char *) &nexp, obsId->pNExpContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init Number exp/dataset SIR record");
   }

   /* 
    * Set up the the total integration time requested 
    */

   if (epToVxPipeWrite( NULL, (char *)(int)(&obsId->exposedRQ), 
                        obsId->pExposedRQContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init total integration time requested SIR record");
   }

   /* 
    * Set up the the number of frames per dataset 
    */

   if (epToVxPipeWrite( NULL, (char *)(int)&nframePerDataset, 
                        obsId->pNFramesContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set number of frames SIR record");
   }

   return (errorNumber);
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detObstype
 *
 *   INVOCATION:
 *   detObstype (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *               sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName        (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix   (const char *)    Record name prefix
 *   (>) cadCmdContext   (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber   (int)             Command number
 *   (>) sdsuId          (SDSU_ID)         Current SDSU context structure
 *   (>) obsId           (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detObstype command
 *
 *   DESCRIPTION:
 *   This function defines the observation type.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detObstype
   (
   const char *    pWfsName,        /* Name of wavefront sensor.              */
   const char *    pRecordPrefix,   /* Record name prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32          errorNumber;     /* Error number reported by task.         */

   char            obsType[EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                    /* Observation type string.               */

   /*
    * Initialise the error number and obtain the attributes provided with 
    * the command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, obsType);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
                 "Observation in progress - abort observation and try again", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   strncpy (obsId->pObsType, obsType, EPICS_MAX_BYTES_STRING_ATTRIB);

   MESSAGE_LOG1 (MSG_LOG, "Observation type defined as %s", obsId->pObsType);

   if (epToVxPipeWrite( NULL, obsType, obsId->pObsTypeContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set observation type SIR record");
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   setDhsInfo
 *
 *   INVOCATION:
 *   setDhsInfo (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *               sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detObstype command
 *
 *   DESCRIPTION:
 *   This function sets the quick look stream.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 setDhsInfo
   (
   const char *    pWfsName,        /* Name of wavefront sensor.              */
   const char *    pRecordPrefix,   /* Record name prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId            /* Observation context structure.         */
   )
{
   uint32          errorNumber;     /* Error number reported by task.         */
   long            dhsOutOptions;   /* DHS output options (0=PERM, 1=TEMP,    */
                                    /* 2=QL)                                  */

   char            qlStream[EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                    /* Quick look stream string.              */

   /*
    * Initialise the error number and obtain the attributes provided with 
    * the command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, qlStream);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, 
                          (char *) &dhsOutOptions);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
                 "Observation in progress - abort observation and try again", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   strncpy (obsId->pQlStream, qlStream, EPICS_MAX_BYTES_STRING_ATTRIB);

   obsId->dhsOutOptions = (int) dhsOutOptions;

   MESSAGE_LOG1 (MSG_LOG, "Quick look stream defined as %s", qlStream);
   MESSAGE_LOG1 (MSG_LOG, "DHS output option defined as %d", 
                 (int)dhsOutOptions);

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   setObserve
 *
 *   INVOCATION:
 *   setObserve (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *               sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (!) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute setObserve command 
 *
 *   DESCRIPTION:
 *   This function sets the parameters for an observation.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *-
 */

uint32 setObserve
   (
   const char *    pWfsName,      /* Name of wavefront sensor.                */
   const char *    pRecordPrefix, /* Record name prefix.                      */
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context data structure.      */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */

   long            outOptions;    /* Output options (0=none, 1=DHS, 2=file).  */

   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for files.                         */
   char         pOutFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Output file name (only if DHS not being used)*/
   char         pSimFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Simulated data file name (only if detector   */
                              /* controller is being simulated).              */
   char         pFullOutFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                              /* Combined path name and output file name.     */
   char         pFullSimFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                              /* Combined path name and simulated data file   */
                              /* name.                                        */

   /*
    * Initialise the error number 
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * This command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY, "Observation already in progress", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /* Obtain the attributes */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, 
                          (char *)&outOptions);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pFilePath);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pOutFileName);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, pSimFileName);

   /* Combine file and path name for output file name */

   detCreateFileName ( pFilePath ,
                       pOutFileName ,
                       pFullOutFileName ) ;
   /*
    * Combine the file path and file names, ignoring the path if not
    * specified and preserving any file name set to "NONE" only for simFile.
    */

   if ( strcmp (pFilePath, "") == 0 )
   {
      strncpy (pFullSimFileName, pSimFileName, 
               EPICS_MAX_BYTES_STRING_ATTRIB);
   }
   else
   {
      if ( strcmp(pSimFileName, "NONE") == 0 )
      {
         strncpy (pFullSimFileName, pSimFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullSimFileName, "%s/%s", pFilePath, pSimFileName );
      }
   }

   /*
    * Append the string ".fits" if it is not already present in any file 
    * name, and the name in question is not "NONE".
    */

   if ((strcmp(pFullSimFileName, "NONE") != 0) && 
       (strstr (pFullSimFileName, ".fits") == NULL))
      strncat (pFullSimFileName, ".fits", EPICS_MAX_BYTES_STRING_ATTRIB);


   /* Load up the observation ID structure with the new information. */

   obsId->outOptions = (int) outOptions;
   strncpy( obsId->pOutFileName, pFullOutFileName, 
            EPICS_MAX_BYTES_STRING_ATTRIB*2 );
   strncpy( obsId->pSimFileName, pFullSimFileName, 
            EPICS_MAX_BYTES_STRING_ATTRIB*2 );

   MESSAGE_LOG1 (MSG_LOG, "Output option defined as %d", (int)outOptions);
   MESSAGE_LOG1 (MSG_LOG, "Output file name defined as %s", pFullOutFileName);
   MESSAGE_LOG1 (MSG_LOG, "Simulation file name defined as %s", pFullSimFileName);

   return (errorNumber);
}
/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSetWcs
 *
 *   INVOCATION:
 *   detSetWCs (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *              sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSetWcs command
 *
 *   DESCRIPTION:
 *   This function defines the WCS calibration parameters.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSetWcs
   (
   const char *    pWfsName,         /* Name of wavefront sensor.             */
   const char *    pRecordPrefix,    /* Record name prefix.                   */
   CAD_CMD_CONTEXT cadCmdContext,    /* CAD command context structure.        */
   int             commandNumber,    /* Command number.                       */
   SDSU_ID         sdsuId,           /* SDSU context structure.               */
   OBS_ID          obsId             /* Observation context structure.        */
   )
{
   uint32    errorNumber;     /* Error number reported by task.               */

   char      pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for file.                          */
   char      pWcsFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Name of file containing WCS calibration.     */
   char      pFullWcsFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                              /* Combined path name and file name.            */

   FILE *    pWcsFile;
   char      pLine[100];      /* Line read from file.                         */
   int       nread;           /* Number of items read from file.              */
   int       nextChar;        /* Character sensed from next line.             */
   int       p;               /* Number of points.                            */

   /*
    * Initialise the error number and obtain the attributes provided 
    * with the command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pFilePath);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pWcsFileName);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
                 "Observation in progress - abort observation and try again", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /*
    * Combine the path and file names together, and append the string ".wcs"
    * to the file name
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
    * Open the WCS calibration file and read its contents into the pixij and 
    * fpxy arrays.
    */

   if ((pWcsFile = fopen (pFullWcsFileName, "r")) == NULL)
   {
      ERROR_SET1 (0, "Failed to open WCS calibration file, %s", 
                  ERROR_LOG_NOW, pFullWcsFileName);
      errorNumber = S_detControl_BAD_FILE;
      return (errorNumber);
   }

   /*
    * Skip over any comment lines at the beginning of the file denoted 
    * by a ";" in column 1.
    */

   while ( (nextChar = fgetc (pWcsFile)) == ';' )      
   {
      ungetc (nextChar, pWcsFile);        /* Undo the effect of fgetc().      */
      fgets (pLine, 100, pWcsFile);       /* Skip the whole line.             */
   }
   ungetc (nextChar, pWcsFile);           /* Undo the final fgetc().          */

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
         printf ("detSetWcs: Point %d: %f %f %f %f\n", p, obsId->pixij[p][0], 
                 obsId->pixij[p][1], obsId->fpxy[p][0], obsId->fpxy[p][1]);
#endif
         p++;
      }
   }

   obsId->nWcsPoints = p;
   MESSAGE_LOG1 (MSG_MINDEBUG, "%d points read from WCS calibration file.", p);
   if ( (p == DET_CONTROL_MAX_WCSPOINTS) && (nread != EOF) )
   {
      MESSAGE_LOG (MSG_WARNING, 
          "WARNING: Maximum number of points reached before end of file");
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

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   observeStart
 *
 *   INVOCATION:
 *   observeStart (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *                 sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (!) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute observe command and start observation
 *
 *   DESCRIPTION:
 *   This function starts an observation.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   DHS and WCS code needs tidying up.
 *
 *   BUGS:
 *   dhsErrno keeps having to be reset to DHS_S_SUCCESS. I think this should
 *   not be necessary, and it reveals a bug or bad design feature in the DHS.
 *   Resource freeing functions such as dhsBdDsFree should free their resources
 *   regardless of the value of dhsErrno, since they might be called to tidy up
 *   after an error. SMB - 2 November 1998.
 *
 *   I have now replaced all the dhsErrno resets with CHECK_DHS. This should
 *   report if the DHS status is found not to be DHS_S_SUCCESS at any point.
 *   SMB - 17 November 1998.
 *
 *   The SDSU controller timing board can appear to hang up if the VME board
 *   thinks it is still waiting to receive data from a previous observation.
 *   This may cause the parameter reads from the timing board to fail before
 *   the observation starts. To work around this problem an "ABT" command is
 *   issued to the SDSU VME board before starting the observation.
 *-
 */

uint32 observeStart
   (
   const char *    pWfsName,      /* Name of wavefront sensor.                */
   const char *    pRecordPrefix, /* Record name prefix.                      */
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context data structure.      */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */

   /* Variables describing the observation. */

   int             defOutputs;    /* Default number of outputs.               */

   /* Variables used to specify data label and file names. */

   char *          pLabelFromDhs; /* Data label provided by DHS server.       */

   char            pDataLabel [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                  /* DHS data label.                          */

   /* Other DHS variables */

   DHS_STATUS    dhsErrno;
   char *        axisLabel[2]={"Xaxis","Yaxis"};
   uint32        dims[1];
   uint32        axisSize[2];
   uint32        origin[2];
   char          *qlStreams[1];
   char          *contrib[1];
   double        bzero ;
   char          telName [40] ;

   /* Variables associated with the provision of WCS information. */

   int            wcsStatus=0;     /* WCS status.                             */
   double         pixis;           /* x to i scale factor.                    */
   double         pixjs;           /* y to j scale factor.                    */
   double         perp;            /* Non-perpendicularity of i and j axes in */
                                   /* radians                                 */
   double         orient;          /* Orientation of (i,j) axes with respect  */
                                   /* to (x,y) in radians.                    */
   struct WCS_CTX ctx;             /* World Coordinate System context.        */
   struct WCS     wcs;             /* Basic TCS World Coordinate System.      */
   struct WCS     wcsij;           /* Transformed World Coordinate System for */
   double         trackRA;         /* TCS track Right Ascension.              */
   double         trackDec;        /* TCS track Declination.                  */

                                   /* IJ.                                     */
   FRAMETYPE      trackFrame;      /* TCS track frame                         */
   struct EPOCH   trackEquinox;    /* TCS track equinox.                      */
   struct EPOCH   trackEpoch;      /* TCS track epoch.                        */
   double         trackWavelength; /* Track wavelength in microns.            */
   double         timeTAI;         /* International Atomic Time.              */
   double         rawTimeWcs;      /* Gemini raw time at which WCS info is    */
                                   /* valid.                                  */
   int            chopState;       /* Chop state to which WCS information     */
                                   /* refers.                                 */
   int            p;               /* Point counter.                          */

   char           raString[16];    /* String which contains the RA value      */
   char           decString[16];   /* String which contains the Dec value     */
   float          crpix1Float;     /* Float value of crpix1                   */
   float          crpix2Float;     /* Float value of crpix2                   */
   float          cd1_1Float;      /* Float value of cd1_1                    */
   float          cd1_2Float;      /* Float value of cd1_2                    */
   float          cd2_1Float;      /* Float value of cd2_1                    */
   float          cd2_2Float;      /* Float value of cd2_2                    */

   /* Variables associated with the frame buffers. */

   int            nPixels;         /* Total number of pixels descrambled      */
   int            nPixelsDhs;      /* Total number of pixels displayed        */

   /* SDSU parameters. */

   uint32         expTim;          /* Exp. time in SDSU units from T_EXPTIM.  */
   double         readoutTimeout;  /* Readout timeout in seconds.             */
   double         waitTimeSecs;    /* Wait time in seconds.                   */

   /* 
    * Variables associated with "observe" command.
    * (Label, datapath and filename use general filename parameters)
    */

   long           observingState;  /* Observation status (busy or idle).      */

   /*
    * Initialise the error number and DHS error number.
    */

   errorNumber = 0;
   dhsErrno = DHS_S_SUCCESS;         /* <---- DHS error number is reset here. */

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /* Determine whether a START or STOP directive has been received. */

   if ( EPTOVX_IS_STOP_DIRECTIVE(cadCmdContext) )
   {
      /* Stop directive received - treat as a STOP command and stop 
       * the observation. 
       */

      errorNumber = detStop (pWfsName, pRecordPrefix, cadCmdContext, 
                             commandNumber, sdsuId, obsId);
   }
   else
   {
      /*
       * START directive obtained. This directive cannot be used when an
       * observation is already in progress.
       */

      if ( obsId->observing )
      {
         ERROR_SET (S_detControl_BUSY, "Observation already in progress", 
                 ERROR_LOG_NOW);
         errorNumber = S_detControl_BUSY;
         return (errorNumber);
      }

      /*
       * Reset the dhs counter
       */

      obsId->dhsCounter = 0;

      /* Obtain the attributes */

      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pDataLabel);

      /* Check if the number of frames fits with the dhs output */
      /* Permanent storage should be used with nframe = 1 */

      if ( (obsId->outOptions == 1) && (obsId->dhsOutOptions == 0) && 
           (obsId->totalFrames != 1) )
      {
         ERROR_SET (S_detControl_BAD_ATTRIBUTE,
               "For permanent DHS storage, the number of frame should be 1",
               ERROR_LOG_NOW);
         errorNumber = S_detControl_BAD_ATTRIBUTE;
         return (errorNumber);
      }

      /*
       * Initialize xPixelsDhs and yPixelsDhs 
       */
      
      if ( obsId->fullImageFlag == TRUE )
      {
         obsId->xPixelsDhs = obsId->xPixels;
         obsId->yPixelsDhs = obsId->yPixels;
         sprintf ( obsId->dataSec , "[1:%d,1:%d]" , 
                   obsId->xPixels , obsId->yPixels ) ; 
         sprintf ( obsId->ccdSec , "[1:%d,1:%d]" , 
                   obsId->xPixels , obsId->yPixels ) ; 
         sprintf ( obsId->origSec , "[1:%d,1:%d]" , 
                   obsId->xPixels , obsId->yPixels ) ; 
      }
      else
      {
         if ( obsId->windowingFlag == TRUE )
         {
            obsId->xPixelsDhs = obsId->x2 - obsId->x1 + 1;
            obsId->yPixelsDhs = obsId->y2 - obsId->y1 + 1;
            sprintf ( obsId->dataSec , "[1:%d,1:%d]" , 
                      obsId->xPixelsDhs , obsId->yPixelsDhs ) ; 
            sprintf ( obsId->ccdSec , "[%d:%d,%d:%d]" , 
                      obsId->x1 , obsId->x2 , obsId->y1 , obsId->y2 ) ; 
            if ( obsId->binningFlag == TRUE )
               sprintf ( obsId->origSec , "[1:%d,1:%d]" , 
                         DET_CONTROL_HRWFS_XSIZE/(obsId->xBin) , 
                         DET_CONTROL_HRWFS_YSIZE/(obsId->yBin) ) ; 
            else
               sprintf ( obsId->origSec , "[1:%d,1:%d]" , 
                   DET_CONTROL_HRWFS_XSIZE , 
                         DET_CONTROL_HRWFS_YSIZE ) ; 
         }
         else
         {
            obsId->xPixelsDhs = obsId->xPixels;
            obsId->yPixelsDhs = obsId->yPixels;
            sprintf ( obsId->dataSec , "[1:%d,1:%d]" , 
                      obsId->xPixels , obsId->yPixels ) ; 
            sprintf ( obsId->ccdSec , "[1:%d,1:%d]" , 
                      obsId->xPixels , obsId->yPixels ) ; 
            sprintf ( obsId->origSec , "[1:%d,1:%d]" , 
                      obsId->xPixels , obsId->yPixels ) ; 
         }
      }
         
#ifdef DEBUG
      printf ( "observeStart: xPixelDhs=%d, yPixelDhs=%d\n" , 
               obsId->xPixelsDhs , obsId->yPixelsDhs) ;
#endif

      /*
       * If a request has been made to send data to the DHS, check that the 
       * DHS is available, otherwise reject the command.
       */

      if ( obsId->outOptions == 1 )
      {
         if ( ( !detDhsInitialised ) ||
              ( detDhsConnection == NULL) ||
              /* ( dhsIsConnected (detDhsConnection, &dhsErrno) 
              != DHS_TRUE ) */ /* DOESN'T WORK */
              ( FALSE )        /* BUG WORK AROUND */
            )
         {
            ERROR_SET (S_detControl_BAD_ATTRIBUTE, "DHS is not available", 
                       ERROR_LOG_NOW);
            errorNumber = S_detControl_BAD_ATTRIBUTE;
            return (errorNumber);
         }
      }

      /*
       * Set the observation in progress and observation stopped flags,
       * initialise the frame counter and set the observeC CAR record to BUSY,
       * via the "observing" record.
       */

      obsId->observing = TRUE;
      obsId->stopped = FALSE;
      obsId->nframes = 0;
      obsId->outNFrames = 0;
      observingState = CAR_BUSY;
      if (epToVxPipeWrite (NULL, (char *) &observingState, 
                           obsId->pDetObservingContext) == ERROR)
      {
         ERROR_LOG ("Failed to set observing state to BUSY.");
      }

      /* Ensure whoever is using the system knows when it is in 
       * simulation mode. 
       */

      if ( sdsuId->simulate )
      {
         MESSAGE_LOG (MSG_WARNING, "Observation started in SIMULATION MODE");
      }
      else
      {
         MESSAGE_LOG (MSG_MINDEBUG, "Observation started");
      }

      if ( obsId->outOptions == 1 )
      {
         if ( (strcmp (pDataLabel,"") != 0) && 
              (strcmp (pDataLabel,"NONE") != 0) )
         {
            MESSAGE_LOG1 (MSG_LOG, 
            "Will send data to DHS with data label provided (%s)", pDataLabel);
         }
         else
         {
            pLabelFromDhs = dhsBdName (detDhsConnection, &dhsErrno);
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
               /*sprintf (pDataLabel, "%s.0.0", pLabelFromDhs);*/
               sprintf (pDataLabel, "%s", pLabelFromDhs);

               MESSAGE_LOG1 (MSG_LOG, 
                  "Successfully obtained data label from DHS (%s)",
                  pDataLabel);
            }
         }
      }
      else if ( obsId->outOptions == 2 )
      {
         MESSAGE_LOG1 (MSG_LOG, "Will save data to directly to file \"%s\"",
                       obsId->pOutFileName);
      }

      /*  
       * Load up the observation ID structure with the new information and 
       * the corresponding SIR record. 
       */

      strncpy( obsId->pDataLabel, pDataLabel, EPICS_MAX_BYTES_STRING_ATTRIB);

      if (epToVxPipeWrite( NULL, obsId->pDataLabel, obsId->pDataLabelContext ) == ERROR)
      {
         ERROR_LOG ("Failed to init Data label SIR record");
      }

      /*
       * Before starting the observation, query some parameters from the SDSU
       * controller.
       * Default values for the parameters are assumed in simulation mode or
       * if the parameters could not be obtained.
       */

      defOutputs = 2;            /* Default number of outputs.    */
      readoutTimeout = 20.0;     /* Readout timeout in seconds.   */

      if ( sdsuId->simulate )
      {
         obsId->outputsNb = defOutputs;
         obsId->exposed = obsId->exposedRQ;
      }
      else
      {

         /*
          * BUG WORK AROUND: Before attempting to query parameters from the
          * timing board, send an ABT command to the VME board. This should
          * ensure the board is not in a state where it thinks it is still
          * waiting for data from a previous observation. The parameter reads
          * from the timing board will fail in this circumstance.
          */

         if (sdsuPrimitive (sdsuId, "ABT", SDSU_IDENT_VME, NULL, NULL) == ERROR)
         {
            ERROR_LOG ("ABT command failed prior to starting observation");
         }

         if (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_OUTPUTS", 
                            &(obsId->outputsNb)) == ERROR)
         {
            ERROR_LOG (
            "Failed to query number of outputs from SDSU controller");
            MESSAGE_LOG1 (MSG_WARNING, "Assuming number of outputs is %d", 
                          defOutputs);
            /*obsId->outputs = defOutputs;*/
         }

         if (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_EXP_TIM", &expTim) 
             == ERROR)
         {
            ERROR_LOG ("Failed to query exposure time from SDSU controller");
            MESSAGE_LOG (MSG_WARNING, 
                    "Assuming exposure time is 1 second per frame");
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
            "Zero exposure time obtained from SDSU controller. Assuming min");
            if (obsId->totalFrames > 0)
            {
               obsId->exposed = 
               (double) obsId->totalFrames * (double) SDSU_EXPOSURE_UNIT;
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
            sdsuId->exposureTicks = 
            (int) (expTim * SDSU_EXPOSURE_UNIT * sysClkRateGet());
         }
      }

      /* Get a timestamp to record the time at which the observation started. */

      if ( timeNow (&(obsId->rawtStart)) != OK )
      {
         ERROR_SET (0, "Failed to get time stamp at observation start", 
                    ERROR_LOG_NOW);
      }

#ifdef DEBUG
      printf ("observeStart: Time at observation start: %f seconds.\n", 
              obsId->rawtStart);
#endif

      /*
       * Start the readout process. The observation should now start in 
       * a parallel thread.
       */

      sdsuId->frameTimeout = (int) (obsId->expTime + 60) * sysClkRateGet();

      MESSAGE_LOG2 (MSG_MINDEBUG, 
         "Starting exposure of %f seconds in %d frames...",
         obsId->exposed, obsId->totalFrames);
      if ( obsId->totalFrames > 1 )
      {
         if (sdsuSimpleReadoutStart (sdsuId, 0, (void *) obsId) 
             == ERROR)
         {
            ERROR_LOG ("Failed to start simple readout process");
            obsId->observing = FALSE;
            observingState = CAR_ERROR;
            if (epToVxPipeWrite (NULL, (char *) &observingState, 
                                 obsId->pDetObservingContext) == ERROR)
            {
               ERROR_LOG ("Also failed to set observing state to ERROR.");
            }
            errorNumber = S_detControl_SDSU_ERROR;
            return (errorNumber);
         }
      }
      else
      {
         if (sdsuSimpleReadoutStart (sdsuId, obsId->totalFrames, (void *) obsId) 
             == ERROR)
         {
            ERROR_LOG ("Failed to start simple readout process");
            obsId->observing = FALSE;
            observingState = CAR_ERROR;
            if (epToVxPipeWrite (NULL, (char *) &observingState, 
                                 obsId->pDetObservingContext) == ERROR)
            {
               ERROR_LOG ("Also failed to set observing state to ERROR.");
            }
            errorNumber = S_detControl_SDSU_ERROR;
            return (errorNumber);
         }
      }

      /*
       * To maximise the efficiency, the following code runs in parallel with
       * the observation. If the observation happens to finish before this code
       * completes (which is unlikely) it will wait for the binary semaphore
       * which is given at the end of this function.
       */

      /*
       * Start an alarm timer which will trigger if the frame sync callback
       * never runs. Set the delay time to the readout timeout plus the largest
       * frame exposure time obtained earlier.
       *
       * THE TIMEOUT IS NOW ONLY USED IN SIMULATION MODE - SMB 21 JAN 99
       */

      if ( sdsuId->simulate )
      {
         if ( obsId->exposed >= obsId->exposedRQ )
         {
            if ( obsId->totalFrames > 0 )
            {
               waitTimeSecs = 
               readoutTimeout + (obsId->exposed / (double) obsId->totalFrames);
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
               waitTimeSecs = 
               readoutTimeout + (obsId->exposedRQ / (double)obsId->totalFrames);
            }
            else
            {
               waitTimeSecs = readoutTimeout + obsId->exposedRQ;
            }
         }

         /* BUG WORK AROUND (FOR INTERRUPT VERSION). 
          * Set the frame wait timeout. - SMB 16 Jan 99 
          */


         if ( timeoutAlarmSet (obsId->timeId, waitTimeSecs, 
              detObserveTimeout, (int) obsId) == ERROR )
         {
            ERROR_SET (0, "Failed to set alarm timer", ERROR_LOG_NOW);
            obsId->observing = FALSE;
            observingState = CAR_ERROR;
            if (epToVxPipeWrite (NULL, (char *) &observingState, 
                                 obsId->pDetObservingContext)
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
       * This time will be used to generate the MJD-OBS field in the 
       * FITS header.
       *
       * There is currently no internationally agreed standard defining the 
       * timescale for MJD-OBS. TAI is used here because it is a sensible choice
       * and, in fact, was once specified in a draft standard in July 1996 that
       * was subsequently withdrawn.
       * Whatever timescale is specified here, it is important that it be
       * continuous across a leap second. Suitable alternatives are
       * Terrestrial Time (TT) and Universal Time 1 (UT1). UTC is NOT suitable.
       *
       * NOTE: At time of writing P. Wallace is checking this with the FITS 
       * committee.
       */

      if (timeThenD (obsId->rawtStart, TAI, &timeTAI) != OK)
      {
         ERROR_SET (0,  "Failed to convert time stamp to TAI", ERROR_LOG_NOW);
      }

      /*
       * Get the current tracking frame, as read from the TCS.
       * (Default values will be supplied if the TCS is not available).
       */

      wfsGetTrackFrame (&trackFrame, &(trackEquinox.type), 
         &(trackEquinox.year), &trackWavelength,
         &trackRA, &trackDec, &(trackEpoch.type), &(trackEpoch.year));
      obsId->equinox = trackEquinox.year;
      obsId->epoch   = trackEpoch.year;
      obsId->RA      = trackRA;
      obsId->Dec     = trackDec;

      /*
       * If sufficient WCS calibration points are available, define the WCS
       * information for this observation.
       */

      if ( obsId->nWcsPoints >= 3 )
      {
         /*
          * Define the (i,j) to (X,Y) transformation.
          * N.B. For efficiency, this need only be done once, each time the
          * detector binning is changed. CHANGE THIS EVENTUALLY.
          *
          * First check if any binning or windowing of the pixels on the
          * detector has been defined.
          */

         if ( obsId->fullImageFlag == FALSE )
         {
            /*
             * There has been some binning and windowing. The original 
             * calibration is assumed to have been made on a full frame 
             * of data without binning, so the calibration
             * points need to be transformed.
             */

            for (p=0; p<obsId->nWcsPoints; p++)
            {
               obsId->detij[p][0] =
               ((obsId->pixij[p][0] - 0.5 - 
                 (double) (obsId->x1 - 1)) /
                (double) obsId->xBin) + 0.5;

               obsId->detij[p][1] =
               ((obsId->pixij[p][1] - 0.5 - 
                 (double) (obsId->y1 - 1)) /
                (double) obsId->yBin) + 0.5;
            }

            /*
             * Calculate the best fit to the focal plane X,Y coordinates
             * against binned detector coordinates.
             */

            wcsStatus = 
            astFitij ( obsId->nWcsPoints, obsId->fpxy, obsId->detij, obsId->cij,
                       &pixis, &pixjs, &perp, &orient );
         }
         else
         {
            /*
             * No windowing or binning have been used.
             * Calculate the best fit to the focal plane X,Y coordinates
             * against the original full frame, unbinned pixel coordinates.
             */

            wcsStatus = 
            astFitij ( obsId->nWcsPoints, obsId->fpxy, obsId->pixij, obsId->cij,
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
         printf ("Cij matrix contains %f %f %f %f %f %f\n", obsId->cij[0], 
                 obsId->cij[1],
                 obsId->cij[2], obsId->cij[3], obsId->cij[4], obsId->cij[5]);
#endif

         /*
          * Obtain the current TCS context from the locally stored copy.
          * This assumes that a TCS context has been obtained elsewhere and
          * stored using astSetCtx(), as described in section 3 of document
          * tcs_ptw_008.
          */

         if ( wcsStatus == 0 )
         {
            wcsStatus = astGetctx (&ctx);
            if (wcsStatus != 0)
            {
               ERROR_SET1 (S_detControl_WCS_ERROR, 
                  "Failed to get current WCS context. Status=%d",
                  ERROR_LOG_NOW, wcsStatus);
            }
         }

         /*
          * Set the chop state to which the WCS coordinate information refers.
          * NOTE: THE ACTUAL CHOP STATE NEEDS TO BE OBTAINED FROM THE PARAMETER
          * GIVEN TO THE "SET CHOP STATE" COMMAND.
          */

         chopState = 0;         /* 0 means chop state A. */

         /*
          * Extract the current focal plane to sky WCS transformation from 
          * the TCS context.
          */

         if ( wcsStatus == 0 )
         {
            wcsStatus = 
            astCtx2tr (ctx, trackFrame, trackEquinox, trackWavelength,
                       chopState, &wcs, &rawTimeWcs);
            if (wcsStatus != 0)
            {
               ERROR_SET1 (S_detControl_WCS_ERROR,
               "Failed to get focal plane to sky WCS transformation from TCS. Status=%d",
               ERROR_LOG_NOW, wcsStatus);
            }
         }

#ifdef DEBUG
         printf (
         "WCS information extracted from TCS context is valid at time %f\n",
         rawTimeWcs);
#endif

         /*
          * Combine the (i,j) to (x,y) model, cij, and (x,y) to (RA,Dec) model,
          * wcs, into a single (i,j) to (RA,Dec) model, wcsij.
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
           * Calculate the WCS header values, expressed in terms of
           * standard FITS header items.
           */

         if ( wcsStatus == 0 )
         {
            wcsStatus = astFITSv (wcsij, trackFrame, trackEquinox, timeTAI,
               obsId->ctype1, &(obsId->crpix1), &(obsId->crval1),
               obsId->ctype2, &(obsId->crpix2), &(obsId->crval2),
               &(obsId->cd1_1), &(obsId->cd1_2), &(obsId->cd2_1), 
               &(obsId->cd2_2), obsId->radecsys, &(obsId->equinox), 
               &(obsId->mjdobs));
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
          * There is insufficient information to provide WCS information.
          * The only valid item which can be added to the header is the 
          * MJD of the observation.
          */

         MESSAGE_LOG (MSG_WARNING,
            "No WCS calibration - there will be no WCS header");

         obsId->mjdobs = timeTAI;
         obsId->wcsStatus = -1;
      }

      /*
       * Convert the time stamps from Gemini raw time into Universal Time
       * and construct these into character strings.
       */

      if (timeThenC( obsId->rawtStart, UT1, 3, obsId->timeArrayStart ) != OK)
      {
         ERROR_SET (0,
            "Failed to convert time stamp at observation start to date/time",
            ERROR_LOG_NOW);
      }
      sprintf (obsId->utStartString, "%04d-%02d-%02d:%02d:%02d:%02d.%03d",
               obsId->timeArrayStart[0], obsId->timeArrayStart[1], obsId->timeArrayStart[2],
               obsId->timeArrayStart[3], obsId->timeArrayStart[4], obsId->timeArrayStart[5],
               obsId->timeArrayStart[6]);

      if (epToVxPipeWrite( NULL, (char *)obsId->utStartString, obsId->pUTstartContext ) == ERROR)
      {
         ERROR_LOG ("Failed to set UT at start of observation SIR record");
      }

#ifdef DEBUG
      printf ( "obsId->utStartString = %s\n" , obsId->utStartString ) ;
#endif


      /*
       * If the DHS is being used then create a dataset to hold the 
       * unscrambled data. Otherwise allocate a buffer directly.
       */

      if ( obsId->outOptions == 1 )
      {

         MESSAGE_LOG (MSG_MINDEBUG, "Creating DHS dataset...");

         /* Set up for the quick look */

         contrib[0] = pDetDhsClientName; 
                                /* a global variable, set in detDhsInit */

         qlStreams[0] = calloc ( EPICS_MAX_BYTES_STRING_ATTRIB+1, sizeof (char) ) ;

         if ( strcmp (obsId->pQlStream , "" ) == 0 )
            strcpy ( qlStreams[0] , "hrwfsScience") ;
         else
         {
            strncpy ( qlStreams[0] , obsId->pQlStream , EPICS_MAX_BYTES_STRING_ATTRIB ) ;
         }

         printf ( "obsId->pQlStream=%s\n" , obsId->pQlStream ) ;
         printf ( "qlStreams[0]=%s\n" , qlStreams[0] ) ;

         /*qlStreams[0] = "hrwfsScience"; */

         wfsGetTelName ( telName ) ;
         printf ( "telName=%s\n" , telName ) ;

         /* NOTE: Lifetime should be definable
          * PERMANENT for permanent data (e.g. calibrations)
          * TRANSIENT for display only (e.g. AC in continuous mode)
          * (see ICD 3).
          */

         /*if ( obsId->totalFrames == 1 )*/            /* only one exposure */
         /*{*/
         if ( obsId->dhsOutOptions == 0 )
         {
            dhsBdCtl(detDhsConnection, DHS_BD_CTL_LIFETIME, 
                     obsId->pDataLabel, DHS_BD_LT_PERMANENT, &dhsErrno);
            MESSAGE_LOG1 (MSG_MINDEBUG,
               "LIFETIME of DHS frame %s set to DHS_BD_LT_PERMANENT", 
               obsId->pDataLabel);
            MESSAGE_LOG1 (MSG_MINDEBUG,
               "Total Frames is %d", obsId->totalFrames);
         }
         else           /* either continuous mode with totalFrames = 0 or > 1 */
         {
            dhsBdCtl(detDhsConnection, DHS_BD_CTL_LIFETIME, 
                     obsId->pDataLabel, DHS_BD_LT_TRANSIENT, &dhsErrno);
            MESSAGE_LOG1 (MSG_MINDEBUG,
               "LIFETIME of DHS frame %s set to DHS_BD_LT_TRANSIENT", 
               obsId->pDataLabel);
            MESSAGE_LOG1 (MSG_MINDEBUG,
               "Total Frames is %d", obsId->totalFrames);
         }
         CHECK_DHS (dhsErrno);
         dhsBdCtl(detDhsConnection, DHS_BD_CTL_CONTRIB, 
            obsId->pDataLabel, 1, contrib, &dhsErrno);
         CHECK_DHS (dhsErrno);
         dhsBdCtl(detDhsConnection, DHS_BD_CTL_QLSTREAM, 
                  obsId->pDataLabel, 1, qlStreams, &dhsErrno);
         CHECK_DHS (dhsErrno);

         free ( qlStreams[0] ) ;

         /* Create the DHS dataset and add the default attributes. */

         obsId->dhsDataset = dhsBdDsNew (&dhsErrno);
         CHECK_DHS (dhsErrno);

         if (dhsErrno == DHS_S_SUCCESS)
         {
            dhsBdAttribAdd (obsId->dhsDataset, "instrument", 
               DHS_DT_STRING, 0, NULL, pDetDhsClientName, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataset, "telescope", DHS_DT_STRING, 
                            0, NULL, telName, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataset, "observatory", DHS_DT_STRING, 
                            0, NULL, telName, &dhsErrno);
            CHECK_DHS (dhsErrno);
         }

         if (dhsErrno != DHS_S_SUCCESS)
         {
            ERROR_SET1 (S_detControl_DHS_ERROR, 
                        "Failed to create dataset (dhsErrno=%d)",
                        ERROR_LOG_NOW, dhsErrno);
            errorNumber = S_detControl_DHS_ERROR;
            return (errorNumber);
         }

         /* Create a frame to contain the data and add the frame header info. */

         dims[0] = 2;
         axisSize[0] = obsId->xPixelsDhs;  
         axisSize[1] = obsId->yPixelsDhs;
         origin[0] = 1;  
         origin[1] = 1;

         dhsErrno = DHS_S_SUCCESS;
         obsId->dhsDataFrame = 
         dhsBdFrameNew (obsId->dhsDataset, "dataArray", 0, DHS_DT_UINT16,
                        2, axisSize,
                        (const void **) &(obsId->pDispFrame), &dhsErrno);
         CHECK_DHS (dhsErrno);

         if (dhsErrno == DHS_S_SUCCESS)
         {
            dhsBdAttribAdd (obsId->dhsDataFrame, "dataType", 
                            DHS_DT_STRING, 0, NULL, "Intensity",
                            &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "bunit", DHS_DT_STRING, 0, 
                            NULL, DET_BUNIT, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "units", DHS_DT_STRING, 0, 
                            NULL, DET_BUNIT, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "origin", DHS_DT_INT32, 1, 
                            dims, origin, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "axisSize", DHS_DT_INT32, 1, 
                            dims, axisSize, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "axisLabel", DHS_DT_STRING, 1,
                            dims, axisLabel, &dhsErrno);
            CHECK_DHS (dhsErrno);

            bzero=(double)(32768.0) ;
            dhsBdAttribAdd (obsId->dhsDataFrame, "bzero", DHS_DT_DOUBLE, 0,
                            NULL, bzero, &dhsErrno);
            CHECK_DHS (dhsErrno);

            dhsBdAttribAdd (obsId->dhsDataFrame, "obstype", DHS_DT_STRING, 0, 
                            NULL, obsId->pObsType, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "exptime", DHS_DT_DOUBLE, 0, 
                            NULL, obsId->exposed, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "darktime", DHS_DT_DOUBLE, 0, 
                            NULL, obsId->exposed, &dhsErrno);
            CHECK_DHS (dhsErrno);

            /* WCS attributes */

            if ( wcsStatus == 0 )
            {
               dhsBdAttribAdd (obsId->dhsDataFrame, "ctype1", DHS_DT_STRING, 
                               0, NULL, obsId->ctype1, &dhsErrno);
               CHECK_DHS (dhsErrno);
               crpix1Float = (float)(obsId->crpix1);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CRPIX1", DHS_DT_FLOAT, 
                               0, NULL, crpix1Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CRVAL1", DHS_DT_DOUBLE, 
                               0, NULL, obsId->crval1, &dhsErrno);
               CHECK_DHS (dhsErrno);
               dhsBdAttribAdd (obsId->dhsDataFrame, "ctype2", DHS_DT_STRING, 
                               0, NULL, obsId->ctype2, &dhsErrno);
               CHECK_DHS (dhsErrno);
               crpix2Float = (float)(obsId->crpix2);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CRPIX2", DHS_DT_FLOAT, 
                               0, NULL, crpix2Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CRVAL2", DHS_DT_DOUBLE, 
                               0, NULL, obsId->crval2, &dhsErrno);
               CHECK_DHS (dhsErrno);
               cd1_1Float = (float)(obsId->cd1_1);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CD1_1", DHS_DT_FLOAT, 
                               0, NULL, cd1_1Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               cd1_2Float = (float)(obsId->cd1_2);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CD1_2", DHS_DT_FLOAT, 
                               0, NULL, cd1_2Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               cd2_1Float = (float)(obsId->cd2_1);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CD2_1", DHS_DT_FLOAT, 
                               0, NULL, cd2_1Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               cd2_2Float = (float)(obsId->cd2_2);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CD2_2", DHS_DT_FLOAT, 
                               0, NULL, cd2_2Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
            }

            sprintf ( raString , "%f" , obsId->RA ) ;
            sprintf ( decString , "%f" , obsId->Dec ) ;

            dhsBdAttribAdd (obsId->dhsDataFrame, "RA", DHS_DT_STRING, 0, NULL,
                            raString, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "DEC", DHS_DT_STRING, 0, NULL,
                            decString, &dhsErrno);
            CHECK_DHS (dhsErrno);

            dhsBdAttribAdd (obsId->dhsDataFrame, "equinox", DHS_DT_DOUBLE, 0, 
                            NULL, obsId->equinox, &dhsErrno);
            CHECK_DHS (dhsErrno);

            dhsBdAttribAdd (obsId->dhsDataFrame, "epoch", DHS_DT_DOUBLE, 0, 
                            NULL, obsId->epoch, &dhsErrno);
            CHECK_DHS (dhsErrno);

            dhsBdAttribAdd (obsId->dhsDataFrame, "mjd-obs", DHS_DT_DOUBLE, 
                            0, NULL, obsId->mjdobs, &dhsErrno);
            CHECK_DHS (dhsErrno);

            /* ADD MORE DATA FRAME HEADER ITEMS HERE. */

            dhsBdAttribAdd (obsId->dhsDataFrame, "xbin", DHS_DT_INT32, 
                            0, NULL, obsId->xBin, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "ybin", DHS_DT_INT32, 
                            0, NULL, obsId->yBin, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "datasec", DHS_DT_STRING, 
                            0, NULL, obsId->dataSec, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "ccdsec", DHS_DT_STRING, 
                            0, NULL, obsId->ccdSec, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "origsec", DHS_DT_STRING, 
                            0, NULL, obsId->origSec, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "utstart", DHS_DT_STRING, 
                            0, NULL, obsId->utStartString, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "dettype", DHS_DT_STRING, 
                            0, NULL, obsId->detType, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "detid", DHS_DT_STRING, 
                            0, NULL, obsId->detId, &dhsErrno);
            CHECK_DHS (dhsErrno);

         }

         if (dhsErrno != DHS_S_SUCCESS)
         {
            ERROR_SET1 (S_detControl_DHS_ERROR, 
                        "Failed to create new data frame (dhsErrno=%d)",
                        ERROR_LOG_NOW, dhsErrno);
            errorNumber = S_detControl_DHS_ERROR;
            return (errorNumber);
         }

         /* Create a frame in case of windowing */

         if ( obsId->windowingFlag == TRUE )
         {
            nPixels = obsId->xPixels * obsId->yPixels;

            obsId->pCurFrame = (uint16 *) malloc (nPixels * sizeof(uint16));
            if ( obsId->pCurFrame == NULL )
            {
               ERROR_LOG( "Failed to allocate image buffer for unscrambled data" );
               errorNumber = S_detControl_INTERNAL;
               return (errorNumber);
            }
            /*printf ( "DHS option : malloc pCurFrame=%p\n" , obsId->pCurFrame ) ;*/
         }
         else
         {
            obsId->pCurFrame = obsId->pDispFrame ;
            /*printf ( "DHS option : pDispFrame and pCurFrame=%p\n" , obsId->pCurFrame ) ;*/
         }
      }
      else
      {
         /* The DHS is not being used. */

         /* Calculate the number of pixels and reserve a buffer for 
          * the unscrambled data. 
          */

         nPixels = obsId->xPixels * obsId->yPixels;
         nPixelsDhs = obsId->xPixelsDhs * obsId->yPixelsDhs;

#ifdef DEBUG
         printf ("observeStart: Allocating frame buffer to hold %d pixels "
                 "of unscrambled data.\n", nPixels);
#endif /* DEBUG */

         obsId->pDispFrame = (uint16 *) malloc (nPixelsDhs * sizeof(uint16));
         if ( obsId->pDispFrame == NULL )
         {
            ERROR_LOG( "Failed to allocate image buffer to display data" );
            errorNumber = S_detControl_INTERNAL;
            return (errorNumber);
         }

         /*printf ( "File option : malloc pDispFrame=%p\n" , obsId->pDispFrame ) ;*/
         if ( obsId->windowingFlag == TRUE )
         {
            obsId->pCurFrame = (uint16 *) malloc (nPixels * sizeof(uint16));
            if ( obsId->pCurFrame == NULL )
            {
               ERROR_LOG( "Failed to allocate image buffer for unscrambled data" );
               errorNumber = S_detControl_INTERNAL;
               return (errorNumber);
            }
            /*printf ( "File option : malloc pCurFrame=%p\n" , obsId->pCurFrame ) ;*/
         }
         else
         {
            obsId->pCurFrame = obsId->pDispFrame ;
            /*printf ( "DHS option : pDispFrame and pCurFrame=%p\n" , obsId->pCurFrame ) ;*/
         }
      }

      /*
       * Give the binary semaphore, which will allow the observation thread to
       * process the data.
       */

      semGive (obsId->syncSem);

#ifdef DEBUG
      printf ("observeStart: START directive finished.\n");
#endif /* DEBUG */

   }

   return (errorNumber);
}
/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detObserveStart
 *
 *   INVOCATION:
 *   detObserveStart (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *                    sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName        (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix   (const char *)    Record name prefix
 *   (>) cadCmdContext   (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber   (int)             Command number
 *   (>) sdsuId          (SDSU_ID)         Current SDSU context structure
 *   (!) obsId           (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detObserve command and start observation
 *
 *   DESCRIPTION:
 *   This function starts an observation.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   DHS and WCS code needs tidying up.
 *
 *   BUGS:
 *   dhsErrno keeps having to be reset to DHS_S_SUCCESS. I think this should
 *   not be necessary, and it reveals a bug or bad design feature in the DHS.
 *   Resource freeing functions such as dhsBdDsFree should free their resources
 *   regardless of the value of dhsErrno, since they might be called to tidy up
 *   after an error. SMB - 2 November 1998.
 *
 *   I have now replaced all the dhsErrno resets with CHECK_DHS. This should
 *   report if the DHS status is found not to be DHS_S_SUCCESS at any point.
 *   SMB - 17 November 1998.
 *
 *   The SDSU controller timing board can appear to hang up if the VME board
 *   thinks it is still waiting to receive data from a previous observation.
 *   This may cause the parameter reads from the timing board to fail before
 *   the observation starts. To work around this problem an "ABT" command is
 *   issued to the SDSU VME board before starting the observation.
 *-
 */

uint32 detObserveStart
   (
   const char *    pWfsName,       /* Name of wavefront sensor.               */
   const char *    pRecordPrefix,  /* Record name prefix.                     */
   CAD_CMD_CONTEXT cadCmdContext,  /* CAD command context structure.          */
   int             commandNumber,  /* Command number.                         */
   SDSU_ID         sdsuId,         /* SDSU context structure.                 */
   OBS_ID          obsId           /* Observation context data structure.     */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */

   /* Variables describing the observation. */

   int             defOutputs;    /* Default number of outputs.               */

   /* Variables used to specify data label and file names. */

   long            outOptions;    /* Output options (0=none, 1=DHS, 2=file).  */
   long            dhsOutOptions; /* DHS output options (0=PERM, 1=TEMP, 2=QL)*/

   char *          pLabelFromDhs; /* Data label provided by DHS server.       */

   char         pDataLabel [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* DHS data label.                              */
   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for files.                         */
   char         pOutFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Output file name (only if DHS not being used)*/
   char         pSimFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Simulated data file name (only if detector   */
                              /* controller is being simulated).              */
   char         pFullOutFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                              /* Combined path name and output file name.     */
   char         pFullSimFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                              /* Combined path name and simulated data file   */
                              /* name.                                        */

   /* Other DHS variables */

   DHS_STATUS    dhsErrno;
   char *        axisLabel[2]={"Xaxis","Yaxis"};
   uint32        dims[1];
   uint32        axisSize[2];
   uint32        origin[2];
   char          *qlStreams[1];
   char          *contrib[1];
   double        bzero ;
   char          telName [40] ;

   /* Variables associated with the provision of WCS information. */

   int            wcsStatus = 0;   /* WCS status.                             */
   double         pixis;           /* x to i scale factor.                    */
   double         pixjs;           /* y to j scale factor.                    */
   double         perp;            /* Non-perpendicularity of i and j axes in */
                                   /* radians                                 */
   double         orient;          /* Orientation of (i,j) axes with respect  */
                                   /* to (x,y) in radians.                    */
   struct WCS_CTX ctx;             /* World Coordinate System context.        */
   struct WCS     wcs;             /* Basic TCS World Coordinate System.      */
   struct WCS     wcsij;           /* Transformed World Coordinate System for */
   double         trackRA;         /* TCS track Right Ascension.              */
   double         trackDec;        /* TCS track Declination.                  */

                                   /* IJ.                                     */
   FRAMETYPE      trackFrame;      /* TCS track frame                         */
   struct EPOCH   trackEquinox;    /* TCS track equinox.                      */
   struct EPOCH   trackEpoch;      /* TCS track epoch.                        */
   double         trackWavelength; /* Track wavelength in microns.            */
   double         timeTAI;         /* International Atomic Time.              */
   double         rawTimeWcs;      /* Gemini raw time at which WCS info is    */
                                   /* valid.                                  */
   int            chopState;       /* Chop state to which WCS information     */
                                   /* refers.                                 */
   int            p;               /* Point counter.                          */

   char           raString[16];    /* String which contains the RA value      */
   char           decString[16];   /* String which contains the Dec value     */
   float          crpix1Float;     /* Float value of crpix1                   */
   float          crpix2Float;     /* Float value of crpix2                   */
   float          cd1_1Float;      /* Float value of cd1_1                    */
   float          cd1_2Float;      /* Float value of cd1_2                    */
   float          cd2_1Float;      /* Float value of cd2_1                    */
   float          cd2_2Float;      /* Float value of cd2_2                    */

   /* Variables associated with the frame buffers. */

   int            nPixels;         /* Total number of pixels descrambled      */
   int            nPixelsDhs;      /* Total number of pixels displayed        */

   /* SDSU parameters. */

   long           nframe;          /* Number of frames.                       */
   long           nframePerDataset;/* Number of frames per dataset            */
   double         exposure;        /* Exposure time in seconds.               */

   uint32         sdsuNframe;      /* Value for SDSU parameter NFRAME.        */
   uint32         expTim;          /* Exp. time in SDSU units from T_EXPTIM.  */
   double         readoutTimeout;  /* Readout timeout in seconds.             */
   double         waitTimeSecs;    /* Wait time in seconds.                   */

   int            nexp;            /* Number of exposure/dataset              */

   /* 
    * Variables associated with "observe" command.
    * (Label, datapath and filename use general filename parameters)
    */

   long           observingState;  /* Observation status (busy or idle).      */

   /*
    * Initialise the error number and DHS error number.
    */

   errorNumber = 0;
   dhsErrno = DHS_S_SUCCESS;         /* <---- DHS error number is reset here. */

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /* Determine whether a START or STOP directive has been received. */

   if ( EPTOVX_IS_STOP_DIRECTIVE(cadCmdContext) )
   {
      /* Stop directive received - treat as a STOP command and stop 
       * the observation. 
       */

      errorNumber = detStop (pWfsName, pRecordPrefix, cadCmdContext, 
                             commandNumber, sdsuId, obsId);
   }
   else
   {
      /*
       * START directive obtained. This directive cannot be used when an
       * observation is already in progress.
       */

      if ( obsId->observing )
      {
         ERROR_SET (S_detControl_BUSY, "Observation already in progress", 
                 ERROR_LOG_NOW);
         errorNumber = S_detControl_BUSY;
         return (errorNumber);
      }

      /* 
       * Reset dhs counter
       */

      obsId->dhsCounter = 0;

#ifdef DEBUG
      /* MODIF 23 SEPT */
      flagFirstTime = FALSE ;
      flagSecondTime = FALSE ;
      readTime1=0;
      readTime2=0;
      unscrambleTime1=0;
      unscrambleTime2=0;
      dhsTime1=0;
      dhsTime2=0;
#endif

      /* Obtain the attributes */

      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, 
                             (char *) & nframe);
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, 
                             (char *) & exposure);
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, 
                             (char *)&outOptions);
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, pDataLabel);
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, 
                             (char *) &dhsOutOptions);
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, pFilePath);
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6, pOutFileName);
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 7, pSimFileName);

      /* Check the number of frames is sensible */

      if ( (nframe <= 0) && (nframe != -1) )
      {
         ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, "Invalid number of frames, %ld",
                     ERROR_LOG_NOW, nframe);
         errorNumber = S_detControl_BAD_ATTRIBUTE;
         return (errorNumber);
      }

      /*
       * Check the exposure time is sensible. The SDSU controller measures
       * exposures in units of 81.92 microseconds and stores the exposure in a
       * 32 bit integer, so the upper limit in seconds is 2**32 * 0.000008192 =
       * 351,843 seconds
       */

      if ( (exposure < 0.0) || (exposure > 351843.0) )
      {
         ERROR_SET1 (S_detControl_BAD_ATTRIBUTE,
                     "Invalid exposure time, %f seconds.",
                     ERROR_LOG_NOW, exposure);
         errorNumber = S_detControl_BAD_ATTRIBUTE;
         return (errorNumber);
      }

      obsId->expTime = exposure;

      if (epToVxPipeWrite( NULL, (char *)(int)&exposure, 
                           obsId->pIntTimeContext ) == ERROR)
      {
         ERROR_LOG ("Failed to set integration time SIR record");
      }

      /*
       * Determine how to read the CCD when in continuous mode:
       * if exposure < readoutTime -> serie of 1 frame
       * if exposure > readoutTime -> CCD in continuous mode
       */

      sdsuId->readoutTime = (SDSU_FULL_READOUT * obsId->pixelsNb) /
                            (DET_CONTROL_HRWFS_XSIZE * DET_CONTROL_HRWFS_YSIZE);

      /*if ( exposure < sdsuId->readoutTime )
      {
         sdsuId->readMethod = 0;
         printf ( "exp < readoutTime (%f<%f) - pixelsNb = %d\n" , 
                  exposure, sdsuId->readoutTime, obsId->pixelsNb);
      }
      else
      {
         sdsuId->readMethod = 1;
         printf ( "exp > readoutTime (%f>%f) - pixelsNb = %d\n" , exposure, 
                  sdsuId->readoutTime, obsId->pixelsNb);
      }*/

      sdsuId->readMethod = 0;

      /* Check if the number of frames fits with the dhs output */
      /* Permanent storage should be used with nframe = 1 */

      if ( (outOptions == 1) && (dhsOutOptions == 0) && (nframe != 1) )
      {
         ERROR_SET (S_detControl_BAD_ATTRIBUTE,
               "For permanent DHS storage, the number of frame should be 1",
               ERROR_LOG_NOW);
         errorNumber = S_detControl_BAD_ATTRIBUTE;
         return (errorNumber);
      }

      /* Combine file and path name for output file name */

      detCreateFileName ( pFilePath ,
                          pOutFileName ,
                          pFullOutFileName ) ;
      /*
       * Combine the file path and file names, ignoring the path if not
       * specified and preserving any file name set to "NONE" only for simFile.
       */

      if ( strcmp (pFilePath, "") == 0 )
      {
         strncpy (pFullSimFileName, pSimFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         if ( strcmp(pSimFileName, "NONE") == 0 )
         {
            strncpy (pFullSimFileName, pSimFileName, 
                     EPICS_MAX_BYTES_STRING_ATTRIB);
         }
         else
         {
            sprintf (pFullSimFileName, "%s/%s", pFilePath, pSimFileName );
         }
      }

      /*
       * Append the string ".fits" if it is not already present in any file 
       * name, and the name in question is not "NONE".
       */

      if ((strcmp(pFullSimFileName, "NONE") != 0) && 
          (strstr (pFullSimFileName, ".fits") == NULL))
         strncat (pFullSimFileName, ".fits", EPICS_MAX_BYTES_STRING_ATTRIB);

      /*
       * Initialize xPixelsDhs and yPixelsDhs 
       */
      
      if ( obsId->fullImageFlag == TRUE )
      {
         obsId->xPixelsDhs = obsId->xPixels;
         obsId->yPixelsDhs = obsId->yPixels;
         sprintf ( obsId->dataSec , "[1:%d,1:%d]" , 
                   obsId->xPixels , obsId->yPixels ) ; 
         sprintf ( obsId->ccdSec , "[1:%d,1:%d]" , 
                   obsId->xPixels , obsId->yPixels ) ; 
         sprintf ( obsId->origSec , "[1:%d,1:%d]" , 
                   obsId->xPixels , obsId->yPixels ) ; 
      }
      else
      {
         if ( obsId->windowingFlag == TRUE )
         {
            if ( obsId->oscanNb != 0 )
            {
               if ( obsId->oscanFlag == FULL )
                  obsId->xPixelsDhs = 
                  obsId->x2 - obsId->x1 + 1 + 2*obsId->oscanNb;
               else
                  obsId->xPixelsDhs = 
                  obsId->x2 - obsId->x1 + 1 + obsId->oscanNb;
            }
            else
               obsId->xPixelsDhs = obsId->x2 - obsId->x1 + 1;

            obsId->yPixelsDhs = obsId->y2 - obsId->y1 + 1;
            sprintf ( obsId->dataSec , "[1:%d,1:%d]" , 
                      obsId->xPixelsDhs , obsId->yPixelsDhs ) ; 
            sprintf ( obsId->ccdSec , "[%d:%d,%d:%d]" , 
                      obsId->x1 , obsId->x2 , obsId->y1 , obsId->y2 ) ; 
            if ( obsId->binningFlag == TRUE )
               sprintf ( obsId->origSec , "[1:%d,1:%d]" , 
                         DET_CONTROL_HRWFS_XSIZE/(obsId->xBin) , 
                         DET_CONTROL_HRWFS_YSIZE/(obsId->yBin) ) ; 
            else
               sprintf ( obsId->origSec , "[1:%d,1:%d]" , 
                   DET_CONTROL_HRWFS_XSIZE , 
                         DET_CONTROL_HRWFS_YSIZE ) ; 
         }
         else
         {
            obsId->xPixelsDhs = obsId->xPixels;
            obsId->yPixelsDhs = obsId->yPixels;
            sprintf ( obsId->dataSec , "[1:%d,1:%d]" , 
                      obsId->xPixels , obsId->yPixels ) ; 
            sprintf ( obsId->ccdSec , "[1:%d,1:%d]" , 
                      obsId->xPixels , obsId->yPixels ) ; 
            sprintf ( obsId->origSec , "[1:%d,1:%d]" , 
                      obsId->xPixels , obsId->yPixels ) ; 
         }
      }
         
/*#ifdef DEBUG*/
      printf ( "detObserveStart: xPixelDhs=%d, yPixelDhs=%d\n" , 
               obsId->xPixelsDhs , obsId->yPixelsDhs) ;
/*#endif*/

      /*
       * If a request has been made to send data to the DHS, check that the 
       * DHS is available, otherwise reject the command.
       */

      if ( outOptions == 1 )
      {
         if ( ( !detDhsInitialised ) ||
              ( detDhsConnection == NULL) ||
              /* ( dhsIsConnected (detDhsConnection, &dhsErrno) 
              != DHS_TRUE ) */ /* DOESN'T WORK */
              ( FALSE )        /* BUG WORK AROUND */
            )
         {
            ERROR_SET (S_detControl_BAD_ATTRIBUTE, "DHS is not available", 
                       ERROR_LOG_NOW);
            errorNumber = S_detControl_BAD_ATTRIBUTE;
            return (errorNumber);
         }
      }

      /*
       * Set the observation in progress and observation stopped flags,
       * initialise the frame counter and set the observeC CAR record to BUSY,
       * via the "observing" record.
       */

      obsId->observing = TRUE;
      obsId->stopped = FALSE;
      obsId->nframes = 0;
      obsId->outNFrames = 0;
      observingState = CAR_BUSY;
      if (epToVxPipeWrite (NULL, (char *) &observingState, 
                           obsId->pDetObservingContext) == ERROR)
      {
         ERROR_LOG ("Failed to set observing state to BUSY.");
      }

      /* Ensure whoever is using the system knows when it is in 
       * simulation mode. 
       */

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
         if ( (strcmp (pDataLabel,"") != 0) && 
              (strcmp (pDataLabel,"NONE") != 0) )
         {
            MESSAGE_LOG1 (MSG_LOG, 
            "Will send data to DHS with data label provided (%s)", pDataLabel);
         }
         else
         {
            pLabelFromDhs = dhsBdName (detDhsConnection, &dhsErrno);
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
               /*sprintf (pDataLabel, "%s.0.0", pLabelFromDhs);*/
               sprintf (pDataLabel, "%s", pLabelFromDhs);

               MESSAGE_LOG1 (MSG_LOG, 
                  "Successfully obtained data label from DHS (%s)",
                  pDataLabel);
            }
         }
      }
      else if ( outOptions == 2 )
      {
         MESSAGE_LOG1 (MSG_LOG, "Will save data to directly to file \"%s\"",
                       pFullOutFileName);
      }

      /*
       * Load up the observation ID structure with the new information and the
       * corresponding SIR record
       */

      obsId->outOptions = (int) outOptions;
      obsId->dhsOutOptions = (int) dhsOutOptions;
      strncpy( obsId->pDataLabel, pDataLabel, EPICS_MAX_BYTES_STRING_ATTRIB);

      if (epToVxPipeWrite( NULL, obsId->pDataLabel, obsId->pDataLabelContext ) == ERROR)
      {
         ERROR_LOG ("Failed to init Data label SIR record");
      }

      strncpy( obsId->pOutFileName, pFullOutFileName, 
               EPICS_MAX_BYTES_STRING_ATTRIB*2 );
      strncpy( obsId->pSimFileName, pFullSimFileName, 
               EPICS_MAX_BYTES_STRING_ATTRIB*2 );

      /*
       * Before starting the observation, query some parameters from the SDSU
       * controller.
       * Default values for the parameters are assumed in simulation mode or
       * if the parameters could not be obtained.
       */

      defOutputs = 2;            /* Default number of outputs.    */
      readoutTimeout = 20.0;     /* Readout timeout in seconds.   */

      if ( sdsuId->simulate )
      {
         obsId->outputsNb = defOutputs;
         obsId->exposed = obsId->exposedRQ;
      }
      else
      {
         if ( nframe == -1 )
         {
            MESSAGE_LOG1 (MSG_LOG, 
            "Setting up for an infinite series of exposures of %f seconds each",
            exposure);

            nframePerDataset = nframe ;

            /* BUG WORK AROUND: THE SDSU CONTROLLER RETURNS FRAME COUNT=1 
             * WHEN ASKED FOR AN INFINITE
             * NUMBER OF FRAMES, WHICH DETCONTROL THEN ASSUMES MEANS THE 
             * LAST FRAME HAS BEEN RECEIVED.
             * UNTIL THE SDSU CODE IS FIXED, SET A FLAG TO INDICATE THE FRAME 
             * COUNT IS INFINITE.
             */

            if ( sdsuId->readMethod == 1 )
            {
               nframe = 0;
                       /* DSP code assumes 0 means infinite number of frames. */

               obsId->continuous = TRUE;
               obsId->totalFrames = nframe;
               sdsuNframe = (uint32) nframe;  /* Modif 23 sept 1999 - cb */
            }
            else
            {
               nframe = 1;
               obsId->continuous = TRUE;
               obsId->totalFrames = 0;
               sdsuNframe = (uint32) nframe;  
            }
         }
         else if ( nframe == 1 )
         {
            MESSAGE_LOG1 (MSG_LOG, 
                    "Setting up for one exposure of %f seconds", exposure);

            nframePerDataset = nframe ;

            /* BUG WORK AROUND */
            obsId->continuous = FALSE;
            obsId->totalFrames = nframe;
            sdsuNframe = (uint32) nframe;  /* Modif 23 sept 1999 - cb */
         }
         else
         {
            MESSAGE_LOG2 (MSG_LOG, 
            "Setting up for %ld exposures of %f seconds each", 
            nframe, exposure);

            nframePerDataset = 1 ;

            /* BUG WORK AROUND */
            if ( sdsuId->readMethod == 1)
            {
               obsId->continuous = FALSE;
               obsId->totalFrames = nframe;
               sdsuNframe = (uint32) 0;  /* Modif 25 oct 1999 - cb */
            }
            else
            {
               obsId->continuous = FALSE;
               obsId->totalFrames = nframe;
               sdsuNframe = (uint32) 1;  
            }
         }

         /* Set the number of frames by writing to the T_NFRAME parameter in the
          * timing DSP Also define the total number of frames in the observation
          * context structure. */

#ifdef DEBUG
         printf ("detExposure: Setting T_NFRAME parameter to %lu\n", 
                 sdsuNframe);
#endif /* DEBUG */

         if ( sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_NFRAME", sdsuNframe ) 
              == ERROR )
         {
            ERROR_LOG ("Error setting number of frames parameter");
            errorNumber = S_detControl_SDSU_ERROR;
         }

         /* Set the exposure time by writing to the T_EXP_TIM parameter in 
          * the timing DSP 
          */

         expTim = (uint32) (exposure / SDSU_EXPOSURE_UNIT);

#ifdef DEBUG
         printf ("detExposure: Setting T_EXP_TIM parameter to %lu\n", expTim);
#endif /* DEBUG */

         if ( sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_EXP_TIM", expTim ) 
              == ERROR )
         {
            ERROR_LOG ("Error setting exposure time parameter");
            errorNumber = S_detControl_SDSU_ERROR;
         }

         /* Update the requested total exposure time in the observation context 
          * structure. 
          */

         obsId->exposedRQ = 1 * exposure;
         sdsuId->exposureTicks = (int) (exposure * sysClkRateGet());

         /* 
          * Set up the the total integration time requested 
          */

         if (epToVxPipeWrite( NULL, (char *)(int)(&obsId->exposedRQ), 
                              obsId->pExposedRQContext ) == ERROR)
         {
            ERROR_LOG ("Failed to init total int time requested SIR record");
         }

         /*
          * Set up the observation mode context
          */

         if ( obsId->continuous == TRUE )
         {
            if (epToVxPipeWrite( NULL, "MOVIE", obsId->pObsModeContext ) 
                == ERROR)
            {
               ERROR_LOG ("Failed to set observation mode SIR record");
            }
         }
         else
         {
            if (epToVxPipeWrite( NULL, "STARE", obsId->pObsModeContext ) 
                == ERROR)
            {
               ERROR_LOG ("Failed to set observation mode SIR record");
            }
         }

         /*
          * Set up the requested and actual number of exposure/dataset.
          * Always 1 for the moment
          */

         nexp = 1 ;
         if (epToVxPipeWrite( NULL, (char *) &nexp, obsId->pNExpRQContext ) 
             == ERROR)
         {
            ERROR_LOG ("Failed to init Number exp/dataset SIR record");
         }
         if (epToVxPipeWrite( NULL, (char *) &nexp, obsId->pNExpContext ) 
             == ERROR)
         {
            ERROR_LOG ("Failed to init Number exp/dataset SIR record");
         }

         /* 
          * Set up the the number of frames per dataset 
          */

         if (epToVxPipeWrite( NULL, (char *)(int)&nframePerDataset, 
                              obsId->pNFramesContext ) == ERROR)
         {
            ERROR_LOG ("Failed to set number of frames SIR record");
         }

         /*
          * BUG WORK AROUND: Before attempting to query parameters from the
          * timing board, send an ABT command to the VME board. This should
          * ensure the board is not in a state where it thinks it is still
          * waiting for data from a previous observation. The parameter reads
          * from the timing board will fail in this circumstance.
          */

         if (sdsuPrimitive (sdsuId, "ABT", SDSU_IDENT_VME, NULL, NULL) == ERROR)
         {
            ERROR_LOG ("ABT command failed prior to starting observation");
         }

         if (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_OUTPUTS", 
                            &(obsId->outputsNb)) == ERROR)
         {
            ERROR_LOG (
            "Failed to query number of outputs from SDSU controller");
            MESSAGE_LOG1 (MSG_WARNING, "Assuming number of outputs is %d", 
                          defOutputs);
            /*obsId->outputs = defOutputs;*/
         }

         if (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_EXP_TIM", &expTim) 
             == ERROR)
         {
            ERROR_LOG ("Failed to query exposure time from SDSU controller");
            MESSAGE_LOG (MSG_WARNING, 
                    "Assuming exposure time is 1 second per frame");
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
            "Zero exposure time obtained from SDSU controller. Assuming min");
            if (obsId->totalFrames > 0)
            {
               obsId->exposed = 
               (double) obsId->totalFrames * (double) SDSU_EXPOSURE_UNIT;
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
            sdsuId->exposureTicks = 
            (int) (expTim * SDSU_EXPOSURE_UNIT * sysClkRateGet());
         }
      }

      /* 
       * Set up the the total integration time  
       */

      if (epToVxPipeWrite( NULL, (char *)(int)(&obsId->exposed), 
                           obsId->pExposedContext ) == ERROR)
      {
         ERROR_LOG ("Failed to init total integration time SIR record");
      }

      /* Get a timestamp to record the time at which the observation started. */

      if ( timeNow (&(obsId->rawtStart)) != OK )
      {
         ERROR_SET (0, "Failed to get time stamp at observation start", 
                    ERROR_LOG_NOW);
      }

#ifdef DEBUG
      printf ("detObserveStart: Time at observation start: %f seconds.\n", 
              obsId->rawtStart);
#endif

      /*
       * Start the readout process. The observation should now start in 
       * a parallel thread.
       */

      sdsuId->frameTimeout = (int) (obsId->expTime + 60) * sysClkRateGet();

      MESSAGE_LOG2 (MSG_MINDEBUG, 
         "Starting exposure of %f seconds in %d frames...",
         obsId->exposed, obsId->totalFrames);
      if ( obsId->totalFrames > 1 )
      {
         if (sdsuSimpleReadoutStart (sdsuId, 0, (void *) obsId) 
             == ERROR)
         {
            ERROR_LOG ("Failed to start simple readout process");
            obsId->observing = FALSE;
            observingState = CAR_ERROR;
            if (epToVxPipeWrite (NULL, (char *) &observingState, 
                                 obsId->pDetObservingContext) == ERROR)
            {
               ERROR_LOG ("Also failed to set observing state to ERROR.");
            }
            errorNumber = S_detControl_SDSU_ERROR;
            return (errorNumber);
         }
      }
      else
      {
         if (sdsuSimpleReadoutStart (sdsuId, obsId->totalFrames, (void *) obsId) 
             == ERROR)
         {
            ERROR_LOG ("Failed to start simple readout process");
            obsId->observing = FALSE;
            observingState = CAR_ERROR;
            if (epToVxPipeWrite (NULL, (char *) &observingState, 
                                 obsId->pDetObservingContext) == ERROR)
            {
               ERROR_LOG ("Also failed to set observing state to ERROR.");
            }
            errorNumber = S_detControl_SDSU_ERROR;
            return (errorNumber);
         }
      }

      /*
       * To maximise the efficiency, the following code runs in parallel with
       * the observation. If the observation happens to finish before this code
       * completes (which is unlikely) it will wait for the binary semaphore
       * which is given at the end of this function.
       */

      /*
       * Start an alarm timer which will trigger if the frame sync callback
       * never runs. Set the delay time to the readout timeout plus the largest
       * frame exposure time obtained earlier.
       *
       * THE TIMEOUT IS NOW ONLY USED IN SIMULATION MODE - SMB 21 JAN 99
       */

      if ( sdsuId->simulate )
      {
         if ( obsId->exposed >= obsId->exposedRQ )
         {
            if ( obsId->totalFrames > 0 )
            {
               waitTimeSecs = 
               readoutTimeout + (obsId->exposed / (double) obsId->totalFrames);
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
               waitTimeSecs = 
               readoutTimeout + (obsId->exposedRQ / (double)obsId->totalFrames);
            }
            else
            {
               waitTimeSecs = readoutTimeout + obsId->exposedRQ;
            }
         }

         /* BUG WORK AROUND (FOR INTERRUPT VERSION). 
          * Set the frame wait timeout. - SMB 16 Jan 99 
          */


         if ( timeoutAlarmSet (obsId->timeId, waitTimeSecs, 
              detObserveTimeout, (int) obsId) == ERROR )
         {
            ERROR_SET (0, "Failed to set alarm timer", ERROR_LOG_NOW);
            obsId->observing = FALSE;
            observingState = CAR_ERROR;
            if (epToVxPipeWrite (NULL, (char *) &observingState, 
                                 obsId->pDetObservingContext)
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
       * This time will be used to generate the MJD-OBS field in the 
       * FITS header.
       *
       * There is currently no internationally agreed standard defining the 
       * timescale for MJD-OBS. TAI is used here because it is a sensible choice
       * and, in fact, was once specified in a draft standard in July 1996 that
       * was subsequently withdrawn.
       * Whatever timescale is specified here, it is important that it be
       * continuous across a leap second. Suitable alternatives are
       * Terrestrial Time (TT) and Universal Time 1 (UT1). UTC is NOT suitable.
       *
       * NOTE: At time of writing P. Wallace is checking this with the FITS 
       * committee.
       */

      if (timeThenD (obsId->rawtStart, TAI, &timeTAI) != OK)
      {
         ERROR_SET (0,  "Failed to convert time stamp to TAI", ERROR_LOG_NOW);
      }

      /*
       * Get the current tracking frame, as read from the TCS.
       * (Default values will be supplied if the TCS is not available).
       */

      wfsGetTrackFrame (&trackFrame, &(trackEquinox.type), 
         &(trackEquinox.year), &trackWavelength,
         &trackRA, &trackDec, &(trackEpoch.type), &(trackEpoch.year));
      obsId->equinox = trackEquinox.year;
      obsId->epoch   = trackEpoch.year;
      obsId->RA      = trackRA;
      obsId->Dec     = trackDec;

      /*
       * If sufficient WCS calibration points are available, define the WCS
       * information for this observation.
       */

      if ( obsId->nWcsPoints >= 3 )
      {
         /*
          * Define the (i,j) to (X,Y) transformation.
          * N.B. For efficiency, this need only be done once, each time the
          * detector binning is changed. CHANGE THIS EVENTUALLY.
          *
          * First check if any binning or windowing of the pixels on the
          * detector has been defined.
          */

         if ( obsId->fullImageFlag == FALSE )
         {
            /*
             * There has been some binning and windowing. The original 
             * calibration is assumed to have been made on a full frame 
             * of data without binning, so the calibration
             * points need to be transformed.
             */

            for (p=0; p<obsId->nWcsPoints; p++)
            {
               obsId->detij[p][0] =
               ((obsId->pixij[p][0] - 0.5 - 
                 (double) (obsId->x1 - 1)) /
                (double) obsId->xBin) + 0.5;

               obsId->detij[p][1] =
               ((obsId->pixij[p][1] - 0.5 - 
                 (double) (obsId->y1 - 1)) /
                (double) obsId->yBin) + 0.5;
            }

            /*
             * Calculate the best fit to the focal plane X,Y coordinates
             * against binned detector coordinates.
             */

            wcsStatus = 
            astFitij ( obsId->nWcsPoints, obsId->fpxy, obsId->detij, obsId->cij,
                       &pixis, &pixjs, &perp, &orient );
         }
         else
         {
            /*
             * No windowing or binning have been used.
             * Calculate the best fit to the focal plane X,Y coordinates
             * against the original full frame, unbinned pixel coordinates.
             */

            wcsStatus = 
            astFitij ( obsId->nWcsPoints, obsId->fpxy, obsId->pixij, obsId->cij,
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
         printf ("Cij matrix contains %f %f %f %f %f %f\n", obsId->cij[0], 
                 obsId->cij[1],
                 obsId->cij[2], obsId->cij[3], obsId->cij[4], obsId->cij[5]);
#endif

         /*
          * Obtain the current TCS context from the locally stored copy.
          * This assumes that a TCS context has been obtained elsewhere and
          * stored using astSetCtx(), as described in section 3 of document
          * tcs_ptw_008.
          */

         if ( wcsStatus == 0 )
         {
            wcsStatus = astGetctx (&ctx);
            if (wcsStatus != 0)
            {
               ERROR_SET1 (S_detControl_WCS_ERROR, 
                  "Failed to get current WCS context. Status=%d",
                  ERROR_LOG_NOW, wcsStatus);
            }
         }

         /*
          * Set the chop state to which the WCS coordinate information refers.
          * NOTE: THE ACTUAL CHOP STATE NEEDS TO BE OBTAINED FROM THE PARAMETER
          * GIVEN TO THE "SET CHOP STATE" COMMAND.
          */

         chopState = 0;         /* 0 means chop state A. */

         /*
          * Extract the current focal plane to sky WCS transformation from 
          * the TCS context.
          */

         if ( wcsStatus == 0 )
         {
            wcsStatus = 
            astCtx2tr (ctx, trackFrame, trackEquinox, trackWavelength,
                       chopState, &wcs, &rawTimeWcs);
            if (wcsStatus != 0)
            {
               ERROR_SET1 (S_detControl_WCS_ERROR,
               "Failed to get focal plane to sky WCS transformation from TCS. Status=%d",
               ERROR_LOG_NOW, wcsStatus);
            }
         }

#ifdef DEBUG
         printf (
         "WCS information extracted from TCS context is valid at time %f\n",
         rawTimeWcs);
#endif

         /*
          * Combine the (i,j) to (x,y) model, cij, and (x,y) to (RA,Dec) model,
          * wcs, into a single (i,j) to (RA,Dec) model, wcsij.
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
           * Calculate the WCS header values, expressed in terms of
           * standard FITS header items.
           */

         if ( wcsStatus == 0 )
         {
            wcsStatus = astFITSv (wcsij, trackFrame, trackEquinox, timeTAI,
               obsId->ctype1, &(obsId->crpix1), &(obsId->crval1),
               obsId->ctype2, &(obsId->crpix2), &(obsId->crval2),
               &(obsId->cd1_1), &(obsId->cd1_2), &(obsId->cd2_1), 
               &(obsId->cd2_2), obsId->radecsys, &(obsId->equinox), 
               &(obsId->mjdobs));
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
          * There is insufficient information to provide WCS information.
          * The only valid item which can be added to the header is the 
          * MJD of the observation.
          */

         MESSAGE_LOG (MSG_WARNING,
            "No WCS calibration - there will be no WCS header");

         obsId->mjdobs = timeTAI;
         obsId->wcsStatus = -1;
      }

      /*
       * Convert the time stamps from Gemini raw time into Universal Time
       * and construct these into character strings.
       */

      if (timeThenC( obsId->rawtStart, UT1, 3, obsId->timeArrayStart ) != OK)
      {
         ERROR_SET (0,
            "Failed to convert time stamp at observation start to date/time",
            ERROR_LOG_NOW);
      }
      sprintf (obsId->utStartString, "%04d-%02d-%02d:%02d:%02d:%02d.%03d",
               obsId->timeArrayStart[0], obsId->timeArrayStart[1], obsId->timeArrayStart[2],
               obsId->timeArrayStart[3], obsId->timeArrayStart[4], obsId->timeArrayStart[5],
               obsId->timeArrayStart[6]);

      if (epToVxPipeWrite( NULL, (char *)obsId->utStartString, obsId->pUTstartContext ) == ERROR)
      {
         ERROR_LOG ("Failed to set UT at start of observation SIR record");
      }

#ifdef DEBUG
      printf ( "obsId->utStartString = %s\n" , obsId->utStartString ) ;
#endif


      /*
       * If the DHS is being used then create a dataset to hold the 
       * unscrambled data. Otherwise allocate a buffer directly.
       */

      if ( obsId->outOptions == 1 )
      {

         MESSAGE_LOG (MSG_MINDEBUG, "Creating DHS dataset...");

         /* Set up for the quick look */

         contrib[0] = pDetDhsClientName; 
                                /* a global variable, set in detDhsInit */

         qlStreams[0] = calloc ( EPICS_MAX_BYTES_STRING_ATTRIB+1, sizeof (char) ) ;

         if ( strcmp (obsId->pQlStream , "" ) == 0 )
            strcpy ( qlStreams[0] , "hrwfsScience") ;
         else
         {
            strncpy ( qlStreams[0] , obsId->pQlStream , EPICS_MAX_BYTES_STRING_ATTRIB ) ;
         }

         printf ( "obsId->pQlStream=%s\n" , obsId->pQlStream ) ;
         printf ( "qlStreams[0]=%s\n" , qlStreams[0] ) ;

         /*qlStreams[0] = "hrwfsScience"; */

         wfsGetTelName ( telName ) ;
         printf ( "telName=%s\n" , telName ) ;

         /* NOTE: Lifetime should be definable
          * PERMANENT for permanent data (e.g. calibrations)
          * TRANSIENT for display only (e.g. AC in continuous mode)
          * (see ICD 3).
          */

         /*if ( obsId->totalFrames == 1 )*/            /* only one exposure */
         /*{*/
         if ( dhsOutOptions == 0 )
         {
            dhsBdCtl(detDhsConnection, DHS_BD_CTL_LIFETIME, 
                     obsId->pDataLabel, DHS_BD_LT_PERMANENT, &dhsErrno);
            MESSAGE_LOG1 (MSG_MINDEBUG,
               "LIFETIME of DHS frame %s set to DHS_BD_LT_PERMANENT", 
               obsId->pDataLabel);
            MESSAGE_LOG1 (MSG_MINDEBUG,
               "Total Frames is %d", obsId->totalFrames);
         }
         else           /* either continuous mode with totalFrames = 0 or > 1 */
         {
            dhsBdCtl(detDhsConnection, DHS_BD_CTL_LIFETIME, 
                     obsId->pDataLabel, DHS_BD_LT_TRANSIENT, &dhsErrno);
            MESSAGE_LOG1 (MSG_MINDEBUG,
               "LIFETIME of DHS frame %s set to DHS_BD_LT_TRANSIENT", 
               obsId->pDataLabel);
            MESSAGE_LOG1 (MSG_MINDEBUG,
               "Total Frames is %d", obsId->totalFrames);
         }
         CHECK_DHS (dhsErrno);
         dhsBdCtl(detDhsConnection, DHS_BD_CTL_CONTRIB, 
            obsId->pDataLabel, 1, contrib, &dhsErrno);
         CHECK_DHS (dhsErrno);
         dhsBdCtl(detDhsConnection, DHS_BD_CTL_QLSTREAM, 
                  obsId->pDataLabel, 1, qlStreams, &dhsErrno);
         CHECK_DHS (dhsErrno);

         free ( qlStreams[0] ) ;

         /* Create the DHS dataset and add the default attributes. */

         obsId->dhsDataset = dhsBdDsNew (&dhsErrno);
         CHECK_DHS (dhsErrno);

         if (dhsErrno == DHS_S_SUCCESS)
         {
            dhsBdAttribAdd (obsId->dhsDataset, "instrument", 
               DHS_DT_STRING, 0, NULL, pDetDhsClientName, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataset, "telescope", DHS_DT_STRING, 
                            0, NULL, telName, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataset, "observatory", DHS_DT_STRING, 
                            0, NULL, telName, &dhsErrno);
            CHECK_DHS (dhsErrno);
         }

         if (dhsErrno != DHS_S_SUCCESS)
         {
            ERROR_SET1 (S_detControl_DHS_ERROR, 
                        "Failed to create dataset (dhsErrno=%d)",
                        ERROR_LOG_NOW, dhsErrno);
            errorNumber = S_detControl_DHS_ERROR;
            return (errorNumber);
         }

         /* Create a frame to contain the data and add the frame header info. */

         dims[0] = 2;
         axisSize[0] = obsId->xPixelsDhs;  
         axisSize[1] = obsId->yPixelsDhs;
         origin[0] = 1;  
         origin[1] = 1;

         dhsErrno = DHS_S_SUCCESS;
         obsId->dhsDataFrame = 
         dhsBdFrameNew (obsId->dhsDataset, "dataArray", 0, DHS_DT_UINT16,
                        2, axisSize,
                        (const void **) &(obsId->pDispFrame), &dhsErrno);
         CHECK_DHS (dhsErrno);

         if (dhsErrno == DHS_S_SUCCESS)
         {
            dhsBdAttribAdd (obsId->dhsDataFrame, "dataType", 
                            DHS_DT_STRING, 0, NULL, "Intensity",
                            &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "bunit", DHS_DT_STRING, 0, 
                            NULL, DET_BUNIT, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "units", DHS_DT_STRING, 0, 
                            NULL, DET_BUNIT, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "origin", DHS_DT_INT32, 1, 
                            dims, origin, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "axisSize", DHS_DT_INT32, 1, 
                            dims, axisSize, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "axisLabel", DHS_DT_STRING, 1,
                            dims, axisLabel, &dhsErrno);
            CHECK_DHS (dhsErrno);

            bzero=(double)(32768.0) ;
            dhsBdAttribAdd (obsId->dhsDataFrame, "bzero", DHS_DT_DOUBLE, 0,
                            NULL, bzero, &dhsErrno);
            CHECK_DHS (dhsErrno);

            dhsBdAttribAdd (obsId->dhsDataFrame, "obstype", DHS_DT_STRING, 0, 
                            NULL, obsId->pObsType, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "exptime", DHS_DT_DOUBLE, 0, 
                            NULL, obsId->expTime, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "darktime", DHS_DT_DOUBLE, 0, 
                            NULL, obsId->expTime, &dhsErrno);
            CHECK_DHS (dhsErrno);

            /* WCS attributes */

            if ( wcsStatus == 0 )
            {
               dhsBdAttribAdd (obsId->dhsDataFrame, "ctype1", DHS_DT_STRING, 
                               0, NULL, obsId->ctype1, &dhsErrno);
               CHECK_DHS (dhsErrno);
               crpix1Float = (float)(obsId->crpix1);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CRPIX1", DHS_DT_FLOAT, 
                               0, NULL, crpix1Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CRVAL1", DHS_DT_DOUBLE, 
                               0, NULL, obsId->crval1, &dhsErrno);
               CHECK_DHS (dhsErrno);
               dhsBdAttribAdd (obsId->dhsDataFrame, "ctype2", DHS_DT_STRING, 
                               0, NULL, obsId->ctype2, &dhsErrno);
               CHECK_DHS (dhsErrno);
               crpix2Float = (float)(obsId->crpix2);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CRPIX2", DHS_DT_FLOAT, 
                               0, NULL, crpix2Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CRVAL2", DHS_DT_DOUBLE, 
                               0, NULL, obsId->crval2, &dhsErrno);
               CHECK_DHS (dhsErrno);
               cd1_1Float = (float)(obsId->cd1_1);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CD1_1", DHS_DT_FLOAT, 
                               0, NULL, cd1_1Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               cd1_2Float = (float)(obsId->cd1_2);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CD1_2", DHS_DT_FLOAT, 
                               0, NULL, cd1_2Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               cd2_1Float = (float)(obsId->cd2_1);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CD2_1", DHS_DT_FLOAT, 
                               0, NULL, cd2_1Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
               cd2_2Float = (float)(obsId->cd2_2);
               dhsBdAttribAdd (obsId->dhsDataFrame, "CD2_2", DHS_DT_FLOAT, 
                               0, NULL, cd2_2Float, &dhsErrno);
               CHECK_DHS (dhsErrno);
            }

            sprintf ( raString , "%f" , obsId->RA ) ;
            sprintf ( decString , "%f" , obsId->Dec ) ;

            dhsBdAttribAdd (obsId->dhsDataFrame, "RA", DHS_DT_STRING, 0, NULL,
                            raString, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "DEC", DHS_DT_STRING, 0, NULL,
                            decString, &dhsErrno);
            CHECK_DHS (dhsErrno);

            dhsBdAttribAdd (obsId->dhsDataFrame, "equinox", DHS_DT_DOUBLE, 0, 
                            NULL, obsId->equinox, &dhsErrno);
            CHECK_DHS (dhsErrno);

            dhsBdAttribAdd (obsId->dhsDataFrame, "epoch", DHS_DT_DOUBLE, 0, 
                            NULL, obsId->epoch, &dhsErrno);
            CHECK_DHS (dhsErrno);

            dhsBdAttribAdd (obsId->dhsDataFrame, "mjd-obs", DHS_DT_DOUBLE, 
                            0, NULL, obsId->mjdobs, &dhsErrno);
            CHECK_DHS (dhsErrno);

            /* ADD MORE DATA FRAME HEADER ITEMS HERE. */

            dhsBdAttribAdd (obsId->dhsDataFrame, "xbin", DHS_DT_INT32, 
                            0, NULL, obsId->xBin, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "ybin", DHS_DT_INT32, 
                            0, NULL, obsId->yBin, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "datasec", DHS_DT_STRING, 
                            0, NULL, obsId->dataSec, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "ccdsec", DHS_DT_STRING, 
                            0, NULL, obsId->ccdSec, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "origsec", DHS_DT_STRING, 
                            0, NULL, obsId->origSec, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "utstart", DHS_DT_STRING, 
                            0, NULL, obsId->utStartString, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "dettype", DHS_DT_STRING, 
                            0, NULL, obsId->detType, &dhsErrno);
            CHECK_DHS (dhsErrno);
            dhsBdAttribAdd (obsId->dhsDataFrame, "detid", DHS_DT_STRING, 
                            0, NULL, obsId->detId, &dhsErrno);
            CHECK_DHS (dhsErrno);

         }

         if (dhsErrno != DHS_S_SUCCESS)
         {
            ERROR_SET1 (S_detControl_DHS_ERROR, 
                        "Failed to create new data frame (dhsErrno=%d)",
                        ERROR_LOG_NOW, dhsErrno);
            errorNumber = S_detControl_DHS_ERROR;
            return (errorNumber);
         }

         /* Create a frame in case of windowing */

         if ( obsId->windowingFlag == TRUE )
         {
            nPixels = obsId->xPixels * obsId->yPixels;

            obsId->pCurFrame = (uint16 *) malloc (nPixels * sizeof(uint16));
            if ( obsId->pCurFrame == NULL )
            {
               ERROR_LOG( "Failed to allocate image buffer for unscrambled data" );
               errorNumber = S_detControl_INTERNAL;
               return (errorNumber);
            }
            /*printf ( "DHS option : malloc pCurFrame=%p\n" , obsId->pCurFrame ) ;*/
         }
         else
         {
            obsId->pCurFrame = obsId->pDispFrame ;
            /*printf ( "DHS option : pDispFrame and pCurFrame=%p\n" , obsId->pCurFrame ) ;*/
         }
      }
      else
      {
         /* The DHS is not being used. */

         /* Calculate the number of pixels and reserve a buffer for 
          * the unscrambled data. 
          */

         nPixels = obsId->xPixels * obsId->yPixels;
         nPixelsDhs = obsId->xPixelsDhs * obsId->yPixelsDhs;

#ifdef DEBUG
         printf ("detObserveStart: Allocating frame buffer to hold %d pixels "
                 "of unscrambled data.\n", nPixels);
#endif /* DEBUG */

         obsId->pDispFrame = (uint16 *) malloc (nPixelsDhs * sizeof(uint16));
         if ( obsId->pDispFrame == NULL )
         {
            ERROR_LOG( "Failed to allocate image buffer to display data" );
            errorNumber = S_detControl_INTERNAL;
            return (errorNumber);
         }

         /*printf ( "File option : malloc pDispFrame=%p\n" , obsId->pDispFrame ) ;*/
         if ( obsId->windowingFlag == TRUE )
         {
            obsId->pCurFrame = (uint16 *) malloc (nPixels * sizeof(uint16));
            if ( obsId->pCurFrame == NULL )
            {
               ERROR_LOG( "Failed to allocate image buffer for unscrambled data" );
               errorNumber = S_detControl_INTERNAL;
               return (errorNumber);
            }
            /*printf ( "File option : malloc pCurFrame=%p\n" , obsId->pCurFrame ) ;*/
         }
         else
         {
            obsId->pCurFrame = obsId->pDispFrame ;
            /*printf ( "DHS option : pDispFrame and pCurFrame=%p\n" , obsId->pCurFrame ) ;*/
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

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detObserveEnd
 *
 *   INVOCATION:
 *   detObserveEnd (sdsuId, obsIdIn, pRawFrame)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) sdsuId    (SDSU_ID) Controller ID
 *   (>) obsIdIn   (void *)  Pointer to observation definition, cast to void *
 *   (>) pRawFrame (SDSU_FRAME *) Pointer to image frame
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Complete observation
 *
 *   DESCRIPTION:
 *   This function ends an observation.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   pDetDhsClientName   (char *)   Name of DHS client = Instrument name
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   UNFINISHED - DHS AND WCS CODE NEEDS FINISHING.
 *
 *   This routine does not check the frame status bits to determine if there
 *   was an error during the data transfer over the fibre.
 *   ANJ - 27 August 1998.
 *
 *   BUGS:
 *   Resource freeing functions such as
 *   dhsBdDsFree should free their resources regardless of the value of 
 *   dhsErrno, since they might be called to tidy up after an error.
 *-
 */

void detObserveEnd
   (
   SDSU_ID        sdsuId,         /* SDSU ID                                  */
   void *         obsIdIn,        /* Pointer to observation ID cast to void * */
   SDSU_FRAME *   pRawFrame       /* Incoming Image frame                     */
   )
{
   /* Variables describing the observation. */

   OBS_ID         obsId;          /* Pointer to observation ID structure.     */

   /* DHS variables (see dhstests.c) */

   DHS_STATUS     dhsErrno;       /* DHS error number.                        */
   DHS_STATUS     dummyDhsErrno;  /* DHS error number used for freeing        */
                                  /* resources.                               */
   DHS_TAG        putTag;         /* DHS data transfer tag.                   */

   /* File names. */

   char         pFileNameString[ (EPICS_MAX_BYTES_STRING_ATTRIB+1)*2 + 10];
                                /* String containing file name.               */

   /* SDSU parameters. */

   uint32       frameCount;        /* SDSU frame counter.                     */
   BOOL         bufferReserved;    /* TRUE if the SDSU frame buffer been      */
                                   /* reserved.                               */
   BOOL         obsAlreadyAborted; /* TRUE if observation already  aborted.   */

   double       readoutTimeout;    /* Readout timeout in seconds.             */
   double       waitTimeSecs;      /* Wait time in seconds.                   */

   /* 
    * Variables associated with "observe" command.
    * (Label, datapath and filename use general filename parameters)
    */

   long         observingState;      /* Observation status (busy or idle).    */

   /*
    * Other variables 
    */

   double       elapsed;

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
      ERROR_SET (S_detControl_INTERNAL, "NULL raw frame pointer", 
                 ERROR_LOG_NOW);
      return;
   }

   /* Convert the observation ID pointer provided as an argument. */

   obsId = (OBS_ID) obsIdIn;

   /*
    * Initialise the DHS error number.
    */

   dhsErrno = DHS_S_SUCCESS;         /* <---- DHS error number is reset here. */

   /*
    * This function should only be called when an observation is in progress.
    */

   if ( !obsId->observing )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation not in progress", 
                 ERROR_LOG_NOW);
      return;
   }

   /*
    * Cancel any observation timer. Failing to cancel this is not a serious
    * error. THIS IS NOW ONLY DONE IN SIMULATION MODE.
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
      ERROR_SET (0, "Failed to get time stamp at observation end", 
                 ERROR_LOG_NOW);
   }

#ifdef DEBUG
   printf ("detObserveEnd: Time at observation end: %f seconds.\n", 
           obsId->rawtEnd);
#endif

   /*
    * Compute the elapsed time
    */

   elapsed = obsId->rawtEnd - obsId->rawtStart ;

   /*
    * Get the frame countdown counter attached to the data and increment
    * the frame counter.
    */

   frameCount = pRawFrame->header.frameCount;
   obsId->nframes++;

   /* BUG WORK AROUND: THE SDSU CONTROLLER REPORTS FRAME COUNT=1 WHEN AN
    * INFINITE NUMBER OF FRAMES ARE BEING RETURNED. IF THE CONTROLLER IS
    * RUNNING IN CONTINUOUS MODE, POKE THE FRAME COUNT WITH ZERO.
    * (REMOVE WHEN SDSU DSP CODE IS FIXED).
    */

   if ( obsId->continuous ) frameCount = 0;

   if ( obsId->totalFrames > 1 ) frameCount = 0; /* MODIF 25 oct */

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
         MESSAGE_LOG2 (MSG_MINDEBUG, 
            "... exposure complete. Frame count=%d (%ld remaining)",
            obsId->nframes, (frameCount-1));
      }
      else if ( frameCount == 1 )
      {
         MESSAGE_LOG1 (MSG_MINDEBUG, 
            "... exposure complete. Frame count=%d (last frame)",
            obsId->nframes);
      }
      else
      {
         MESSAGE_LOG1 (MSG_MINDEBUG, 
            "... exposure complete. Frame count=%d (continuous)",
            obsId->nframes);
      }
   }

   /*
    * If this is the first frame, wait for the binary semaphore indicating
    * that the code executed at the start of the the observation has
    * completed. (This will only matter for very short observations).
    *
    * If an error occurs jump to the ERROR_EXIT at the end of this function.
    * I do not like "goto" statements but they seem to be necessary in this case
    * where the function is void and I cannot use "return (ERROR)" and have
    * the caller set the "observing" flag. The alternative to the "goto" would
    * be to fill the rest of the function with "if (!error)" tests, which
    * would be even more incomprehensible.
    * SMB - 11 December 1998.
    */

   if ( obsId->nframes <= 1 )
   {
#ifdef DEBUG
      printf ("detObserveEnd: Waiting for observation sync semaphore...");
#endif
      if ( semTake ( obsId->syncSem, OBS_WAIT_TIMEOUT ) == ERROR )
      {
         ERROR_SET (0, "Failed to take observation synchronisation semaphore", 
                 ERROR_LOG_NOW);
         goto ERROR_EXIT;
      }
#ifdef DEBUG
      printf (" ... got observation sync semaphore...\n");
#endif
   }

   /*
    * Check the status of the frame just received and only process the data
    * if the frame has been received successfully.
    */

   if ( sdsuId->fatal )
   {
      ERROR_SET1 (0, "Fatal error at frame %d - observation abandoned",
                  ERROR_LOG_NOW, (int)obsId->nframes);
      goto ERROR_EXIT;
   }
   else if ( pRawFrame->header.status != 0 )
   {
      if ((pRawFrame->header.status & SDSU_FSTAT_TIMEOUT) != 0)
      {
         MESSAGE_LOG1 (MSG_WARNING, 
         "Timeout in frame %d - frame ignored", (int)obsId->nframes);
      }
      else if ((pRawFrame->header.status & SDSU_FSTAT_OVERRUN) != 0)
      {
         MESSAGE_LOG1 (MSG_WARNING, 
         "Data overrun in frame %d - frame ignored", (int)obsId->nframes);
      }
      else if ((pRawFrame->header.status & SDSU_FSTAT_FRAMESYNC) != 0)
      {
         MESSAGE_LOG1 (MSG_WARNING, 
         "Sync error in frame %d - ignored", (int)obsId->nframes);
      }
      else if ((pRawFrame->header.status & SDSU_FSTAT_CHECKSUM) != 0)
      {
         MESSAGE_LOG1 (MSG_WARNING, 
         "Checksum error in frame %d - ignored", (int)obsId->nframes);
      }
      else if ((pRawFrame->header.status & SDSU_FSTAT_NOK) != 0)
      {
         MESSAGE_LOG1 (MSG_WARNING,
         "Overwritten error in frame %d - ignored", (int)frameCount);
      }
   }
   else
   {
      /* Update the dhs counter */

      obsId->dhsCounter ++ ;

      /*
       * Unscramble the data. The algorithm used depends on the number of
       * detector outputs, obtained earlier.
       */

#ifdef DEBUG
      /* ADD 23 SEPT */
      if ( flagFirstTime == FALSE )  
      {
         readTime1 = tickGet () ;
         /*printf ( "flagFirstTime = FALSE, readTime1 = %d\n" , readTime1 ) ;*/
      }
      else
      {
         /*printf ( "flagFirstTime = TRUE \n" ) ;*/
         if ( flagSecondTime == FALSE )
         {
            readTime2 = tickGet () ;
            /*printf ( "flagSecondTime = FALSE, readTime2 = %d\n" , readTime2 ) ;*/
         }
      }
#endif

/*
      if ( detFrameUnscrambleUint16( obsId->xPixels, obsId->yPixels, 
                                     (int) obsId->outputsNb,
                                     pRawFrame, obsId->pCurFrame ) == ERROR )
*/
      if ( newDetFrameUnscrambleUint16( obsId->xPixels, obsId->yPixels, 
                                        (int) obsId->oscanNb,
                                        pRawFrame, obsId->pCurFrame ) == ERROR )
      {
         ERROR_LOG ("Failed to unscramble data");
         if ( obsId->outOptions == 1 )
         {
            dummyDhsErrno = DHS_S_SUCCESS;   /* Fudge around bad DHS feature. */
            dhsBdDsFree ( obsId->dhsDataset, &dummyDhsErrno );
            if ( obsId->windowingFlag == TRUE )
            {
               free (obsId->pCurFrame);
               obsId->pCurFrame = NULL;
            }
         }
         else
         {
            free (obsId->pDispFrame);
            obsId->pDispFrame = NULL;
            if ( obsId->windowingFlag == TRUE )
            {
               free (obsId->pCurFrame);
               obsId->pCurFrame = NULL;
            }
         }
         goto ERROR_EXIT;
      }

      if ( obsId->windowingFlag == TRUE )
      {
         if ( detFrameReduceUint16 ( obsId ) == ERROR )
         {
            ERROR_LOG ("Failed to reduce data");
            if ( obsId->outOptions == 1 )
            {
               dummyDhsErrno = DHS_S_SUCCESS;   /* Fudge around bad DHS feature. */
               dhsBdDsFree ( obsId->dhsDataset, &dummyDhsErrno );
               free (obsId->pCurFrame); /* windowingFlag = TRUE */
               obsId->pCurFrame = NULL;
            }
            else
            {
               free (obsId->pDispFrame);
               obsId->pDispFrame = NULL;
               free (obsId->pCurFrame); /* windowingFlag = TRUE */
               obsId->pCurFrame = NULL;
            }
            goto ERROR_EXIT;
         }
      }

#ifdef DEBUG
      /* ADD 23 SEPT */
      if ( flagFirstTime == FALSE ) 
      {
         unscrambleTime1 = tickGet () ;
         /*printf ( "flagFirstTime = FALSE, unscrambleTime1 = %d\n" , 
                  unscrambleTime1 ) ;*/
      }
      else
      {
         /*printf ( "flagFirstTime = TRUE \n" ) ;*/
         if ( flagSecondTime == FALSE )
         {
            unscrambleTime2 = tickGet () ;
            /*printf ( "flagSecondTime = FALSE, unscrambleTime2 = %d\n" , 
                      unscrambleTime2 ) ;*/
         }
      }
#endif

      /*
       * Convert the time stamps from Gemini raw time into Universal Time
       * and construct these into character strings.
       */

      if (timeThenC( obsId->rawtEnd, UT1, 3, obsId->timeArrayEnd ) != OK)
      {
         ERROR_SET (0,
         "Failed to convert time stamp at observation end to date/time",
         ERROR_LOG_NOW);
      }
         
      sprintf (obsId->utEndString, "%04d-%02d-%02d:%02d:%02d:%02d.%03d",
        obsId->timeArrayEnd[0], obsId->timeArrayEnd[1], obsId->timeArrayEnd[2],
        obsId->timeArrayEnd[3], obsId->timeArrayEnd[4], obsId->timeArrayEnd[5],
        obsId->timeArrayEnd[6]);

      if (epToVxPipeWrite( NULL, (char *)obsId->utEndString, 
                           obsId->pUTendContext ) == ERROR)
      {
         ERROR_LOG ("Failed to set UT at end of observation SIR record");
      }

      if (epToVxPipeWrite( NULL, (char *)(int)&elapsed, 
                           obsId->pElapsedContext ) == ERROR)
      {
         ERROR_LOG ("Failed to set elapsed time SIR record");
      }

      /*
       * Send the data to the DHS, store it to disk or do nothing, 
       * as appropriate
       */

      obsId->outNFrames ++ ;

      if ( (obsId->outOptions == 1) && 
           ((obsId->dhsCounter % obsId->dhsQlRate) == 0) )
      {
         MESSAGE_LOG (MSG_MINDEBUG, "Sending data to DHS...");

         if ( obsId->totalFrames == 1 )
         {
            dhsBdAttribAdd (obsId->dhsDataFrame, "utend", DHS_DT_STRING,
                            0, NULL, obsId->utEndString, &dhsErrno);
            CHECK_DHS (dhsErrno);
         }

#ifdef DEBUG
         dhsBdDsPrint (obsId->dhsDataset, &dhsErrno);
         CHECK_DHS (dhsErrno);
#endif /* DEBUG */

         /* Send the data to the dhs */

#ifdef DEBUG
       printf (
       "detObserveEnd: dhsBdPut, dhsConnection=%d, pDataLabel=%s, dataset=%d\n",
       (int) detDhsConnection, obsId->pDataLabel, (int) obsId->dhsDataset);
#endif /* DEBUG */

         if ( obsId->dhsOutOptions == 2 ) /* QL only */
         {
            if ( obsId->totalFrames == 1 )
            {
               putTag = 
               dhsBdPut (detDhsConnection, obsId->pDataLabel, 
               DHS_BD_PT_DS_QL, DHS_TRUE, obsId->dhsDataset, NULL, &dhsErrno);
            }
            else
            {
               putTag = 
               dhsBdPut (detDhsConnection, obsId->pDataLabel, 
                         DHS_BD_PT_DS_QL, DHS_FALSE, obsId->dhsDataset, NULL, 
                         &dhsErrno);
            }
         }
         else
         {
            if ( obsId->totalFrames == 1 )
            {
               putTag = 
               dhsBdPut (detDhsConnection, obsId->pDataLabel, 
               DHS_BD_PT_DS, DHS_TRUE, obsId->dhsDataset, NULL, &dhsErrno);
            }
            else
            {
               putTag = 
               dhsBdPut (detDhsConnection, obsId->pDataLabel, 
                         DHS_BD_PT_DS, DHS_FALSE, obsId->dhsDataset, NULL, 
                         &dhsErrno);
            }
         }
         CHECK_DHS (dhsErrno);

         if (dhsErrno != DHS_S_SUCCESS)
         {
            ERROR_SET1 (S_detControl_DHS_ERROR, 
                        "Failed to initiate data transfer (dhsErrno=%d)",
                        ERROR_LOG_NOW, dhsErrno);
            dummyDhsErrno = DHS_S_SUCCESS;   /* Fudge around bad DHS feature. */
            dhsTagFree (putTag, &dummyDhsErrno);
            CHECK_DHS (dummyDhsErrno); 
            dummyDhsErrno = DHS_S_SUCCESS;   /* Fudge around bad DHS feature. */
            dhsBdDsFree (obsId->dhsDataset, &dummyDhsErrno);
            CHECK_DHS (dummyDhsErrno); 
            if ( obsId->windowingFlag == TRUE )
            {
               free (obsId->pCurFrame);
               obsId->pCurFrame = NULL;
            }
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
            ERROR_SET1 (S_detControl_DHS_ERROR, 
                        "Error during wait for data transfer (dhsErrno=%d)",
                        ERROR_LOG_NOW, dhsErrno);

            dummyDhsErrno = DHS_S_SUCCESS;   /* Fudge around bad DHS feature. */
            dhsTagFree (putTag, &dummyDhsErrno);
            CHECK_DHS (dummyDhsErrno);
            dummyDhsErrno = DHS_S_SUCCESS;   /* Fudge around bad DHS feature. */
            dhsBdDsFree (obsId->dhsDataset, &dummyDhsErrno);
            CHECK_DHS (dummyDhsErrno); 
            if ( obsId->windowingFlag == TRUE )
            {
               free (obsId->pCurFrame);
               obsId->pCurFrame = NULL;
            }
            goto ERROR_EXIT;
         }

         if ( detDhsCheckCmdStatus (putTag) == ERROR )
         {
            ERROR_SET (S_detControl_DHS_ERROR, "Data transfer failed", 
                 ERROR_LOG_NOW);

            dummyDhsErrno = DHS_S_SUCCESS;   /* Fudge around bad DHS feature. */
            dhsTagFree (putTag, &dummyDhsErrno);
            CHECK_DHS (dummyDhsErrno);
            dummyDhsErrno = DHS_S_SUCCESS;   /* Fudge around bad DHS feature. */
            dhsBdDsFree (obsId->dhsDataset, &dummyDhsErrno);  
            CHECK_DHS (dummyDhsErrno);
            if ( obsId->windowingFlag == TRUE )
            {
               free (obsId->pCurFrame);
               obsId->pCurFrame = NULL;
            }
            goto ERROR_EXIT;
         }

         /*
          * Free the DHS tag.
          */

         dhsErrno = DHS_S_SUCCESS;         /* Fudge around bad DHS feature. */
         dhsTagFree (putTag, &dhsErrno);
         CHECK_DHS (dhsErrno);

         /*
          * If the last frame has been received free the DHS dataset.
          */

         if ( (frameCount == 1) || (obsId->stopped) )
         {
            /*dhsErrno = DHS_S_SUCCESS; */   /* Fudge around bad DHS feature. */
            /*dhsTagFree (putTag, &dhsErrno);
            CHECK_DHS (dhsErrno);*/
            dhsBdDsFree (obsId->dhsDataset, &dhsErrno); 
            CHECK_DHS (dhsErrno);
            if ( obsId->windowingFlag == TRUE )
            {
               free (obsId->pCurFrame);
               obsId->pCurFrame = NULL;
            }
         }
#ifdef DEBUG
         /* ADD 23 SEPT */
         if ( flagFirstTime == FALSE ) 
         {
            dhsTime1 = tickGet () ;
            flagFirstTime = TRUE ;
         /*   printf ( "flagFirstTime = FALSE -> TRUE, dhsTime1 = %d\n" , 
                     dhsTime1 ) ;*/
         }
         else
         {
            /*printf ( "flagFirstTime = TRUE\n" ) ;*/
            if ( flagSecondTime == FALSE )
            {
               dhsTime2 = tickGet () ;
               flagSecondTime = TRUE ;
               /*printf ( "flagSecondTime = FALSE -> TRUE, dhsTime2 = %d\n" , 
                     dhsTime2 ) ;*/
            }
         }
#endif
      }
      else if ( obsId->outOptions == 2 )
      {
         /*
          * The DHS is not being used and the data will be saved to FITS files.
          * If this is the first frame of the observation the standard names
          * will be used.
          * Frames 2 onwards have .2, .3, etc... appended to the names.
          */

         /* Save the unscrambled data to a FITS file. */

         if ( obsId->totalFrames != 1 )
         {
            sprintf( pFileNameString, "%s.%d.fits", obsId->pOutFileName, 
                     obsId->outNFrames );
         }
         else 
         {
            sprintf( pFileNameString, "%s.fits", obsId->pOutFileName) ; 
         }

         MESSAGE_LOG2 (MSG_MINDEBUG, 
         "Saving unscrambled data from %p to directly to file \"%s\"...",
         obsId->pDispFrame, pFileNameString);

         if (detWriteFitsUint16 (pFileNameString, obsId, obsId->xPixelsDhs, 
                                 obsId->yPixelsDhs, obsId->pDispFrame) == ERROR)
         {
            ERROR_LOG ("Failed to write FITS file");
            free (obsId->pDispFrame);
            obsId->pDispFrame = NULL;
            if ( obsId->windowingFlag == TRUE )
            {
               free (obsId->pCurFrame);
               obsId->pCurFrame = NULL;
            }
            goto ERROR_EXIT;
         }
         MESSAGE_LOG (MSG_MINDEBUG, "... file saved ok");
      }

      /* If obsId->totalFrames > 1 and obsId->outNFrames = obsId->totalFrames */
      /* stop the observation */

#ifdef DEBUG
      printf ( "detObserveEnd : ouNFrames = %d, totalFrames = %d\n" ,
               obsId->outNFrames , obsId->totalFrames ) ;
#endif
      if ( (obsId->totalFrames > 1) && (obsId->outNFrames == obsId->totalFrames) )
         obsId->stopped = TRUE ;

   }

   /*
    * If the DHS is not being used and the last frame has been received,
    * free the unscrambled data buffer.
    */

   if ( obsId->outOptions != 1 )
   {
      if ( (frameCount == 1) || (obsId->stopped) )
      {
         free (obsId->pDispFrame);
         obsId->pDispFrame = NULL;
         if ( obsId->windowingFlag == TRUE )
         {
            free (obsId->pCurFrame);
            obsId->pCurFrame = NULL;
         }
         else
            obsId->pCurFrame = NULL;
        
      }
   }

   /*
    * Abort any further readouts if the observation was stopped prematurely.
    */

   if ( obsId->stopped )
   {
      /* add 14 oct 99 for slow stop pb */
      printf ( "detObserveEnd() -> sdsuReadoutAbort()\n" ) ;

      obsAlreadyAborted = TRUE;
      if (sdsuReadoutAbort (sdsuId) == ERROR)
      {
         ERROR_LOG ("Failed to abort readouts on receipt of STOP instruction");
         goto ERROR_EXIT;
      }
      /* add 14 oct 99 for slow stop pb */
      printf ( "detObserveEnd() -> sdsuReadoutAbort() done \n" ) ;
   }


/* NORMAL_EXIT: */

   /*
    * Release the SDSU frame buffer.
    */

   /*if ( bufferReserved )
   {
      sdsuFrameRelease (sdsuId, pRawFrame);
      bufferReserved = FALSE;
   }*/

   /*
    * If the last frame has been received, set the observing flag FALSE
    * and set the observeC CAR record to IDLE via the "observing" record.
    * Otherwise set a timeout on the receipt of the next frame.
    */

   if ( (frameCount == 1) || (obsId->stopped) )
   {
      if ( sdsuId->frameErrors <= 0 )
      {
         MESSAGE_LOG1 (MSG_LOG, 
         "Observation completed successfully, frames lost: %d", sdsuFrameLost);
#ifdef DEBUG
         /* MODIF 23 SEPT */
         printf ( "readTime1:%d, unscrambleTime1:%d, dhsTime1:%d\n" , 
                  readTime1, unscrambleTime1, dhsTime1 ) ;
         printf ( "readTime2:%d, unscrambleTime2:%d, dhsTime2:%d\n" , 
                  readTime2, unscrambleTime2, dhsTime2 ) ;
#endif
      }
      else if ( sdsuId->frameErrors < obsId->nframes )
      {
         MESSAGE_LOG2 (MSG_WARNING, 
         "Observation completed with %d frames lost and %d frames with error",
         sdsuFrameLost , sdsuId->frameErrors);
#ifdef DEBUG
         /* MODIF 23 SEPT */
         printf ( "readTime1:%d, unscrambleTime1:%d, dhsTime1:%d\n" , 
                  readTime1, unscrambleTime1, dhsTime1 ) ;
         printf ( "readTime2:%d, unscrambleTime2:%d, dhsTime2:%d\n" , 
                  readTime2, unscrambleTime2, dhsTime2 ) ;
#endif
      }
      else
      {
         ERROR_LOG ("Observation failed - all frames lost");
         goto ERROR_EXIT;
      }

      obsId->observing = FALSE;
      observingState = CAR_IDLE;
      if (epToVxPipeWrite (NULL, (char *) &observingState, 
                           obsId->pDetObservingContext) == ERROR)
      {
         ERROR_LOG ("Failed to set observing flag to IDLE");
      }
   }
   else
   {
#ifdef DEBUG
      printf ( "detObserveEnd: Further frames are anticipated - observation not finished.\n");
#endif

      /*
       * Start an alarm timer which will trigger if the frame sync callback
       * never runs.
       * Set the delay time to the readout timeout plus the largest frame
       * exposure time obtained earlier.
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
               waitTimeSecs = 
               readoutTimeout + (obsId->exposed / (double) obsId->totalFrames);
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
               waitTimeSecs = 
               readoutTimeout + (obsId->exposedRQ / (double)obsId->totalFrames);
            }
            else
            {
               waitTimeSecs = readoutTimeout + obsId->exposedRQ;
            }
         }

         if ( timeoutAlarmSet (obsId->timeId, waitTimeSecs, detObserveTimeout, 
                               (int) obsId) == ERROR )
         {
            ERROR_LOG ("Failed to set alarm timer");
         }
      }
   }

   return;


ERROR_EXIT:

   /*
    * If an error occurred, abort the observation, release the SDSU frame
    * buffer (if necessary)
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

   /* MODIF 23 SEPT */
   /*if ( bufferReserved )
   {
      sdsuFrameRelease (sdsuId, pRawFrame);
      bufferReserved = FALSE;
   }*/

   obsId->observing = FALSE;
   observingState = CAR_ERROR;
   if (epToVxPipeWrite (NULL, (char *) &observingState, 
                        obsId->pDetObservingContext) == ERROR)
   {
      ERROR_LOG ("Failed to set observing flag to ERROR");
   }

   return;
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detObserveTimeout
 *
 *   INVOCATION:
 *   detObserveTimeout (timeId, obsIdInt)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) timeId   (timer_t) Timer ID
 *   (>) obsIdInt (int)     Pointer to observation definition, cast to integer
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Handle an observation timeout.
 *
 *   DESCRIPTION:
 *   Handle the situation when a readout does not complete within the time
 *   when its exposure and readout should have finished.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   UNFINISHED
 *-
 */

void detObserveTimeout
   (
   timer_t     timeId,             /* Timer ID.                               */
   int         obsIdInt            /* Pointer to observation ID cast to int.  */
   )
{
   OBS_ID      obsId = (OBS_ID) obsIdInt;
   SDSU_ID     sdsuId = (SDSU_ID) obsId->sdsuId;

   char *      mainKeywords[] = {"NAXIS1", "NAXIS2", "SECTORS", "OSP_FSZ"};
                              /* Main FITS keywords to read from header.      */
   int         mainValues[4]; /* Values corresponding to main FITS keywords.  */

   /*
    * The following variables will be used to test additional header items in
    * a file of simulated data, but the check has not been implemented yet.
    */

   long        observingState; /* Observation status (busy or idle).          */
   int         simOption;      /* Simulation option.                          */


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
      
      pFrame->header.packetCount   = 0;
      pFrame->header.status        = 0;   /* No errors during readout */
      pFrame->header.parameterId   = 0;   /* Simulated parameter ID */
      pFrame->header.frameCount    = 1;   /* Simulate just one frame */

      if ( strcmp(obsId->pSimFileName, "NONE") == 0 )
      {
         /*
          * Simulate the data internally, writing the result to the current
          * SDSU frame. Use option 1 (a simple ramp) for large data frames and
          * option 2 (simulated Shack-Hartmann spots) for small data frames.
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

         if ( detSimulateData (obsId->xPixels, obsId->yPixels, simOption, 
                               pFrame) == ERROR )
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

         MESSAGE_LOG1 (MSG_LOG, "Reading simulated data from %s\n", 
                       obsId->pSimFileName);
         if (detReadFitsHeaderInt (obsId->pSimFileName, 3, mainKeywords, 
                                   mainValues) == ERROR )
         {
            ERROR_SET (0, "Failed to read simulated data header", 
                 ERROR_LOG_NOW);
         }

         if ( (mainValues[0] == obsId->xPixels) && 
              (mainValues[1] == obsId->yPixels) &&
              (mainValues[2] == obsId->outputsNb)
            )
         {

            /*
             * The file is acceptable. Now read its contents.
             */

            MESSAGE_LOG (MSG_MINDEBUG, "Simulated data header looks OK");
            if (detReadFitsImageUint16 ((uint16 *)& (pFrame->pixel[0]), 
                                        obsId->pSimFileName,
                                        (obsId->xPixels)*(obsId->yPixels))
                == ERROR )
            {
               ERROR_SET (0, "Failed to read simulated data", ERROR_LOG_NOW);
            }
         }
         else
         {
            /*
             * The simulated data contained in the file does not match the
             * simulated data required.
             */

            ERROR_SET4 (S_detControl_BAD_FILE,
            "Required size is %d x %d, simulated data file contains %d x %d",
            ERROR_LOG_SAVE, obsId->xPixels, obsId->yPixels,
            mainValues[0], mainValues[1]);
            ERROR_SET2 (0, 
            "%ld detector outputs are required, simulated data file assumes %d",
            ERROR_LOG_SAVE, obsId->outputsNb, mainValues[2]);
            ERROR_LOG ("Mismatch between simulated data file and requirements");
         }
      }

      /* Simulate the packet count reaching the desired value. */

      pFrame->header.packetCount   = sdsuId->packetsPerFrame;

      /*
       * Simulate an SDSU frame sync interrupt. This should cause the
       * detObserveEnd callback to be executed.
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
       * Assume the data are available in the buffer and simulate an 
       * SDSU frame sync interrupt.
       * This should cause the detObserveEnd callback to be executed.
       */

      MESSAGE_LOG (MSG_MINDEBUG, 
      "Observation time completed with interrupts disabled");
   
      if ( sdsuSimulateSimpleSync(sdsuId) == ERROR)
      {
         ERROR_LOG ("Failed to simulate frame sync interrupt");
      }
   }
   else
   {
      /* sysIntDisable(6); */                     /* DEBUG TEST */

      /*
       * The observation completion was supposed to have been signalled by an
       * interrupt and timed out.
       * Set the observing flag FALSE and set the observeC CAR record to ERROR,
       * via the "observing" record.
       */

      MESSAGE_LOG (MSG_WARNING, 
      "Observation timed out - trying to read data anyway...");

      obsId->observing = FALSE;
      observingState = CAR_ERROR;
      if (epToVxPipeWrite (NULL, (char *) &observingState, 
          obsId->pDetObservingContext) == ERROR)
      {
         ERROR_LOG ("Failed to set observing flag to ERROR");
      }

      /* Try and simulate a frame sync interrupt to force a data readout. 
         This may or may not work. 
       */

      if ( sdsuSimulateSimpleSync (sdsuId) == ERROR)
      {
         ERROR_LOG ("Failed to simulate frame sync interrupt");
      }
   }
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detStop
 *
 *   INVOCATION:
 *   detStop (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, 
 *            obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record Name Prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Current observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detStop command
 *
 *   DESCRIPTION:
 *   This function stops an observation.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

uint32 detStop
   (
   const char *      pWfsName,       /* Name of wavefront sensor.             */
   const char *      pRecordPrefix,  /* Record Name Prefix.                   */
   CAD_CMD_CONTEXT   cadCmdContext,  /* CAD command context structure.        */
   int               commandNumber,  /* Command number.                       */
   SDSU_ID           sdsuId,         /* SDSU context structure.               */
   OBS_ID            obsId           /* Observation context structure.        */
   )
{
   uint32         errorNumber;      /* Error number reported by task.         */

   /*
    * Initialise the error number.
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * This function should only be called when an observation is in progress.
    */

   if ( !obsId->observing )
   {
      /*ERROR_SET (S_detControl_INTERNAL, "Observation not in progress", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;*/

      MESSAGE_LOG (MSG_LOG, "Observation not in progress" );
      errorNumber = 0;

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

   /* add 14 oct 1999 for slow stop pb */
   printf ( "detStop(): obsId->stopped=TRUE\n" );

   return (errorNumber);
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detAbort
 *
 *   INVOCATION:
 *   detAbort (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, 
 *             obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record Name Prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Current observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detAbort command
 *
 *   DESCRIPTION:
 *   This function aborts an observation. It will send an abort to the SDSU
 *   controller even if an observation appears not to be taking place.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detAbort
   (
   const char *      pWfsName,      /* Name of wavefront sensor.              */
   const char *      pRecordPrefix, /* Record Name Prefix.                    */
   CAD_CMD_CONTEXT   cadCmdContext, /* CAD command context structure.         */
   int               commandNumber, /* Command number.                        */
   SDSU_ID           sdsuId,        /* SDSU context structure.                */
   OBS_ID            obsId          /* Observation context structure.         */
   )
{
   uint32         errorNumber;      /* Error number reported by task.         */

   int            observingState;   /* Observation status (busy or idle).     */

   /*
    * Initialise the error number.
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
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
      MESSAGE_LOG (MSG_WARNING, 
      "WARNING: Observation not in progress but attempting to abort anyway");
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
    * Reset the "observation in progress" flag and set the observeC CAR record 
    * to IDLE, via the "observing" record.
    */

   if ( obsId->observing )
   {

      obsId->observing = FALSE;
      observingState = CAR_IDLE;
      if (epToVxPipeWrite (NULL, (char *) &observingState, 
                           obsId->pDetObservingContext) == ERROR)
      {
         ERROR_LOG ("Failed to set observing flag to IDLE.");
      }
   }

   return (errorNumber);
}



/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detInit
 *
 *   INVOCATION:
 *   detInit (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, pSdsuId, 
 *            obsId, pVmeAddress, pmaxFrames)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record Name Prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (!) pSdsuId       (SDSU_ID *)       Pointer to current SDSU context
 *                                       structure
 *   (<) obsId         (OBS_ID)          Observation context structure
 *   (!) pVmeAddress   (uint32 *)        Pointer to VME address of SDSU
 *                                       controller
 *   (!) pMaxFrames    (int *)           Pointer to max frames in data buffer
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detInit command
 *
 *   DESCRIPTION:
 *   This function initialises the SDSU controller and redownloads the DSP code.
 *
 *   EXTERNAL VARIABLES:
 *   (<) detSdsuIdHr (SDSU_ID) SDSU context structure for HRWFS
 *
 *   PRIOR REQUIREMENTS:
 *   The VME address supplied must have been previously verified to be 
 *   correct (see below).
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   Memory problems can arise if an attempt is made to initialise the
 *   controller at an invalid address. The address should be verified to be
 *   correct before attempting to initialise the controller.
 *   Note that a zero address is used to flag simulation mode, and is
 *   therefore acceptable.
 *-
 */

uint32 detInit
   (
   const char *    pWfsName,         /* Name of wavefront sensor.             */
   const char *    pRecordPrefix,    /* Record Name Prefix.                   */
   CAD_CMD_CONTEXT cadCmdContext,    /* CAD command context structure.        */
   int             commandNumber,    /* Command number.                       */
   SDSU_ID *       pSdsuId,          /* Pointer to SDSU context structure.    */
   OBS_ID          obsId,            /* Observation context structure.        */
   uint32 *        pVmeAddress,      /* Pointer to VME address of SDSU        */
                                     /* controller.                           */
   int *           pMaxFrames        /* Maximum number of frames in buffer.   */
   )
{
   uint32       errorNumber;      /* Error number reported by task.           */
   long         initState;        /* Initialisation state.                    */
   long         simulate;         /* TRUE if SDSU interface is simulated.     */

   char         pStatusString [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Status string.                               */
   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for file.                          */
   char         pOmfFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* File name.                                   */
   char         pFullOmfFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                              /* Combined path name and file name.            */
   BOOL         limitAdrsRange;  /* Flag for limiting address range in DSP    */
                                 /* memory                                    */
   int          nPixels;         /* Total number of digitised pixels.         */
   int          newMaxFrames;    /* New maximum number of frames.             */

   uint32       mode;

   long         i;               /* index                                     */
   long         offsetVect[2];   /* ADC offset vector                         */
   long         offsetFullVect[2];
                                 /* ADC offset vector - no binning            */
   long         offsetBinVect[2];/* ADC offset vector - binning               */
   uint32       tempCode;        /* Target temperature code                   */
   uint32       tempCoeff;       /* Coefficient for temperature control       */
   char         defFileName [ STRING_SIZE ] ;
                                 /* Default file name according to the site   */
   char         detContInitFileName [ STRING_SIZE ] ;
                                 /* Full Name of the detector controller      */
                                 /* init file                                 */
   
   /*
    * Initialise the error number.
    */

   errorNumber = 0;

   /*
    * If the controller does not currently have control of the hardware try and
    * get it. A failure to gain access to the hardware is not regarded as an
    * error, since if the hardware is being shared it is normal for one of the
    * controllers not to have access (and all the controllers may be being
    * initialised at the same time). If a failure occurs just issue a warning.
    */

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( (obsId != NULL) && (obsId->observing) )
   {
      ERROR_SET (S_detControl_BUSY,
          "Observation in progress - abort observation and try again", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /* Set to FALSE the temperature Flag */

   readTempReadyFlag = FALSE;

   /* Set the initialisation state to BUSY. */

   if (epToVxPipeWrite( NULL, "INITIALIZING", obsId->pStateContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set INITIALIZING state");
   }

   initState = CAR_BUSY;
   if (epToVxPipeWrite (NULL, (char *) &initState, obsId->pDetInitContext) 
       == ERROR)
   {
      ERROR_LOG ("Failed to set initialisation state to BUSY");
   }

   /*
    * Obtain the VME address of the SDSU controller.
    * An address of zero signifies simulation mode.
    */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, 
                          (char *) pVmeAddress);

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
    * Now attempt to create a new context structure, remembering to call 
    * sdsuReset() immediately after sdsuContextCreate().
    */

   *pSdsuId = sdsuContextCreate (*pVmeAddress, simulate);
   if ( (*pSdsuId  == NULL) ||
        (sdsuReset (*pSdsuId, SDSU_RESET_VME | SDSU_RESET_CONTROLLER) == ERROR)
      )
   {
      if ( simulate )
      {
         ERROR_SET1 (0, 
            "WFS %s: Error initialising SDSU controller in simulation mode",
            ERROR_LOG_NOW, pWfsName);
      }
      else
      {
         ERROR_SET2 (0, 
            "WFS %s: Error initialising SDSU controller at VME address %#lx",
            ERROR_LOG_NOW, pWfsName, *pVmeAddress);
      }
      errorNumber = S_detControl_SDSU_ERROR;

      /* Set the initialisation state to ERROR. */

      initState = CAR_ERROR;
      if (epToVxPipeWrite (NULL, (char *) &initState, obsId->pDetInitContext) 
          == ERROR)
      {
         ERROR_LOG ("Failed to set initialisation state to ERROR");
      }

      if (epToVxPipeWrite( NULL, "RUNNING", obsId->pStateContext ) == ERROR)
      {
         ERROR_LOG ("Failed to set RUNNING state");
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

      MESSAGE_LOG1 (MSG_LOG, 
          "SDSU ID structure at %#x initialised successfully", (int) *pSdsuId);

      /* Write a new string to the SDSU initialisation status SIR record. */

      if ( simulate )
      {
         sprintf (pStatusString, "SDSU SIMULATED: ID = %-#8x", (int) *pSdsuId);
      }
      else
      {
         sprintf (pStatusString, 
                  "SDSU Initialised OK: ID = %-#8x", (int) *pSdsuId);
      }

      if (epToVxPipeWrite (NULL, pStatusString, obsId->pDetInitStatusContext) 
          == ERROR)
      {
         ERROR_LOG ("Failed to write init message to SDSU status pipe.");
      }

      /* Update the global variables used to remember the SDSU contexts, as an
       * aid to engineering.
       */

      /* pWfsName = "hr" */

      (*pSdsuId)->fastCamera = FALSE;
      detSdsuIdHr = *pSdsuId;
   }

   /* Now obtain the path of the directory containing the DSP code. */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pFilePath);

   /* Determine whether any code should be downloaded to the VME DSP */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pOmfFileName);

   if ( (strcmp (pOmfFileName, "") != 0) && 
        (strcmp (pOmfFileName, "NONE") != 0) )
   {

      /*
       * The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      if ( strcmp (pFilePath, "") == 0 )
      {
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to VME DSP...",
         pFullOmfFileName);

      if (sdsuFileDnload (*pSdsuId, pFullOmfFileName, SDSU_IDENT_VME, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download OMF file to VME DSP");
         errorNumber = S_detControl_SDSU_ERROR;

         /* Set the initialisation state to ERROR. */

         initState = CAR_ERROR;
         if (epToVxPipeWrite (NULL, (char *) &initState, obsId->pDetInitContext)
             == ERROR)
         {
            ERROR_LOG ("Failed to set initialisation state to ERROR");
         }

         /*
          * If an OMF file could not be downloaded the SDSU controller is in a
          * state where it can only obey a subset of the commands and cannot
          * make observations, so set the health to WARNING.
          */

         epToVxSetHealth( pRecordPrefix, "WARNING" );
      }
   }

   /* Determine whether any code should be downloaded to the Timing DSP */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, pOmfFileName);

   if ( (strcmp (pOmfFileName, "") != 0) && 
        (strcmp (pOmfFileName, "NONE") != 0) )
   {

      /*
       * The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;
      if ( strcmp (pFilePath, "") == 0 )
      {
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to TIMING DSP...", 
                    pFullOmfFileName);

      if (sdsuFileDnload (*pSdsuId, pFullOmfFileName, SDSU_IDENT_TIM, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download OMF file to TIMING DSP");
         errorNumber = S_detControl_SDSU_ERROR;

         /*
          * If an OMF file could not be downloaded the SDSU controller is in
          * a state where it can only obey a subset of the commands and cannot
          * make observations, so set the health to WARNING.
          */

         epToVxSetHealth( pRecordPrefix, "WARNING" );
      }
   }

   /* Determine whether any code should be downloaded to the Utility DSP */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, pOmfFileName);

   if ( (strcmp (pOmfFileName, "") != 0) && 
        (strcmp (pOmfFileName, "NONE") != 0) )
   {

      /* The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      if ( strcmp (pFilePath, "") == 0 )
      {
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to UTILITY DSP...", 
                    pFullOmfFileName);

      if (sdsuFileDnload (*pSdsuId, pFullOmfFileName, SDSU_IDENT_UTL, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download OMF file to UTILITY DSP");
         errorNumber = S_detControl_SDSU_ERROR;

         /*
          * If an OMF file could not be downloaded the SDSU controller is in
          * a state where it can only obey a subset of the commands and cannot
          * make observations, so set the health to WARNING.
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
            if (sdsuParamWrite (*pSdsuId, SDSU_IDENT_TIM, "T_MODE", mode) 
                == ERROR)
            {
               ERROR_LOG ("Failed to clear sync mode on timing board");
            }
         }
      }
   }

   /*
    * After successfully downloading new OMF code, the controller must be
    * reinitialised by sending an "INI" command to the utility DSP and a "LDP"
    * command to the timing DSP. If this fails the controller may not be usable,
    * so the health must be set to WARNING.
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

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, 
                          (char *) &newMaxFrames);

   if ( newMaxFrames > 0 ) *pMaxFrames = newMaxFrames;

   /*
    * Compare the default detector geometry contained in the DSP code with the
    * current values for xPixels and yPixels in obsId. After calling 
    * detCheckGeometry, xMax and yMax in obsId should contain the maximum 
    * possible data array size, allowing a data buffer of suitable size to 
    * be allocated.
    */

   if (detCheckGeometry (pWfsName, *pSdsuId, obsId) == ERROR)
   {
      ERROR_LOG ("Error while checking default detector geometry");
   /* This is not a serious error. Do not change the error number or health. */
   }

   /*
    * Allocate a buffer capable of holding several frames of data, using the
    * *pxMax and *pyMax determined above. If this fails, the controller will
    * not be able to store data, so the health must be set WARNING.
    */

#ifdef DEBUG
   printf (
   "detInit: Creating new data buffer to hold %d frames of (%d x %d) pixels.\n",
   *pMaxFrames, obsId->xMax, obsId->yMax);
#endif /* DEBUG */

   nPixels = (obsId->xMax) * (obsId->yMax);
   if (sdsuBufferCreate (*pSdsuId, nPixels, *pMaxFrames) == ERROR)
   {
      ERROR_LOG ("Failed to create frame data buffers on initialisation");
      errorNumber = S_detControl_SDSU_ERROR;
      epToVxSetHealth( pRecordPrefix, "WARNING" );
   }
   
   /*
    * Next we initialise the readout process with our frame callback.
    * If this fails the SDSU controller will be unable to readout data, 
    * so the health must be set to WARNING.
    *
    * There is no packet callback in this version of the code.
    * INTERRUPTS DISABLED. SIMPLE VERSION. HRWFS RUNS AT LOWER PRIORITY.
    */
   
#ifdef DEBUG
   printf ("detInit: Starting the readout task and frame sync callback.\n");
#endif /* DEBUG */

   if (sdsuSimpleReadoutOpen (*pSdsuId, NULL, detObserveEnd, 1, TRUE) == ERROR)
   {
      ERROR_LOG ("Failed to start readout task on initialisation");
      errorNumber = S_detControl_SDSU_ERROR;
      epToVxSetHealth( pRecordPrefix, "WARNING" );
   }

   /*
    * Read the default settings from the detector controller init file
    */

#if (MK)
   strcpy ( defFileName, DET_CONTROL_HRWFS_MK_INIT_FILE);
#else
   strcpy ( defFileName, DET_CONTROL_HRWFS_CP_INIT_FILE);
#endif

   printf ( "defFileName =%s\n", defFileName);

   if ( strcmp (defFileName, "NONE") != 0 )
   {
      strcpy ( detContInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
      strcat ( detContInitFileName , "/" ) ;
      strcat ( detContInitFileName , defFileName ) ;

      if ( detContInit ( detContInitFileName, &tempCode, &tempCoeff,
                         offsetFullVect, offsetBinVect, obsId->detId) == ERROR )
      {
         MESSAGE_LOG ( MSG_LOG,
           "Failed to init detector controller default settings from file");

         /* Set the temperature to -20.0C anyway and ADC offsets to 2560
            which is default value */

         tempCode = (uint32)1282 ;
         tempCoeff = (uint32)128 ;
         for ( i = 0 ; i < obsId->outputsNb ; i ++ )
             offsetVect[i] = 2560;
         strcpy ( obsId->detId , DET_CCD_SN ) ;
      }
      else
      {
         if ( obsId->binningFlag == FALSE )
         {
            for ( i = 0 ; i < obsId->outputsNb ; i ++ )
                offsetVect[i] = offsetFullVect[i];
         }
         else
         {
            for ( i = 0 ; i < obsId->outputsNb ; i ++ )
                offsetVect[i] = offsetBinVect[i];
         }
      }
   }
   else
   {
      /* Set the temperature to -20.0C anyway and ADC offsets to 2560
         which is default value */

      tempCode = (uint32)1282 ;
      tempCoeff = (uint32)128 ;
      for ( i = 0 ; i < obsId->outputsNb ; i ++ )
          offsetVect[i] = 2560;
      strcpy ( obsId->detId , DET_CCD_SN ) ;
   }

   /*
    * Write the CCD serial number to the corresponding SIR record
    */

   if (epToVxPipeWrite( NULL, obsId->detId, obsId->pDetIdContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set default detector type");
      return (ERROR);
   }

   /* 
    * Set the default offsets for HRWFS
    */

   MESSAGE_LOG2 (MSG_LOG, "Defining new ADC offset levels: %#lx %#lx",
                 offsetVect[0], offsetVect[1]);

   if ( sdsuParamWRP (*pSdsuId, SDSU_IDENT_TIM, "T_ADC_OS0",
                      (uint32) offsetVect[0] ) == ERROR )
   {
      ERROR_LOG ("Error setting ADC offset 0 parameter");
   }

   if ( sdsuParamWRP (*pSdsuId, SDSU_IDENT_TIM, "T_ADC_OS1",
                      (uint32) offsetVect[1] ) == ERROR )
   {
      ERROR_LOG ("Error setting ADC offset 1 parameter");
   }
   if (sdsuPrimitive (*pSdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
   {
      ERROR_LOG (
      "Failed to activate TIMING DSP parameters with LDP command");
   }

   /*
    * Update the adc sir records
    */

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[0]) ,
                        obsId->pAdc0Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc0 sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[1]) ,
                        obsId->pAdc1Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc1 sad record");
      return (ERROR);
   }

   /* 
    * Set the default temperature for the HRWFS 
    */

   MESSAGE_LOG2 (MSG_LOG, 
                 "Defining temperature control parameters: %#lx %#lx",
                 tempCode, tempCoeff);

   if ( (sdsuParamWrite (*pSdsuId, SDSU_IDENT_UTL, "U_CCDT_TGT", tempCode )
         == ERROR) ||
        (sdsuParamWrite (*pSdsuId, SDSU_IDENT_UTL, "U_TCF", (uint32)tempCoeff )
         == ERROR) )
   {
      ERROR_LOG ("Error setting temperasture control parameters");
   }

   /*
    * If the error number is good after initialisation the health of
    * the controller can be restored to "GOOD".
    *
    * Also set the initialisation state to IDLE or ERROR, depending on the
    * error number.
    */

   if ( errorNumber == 0 )
   {
      epToVxSetHealth( pRecordPrefix, "GOOD" );

      initState = CAR_IDLE;
      if (epToVxPipeWrite (NULL, (char *) &initState, obsId->pDetInitContext) 
          == ERROR)
      {
         ERROR_LOG ("Failed to set initialisation state to IDLE");
      }

      /* Set to TRUE the temperature Flag */

      readTempReadyFlag = TRUE;

      if (epToVxPipeWrite( NULL, "RUNNING", obsId->pStateContext ) == ERROR)
      {
         ERROR_LOG ("Failed to set RUNNING state");
      }
   }
   else
   {
      initState = CAR_ERROR;
      if (epToVxPipeWrite (NULL, (char *) &initState, obsId->pDetInitContext) 
          == ERROR)
      {
         ERROR_LOG ("Failed to set initialisation state to ERROR");
      }

      if (epToVxPipeWrite( NULL, "RUNNING", obsId->pStateContext ) == ERROR)
      {
         ERROR_LOG ("Failed to set RUNNING state");
      }
   }

   return (errorNumber);
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detReset
 *
 *   INVOCATION:
 *   detReset (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, 
 *             obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record Name Prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *                                       (0=simulate)
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detReset command
 *
 *   DESCRIPTION:
 *   Reset the SDSU controller
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detReset
   (
   const char *    pWfsName,       /* Name of wavefront sensor.               */
   const char *    pRecordPrefix,  /* Record Name Prefix.                     */
   CAD_CMD_CONTEXT cadCmdContext,  /* CAD command context structure.          */
   int             commandNumber,  /* Command number.                         */
   SDSU_ID         sdsuId,         /* SDSU context structure.                 */
   OBS_ID          obsId           /* Observation context structure.          */
   )
{
   uint32       errorNumber;       /* Error number reported by task.          */
   long         resetVme;          /* Flag set to reset SDSU VME interface.   */
   long         resetCtrl;         /* Flag set to reset SDSU controller.      */

   uint32       resetMask;         /* Mask specifying what to reset.          */

   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                   /* Path name for file.                     */
   char         pOmfFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                   /* File name.                              */
   char         pFullOmfFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                                   /* Combined path name and file name.       */
   BOOL         limitAdrsRange;    /* Flag for limiting address range         */

   uint32       mode;

   long         i;                 /* index                                   */
   long         offsetFullVect[2]; /* ADC offset vector - no binning.         */
   long         offsetBinVect[2];  /* ADC offset vector - binning.            */
   long         offsetVect[2];     /* ADC offset vector                       */
   char         defFileName [ STRING_SIZE ] ;
                                   /* Default file name according to the site */
   char         detContInitFileName [ STRING_SIZE ] ;
                                   /* Full Name of the detector controller    */
                                   /* init file                               */
   uint32       tempCode;          /* Target temperature code                 */
   uint32       tempCoeff;         /* Coefficient for temperature control     */

   /*
    * Initialise the error number and obtain the attributes provided 
    * with the command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & resetVme);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, 
                          (char *) & resetCtrl);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
                 "Observation in progress - abort observation and try again", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /* Set to FLASE the temperature Flag */

   readTempReadyFlag = FALSE ;

   /*
    * Reset the SDSU hardware.
    */

   MESSAGE_LOG2 (MSG_LOG, 
         "About to %s SDSU VME interface and %s SDSU controller",
         (resetVme ? "reset" : "NOT reset"), 
         (resetCtrl ? "reset" : "NOT reset"));

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
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to VME DSP...",
                    pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_VME, 
                          limitAdrsRange) == ERROR)
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
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, 
              "Downloading OMF file %s to TIMING DSP...", pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_TIM, 
                          limitAdrsRange) == ERROR)
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
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, 
                "Downloading OMF file %s to UTILITY DSP...", pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_UTL, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download OMF file to UTILITY DSP");
         errorNumber = S_detControl_SDSU_ERROR;
      }
   }

   if ( (resetCtrl) && (errorNumber == 0) )
   {
      /*
       * On the HRWFS the default packet size must be increased.
       * The HRWFS should not be run in sync mode to ensure adequate dynamic 
       * range.
       * (It is not a serious error if this does not happen).
       *
       * THIS BLOCK OF CODE CAN BE REMOVED WHEN TIM HARDY'S NEW DSP CODE SETS
       * APPROPRIATE DEFAULTS.
       */

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
            if (sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_MODE", mode) 
                == ERROR)
            {
               ERROR_LOG ("Failed to clear sync mode on timing board");
            }
         }
      }
   }

   /*
    * After successfully downloading new OMF code, the controller must be
    * reinitialised by sending an "INI" command to the utility DSP and a "LDP"
    * command to the timing DSP.
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

   /*
    * Set the default settings from the detector controller init file
    */

#if (MK)
   strcpy ( defFileName, DET_CONTROL_HRWFS_MK_INIT_FILE);
#else
   strcpy ( defFileName, DET_CONTROL_HRWFS_CP_INIT_FILE);
#endif

   printf ( "defFileName =%s\n", defFileName);

   if ( strcmp (defFileName, "NONE") != 0 )
   {
      strcpy ( detContInitFileName , DET_CONTROL_PAR_FILE_PATH ) ;
      strcat ( detContInitFileName , "/" ) ;
      strcat ( detContInitFileName , defFileName ) ;

      if ( detContInit ( detContInitFileName, &tempCode, &tempCoeff,
                         offsetFullVect, offsetBinVect, obsId->detId) == ERROR )
      {
         MESSAGE_LOG ( MSG_LOG,
           "Failed to init detector controller default settings from file");

         /* Set the temperature to -20.0C anyway and ADC offsets to 2560
            which is default value */

         tempCode = (uint32)1282 ;
         tempCoeff = (uint32)128 ;
         for ( i = 0 ; i < obsId->outputsNb ; i ++ )
             offsetVect[i] = 2560;
         strcpy ( obsId->detId , DET_CCD_SN ) ;
      }
      else
      {
         if ( obsId->binningFlag == FALSE )
         {
            for ( i = 0 ; i < obsId->outputsNb ; i ++ )
                offsetVect[i] = offsetFullVect[i];
         }
         else
         {
            for ( i = 0 ; i < obsId->outputsNb ; i ++ )
                offsetVect[i] = offsetBinVect[i];
         }
      }
   }
   else
   {
      /* Set the temperature to -20.0C anyway and ADC offsets to 2560
         which is default value */

      tempCode = (uint32)1282 ;
      tempCoeff = (uint32)128 ;
      for ( i = 0 ; i < obsId->outputsNb ; i ++ )
          offsetVect[i] = 2560;
      strcpy ( obsId->detId , DET_CCD_SN ) ;
   }

   /*
    * Write the CCD serial number to the corresponding SIR record
    */

   if (epToVxPipeWrite( NULL, obsId->detId, obsId->pDetIdContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set default detector type");
      return (ERROR);
   }

   /*
    * Set back the default offsets
    */

   MESSAGE_LOG2 (MSG_LOG, "Defining new ADC offset levels: %#lx %#lx",
                 offsetVect[0], offsetVect[1]);

   if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS0",
                      (uint32) offsetVect[0] ) == ERROR )
   {
      ERROR_LOG ("Error setting ADC offset 0 parameter");
   }

   if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS1",
                     (uint32) offsetVect[1] ) == ERROR )
   {
      ERROR_LOG ("Error setting ADC offset 1 parameter");
   }
   if (sdsuPrimitive (sdsuId, "LDP", SDSU_IDENT_TIM, NULL, NULL) == ERROR)
   {
      ERROR_LOG (
      "Failed to activate TIMING DSP parameters with LDP command");
   }

   /*
    * Update the adc sir records
    */

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[0]) ,
                        obsId->pAdc0Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc0 sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[1]) ,
                        obsId->pAdc1Context ) == ERROR)
   {
      ERROR_LOG ("Failed to init adc1 sad record");
      return (ERROR);
   }

   /* 
    * Set the default temperature for the HRWFS 
    */

   MESSAGE_LOG2 (MSG_LOG, 
                 "Defining temperature control parameters: %#lx %#lx",
                 tempCode, tempCoeff);

   if ( (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_CCDT_TGT", tempCode )
         == ERROR) ||
        (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_TCF", (uint32)tempCoeff )
         == ERROR) )
   {
      ERROR_LOG ("Error setting temperasture control parameters");
   }

   /*
    * If no errors have occurred during the reset set the controller health to
    * GOOD.
    * If the reset failed the controller may be in an unusable state, so set the
    * health to BAD.
    */

   if ( errorNumber == 0 )
   {
      epToVxSetHealth( pRecordPrefix, "GOOD" );

      /* Set to TRUE the temperature Flag */

      readTempReadyFlag = TRUE ;
   }
   else
   {
      epToVxSetHealth( pRecordPrefix, "BAD" );
   }

   return (errorNumber);
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detTest
 *
 *   INVOCATION:
 *   detTest (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *            sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detTest command
 *
 *   DESCRIPTION:
 *   Test the SDSU controller
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detTest
   (
   const char *    pWfsName,      /* Name of wavefront sensor.                */
   const char *    pRecordPrefix, /* Record name prefix.                      */
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */

   long            testLevel;     /* Test level.                              */
   long            verbose;       /* Flag for verbose mode.                   */
   long            testingState;  /* Testing state flag                       */

   uint32          testMask;      /* Mask for types of tests.                 */
   uint32          dspMask;       /* Mask for DSP to be tested.               */

   char         pTestResults[EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                  /* String to contain test results.          */

   /*
    * Initialise the error number, the test level and the verbose
    */

   errorNumber = 0;
   testLevel = 8;
   verbose = 1;

   /*
    * The controller can only self-test if it has access to the SDSU hardware.
    * Note having access to the hardware should not be regarded as a test
    * failure, since all controllers will be tested routinely on startup,
    * and when the hardware is shared there will always be one controller
    * without access. Instead issue a warning.
    */

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      if (epToVxPipeWrite (NULL, "Bad SDSU context", obsId->pTestResultsContext)
          == ERROR)
      {
         ERROR_LOG ("Failed to write test results");
      }
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      if (epToVxPipeWrite (NULL, "Bad observation context", 
          obsId->pTestResultsContext) == ERROR)
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
      ERROR_SET (S_detControl_BUSY, 
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /* Set the testC CAR record to BUSY */

   testingState = CAR_BUSY;
   if (epToVxPipeWrite (NULL, (char *) &testingState,
                        obsId->pTestingContext) == ERROR)
   {
      ERROR_LOG ("Failed to set testing state to BUSY.");
   }

   MESSAGE_LOG1 (MSG_LOG, "Testing SDSU controller: level=%ld", testLevel);

   /*
    * Load up the required masks and test the controller.
    * For now, all DSPs are tested in verbose mode.
    */

   testMask = 0;

   if ( testLevel > 0 )
      testMask |= SDSU_TEST_LINK;    /* Test integrity of data link to DSP.   */

   if ( testLevel > 1 )
      testMask |= SDSU_TEST_RDM;     /* Test read access to DSP memory.       */

   if ( testLevel > 2 )
      testMask |= SDSU_TEST_WRM;     /* Test write access to DSP memory.      */

   /* Other tests can be added when they are supported by sdsuTest. */

   dspMask = 0x7;                    /* Test all three DSPs (bits 0,1,2).     */

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

      if (epToVxPipeWrite (NULL, pTestResults, obsId->pTestResultsContext) 
          == ERROR)
      {
         ERROR_LOG ("Failed to write test results");
      }
      
      testingState = CAR_ERROR;
      if (epToVxPipeWrite (NULL, (char *) &testingState,
                           obsId->pTestingContext) == ERROR)
      {
         ERROR_LOG ("Also failed to set testing state to ERROR.");
      }

   }
   else
   {
      MESSAGE_LOG (MSG_LOG, "Test completed successfully");

      if (epToVxPipeWrite (NULL, "Tested OK", obsId->pTestResultsContext) 
          == ERROR)
      {
         ERROR_LOG ("Failed to write test results");
      }
      testingState = CAR_IDLE;
      if (epToVxPipeWrite (NULL, (char *) &testingState,
                           obsId->pTestingContext) == ERROR)
      {
         ERROR_LOG ("Failed to set testing state to IDLE.");
      }
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSave
 *
 *   INVOCATION:
 *   detSave (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, sdsuId, 
 *            obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detSave command
 *
 *   DESCRIPTION:
 *   This function uploads SDSU parameters to a file.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detSave
   (
   const char *    pWfsName,      /* Name of wavefront sensor.                */
   const char *    pRecordPrefix, /* Record name prefix.                      */
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32       errorNumber;      /* Error number reported by task.           */

   long         destId;           /* Destination DSP ID.                      */

   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Path name for file.                          */
   char         pParamFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                              /* Name of file to contain SDSU parameter values*/
   char         pFullParamFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                              /* Combined path name and file name.            */

   /*
    * Initialise the error number and obtain the attributes 
    * provided with the command.
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
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY, 
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /*
    * Combine the path and file names together, and append the string ".par"
    * to the file name if it is not already present. Ignore the path if not
    * specified.
    */

   if ( strcmp (pFilePath, "") == 0 )
   {
      strncpy (pFullParamFileName, pParamFileName, 
               EPICS_MAX_BYTES_STRING_ATTRIB);
   }
   else
   {
      sprintf (pFullParamFileName, "%s/%s", pFilePath, pParamFileName );
   }

   if (strstr (pFullParamFileName, ".par") == NULL)
      strncat (pFullParamFileName, ".par", EPICS_MAX_BYTES_STRING_ATTRIB);

   /*
    * Upload SDSU parameters from the specified DSP to the specified file.
    * (If the DSP is specified as "-1" all the known SDSU parameters will be
    * uploaded to the file).
    */

   MESSAGE_LOG1 (MSG_LOG, "Uploading SDSU parameters to %s", 
                 pFullParamFileName);

   if ( sdsuParamUpload( sdsuId, pFullParamFileName, (uint32) destId) == ERROR )
   {
      ERROR_LOG ("Failed to upload SDSU parameters");
      errorNumber = S_detControl_SDSU_ERROR;
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detGeometry
 *
 *   INVOCATION:
 *   detGeometry (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *                sdsuId, obsId, pOffsetFullVect, pOffsetBinVect) 
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName        (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix   (const char *)    Record name prefix
 *   (>) cadCmdContext   (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber   (int)             Command number
 *   (>) sdsuId          (SDSU_ID)         Current SDSU context structure
 *   (!) obsId           (OBS_ID)          Observation context structure
 *   (>) pOffsetFullVect (long *)          ADC offset vector - no binning
 *   (>) pOffsetBinVect  (long *)          ADC offset vector - binning
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detGeometry command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU detector geometry parameters.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   NONE
 *-
 */

uint32 detGeometry
   (
   const char *    pWfsName,        /* Name of wavefront sensor.              */
   const char *    pRecordPrefix,   /* Record name prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId,           /* Observation context structure.         */
   long *          pOffsetFullVect, /* ADC offset vector - no binning         */
   long *          pOffsetBinVect   /* ADC offset vector - binning            */
   )
{
   uint32         errorNumber;      /* Error number reported by task.         */

   /*
    * Variables associated with "Set detector readout geometry and binning
    * mode" command.
    */

   long         xReqSubap;  /* Number of subapertures per sector in X         */
                            /* direction (WFS only).                          */
   long         yReqSubap;  /* Number of subapertures per sector in Y         */
                            /* direction (WFS only).                          */
   long         xReqBin;    /* X binning factor (pixels per superpixel)       */
   long         yReqBin;    /* Y binning factor (pixels per superpixel)       */
   long         xReqRas;    /* Size of each subaperture in X direction in     */
                            /* super-pixels (wfs ONLY)                        */
   long         yReqRas;    /* Size of each subaperture in Y direction in     */
                            /* super-pixels (WFS only)                        */
   long         xReqSpace;  /* Spacing between subapertures in X direction in */
                            /* pixels (WFS only)                              */
   long         yReqSpace;  /* Spacing between subapertures in Y direction in */
                            /* pixels (WFS only)                              */
   long         xReqStart;  /* X offset from bottom left corner of array in   */
                            /* pixels.                                        */
   long         yReqStart;  /* Y offset from bottom left corner of array in   */
                            /* pixels.                                        */

   long         xReqPixels; /* Number of X pixels in digitised image (AC only)*/
   long         yReqPixels; /* Number of Y pixels in digitised image (AC only)*/
   long         xReqTail;   /* Number of trailing X pixels to be discarded on */
                            /* each row.                                      */
   long         reqPixelsNb;/* Total number of digitised pixels.              */
   long         defPixelsNb;/* Total number of digitised pixels.              */
   int          nPackets;   /* Number of packets expected per frame.          */
   long         reqOscanNb; /* Number of column of the overscan region/output */
   long         i;          /* index                                          */
   long         offsetVect[2];
                            /* ADC offset vector                              */

   /*
    * Initialise the error number and obtain the attributes provided with 
    * the command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) &xReqSubap);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) &yReqSubap);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *) &xReqRas);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *) &yReqRas);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, (char *) &xReqBin);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, (char *) &yReqBin);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6, (char *) &xReqStart);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 7, (char *) &yReqStart);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 8, (char *) &xReqSpace);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 9, (char *) &yReqSpace);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 10, 
                          (char *) &reqOscanNb);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can be used when an observation is in progress, as it only
    * redefines "on-the-fly" parameters. However, warn the user this is
    * happening.
    */

   if ( obsId->observing )
   {
      MESSAGE_LOG (MSG_WARNING,
         "NOTE: Changing on-the-fly parameters while observation in progress.");
   }

   /*
    * The number of subapertures per sector must be positive and non-zero.
    */

   if ( (xReqSubap < 1) || (yReqSubap < 1) )
   {
      ERROR_SET2 (S_detControl_BAD_ATTRIBUTE, 
         "Invalid number of subapertures: %ld x %ld",
         ERROR_LOG_NOW, xReqSubap, yReqSubap);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
   }

   /*
    * The binning factors must be 1 or 2
    */

   if ( ((xReqBin != 1) && (xReqBin != 2)) || 
        ((yReqBin != 1) && (yReqBin != 2)) )
   {
      ERROR_SET2 (S_detControl_BAD_ATTRIBUTE, 
         "Invalid binning factors: %ld, %ld",
         ERROR_LOG_NOW, xReqBin, yReqBin);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
   }

   /*
    * Calculate the number of digitized pixels per frame, and update the
    * current number of X and Y pixels. Also calculate the number of packets
    * into which these pixels will fit. (The ceil function is used because the
    * number of packets is always rounded up to the nearest integer).
    */

   xReqPixels = xReqSubap * xReqRas * obsId->outputsNb; 
                             /* xReqRas contains already the xReqBin division */
   yReqPixels = yReqSubap * yReqRas;

   reqPixelsNb = xReqPixels * yReqPixels;

   defPixelsNb = (DET_CONTROL_HRWFS_MAX_XSIZE)*(DET_CONTROL_HRWFS_MAX_YSIZE); 

   if ( reqPixelsNb > defPixelsNb )
   {
      ERROR_SET2 (S_detControl_BAD_ATTRIBUTE, 
         "Invalid total number of pixels to read : %ld, max: %ld",
         ERROR_LOG_NOW, reqPixelsNb, defPixelsNb);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
   }

   /*
    * Calculate the number of trailing X pixels. This is required by the DSP
    * code as a check. If the value is negative then the subapertures span the
    * boundary between outputs (not physically possible), and xTail should
    * be set zero.
    *
    * xTail is the number of pixels that need to be discarded at the end of
    * each row, and is calculated by starting with the total number of pixels
    * to read (xSize) and subtracting off the pixels that are read out and/or
    * discarded during a readout.
    */

   xReqTail = obsId->xSize - (((xReqRas * xReqBin) + xReqSpace) * xReqSubap) 
              + xReqSpace - xReqStart - obsId->uscanNb + xReqBin*reqOscanNb;

   if ( xReqTail < 0 )
   {
      ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, 
      "Xtail is %ld. Should not be less than zero", ERROR_LOG_NOW, xReqTail);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
      /*
      MESSAGE_LOG1 (MSG_LOG,
      "Xtail is %ld. Should not be less than zero", xReqTail);
      xReqTail = 0;
      */
   }

   if ( (!sdsuId->simulate) && (obsId->packetSize > 0) )
   {
      nPackets = (int) ceil ( (double) (reqPixelsNb) / 
                 (double) obsId->packetSize );
   }
   else
   {
      nPackets = 1;
   }

   /*
    * Everything is ok, init obsId.
    * Determine whether the given parameters will put the detector controller
    * into full frame mode. This happens when the there is one subaperture per
    * output and the subapertures fill the detector surface without any gaps.
    */

   obsId->xStart = xReqStart;
   obsId->yStart = yReqStart;
   obsId->xBin = xReqBin;
   obsId->yBin = yReqBin;
   obsId->xRaster = xReqRas;
   obsId->yRaster = yReqRas;
   obsId->xSpace = xReqSpace;
   obsId->ySpace = yReqSpace;
   obsId->xSubapNb = xReqSubap;
   obsId->ySubapNb = yReqSubap;
   obsId->xPixels = xReqPixels;
   obsId->yPixels = yReqPixels;
   obsId->pixelsNb = reqPixelsNb;
   obsId->xTail = xReqTail;
   obsId->oscanNb = reqOscanNb;

   if ( (xReqSubap == 1) && (yReqSubap == 1) && (xReqStart == 16) && (yReqStart == 1) &&
        (xReqSpace == 0) && (yReqSpace == 0)
      )
   {
      obsId->windowingFlag = FALSE;
   }
   else
   {
      obsId->windowingFlag = TRUE;
   }

   obsId->x1 = obsId->xStart - 15; /* -16 + 1 */
   obsId->x2 = obsId->xPixels;
   obsId->y1 = obsId->yStart;
   obsId->y2 = obsId->yPixels;

   if ( (xReqBin == 2) || ( yReqBin == 2) )
   {
      obsId->binningFlag = TRUE ;

      for ( i = 0 ; i < obsId->outputsNb ; i ++ )
          offsetVect[i] = pOffsetBinVect[i];
   }
   else
   {
      obsId->binningFlag = FALSE ;

      for ( i = 0 ; i < obsId->outputsNb ; i ++ )
          offsetVect[i] = pOffsetFullVect[i];
   }

   if ( (obsId->binningFlag == TRUE) || (obsId->windowingFlag == TRUE) )
      obsId->fullImageFlag = FALSE ;
   else
      obsId->fullImageFlag = TRUE ;

   if (  obsId->oscanNb != 0 )
      obsId->oscanFlag = FULL;
   else
      obsId->oscanFlag = FALSE;

#ifdef DEBUG
   /*
    * Show CCD Geometry information
    */

   printf ( "CCD Geometry information from obsId \n" ) ;
   printf ( "Outputs number : %d\n" , (int) obsId->outputsNb ) ;
   printf ( "xSize (per output) : %d\n" , obsId->xSize ) ;
   printf ( "ySize (per output) : %d\n" , obsId->ySize ) ;
   printf ( "xMax : %d\n" , obsId->xMax ) ;
   printf ( "yMax : %d\n" , obsId->yMax ) ;
   printf ( "xStart : %d\n" , obsId->xStart ) ;
   printf ( "yStart : %d\n" , obsId->yStart ) ;
   printf ( "xBin : %d\n" , obsId->xBin ) ;
   printf ( "yBin : %d\n" , obsId->yBin ) ;
   printf ( "xRaster : %d\n" , obsId->xRaster ) ;
   printf ( "yRaster : %d\n" , obsId->yRaster ) ;
   printf ( "xSpace : %d\n" , obsId->xSpace ) ;
   printf ( "ySpace : %d\n" , obsId->ySpace ) ;
   printf ( "xSubap number : %d\n" , obsId->xSubapNb ) ;
   printf ( "ySubap number : %d\n" , obsId->ySubapNb ) ;
   printf ( "xPixels : %d\n" , obsId->xPixels ) ;
   printf ( "pixels number : %d\n" , obsId->pixelsNb ) ;
   printf ( "uscan number : %d\n" , obsId->uscanNb ) ;
   printf ( "oscan number : %d\n" , obsId->oscanNb ) ;
   if ( obsId->oscanFlag == FALSE )
      printf ( "oscanFlag : FALSE\n" ) ;
   else
   {
      if ( obsId->oscanFlag == FULL )
         printf ( "oscanFlag : FULL\n" ) ;
      else
         printf ( "oscanFlag : HALF\n" ) ;
   }
   printf ( "xTail : %d\n" , obsId->xTail ) ;
   printf ( "packet size : %d\n" , obsId->packetSize ) ;
   if ( obsId->fullImageFlag == TRUE )
      printf ( "FullImageFlag : TRUE\n" ) ;
   else
      printf ( "FullImageFlag : FALSE\n" ) ;
   if ( obsId->binningFlag == TRUE )
      printf ( "binningFlag : TRUE\n" ) ;
   else
      printf ( "binningFlag : FALSE\n" ) ;
   if ( obsId->windowingFlag == TRUE )
      printf ( "windowingFlag : TRUE\n" ) ;
   else 
      printf ( "windowingFlag : FALSE\n" ) ;
   printf ( "x1=%d, x2=%d\n" , obsId->x1, obsId->x2 ) ;
   printf ( "y1=%d, y2=%d\n" , obsId->y1, obsId->y2 ) ;
#endif

   MESSAGE_LOG1 (MSG_LOG, "Setting new detector geometry (%s frame mode)",
      (obsId->fullImageFlag ? "full":"reduced"));

   MESSAGE_LOG4 (MSG_MINDEBUG, "XSIZE=%d, YSIZE=%d, XPIXELS=%d, YPIXELS=%d",
      obsId->xSize, obsId->ySize, obsId->xPixels, obsId->yPixels);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XSUBAP=%d, YSUBAP=%d, XBIN=%d, YBIN=%d",
      obsId->xSubapNb, obsId->ySubapNb, obsId->xBin, obsId->yBin);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XRAS=%d, YRAS=%d, XSPACE=%d, YSPACE=%d",
      obsId->xRaster, obsId->yRaster, obsId->xSpace, obsId->ySpace);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XSTART=%d, YSTART=%d, XTAIL=%d, NPIXELS=%d",
      obsId->xStart, obsId->yStart, obsId->xTail, obsId->pixelsNb);

   MESSAGE_LOG2 (MSG_FULLDEBUG, 
      "Each frame will consist of %d packets of %d pixels each",
      nPackets, obsId->packetSize);

   /*
    * Update the geometry parameters in the SDSU timing DSP. These are all
    * "on-the-fly" parameters and need to be downloaded with sdsuParamWRP()
    * and activated by sending a "LDP" command.
    */

   if ( !sdsuId->simulate ) 
   {
      if ( (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSUBAP", 
                          (uint32) xReqSubap) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSUBAP", 
                          (uint32) yReqSubap) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSTART", 
                          (uint32) xReqStart) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSTART", 
                          (uint32) yReqStart) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XRAS",   
                          (uint32) xReqRas) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YRAS",   
                          (uint32) yReqRas) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSPACE", 
                          (uint32) xReqSpace) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSPACE", 
                          (uint32) yReqSpace) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XBIN",   
                          (uint32) xReqBin) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YBIN",   
                          (uint32) yReqBin) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XTAIL",  
                          (uint32) xReqTail) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_NPIXEL", 
                          (uint32) reqPixelsNb) == ERROR)
        )
      {
         ERROR_LOG ("Failed to download geometry parameters to TIMING DSP");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS0",
                        (uint32) offsetVect[0] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 0 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS1",
                         (uint32) offsetVect[1] ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 1 parameter");
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
       * Update the adc sir records
       */

      if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[0]) ,
                           obsId->pAdc0Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc0 sad record");
         return (ERROR);
      }

      if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[1]) ,
                           obsId->pAdc1Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc1 sad record");
         return (ERROR);
      }

      /*
       * Update the number of packets per frame in the SDSU context structure.
       */

      sdsuId->packetsPerFrame = nPackets;
   }

   /*
    * Init the geometry sad records
    */

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xStart) , 
                        obsId->pXstartContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xstart sad record");
      return (ERROR);
   }
   
   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->yStart) , 
                        obsId->pYstartContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ystart sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xSubapNb) , 
                        obsId->pXsubapContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xsubap sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->ySubapNb) , 
                        obsId->pYsubapContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init Ysubap sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xRaster) , 
                        obsId->pXrasterContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xraster sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->yRaster) , 
                        obsId->pYrasterContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init yraster sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xSpace) , 
                        obsId->pXspaceContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xspace sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->ySpace) , 
                        obsId->pYspaceContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init yspace sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xBin) , 
                        obsId->pXbinContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xbin sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->yBin) , 
                        obsId->pYbinContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ybin sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->oscanNb) , 
                        obsId->pOscanContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init oscan sad record");
      return (ERROR);
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detFrameSize
 *
 *   INVOCATION:
 *   detFrameSize (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *                sdsuId, obsId, pOffsetFullVect, pOffsetBinVect) 
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName        (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix   (const char *)    Record name prefix
 *   (>) cadCmdContext   (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber   (int)             Command number
 *   (>) sdsuId          (SDSU_ID)         Current SDSU context structure
 *   (!) obsId           (OBS_ID)          Observation context structure
 *   (>) pOffsetFullVect (long *)          ADC offset vector - no binning
 *   (>) pOffsetBinVect  (long *)          ADC offset vector - binning
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detFrameSize command
 *
 *   DESCRIPTION:
 *   This function modify the SDSU detector geometry parameters.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   NONE
 *-
 */

uint32 detFrameSize
   (
   const char *    pWfsName,        /* Name of wavefront sensor.              */
   const char *    pRecordPrefix,   /* Record name prefix.                    */
   CAD_CMD_CONTEXT cadCmdContext,   /* CAD command context structure.         */
   int             commandNumber,   /* Command number.                        */
   SDSU_ID         sdsuId,          /* SDSU context structure.                */
   OBS_ID          obsId,           /* Observation context structure.         */
   long *          pOffsetFullVect, /* ADC offset vector - no binning         */
   long *          pOffsetBinVect   /* ADC offset vector - binning            */
   )
{
   uint32         errorNumber;      /* Error number reported by task.         */

   /*
    * Variables associated with "Set detector readout geometry and binning
    * mode" command.
    */

   long         binFlag;
   long         winFlag;
   long         reqX;
   long         reqY;
   long         reqXWidth;
   long         reqYWidth;
   long         reqX1;
   long         reqX2;
   long         reqY1;
   long         reqY2;
   long         reqOscanNb;
   long         max;
   long         maxOutput;
   long         size;
   long         size1;
   long         size2;

   long         xReqPixels; /* Number of X pixels in digitised image (AC only)*/
   long         yReqPixels; /* Number of Y pixels in digitised image (AC only)*/
   long         xReqSubap;  /* Number of subapertures per sector in X         */
                            /* direction (WFS only).                          */
   long         yReqSubap;  /* Number of subapertures per sector in Y         */
                            /* direction (WFS only).                          */
   long         xReqBin;    /* X binning factor (pixels per superpixel)       */
   long         yReqBin;    /* Y binning factor (pixels per superpixel)       */
   long         xReqRas;    /* Size of each subaperture in X direction in     */
                            /* super-pixels (wfs ONLY)                        */
   long         yReqRas;    /* Size of each subaperture in Y direction in     */
                            /* super-pixels (WFS only)                        */
   long         xReqSpace;  /* Spacing between subapertures in X direction in */
                            /* pixels (WFS only)                              */
   long         yReqSpace;  /* Spacing between subapertures in Y direction in */
                            /* pixels (WFS only)                              */
   long         xReqStart;  /* X offset from bottom left corner of array in   */
                            /* pixels.                                        */
   long         yReqStart;  /* Y offset from bottom left corner of array in   */
                            /* pixels.                                        */

   long         xReqTail;   /* Number of trailing X pixels to be discarded on */
                            /* each row.                                      */
   long         reqPixelsNb;/* Total number of digitised pixels.              */
   int          nPackets;   /* Number of packets expected per frame.          */

   /*
    * Parameters to update the ADC offset
    */

   int          updateOffset = FALSE;
   long         i;                  /* index                                  */
   long         offsetVect[2];      /* ADC offset vector                      */


   /*
    * Initialise the error number and obtain the attributes provided with 
    * the command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) &binFlag);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) &winFlag);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *) &reqX);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *) &reqY);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 4, (char *) &reqXWidth);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 5, (char *) &reqYWidth);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 6, 
                          (char *) &reqOscanNb);

   reqX1 = reqX - (int)(reqXWidth/2);
   reqX2 = reqX + (int)(reqXWidth/2);
   reqY1 = reqY - (int)(reqYWidth/2);
   reqY2 = reqY + (int)(reqYWidth/2);

   printf ( "reqX1=%d, reqX2=%d, reqY1=%d, reqY2=%d\n", 
	    (int)reqX1, (int)reqX2, (int)reqY1, (int)reqY2);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /*
    * If windowing, check 1 <= x1 < x2 <= max
    */

   if ( binFlag == TRUE )
      max = (DET_CONTROL_HRWFS_XSIZE / 2); 
   else
      max = DET_CONTROL_HRWFS_XSIZE;

   if ( (winFlag == 1) && ((reqX1 < 1) || (reqX2 <= reqX1) || (reqX2 > max)) )
   {
      ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, 
         "Invalid number for x1 and x2 should verify 1<=x1<x2<=%d",
         ERROR_LOG_NOW, (int)max);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
   }

   /*
    * If windowing, check 1 <= y1 < y2 <= max
    */

   if ( binFlag == TRUE )
      max = (DET_CONTROL_HRWFS_YSIZE / 2);
   else
      max = DET_CONTROL_HRWFS_YSIZE;

   if ( (winFlag == 1) && ((reqY1 < 1) || (reqY2 <= reqY1) || (reqY2 > max)) )
   {
      ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, 
         "Invalid number for y1 and y2 should have 1<=y1<y2<=%d",
         ERROR_LOG_NOW, (int)max);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
   }

   /*
    * Init xBin and yBin
    */

   if ( binFlag == TRUE )
   {
      xReqBin = 2;
      yReqBin = 2;
   }
   else
   {
      xReqBin = 1;
      yReqBin = 1;
   }

   /*
    * Init oscanFlag 
    */

   if ( reqOscanNb != 0 )
      obsId->oscanFlag = FULL;
   else
      obsId->oscanFlag = FALSE;
      
   /*
    * Init the different parameters
    */

   if ( winFlag == TRUE )
   {
      xReqSubap = 1;
      yReqSubap = 1;
      xReqSpace = 0;
      yReqSpace = 0;
      if ( yReqBin == 1 )
         yReqStart = reqY1;
      else
         yReqStart = (reqY1*yReqBin) - 1;
      yReqRas = reqY2 - reqY1 + 1;
      yReqPixels = yReqRas * yReqSubap;

      size = reqX2 - reqX1 + 1;
      maxOutput = DET_CONTROL_HRWFS_XSIZE / (xReqBin * obsId->outputsNb) ;
      
      if ( reqX1 <= maxOutput )
      {
         if ( reqX2 <= maxOutput )
         {
            if ( xReqBin == 1 )
               xReqStart = reqX1 + 15;
            else
               xReqStart = (reqX1*xReqBin) - 1 + 15;

            if ( reqOscanNb == 0 )
               xReqRas = size;
            else
            {
               xReqRas = maxOutput - reqX1 + 1 + reqOscanNb;
               obsId->oscanFlag = HALF;
            }
         }
         else
         {
            size1 = maxOutput - reqX1 + 1;
            size2 = reqX2 - maxOutput;      /* = reqX2 - (maxOutput + 1) + 1 */

            if ( size1 > size2 )
            {
               if ( xReqBin == 1 )
                  xReqStart = reqX1 + 15;
               else
                  xReqStart = (reqX1*xReqBin) - 1 + 15;
               xReqRas = size1 + reqOscanNb;
            }
            else
            {
               if ( xReqBin == 1 )
                  xReqStart = (DET_CONTROL_HRWFS_XSIZE/xReqBin) - reqX2 + 1 + 15;
               else
                  xReqStart = ((DET_CONTROL_HRWFS_XSIZE/xReqBin) - reqX2 + 1)*xReqBin -1 + 15;
               xReqRas = size2 + reqOscanNb;
            }
         }
      }
      else
      {
         if ( xReqBin == 1 )
            xReqStart = (DET_CONTROL_HRWFS_XSIZE/xReqBin) - reqX2 + 1 + 15;
         else
            xReqStart = ((DET_CONTROL_HRWFS_XSIZE/xReqBin) - reqX2 + 1)*xReqBin 
                        -1 + 15;
         xReqRas = reqX2 - maxOutput + reqOscanNb;

         if ( reqOscanNb != 0 )
            obsId->oscanFlag = HALF;
      }
            
      xReqPixels = obsId->outputsNb * xReqRas * xReqSubap ;
      reqPixelsNb = xReqPixels * yReqPixels ;
      xReqTail = obsId->xSize - (((xReqRas * xReqBin) + xReqSpace) * xReqSubap)
                 + xReqSpace - xReqStart - obsId->uscanNb + xReqBin*reqOscanNb;

      if ( xReqTail < 0 )
      {
         ERROR_SET1 (S_detControl_BAD_ATTRIBUTE,
         "Xtail is %ld. Should not be less than zero", ERROR_LOG_NOW, xReqTail);
         errorNumber = S_detControl_BAD_ATTRIBUTE;
         return (errorNumber);
      }
   }
   else
   {
      if ( obsId->windowingFlag == FALSE )
      {
         /* first time binning */ 
         if ( (binFlag == TRUE) && (obsId->binningFlag == FALSE) ) 
         {
            xReqStart = obsId->xStart ;
            yReqStart = obsId->yStart ;
            xReqRas = (obsId->xRaster - obsId->oscanNb) / xReqBin + reqOscanNb;
            yReqRas = obsId->yRaster / yReqBin ;
            xReqSpace = obsId->xSpace ;
            yReqSpace = obsId->ySpace ;
            xReqSubap = obsId->xSubapNb ;
            yReqSubap = obsId->ySubapNb ;
            xReqPixels = obsId->outputsNb * xReqRas * xReqSubap ;
            yReqPixels = yReqRas * yReqSubap ;
            reqPixelsNb = xReqPixels * yReqPixels ;
            xReqTail = obsId->xSize - 
                       (((xReqRas * xReqBin) + xReqSpace) * xReqSubap) + 
                       xReqSpace - xReqStart - obsId->uscanNb + 
                       xReqBin*reqOscanNb;

            if ( xReqTail < 0 )
            {
               ERROR_SET1 (S_detControl_BAD_ATTRIBUTE,
               "Xtail is %ld. Should not be less than zero", ERROR_LOG_NOW, 
               xReqTail);
               errorNumber = S_detControl_BAD_ATTRIBUTE;
               return (errorNumber);
            }

            reqX1 = xReqStart - 15 ; /* -16 + 1 */
            reqX2 = xReqPixels ;
            reqY1 = yReqStart ; 
            reqY2 = yReqPixels ;

            /* Init the ADC offset vector */

            for ( i = 0 ; i < obsId->outputsNb ; i ++ )
                offsetVect[i] = pOffsetBinVect[i];

            updateOffset = TRUE;
         }
         else if ( ( (binFlag == TRUE) && (obsId->binningFlag == TRUE) ) ||
                   ( (binFlag == FALSE) && (obsId->binningFlag == FALSE) ) ) 
         /* do not change anything */
         {
            xReqStart = obsId->xStart ;
            yReqStart = obsId->yStart ;
            xReqBin = obsId->xBin ;
            yReqBin = obsId->yBin ;
            xReqRas = obsId->xRaster - obsId->oscanNb + reqOscanNb;
            yReqRas = obsId->yRaster ;
            xReqSpace = obsId->xSpace ;
            yReqSpace = obsId->ySpace ;
            xReqSubap = obsId->xSubapNb ;
            yReqSubap = obsId->ySubapNb ;
            xReqPixels = obsId->outputsNb * xReqRas * xReqSubap ;
            yReqPixels = obsId->yPixels;
            reqPixelsNb = xReqPixels * yReqPixels ;
            xReqTail = obsId->xSize - 
                       (((xReqRas * xReqBin) + xReqSpace) * xReqSubap) + 
                       xReqSpace - xReqStart - obsId->uscanNb + 
                       xReqBin*reqOscanNb;

            reqX1 = xReqStart - 15 ; /* -16 + 1 */
            reqX2 = xReqPixels ;
            reqY1 = yReqStart ; 
            reqY2 = yReqPixels ;
         }
         else  /* cancel binning */
         {
            xReqStart = obsId->xStart ;
            yReqStart = obsId->yStart ;
            xReqRas = (obsId->xRaster - obsId->oscanNb) * obsId->xBin + 
                      reqOscanNb;
            yReqRas = obsId->yRaster * obsId->yBin ;
            xReqSpace = obsId->xSpace ;
            yReqSpace = obsId->ySpace ;
            xReqSubap = obsId->xSubapNb ;
            yReqSubap = obsId->ySubapNb ;
            xReqPixels = obsId->outputsNb * xReqRas * xReqSubap ;
            yReqPixels = yReqRas * yReqSubap ;
            reqPixelsNb = xReqPixels * yReqPixels ;
            xReqTail = obsId->xSize - 
                       (((xReqRas * xReqBin) + xReqSpace) * xReqSubap)
                       + xReqSpace - xReqStart - obsId->uscanNb +
                       xReqBin*reqOscanNb;

            if ( xReqTail < 0 )
            {
               ERROR_SET1 (S_detControl_BAD_ATTRIBUTE,
               "Xtail is %ld. Should not be less than zero", ERROR_LOG_NOW, 
               xReqTail);
               errorNumber = S_detControl_BAD_ATTRIBUTE;
               return (errorNumber);
            }

            reqX1 = xReqStart - 15 ; /* -16 + 1 */
            reqX2 = xReqPixels ;
            reqY1 = yReqStart ; 
            reqY2 = yReqPixels ;

            /* Init the ADC offset vector */

            for ( i = 0 ; i < obsId->outputsNb ; i ++ )
                offsetVect[i] = pOffsetFullVect[i];

            updateOffset = TRUE;
         }
      }
      else /* suppress windowing */
      {
         if ( (binFlag == TRUE) && (obsId->binningFlag == FALSE) ) 
         /* first time binning */ 
         {
            xReqStart = 16 ;
            yReqStart = 1 ;
            xReqRas = DET_CONTROL_HRWFS_XSIZE / ( obsId->outputsNb * xReqBin ) 
                      + reqOscanNb;
            yReqRas = DET_CONTROL_HRWFS_YSIZE / yReqBin ;
            xReqSpace = obsId->xSpace ;
            yReqSpace = obsId->ySpace ;
            xReqSubap = obsId->xSubapNb ;
            yReqSubap = obsId->ySubapNb ;
            xReqPixels = obsId->outputsNb * xReqRas * xReqSubap ;
            yReqPixels = yReqRas * yReqSubap ;
            reqPixelsNb = xReqPixels * yReqPixels ;
            xReqTail = obsId->xSize - 
                       (((xReqRas * xReqBin) + xReqSpace) * xReqSubap)
                       + xReqSpace - xReqStart - obsId->uscanNb
                       + xReqBin*reqOscanNb;

            if ( xReqTail < 0 )
            {
               ERROR_SET1 (S_detControl_BAD_ATTRIBUTE,
               "Xtail is %ld. Should not be less than zero", ERROR_LOG_NOW, 
               xReqTail);
               errorNumber = S_detControl_BAD_ATTRIBUTE;
               return (errorNumber);
            }

            reqX1 = xReqStart - 15 ; /* -16 + 1 */
            reqX2 = xReqPixels ;
            reqY1 = yReqStart ; 
            reqY2 = yReqPixels ;

            /* Init the ADC offset vector */

            for ( i = 0 ; i < obsId->outputsNb ; i ++ )
                offsetVect[i] = pOffsetBinVect[i];

            updateOffset = TRUE;
         }
         else if ( ( (binFlag == TRUE) && (obsId->binningFlag == TRUE) ) ||
                   ( (binFlag == FALSE) && (obsId->binningFlag == FALSE) ) ) 
         /* do not change anything */
         {
            xReqStart = 16 ;
            yReqStart = 1 ;
            xReqBin = obsId->xBin ;
            yReqBin = obsId->yBin ;
            xReqRas = DET_CONTROL_HRWFS_XSIZE / ( obsId->outputsNb * xReqBin ) 
                      + reqOscanNb;
            yReqRas = DET_CONTROL_HRWFS_YSIZE / yReqBin ;
            xReqSpace = obsId->xSpace ;
            yReqSpace = obsId->ySpace ;
            xReqSubap = obsId->xSubapNb ;
            yReqSubap = obsId->ySubapNb ;
            xReqPixels = xReqRas * xReqSubap * obsId->outputsNb ;
            yReqPixels = yReqRas * yReqSubap ;
            reqPixelsNb = xReqPixels * yReqPixels ;
            xReqTail = obsId->xSize - 
                       (((xReqRas * xReqBin) + xReqSpace) * xReqSubap)
                       + xReqSpace - xReqStart - obsId->uscanNb 
                       + xReqBin * reqOscanNb;

            if ( xReqTail < 0 )
            {
               ERROR_SET1 (S_detControl_BAD_ATTRIBUTE,
               "Xtail is %ld. Should not be less than zero", ERROR_LOG_NOW, 
               xReqTail);
               errorNumber = S_detControl_BAD_ATTRIBUTE;
               return (errorNumber);
            }

            reqX1 = xReqStart - 15 ; /* -16 + 1 */
            reqX2 = xReqPixels ;
            reqY1 = yReqStart ; 
            reqY2 = yReqPixels ;
         }
         else  /* cancel binning */
         {
            xReqStart = 16 ;
            yReqStart = 1 ;
            xReqRas = (DET_CONTROL_HRWFS_XSIZE / obsId->outputsNb) 
                      + reqOscanNb ;
            yReqRas = DET_CONTROL_HRWFS_YSIZE ;
            xReqSpace = obsId->xSpace ;
            yReqSpace = obsId->ySpace ;
            xReqSubap = obsId->xSubapNb ;
            yReqSubap = obsId->ySubapNb ;
            xReqPixels = obsId->outputsNb * xReqRas * xReqSubap ;
            yReqPixels = yReqRas * yReqSubap ;
            reqPixelsNb = xReqPixels * yReqPixels ;
            xReqTail = obsId->xSize - 
                       (((xReqRas * xReqBin) + xReqSpace) * xReqSubap)
                       + xReqSpace - xReqStart - obsId->uscanNb
                       + xReqBin * reqOscanNb;

            if ( xReqTail < 0 )
            {
               ERROR_SET1 (S_detControl_BAD_ATTRIBUTE,
               "Xtail is %ld. Should not be less than zero", ERROR_LOG_NOW, 
               xReqTail);
               errorNumber = S_detControl_BAD_ATTRIBUTE;
               return (errorNumber);
            }

            reqX1 = xReqStart - 15 ; /* -16 + 1 */
            reqX2 = xReqPixels ;
            reqY1 = yReqStart ; 
            reqY2 = yReqPixels ;

            /* Init the ADC offset vector */

            for ( i = 0 ; i < obsId->outputsNb ; i ++ )
                offsetVect[i] = pOffsetFullVect[i];

            updateOffset = TRUE;
         }
      }
   }

   if ( (!sdsuId->simulate) && (obsId->packetSize > 0) )
   {
      nPackets = (int) ceil ( (double) (reqPixelsNb) / 
                 (double) obsId->packetSize );
   }
   else
   {
      nPackets = 1;
   }

   /*
    * Everything is ok, init obsId.
    * Determine whether the given parameters will put the detector controller
    * into full frame mode. This happens when the there is one subaperture per
    * output and the subapertures fill the detector surface without any gaps.
    */

   obsId->xStart = xReqStart;
   obsId->yStart = yReqStart;
   obsId->xBin = xReqBin;
   obsId->yBin = yReqBin;
   obsId->xRaster = xReqRas;
   obsId->yRaster = yReqRas;
   obsId->xSpace = xReqSpace;
   obsId->ySpace = yReqSpace;
   obsId->xSubapNb = xReqSubap;
   obsId->ySubapNb = yReqSubap;
   obsId->xPixels = xReqPixels;
   obsId->yPixels = yReqPixels;
   obsId->pixelsNb = reqPixelsNb;
   obsId->xTail = xReqTail;
   obsId->oscanNb = reqOscanNb;

   if ( winFlag == FALSE )
   {
      obsId->windowingFlag = FALSE;
   }
   else
   {
      obsId->windowingFlag = TRUE;
   }

   if ( binFlag == TRUE )
      obsId->binningFlag = TRUE ;
   else
      obsId->binningFlag = FALSE ;

   if ( (obsId->binningFlag == TRUE) || (obsId->windowingFlag == TRUE) )
      obsId->fullImageFlag = FALSE ;
   else
      obsId->fullImageFlag = TRUE ;

   obsId->x1 = reqX1 ;
   obsId->x2 = reqX2 ;
   obsId->y1 = reqY1 ;
   obsId->y2 = reqY2 ;

/*#ifdef DEBUG*/
   /*
    * Show CCD Geometry information
    */

   printf ( "CCD Geometry information from obsId \n" ) ;
   printf ( "Outputs number : %d\n" , (int) obsId->outputsNb ) ;
   printf ( "xSize (per output) : %d\n" , obsId->xSize ) ;
   printf ( "ySize (per output) : %d\n" , obsId->ySize ) ;
   printf ( "xMax : %d\n" , obsId->xMax ) ;
   printf ( "yMax : %d\n" , obsId->yMax ) ;
   printf ( "xStart : %d\n" , obsId->xStart ) ;
   printf ( "yStart : %d\n" , obsId->yStart ) ;
   printf ( "xBin : %d\n" , obsId->xBin ) ;
   printf ( "yBin : %d\n" , obsId->yBin ) ;
   printf ( "xRaster : %d\n" , obsId->xRaster ) ;
   printf ( "yRaster : %d\n" , obsId->yRaster ) ;
   printf ( "xSpace : %d\n" , obsId->xSpace ) ;
   printf ( "ySpace : %d\n" , obsId->ySpace ) ;
   printf ( "xSubap number : %d\n" , obsId->xSubapNb ) ;
   printf ( "ySubap number : %d\n" , obsId->ySubapNb ) ;
   printf ( "xPixels : %d\n" , obsId->xPixels ) ;
   printf ( "yPixels : %d\n" , obsId->yPixels ) ;
   printf ( "pixels number : %d\n" , obsId->pixelsNb ) ;
   printf ( "uscan number : %d\n" , obsId->uscanNb ) ;
   printf ( "oscan number : %d\n" , obsId->oscanNb ) ;
   if ( obsId->oscanFlag == FALSE )
      printf ( "oscanFlag : FALSE\n" ) ;
   else
   {
      if ( obsId->oscanFlag == FULL )
         printf ( "oscanFlag : FULL\n" ) ;
      else
         printf ( "oscanFlag : HALF\n" ) ;
   }
   printf ( "Xtail : %d\n" , obsId->xTail ) ;
   printf ( "packet size : %d\n" , obsId->packetSize ) ;
   if ( obsId->fullImageFlag == TRUE )
      printf ( "FullImageFlag : TRUE\n" ) ;
   else
      printf ( "FullImageFlag : FALSE\n" ) ;
   if ( obsId->binningFlag == TRUE )
      printf ( "binningFlag : TRUE\n" ) ;
   else
      printf ( "binningFlag : FALSE\n" ) ;
   if ( obsId->windowingFlag == TRUE )
      printf ( "windowingFlag : TRUE\n" ) ;
   else
      printf ( "windowingFlag : FALSE\n" ) ;
   printf ( "x1=%d, x2=%d\n" , obsId->x1, obsId->x2 ) ;
   printf ( "y1=%d, y2=%d\n" , obsId->y1, obsId->y2 ) ;
/*
#endif
*/

   MESSAGE_LOG1 (MSG_LOG, "Setting new detector geometry (%s frame mode)",
      (obsId->fullImageFlag ? "full":"reduced"));

   MESSAGE_LOG4 (MSG_MINDEBUG, "XSIZE=%d, YSIZE=%d, XPIXELS=%d, YPIXELS=%d",
      obsId->xSize, obsId->ySize, obsId->xPixels, obsId->yPixels);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XSUBAP=%d, YSUBAP=%d, XBIN=%d, YBIN=%d",
      obsId->xSubapNb, obsId->ySubapNb, obsId->xBin, obsId->yBin);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XRAS=%d, YRAS=%d, XSPACE=%d, YSPACE=%d",
      obsId->xRaster, obsId->yRaster, obsId->xSpace, obsId->ySpace);
   MESSAGE_LOG4 (MSG_MINDEBUG, "XSTART=%d, YSTART=%d, XTAIL=%d, NPIXELS=%d",
      obsId->xStart, obsId->yStart, obsId->xTail, obsId->pixelsNb);

   MESSAGE_LOG2 (MSG_FULLDEBUG,
      "Each frame will consist of %d packets of %d pixels each",
      nPackets, obsId->packetSize);


   /*
    * Update the geometry parameters in the SDSU timing DSP. These are all
    * "on-the-fly" parameters and need to be downloaded with sdsuParamWRP()
    * and activated by sending a "LDP" command.
    */

   if ( !sdsuId->simulate ) 
   {
      if ( (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSUBAP", 
                          (uint32) xReqSubap) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSUBAP", 
                          (uint32) yReqSubap) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSTART", 
                          (uint32) xReqStart) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSTART", 
                          (uint32) yReqStart) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XRAS",   
                          (uint32) xReqRas) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YRAS",   
                          (uint32) yReqRas) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XSPACE", 
                          (uint32) xReqSpace) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YSPACE", 
                          (uint32) yReqSpace) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XBIN",   
                          (uint32) xReqBin) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_YBIN",   
                          (uint32) yReqBin) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_XTAIL",  
                          (uint32) xReqTail) == ERROR) ||
           (sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_NPIXEL", 
                          (uint32) reqPixelsNb) == ERROR)
        )
      {
         ERROR_LOG ("Failed to download geometry parameters to TIMING DSP");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if ( updateOffset == TRUE )
      {
         if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS0",
                           (uint32) offsetVect[0] ) == ERROR )
         {
            ERROR_LOG ("Error setting ADC offset 0 parameter");
            errorNumber = S_detControl_SDSU_ERROR;
            return (errorNumber);
         }

         if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_ADC_OS1",
                            (uint32) offsetVect[1] ) == ERROR )
         {
            ERROR_LOG ("Error setting ADC offset 1 parameter");
            errorNumber = S_detControl_SDSU_ERROR;
            return (errorNumber);
         }

         /*
          * Update the adc sir records
          */

         if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[0]) ,
                              obsId->pAdc0Context ) == ERROR)
         {
            ERROR_LOG ("Failed to init adc0 sad record");
            return (ERROR);
         }

         if (epToVxPipeWrite( NULL, (char *)(int)& (offsetVect[1]) ,
                              obsId->pAdc1Context ) == ERROR)
         {
            ERROR_LOG ("Failed to init adc1 sad record");
            return (ERROR);
         }
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
   }

   /*
    * Init the geometry sad records
    */

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xStart) , 
                        obsId->pXstartContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xstart sad record");
      return (ERROR);
   }
   
   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->yStart) , 
                        obsId->pYstartContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ystart sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xSubapNb) , 
                        obsId->pXsubapContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xsubap sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->ySubapNb) , 
                        obsId->pYsubapContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init Ysubap sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xRaster) , 
                        obsId->pXrasterContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xraster sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->yRaster) , 
                        obsId->pYrasterContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init yraster sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xSpace) , 
                        obsId->pXspaceContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xspace sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->ySpace) , 
                        obsId->pYspaceContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init yspace sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->xBin) , 
                        obsId->pXbinContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init xbin sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->yBin) , 
                        obsId->pYbinContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init ybin sad record");
      return (ERROR);
   }

   if (epToVxPipeWrite( NULL, (char *)(int)& (obsId->oscanNb) , 
                        obsId->pOscanContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init oscan sad record");
      return (ERROR);
   }

   return (errorNumber);
}



/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDownload
 *
 *   INVOCATION:
 *   detDownload (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *                sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *   (>) vmeAddress    (uint32)          VME address of SDSU controller
 *                                       (0=simulate)
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detDownload command
 *
 *   DESCRIPTION:
 *   This function downloads DSP code from OMF files.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detDownload
   (
   const char *    pWfsName,       /* Name of wavefront sensor.               */
   const char *    pRecordPrefix,  /* Record name prefix.                     */
   CAD_CMD_CONTEXT cadCmdContext,  /* CAD command context structure.          */
   int             commandNumber,  /* Command number.                         */
   SDSU_ID         sdsuId,         /* SDSU context structure.                 */
   OBS_ID          obsId           /* Observation context structure.          */
   )
{
   uint32       errorNumber;       /* Error number reported by task.          */

   char         pFilePath [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                   /* Path name for file.                     */
   char         pOmfFileName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                   /* File name.                              */
   char         pFullOmfFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                                   /* Combined path name and file name.       */

   BOOL         limitAdrsRange;    /* Flag for limiting adr range in DSP mem  */

   uint32       mode;

   /*
    * Initialise the error number.
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY, 
         "Observation in progress - abort observation and try again",
         ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /* First obtain the path of the directory containing the DSP code. */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pFilePath);

   /* Determine whether any code should be downloaded to the VME DSP */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, pOmfFileName);

   if ( (strcmp (pOmfFileName, "") != 0) && 
        (strcmp (pOmfFileName, "NONE") != 0) )
   {

      /* The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
        * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      if ( strcmp (pFilePath, "") == 0 )
      {
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to VME DSP...",
                    pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_VME, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download OMF file to VME DSP");
         errorNumber = S_detControl_SDSU_ERROR;
         epToVxSetHealth( pRecordPrefix, "WARNING" );
      }
   }

   /* Determine whether any code should be downloaded to the Timing DSP */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, pOmfFileName);

   if ( (strcmp (pOmfFileName, "") != 0) && 
        (strcmp (pOmfFileName, "NONE") != 0) )
   {

      /* The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      if ( strcmp (pFilePath, "") == 0 )
      {
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to TIMING DSP...",
                    pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_TIM, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download OMF file to TIMING DSP");
         errorNumber = S_detControl_SDSU_ERROR;
         epToVxSetHealth( pRecordPrefix, "WARNING" );
      }
   }

   /* Determine whether any code should be downloaded to the Utility DSP */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, pOmfFileName);

   if ( (strcmp (pOmfFileName, "") != 0) && 
        (strcmp (pOmfFileName, "NONE") != 0) )
   {

      /* The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      if ( strcmp (pFilePath, "") == 0 )
      {
         strncpy (pFullOmfFileName, pOmfFileName, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pFullOmfFileName, "%s/%s", pFilePath, pOmfFileName );
      }

      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to UTILITY DSP...",
                    pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_UTL, 
                          limitAdrsRange) == ERROR)
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
            if (sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_MODE", mode)==ERROR)
            {
               ERROR_LOG ("Failed to clear sync mode on timing board");
            }
         }
      }
   }

   /*
    * After successfully downloading new OMF code, the controller must be
    * reinitialised by sending an "INI" command to the utility DSP and a "LDP"
    * command to the timing DSP.
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


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detPrimitive
 *
 *   INVOCATION:
 *   detPrimitive (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *                 sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName             (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix        (const char *)    Record name prefix
 *   (>) cadCmdContext        (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber        (int)             Command number
 *   (>) sdsuId               (SDSU_ID)         Current SDSU context structure
 *   (>) obsId                (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detPrimitive command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU exposure parameters.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detPrimitive
   (
   const char *    pWfsName,       /* Name of wavefront sensor.               */
   const char *    pRecordPrefix,  /* Record name prefix.                     */
   CAD_CMD_CONTEXT cadCmdContext,  /* CAD command context structure.          */
   int             commandNumber,  /* Command number.                         */
   SDSU_ID         sdsuId,         /* SDSU context structure.                 */
   OBS_ID          obsId           /* Observation context structure.          */
   )
{
   uint32          errorNumber;     /* Error number reported by task.         */

   long            destId;          /* Destination DSP ID.                    */

   long            pCmdArg [6] = {0, 0, 0, 0, 0, 0};
                                    /* Primitive command arguments.           */
   long            pRepArg [3] = {0, 0, 0};
                                    /* Primitive command reply arguments.     */

   char            pStringAttrib [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                                    /* Contents of general string attribute.  */

   uint32          i;               /* Loop counter.                          */

   /*
    * Initialise the error number.
    */

   errorNumber = 0;

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * Some primitive commands can be accepted while an observation is in
    * progress. However, warn the user if there is an observation in progress.
    */

   if ( obsId->observing )
   {
      MESSAGE_LOG (MSG_WARNING, 
      "NOTE: Issuing primitive command while observation in progress.");
   }

   /*
    * Get the command name, destination DSP and up to 6 command arguments
    * from the attributes supplied with the CAD command.
    */

   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, pStringAttrib);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & destId);

   for (i = 0; i < 6; i++)
   {
      EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 
                             i + 2, (char *) & pCmdArg [i]);
   }

   MESSAGE_LOG4 (MSG_LOG, 
   "About to execute SDSU primitive command \"%s\" to %ld with %#lx ... %#lx",
   pStringAttrib, destId, pCmdArg[0], pCmdArg[5]);

   /*
    * Issue the primitive command to the SDSU controller.
    */

   if (sdsuPrimitive (sdsuId, pStringAttrib, (uint32) destId, 
       (uint32 *) pCmdArg, (uint32 *) pRepArg) == ERROR)
   {
      ERROR_LOG ("Failed to execute SDSU primitive command");
      errorNumber = S_detControl_SDSU_ERROR;
   }

   /* Write the reply to the SDSU primitive reply SIR record. */   

   sprintf (pStringAttrib, "0x%08lx 0x%08lx 0x%08lx", 
            pRepArg [0], pRepArg [1], pRepArg [2]);
   if (epToVxPipeWrite (NULL, pStringAttrib, obsId->pDetPrimReplyContext) 
       == ERROR)
   {
      ERROR_LOG ("Failed to write message to SDSU primitive reply pipe.");
      if ( errorNumber == 0 ) errorNumber = (uint32) errnoGet();
   }

   return (errorNumber);
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detMode
 *
 *   INVOCATION:
 *   detMode (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *            sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detMode command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU readout mode.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detMode
   (
   const char *    pWfsName,      /* Name of wavefront sensor.                */
   const char *    pRecordPrefix, /* Record name prefix.                      */
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */

   long            mode;          /* Readout mode parameter.                  */
   long            samples;       /* Number of samples per frame.             */
   long            tInt;          /* CDI integration time.                    */
   long            gainSp;        /* Combined amplifier gain and integrator   */
                                  /* speed.                                   */

   BOOL        ccdFailed = FALSE; /* Set TRUE if CCD parameter setup fails.   */
   BOOL        irFailed = FALSE;  /* Set TRUE if CCD parameter setup fails.   */

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & mode);
   /* CCD only */
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & tInt);
   /* CCD only */
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 2, (char *) & gainSp);
   /* IR only */
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 3, (char *) & samples);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can be used when an observation is in progress, as it only
    * redefines "on-the-fly" parameters. However, warn the user this is
    * happening.
    */

   if ( obsId->observing )
   {
      MESSAGE_LOG (MSG_WARNING,
         "NOTE: Changing on-the-fly parameters while observation in progress.");
   }

   MESSAGE_LOG4 (MSG_LOG, "Defining new readout mode: %#lx %#lx %#lx %#lx",
      mode, tInt, gainSp, samples);

   /*
    * Set the readout mode by writing the appropriate SDSU parameters. All are
    * on-the-fly parameters, except GAIN_SP, and need to be downloaded with
    * sdsuParamWRP() and activated by sending a "LDP" command. GAIN_SP cannot
    * be changed if an observation is in progress, and must be updated
    * separately with sdsuParamWrite().
    *
    * If any parameter is defined as -1 it is not changed.
    *
    * Only the T_MODE parameter is universal. The T_INT_TIM and T_GAIN_SP
    * parameters are valid for CCD detectors only, and T_SAMPLES is valid for
    * IR detectors only. An error is only reported if an attempt to write both
    * the CCD and IR parameters fails.
    */

   if ( mode != -1 )
   {
      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_MODE", 
                         (uint32) mode ) == ERROR )
      {
         ERROR_LOG ("Error setting readout mode parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }
   }

   if ( tInt != -1 )
   {
      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_INT_TIM", 
                         (uint32) tInt ) == ERROR )
      {
         ccdFailed = TRUE;
      }
   }

   if ( samples != -1 )
   {
      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, "T_SAMPLES", 
                         (uint32) samples ) == ERROR )
      {
         irFailed = TRUE;
      }
   }

   if ( (!obsId->observing) && (gainSp != -1) )
   {
      if ( sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_GAIN_SP", 
                          (uint32) gainSp ) == ERROR )
      {
         ccdFailed = TRUE;
      }
   }
   else if ( obsId->observing )
   {
      MESSAGE_LOG (MSG_WARNING, 
                   "T_GAIN_SP parameter not changed while observing");
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


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detOffset
 *
 *   INVOCATION:
 *   detOffset (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *              sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detMode command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU readout mode.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 detOffset
   (
   const char *    pWfsName,      /* Name of wavefront sensor.                */
   const char *    pRecordPrefix, /* Record name prefix.                      */
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32       errorNumber;     /* Error number reported by task.         */

   long         offset0;         /* ADC offset for output 0.               */
   long         offset1;         /* ADC offset for output 1.               */

   /*
    * Initialise the error number and obtain the attributes provided with 
    * the command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) & offset0);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, (char *) & offset1);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can be used when an observation is in progress, as it only
    * redefines "on-the-fly" parameters. However, warn the user this is
    * happening.
    */

   if ( obsId->observing )
   {
      MESSAGE_LOG (MSG_WARNING,
         "NOTE: Changing on-the-fly parameters while observation in progress.");
   }

   MESSAGE_LOG2 (MSG_LOG, "Defining new ADC offset levels: %#lx %#lx",
      offset0, offset1);

   /*
    * Set the offsets by writing the appropriate SDSU parameters.
    * If any parameter is defined as -1 it is not changed.
    */

   if ( offset0 != -1 )
   {
      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, 
           "T_ADC_OS0", (uint32) offset0 ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 0 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if (epToVxPipeWrite( NULL, (char *)(int)& (offset0) ,
                           obsId->pAdc0Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc0 sad record");
         return (ERROR);
      }
   }

   if ( offset1 != -1 )
   {
      if ( sdsuParamWRP (sdsuId, SDSU_IDENT_TIM, 
           "T_ADC_OS1", (uint32) offset1 ) == ERROR )
      {
         ERROR_LOG ("Error setting ADC offset 1 parameter");
         errorNumber = S_detControl_SDSU_ERROR;
         return (errorNumber);
      }

      if (epToVxPipeWrite( NULL, (char *)(int)& (offset1) ,
                           obsId->pAdc1Context ) == ERROR)
      {
         ERROR_LOG ("Failed to init adc1 sad record");
         return (ERROR);
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

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detTemp
 *
 *   INVOCATION:
 *   detTemp (pWfsName, pRecordPrefix, cadCmdContext, commandNumber, 
 *            sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *)    Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *)    Record name prefix
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (>) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detTemp command
 *
 *   DESCRIPTION:
 *   This function sets up the SDSU exposure parameters.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   Assumes SDSU_TEMP_UNIT is not zero.
 *-
 */

uint32 detTemp
   (
   const char *    pWfsName,         /* Name of wavefront sensor.             */
   const char *    pRecordPrefix,    /* Record name prefix.                   */
   CAD_CMD_CONTEXT cadCmdContext,    /* CAD command context structure.        */
   int             commandNumber,    /* Command number.                       */
   SDSU_ID         sdsuId,           /* SDSU context structure.               */
   OBS_ID          obsId             /* Observation context structure.        */
   )
{
   uint32         errorNumber;       /* Error number reported by task.        */

   /* 
    * Variables associated with the "set detector temperature 
    * parameters" command.
    */

   double         tempTarget;        /* Target temperature in Celsius.        */
   uint32         tempCode;          /* Target temperature code.              */
   long           tempCoeff;         /* Coefficient for temperature control.  */

   /*
    * Initialise the error number and obtain the attributes provided with the 
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, 
                          (char *) & tempTarget);
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 1, 
                          (char *) & tempCoeff);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can only be used when an observation is not in progress.
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY, 
                 "Observation in progress - abort observation and try again",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   if ( tempTarget <= 0.0 )
   {
      tempCode = (uint32) ((SDSU_TEMP_BASE - tempTarget) / SDSU_TEMP_UNIT);
      tempCode &= 0xfff; /* Truncate to 0xfff (which is the maximum allowed) */
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

   if ( (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_CCDT_TGT", 
                         tempCode ) == ERROR) ||
        (sdsuParamWrite (sdsuId, SDSU_IDENT_UTL, "U_TCF", 
                         (uint32) tempCoeff ) == ERROR)
      )
   {
      ERROR_LOG ("Error setting temperasture control parameters");
      errorNumber = S_detControl_SDSU_ERROR;
      return (errorNumber);
   }

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */
/*+
 *   FUNCTION NAME:
 *   detShow
 *
 *   INVOCATION:
 *   detShow (pWfsName, verbose)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName (const char *) Name of WFS
 *   (>) verbose  (const BOOL)   Enable verbose printout
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Show status of detector control task
 *
 *   DESCRIPTION:
 *   This is an engineering function which displays the current status of the
 *   detector control task.
 *
 *   NOTE:
 *   This function is designed to be invoked from the VxWorks shell
 *
 *   EXTERNAL VARIABLES:
 *   (>) detSdsuIdHr (SDSU_ID) SDSU context structure for HRWFS
 *   (>) detObsIdHr  (OBS_ID)  Observation context structure for HRWFS
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

void detShow
   (
   const char *   pWfsName,
   const BOOL     verbose
   )
{
   /*
    * Display the contents of the SDSU context structures for the
    * wavefront sensor: PWFS2
    */

   if ( (pWfsName == NULL) || (strcmp (pWfsName, " ") == 0) ||
        (strstr(pWfsName, "hr") != NULL) || (strstr(pWfsName, "hrwfs") 
        != NULL)
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


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detStatusShow
 *
 *   INVOCATION:
 *   detStatusShow (pWfsName)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName (const char *) Name of WFS
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Show status parameters of detector control task
 *
 *   DESCRIPTION:
 *   This is an engineering function which displays the status parameters of the
 *   detector control task.
 *
 *   NOTE:
 *   This function is designed to be invoked from the VxWorks shell
 *
 *   EXTERNAL VARIABLES:
 *   (>) detSdsuIdHr (SDSU_ID) SDSU context structure for HRWFS
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

void detStatusShow
   (
   const char *   pWfsName
   )
{
   /*
    * Display the SDSU status parameters for HRWFS
    */

   if ( (pWfsName == NULL) || (strcmp (pWfsName, " ") == 0) ||
        (strstr(pWfsName, "hr") != NULL) || (strstr(pWfsName, "hrwfs") != NULL)
      )
   {

      printf ("detStatusShow:          HRWFS\n");
      printf ("detStatusShow:          -----\n");

      if ( detSdsuIdHr != NULL )
      {
         if ( sdsuStatusShow (detSdsuIdHr) == ERROR )
         {
            printf (
            "detStatusShow: SDSU controller context for HRWFS invalid.\n");
         }
      }
      else
      {
         printf ("detStatusShow: SDSU controller for HRWFS not initialised.\n");
      }
   }

   return;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detTempShow
 *
 *   INVOCATION:
 *   detTempShow (pWfsName)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName (const char *) Name of WFS
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Show temperature parameters of detector control task
 *
 *   DESCRIPTION:
 *   This is an engineering function which displays the temperature parameters 
 *   of the detector control task
 *
 *   NOTE:
 *   This function is designed to be invoked from the VxWorks shell
 *
 *   EXTERNAL VARIABLES:
 *   (>) detSdsuIdHr (SDSU_ID) SDSU context structure for HRWFS
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

void detTempShow
   (
   const char *   pWfsName
   )
{
   /*
    * Display the SDSU status parameters for HRWFS
    */

   if ( (pWfsName == NULL) || (strcmp (pWfsName, " ") == 0) ||
        (strstr(pWfsName, "hr") != NULL) || (strstr(pWfsName, "hrwfs") != NULL)
      )
   {

      printf ("detTempShow:          HRWFS\n");
      printf ("detTempShow:          -----\n");

      if ( detSdsuIdHr != NULL )
      {
         if ( sdsuTempShow (detSdsuIdHr) == ERROR )
         {
            printf (
            "detTempShow: SDSU controller context for HRWFS invalid.\n");
         }
      }
      else
      {
         printf ("detTempShow: SDSU controller for HRWFS not initialised.\n");
      }
   }

   return;
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detObsContextCreate
 *
 *   INVOCATION:
 *   detObsContextCreate (void)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (OBS_ID)   Pointer to observation ID, or NULL if unsuccessful.
 *
 *   PURPOSE:
 *   Create an observation ID structure
 *
 *   DESCRIPTION:
 *   This function creates and initialises an observation ID structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

OBS_ID detObsContextCreate (void)
{
   OBS_ID   obsId;

   /* Allocate memory for the observation ID structure, initialising its
    * contents to zero.
    */

#ifdef DEBUG
   printf (
   "detObsContextCreate: Allocating %d bytes of memory for OBS_ID struct.\n",
   sizeof (OBS_ID_STRUCT));
#endif /* DEBUG */

   if ((obsId = (OBS_ID) calloc ((size_t) 1, sizeof (OBS_ID_STRUCT))) == NULL)
   {
      ERROR_SET (0,"Memory allocation for observation context failed", 
                 ERROR_LOG_SAVE);
      return (NULL);
   }

   /* Create a binary semaphore for synchronising observation threads. */

   obsId->syncSem = semBCreate( SEM_Q_FIFO, SEM_EMPTY );
   if ( obsId->syncSem == NULL )
   {
      ERROR_SET (0, "Failed to create observation synchronisation semaphore", 
                 ERROR_LOG_SAVE);
      cfree ((char *) obsId);
      return (NULL);
   }

   return (obsId);
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detObsShow
 *
 *   INVOCATION:
 *   detObsShow (obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   obsId   (OBS_ID)      Pointer to observation ID
 *   (>)   verbose (const BOOL)  Enable verbose mode
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Display the contents of an observation ID structure
 *
 *   DESCRIPTION:
 *   This function creates and initialises an observation ID structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detObsShow (
   OBS_ID      obsId,
   const BOOL  verbose
   )
{
   const char *   outOptionStrings[3] =
      {
         "NONE", "DHS", "FILE"
      };

   const char *   dhsOutOptionStrings[3] =
      {
         "PERM", "TEMP", "QL"
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
   printf ("Observing?                       : %s\n", 
           (obsId->observing ? "TRUE" : "FALSE") );
   printf ("stopped?                         : %s\n", 
           (obsId->stopped ? "TRUE" : "FALSE") );
   printf ("continuous?                      : %s\n", 
           (obsId->continuous ? "TRUE" : "FALSE") );
   printf ("Total number of frames           : %d\n", obsId->totalFrames);
   printf ("Out number of frames             : %d\n", obsId->outNFrames);
   printf ("Frame counter                    : %d\n", obsId->nframes);
   printf ("Alarm timer ID                   : %d\n", (int) obsId->timeId);
   printf ("Name of wfs                      : %s\n", obsId->pWfsName );
   printf ("Observation type                 : %s\n", obsId->pObsType );

   printf ("Output options                   : %s\n", 
           outOptionStrings[obsId->outOptions] );
   printf ("DHS output options               : %s\n",
           dhsOutOptionStrings[obsId->dhsOutOptions] );
   printf ("dhsQlRate                        : %d\n",
           obsId->dhsQlRate );
   printf ("dhsCounter                       : %d\n",
           obsId->dhsCounter );
   printf ("pCurFrame                        : %p\n",
            obsId->pCurFrame );
   printf ("pDispFrame                       : %p\n",
            obsId->pDispFrame );
   printf ("Size of frame in pixels for DHS (X x Y)  : %d x %d\n",
           obsId->xPixelsDhs, obsId->yPixelsDhs);
   printf ("Data label                       : %s\n", obsId->pDataLabel);
   printf ("Output data file name            : %s\n", obsId->pOutFileName);
   printf ("  Simulated data file name       : %s (simulate=%s)\n", 
           obsId->pSimFileName,
           ((obsId->sdsuId == NULL) ? "DON'T KNOW" : (obsId->sdsuId->simulate ? "YES" : "NO")) );

   printf ("dataSec[]                        : %s\n", obsId->dataSec);
   printf ("ccdSec[]                         : %s\n", obsId->ccdSec);
   printf ("origSec[]                        : %s\n", obsId->origSec);
   printf ("utStartString                    : %s\n", obsId->utStartString);
   printf ("utEndString                      : %s\n", obsId->utEndString);
   printf ("detType                          : %s\n", obsId->detType);
   printf ("detId                            : %s\n", obsId->detId);

   printf ("outputNb                         : %d\n", (int)obsId->outputsNb);
   printf ("xSize                            : %d\n", obsId->xSize);
   printf ("ySize                            : %d\n", obsId->ySize);
   printf ("xMax                             : %d\n", obsId->xMax);
   printf ("yMax                             : %d\n", obsId->yMax);
   printf ("xStart                           : %d\n", obsId->xStart);
   printf ("yStart                           : %d\n", obsId->yStart);
   printf ("xBin                             : %d\n", obsId->xBin);
   printf ("yBin                             : %d\n", obsId->yBin);
   printf ("xRaster                          : %d\n", obsId->xRaster);
   printf ("yRaster                          : %d\n", obsId->yRaster);
   printf ("xSpace                           : %d\n", obsId->xSpace);
   printf ("ySpace                           : %d\n", obsId->ySpace);
   printf ("xSubapNb                         : %d\n", obsId->xSubapNb);
   printf ("ySubapNb                         : %d\n", obsId->ySubapNb);
   printf ("xPixels                          : %d\n", obsId->xPixels);
   printf ("yPixels                          : %d\n", obsId->yPixels);
   printf ("pixelsNb                         : %d\n", obsId->pixelsNb);
   printf ("uscanNb                          : %d\n", obsId->uscanNb);
   printf ("oscanNb                          : %d\n", obsId->oscanNb);
   printf ("oscanFlag                        : %d\n", obsId->oscanFlag);
   printf ("xTail                            : %d\n", obsId->xTail);
   printf ("PacketSize                       : %d\n", obsId->packetSize);
   printf ("PacketNb                         : %d\n", obsId->packetNb);
   printf ("fullImageFlag                    : %s\n",
           (obsId->fullImageFlag ? "TRUE" : "FALSE") );
   printf ("binningFlag                      : %s\n",
           (obsId->binningFlag ? "TRUE" : "FALSE") );
   printf ("windowingFlag                    : %s\n",
           (obsId->windowingFlag ? "TRUE" : "FALSE") );
   printf ("x1: %d, x2: %d, y1: %d, y2: %d\n",
            obsId->x1, obsId->x2, obsId->y1, obsId->y2);
   printf ("Time at observation start/end    : %f %f\n", obsId->rawtStart,
           obsId->rawtEnd);
   printf ("Exposure time in seconds         : %f\n", obsId->expTime);
   printf ("Exposure in seconds reqst/actual : %f %f\n", 
           obsId->exposedRQ, obsId->exposed);
   printf ("frameTime                        : %f\n", obsId->frameTime);

   printf ("Axis 1 world coordinate info.    : %s %f %f\n",
      obsId->ctype1, obsId->crpix1, obsId->crval1);
   printf ("Axis 2 world coordinate info.    : %s %f %f\n",
      obsId->ctype2, obsId->crpix2, obsId->crval2);
   printf ("Rotation/skew matrix             : %f %f %f %f\n",
      obsId->cd1_1, obsId->cd1_2, obsId->cd2_1, obsId->cd2_2);
   printf ("Radecsys, equinox, mjd           : %s %f %f\n",
      obsId->radecsys, obsId->equinox, obsId->mjdobs);
   printf ("\n\n");


   printf ("\n");         /* Blank line for spacing */

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detPacketCallback
 *
 *   INVOCATION:
 *   detPacketCallback (sdsuId, obsIdIn, frameId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) sdsuId  (SDSU_ID)      SDSU context structure
 *   (>) obsIdIn (void *)       Observation context pointer cast to void *
 *   (>) pFrame  (SDSU_FRAME *) Frame pointer
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Detector controller packet callback function
 *
 *   DESCRIPTION:
 *   This function will be called each time a packet is received from a
 *   wavefront sensor.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   Doesn't do anything, not used yet either.
 *-
 */

void detPacketCallback
   (
   SDSU_ID        sdsuId,           /* SDSU context structure.                */
   void *         obsIdIn,          /* Observation context structure.         */
   SDSU_FRAME *   pFrame            /* Frame pointer.                         */
   )
{

   printf ("Packet callback\n");

   return;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detFrameCallback
 *
 *   INVOCATION:
 *   detFrameCallback (sdsuId, obsIdIn, pFrame)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) sdsuId  (SDSU_ID)      SDSU context structure
 *   (>) obsIdIn (void *)       Observation context pointer cast to void *
 *   (>) pFrame  (SDSU_FRAME *) Frame pointer
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Detector controller frame callback function
 *
 *   DESCRIPTION:
 *   This function is called each time a frame readout from a wavefront 
 *   sensor is finished.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   Doesn't do anything, not used yet either.
 *-
 */

void detFrameCallback
   (
   SDSU_ID        sdsuId,            /* SDSU context structure.               */
   void *         obsIdIn,           /* Observation context structure.        */
   SDSU_FRAME *   pFrame             /* Frame pointer.                        */
   )
{

   printf ("Frame callback\n");

   return;
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detSimulateData
 *
 *   INVOCATION:
 *   detSimulateData (xPixels, yPixels, option, pFrame)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) xPixels (const int)    Number of pixels in X
 *   (>) yPixels (const int)    Number of pixels in Y
 *   (>) option  (const int)    Simulation option
 *   (!) pFrame  (SDSU_FRAME *) Pointer to SDSU frame
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Fill frame buffer with simulated data (TEMPORARY FUNCTION)
 *
 *   DESCRIPTION:
 *   This function fills a frame buffer with simulated data with the following
 *   options:
 *
 *   Option 1 consists of an incrementing series. The first pixel (output 1)
 *   contains zero, the second pixel (output 2) is one, and so on. Thus within
 *   each quadrant the least significant 2 bits should always be the same for a
 *   4-output device, or the least significant bit the same for 2-output
 *   devices.
 *
 *   Option 2 consists of...
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   It is assumed that pFrame points to an SDSU frame structure (initialised
 *   with sdsuFrameAlloc, sdsuFrameFind and sdsuFrameReserve) containing
 *   sufficient storage space for xPixels * yPixels values.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   Option 2 is very wasteful of CPU. It should be used for small images only.
 *
 *   At the moment this function only simulates a 2x2 array of spots for a CCD
 *   with 2x2 sectors. It can be extended if necessary.
 *-
 */

STATUS detSimulateData
   (
   const int      xPixels,         /* Number of pixels in X.                  */
   const int      yPixels,         /* Number of pixels in Y.                  */
   const int      option,          /* Simulation option.                      */
   SDSU_FRAME *   pFrame           /* Pointer to frame buffer.                */
   )
{
   const int      nPixels = xPixels * yPixels;
                                   /* Total number of pixels.                 */

   const int      nSectors = 4;    /* Number of sectors/outputs.              */
   int            sector;          /* Sector counter.                         */
   int            xPixelsSector;   /* Number of columns per sector.           */
   int            yPixelsSector;   /* Number of rows per sector.              */
   int            i, j;            /* Column and row counters.                */
   int            is, js;          /* Column and row for a particular sector  */

   const int      nSpots = 4;      /* Number of simulated spots.              */
   int            spot;            /* Spot counter.                           */
   int            spotx[4];        /* X coordinates of simulated spot centres */
   int            spoty[4];        /* y coordinates of simulated spot centres */


   double         dist;            /* Distance between pixel and spot centre. */
   double         dvalue;          /* Double valueto write into frame buffer. */
   uint16         value;           /* Integer value to write into frame buffer*/

   volatile uint16 *   ptr;        /* Pointer into frame buffer.              */

   /* Check the frame buffer pointer and size are valid. */

   if (pFrame == NULL)
   {
      ERROR_SET(S_detControl_INTERNAL, "No frame buffer defined", 
                ERROR_LOG_SAVE);
      return (ERROR);
   }

   if ((xPixels <= 0) || (yPixels <= 0 ))
   {
      ERROR_SET2 (S_detControl_BAD_ATTRIBUTE, 
                  "Bad number of pixels given, %d x %d", ERROR_LOG_SAVE,
                  xPixels, yPixels);
      return (ERROR);
   }

#ifdef DEBUG
   printf (
   "detSimulateData: Simulating %d x %d pixels of data to buffer at %p - option %d\n",
   xPixels, yPixels, pFrame, option );
#endif   /* DEBUG */

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

         /* First initialise the number of pixels per sector, 
          * based on the number of sectors. */

         if ( nSectors == 2 )
         {
            /* There are two outputs and therefore 2 sectors in a 2x1 pattern */

            xPixelsSector = xPixels / 2;
            yPixelsSector = yPixels;
         }
         else if ( nSectors == 4 )
         {
            /* There are four outputs and therefore 2 sectors in a 2x2 pattern*/

            xPixelsSector = xPixels / 2;
            yPixelsSector = yPixels / 2;
         }

         /* Now load up the array of spot centroids */

/* Comment out ideal positions
         spotx[0] = xPixels / nSpots;      
         spoty[0] = yPixels / nSpots;
         spotx[1] = spotx[0] * (nSpots-1);
         spoty[1] = spoty[0];
         spotx[2] = spotx[0];
         spoty[2] = spoty[0] * (nSpots-1);
         spotx[3] = spotx[1];
         spoty[3] = spoty[2];
*/

         /* Real positions for 2x2 wavefront sensor */
         spotx[0] = 32;            spoty[0] = 27;
         spotx[1] = 61;            spoty[1] = 25;
         spotx[2] = 25;            spoty[2] = 59;
         spotx[3] = 61;            spoty[3] = 58;

         /*
          * Initialise ptr to the start of the frame pixels and then step
          * through the rows and columns within each sector.
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
                   * Use the row and column coordinates calculated above to
                   * determine the the distance of this point from each spot
                   * centre, calculate the sum of the light from each spot
                   * (assuming a Gaussian distribution), and write
                   * this sum to the location pointed to by ptr. The value is
                   * not allowed to exceed 65535 because it needs to be stored
                   * as an unsigned 16 bit integer. Finally, ptr is incremented.
                   *
                   * The constant factors used in the following equations are
                   * arbitrary.
                   */

                  dvalue = (double) ( 1000 * rand() / RAND_MAX );  
                                                        /* Random background */

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

         ERROR_SET( S_detControl_BAD_ATTRIBUTE, "Unknown simulation option", 
                    ERROR_LOG_SAVE);
         return (ERROR);
         break;
   }

   return (OK);
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detFrameUnscramble
 *
 *   INVOCATION:
 *   detFrameUnscramble (xPixels, yPixels, outputs, inFrame, outBuffer)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) xPixels   (const int)    Number of columns
 *   (>) yPixels   (const int)    Number of rows
 *   (>) outputs   (const int)    Number of detector outputs (2 or 4)
 *   (>) inFrame   (SDSU_FRAME *) Pointer to input frame
 *   (<) outBuffer (float *)      Pointer to output frame buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Unscramble an entire frame of data
 *
 *   DESCRIPTION:
 *   This function takes a raw frame of data containing pixels in the order
 *   they are read from the detector and unscrambles them to generate an output
 *   frame with pixels in the correct order.
 *
 *   ACKNOWLEDGEMENTS:
 *   This function is based around the LeachDeScramble (lds) program provided
 *   by Les Saddlemyer and Tim Hardy, Hertzberg Institute of Astrophysics,
 *   Canada.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   inFrame is a data frame which contains the scrambled SDSU pixels.
 *   outBuffer must point to a buffer large enough to contain at least 
 *   xPixels*yPixels floating point values.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detFrameUnscramble
   (
   const int      xPixels,         /* Number of columns.                      */
   const int      yPixels,         /* Number of rows.                         */
   const int      outputs,         /* Number of detector outputs (2 or 4).    */
   SDSU_FRAME *   inFrame,         /* Pointer to input frame                  */
   float *        outBuffer        /* Pointer to output frame buffer.         */
   )
{
   volatile uint16 *   ptr;        /* Pointer into frame buffer.              */

   int            i, j;            /* Counters.                               */

   int            nPixels;         /* Total number of pixels.                 */

   int            xPixelsSector;   /* Number of columns per sector.           */
   int            yPixelsSector;   /* Number of rows per sector.              */

   volatile uint16 *  inDataPtr;   /* Pointer to start of input data.         */
   float *            outDataPtr;  /* Pointer to start of output data.        */

   float *         ps1;            /* Pointer to beginning of sector 1.       */
   float *         ps2;            /* Pointer to beginning of sector 2.       */
   float *         ps3;            /* Pointer to beginning of sector 3.       */
   float *         ps4;            /* Pointer to beginning of sector 4.       */

#ifdef DEBUG
   float           min, max;       /* Minimum and maximum.                    */
#endif /* DEBUG */

   if ( (inFrame == NULL) || (outBuffer == NULL) )
   {
      ERROR_SET(S_detControl_INTERNAL, 
                "No input and/or output buffers defined", ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf (
   "detFrameUnscramble: Unscrambling %d x %d pixels from frame at %p to %p\n",
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
    * from the detector. If there are two outputs the sectors are arranged 
    * like this
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
         for ( i = 0 ; i < 300 ; i++ )
             printf ( "inPixel[%d]=%d\n", i , *(inDataPtr + i) ) ;
             
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
         printf ("Four sectors of size %d x %d\n", 
                 xPixelsSector, yPixelsSector);
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
         ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, 
                     "Bad number of outputs given, %d", ERROR_LOG_SAVE,
                     outputs);
         return (ERROR);
   }

   return (OK);
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detFrameScramble
 *
 *   INVOCATION:
 *   detFrameScramble (xPixels, yPixels, outputs, inBuffer, outBuffer)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) xPixels   (const int)  Number of columns
 *   (>) yPixels   (const int)  Number of rows
 *   (>) outputs   (const int)  Number of detector outputs (2 or 4)
 *   (>) inFrame   (float *)    Pointer to input frame buffer
 *   (<) outBuffer (uint16 *)   Pointer to output frame buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Scramble an entire frame of data
 *
 *   DESCRIPTION:
 *   This function takes a simulated frame of data and scrambles the pixels
 *   into the order they are read from the detector.
 *
 *   ACKNOWLEDGEMENTS:
 *   This function is based around the LeachDeScramble (lds) program provided
 *   by Les Saddlemyer and Tim Hardy, Hertzberg Institute of Astrophysics,
 *   Canada.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   inBuffer must point to a buffer containing 
 *   xPixels*yPixels floating point values.
 *   outBuffer must point to a buffer large enough to contain at least 
 *   xPixels*yPixels unsigned short integer values.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detFrameScramble
   (
   const int      xPixels,         /* Number of columns.                      */
   const int      yPixels,         /* Number of rows.                         */
   const int      outputs,         /* Number of detector outputs (2 or 4).    */
   float *        inBuffer,        /* Pointer to input frame buffer           */
   uint16 *       outBuffer        /* Pointer to output frame buffer.         */
   )
{

   float *        ptr;             /* Pointer into input frame buffer.        */
   int            i, j;            /* Counters.                               */
   int            nPixels;         /* Total number of pixels.                 */
   int            xPixelsSector;   /* Number of columns per sector.           */
   int            yPixelsSector;   /* Number of rows per sector.              */


   uint16 *       ps1;             /* Pointer to beginning of sector 1.       */
   uint16 *       ps2;             /* Pointer to beginning of sector 2.       */
   uint16 *       ps3;             /* Pointer to beginning of sector 3.       */
   uint16 *       ps4;             /* Pointer to beginning of sector 4.       */

   if ( (inBuffer == NULL) || (outBuffer == NULL) )
   {
      ERROR_SET(S_detControl_INTERNAL, 
      "No input and/or output buffers defined", ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf (
   "detFrameUnscramble: Scrambling %d x %d pixels from frame at %p to %p\n",
   xPixels, yPixels, inBuffer, outBuffer);
#endif /* DEBUG */

   /*
    * The algorithm used to unscramble the data depends on the number of outputs
    * from the detector. If there are two outputs the sectors are arranged 
    * like this
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
         printf ("Four sectors of size %d x %d\n", xPixelsSector, 
                 yPixelsSector);
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
         ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, 
             "Bad number of outputs given, %d", ERROR_LOG_SAVE,
             outputs);
         return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detFrameUnscrambleUint16
 *
 *   INVOCATION:
 *   detFrameUnscrambleUint16 (xPixels, yPixels, outputs, inFrame, outBuffer)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) xPixels   (const int)    Number of columns
 *   (>) yPixels   (const int)    Number of rows
 *   (>) outputs   (const int)    Number of detector outputs (2 or 4)
 *   (>) inFrame   (SDSU_FRAME *) Pointer to input frame
 *   (<) outBuffer (uint16 *)     Pointer to output frame buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Unscramble an entire frame of data
 *
 *   DESCRIPTION:
 *   This function takes a raw frame of data containing pixels in the order
 *   they are read from the detector and unscrambles them to generate an output
 *   frame with pixels in the correct order.
 *   Output data are unsigned short int.
 *
 *   ACKNOWLEDGEMENTS:
 *   This function is based around the LeachDeScramble (lds) program provided
 *   by Les Saddlemyer and Tim Hardy, Hertzberg Institute of Astrophysics,
 *   Canada.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   inFrame is a data frame which contains the scrambled SDSU pixels.
 *   outBuffer must point to a buffer large enough to contain at least 
 *   xPixels*yPixels unsigned short integer.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detFrameUnscrambleUint16
   (
   const int      xPixels,         /* Number of columns.                      */
   const int      yPixels,         /* Number of rows.                         */
   const int      outputs,         /* Number of detector outputs (2 or 4).    */
   SDSU_FRAME *   inFrame,         /* Pointer to input frame                  */
   uint16 *       outBuffer        /* Pointer to output frame buffer.         */
   )
{
   volatile uint16 *   ptr;        /* Pointer into frame buffer.              */

   int            i, j;            /* Counters.                               */

   int            nPixels;         /* Total number of pixels.                 */

   int            xPixelsSector;   /* Number of columns per sector.           */
   int            yPixelsSector;   /* Number of rows per sector.              */

   volatile uint16 *  inDataPtr;   /* Pointer to start of input data.         */
   uint16 *           outDataPtr;  /* Pointer to start of output data.        */

   uint16 *        ps1;            /* Pointer to beginning of sector 1.       */
   uint16 *        ps2;            /* Pointer to beginning of sector 2.       */
   uint16 *        ps3;            /* Pointer to beginning of sector 3.       */
   uint16 *        ps4;            /* Pointer to beginning of sector 4.       */

#ifdef DEBUG
   uint16          min, max;       /* Minimum and maximum.                    */
#endif /* DEBUG */

   if ( (inFrame == NULL) || (outBuffer == NULL) )
   {
      ERROR_SET(S_detControl_INTERNAL, 
                "No input and/or output buffers defined", ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf (
   "detFrameUnscramble: Unscrambling %d x %d pixels from frame at %p to %p\n",
   xPixels, yPixels, inFrame, outBuffer);

   min = USHRT_MAX;
   max = 0;
#endif /* DEBUG */

   /*
    * Set pointers to the start of the data.
    */

   inDataPtr = & inFrame->pixel[0];
   outDataPtr = outBuffer;

   /*
    * The algorithm used to unscramble the data depends on the number of outputs
    * from the detector. If there are two outputs the sectors are arranged 
    * like this
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
               if ( *ptr < min ) min = *ptr;
               if ( *ptr > max ) max = *ptr;
               if ( *(ptr+1) < min ) min = *(ptr+1);
               if ( *(ptr+1) > max ) max = *(ptr+1);
#endif
               /*
                * Change the order here if sectors 1, 2 is
                * different from the order of arrival
                */

               *ps1++ = *ptr++;
               *ps2-- = *ptr++;
            }
            ps1 += xPixelsSector;
            ps2 += xPixelsSector * 3;
         }
#ifdef DEBUG
         printf ("Values range from %d to %d\n", (int)min, (int)max);
#endif /* DEBUG */
         break;

      case (4):

         /* There are four outputs and therefore 4 sectors in a 2x2 pattern. */

         nPixels = xPixels * yPixels;
         xPixelsSector = xPixels / 2;
         yPixelsSector = yPixels / 2;

#ifdef DEBUG
         printf ("Four sectors of size %d x %d\n", 
                 xPixelsSector, yPixelsSector);
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
               if ( *ptr < min ) min = *ptr;
               if ( *ptr > max ) max = *ptr;
               if ( *(ptr+1) < min ) min = *(ptr+1);
               if ( *(ptr+1) > max ) max = *(ptr+1);
               if ( *(ptr+2) < min ) min = *(ptr+2);
               if ( *(ptr+2) > max ) max = *(ptr+2);
               if ( *(ptr+3) < min ) min = *(ptr+3);
               if ( *(ptr+3) > max ) max = *(ptr+3);
#endif
               /*
                * change the order here if sectors 1, 2, 3, 4 is
                * different from the order of arrival
                */

               *ps1++ = *ptr++;
               *ps2-- = *ptr++;
               *ps3-- = *ptr++;
               *ps4++ = *ptr++;
            }
            ps1 += xPixelsSector;
            ps2 += xPixelsSector * 3;
            ps3 -= xPixelsSector;
            ps4 -= xPixelsSector * 3;
         }
#ifdef DEBUG
         printf ("Values range from %d to %d\n", (int)min, (int)max);
#endif /* DEBUG */
         break;

      default:
         ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, 
                     "Bad number of outputs given, %d", ERROR_LOG_SAVE,
                     outputs);
         return (ERROR);
   }

   return (OK);
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detFrameScrambleUint16
 *
 *   INVOCATION:
 *   detFrameScrambleUint16 (xPixels, yPixels, outputs, inBuffer, outBuffer)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) xPixels   (const int)  Number of columns
 *   (>) yPixels   (const int)  Number of rows
 *   (>) outputs   (const int)  Number of detector outputs (2 or 4)
 *   (>) inFrame   (uint16 *)   Pointer to input frame buffer
 *   (<) outBuffer (uint16 *)   Pointer to output frame buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Scramble an entire frame of data
 *
 *   DESCRIPTION:
 *   This function takes a simulated frame of data and scrambles the pixels
 *   into the order they are read from the detector.
 *
 *   ACKNOWLEDGEMENTS:
 *   This function is based around the LeachDeScramble (lds) program provided
 *   by Les Saddlemyer and Tim Hardy, Hertzberg Institute of Astrophysics,
 *   Canada.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   inBuffer must point to a buffer containing 
 *   xPixels*yPixels unsigned short int.
 *   outBuffer must point to a buffer large enough to contain at least 
 *   xPixels*yPixels unsigned short integer values.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detFrameScrambleUint16
   (
   const int      xPixels,         /* Number of columns.                      */
   const int      yPixels,         /* Number of rows.                         */
   const int      outputs,         /* Number of detector outputs (2 or 4).    */
   uint16 *       inBuffer,        /* Pointer to input frame buffer           */
   uint16 *       outBuffer        /* Pointer to output frame buffer.         */
   )
{

   uint16 *        ptr;            /* Pointer into input frame buffer.        */
   int            i, j;            /* Counters.                               */
   int            nPixels;         /* Total number of pixels.                 */
   int            xPixelsSector;   /* Number of columns per sector.           */
   int            yPixelsSector;   /* Number of rows per sector.              */


   uint16 *       ps1;             /* Pointer to beginning of sector 1.       */
   uint16 *       ps2;             /* Pointer to beginning of sector 2.       */
   uint16 *       ps3;             /* Pointer to beginning of sector 3.       */
   uint16 *       ps4;             /* Pointer to beginning of sector 4.       */

   if ( (inBuffer == NULL) || (outBuffer == NULL) )
   {
      ERROR_SET(S_detControl_INTERNAL, 
      "No input and/or output buffers defined", ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf (
   "detFrameUnscramble: Scrambling %d x %d pixels from frame at %p to %p\n",
   xPixels, yPixels, inBuffer, outBuffer);
#endif /* DEBUG */

   /*
    * The algorithm used to unscramble the data depends on the number of outputs
    * from the detector. If there are two outputs the sectors are arranged 
    * like this
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

               *ps1++ = *ptr++;
               *ps2-- = *ptr++;
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
         printf ("Four sectors of size %d x %d\n", xPixelsSector, 
                 yPixelsSector);
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

               *ps1++ = *ptr++;
               *ps2-- = *ptr++;
               *ps3-- = *ptr++;
               *ps4++ = *ptr++;
            }
            ps1 += xPixelsSector;
            ps2 += xPixelsSector * 3;
            ps3 -= xPixelsSector;
            ps4 -= xPixelsSector * 3;
         }
         break;

      default:
         ERROR_SET1 (S_detControl_BAD_ATTRIBUTE, 
             "Bad number of outputs given, %d", ERROR_LOG_SAVE,
             outputs);
         return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detFrameReduceUint16
 *
 *   INVOCATION:
 *   detFrameReduceUint16 ( obsId ) 
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Reduce frame in case of windowing
 *
 *   DESCRIPTION:
 *   In case of windowing, because of the two outputs, the number of read pixels 
 *   can be bigger that the real window requested. This function cancels the 
 *   pixels which are outside the window.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   NONE
 *-
 */

uint32 detFrameReduceUint16
   (
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32         errorNumber;      /* Error number reported by task.         */

   int            row;
   int            col;
   int            xWindowSize;
   int            xWindowDataSize;
   int            yWindowSize;
   int            maxOutput;
   int            size1;
   int            size2;
   int            offset;
   uint16 *       pDisp;
   uint16 *       pCur;
   uint16 *       pc;
   uint16 *       pd;

   /*
    * Check there is valid observation context structure
    */

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised", 
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * Check if the windowingFlag is set
    */

   if ( obsId->windowingFlag == FALSE ) 
   {
      ERROR_SET (S_detControl_BAD_ATTRIBUTE, 
         "The windowing flag has to be set" , ERROR_LOG_NOW);
      errorNumber = S_detControl_BAD_ATTRIBUTE;
      return (errorNumber);
   }

   /*
    * Reduce the frame
    */

   xWindowDataSize = obsId->x2 - obsId->x1 + 1;
   if ( obsId->oscanNb == 0 )
      xWindowSize = xWindowDataSize;
   else
   {
      if ( obsId->oscanFlag == FULL )
         xWindowSize = xWindowDataSize + 2*obsId->oscanNb;
      else
         xWindowSize = xWindowDataSize + obsId->oscanNb;
   }

   yWindowSize = obsId->y2 - obsId->y1 + 1;

   maxOutput = DET_CONTROL_HRWFS_XSIZE / (obsId->xBin * obsId->outputsNb);

   pDisp = obsId->pDispFrame;
   pCur = obsId->pCurFrame;
      
   if ( obsId->x1 <= maxOutput )
   {
      if ( obsId->x2 <= maxOutput )
      {
         for ( row = 0 ; row < yWindowSize ; row ++)
         {
             pd = pDisp + row*obsId->xPixelsDhs;
             pc = pCur + row*obsId->xPixels;

             for ( col = 0 ; col < xWindowDataSize ; col ++)
                 *(pd + col) = *(pc + col);

             for ( col = xWindowDataSize ; col < xWindowSize ; col ++)
                 *(pd + col) = 
                 *(pc + col + 2*maxOutput - obsId->x1 - obsId->x2 + 1);
         }
      }
      else
      {
         size1 = maxOutput - obsId->x1 + 1;
         size2 = obsId->x2 - maxOutput;

         if ( size1 > size2 )
         {
            for ( row = 0 ; row < yWindowSize ; row ++)
            {
                pd = pDisp + row*obsId->xPixelsDhs;
                pc = pCur + row*obsId->xPixels;

                for ( col = 0 ; col < xWindowDataSize ; col ++)
                    *(pd + col) = *(pc + col);

                for ( col = xWindowDataSize ; col < xWindowSize ; col ++)
                    *(pd + col) = *(pc + col + 2*maxOutput - obsId->x2 
                                    - obsId->x1 + 1);
            }
         }
         else
         {
            if ( obsId->xBin == 1 )
               offset = obsId->x1 - obsId->xStart + 16 - 1;
            else
               offset = obsId->x1 - (DET_CONTROL_HRWFS_XSIZE/obsId->xBin - obsId->x2 + 1) ;
            for ( row = 0 ; row < yWindowSize ; row ++)
            {
                pd = pDisp + row*obsId->xPixelsDhs;
                pc = pCur + row*obsId->xPixels + offset;

                for ( col = 0 ; col < xWindowSize ; col ++)
                    *(pd + col) = *(pc + col);
            }
         }
      }
   }
   else
   {
      for ( row = 0 ; row < yWindowSize ; row ++)
      {
          pd = pDisp + row*obsId->xPixelsDhs;
          pc = pCur + row*obsId->xPixels + 
               obsId->x1 + obsId->x2 - 2*maxOutput -1;

          for ( col = 0 ; col < xWindowDataSize ; col ++)
              *(pd + col) = *(pc + col);

          for ( col = xWindowDataSize ; col < xWindowSize ; col ++)
              *(pd + col) = *(pc + col + obsId->oscanNb);
      }
   }
            
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detWriteFitsUint16
 *
 *   INVOCATION:
 *   detWriteFitsUint16 (filename. obsId, xPixels, yPixels, pImageBuffer)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) filename     (char *)    Name of file to contain data.
 *   (>) obsId        (OBS_ID)    Current observation context structure
 *   (>) xPixels      (int)       Number of pixels along X axis
 *   (>) yPixels      (int)       Number of pixels along Y axis
 *   (!) pImageBuffer (uint16 *)  Pointer to image buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Write unsigned short integer data to FITS file (TEMPORARY FUNCTION)
 *
 *   DESCRIPTION:
 *   This function writes the contents of the frame buffer to a FITS file.
 *
 *   ACKNOWLEDGEMENTS:
 *   This function is based on a private function provided by Andrew Johnson.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   It is assumed that pImageBuffer points to a buffer of memory containing
 *   xPixels*yPixels unsigned short integer pixel values.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   This function does not write very good FITS files. It needs to be rewritten
 *   to use the cFitsio library.
 *-
 */

STATUS detWriteFitsUint16
   (
   char *         filename,        /* Name of file to be written.             */
   OBS_ID         obsId,           /* Current observation context structure.  */
   int            xPixels,         /* Number of pixels along X axis.          */
   int            yPixels,         /* Number of pixels along Y axis.          */
   uint16 *       pImageBuffer     /* Pointer to image data.                  */
   )
{

   int            nPixels;           /* Number of pixels.                     */
   int            i;                 /* Counter.                              */
   uint16 *       ptr;               /* Pointer into image buffer.            */
   uint16         value;             /* Value to write into image buffer.     */
   FILE *         fp;                /* File descriptor.                      */

   int            headerCount;       /* Count of header items written.        */

   uint16         fileBuffer[1440];  /* 2880 byte buffer for FITS file.       */
                                     /* [assumes sizeof(uint16)=2].           */
   uint16*        pFileData;
   int            nBlocks;
   int            block;
   int            extra;

   char           telName [40] ;
   char           utStartReduceString [20] ;
   char           utEndReduceString [20] ;

   /*
    * Check the parameters provided.
    */

   if (pImageBuffer == NULL)
   {
      ERROR_SET (S_detControl_INTERNAL, 
                 "No image buffer defined", ERROR_LOG_SAVE);
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
    * Get the telescope name
    */

   wfsGetTelName ( telName ) ;

   /*
    * Reduce strings for utstart and utend
    */

   utStartReduceString[0] = '\0' ;
   utEndReduceString[0] = '\0' ;
   strncat ( utStartReduceString , obsId->utStartString , 19) ;
   strncat ( utEndReduceString , obsId->utEndString , 19) ;

   /*
    * Open and write the fits file
    */

   fp = fopen (filename, "w");

   if (fp == NULL)
   {
      ERROR_SET(0, "Can't create/open FITS file", ERROR_LOG_SAVE);
      return (ERROR);
   }

   headerCount = 0;

   fprintf (fp, "SIMPLE  =                    T /                                                ");

   headerCount++;
   fprintf (fp, "BITPIX  =                   16 /                                                ");
   headerCount++;
   fprintf (fp, "NAXIS   =                    2 /                                                ");
   headerCount++;
   fprintf (fp, "NAXIS1  =                %5d /                                                ", xPixels);
   headerCount++;
   fprintf (fp, "NAXIS2  =                %5d /                                                ", yPixels);
   headerCount++;
   fprintf (fp, "BZERO   =                32768 /                                                ");
   headerCount++;
   fprintf (fp, "BSCALE  =                    1 /                                                ");
   headerCount++;
   fprintf (fp, "EXTEND  =                    T /                                                ");
   headerCount++;
   fprintf (fp, "UTSTART ='%20s'/                                                ", utStartReduceString);
   headerCount++;
   fprintf (fp, "UTEND   ='%20s'/                                                ", utEndReduceString);
   headerCount++;
   fprintf (fp, "EXPTIME =      %15f /                                                ", obsId->expTime);
   headerCount++;
   fprintf (fp, "DARKTIME=      %15f /                                                ", obsId->expTime);
   headerCount++;
   fprintf (fp, "ELAPSED =      %15f /                                                ", (obsId->rawtEnd - obsId->rawtStart));
   headerCount++;
   fprintf (fp, "TELESCOP='%20s'/                                                ", telName);
   headerCount++;
   fprintf (fp, "INSTRUME='%20s'/                                                ", obsId->pWfsName);
   headerCount++;
   fprintf (fp, "OBSERVAT='%20s'/                                                ", telName);
   headerCount++;
   fprintf (fp, "BUNIT   ='%20s'/                                                ", DET_BUNIT);
   headerCount++;
   fprintf (fp, "UNITS   ='%20s'/                                                ", DET_BUNIT);
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
   fprintf (fp, "XBIN    =                %5d /                                                ", obsId->xBin);
   headerCount++;
   fprintf (fp, "YBIN    =                %5d /                                                ", obsId->yBin);
   headerCount++;
   fprintf (fp, "DATASEC ='%20s'/                                                ", obsId->dataSec);
   headerCount++;
   fprintf (fp, "CCDSEC  ='%20s'/                                                ", obsId->ccdSec);
   headerCount++;
   fprintf (fp, "ORIGSEC ='%20s'/                                                ", obsId->origSec);
   headerCount++;
   fprintf (fp, "DETTYPE ='%20s'/                                                ", obsId->detType);
   headerCount++;
   fprintf (fp, "DETID   ='%20s'/                                                ", obsId->detId);
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
    * Subtract 32768 from the image data to counteract the BZERO=32768 in the 
    * FITS header. (This is necessary because FITS readers will assume the 
    * data are signed).
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
   extra   = nPixels % 1440;

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
      for (; i<1440; i++)                  /* Pad remainder of block */
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

   return (OK);
}

/* -------------------------------------------------------------------------- */

void detDhsErrorCallback         /* DHS error callback function.              */
   (
   DHS_CONNECT     connect,      /* DHS connection ID for connection causing  */
                                 /* error.                                    */
   DHS_STATUS      errorNum,     /* DHS error number.                         */
   DHS_ERR_LEVEL   errorLev,     /* DHS error level.                          */
   char *          msg,          /* DHS error message string.                 */
   DHS_TAG         tag,          /* DHS command tag of the error.             */
   void *          userData      /* Pointer to user data (if any).            */
   )
{
   printErr ("DHS error callback: connection=%d errNum=%d level=%d \"%s\"\n",
             (int) connect, (int) errorNum, (int) errorLev, msg);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsInit
 *
 *   INVOCATION:
 *   detDhsInit (pClientName, numConnect, pHostName, pSeverName)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pClientName  (const char *)  Unique name for DHS client.
 *   (>) numConnect   (const int)     Maximum number of DHS connections.
 *   (>) pHostName    (const char *)  Name of DHS data server host.
 *   (>) pServerName  (const char *)  Name of DHS data server.
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Initialise the DHS library and define DHS server information
 *
 *   DESCRIPTION:
 *   This function initialises the DHS library and sets up the DHS server
 *   information used by the detector controller.
 *
 *   EXTERNAL VARIABLES:
 *   (<) detDhsInitialised (BOOL)   DHS initialised flag.
 *   (<) pDetDhsClientName (char *) Current name of DHS client= Instrument name.
 *   (<) pDetDhsHostName   (char *) Current name of DHS server host.
 *   (<) pDetDhsServerName (char *) Current name of DHS server.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   dhs.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *   BUGS:
 *   There is a bug in the DHS event loop which causes it to hang up the
 *   VxWorks crate when an attempt is made to start it. This call is commented
 *   out, which means that a DHS event loop will not be running. Without an
 *   event loop the software must call dhsWait() explicitly to wait for the
 *   completion of each asynchronous command.
 *   SMB - 14 Sep 1998.
 *
 *   It is rumoured that the above bug has been fixed. Event loop call
 *   tentatively restored. SMB - 16 Nov 1998.
 *-
 */

STATUS detDhsInit
   (
   const char *   pClientName,      /* Unique name of DHS client.             */
   const int      numConnect,       /* Maximum number of DHS connections.     */
   const char *   pHostName,        /* Name of data server host.              */
   const char *   pServerName       /* Name of server.                        */
   )
{
   DHS_STATUS      dhsErrno;         /* DHS error number.                  */
   DHS_THREAD      dhsThreadId;      /* DHS thread ID.                     */

   /* Initialise the DHS error number. */

   dhsErrno = DHS_S_SUCCESS;       /* <--- DHS error number initialised here. */

   /*
    * Check the DHS library has not already been initialised.
    */

   if (detDhsInitialised)
   {
      ERROR_SET (S_detControl_DHS_ERROR, "DHS already initialised", 
                 ERROR_LOG_NOW);
      return (ERROR);
   }

   /*
    * Initialise the DHS, specifying a unique name and maximum number 
    * of connections.
    */

#ifdef DEBUG
   printf ("detDhsInit: dhsInit pClientName=%s numConnect=%d\n", 
           pClientName, numConnect);
#endif /* DEBUG */

   dhsInit (pClientName, numConnect, &dhsErrno);
   CHECK_DHS (dhsErrno);

   if (dhsErrno != DHS_S_SUCCESS)
   {
      ERROR_SET1 (S_detControl_DHS_ERROR, 
                  "Failed to initialise DHS (dhsErrno=%d)",
                  ERROR_LOG_SAVE, dhsErrno);
      return (ERROR);
   }

   /* Set up callbacks. */

#ifdef DEBUG
   printf (
   "detDhsInit: dhsCallbackSet DHS_CBT_ERROR=%d detDhsErrorCallback=%p\n",
   DHS_CBT_ERROR, detDhsErrorCallback);
#endif /* DEBUG */

   dhsCallbackSet (DHS_CBT_ERROR, detDhsErrorCallback, &dhsErrno);
   CHECK_DHS (dhsErrno);

   if (dhsErrno != DHS_S_SUCCESS)
   {
      ERROR_SET1 (S_detControl_DHS_ERROR, 
         "Failed to set up DHS error callback (dhsErrno=%d)",
         ERROR_LOG_SAVE, dhsErrno);
      return (ERROR);
   }

   /*
    * Start the DHS event loop.
    *
    * BUG WORK AROUND - THIS CODE COMMENTED OUT - SEE "BUGS" SECTION IN HEADER.
    * REINSTATED - SMB 16 NOV 98
    */

#ifdef DEBUG
   printf ("detDhsInit: dhsEventLoop DHS_ELT_THREADED=%d ... ", 
           DHS_ELT_THREADED);
#endif /* DEBUG */

   dhsEventLoop (DHS_ELT_THREADED, &dhsThreadId, &dhsErrno);
   CHECK_DHS (dhsErrno);

#ifdef DEBUG
   printf ("dhsThreadId=%d dhsErrno=%d\n", dhsThreadId, dhsErrno);
#endif /* DEBUG */

   if (dhsErrno != DHS_S_SUCCESS)
   {
      ERROR_SET1 (S_detControl_DHS_ERROR, 
            "Failed to start DHS event loop (dhsErrno=%d)",
            ERROR_LOG_SAVE, dhsErrno);
      return (ERROR);
   }

   /* Store the given client name, host name and server name in global 
    * variables. 
    */

   strncpy (pDetDhsClientName, pClientName, EPICS_MAX_BYTES_STRING_ATTRIB);
   strncpy (pDetDhsHostName, pHostName, EPICS_MAX_BYTES_STRING_ATTRIB);
   strncpy (pDetDhsServerName, pServerName, EPICS_MAX_BYTES_STRING_ATTRIB);

   /* Finally, set the detDhsInitialised flag and return the semaphore. */

   detDhsInitialised = TRUE;

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsConnect
 *
 *   INVOCATION:
 *   detDhsConnect ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Initialise connection to DHS 
 *
 *   DESCRIPTION:
 *   This function initialises the connection to the DHS.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   detDhsInitialised   (BOOL)        DHS initialised flag
 *   (>)   pDetDhsHostName     (char *)      DHS server host name
 *   (>)   pDetDhsServerName   (char *)      DHS server name
 *   (!)   detDhsConnection    (DHS_CONNECT) DHS connection Id
 *
 *   PRIOR REQUIREMENTS:
 *   The DHS library should already have been initialised by calling detDhsInit.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   dhs.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detDhsConnect
   (
   )
{
   DHS_STATUS     dhsErrno;           /* DHS error number.                    */

   /* Initialise the DHS error number. */

   dhsErrno = DHS_S_SUCCESS;

   /*
    * Check the DHS library has been initialised.
    */

   if (!detDhsInitialised)
   {
      ERROR_SET (S_detControl_DHS_ERROR, "DHS not initialised", ERROR_LOG_NOW);
      detDhsConnected = NOT_INIT;
      return (ERROR);
   }

   /*
    * Connect to the DHS server. There is no user data to be supplied 
    * (hence NULL).
    */

   MESSAGE_LOG2 (MSG_LOG, "Connecting to DHS server %s on host %s",
                 pDetDhsServerName, pDetDhsHostName);

   detDhsConnection = 
   dhsConnect (pDetDhsHostName, pDetDhsServerName, NULL, &dhsErrno);
   CHECK_DHS (dhsErrno);

#ifdef DEBUG
   printf ("dhsConnect: dhsConnection=%ld dhsErrno=%d\n", 
           detDhsConnection, dhsErrno);
#endif /* DEBUG */

   if (dhsErrno != DHS_S_SUCCESS)
   {
      ERROR_SET3 (S_detControl_DHS_ERROR, 
         "Failed to connect to DHS server %s on %s (dhsErrno=%d)",
         ERROR_LOG_SAVE, pDetDhsServerName, pDetDhsHostName, dhsErrno);
      return (ERROR);
   }

   /* Finally, return the semaphore and set the detDhsConnected flag. */

   detDhsConnected = CONNECTED;
   MESSAGE_LOG (MSG_LOG, "Connected to DHS");


   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsCheckErrno
 *
 *   INVOCATION:
 *   detDhsCheckErrno (dhsErrno, line, filename)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) dhsErrno   (const DHS_STATUS)  DHS error number (unchanged)
 *   (>) line       (const int)         Line number to report
 *   (>) filename   (const char *)      File name to report
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Check DHS error number and report any error messages
 *
 *   DESCRIPTION:
 *   This function checks the DHS error number provided. If the status suggests
 *   an error has occurred, the dhsMessage() functions are used to extract
 *   information from the DHS message stack.
 *   This function should be called after every DHS function to ensure all the
 *   relevant DHS errors are reported.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   dhs.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *   BUGS:
 *   This function appears to cause problems with the DHS.
 *   For the time being its contents are commented out and replaced by a
 *   trivial report. SMB - 17 Jan 1999.
 *-
 */

void detDhsCheckErrno
   (
   const DHS_STATUS  dhsErrno,        /* DHS error number.                    */
   const int         line,            /* Line number.                         */
   const char *      filename         /* File name.                           */
   )
{

   /*
    * If the DHS error number is ok, this function will return without doing 
    * anything.
    */

   if ( dhsErrno != DHS_S_SUCCESS )
   {
      errorSet ( line, filename, 0, "DHS error detected", ERROR_LOG_NOW );
   }

   return;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsCheckCmdStatus
 *
 *   INVOCATION:
 *   detDhsCheckCmdStatus (dhsTag)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) dhsTag (const DHS_TAG) DHS command tag (unchanged)
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Check and reports DHS command status 
 *
 *   DESCRIPTION:
 *   This function checks the DHS command status and reports a message if the
 *   status is not DHS_CS_DONE.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   dhs.h
 *
 *   DEFICIENCIES:
 *   The DHS allocates a buffer to store the command status message. It would
 *   be more sensible if the buffer was allocated here and provided to the DHS,
 *   as there would then be more control over the buffer. At the moment the
 *   buffer has to be explicitly freed because the DHS does not do this.
 *   SMB - 17 Jan 1999.
 *
 *   I am informed that dhsMessageClear() should be used to free the message, 
 *   but I still don't understand how dhsMessageClear is supposed to know 
 *   which string to free.
 *   SMB - 15 Feb 1999.
 *
 *   BUG:
 *   Access faults seem to occur when dhsStatus() is used.
 *   For now this function is commented out.
 *   SMB - 29 Jan 1999 + 15 Feb 1999.
 *-
 */

STATUS detDhsCheckCmdStatus
   (
   const DHS_TAG   dhsTag            /* DHS command tag.                      */
   )
{
   DHS_CMD_STATUS  sendStatus;       /* DHS command status.                   */
   DHS_STATUS      dhsErrno;         /* DHS error number.                     */
   char            *msg = NULL;      /* Command status message.               */


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

   sendStatus = DHS_CS_DONE ;      /* FUDGE */

   /*
    * Check that the query worked and report an error if it didn't.
    * If the query returned DHS_CS_DONE nothing more needs to be done.
    * Any other command status is reported as an error.
    */

   if ( dhsErrno != DHS_S_SUCCESS )
   {

      ERROR_SET2 (0, 
         "Failed to query DHS command status for tag %ld, (dhsErrno=%d)",
         ERROR_LOG_SAVE, dhsTag, dhsErrno);

      /* Free the message buffer if allocated. */
      /* if ( msg != NULL ) free (msg); */
      /* Replace with dhsMessageClear(&dhsErrno) ??? */
      return (ERROR);
   }
   else if ( sendStatus != DHS_CS_DONE )
   {
      switch (sendStatus)
      {
         case (DHS_CS_IDLE):

            ERROR_SET1 (0, "Command still waiting to execute, %s", 
                        ERROR_LOG_SAVE, msg);
            break;

         case (DHS_CS_BUSY):

            ERROR_SET1 (0, "Command is still executing, %s", 
                        ERROR_LOG_SAVE, msg);
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
            ERROR_SET2 (0, "Unknown command status, %d, %s", ERROR_LOG_SAVE, 
                        sendStatus, msg);
            break;
      }

      /* Free the message buffer if allocated. */
      /* if ( msg != NULL ) free (msg); */      
      /* Replace with dhsMessageClear(&dhsErrno) ??? */

      return (ERROR);
   }

   return (OK);
}

/* ------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDownloadDefault
 *
 *   INVOCATION:
 *   detDownloadDefault (pWfsName, pRecordPrefix, sdsuId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName      (const char *) Name of wavefront sensor hr
 *   (>) pRecordPrefix (const char *) Record Name prefix
 *   (>) sdsuId        (SDSU_ID)      Current SDSU context structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Download default OMF files
 *
 *   DESCRIPTION:
 *   This function downloads DSP code from the default OMF files. Executed on
 *   startup.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detDownloadDefault
   (
   const char *   pWfsName,         /* Name of wavefront sensor.              */
   const char *   pRecordPrefix,    /* Record Name prefix                     */
   SDSU_ID        sdsuId            /* SDSU context structure.                */
   )
{
   char         pFullOmfFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                                    /* Combined path name and file name.      */
   /* 
    * Variables associated with "Download OMF file" command.
    * (omfPath, vmeFile, timFile and utlFile use general filename parameters)
    */

   BOOL         limitAdrsRange;     /* Flag for limiting addr range in DSP mem*/

   uint32       mode;

   /*
    * Check there is a valid SDSU context structure.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      return (ERROR);
   }

   /*
    * Download default OMF code to the VME DSP, unless the default file name is
    * "NONE" or blank. If the code could not be downloaded, the controller
    * health is set "BAD", since it cannot do anything until this code is
    * downloaded.
    */

   if ( (strcmp (DET_CONTROL_OMF_VME_FILE, "") != 0) &&
        (strcmp (DET_CONTROL_OMF_VME_FILE, "NONE") != 0) )
   {

      /* The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
        * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      sprintf (pFullOmfFileName, "%s/%s", DET_CONTROL_OMF_FILE_PATH, 
               DET_CONTROL_OMF_VME_FILE);
      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to VME DSP...", 
                    pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_VME, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download default OMF file to VME DSP");
         epToVxSetHealth( pRecordPrefix, "BAD" );
         return (ERROR);
      }
   }

   /*
    * Download default OMF code to the TIMING DSP, unless the default file
    * name is "NONE" or blank. If the code could not be downloaded, the
    * controller health is set "BAD", since it cannot do anything until this
    * code is downloaded.
    */


   if ( (strcmp (DET_CONTROL_HRWFS_OMF_TIM_FILE, "") != 0) &&
        (strcmp (DET_CONTROL_HRWFS_OMF_TIM_FILE, "NONE") != 0) )
   {

      /* The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      sprintf (pFullOmfFileName, "%s/%s", DET_CONTROL_OMF_FILE_PATH,
               DET_CONTROL_HRWFS_OMF_TIM_FILE);
      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to TIMING DSP...", 
                    pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_TIM, 
                          limitAdrsRange) == ERROR)
      {
         ERROR_LOG ("Failed to download default OMF file to TIMING DSP");
         epToVxSetHealth( pRecordPrefix, "BAD" );
         return (ERROR);
      }
   }

   /*
    * Download default OMF code to the UTILITY DSP, unless the default file
    * name is "NONE" or blank. If the code could not be downloaded, the
    * controller health is set "BAD", since it cannot do anything until this
    * code is downloaded.
    */

   if ( (strcmp (DET_CONTROL_OMF_UTL_FILE, "") != 0) &&
        (strcmp (DET_CONTROL_OMF_UTL_FILE, "NONE") != 0) )
   {

      /* The ability to limit the address range is ignored. It is rarely needed
       * and can only be done by executing sdsuFileDnload at the console (since
       * sdsuFileDnload expects to prompt for the values).
       */

      limitAdrsRange = 0;

      sprintf (pFullOmfFileName, "%s/%s", DET_CONTROL_OMF_FILE_PATH, 
               DET_CONTROL_OMF_UTL_FILE);
      MESSAGE_LOG1 (MSG_LOG, "Downloading OMF file %s to UTILITY DSP...", 
                    pFullOmfFileName);

      if (sdsuFileDnload (sdsuId, pFullOmfFileName, SDSU_IDENT_UTL, 
                          limitAdrsRange) == ERROR)
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

   /*
    * After successfully downloading new OMF code, the controller must be
    * reinitialised by sending an "INI" command to the utility DSP and a
    * "LDP" command to the timing DSP.
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


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detCheckGeometry
 *
 *   INVOCATION:
 *   detCheckGeometry (pWfsName, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName (const char *) Name of wavefront sensor hr
 *   (>) sdsuId   (SDSU_ID)      Current SDSU context structure
 *   (<) obsId    (OBS_ID)       Observation context structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Check and update default detector geometry
 *
 *   DESCRIPTION:
 *   This function compares the default detector geometry contained in xPixels,
 *   yPixels in obsId with the default parameters defined by the SDSU DSP code and
 *   ensures that on exit xPixels, yPixels contain the maximum expected detector
 *   geometry. Executed on startup.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detCheckGeometry
   (
   const char *  pWfsName,        /* Name of wavefront sensor.                */
   SDSU_ID       sdsuId,          /* SDSU context structure.                  */
   OBS_ID        obsId            /* Observation context structure.           */
   )
{
   uint32        xRas;            /* SDSU "number of X super pixels per       */
                                  /* subaperture" parameter (T_XRAS).         */
   uint32        yRas;            /* SDSU "number of Y super pixels per       */
                                  /* subaperture" parameter (T_YRAS).         */
   uint32        xSubap;          /* SDSU number of X subapertures per output */
                                  /* parameter (T_XSUBAP).                    */
   uint32        ySubap;          /* SDSU number of Y subapertures per output */
                                  /* parameter (T_YSUBAP).                    */
   uint32        outputs;         /* SDSU "number of outputs " parameter      */
                                  /* (T_OUTPUTS).                             */
   uint32        xChip;           /* Maximum X pixels per output (T_XSIZE).   */
   uint32        yChip;           /* Maximum Y pixels per output (T_YSIZE).   */
   uint32        pSize;           /* Packet size in pixels.                   */
   int           nPackets;        /* Number of packets expected per frame.    */

   int           xPixelsOutput;   /* Number of X super pixels per output.     */
   int           yPixelsOutput;   /* Number of Y super pixels per output.     */
   int           dspxPixels;      /* Number of X pixels expected by DSP code. */
   int           dspyPixels;      /* Number of Y pixels expected by DSP code. */
   int           dspxMax;         /* Maximum X pixels expected by DSP code.   */
   int           dspyMax;         /* Maximum Y pixels expected by DSP code.   */
   uint32        uscan;           /* Underscan pixel (T_USCAN)                */

   /*
    * Check there is a valid SDSU context structure.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      return (ERROR);
   }

   /*
    * In simulation mode no DSP code will have been downloaded, and nothing
    * needs to be checked.
    */

   if ( !sdsuId->simulate )
   {

      /*
       * Obtain the xChip, yChip, xRas, yRas, xSubap, ySubap and number of
       * outputs parameters from the SDSU controller and use these to calculate
       * the default size expected by the DSP code.
       */

      if ( (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XSIZE", &xChip) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YSIZE", &yChip) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XRAS", &xRas) == ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YRAS", &yRas) == ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XSUBAP", &xSubap) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YSUBAP", &ySubap) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_OUTPUTS", &outputs) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_VME, "V_PSIZE", &pSize) ==
            ERROR) ||
           (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_USCAN", &uscan) == 
            ERROR)
         )
      {
         ERROR_SET (0,
"Failed to read T_XSIZE, T_YSIZE, T_XRAS, T_YRAS, T_XSUBAP, T_YSUBAP, T_OUTPUTS, V_PSIZE, USCAN parameters",
            ERROR_LOG_SAVE);
         return (ERROR);
      }

      obsId->xSize = xChip ;
      obsId->ySize = yChip ;
      obsId->outputsNb = outputs ;
      obsId->packetSize = pSize ;
      obsId->uscanNb = (int)uscan ;
      obsId->xSubapNb = xSubap ;
      obsId->ySubapNb = ySubap ;
      obsId->xRaster = xRas ;
      obsId->yRaster = yRas ;
      
      /*
       * xPixelsOutput=(xRas*xSubap) and yPixelsOutput=(yRas*ySubap) represent 
       * the number of pixels per output. The arrangement depends on the number 
       * of outputs.
       * In this case, there are two outputs the sectors generated from each 
       * output are arranged like this
       *
       *   +------------+------------+
       *   |  sector 1  |  sector 2  |
       *   0----->------+-----<------0
       *
       * "0" shows the origin of each sector and ">" the direction of readout.
       */

      xPixelsOutput = (int) (xRas * xSubap);
      yPixelsOutput = (int) (yRas * ySubap);

      dspxPixels = xPixelsOutput * 2;
      dspyPixels = yPixelsOutput;
      dspxMax    = (int) xChip * 2;
      dspyMax    = (int) yChip;

      /*
       * Compare the default detector size downloaded in the DSP code with
       * xMax and yMax and increase if necessary. Replace the current xPixels
       * and yPixels with that found in the DSP code.
       */

      MESSAGE_LOG4 (MSG_FULLDEBUG,
         "detControl assumed detector size (%d,%d); DSP code assumed (%d,%d)",
         obsId->xMax, obsId->yMax, dspxMax, dspyMax);

      MESSAGE_LOG4 (MSG_FULLDEBUG,
         "detControl assumed readout size (%d,%d); DSP code assumed (%d,%d)",
         obsId->xPixels, obsId->yPixels, dspxPixels, dspyPixels);

      if ( dspxMax > obsId->xMax )
      {
         MESSAGE_LOG2 (MSG_LOG, 
            "Maximum number of X pixels increased from %d to %d\n",
            obsId->xMax, dspxMax);
         obsId->xMax = dspxMax;
      }

      if ( dspyMax > obsId->yMax )
      {
         MESSAGE_LOG2 (MSG_LOG, 
            "Maximum number of Y pixels increased from %d to %d\n",
            obsId->yMax, dspyMax);
         obsId->yMax = dspyMax;
      }

      if ( dspxPixels != obsId->xPixels )
      {
         MESSAGE_LOG2 (MSG_LOG, 
            "Default number of X pixels changed from %d to %d\n",
            obsId->xPixels, dspxPixels);
         obsId->xPixels = dspxPixels;
      }

      if ( dspyPixels != obsId->yPixels )
      {
         MESSAGE_LOG2 (MSG_LOG, 
            "Default number of Y pixels changed from %d to %d\n",
            obsId->yPixels, dspyPixels);
         obsId->yPixels = dspyPixels;
      }

      obsId->pixelsNb = (obsId->xPixels) * (obsId->yPixels) ;
      
      /*
       * Update the expected number of packets per frame using the number of
       * pixels read from the controller.
       */

      if ( pSize > 0 )
      {
         nPackets = 
         (int)ceil ( (double) (outputs * xPixelsOutput * yPixelsOutput) / (double) pSize );
      }
      else
      {
         nPackets = 1;
      }

      sdsuId->packetsPerFrame = nPackets;
      obsId->packetNb = nPackets;
      /*printf ("xMax=%d, yMax=%d, nPackets=%d\n", obsId->xMax, obsId->yMax, nPackets ) ;*/
   }
   else
   {
      /* In simu. mode the nb of packets per frame needs to be init. to 1. */

      sdsuId->packetsPerFrame = 1;
      obsId->packetNb = 1;
   }

   MESSAGE_LOG2 (MSG_FULLDEBUG, 
      "Each frame will consist of %d packets of %lu pixels each",
      nPackets, pSize);

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detCopyGeometry
 *
 *   INVOCATION:
 *   detCopyGeometry (pWfsName, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pWfsName    (const char *)          Name of wavefront sensor hr
 *   (>) sdsuId      (SDSU_ID)               Current SDSU context structure
 *   (<) obsId       (OBS_ID)                Observation context structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Copy detector geometry to obsId structure
 *
 *   DESCRIPTION:
 *   This function copies the current detector geometry to the obsId  
 *   structure.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS detCopyGeometry
   (
   const char *         pWfsName,         /* Name of wavefront sensor.        */
   SDSU_ID              sdsuId,           /* SDSU context structure.          */
   OBS_ID               obsId             /* Observation context structure.   */
   )
{
   uint32         xSdsuStart;     /* SDSU parameter (T_XSTART).               */
   uint32         ySdsuStart;     /* SDSU parameter (T_YSTART).               */
   uint32         xSdsuBin;       /* SDSU parameter (T_XBIN).                 */
   uint32         ySdsuBin;       /* SDSU parameter (T_YBIN).                 */
   uint32         xSdsuSpace;     /* SDSU parameter (T_XSPACE).               */
   uint32         ySdsuSpace;     /* SDSU parameter (T_YSPACE).               */

   /*
    * Check there is a valid SDSU context structure.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised", 
                 ERROR_LOG_NOW);
      return (ERROR);
   }

   /*
    * In simulation mode the SDSU parameters will not have sensible values
    * when read back.
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

   if ( (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XSTART", &xSdsuStart) ==
         ERROR) ||
        (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YSTART", &ySdsuStart) ==
         ERROR) ||
        (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XBIN", &xSdsuBin) == ERROR) ||
        (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YBIN", &ySdsuBin) == ERROR) ||
        (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_XSPACE", &xSdsuSpace) ==
         ERROR) ||
        (sdsuParamRead (sdsuId, SDSU_IDENT_TIM, "T_YSPACE", &ySdsuSpace) ==
         ERROR)
      )
   {
      ERROR_SET (0, "Failed to read SDSU geometry parameters", ERROR_LOG_SAVE);
      return (ERROR);
   }
   /* 
    * Init obsId with these values 
    */
   
   obsId->xStart = xSdsuStart ;
   obsId->yStart = ySdsuStart ;
   obsId->xBin = xSdsuBin ;
   obsId->yBin = ySdsuBin ;
   obsId->xSpace = xSdsuSpace ;
   obsId->ySpace = ySdsuSpace ;
    
   /*
    * Set the flag parameters full ImageFlag, binningFlag, windowingFlag.
    */

   if ( ( obsId->xBin != 1 ) || ( obsId->yBin != 1) )
      obsId->binningFlag = TRUE ;
   else
      obsId->binningFlag = FALSE ;

   if ( (obsId->xSubapNb == 1) && (obsId->ySubapNb == 1) && 
        (obsId->xStart == 16) && (obsId->yStart == 1) &&
        (obsId->xSpace == 0) && (obsId->ySpace == 0) )
   {
      obsId->windowingFlag = FALSE;
   }
   else
   {
      obsId->windowingFlag = TRUE;
   }

   obsId->x1 = obsId->xStart - 15 ; /* -16 + 1 */
   obsId->x2 = obsId->xPixels ;
   obsId->y1 = obsId->yStart ;
   obsId->y2 = obsId->yPixels ;

   if ( (obsId->binningFlag == TRUE) || (obsId->windowingFlag == TRUE) )
      obsId->fullImageFlag = FALSE ;
   else
      obsId->fullImageFlag = TRUE ;

   /*
    * Set the overscan region to default
    */

   obsId->oscanNb = DET_CONTROL_HRWFS_OSCAN_SIZE;
   obsId->oscanFlag = FALSE;

#ifdef DEBUG
   /*
    * Show CCD Geometry information
    */

   printf ( "CCD Geometry information from obsId \n" ) ;
   printf ( "Outputs number : %d\n" , (int) obsId->outputsNb ) ;
   printf ( "xSize (per output) : %d\n" , obsId->xSize ) ;
   printf ( "ySize (per output) : %d\n" , obsId->ySize ) ;
   printf ( "xMax : %d\n" , obsId->xMax ) ;
   printf ( "yMax : %d\n" , obsId->yMax ) ;
   printf ( "xStart : %d\n" , obsId->xStart ) ;
   printf ( "yStart : %d\n" , obsId->yStart ) ;
   printf ( "xBin : %d\n" , obsId->xBin ) ;
   printf ( "yBin : %d\n" , obsId->yBin ) ;
   printf ( "xRaster : %d\n" , obsId->xRaster ) ;
   printf ( "yRaster : %d\n" , obsId->yRaster ) ;
   printf ( "xSpace : %d\n" , obsId->xSpace ) ;
   printf ( "ySpace : %d\n" , obsId->ySpace ) ;
   printf ( "xSubap number : %d\n" , obsId->xSubapNb ) ;
   printf ( "ySubap number : %d\n" , obsId->ySubapNb ) ;
   printf ( "xPixels : %d\n" , obsId->xPixels ) ;
   printf ( "yPixels : %d\n" , obsId->yPixels ) ;
   printf ( "pixels number : %d\n" , obsId->pixelsNb ) ;
   printf ( "uscan number : %d\n" , obsId->uscanNb ) ;
   printf ( "oscan number : %d\n" , obsId->oscanNb ) ;
   printf ( "xTail : %d\n" , obsId->xTail ) ;
   printf ( "packet size : %d\n" , obsId->packetSize ) ;
   if ( obsId->fullImageFlag == TRUE )
      printf ( "FullImageFlag : TRUE\n" ) ;
   else
      printf ( "FullImageFlag : FALSE\n" ) ;
   if ( obsId->binningFlag == TRUE )
      printf ( "binningFlag : TRUE\n" ) ;
   else
      printf ( "binningFlag : FALSE\n" ) ;
   if ( obsId->windowingFlag == TRUE )
      printf ( "windowingFlag : TRUE\n" ) ;
   else 
      printf ( "windowingFlag : FALSE\n" ) ;
   printf ( "x1=%d, x2=%d\n" , obsId->x1, obsId->x2 ) ;
   printf ( "y1=%d, y2=%d\n" , obsId->y1, obsId->y2 ) ;
#endif

   return (OK);
}

/* -------------------------------------------------------------------------- */

/* This function is purely an engineering fudge to reset the "observing" flag
 * if it screws up.
 */

void detPokeObserving
   (
   OBS_ID    obsId,
   BOOL      newValue
   )
{

   obsId->observing = newValue;

   return;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detCreateFileName
 *
 *   INVOCATION:
 *   detCreateFileName (pFilePath, pOutFileName, pFullOutFileName)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pFilePath        (char *)  Pointer to the file path name         
 *   (>) pOutFileName     (char *)  Pointer to the output file name       
 *   (<) pFullOutFileName (char *)  Pointer to the combined path and file name 
 *
 *   FUNCTION VALUE:
 *   (uint32)	always OK
 *
 *   PURPOSE:
 *   Combine path and file name 
 *
 *   DESCRIPTION:
 *   Combine path and file name and cancel the .fits at the end if this 
 *   one exists
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 * 
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS detCreateFileName 
   ( 
   char *   pFilePath,         /* Pointer to the file path name               */
   char *   pOutFileName,      /* Pointer to the output file name             */
   char *   pFullOutFileName   /* Pointer to the combined path and file name  */
                               /* Size of path and file name is               */
                               /* EPICS_MAX_BYTES_STRING_ATTRIB + 1           */
                               /* Size of full file name is                   */
                               /* 2*(EPICS_MAX_BYTES_STRING_ATTRIB + 1)       */
   )
{
   char     firstPartOutFileName [ EPICS_MAX_BYTES_STRING_ATTRIB + 1 ] ;
   char     lastCharOutFileName [ EPICS_MAX_BYTES_STRING_ATTRIB + 1 ] ;
   int      sizeOutFileName ;
   int      sizeFits ;
   int      i, j ;

   sizeFits = strlen ( ".fits" ) ;

   /* Check if pOutFileName contains a string */

   if ( strcmp ( pOutFileName, "" ) == 0 )
   {
      /* Default file name hrwfs.fits */

      if ( strcmp ( pFilePath, "" ) == 0 )
         strcpy ( pFullOutFileName, "hrwfs" ) ;
      else
         sprintf ( pFullOutFileName, "%s/hrwfs" , pFilePath ) ;
      
      return ( OK ) ;
   }

   /* Check if pOutFileName contains the string .fits */

   if ( strstr ( pOutFileName, ".fits" ) != NULL )
   {
      sizeOutFileName = strlen ( pOutFileName ) ;

      if ( sizeOutFileName < sizeFits )
         strncpy ( firstPartOutFileName , pOutFileName , 
                   EPICS_MAX_BYTES_STRING_ATTRIB ) ;
      else
      {
         /* Check if the last 5 char are .fits */
         i = 0 ;
	 for ( j = sizeOutFileName - sizeFits ; j < sizeOutFileName ; j ++ )
	 {
	     lastCharOutFileName[i] = pOutFileName[j];
	     i ++ ;
	 }
	 lastCharOutFileName [i] = '\0' ;

	 if ( strcmp ( lastCharOutFileName , ".fits" ) == 0 )
	 {
            /* Read the first part of pOutFileName witout .fits */
	    for ( j = 0 ; j < sizeOutFileName - sizeFits ; j ++ )
	    {
                firstPartOutFileName[j] = pOutFileName[j];
	    }
	    firstPartOutFileName [j] = '\0' ;
	 }
	 else
	 {
            strncpy ( firstPartOutFileName , pOutFileName , 
                      EPICS_MAX_BYTES_STRING_ATTRIB ) ;
	 }

      }
   }
   else
   {
      strncpy ( firstPartOutFileName , pOutFileName , 
                EPICS_MAX_BYTES_STRING_ATTRIB ) ;
   }

   /* Now combine firstPartOutFileName and pFilePath */

   if ( strcmp ( pFilePath , "" ) == 0 )
      strncpy ( pFullOutFileName, firstPartOutFileName, 
                EPICS_MAX_BYTES_STRING_ATTRIB ) ;
   else
      sprintf ( pFullOutFileName, "%s/%s" , pFilePath , firstPartOutFileName ) ;

   return ( OK ) ;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detReadFitsHeaderInt
 *
 *   INVOCATION:
 *   detReadFitsHeaderInt (fileName, nKey, keyName, keyVal)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) fileName     (char *)    Name of FITS file whose header is being read
 *   (>) nKey         (int)       Maximum number of keywords to be read
 *   (>) keyName      (char **)   Name of array of keywords
 *   (<) keyVal       (int *)     Array of integer values of keywords
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read the integer values of an array of keywords from a FITS header
 *
 *   DESCRIPTION:
 *   Uses functions from the cfitsio library to open a FITS file for reading,
 *   and read the values of specified keywords with integer values. The keywords
 *   elements of an array, and their values are read into the corresponding
 *   elements of an array of integers. The number of elements of the keyword
 *   array may exceed the number of keywords actually present, but obviously not
 *   the number of array elements allocated. The keywords should be consecutive
 *   elements of their array, as reading of keywords will end when a NULL value
 *   is encountered as a keyword. The file is closed when reading is finished.
 *
 *   ACKNOWLEDGEMENTS:
 *   This function is based on a private function provided by Steven Heddle, 
 *   UKATC, Edinburgh 18/1/1999
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   Restricted to keywords with integer values.
 *-
 */

STATUS detReadFitsHeaderInt
   (
   char *         fileName,          /* Name of file to be read.              */
   int            nKey,              /* Maximum number of keywords to be read */
   char **        keyName,           /* Name of array of keywords             */
   int *          keyVal             /* Array of integer values of keywords   */
   )
{
   fitsfile *     fp;                /* FITS File descriptor.                 */
   int            fitsStatus;        /* FITS status used by the fits function */
   int            i;                 /* Counter.                              */
   char           comment [80] ;     /* Comment buffer read from FITS file    */

   /*
    * Set to zero the FITS status
    */

   fitsStatus = 0 ;

   /*
    * Open the FITS file
    */

   if ( fits_open_file ( &fp, fileName, FITSIO_READONLY, &fitsStatus) )
   {
      ERROR_SET2(0, "Can't open FITS file %s: %d", ERROR_LOG_SAVE, fileName, 
                 fitsStatus);
      return (ERROR);
   }

   /*
    * Read the keywords array and write their values into the corresponding 
    * int array
    */

   i = 0 ;

   while ( (keyName[i] != NULL) && (i < nKey) )
   {
      if ( fits_read_key(fp, TINT, keyName[i], keyVal+i, comment, &fitsStatus) )
      {
         ERROR_SET2(0, "Failed on reading %s keyword: %d", ERROR_LOG_SAVE, 
                    keyName[i], fitsStatus);
      }
      i++;
   }

   /* 
    * Close the FITS file
    */

   if (fits_close_file (fp, &fitsStatus))
   {
      ERROR_SET1(0, "Problem closing FITS file : %d", ERROR_LOG_SAVE, fitsStatus);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detReadFitsImageUint16
 *
 *   INVOCATION:
 *   detReadFitsImageUint16 (pImageBuffer, fileName, buffSize)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (<) pImageBuffer (uint16 *) Pointer to buffer for image read in
 *   (>) fileName     (char *)   Name of FITS file whose image is being read
 *   (>) buffSize     (int)      Size of the image buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Read the unsigned short int image into a buffer from a FITS file
 *
 *   DESCRIPTION:
 *   The FITS file is opened and the NAXIS keywords read to get the image
 *   size. If the size is greater than buffsize, only enough of the image to
 *   fill the buffer is read in. If the image is smaller than or equal to the
 *   size of the buffer, the whole image is read in. No padding to fill any
 *   unassigned elements of the buffer takes place, as the image dimensions for
 *   any subsequent processing should be strictly controlled to match the
 *   xframesize and yframesize dimensions specified in the context structure.
 *   The FITS file is then closed.
 *
 *   ACKNOWLEDGEMENTS:
 *   This function is based on a private function provided by Steven Heddle, 
 *   UKATC, Edinburgh 18/1/1999
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   The buffer pointed to by pImageBuffer has been allocated large enough to
 *   accomodate buffSize unsigned short ints
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS detReadFitsImageUint16
   (
   uint16 *       pImageBuffer, /* Pointer to buffer for image read in     */
   char *         fileName,     /* Name of file to be read.                */
   int            buffSize      /* Size of the image buffer                */
   )
{
   fitsfile *     fp;           /* FITS File descriptor.                   */
   int            fitsStatus;   /* FITS status used by the fits function   */
   int            nFound;       /* Number of keywords founds               */
   long           nAxes[2];     /* Array of keyword NAXIS values           */
   long           nPixels;      /* Number of pixels of the image           */
   long           nElemRead;    /* Number of pixels read                   */
   long           firstPixel;    /* Number of pixels read                   */
   uint16         nullval;      /* Value for undefined pixels when reading */
   int            anynull;      /* Set to 1 if any values are null; else 0 */

   /*
    * Set to zero the FITS status
    */

   fitsStatus = 0 ;

   /*
    * Open the FITS file
    */

   if ( fits_open_file ( &fp, fileName, FITSIO_READONLY, &fitsStatus) )
   {
      ERROR_SET2(0, "Can't open FITS file %s: %d", ERROR_LOG_SAVE, fileName, 
                 fitsStatus);
      return (ERROR);
   }

   /*
    * Read the keywords NAXIS1 and NAXIS2 to get image size
    */

   if ( fits_read_keys_lng(fp, "NAXIS", 1, 2, nAxes, &nFound, &fitsStatus) )
   {
      ERROR_SET1(0, "Failed to read keywords NAXIS: %d", ERROR_LOG_SAVE, 
                 fitsStatus);

      if (fits_close_file (fp, &fitsStatus))
      {
         ERROR_SET1(0, "Problem closing FITS file: %d", ERROR_LOG_SAVE, fitsStatus);
      }

      return (ERROR) ;
   }

   nPixels = nAxes[0] * nAxes[1];

   /*
    * Check the image size in comparison to the buffer size 
    */

   if ( buffSize < nPixels )
      nElemRead = buffSize ;
   else
      nElemRead = nPixels ;

   /*
    * Read the image 
    */

   firstPixel = 1;
   nullval = 0;           /* don't check for null values in the image */

   /* Note that even though the FITS images contains unsigned integer */
   /* pixel values (or more accurately, signed integer pixels with    */
   /* a bias of 32768),  this routine is reading the values into a    */
   /* float array.Cfitsio automatically performs the datatype         */
   /* conversion in cases like this.                                  */

   if ( fits_read_img (fp, TUSHORT, firstPixel, nElemRead, &nullval,
                       pImageBuffer, &anynull, &fitsStatus) )
   {
      ERROR_SET1(0, "Failed to read image: %d", ERROR_LOG_SAVE, 
                 fitsStatus);

      if (fits_close_file (fp, &fitsStatus))
      {
         ERROR_SET1(0, "Problem closing FITS file: %d", ERROR_LOG_SAVE, fitsStatus);
      }

      return (ERROR) ;
   }

   /* 
    * Close the FITS file
    */

   if (fits_close_file (fp, &fitsStatus))
   {
      ERROR_SET1(0, "Problem closing FITS file: %d", ERROR_LOG_SAVE, fitsStatus);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detHeadTempGet
 *
 *   INVOCATION:
 *   detReadFitsImageUint16 (struct sirRecord *psir)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (<) psir (struct sirRecord *) Pointer to headTemp sir record
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Write the temperature of the CCD into the SIR record
 *
 *   DESCRIPTION:
 *   Fot this sir record, I have decided to use Epics facilities and not epToVxLib.
 *   Faster and simpler.
 *
 *   EXTERNAL VARIABLES:
 *   None. 
 *
 *   PRIOR REQUIREMENTS:
 *   external variables :detSdsuIdHr, detObsIdHr
 *
 *   INCLUDE FILES:
 *   detControl.h
 *   fitsio.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS detHeadTempGet
   (
   struct sirRecord *       psir /* Pointer to "headTemp" sir record       */
   )
{
   uint32   value ;

   int      sample ;

   double   meanValue6, meanValue7;
   double   sdsuTemp6, sdsuTemp7, sdsuTemp;

   if ( detObsIdHr == NULL )
   {
      return (ERROR);
   }

   if ( detSdsuIdHr == NULL )
   {
      return (ERROR);
   }

   if ( ( detObsIdHr->observing != TRUE ) && ( readTempReadyFlag != FALSE ) )
   {
      meanValue6 = meanValue7 = 0.0;
      for ( sample=0; sample<1; sample++)
      {
         if (sdsuParamRead (detSdsuIdHr, SDSU_IDENT_UTL, "U_ADC6", &value) == ERROR)
         {
            ERROR_LOG ("Failed to read thermistor 1 temperature parameter");
            return (ERROR);
         }
         else
         {
            meanValue6 += (double) value;
         }

         if (sdsuParamRead (detSdsuIdHr, SDSU_IDENT_UTL, "U_ADC7", &value) == ERROR)
         {
            ERROR_LOG ("Failed to read thermistor 2 temperature parameter");
            return (ERROR);
         }
         else
         {
            meanValue7 += (double) value;
         }
      }

      /*meanValue6 /= 20.0;
      meanValue7 /= 20.0;*/

      sdsuTemp6 = meanValue6 * (-0.01545); /* 0.01545 is not quite SDSU_TEMP_UNIT*/
      sdsuTemp7 = meanValue7 * (-0.01545); /* 0.01545 is not quite SDSU_TEMP_UNIT*/

      sdsuTemp = (sdsuTemp6 + sdsuTemp7) / 2.0 ;

#ifdef DEBUG
      printf ( "detHeadTempGet() : not observing -> val = %f\n" , sdsuTemp ) ;
#endif
      *(double *)psir->val = sdsuTemp ;
   }
#ifdef DEBUG
   else
   {
      printf ( "detHeadTempGet() observing then wait...\n" ) ;
   }
#endif

   return (OK) ;
}
  
/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsReconnect
 *
 *   INVOCATION:
 *   detDhsReconnect (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (!) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detDhsReconnect command
 *
 *   DESCRIPTION:
 *   This function disconnects or reconnects to the dhs .
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *-
 */

uint32 detDhsReconnect
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */

   DHS_STATUS      dhsErrno;      /* DHS error number.                        */

   long            connect;


   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) &connect);

   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can be used when an observation is not in progress
    */

   if ( obsId->observing )
   {
      ERROR_SET (S_detControl_BUSY,
         "Observation in progress - abort observation and try again",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_BUSY;
      return (errorNumber);
   }

   /*
    * Check wether it is connect or disconnect command
    */

   if ( connect == 0 ) /* disconnect requested */
   {
      if ( detDhsConnected == CONNECTED )
      {
         dhsErrno = 0;
         dhsDisconnect (detDhsConnection, &dhsErrno);
         CHECK_DHS (dhsErrno);
         if ( dhsErrno == DHS_S_SUCCESS )
         {
            detDhsConnected = NOT_CONNECTED;
            MESSAGE_LOG (MSG_LOG, "Disconnected to DHS");
         }
      }
   }
   else                /* connect requested */
   {
      if ( detDhsConnected == NOT_CONNECTED )
      {
         if ( detDhsConnect () == ERROR )
         {
            ERROR_SET (0, "Can't reconnect to the dhs", ERROR_LOG_NOW);
            return (ERROR);
         }
      }
   }

   /*
    * Now report to the SIR record 
    */

   if ( detDhsConnected == CONNECTED )
   {
      if (epToVxPipeWrite (NULL, "CONNECTED", obsId->pDhsConContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to initialise DET_CONTROL_DHSCON_SIR_NAME record");
         errorNumber = ERROR;
      }
   };

   if ( detDhsConnected == NOT_CONNECTED )
   {
      if (epToVxPipeWrite (NULL, " NOT CONNECTED", obsId->pDhsConContext)
          == ERROR)
      {
         ERROR_LOG (
         "Failed to initialise DET_CONTROL_DHSCON_SIR_NAME record");
         errorNumber = ERROR;
       }
   };

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detContInit
 *
 *   INVOCATION:
 *   detContInit (pInitFileName, pTempCode, pTempCoeff, pOffsetFullVect, 
 *                pOffsetBinVect, pCcdSn)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pInitFileName   (char *)   Init file Name
 *   (>) pTempCode       (uint32 *) Target temperature code
 *   (>) pTempCoeff      (uint32 *) Coefficient for temperature control
 *   (>) pOffsetFullVect (long *)   ADC offset vector when no binning [2]
 *   (>) pOffsetBinVect  (long *)   ADC offset vector when binning [2]
 *   (>) pCcdSn          (char *)   CCD serial number
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Init defaults values for the detector controller
 *
 *   DESCRIPTION:
 *   Init default values for target temperature, ADC offsets and 
 *   the serial number of the CCD from a init file pInitFileName
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   The pInitFileName is the full name of the file including the path.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */


uint32 detContInit
   (
   char *   pInitFileName,          /* Init file Name                         */
   uint32 * pTempCode,              /* Target temperature code                */
   uint32 * pTempCoeff,             /* Coefficient for temperature control    */
   long   * pOffsetFullVect,        /* ADC offset vector [2] - no binning     */
   long   * pOffsetBinVect,         /* ADC offset vector [2] - binning        */
   char *   pCcdSn                  /* CCD serial number                      */
   )
{
   FILE *       pFile;
   char         comment [STRING_SIZE];
   float        tempTarget;
   int          coeff;
   int          offset;

   /* Open the file in read mode */

   pFile = fopen ( pInitFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      printf ( "Failed to open the Detector Controller init file %s",
		   pInitFileName );
      ERROR_SET1 ( 0, "Failed to open the Detector Controller init file %s",
		   ERROR_LOG_SAVE, pInitFileName );
      return (ERROR);
   }

   /* Read the first line: should be a comment line */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
            "Failed to read first line of comments from the DC init file %s",
            ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): first line of comments:\n" );
   printf ( "%s\n" , comment );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the default target temperature */

   if ( (fscanf (pFile, "%f\n", &tempTarget)) == EOF )
   {
      ERROR_SET1 ( 0,
            "Failed to read the target temperature from the DC init file %s",
            ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( tempTarget <= -40.0 )
   {
      ERROR_SET ( 0,
                  "Target temperature should be greater than -40.0C",
                  ERROR_LOG_SAVE);
      fclose (pFile);
      return (ERROR);
   }

   if ( tempTarget <= 0.0 )
   {
      *pTempCode = (uint32) ((SDSU_TEMP_BASE - tempTarget) / SDSU_TEMP_UNIT);
      *pTempCode &= 0xfff; 
			  /* Truncate to 0xfff (which is the maximum allowed) */
   }
   else
   {
      /* Switch off cooling altogether for temperatures above 0C. */
      *pTempCode = 0;
   }

#ifdef DEBUG
   printf ( "detContInit(): target temperature = %d\n", *pTempCode );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the default temperature coefficient */

   if ( (fscanf (pFile, "%d\n", &coeff)) == EOF )
   {
      ERROR_SET1 ( 0,
        "Failed to read the temperature coefficient from the DC init file %s",
        ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   *pTempCoeff = coeff;

#ifdef DEBUG
   printf ( "detContInit(): temperature coefficient = %d\n", coeff );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the default ADC offset for output 0 - no binning */

   if ( (fscanf (pFile, "%d\n", &offset)) == EOF )
   {
      ERROR_SET1 ( 0,
        "Failed to read the ADC offset0 (full) from the DC init file %s",
        ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   *(pOffsetFullVect + 0) = offset;

#ifdef DEBUG
   printf ( "detContInit(): ADC offset for output 0 (full) = %d\n", offset );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the ADC offset for output 1 - no binning */

   if ( (fscanf (pFile, "%d\n", &offset)) == EOF )
   {
      ERROR_SET1 ( 0,
        "Failed to read the ADC offset1 (full) from the DC init file %s",
        ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   *(pOffsetFullVect + 1) = offset;

#ifdef DEBUG
   printf ( "detContInit(): ADC offset for output 1 (full) = %d\n", offset );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the default ADC offset for output 0 - binning */

   if ( (fscanf (pFile, "%d\n", &offset)) == EOF )
   {
      ERROR_SET1 ( 0,
        "Failed to read the ADC offset0 (bin) from the DC init file %s",
        ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   *(pOffsetBinVect + 0) = offset;

#ifdef DEBUG
   printf ( "detContInit(): ADC offset for output 0 (bin) = %d\n", offset );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the ADC offset for output 1 - binning */

   if ( (fscanf (pFile, "%d\n", &offset)) == EOF )
   {
      ERROR_SET1 ( 0,
        "Failed to read the ADC offset1 (bin) from the DC init file %s",
        ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   *(pOffsetBinVect + 1) = offset;

#ifdef DEBUG
   printf ( "detContInit(): ADC offset for output 1 (bin) = %d\n", offset );
#endif

   /* Skip the next line of comment */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the second line of comments from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "detContInit(): %s\n", comment );
#endif

   /* Read the CCD serial number from the file */

   if ( fgets (pCcdSn, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
         "Failed to read the CCD SN from the DC init file %s",
         ERROR_LOG_SAVE, pInitFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( pCcdSn[strlen(pCcdSn) - 1] == '\n' )
   {
      pCcdSn[strlen(pCcdSn) - 1] = '\0';
#ifdef DEBUG
      printf ( "detControlInit(): last character of %s was return\n",
               pCcdSn );
#endif

   }

#ifdef DEBUG
   printf ( "detContInit(): CCD serial number: %s\n", pCcdSn );
#endif

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detGetSirContext
 *
 *   INVOCATION:
 *   detGetSirContext (pRecordPrefix, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *)    Record Name Prefix
 *   (!) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Get the context structures for the SIR records.
 *
 *   DESCRIPTION:
 *   Get the context structures for the SIR records. 
 *   Each SIR is referenced by its name: first get the name of each SIR,
 *   then call epToVxRecContextGet() in order to look-up the context structure
 *   that has previously been assigned to the SIR during initialisation of the
 *   local record data-base. 
 *
 *   EXTERNAL VARIABLES:
 *
 *   PRIOR REQUIREMENTS:
 *   obsId has to be allocated before calling this function.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *-
 */

uint32 detGetSirContext
   (
   const char * pRecordPrefix,       /* Record Name Prefix.                   */
   OBS_ID       obsId                /* Observation context structure.        */
   )
{

   uint32       errorNumber;         /* Error number reported by task.        */

   char         pRecordName [EPICS_MAX_BYTES_RECORD_NAME + 1];
                                     /* String to store record names.         */

   /* Initialize the erroNumber */

   errorNumber = OK;

   /* Get the context of the wfs control "state" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix, DET_CONTROL_STATE_SIR_NAME );
   if (epToVxRecContextGet (pRecordName, & (obsId->pStateContext), NULL) == 
       ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_STATE_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "initialising" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix, DET_CONTROL_INIT_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetInitContext), NULL) == 
       ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_INIT_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "detInitStatus" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_INIT_STATUS_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetInitStatusContext), 
                            NULL) == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_INIT_STATUS SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "testResults" sir record */

   sprintf (pRecordName, "%s", DET_CONTROL_TEST_RESULTS_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pTestResultsContext), 
                            NULL) == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_TEST_RESULTS_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "testing" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_TESTING_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pTestingContext), NULL) 
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_TESTING_SIR_NAME SIR context");
      return (ERROR);
   }

   /* Get the context of the "detPrimReply" sir record */
   
   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_PRIM_REPLY_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetPrimReplyContext), 
                            NULL) == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_PRIM_REPLY_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "obsType" sir record */

   sprintf (pRecordName, "%s", SEQ_CONTROL_OBSTYPE_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pObsTypeContext), NULL) 
       == ERROR)
   {
      ERROR_LOG ("Failed to get SEQ_CONTROL_OBSTYPE_SIR_NAME SIR context");
      return (ERROR);
   }

   /* Get the context of the "detType" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_DETTYPE_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetTypeContext), NULL) == 
       ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_DETTYPE_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "detID" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_DETID_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetIdContext), NULL) == 
       ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_DETID_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "bunit" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_BUNIT_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pBunitContext), NULL) == 
       ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_BUNIT_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "observing" sir record */

   sprintf (pRecordName, "%s", DET_CONTROL_OBSERVING_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetObservingContext), 
                            NULL) == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_OBSERVING_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "obsMode" sir record */

   sprintf (pRecordName, "%s", SEQ_CONTROL_OBSMODE_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pObsModeContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get SEQ_CONTROL_OBSMODE_SIR_NAME SIR context");
      return (ERROR);
   }

   /* Get the context of the "dataLabel" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_DATALABEL_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDataLabelContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_DATALABEL_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "intTime" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_INTTIME_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pIntTimeContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_INTTIME_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "outputs" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_OUTPUTS_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pOutputsContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_OUTPUTS_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "detXsize" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_DETXSIZE_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetXsizeContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_DETXSIZE_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "detYsize" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_DETYSIZE_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDetYsizeContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_DETYSIZE_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "xsubap" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_XSUBAP_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pXsubapContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_XSUBAP_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "ysubap" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_YSUBAP_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pYsubapContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_YSUBAP_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "xstart" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_XSTART_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pXstartContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_XSTART_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "ystart" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_YSTART_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pYstartContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_YSTART_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "xras" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_XRASTER_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pXrasterContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_XRASTER_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "yras" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_YRASTER_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pYrasterContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_YRASTER_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "xspace" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_XSPACE_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pXspaceContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_XSPACE_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "yspace" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_YSPACE_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pYspaceContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_YSPACE_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "xbin" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_XBIN_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pXbinContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_XBIN_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "ybin" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_YBIN_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pYbinContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_YBIN_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "nexpRQ" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_NEXPRQ_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pNExpRQContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_NEXPRQ_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "nexp" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_NEXP_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pNExpContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_NEXP_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "nframes" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_NFRAMES_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pNFramesContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_NFRAMES_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "utstart" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_UTSTART_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pUTstartContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_UTSTART_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "utend" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_UTEND_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pUTendContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_UTEND_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "exposed" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_EXPOSED_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pExposedContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_EXPOSED_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "exposedRQ" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_EXPOSEDRQ_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pExposedRQContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_EXPOSEDRQ_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "elapsed" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_ELAPSED_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pElapsedContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_ELAPSED_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "dhsCon" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_DHSCON_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pDhsConContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_DHSCON_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "adc0" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_ADC0_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAdc0Context), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_ADC0_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "adc1" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_ADC1_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pAdc1Context), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_ADC1_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Get the context of the "oscan" sir record */

   sprintf (pRecordName, "%s:%s", pRecordPrefix,
            DET_CONTROL_OSCAN_SIR_NAME);
   if (epToVxRecContextGet (pRecordName, & (obsId->pOscanContext), NULL)
       == ERROR)
   {
      ERROR_LOG ("Failed to get DET_CONTROL_OSCAN_SIR_NAME SIR context");
      errorNumber = ERROR;
   }

   /* Return */

   return ( errorNumber );
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detWriteDefSirContext
 *
 *   INVOCATION:
 *   detWriteDefSirContext (obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Write Default values to the SIR records.
 *
 *   DESCRIPTION:
 *   Write Default values to the SIR records.
 *
 *   EXTERNAL VARIABLES:
 *
 *   PRIOR REQUIREMENTS:
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *-
 */

uint32 detWriteDefSirContext
   (
   OBS_ID       obsId                /* Observation context structure.        */
   )
{

   uint32       errorNumber;         /* Error number reported by task.        */

   /* Initialize the erroNumber */

   errorNumber = OK;

   /* Init the "testResults" sir record */

   if (epToVxPipeWrite (NULL, "Not tested", obsId->pTestResultsContext) 
       == ERROR)
   {
      ERROR_LOG (
      "Failed to initialise DET_CONTROL_TEST_RESULTS_SIR_NAME record");
      errorNumber = ERROR;
   }

   /* Init the "obsType" sir record */

   if (epToVxPipeWrite( NULL, "UNDEFINED", obsId->pObsTypeContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set default observation type to UNDEFINED");
      return (ERROR);
   }


   /* Init the "detType" sir record */

   if (epToVxPipeWrite( NULL, DET_TYPE, obsId->pDetTypeContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set default detector type");
      errorNumber = ERROR;
   }

   /* Init the "bunit" sir record */

   if (epToVxPipeWrite( NULL, DET_BUNIT, obsId->pBunitContext ) == ERROR)
   {
      ERROR_LOG ("Failed to set default detector type");
      errorNumber = ERROR;
   }

   /* Init the "dhsCon" sir record */

   if (epToVxPipeWrite( NULL, "NOT CONNECTED", obsId->pDhsConContext ) == ERROR)
   {
      ERROR_LOG ("Failed to init dhs connection sir record");
      errorNumber = ERROR;
   }

   /* return */

   return (errorNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   detDhsDisplay
 *
 *   INVOCATION:
 *   detDhsDisplay (cadCmdContext, commandNumber, sdsuId, obsId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) cadCmdContext (CAD_CMD_CONTEXT) CAD command context structure
 *   (>) commandNumber (int)             Command number
 *   (>) sdsuId        (SDSU_ID)         Current SDSU context structure
 *   (!) obsId         (OBS_ID)          Observation context structure
 *
 *   FUNCTION VALUE:
 *   (uint32)   Error number. 0 if command successful.
 *
 *   PURPOSE:
 *   Execute detDhsDisplay command
 *
 *   DESCRIPTION:
 *   This function sets the parameters to send the data to the QL of the DHS.
 *
 *   EXTERNAL VARIABLES:
 *   None.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *-
 */

uint32 detDhsDisplay
   (
   CAD_CMD_CONTEXT cadCmdContext, /* CAD command context structure.           */
   int             commandNumber, /* Command number.                          */
   SDSU_ID         sdsuId,        /* SDSU context structure.                  */
   OBS_ID          obsId          /* Observation context structure.           */
   )
{
   uint32          errorNumber;   /* Error number reported by task.           */
   
   long            rate;

   /*
    * Initialise the error number and obtain the attributes provided with the
    * command.
    */

   errorNumber = 0;
   EPTOVX_CAD_ATTRIB_GET (cadCmdContext, commandNumber, 0, (char *) &rate);
   
   /*
    * Check there are valid SDSU and observation context structures.
    */

   if ( sdsuId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "SDSU context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   if ( obsId == NULL )
   {
      ERROR_SET (S_detControl_INTERNAL, "Observation context not initialised",
                 ERROR_LOG_NOW);
      errorNumber = S_detControl_INTERNAL;
      return (errorNumber);
   }

   /*
    * The command can be used when an observation is in progress.
    * Set the obsId parameters.
    */

   obsId->dhsQlRate = rate;

   return (OK); 
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   newDetFrameUnscrambleUint16
 *
 *   INVOCATION:
 *   newDetFrameUnscrambleUint16 (xPixels, yPixels, oscanNb, inFrame, outBuffer)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) xPixels   (const int)    Number of columns
 *   (>) yPixels   (const int)    Number of rows
 *   (>) oscanNb   (const int)    Overscan number
 *   (>) inFrame   (SDSU_FRAME *) Pointer to input frame
 *   (<) outBuffer (uint16 *)     Pointer to output frame buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if command successful, ERROR if unsuccessful
 *
 *   PURPOSE:
 *   Unscramble an entire frame of data
 *
 *   DESCRIPTION:
 *   This function takes a raw frame of data containing pixels in the order
 *   they are read from the detector and unscrambles them to generate an output
 *   frame with pixels in the correct order.
 *   Output data are unsigned short int.
 *
 *   ACKNOWLEDGEMENTS:
 *   This function is based around the LeachDeScramble (lds) program provided
 *   by Les Saddlemyer and Tim Hardy, Hertzberg Institute of Astrophysics,
 *   Canada.
 *
 *   EXTERNAL VARIABLES:
 *   None. (The function needs to be reentrant)
 *
 *   PRIOR REQUIREMENTS:
 *   inFrame is a data frame which contains the scrambled SDSU pixels.
 *   outBuffer must point to a buffer large enough to contain at least 
 *   xPixels*yPixels unsigned short integer.
 *
 *   INCLUDE FILES:
 *   detControl.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS newDetFrameUnscrambleUint16
   (
   const int      xPixels,         /* Number of columns.                      */
   const int      yPixels,         /* Number of rows.                         */
   const int      oscanNb,         /* Number of overscan column per output    */
   SDSU_FRAME *   inFrame,         /* Pointer to input frame                  */
   uint16 *       outBuffer        /* Pointer to output frame buffer.         */
   )
{
   volatile uint16 *   ptr;        /* Pointer into frame buffer.              */

   int            i, j;            /* Counters.                               */

   int            nPixels;         /* Total number of pixels.                 */

   int            xPixelsSector;   /* Number of columns per sector.           */
   int            xPixelsDataSector;
                                   /* Number of columns of data per sector.   */
   int            xPixelsData;     /* Number of columns of data.              */
   int            yPixelsSector;   /* Number of rows per sector.              */

   volatile uint16 *  inDataPtr;   /* Pointer to start of input data.         */
   uint16 *           outDataPtr;  /* Pointer to start of output data.        */

   uint16 *        ps1;            /* Pointer to beginning of sector 1.       */
   uint16 *        ps2;            /* Pointer to beginning of sector 2.       */

#ifdef DEBUG
   uint16          min, max;       /* Minimum and maximum.                    */
#endif /* DEBUG */

   if ( (inFrame == NULL) || (outBuffer == NULL) )
   {
      ERROR_SET(S_detControl_INTERNAL, 
                "No input and/or output buffers defined", ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf (
   "newDetFrameUnscrambleUint16: Unscrambling %d x %d pixels from frame at %p to %p\n",
   xPixels, yPixels, inFrame, outBuffer);

   min = USHRT_MAX;
   max = 0;
#endif /* DEBUG */

   /*
    * Set pointers to the start of the data.
    */

   inDataPtr = & inFrame->pixel[0];
   outDataPtr = outBuffer;

   /*
    * The algorithm used to unscramble the data depends on the number of outputs
    * from the detector. If there are two outputs the sectors are arranged 
    * like this
    *
    *   +------------+------------+
    *   |  sector 1  |  sector 2  |
    *   0----->------+-----<------0
    *
    * "0" shows the origin of each sector and ">" the direction of readout.
    */


   /* There are two outputs and therefore 2 sectors in a 2x1 pattern. */

   nPixels = xPixels * yPixels;
   xPixelsSector = xPixels / 2;
   xPixelsDataSector = xPixelsSector - oscanNb;
   xPixelsData = 2 * xPixelsDataSector;

   yPixelsSector = yPixels;

#ifdef DEBUG
   printf ("Two sectors of size %d x %d\n", xPixelsSector, yPixelsSector);
#endif /* DEBUG */

   /* Initialise the starting position for each sector */

   ps1 = outDataPtr;
   ps2 = &outDataPtr[xPixelsData - 1];
   ptr = inDataPtr;

   /* Treat one line at a time, moving sector pointers */

   for (i = 0; i < yPixelsSector; i++)
   {
      for (j = 0; j < xPixelsDataSector; j++)
      {
#ifdef DEBUG
         if ( *ptr < min ) min = *ptr;
         if ( *ptr > max ) max = *ptr;
         if ( *(ptr+1) < min ) min = *(ptr+1);
         if ( *(ptr+1) > max ) max = *(ptr+1);
#endif
         /*
          * Change the order here if sectors 1, 2 is
          * different from the order of arrival
          */

         *ps1++ = *ptr++;
         *ps2-- = *ptr++;
      }

      ps1 += xPixelsDataSector;
      ps2 += (xPixelsDataSector + (2*oscanNb));

      for ( j = xPixelsDataSector ; j < xPixelsSector ; j ++ )
      {
          *ps1 ++ = *ptr ++;
          *ps2 -- = *ptr ++;
      }

      ps1 += oscanNb;
      ps2 += (oscanNb + (2*xPixelsDataSector));
   }
#ifdef DEBUG
   printf ("Values range from %d to %d\n", (int)min, (int)max);
#endif /* DEBUG */

   return (OK);
}
