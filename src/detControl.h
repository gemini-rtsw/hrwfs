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
 *   HISTORY MODIFICATION
 *   09 Jan 2001: CB - Add detPowerOn
 *   07 Jun 2001 - cb add frame in obsId structure
 *   05 Jun 2001 - cb add detTemp, instName, inport in obsId structure
 *                 modify the ccdsec,...
 *   01 Jun 2001 - cb add oscan SIR record 
 *   30 May 2001 - cb add DET_CONTROL_HRWFS_MAX_XSIZE and 
 *                 DET_CONTROL_HRWFS_MAX_Y_SIZE and oscanNb
 *   03 Apr 2001 - cb add sir adc0, adc1
 *   19 feb 2001 - cb add sir dhsCon
 *   16 feb 2001 - cb add DET_CONTROL_CMD_DHS_DISPLAY, dhsQlRate, dhsCounter
 *   06 feb 2001 - cb move all the DATREC_CONTEXT structures into the obsId
 *                 structure
 *   26 jan 2001 - cb add DET_CONTROL_HRWFS_CP_INIT_FILE and
 *                 DET_CONTROL_HRWFS_MK_INIT_FILE
 *   15 may 2000 - cb add detDhsReconnect
 *   11 feb 2000 - cb add some detector geometry sir records
 *   10 feb 2000 - cb add some sir records
 *   31 jan 2000 - cb add detector controller state SIR record
 *   19 jan 2000 - cb add testing sir record
 *   18 nov 1999 - cb add qlStream parameter and add cmd DET_CONTROL_CMD_DHSINFO
 *   27 oct 1999 - cb add fits keywords
 *   25 oct 1999 - cb add dhsOutOptions (perm, temp, ql)
 *   19 oct 1999 - cb cancel ospGeometry and replace by all coeff needed by 
 *                 dsp code to do binning and windowing.
 *   14 oct 1999 - cb simplified version for HR only
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

#ifndef NO_DHS
#include "dhs.h"  
#else
typedef   unsigned long   DHS_CONNECT;
#endif   /* NO_DHS */


/****************************************************************** defines ***/

#define   STRING_SIZE       160     /* Size of a string                 */

#define   HALF              2       /* HALF flag for overscan region    */
#define   FULL              4       /* FULL flag for overscan region    */

#define   DET_CONTROL_TASK_NAME               "detControl"
                                    /* Detector Controller task name.   */

#define   DET_CONTROL_INIT_SIR_NAME           "initialising"
                                    /* Name of SIR record containing    */
                                    /* initialisation state.            */

#define   DET_CONTROL_INIT_STATUS_SIR_NAME    "detInitStatus"
                                    /* Name of SIR record containing    */
                                    /* SDSU initialisation status.      */

#define   DET_CONTROL_TEST_RESULTS_SIR_NAME   "testResults"
                                    /* Name of SIR record containing    */
                                    /* SDSU test results.               */

#define   DET_CONTROL_PRIM_REPLY_SIR_NAME     "detPrimReply"
                                    /* Name of SIR record containing    */
                                    /* reply from SDSU primitive cmd.   */

#define   DET_CONTROL_OBSERVING_SIR_NAME      "observing"
                                    /* Name of SIR record containing    */
                                    /* observing state.                 */

#define   DET_CONTROL_TESTING_SIR_NAME        "testing"
                                    /* Name of SIR record containing    */
                                    /* testing state.                   */

#define   DET_CONTROL_STATE_SIR_NAME          "state"
                                    /* Name of SIR record containing    */
                                    /* the state of detector controller */

#define   DET_CONTROL_DETTYPE_SIR_NAME        "detType"
                                    /* Name of SIR record containing    */
                                    /* the type of detector controller  */

#define   DET_CONTROL_DETID_SIR_NAME          "detID"
                                    /* Name of SIR record containing    */
                                    /* the SN of the CCD                */

