/*+
 *   MODULE NAME:
 *   wfsLib
 *
 *   FILENAME:
 *   wfsLib.h
 *
 *   PURPOSE:
 *   Include file for wfsLib
 *
 *   MODIFICATION
 *   - 10 dec 1999 - cb tidy up
 *   - A lot of smb modifications before UK delivery (see CVS for previous 
 *     versions)
 *   - Creation 28 Nov 1997 by UK
 *-
 */

#ifndef   __INCwfsLibh
#define   __INCwfsLibh


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#endif   /* vxWorks */

#include "gemTypes.h"
#include "gemModNum.h"


/* defines */

#define PIPE_DRV_TIMEOUT_PROC_0 120.0  /* Pipe driver timeout (in seconds) for   */
                                       /* processor 0.                        */
#define PIPE_DRV_TIMEOUT_PROC_N -1.0   /* Pipe driver timeout for other       */
                                       /* processors. (A negative value means */
                                       /* means infinite timeout).            */
   /*
    * Error number codes used by wfsLib.
    * These are designed to be processed using the vxWorks "makeStatTbl" utility
    */

#define S_wfsLib_ERRLOG_PIPE_FAIL (M_wfsLib | 1) /* Pipe create or open failed*/
#define S_wfsLib_INVALID_DATA     (M_wfsLib | 2) /* Invalid data structure    */
                                                 /* defined.                  */
#define S_wfsLib_BAD_ARGUMENT     (M_wfsLib | 3) /* Bad argument supplied.    */
#define S_wfsLib_NOT_CONNECTED    (M_wfsLib | 4) /* Database not connected.   */

   /*
    * The following defines a data structure to contain information about
    * the processor used by each wavefront sensor control task.
    */

typedef struct
{
   char   pProcName [80];              /* Processor name.                     */
   uint32 targetType;                  /* Processor type (e.g.                */
                                       /* TARGET_TYPE_MV167, as defined in    */
                                       /* gemTypes.h).                        */
   uint32 procIpAddrs;                 /* Processor IP address.               */
   float  procClockRate;               /* Processor clock rate in Hz.         */
   uint32 procRamSize;                 /* Processor RAM size in bytes.        */
   } WFS_ARCH_PROCESSOR;


   /*
    * The following defines a data structure to contain information about
    * the Component Controller information
    */

typedef struct
{
   char   clFilterName [40];           /* AC/HRWFS Color filter name.         */
   char   ndFilterName [40];           /* AC/HRWFS ND filter name.            */
   char   lensName [40];               /* AC/HRWFS Lens name.                 */
   char   fldStopName [40];            /* AC/HRWFS Field Stop name.           */
   char   calName [40];                /* AC/HRWFS Calibration Source name.   */
   double focusPos;                    /* AC/HRWFS Focus Position (mm).       */
} AC_CC_STRUCT;

/* function declarations */

IMPORT STATUS wfsLibInit (void);
IMPORT STATUS wfsTargetTypeGet (const int processorNumber);
IMPORT int    wfsNumProcsGet (void);
IMPORT STATUS wfsSysInit (const int processorNumber, 
                          const BOOL redirectErrorLog);
IMPORT STATUS wfsShow (void);
IMPORT void   wfsGetTelName (char *pTelName);
IMPORT void   showAcCCStruct ();

#endif /* __INCwfsLibh */
