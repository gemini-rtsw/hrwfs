static struct {void *v; char *c;} rcsid = {&rcsid,
	"$Id: wfsWcs.c,v 1.1.1.1 1999-03-17 03:14:24 cboyer Exp $"};

/*+
 *	MODULE NAME:
 *	wfsWcs
 *
 *	FILENAME:
 *	wfsWcs.c
 *
 *	PURPOSE:
 *	Wavefront Sensing World Coordinate System functions
 *
 *	DESCRIPTION:
 *	This library contains a miscellaneous collection of WCS functions.
 *
 *	FUNCTION NAME(S):
 *	wfsUpdateAstCtx			- Updates local World Coordinate System context
 *
 *	IGNORED FUNCTION NAME(S):
 *
 *	EXTERNAL MODULES:
 *	errorLib.c				- Contains error count for current processor, errorCount
 *	astLib.c				- WCS library
 *
 *	AUTHORS:
 *	Steven Beard
 *
 *	DEFICIENCIES:
 *	These functions have to be separate from wfsLib because of problems including epToVxLib.h
 *	and dbDefs.h at the same time - TO BE RESOLVED LATER. SMB - 16 November 1998.
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.3  1998/12/11 09:21:41  cics
 * Extra error checking
 *
 * Revision 1.2  1998/12/07 11:17:31  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.1  1998/11/30 15:54:45  cics
 * Modifications made during SMB visit to Hilo, November 1998
 *
 *INDENT-ON*
 *-
 */

/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#include <sysLib.h>
#else
#error This code only runs under VxWorks
#endif	/* vxWorks */

#include <stdio.h>

#include  <dbDefs.h>
#include  <genSubRecord.h>
#include  <dbCommon.h>
#include  <recSup.h>
#include  <alarm.h>

/* #define DEBUG */							/* Define this macro to enable debugging. */

#include "timeLib.h"
#include "slalib.h"
#include "astLib.h"

#include "gemTypes.h"
#include "errorLib.h"
#include "wfsLib.h"
#include "wfsWcs.h"

/* Global variables */

/*
 * Store the current TCS tracking variables in global variables. It is safe to do this
 * because all wavefront sensors will share this information.
 * The global variables are initialised to sensible defaults.
 */

