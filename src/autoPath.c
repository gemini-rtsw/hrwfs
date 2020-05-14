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
static char *mdaytomonth[] = {"jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "nov", "dec"};

#define SECSNADAY	(24 * 60 * 60)

STATUS autoPath () {
   struct tm *tmnow;
   time_t now;
   int secstomidnight;
   struct stat sb;

   if (errorInit () == ERROR) {
      printErr("autoPath: Failed to initialise error context structure.\n");
      return (ERROR);
   }

   while (1) {
      now = time(NULL);
      tmnow = gmtime(&now);

      sprintf(ioc_path, "%s/%d%s%d", DET_CONTROL_DATA_FILE_PATH,
         1900 + tmnow->tm_year, (char *)mdaytomonth[tmnow->tm_mon], tmnow->tm_mday);

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

      now = time(NULL);
      secstomidnight = SECSNADAY - (now % SECSNADAY);
      /* secstomidnight = 30;	DEBUG */
      printf("autoPath: waiting %d seconds to midnight UTC\n", secstomidnight);
      taskDelay(secstomidnight * sysClkRateGet());	/* Wait until midnight */
   }

   MESSAGE_LOG(MSG_WARNING, "autoPath task exited.");

   return (OK);
}