#define   DET_CONTROL_DATALABEL_SIR_NAME      "dataLabel"
                                    /* Name of SIR record containing    */
                                    /* the most recent DHS data label   */

#define   DET_CONTROL_INTTIME_SIR_NAME        "intTime"
                                    /* Name of SIR record containing    */
                                    /* the integration time             */

#define   DET_CONTROL_NEXPRQ_SIR_NAME         "nexpRQ"
                                    /* Name of SIR record containing    */
                                    /* requested nb of exp/dataset      */

#define   DET_CONTROL_NEXP_SIR_NAME           "nexp"
                                    /* Name of SIR record containing    */
                                    /* current nb of exp/dataset        */

#define   DET_CONTROL_NFRAMES_SIR_NAME        "nframes"
                                    /* Name of SIR record containing    */
                                    /* nb of frames/dataset             */

#define   DET_CONTROL_BUNIT_SIR_NAME          "bunit"
                                    /* Name of SIR record containing    */
                                    /* the data unit                    */

#define   DET_CONTROL_UTSTART_SIR_NAME        "utstart"
                                    /* Name of SIR record containing    */
                                    /* the ut at start of observation   */

#define   DET_CONTROL_UTEND_SIR_NAME          "utend"
                                    /* Name of SIR record containing    */
                                    /* the ut at end of observation     */

#define   DET_CONTROL_EXPOSED_SIR_NAME        "exposed"
                                    /* Name of SIR record containing    */
                                    /* the total integration time       */

#define   DET_CONTROL_EXPOSEDRQ_SIR_NAME      "exposedRQ"
                                    /* Name of SIR record containing    */
                                    /* the requested total integration  */
                                    /* time                             */

#define   DET_CONTROL_ELAPSED_SIR_NAME        "elapsed"
                                    /* Name of SIR record containing    */
                                    /* the elapsed time                 */

#define   DET_CONTROL_OUTPUTS_SIR_NAME        "outputs"
                                    /* Name of SIR record containing    */
                                    /* the number of ouputs             */

#define   DET_CONTROL_DETXSIZE_SIR_NAME       "detXsize"
                                    /* Name of SIR record containing    */
                                    /* the X detector size              */

#define   DET_CONTROL_DETYSIZE_SIR_NAME       "detYsize"
                                    /* Name of SIR record containing    */
                                    /* the Y detector size              */

#define   DET_CONTROL_XSUBAP_SIR_NAME         "xsubap"
                                    /* Name of SIR record containing    */
                                    /* the X detector size              */

#define   DET_CONTROL_YSUBAP_SIR_NAME         "ysubap"
                                    /* Name of SIR record containing    */
                                    /* the Y detector size              */

#define   DET_CONTROL_XSTART_SIR_NAME         "xstart"
                                    /* Name of SIR record containing    */
                                    /* the X left offset                */

#define   DET_CONTROL_YSTART_SIR_NAME         "ystart"
                                    /* Name of SIR record containing    */
                                    /* the Y bottom offset              */

#define   DET_CONTROL_XRASTER_SIR_NAME        "xras"
                                    /* Name of SIR record containing    */
                                    /* the X subaperture size           */

#define   DET_CONTROL_YRASTER_SIR_NAME        "yras"
                                    /* Name of SIR record containing    */
                                    /* the Y subaperture size           */

#define   DET_CONTROL_XSPACE_SIR_NAME         "xspace"
                                    /* Name of SIR record containing    */
                                    /* the X space between subapertures */

#define   DET_CONTROL_YSPACE_SIR_NAME         "yspace"
                                    /* Name of SIR record containing    */
                                    /* the Y space between subapertures */

#define   DET_CONTROL_XBIN_SIR_NAME           "xbin"
                                    /* Name of SIR record containing    */
                                    /* the X binning factor             */

#define   DET_CONTROL_YBIN_SIR_NAME           "ybin"
                                    /* Name of SIR record containing    */
                                    /* the Y binning factor             */

