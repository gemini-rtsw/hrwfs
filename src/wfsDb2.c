
/*+
 *	MODULE NAME:
 *	wfsDb
 *
 *	FILENAME:
 *	wfsDb.c
 *
 *	PURPOSE:
 *	Wavefront sensor database definition
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
 *	NOTE:
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
 * Revision 1.6  1998/12/07 15:25:30  cics
 * Changed output options in observe command. Fixed some sdsuLib bugs related to continuous observing.
 *
 * Revision 1.5  1998/10/20 10:00:43  cics
 * Removed duplicate rcsid
 *
 * Revision 1.4  1998/10/20 09:51:11  cics
 * Brought into line. Made to include standard version
 *
 * Revision 1.3  1998/10/20 09:28:02  cics
 * New testResults and historyLog records.
 *
 * Revision 1.2  1998/10/14 09:55:09  cics
 * HRWFS records added. detFrame command deleted. detGiveUp command added.
 *
 * Revision 1.1  1998/10/12 11:20:30  cics
 * Temporary second versions to allow parallel testing
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



/* Override standard version of wfsDb with wfsDb2. */

#include "dbTypes.h"
#include "wfsDb2.h"

/* Switch to standard version of wfsDb.c */

#include "wfsDb.c"
