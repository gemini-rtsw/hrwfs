static struct {void *v; char *c;} rcsid = {&rcsid,
	"$Id: testHarness.c,v 1.1.1.1 1999-03-17 03:14:25 cboyer Exp $"};

/*+
 *	MODULE NAME:
 *	testHarness
 *
 *	FILENAME:
 *	testHarness.c
 *
 *	PURPOSE:
 *	Test harness for wavefront sensing genSub records
 *
 *	DESCRIPTION:
 *	This file contains the functions used to simulate the input of data
 *	to the wavefront sensing system through genSub records. The functions
 *	are executed each time the test genSub records are executed. They
 *	generate test data and write that data as an array to the OUTJ field
 *	of the test genSub record. The OUTJ field is assumed to be connected to
 *	the J field of the corresponding genSub record in the wavefront sensing
 *	system, and simulates the receipt of a packet of information from the TCS.
 *
 *	FUNCTION NAME(S):
 *	testInit		- Initialisation function
 *	testTtfZero		- Test the tip tilt focus zero point update
 *	testAoZero		- Test the active optics zero point update
 *	testProbeOffset		- Test the probe offset update
 *
 *	EXTERNAL MODULES:
 *	timeLib			- The gemini time library
 *
 *	PRIOR REQUIREMENTS:
 *	Each of these functions assumes that it has been specified in the "SNAM" field
 *	of an EPICS genSub record, and the function is executed when the genSub record
 *	is processed. It is assumed that the OUTJ field of each genSub record is
 *	connected to the J field of another genSub.
 *
 *	INCLUDE FILES:
 *	recSup.h           - EPICS record support constants
 *	dbCommon.h         - Data structure and definitions common to all EPICS records
 *	genSubRecord.h     - EPICS genSub record data structure and definitions
 *	timeLib.h          - Gemini time library
 *
 *	AUTHOR:
 *	Steven Beard  (smb@roe.ac.uk)
 *-
 */
/* *INDENT-OFF* */
/*
 * $Log: not supported by cvs2svn $
 * Revision 1.4  1998/12/11 09:22:41  cics
 * TCS simulation function testSimulateAstCtxInit() added.
 *
 * Revision 1.3  1998/11/30 15:54:42  cics
 * Modifications made during SMB visit to Hilo, November 1998
 *
 * Revision 1.2  1998/07/13 15:50:42  smb
 * Debugging switched off
 *
 * Revision 1.1  1998/05/13 10:22:51  smb
 * Test harness functions added to repository
 *
 *
 */
/* *INDENT-ON* */

#include  <vxWorks.h>
#include  <stdioLib.h>
#include  <string.h>
#include  <math.h>


#include  <dbDefs.h>
#include  <genSubRecord.h>
#include  <dbCommon.h>
#include  <recSup.h>

#include  "timeLib.h"
#include  "slalib.h"
#include  "astLib.h"

#include  "errorLib.h"

#include  "testHarness.h"

/* #define DEBUG */		/* Define this to switch on debugging messages */

