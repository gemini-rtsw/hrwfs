static struct {void *v; char *c;} rcsid = {&rcsid, "$Id: autoPath.c 39188 2011-11-17 02:20:32Z aebbers $"};

/*+
 *   MODULE NAME:
 *   autoPath
 *
 *   FILENAME:
 *   autoPath.c
 *
 *   PURPOSE:
 *   Wavefront sensor task application code
 *
 *   DESCRIPTION:
 *   This file contains the function "autoPath", which is started from
 *   the command line (i.e., startup script) to watch for UTC midnight and
 *   create a default directory and default paths for writing fits files
 *   and circular buffers.
 *
 *   INCLUDE FILES:
 *   None
 *
 *   DEFICIENCIES:
 *   Never
 *
 *   ORIGINAL AUTHOR:
 *   TCC
 *
 *   MODIFIED BY:
 *   TCC
 *
 * INDENT-OFF*
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

#include <stdio.h>
#include <taskLib.h>
#include <pipeDrv.h>
#include <ioLib.h>
#include <memLib.h>
#include <math.h>
#include <tickLib.h>
#include <sys/stat.h>
#include "gemTypes.h"
#include "errorLib.h"
#include <time.h>
#include "detControl.h"

extern int snprintf(char *str, size_t count, const char *fmt, ...);

/* Note that EPICS_MAX_BYTES_STRING_ATTRIB == 40, and the current path is at 32 chars */
extern char ioc_path[EPICS_MAX_BYTES_STRING_ATTRIB];		/* Path to where to write images, cb's	*/

extern char epToVxTopName[];					/* Name of pwfs				*/

#define SECSNADAY	(24 * 60 * 60)

STATUS autoPath () {
   struct tm *tmutc;	/* UTC tm struct */
   struct tm *tmloc;	/* Localtime tm struct */
   time_t now;
   int twopm;
   struct stat sb;

   if (errorInit () == ERROR) {
      printErr("autoPath: Failed to initialise error context structure.\n");
      return (ERROR);
   }

   while (1) {
      now = time(NULL);
      tmutc = gmtime(&now);
      tmloc = localtime(&now);

      /* printf("autoPath: Localhour vs UTC %d %d\n", tmutc->tm_hour, tmloc->tm_hour); */

      sprintf(ioc_path, "%s/hrwfs/%04d%02d%02d", DET_CONTROL_DATA_FILE_PATH,
         1900 + tmutc->tm_year, tmutc->tm_mon + 1, tmutc->tm_mday);

      printf("autoPath: Creating IOC path: \"%s\".\n", ioc_path);


      /*
       * Attempt to create the directory. On failure, wait 10 minutes and try again
       * just in case there's a transient error on nfs.
       */

      if (stat(ioc_path, &sb) == ERROR) {	/* Should fail as it hasn't been created yet */
         if (mkdir(ioc_path) == ERROR) {
            printf("autoPath: Failed to create directory \"%s\".\n", ioc_path);
            taskDelay(10 * 60 * sysClkRateGet());
            continue;
         }
      }
      else {
         printf("autoPath: Directory \"%s\" already exists.\n", ioc_path);
      }


      twopm = tmutc->tm_hour - tmloc->tm_hour;	/* Get localtime hours delta from UTC	*/
      if (twopm < 0) twopm += 24;		/* Handle crossing over to next day	*/
      now -= twopm * (60 * 60);			/* Adjust "now" in UTC to localtime	*/
      now = SECSNADAY - (now % SECSNADAY);	/* Seconds until midnight localtime	*/
      now += (14 * 60 * 60);			/* Seconds until 2pm localtime		*/
      printf("autoPath: waiting %d seconds to 2pm localtime\n", now);
      taskDelay(now * sysClkRateGet());		/* Wait until midnight			*/
   }

   MESSAGE_LOG(MSG_WARNING, "autoPath task exited.");

   return (OK);
}
