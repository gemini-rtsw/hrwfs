/* Id:$ */
/* ===================================================================== */
/* INDENT OFF */
/*+
 *
 * FILENAME
 * -------- 
 * testKit.c
 * 
 * PURPOSE
 * -------
 * Provide dummy source of wfs data to exercise the TCS gensubs. Also
 * provide dummy TCS receiving gensubs for ao data
 * 
 * FUNCTION NAME(S)
 * ----------------
 * 
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
 * 29-Oct-1998: array index from wfs sources run 1 -> np NOT 0 -> (np-1)
 */
/* INDENT ON */
/* ===================================================================== */

/* specify constant definitions */

#ifndef MAX_WFS_SOURCES
#define MAX_WFS_SOURCES	5
#define	TTF_ARRAY_SIZE	8
#define AO_ARRAY_SIZE	40
#define AO_ZERO_ARRAY_SIZE 24
#endif

/* specify include files */

#include <vxWorks.h>
#include <taskLib.h>
#include <semLib.h>
#include <stdioLib.h>
#include <dbDefs.h>
#include <wdLib.h>
#include <msgQLib.h>
#include <subRecord.h>
#include <cadRecord.h>
#include <recSup.h>
#include <dbCommon.h>
#include <genSubRecord.h>
#include <timeLib.h>
#include <time.h>

#include "osp.h"
#include "synchroMap.h"

typedef struct
{
	double  probeAngle;	/* angle of guide probe supplied by Zeiss */
	double  tcsAngle;	/* rotation angle supplied by TCS */
	double	theta;
	double	sinTheta;
	double	cosTheta;
	double	null[AO_ZERO_ARRAY_SIZE];
	SEM_ID	access;
}frame;

/* declare global variables */

extern	double	ttfData[MAX_WFS_SOURCES][AO_ARRAY_SIZE+2];
extern	double	aoData[MAX_WFS_SOURCES][AO_ARRAY_SIZE+2];
extern	float	data[MAX_WFS_SOURCES][AO_ARRAY_SIZE+2];
extern	float	errors[MAX_WFS_SOURCES][AO_ARRAY_SIZE+2];
extern  frame	*ag2m2[MAX_WFS_SOURCES];
extern  frame	*ag2tcs[MAX_WFS_SOURCES];

struct	OSP_CONTEXT *testp1, *testp2, *testp3;

/* declare prototypes */

STATUS writeWfsToTcs(struct OSP_CONTEXT *pWfs);
STATUS writeWfsToSynchro(struct OSP_CONTEXT *pWfs);

char *ttfName[] =
{
    "Time         ",
    "number (np)  ",
    "z2           ",
    "z3           ",
    "z4           ",
    "e2           ",
    "e3           ",
    "e4           ",
    NULL
};

char *aoName[] =
{
    "Time         ",
    "number (np)  ",
    "z2           ",
    "z3           ",
    "z4           ",
    "z5           ",
    "z6           ",
    "z7           ",
    "z8           ",
    "z9           ",
    "z10          ",
    "z11          ",
    "z12          ",
    "z13          ",
    "z14          ",
    "z15          ",
    "z16          ",
    "z17          ",
    "z18          ",
    "z19          ",
    "z20          ",
    "e2           ",
    "e3           ",
    "e4           ",
    "e5           ",
    "e6           ",
    "e7           ",
    "e8           ",
    "e9           ",
    "e10          ",
    "e11          ",
    "e12          ",
    "e13          ",
    "e14          ",
    "e15          ",
    "e16          ",
    "e17          ",
    "e18          ",
    "e19          ",
    "e20          ",
    NULL
};

/* ===================================================================== */

long    ttfReceiver (struct genSubRecord * pgsub)
{
	int     index = 0;
	double	rxData[TTF_ARRAY_SIZE];
	double *ptr;

	ptr = (double *) pgsub->j;

	/* read in the array */

	for (index = 0; index < TTF_ARRAY_SIZE; index++)
	{
	    rxData[index] = *(ptr++);
	}

	/* write sample values to genSub ouputs */

	*(double *) pgsub->vala = rxData[0];	/* TAI time */ 
	*(double *) pgsub->valb = rxData[1];	/* Number of coefficients */ 
	*(double *) pgsub->valc = rxData[2];	/* z2 */
	*(double *) pgsub->vald = rxData[3];	/* z3 */
	*(double *) pgsub->vale = rxData[4];	/* z4 */
	*(double *) pgsub->valf = rxData[5];	/* e2 */
	*(double *) pgsub->valg = rxData[6];	/* e3 */
	*(double *) pgsub->valh = rxData[7];	/* e4 */

	return (OK);
}

