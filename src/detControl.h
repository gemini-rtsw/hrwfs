/*+
 *   MODULE NAME:
 *   detControl
 *
 *   FILENAME:
 *   detControl.h
 *
 *   PURPOSE:
 *   Include file for detControl
 *
 *   IMPORTANT:
 *   *** THIS FILE MUST BE MODIFIED TO REFLECT THE ACTUAL VME ADDRESSES OF
 *   *** THE SDSU CONTROLLERS AT YOUR SITE. SEE DEFINITIONS BELOW.
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.31  1998/12/07 15:25:31  cics
 * Changed output options in observe command. Fixed some sdsuLib bugs related to continuous observing.
 *
 * Revision 1.30  1998/11/30 15:54:46  cics
 * Modifications made during SMB visit to Hilo, November 1998
 *
 * Revision 1.29  1998/10/22 15:14:16  cics
 * Initialises signal processing parameters on startup. Does not yet update ospGeometry.
 *
 * Revision 1.28  1998/10/21 16:31:20  cics
 * detShow can now show WFSs individually
 *
 * Revision 1.27  1998/10/20 09:32:22  cics
 * testResults and initialising records added.
 *
 * Revision 1.26  1998/10/14 09:56:32  cics
 * HRWFS code added, with HRWFS and OIWFS having the ability to share the same SDSU hardware through a mutex semaphore.
 *
 * Revision 1.25  1998/10/12 11:22:36  cics
 * Updated to keep up with changes to signal processing API
 *
 * Revision 1.24  1998/10/01 13:54:34  cics
 * Signal processor task removed and all signal processing now handled by detControl
 *
 * Revision 1.23  1998/09/28 11:22:11  cics
 * Give warning if not compiled for VxWorks
 *
 * Revision 1.22  1998/09/17 08:30:35  cics
 * Reduced no frames to save memory. Added detDhsCheckErrno. Fixed bugs in detObserveEnd.
 *
 * Revision 1.21  1998/08/27 17:29:40  anj
 * Implemented frame & packet sync interrupts and application callbacks.
 *
 * Revision 1.20  1998/08/21 16:15:12  smb
 * Comments added before releasing to Gemini
 *
 * Revision 1.19  1998/08/20 16:02:46  smb
 * Removed old commented out code
 *
 * Revision 1.18  1998/08/20 14:46:57  smb
 * Split detObserve into detObserveStart and detObserveEnd (callback). Added more DHS functions. Updated detSave.
 *
 * Revision 1.17  1998/08/20 12:21:01  anj
 * Modified the frame buffer allocation system.
 * Deleted sdsuSync, some mailbox stuff and other old Nick Dillon code
 * which will not be used.
 *
 * Revision 1.16  1998/08/18 10:58:21  smb
 * Test command added.
 *
 * Revision 1.15  1998/07/30 16:51:24  smb
 * Observation ID structure added. Time stamps obtained and written to FITS header.
 *
 * Revision 1.14  1998/07/28 15:54:17  smb
 * Parameters to setGeometry setMode and setTemp modified.
 *
 * Revision 1.13  1998/07/17 12:07:06  anj
 * Set SDSU card addresses; modified sdsuContextCreate to check for
 * absent cards without crashing when sysextLib not used.
 *
 * Revision 1.12  1998/07/16 16:39:06  smb
 * File paths no longer have to end in slash. Fixed problem with image buffer pointer not being returned properly from detObserve. Download DSP code automatically on startup.
 *
 * Revision 1.11  1998/07/16 11:02:42  smb
 * Brought up to date with new DSP parameters
 *
 * Revision 1.10  1998/07/13 15:17:14  smb
 * HRWFS added. Code for data simulation and saving to disk added. DHS code added (commented out)
 *
 * Revision 1.9  1998/06/30 11:32:11  smb
 * Simulation option in sdsuLib made use of. Dependency on sysextLib and mpPipeDrv can be removed using NO_SYSEXTLIB and NO_MPPIPEDRV macros.
 *
 * Revision 1.8  1998/05/12 13:17:10  smb
 * Camera name changed to WFS name
 *
 * Revision 1.7  1998/02/18 11:16:26  smb
 * Commands brought up to date with ICD 162/163
 *
 * Revision 1.6  1998/02/05 15:34:07  smb
 * Ability to update initialising and observing records added
 *
 * Revision 1.5  1998/02/02 17:26:07  smb
 * Free resources if task stopped
 *
 * Revision 1.4  1998/01/15 13:45:19  smb
 * Restructured and documented
 *
 * Revision 1.3  1998/01/14 14:37:13  smb
 * Added high level command for downloading COFF file
 *
 * Revision 1.2  1997/12/15 11:58:18  smb
 * Replaced arbitrary error numbers with real ones
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

#include <timers.h>
#include "gemModNum.h"
#include "epToVxLib.h"
#include "sdsuLib.h"
#include "osp.h"

#ifndef NO_DHS
#include "dhs.h"                  /* Include Data Handling System constants.            */
#else
typedef   unsigned long   DHS_CONNECT;
#endif   /* NO_DHS */


