static struct {void *v; char *c;} rcsid = {&rcsid,
	"$Id: wfsHrwfsDb.c,v 1.1.1.1 1999-03-17 03:14:23 cboyer Exp $"};

/*+
 *	MODULE NAME:
 *	wfsHrwfsDb
 *
 *	FILENAME:
 *	wfsHrwfsDb.c
 *
 *	PURPOSE:
 *	Wavefront sensor database definition - HRWFS
 *
 *  DESCRIPTION:
 *	This module initialises the data structures which describe the records
 *	contained in the EPICS database and the properties of the commands
 *	associated with those records. The data structure arrays are declared in
 *  "wfsDb.h" and their contents declared in "dbTypes.h". A record cannot
 *	by accessed by the epToVxLib library unless it is declared here.
 *
 *	The database is initialised in wfsDb.c rather than wfsDb.h because of
 *	the programming convention that header files only declare objects and do
 *	not allocate memory space
 *
 *	FUNCTION NAME(S):
 *	None
 *
 *	DEFICIENCIES:
 *	This module assumes that all EPICS records have the same prefix, whereas
 *	this is not the case for HRWFS/AC records.
 *
 *	The default values of attributes for the detGeometry command should be set to
 *	values derived from the detector properties rather than fixed values. 
 *
 *	NOTE:
 *	This file only defines the records that the WFS epToVxLib library needs to
 *	know about. There may be other EPICS records - see the Capfast schematics.
 *
 *	I am concerned that all the EPICS database definitions need to be
 *	duplicated here and in the Capfast schematics, as there is a risk the
 *	two definitions will diverge. Can the database information be extracted
 *	from the files generated from the Capfast schematics, or at least
 *	downloaded from a file or defined in function calls at boot time?
 *	SMB - 26 Nov 97.
 *
 *	Note that the initialisers in this module do not necessarily fill
 *	all of a CAD record structure. For example, if a CAD record does not
 *	have any attributes, then its attribute structure is not initialised.
 *	This feature may generate warnings with some compilers. It is assumed
 *	that uninitialised parts of the data structures will be filled with
 *	zero or NULL values.
 *
 *	IMPORTANT:
 *	The record names and fields declared in this file should exactly
 *	match the names and fields for those same records as defined in
 *	the Capfast schematics
 *
 *	ORIGINAL AUTHOR:
 *	Nick Dillon
 *
 *	MODIFIED BY:
 *	Steven Beard
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.32  1998/12/07 15:25:30  cics
 * Changed output options in observe command. Fixed some sdsuLib bugs related to continuous observing.
 *
 * Revision 1.31  1998/11/30 15:54:43  cics
 * Modifications made during SMB visit to Hilo, November 1998
 *
 * Revision 1.30  1998/10/22 15:14:16  cics
 * Initialises signal processing parameters on startup. Does not yet update ospGeometry.
 *
 * Revision 1.29  1998/10/20 09:28:01  cics
 * New testResults and historyLog records.
 *
 * Revision 1.28  1998/10/14 09:55:08  cics
 * HRWFS records added. detFrame command deleted. detGiveUp command added.
 *
 * Revision 1.27  1998/10/12 10:44:28  cics
 * Command timeouts increased
 *
 * Revision 1.26  1998/10/08 16:19:56  cics
 * detSigInit command added
 *
 * Revision 1.25  1998/10/01 13:50:12  cics
 * signalProc commands and genSub records moved to detControl
 *
 * Revision 1.24  1998/09/28 08:56:27  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks. detGeometry command changed.
 *
 * Revision 1.23  1998/09/09 14:35:36  cics
 * Global variables renamed to ensure they are unique
 *
 * Revision 1.22  1998/08/17 12:06:58  smb
 * Added detTest command
 *
 * Revision 1.21  1998/08/13 09:09:32  smb
 * Added author comment
 *
 * Revision 1.20  1998/07/30 16:54:04  smb
 * Make detInit command redownload DSP code
 *
 * Revision 1.19  1998/07/28 15:53:59  smb
 * Parameters to setGeometry setMode and setTemp modified.
 *
 * Revision 1.18  1998/07/27 08:21:13  smb
 * USCAN moved from detMode to detGeometry. Single parameter file used by detSetup and detSave.
 *
 * Revision 1.17  1998/07/16 16:39:08  smb
 * File paths no longer have to end in slash. Fixed problem with image buffer pointer not being returned properly from detObserve. Download DSP code automatically on startup.
 *
 * Revision 1.16  1998/07/15 15:32:40  smb
 * Command defaults updated
 *
 * Revision 1.15  1998/07/13 15:47:05  smb
 * Detector controller command arguments changed.
 *
 * Revision 1.14  1998/06/02 10:19:20  smb
 * wfsTasks.h replaced with wfsControl.h
 *
 * Revision 1.13  1998/05/13 10:36:29  smb
 * genSub records added
 *
 * Revision 1.12  1998/03/04 17:12:16  smb
 * Comment dates made more international
 *
 * Revision 1.11  1998/02/23 13:38:56  smb
 * Rearranged code for printability
 *
 * Revision 1.10  1998/02/19 12:19:15  smb
 * Minor changes to ICD 162/163
 *
 * Revision 1.9  1998/02/18 11:14:32  smb
 * CAD/CAR/SIR records brought up to date with ICD 162/163
 *
 * Revision 1.8  1998/02/05 15:33:09  smb
 * Added observing and initialising records
 *
 * Revision 1.7  1998/01/27 17:04:49  smb
 * Modified to reflect changes to EPICS database
 *
 * Revision 1.6  1998/01/21 10:47:08  smb
 * Message logging added
 *
 * Revision 1.5  1998/01/19 16:18:21  smb
 * Update individual health records
 *
 * Revision 1.4  1998/01/16 11:48:46  smb
 * Download all three DSP files on one operation
 *
 * Revision 1.3  1998/01/14 15:49:44  smb
 * Observe command updated
 *
 * Revision 1.2  1998/01/14 14:39:18  smb
 * Added CAD records for downloading COFF file
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


#include "dbTypes.h"
#include "wfsHrwfsDb.h"
#include "wfsLib.h"
#include "epToVxLib.h"
#include "detControl.h"					/* This is where DET_CONTROL_ parameters come from.	*/
#include "osp.h"						/* This is where the OSP_ parameters come from.		*/
#include "wfsControl.h"					/* This is where WFS_CONTROL_ parameters come from.	*/
#include "errorLog.h"					/* This is where LOGTASK_ parameters comes from.	*/


