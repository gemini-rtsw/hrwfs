static struct {void *v; char *c;} rcsid = {&rcsid,
   "$Id: wfsSite.c,v 1.1.1.1 1999-03-17 03:14:25 cboyer Exp $"};

/*+
 *   MODULE NAME:
 *   wfsSite
 *
 *   FILENAME:
 *   wfsSite.c
 *
 *   PURPOSE:
 *   Wavefront sensing site defintion - GEMINI NORTH VERSION.
 *
 *   DESCRIPTION:
 *   This module initialises the data structure which contains a definition of the CPU
 *   configuration at the current site, consisting of
 *   - the VxWorks target name for the processor;
 *   - the target architecture (TARGET_TYPE_MV167 or TARGET_TYPE_HKBAJA47);
 *   - the ethernet address (defined using an IPADDR_TO_HEX(a,b,d,c) macro);
 *   - the processor clock speed in Hz;
 *   - the amount of local RAM in bytes; and
 *   - a collection of flags describing the behaviour of the VME interface.
 *   which MUST be defined correctly for each site before the wavefront
 *   sensing system will function.
 *
 *   The data structure is initialised in wfsSite.c rather than wfsSite.h because of
 *   the programming convention that header files only declare objects and do
 *   not allocate memory space.
 *
 *   THIS FILE IS NOT RELEVANT AS LONG AS MPPIPEDRV AND SYSEXTLIB ARE NOT BEING USED.
 *
 *   FUNCTION NAME(S):
 *   None
 *
 *   NOTES:
 *   In its present form there needs to be a separate copy of this module
 *   for each site at which the AGWPS software will run, which leads to the
 *   risk that the various copies of this library will diverge. Can this sort of
 *   information not be downloaded from a file or defined from a function call at
 *   boot time?    SMB - 26 Nov 97.
 *
 *   The definitions in this file must be modified to reflect the
 *   actual cpu configuration at your site. See definitions below.
 *
 *   THIS FILE IS ONLY USED WHEN THE MULTI-PROCESSOR PIPE DRIVER AND
 *   SYSEXTLIB ARE BEING USED. OTHERWISE ITS CONTENTS ARE IGNORED.
 *
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.2  1998/12/07 11:17:29  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.1  1998/11/30 15:54:44  cics
 * Modifications made during SMB visit to Hilo, November 1998
 *
 * Revision 1.6  1998/10/08 16:17:44  cics
 * Local RAM of mv167 increased from 16 to 32 Mbytes
 *
 * Revision 1.5  1998/09/28 08:52:31  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks
 *
 * Revision 1.4  1998/09/09 14:35:40  cics
 * Global variables renamed to ensure they are unique
 *
 * Revision 1.3  1998/06/30 13:31:37  smb
 * Do not include VME mode if NO_SYSEXTLIB requested.
 *
 * Revision 1.2  1998/03/05 14:23:25  smb
 * Comment dates made more international
 *
 * Revision 1.1  1998/03/02 14:06:30  smb
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

#include <taskLib.h>
#include <stdio.h>
#include <ioLib.h>
#include <memLib.h>
#include "gemTypes.h"
#include "errorLib.h"

#ifndef NO_SYSEXTLIB                     /* Define this macro to remove sysextLib   */
#include "sysextLib.h"
#endif   /* NO_SYSEXTLIB */

#include "wfsLib.h"
#include "wfsSite.h"


/* defines */

/* global variables */

   /*
    * Initialise the data structure describing the properties of each of the
    * processors on which wavefront sensor control processes will run.
    * "pWfsArchProcessor" is an array of structures with one element of that
    * array for each processor.
    *
    * NOTE: THE FOLLOWING DEFINITIONS MUST BE EDITED TO DESCRIBE THE ACTUAL
    * PROCESSORS USED ON YOUR SYSTEM, ESPECIALLY THE TARGET NAMES AND ETHERNET
    * ADDRESSES WHICH ARE ALMOST CERTAINLY GOING TO BE DIFFERENT AT EACH SITE.
    * IF A SECOND BAJA IS USED IT SHOULD BE ADDED AS ANOTHER ARRAY ELEMENT.
    *
    * THE DEFINITIONS SHOWN HERE REFLECT THE STATE OF THE HARDWARE AT GEMINI NORTH
    */

char pWfsSiteName[] = "GEMINI-NORTH";         /* Record your site name here as a reminder */

WFS_ARCH_PROCESSOR pWfsArchProcessor [] =
   {
      {
         "wfs",                     /* Target name.               */
         TARGET_TYPE_MV167,            /* Architecture.            */
         IPADDR_TO_HEX (0,0,0,0),      /* IP address (unused).         */
                                 /* Actually (192,108,120,9).   */
         33.0e06,                  /* Processor clock speed, Hz.   */
         0x02000000,                  /* Size of local RAM, in bytes.   */
#ifndef NO_SYSEXTLIB
         {
            0,                     /* Bus request level.         */
            TRUE,                  /* Release when done mode.      */
            TRUE,                  /* Fair requester.            */
            TRUE,                  /* Round robin arbiter.         */
            0,                     /* DMA bus request level.      */
            TRUE                  /* DMA fair requester.         */
         }
#endif   /* NO_SYSEXTLIB */
      }
   };

int   pWfsNumProcessors = NELEMENTS (pWfsArchProcessor);
                                 /* Number of processors         */
                                 /* defined above.            */