/* defines */

#define   DET_CONTROL_TASK_NAME            "detControl"
                                    /* Detector Controller task name.   */

#define   DET_CONTROL_INIT_SIR_NAME         "initialising"
                                    /* Name of SIR record containing   */
                                    /* initialisation state.         */

#define   DET_CONTROL_INIT_STATUS_SIR_NAME   "detInitStatus"
                                    /* Name of SIR record containing   */
                                    /* SDSU initialisation status.      */

#define   DET_CONTROL_TEST_RESULTS_SIR_NAME   "testResults"
                                    /* Name of SIR record containing   */
                                    /* SDSU test results.            */

#define   DET_CONTROL_PRIM_REPLY_SIR_NAME      "detPrimReply"
                                    /* Name of SIR record containing   */
                                    /* reply from SDSU primitive cmd.   */

#define   DET_CONTROL_OBSERVING_SIR_NAME      "observing"
                                    /* Name of SIR record containing   */
                                    /* observing state.               */


   /*
    * Define the VME addresses of the SDSU controllers installed on the bus.
    * If a particular controller is not installed its address should be set
    * to 0x0, and the controller will then be simulated.
    * NOTE: IT IS VERY IMPORTANT THAT THESE ADDRESSES ARE CORRECT.
    */


#define   DET_CONTROL_PWFS1_SDSU_ADRS_VME      0x0      /* Will eventually be 0xc0000010   */
                                    /* VME address of SDSU controller   */
                                    /* for PWFS1.                  */

#define   DET_CONTROL_PWFS2_SDSU_ADRS_VME      0x0      /* Will eventually be 0xc0000020   */
                                    /* VME address of SDSU controller   */
                                    /* for PWFS2.                  */

#define   DET_CONTROL_SWITCH_SDSU_ADRS_VME   0xc0000000      /* Will eventually be 0xc0000000   */
                                    /* VME address of SDSU controller   */
                                    /* for the OIWFS and HRWFS switch.   */
   /*
    * Define the bit masks used to stop detector control processes individually.
    */

#define   DET_CONTROL_PWFS1_MASK            0x1      /* Bit 0 set */
#define   DET_CONTROL_PWFS2_MASK            0x2      /* Bit 1 set */
#define   DET_CONTROL_OIWFS_MASK            0x4      /* Bit 2 set */
#define   DET_CONTROL_HRWFS_MASK            0x8      /* Bit 3 set */

   /*
    * Define the maximum data frame sizes for each of the wavefront sensors.
    */

#define DET_CONTROL_PWFS1_XSIZE            80
#define DET_CONTROL_PWFS1_YSIZE            80
#define DET_CONTROL_PWFS2_XSIZE            80
#define DET_CONTROL_PWFS2_YSIZE            80
#define DET_CONTROL_OIWFS_XSIZE            80
#define DET_CONTROL_OIWFS_YSIZE            80
#define DET_CONTROL_HRWFS_XSIZE            1024 /* 1072 ? */   /* Reduce if you have less than 24MB memory */
#define DET_CONTROL_HRWFS_YSIZE            1024   /* Reduce if you have less than 24MB memory */

   /*
    * Define the default number of SDSU data buffers allocated for each of the wavefront sensors.
    * sdsuLib expects there to be at least 2 buffers.
    */

#define DET_CONTROL_PWFS1_MAX_FRAMES      2   /* Was 5 - only 2 needed for simple task */
#define DET_CONTROL_PWFS2_MAX_FRAMES      2   /* Was 5 - only 2 needed for simple task */
#define DET_CONTROL_OIWFS_MAX_FRAMES      2   /* Was 5 - only 2 needed for simple task */
#define DET_CONTROL_HRWFS_MAX_FRAMES      1   /* Was 2 - only 1 needed for simple task */

   /*
    * Define World Coordinate System constants.
    */