#define   DET_CONTROL_DHSCON_SIR_NAME         "dhsCon"
                                    /* Name of SIR record containing    */
                                    /* the status of the dhs connection */

#define   DET_CONTROL_ADC0_SIR_NAME           "adc0"
                                    /* Name of SIR record containing the*/
                                    /* ADC of the output 0              */
#define   DET_CONTROL_ADC1_SIR_NAME           "adc1"
                                    /* Name of SIR record containing the*/
                                    /* ADC of the output 1              */

#define   DET_CONTROL_OSCAN_SIR_NAME          "oscan"
                                    /* Name of SIR record containing    */
                                    /* the overscan number              */

#define   DET_CONTROL_HRWFS_SDSU_ADRS_VME     0x08000000   
                                    /* VME address of HRWFS SDSU controller   */
                                    /* If the controller is not installed its */
                                    /* address should be set to 0x0, and the  */
                                    /* controller will then be simulated      */
                                    /* MVME167 0xc0000020, POWERPC 0x08000000 */

#define   DET_CONTROL_HRWFS_MASK              0x8      
                                    /* Define the bit masks used to stop the  */
                                    /* detector control processes individually*/
                                    /* Bit 3 set                              */

#define DET_CONTROL_HRWFS_XSIZE               1024 
#define DET_CONTROL_HRWFS_YSIZE               1024
                                    /* Define the maximum data frame sizes    */
                                    /* for HRWFS                              */

#define DET_CONTROL_HRWFS_MAX_XSIZE           1072 
#define DET_CONTROL_HRWFS_MAX_YSIZE           1032
                                    /* Define the maximum storage frame sizes */
                                    /* for HRWFS                              */

#define DET_CONTROL_HRWFS_OSCAN_SIZE          0

                                    /* Define the column number for the       */
                                    /* overscan region for HRWFS              */

#define DET_CONTROL_HRWFS_MAX_FRAMES          1   
                                    /* Define the default number of SDSU data */
                                    /* buffers allocated for HRWFS            */

#define DET_CONTROL_MAX_WCSPOINTS             40 
                                    /* Max number of WCS calibration points.  */

#define DET_CONTROL_HRWFS_MK_INIT_FILE        "defDetContHRMK.dat"
                                    /* Define the MK default init file for    */
                                    /* HRWFS detector controller. Set to      */
                                    /* "NONE" if no default settings is       */
                                    /* required.                              */

#define DET_CONTROL_HRWFS_CP_INIT_FILE        "defDetContHRCP.dat"
                                    /* Define the CP default init file for    */
                                    /* HRWFS detector controller. Set to      */
                                    /* "NONE" if no default settings is       */
                                    /* required.                              */

#define   DET_CONTROL_OMF_FILE_PATH           "./bin/asm56000"
                                    /* Directory containing OMF files.        */

#define   DET_CONTROL_OMF_VME_FILE            "vme-47.lod"
                                    /* OMF file to download to VME DSP.       */

#define   DET_CONTROL_HRWFS_OMF_TIM_FILE      "tim-47.lod"
                                    /* OMF file to download to TIMING DSP.    */
                                    /* for HRWFS and acquisition camera.      */

#define   DET_CONTROL_OMF_UTL_FILE            "util.lod"
                                    /* OMF file to download to UTILITY DSP.   */

#define   DET_CONTROL_PAR_FILE_PATH           "./data"
                                    /* Define the name of the directory       */
                                    /* containing parameter files.            */

#define   DET_CONTROL_DATA_FILE_PATH          "."
                                    /* Define the default directory to contain*/
                                    /* engineering data files.                */

#define   DET_TYPE                            "CCD47+SDSUII"
                                    /* Define the default detector type       */

#define   DET_CCD_SN                          "8283-4-3"
                                    /* Define the SN of the CCD Chip          */

#define   DET_BUNIT 			      "SDSU ADC units"
                                    /* Define the units of CCD data           */

/********************************************************************* Enum ***/