FRAMETYPE	tcsTrackFrame		= FK5;
double		tcsTrackRA			= 0.0;
double		tcsTrackDec			= 0.0;
char		tcsTrackEquinoxType	= 'J';
double		tcsTrackEquinoxYear	= 2000.0;
char		tcsTrackEpochType	= 'J';
double		tcsTrackEpochYear	= 2000.0;
double		tcsTrackWavelength	= 0.55;


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	wfsUpdateAstCtxInit
 *
 *	INVOCATION:
 *	wfsUpdateAstCtxInit (pgensub)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pgensub	(struct genSubRecord *)	pointer to genSub record structure
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK, or ERROR if the routine failed
 *
 *	PURPOSE:
 *	Initialise update local World Coordinate System context genSub record
 *
 *	DESCRIPTION:
 *	This function carries out any initialisation required by the
 *	update local World Coordinate System context genSub record. In practise
 *	it just initialises the error library.
 *
 *	REFERENCE:
 *
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	epToVxLib.h
 *	astLib.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS	wfsUpdateAstCtxInit
	(
	struct genSubRecord *	pgensub
	)
{
	/* Initialise the error handling library. */

    if ( errorInit() == ERROR )
	{
		printErr ("wfsUpdateAstCtxInit: Failed to initialise error context structure.\n");
		return (ERROR);
	}

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	wfsUpdateAstCtx
 *
 *	INVOCATION:
 *	wfsUpdateAstCtx (pgensub)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pgensub	(struct genSubRecord *)	pointer to genSub record structure
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK, or ERROR if the routine failed
 *
 *	PURPOSE:
 *	Update local World Coordinate System context based on inputs to genSub record
 *
 *	DESCRIPTION:
 *	This function updates the current World Coordinate System context using
 *	information obtained from the TCS. It is assumed that the INPA field
 *	of the genSub record with which this function is associated is connected
 *	to the VALA field of the "astCtx" record in the TCS database. It is assumed that the
 *	NOA field is defined as AST_CTXA_SIZE (=39) and the FTA field is defined as DOUBLE.
 *
 *	REFERENCE:
 *	See the document tcs_ptw_008, "World Coordinates, Part I: Astrometry" (Section entitled
 *	"Real Time Aspects") for more information.
 *
 *	SUPPORT FOR THIS ROUTINE:
 *	This routine makes use of one or more EPICS libraries and can
 *	therefore only be used on a CPU which can call those EPICS libraries.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	It is assumed that the genSub record is connected in such a way that TCS WCS context
 *	information is written to the INPA link.
 *
 *	INCLUDE FILES:
 *	epToVxLib.h
 *	astLib.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS	wfsUpdateAstCtx
	(
	struct genSubRecord *	pgensub
	)
{
#ifdef DEBUG
	int			noa;			/* Number of input values.						*/
	int			i;				/* Index.										*/
	double *	darray;			/* Pointer to array of context values obtained.	*/
#endif
	FRAMETYPE	trackFrame;
	char		trackEquinoxType;
	double		trackEquinoxYear;
	double		trackWavelength;
	double		trackRA;
	double		trackDec;
	char		trackEpochType;
	double		trackEpochYear;

	static BOOL	tcsWasConnected = TRUE;
								/* Flag used to record changes in TCS connection state. */
	static BOOL firstTime = TRUE;

	/*
	 * Don't do anything the first time this function is called to allow time for
	 * the database to settle down and all the connections to be made. This will prevent
	 * the output of a "TCS database not connected" error followed immediately by a
	 * "TCS database reconnected" message.
	 */

	if ( firstTime )
	{
#ifdef DEBUG
		printf ("wfsUpdateAstCtx: First time - do nothing.\n");
#endif
		firstTime = FALSE;
		return (OK);
	}

	/*
	 * If the TCS database is not connected it will not be possible to obtain values.
	 * Whenever the TCS disconnects the genSub record changes its alarm severity to INVALID.
	 *
	 * This error message can get annoying if it repeats regularly, so only changes
	 * in status are recorded.
	 */

	if ( pgensub->sevr == INVALID_ALARM )
	{
		if ( tcsWasConnected )
		{
			ERROR_SET (0, "**** TCS database not connected ****", ERROR_LOG_NOW);
			tcsWasConnected = FALSE;
			return (ERROR);
		}
	}
	else if ( !tcsWasConnected )
	{
		MESSAGE_LOG (MSG_LOG, "**** TCS database reconnected ****");
		tcsWasConnected = TRUE;
	}

	/*
	 * Pass the information contained in field A to astSetCtx to set the local WCS context.
	 */

#ifdef DEBUG
	noa = (int) pgensub->noa;
	darray = (double *) pgensub->a;

	printf ("wfsUpdateAstCtx: ");

	for ( i=0; i<noa; i++ )
	{
		printf ("%f ", darray[i]);
	}
	printf ("\n");
#endif

	astSetctx ( pgensub->a );

	/*
	 * Obtain the tracking frame, equinox, wavelength, RA, Dec and epoch from fields
	 * B, C, D, E, F and G and set the local tracking context.
	 */

	if ( strncmp( (char *) pgensub->b, "FK5", 3 ) == 0 )
	{
		trackFrame = FK5;
	}
	else if ( strncmp( (char *) pgensub->b, "FK4", 3 ) == 0 )
	{
		trackFrame = FK4;
	}
	else if ( strncmp( (char *) pgensub->b, "APPT", 4 ) == 0 )
	{
		trackFrame = APPT;
	}
	else if ( strcmp( (char *) pgensub->b, "AZEL_TOPO" ) == 0 )
	{
		trackFrame = AZEL_TOPO;
	}
	else if ( strcmp( (char *) pgensub->b, "AZEL_MNT" ) == 0 )
	{
		trackFrame = AZEL_MNT;
	}
	else
	{
		/* Unrecognised value - use the default. */
		trackFrame = FK5;
	}

	/* Separate the equinox type character from the year. */

	if ( ( sscanf (pgensub->c, "%c%lf", &trackEquinoxType, &trackEquinoxYear) != 2 ) ||
	     ( trackEquinoxType < 'A') ||
	     ( trackEquinoxType > 'Z') ||
	     ( trackEquinoxYear <= 0.0 )
	   )
	{
		/* If the sccanf failed use defaults. */
		trackEquinoxType = 'J';
		trackEquinoxYear = 2000.0;
	}

	/* Convert wavelength from Angstroms into microns. */
	trackWavelength   = (* ((double *) pgensub->d)) / 10000.0;

	/* Convert the RA from degrees into hours. Leave the Dec as degrees. */
	trackRA  = (* ((double *) pgensub->e)) / 15.0;
	trackDec = * ((double *) pgensub->f);

	/* Separate the epoch type character from the year. */

	if ( ( sscanf (pgensub->g, "%c%lf", &trackEpochType, &trackEpochYear) != 2 ) ||
	     ( trackEpochType < 'A') ||
	     ( trackEpochType > 'Z') ||
	     ( trackEpochYear <= 0.0 )
	   )
	{
		/* If the sccanf failed use defaults. */
		trackEpochType = 'J';
		trackEpochYear = 2000.0;
	}

#ifdef DEBUG
	printf ("wfsUpdateAstCtx: Track frame=%s (%d), Track equinox=%s (%c %f), Wavelength=%f\n",
		(char *) pgensub->b, (int) trackFrame, (char *) pgensub->c, trackEquinoxType,
		trackEquinoxYear, trackWavelength);
	printf ("                 Track RA/Dec=(%f,%f), Track epoch=%s (%c %f)\n",
		trackRA, trackDec, (char *) pgensub->g, trackEpochType, trackEpochYear);
#endif

	wfsSetTrackFrame (trackFrame, trackEquinoxType, trackEquinoxYear, trackWavelength,
		trackRA, trackDec, trackEpochType, trackEpochYear);

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	wfsSetTrackFrame
 *
 *	INVOCATION:
 *	wfsSetTrackFrame (FRAMETYPE frame, char equinoxType, double equinoxYear, double wavelength,
 *		double RA, double Dec, char epochType, double epochYear)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	frame			(FRAMETYPE)		Tracking frame
 *	(>)	equinoxType		(char)			Equinox type
 *	(>)	equinoxYear		(double)		Equinox year
 *	(>)	wavelength		(double)		Wavelength in microns
 *	(>)	RA				(double)		Right Ascension in hours
 *	(>) Dec				(double)		Declination in degrees
 *	(>)	epochType		(char)			Epoch type
 *	(>)	epochYear		(double)		Epoch year
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Set the local copy of the current TCS tracking frame
 *
 *	DESCRIPTION:
 *
 *	EXTERNAL VARIABLES:
 *	(<)	tcsTrackFrame		(FRAMETYPE)
 *	(<)	tcsTrackEquinoxType (char)
 *	(<)	tcsTrackEquinoxYear	(double)
 *	(<)	tcsTrackWavelength	(double)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	epToVxLib.h
 *	astLib.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

void	wfsSetTrackFrame
	(
	FRAMETYPE		frame,
	char			equinoxType,
	double			equinoxYear,
	double			wavelength,
	double			RA,
	double			Dec,
	char			epochType,
	double			epochYear
	)
{
	
	tcsTrackFrame		= frame;
	tcsTrackEquinoxType = equinoxType;
	tcsTrackEquinoxYear	= equinoxYear;
	tcsTrackWavelength	= wavelength;
	tcsTrackRA			= RA;
	tcsTrackDec			= Dec;
	tcsTrackEpochType 	= epochType;
	tcsTrackEpochYear	= epochYear;

	return;
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	wfsGetTrackFrame
 *
 *	INVOCATION:
 *	wfsGetTrackFrame (FRAMETYPE * pFrame, char * pEquinoxType, double * pEquinoxYear,
 *		double * pWavelength, double * pRA, double * pDec,  char * pEpochType, double * pEpochxYear)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(<)	pFrame				(FRAMETYPE *)	Tracking frame
 *	(<)	pEquinoxType		(char *)		Equinox type
 *	(<)	pEquinoxYear		(double *)		Equinox year
 *	(<)	pWavelength			(double *)		Wavelength in microns
 *	(<)	pRA					(double *)		Right Ascension in hours
 *	(<) pDec				(double *)		Declination in degrees
 *	(<)	pEpochType			(char *)		Epoch type
 *	(<)	pEpochYear			(double *)		Epoch year
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Get the local copy of the TCS tracking frame
 *
 *	DESCRIPTION:
 *	This function updates the current World Coordinate System context using
 *	information obtained from the TCS. It is assumed that the INPA field
 *	of the genSub record with which this function is associated is connected
 *	to the VALA field of the "astCtx" record in the TCS database.
 *
 *	EXTERNAL VARIABLES:
 *	(>)	tcsTrackFrame		(FRAMETYPE)
 *	(>)	tcsTrackEquinoxType (int)
 *	(>)	tcsTrackEquinoxYear	(double)
 *	(>)	tcsTrackWavelength	(double)
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	epToVxLib.h
 *	astLib.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

void	wfsGetTrackFrame
	(
	FRAMETYPE *		pFrame,
	char *			pEquinoxType,
	double *		pEquinoxYear,
	double *		pWavelength,
	double *		pRA,
	double *		pDec,
	char *			pEpochType,
	double *		pEpochYear
	)
{
	
	* pFrame        = tcsTrackFrame;
	* pEquinoxType  = tcsTrackEquinoxType;
	* pEquinoxYear  = tcsTrackEquinoxYear;
	* pWavelength   = tcsTrackWavelength;
	* pRA           = tcsTrackRA;
	* pDec          = tcsTrackDec;
	* pEpochType    = tcsTrackEpochType;
	* pEpochYear    = tcsTrackEpochYear;

	return;
}

/* ------------------------------------------------------------------------------------------------ */