#define DET_CONTROL_MAX_WCSPOINTS         40   /* Maximum number of WCS calibration points. */

   /*
    * Define the default signal processing initialisation files for each of the wavefront sensors.
    * Set to "NONE" if no default signal processing initialisation is required.
    */

#define DET_CONTROL_PWFS1_OSPFGINI_FILE      "NONE"         /* Will be "pwfs1fg.ini"   */
#define DET_CONTROL_PWFS1_OSPAOINI_FILE      "NONE"         /* Will be "pwfs1ao.ini"   */
#define DET_CONTROL_PWFS2_OSPFGINI_FILE      "NONE"         /* Will be "pwfs2fg.ini"   */
#define DET_CONTROL_PWFS2_OSPAOINI_FILE      "NONE"         /* Will be "pwfs2ao.ini"   */
#define DET_CONTROL_OIWFS_OSPFGINI_FILE      "NONE"         /* Will be "???.ini"   */
#define DET_CONTROL_OIWFS_OSPAOINI_FILE      "NONE"         /* Will be "???.ini"   */
#define DET_CONTROL_HRWFS_OSPINI_FILE      "NONE"         /* No signal processing for HRWFS */

   /*
    * Define the names of the OMF files containing the DSP code. These files are
    * downloaded automatically on startup. The same files are used for most of the
    * wavefront sensors; the exception being the HRWFS, which uses a CCD47 chip and
    * therefore needs different TIMING DSP code.
    */

#define   DET_CONTROL_OMF_FILE_PATH         "./bin/asm56000"
                                    /* Directory containing OMF files.      */

#define   DET_CONTROL_OMF_VME_FILE         "vme.lod"
                                    /* OMF file to download to VME DSP.      */

#define   DET_CONTROL_GBD_OMF_TIM_FILE      "tim-39.lod"
                                    /* OMF file to download to TIMING DSP   */
                                    /* for PWFS and OIWFS.               */

#define   DET_CONTROL_HRWFS_OMF_TIM_FILE      "tim-47-full.lod"
                                    /* OMF file to download to TIMING DSP.   */
                                    /* for HRWFS and acquisition camera.   */

#define   DET_CONTROL_OMF_UTL_FILE         "util.lod"
                                    /* OMF file to download to UTILITY DSP.   */

   /* Define the name of the directory containing parameter files. */

#define   DET_CONTROL_PAR_FILE_PATH         "./data"

   /* Define the default directory to contain engineering data files. */

#define   DET_CONTROL_DATA_FILE_PATH         "."


