/*+
 *	MODULE NAME:
 *	timeoutLib
 *
 *	FILENAME:
 *	timeoutLib.h
 *
 *	PURPOSE:
 *	Include file for timeoutLib
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.4  1998/12/07 11:17:26  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.3  1998/10/01 13:45:45  cics
 * Unchanged variables changed to const
 *
 * Revision 1.2  1998/08/13 08:47:48  smb
 * Added alarm functions
 *
 * Revision 1.1.1.1  1997/11/28 11:46:18  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

#ifndef	__INCtimeoutLibh
#define	__INCtimeoutLibh


/* includes */

#include <vxWorks.h>
#include <timers.h>
#include <sysLib.h>


/* defines */


	/* The START_TIMEOUT macro gets the current time from the real time clock
	 * and writes it to the structure pointed to be its argument.
	 */

#define	START_TIMEOUT(pTime)		(clock_gettime ((clockid_t) CLOCK_REALTIME, pTime))


	/* The SEC_TO_NTICK macro converts a time interval in seconds into the
	 * number of system clock ticks. If the time interval is negative the
	 * WAIT_FOREVER code is returned.
	 */

#define	SEC_TO_NTICK(t)															\
(																				\
	(double) (t) < 0.0 ? WAIT_FOREVER : (int) ((double) (t) * sysClkRateGet ())	\
)


	/*
	 * Error number codes used by timeoutLib.
	 * These are designed to be processed using the vxWorks "makeStatTbl" utility.
	 */

	/* (No error codes) */


/* function declarations */

/*
 * The clock_setres function should really be declared in timers.h, but it
 * isn't (in VxWorks Vn 5.2).
 * The declaration below is included to stop the compiler from issuing a
 * warning message. May need to remove this declaration if/when timers.h
 * gets fixed.
 */

IMPORT int		clock_setres (clockid_t clock_id, struct timespec * res);

IMPORT BOOL		timeoutExpired (const double timeoutPeriod, const struct timespec * pTimeStart);
IMPORT STATUS	timeoutInit (void);
IMPORT STATUS	timeoutAlarmInit (timer_t * pTimeId);
IMPORT STATUS	timeoutAlarmSet (timer_t timeId, double seconds, VOIDFUNCPTR function, int arg);
IMPORT STATUS	timeoutAlarmCancel (timer_t timeId);
IMPORT void		timeoutAlarmSignal (timer_t timeId, int arg);

#endif /* __INCtimeoutLibh */