LOCAL const double PI = 3.1415926535;
LOCAL const double AS2R = 4.84813681109536e-6;
LOCAL const double D2R = 0.0174532925199433;


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	testInit
 *
 *	INVOCATION:
 *	testInit (pgensub)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pgensub	(struct genSubRecord *)	pointer to genSub record structure
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK, or ERROR if the initialisation routine failed
 *
 *	PURPOSE:
 *	Initialisation routine for genSub record
 *
 *	DESCRIPTION:
 *	This is the initialisation routine for a genSub record. It is called when the
 *	record is initialised - normally when the IOC is initialised via iocInit().
 *	The routine is generic and re-entrant so that the same initialisation routine
 *	is used for all genSub records.
 *	THE FUNCTION IS UNFINISHED.
 *
 *	ERROR HANDLING:
 *	Any errors are logged immediately via the error-logging library, errorLib.
 *
 *	SUPPORT FOR THIS ROUTINE:
 *	This routine makes use of one or more EPICS libraries and can
 *	therefore only be used on a CPU which can call those EPICS libraries.
 *
 *	EXTERNAL VARIABLES:
 *	(!)	epToVxSymtab		(SYMTAB_ID)	symbol table used internally by epToVxLib
 *	(!)	pRecordInitialised	(BOOL *)	array of record-initialisation-done flags
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	epToVxLib.h
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS	testInit
	(
	struct genSubRecord *	pgensub 
	)
{

#ifdef DEBUG
	printf ("testInit: %s\n", pgensub->name);
#endif /* DEBUG */

	/* No initialisation is needed. */

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	testTtfZero
 *
 *	INVOCATION:
 *	testTtfZero (pgensub)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pgensub	(struct genSubRecord *)	pointer to genSub record structure
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK, or ERROR if the routine failed
 *
 *	PURPOSE:
 *	Generate test values for "ttfZero" genSub record
 *
 *	DESCRIPTION:
 *	This function generates test data for the "ttfZero" genSub record
 *	each time the record is executed. "ttfZero" inputs are expected
 *	to be
 *v
 *v	J[0] = tSent     - Time at which data were sent
 *v	J[1] = tAppl     - Time for which data apply
 *v	J[2] = trackId   - Track identifier associated with current stream of data
 *v	J[3] = angle     - Rotation required to convert data to M2 frame of reference
 *v	J[4] = nCoeffs   - Number ofcoefficients (always 3)
 *v	J[5] = x         - Tip zero point (millimetres)
 *v	J[6] = y         - Tilt zero point (millimetres)
 *v	J[2] = z         - Focus zero point (millimetres)
 *
 *	ERROR HANDLING:
 *	Any errors are logged immediately via the error-logging library, errorLib.
 *
 *	SUPPORT FOR THIS ROUTINE:
 *	This routine makes use of one or more EPICS libraries and can
 *	therefore only be used on a CPU which can call those EPICS libraries.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	epToVxLib.h
 *
 *	DEFICIENCIES:
 *	THE FUNCTION IS UNFINISHED.
 *-
 */

STATUS	testTtfZero
	(
	struct genSubRecord *	pgensub
	)
{
	static int		index=0;				/* Index incremented each time this function is called.	*/
	double *		outputArray;			/* Pointer to output array of double values.		*/
	double			tSent, tAppl;			/* Times.											*/
	double			trackId;				/* Track ID.	*/
	double			angle;					/* Rotation angle.	*/

#ifdef DEBUG
	printf ("testTtfZero: %s. ", pgensub->name);
#endif /* DEBUG */

	/*
	 * Increment the index.
	 */

	index++;

	/*
	 * Define the output array pointer to point to the VALJ output values in the pgensub structure.
	 * Using this pointer will access the VALJ structure as if it were an array of double values.
	 */

	outputArray = (double *)pgensub->valj;

	/*
	 * Load the output array with the time at which the data were sent and apply
	 * (assumed the same for test purposes).
	 */

	/* timeNow (&tSent); */	/* fetch current time */

	tSent = 1.0;
	tAppl = tSent;
	outputArray[0] = tSent;
	outputArray[1] = tAppl;

	/*
	 * Load the output array with the track ID.
	 */

	trackId = 0.0;
	outputArray[2] = trackId;

	/*
	 * Load the output array with the current A&G to M2 rotation angle.
	 * Modify the angle each time based on index.
	 */

	angle = 15.0 + (0.05 * index);
	outputArray[3] = angle;

	/*
	 * Load the output array with the number of coefficients (which is always 3).
	 */

	outputArray[4] = 3.0;

	/*
	 * Load new tip, tilt and focus zero points
	 * Modify the values each time based on index.
	 */

	outputArray[5] = 0.5 * sin (2 * PI * index / 90.0 );
	outputArray[6] = 0.7 * cos (2 * PI * index / 120.0 );
	outputArray[7] = 2.5 * sin ( 2 * PI * index / 60.0 );

#ifdef DEBUG
	printf ("tSent=%f tAppl=%f trackId=%f angle=%f x=%f, y=%f z=%f\n", outputArray[0],
	        outputArray[1], outputArray[2], outputArray[3], outputArray[5], outputArray[6],
	        outputArray[7]);
#endif /* DEBUG */

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	testAoZero
 *
 *	INVOCATION:
 *	testAoZero (pgensub)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pgensub	(struct genSubRecord *)	pointer to genSub record structure
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK, or ERROR if the routine failed
 *
 *	PURPOSE:
 *	Generate test values for "aoZero" genSub record
 *
 *	DESCRIPTION:
 *	This function generates test data for the "aoZero" genSub record
 *	each time the record is executed. "aoZero" inputs are expected
 *	to be
 *v
 *v	J[0] = tSent     - Time at which data were sent
 *v	J[1] = tAppl     - Time for which data apply
 *v	J[2] = trackId   - Track identifier associated with current stream of data
 *v	J[3] = angle     - Rotation required to convert data to M2 frame of reference
 *v	J[4] = nCoeffs   - Number ofcoefficients
 *v	J[5] = x         - Zernike coefficient 1 (millimetres)
 *v	...
 *v	J[23] = z        - Zernike coefficient 19 (millimetres)
 *
 *	ERROR HANDLING:
 *	Any errors are logged immediately via the error-logging library, errorLib.
 *
 *	SUPPORT FOR THIS ROUTINE:
 *	This routine makes use of one or more EPICS libraries and can
 *	therefore only be used on a CPU which can call those EPICS libraries.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	epToVxLib.h
 *
 *	DEFICIENCIES:
 *	THE FUNCTION IS UNFINISHED.
 *-
 */

STATUS	testAoZero
	(
	struct genSubRecord *	pgensub
	)
{
	FAST int		i;						/* Element number.									*/
	static int		index=0;				/* Index incremented each time this function is called.	*/
	double *		outputArray;			/* Pointer to output array of double values.		*/
	double			tSent, tAppl;			/* Times.											*/
	double			trackId;				/* Track ID.	*/
	double			angle;					/* Rotation angle.	*/

#ifdef DEBUG
	printf ("testAoZero: %s. ", pgensub->name);
#endif /* DEBUG */

	/*
	 * Increment the index.
	 */

	index++;

	/*
	 * Define the output array pointer to point to the VALJ output values in the pgensub structure.
	 * Using this pointer will access the VALJ structure as if it were an array of double values.
	 */

	outputArray = (double *)pgensub->valj;

	/*
	 * Load the output array with the time at which the data were sent and apply
	 * (assumed the same for test purposes).
	 */

	/* timeNow (&tSent); */	/* fetch current time */

	tSent = 1.0;
	tAppl = tSent;
	outputArray[0] = tSent;
	outputArray[1] = tAppl;

	/*
	 * Load the output array with the track ID.
	 */

	trackId = 0.0;
	outputArray[2] = trackId;

	/*
	 * Load the output array with the current A&G to M2 rotation angle.
	 * Modify the angle each time based on index.
	 */

	angle = 15.0 + (0.05 * index);
	outputArray[3] = angle;

	/*
	 * Load the output array with the number of coefficients (which is always 3-19).
	 */

	outputArray[4] = 19.0;

	/*
	 * Load new Zernike zero points
	 * Modify the values each time based on index.
	 */

	for ( i=5; i < 24; i++ )
	{
		outputArray[i] = 0.01 * (24.0 - i) * sin (2 * PI * index / 90.0 );
	}

#ifdef DEBUG
	printf ("tSent=%f tAppl=%f trackId=%f angle=%f coeffs = %f %f %f ... %f\n", outputArray[0],
	        outputArray[1], outputArray[2], outputArray[3], outputArray[5], outputArray[6],
	        outputArray[7], outputArray[23]);
#endif /* DEBUG */

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	testProbeOffset
 *
 *	INVOCATION:
 *	testProbeOffset (pgensub)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pgensub	(struct genSubRecord *)	pointer to genSub record structure
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK, or ERROR if the routine failed
 *
 *	PURPOSE:
 *	Generate test values for "probeOffset" genSub record
 *
 *	DESCRIPTION:
 *	This function generates test data for the "ttfZero" genSub record
 *	each time the record is executed. "ttfZero" inputs are expected
 *	to be
 *v
 *v	J[0] = tMeasured - Time at which offset was measured
 *v	J[1] = trackId   - Track identifier associated with current stream of data
 *v	J[2] = angle     - Rotation of wavefront sensor
 *v	J[3] = x         - Tip offset (millimetres)
 *v	J[4] = xErr      - Tip offset error (millimetres)
 *v	J[5] = y         - Tilt offset (millimetres)
 *v	J[6] = yErr      - Tilt offset error (millimetres)
 *v	J[7] = z         - Focus offset (millimitres)
 *v	J[8] = zErr      - Focus offset error (millimetres)
 *
 *	ERROR HANDLING:
 *	Any errors are logged immediately via the error-logging library, errorLib.
 *
 *	SUPPORT FOR THIS ROUTINE:
 *	This routine makes use of one or more EPICS libraries and can
 *	therefore only be used on a CPU which can call those EPICS libraries.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	INCLUDE FILES:
 *	epToVxLib.h
 *
 *	DEFICIENCIES:
 *	THE FUNCTION IS UNFINISHED.
 *-
 */

STATUS	testProbeOffset
	(
	struct genSubRecord *	pgensub
	)
{
	static int		index=0;				/* Index incremented each time this function is called.	*/
	double *		outputArray;			/* Pointer to output array of double values.		*/
	double			tMeasured;				/* Times.											*/
	double			trackId;				/* Track ID.	*/
	double			angle;					/* Rotation angle.	*/

#ifdef DEBUG
	printf ("testProbeOffset: %s. ", pgensub->name);
#endif /* DEBUG */

	/*
	 * Increment the index.
	 */

	index++;

	/*
	 * Define the output array pointer to point to the VALJ output values in the pgensub structure.
	 * Using this pointer will access the VALJ structure as if it were an array of double values.
	 */

	outputArray = (double *)pgensub->valj;

	/*
	 * Load the output array with the time at which the data were sent and apply
	 * (assumed the same for test purposes).
	 */

	/* timeNow (&tMeasured); */	/* fetch current time */

	tMeasured = 1.0;
	outputArray[0] = tMeasured;

	/*
	 * Load the output array with the track ID.
	 */

	trackId = 0.0;
	outputArray[1] = trackId;

	/*
	 * Load the output array with the current WFS rotation angle.
	 * Modify the angle each time based on index.
	 */

	angle = 15.0 + (0.05 * index);
	outputArray[2] = angle;

	/*
	 * Load new tip, tilt and focus offsets and offset errors.
	 * Modify the values each time based on index.
	 */

	outputArray[3] = 0.5 * sin (2 * PI * index / 90.0 );
	outputArray[4] = outputArray[3] / 10.0;
	outputArray[5] = 0.5 * sin (2 * PI * index / 90.0 );
	outputArray[6] = outputArray[5] / 10.0;
	outputArray[7] = 2.5 * sin ( 2 * PI * index / 60.0 );
	outputArray[8] = outputArray[7] / 10.0;

#ifdef DEBUG
	printf ("tMeasured=%f trackId=%f angle=%f x=%f %f, y=%f %f z=%f %f\n", outputArray[0],
	        outputArray[1], outputArray[2], outputArray[3], outputArray[4], outputArray[5],
	        outputArray[6], outputArray[7], outputArray[8]);
#endif /* DEBUG */

	return (OK);
}

/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	testSimulateAstCtxInit
 *
 *	INVOCATION:
 *	testSimulateAstCtxInit (pgensub)
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

STATUS	testSimulateAstCtxInit
	(
	struct genSubRecord *	pgensub
	)
{
	/* Initialise the error handling library. */

    if ( errorInit() == ERROR )
	{
		printErr ("testSimulateAstCtxInit: Failed to initialise error context structure.\n");
		return (ERROR);
	}

	return (OK);
}


/* ------------------------------------------------------------------------------------------------ */

/*+
 *	FUNCTION NAME:
 *	testSimulateAstCtx
 *
 *	INVOCATION:
 *	testSimulateAstCtx (pgensub)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pgensub	(struct genSubRecord *)	pointer to genSub record structure
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK, or ERROR if the routine failed
 *
 *	PURPOSE:
 *	Simulate World Coordinate System context
 *
 *	DESCRIPTION:
 *	This function simulates a World Coordinate System context and makes it available on
 *	a genSub output field VALA. It is assumed that field NOVA has been set to AST_CTXA_SIZE
 *	(=39) and FTVA has been set to DOUBLE.
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

STATUS	testSimulateAstCtx
	(
	struct genSubRecord *	pgensub
	)
{
	double		rawTime;		/* Raw time.									*/
	double		tai;			/* MJD in international atomic time.			*/
	double		ut1;			/* MJD in UT1.									*/
	double		tt;				/* MJD in TT.									*/
	double		elong, elongm;	/* East longitude (radians).					*/
	double		phi, phim;		/* Latitude (radians).							*/
	double		hm;				/* Height above reference spheroid (metres).	*/
	double		xp, yp;			/* Polar motion xy coordinates (radians).		*/
	double		tdk;			/* Ambient temperature (Kelvin).                */
	double		pmb;			/* Ambient pressure (millibars).				*/
	double		rh;				/* Relative humidity (0-1).           			*/

	double		tlr;			/* Tropospheric laspse rate (Kelvin/metre).		*/
	struct PMPXRV	pmotion;	/* Proper motion structure.						*/
	struct TELP		tel;		/* Telescope dependent parameters.				*/

	/* Assume M2 is at neutral tip and tilt. */

	double	m2xy[3][2] = { { 0.0, 0.0 },
	                       { 0.0, 0.0 },
	                       { 0.0, 0.0 },
                         };

	double			a0, b0;			/* Pre-flexure Az/El.						*/
	float			v0[3];			/* Mount coordinate vector.					*/
	double			track_wl;		/* Track wavelength (microns).				*/
	double			track_ra;		/* Right ascemsion (radians)				*/
	double			track_dec;		/* Decination (radians)						*/
	FRAMETYPE		track_frame;	/* Coordinate system.						*/
	struct EPOCH	track_equinox;	/* Equinox of mean RA,Dec.					*/

	struct EPOCH epoch = { 0.0, 'J' };	/* Current epoch */


	double		aoprms[15];		/* Apparent-to-observed parameters.				*/

	double *	outputArray;	/* Pointer to output array of double values.	*/

#ifdef DEBUG
	int			i;

	printf ("testAoZero: %s. ", pgensub->name);
#endif /* DEBUG */

	/* Check the output field is of sufficient size */

	if ( pgensub->nova < AST_CTXA_SIZE )
	{
		printErr ("testSimulateAstCtx: genSub record has NOVA < AST_CTXA_SIZE\n");
		return (ERROR);
	}

	/*
	 * Define the output array pointer to point to the VALA output values in the pgensub structure.
	 * Using this pointer will access the VALA structure as if it were an array of double values.
	 */

	outputArray = (double *)pgensub->vala;

	/* Define site location and meteorological readings. */

	elongm = -2.71349248271422;
	phim = 0.346040618846507;
	hm = 4145.0;
	xp = 0.25 * AS2R;
	yp = 0.4 * AS2R;
	tdk = 275.0;
	pmb = 605.0;
	rh = 0.8;
	tlr = 0.0065;

	/* Define telescope parameters */

	tel.fl = 128000.0;
	tel.rma = 30.0 * D2R;
	tel.an = -12.0 * AS2R;
	tel.aw = -5.0 * AS2R;
	tel.pnpae = 8.0 * AS2R;
	tel.ca = -110.0 * AS2R;
	tel.ce = 22.0 * AS2R;

	tel.pox = 0.0;
	tel.poy = 0.0;

	/* Tracking frame and target information */

	track_wl = 0.55;
	track_frame = FK5;
	track_equinox.type = 'J';
	track_equinox.year = 2000.0;
	track_ra = 36.0 * D2R;
	track_dec = 45.0 * D2R;

	/* Get a time stamp and convert it to TAI */

	if ( timeNow( &rawTime ) == ERROR )
	{
		printErr ("testSimulateAstCtx: Failed to get current time.\n");
		return (ERROR);
	}
	if ( timeThenD( rawTime, TAI, &tai ) == ERROR )
	{
		printErr ("testSimulateAstCtx: Failed to convert current time to TAI.\n");
		return (ERROR);
	}
	if ( timeThenD( rawTime, UT1, &ut1 ) == ERROR )
	{
		printErr ("testSimulateAstCtx: Failed to convert current time to UT1.\n");
		return (ERROR);
	}
	if ( timeThenD( rawTime, TT, &tt ) == ERROR )
	{
		printErr ("testSimulateAstCtx: Failed to convert current time to TT.\n");
		return (ERROR);
	}

	/* Generate the apparent-to-observed parameters. */

	slaAoppa ( ut1, 0.0, elongm, phim, hm, xp, yp, tdk, pmb, rh, track_wl, tlr, aoprms );
	slaPolmo ( elongm, phim, xp, yp, &elong, &phi, &(aoprms[14]) );

	/* Generate the pre-flexure mount coordinates. */

	pmotion.pm = FALSE;
	pmotion.px = 0.0;
	pmotion.rv = 0.0;

	if ( astCoco ( track_ra, track_dec, pmotion, track_frame, track_equinox, epoch, AZEL_MNT,
	               track_equinox, tt, aoprms, tel, &a0, &b0 ) != 0
	   )
	{
		printErr ("testSimulateAstCtx: Failed to generate the pre-flexure mount coordinates.\n");
		return (ERROR);
	}

	/* Generate the mount pointing vector. */

	slaCs2c ( (float) a0, (float) b0, v0 );

	/*
	 * Now copy the WCS context just defined to the output array.
	 */

	/* Copy the timestamp. */

	outputArray[0] = tai;

	/* Copy the pre-flexure mount coordinates. */

   outputArray[1] = (double) -v0[0];
   outputArray[2] = (double) v0[1];
   outputArray[3] = (double) v0[2];

	/* Copy the telescope parameters. */

   outputArray[4] = tel.fl;
   outputArray[5] = tel.rma;
   outputArray[6] = tel.an;
   outputArray[7] = tel.aw;
   outputArray[8] = tel.pnpae;
   outputArray[9] = tel.ca;
   outputArray[10] = tel.ce;

	/* Copy the apparent-to-observed parameters. */

	outputArray[11] = aoprms[0];
	outputArray[12] = aoprms[1];
	outputArray[13] = aoprms[2];
	outputArray[14] = aoprms[3];
	outputArray[15] = aoprms[4];
	outputArray[16] = aoprms[5];
	outputArray[17] = aoprms[6];
	outputArray[18] = aoprms[7];
	outputArray[19] = aoprms[8];
	outputArray[20] = aoprms[9];
	outputArray[21] = aoprms[10];
	outputArray[22] = aoprms[11];
	/* aoprms[12] ignored. */
	outputArray[23] = aoprms[13];
	outputArray[24] = aoprms[14];

	/* Copy the m2 tip tilts. */

	outputArray[25] = m2xy[0][0];
	outputArray[26] = m2xy[0][1];
	outputArray[27] = m2xy[1][0];
	outputArray[28] = m2xy[1][1];
	outputArray[29] = m2xy[2][0];
	outputArray[30] = m2xy[2][1];

	/* Copy the pointing origins. */

	outputArray[31] = 0.0;		/* pop->mx	*/
	outputArray[32] = 0.0;		/* pop->mx	*/
	outputArray[33] = 0.0;		/* pop->ax	*/
	outputArray[34] = 0.0;		/* pop->ay	*/
	outputArray[35] = 0.0;		/* pop->bx	*/
	outputArray[36] = 0.0;		/* pop->by	*/
	outputArray[37] = 0.0;		/* pop->cx	*/
	outputArray[38] = 0.0;		/* pop->cy	*/

#ifdef DEBUG
	printf ("testAoZero: ");
	for (i=0; i < AST_CTXA_SIZE; i++)
	{
		printf ("%f ", outputArray[i]);
	}
	printf ("\n");
#endif

	return (OK);
}