typedef   struct                  /* Context structure used to describe an observation.   */
{
                           /* AGWPS context information.                     */
                           /* --------------------------                     */
   SDSU_ID         sdsuId;         /* SDSU context.                              */
   BOOL         observing;      /* Flag set TRUE when observing.                  */
   BOOL         stopped;      /* Flag set TRUE when observation stopped.            */
   BOOL         continuous;      /* BUG WORK AROUND: Set TRUE whenever the SDSU         */
                           /* controller is in continuous mode.               */
   DATREC_CONTEXT   pDetObservingContext;
                           /* Observing record context.                     */
   int            totalFrames;   /* Total frames for observation.                  */
   int            nframes;      /* Frame counter for this observation.               */
   SEM_ID         syncSem;      /* Observation synchronsisation semaphore.            */

                           /* Timer information.                           */
                           /* ------------------                           */
   timer_t         timeId;         /* ID of timer used to time observation in simulation.   */

                           /* High level information.                        */
                           /* -----------------------                        */
   char         pWfsName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                           /* Name of wavefront sensor.                     */
   char         pObsType [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                           /* Type of observation.                           */

                           /* Data handling information.                     */
                           /* --------------------------                     */
   int            outOptions;      /* Output options (0=none, 1=DHS, 2=file).            */
   DHS_CONNECT      dhsConnection;   /* DHS connection ID.                           */
   DHS_BD_DATASET   dhsDataset;      /* DHS dataset ID.                              */
   DHS_BD_FRAME   dhsDataFrame;   /* DHS data frame ID.                           */
   float *         pCurFrame;      /* Pointer to current unscrambled data frame.         */
   int            xPixels;      /* Number of columns in frame, in pixels.            */
   int            yPixels;      /* Number of rows in frame, in pixels.               */
   uint32         outputs;      /* Number of detector outputs.                     */

   char         pDataLabel [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                           /* DHS data label.                              */
#ifdef SAVE_RAW_DATA
   char         pRawFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                           /* Combined path name and file name for raw data.      */
                           /* (This file is used for engineering only).         */
#endif /* SAVE_RAW_DATA */
   char         pOutFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                           /* Combined path name and file name for processed data.   */
   char         pSimFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                           /* Combined path name and file name for simulated data.   */
                           /* (This file is used for engineering only).         */

                           /* Signal processing information.                  */
                           /* ------------------------------                  */
    struct OSP_HRCONTEXT *
               ospHRContext;   /* Pointer to HR dedicated signal processing context    */
                                    /* structure. Temporary solution                        */
    struct OSP_CONTEXT *
               ospFGContext;   /* Pointer to FG signal processing context structure.   */
   struct OSP_CONTEXT *
               ospAOContext;   /* Pointer to AO signal processing context structure.   */
   struct OSP_GEOMETRY *
               ospGeometry;   /* Pointer to signal processing geometry structure.      */
   long         sigMode;      /* Signal processing mode.                        */
   long         nCoaddFrames;   /* Number of frames to coadd.                     */
   double         ttCalAmplitude;   /* Tip tilt calibration amplitude.                  */
   double         coaddTimeout;   /* Signal processing coadd timeout in seconds.         */
   char         pCoeffsFile [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                           /* File of coefficients, centroids or whatever.         */
   int            coaddCounter;   /* Counter used to decide when to save coadded data.   */

                           /* Time stamps.                                 */
                           /* ------------                                 */
   double         rawtStart;      /* Raw Gemini time at start of observation.            */
   double         rawtEnd;      /* Raw Gemini time at end of observation.            */
   double         exposedRQ;      /* Requested total exposure time.                  */
   double         exposed;      /* Actual total exposure time.                     */
   double         frameTime;      /* Frame time.                                 */

                           /* World Coordinate System (WCS) information.         */
                           /* ------------------------------------------         */
   int            wcsStatus;      /* WCS status (0 if WCS information OK).            */
   int            nWcsPoints;      /* Number of WCS calibration points.               */
   double         fpxy[DET_CONTROL_MAX_WCSPOINTS][2];
                           /* Array of defined focal plane XY coordinates         */
   double         pixij[DET_CONTROL_MAX_WCSPOINTS][2];
                           /* Array of measured pixel IJ coordinates            */
   double         detij[DET_CONTROL_MAX_WCSPOINTS][2];
                           /* Array of pixel IJ coordinates applying to data read    */
                           /* from the detector.                           */
   double         cij[6];         /* XY to IJ transformation matrix.                  */
   char         ctype1[9];      /* World Coordinate System projection type for axis 1.   */
   double         crpix1;         /* Pixel coordinate reference for axis 1.            */
   double         crval1;         /* World coordinate reference for axis 1.            */
   char         ctype2[9];      /* World Coordinate System projection type for axis 2.   */
   double         crpix2;         /* Pixel coordinate reference for axis 2.            */
   double         crval2;         /* World coordinate reference for axis 2.            */
   double         cd1_1;         /* xi rotation/skew matrix element.                  */
   double         cd1_2;         /* xj rotation/skew matrix element.                  */
   double         cd2_1;         /* yi rotation/skew matrix element.                  */
   double         cd2_2;         /* yj rotation/skew matrix element.                  */
   double         RA;            /* Right Ascension in hours.                     */
   double         Dec;         /* Declination in degrees.                        */
   char         radecsys[9];   /* Type of RA/Dec (for celestial coordinate).         */
   double         equinox;      /* Epoch of mean equator & equinox (celestial coords).   */
   double         epoch;         /* Epoch of observation as a year.                  */
   double         mjdobs;         /* Epoch of observation as a modified Julian date.      */
} OBS_ID_STRUCT, * OBS_ID;

   /*
    * Error number codes used by detControl.
    * These are designed to be processed using the vxWorks "makeStatTbl" utility.
    */

#define   S_detControl_BAD_COMMAND      (M_detControl | 1)   /* Unrecognised command         */
#define S_detControl_BAD_WFS_NAME      (M_detControl | 2)   /* Unrecognised WFS name      */
#define   S_detControl_BAD_ATTRIBUTE      (M_detControl | 3)   /* Bad attribute value         */
#define   S_detControl_BAD_FILE         (M_detControl | 4)   /* Bad parameter file         */
#define   S_detControl_SDSU_ERROR         (M_detControl | 5)   /* Error from SDSU controller   */
#define   S_detControl_DHS_ERROR         (M_detControl | 6)   /* Error from DHS            */
#define   S_detControl_INTERNAL         (M_detControl | 7)   /* detControl internal failure   */
#define   S_detControl_BUSY            (M_detControl | 8)   /* Controller busy            */
#define   S_detControl_NO_ACCESS         (M_detControl | 9)   /* No access to hardware      */
#define   S_detControl_WCS_ERROR         (M_detControl | 10)   /* Error in WCS calculation      */

   /*
    * Define the commands recognised by the detector controller task.
    */

enum
   {

   /* GBDS commands. */

   DET_CONTROL_CMD_SETUP = 0,            /* Set up SDSU controller parameters.      */
   DET_CONTROL_CMD_CHOP,               /* Specify chop states mask.            */
   DET_CONTROL_CMD_EXPOSURE,            /* Specify exposure time.               */
   DET_CONTROL_CMD_OBSTYPE,            /* Specify observation type.            */
   DET_CONTROL_CMD_SETDHS,               /* Set Data Handling System parameters.      */
   DET_CONTROL_CMD_SETWCS,               /* Set World Coordinate System parameters.   */
   DET_CONTROL_CMD_OBSERVE,            /* Make observation.                  */
   DET_CONTROL_CMD_PAUSE,               /* Pause observation.                  */
   DET_CONTROL_CMD_CONTINUE,            /* Continue observation.               */
   DET_CONTROL_CMD_STOP,               /* Stop observation.                  */
   DET_CONTROL_CMD_ABORT,               /* Abort observation.                  */
   DET_CONTROL_CMD_SIGINIT,            /* Initialise signal processing.         */
   DET_CONTROL_CMD_SIGMODE,            /* Configure signal processing.            */

   /* genSub commands. */

   DET_CONTROL_CMD_TTFZERO,            /* ttfZero.                           */
   DET_CONTROL_CMD_AOZERO,               /* aoZero.                           */
   DET_CONTROL_CMD_PROBEOFFSET,         /* probeOffset.                        */

   /* Engineering commands. */

   DET_CONTROL_CMD_INITIALISE,            /* Initialise SDSU controller.            */
   DET_CONTROL_CMD_RESET,               /* Reset SDSU controller.               */
   DET_CONTROL_CMD_TEST,               /* Test SDSU controller.               */
   DET_CONTROL_CMD_GIVEUP,               /* Give up control of hardware (HRWFS/OIWFS).*/
   DET_CONTROL_CMD_SAVE,               /* Save SDSU controller parameters.         */
   DET_CONTROL_CMD_GEOMETRY,            /* Set detector readout geometry.         */
   DET_CONTROL_CMD_PRIMITIVE,            /* Execute SDSU primitive command.         */
   DET_CONTROL_CMD_DOWNLOAD,            /* Download DSP code.                  */
   DET_CONTROL_CMD_MODE,               /* Set detector readout mode.            */
   DET_CONTROL_CMD_OFFSET,               /* Set detector ADC offsets.            */
   DET_CONTROL_CMD_TEMP               /* Define temperature control params.      */
   };

   /* Public variables */

IMPORT BOOL         detDhsInitialised;      /* Flag to determine whether the DHS      */
                                 /* library has been initialised.         */
IMPORT SEM_ID      detDhsSem;            /* Semaphore to control access to DHS.      */

IMPORT SDSU_ID      detSdsuIdP1;         /* SDSU context structure for PWFS1.      */
IMPORT SDSU_ID      detSdsuIdP2;         /* SDSU context structure for PWFS2.      */
IMPORT SDSU_ID      detSdsuIdOi;         /* SDSU context structure for OIWFS.      */
IMPORT SDSU_ID      detSdsuIdHr;         /* SDSU context structure for HRWFS.      */

   /* Public functions */

IMPORT void         detShow (const char * pWfsNames, const BOOL verbose);
IMPORT void         detStatusShow (const char * pWfsNames);
IMPORT void         detTempShow (const char * pWfsNames);
IMPORT STATUS      detObsShow (OBS_ID obsId, const BOOL verbose);
IMPORT void         detDhsErrorCallback (DHS_CONNECT connect, DHS_STATUS errorNum,
                  DHS_ERR_LEVEL errorLev, char * msg, DHS_TAG tag, void * userData);
IMPORT STATUS      detDhsInit (const char * pClientName, const int numConnect,
                  const char * pHostName, const char * pServerName);
