/*+
 *	MODULE NAME:
 *	wfsResourceMonitor
 *
 *	FILENAME:
 *	wfsResourceMonitor.h
 *
 *	PURPOSE:
 *	Include file for wavefront sensor application code
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.5  1998/12/07 11:17:29  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.4  1998/10/01 13:48:49  cics
 * Minor comment changes
 *
 * Revision 1.3  1998/09/28 08:53:40  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks
 *
 * Revision 1.2  1998/06/30 12:40:27  smb
 * Dependency on sysextLib can be removed by defining NO_SYSEXTLIB macro.
 *
 * Revision 1.1  1998/05/08 16:21:07  smb
 * wfsTasks split into wfsControl and wfsResourceMonitor
 *
 * Revision 1.4  1998/02/18 11:16:29  smb
 * Commands brought up to date with ICD 162/163
 *
 * Revision 1.3  1998/02/05 15:34:08  smb
 * Ability to update initialising and observing records added
 *
 * Revision 1.2  1997/12/15 11:58:20  smb
 * Replaced arbitrary error numbers with real ones
 *
 * Revision 1.1.1.1  1997/11/28 11:46:17  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

#ifndef	__INCwfsResourceMonitorh
#define	__INCwfsResourceMonitorh


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif	/* vxWorks */

#include "wfsLib.h"
#include "gemTypes.h"
#include "gemModNum.h"


/* defines */

#define	WFS_RAM_USED_SIR_NAME		"ramUsed"		/* Name of SIR record to	*/
													/* contain amount of RAM	*/
													/* used.					*/

#define	WFS_RAM_LARGE_BLK_SIR_NAME	"ramFreeblk"	/* Name of SIR record to	*/
													/* contain size of largest	*/
													/* free RAM block.			*/

#define	WFS_CPU_USAGE_SIR_NAME		"cpuUsed"		/* Name of SIR record to	*/
													/* contain amount of CPU	*/
													/* used.					*/

#define	WFS_CPU_PRIORITY_MAX			0			/* Max task priority.		*/
#define	WFS_CPU_PRIORITY_MIN			255			/* Min task priority.		*/

	/*
	 * Error number codes used by wfsResourceMonitor.
	 * These are designed to be processed using the vxWorks "makeStatTbl" utility.
	 */

	/* (No error codes) */


/* function declarations */

IMPORT STATUS	wfsCpuResourceMonitor (int updateIntervalMicrosec, int cpuAveragingMicrosec);

#endif /* __INCwfsResourceMonitorh */