/* The pWfsDbCadList data structure array contains information on the CAD records
 * recognised by the system, and the commands associated with them. Each
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
		WFS_CONTROL_TASK_NAME,
		WFS_CONTROL_CMD_INIT,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		NO_TIMEOUT
	},
	{
		RECORD_NAME ("park"),
		WFS_CONTROL_TASK_NAME,
		WFS_CONTROL_CMD_PARK,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		NO_TIMEOUT
	},
	{
		RECORD_NAME ("setRouter"),
		WFS_CONTROL_TASK_NAME,
		WFS_CONTROL_CMD_SETROUTER,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		120.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, 	"1",			{"1", "5"}
	},
	{
		RECORD_NAME ("startMeasure"),
		WFS_CONTROL_TASK_NAME,
		WFS_CONTROL_CMD_STARTMEASURE,
		STOP_DIRECTIVE_SUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		120.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, 	"3",			{"2", "3"},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, 	"19",			{"3", "19"},
		CAD_ATTRIB_C, EPICS_DATA_TYPE_DOUBLE, 	"0.05",			{"0.001", "100.0"},
		CAD_ATTRIB_D, EPICS_DATA_TYPE_DOUBLE, 	"0.5",			{"0.05", "10000.0"},
		CAD_ATTRIB_E, EPICS_DATA_TYPE_LONG, 	"0",			{"0", "7"},
		CAD_ATTRIB_F, EPICS_DATA_TYPE_LONG, 	"0",			{"0", "7"},
		CAD_ATTRIB_G, EPICS_DATA_TYPE_LONG, 	"0",			{"0", "7"}
	},
	{
		RECORD_NAME ("stopMeasure"),
		WFS_CONTROL_TASK_NAME,
		WFS_CONTROL_CMD_STOPMEASURE,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		120.0
	},
	{
		RECORD_NAME ("gbdObserve"),
		WFS_CONTROL_TASK_NAME,
		WFS_CONTROL_CMD_GBDOBSERVE,
		STOP_DIRECTIVE_SUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		120.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, 	"7",			{"0", "7"},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, 	"WFS",			{"WFS"},
		CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG, 	"1",			{"-1", NO_HI_LIMIT},
		CAD_ATTRIB_D, EPICS_DATA_TYPE_DOUBLE, 	"1.0",			{"0.001", "1000.0"},
		CAD_ATTRIB_E, EPICS_DATA_TYPE_LONG, 	"0",			{"0", "1"},
		CAD_ATTRIB_F, EPICS_DATA_TYPE_STRING, 	"NONE",			{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_G, EPICS_DATA_TYPE_STRING, 	"NONE",			{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_H, EPICS_DATA_TYPE_STRING, 	"NONE",			{NO_ATTRIBUTE_LIMITS}
	},
	{
		RECORD_NAME ("calibrate"),
		WFS_CONTROL_TASK_NAME,
		WFS_CONTROL_CMD_CALIBRATE,
		STOP_DIRECTIVE_SUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		120.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, 	"7",			{"0", "7"},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, 	"DARK",			{"DARK", "FLAT", "ZNULL"},
		CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG, 	"1",			{"1", NO_HI_LIMIT},
		CAD_ATTRIB_D, EPICS_DATA_TYPE_DOUBLE, 	"1.0",			{"0.001", "1000.0"},
		CAD_ATTRIB_E, EPICS_DATA_TYPE_LONG, 	"0",			{"0", "1"},
		CAD_ATTRIB_F, EPICS_DATA_TYPE_STRING, 	"NONE",			{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_G, EPICS_DATA_TYPE_STRING, 	"NONE",			{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_H, EPICS_DATA_TYPE_STRING, 	"NONE",			{NO_ATTRIBUTE_LIMITS}
	},
	{
		RECORD_NAME ("reboot"),
		WFS_CONTROL_TASK_NAME,
		WFS_CONTROL_CMD_REBOOT,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		NO_TIMEOUT
	},
	{
		RECORD_NAME ("simulate"),
		WFS_CONTROL_TASK_NAME,
		WFS_CONTROL_CMD_SIMULATE,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_UNSUPPORTED,
		10.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG,		ATTRIB (EPTOVX_SIM_MODE_NONE),
																{ATTRIB (EPTOVX_SIM_MODE_VSM),
																ATTRIB (EPTOVX_SIM_MODE_NONE)}
	},
	{
		RECORD_NAME ("debug"),
		WFS_CONTROL_TASK_NAME,
		WFS_CONTROL_CMD_DEBUG,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_UNSUPPORTED,
		10.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG,		ATTRIB (EPTOVX_DEBUG_MODE_NONE),
																{ATTRIB (EPTOVX_DEBUG_MODE_NONE),
																ATTRIB (EPTOVX_DEBUG_MODE_FULL)}
	},
	{
		RECORD_NAME ("dc:detSetup"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_SETUP,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		120.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, 	DET_CONTROL_PAR_FILE_PATH,	{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, 	"hrparams.par",				{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG,		"-1",						{"-1", "3"}
	},
	{
		RECORD_NAME ("dc:detChop"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_CHOP,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		40.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, 	"0",			{"0", "7"}
	},
	{
		RECORD_NAME ("dc:detExposure"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_EXPOSURE,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		30.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, 	"1",			{"-1", NO_HI_LIMIT},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_DOUBLE, 	"1.0",			{"0.0001", "100000.0"}
	},
	{
		RECORD_NAME ("dc:detObstype"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_OBSTYPE,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		40.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, 	"IMAGE",	{"IMAGE", "WFS", "DARK", "FLAT", "ZNULL"},
	},
	{
		RECORD_NAME ("dc:detSetWcs"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_SETWCS,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		120.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, 	DET_CONTROL_PAR_FILE_PATH,	{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, 	"hrcalib.wcs",				{NO_ATTRIBUTE_LIMITS}
	},
	{
		RECORD_NAME ("dc:observe"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_OBSERVE,
		STOP_DIRECTIVE_SUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		120.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, 	"NONE",						{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, 	"1",						{"0", "2"},
		CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING, 	DET_CONTROL_DATA_FILE_PATH,	{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_D, EPICS_DATA_TYPE_STRING, 	"hrwfs.fits",				{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_E, EPICS_DATA_TYPE_STRING, 	"NONE",						{NO_ATTRIBUTE_LIMITS}
	},
	{
		RECORD_NAME ("dc:stop"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_STOP,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		120.0
	},
	{
		RECORD_NAME ("dc:abort"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_ABORT,
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
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, 	ATTRIB (DET_CONTROL_SWITCH_SDSU_ADRS_VME),
																				{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING,	DET_CONTROL_OMF_FILE_PATH,		{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING,	DET_CONTROL_OMF_VME_FILE,		{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_D, EPICS_DATA_TYPE_STRING,	DET_CONTROL_HRWFS_OMF_TIM_FILE,	{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_E, EPICS_DATA_TYPE_STRING,	DET_CONTROL_OMF_UTL_FILE,		{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_F, EPICS_DATA_TYPE_LONG,		ATTRIB (DET_CONTROL_HRWFS_MAX_FRAMES),	{"0", "100"}
},
	{
		RECORD_NAME ("dc:detReset"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_RESET,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		60.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, 	"1",							{"0", "1"},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG,		"1",							{"0", "1"},
		CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING,	DET_CONTROL_OMF_FILE_PATH,		{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_D, EPICS_DATA_TYPE_STRING,	DET_CONTROL_OMF_VME_FILE,		{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_E, EPICS_DATA_TYPE_STRING,	DET_CONTROL_HRWFS_OMF_TIM_FILE,	{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_F, EPICS_DATA_TYPE_STRING,	DET_CONTROL_OMF_UTL_FILE,		{NO_ATTRIBUTE_LIMITS}
	},
	{
		RECORD_NAME ("dc:detTest"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_TEST,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		120.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, 	"8",			{"0", "8"},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, 	"0",			{"0", "1"}
	},
	{
		RECORD_NAME ("dc:detGiveUp"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_GIVEUP,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_UNSUPPORTED,
		20.0
	},
	{
		RECORD_NAME ("dc:detSave"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_SAVE,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		120.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, 	DET_CONTROL_PAR_FILE_PATH,	{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING, 	"hrnewparams.par",			{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG,		"-1",						{"-1", "3"}
	},
	{
		RECORD_NAME ("dc:detGeometry"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_GEOMETRY,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		20.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, 	"1",			{"1", "40"},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, 	"1",			{"1", "40"},
		CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG, 	"512",			{"1", ATTRIB (DET_CONTROL_HRWFS_XSIZE)},
		CAD_ATTRIB_D, EPICS_DATA_TYPE_LONG, 	"1024",			{"1", ATTRIB (DET_CONTROL_HRWFS_YSIZE)},
		CAD_ATTRIB_E, EPICS_DATA_TYPE_LONG, 	"1",			{"1", ATTRIB (DET_CONTROL_HRWFS_XSIZE)},
		CAD_ATTRIB_F, EPICS_DATA_TYPE_LONG, 	"1",			{"1", ATTRIB (DET_CONTROL_HRWFS_YSIZE)},
		CAD_ATTRIB_G, EPICS_DATA_TYPE_LONG, 	"16",			{"0", ATTRIB (DET_CONTROL_HRWFS_XSIZE)},
		CAD_ATTRIB_H, EPICS_DATA_TYPE_LONG, 	"1",			{"0", ATTRIB (DET_CONTROL_HRWFS_YSIZE)},
		CAD_ATTRIB_I, EPICS_DATA_TYPE_LONG, 	"0",			{"0", ATTRIB (DET_CONTROL_HRWFS_XSIZE)},
		CAD_ATTRIB_J, EPICS_DATA_TYPE_LONG, 	"0",			{"0", ATTRIB (DET_CONTROL_HRWFS_YSIZE)}
	},
	{
		RECORD_NAME ("dc:detPrim"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_PRIMITIVE,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		120.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING,	"TDL",			{NO_ATTRIBUTE_LIMITS},
																			/* Allow any command. */
		CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG,		"1",			{"1", "3"},
		CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG,		"0x55aaff",		{"0",	NO_HI_LIMIT},
		CAD_ATTRIB_D, EPICS_DATA_TYPE_LONG,		"0",			{"0",	NO_HI_LIMIT},
		CAD_ATTRIB_E, EPICS_DATA_TYPE_LONG,		"0",			{"0",	NO_HI_LIMIT},
		CAD_ATTRIB_F, EPICS_DATA_TYPE_LONG,		"0",			{"0",	NO_HI_LIMIT},
		CAD_ATTRIB_G, EPICS_DATA_TYPE_LONG,		"0",			{"0",	NO_HI_LIMIT},
		CAD_ATTRIB_H, EPICS_DATA_TYPE_LONG,		"0",			{"0",	NO_HI_LIMIT}
	},
	{
		RECORD_NAME ("dc:detMode"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_MODE,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		40.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, 	"-1",			{"-1",	NO_HI_LIMIT},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, 	"-1",			{"-1",	NO_HI_LIMIT},
		CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG,		"-1",			{"-1",	"0xFFF"},
		CAD_ATTRIB_D, EPICS_DATA_TYPE_LONG, 	"-1",			{"-1",	NO_HI_LIMIT}
	},
	{
		RECORD_NAME ("dc:detOffset"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_OFFSET,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		40.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, 	"-1",			{"-1",	NO_HI_LIMIT},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, 	"-1",			{"-1",	NO_HI_LIMIT},
		CAD_ATTRIB_C, EPICS_DATA_TYPE_LONG,		"-1",			{"-1",	"-1"},
		CAD_ATTRIB_D, EPICS_DATA_TYPE_LONG, 	"-1",			{"-1",	"-1"}
	},
	{
		RECORD_NAME ("dc:detTemp"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_TEMP,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		40.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_DOUBLE, 	"25",			{"-63", "25"},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG,		"0x80",			{NO_ATTRIBUTE_LIMITS}
	},
	{
		RECORD_NAME ("dc:detSigInit"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_SIGINIT,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		120.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_STRING, 	DET_CONTROL_PAR_FILE_PATH,		{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_STRING,	DET_CONTROL_HRWFS_OSPINI_FILE,	{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_C, EPICS_DATA_TYPE_STRING,	"(NOT USED)",					{NO_ATTRIBUTE_LIMITS}
	},
	{
		RECORD_NAME ("dc:detSigMode"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		DET_CONTROL_CMD_SIGMODE,
		STOP_DIRECTIVE_UNSUPPORTED,
		SIMULATION_MODE_SUPPORTED,
		40.0,
		CAD_ATTRIB_A, EPICS_DATA_TYPE_LONG, 	"1",						{"0", "8"},
		CAD_ATTRIB_B, EPICS_DATA_TYPE_LONG, 	"1",						{"1", NO_HI_LIMIT},
		CAD_ATTRIB_C, EPICS_DATA_TYPE_DOUBLE, 	"1.0",						{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_D, EPICS_DATA_TYPE_DOUBLE, 	"60.0",						{NO_ATTRIBUTE_LIMITS},
		CAD_ATTRIB_E, EPICS_DATA_TYPE_STRING, 	"NONE",						{NO_ATTRIBUTE_LIMITS}
	},
};


/* The pWfsDbGsubList data structure array contains information on the genSub records
 * recognised by the system, and the commands associated with them. Each
 * genSub record is described by the following information:
 * - Record name (excluding the system prefix).
 * - Name of task to receive commands from that record.
 * - Command number associated with that record.
 * - Command timeout in seconds.
 */

GSUB_RECORD pWfsDbGsubList [] =
{
	{
		RECORD_NAME ("dc:ttfZero"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		"hr",
		GSUB_INPUT,
		DET_CONTROL_CMD_TTFZERO,
		1.0,
		8
	},
	{
		RECORD_NAME ("dc:aoZero"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		"hr",
		GSUB_INPUT,
		DET_CONTROL_CMD_AOZERO,
		1.0,
		24
	},
	{
		RECORD_NAME ("dc:probeOffset"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		"hr",
		GSUB_INPUT,
		DET_CONTROL_CMD_PROBEOFFSET,
		1.0,
		9
	},
	{
		RECORD_NAME ("dc:ttf"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		"hr",
		GSUB_OUTPUT,
		0,
		NO_TIMEOUT,
		8
	},
	{
		RECORD_NAME ("dc:ao"),
		TASK_NAME ("hr", DET_CONTROL_TASK_NAME),
		"hr",
		GSUB_OUTPUT,
		0,
		NO_TIMEOUT,
		40
	}
};


/* The pWfsDbCarList data structure array contains information on the CAR records
 * recognised by the system. Each CAR record is described by the following
 * information:
 * - Record name (excluding the system prefix).
 * - Name of task responsible for that record.
 */

CAR_RECORD	pWfsDbCarList [] =
{
	{
		RECORD_NAME ("controlC"),
		WFS_CONTROL_TASK_NAME
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

SIR_RECORD	pWfsDbSirList [] =
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
		RECORD_NAME ("health"),
		EPICS_DATA_TYPE_STRING
	},
	{
		RECORD_NAME ("controlHealth"),
		EPICS_DATA_TYPE_STRING
	},
	{
		RECORD_NAME ("version"),
		EPICS_DATA_TYPE_STRING
	},
	{
		RECORD_NAME ("debugMode"),
		EPICS_DATA_TYPE_STRING
	},
	{
		RECORD_NAME ("simMode"),
		EPICS_DATA_TYPE_STRING
	},
	{
		RECORD_NAME ("initialising"),
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
		RECORD_NAME ("historyLog"),
		EPICS_DATA_TYPE_STRING
	},
	{
		RECORD_NAME ("historyLog1"),
		EPICS_DATA_TYPE_STRING
	},
	{
		RECORD_NAME ("errorLog"),
		EPICS_DATA_TYPE_STRING
	},
	{
		RECORD_NAME ("errorLog1"),
		EPICS_DATA_TYPE_STRING
	},
	{
		RECORD_NAME ("cpuUsed00"),
		EPICS_DATA_TYPE_LONG,
		5.0
	},
	{
		RECORD_NAME ("cpuUsed01"),
		EPICS_DATA_TYPE_LONG,
		5.0
	},
	{
		RECORD_NAME ("cpuUsed02"),
		EPICS_DATA_TYPE_LONG,
		5.0
	},
	{
		RECORD_NAME ("ramUsed00"),
		EPICS_DATA_TYPE_LONG,
		2.0
	},
	{
		RECORD_NAME ("ramUsed01"),
		EPICS_DATA_TYPE_LONG,
		2.0
	},
	{
		RECORD_NAME ("ramUsed02"),
		EPICS_DATA_TYPE_LONG,
		2.0
	},
	{
		RECORD_NAME ("ramFreeblk00"),
		EPICS_DATA_TYPE_LONG,
		256.0
	},
	{
		RECORD_NAME ("ramFreeblk01"),
		EPICS_DATA_TYPE_LONG,
		256.0
	},
	{
		RECORD_NAME ("ramFreeblk02"),
		EPICS_DATA_TYPE_LONG,
		256.0
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
		RECORD_NAME ("dc:testResults"),
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
		RECORD_NAME ("dc:observing"),
		EPICS_DATA_TYPE_LONG
	}
};

/* Determine the number of CAD, CAR and SIR records defined above. */

int		wfsDbNCadRecord		= NELEMENTS (pWfsDbCadList);
int		wfsDbNGsubRecord	= NELEMENTS (pWfsDbGsubList);
int		wfsDbNCarRecord		= NELEMENTS (pWfsDbCarList);
int		wfsDbNSirRecord		= NELEMENTS (pWfsDbSirList);

char	pWfsDbRecNamePrefix [] = TOP;

/*
 * Record field names and types must be given in the order of the enums
 * nnn_RECORD_TYPE where nnn = CAD, CAR, SIR etc (see dbTypes.h). If field types
 * are listed, then the Value field must be the last one in the list, since the
 * fields are accessed in the order given here and, when writing to a record,
 * processing is triggered when the Value field is written. All record types are
 * initially un-initialised (pWfsDbRecInitialised[type] = FALSE.
 */

BOOL	pWfsDbRecInitialised [N_RECORD_TYPES] = {FALSE, FALSE, FALSE, FALSE};
char	pppWfsDbRecFieldName [N_RECORD_TYPES][EPICS_MAX_NFIELD_PER_RECORD][EPICS_MAX_BYTES_FIELD_NAME + 2] =
	{
		{""},												/* CAD record field names		*/
		{".J", ".VALJ"},									/* genSub record field names	*/
		{"ID", ".IERR", ".IMSS", ".IVAL"},					/* CAR record field names		*/
		{".VAL"}											/* SIR record field names		*/
	};

/*
 * Initialise the flag indicating whether the EPICS database is contained on the
 * local processor. This flag is reset to TRUE by the wavefront sensor control
 * task running on the root processor.
 */

BOOL	wfsDbEpicsDbIsLocal = FALSE;
