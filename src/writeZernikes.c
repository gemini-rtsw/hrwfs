/* Id:$ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * writeZernikes.c
 * 
 * PURPOSE
 * -------
 * Take zernike data from the wavefront processing section and make available
 * on the synchro bus and to the TCS
 * 
 * FUNCTION NAME(S)
 * ----------------
 * gensubToTcsInit	- Initialisation semaphores
 * gensubToTcsTtf	- Write ttf data from global array to port VALJ
 * gensubToTcsAo	- Write ao data from global array to port VALJ
 * writeWfsToTcs	- Write data from designated structure to global array
 *			  and low pass filter
 * writeWfsToSynchro	- Write data from designated structure to synchro bus
 * dfilter		- low pass filter
 * ttfZero		- Receive ttfZero array from TCS
 * aoZero		- Receive aoZero array from TCS
 * showAoDiags		- Write diagnostic data from ao osp structure to gensub
 *			  outputs for display
 * showFgDiags		- Write diagnostic data from fg osp structure to gensub
 *			  outputs for display
 * gensubFanDouble	- receive array of doubles on port A, write elements to
 *			  individual output ports
 * 
 * DEPENDENCIES
 * ------------
 *
 * LIMITATIONS
 * -----------
 * 
 * AUTHOR
 * ------
 * Sean Prior (srp@roe.ac.uk)
 * 
 * HISTORY
 * -------
 * 
 * 28-Oct-1998: Original (srp)
 * 29-Oct-1998: Array indexing for wfs structures is 1 ->np, not 0 -> (np-1)
 * 11-Nov-1998: Add frame of reference conversion
 * 05-Jan-1999: Include nulling zernike values from ttfZero and aoZero
 * 12-Jan-1999: Add showAoDiags and showFgDiags function
 * 13-Jan-1999: Modify gensubToTcsTtf to write zernikes and errors to output ports
 * 21-Jan-1999: Modify to continue initialisation even if synchro bus
 *              not present then inhibit synchro writes accordingly. Also initialise time
 *		written to synchro to zero until format sorted out, increment time count
 *		for each write to the synchro bus as temporary measure.
 * 22-Jan-1999: Add function to read guide probe angle in ttfZero and aoZero.
 * 23-Jan-1999: Modify gensubToTcsAo to write arrays of zernikes and errors to outputs a and b
 *		Add function gensubFanDoubles to read array of doubles and write elements to
 *		the output ports
 * 24-Jan-1999: ttfZero and aoZero - read ports B and C for fudge factors in polarity and rotation
 *
 */
/* INDENT ON */
/* ===================================================================== */

/* specify constant definitions */

#ifndef PI
#define PI 3.14159265358979
#endif

#define	TTF_ARRAY_SIZE		8
#define	TTF_ZERO_ARRAY_SIZE	9
#define	AO_ZERO_ARRAY_SIZE	24
#define AO_ARRAY_SIZE		40
#define MAX_FILTERS		(3 * MAX_WFS_SOURCES)
#define WFS_TIMEOUT		40
#define DEGS2RADS		((double)(2.0*PI)/(double)360.0)	/* conversion factor for degrees to radians */
#define DISCARD_THRESHOLD 60
#define LOW_PROBE_ANGLE		-360.0	/* high limit on guide probe angle (degrees) */	
#define HIGH_PROBE_ANGLE	360.0	/* low limit on guide probe angle (degrees) */

/* specify include files */

#include <vxWorks.h>
#include <taskLib.h>
#include <semLib.h>
#include <logLib.h>
#include <stdioLib.h>
#include <dbDefs.h>
#include <wdLib.h>
#include <msgQLib.h>
#include <string.h>
#include <logLib.h>
#include <subRecord.h>
#include <cadRecord.h>
#include <recSup.h>
#include <dbCommon.h>
#include <genSubRecord.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <gemTypes.h>
#include <sdsuLib.h>

#include "osp.h"
#include "synchroMap.h"

typedef struct
{
	double probeAngle;	/* angle of guide probe supplied by Zeiss */
	double tcsAngle;	/* rotation angle supplied by TCS */
	double	theta;
	double	sinTheta;
	double	cosTheta;
	double	null[AO_ZERO_ARRAY_SIZE];
	SEM_ID	access;
}frame;

typedef struct
{
	double	z2;
	double	z3;
	double	z4;
	double	z5;
	double	z6;
	double	z7;
	double	z8;
	double	z9;
	double	z10;
	double	z11;
	double	z12;
	double	z13;
	double	z14;
	double	z15;
	double	z16;
	double	z17;
	double	z18;
	double	z19;
	double	z20;
}converted;

/* declare global variables */

frame	*ag2m2[MAX_WFS_SOURCES];
frame	*ag2tcs[MAX_WFS_SOURCES];
wfs	*ptr[MAX_WFS_SOURCES];
double	ttfData[MAX_WFS_SOURCES][AO_ARRAY_SIZE+2];
double	aoData[MAX_WFS_SOURCES][AO_ARRAY_SIZE+2];
float	data[MAX_WFS_SOURCES][AO_ARRAY_SIZE+2];
float	errors[MAX_WFS_SOURCES][AO_ARRAY_SIZE+2];
SEM_ID	wfsLock[MAX_WFS_SOURCES];
SDSU_ID sdsuId[MAX_WFS_SOURCES];

/* declare externals */

extern struct OSP_CONTEXT *wfsAoAddr[MAX_WFS_SOURCES];
extern struct OSP_CONTEXT *wfsFgAddr[MAX_WFS_SOURCES];


/* declare prototypes */

long rmIntSend(int interrupt, int node);

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * dfilter
 *
 * INVOCATION:
 * double newSample
 * int Id
 *
 * double	dfilter(double newSample, int Id)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > double newSample       - latest data sample
 * > int    iD              - identification of filter bank. There are three filters
 *                            per wfs source hence:-
 *
 *                            source     xtilt       ytilt        focus
 *                            -----------------------------------------
 *                            HRWFS        0           1            2
 *                            PWFS1        3           4            5
 *                            PWFS2        6           7            8
 *                            OIWFS        9           10           11
 *                            AOWFS        12          13           14
 *
 * FUNCTION VALUE:
 * double     returns current filtered value
 *
 * PURPOSE:
 * Filter the data in accordance with the IIR filter coefficients specified
 *
 * DESCRIPTION:
 * The function performs a two pole low pass butterworth filter on the supplied
 * data. A history array is maintained for each bank identified by the index Id.
 * The cutoff frequency is approximately 0.05*Fsample hence for a 200Hz update
 * the spectral content above 10Hz is substantially attenuated suitable for
 * sub-sampling at 20Hz.
 *
 * EXTERNAL VARIABLES:
 * 
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 *
 *
 * HISTORY (optional):
 * 28-Oct-1998  Original version                                Sean Prior
 *-
 */

