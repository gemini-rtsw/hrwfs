
/*+
 *	MODULE NAME:
 *	epToVxLib
 *
 *	FILENAME:
 *	epToVxLib.c
 *
 *	PURPOSE:
 *	EPICS to VxWorks interface library
 *
 *	DESCRIPTION:
 *	The epToVxLib library provides a pipe-based interface between an EPICS
 *	database running on one CPU and VxWorks tasks running on that and other
 *	CPUs which share the same VME bus. A central EPICS database resides on
 *	a nominated CPU in the system. Only the nominated CPU is required to
 *	support EPICS. The other CPUs only require VxWorks support.
 *
 *	The epToVxLib library uses the multi-processor pipe driver, mpPipeDrv,
 *	for communication between CPUs, and the standard VxWorks pipe driver,
 *	pipeDrv, for communication on the same CPU. Many of the epToVxLib
 *	functions allow the pipe driver to be chosen by providing a pointer
 *	to the appropriate "pipeCreate" function.
 *
 *	It is assumed that a description of the EPICS records known by the
 *	system is contained in array externals pWfsDbCadList, pWfsDbCarList and pWfsDbSirList,
 *	which are defined in wfsDb.h and wfsDb.c
 *
 *	EXTERNAL MODULES:
 *	mpPipeDrv			- Multi-processor pipe driver
 *	timeoutLib			- Timeout library
 *	errorLib			- Error handling library
 *
 *	FUNCTION NAME(S):
 *	epToVxInit				-	initialisation routine for epToVxLib
 *	epToVxDbInitCadCar		-	initialise CAD/CAR records in epToVxLib symbol table
 *	epToVxDbInitGensub		-	initialise genSub records in epToVxLib symbol table
 *	epToVxDbInitSir			-	initialise SIR records in epToVxLib's symbol table
 *	epToVxCaInit			-	initialise channel access for specified EPICS record
 *	epToVxCaInitRecords		-	initialise all records for channel access operation
 *	epToVxCaShow			-	print contents of channel access definition structure
 *	epToVxCaInitCar			-	initialise channel access for all CAR records
 *	epToVxCaInitSir			-	initialise channel access for all SIR records
 *	epToVxCaWrite			-	channel-access write EPICS record
 *	epToVxCaRead			-	channel-access read EPICS record
 *	epToVxCaWriteDaemon		-	daemon task serves channel-access writes via pipes
 *	epToVxPipeInit			-	initialise pipe interface to EPICS records
 *	epToVxPipeOpen			-	open (and optionally create) a pipe
 *	epToVxPipeWrite			-	write EPICS record via pipe interface
 *	epToVxSetHealth			-	write to EPICS health record via pipe interface
 *	epToVxCmdInit			-	initialise control task prior to CAD command execution
 *	epToVxCmdFree			-	free resources allocated to control task
 *	epToVxCmdRead			-	read CAD command via pipe
 *	epToVxCmdAttribGet		-	get CAD command attribute from pipe data packet
 *	epToVxCmdFinish			-	issue CAD command response to CAR daemon via pipe interface
 *	epToVxUpdateInit		-	initialise control task prior to receiving genSub updates
 *	epToVxUpdateRead		-	read genSub data update via pipe
 *	epToVxCarDaemon			-	daemon task maintains CAR record status
 *	epToVxCadInit			-	generic CAD initialisation routine
 *	epToVxCadExecute		-	generic CAD execute routine
 *	epToVxCadReject			-	CAD execute routine used for unsupported commands
 *	epToVxSetCadSimMode		-	set simulation mode for all CAD records
 *	epToVxGensubInit		-	generic genSub initialisation routine
 *	epToVxGensubInput		-	execute routine for input genSub
 *	epToVxGensubOutput		-	execute routine for output genSub
 *	epToVxRecContextGet		-	get context structure for EPICS record
 *	epToVxShow				-	print information about an EPICS record
 *	epToVxCadContextShow	-	Print contents of CAD context structure
 *	epToVxCarContextShow	-	Print contents of CAR context structure
 *	epToVxSirContextShow	-	Print contents of SIR context structure
 *	epToVxGsubContextShow	-	Print contents of genSub context structure
 *
 *
 *	DEVELOPMENT NOTES:
 *	There are a number of potential deadlock scenarios caused by the way in
 *	which errors and messages are reported. If the EPICS database is used
 *	to maintain an error count (as is done in the Gemini A&G Wavefront
 *	Processing System) then problems may occur if any of the daemon tasks
 *	in this library attempt to report an error or a message. The deadlocks
 *	are avoided explicitly, as described in the comments for
 *	epToVxCaWriteDaemon and epToVxCarDaemon.
 *
 *	I think this library is a lot more complicated than it needs to be. This
 *	library has suffered a lot from memory corruption bugs, caused by the fact
 *	that memory is allocated dynamically in several places, pointers are heavily
 *	used and arithmetic and type conversion are frequently carried out on pointers.
 *	In the long term I think this library should be replaced by something based
 *	around EPICS device support, and the pipe-based method of communicating database
 *	information should be replaced by a simpler shared memory technique.
 *	SMB - 1 July 1998.
 *
 *	BUGS:
 *	The Channel Access ID structures for EPICS SIR records can get corrupted,
 *	and I have found this happens most frequently when this library is used
 *	with the DEBUG compiler flag defined. I have spent several months debugging
 *	this library but have not yet managed to track this problem down. I suspect
 *	it is related to the memory corruption bugs I have been seeing (as described
 *	in the "BUGS" section of epToVxCaWrite and epToVxCaWriteDaemon).
 *
 *	I am guarding against the memory corruption, for the time being,
 *	by allocating some padding space at the end of every command packet, data packet
 *	and update packet. I suspect some functions are writing beyond the end of these
 *	packets. Unfortunately, this work around can only guard against memory corruptions
 *	which happen within the padding space.
 *	SMB - 1 July 1998.
 *
 *	The memory corruption has been found to be entirely within the channel access
 *	data structures. I have worked around it by commenting out the calls to channel
 *	access in epToVxCaWriteDaemon and epToVxCarDaemon and replacing them with
 *	database access calls.
 *	SMB - 7 July 1998.
 *
 *	ORIGINAL AUTHOR:
 *	Nick Dillon (and Andy Foster?)
 *
 *	DEBUGGED AND MODIFIED BY:
 *	Steven Beard
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.4  1998/12/07 11:17:16  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.3  1998/10/20 10:00:43  cics
 * Removed duplicate rcsid
 *
 * Revision 1.2  1998/10/20 09:51:09  cics
 * Brought into line. Made to include standard version
 *
 * Revision 1.1  1998/10/12 11:20:32  cics
 * Temporary second versions to allow parallel testing
 *
 * Revision 1.27  1998/10/12 10:45:13  cics
 * Error messages enhanced
 *
 * Revision 1.26  1998/10/01 13:53:24  cics
 * Some unchanged variables changed to const
 *
 * Revision 1.25  1998/09/28 08:56:48  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks
 *
 * Revision 1.24  1998/09/09 14:35:25  cics
 * Global variables renamed to ensure they are unique
 *
 * Revision 1.23  1998/08/13 09:10:42  smb
 * Added author comment
 *
 * Revision 1.22  1998/07/16 16:25:02  smb
 * Removed superflous printf from epToVxSetHealth
 *
 * Revision 1.21  1998/07/15 15:30:12  smb
 * Hexadecimal to decimal conversion for LONG attributes fixed.
 *
 * Revision 1.20  1998/07/13 15:24:09  smb
 * More bugs fixed. Channel access replaced by database access.
 *
 * Revision 1.19  1998/07/01 12:51:39  smb
 * Work around memory corruption by padding command and data packets. Added more debugging functions.
 *
 * Revision 1.18  1998/06/30 14:07:12  smb
 * Ensure everything works when sysextLib and mpPipeDrv removed. Fixed mistakes.
 *
 * Revision 1.17  1998/06/29 16:28:16  smb
 * Removed references to sysextLib. Allow dependence on mpPipeDrv to be disabled by defining NO_MPPIPEDRV macro.
 *
 * Revision 1.16  1998/05/29 13:49:56  smb
 * Ability to transmit data updates added
 *
 * Revision 1.15  1998/05/13 10:37:41  smb
 * Added genSub processing functions
 *
 * Revision 1.14  1998/03/27 12:03:50  smb
 * Fixed bug in file descriptor array size
 *
 * Revision 1.13  1998/03/24 16:16:34  smb
 * Several bugs fixed and notes added
 *
 * Revision 1.12  1998/02/23 13:38:49  smb
 * Rearranged code for printability
 *
 * Revision 1.11  1998/02/18 11:15:21  smb
 * epToVxCadReject added
 *
 * Revision 1.10  1998/02/05 15:22:51  smb
 * CAD_DATA_TYPE -> EPICS_DATA_TYPE. Also fixed bugs in epToVxRecContextGet and epToVxPipeWrite
 *
 * Revision 1.9  1998/02/02 17:24:52  smb
 * Added epToVxCmdFree to free resources allocated by epToVxCmdInit
 *
 * Revision 1.8  1998/01/30 15:30:09  smb
 * Fixed some problems uncovered by prolint
 *
 * Revision 1.7  1998/01/28 10:10:38  smb
 * Improved errors and messages. epToVxShow can deal with SIR records
 *
 * Revision 1.6  1998/01/19 16:18:18  smb
 * Update individual health records
 *
 * Revision 1.5  1998/01/08 15:27:55  smb
 * Bugs noted but not yet fixed
 *
 * Revision 1.4  1997/12/15 16:49:37  smb
 * Fixed epToVxPipeWrite bug. Also increased cmd buffer size to 4
 *
 * Revision 1.3  1997/12/11 15:35:03  smb
 * Expand terse error messages - no changes to algorithms
 *
 * Revision 1.2  1997/12/05 14:30:53  smb
 * Bug in epToVxPipeWrite noted but not fixed yet
 *
 * Revision 1.1.1.1  1997/11/28 11:46:17  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif	/* vxWorks */

/* Override standard version of wfsDb with wfsHrwfsDb */

#include "dbTypes.h"
#include "wfsHrwfsDb.h"

/* Switch to standard version of epToVxLib */

#include "epToVxLib.c"
