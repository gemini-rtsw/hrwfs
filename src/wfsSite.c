static struct {void *v; char *c;} rcsid = {&rcsid,
   "$Id: wfsSite.c,v 1.2 2000-01-05 20:10:10 cboyer Exp $"};

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
 *   This module initialises the data structure which contains a definition 
 *   of the CPU configuration at the current site, consisting of
 *   - the VxWorks target name for the processor;
 *   - the target architecture 
 *   - the ethernet address (defined using an IPADDR_TO_HEX(a,b,d,c) macro);
 *   - the processor clock speed in Hz;
 *   - the amount of local RAM in bytes; and
 *   - a collection of flags describing the behaviour of the VME interface.
 *   which MUST be defined correctly for each site before the wavefront
 *   sensing system will function.
 *
 *   The data structure is initialised in wfsSite.c rather than wfsSite.h 
 *   because of the programming convention that header files only declare 
 *   objects and do not allocate memory space.
 *
 *   THIS FILE IS NOT RELEVANT AS LONG AS MPPIPEDRV AND SYSEXTLIB ARE NOT 
 *   BEING USED.
 *
 *   FUNCTION NAME(S):
 *   None
 *
 *   NOTES:
 *   In its present form there needs to be a separate copy of this module
 *   for each site at which the AGWPS software will run, which leads to the
 *   risk that the various copies of this library will diverge. Can this sort of
 *   information not be downloaded from a file or defined from a function call 
 *   at boot time?    SMB - 26 Nov 97.
 *
 *   The definitions in this file must be modified to reflect the
 *   actual cpu configuration at your site. See definitions below.
 *
 *   THIS FILE IS ONLY USED WHEN THE MULTI-PROCESSOR PIPE DRIVER AND
 *   SYSEXTLIB ARE BEING USED. OTHERWISE ITS CONTENTS ARE IGNORED.
 *
 *   MODIFICATION
 *   10 dec 1999 tidy up cb
 *   a lot of modif from smb before delivery
 *   2 mar 98 Original creation
 *
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
    * THE DEFINITIONS SHOWN HERE REFLECT THE STATE OF THE HARDWARE AT GEMINI 
    * NORTH
    */

char pWfsSiteName[] = "GEMINI-NORTH";  /* Record your site name here as a     */
                                       /* reminder                            */

WFS_ARCH_PROCESSOR pWfsArchProcessor [] =
   {
      {
         "hrwfs",                      /* Target name.                        */
         TARGET_TYPE_MV2700,           /* Architecture.                       */
         IPADDR_TO_HEX (0,0,0,0),      /* IP address (unused).                */
                                       /* Actually (192,108,120,9).           */
         266.0e06,                     /* Processor clock speed, Hz.          */
         0x04000000,                   /* Size of local RAM, in bytes.        */
      }
   };

int   pWfsNumProcessors = NELEMENTS (pWfsArchProcessor);
                                 /* Number of processors      */
                                 /* defined above.            */

