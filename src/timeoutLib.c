static struct {void *v; char *c;} rcsid = {&rcsid,
	"$Id: timeoutLib.c,v 1.1.1.1 1999-03-17 03:14:24 cboyer Exp $"};

/*+
 *	MODULE NAME:
 *	timeoutLib
 *
 *	FILENAME:
 *	timeoutLib.c
 *
 *	PURPOSE:
 *	Timeout handling library
 *
 *	DESCRIPTION:
 *	This library contains functions for defining a timeout and
 *	testing for expiration of a timeout. The functions in this library
 *	are designed to be used with the START_TIMEOUT and SEC_TO_NTICK macros,
 *	which are declared in "timeoutLib.h".	
 *v
 *v		START_TIMEOUT(pTime)
 *v
 *	The START_TIMEOUT macro gets the current time from the real time clock
 *	(using clock_gettime()) and writes it to the structure pointed to be its
 *	argument, pTime.
 *v
 *v		SEC_TO_NTICK(t)
 *v
 *	The SEC_TO_NTICK macro converts a time interval, t, in seconds into the
 *	number of system clock ticks, with the aid of sysClkRateGet. If a negative
 *	time interval is provided, the WAIT_FOREVER code is returned.
 *
 *	FUNCTION NAME(S):
 *	timeoutInit			- Initialise timeout library
 *	timeoutExpired		- Test for expiration of timeout period
 *	timeoutAlarmInit	- Create timer for alarm
 *	timeoutAlarmSet		- Set alarm
 *	timeoutAlarmCancel	- Cancel alarm
 *	timeoutAlarmSignal	- Example alarm signal handler
 *
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.10  1998/12/07 11:17:25  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.9  1998/10/01 13:45:45  cics
 * Unchanged variables changed to const
 *
 * Revision 1.8  1998/08/14 15:32:22  smb
 * Some debug statements added
 *
 * Revision 1.5  1998/06/24 17:49:14  anj
 * Minor changes, including the odd header comment bug.
 *
 * Revision 1.4  1998/02/23 13:38:56  smb
 * Rearranged code for printability
 *
 * Revision 1.3  1998/01/30 15:30:18  smb
 * Fixed some problems uncovered by prolint
 *
 * Revision 1.2  1998/01/16 15:57:27  smb
 * Quell compiler warning about rcsid using anj's idea
 *
 * Revision 1.1.1.1  1997/11/28 11:46:18  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */


/* includes */

#include <vxWorks.h>
#include <timers.h>
#include <sysLib.h>
#include <math.h>
#include "errorLib.h"
#include "timeoutLib.h"

/* defines */

/* #define DEBUG */								/* Define this macro to enable debug messages	*/

	/*
	 * timers.h defines a symbol to specify an absolute time (TIMER_ABSTIME), but there isn't a symbol
	 * to specify a relative time. A relative time is assumed if any value different from 
	 * TIMER_ABSTIME is specified. The following macro defines a value guaranteed to be different
	 * from TIMER_ABSTIME.
	 */

#define		TIMER_RELTIME	(~TIMER_ABSTIME)	/* Value guaranteed not to be TIMER_ABSTIME 	*/

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	timeoutExpired
 *
 *	INVOCATION:
 *	timeoutExpired (timeoutPeriod, pTimeStart)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	timeoutPeriod	(const double)				duration of timeout, seconds
 *	(>)	pTimeStart		(const struct timespec *)	system time when initialised
 *
 *	FUNCTION VALUE:
 *	(BOOL)	TRUE if the timeout period has expired, FALSE if it has not.
 *
 *	PURPOSE:
 *	Test for expiration of timeout period
 *
 *	DESCRIPTION:
 *	This routine subtracts the current system time from that held in
 *	argument timeStart. If the difference exceeds the specified timeout
 *	period then it returns TRUE to indicate that the timeout has expired,
 *	otherwise the routine returns FALSE. An infinite timeout period may
 *	be specified by setting parameter timeoutPeriod < 0.0, in which case
 *	the routine always returns FALSE.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	INCLUDE FILES:
 *	errorLib.h.
 *	timeoutLib.h
 *
 *	PRIOR REQUIREMENTS:
 *	The timespec structure pointed to by pTimeStart must have been
 *	previously initialised, e.g. in a call to START_TIMEOUT() or to
 *	clock_gettime(). The realtime clock must have been initialised
 *	in a previous call to timeoutInit() or clock_setres().
 *
 *	DEFICIENCIES:
 *	None known
 *
 *	AUTHOR:
 *	Nick Dillon
 *-