enum
{
   NOT_INIT = 0,           /* DHS is not initialized                          */
   CONNECTED,              /* DHS is connected                                */
   NOT_CONNECTED           /* DHS is not connected                            */
};

/****************************************************************** Typedef ***/

typedef   struct      /* Context structure used to describe an observation.   */
{
                           /* SDSU context information.                       */
                           /* -------------------------                       */
   SDSU_ID      sdsuId;    /* SDSU context.                                   */
   BOOL         observing; /* Flag set TRUE when observing.                   */
   BOOL         stopped;   /* Flag set TRUE when observation stopped.         */
   BOOL         continuous;/* BUG WORK AROUND: Set TRUE whenever the SDSU     */
                           /* controller is in continuous mode.               */
   int          totalFrames;/* Total frames for observation.                  */
   int          outNFrames;/* Frame counter for output display.               */
   int          nframes;   /* Frame counter for this observation.             */
   SEM_ID       syncSem;   /* Observation synchronsisation semaphore.         */

                           /* Timer information.                              */
                           /* ------------------                              */
   timer_t      timeId;    /* ID of timer used to time observation in simu.   */

                           /* High level information.                         */
                           /* -----------------------                         */
   char         pWfsName [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                           /* Name of wavefront sensor.                       */
   char         pObsType [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                           /* Type of observation.                            */

                           /* Data handling information.                      */
                           /* --------------------------                      */
   int          outOptions;/* Output options (0=none, 1=DHS, 2=file).         */
   int          dhsOutOptions;/* DHS Output options (0=PERM, 1=TEMP, 2=QL).   */
   int          dhsCounter;/* Counter for frames to be sent to the QL         */
   int          dhsQlRate; /* Number of frames send to the DHS QL             */
   DHS_BD_DATASET dhsDataset; /* DHS dataset ID.                              */
   DHS_BD_FRAME dhsDataFrame; /* DHS data frame ID.                           */
   uint16*      pCurFrame; /* Pointer to current unscrambled data frame.      */
   uint16*      pDispFrame;/* Pointer to current data frame to be displayed.  */
                           /* if windowing, different from pCurFrame          */
   int          xPixelsDhs;/* Number of columns in pixels to be displayed     */
   int          yPixelsDhs;/* Number of rows in pixels to be displayed        */
   char         pDataLabel [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                           /* DHS data label.                                 */
   char         pQlStream [EPICS_MAX_BYTES_STRING_ATTRIB + 1];
                           /* DHS Quick look stream.                          */
   char         pOutFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                           /* Combined path name and file name for processed  */
                           /* data.                                           */
   char         pSimFileName [(EPICS_MAX_BYTES_STRING_ATTRIB + 1)*2];
                           /* Combined path name and file name for simulated  */
                           /* data.   */
                           /* (This file is used for engineering only).       */


                           /* Fits keywords                                   */
                           /* -------------                                   */

   char          ccdSize[22];
   char          ccdSec[22];
   char          ccdSec1[22];
   char          ccdSec2[22];
   char          dataSec1[22];
   char          dataSec2[22];
   char          biasSec1[22];
   char          biasSec2[22];
   int           timeArrayStart[7];/* Array of year/month/day/hour/min/sec    */
   char          utStartString[24];/* String which contains UTSTART data      */
   char          utDateStartString[11];/* String which contains UTSTART date  */
   char          utTimeStartString[13];/* String which contains UTSTART time  */
   int           timeArrayEnd[7];  /* Array of year/month/day/hour/min/sec    */
   char          utEndString[24];  /* String to contain UTEND data            */
   char          detType[16];
   char          detId[16];
   char          instName[40];   /* Name of the instrument                  */
   int           inport;           /* ISS port where AC/HRWFS is located      */

                           /* CCD geometry information.                       */
                           /* ------------------------                        */
   uint32      outputsNb ;             /* Number of sectors of the CCD        */
                                       /* By default T_OUTPUTS = 2            */
   int         xSize ;                 /* Columns number in pixels of the CCD */
                                       /* per output, T_XSIZE                 */
   int         ySize ;                 /* Rows number in pixels of the CCD    */
                                       /* per output, T_YSIZE                 */
   int         xMax ;                  /* Max column number in pixels of the  */
                                       /* CCD, xSize * ouputsNb               */
   int         yMax ;                  /* Max row number in pixels of the     */
                                       /* CCD, ySize                          */
   int         xStart ;                /* Number of columns to be discarded   */
                                       /* before the first subaperture        */
                                       /* T_XSTART                            */
   int         yStart ;                /* Number of rows to be discarded      */
                                       /* before the first subaperture        */
                                       /* T_YSTART                            */
   int         xBin ;                  /* Binning factor in X (by default 1)  */
                                       /* T_XBIN                              */
   int         yBin ;                  /* Binning factor in Y (by default 1)  */
                                       /* T_YBIN                              */
   int         xRaster ;               /* X size of a subaperture in binned   */
                                       /* pixels, T_XRAS                      */
   int         yRaster ;               /* Y size of a subaperture in binned   */
                                       /* pixels, T_YRAS                      */
   int         xSpace ;                /* Columns number in pixels to be      */
                                       /* discarded between subapertures      */
                                       /* T_XSPACE                            */
   int         ySpace ;                /* Rows number in pixels to be         */
                                       /* discarded between subapertures      */
                                       /* T_YSPACE                            */
   int         xSubapNb ;              /* Column number of subapertures per   */
                                       /* output, T_XSUBAP                    */
   int         ySubapNb ;              /* Row number of subapertures per      */
                                       /* output, T_YSUBAP                    */
   int         xPixels ;               /* Columns number in pixels of the     */
                                       /* image. Will be equal to xRaster *   */
                                       /* xSubapNb * outputsNb                */
   int         yPixels ;               /* Rows number in pixels of the image  */
                                       /* Will be equals to yRaster * ySubapNb*/
   int         pixelsNb ;              /* Total number of pixels of the image */
                                       /* Will be equal to xPixels * yPixels  */
                                       /* T_NPIXEL                            */
   int         uscanNb ;               /* Number of underscan pixels, (default*/
                                       /* is 8), T_USCAN                      */
   int         oscanNb ;               /* Number of ovsercan columns per      */
                                       /* output (0 to 8), (default is 0)     */
   int         xTail ;                 /* Remaining pixels to discard by row  */
                                       /* T_XTAIL                             */
   int         packetSize ;            /* Packet Size in pixels, V_PSIZE      */
   int         packetNb ;              /* Packet number, will be equal to:    */
                                       /* pixelsNb / VPSIZE                   */
   int         fullImageFlag ;         /* TRUE or FALSE, if binning and/or    */
                                       /* windowing                           */
   int         binningFlag ;           /* TRUE or FALSE, if binning or not    */
   int         windowingFlag ;         /* TRUE or FALSE, if windowing or not  */
   int         oscanFlag ;             /* FALSE, FULL or HALF, if no overscan */
                                       /* region, full ovsercan region or half*/
                                       /* overscan region                     */
   int         oscanHalfFlag ;         /* TRUE or FALSE, if windowing or not  */
   int         x1, x2 ;                /* In case of windowing x coordinates  */
                                       /* of the window                       */
   int         y1, y2 ;                /* In case of windowing y coordinates  */
                                       /* of the window                       */

                           /* CCD global information                          */
                           /* ----------------------                          */
   double      detTemp;                /* Detector temperature                */


                           /* Time stamps.                                    */
                           /* ------------                                    */
   double       rawtStart; /* Raw Gemini time at start of observation.        */
   double       rawtEnd;   /* Raw Gemini time at end of observation.          */
   double       expTime;   /* Current exposure time.                          */
   double       exposedRQ; /* Requested total exposure time.                  */
   double       exposed;   /* Actual total exposure time.                     */
   double       frameTime; /* Frame time.                                     */

                           /* World Coordinate System (WCS) information.      */
                           /* ------------------------------------------      */
   int          wcsStatus; /* WCS status (0 if WCS information OK).           */
   int          nWcsPoints;/* Number of WCS calibration points.               */
   double       fpxy[DET_CONTROL_MAX_WCSPOINTS][2];
                           /* Array of defined focal plane XY coordinates     */
   double       pixij[DET_CONTROL_MAX_WCSPOINTS][2];
                           /* Array of measured pixel IJ coordinates          */
   double       detij[DET_CONTROL_MAX_WCSPOINTS][2];
                           /* Array of pixel IJ coordinates applying to data  */
                           /* read from the detector.                         */
   double       cij[6];    /* XY to IJ transformation matrix.                 */
   char         ctype1[9]; /* WCS projection type for axis 1.                 */
   double       crpix1;    /* Pixel coordinate reference for axis 1.          */
   double       crval1;    /* World coordinate reference for axis 1.          */
   char         ctype2[9]; /* WCS projection type for axis 2.                 */
   double       crpix2;    /* Pixel coordinate reference for axis 2.          */
   double       crval2;    /* World coordinate reference for axis 2.          */
   double       cd1_1;     /* xi rotation/skew matrix element.                */
   double       cd1_2;     /* xj rotation/skew matrix element.                */
   double       cd2_1;     /* yi rotation/skew matrix element.                */
   double       cd2_2;     /* yj rotation/skew matrix element.                */
   double       RA;        /* Right Ascension in hours.                       */
   double       Dec;       /* Declination in degrees.                         */

   char         radecsys[9];/* Type of RA/Dec (for celestial coordinate).     */
   double       equinox;   /* Epoch of mean equator & equinox (celestial      */
                           /* coords).                                        */
   double       epoch;     /* Epoch of observation as a year.                 */

   double       mjdobs;    /* Epoch of observation as a modified Julian date. */
   char         frame[10]; /* Target coordinate system.                       */

                           /* SAD information.                                */
                           /* ----------------                                */

   DATREC_CONTEXT pStateContext;      /* Context structure for state SIR      */
                                      /* record.                              */
   DATREC_CONTEXT pDetInitContext;    /* Context structure for initialising   */
                                      /* state SIR record.                    */
   DATREC_CONTEXT pDetInitStatusContext;
                                      /* Context structure for SDSU           */
                                      /* initialisation status SIR record     */
   DATREC_CONTEXT pTestResultsContext;/* Context structure for SDSU test      */
                                      /* results SIR record.                  */
   DATREC_CONTEXT pTestingContext;    /* Context structure for testing state  */
                                      /* SIR record.                          */
   DATREC_CONTEXT pDetPrimReplyContext;  
                                      /* Context structure for SDSU primitive */
                                      /* reply string SIR record              */
   DATREC_CONTEXT pObsTypeContext;    /* Context structure for observation    */
                                      /* type SIR record.                     */
   DATREC_CONTEXT pDetTypeContext;    /* Context structure for detector       */
                                      /* controller type.                     */
   DATREC_CONTEXT pDetIdContext;      /* Context structure for detector Id or */
                                      /* SN                                   */
   DATREC_CONTEXT pBunitContext ;     /* Data unit SIR record context         */
                                      /* structure                            */
   DATREC_CONTEXT pDetObservingContext;
                                      /* Observing record context.            */
   DATREC_CONTEXT pObsModeContext ;   /* Observation mode SIR record context  */
                                      /* structure                            */
   DATREC_CONTEXT pDataLabelContext ; /* Data Label SIR record context        */
                                      /* structure                            */
   DATREC_CONTEXT pIntTimeContext ;   /* Integration time SIR record context  */
                                      /* structure                            */
   DATREC_CONTEXT pNExpRQContext ;    /* Requested number of exp/data set SIR */
                                      /* record context structure             */
   DATREC_CONTEXT pNExpContext ;      /* Actual number of exp/data set SIR    */
                                      /* record context structure             */
   DATREC_CONTEXT pNFramesContext ;   /* Number of frames/data set SIR        */
                                      /* record context structure             */
   DATREC_CONTEXT pUTstartContext ;   /* UT at start of observation SIR record*/
                                      /* context structure                    */
   DATREC_CONTEXT pUTendContext ;     /* UT at end of observation SIR record  */
                                      /* context structure                    */
   DATREC_CONTEXT pExposedRQContext ; /* Requested total integration time SIR */
                                      /* record context structure             */
   DATREC_CONTEXT pExposedContext ;   /* Actual total integration time SIR    */
                                      /* record context structure             */
   DATREC_CONTEXT pElapsedContext ;   /* Actual elapsed time SIR record       */
                                      /* context structure                    */
   DATREC_CONTEXT pOutputsContext ;   /* Number of outputs SIR record         */
                                      /* context structure                    */
   DATREC_CONTEXT pDetXsizeContext ;  /* X detector size SIR record context   */
                                      /* structure                            */
   DATREC_CONTEXT pDetYsizeContext ;  /* Y detector size SIR record context   */
                                      /* structure                            */
   DATREC_CONTEXT pXsubapContext ;    /* X subaperture size SIR record        */
                                      /* context structure                    */
   DATREC_CONTEXT pYsubapContext ;    /* Y subaperture size SIR record        */
                                      /* context structure                    */
   DATREC_CONTEXT pXstartContext ;    /* X left offset SIR record context     */
                                      /* structure                            */
   DATREC_CONTEXT pYstartContext ;    /* Y bottom offset SIR record context   */
                                      /* structure                            */
   DATREC_CONTEXT pXrasterContext ;   /* X subaperture size SIR record        */
                                      /* context structure                    */
   DATREC_CONTEXT pYrasterContext ;   /* Y subaperture size SIR record        */
                                      /* context structure                    */
   DATREC_CONTEXT pXspaceContext ;    /* X space between subapertures SIR     */
                                      /* record context structure             */
   DATREC_CONTEXT pYspaceContext ;    /* Y space between subapertures SIR     */
                                      /* record context structure             */
   DATREC_CONTEXT pXbinContext ;      /* X binning factor SIR record context  */
                                      /* structure                            */
   DATREC_CONTEXT pYbinContext ;      /* Y binning factor SIR  record context */
                                      /* structure                            */
   DATREC_CONTEXT pDhsConContext ;    /* dhs connection status SIR record     */
                                      /* context structure                    */
   DATREC_CONTEXT pAdc0Context ;      /* ADC 0 SIR record context structure   */
   DATREC_CONTEXT pAdc1Context ;      /* ADC 1 SIR record context structure   */
   DATREC_CONTEXT pOscanContext ;     /* oscan SIR record context structure   */
} OBS_ID_STRUCT, * OBS_ID;

   /*
    * Error number codes used by detControl.
    * These are designed to be processed using the vxWorks "makeStatTbl" utility
    */

#define S_detControl_BAD_COMMAND   (M_detControl | 1) /* Unrecognised command */
#define S_detControl_BAD_WFS_NAME  (M_detControl | 2) /* Unrecognised WFS name*/
#define S_detControl_BAD_ATTRIBUTE (M_detControl | 3) /* Bad attribute value  */
#define S_detControl_BAD_FILE      (M_detControl | 4) /* Bad parameter file   */
#define S_detControl_SDSU_ERROR    (M_detControl | 5) /* Error from SDSU      */
                                                      /* controller           */
#define S_detControl_DHS_ERROR     (M_detControl | 6) /* Error from DHS       */
#define S_detControl_INTERNAL      (M_detControl | 7) /* detControl internal  */
                                                      /* failure              */
#define S_detControl_BUSY          (M_detControl | 8) /* Controller busy      */
#define S_detControl_NO_ACCESS     (M_detControl | 9) /* No access to hardware*/
#define S_detControl_WCS_ERROR     (M_detControl | 10)/* Error in WCS         */
                                                      /* calculation          */

   /*
    * Define the commands recognised by the detector controller task.
    */

enum
   {

   DET_CONTROL_CMD_SETUP = 0,  /* Set up SDSU controller parameters.          */
   DET_CONTROL_CMD_EXPOSURE,   /* Specify exposure time.                      */
   DET_CONTROL_CMD_OBSTYPE,    /* Specify observation type.                   */
   DET_CONTROL_CMD_DHSINFO,    /* Specify quick look stream.                  */
   DET_CONTROL_CMD_SETOBSERVE, /* Specify quick look stream.                  */
   DET_CONTROL_CMD_SETDHS,     /* Set Data Handling System parameters.        */
   DET_CONTROL_CMD_SETWCS,     /* Set World Coordinate System parameters.     */
   DET_CONTROL_CMD_DETOBSERVE, /* Make observation.                           */
   DET_CONTROL_CMD_OBSERVE,    /* OBSERVE command for the OCS                 */
   DET_CONTROL_CMD_PAUSE,      /* Pause observation.                          */
   DET_CONTROL_CMD_CONTINUE,   /* Continue observation.                       */
   DET_CONTROL_CMD_STOP,       /* Stop observation.                           */
   DET_CONTROL_CMD_ABORT,      /* Abort observation.                          */
   DET_CONTROL_CMD_INITIALISE, /* Initialise SDSU controller.                 */
   DET_CONTROL_CMD_RESET,      /* Reset SDSU controller.                      */
   DET_CONTROL_CMD_TEST,       /* Test SDSU controller                        */
   DET_CONTROL_CMD_SAVE,       /* Save SDSU controller parameters.            */
   DET_CONTROL_CMD_GEOMETRY,   /* Set detector readout geometry.              */
   DET_CONTROL_CMD_PRIMITIVE,  /* Execute SDSU primitive command.             */
   DET_CONTROL_CMD_POWER_ON,   /* Execute POWER ON primitive command.         */
   DET_CONTROL_CMD_DOWNLOAD,   /* Download DSP code.                          */
   DET_CONTROL_CMD_MODE,       /* Set detector readout mode.                  */
   DET_CONTROL_CMD_OFFSET,     /* Set detector ADC offsets.                   */
   DET_CONTROL_CMD_TEMP,       /* Define temperature control params.          */
   DET_CONTROL_CMD_FRAME_SIZE, /* Define frame size                           */
   DET_CONTROL_CMD_DHS_RECONNECT, 
                               /* Set connection with the DHS                 */
   DET_CONTROL_CMD_DHS_DISPLAY,/* Set display parameters for dhs QL           */
   DET_CONTROL_CMD_DEBUG,      /* Set debugging mode                          */
   DET_CONTROL_CMD_SIMULATE    /* Set simulation mode                         */
   };

   /* Public variables */

IMPORT BOOL        detDhsInitialised;
IMPORT SDSU_ID     detSdsuIdHr;     /* SDSU context structure for HRWFS.      */

   /* Public functions */

IMPORT void        detShow (const char * pWfsName, const BOOL verbose);
IMPORT void        detStatusShow (const char * pWfsName);
IMPORT void        detTempShow (const char * pWfsName);
IMPORT STATUS      detObsShow (OBS_ID obsId, const BOOL verbose);
IMPORT void        detDhsErrorCallback (DHS_CONNECT connect,
                                        DHS_STATUS errorNum,
                                        DHS_ERR_LEVEL errorLev, char * msg,
                                        DHS_TAG tag, void * userData);
IMPORT STATUS      detDhsParamInit (const char * pClientName, 
                                    const int numConnect,
                                    const char * pHostName,
                                    const char * pServerName);