/* ===================================================================== */

long    aoReceiver (struct genSubRecord * pgsub)
{
	int     index = 0;
	double	rxData[AO_ARRAY_SIZE];
	double *ptr;

	ptr = (double *) pgsub->j;

	/* read in the array */

	for (index = 0; index < AO_ARRAY_SIZE; index++)
	{
	    rxData[index] = *(ptr++);
	}

	/* write sample values to genSub ouputs */

	*(double *) pgsub->vala = rxData[0];	/* TAI time */ 
	*(double *) pgsub->valb = rxData[1];	/* Number of coefficients */ 
	*(double *) pgsub->valc = rxData[2];	/* z2 */
	*(double *) pgsub->vald = rxData[3];	/* z3 */
	*(double *) pgsub->vale = rxData[4];	/* z4 */
	*(double *) pgsub->valf = rxData[5];	/* e2 */
	*(double *) pgsub->valg = rxData[6];	/* e3 */
	*(double *) pgsub->valh = rxData[7];	/* e4 */

	return (OK);
}

/* ===================================================================== */

struct OSP_CONTEXT * createWfsSource (int wfsSource)
{
	struct	OSP_CONTEXT *ptest;
	int i = 0;

	if(wfsSource > (MAX_WFS_SOURCES - 1))
	{
		printf("wfsSource %d out of range\n", wfsSource);
		return(NULL);
	}

	ptest = (struct OSP_CONTEXT *) malloc (sizeof (struct OSP_CONTEXT));

	if(ptest == NULL)
	{
		printf("malloc failure\n");
		return(NULL);
	}

	ptest->z = (float *)data[wfsSource];
	ptest->err = (float *)errors[wfsSource];

	ptest->time = 666.0;
	ptest->np = 19;

	for(i = 1; i <= ptest->np; i++)
	{
		ptest->z[i] = 199.0;
		ptest->err[i] = 299.00;
	}

	ptest->wfsSource = wfsSource;

	/* trace message */

	printf("in createWfsSource ptest = %p, wfsSource = %d\n", ptest, ptest->wfsSource);

	return(ptest);
}

/* ===================================================================== */

STATUS fillWfsSource (struct OSP_CONTEXT *ptest, double value)
{
	int i = 0;

	if(ptest == NULL)
	{
		printf("passed pointer is null\n");
		return(ERROR);
	}

	ptest->time = value;
	ptest->np = 19;

	for(i = 1; i <= 19; i++)
	{
		ptest->z[i] = (float)(value+i-1);
		ptest->err[i] = (float)(value+i + 18) ;
	}

	return(OK);
}

/* ===================================================================== */

STATUS wfsDummy(void)
{
	/* pretend to be steven's wfs processing */
	
	if(testp1 == NULL)
		testp1 = createWfsSource(PWFS1);

	if(testp2 == NULL)
		testp2 = createWfsSource(PWFS2);

	if(testp3 == NULL)
		testp3 = createWfsSource(OIWFS);

	/* at each pass increment the data values */

	fillWfsSource(testp1, 1.0);
	fillWfsSource(testp2, 10.0);
	fillWfsSource(testp3, 100.0);

	/* write updated data to Tcs */

	writeWfsToTcs(testp1);
	writeWfsToTcs(testp2);
	writeWfsToTcs(testp3);

	/* write updated data to Synchro */

	writeWfsToSynchro(testp1);
	writeWfsToSynchro(testp2);
	writeWfsToSynchro(testp3);

	return(OK);
}
/* ===================================================================== */

void showTtfNull(int source)
{
	if(source >= MAX_WFS_SOURCES)
	{
		printf("wfsNumber %d out of range\n", source);
		return;
	}

	printf("wfs %d\n\n", source);
	printf("theta    = %f (rads)\n", ag2m2[source]->theta);
	printf("sinTheta = %f\n", ag2m2[source]->sinTheta);
	printf("cosTheta = %f\n", ag2m2[source]->cosTheta);
	printf("null z2  = %f\n", ag2m2[source]->null[5]);
	printf("null z3  = %f\n", ag2m2[source]->null[6]);
	printf("null z4  = %f\n", ag2m2[source]->null[7]);
}