*/

BOOL timeoutExpired
	(
	const double				timeoutPeriod,
	const struct timespec *		pTimeStart
	)
{
	struct timespec				timeNow;
	double						timeInterval;


	/*
	 * If timeoutPeriod is negative there is no timeout, so the timeout period
	 * will never expire.
	 */

	if (timeoutPeriod < 0.0) return (FALSE);

	/* Get the current time from the real time clock and compare it with the
	 * time recorded at the start of the timeout period. If the time interval
	 * is greater than or equal to the timeout period return TRUE, otherwise
	 * return FALSE. (The 1.0e-09 factor converts nanoseconds to seconds).
	 *
	 * There is no need to test for an ERROR return, since START_TIMEOUT()
	 * must have been passed before the call to clock_gettime() call made.
	 */

	clock_gettime ((clockid_t) CLOCK_REALTIME, & timeNow);

	timeInterval =
	  (timeNow.tv_sec - pTimeStart->tv_sec) + (timeNow.tv_nsec - pTimeStart->tv_nsec) * 1.0e-09;

	if (timeInterval >= timeoutPeriod)
	{
#ifdef DEBUG
		printf ("timeoutExpired: timeout occurred, interval = %g, threshold = %g\n", 
				timeInterval, timeoutPeriod);
#endif /* DEBUG */
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	timeoutInit
 *
 *	INVOCATION:
 *	timeoutInit ()
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	None
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if successful, ERROR if not.
 *
 *	PURPOSE:
 *	Initialise the timeoutLib library
 *
 *	DESCRIPTION:
 *	This routine initialises the real time clock so it may be used by the
 *	timeoutLib library.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	INCLUDE FILES:
 *	errorLib.h.
 *	timeoutLib.h
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	DEFICIENCIES:
 *	None known
 *
 *	AUTHOR:
 *	Nick Dillon
 *-
*/

STATUS timeoutInit (void)
{
	struct timespec	timeStart;

	/*
	 * Set clock resolution based on the system clock rate (the system clock is used
	 * for timeouts).
	 */

	timeStart.tv_sec = 0;
	timeStart.tv_nsec = (long) ((1.0e09) / sysClkRateGet ());

	if (timeStart.tv_nsec < 1) timeStart.tv_nsec = 1;

	return (clock_setres ((clockid_t) CLOCK_REALTIME, & timeStart));
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	timeoutAlarmInit
 *
 *	INVOCATION:
 *	timeoutAlarmInit (pTimeId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(<)		pTimeId		(timer_t *)		Pointer to alarm timer ID
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if successful, ERROR if not.
 *
 *	PURPOSE:
 *	Create the timer used for generating an alarm
 *
 *	DESCRIPTION:
 *	This routine creates the timer used for generating an alarm and returns
 *	its ID.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	INCLUDE FILES:
 *	errorLib.h.
 *	timeoutLib.h
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	DEFICIENCIES:
 *	None known
 *
 *	AUTHOR:
 *	Steven Beard
 *-
*/

STATUS timeoutAlarmInit
	(
	timer_t *			pTimeId
	)
{
	/*
	 * Create the timer, using the system realtime clock, and return
	 * the timer ID in the object pointed to be pTimeId.
	 */

	return (timer_create (CLOCK_REALTIME, NULL, pTimeId));
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	timeoutAlarmSet
 *
 *	INVOCATION:
 *	timeoutAlarmSet (timeId, seconds, function, arg)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)		timeId		(timer_t)		Timer ID
 *	(>)		seconds		(double)		Number of seconds until timer expires
 *	(>)		function	(VOIDFUNCPTR) 	Function to be called when timer expires
 *	(>)		arg			(int)			Argument to be passed to signal handler
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if successful, ERROR if not.
 *
 *	PURPOSE:
 *	Set an alarm
 *
 *	DESCRIPTION:
 *	This routine connects the alarm timer to a specified signal handling function
 *	and then starts the timer for a specified number of seconds. When the timer
 *	expires, the signal handling function will be called with the specified argument.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	INCLUDE FILES:
 *	errorLib.h.
 *	timeoutLib.h
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	DEFICIENCIES:
 *	None known
 *
 *	AUTHOR:
 *	Steven Beard
 *-
*/

STATUS timeoutAlarmSet
	(
	timer_t			timeId,				/* Timer ID.									*/
	double			seconds,			/* Number of seconds until timer expires.		*/
	VOIDFUNCPTR		function,			/* Function to be called when timer expires.	*/
	int				arg					/* Argument to be passed to function.			*/
	)
{
	struct itimerspec	newTime;		/* Timer specification structure.				*/
	double				fsecs;			/* seconds with fractional part removed.		*/

	/*
	 * Connect the specified function to the alarm timer, specifying the
	 * argument the function will be called with when the timer expires.
	 */

#ifdef DEBUG
	printf ("timeoutAlarmSet: Connecting function %p to timer %d with argument %d\n",
	         function, (int) timeId, arg);
#endif

	if ( timer_connect( timeId, function, arg ) == ERROR )
	{
		ERROR_SET (0, "timer_connect failed", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/*
	 * Load the newTime data structure, specifying non-periodic time interval
	 * of the specified number of seconds.
	 */

	fsecs = floor(seconds);

	newTime.it_interval.tv_sec  = 0;			/* Non periodic */
	newTime.it_interval.tv_nsec = 0;
	newTime.it_value.tv_sec  = (int) fsecs;
	newTime.it_value.tv_nsec = (int) ((seconds - fsecs) * 1.0e9);

#ifdef DEBUG
	printf ("timeoutAlarmSet: Setting timer for %f seconds (%d s + %d ns)\n",
	         seconds, newTime.it_value.tv_sec, newTime.it_value.tv_nsec);
#endif

	/*
	 * Set the alarm timer relative to the current time and start it counting down.
	 * Once this call is made, the specified function will be called automatically
	 * when the timer expires, unless the timer is cancelled.
	 */

	if ( timer_settime( timeId, TIMER_RELTIME, &newTime, NULL ) == ERROR )
	{
		ERROR_SET (0, "timer_settime failed", ERROR_LOG_SAVE);
		return (ERROR);
	}

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	timeoutAlarmCancel
 *
 *	INVOCATION:
 *	timeoutAlarmCancel (timeId)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)		timeId		(timer_t)		Timer ID
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if successful, ERROR if not.
 *
 *	PURPOSE:
 *	Cancel an alarm
 *
 *	DESCRIPTION:
 *	This routine cancels an alarm started with timeoutAlarmSet.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	INCLUDE FILES:
 *	errorLib.h.
 *	timeoutLib.h
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	DEFICIENCIES:
 *	None known
 *
 *	AUTHOR:
 *	Steven Beard
 *-
*/

STATUS timeoutAlarmCancel
	(
	timer_t			timeId				/* Timer ID.									*/
	)
{
	struct itimerspec	newTime;		/* Timer specification structure.				*/


	/*
	 * Load the newTime data structure with a non-periodic zero time interval, which should
	 * cancel the timer.
	 */

	newTime.it_interval.tv_sec  = 0;			/* Non periodic */
	newTime.it_interval.tv_nsec = 0;
	newTime.it_value.tv_sec  = 0;				/* Cancel timer */
	newTime.it_value.tv_nsec = 0;

	/*
	 * Cancel the alarm timer.
	 */

	if ( timer_settime( timeId, TIMER_RELTIME, &newTime, NULL ) == ERROR )
	{
		ERROR_SET (0, "timer_settime failed", ERROR_LOG_SAVE);
		return (ERROR);
	}

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	timeoutAlarmSignal
 *
 *	INVOCATION:
 *	timeoutAlarmSignal (timeId, arg)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)		timeId		(timer_t)		Timer ID
 *	(>)		arg			(int)			Argument defined when timer started
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Example alarm signal handler
 *
 *	DESCRIPTION:
 *	This is an example of an alarm signal handler. It may be used as a template
 *	for a real signal handler and for testing the timeoutAlarm functions.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	INCLUDE FILES:
 *	errorLib.h.
 *	timeoutLib.h
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	DEFICIENCIES:
 *	None known
 *
 *	AUTHOR:
 *	Steven Beard
 *-
*/

void timeoutAlarmSignal
	(
	timer_t			timeId,				/* Timer ID.									*/
	int				arg					/* Argument passed to function.					*/
	)
{

	/*
	 * Print a message to say the signal handler has been called.
	 */

	printf ("timeoutAlarmSignal: Alarm triggered with argument %d\n", arg);
}
