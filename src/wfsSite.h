/*+
 *   MODULE NAME:
 *   wfsSite
 *
 *   FILENAME:
 *   wfsSite.h
 *
 *   PURPOSE:
 *   Include file for wfsSite
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/03/17 03:14:27  cboyer
 * Initial creation of the Gemini HRWFS repository
 *
 * Revision 1.5  1998/12/07 11:17:30  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.4  1998/09/28 08:52:30  cics
 * Give warning if an attempt if made to compile this file for anything other 
 * than vxWorks
 *
 * Revision 1.3  1998/09/09 14:35:39  cics
 * Global variables renamed to ensure they are unique
 *
 * Revision 1.2  1998/06/30 13:31:38  smb
 * Do not include VME mode if NO_SYSEXTLIB requested.
 *
 * Revision 1.1  1998/03/02 14:07:07  smb
 * Site specific parts removed from wfsLib
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

#include "gemTypes.h"


/* defines */

   /*
    * The IPADDR_TO_HEX macro converts the four numbers which define an
    * IP address into a hexadecimal code.
    *
    * NOTE: Each of the four arguments to this macro must be values
    * that can fit into a single byte, otherwise an overflow will
    * occur.
    */

#define   IPADDR_TO_HEX(a,b,c,d)     (((a) & 0xff << 24) | \
                                     ((b) & 0xff << 16) |  \
                                     ((c) & 0xff << 8)  |  \
                                     ((d) & 0xff))


   /*
    * Declare the data structure to contain information about
    * the processor used by each wavefront sensor control task.
    */

IMPORT char               pWfsSiteName[];      /* Site name string.           */

IMPORT WFS_ARCH_PROCESSOR pWfsArchProcessor[]; /* Processor definition        */
                                               /* data structures.            */

IMPORT int                pWfsNumProcessors;   /* Number of processors.       */