void showAoNull(int source)
{
    int index = 0;

	if(source >= MAX_WFS_SOURCES)
	{
		printf("wfsNumber %d out of range\n", source);
		return;
	}

	printf("wfs %d\n\n", source);
	printf("theta    = %f (rads)\n", ag2tcs[source]->theta);
	printf("sinTheta = %f\n", ag2tcs[source]->sinTheta);
	printf("cosTheta = %f\n", ag2tcs[source]->cosTheta);

    for(index = 0; index < 19; index++)
    {
	printf("null z%d  = %f\n", (index+2), ag2tcs[source]->null[5 + index]);
    }
}

/* ===================================================================== */

void showTtf(int wfsNumber)
{
	int index = 0;

	if(wfsNumber >= MAX_WFS_SOURCES)
	{
		printf("wfsNumber %d out of range\n", wfsNumber);
		return;
	}
	printf("wfs %d\n\n", wfsNumber);

	for(index = 0; index < TTF_ARRAY_SIZE; index++)
	{
		printf("%s\t%f\n", ttfName[index], ttfData[wfsNumber][index]);
	}
}

/* ===================================================================== */

void showAo(int wfsNumber)
{
	int index = 0;

	if(wfsNumber >= MAX_WFS_SOURCES)
	{
		printf("wfsNumber %d out of range\n", wfsNumber);
		return;
	}

	printf("wfs %d\n\n", wfsNumber);

	for(index = 0; index < AO_ARRAY_SIZE; index++)
	{
		printf("%s\t%f\n", aoName[index], aoData[wfsNumber][index]);
	}
}

/* ===================================================================== */