double	dfilter(double newSample, int Id)
{
	int i = 0;
	double sum = 0;
	static double sample[6][MAX_FILTERS];

	static double coeffs[] = {
		1.0,
		1.77863177782458,
		-0.80080264666571,
		0.00554271721028,
		0.01108543442056,
		0.00554271721028
	};

	/* put new sample into the array */

	sample[3][Id] = newSample;

	/* multiply samples by coefficients and accumulate */

	for(i=1; i < 6; i++)
		sum += sample[i][Id]*coeffs[i];

	/* ripple samples ready for next call */

	sample[5][Id] = sample[4][Id];
	sample[4][Id] = sample[3][Id];
	sample[2][Id] = sample[1][Id];
	sample[1][Id] = sum;

	return(sum);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * gensubToTcsInit
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long	status;
 *
 * long    gensubToTcsInit(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)	pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Initialise structures and semaphores for wfs processing
 *
 * DESCRIPTION:
 * Detects the presence of the synchro bus card at the expected address
 * Create array of mutex semaphores corresponding to each of the wfs sources
 * Create structures to hold coordinate conversion transformations
 * Assign pointers to synchro bus pages for each wfs source
 *
 * EXTERNAL VARIABLES:
 * wfsLock[]	- Global array of mutex semaphores
 * ag2m2[]	- array of pointers to coord conversion structures
 * ag2tcs[]	- array of pointers to coord conversion structures
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 28-Oct-1998: Original version (srp)
 * 05-Jan-1999: Put all initialisation and semaphore creation in this section rather
 *		than creating as necessary during operation
 * 22-Jan-1999: Initialise time values on synchro bus to 0.0
 *-
 */

long    gensubToTcsInit(struct genSubRecord * pgsub)
{
	int source;
	memMap	*basePtr = (memMap *)SYNCHROBASE;
	static	int processedFlag = FALSE;
	char	junk;

	/* this initialisation routine only needs to be called once */

	if(processedFlag != FALSE)
		return(OK);
	else
		processedFlag = TRUE;


	for (source = HRWFS; source < MAX_WFS_SOURCES; source++)
    	{
		/* create semaphore to prevent multiple access to wfs data */

		if(wfsLock[source] == NULL)
		{
			if ((wfsLock[source] = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) == NULL)
			{
		    		printf ("unable to create wfsLock[%d] sem\n", source);
			}
		}
	}

	for (source = HRWFS; source < MAX_WFS_SOURCES; source++)
	{
		/* create structure holding angle and null values for ttf data */

		if((ag2tcs[source] = (frame *)calloc(1, sizeof(frame))) == NULL)
		{
			logMsg("Unable to calloc conversion frame for source %d\n", (int)source, 0, 0, 0, 0, 0);
		}
		else
		{
		    if ((ag2tcs[source]->access = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) == NULL)
		    {
			logMsg("Unable to create mutex for conversion frame\n", 0, 0, 0, 0, 0 ,0);
		    }
		    else
		    {
			/* initialise trig values */

			ag2tcs[source]->probeAngle = 0.0;
			ag2tcs[source]->tcsAngle = 0.0;
			ag2tcs[source]->theta	= 0.0;
			ag2tcs[source]->sinTheta = sin(0.0);
			ag2tcs[source]->cosTheta = cos(0.0);
		    }
		}

		/* create structure holding angle and null values for ao data */

		if( (ag2m2[source] = (frame *)calloc(1, sizeof(frame))) == NULL)
		{
			logMsg("Unable to calloc conversion frame for source %d\n", (int)source, 0, 0, 0, 0, 0);
		}
		else
		{
		    if ((ag2m2[source]->access = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | SEM_INVERSION_SAFE)) == NULL)
		    {
			logMsg("Unable to create mutex for conversion frame\n", 0, 0, 0, 0, 0 ,0);
		    }
		    else
		    {
			/* initialise trig values */

			ag2m2[source]->probeAngle = 0.0;
			ag2m2[source]->tcsAngle = 0.0;
			ag2m2[source]->theta	= 0.0;
			ag2m2[source]->sinTheta = sin(0.0);
			ag2m2[source]->cosTheta = cos(0.0);
		    }
		}
	}

	/* verify presence of 5588 synchro card */

	if (vxMemProbe ((void *)basePtr, VX_READ, 1, &junk) != OK)    
	{
		logMsg("synchro card not detected at address %p\n", (int)basePtr, 0, 0, 0, 0, 0);
		basePtr = NULL;
		return(ERROR);
	}

	/* if synchro card present, initialise structure pointers */

	for (source = PWFS1; source < MAX_WFS_SOURCES; source++)
	{
		/* assign pointers and write ID strings for synchro bus */

		if(ptr[source] == NULL)
		{
			switch(source)
			{
			case PWFS1:
				ptr[PWFS1] = (wfs*)&basePtr->pwfs1;
				strncpy(ptr[PWFS1]->name, "pwfs1", 15);
				ptr[PWFS1]->time = 0.0;
				break;
			case PWFS2:
				ptr[PWFS2] = (wfs*)&basePtr->pwfs2;
				strncpy(ptr[PWFS2]->name, "pwfs2", 15);
				ptr[PWFS2]->time = 0.0;
				break;
			case OIWFS:
				ptr[OIWFS] = (wfs*)&basePtr->oiwfs;
				strncpy(ptr[OIWFS]->name, "oiwfs", 15);
				ptr[OIWFS]->time = 0.0;
				break;
			case AOWFS:
				ptr[AOWFS] = (wfs*)&basePtr->gaos;
				strncpy(ptr[AOWFS]->name, "gaos", 15);
				ptr[AOWFS]->time = 0.0;
				break;
			default:
				logMsg("wfs index > %d not recognised\n", (int)source, 0, 0, 0, 0, 0);
				return(ERROR);
			}
		}
	}

	return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * gensubToTcsTtf
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long	status;
 *
 * long    gensubToTcsTtf(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)	pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy data from the global ao data arrays to the VALJ port for reading by the TCS
 *
 * DESCRIPTION:
 * As this same function is called by all of the ttf gensubs the first action is to
 * identify which record is calling by comparison of the name fields leading to
 * identification of an array index number. Mutex access to the global ao data array
 * is then attempted and data is copied out to the VALJ port. Status return will be
 * bad if either the calling record name is not recognised or there is a timeout on
 * mutex access.
 *
 * EXTERNAL VARIABLES:
 * wfsLock[]       - Global array of mutex semaphores
 * ttfData[]       - Array of ttf data
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 28-Oct-1998  Original version                                Sean Prior
 * 13-Jan-1999: Write zernikes and errors to outputs for screen display (srp)
 *-
 */

long    gensubToTcsTtf (struct genSubRecord * pgsub)
{
	int wfsSource = 0;

	/* identify calling source for this routine */

	if(strstr(pgsub->name, "p1:ttf"))
		wfsSource = PWFS1;
	else if(strstr(pgsub->name, "p2:ttf"))
		wfsSource = PWFS2;
	else if(strstr(pgsub->name, "oi:ttf"))
		wfsSource = OIWFS;
	else if(strstr(pgsub->name, "hr:ttf"))
		wfsSource = HRWFS;
	else if(strstr(pgsub->name, "ao:ttf"))
		wfsSource = AOWFS;
	else
	{
		logMsg("ttf name > %s not recognised\n", (int)pgsub->name, 0, 0, 0, 0, 0);
		return(ERROR);
	}

	/* write array to TCS system */

	if(semTake(wfsLock[wfsSource], WFS_TIMEOUT) != OK)
	{
		logMsg("timeout on mutex access wfsLock[%d]\n", (int)wfsSource, 0, 0, 0, 0, 0);
		return(ERROR);
	}
	else
	{
		memcpy (pgsub->valj, ttfData[wfsSource], TTF_ARRAY_SIZE * sizeof (double));

		/* also write values to gensub outputs for screen display */

		*(double *)pgsub->vala = ttfData[wfsSource][8];	/* z2 */
		*(double *)pgsub->valb = ttfData[wfsSource][9];	/* z3 */
		*(double *)pgsub->valc = ttfData[wfsSource][10];/* z4 */
		*(double *)pgsub->vald = ttfData[wfsSource][5];	/* e2 */
		*(double *)pgsub->vale = ttfData[wfsSource][6];	/* e3 */
		*(double *)pgsub->valf = ttfData[wfsSource][7];	/* e4 */

		semGive(wfsLock[wfsSource]);
	}

	return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * gensubToTcsAo
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long	status;
 *
 * long    gensubToTcsAo(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)	pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy data from the global ao data arrays to the VALJ port for reading by the TCS
 *
 * DESCRIPTION:
 * As this same function is called by all of the ao gensubs the first action is to
 * identify which record is calling by comparison of the name fields leading to
 * identification of an array index number. Mutex access to the global ao data array
 * is then attempted and data is copied out to the VALJ port. Status return will be
 * bad if either the calling record name is not recognised or there is a timeout on
 * mutex access.
 *
 * EXTERNAL VARIABLES:
 * wfsLock[]       - Global array of mutex semaphores
 * ttfData[]       - Array of ttf data
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 28-Oct-1998  Original version                                Sean Prior
 * 23-Jan-1999	Write arrays of zernikes and errors to vala and valb
 *		to be picked up and displayed by other gensubs (srp)
 *-
 */

long    gensubToTcsAo (struct genSubRecord * pgsub)
{
	int wfsSource = 0, index = 0;
	double zernikes[19];
	double errors[19];

	/* identify calling source for this routine */

	if(strstr(pgsub->name, "p1:ao"))
		wfsSource = PWFS1;
	else if(strstr(pgsub->name, "p2:ao"))
		wfsSource = PWFS2;
	else if(strstr(pgsub->name, "oi:ao"))
		wfsSource = OIWFS;
	else if(strstr(pgsub->name, "hr:ao"))
		wfsSource = HRWFS;
	else if(strstr(pgsub->name, "ao:ao"))
		wfsSource = AOWFS;
	else
	{
		logMsg("ao name %s not recognised\n", (int)pgsub->name, 0, 0, 0, 0, 0);
		return(ERROR);
	}

	/* write array to TCS system */

	if(semTake(wfsLock[wfsSource], WFS_TIMEOUT) != OK)
	{
		logMsg("timeout on mutex access wfsLock[%d]\n", (int)wfsSource, 0, 0, 0, 0, 0);
		return(ERROR);
	}
	else
	{
		for(index = 0; index < 19; index++)
		{
			zernikes[index] = aoData[wfsSource][index+2];
			errors[index] = aoData[wfsSource][index+21];
		}

		/* write whole array to valj for the TCS to pick up */

		memcpy (pgsub->valj, aoData[wfsSource], AO_ARRAY_SIZE * sizeof (double));

		/* write Zernike values to vala for display */

		memcpy (pgsub->vala, zernikes, 19 * sizeof (double));

		/* write error values to valb for display */

		memcpy (pgsub->valb, errors, 19 * sizeof (double));

		semGive(wfsLock[wfsSource]);
	}

	return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * writeWfsToTcs
 *
 * INVOCATION:
 * struct OSP_CONST_STRUCT *pWfs
 * int wfsSource
 * long	STATUS;
 *
 * STATUS writeWfsToTcs(struct OSP_CONTEXT *pWfs, int wfsSource)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > struct OSP_CONST_STRUCT *pWfs     - pointer to structure of ao data
 * > int    wfsSource                  - index to identify wfs source
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Convert values to the TCS required frame of reference and write to global
 * arrays ready for the gensubs to pick up and transmit to the TCS.
 *
 * DESCRIPTION:
 * WFS index is first checked to be in range. If OK the zernikes are retrieved and
 * rotated where necessary to the TCS frame of reference.
 *
 * EXTERNAL VARIABLES:
 * wfsLock[]       - Global array of mutex semaphores
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * A pointer is supplied to the structure of ao data in the wfs processing section.
 * The contents of this structure is not protected by mutex so there is a requirement
 * placed on the calling task that the contents of the structure do not change for the
 * duration of this routine.
 *
 * HISTORY (optional):
 * 28-Oct-1998  Original version					(srp)
 *-11-Nov-1998	Add frame of reference conversion
 * 05-Jan-1999	Add null zernike calculation
 */

STATUS writeWfsToTcs(struct OSP_CONTEXT *pWfs)
{
	int	i=0;
	frame	*f;
	converted result;

	/* check wfs source in range */

	if(pWfs->wfsSource < HRWFS || pWfs->wfsSource > AOWFS)
	{
	    logMsg("wfsSource > %d out of range\n", (int)pWfs->wfsSource, 0, 0, 0, 0, 0);
	    return(ERROR);
	}

	/* check that array counts are within limits */

	if(pWfs->np > 19)
	{
	    logMsg("zernike count np = %d out of limits\n", (int)pWfs->np, 0, 0, 0, 0, 0);
	    return(ERROR);
	}

	f = ag2tcs[pWfs->wfsSource];

	/* access frame */

	if(semTake(f->access, WFS_TIMEOUT) == OK)
	{
		/* first rotate the tip and tilt values to the tcs frame of reference */

		result.z2 = (f->cosTheta*pWfs->z[1] - f->sinTheta*pWfs->z[2]) - f->null[5];
		result.z3 = (f->sinTheta*pWfs->z[1] + f->cosTheta*pWfs->z[2]) - f->null[6];
		result.z4 = pWfs->z[3] - f->null[7];
		result.z5 = (f->cosTheta*pWfs->z[4] - f->sinTheta*pWfs->z[5]) - f->null[8];
		result.z6 = (f->sinTheta*pWfs->z[4] + f->cosTheta*pWfs->z[5]) - f->null[9];
		result.z7 = (f->cosTheta*pWfs->z[6] - f->sinTheta*pWfs->z[7]) - f->null[10];
		result.z8 = (f->sinTheta*pWfs->z[6] + f->cosTheta*pWfs->z[7]) - f->null[11];
		result.z9 = pWfs->z[8] - f->null[12];
		result.z10 = (f->cosTheta*pWfs->z[9] - f->sinTheta*pWfs->z[10]) - f->null[13];
		result.z11 = (f->sinTheta*pWfs->z[9] + f->cosTheta*pWfs->z[10]) - f->null[14];
		result.z12 = (f->cosTheta*pWfs->z[11] - f->sinTheta*pWfs->z[12]) - f->null[15];
		result.z13 = (f->sinTheta*pWfs->z[11] + f->cosTheta*pWfs->z[12]) - f->null[16];
		result.z14 = (f->cosTheta*pWfs->z[13] - f->sinTheta*pWfs->z[14]) - f->null[17];
		result.z15 = (f->sinTheta*pWfs->z[13] + f->cosTheta*pWfs->z[14]) - f->null[18];
		result.z16 = pWfs->z[15] - f->null[19];
		result.z17 = (f->cosTheta*pWfs->z[16] - f->sinTheta*pWfs->z[17]) - f->null[20];
		result.z18 = (f->sinTheta*pWfs->z[16] + f->cosTheta*pWfs->z[17]) - f->null[21];
		result.z19 = (f->cosTheta*pWfs->z[18] - f->sinTheta*pWfs->z[19]) - f->null[22];
		result.z20 = (f->sinTheta*pWfs->z[18] + f->cosTheta*pWfs->z[19]) - f->null[23];

		semGive(f->access);
	}
	else
	{
		logMsg("writeWfsToTcs - unable to get mutex for conversion frame\n", 0, 0, 0, 0 ,0 ,0);
		return(ERROR);
	}

	/* take mutex semaphore to gain access to wfs arrays */

	if(semTake(wfsLock[pWfs->wfsSource], WFS_TIMEOUT) != OK)
	{
		logMsg("timeout on mutex access wfsLock[%d]\n", (int)pWfs->wfsSource, 0, 0, 0, 0, 0);
		return(ERROR);
	}
	else
	{
		/* fill ao data array */

		aoData[pWfs->wfsSource][0] = (double)pWfs->time;/* time */
		aoData[pWfs->wfsSource][1] = (double)pWfs->np;	/* number of coefficients */

		aoData[pWfs->wfsSource][2] = result.z2;
		aoData[pWfs->wfsSource][3] = result.z3;
		aoData[pWfs->wfsSource][4] = result.z4;
		aoData[pWfs->wfsSource][5] = result.z5;
		aoData[pWfs->wfsSource][6] = result.z6;
		aoData[pWfs->wfsSource][7] = result.z7;
		aoData[pWfs->wfsSource][8] = result.z8;
		aoData[pWfs->wfsSource][9] = result.z9;
		aoData[pWfs->wfsSource][10] = result.z10;
		aoData[pWfs->wfsSource][11] = result.z11;
		aoData[pWfs->wfsSource][12] = result.z12;
		aoData[pWfs->wfsSource][13] = result.z13;
		aoData[pWfs->wfsSource][14] = result.z14;
		aoData[pWfs->wfsSource][15] = result.z15;
		aoData[pWfs->wfsSource][16] = result.z16;
		aoData[pWfs->wfsSource][17] = result.z17;
		aoData[pWfs->wfsSource][18] = result.z18;
		aoData[pWfs->wfsSource][19] = result.z19;
		aoData[pWfs->wfsSource][20] = result.z20;

		/* copy across error terms */
		for(i=0; i < pWfs->np; i++)
		{
			aoData[pWfs->wfsSource][21+i] = (double)pWfs->err[i+1];
		}

		/* release mutex */

		semGive(wfsLock[pWfs->wfsSource]);
	}

	return(OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * writeWfsToSynchro
 *
 * INVOCATION:
 * struct OSP_CONST_STRUCT *pWfs
 * int wfsSource
 * long	STATUS;
 *
 * STATUS writeWfsToSynchro(struct OSP_CONTEXT *pWfs, int wfsSource)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > struct OSP_CONST_STRUCT *pWfs     - pointer to structure of ao data
 * > int    wfsSource                  - index to identify wfs source
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Write tilt and focus data to the synchro bus and make available to the TCS via
 * gensub records.
 *
 * DESCRIPTION:
 * If the wfs source corresponding to the calling pointer is in range the function
 * looks up the pointer for the appropriate section of reflective memory. The tip and tilt values
 * are rotated and scaled as appropriate before being written to the synchro bus.
 * In addition the rotated tip and tilt values are also low pass filtered and written
 * to an array ready for transmission to the TCS when required.
 *
 * EXTERNAL VARIABLES:
 * wfsLock[]       - Global array of mutex semaphores
 * ttfData[]       - Array of ttf data
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * A pointer is supplied to the structure of ao data in the wfs processing section.
 * The contents of this structure is not protected by mutex so there is a requirement
 * placed on the calling task that the contents of the structure do not change for the
 * duration of this routine.
 *
 * HISTORY (optional):
 * 28-Oct-1998  Original version					(srp)
 * 09-Nov-1998	Write fast tip/tilt to synchro bus			(srp)
 * 05-Jan-1999	Add null zernike calculation
 *-
 */

STATUS writeWfsToSynchro(struct OSP_CONTEXT *pWfs)
{
	converted	result;
	frame		*f;

	/* check wfs source in range */

	if(pWfs->wfsSource < HRWFS || pWfs->wfsSource > AOWFS)
	{
	    logMsg("wfsSource > %d out of range\n", (int)pWfs->wfsSource, 0, 0, 0, 0, 0);
	    return(ERROR);
	}

	f = ag2m2[pWfs->wfsSource];

	/* access frame */

	if(semTake(f->access, WFS_TIMEOUT) == OK)
	{
		/* first rotate the tip and tilt values to the m2 frame of reference */

		result.z2 = (f->cosTheta*pWfs->z[1] - f->sinTheta*pWfs->z[2]) - f->null[5];
		result.z3 = (f->sinTheta*pWfs->z[1] + f->cosTheta*pWfs->z[2]) - f->null[6];
		result.z4 = pWfs->z[3] - f->null[7];

		semGive(f->access);
	}
	else
	{
		logMsg("writeWfsToSynchro - unable to get mutex for conversion frame\n", 0, 0, 0, 0 ,0 ,0);
		return(ERROR);
	}

	/* scale data and write to the synchro bus, check that pointer has been initialised with null check */

	if(ptr[pWfs->wfsSource] != NULL)
	{
	  ptr[pWfs->wfsSource]->z1	= (float)(result.z2);
	  ptr[pWfs->wfsSource]->z2	= (float)(result.z3);
	  ptr[pWfs->wfsSource]->z3	= (float)(result.z4);
	  ptr[pWfs->wfsSource]->err1	= (float)(pWfs->err[1]);
	  ptr[pWfs->wfsSource]->err2	= (float)(pWfs->err[2]);
	  ptr[pWfs->wfsSource]->err3	= (float)(pWfs->err[3]);
	  ptr[pWfs->wfsSource]->interval  = (float)(0.0);

	  /* temporarily just increment the time parameter until bancomm access sorted */

	  ptr[pWfs->wfsSource]->time++;

	  /* raise interrupt on SCS */

	  rmIntSend (INT3, SCS_NODE);
	}

	/* filter the tilt values and make available to the TCS gensubs */

	if(semTake(wfsLock[pWfs->wfsSource], WFS_TIMEOUT) != OK)
	{
		logMsg("timeout on mutex access wfsLock[%d]\n", (int)pWfs->wfsSource, 0, 0, 0, 0, 0);
		return(ERROR);
	}
	else
	{
		ttfData[pWfs->wfsSource][0] = (double)pWfs->time;
		ttfData[pWfs->wfsSource][1] = (double)pWfs->np;
		ttfData[pWfs->wfsSource][2] = dfilter(result.z2, (3*pWfs->wfsSource + 0));
		ttfData[pWfs->wfsSource][3] = dfilter(result.z3, (3*pWfs->wfsSource + 1));
		ttfData[pWfs->wfsSource][4] = dfilter(result.z4, (3*pWfs->wfsSource + 2));
		ttfData[pWfs->wfsSource][5] = (double)pWfs->err[1];
		ttfData[pWfs->wfsSource][6] = (double)pWfs->err[2];
		ttfData[pWfs->wfsSource][7] = (double)pWfs->err[3];

		/* pop raw zernike values in for later display if desired */

		ttfData[pWfs->wfsSource][8] = (double)result.z2;
		ttfData[pWfs->wfsSource][9] = (double)result.z3;
		ttfData[pWfs->wfsSource][10] = (double)result.z4;

		/* release mutex */

		semGive(wfsLock[pWfs->wfsSource]);
	}

	return(OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * ttfZero
 * 
 * Purpose:
 * Receive ttfZero information from the TCS combined with probe angle
 *
 * Invocation:
 * long    ttfZero (struct genSubRecord * pgsub)
 *
 * Parameters in:
 *		> struct genSubRecord *pgsub pointer to gensub record
 *		> pgsub->a  string    guide probe angle
 *
 * Parameters out:
 * None
 * 
 * Return value:
 *		< status	int		OK or ERROR
 *
 * Globals: 
 *	External functions:
 *	None
 * 
 *	External variables:
 * 
 * Requirements:
 * 
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 20-Nov-1998: Original(srp)
 * 21-Jan-1999: Read in probe angle on input A, add to tcs angle
 * 22-Jan-1999: Problem workaround - if cannot connect to probeAngle record
 *		then set probeAngle to 0.0 but do not logMsg
 * 24-Jan-1999: read ports B and C for fudge factors - port B selects add or subtract
 *		of the Zeiss angle, port C provides an additional rotation angle
 *		rotationAngle = tcsAngle + (polarityFudge * (zeiss angle + rotationFudge))
 *
 */

/* INDENT ON */

/* ===================================================================== */

long    ttfZero (struct genSubRecord * pgsub)
{
	int     index = 0;
	int	wfsSource = 0;
	frame	*f;
	double	*ptr;
	double	probeAngle = 0.0, polarityFudge = 1.0, rotationFudge = 0.0, compositeAngle = 0.0;

	ptr = (double *) pgsub->j;

	/* identify calling source for this routine */

	if(strstr(pgsub->name, "p1"))
		wfsSource = PWFS1;
	else if(strstr(pgsub->name, "p2"))
		wfsSource = PWFS2;
	else if(strstr(pgsub->name, "oi"))
		wfsSource = OIWFS;
	else if(strstr(pgsub->name, "hr"))
		wfsSource = HRWFS;
	else if(strstr(pgsub->name, "ao"))
		wfsSource = AOWFS;
	else
	{
		logMsg("ttfZero name > %s not recognised\n", (int)pgsub->name, 0, 0, 0, 0, 0);
		return(ERROR);
	}

	/* read angle of guide probe */

	if(wfsSource == PWFS1 || wfsSource == PWFS2)
	{
		/* read conversion factors from input ports */

		if(sscanf(pgsub->a, "%lf", &probeAngle) != 1)
		{
			probeAngle = 0.0;
		}

		if(sscanf(pgsub->b, "%lf", &polarityFudge) != 1)
		{
			polarityFudge = 1.0;
		}

		if(sscanf(pgsub->c, "%lf", &rotationFudge) != 1)
		{
			rotationFudge = 0.0;
		}

		/* sanity check conversion factors */

		if(probeAngle < LOW_PROBE_ANGLE || probeAngle > HIGH_PROBE_ANGLE)
		{
			logMsg("ttfZero > %s probe angle out of range\n", (int)pgsub->name, 0, 0, 0, 0, 0);
			probeAngle = 0.0;
		}
	}

	f = ag2m2[wfsSource];

	/* access frame */

	if(semTake(f->access, WFS_TIMEOUT) == OK)
	{
		/* read in the array */

		for (index = 0; index < TTF_ZERO_ARRAY_SIZE; index++)
		{
		    f->null[index] = *(ptr++);
		}

		/* calculate composite correction angle */

		compositeAngle = (f->null[3]*DEGS2RADS) + (polarityFudge * ((probeAngle + rotationFudge)*DEGS2RADS));
		f->theta	= compositeAngle;
		f->sinTheta	= sin(f->theta);
		f->cosTheta	= cos(f->theta);

		semGive(f->access);
	}
	else
	{
		logMsg("Modify frame - unable to get mutex for conversion frame\n", 0, 0, 0, 0 ,0 ,0);
		return(ERROR);
	}

	/* write sample values to genSub ouputs */

	*(double *) pgsub->vala = f->null[0];			/* tSent */ 
	*(double *) pgsub->valb = f->null[1];			/* tAppl */
	*(double *) pgsub->valc = f->null[2];			/* trackId */
	*(double *) pgsub->vald = f->null[3];			/* tcsAngle (degrees) */
	*(double *) pgsub->vale = probeAngle;			/* probeAngle (degrees) */
	*(double *) pgsub->valf = compositeAngle/DEGS2RADS;	/* composite angle (degrees) */
	*(double *) pgsub->valg = f->null[5];			/* z2 */
	*(double *) pgsub->valh = f->null[6];			/* z3 */
	*(double *) pgsub->vali = f->null[7];			/* z4 */

	return (OK);
}

/* ===================================================================== */
/* INDENT OFF */
/*
 * Function name:
 * aoZero
 * 
 * Purpose:
 * Receive aoZero information from the TCS
 *
 * Invocation:
 * long    aoZero (struct genSubRecord * pgsub)
 *
 * Parameters in:
 *		> struct genSubRecord *pgsub pointer to gensub record
 *
 * Parameters out:
 * None
 * 
 * Return value:
 *		< status	int		OK or ERROR
 *
 * Globals: 
 *	External functions:
 *	None
 * 
 *	External variables:
 * 
 * Requirements:
 * 
 * 
 * Author:
 * Sean Prior  (srp@roe.ac.uk)
 * 
 * History:
 * 20-Nov-1998: Original(srp)
 * 21-Jan-1999: Read in probe angle on input A, add to tcs angle
 * 22-Jan-1999: Problem workaround - if cannot connect to probeAngle record
 *		then set probeAngle to 0.0 but do not logMsg
 * 24-Jan-1999: read ports B and C for fudge factors - port B selects add or subtract
 *		of the Zeiss angle, port C provides an additional rotation angle
 *		rotationAngle = tcsAngle + (polarityFudge * (zeiss angle + rotationFudge))
 *
 */

/* INDENT ON */

/* ===================================================================== */

long    aoZero (struct genSubRecord * pgsub)
{
	int     index = 0;
	int	wfsSource = 0;
	frame	*f;
	double	*ptr;
	double	probeAngle = 0.0, polarityFudge = 1.0, rotationFudge = 0.0, compositeAngle = 0.0;

	ptr = (double *) pgsub->j;

	/* identify calling source for this routine */

	if(strstr(pgsub->name, "p1"))
		wfsSource = PWFS1;
	else if(strstr(pgsub->name, "p2"))
		wfsSource = PWFS2;
	else if(strstr(pgsub->name, "oi"))
		wfsSource = OIWFS;
	else if(strstr(pgsub->name, "hr"))
		wfsSource = HRWFS;
	else if(strstr(pgsub->name, "ao"))
		wfsSource = AOWFS;
	else
	{
		logMsg("aoZero name > %s not recognised\n", (int)pgsub->name, 0, 0, 0, 0, 0);
		return(ERROR);
	}

	/* read angle of guide probe */

	if(wfsSource == PWFS1 || wfsSource == PWFS2)
	{
		/* read conversion factors from input ports */

		if(sscanf(pgsub->a, "%lf", &probeAngle) != 1)
		{
			probeAngle = 0.0;
		}

		if(sscanf(pgsub->b, "%lf", &polarityFudge) != 1)
		{
			polarityFudge = 1.0;
		}

		if(sscanf(pgsub->c, "%lf", &rotationFudge) != 1)
		{
			rotationFudge = 0.0;
		}

		/* sanity check conversion factors */

		if(probeAngle < LOW_PROBE_ANGLE || probeAngle > HIGH_PROBE_ANGLE)
		{
			logMsg("aoZero > %s probe angle out of range\n", (int)pgsub->name, 0, 0, 0, 0, 0);
			probeAngle = 0.0;
		}
	}

	f = ag2tcs[wfsSource];

	/* access frame */

	if(semTake(f->access, WFS_TIMEOUT) == OK)
	{
		/* read in the array */

		for (index = 0; index < AO_ZERO_ARRAY_SIZE; index++)
		{
		    f->null[index] = *(ptr++);
		}

		/* calculate composite correction angle */

		compositeAngle = (f->null[3]*DEGS2RADS) + (polarityFudge * ((probeAngle + rotationFudge)*DEGS2RADS));
		f->theta	= compositeAngle;
		f->sinTheta	= sin(f->theta);
		f->cosTheta	= cos(f->theta);

		semGive(f->access);
	}
	else
	{
		logMsg("Modify frame - unable to get mutex for conversion frame\n", 0, 0, 0, 0 ,0 ,0);
		return(ERROR);
	}

	/* write sample values to genSub ouputs */

	*(double *) pgsub->vala = f->null[0];			/* tSent */ 
	*(double *) pgsub->valb = f->null[1];			/* tAppl */
	*(double *) pgsub->valc = f->null[2];			/* trackId */
	*(double *) pgsub->vald = f->null[3];			/* tcsAngle (degrees) */
	*(double *) pgsub->vale = probeAngle;			/* probeAngle (degrees) */
	*(double *) pgsub->valf = compositeAngle/DEGS2RADS;	/* composite angle (degrees) */
	*(double *) pgsub->valg = f->null[5];			/* z2 */
	*(double *) pgsub->valh = f->null[6];			/* z3 */
	*(double *) pgsub->vali = f->null[7];			/* z4 */

	return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showAoDiags
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long	status;
 *
 * long    showAoDiags(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)	pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from ao osp structure to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 12-Jan-1999  Original version	Sean Prior
 *-
 */

STATUS showAoDiags(struct genSubRecord * pgsub)
{
	int i = 0;
	int wfsSource = 0;
	double localDiag[DIAG_ARRAY_SIZE];
	static int discardCount[MAX_WFS_SOURCES];

	/* identify calling source for this routine */

	if(strstr(pgsub->name, "p1:"))
		wfsSource = PWFS1;
	else if(strstr(pgsub->name, "p2:"))
		wfsSource = PWFS2;
	else if(strstr(pgsub->name, "oi:"))
		wfsSource = OIWFS;
	else if(strstr(pgsub->name, "hr:"))
		wfsSource = HRWFS;
	else if(strstr(pgsub->name, "ao:"))
		wfsSource = AOWFS;
	else
	{
		logMsg("showDiags name > %s not recognised\n", (int)pgsub->name, 0, 0, 0, 0, 0);
		return(ERROR);
	}

	if(wfsAoAddr[wfsSource] == NULL)
	{
	    /* context structure not yet initialised */
	    return(OK);
	}

	/* grab data from osp structure */

	for(i = 0; i < DIAG_ARRAY_SIZE; i++)
	{
		localDiag[i] = (double)wfsAoAddr[wfsSource]->ospdiag[i];
	}

	/* check whether data has been updated during read */

	if( (fabs(localDiag[GUARD1] - localDiag[GUARD2])) > DBL_EPSILON)
	{
	    /* printf("guard1 = %f, guard2 = %f\n", localDiag[GUARD1], localDiag[GUARD2]); */

		/* array has been written by another process during read - discard */

		if(++discardCount[wfsSource] > DISCARD_THRESHOLD)
		{
			logMsg("showAoDiags - source %d exceeded discard count\n", (int)wfsSource, 0, 0, 0, 0, 0);
			discardCount[wfsSource] = 0;
		}
	}
	else
	{
		/* data intact, write to genSub outputs */

		*(double *)pgsub->vala = localDiag[1];
		*(double *)pgsub->valb = localDiag[2];
		*(double *)pgsub->valc = localDiag[3];
		*(double *)pgsub->vald = localDiag[4];
		*(double *)pgsub->vale = localDiag[5];
		*(double *)pgsub->valf = localDiag[6];
		*(double *)pgsub->valg = localDiag[7];
		*(double *)pgsub->valh = localDiag[8];
		*(double *)pgsub->vali = localDiag[9];
		*(double *)pgsub->valj = localDiag[10];
	}	

	return (OK);
}

/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showFgDiags
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long	status;
 *
 * long    showFgDiags(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)	pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Copy diagnostic data from fg osp structure to gensub outputs for display
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 12-Jan-1999  Original version	Sean Prior
 *-
 */

STATUS showFgDiags(struct genSubRecord * pgsub)
{
	int i = 0;
	int wfsSource = 0;
	double localDiag[DIAG_ARRAY_SIZE];
	static int discardCount[MAX_WFS_SOURCES];

	/* identify calling source for this routine */

	if(strstr(pgsub->name, "p1:"))
		wfsSource = PWFS1;
	else if(strstr(pgsub->name, "p2:"))
		wfsSource = PWFS2;
	else if(strstr(pgsub->name, "oi:"))
		wfsSource = OIWFS;
	else if(strstr(pgsub->name, "hr:"))
		wfsSource = HRWFS;
	else if(strstr(pgsub->name, "ao:"))
		wfsSource = AOWFS;
	else
	{
		logMsg("showDiags name > %s not recognised\n", (int)pgsub->name, 0, 0, 0, 0, 0);
		return(ERROR);
	}

	if(wfsFgAddr[wfsSource] == NULL)
	{
	    /* context structure not yet initialised */
	    return(OK);
	}

	/* grab data from osp structure */

	/*printf("diag source %d, wfsFgAddr = %p\n", wfsSource, wfsFgAddr[wfsSource]);*/

	for(i = 0; i < DIAG_ARRAY_SIZE; i++)
	{
		localDiag[i] = (double)wfsFgAddr[wfsSource]->ospdiag[i];
		/*printf("localDiag[%d] = %f, original[%d] = %f\n", i, localDiag[i], i, wfsFgAddr[wfsSource]->ospdiag[i]); */
	}

	/* check whether data has been updated during read */

	if(localDiag[GUARD1] != localDiag[GUARD2])
	{
		/* array has been written by another process during read - discard */

		if(++discardCount[wfsSource] > DISCARD_THRESHOLD)
		{
			logMsg("showFgDiags - source %d exceeded discard count\n", (int)wfsSource, 0, 0, 0, 0, 0);
			discardCount[wfsSource] = 0;
		}
	}
	else
	{
		/* data intact, write to genSub outputs */

		*(double *)pgsub->vala = localDiag[1];
		*(double *)pgsub->valb = localDiag[2];
		*(double *)pgsub->valc = localDiag[3];
		*(double *)pgsub->vald = localDiag[4];
		*(double *)pgsub->vale = localDiag[5];
		*(double *)pgsub->valf = localDiag[6];
		*(double *)pgsub->valg = localDiag[7];
		*(double *)pgsub->valh = localDiag[8];
		*(double *)pgsub->vali = localDiag[9];
		*(double *)pgsub->valj = localDiag[10];
	}	

	return (OK);
}


/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * gensubFanDoubles
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long	status;
 *
 * long    gensubFanDoubles(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)	pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Receive array of double on port A, copy individual elements to output
 * ports. Primarily intended for display to dm screens.
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 23-Jan-1999  Original version	Sean Prior
 *-
 */

long    gensubFanDoubles (struct genSubRecord * pgsub)
{
	int index = 0;
	double	  localArray[19];
	double	  *ptr = NULL;

	ptr = (double *) pgsub->a;

	/* read in the array from port A */

	for (index = 0; index < 19; index++)
	{
	    localArray[index] = *(ptr++);
	}

	/* write values to gensub outputs for screen display */

	*(double *)pgsub->vala = localArray[0];	     /* Z2 or E2 */
	*(double *)pgsub->valb = localArray[1];
	*(double *)pgsub->valc = localArray[2];
	*(double *)pgsub->vald = localArray[3];
	*(double *)pgsub->vale = localArray[4];
	*(double *)pgsub->valf = localArray[5];
	*(double *)pgsub->valg = localArray[6];
	*(double *)pgsub->valh = localArray[7];
	*(double *)pgsub->vali = localArray[8];
	*(double *)pgsub->valj = localArray[9];
	*(double *)pgsub->valk = localArray[10];
	*(double *)pgsub->vall = localArray[11];
	*(double *)pgsub->valm = localArray[12];
	*(double *)pgsub->valn = localArray[13];
	*(double *)pgsub->valo = localArray[14];
	*(double *)pgsub->valp = localArray[15];
	*(double *)pgsub->valq = localArray[16];
	*(double *)pgsub->valr = localArray[17];
	*(double *)pgsub->vals = localArray[18];	/* Z20 or E20 */

	return (OK);
}


/* ===================================================================== */
/*
 *+
 * FUNCTION NAME:
 * showSdsuTemperature
 *
 * INVOCATION:
 * struct genSubRecord * pgsub
 * long	status;
 *
 * long    showSdsuTemperature(struct genSubRecord * pgsub)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > genSubRecord (struct genSubRecord *)	pointer to record
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Read temperatures from the sdsu structures and display to ouput ports
 *
 * DESCRIPTION:
 *
 * EXTERNAL VARIABLES:
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known.
 *
 * HISTORY (optional):
 * 26-Jan-1999  Original version	Sean Prior
 *-
 */

STATUS showSdsuTemperature(struct genSubRecord * pgsub)
{
	int wfsSource = 0;
	SDSU_ID id = 0;
	int i = 0;
	int t = 0;
	float temp = 0.0;
	uint32 data;
	int readError = FALSE;

	/* identify calling source for this routine */

	if(strstr(pgsub->name, "p1:"))
		wfsSource = PWFS1;
	else if(strstr(pgsub->name, "p2:"))
		wfsSource = PWFS2;
	else if(strstr(pgsub->name, "oi:"))
		wfsSource = OIWFS;
	else if(strstr(pgsub->name, "hr:"))
		wfsSource = HRWFS;
	else if(strstr(pgsub->name, "ao:"))
		wfsSource = AOWFS;
	else
	{
		logMsg("showSdsuTemperature name > %s not recognised\n", (int)pgsub->name, 0, 0, 0, 0, 0);
		return(ERROR);
	}

	id = sdsuId[wfsSource];

	if(id == 0)
		return(ERROR);

	/* read temperature 0 */

	if(sdsuParamRead(id, 3, "U_ADC0", &data) == ERROR)
	{
	    strncpy((char *)pgsub->vala, "READ ERROR", 15);
	}
	else
	{
	    temp = 25.0 + 0.38*((float)t - 2789.0);
	    sprintf ((char *)pgsub->vala, "%4.2f", temp);
	}

	/* read temperature 1 */

	readError = FALSE;
	t = 0;
	for(i = 0; i < 20; i++)
	{
	    if(sdsuParamRead(id, 3, "U_ADC6", &data) == ERROR)
	    {
		readError = TRUE;
                break;
	    }
	    else
	    {
	        t = t+data;
	    }
	}

	if(readError == FALSE)
	{
	    temp = ((float)t / 20.0) * (-0.01545);
	    sprintf ((char *)pgsub->valb, "%4.2f", temp);
	}
	else
	{
	    strncpy((char *)pgsub->valb, "READ ERROR", 15);
	}

	/* read temperature 2 */

	readError = FALSE;
	t = 0;
	for(i = 0; i < 20; i++)
	{
	  if(sdsuParamRead(id, 3, "U_ADC7", &data) == ERROR)
	  {
		readError = TRUE;
		break;
	  }
	  else
	  {
	      t = t + data;
	  }
	}

	if(readError == FALSE)
	{
            temp = ((float)t / 20.0) * (-0.01545);
	    sprintf ((char *)pgsub->valc, "%4.2f", temp);
	}
	else
	{
	    strncpy((char *)pgsub->valc, "READ ERROR", 15);
	}

	/* read target temperature */

	if(sdsuParamRead(id, 3, "U_CCDT_TGT", &data) == ERROR)
	{
	    strncpy((char *)pgsub->vald, "READ ERROR", 15);
	}
	else
	{
	    temp = ((float)data - 2048) * (5.0/2048.0);
	    sprintf ((char *)pgsub->vald, "%.4f", temp);
	}

	/* read TEC voltage */

	if(sdsuParamRead(id, 3, "U_DAC2", &data) == ERROR)
	{
	    strncpy((char *)pgsub->vale, "READ ERROR", 15);
	}
	else
	{
	    temp = ((float)data - 2048) * (5.0/2048.0);
	    sprintf ((char *)pgsub->vale, "%4.2f", temp);
	}

	return (OK);
}






















