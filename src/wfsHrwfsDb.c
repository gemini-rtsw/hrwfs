static struct {void *v; char *c;} rcsid = {&rcsid,
	"$Id: wfsHrwfsDb.c,v 1.19 2006/07/31 19:50:33 gemvx Exp $"};

/*+
 *   MODULE NAME:
 *   wfsHrwfsDb
 *
 *   FILENAME:
 *   wfsHrwfsDb.c
 *
 *   PURPOSE:
 *   Wavefront sensor database definition - HRWFS
 *
 *   DESCRIPTION:
 *   This module initialises the data structures which describe the records
 *   contained in the EPICS database and the properties of the commands
 *   associated with those records. The data structure arrays are declared in
 *   "wfsDb.h" and their contents declared in "dbTypes.h". A record cannot
 *   by accessed by the epToVxLib library unless it is declared here.
 *
 *   The database is initialised in wfsDb.c rather than wfsDb.h because of
 *   the programming convention that header files only declare objects and do
 *   not allocate memory space
 *
 *   FUNCTION NAME(S):
 *   None
 *
 *   DEFICIENCIES:
 *   This module assumes that all EPICS records have the same prefix, whereas
 *   this is not the case for HRWFS/AC records.
 *
 *   The default values of attributes for the detGeometry command should be 
 *   set to values derived from the detector properties rather than fixed 
 *   values. 
 *
 *   NOTE:
 *   This file only defines the records that the WFS epToVxLib library needs to
 *   know about. There may be other EPICS records - see the Capfast schematics.
 *
 *   I am concerned that all the EPICS database definitions need to be
 *   duplicated here and in the Capfast schematics, as there is a risk the
 *   two definitions will diverge. Can the database information be extracted
 *   from the files generated from the Capfast schematics, or at least
 *   downloaded from a file or defined in function calls at boot time?
 *   SMB - 26 Nov 97.
 *
 *   Note that the initialisers in this module do not necessarily fill
 *   all of a CAD record structure. For example, if a CAD record does not
 *   have any attributes, then its attribute structure is not initialised.
 *   This feature may generate warnings with some compilers. It is assumed
 *   that uninitialised parts of the data structures will be filled with
 *   zero or NULL values.
 *
 *   IMPORTANT:
 *   The record names and fields declared in this file should exactly
 *   match the names and fields for those same records as defined in
 *   the Capfast schematics
 *
 *   ORIGINAL AUTHOR:
 *   Nick Dillon
 *
 *   MODIFIED BY:
 *   Steven Beard
 *
 *   HISTORY MODIFICATION
 *   08 Oct 2002 - cb add detPowerOff
 *   21 Mar 2002 - cb modify init 
 *   09 Jan 2002 - cb add detPowerOn
 *   05 Oct 2001 - cb setDhsInfo: dhsOutOptions = (0,3)
 *   14 Jun 2001 - cb detDhsReconnect modify timeout to be NO_TIMEOUT
 *   01 Jun 2001 - cb add overscan region: detGeometry and detFrameSize are
 *                    modified, add oscan sir record
 *   03 Apr 2001 - cb add adc0, adc1
 *   19 feb 2001 - cb add sir dhsCon
 *   16 feb 2001 - cb add cad detDhsDisplay
 *   15 may 2000 - cb add detDhsReconnect cad record
 *   01 mar 2000 - cb work on historyLog of seq and debug and simulate...
 *   11 feb 2000 - cb add dc:exposed, dc:exposedRQ, dc:utstart, dc:utend, 
 *                    dc:elapsed
 *                    + add all the sir record containing the detector geometry
 *   31 jan 2000 - cb add obsType, obsMode, dc:detType, dc:detID sirs
 *   21 jan 2000 - cb add command setObserve
 *   20 jan 2000 - cb add verify and verifying, endVerify and 
 *                    endVerifying, guide and guiding
 *                    endGuiding and endGuide 
 *                    endObserving and endObserve 
 *   19 jan 2000 - cb add rebooting, testing sir records
 *                    add parking, datum, datuming records
 *                    add test command
 *   13 jan 2000 - cb remove include osp.h file
 *   18 nov 1999 - cb new setDhsInfo command 
 *   25 oct 1999 - cb new observe command 
 *   14 oct 1999 - cb simplified version for HRWFS only
 *-
 */


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */


#include "dbTypes.h"
#include "wfsHrwfsDb.h"
#include "wfsLib.h"
#include "epToVxLib.h"
#include "detControl.h"    /* This is where DET_CONTROL_ parameters come from.*/
#include "seqControl.h"    /* This is where SEQ_CONTROL_ parameters come from.*/
#include "errorLog.h"      /* This is where LOGTASK_ parameters comes from.   */

char ioc_path[EPICS_MAX_BYTES_STRING_ATTRIB];

/* The pWfsDbCadList data structure array contains information on the CAD 
 * records recognised by the system, and the commands associated with them. Each
 * CAD record is described by the following information:
 * - Record name (excluding the system prefix).
 * - Name of task to receive commands from that record.
 * - Command number associated with that record.
 * - Flag indicating whether the stop directive is supported.
 * - Flag indicating whether the command can run in simulation mode.
 * - Command timeout in seconds.
 * - List of command attributes, together with the data type, default value
 *   and allowed range for each attribute. There can be zero or more attributes.
 */

CAD_RECORD pWfsDbCadList [] =
{
   {
      RECORD_NAME ("init"),
      SEQ_CONTROL_TASK_NAME,
      SEQ_CONTROL_CMD_INIT,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      NO_TIMEOUT
   },
   {
      RECORD_NAME ("park"),
      SEQ_CONTROL_TASK_NAME,
      SEQ_CONTROL_CMD_PARK,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      NO_TIMEOUT
   },
   {
      RECORD_NAME ("reboot"),
      SEQ_CONTROL_TASK_NAME,
      SEQ_CONTROL_CMD_REBOOT,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      NO_TIMEOUT
   },
   {
      RECORD_NAME ("datum"),
      SEQ_CONTROL_TASK_NAME,
      SEQ_CONTROL_CMD_DATUM,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      NO_TIMEOUT
   },
   {
      RECORD_NAME ("verify"),
      SEQ_CONTROL_TASK_NAME,
      SEQ_CONTROL_CMD_VERIFY,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      NO_TIMEOUT
   },
   {
      RECORD_NAME ("endVerify"),
      SEQ_CONTROL_TASK_NAME,
      SEQ_CONTROL_CMD_ENDVERIFY,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      NO_TIMEOUT
   },
   {
      RECORD_NAME ("guide"),
      SEQ_CONTROL_TASK_NAME,
      SEQ_CONTROL_CMD_GUIDE,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      NO_TIMEOUT
   },
   {
      RECORD_NAME ("endGuide"),
      SEQ_CONTROL_TASK_NAME,
      SEQ_CONTROL_CMD_ENDGUIDE,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      NO_TIMEOUT
   },
   {
      RECORD_NAME ("endObserve"),
      SEQ_CONTROL_TASK_NAME,
      SEQ_CONTROL_CMD_ENDOBSERVE,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      NO_TIMEOUT
   },
   {
      RECORD_NAME ("test"),
      SEQ_CONTROL_TASK_NAME,
      SEQ_CONTROL_CMD_TEST,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      120.0
   },
   {
      RECORD_NAME ("dc:simulate"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_SIMULATE,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_UNSUPPORTED,
      10.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, ATTRIB (EPTOVX_SIM_MODE_NONE), {ATTRIB (EPTOVX_SIM_MODE_VSM), ATTRIB (EPTOVX_SIM_MODE_NONE)}
   },
   {
      RECORD_NAME ("dc:debug"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_DEBUG,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_UNSUPPORTED,
      10.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, ATTRIB (EPTOVX_DEBUG_MODE_NONE), {ATTRIB (EPTOVX_DEBUG_MODE_NONE), ATTRIB (EPTOVX_DEBUG_MODE_FULL)}
   },
   {
      RECORD_NAME ("dc:detSetup"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_SETUP,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      120.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, DET_CONTROL_PAR_FILE_PATH, {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, "hrparams.par", {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG, "-1", {"-1", "3"}
   },
   {
      RECORD_NAME ("dc:detExposure"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_EXPOSURE,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      30.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG,    "-1",    {"-1", NO_HI_LIMIT},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_DOUBLE,  "1.0",  {"0.0001", "100000.0"}
   },
   {
      RECORD_NAME ("dc:detObstype"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_OBSTYPE,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      40.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, "UNDEFINED", {NO_ATTRIBUTE_LIMITS}
   },
   {
      RECORD_NAME ("dc:setDhsInfo"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_DHSINFO,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      40.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, "hrwfsScience", {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG,   "2", {"0", "3"}
   },
   {
      RECORD_NAME ("dc:detSetWcs"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_SETWCS,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      120.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, DET_CONTROL_PAR_FILE_PATH,   {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, "hrcalib.wcs",            {NO_ATTRIBUTE_LIMITS}
   },
   {
      RECORD_NAME ("dc:setObserve"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_SETOBSERVE,
      STOP_DIRECTIVE_SUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      120.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG,   "1", {"0", "2"},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, ioc_path, {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING, "hrwfs.fits", {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_D, EPICS_DATA_TYPE_STRING, "NONE", {NO_ATTRIBUTE_LIMITS}
   },
   {
      RECORD_NAME ("observe"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_OBSERVE,
      STOP_DIRECTIVE_SUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      120.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, "NONE", {NO_ATTRIBUTE_LIMITS}
   },
   {
      RECORD_NAME ("dc:detObserve"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_DETOBSERVE,
      STOP_DIRECTIVE_SUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      120.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG,    "-1",    {"-1", NO_HI_LIMIT},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_DOUBLE,  "1.0",  {"0.005", "100000.0"},
      CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG,   "1", {"0", "2"},
      CAD_ATTRIB_D, EPICS_DATA_TYPE_STRING, "NONE", {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_E, EPICS_DATA_TYPE_LONG,   "2", {"0", "3"},
      CAD_ATTRIB_F, EPICS_DATA_TYPE_STRING, ioc_path, {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_G, EPICS_DATA_TYPE_STRING, "hrwfs.fits", {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_H, EPICS_DATA_TYPE_STRING, "NONE", {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_I, EPICS_DATA_TYPE_DOUBLE, "0.0", {"0.0", NO_HI_LIMIT}
   },
   {
      RECORD_NAME ("stop"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_STOP,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      120.0
   },
   {
      RECORD_NAME ("abort"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_ABORT,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      120.0
   },
   {
      RECORD_NAME ("dc:detTest"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_TEST,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      120.0
   },
   {
      RECORD_NAME ("dc:detInit"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_INITIALISE,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      180.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, ATTRIB (DET_CONTROL_HRWFS_SDSU_ADRS_VME), {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, DET_CONTROL_OMF_FILE_PATH, {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING, DET_CONTROL_OMF_VME_FILE, {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_D, EPICS_DATA_TYPE_STRING, "tim-47.lod", {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_E, EPICS_DATA_TYPE_STRING, "util-47.lod", {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_F, EPICS_DATA_TYPE_LONG, ATTRIB (DET_CONTROL_HRWFS_MAX_FRAMES), {"0", "100"},
  CAD_ATTRIB_G, EPICS_DATA_TYPE_LONG,"1" , {"0", "1"},
  CAD_ATTRIB_H, EPICS_DATA_TYPE_LONG,"0" , {"0", "1"}
   },
   {
      RECORD_NAME ("dc:detReset"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_RESET,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      60.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG,    "1",                     {"0", "1"},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG,      "1",                     {"0", "1"},
      CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING,   DET_CONTROL_OMF_FILE_PATH,      {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_D, EPICS_DATA_TYPE_STRING,   DET_CONTROL_OMF_VME_FILE,      {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_E, EPICS_DATA_TYPE_STRING,   DET_CONTROL_HRWFS_OMF_TIM_FILE,   {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_F, EPICS_DATA_TYPE_STRING,   DET_CONTROL_OMF_UTL_FILE,      {NO_ATTRIBUTE_LIMITS}
   },
   {
      RECORD_NAME ("dc:detSave"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_SAVE,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      120.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING,    DET_CONTROL_PAR_FILE_PATH,   {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING,    "hrnewparams.par",         {NO_ATTRIBUTE_LIMITS},
      CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG,      "-1",                  {"-1", "3"}
   },
   {
      RECORD_NAME ("dc:detGeometry"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_GEOMETRY,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      20.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG,    "1",         {"1", "40"},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG,    "1",         {"1", "40"},
      CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG,    "512",         {"1", ATTRIB (DET_CONTROL_HRWFS_XSIZE)},
      CAD_ATTRIB_D, EPICS_DATA_TYPE_LONG,    "1024",         {"1", ATTRIB (DET_CONTROL_HRWFS_YSIZE)},
      CAD_ATTRIB_E, EPICS_DATA_TYPE_LONG,    "1",         {"1", ATTRIB (DET_CONTROL_HRWFS_XSIZE)},
      CAD_ATTRIB_F, EPICS_DATA_TYPE_LONG,    "1",         {"1", ATTRIB (DET_CONTROL_HRWFS_YSIZE)},
      CAD_ATTRIB_G, EPICS_DATA_TYPE_LONG,    "16",         {"0", ATTRIB (DET_CONTROL_HRWFS_XSIZE)},
      CAD_ATTRIB_H, EPICS_DATA_TYPE_LONG,    "1",         {"0", ATTRIB (DET_CONTROL_HRWFS_YSIZE)},
      CAD_ATTRIB_I, EPICS_DATA_TYPE_LONG,    "0",         {"0", ATTRIB (DET_CONTROL_HRWFS_XSIZE)},
      CAD_ATTRIB_J, EPICS_DATA_TYPE_LONG,    "0",         {"0", ATTRIB (DET_CONTROL_HRWFS_YSIZE)},
      CAD_ATTRIB_K, EPICS_DATA_TYPE_LONG,    "0",         {"0", "8"}
   },
   {
      RECORD_NAME ("dc:detPrim"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_PRIMITIVE,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      120.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING,   "TDL",         {NO_ATTRIBUTE_LIMITS},
                                                         /* Allow any command. */
      CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG,      "1",         {"1", "3"},
      CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG,      "0x55aaff",      {"0",   NO_HI_LIMIT},
      CAD_ATTRIB_D, EPICS_DATA_TYPE_LONG,      "0",         {"0",   NO_HI_LIMIT},
      CAD_ATTRIB_E, EPICS_DATA_TYPE_LONG,      "0",         {"0",   NO_HI_LIMIT},
      CAD_ATTRIB_F, EPICS_DATA_TYPE_LONG,      "0",         {"0",   NO_HI_LIMIT},
      CAD_ATTRIB_G, EPICS_DATA_TYPE_LONG,      "0",         {"0",   NO_HI_LIMIT},
      CAD_ATTRIB_H, EPICS_DATA_TYPE_LONG,      "0",         {"0",   NO_HI_LIMIT}
   },
   {
      RECORD_NAME ("dc:detMode"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_MODE,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      40.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG,    "-1",         {"-1",   NO_HI_LIMIT},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG,    "-1",         {"-1",   NO_HI_LIMIT},
      CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG,      "-1",         {"-1",   "0xFFF"},
      CAD_ATTRIB_D, EPICS_DATA_TYPE_LONG,    "-1",         {"-1",   NO_HI_LIMIT}
   },
   {
      RECORD_NAME ("dc:detOffset"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_OFFSET,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      40.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG,    "-1",         {"-1",   NO_HI_LIMIT},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG,    "-1",         {"-1",   NO_HI_LIMIT}
   },
#if (MK)
   {
      RECORD_NAME ("dc:detTemp"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_TEMP,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      40.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_DOUBLE,    "-20",         {"-63", "25"},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG,      "0x80",         {NO_ATTRIBUTE_LIMITS}
   },
#else
   {
      RECORD_NAME ("dc:detTemp"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_TEMP,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      40.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_DOUBLE,    "-30",         {"-63", "25"},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG,      "0x80",         {NO_ATTRIBUTE_LIMITS}
   },
#endif
   {
      RECORD_NAME ("dc:detPowerOn"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_POWER_ON,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      40.0
   },
   {
      RECORD_NAME ("dc:detPowerOff"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_POWER_OFF,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      40.0
   },
   {
      RECORD_NAME ("dc:detFrameSize"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_FRAME_SIZE,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      20.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG,    "0",         {"0", "1"},
      CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG,    "0",         {"0", "1"},
      CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG,    "512",         {"1", ATTRIB (DET_CONTROL_HRWFS_XSIZE+1)},
      CAD_ATTRIB_D, EPICS_DATA_TYPE_LONG,    "512",         {"1", ATTRIB (DET_CONTROL_HRWFS_XSIZE+1)},
      CAD_ATTRIB_E, EPICS_DATA_TYPE_LONG,    "100",         {"1", ATTRIB (DET_CONTROL_HRWFS_YSIZE+1)},
      CAD_ATTRIB_F, EPICS_DATA_TYPE_LONG,    "100",         {"1", ATTRIB (DET_CONTROL_HRWFS_YSIZE+1)},
      CAD_ATTRIB_G, EPICS_DATA_TYPE_LONG,    "0",         {"0", "8"},
   },
   {
      RECORD_NAME ("dc:detDhsReconnect"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_DHS_RECONNECT,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      NO_TIMEOUT,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "0", {"0", "1"}
   },
   {
      RECORD_NAME ("dc:detDhsDisplay"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
      DET_CONTROL_CMD_DHS_DISPLAY,
      STOP_DIRECTIVE_UNSUPPORTED,
      SIMULATION_MODE_SUPPORTED,
      40.0,
      CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, "1", {"1", "500"}
   }
};


/* The pWfsDbGsubList data structure array contains information on the genSub records
 * recognised by the system, and the commands associated with them. Each
 * genSub record is described by the following information:
 * - Record name (excluding the system prefix).
 * - Name of task to receive commands from that record.
 * - Command number associated with that record.
 * - Command timeout in seconds.
 */

GSUB_RECORD pWfsDbGsubList [] = {};


/* The pWfsDbCarList data structure array contains information on the CAR records
 * recognised by the system. Each CAR record is described by the following
 * information:
 * - Record name (excluding the system prefix).
 * - Name of task responsible for that record.
 */

CAR_RECORD   pWfsDbCarList [] =
{
   {
      RECORD_NAME ("controlC"),
      SEQ_CONTROL_TASK_NAME
   },
   {
      RECORD_NAME ("dc:detC"),
      TASK_NAME ("hr", DET_CONTROL_TASK_NAME)
   }
};


/* The pWfsDbSirList data structure array contains information on the SIR records
 * recognised by the system. Each SIR record is described by the following
 * information:
 * - Record name (excluding the system prefix).
 * - Record data type.
 * - Hysteresis value (optional).
 */

SIR_RECORD   pWfsDbSirList [] =
{
   {
      RECORD_NAME ("name"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("state"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("controlState"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("health"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("controlHealth"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("initialising"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("rebooting"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("datuming"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("parking"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("verifying"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("endVerifying"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("guiding"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("endGuiding"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("endObserving"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("testing"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("measuring"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("trackId"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("arrayS"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("seeing"),
      EPICS_DATA_TYPE_DOUBLE
   },
   {
      RECORD_NAME ("obsType"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("obsMode"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("historyLog"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("cpuUsed00"),
      EPICS_DATA_TYPE_LONG,
      5.0
   },
   {
      RECORD_NAME ("ramUsed00"),
      EPICS_DATA_TYPE_LONG,
      2.0
   },
   {
      RECORD_NAME ("ramFreeblk00"),
      EPICS_DATA_TYPE_LONG,
      256.0
   },
   {
      RECORD_NAME ("testResults"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:name"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:state"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:health"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:historyLog"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:historyLog1"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:errorLog"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:errorLog1"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:debugMode"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:simMode"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:detPrimReply"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:detInitStatus"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:initialising"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:testing"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:detType"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:detID"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:dataLabel"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:intTime"),
      EPICS_DATA_TYPE_DOUBLE
   },
   {
      RECORD_NAME ("dc:nexpRQ"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:nexp"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:nframes"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:bunit"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:exposed"),
      EPICS_DATA_TYPE_DOUBLE
   },
   {
      RECORD_NAME ("dc:elapsed"),
      EPICS_DATA_TYPE_DOUBLE
   },
   {
      RECORD_NAME ("dc:exposedRQ"),
      EPICS_DATA_TYPE_DOUBLE
   },
   {
      RECORD_NAME ("dc:utstart"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:utend"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:outputs"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:detXsize"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:detYsize"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:xsubap"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:ysubap"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:xstart"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:ystart"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:xras"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:yras"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:xspace"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:yspace"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:xbin"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:ybin"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("observing"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:dhsCon"),
      EPICS_DATA_TYPE_STRING
   },
   {
      RECORD_NAME ("dc:adc0"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:adc1"),
      EPICS_DATA_TYPE_LONG
   },
   {
      RECORD_NAME ("dc:oscan"),
      EPICS_DATA_TYPE_LONG
   }
};

/* Determine the number of CAD, CAR and SIR records defined above. */

int      wfsDbNCadRecord      = NELEMENTS (pWfsDbCadList);
int      wfsDbNCarRecord      = NELEMENTS (pWfsDbCarList);
int      wfsDbNSirRecord      = NELEMENTS (pWfsDbSirList);
int      wfsDbNGsubRecord     = NELEMENTS (pWfsDbGsubList);

char   pWfsDbRecNamePrefix [] = TOP;

/*
 * Record field names and types must be given in the order of the enums
 * nnn_RECORD_TYPE where nnn = CAD, CAR, SIR etc (see dbTypes.h). If field types
 * are listed, then the Value field must be the last one in the list, since the
 * fields are accessed in the order given here and, when writing to a record,
 * processing is triggered when the Value field is written. All record types are
 * initially un-initialised (pWfsDbRecInitialised[type] = FALSE.
 */

BOOL   pWfsDbRecInitialised [N_RECORD_TYPES] = {FALSE, FALSE, FALSE, FALSE};
char   pppWfsDbRecFieldName [N_RECORD_TYPES][EPICS_MAX_NFIELD_PER_RECORD][EPICS_MAX_BYTES_FIELD_NAME + 2] =
   {
      {""},                                    /* CAD record field names      */
      {".J", ".VALJ"},                         /* genSub record field names   */
      {"ID", ".IERR", ".IMSS", ".IVAL"},       /* CAR record field names      */
      {".VAL"}                                 /* SIR record field names      */
   };

/*
 * Initialise the flag indicating whether the EPICS database is contained on the
 * local processor. This flag is reset to TRUE by the wavefront sensor control
 * task running on the root processor.
 */

BOOL   wfsDbEpicsDbIsLocal = FALSE;
