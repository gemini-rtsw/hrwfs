/*+
 *   MODULE NAME:
 *   seqControl
 *
 *   FILENAME:
 *   seqControl.h
 *
 *   PURPOSE:
 *   Include file for HRWFS sequencer application code
 *
 *   HISTORY MODIFICATION:
 *   31 Jan 2000 - cb rename wfsControl with seqControl
 *   20 Jan 2000 - cb add "verifying" and "guiding" SIR RECORD
 *                    add "endVerifying" SIR RECORD
 *                    add "endGuiding" SIR RECORD
 *                    add "endObserving" SIR RECORD
 *   19 Jan 2000 - cb add "rebooting" SIR RECORD
 *                    add "parking" and "datuming" SIR RECORD
 *-
 */

#ifndef   __INCseqControlh
#define   __INCseqControlh


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */

#include "wfsLib.h"
#include "gemTypes.h"
#include "gemModNum.h"


/* defines */

#define SEQ_CONTROL_TASK_NAME      "seqControl"    /* Sequencer control task  */
                                                   /* name                    */

#define SEQ_CONTROL_STATE_SIR_NAME "controlState"  /* Name of SIR record to   */
                                                   /* contain sequencer state */

#define SEQ_CONTROL_HISTORYLOG_SIR_NAME "historyLog"  
                                                   /* Name of SIR record to   */
                                                   /* contain seq history log */

#define SEQ_CONTROL_INIT_SIR_NAME  "initialising"  /* Name of SIR record to   */
                                                   /* contain sequencer       */
                                                   /* initialisation state.   */

#define SEQ_CONTROL_TEST_SIR_NAME  "testing"       /* Name of SIR record to   */
                                                   /* contain sequencer test  */
                                                   /* state.                  */

#define SEQ_CONTROL_MEAS_SIR_NAME  "measuring"     /* Name of SIR record to   */
                                                   /* contain sequencer       */
                                                   /* wavefront measurement   */
                                                   /* state.                  */

#define SEQ_CONTROL_REBOOT_SIR_NAME "rebooting"    /* Name of SIR record to   */
                                                   /* contain sequencer       */
                                                   /* rebooting state.        */

#define SEQ_CONTROL_PARK_SIR_NAME "parking"        /* Name of SIR record to   */
                                                   /* contain sequencer       */
                                                   /* park state.             */

#define SEQ_CONTROL_DATUM_SIR_NAME "datuming"      /* Name of SIR record to   */
                                                   /* contain sequencer       */
                                                   /* datum state.            */

#define SEQ_CONTROL_VERIFY_SIR_NAME "verifying"    /* Name of SIR record to   */
                                                   /* contain sequencer       */
                                                   /* verify state.           */

#define SEQ_CONTROL_ENDVERIFY_SIR_NAME "endVerifying" /* Name of SIR record to*/
                                                   /* contain sequencer       */
                                                   /* endVerify state.        */

#define SEQ_CONTROL_GUIDE_SIR_NAME "guiding"       /* Name of SIR record to   */
                                                   /* contain sequencer       */
                                                   /* guide state.            */

#define SEQ_CONTROL_ENDGUIDE_SIR_NAME "endGuiding" /* Name of SIR record to   */
                                                   /* contain sequencer       */
                                                   /* endGuide state.         */

#define SEQ_CONTROL_ENDOBSERVE_SIR_NAME "endObserving" 
                                                   /* Name of SIR record to   */
                                                   /* contain sequencer       */
                                                   /* endObserve state.       */

#define SEQ_CONTROL_HEALTH_SIR_NAME "controlHealth" /* Name of SIR record to  */
                                                   /* contain sequencer       */
                                                   /* health.                 */


#define SEQ_CONTROL_OBSMODE_SIR_NAME "obsMode"     /* Name of SIR record to   */
                                                   /* contain observation mode*/

#define SEQ_CONTROL_OBSTYPE_SIR_NAME "obsType"     /* Name of SIR record to   */
                                                   /* contain observation type*/

   /*
    * Error number codes used by seqControl.
    * These are designed to be processed using the vxWorks "makeStatTbl" 
    * utility.
    */

#define S_seqControl_BAD_COMMAND (M_seqControl | 1)   /* Unrecognised command */

/*
 * Define the commands recognised by the sequencer control task.
 */

enum
   {
   SEQ_CONTROL_CMD_INIT = 0,        /* Initialise HRWFS sensor and mechanisms */
   SEQ_CONTROL_CMD_TEST,            /* Test HRWFS sensor and mechanisms       */
   SEQ_CONTROL_CMD_PARK,            /* Park HRWFS mechanisms                  */
   SEQ_CONTROL_CMD_REBOOT,          /* Reboot sequencer and HRWFS DC          */
   SEQ_CONTROL_CMD_DATUM,           /* Datum HRWFS mechanisms                 */
   SEQ_CONTROL_CMD_VERIFY,          /* Verify command                         */
   SEQ_CONTROL_CMD_ENDVERIFY,       /* EndVerify command                      */
   SEQ_CONTROL_CMD_GUIDE,           /* Guide command                          */
   SEQ_CONTROL_CMD_ENDGUIDE,        /* EndGuide command                       */
   SEQ_CONTROL_CMD_ENDOBSERVE       /* EndObserve command                     */
   };


/* function declarations */

#ifndef   NO_EPICS
IMPORT STATUS   seqControl (void);
#endif /* NO_EPICS */

#endif /* __INCseqControlh */