void    showSynchro (const memMap * buffPtr)
{
	/* printout the memory locations of the specified buffer area */

	printf ("\nPage 8  - pwfs1 data\n");
	printf ("pwfs1		Addr = %p,           \n", &buffPtr->pwfs1.z1);

	printf ("z1		Addr = %p, Value = %f\n", &buffPtr->pwfs1.z1, buffPtr->pwfs1.z1);
	printf ("z2		Addr = %p, Value = %f\n", &buffPtr->pwfs1.z2, buffPtr->pwfs1.z2);
	printf ("z3		Addr = %p, Value = %f\n", &buffPtr->pwfs1.z3, buffPtr->pwfs1.z3);
	printf ("err1		Addr = %p, Value = %f\n", &buffPtr->pwfs1.err1, buffPtr->pwfs1.err1);
	printf ("err2		Addr = %p, Value = %f\n", &buffPtr->pwfs1.err2, buffPtr->pwfs1.err2);
	printf ("err3		Addr = %p, Value = %f\n", &buffPtr->pwfs1.err3, buffPtr->pwfs1.err3);
	printf ("interval	Addr = %p, Value = %f\n", &buffPtr->pwfs1.interval, buffPtr->pwfs1.interval);
	printf ("time		Addr = %p, Value = %f\n", &buffPtr->pwfs1.time, buffPtr->pwfs1.time);
	printf ("name		Addr = %p, Value = %s\n",  buffPtr->pwfs1.name, buffPtr->pwfs1.name);

	printf ("\nPage 9  - pwfs2 data\n");
	printf ("pwfs2		Addr = %p,           \n", &buffPtr->pwfs2.z1);
	printf ("z1		Addr = %p, Value = %f\n", &buffPtr->pwfs2.z1, buffPtr->pwfs2.z1);
	printf ("z2		Addr = %p, Value = %f\n", &buffPtr->pwfs2.z2, buffPtr->pwfs2.z2);
	printf ("z3		Addr = %p, Value = %f\n", &buffPtr->pwfs2.z3, buffPtr->pwfs2.z3);
	printf ("err1		Addr = %p, Value = %f\n", &buffPtr->pwfs2.err1, buffPtr->pwfs2.err1);
	printf ("err2		Addr = %p, Value = %f\n", &buffPtr->pwfs2.err2, buffPtr->pwfs2.err2);
	printf ("err3		Addr = %p, Value = %f\n", &buffPtr->pwfs2.err3, buffPtr->pwfs2.err3);
	printf ("interval	Addr = %p, Value = %f\n", &buffPtr->pwfs2.interval, buffPtr->pwfs2.interval);
	printf ("time		Addr = %p, Value = %f\n", &buffPtr->pwfs2.time, buffPtr->pwfs2.time);
	printf ("name		Addr = %p, Value = %s\n",  buffPtr->pwfs2.name, buffPtr->pwfs2.name);

	printf ("\nPage 10 - oiwfs data\n");
	printf ("oiwfs		Addr = %p,           \n", &buffPtr->oiwfs.z1);
	printf ("z1		Addr = %p, Value = %f\n", &buffPtr->oiwfs.z1, buffPtr->oiwfs.z1);
	printf ("z2		Addr = %p, Value = %f\n", &buffPtr->oiwfs.z2, buffPtr->oiwfs.z2);
	printf ("z3		Addr = %p, Value = %f\n", &buffPtr->oiwfs.z3, buffPtr->oiwfs.z3);
	printf ("err1		Addr = %p, Value = %f\n", &buffPtr->oiwfs.err1, buffPtr->oiwfs.err1);
	printf ("err2		Addr = %p, Value = %f\n", &buffPtr->oiwfs.err2, buffPtr->oiwfs.err2);
	printf ("err3		Addr = %p, Value = %f\n", &buffPtr->oiwfs.err3, buffPtr->oiwfs.err3);
	printf ("interval	Addr = %p, Value = %f\n", &buffPtr->oiwfs.interval, buffPtr->oiwfs.interval);
	printf ("time		Addr = %p, Value = %f\n", &buffPtr->oiwfs.time, buffPtr->oiwfs.time);
	printf ("name		Addr = %p, Value = %s\n",  buffPtr->oiwfs.name, buffPtr->oiwfs.name);

	return;

	printf ("\nPage 11 - gaos data\n");
	printf ("gaos		Addr = %p,           \n", &buffPtr->gaos.z1);
	printf ("z1		Addr = %p, Value = %f\n", &buffPtr->gaos.z1, buffPtr->gaos.z1);
	printf ("z2		Addr = %p, Value = %f\n", &buffPtr->gaos.z2, buffPtr->gaos.z2);
	printf ("z3		Addr = %p, Value = %f\n", &buffPtr->gaos.z3, buffPtr->gaos.z3);
	printf ("err1		Addr = %p, Value = %f\n", &buffPtr->gaos.err1, buffPtr->gaos.err1);
	printf ("err2		Addr = %p, Value = %f\n", &buffPtr->gaos.err2, buffPtr->gaos.err2);
	printf ("err3		Addr = %p, Value = %f\n", &buffPtr->gaos.err3, buffPtr->gaos.err3);
	printf ("interval	Addr = %p, Value = %f\n", &buffPtr->gaos.interval, buffPtr->gaos.interval);
	printf ("time		Addr = %p, Value = %f\n", &buffPtr->gaos.time, buffPtr->gaos.time);
	printf ("name		Addr = %p, Value = %s\n",  buffPtr->gaos.name, buffPtr->gaos.name);

	printf ("\nPage 12 - gyro data\n");
	printf ("gyro		Addr = %p,           \n", &buffPtr->gyro.z1);
	printf ("z1		Addr = %p, Value = %f\n", &buffPtr->gyro.z1, buffPtr->gyro.z1);
	printf ("z2		Addr = %p, Value = %f\n", &buffPtr->gyro.z2, buffPtr->gyro.z2);
	printf ("z3		Addr = %p, Value = %f\n", &buffPtr->gyro.z3, buffPtr->gyro.z3);
	printf ("err1		Addr = %p, Value = %f\n", &buffPtr->gyro.err1, buffPtr->gyro.err1);
	printf ("err2		Addr = %p, Value = %f\n", &buffPtr->gyro.err2, buffPtr->gyro.err2);
	printf ("err3		Addr = %p, Value = %f\n", &buffPtr->gyro.err3, buffPtr->gyro.err3);
	printf ("interval	Addr = %p, Value = %f\n", &buffPtr->gyro.interval, buffPtr->gyro.interval);
	printf ("time		Addr = %p, Value = %f\n", &buffPtr->gyro.time, buffPtr->gyro.time);
	printf ("name		Addr = %p, Value = %s\n",  buffPtr->gyro.name, buffPtr->gyro.name);

}

/* ===================================================================== */

void    showTime (void)
{
	int     j, c[7];

	j = timeNowC (TAI, 3, c);
	printf ("status = %d time > %d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%3.3d (TAI)\n", j, c[0], c[1], c[2], c[3], c[4], c[5], c[6]);
}

/* ===================================================================== */

void    rawTime (void)
{
	int     j;
	double  rawt = 99;

	j = timeNow (&rawt);

	printf ("status = %d rawtime > %f\n", j, rawt);
}





























