#ifdef vxWorks
#include <vxWorks.h>
#include <sysLib.h>
#endif /* vxWorks */

#include <string.h>        
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* Bancom purposes */
#include "timeLib.h"

#define NRANSI
#include "fitsio.h"
#include "osp.h"

struct OSP_CONTEXT *wfsAoAddr[MAX_WFS_SOURCES];
struct OSP_CONTEXT *wfsFgAddr[MAX_WFS_SOURCES];

#ifdef vxWorks
int do_it();
int do_it()
#else
int main(int argc, char * argv[])
#endif /*vxWorks*/ 
{
    float buffer[OSP_BUFFMAX];
    float redbuffer[OSP_BUFFMAX];
    struct OSP_CONTEXT * pwfsao;
    struct OSP_GEOMETRY * ospGeom;
    int i;
    clock_t t1,t2;
    int iterations;
    float t_coadd, t_measure, t_fgmeasure;

/*
 * initialise the wfs context structures for aO and fast guide
 * - here we use the same .ini file and simply reduce the number
 * of Zernikes to be calculated for the fast guide
 */
    pwfsao = ospInit("data/pwfs.ini",NULL);

/* 
 * populate, transform and invert a reconstructor matrix using
 * calibration frames to obtain the control matrix
 */

    ospNullCorrection(pwfsao);
    ospNewCalibrate("../data/",pwfsao);
    printf ( "End of ospNewCalibrate\n\n" ) ;

/* 
 * change geometry to a reduced frame- use ospChangeGeometry
 * with an OSP_GEOMETRY structure supplied, differing only in that
 * the framesizeflag is 0
 *
 * create and populate an OSP_GEOMETRY structure 
 */

    /*ospGeom=(struct OSP_GEOMETRY *) malloc (sizeof (struct OSP_GEOMETRY));
    ospGeom->sectors= pwfsao->sectors;
    ospGeom->xstart = pwfsao->ospxstart;
    ospGeom->ystart = pwfsao->ospystart;
    ospGeom->xbin = pwfsao->ospxbin ;
    ospGeom->ybin = pwfsao->ospybin ;
    ospGeom->xraster = pwfsao->ospxraster ;
    ospGeom->yraster = pwfsao->ospyraster ;
    ospGeom->xspace = pwfsao->ospxspace ;
    ospGeom->yspace = pwfsao->ospyspace ;
    ospGeom->xsubap = pwfsao->ospxsubap ;
    ospGeom->ysubap = pwfsao->ospysubap ;
    ospGeom->xarraysize = pwfsao->xarraysize ;
    ospGeom->yarraysize = pwfsao->yarraysize ;
    ospGeom->framesizeflag = 0 ;

    printf("ospGeom %p\n",ospGeom);

    ospChangeGeometry(ospGeom,pwfsao);

    ospReadFloatImage(buffer,"./data/z1_perf5.fits",6400);

    if(pwfsao->framesizeflag==0)
	ospReduceFrame(buffer, redbuffer, pwfsao,6400);

    pwfsao->thresh = 0;
    ospChangeThreshold(200,pwfsao);

    printf("ospFGMeasure\n");
    ospFGMeasure(redbuffer,pwfsao);
    ospShow(pwfsao);*/
/*    

    printf("now timexN(ospFGMeasure, %p, %p)\n",redbuffer,pwfsao);
    printf("now timexN(ospFrameScramble, %d, %d, 4, %p, %p)\n",
	   pwfsao->xframesize,pwfsao->yframesize,buffer,usbuffer);

#ifdef vxWorks
    timexN(ospFGMeasure,redbuffer,pwfsao);
    timexN(ospFrameScramble, pwfsao->xframesize,pwfsao->yframesize,4,
	   buffer,usbuffer);   
#endif*/ /*vxWorks*/

/*    iterations = 1000;
        
    t1 = clock();
    for(i=0;i<iterations;i++)
    {
	ospFGMeasure(redbuffer,pwfsao);
    }
    t2=clock();
    ospShow(pwfsao);
    t_fgmeasure = (float)((float)(t2-t1)/(float)CLOCKS_PER_SEC);

    printf("Time taken for %d fast measures is %f\n",iterations,(float)((float)(t2-t1)/(float)CLOCKS_PER_SEC));
    t1 = clock();
    for(i=0;i<iterations;i++)
    {
	ospCoAdd(redbuffer,10,0.1,pwfsao);
	printf("pwfsao->coaddcounter = %d\n",pwfsao->coaddcounter);
    }
    t2=clock();
    
    t_coadd = (float)((float)(t2-t1)/(float)CLOCKS_PER_SEC);
    ospShow(pwfsao);
    printf("Time taken for %d coadds is %f\n",iterations,(float)((float)(t2-t1)/(float)CLOCKS_PER_SEC));

    t1 = clock();
    for(i=0;i<iterations;i++)
    {
	ospMeasure(redbuffer,pwfsao);
    }
    t2=clock();
    t_measure = (float)((float)(t2-t1)/(float)CLOCKS_PER_SEC);
        
    ospShow(pwfsao);

    printf("Time taken for %d measures is %f\n",iterations,(float)((float)(t2-t1)/(float)CLOCKS_PER_SEC));

   printf("Time taken for %d loops is:\n",iterations);
   printf("   FG Measure  %f s\n", t_fgmeasure);
   printf("   Measure     %f s\n", t_measure);
   printf("   Coadd       %f s\n", t_coadd);
   


*/


/*
 *    ospConvertAndScrambleFITS("z2_red.fits","z2_reduss.fits",pwfsao);
 */

/*
 *   ospConvertAndScrambleFITS("../data/z2_perf5.fits","z2_uss.fits",pwfsao);
 *   ospReadHeaderInt("z1_uss.fits",4,keys,vals);
 *   ospReadUShortImage(usbuffer,"z1_uss.fits",6400);
 *   ospWriteUShortImage(usbuffer,"z1_uss.fits.2",80,80);
 */    

#ifndef vxWorks
    ospTidyUp(pwfsao);
#else
    printf("Remember and ospTidyUp when you've finished with the structures\n");
#endif /*vxWorks */
    free(ospGeom);
    return(OK);
 }
/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME: 
 * ospChangeGeometry
 *
 * INVOCATION:
 * ospChangeGeometry(ospGeom,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) ospGeom       (struct OSP_GEOMETRY *)  structure containing modifications
 *                                            to the detector readout geometry
 * (!) wfsSpecific   (struct OSP_CONTEXT *)   wavefront sensor context structure
 *
 * FUNCTION VALUE:
 * (int)  A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To adapt the wfs context to changes in detector readout geometry
 * To allow the wfs software to adapt correctly to changes in the detector 
 * readout geometry, to the values specified in ospGeom.
 *
 * DESCRIPTION:
 * Updates the wfs context structure, checks for consistency of the new 
 * parameters, recalculates subtractive and multiplicative subframes to 
 * correspond with new readout geometry. If wfsSpecific->framesizeflag = 1
 * the ccd readout geometry reverts to the default values, i.e. the values
 * read in from the appropriate .ini file when the wfs context structure
 * was created by ospInit- with the exception of the value of framesizeflag
 * which may have been set to zero in the .ini file, but now will be 1.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known.
 *-
 */
int /*STATUS*/ ospChangeGeometry(struct OSP_GEOMETRY * ospGeom, 
				 struct OSP_CONTEXT * wfsSpecific)
{
    int ospstatus  = OK;     /* status flag */
    int i;                   /* counter     */

/*
 * Check for validity of supplied pointers to structures
 */
    if(ospGeom==NULL)
    {
        fprintf(stderr,"Pointer to struct OSP_GEOMETRY, ospGeom, is null,\n");
        fprintf(stderr,"osp geometric parameters unchanged\n");
        fprintf(stderr,"...error took place in ospChangeGeometry.\n");
        return(ERROR);
    }
    if(wfsSpecific==NULL)
    {
       fprintf(stderr,"Pointer to struct OSP_CONTEXT, wfsSpecific, is null,\n");
        fprintf(stderr,"osp geometric parameters unchanged\n");
        fprintf(stderr,"...error took place in ospChangeGeometry.\n");
        return(ERROR);
    }


/*
 * Update context structure with new geometric parameters
 */
    if(wfsSpecific->xarraysize != ospGeom->xarraysize ||
       wfsSpecific->yarraysize != ospGeom->yarraysize)
    {
	fprintf(stderr,
		"xarraysize or yarraysize redefined relative to .ini file\n");
	fprintf(stderr,
		"these parameters are specific to the array chip and should\n");
	fprintf(stderr,
		"not change on the fly!\n");
	fprintf(stderr,"...exiting ospChangeGeometry with no changes.\n");
	return(ERROR);
    }

    wfsSpecific->framesizeflag = ospGeom->framesizeflag ;
    wfsSpecific->sectors = ospGeom->sectors ;

    /*
     * If reduced frame is flagged in ospGeom structure, update remaining 
     * parameters of the detector geometry. If ospGeom->framesizeflag = 1
     * however, we use the default geometry parameters as specified in 
     * the .ini file for full frame processing, regardless of what value 
     * the .ini file may have for framesizeflag. 
     * Note consistency checks, and current limitation that no
     * binning is allowed
     */
    if(wfsSpecific->framesizeflag==0)
    {
	wfsSpecific->ospxstart = ospGeom->xstart ;
	wfsSpecific->ospystart = ospGeom->ystart ;
	
	if(ospGeom->xbin != 1 || ospGeom->ybin != 1)
	{
	    fprintf(stderr,
		    "Binning not yet implemented: xbin and ybin set to 1\n");
	    wfsSpecific->ospxbin = 1;
	    wfsSpecific->ospybin = 1;
	    ospstatus=ERROR;
	}
	
	wfsSpecific->ospxraster = ospGeom->xraster ;
	wfsSpecific->ospyraster = ospGeom->yraster ;
	if(ospGeom->xraster != ospGeom->yraster)
	{
	    fprintf(stderr,"yraster assumed equal to xraster\n");
	    wfsSpecific->ospyraster= wfsSpecific->ospxraster;
	    ospstatus=ERROR;
	}
	
	wfsSpecific->side = wfsSpecific->ospxraster*wfsSpecific->ospxbin;

	wfsSpecific->ospxspace = ospGeom->xspace ;
	wfsSpecific->ospyspace = ospGeom->yspace ;
	wfsSpecific->ospxsubap = ospGeom->xsubap ;
	wfsSpecific->ospysubap = ospGeom->ysubap ;

	if((wfsSpecific->xarraysize/2 - wfsSpecific->ospxstart - 
	    wfsSpecific->ospxsubap * (wfsSpecific->side+wfsSpecific->ospxspace) 
	    + wfsSpecific->ospxspace)<0)
	{
	    fprintf(stderr,
		    "Specified subapertures not realisable: xtail -ve\n");
	    ospstatus = ERROR;
	}
	if((wfsSpecific->yarraysize*2/wfsSpecific->sectors  
	    - wfsSpecific->ospystart - wfsSpecific->ospysubap * 
	    (wfsSpecific->side + wfsSpecific->ospyspace) 
	    + wfsSpecific->ospyspace)<0)
	{
	    fprintf(stderr,
		    "Specified subapertures not realisable: ytail -ve\n");
	    ospstatus = ERROR;
	}
	
        wfsSpecific->xframesize=(wfsSpecific->ospxsubap * 2 *
                                 wfsSpecific->ospxraster*wfsSpecific->ospxbin);
        wfsSpecific->yframesize=(wfsSpecific->ospysubap * wfsSpecific->sectors/2
                                 *wfsSpecific->ospyraster*wfsSpecific->ospybin);

    }
    else
    {
	wfsSpecific->ospxstart = wfsSpecific->defxstart;
	wfsSpecific->ospystart = wfsSpecific->defystart;
	wfsSpecific->ospxbin   = wfsSpecific->defxbin;
	wfsSpecific->ospybin   = wfsSpecific->defybin;
	wfsSpecific->ospxraster= wfsSpecific->defxraster;
	wfsSpecific->ospyraster= wfsSpecific->defyraster;
	wfsSpecific->ospxspace = wfsSpecific->defxspace;
	wfsSpecific->ospyspace = wfsSpecific->defyspace;
	wfsSpecific->ospxsubap = wfsSpecific->defxsubap;
	wfsSpecific->ospysubap = wfsSpecific->defysubap;
	wfsSpecific->side = wfsSpecific->ospxraster*wfsSpecific->ospxbin;

	wfsSpecific->xframesize=wfsSpecific->xarraysize;
	wfsSpecific->yframesize=wfsSpecific->yarraysize;
	if(wfsSpecific->framesizeflag!=1)
	{
	    ospstatus = ERROR;
	    fprintf(stderr,"Invalid value (%d) entered for framesizeflag:\n",
		    wfsSpecific->framesizeflag);
	    fprintf(stderr,"setting framesizeflag to 1\n");
	    wfsSpecific->framesizeflag = 1;
	}
    }
    
    /*
     * Calculate the positions of the bottom left hand corner and null position
     * for each subaperture, relative to the newly defined reduced or full frame
     * geometry.
     */
    if((ospCalculateSubaps(wfsSpecific))== ERROR)
    {
        fprintf(stderr,"Error in ospCalculateSubaps\n");
        ospstatus=ERROR;
    }

    /* 
     * Need to do ospReduceFrames for the subtractive and multiplicative offsets
     * if indeed the frame is reduced- otherwise set the reduced frames equal to
     * the full frames.
     */
    if(wfsSpecific->framesizeflag==1)
    {
	for(i=0;i<wfsSpecific->buffsize;i++)
	    wfsSpecific->redsubbuff[i] = wfsSpecific->ffsubbuff[i];
    }
    else
	if(ospReduceFrame(wfsSpecific->ffsubbuff, wfsSpecific->redsubbuff, 
			  wfsSpecific, 
			  wfsSpecific->xframesize*wfsSpecific->yframesize)
	   ==ERROR)
	{
	    fprintf(stderr,
		    "ospReduceFrame failed for subtractive offsets\n");
	    ospstatus=ERROR;
	}

    if(wfsSpecific->framesizeflag==1)
    {
	for(i=0;i<wfsSpecific->buffsize;i++)
	    wfsSpecific->redmultbuff[i] = wfsSpecific->ffmultbuff[i];
    }
    else
	if(ospReduceFrame(wfsSpecific->ffmultbuff, wfsSpecific->redmultbuff, 
			  wfsSpecific, 
			  wfsSpecific->xframesize*wfsSpecific->yframesize)
	   ==ERROR)
	{
	    fprintf(stderr,
		    "ospReduceFrame failed for multiplicative offsets\n");
	    ospstatus=ERROR;
	}

    if(ospstatus == ERROR)
        fprintf(stderr,"...error(s) took place in ospChangeGeometry.\n");

    return(ospstatus);
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospReduceFrame
 *
 * INVOCATION:
 * ospReduceFrame (buffp, newbuffp, wfsSpecific, buffsize)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) buffp         (float *)              pointer to full frame as input
 * (<) newbuffp      (float *)              pointer to reduced frame
 * (>) wfsSpecific   (struct OSP_CONTEXT *) pointer to wfs context structure
 * (>) buffsize      (int)                  buffer size for reduced frame
 *
 * FUNCTION VALUE:
 * (int)  A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To reduce the frame size to the minimum containing all subaperture pixels
 * To reduce the size of the data frame by removing the pixels which lie outwith
 * the square which contains the subapertures, and also the pixels which lie 
 * on the lines between the subapertures. Thus a full frame of 80x80 pixels
 * with a 3x3 array of subapertures each of 6x6 pixels will be reduced to a
 * an 18x18 subframe. This function has been written to enable reduced frame 
 * versions of the offset frames to be generated from their full frame versions
 * in response to any change of detector readout geometry, and also to enable
 * testing of realistic readout geometries from full frame data.
 *
 * DESCRIPTION:
 * The function uses the geometric parameters specified by the sectors, xstart,
 * ystart, xbin, ybin, xraster, yraster, xspace, yspace, xsubap, ysubap, 
 * xarraysize and yarraysize elements of the OSP_CONTEXT structure to calculate
 * the (full frame) coordinates of the pixels which lie within the grid of 
 * subapertures, and then calculates the new coordinates for the reduced frame,
 * such that (xstart+1, ystart+1) becomes (1,1), and the subapertures are 
 * immediately adjacent to one another. The symmetry requirements imposed by 
 * either the two or four sector readouts are observed in the calculation of 
 * the subaperture coordinates, in the same manner as ospCalculateSubaps.
 * xtail and ytail are calculated but no testing is done to see if they are 
 * realisable physically- this testing is done in ospInit or ospChangeGeometry.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * none
 *
 * PRIOR REQUIREMENTS:
 * That the OSP_CONTEXT structure has been updated to correspond with 
 * the current geometric parameters for the CCD readout. It is not necessary
 * for structure element framesizeflag to be set to 0, corresponding to a
 * reduced frame.
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * No provision for binning of pixels into superpixels.
 *-
 */
int /*STATUS*/ ospReduceFrame(float * buffp, float * newbuffp, struct OSP_CONTEXT * wfsSpecific,int buffsize)
{
    float * buff0; /* pointer to bot. left pixel of (bot. left|left) sector*/
    float * buff1; /* pointer to bot. left pixel of (bot. right|right) sector*/
    float * buff2; /* pointer to bot. left pixel of top right sector*/
    float * buff3; /* pointer to bot. left pixel of top left sector*/
    int i,j,k,l;   /* counters */
    int xtail;     /* number of pixels between right edge of sector and the 
		      rightmost pixel in a subaperture in that sector*/
    int ytail;     /* number of pixels between top edge of sector and the 
		      topmost pixel in a subaperture in that sector*/
    int ospstatus = OK; /* status flag*/
    int ii=0;      /* additional index of newbuffp, the reduced frame pointer*/
    int xarraysize;/*local declaration of xarraysize for notational handiness*/
    int yarraysize;/*local declaration of yarraysize for notational handiness*/
    int xpitch;    /*x pitch of subapertures, in pixels */
    int ypitch;    /*y pitch of subapertures, in pixels */
    int side;      /*loacl declaration of subaperture side, in pixels*/

    xarraysize = wfsSpecific->xarraysize;
    yarraysize = wfsSpecific->yarraysize;
    side = wfsSpecific->side;
    xpitch = side + wfsSpecific->ospxspace;
    ypitch = side + wfsSpecific->ospyspace;

    /* calculate xtail and ytail */

    xtail = xarraysize/2 - wfsSpecific->ospxstart - 
	(wfsSpecific->ospxsubap * xpitch) + wfsSpecific->ospxspace;
    ytail = yarraysize*2/wfsSpecific->sectors  - wfsSpecific->ospystart - 
	(wfsSpecific->ospysubap * ypitch) + wfsSpecific->ospyspace;

#ifdef OSP_VERBOSE
    printf("In ospReduceFrame: xtail is %d ytail is %d\n", xtail,ytail);
#endif /*OSP_VERBOSE*/


    /* calculate the start pixels of the buffers corresponding to the
     * first pixels of the bottom left and right sectors (4 sector case)
     * or left and right sectors (2 sector case) */
     

    buff0 = buffp + ((wfsSpecific->ospxstart) +
		     (wfsSpecific->ospystart) * xarraysize);
    buff1 = buff0 + ((wfsSpecific->ospxsubap * xpitch) - 
		     wfsSpecific->ospxspace + 2 * xtail);

    /* Move along the rows of the pixels of the subapertures, a row of pixels 
     * at a time and a row of all subapertures across the array at a time, 
     * copying the pixels within the subapertures into the buffer for the
     * reduced frame pointed to by newbuffp (for the lower two sectors in
     * the four sector case. */

    for(i=0;i<wfsSpecific->ospysubap;i++)
    {
	for(j=0;j<side;j++)
	{
	    for(k=0;k<wfsSpecific->ospxsubap;k++)
	    {
		for(l=0;l<side;l++)
		{
		*(newbuffp+ii) = *(buff0 +l + k * xpitch + j * xarraysize
				   + i * xarraysize * ypitch); 
		ii++;
		}
	    }
	    for(k=0;k<wfsSpecific->ospxsubap;k++)
	    {
		for(l=0;l<side;l++)
		{
		*(newbuffp+ii) = *(buff1 +l + k * xpitch + j * xarraysize
				   + i * xarraysize * ypitch); 
		ii++;
		}
	    }
	}
    }

    /* deal with the top two sectors in the four sector case in the same way */

    if(wfsSpecific->sectors == 4)
    {    
	buff2 = buff1 + ((wfsSpecific->ospysubap * ypitch) 
			 -wfsSpecific->ospyspace + 2 * ytail) * xarraysize;
	buff3 = buff2 -((wfsSpecific->ospxsubap * xpitch) 
			- wfsSpecific->ospxspace + 2 * xtail);
	for(i=0;i<wfsSpecific->ospysubap;i++)
	{
	    for(j=0;j<side;j++)
	    {
		for(k=0;k<wfsSpecific->ospxsubap;k++)
		{
		    for(l=0;l<side;l++)
		    {
			*(newbuffp+ii) = *(buff3 +l + k * xpitch + 
					   j * xarraysize +
					   i * xarraysize * ypitch); 
			ii++;
		    }
		}
		for(k=0;k<wfsSpecific->ospxsubap;k++)
		{
		    for(l=0;l<side;l++)
		    {
			*(newbuffp+ii) = *(buff2 +l + k * xpitch + 
					   j * xarraysize +
					   i * xarraysize * ypitch); 
			ii++;
		    }
		}
	    }
	}
    }
#ifdef OSP_VERBOSE
    printf("In ospReduceFrame:ii= %d\n",ii);
#endif /*OSP_VERBOSE*/
    return(ospstatus);
}
/*------------------------------------------------------------------------*/


/*
 *+
 * FUNCTION NAME:
 * ospReduceFits
 *
 * INVOCATION:
 * ospReduceFits( infile, outfile, wfsSpecific, buffsize)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) infile        (char *) name of input full frame FITS file
 * (<) outfile       (char *) name of output reduced frame FITS file
 * (>) wfsSpecific   (struct OSP_CONTEXT *) pointer to wfs context structure
 * (>) buffsize      (int)                  buffer size for reduced frame
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To generate a reduced frame FITS file from a full frame FITS file
 *
 * DESCRIPTION:
 * Buffers for the full and reduced frame are dynamically allocated. The
 * full frame FITS file is read into the full frame buffer, and ospMeasure is 
 * called to write the reduced version to the reduced frame buffer, which is 
 * then written to a FITS file named outfile. The structure element 
 * framesizeflag is set to zero (corresponding to a reduced frame) and
 * the geometric parameters of the CCD readout used to reduce the frame 
 * are written to the FITS header by calling ospAddContextToHeader. 
 * framesizeflag is restored to its entry value and the buffers are freed.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None.
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h, fitsio.h
 *
 * DEFICIENCIES:
 * FITS header from original file not copied to new reduced file, thus
 * losing the user specified keyword information from the original. Reads and
 * writes float images specifically- a future modification should read the
 * BITPIX keyword from the input FITS file and react accordingly. No provision
 * for binning of pixels into superpixels.
 *
 *-
 */
int /*STATUS*/ ospReduceFITS(char * infile, char * outfile, struct OSP_CONTEXT * wfsSpecific, int buffsize)
{
    float * buffp;         /* buffer for input full frame image */
    float * newbuffp;      /* buffer for output reduced frame image */
    int tempframeflag;     /* store for entry value of framesizeflag */
    int ospstatus = OK;    /* status flag */
    int side;              /* side size of subaperure, in pixels */
    side=wfsSpecific->side;

    /* allocate buffers */

    if((buffp = (float *) malloc ((size_t)(buffsize)*sizeof(float)))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for buffp\n");

	fprintf(stderr,"...error took place in ospReduceFITS\n");
	return(ERROR);
    }

    if((newbuffp = (float *) malloc( 
	(size_t)(side*side*wfsSpecific->ospxsubap*wfsSpecific->ospysubap*wfsSpecific->sectors)*
		 sizeof(float)))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for newbuffp\n");
	fprintf(stderr,"...error took place in ospReduceFITS\n");
	return(ERROR);
    }

    /* read in full frame image */

    if(ospReadFloatImage(buffp,infile,buffsize)==ERROR)
    {
	fprintf(stderr,"ospReadFloatImage failed for %s in ospReduceFITS\n",
		infile);
	fprintf(stderr,"... %s not created\n",outfile);
    }

    /* generate reduced frame */

    ospReduceFrame(buffp,newbuffp,wfsSpecific,buffsize);

    /* write out reduced frame image */

    ospWriteFloatImage(newbuffp,outfile,2*side*wfsSpecific->ospxsubap, 
		       (wfsSpecific->sectors/2)*side*wfsSpecific->ospysubap);

    /* write ccd readout parameters used to FITS header of new file */

    tempframeflag = wfsSpecific->framesizeflag;
    wfsSpecific->framesizeflag = 0;
    ospAddContextToHeader(outfile,wfsSpecific);
    wfsSpecific->framesizeflag = tempframeflag;
    
    /* free buffers */

    free(buffp);
    free(newbuffp);


    return(ospstatus);
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospReadHeaderInt
 *
 * INVOCATION:
 * ospReadHeaderInt (filename,numelem,keynames,values)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) filename     (char *)  name of FITS file whose header is being read
 * (>) numelem      (int)     maximaum number of keywords to be read
 * (>) keynames     (char **) name of array of keywords
 * (<) values       (int *)   array of integer values of keywords
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To read the integer values of an array of keywords from a FITS header
 *
 * DESCRIPTION:
 * Uses functions from the cfitsio library to open a FITS file for reading, 
 * and read the values of specified keywords with integer values. The keywords
 * elements of an array, and their values are read into the corresponding 
 * elements of an array of integers. The number of elements of the keyword
 * array may exceed the number of keywords actually present, but obviously not
 * the number of array elements allocated. The keywords should be consecutive 
 * elements of their array, as reading of keywords will end when a NULL value 
 * is encountered as a keyword. The file is closed when reading is finished.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 * 
 *
 * EXTERNAL VARIABLES:
 * none
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h, fitsio.h
 *
 * DEFICIENCIES:
 * Restricted to keywords with integer values.
 *
 *-
 */
int /*STATUS*/ ospReadHeaderInt(char * filename, int numelem, char ** keynames,
				int * values)
{
    fitsfile * fptr;  /* pointer to a fitsfile structure */
    int status=0;     /* status flag relevant to fits functions*/ 
    int ospstatus=OK; /* overall status flag for function */
    int i;            /* counter */
    char comment[80]; /* buffer for comment string read from FITS file */

    /* open FITS file */

    if(fits_open_file(&fptr,filename,1,&status))
    {
	ospPrintError( status );    
	fprintf(stderr,"...error took place in ospReadHeaderInt\n");
	return(ERROR);
    }

    /* read keywords array and write their values to correponding
     * elements of an integer array */

    i=0;
    while(keynames[i]!=NULL && i< numelem)
    {
	if(fits_read_key(fptr,TINT,keynames[i],values+i,comment,&status))
	{	   
	    ospstatus=ospPrintError( status );
	    fprintf(stderr,"...failed on %s",keynames[i]);
	} 
#ifdef OSP_VERBOSE
	printf("%s    \t%d\n",keynames[i],values[i]);
#endif /*OSP_VERBOSE*/
	i++;
    }

    /* close FITS file */

    if(fits_close_file(fptr,&status))
	ospstatus=ospPrintError( status );

    if(ospstatus==ERROR)
	fprintf(stderr,"...error took place in ospReadHeaderInt\n");
    return(ospstatus);
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospAddContextToHeader
 *
 * INVOCATION:
 * ospAddContextToHeader (filename,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) filename     (char *)  name of FITS file to which we add header info
 * (>) wfsSpecific  (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To write ccd readout geometry to a FITS header
 *
 * DESCRIPTION:
 * Opens FITS file for writing, and writes the following keywords and comments
 * into the header (values included by way of example): 
 * XSTART  =                   11 / [pixels] ospxstart                          
 * YSTART  =                   11 / [pixels] ospystart                  
 * XBIN    =                    1 / pixel binning factor in x dirn.           
 * YBIN    =                    1 / pixel binning factor in y dirn.           
 * XRASTER =                    8 / number of x superpixels across subap.       
 * YRASTER =                    8 / number of y superpixels across subap.       
 * XSPACE  =                    2 / [pixels] number in x between subaps.        
 * YSPACE  =                    2 / [pixels] number in y betweem subaps.        
 * XSUBAP  =                    3 / number of x subaps. per output              
 * YSUBAP  =                    3 / number of y subaps. per output              
 * OSP_FSZ =                    1 / framesizeflag (0:reduced,1:fullframe)       
 * OSP_NP  =                   14 / no. of Zernike coeffs. calculated           
 * OSP_MP  =                   64 / 2 x total number of subaps used in array    
 * OSP_SIDE=                    8 / [pixels] side length of subap               
 * OSP_NSIG=         3.000000E+00 / nsigma                                      
 * OSP_RDSQ=         3.600000E+01 / [e- **2] read noise per pixel sqared        
 * OSP_WT  =                    0 / weighting applied? (0:no, 1:yes)            
 * OSP_THR =        -1.000000E+00 / argument supplied to ospThreshold           
 * The FITS file is then closed.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * none
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h, fitsio.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
int /*STATUS*/ ospAddContextToHeader(char * filename,struct OSP_CONTEXT * wfsSpecific)
{
    fitsfile * fptr;       /* pointer to fitsfile structure */
    int status=0;          /* status flag relevant to fitsio functions */
    int ospstatus=OK;      /* overall status flag for function */

    if(fits_open_file(&fptr,filename,1,&status))
    {
	ospPrintError( status );    
	fprintf(stderr,"...error took place in ospAddContextToHeader\n");
	return(ERROR);
    }

    if(fits_update_key(fptr,TINT,"SECTORS",&(wfsSpecific->sectors),
		       "no. of CCD outputs",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"XSTART",&(wfsSpecific->ospxstart),
		       "[pixels] ospxstart",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"YSTART",&(wfsSpecific->ospystart),
		       "[pixels] ospystart",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"XBIN",&(wfsSpecific->ospxbin),
		       "pixel binning factor in x dirn.",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"YBIN",&(wfsSpecific->ospybin),
		       "pixel binning factor in y dirn.",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"XRASTER",&(wfsSpecific->ospxraster),
		       "number of x superpixels across subap.",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"YRASTER",&(wfsSpecific->ospyraster),
		       "number of y superpixels across subap.",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"XSPACE",&(wfsSpecific->ospxspace),
		       "[pixels] number in x between subaps.",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"YSPACE",&(wfsSpecific->ospyspace),
		       "[pixels] number in y betweem subaps.",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"XSUBAP",&(wfsSpecific->ospxsubap),
		       "number of x subaps. per output",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"YSUBAP",&(wfsSpecific->ospysubap),
		       "number of y subaps. per output",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"OSP_FSZ",&(wfsSpecific->framesizeflag),
		       "framesizeflag (0:reduced,1:fullframe)",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"OSP_NP",&(wfsSpecific->np),
		       "no. of Zernike coeffs. calculated",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"OSP_MP",&(wfsSpecific->mp),
		       "2 x total number of subaps used in array",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"OSP_SIDE",&(wfsSpecific->side),
		       "[pixels] side length of subap ",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TFLOAT,"OSP_NSIG",&(wfsSpecific->nsigma),
		       "nsigma",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TFLOAT,"OSP_RDSQ",&(wfsSpecific->readsq),
		       "[e- **2] read noise per pixel sqared",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TINT,"OSP_WT",&(wfsSpecific->weight),
		       "weighting applied? (0:no, 1:yes)",&status))
	ospstatus=ospPrintError( status );    
    if(fits_update_key(fptr,TFLOAT,"OSP_THR",&(wfsSpecific->thresh),
		       "argument supplied to ospThreshold",&status))
	ospstatus=ospPrintError( status );    
    if(fits_close_file(fptr,&status))
	ospstatus=ospPrintError( status );

    if(ospstatus==ERROR)
	fprintf(stderr,"...error took place in ospAddContextToHeader\n");
    return(ospstatus);
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospShow
 *
 * INVOCATION:
 * ospShow(wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) wfsSpecific     (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To display the current values of the wfs context
 *
 * DESCRIPTION:
 * Generates a neatly formatted summary of most of the context structure's
 * current values on standard output. Parameters reported are:
 * osp structure                      : wfsSpecific
 * SECTORS                            : wfsSpecific->sectors
 * XSTART                             : wfsSpecific->ospxstart
 * YSTART                             : wfsSpecific->ospystart
 * XBIN                               : wfsSpecific->ospxbin
 * YBIN                               : wfsSpecific->ospybin
 * XRASTER                            : wfsSpecific->ospxraster
 * YRASTER                            : wfsSpecific->ospyraster
 * XSPACE                             : wfsSpecific->ospxspace
 * YSPACE                             : wfsSpecific->ospyspace
 * XSUBAP                             : wfsSpecific->ospxsubap
 * YSUBAP                             : wfsSpecific->ospysubap
 * OSP_FSZ (0:reduced,1:full frame)   : wfsSpecific->framesizeflag
 * Number of Zernikes (np)            : wfsSpecific->np
 * 2*TOTAL number of subaps (mp) used : wfsSpecific->mp
 * xarraysize (in pixels)             : wfsSpecific->xarraysize
 * yarraysize (in pixels)             : wfsSpecific->yarraysize
 * xframesize (in pixels)             : wfsSpecific->xframesize
 * yframesize (in pixels)             : wfsSpecific->yframesize
 * side length of subap (in pixels)   : wfsSpecific->side
 * size of buffer for frame           : wfsSpecific->buffsize
 * nsigma (used in ospThreshold)      : wfsSpecific->nsigma
 * Read noise per subaperture,squared : wfsSpecific->readsq
 * Weighting applied? (0:no, 1:yes)   : wfsSpecific->weight
 * Threshold parameter                : wfsSpecific->thresh
 * Coaddcounter (frames coadded -1)   : wfsSpecific->coaddcounter
 * WfsSource (wfs id)                 : wfsSpecific->wfsSource
 * WfsMode (AO = 0, FG = 1)           : wfsSpecific->wfsMode
 * Centres data: number of centres*4  : wfsSpecific->centres[0]
 *             : x0, xoff, y0, yoff   : wfsSpecific->centres[]...
 * Control matrix used (first 10 cols): wfsSpecific->c[][]...
 * Displacements s-x, ds-x, s-y, ds-y : wfsSpecific->s[]...
 * Fitting variances                  : wfsSpecific->fvars[]...
 * Measurement variances              : wfsSpecific->mvars[]...
 * Zernikes calculated                : wfsSpecific->z[]...
 *
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * none
 *
 * PRIOR REQUIREMENTS:
 * 
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * Should test if the structure is NULL
 *-
 */

int /*STATUS*/ ospShow(struct OSP_CONTEXT * wfsSpecific)
{
    int ospstatus = OK;      /* status flag */
    int i;                   /* counter */
    printf("ospShow says........................\n");
    printf("osp structure                      : %p\n", wfsSpecific);
    printf("SECTORS                            : %6d\n",wfsSpecific->sectors);
    printf("XSTART                             : %6d\n",wfsSpecific->ospxstart);
    printf("YSTART                             : %6d\n",wfsSpecific->ospystart);
    printf("XBIN                               : %6d\n",wfsSpecific->ospxbin);
    printf("YBIN                               : %6d\n",wfsSpecific->ospybin);
    printf("XRASTER                            : %6d\n",wfsSpecific->ospxraster);
    printf("YRASTER                            : %6d\n",wfsSpecific->ospyraster);
    printf("XSPACE                             : %6d\n",wfsSpecific->ospxspace);
    printf("YSPACE                             : %6d\n",wfsSpecific->ospyspace);
    printf("XSUBAP                             : %6d\n",wfsSpecific->ospxsubap);
    printf("YSUBAP                             : %6d\n",wfsSpecific->ospysubap);
    printf("OSP_FSZ (0:reduced,1:full frame)   : %6d\n",
	   wfsSpecific->framesizeflag);
    printf("Number of Zernikes (np)            : %6d\n", wfsSpecific->np);
    printf("2*TOTAL number of subaps (mp) used : %6d\n", wfsSpecific->mp);
    printf("xarraysize (in pixels)             : %6d\n",
	   wfsSpecific->xarraysize);
    printf("yarraysize (in pixels)             : %6d\n",
	   wfsSpecific->yarraysize);
    printf("xframesize (in pixels)             : %6d\n",
	   wfsSpecific->xframesize);
    printf("yframesize (in pixels)             : %6d\n",
	   wfsSpecific->yframesize);
    printf("side length of subap (in pixels)   : %6d\n", wfsSpecific->side);
    printf("size of buffer for frame           : %6d\n", wfsSpecific->buffsize);
    printf("nsigma (used in ospThreshold)      : %6.3f\n", wfsSpecific->nsigma);
    printf("Read noise per subaperture,squared : %6.3f\n", wfsSpecific->readsq);
    printf("Weighting applied? (0:no, 1:yes)   : %6d\n", wfsSpecific->weight);
    printf("Threshold parameter                : %6.3f\n", wfsSpecific->thresh);
    printf("Coaddcounter (frames coadded)      : %6d\n", 
	   wfsSpecific->coaddcounter);
    printf("Tipscale parameter                 : %6.3f\n",
	   wfsSpecific->tipscale);
    printf("Tiltscale parameter                : %6.3f\n",
	   wfsSpecific->tiltscale);
    printf("Focusscale parameter               : %6.3f\n", 
	   wfsSpecific->focusscale);
    printf("Angle                              : %6.3f\n", (wfsSpecific->angle));
    printf("WfsSource (wfs id)                 : %6d\n",wfsSpecific->wfsSource);
    printf("WfsSource (wfs id)                 : %6d\n",wfsSpecific->wfsSource);
    printf("WfsMode (AO = 0 FG = 1)            : %6d\n",wfsSpecific->wfsMode);
    printf("Centres data: number of centres*4  : %6.3f\n", 
	   wfsSpecific->centres[0]);
    printf("            :__x0_____xoff_______y0_____yoff__\n");
    for (i=1;i<((wfsSpecific->centres[0])+1);i=i+4)
    {
	printf("          %9.3f%9.3f%9.3f%9.3f\n",
	       wfsSpecific->centres[i],
	       wfsSpecific->centres[i+1],
	       wfsSpecific->centres[i+2],
	       wfsSpecific->centres[i+3]);
    }
    printf("Control matrix used (first 10 cols):\n");
    ospShowMatrix(wfsSpecific->c,wfsSpecific->np,10);
    printf("Displacements:__s-x____dssq-x__ _s-y_____dssq-y_\n");
    for(i=1;i<=wfsSpecific->mp;i=i+2)
    {
	printf("           %9.3f%9.4f%9.3f%9.4f\n",
	       wfsSpecific->s[i],
	       wfsSpecific->dssq[i],
	       wfsSpecific->s[i+1],
	       wfsSpecific->dssq[i+1]);
    }
    printf("Fitting variances:\n");
    ospShowVector(wfsSpecific->fvars,wfsSpecific->np);
    printf("Measurement variances:\n");
    ospShowVector(wfsSpecific->mvars,wfsSpecific->np);
    printf("Zernikes calculated:\n");
    ospShowVector(wfsSpecific->z,wfsSpecific->np);

    return(ospstatus);
}
/*------------------------------------------------------------------------*/



/*
 *+
 * FUNCTION NAME:
 * ospConvertAndScrambleFITS
 *
 * INVOCATION:
 * ospConvertAndScrambleFITS(infile, outfile, wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * infile       (char *)               name of input file 
 * outfile      (char *)               name of output file
 * wfsSpecific  (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR.
 *
 * PURPOSE:
 * To convert a float FITS image to ushort int, scramble it and write it to file
 * This was written initially to generate test data of the same format as 
 * would be expected from a ccd after a frame callback, from our readily
 * available unscrambled float images.
 *
 * DESCRIPTION:
 * The input file is opened and the NAXIS1 and NAXIS2 keywordss read from the 
 * FITS header to check if the dimensions are consistent with the current
 * detector readout geometry as stored in the context structure. If not, 
 * the function exits with a warning; if so, two buffers of the appropriate 
 * size are allocated, one for the input float image, the other for the output 
 * scrambled unsigned short int image. The input image is read into its buffer 
 * from infile, and then both scrambled and converted to unsigned short int by
 * a call to ospFrameScramble. The resultant image is written to a FITS file, 
 * and the current detector readout geometry added to the header. The buffers
 * are freed.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * none
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h, fitsio.h
 *
 * DEFICIENCIES:
 * none known
 *-
 */

int /*STATUS*/ ospConvertAndScrambleFITS(char * infile, char * outfile, struct OSP_CONTEXT * wfsSpecific)
{
    float * buffp;                /* ptr to buffer for input float image */
    unsigned short int * usbuffp; /* ptr to buffer for output image */
    int ospstatus=OK;             /* overall status flag for function */
    int xframesize;               /* xframesize of input image, in pixels */
    int yframesize;               /* yframesize of input image, in pixels */
    int outputs;                  /* no. of outputs from ccd */
    fitsfile * fptr;              /* pointer to fitsfile structure */
    int status=0;                 /* status flag relevant to fitsio functions*/
    char comment[80];       /* buffer for comments string read from FITS file*/

    if(fits_open_file(&fptr,infile,1,&status))
    {
	ospPrintError( status );    
	fprintf(stderr,"...error took place in ospConvertAndScrambleFITS\n");
	return(ERROR);
    }
    if(fits_read_key(fptr,TINT,"NAXIS1",&xframesize,comment,&status))
    {
	ospstatus=ERROR;
	fprintf(stderr,"fits_read_key failed for NAXIS1 in ");
	fprintf(stderr,"ospConvertAndScrambleFITS\n");
    }
    if(fits_read_key(fptr,TINT,"NAXIS2",&yframesize,comment,&status))
    {
	ospstatus=ERROR;
	fprintf(stderr,"fits_read_key failed for NAXIS2 in ");
	fprintf(stderr,"ospConvertAndScrambleFITS\n");
    }
    if(fits_close_file(fptr,&status))
	ospstatus=ospPrintError( status );

    if(xframesize!=wfsSpecific->xframesize)
    {
	fprintf(stderr,"Warning: NAXIS1 (= %d) from %s ",xframesize,infile);
	fprintf(stderr,"in ospConvertAndScrambleFITS\n");
	fprintf(stderr,
		"is inconsistent with xframesize in context structure\n");
    }
    if(yframesize!=wfsSpecific->yframesize)
    {
	fprintf(stderr,"Warning: NAXIS2 (= %d) from %s ",yframesize,infile);
	fprintf(stderr,"in ospConvertAndScrambleFITS\n");
	fprintf(stderr,
		"is inconsistent with yframesize in context structure\n");
    }
    outputs = wfsSpecific->sectors;

    if((buffp = (float *)malloc((size_t)(xframesize*yframesize)*sizeof(float)))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for buffp ");
	fprintf(stderr,"in ospConvertAndScrambleFITS.\n");
	return(ERROR);
    }

    if((usbuffp = (unsigned short int *)malloc((size_t)(xframesize*yframesize)*sizeof(unsigned short int)))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for usbuffp \n");
	fprintf(stderr,"in ospConvertAndScrambleFITS.\n");
	free(buffp);
	return(ERROR);
    }    



    if((ospReadFloatImage(buffp,infile,xframesize*yframesize))==ERROR)
    {
	fprintf(stderr,"ospReadFloatImage failed in ospConvertAndScramble\n");
	ospstatus=ERROR;
    }	
    if((ospFrameScramble(xframesize,yframesize,outputs,buffp,usbuffp))==ERROR)
    {
	fprintf(stderr,"ospFrameScramble failed in ospConvertAndScramble\n");
	ospstatus=ERROR;
    }	
    if((ospWriteUShortImage(usbuffp,outfile,xframesize,yframesize))==ERROR)
    {
	fprintf(stderr,"ospWriteUShortImage failed in ospConvertAndScramble\n");
	ospstatus=ERROR;
    }	
    if((ospAddContextToHeader(outfile,wfsSpecific))==ERROR)
    {
	fprintf(stderr,"ospAddContextToHeader failed in ospConvertAndScramble\n");
	ospstatus=ERROR;
    }
    free(buffp);
    free(usbuffp);
    return(ospstatus);

}
 /*------------------------------------------------------------------------*/


/*+
 * FUNCTION NAME:
 * ospFrameScramble
 *
 * INVOCATION:
 * ospFrameScramble (xPixels, yPixels, outputs, inBuffer, outBuffer)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>)	xPixels	   (const int)	Number of columns
 * (>)	yPixels	   (const int)	Number of rows
 * (>)	outputs	   (const int)	Number of detector outputs (2 or 4)
 * (>)  inFrame	   (float *)	Pointer to input frame buffer
 * (<)  outBuffer  (unsigned short int *)  Pointer to output frame buffer
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * Scramble an entire frame of data
 *
 * DESCRIPTION:
 * This function takes a simulated frame of data and scrambles the pixels 
 * into the order they are read from the detector.
 *
 * ACKNOWLEDGEMENTS:
 * This function is almost identical to detFrameScramble, except that 
 * uint16 has been explicitly declared as unsigned short int, and the
 * interim error handling is simpler.
 * This function is based around the LeachDeScramble (lds) program 
 * provided by Les Saddlemyer
 * and Tim Hardy, Hertzberg Institute of Astrophysics, Canada.
 *
 * EXTERNAL VARIABLES:
 * None. (The function needs to be reentrant)
 *
 * PRIOR REQUIREMENTS:
 * inBuffer must point to a buffer containing 
 * xPixels*yPixels floating point values.
 * outBuffer must point to a buffer large enough to contain at least 
 * xPixels*yPixels unsigned short integer values.
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */

int /* STATUS*/ ospFrameScramble
(
    const int  xPixels,             /* Number of columns. */
    const int  yPixels,             /* Number of rows.	*/
    const int  outputs,             /* Number of detector outputs (2 or 4).  */
    float *    inBuffer,            /* Pointer to input frame buffer	*/
    unsigned short int * outBuffer  /* Pointer to output frame buffer.	*/
    )
{
    unsigned short int *  ptr;	/* Pointer into output frame buffer.  */

    int	i, j;			/* Counters.			      */
    int	nPixels;		/* Total number of pixels.	      */
    int	xPixelsSector;		/* Number of columns per sector.      */
    int	yPixelsSector;		/* Number of rows per sector.	      */
    
    float * ps1;		/* Pointer to beginning of sector 1.  */
    float * ps2;		/* Pointer to beginning of sector 2.  */
    float * ps3;		/* Pointer to beginning of sector 3.  */
    float * ps4;		/* Pointer to beginning of sector 4.  */
    
    
	if ( (inBuffer == NULL) || (outBuffer == NULL) )
	{
	    fprintf(stderr,"No input and/or output buffers defined\n");
	    fprintf(stderr,"... error took place in ospFrameScramble\n");
		return (ERROR);
	}

#ifdef OSP_VERBOSE
	printf ("ospFrameScramble: Scrambling %d x %d pixels from frame at %p to %p\n",
	        xPixels, yPixels, inBuffer, outBuffer);
#endif /* OSP_VERBOSE */


	/*
	 * The algorithm used to unscramble the data depends on the number of outputs
	 * from the detector. If there are two outputs the sectors are arranged like this
	 *
	 *   +------------+------------+
	 *   |  sector 1  |  sector 2  |
	 *   0----->------+-----<------0
	 *
	 * and if there are four outputs the sectors are arranged like this
	 *
	 *   0----->------+-----<------0
	 *   |  sector 4  |  sector 3  |
	 *   +------------+------------+
	 *   |  sector 1  |  sector 2  |
	 *   0----->------+-----<------0
	 *
	 * "0" shows the origin of each sector and ">" the direction of readout.
	 */

	switch (outputs)
	{
		case (2):

			/* There are two outputs and therefore 2 sectors in a 2x1 pattern. */

			nPixels = xPixels * yPixels;
			xPixelsSector = xPixels / 2;
			yPixelsSector = yPixels;

#ifdef OSP_VERBOSE
			printf ("Two sectors of size %d x %d\n", xPixelsSector, yPixelsSector);
#endif /* OSP_VERBOSE */

			/* Initialise the starting position for each sector */

			ps1 = inBuffer;
			ps2 = &inBuffer[xPixels - 1];
			ptr = outBuffer;

			/* Treat one line at a time, moving sector pointers */

			for (i = 0; i < yPixelsSector; i++)
			{
				for (j = 0; j < xPixelsSector; j++)
				{
					/*
					 * Change the order here if sectors 1, 2 is
					 * different from the order of arrival
					 */

					*ptr++ = (unsigned short int) *ps1++;
					*ptr++ = (unsigned short int) *ps2--;
				}
				ps1 += xPixelsSector;
				ps2 += xPixelsSector * 3;
			}
			break;

		case (4):

			/* There are four outputs and therefore 4 sectors in a 2x2 pattern. */

			nPixels = xPixels * yPixels;
			xPixelsSector = xPixels / 2;
			yPixelsSector = yPixels / 2;

#ifdef OSP_VERBOSE
			printf ("Four sectors of size %d x %d\n", xPixelsSector, yPixelsSector);
#endif /* OSP_VERBOSE */

			/* Initialise the starting position for each sector */

			ps1 = inBuffer;
			ps2 = &inBuffer[xPixels - 1];
			ps3 = &inBuffer[nPixels - 1];
			ps4 = &inBuffer[nPixels - xPixels];
			ptr = outBuffer;

			/* Treat one line at a time, moving sector pointers */

			for (i = 0; i < yPixelsSector; i++)
			{
				for (j = 0; j < xPixelsSector; j++)
				{
					/*
					 * change the order here if sectors 1, 2, 3, 4 is
					 * different from the order of arrival
					 */

					*ptr++ = (unsigned short int) *ps1++;
					*ptr++ = (unsigned short int) *ps2--;
					*ptr++ = (unsigned short int) *ps3--;
					*ptr++ = (unsigned short int) *ps4++;
				}
				ps1 += xPixelsSector;
				ps2 += xPixelsSector * 3;
				ps3 -= xPixelsSector;
				ps4 -= xPixelsSector * 3;
			}
			break;

		default:

		    {
			fprintf(stderr,"Bad number of outputs given\n");
			fprintf(stderr,"... error took place in ospFrameScramble\n");
		    }
			return (ERROR);
	}

	return (OK);
}
/*------------------------------------------------------------------------*/



/*
 *+
 * FUNCTION NAME:
 * ospFrameTypeConvert
 *
 * INVOCATION:
 * ospFrameTypeConvert ( usbuffp, fbuffp, mode, buffsize)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (> or <)  usbuffp    (char *) pointer to buffer of unsigned short ints
 * (< or >)  fbuffp     (char *) pointer to buffer of floats
 * (>)       mode       (char *) direction of type conversion
 * (>)       buffsize   (int)    size of frame
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To convert a frame of floats to a frame of unsigned short ints, or vice-versa
 * 
 * DESCRIPTION:
 * Takes a buffer of unsigned short ints, and copies and casts it into a buffer
 * of floats if mode is ">", or vice-versa if mode is "<".
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * none
 *
 * PRIOR REQUIREMENTS:
 * That arrays of the required size (i.e. greater than or equal to buffsize)
 * have been allocated .
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * none known
 *-
 */

int /*STATUS*/ ospFrameTypeConvert(unsigned short int * usbuffp, float * fbuffp, char * mode,int buffsize)
{
    int i;     /* counter */
    
    if (strncmp(mode,">",1)==0)
	for(i=0;i<buffsize;i++)
	    * (fbuffp+i) =  (float) (*(usbuffp+i));
    else if (strncmp(mode,"<",1)==0)
	for(i=0;i<buffsize;i++)
	{
	    * (usbuffp+i) = (unsigned short int) *(fbuffp+i);
	}
    else
    {
	fprintf(stderr,"Invalid mode supplied as argument to ospFrameTypeConvert\n");
        return(ERROR);
    }
    return(OK);  
}

/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospTidyUpHr
 *
 * INVOCATION:
 * ospTidyUpHr(hrwfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) hrwfsSpecific (struct OSP_HRCONTEXT *) pointer to hrwfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To free all the memory allocated from the corresponding ospInitHr()
 *
 * DESCRIPTION:
 * The memory allocated when hrwfsSpecific was created using ospInithr is 
 * freed, including the memory allocated to contain the structure itself.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */

int ospTidyUpHr ( struct OSP_HRCONTEXT * hrwfsSpecific )
{
    int ospstatus =OK;

    if(hrwfsSpecific == NULL)
    {
	fprintf(stderr,"ospTidyUpHr failed: argument null pointer\n");
	return(ERROR);
    }
    else
    {
	if(hrwfsSpecific->ffsubbuff == NULL)
	{
	    fprintf(stderr,"Error in ospTidyUpHr: pointer to ffsubbuff is null\n");
	    ospstatus =ERROR;
	}
	else
	    free(hrwfsSpecific->ffsubbuff);

	free(hrwfsSpecific);
    }
    return(ospstatus);
}


/*------------------------------------------------------------------------*/


/*
 *+
 * FUNCTION NAME:
 * ospTidyUp
 *
 * INVOCATION:
 * ospTidyUp(wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) wfsSpecific (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To free all the memory allocated from the corresponding ospInit()
 *
 * DESCRIPTION:
 * The memory allocated when wfsSpecific was created using ospInit is 
 * freed, including the memory allocated to contain the structure itself.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
int /*STATUS*/ ospTidyUp( struct OSP_CONTEXT * wfsSpecific)
{
    int ospstatus =OK;

    if(wfsSpecific == NULL)
    {
	fprintf(stderr,"ospTidyUp failed: argument null pointer\n");
	return(ERROR);
    }
    else
    {
	if(wfsSpecific->c == NULL)
	{
	    fprintf(stderr,"Error in ospTidyUp: pointer to c is null\n");
	    ospstatus =ERROR;
	}
	else
	    freeMatrix(wfsSpecific->c,1,OSP_ZMAX,1,2*OSP_SUBAPSMAX);

	if(wfsSpecific->fvars == NULL)
	{
	    fprintf(stderr,"Error in ospTidyUp: pointer to fvars is null\n");
	    ospstatus =ERROR;
	}
	else
	    freeVector(wfsSpecific->fvars,1,OSP_ZMAX);

	if(wfsSpecific->mvars == NULL)
	{
	    fprintf(stderr,"Error in ospTidyUp: pointer to mvars is null\n");
	    ospstatus =ERROR;
	}
	else
	    freeVector(wfsSpecific->mvars,1,OSP_ZMAX);

	if(wfsSpecific->s == NULL)
	{
	    fprintf(stderr,"Error in ospTidyUp: pointer to s is null\n");
	    ospstatus =ERROR;
	}
	else
	    freeVector(wfsSpecific->s,1,2*OSP_SUBAPSMAX);

	if(wfsSpecific->dssq == NULL)
	{
	    fprintf(stderr,"Error in ospTidyUp: pointer to dssq is null\n");
	    ospstatus =ERROR;
	}
	else
	    freeVector(wfsSpecific->dssq,1,2*OSP_SUBAPSMAX);

	if(wfsSpecific->z == NULL)
	{
	    fprintf(stderr,"Error in ospTidyUp: pointer to z is null\n");
	    ospstatus =ERROR;
	}
	else
	    freeVector(wfsSpecific->z,1,OSP_ZMAX);

	if(wfsSpecific->err == NULL)
	{
	    fprintf(stderr,"Error in ospTidyUp: pointer to err is null\n");
	    ospstatus =ERROR;
	}
	else
	    freeVector(wfsSpecific->err,1,OSP_ZMAX);

	if(wfsSpecific->ffsubbuff == NULL)
	{
	    fprintf(stderr,"Error in ospTidyUp: pointer to ffsubbuff is null\n");
	    ospstatus =ERROR;
	}
	else
	    free(wfsSpecific->ffsubbuff);

	if(wfsSpecific->redsubbuff == NULL)
	{
	    fprintf(stderr,"Error in ospTidyUp: pointer to redsubbuff is null\n");
	    ospstatus =ERROR;
	}
	else
	    free(wfsSpecific->redsubbuff);

	if(wfsSpecific->sumbuff == NULL)
	{
	    fprintf(stderr,"Error in ospTidyUp: pointer to sumbuff is null\n");
	    ospstatus =ERROR;
	}
	else
	    free(wfsSpecific->sumbuff);

	if(wfsSpecific->ffmultbuff == NULL)
	{
	    fprintf(stderr,"Error in ospTidyUp: pointer to ffmultbuff is null\n");
	    ospstatus =ERROR;
	}
	else
	    free(wfsSpecific->ffmultbuff);

	if(wfsSpecific->redmultbuff == NULL)
	{
	    fprintf(stderr,"Error in ospTidyUp: pointer to redmultbuff is null\n");
	    ospstatus =ERROR;
	}
	else
	    free(wfsSpecific->redmultbuff);

	/* ensure that global pointers to deleted structures are null */

#ifdef vxWorks
	if(wfsSpecific->wfsMode == AO)
	{
	    if(wfsAoAddr[wfsSpecific->wfsSource] == NULL)
	    {
		fprintf(stderr,"Error in ospTidyUp: pointer to wfsAoAddr is null\n");
		ospstatus =ERROR;
	    }
	    else
	    {
		wfsAoAddr[wfsSpecific->wfsSource] = NULL;
	    }
	}
	else
	{
	    if(wfsFgAddr[wfsSpecific->wfsSource] == NULL)
	    {
		fprintf(stderr,"Error in ospTidyUp: pointer to wfsFgAddr is null\n");
		ospstatus =ERROR;
	    }
	    else
	    {
		wfsFgAddr[wfsSpecific->wfsSource] = NULL;
	    }
	}
#endif /*vxWorks*/
	free(wfsSpecific);
    }
    return(ospstatus);
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospCalibrate
 *
 * INVOCATION:
 * ospCalibrate(calibpath, wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) calibpath (char *) path to directory of calibration data
 * (!) wfsSpecific (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To populate a reconstructor matrix and invert to obtain control matrix
 *
 * DESCRIPTION:
 * Calibration frames of displaced SH spots corresponding to linearly
 * idependent combinations of known Zernike
 * aberrations are used to form a reconstructor matrix with respect to the
 * basis functions defined by the linear combinations. This reconstructor matrix
 * is transformed into one with respect to the individual Zernike polynomials
 * of unit magnitude, and further inverted to provide the necessary control
 * matrix which is applied to the SH spot displacements in an arbitrary
 * input frame to yield the Zernike coefficients. The control matrix is written
 * to a file specified by the character string wfsSpecific->controlfile, and the
 * calculated fitting variances written to a file specified by the character
 * string wfsSpecific->fvarsfile. If either of these files exist, they are 
 * copied to a file with a .bak extension added, befeore the new version is 
 * created.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 *
 * INCLUDE FILES:
 * osp.h, fitsio.h
 *
 * DEFICIENCIES:
 * Not general enough-need to prompt for data input, basis functions etc.
 *
 *-
 */
int /*STATUS*/ ospCalibrate(char * calibpath, struct OSP_CONTEXT * wfsSpecific) 
{
    float * buffp;
    float buffer[OSP_BUFFMAX];
    float *w;
    float **a;
    float **u;
    float **v;
    float **f;
    float **ca;
    float **finv;
    int i;
    int j;
    int ospstatus=OK;
/*  int k; */
    int tempflag;
    int tempxsize;
    int tempysize;
    char name[80];

    char controltempname[OSP_MAXSTR];
    char fvarstempname[OSP_MAXSTR];

    buffp = buffer;
    w=vector(1,wfsSpecific->np);
    a=matrix(1,wfsSpecific->mp,1,wfsSpecific->np);
    u=matrix(1,wfsSpecific->mp,1,wfsSpecific->np);
    v=matrix(1,wfsSpecific->np,1,wfsSpecific->np);
    f=matrix(1,wfsSpecific->np,1,wfsSpecific->np);
    ca=matrix(1,wfsSpecific->np,1,wfsSpecific->np);
    finv=matrix(1,wfsSpecific->np,1,wfsSpecific->mp);

    for(i=1;i<=wfsSpecific->np;i++)
    {
	for(j=1;j<=wfsSpecific->np;j++)
	{
	    f[i][j]=0.0;
	}
    }
/**** Populate reconstructor, and [f], matrix showing basis function
***** representations in terms of Zernike polynomials */

/**** Do so using previously calculated centroid positions-
***** if the vector of centroid positions contains entries for
***** the corner subapertures they may be removed by an if statement
***** which ignores the (2*index of subaperture)th and 
***** (2*index of subaperture + 1)th entries */
/*
    for (i=1;i<=wfsSpecific->np;i++)
    {
	sprintf(name,"../data/z%d_perf.norm",i);
	ospReadVectorFromFile(name,s,1,wfsSpecific->mp,1);
	k=0;
	for (j=1;j<=72;j++)
	{
	if((j!=1) && (j!=2) && (j!=11) && (j!=12) && (j!=61) && (j!=62) && (j!=71) && (j!=72))
	 {
	    k++;
	    a[k][i] = wfsSpecific->s[j];
	 }

	}
	f[i][i] = 1.0;
    }	       
    */

/**** Do so using fits images corresponding individual Zernike coefficients 
***** of magnitude OSP_TESTMAG. In general the fits images may be made up of
***** linear combinations of Zernikes, and the f[][] matrix must be modified 
***** to take account of this. */

/**** N.B. We read a centres file that is specific to the calibration
***** data here. The test data may need a centres file with different null
***** positions, which is input at the command line. */

/*    ospSaveNullPositions("../data/z0_perf5.fits", buffp, 
                           s,ds,wfsSpecific);
			 */

    tempflag = wfsSpecific->framesizeflag;
    tempxsize = wfsSpecific->xframesize;
    tempysize = wfsSpecific->yframesize;
    wfsSpecific->framesizeflag=1;
    wfsSpecific->xframesize = wfsSpecific->xarraysize;
    wfsSpecific->yframesize = wfsSpecific->yarraysize;
    ospReadNulls(wfsSpecific->nullfile,wfsSpecific);
    ospCalculateSubaps(wfsSpecific);


    for (i=1;i<=wfsSpecific->np;i++)
    {
        sprintf(name,"%s/z%d_perf%d.fits",calibpath,i,OSP_TESTMAG);
        ospReadFloatImage(buffp,name,wfsSpecific->buffsize); 

        printf("Reading image data from ");
        printf(name);
        printf("\n");

	ospSubtractFrameFromFrame(buffp, wfsSpecific->ffsubbuff, wfsSpecific);
	ospMultiplyFrameByFrame(buffp, wfsSpecific->ffmultbuff, wfsSpecific);
	ospCentroidWrapper(buffp,wfsSpecific);
        for (j=1;j<=wfsSpecific->mp;j++)
        {
            a[j][i] = wfsSpecific->s[j];
        }
        f[i][i] = OSP_TESTMAG;
    }          
    printf("Showing a: \n");
    ospShowMatrix(a,wfsSpecific->mp,wfsSpecific->np);
    
    wfsSpecific->framesizeflag = tempflag;
    wfsSpecific->xframesize = tempxsize;
    wfsSpecific->yframesize = tempysize;
    ospReadNulls(wfsSpecific->nullfile,wfsSpecific);
    ospCalculateSubaps(wfsSpecific);

/**** Multiply by the matrix f[][](inverse) which takes account of the basis
***** set used to calculate the reconstructor. Post multiplying the 
***** reconstructor matrix with respect to the arbitrary basis set by the
***** matrix inverse of f[][] gives a reconstructor in terms of the
***** normalised Zernike polynomials, and has the benefit that later
***** the variances are calculated wrt to the correct basis set, and that
***** uncertainties in the determination of the basis functions should be 
***** accommodated in the estimate of fitting error.  */

    svdcmp(f,wfsSpecific->np,wfsSpecific->np,w,v);     /* First calculate the inverse */
    ospCalculateInverse(finv,f,v,w,wfsSpecific->np,wfsSpecific->np);
    printf("Matrix finv:\n");
    ospShowMatrix(finv,wfsSpecific->np,wfsSpecific->np);

    ospMatrixProduct(u,a,finv, wfsSpecific->mp,wfsSpecific->np,wfsSpecific->np);    /* The matrix product */

    for(i=1;i<=wfsSpecific->mp; i++)     /* Set a[][] equal to u[][] */
    {
	for(j=1;j<=wfsSpecific->np;j++)
	{
	    a[i][j] = u[i][j];
	}
    }

    printf("Showing u: \n");
    ospShowMatrix(u,wfsSpecific->mp,wfsSpecific->np);


/**** Invert the reconstructor matrix using svd to obtain the control 
***** matrix c[][] */

    svdcmp(u,wfsSpecific->mp,wfsSpecific->np,w,v);
    ospCalculateInverse(wfsSpecific->c,u,v,w,wfsSpecific->np,wfsSpecific->mp);

/**** Calculate and display the condition number of the matrix */

    ospConditionOfW(w,wfsSpecific->np);

/**** Display evidence that inverse has been calculated */
    ospMatrixProduct(ca,wfsSpecific->c,a,wfsSpecific->np,wfsSpecific->mp,wfsSpecific->np);
    printf("Product i = c * a:\n");

    ospShowMatrix(ca,wfsSpecific->np,wfsSpecific->np);

    strncpy(controltempname,wfsSpecific->controlfile,OSP_MAXSTR-5);
    strncat(controltempname,".bak",OSP_MAXSTR-1);
    if((remove(controltempname))!=0)
    {
	fprintf(stderr,"remove failed for %s in ospCalibrate\n",
		controltempname);
	ospstatus=ERROR;
    }
    if((rename(wfsSpecific->controlfile,controltempname))!=0)
    {
	fprintf(stderr,"rename failed for %s in ospCalibrate\n",
		wfsSpecific->controlfile);
	ospstatus=ERROR;
    }
    ospWriteMatrixToFile(wfsSpecific->controlfile,wfsSpecific->c,
			 wfsSpecific->np, wfsSpecific->mp);

    ospFitVars(v,w,wfsSpecific);

    strncpy(fvarstempname,wfsSpecific->fvarsfile,OSP_MAXSTR-5);
    strncat(fvarstempname,".bak",OSP_MAXSTR-1);
    if((remove(fvarstempname))!=0)
    {
	fprintf(stderr,"remove failed for %s in ospCalibrate\n",
		fvarstempname);
	ospstatus=ERROR;
    }
    if((rename(wfsSpecific->fvarsfile,fvarstempname))!=0)
    {
	fprintf(stderr,"rename failed for %s in ospCalibrate\n",
		wfsSpecific->fvarsfile);
	ospstatus=ERROR;
    }
    ospWriteVectorToFile(wfsSpecific->fvarsfile,wfsSpecific->fvars,wfsSpecific->np);
    
    freeVector(w,1,wfsSpecific->np);
    freeMatrix(a,1,wfsSpecific->mp,1,wfsSpecific->np);
    freeMatrix(u,1,wfsSpecific->mp,1,wfsSpecific->np);
    freeMatrix(v,1,wfsSpecific->np,1,wfsSpecific->np);
    freeMatrix(f,1,wfsSpecific->np,1,wfsSpecific->np);
    freeMatrix(ca,1,wfsSpecific->np,1,wfsSpecific->np);
    freeMatrix(finv,1,wfsSpecific->np,1,wfsSpecific->mp);

    return(ospstatus);	
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospMeasure
 *
 * INVOCATION:
 * ospMeasure(buffp,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) buffp        (float *) input frame buffer
 * (!) wfsSpecific  (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To accept an input frame of SH spots and calculate the Zernike coefficients
 *
 * DESCRIPTION:
 * The subtractive and multiplicative offset frames are applied to the input
 * pointed to by buffp. Thresholding and centroiding then takes place through
 * a call to ospCentroidWrapper, and the Zernike coefficients are calculated
 * and stored in the wfs context structure. Measurement variances are calculated
 * and added to the fitting variances and the square root taken to give the
 * error estimates for the Zernike coefficients, which are also stored in the 
 * wfs context structure.
 * NB The input buffer, on exit has had the subtractive and multiplicative 
 * offsets applied, and has been thresholded.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * Need to check the return values of the functions called.
 *-
 */
int /*STATUS*/ ospMeasure(float * buffp, struct OSP_CONTEXT * wfsSpecific)
{
    int i;
    ospSubtractFrameFromFrame(buffp, wfsSpecific->redsubbuff, wfsSpecific);
    ospMultiplyFrameByFrame(buffp, wfsSpecific->redmultbuff, wfsSpecific);
    ospCentroidWrapper(buffp,wfsSpecific);
    ospApplyControlMatrix(wfsSpecific->c,wfsSpecific->z,wfsSpecific->s,
			  wfsSpecific->np,wfsSpecific->mp); 
    wfsSpecific->z[1] *= wfsSpecific->tipscale;
    wfsSpecific->z[2] *= wfsSpecific->tiltscale;
    wfsSpecific->z[3] *= wfsSpecific->focusscale;

    ospMeasVars(wfsSpecific);
    for (i=1;i<=wfsSpecific->np;i++)
    {
	wfsSpecific->err[i]=sqrt((wfsSpecific->fvars[i]+wfsSpecific->mvars[i])/
				 (wfsSpecific->coaddcounter + 1));
    }
/*
 *  May wish to divide this value by sqrt(wfsSpecific->coaddcounter+1) to
 *  obtain the standard error- only different for the coadded frame.
 *  13/1/1999: this has been done.
 */

/*    ospShowVector(wfsSpecific->z,wfsSpecific->np); */

#ifdef vxWorks
    if ( timeNow (&(wfsSpecific->time)) != OK )
    {
    	fprintf (stderr,
	         "Error: ospMeasure failed to take bancom time\n" ) ;
    } ;

    writeWfsToTcs(wfsSpecific);
#endif /*vxWorks*/

    return(OK);
}


/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospInitHr
 *
 * INVOCATION:
 * hrwfsSpecific = ospInitHr(hrwfsName)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) hrwfsName (char *)            name of the .ini file to be read
 *
 * FUNCTION VALUE:
 * (struct OSP_HRCONTEXT *) pointer to hrwfs context structure
 *
 * PURPOSE:
 * To create and initialise a HRwfs context structure
 *
 * DESCRIPTION:
 * A hrwfs context structure is created, with its pointer the return value of
 * the function. The .ini file named as hrwfsName is read, and the values read 
 * in either directly initialise elements of the context structure, or are 
 * names of files which in turn are read to initialise elements. The
 * context structure is/will be documented in the osp.h header file.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */

struct OSP_HRCONTEXT * ospInitHr ( char * hrwfsName )
{
    FILE *fp;
    char subfile[OSP_MAXSTR];
    char dummy[OSP_MAXSTR];
    char name[OSP_MAXSTR];

    int i;
    int ixarraysize;
    int iyarraysize;
    int status=OK;
    struct OSP_HRCONTEXT * hrwfsSpecific;

    if((hrwfsSpecific=(struct OSP_HRCONTEXT *) malloc (sizeof (struct OSP_HRCONTEXT)))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for HR context structure\n");
	fprintf(stderr,"-didn't get as far as opening %s for reading\n",name);
	fprintf(stderr,"...fatal error in ospInitHr:\n");
	return(NULL);
    }
	
    strncpy(name,hrwfsName, OSP_MAXSTR-1);

    if((fp = fopen(name,"r"))==NULL)
	{
	    printf("Error in opening HRWFS ini file %s for reading\n",name);
	    printf("during function ospInitHr\n");
	    return(NULL);
	}

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }

    if((fgets(subfile,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading subfile name\n");
    }
    i=0;
    while(subfile[i] != ' ' && subfile[i] != '\n' && subfile[i] != '\0'
          && i < OSP_MAXSTR)
    {
	i++;
    }
    subfile[i] ='\0';
    strncpy(hrwfsSpecific->subfile,subfile,OSP_MAXSTR);


    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }

    if((fscanf(fp,"%d\n",&ixarraysize))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading xarraysize\n");
    }
    hrwfsSpecific->xarraysize = ixarraysize; 

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&iyarraysize))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading yarraysize\n");
    }
    hrwfsSpecific->yarraysize = iyarraysize; 

    hrwfsSpecific->buffsize = hrwfsSpecific->xarraysize * hrwfsSpecific->yarraysize;
    if(hrwfsSpecific->buffsize > OSP_HRBUFFMAX)
    {
	status = ERROR;
	fprintf(stderr,"buffsize set greater than OSP_HRBUFFMAX\n");
    }

/******Allocate memory and initialise vector**********/


    if((hrwfsSpecific->ffsubbuff=(float *) malloc(
	hrwfsSpecific->buffsize*sizeof (float)))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for ffsubbuff\n");
	fprintf(stderr,"...fatal error in ospInitHr:\n");
	return(NULL);
    }
    if(ospReadFloatImage(hrwfsSpecific->ffsubbuff,hrwfsSpecific->subfile,
			 hrwfsSpecific->buffsize) ==ERROR)
    {
	fprintf(stderr,
		"Failed to read %s, the FITS file of subtractive offsets\n",
		hrwfsSpecific->subfile);
	fprintf(stderr,
		"- initialising with frame of zeroes, and writing to file\n");
	for(i=0;i<hrwfsSpecific->buffsize; i++)
	    hrwfsSpecific->ffsubbuff[i]=0.0;
	/*ospWriteFloatImage(hrwfsSpecific->ffsubbuff,hrwfsSpecific->subfile,
			   hrwfsSpecific->xarraysize,hrwfsSpecific->yarraysize);*/
	status=ERROR;
    }
    else
    {
	fprintf(stdout,"Reading %s, the FITS file of subtractive offsets\n",
		hrwfsSpecific->subfile);
    }

    return(hrwfsSpecific);
}

/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospInit
 *
 * INVOCATION:
 * wfsSpecific = ospInit(wfsName,ospGeom)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) wfsName (char *)            name of the .ini file to be read
 * (>) ospGeom (struct OSP_GEOMETRY *) pointer to ccd readout geometry structure
 *
 * FUNCTION VALUE:
 * (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * PURPOSE:
 * To create and initialise a wfs context structure
 *
 * DESCRIPTION:
 * A wfs context structure is created, with its pointer the return value of
 * the function. The .ini file named as wfsName is read, and the values read 
 * in either directly initialise elements of the context structure, or are 
 * names of files which in turn are read to initialise elements. The
 * context structure is/will be documented in the osp.h header file, but 
 * includes the ccd readout geometry, the null positions of the SH spots,
 * the control matrix, frames of subtractive
 * and multiplicative offsets which are to be applied to subsequent frames
 * of SH spots, measured spot displacements, fitting and measurement 
 * variances, and ultimately, calculated Zernike coefficients. The values
 * for the ccd readout geometry read in from the .ini file are also stored
 * in the structure as default values which will be reverted to should
 * ospChangeGeometry be called with the framesizeflag element of its argument
 * equal to 1 (full frame). This behaviour should perhaps be modified as it 
 * limits us to a single fullframe readout geometry, unless we modify the .ini
 * file and call ospInit again. The ospGeom structure allows the ccd readout
 * geometry to be modified for reduced frames on calling ospInit, but as 
 * this is carried out by ospChangeGeometry the same restriction on 
 * the full frame readout using the default values applies. Full frame versions
 * of the offset frames are stored in the wfs context structure, from which 
 * any versions for reduced frame readout may be calculated, and also stored
 * in the wfs context structure.
 * If no ospGeom structure modifying the readout geometry is necessary on
 * initialisation, supply NULL instead.
 * Much memory allocation takes place during this function- remember to use the
 * companion function ospTidyUp(wfsSpecific) to deallocate the memory and 
 * destroy the pointer to the structure.
 * The pointer to the ospGeom structure is freed separately.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
struct OSP_CONTEXT * ospInit(char * wfsName, struct OSP_GEOMETRY * ospGeom)
/* read constants from file */
{
    FILE *fp;
    char nullfile[OSP_MAXSTR];
    char subfile[OSP_MAXSTR];
    char multfile[OSP_MAXSTR];
    char controlfile[OSP_MAXSTR];
    char fvarsfile[OSP_MAXSTR];
    char dummy[OSP_MAXSTR];
    char name[OSP_MAXSTR];

    int i;
    int ii;
    int jj;
    int inp;
    int imp;
    int iwfsSource;
    int iwfsMode;
    int isectors;
    int ixarraysize;
    int iyarraysize;
    int ixstart;
    int iystart;
    int ixbin;
    int iybin;
    int ixraster;
    int iyraster;
    int ixspace;
    int iyspace;
    int ixsubap;
    int iysubap;
    int iframesizeflag;
    float insigma;
    float ireadsq;
    int iweight;
    float ithresh;
    float itipscale;
    float itiltscale;
    float ifocusscale;
    float iangle;
    float ixcenter;
    float iycenter;
    int status=OK;
    struct OSP_CONTEXT * wfsSpecific;

    if((wfsSpecific=(struct OSP_CONTEXT *) malloc (sizeof (struct OSP_CONTEXT)))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for context structure\n");
	fprintf(stderr,"-didn't get as far as opening %s for reading\n",name);
	fprintf(stderr,"...fatal error in ospInit:\n");
	return(NULL);
    }
	

    strncpy(name,wfsName, OSP_MAXSTR-1);

    if((fp = fopen(name,"r"))==NULL)
	{
	    printf("Error in opening WFS ini file %s for reading\n",name);
	    printf("during function ospInit\n");
	    return(NULL);
	}

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&iwfsSource))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading wfsSource\n");
    }
    wfsSpecific->wfsSource = iwfsSource;

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&iwfsMode))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading wfsMode\n");
    }
    wfsSpecific->wfsMode = iwfsMode;

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fgets(nullfile,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading nullfile name\n");
    }
    i=0;
    while(nullfile[i] != ' ' && nullfile[i] != '\n' && nullfile[i] != '\0' 
          && i < OSP_MAXSTR)
    {
	i++;
    }
    nullfile[i] ='\0';
    strncpy(wfsSpecific->nullfile,nullfile,OSP_MAXSTR);


    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fgets(subfile,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading subfile name\n");
    }
    i=0;
    while(subfile[i] != ' ' && subfile[i] != '\n' && subfile[i] != '\0'
          && i < OSP_MAXSTR)
    {
	i++;
    }
    subfile[i] ='\0';
    strncpy(wfsSpecific->subfile,subfile,OSP_MAXSTR);


    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fgets(multfile,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading multfile name\n");
    }
    i=0;
    while(multfile[i] != ' ' && multfile[i] != '\n' && multfile[i] !='\0'
          && i < OSP_MAXSTR)
    {
	i++;
    }
    multfile[i] ='\0';
    strncpy(wfsSpecific->multfile,multfile,OSP_MAXSTR);


    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fgets(controlfile,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading control matrix filename\n");
    }
    i=0;
    while(controlfile[i] != ' ' && controlfile[i] != '\n' 
	  && controlfile[i] !='\0'  && i < OSP_MAXSTR)
    {
	i++;
    }
    controlfile[i] ='\0';
    strncpy(wfsSpecific->controlfile,controlfile,OSP_MAXSTR);


    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fgets(fvarsfile,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading fvars filename\n");
    }
    i=0;
    while(fvarsfile[i] != ' ' && fvarsfile[i] != '\n' 
	  && fvarsfile[i] !='\0'  && i < OSP_MAXSTR)
    {
	i++;
    }
    fvarsfile[i] ='\0';
    strncpy(wfsSpecific->fvarsfile,fvarsfile,OSP_MAXSTR);

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&inp))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading np\n");
    }
    wfsSpecific->np = inp; 

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&imp))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading mp\n");
    }
    wfsSpecific->mp = imp; 

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&ixarraysize))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading xarraysize\n");
    }
    wfsSpecific->xarraysize = ixarraysize; 

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&iyarraysize))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading yarraysize\n");
    }
    wfsSpecific->yarraysize = iyarraysize; 

    wfsSpecific->buffsize = wfsSpecific->xarraysize * wfsSpecific->yarraysize;
    if(wfsSpecific->buffsize > OSP_BUFFMAX)
    {
	status = ERROR;
	fprintf(stderr,"buffsize set greater than OSP_BUFFMAX\n");
    }

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&ixstart))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading ospxstart\n");
    }
    wfsSpecific->defxstart = ixstart;
    wfsSpecific->ospxstart = ixstart;

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&iystart))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading ospystart\n");
    }
    wfsSpecific->defystart = iystart;
    wfsSpecific->ospystart = iystart;

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&ixbin))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading ospxbin\n");
    }
    wfsSpecific->defxbin = ixbin;
    wfsSpecific->ospxbin = ixbin;

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&iybin))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading ospybin\n");
    }
    wfsSpecific->defybin = iybin;
    wfsSpecific->ospybin = iybin;

    if(ixbin != 1 || iybin != 1)
    {
	fprintf(stderr,"Binning not yet implemented: xbin and ybin set to 1\n");
	wfsSpecific->defxbin = 1;
	wfsSpecific->defybin = 1;
	wfsSpecific->ospxbin = 1;
	wfsSpecific->ospybin = 1;
	status=ERROR;
    }

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&ixraster))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading ospxraster\n");
    }
    wfsSpecific->defxraster = ixraster;
    wfsSpecific->ospxraster = ixraster;

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&iyraster))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading ospyraster\n");
    }
    wfsSpecific->defyraster = iyraster;
    wfsSpecific->ospyraster = iyraster;

    if(ixraster != iyraster)
    {
	fprintf(stderr,"yraster assumed equal to xraster\n");
	wfsSpecific->defyraster= ixraster;
	wfsSpecific->ospyraster= ixraster;
	status=ERROR;
    }
    
    wfsSpecific->side = ixraster*wfsSpecific->ospxbin; 
	   

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&ixspace))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading ospxspace\n");
    }
    wfsSpecific->defxspace = ixspace;
    wfsSpecific->ospxspace = ixspace;

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&iyspace))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading ospyspace\n");
    }
    wfsSpecific->defyspace = iyspace;
    wfsSpecific->ospyspace = iyspace;

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&isectors))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading sectors\n");
    }
    wfsSpecific->sectors = isectors; 

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&ixsubap))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading ospxsubap\n");
    }
    wfsSpecific->defxsubap = ixsubap;
    wfsSpecific->ospxsubap = ixsubap;

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&iysubap))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading ospysubap\n");
    }
    wfsSpecific->defysubap = iysubap;
    wfsSpecific->ospysubap = iysubap;

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&iframesizeflag))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading framesizeflag\n");
    }
    if(iframesizeflag != 0 && iframesizeflag != 1)
    {
       status = ERROR;
       fprintf(stderr,"Invalid value (%d) entered for framesizeflag:\n",
	       iframesizeflag);
       fprintf(stderr,"setting framesizeflag to 1\n");
       iframesizeflag = 1;
    }
    wfsSpecific->framesizeflag = iframesizeflag;

    if(wfsSpecific->framesizeflag==1)
    {
	wfsSpecific->xframesize=wfsSpecific->xarraysize;
	wfsSpecific->yframesize=wfsSpecific->yarraysize;
    }
    else
    {
	wfsSpecific->xframesize=(wfsSpecific->ospxsubap * 2 *
				 wfsSpecific->ospxraster*wfsSpecific->ospxbin);

	/* SBH version - 23 Jan 1999. This appears to be more general. Does it work? */
	wfsSpecific->yframesize=(wfsSpecific->ospysubap * wfsSpecific->sectors/2
				 *wfsSpecific->ospyraster * 
				 wfsSpecific->ospybin);
    }


    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%f\n",&insigma))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading nsigma\n");
    }
    wfsSpecific->nsigma = insigma; 

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%f\n",&ireadsq))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading readsq\n");
    }
    wfsSpecific->readsq = ireadsq; 

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%d\n",&iweight))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading weight\n");
    }
    wfsSpecific->weight = iweight; 

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%f\n",&ithresh))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading thresh\n");
    }
    wfsSpecific->thresh = ithresh; 

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%f\n",&itipscale))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading tipscale\n");
    }
    wfsSpecific->tipscale = itipscale; 
    wfsSpecific->tipCor = 1.0; 
    printf ( "tipCor = %f\n" , wfsSpecific->tipCor ) ;

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%f\n",&itiltscale))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading tiltscale\n");
    }
    wfsSpecific->tiltscale = itiltscale; 
    wfsSpecific->tiltCor = 1.0; 
    printf ( "tiltCor = %f\n" , wfsSpecific->tiltCor ) ;

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%f\n",&ifocusscale))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading focusscale\n");
    }
    wfsSpecific->focusscale = ifocusscale; 

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%f\n",&iangle))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading angle\n");
        iangle = 0.0 ;
    }
    wfsSpecific->angle = (double)(iangle); 

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%f\n",&ixcenter))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading xcenter\n");
        ixcenter = 40.5 ;
    }
    wfsSpecific->xcenter = (double)(ixcenter); 

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading comments\n");
    }
    if((fscanf(fp,"%f\n",&iycenter))==EOF)
    {
	status=ERROR; 
	fprintf(stderr,"Error reading xcenter\n");
        iycenter = 40.5 ;
    }
    wfsSpecific->ycenter = (double)(iycenter); 

    fclose(fp);
    

/******Allocate memory and initialise matrix and vectors**********/

    if((wfsSpecific->sumbuff=(float *) malloc(
	wfsSpecific->buffsize*sizeof (float)))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for sumbuff\n");
	fprintf(stderr,"...fatal error in ospInit:\n");
	return(NULL);
    }
    for(i=0; i<wfsSpecific->buffsize; i++)
	wfsSpecific->sumbuff[i] = 0.0;    
    wfsSpecific->coaddcounter=0;

    if((wfsSpecific->ffsubbuff=(float *) malloc(
	wfsSpecific->buffsize*sizeof (float)))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for ffsubbuff\n");
	fprintf(stderr,"...fatal error in ospInit:\n");
	return(NULL);
    }
    if(ospReadFloatImage(wfsSpecific->ffsubbuff,wfsSpecific->subfile,
			 wfsSpecific->buffsize) ==ERROR)
    {
	fprintf(stderr,
		"Failed to read %s, the FITS file of subtractive offsets\n",
		wfsSpecific->subfile);
	fprintf(stderr,
		"- initialising with frame of zeroes, and writing to file\n");
	for(i=0;i<wfsSpecific->buffsize; i++)
	    wfsSpecific->ffsubbuff[i]=0.0;
	ospWriteFloatImage(wfsSpecific->ffsubbuff,wfsSpecific->subfile,
			   wfsSpecific->xarraysize,wfsSpecific->yarraysize);
	status=ERROR;
    }
    else
    {
	fprintf(stdout,"Reading %s, the FITS file of subtractive offsets\n",
		wfsSpecific->subfile);
    }

    if((wfsSpecific->redsubbuff=(float *) malloc(
	wfsSpecific->buffsize*sizeof (float)))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for redsubbuff\n");
	fprintf(stderr,"...fatal error in ospInit:\n");
	return(NULL);
    }
    if(wfsSpecific->framesizeflag==1)
    {
	for(i=0;i<wfsSpecific->buffsize;i++)
	    wfsSpecific->redsubbuff[i] = wfsSpecific->ffsubbuff[i];
    }
    else
	if(ospReduceFrame(wfsSpecific->ffsubbuff, wfsSpecific->redsubbuff, 
			  wfsSpecific, 
			  wfsSpecific->xframesize*wfsSpecific->yframesize)
	   ==ERROR)
	{
	    fprintf(stderr,"ospReduceFrame failed for subtractive offsets\n");
	    status=ERROR;
	}


    if((wfsSpecific->ffmultbuff=(float *) malloc(
	wfsSpecific->buffsize*sizeof (float)))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for ffmultbuff\n");
	fprintf(stderr,"...fatal error in ospInit:\n");
	return(NULL);
    }
    if(ospReadFloatImage(wfsSpecific->ffmultbuff,wfsSpecific->multfile,
			 wfsSpecific->buffsize) ==ERROR)
    {
	fprintf(stderr,
		"Failed to read %s, the FITS file of multiplicative offsets\n",
		wfsSpecific->multfile);
	fprintf(stderr,
		"- initialising with frame of ones, and writing to file\n");
	for(i=0;i<wfsSpecific->buffsize; i++)
	    wfsSpecific->ffmultbuff[i]=1.0;
	ospWriteFloatImage(wfsSpecific->ffmultbuff,wfsSpecific->multfile,
			   wfsSpecific->xarraysize,wfsSpecific->yarraysize);
	status=ERROR;
    }
    else
    {
	fprintf(stdout,"Reading %s, the FITS file of multiplicative offsets\n",
		wfsSpecific->multfile);
    }

    if((wfsSpecific->redmultbuff=(float *) malloc(
	wfsSpecific->buffsize*sizeof (float)))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for redmultbuff\n");
	fprintf(stderr,"...fatal error in ospInit:\n");
	return(NULL);
    }
    if(wfsSpecific->framesizeflag==1)
    {
	for(i=0;i<wfsSpecific->buffsize;i++)
	    wfsSpecific->redmultbuff[i] = wfsSpecific->ffmultbuff[i];
    }
    else
	if(ospReduceFrame(wfsSpecific->ffmultbuff, wfsSpecific->redmultbuff, 
			  wfsSpecific, 
			  wfsSpecific->xframesize*wfsSpecific->yframesize)
	   ==ERROR)
	{
	    fprintf(stderr,
		    "ospReduceFrame failed for multiplicative offsets\n");
	    status=ERROR;
	}


    if((wfsSpecific->c=matrix(1,OSP_ZMAX,1,2*OSP_SUBAPSMAX))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for matrix c\n");
	fprintf(stderr,"...fatal error in ospInit:\n");
	return(NULL);
    }
    for(jj=1; jj<=2*OSP_SUBAPSMAX; jj++)
    {
	for(ii=1;ii<=OSP_ZMAX; ii++)
	{
	    wfsSpecific->c[ii][jj] = 0.0;
	}
    }

    if((wfsSpecific->err=vector(1,OSP_ZMAX))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for vector err\n");
	fprintf(stderr,"...fatal error in ospInit:\n");
	return(NULL);
    }
    for (ii=1;ii<=OSP_ZMAX;ii++)
	wfsSpecific->err[ii]=99.0;


    if((wfsSpecific->fvars=vector(1,OSP_ZMAX))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for vector fvars\n");
	fprintf(stderr,"...fatal error in ospInit:\n");
	return(NULL);
    }
    for (ii=1;ii<=OSP_ZMAX;ii++)
	wfsSpecific->fvars[ii]=99.0;

    if((wfsSpecific->s=vector(1,2*OSP_SUBAPSMAX))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for vector s\n");
	fprintf(stderr,"...fatal error in ospInit:\n");
	return(NULL);
    }
    for (ii=1;ii<=2*OSP_SUBAPSMAX;ii++)
	wfsSpecific->s[ii]=0.0;

    if((wfsSpecific->dssq=vector(1,2*OSP_SUBAPSMAX))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for vector dssq\n");
	fprintf(stderr,"...fatal error in ospInit:\n");
	return(NULL);
    }
    for (ii=1;ii<=2*OSP_SUBAPSMAX;ii++)
	wfsSpecific->dssq[ii]=99.0;

    if((wfsSpecific->mvars=vector(1,OSP_ZMAX))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for vector mvars\n");
	fprintf(stderr,"...fatal error in ospInit:\n");
	return(NULL);
    }
    for (ii=1;ii<=OSP_ZMAX;ii++)
	wfsSpecific->mvars[ii]=99.0;

    if((wfsSpecific->z=vector(1,OSP_ZMAX))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for vector z\n");
	fprintf(stderr,"...fatal error in ospInit:\n");
	return(NULL);
    }
    for (ii=1;ii<=OSP_ZMAX;ii++)
	wfsSpecific->z[ii]=0.0;

    if((ospReadNulls(wfsSpecific->nullfile,wfsSpecific))== ERROR)
    {
	fprintf(stderr,"Error in call to ospReadNulls during ospInit:\n");
	fprintf(stderr,"...filename used was %s\n",wfsSpecific->nullfile);
	status=ERROR;
    }

    if((ospCalculateSubaps(wfsSpecific))== ERROR)
    {
	fprintf(stderr,"Error in ospCalculateSubaps\n");
	status=ERROR;
    }

    if(wfsSpecific->centres[0]==0) 
    {
	fprintf(stderr,"Apparently no centres data to be read in %s !\n", 
		wfsSpecific->nullfile);
	status =ERROR;
    }
    if(((int)(wfsSpecific->centres[0]) % 4) != 0)
    {
	fprintf(stderr,"Incomplete centres data read from %s !\n", 
		wfsSpecific->nullfile);
	status =ERROR;
    }


    if((ospReadMatrixFromFile(wfsSpecific->controlfile, wfsSpecific->c))==ERROR)
    {
	fprintf(stderr,"Call to ospReadMatrixFromFile failed:\n");
	fprintf(stderr,"...filename used was %s\n",wfsSpecific->controlfile);
	status=ERROR;
    }

    if((ospReadVectorFromFile(wfsSpecific->fvarsfile,wfsSpecific->fvars,1,
	wfsSpecific->np,1))==ERROR)
    {
	fprintf(stderr,"Call to ospReadVectorFromFile failed:\n");
	fprintf(stderr,"...filename used was %s\n",wfsSpecific->fvarsfile);
	status=ERROR;
    }


    if(status==ERROR)
	{
	    fprintf(stderr,"...error(s) took place in ospInit that\n");
	    fprintf(stderr,"indicate initialisation data is incomplete.\n");
	}	    

    if(ospGeom==NULL)
    {
	fprintf(stderr,"WARNING: Pointer to struct OSP_GEOMETRY, ospGeom,\n");
	fprintf(stderr,"is null, osp geometric parameters unchanged\n");
	fprintf(stderr,"... took place in ospInit.\n");
    }
    else
    {
	if(ospChangeGeometry(ospGeom,wfsSpecific)==ERROR)
	{
	    fprintf(stderr,"Error in ospChangeGeometry\n");
	    status = ERROR;
	}
    }

#ifdef vxWorks

    /* write structure address to global array for access by EPICS */

    /* check that wfs number is in range */

    if(wfsSpecific->wfsSource >= HRWFS && wfsSpecific->wfsSource <= AOWFS)
    {

	/* check whether new structure is for fast guide or ao data */

	if(wfsSpecific->wfsMode == AO)
	{
	    wfsAoAddr[wfsSpecific->wfsSource] = wfsSpecific;
	}
	else
	{
	    wfsFgAddr[wfsSpecific->wfsSource] = wfsSpecific;
	}
    }

    /* initialise guard fields of the ospdiag array */

    wfsSpecific->ospdiag[GUARD1] = 0.0;
    wfsSpecific->ospdiag[GUARD2] = 0.0;

#endif

#ifdef OSP_VERBOSE
    ospShow(wfsSpecific);
#endif /*OSP_VERBOSE*/

    return(wfsSpecific);
}

/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospSaveNullPositions
 *
 * INVOCATION:
 * ospSaveNullPositions(nullname,outfile, wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) nullname   (char *) name of full frame FITS image from which null 
 *                         positions are to be read
 * (>) oflag      (int)    determines whether offsets are to be applied
 *                         OFFSET_CORRECTION = 1, NO_OFFSET_CORRECTION=0
 * (>) outfile    (char *) name of modified nullfile
 * (!) wfsSpecific (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To create or update null positions data from a calibration file
 *
 * DESCRIPTION:
 * The calibration frame is read from a FITS file, with the NAXIS1 and NAXIS2
 * keyword values read to ensure that we are dealing with full frame data. 
 * Centroiding is carried out to determine the centroid positions, which here
 * are corrections to the current null positions. These corrections are 
 * added to the current null values and written out to a file.
 * We use a 'null' file of x and y coordinates relative to the bottom 
 * left hand corner of the whole array, which is read in and the bottom
 * left corner positions of the subapertures generated from knowledge of
 * the detector geomemtry, and the corresponding xoff, yoff values calculated.
 * As the subaperture positions are calculated in order from left to right
 * and from top to bottom with no prior knowledge of which subapertures are 
 * unused, unused subapertures are indicated with a null position of -1,-1.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * none
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * Should save previous nullfile before overwriting, just in case.
 *
 *-
 */
int /*STATUS*/ ospSaveNullPositions(char * nullname,int oflag,char * outfile, 
				    struct OSP_CONTEXT * wfsSpecific)
{
    int ii,jj;                      /* counter */
    FILE * fp;                   /* pointer to FILE structure */
  
    float * buffp;                /* ptr to buffer for input float image */
    float temp1,temp2;
    int ospstatus=OK;             /* overall status flag for function */
    int xframesize;               /* xframesize of input image, in pixels */
    int yframesize;               /* yframesize of input image, in pixels */
    fitsfile * fptr;              /* pointer to fitsfile structure */
    int status=0;                 /* status flag relevant to fitsio functions*/
    char comment[80];       /* buffer for comments string read from FITS file*/
    char tempname[OSP_MAXSTR];

    if(fits_open_file(&fptr,nullname,1,&status))
    {
	ospPrintError( status );    
	fprintf(stderr,"...error took place in ospSaveNullPositions\n");
	return(ERROR);
    }
    if(fits_read_key(fptr,TINT,"NAXIS1",&xframesize,comment,&status))
    {
	ospstatus=ERROR;
	fprintf(stderr,"fits_read_key failed for NAXIS1 in ");
	fprintf(stderr,"ospSaveNullPositions\n");
    }
    if(fits_read_key(fptr,TINT,"NAXIS2",&yframesize,comment,&status))
    {
	ospstatus=ERROR;
	fprintf(stderr,"fits_read_key failed for NAXIS2 in ");
	fprintf(stderr,"ospSaveNullPositions\n");
    }
    if(fits_close_file(fptr,&status))
	ospstatus=ospPrintError( status );

    if(xframesize!=wfsSpecific->xarraysize)
    {
	fprintf(stderr,"Warning: NAXIS1 (= %d) from %s ",xframesize,nullname);
	fprintf(stderr,"in ospSaveNullPositions\n");
	fprintf(stderr,
		"is inconsistent with xarraysize in context structure\n");
	fprintf(stderr,"(full frame image assumed for calibration)\n");
    }
    if(yframesize!=wfsSpecific->yarraysize)
    {
	fprintf(stderr,"Warning: NAXIS2 (= %d) from %s ",yframesize,nullname);
 	fprintf(stderr,"in ospSaveNullPositions\n");
	fprintf(stderr,
		"is inconsistent with yarraysize in context structure\n");
	fprintf(stderr,"(full frame image assumed for calibration)\n");
    }

    if((buffp = (float *)malloc((size_t)(xframesize*yframesize)*sizeof(float)))==NULL)
    {
	fprintf(stderr,"Failed to allocate memory for buffp ");
	fprintf(stderr,"in ospSaveNullPositions.\n");
	return(ERROR);
    }

    if((ospReadFloatImage(buffp,nullname,xframesize*yframesize))==ERROR)
    {
        fprintf(stderr,"ospReadFloatImage failed in ospSaveNullPositions\n");
        ospstatus=ERROR;
    }   

    if(oflag==OFFSET_CORRECTION)
    {
	ospSubtractFrameFromFrame(buffp, wfsSpecific->ffsubbuff, wfsSpecific);
	ospMultiplyFrameByFrame(buffp, wfsSpecific->ffmultbuff, wfsSpecific);
    }

    ospCentroidWrapper(buffp,wfsSpecific);

    free(buffp);

    strncpy(tempname,outfile,OSP_MAXSTR-5);
    strncat(tempname,".bak",OSP_MAXSTR-1);
    /*remove(tempname);*/
    /*if((rename(wfsSpecific->nullfile, tempname))!=0)
	fprintf(stderr, "Warning: backup of existing %s as %s failed in ospCalibrate\n", wfsSpecific->nullfile,tempname);*/
    strcpy ( wfsSpecific->nullfile, tempname ) ;

    printf ( "New null file name : wfsSpecific->nullfile\n" ) ;
    /*else
    {*/
	printf("Backing up existing %s as %s \n", 
	       wfsSpecific->nullfile,tempname);
	if((fp = fopen(outfile,"w"))==NULL)
	{
	    fprintf(stderr,"Failed to open %s for writing ",outfile);
	    fprintf(stderr,"in ospSaveNullPositions.\n");
	    ospstatus = ERROR;
	}
	else
	{
	    printf("Recalculated null positions follow.\n");
	    printf("----------------------------------\n");
	    
	    jj=0;
	    for(ii=0; ii< (wfsSpecific->ospxsubap*wfsSpecific->ospysubap
			   *wfsSpecific->sectors*2);ii=ii+2)
	    {
		if(wfsSpecific->nulls[ii] >= 0)
		{
		    temp1=wfsSpecific->nulls[ii]+wfsSpecific->s[jj+1];
		    temp2=wfsSpecific->nulls[ii+1]+wfsSpecific->s[jj+2];
		    jj=jj+2;
		}
		else
		{
		    temp1=wfsSpecific->nulls[ii];
		    temp2=wfsSpecific->nulls[ii+1];
		}
		printf("%f %f\n",temp1,temp2);
		fprintf(fp,"%f %f\n",temp1,temp2);
	    }
	    printf("\n");
	    printf("Recalculated null positions written to %s, and applied here.\n",outfile);
	    fclose(fp);
	}
    /*}*/

    return(ospstatus);
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospAddFrameToFrame
 *
 * INVOCATION:
 * ospAddFrameToFrame(buffp1,buffp2,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) buffp1        (float *)  pointer to buffer to which frame is to be added
 * (>) buffp2        (float *)  pointer to buffer containing frame to be added
 * (>) wfsSpecific   (struct OSP_CONTEXT *)  pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To add a frame to an existing frame of the same size, pixel by pixel
 *
 * DESCRIPTION:
 * Adds a frame pointed to by buffp2 to an existing frame buffer pointed to by 
 * buffp1. The frames are both assumed to be wfsSpecific->xframesize by
 * wfsSpecific->yframesize. The addition is done pixel by pixel. No checking of
 * the frame size is done, due to the function's intended use of making 
 * corrections to fast guide data frames, which requires low latency.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * That the pointers point to buffers which are already allocated and 
 * large enough.
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
int /*STATUS*/ ospAddFrameToFrame(float *buffp1, float *buffp2,
				  struct OSP_CONTEXT * wfsSpecific)
{
    float * p1, * p2, * maxp1;
    int buffSize;

    buffSize = wfsSpecific->xframesize * wfsSpecific->yframesize;
    maxp1 = (float *)((int)buffp1 + buffSize*sizeof(float));
    p2 = buffp2;

    for (p1 = buffp1; p1 < maxp1;)
    {
	*p1 = *(p1++) + *(p2++);
    }
    return(OK);
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospSubtractFrameFromFrame
 *
 * INVOCATION:
 * ospSubtractFrameFromFrame(buffp1,buffp2,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) buffp1  (float *)  pointer to buffer from which frame is to be subtracted
 * (>) buffp2  (float *)  pointer to buffer containing frame to be subtracted   
 * (>) wfsSpecific  (struct OSP_CONTEXT *)  pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To subtract a frame from an existing frame of the same size, pixel by pixel
 *
 * DESCRIPTION:
 * Subtracts a frame pointed to by buffp2 from an existing frame buffer
 * pointed to by buffp1.
 * The frames are both assumed to be wfsSpecific->xframesize by
 * wfsSpecific->yframesize. The subtraction is done pixel by pixel. No checking
 * of the frame size is done, due to the function's intended use of making 
 * corrections to fast guide data frames, which requires low latency.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * That the pointers point to buffers which are already allocated and 
 * large enough.
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
int /*STATUS*/ ospSubtractFrameFromFrame(float *buffp1, float *buffp2,
					 struct OSP_CONTEXT * wfsSpecific)
{
    float * p1, * p2, * maxp1;
    int buffSize;

    buffSize = wfsSpecific->xframesize * wfsSpecific->yframesize;
    maxp1 = (float *)((int)buffp1 + buffSize*sizeof(float));
    p2 = buffp2;

    for (p1 = buffp1; p1 < maxp1;)
    {
	*p1 = *(p1++) - *(p2++);
    }
    return(OK);
}

/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospNewSubtractFrameFromFrame
 *
 * INVOCATION:
 * ospNewSubtractFrameFromFrame(buffp1,buffp2,buffSize)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) buffp1  (float *)  pointer to buffer from which frame is to be subtracted
 * (>) buffp2  (float *)  pointer to buffer containing frame to be subtracted   
 * (>) buffSize (int)     buffers size
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To subtract a frame from an existing frame of the same size, pixel by pixel
 *
 * DESCRIPTION:
 * Subtracts a frame pointed to by buffp2 from an existing frame buffer
 * pointed to by buffp1. The frames are both assumed to be buffSize. 
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * That the pointers point to buffers which are already allocated and 
 * large enough.
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */

int ospNewSubtractFrameFromFrame ( float *buffp1, 
                                   float *buffp2,
		                   int buffSize )
{
    float * p1, * p2, * maxp1;

    maxp1 = (float *)((int)buffp1 + buffSize*sizeof(float));
    p2 = buffp2;

    for (p1 = buffp1; p1 < maxp1;)
    {
	*p1 = *(p1++) - *(p2++);
    }
    return(OK);
}



/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospMultiplyFrameByFrame
 *
 * INVOCATION:
 * ospMultiplyFrameByFrame(buffp1,buffp2,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) buffp1        (float *) pointer to existing frame which is to be 
 *                            modified by multiplication
 * (>) buffp2        (float *) pointer to frame by which existing frame
 *                            is to be multiplied
 * (>) wfsSpecific (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To multiply a frame by an existing frame of the same size, pixel by pixel
 *
 * DESCRIPTION:
 * Multiplies an existing frame buffer pointed to by buffp1 by another,
 * pointed to by buffp2.
 * The frames are both assumed to be wfsSpecific->xframesize by
 * wfsSpecific->yframesize. The multiplication is done pixel by pixel. 
 * No checking
 * of the frame size is done, due to the function's intended use of making 
 * corrections to fast guide data frames, which requires low latency.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * That the pointers point to buffers which are already allocated and 
 * large enough.
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
int /*STATUS*/ ospMultiplyFrameByFrame(float *buffp1, float *buffp2,
				       struct OSP_CONTEXT * wfsSpecific)
{
    float * p1, * p2, * maxp1;
    int buffSize;

    buffSize = wfsSpecific->xframesize * wfsSpecific->yframesize;
    maxp1 = (float *)((int)buffp1 + buffSize*sizeof(float));
    p2 = buffp2;

    for (p1 = buffp1; p1 < maxp1;)
    {
	*p1 = *(p1++) * *(p2++);
    }
    return(OK);
}
/*------------------------------------------------------------------------*/


/*
 *+
 * FUNCTION NAME:
 * ospAddConstantToFrame
 *
 * INVOCATION:
 * ospAddConstantToFrame(buffp1,constname,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) buffp1          (float *) pointer to buffer to be added to
 * (>) constname       (float)   additive constant
 * (>) wfsSpecific     (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To add to each pixel in a frame the same specified amount
 *
 * DESCRIPTION:
 * Adds to each pixel in an existing frame buffer pointed to by buffp1
 * the floating point value constname.
 * The frame is assumed to be wfsSpecific->xframesize by 
 * wfsSpecific->yframesize
 * No checking of the frame size is done, due to the function's intended use of
 * making corrections to fast guide data frames, which requires low latency.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * That the pointer points to a buffer which is already allocated and 
 * large enough.
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
int /*STATUS*/ ospAddConstantToFrame(float *buffp1, float constname,
				     struct OSP_CONTEXT * wfsSpecific)
{
    float * p1, * maxp1;
    int buffSize;

    buffSize = wfsSpecific->xframesize * wfsSpecific->yframesize;
    maxp1 = (float *)((int)buffp1 + buffSize*sizeof(float));

    for (p1 = buffp1; p1 < maxp1;)
    {
	*p1 = *(p1++) + constname;
    }
    return(OK);
}
/*------------------------------------------------------------------------*/


/* FUNCTION NAME:
 * ospMultiplyFrameByConstant
 *
 * INVOCATION:
 * ospMultiplyFrameByConstant(buffp1,constname,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) buffp1          (float *) pointer to buffer to be multiplied
 * (>) constname       (float)   multiplying constant
 * (>) wfsSpecific     (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To multiply each pixel in a frame by the same specified amount
 *
 * DESCRIPTION:
 * Multiplies each pixel in an existing frame buffer pointed to by buffp1
 * by the floating point value constname.
 * The frame is assumed to be wfsSpecific->xframesize by
 * wfsSpecific->yframesize. 
 * No checking of the frame size is done, due to the function's intended use of
 * making corrections to fast guide data frames, which requires low latency.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * That the pointer points to a buffer which is already allocated and 
 * large enough.
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
int /*STATUS*/ ospMultiplyFrameByConstant(float *buffp1, float constname,
				  struct OSP_CONTEXT * wfsSpecific)
{
    float * p1, * maxp1;
    int buffSize;

    buffSize = wfsSpecific->xframesize * wfsSpecific->yframesize;
    maxp1 = (float *)((int)buffp1 + buffSize*sizeof(float));

    for (p1 = buffp1; p1 < maxp1;)
    {
	*p1 = *(p1++) * constname;
    }
    return(OK);
}

/*------------------------------------------------------------------------*/

/* Add by CB, allow to add N frames into a buffer for further calibrations */
/* 23 Janv 1999                             */
/* Comments to be added */

int ospCoAddOnly ( float *buffp , 
                   int N ,
                   struct OSP_CONTEXT *wfsSpecific )
{

    int   buffSize;        /* size of frame buffer */
    float *p;
    float *bp;
    float *sumbuffp;
    float *maxp;

#ifdef DEBUG
    if ( (N <= 0) ) 
    {
       fprintf ( stderr,
                 "Error: ospCoAddOnly called with wrong number of frames\n" ) ;
       return ( ERROR ) ;
    }
#endif
    
    buffSize = wfsSpecific->xframesize * wfsSpecific->yframesize;
    sumbuffp = wfsSpecific->sumbuff;
    maxp = (float *)((int)sumbuffp + buffSize * sizeof(float)) ;
    bp = buffp ;

    if ( wfsSpecific->coaddcounter == 0 )
    {
	for ( p = sumbuffp ; p < maxp ; )
        {
	    *(p++) = *(bp++) ;
	}
       
        wfsSpecific->coaddcounter ++;
    }
    else
    {
        if ( wfsSpecific->coaddcounter < N )
        {
	   for ( p = sumbuffp ; p < maxp ; )
	   {
	       *p = *(p++) + *(bp++);
	   }
	
           wfsSpecific->coaddcounter ++;
        }
        else
        {
	   for ( p = sumbuffp ; p < maxp; )
	   {
	       * p = * (p++) / wfsSpecific->coaddcounter;
	   }
	   wfsSpecific->coaddcounter=0;
        }
    }

return(OK);
}

/*------------------------------------------------------------------------*/

/* Add by CB, allow to rotate the centroids - 07 feb 1999                 */
/* Comments to be added */

int ospRotateCentroids ( struct OSP_CONTEXT *wfsSpecific )
{
int i ;
float x ;
float y ;
float cosAngle ;
float sinAngle ;

cosAngle = (float) cos(wfsSpecific->angle) ;
sinAngle = (float) sin(wfsSpecific->angle) ;

   for ( i = 1 ; i <= wfsSpecific->mp ;  i += 2 )
   {
       if ( (wfsSpecific->dssq[i] != OSP_NO_LIGHT) && (wfsSpecific->dssq[i+1] != OSP_NO_LIGHT) )
       {
          x = (cosAngle * (wfsSpecific->s[i])) - (sinAngle * (wfsSpecific->s[i+1])) ; 
          y = (sinAngle * (wfsSpecific->s[i])) + (cosAngle * (wfsSpecific->s[i+1])) ; 
          wfsSpecific->s[i] = x ;
          wfsSpecific->s[i+1] = y ;
       } ;
   }

return (OK) ;
}

/*------------------------------------------------------------------------*/

/* Add by CB, allow to calibrate only tip when a least one                */
/* subaperture has enough light. 23 Janv 1999                             */
/* Comments to be added */

int ospCalibrateTip ( float *buffp , 
                      int N ,
                      float amplitude,
                      struct OSP_CONTEXT *wfsSpecific )
{

    int   buffSize;        /* size of frame buffer */
    int   nbSubApertureUsed ;
    int   i ;
    float *p;
    float *bp;
    float *sumbuffp;
    float *maxp;
    float meanX, errorY ;

#ifdef DEBUG
    if ( (N <= 0) ) 
    {
       fprintf ( stderr,
                 "Error: ospCalibrateTip called with wrong number of frames\n" ) ;
       return ( ERROR ) ;
    }
#endif
    
    buffSize = wfsSpecific->xframesize * wfsSpecific->yframesize;
    sumbuffp = wfsSpecific->sumbuff;
    maxp = (float *)((int)sumbuffp + buffSize * sizeof(float)) ;
    bp = buffp ;

    if ( wfsSpecific->coaddcounter == 0 )
    {
       for ( p = sumbuffp ; p < maxp ; )
        {
	    *(p++) = *(bp++) ;
	}
       
        wfsSpecific->coaddcounter ++;
    }
    else
    {
        if ( wfsSpecific->coaddcounter < N )
        {
	   for ( p = sumbuffp ; p < maxp ; )
	   {
	       *p = *(p++) + *(bp++);
	   }
	
           wfsSpecific->coaddcounter ++;
        }
    }

    if ( wfsSpecific->coaddcounter == N )
    {
       for ( p = sumbuffp ; p < maxp; )
       {
           * p = * (p++) / wfsSpecific->coaddcounter;
       }
       wfsSpecific->coaddcounter=0;
           
       if ( ospFGCentroidWrapper ( sumbuffp , wfsSpecific ) == ERROR )
       {
          fprintf ( stderr,
                    "Error: ospCalibrateTip centroid computation fails...\n" ) ;
          return ( ERROR ) ;
       } ;

       ospRotateCentroids ( wfsSpecific ) ;

       meanX = 0.0 ;
       errorY = 0.0 ;
       nbSubApertureUsed = 0 ;
       for ( i = 1 ; i <= wfsSpecific->mp ;  i += 2 )
       {
           if ( (wfsSpecific->dssq[i] != OSP_NO_LIGHT) && (wfsSpecific->dssq[i+1] != OSP_NO_LIGHT) )
           {
              meanX += wfsSpecific->s[i] ;
              errorY += wfsSpecific->s[i+1] ;
              nbSubApertureUsed ++ ;
           } ;
       }
       if ( nbSubApertureUsed != 0 )
       { 
          meanX = meanX/nbSubApertureUsed ;
          errorY = errorY/nbSubApertureUsed ;

          if ( fabs ((double)meanX) > 1e-10 )
             wfsSpecific->tipCor = (-1.0) * amplitude / meanX ;
          else
             wfsSpecific->tipCor = 1.0 ;

          printf ( "ospCalibrateTip : tipCor =%f, tiltError = %f\n" , 
                 wfsSpecific->tipCor , errorY ) ;
       }
       else
       {
          wfsSpecific->tipCor = 1.0 ;
          printf ( "ospCalibratetip : No computation for tipCor = 1.0\n" ) ;
       }
    }

    return(OK);
}

/*------------------------------------------------------------------------*/

/* Add by CB, allow to calibrate only tilt when a least one           */
/* subaperture has enough light. 23 Janv 1999                             */
/* Comments to be added */

int ospCalibrateTilt ( float *buffp , 
                       int N ,
                       float amplitude,
                       struct OSP_CONTEXT *wfsSpecific )
{

    int   buffSize;        /* size of frame buffer */
    int   nbSubApertureUsed ;
    int   i ;
    float *p;
    float *bp;
    float *sumbuffp;
    float *maxp;
    float errorX, meanY ;

#ifdef DEBUG
    if ( (N <= 0) ) 
    {
       fprintf ( stderr,
                 "Error: ospCalibrateTilt called with wrong number of frames\n" ) ;
       return ( ERROR ) ;
    }
#endif
    
    buffSize = wfsSpecific->xframesize * wfsSpecific->yframesize;
    sumbuffp = wfsSpecific->sumbuff;
    maxp = (float *)((int)sumbuffp + buffSize * sizeof(float)) ;
    bp = buffp ;

    if ( wfsSpecific->coaddcounter == 0 )
    {
       for ( p = sumbuffp ; p < maxp ; )
        {
	    *(p++) = *(bp++) ;
	}
       
        wfsSpecific->coaddcounter ++;
    }
    else
    {
        if ( wfsSpecific->coaddcounter < N )
        {
	   for ( p = sumbuffp ; p < maxp ; )
	   {
	       *p = *(p++) + *(bp++);
	   }
	
           wfsSpecific->coaddcounter ++;
        }
    }

    if ( wfsSpecific->coaddcounter == N )
    {
       for ( p = sumbuffp ; p < maxp; )
       {
           * p = * (p++) / wfsSpecific->coaddcounter;
       }
       wfsSpecific->coaddcounter=0;
           
       if ( ospFGCentroidWrapper ( sumbuffp , wfsSpecific ) == ERROR )
       {
          fprintf ( stderr,
                    "Error: ospCalibrateTilt centroid computation fails...\n" ) ;
          return ( ERROR ) ;
       } ;

       ospRotateCentroids ( wfsSpecific ) ;

       errorX = 0.0 ;
       meanY = 0.0 ;
       nbSubApertureUsed = 0 ;
       for ( i = 1 ; i <= wfsSpecific->mp ;  i += 2 )
       {
           if ( (wfsSpecific->dssq[i] != OSP_NO_LIGHT) && (wfsSpecific->dssq[i+1] != OSP_NO_LIGHT) )
           {
              errorX += wfsSpecific->s[i] ;
              meanY += wfsSpecific->s[i+1] ;
              nbSubApertureUsed ++ ;
           } ;
       }
       if ( nbSubApertureUsed != 0 )
       { 
          errorX = errorX/nbSubApertureUsed ;
          meanY = meanY/nbSubApertureUsed ;

          if ( fabs ((double)meanY) > 1e-10 )
             wfsSpecific->tiltCor =  (-1.0) * amplitude / meanY ;
          else
             wfsSpecific->tiltCor = 1.0 ;

          printf ( "ospCalibrateTilt : tipError =%f, tiltCor = %f\n" , 
                 errorX , wfsSpecific->tiltCor ) ;
       }
       else
       {
          wfsSpecific->tiltCor = 1.0 ;
          printf ( "ospCalibrateTilt : No computation for tiltCor = 1.0\n" ) ;
       }
    }

    return(OK);
}

/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME: 
 * ospCoAdd
 *
 * INVOCATION: 
 * ospCoAdd (buffp, N, deltaT,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) buffp     (float *)    pointer to buffer for frame to be added
 * (>) N         (int)        Number of frames to be co-added
 * (>) deltaT    (float)      Timeout period for summation of frames in seconds
 * (!) wfsSpecific (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 * 
 * PURPOSE:
 * To co-add frames, either a specified number or timeout period's worth
 *
 * DESCRIPTION:
 * Is called repeatedly to coadd N frames, with the first call (as indicated 
 * by a frame counter, which is part of the wfs context structure, being set to
 * zero) initialising a frame buffer with the input frame, starting a clock and
 * incrementing the frame counter. The frame counter is initially set to zero 
 * either by ospInit or the successful conclusion of a series of coadds.
 * Repeated calls check the clock, and if the timeout period has not
 * been reached, the current input frame is added to the coadded frame buffer 
 * and the frame counter incremented, until the specified number of frames 
 * is reached. The start time and counter are stored as part of the wfs context
 * structure. When either the specified number of frames or timeout is reached, 
 * the coadded frame buffer has each pixel divided by the number of frames, and
 * ospMeasure is called to calculate Zernikes etc. The Zernike coefficients
 * and their estimated errors are written to the TCS, and the frame counter
 * is set to zero so the next cycle can begin.  
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 * time.h
 *
 * DEFICIENCIES:
 * Need to get the return values of ospMeasure and writeWfsToTcs!
 *
 *-
 */

int /*STATUS*/ ospCoAdd(float * buffp, int N,
			float deltaT, struct OSP_CONTEXT * wfsSpecific)
{

    int buffSize;        /* size of frame buffer */
    clock_t timenow;     /* time variable: current time */
    float timetaken;     /* time between start time and current time (seconds)*/
    float * p;
    float * sumbuffp;
    float * maxp;
    float * bp;
    bp = buffp;

    if((deltaT <= 0) || (N <= 0)) 
    {
	fprintf(stderr,
	     "Error: ospCoadd called with -ve timeout or number of frames\n");
	return(ERROR);
    }
    
    buffSize = wfsSpecific->xframesize * wfsSpecific->yframesize;
    sumbuffp = wfsSpecific->sumbuff;
    maxp = (float *)((int)sumbuffp + buffSize * sizeof(float));

    if(wfsSpecific->coaddcounter == 0)
    {
	for( p = sumbuffp;p < maxp; )
	{
	    * (p++) = * (bp++);
	}

	wfsSpecific->coaddstart = clock();

    	wfsSpecific->coaddcounter ++;
        /*printf ( "ospCoAdd: coaddcounter=0, coaddstart=%f\n" , (float)wfsSpecific->coaddstart ) ;*/

    }
    else
    {
	timenow = clock();
	timetaken = ((float)(timenow-wfsSpecific->coaddstart)/CLOCKS_PER_SEC);
        /*printf ( "ospCoAdd: timetaken=%f\n" , timetaken ) ;*/
	if(timetaken > deltaT)
	{
	    fprintf(stderr,
		    "Error: ospCoAdd timed out after %f seconds", timetaken);
	    fprintf(stderr," and %d frames\n",wfsSpecific->coaddcounter);
	    fprintf(stderr,
		    "       (timeout set at %f seconds)\n", deltaT);
	    fprintf(stderr,"       ...time averaged values");
	    fprintf(stderr," calculated from available data\n");

	    for(p = sumbuffp ; p < maxp ; )
	    {
		* p = * (p++) / wfsSpecific->coaddcounter;
	    }
	 
/* 
 * decrement coaddcounter so that calculation of the standard error will be
 * correct for both single and coadded frames 
 */
	    wfsSpecific->coaddcounter--;
	    ospMeasure(wfsSpecific->sumbuff,wfsSpecific);
	
/* 
 * Get a timestamp to record at which time zernikes data are sent to the TCS 
 */

#ifdef vxWorks
	    if ( timeNow (&(wfsSpecific->time)) != OK )
	    {
	    	fprintf (stderr,
		         "Error: ospCoAdd failed to take bancom time\n" ) ;
	    } ;

	    writeWfsToTcs(wfsSpecific);
#endif /*vxWorks*/

	    wfsSpecific->coaddcounter=0;
	    return(ERROR);
	}

	for( p = sumbuffp ; p < maxp;)
	{
	    * p = * (p++) + * (bp++);
	}

	
/*	for (i=0; i<wfsSpecific->xframesize * wfsSpecific->yframesize; i++)
	{
	    wfsSpecific->sumbuff[i] = (wfsSpecific->sumbuff[i] + * (bp+i));
	}
	*/

	

    wfsSpecific->coaddcounter++;
    }

#ifdef OSP_VERBOSE
	printf("In ospCoAdd: n is %d , time taken is %f\n",
	       wfsSpecific->coaddcounter, timetaken);
#endif /*OSP_VERBOSE*/
	
    if(wfsSpecific->coaddcounter>=N)
    {

	for ( p = sumbuffp ; p < maxp; )
	{
	    * p = * (p++) / wfsSpecific->coaddcounter;
	}



/*	for (i=0; i<wfsSpecific->xframesize * wfsSpecific->yframesize; i++)
	{
	    wfsSpecific->sumbuff[i] = wfsSpecific->sumbuff[i]/
		wfsSpecific->coaddcounter;
	}
	*/
/* 
 * decrement coaddcounter so that calculation of the standard error will be
 * correct for both single and coadded frames 
 */
	wfsSpecific->coaddcounter--;
	ospMeasure(wfsSpecific->sumbuff,wfsSpecific);
#ifdef vxWorks
	if ( timeNow (&(wfsSpecific->time)) != OK )
	{
	   fprintf (stderr,
	            "Error: ospCoAdd failed to take bancom time\n" ) ;
	} ;
	writeWfsToTcs(wfsSpecific);
#endif /*vxWorks*/
	wfsSpecific->coaddcounter=0;
    }
    
    return(OK);
}
/*-------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME: ospFitVars
 *
 *
 * INVOCATION:
 *  ospFitVars(v, w, wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) v       (float **) matrix from SVD of reconstructor
 * (>) w       (float *)  vector of diagonal elements of matrix from SVD of 
 *                        reconstructor
 * (!) wfsSpecific (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK
 *
 * PURPOSE:
 * To estimate the fitting variances of the calculated Zernike coefficients
 *
 * DESCRIPTION:
 * Variance derived from the fitting error is derived from matrices [v] and [w].
 * The equation used may be found in 'Numerical recipes in C' page 536, equation
 * 14.3.19. Equations and page numbers are wrt the first edition- in all
 * editions the equation may be found under the 'Solution by use of Singular
 * Value Decomposition' subsection of the 'General Linear Least Squares' 
 * section of the 'Modeling (sic) of Data' chapter.
 * The values calculated are stored in the context structure. As the fitting
 * variances are only calculated at the same time as the control matrix, ospInit
 * will generally just read the fvars values from file, rather than apply this
 * function. The values as calculated here may be written to such a file using
 * ospWriteVectorToFile, as is done in ospCalibrate.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * Could benefit from checking for divide by zero from w[]. However the
 * calculation of the control matrix by ospCalibrate ensures that w[]
 * is real and positive.
 *
 *-
 */
int /*STATUS*/ ospFitVars(float **v, float *w,
			  struct OSP_CONTEXT * wfsSpecific)
{
    int i; /* counter variable */
    int j; /* counter variable */

    for (j=1;j<=wfsSpecific->np;j++)
    {
	wfsSpecific->fvars[j]=0;
	for (i=1;i<=wfsSpecific->np;i++)
	{
	    wfsSpecific->fvars[j] = wfsSpecific->fvars[j] + (v[j][i]*v[j][i]) / (w[i]*w[i]);
	}
#ifdef OSP_VERBOSE
	printf("Fitting var: %f for Z%d \n",wfsSpecific->fvars[j],j);
#endif /*OSP_VERBOSE*/
    }
    return(OK);
}
/*------------------------------------------------------------------------*/


/*
 *+
 * FUNCTION NAME: 
 * ospMeasVars
 *
 * INVOCATION:
 * ospMeasVars(wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) wfsSpecific   (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK.
 *
 * PURPOSE:
 * To estimate the measurement variances of the calculated Zernike coefficients
 *
 * DESCRIPTION:
 * Variance derived from measurement errors in the vector ds of SH spot 
 * displacements is obtained from the sum of 
 * squares of the (elements of ds multiplied by the appropriate row of [c])
 *.for each Zernike coefficient. The values calculated are stored in the
 * context structure.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * none
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
int /*STATUS*/ ospMeasVars( struct OSP_CONTEXT * wfsSpecific)
{
    int i; /* counter variable */
    int j; /* counter variable */

   for (j=1;j<=wfsSpecific->np;j++)
   {
	wfsSpecific->mvars[j]=0;
	for (i=1;i<=wfsSpecific->mp;i++)
	{
	    wfsSpecific->mvars[j] = wfsSpecific->mvars[j]+wfsSpecific->c[j][i]*wfsSpecific->c[j][i]* wfsSpecific->dssq[i];
	}
#ifdef OSP_VERBOSE
	printf("Measurement var: %f for Z%d \n",wfsSpecific->mvars[j],j);
#endif /*OSP_VERBOSE*/
   }
    return(OK);
}
/*------------------------------------------------------------------------*/



/*
 *+
 * FUNCTION NAME: 
 * ospSimulateCentroids
 *
 * INVOCATION:
 * ospSimulateCentroids(matr,vect)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) matr     (float **)  reconstructor matrix relative to basis functions 
 *                          of Zernike polynomials in ascending order starting 
 *                          with tip, tilt, focus...
 * (<) v        (float *)   vector of displacements (in pixels) corresponding 
 *                          to specified combination of Zernike polynomials
 *
 * FUNCTION VALUE:
 * (void)
 *
 * PURPOSE:
 * To generate centroid data for a specified combination of Zernike functions
 *
 * DESCRIPTION:
 * Using displacements for normalised model Zernike functions, the specified
 * combination is built up by addition of the scaled displacements from each
 * individual Zernike function, stored in the form of a reconstructor matrix,
 * The scale factor is prompted for on the standard output and suppplied on 
 * the standard input.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * Should implement error checking to ascertain that none of the columns of
 * the matrix matr are zero vectors
 *-
 */
void ospSimulateCentroids(float ** matr, float * vect,
			  int np, int mp)
{
    int i;            /* general purpose counter */
    int j;            /* general purpose counter */
    float mag;        /* magnitude of each Zernike polynomial prompted for */

    for(j=1;j<=mp;j++) vect[j] = 0.0;

    for (i=1;i<=np;i++)
    {
	printf("Enter magnitude of Z%d:\t",i);
	scanf("%f",&mag);
	for (j=1;j<=mp;j++)
	{
	    vect[j] = vect[j] + mag * matr[j][i];
	}
    }	       
    printf("\n");
    return;
}
/*------------------------------------------------------------------------*/


/*
 *+
 * FUNCTION NAME: 
 * ospCentroidWrapper
 *
 * INVOCATION:
 * ospCentroidWrapper(buffp,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) buffp    (float *) pointer to input buffer containing image of SH spots
 * (!) wfsSpecific (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int)   A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To threshold the frame and repeatedly apply the ospCentroid() function to every subaperture with a specified centre
 *
 * DESCRIPTION:
 * Reads the array of centres and invokes ospCentroid() for each complete set of
 * x0, xoffset, y0, yoffset, and stores all the centroid positions as
 * a vector wfsSpecific->s of x and y displacements of the centroid and a 
 * vector wfsSpecific->dssq of the x and y variances.
 * If the weight argument is set to 1, the x and y displacements in s are 
 * replaced by values weighted by the reciprocal of the respective standard 
 * deviations. NB this weighting will be changed to something more...correct.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * That ospCalculateSubaps has generated the centres[] array.
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * Need to read the return values from ospCentroid and ospThreshold
  *-
 */
int /*STATUS*/ ospCentroidWrapper(float * buffp, 
				  struct OSP_CONTEXT * wfsSpecific)
{
    int j;            /* general purpose counter */
    int i=1;          /* general purpose counter */
    float meanval[2*OSP_SUBAPSMAX/2 +1];
    int meanvali=1;
    float disp[4];

    ospThreshold(buffp,meanval,wfsSpecific);

#ifdef OSP_VERBOSE
    printf("disp[0]   disp[1]   disp[2]   disp[3]  disp[0]/disp[1] disp[2]/disp[3] meanval\n");
    printf("--------------------------------------------------------------------------------\n");
#endif /*OSP_VERBOSE*/
    
    for (j = 1; j < wfsSpecific->centres[0]+1; j=j+4) 
         {
          ospCentroid(buffp,
		      (int)(wfsSpecific->centres[j]),
		      wfsSpecific->centres[j+1],
		      (int)(wfsSpecific->centres[j+2]), 
		      wfsSpecific->centres[j+3],
		      disp, 
		      meanval[meanvali],
		      wfsSpecific);

	  if (wfsSpecific->weight == 0)
	  {
	      wfsSpecific->s[i]=disp[0];
	      wfsSpecific->s[i+1]=disp[2];
	  }
	  else
	  {
	      if(disp[1]==0)
	      {
		  fprintf(stderr,"Divide by zero in ospCentroidWrapper ");
		  fprintf(stderr,"for disp[1] and i = %d\n",i);
		  return(ERROR);
	      }
	      else
		  wfsSpecific->s[i]=disp[0]/disp[1];
	      if(disp[3]==0)
	      {
		  fprintf(stderr,"Divide by zero in ospCentroidWrapper ");
		  fprintf(stderr,"for disp[3] and i = %d\n",i);
		  return(ERROR);
	      }
	      else
		  wfsSpecific->s[i+1]=disp[2]/disp[3];
	  }

	  wfsSpecific->dssq[i]=disp[1];
	  wfsSpecific->dssq[i+1]=disp[3];

#ifdef OSP_VERBOSE 
	  print ("%9.3f %9.3f %9.3f %9.3f %9.3f %9.3f %9.3f \n",disp[0],disp[1],disp[2],disp[3],disp[0]/disp[1],disp[2]/disp[3], meanval);
#endif /*OSP_VERBOSE*/

	  i=i+2;
	  
	  meanvali++;

         }
    return(OK);
}
/*------------------------------------------------------------------------*/


/*
 *+
 * FUNCTION NAME: 
 * ospGetBasisFunction
 *
 * INVOCATION:
 * ospGetBasisFunction (f,i,znum,mag)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) f    (float **)    output matrix showing basis functions in terms of 
 *                        Zernike coefficients
 * (>) i    (int)         index of component, equal to index of actual Zernike 
 *                        function
 * (>) znum (int)         index of component, equal to index of arbitrary
 *                         function
 * (>) mag  (float)       size of component
 *
 * FUNCTION VALUE:
 * (void)
 *
 * PURPOSE:
 * To populate the matrix which defines an arbitrary basis set in terms of 
 * Zernike polynomials and their coefficients
 *
 * DESCRIPTION:
 * Puts an element into a matrix. This process must be tied in to ospCalibrate
 * and also linked with the vector (or file containing the vector) of the
 * numbered basis function for which we are storing the Zernike coefficients.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * f[][] must be intialised as identically zero, and of the appropriate
 * dimensions, as is done in ospCalibrate. 
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * Incomplete, never tested...
 *-
 */
void ospGetBasisFunction( float ** f, int i, int znum, float mag)
{
    f[znum][i]=mag;
    return;
}
/*------------------------------------------------------------------------*/


/*
 *+
 * FUNCTION NAME: 
 * ospTestData
 *
 * INVOCATION:
 * ospTestData (buffp,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) buffp    (float *)  pointer to output buffer containing test frame
 * (>) wfsSpecific (struct OSP_CONTEXT *) wfs context structure
 *
 * FUNCTION VALUE:
 * (void)
 *
 * PURPOSE:
 * To generate a known test frame of data
 *
 * DESCRIPTION:
 * Generates a linear ramp of increasing intensity in the x direction.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * That the buffer pointed to by buffp has been allocated large enough to
 * accomodate wfsSpecific->xarraysize x wfsSpecific->yarraysize floats.
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *
 *-
 */
void ospTestData (float * buffp, struct OSP_CONTEXT * wfsSpecific)
    /**********************************************************************/
    /* generates known test data pattern                                  */
    /**********************************************************************/
{
    int ii=0;                          /* general purpose counter */
    int jj=0;                          /* general purpose counter */
   
    for(jj=0; jj<wfsSpecific->yarraysize;jj++)
    {
	for(ii=0;ii<wfsSpecific->xarraysize;ii++)
	{
	    * (buffp + ii + jj*wfsSpecific->xarraysize) = ii;
	}
    }
    return;
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospNewReadCentres
 *
 * INVOCATION:
 * ospNewReadCentres(nullfile,centres,side,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) nullfile     (char *)    name of file containing null positions 
 *                              of the SH spots in the subapertures
 * (<) centres      (float *)   pointer to array containing the coordinates of
 *                              the subapertures bottom left hand corners, and
 *                              the relative offsets of the null positions 
 *                              (in pixels)
 * (>) side         (int)       side length of subap, in pixels
 * (!) wfsSpecific  (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK
 *
 * PURPOSE:
 * To calculate coordinate data for the subaperture null positions from data stored in file
 *
 * DESCRIPTION:
 * This function is essentially a wrapper for the two functions ospReadNulls and
 * ospCalculateSubaps, and was written as an intermediate stage in the
 * evolution of these two functions from the previous ospReadCentres, which read
 * from a file containing different data in another format. Essentially, the
 * null positions are read from a file, coordinates relative to the whole array,
 * in pixels, and negative coordinates supplied for the subapertures to be 
 * ignored... for more information see ospReadNulls. The postions of the 
 * bottom left hand corners of the subapertures are calculated, given the 
 * values of the detector readout geometry stored in the context structure, and 
 * the offsets of the given null positions from these coordinates calculated for
 * each subaperture. This information is stored in the wfsSpecific->centres
 * array, and here is copied to the centres argument.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * Possibly irrelevant
 *-
 */
int /*STATUS*/ ospNewReadCentres( char *nullfile, float * centres, int side,
			       struct OSP_CONTEXT * wfsSpecific)
{
    ospReadNulls(nullfile, wfsSpecific);
    ospCalculateSubaps(wfsSpecific);
    centres = wfsSpecific->centres;
    return(OK);
}

/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospReadNulls
 *
 * INVOCATION:
 * ospReadNulls(nullfile, wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) nullfile     (char *)  name of file containing the null positions
 *                            of the SH spots in the subapertures
 * (!) wfsSpecific  (struct OSP_CONTEXT *)pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To read the coordinates of the null positions from a file
 *
 * DESCRIPTION:
 * The file named nullfile is opened, and the null positions for the 
 * subapertures stored in it read into the wfsSpecific->nulls[] structure
 * elements. The null positions are in units of pixels, 
 * and are relative to the bottom left hand corner of the CCD array. The 
 * centre of the bottom left hand pixel in the array has coordinate (1,1).
 * The data is stored in the file with two records per line, separated by a
 * space, corresponding to the x and y coordinate, and each line corresponding
 * to a subaperture. The data are read into wfsSpecific->nulls in the order
 * wfsSpecific->nulls[0] = first x coordinate,
 * wfsSpecific->nulls[1] = first y coordinate,
 * wfsSpecific->nulls[2] = second x coordinate etc.
 * The lines of data are ordered to match the order in which
 * the subaperture coordinates are subsequently calculated: the null position
 * coordinates for the bottom left subaperture are first, then the subaperture
 * along the bottom row from left to right, moving up a row at a time. By way of
 * example, for a 3 x 3 array of subapertures, they would be ordered as follows:
 *  _______ _______ _______
 * |       |       |       |
 * |   7   |   8   |   9   |
 * |_______|_______|_______|
 * |       |       |       |
 * |   4   |   5   |   6   |
 * |_______|_______|_______|
 * |       |       |       |
 * |   1   |   2   |   3   |
 * |_______|_______|_______|
 *
 * Subapertures for which a centroid is not to be calculated, and/or for which
 * a null position may not be calculated (e.g. they may lie outwith the
 * illuminated field) still require a line of data corresponding to coordinates
 * to be input- these coordinates however should be negative to indicate that 
 * the subaperture is to be subsequently ignored. 
 * The pairs of subapertures are read until an EOF is encountered, or until the
 * limit set by OSP_SUBAPSMAX in osp.h is reached. The number of pairs of 
 * coordinates is checked to be consistent with the number of subapertures
 * specified by the ccd readout geometry stored in wfsSpecific, and an ERROR
 * returned instead of OK, after closing the file.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
int /*STATUS*/ ospReadNulls(char * nullfile, 
			    struct OSP_CONTEXT * wfsSpecific)
{
    int i=0;          /* counter */
    int ospstatus=OK; /* status flag */
    float f1;         /* temporary storage for first value on line of data */
    float f2;         /* temporary storage for second value on line of data */
    FILE *fp1;        /* file pointer */

    if((fp1 = fopen(nullfile,"r")) == NULL)
	{
	    fprintf(stderr,"Error in opening %s for reading \n", nullfile);
	    fprintf(stderr,"...during function ospReadNulls\n");
	    return(ERROR);
	}
    
    while(((fscanf(fp1,"%f %f",&f1,&f2)) != EOF) && (i<2*OSP_SUBAPSMAX))
    { 
	wfsSpecific->nulls[i]   = f1; 
	wfsSpecific->nulls[i+1] = f2;

#ifdef OSP_VERBOSE
	fprintf(stderr,"In ospReadNulls: nulls %f\t%d\n",
		wfsSpecific->nulls[i], i+1);
	fprintf(stderr,"In ospReadNulls: nulls %f\t%d\n",
		wfsSpecific->nulls[i+1], i+2);
#endif /*OSP_VERBOSE*/

	i=i+2;
    }


    printf("%d pairs of coordinates read from ",i/2);
    puts ( nullfile ); 

    if(i != (2*wfsSpecific->ospxsubap * wfsSpecific->ospysubap *wfsSpecific->sectors))
    {
	ospstatus=ERROR;
	fprintf(stderr,"Number of coordinates (%d) inconsistent with total ",i);
	fprintf(stderr,"number of subapertures (%d)\n",
		wfsSpecific->ospxsubap * 
		wfsSpecific->ospysubap *wfsSpecific->sectors);
	fprintf(stderr,"...during function ospReadNulls\n");
    }


    fclose(fp1);
    return(ospstatus);
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospCalculateSubaps
 *
 * INVOCATION:
 * ospCalculateSubaps(wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) wfsSpecific (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To calculate the coordinates of the subapertures and null positions wrt to the readout geometry
 *
 * DESCRIPTION:
 * The positions of the bottom left hand corners of the subapertures (in pixels)
 * are calculated using the ccd readout geometry stored in the context 
 * structure, with respect to either a full frame or reduced frame as
 * appropriate, and the null positions of each subaperture which is to be used
 * are calculated relative to these coordinates. This data is then stored in 
 * wfsSpecific->centres[], with centres[0] storing the number of data items to 
 * follow (which is four times the number of subapertures for which the null
 * positions are positive values, which is clearly not necessarily the same
 * as four times the number of subapertures).The subsequent entries are
 * centres[1] = x coordinate (in pixels) of bottom left hand pixel of first
 *              subaperture with positive coordinates for its null position.
 * centres[2] = offset in x (in pixels) of null position from bottom left hand
 *              pixel of first subaperture...
 * centres[3] = y coordinate (in pixels) of bottom left hand pixel of first
 *              subaperture with positive coordinates for its null position.
 * centres[4] = offset in y (in pixels) of null position from bottom left hand 
 *              pixel of first subaperture...
 * and so on in groups of 4 for subsequent subapertures with positive
 * coordinates for their null positions, i.e. centres[5] to centres[8] are the
 * corresponding data for the second subaperture with positive coordinates
 * for its null position.
 * The null positions used in the calculation are those stored in 
 * wfsSpecific->nulls[], which use negative coordinates to indicate that a 
 * subaperture is not to be used.
 * 
 * The ccd readout geometry defines the positions of the subapertures on the
 * array, and the coordinates of the bottom left hand pixel of each subaperture
 * are calculated with respect to an x-y coordinate system which applies to the 
 * whole array, and is not redefined for each sector. The calculation of the
 * subaperture positions in each sector however takes account of the various 
 * symmetry properties of the sectors with respect to each other. Note that
 * the definition of the centre of the bottom left hand pixel of the whole
 * area of the array which is read out as (1,1) has been chosen for convenience
 * when calculating the subaperture positions in the various sectors. 
 * The ccd readout geometry is defined in the following elements of the context
 * structure:sectors - number of 
 *           xstart  - pixels in x skipped before first subap
 *           ystart  - pixels in y skipped before first subap
 *           xbin    - binning factor in x for the pixels read out
 *           ybin    - binning factor in y for the pixels read out
 *           xraster - x size of the subapertures in binned pixels
 *           yraster - y size of the subapertures in binned pixels
 *           xspace  - pixels in x between subapertures
 *           yspace  - pixels in y between subapertures
 *           xsubap  - number of subapertures readout PER SECTOR in x
 *           ysubap  - number of subapertures readout PER SECTOR in y
 *           xarraysize - x size of the frame supplied to the centroiding func.
 *           yarraysize - y size of the frame supplied to the centroiding func.
 *           framesizeflag - flag for full frame (1) or reduced frame (0) image
 * which correspond with the geometry shown most clearly in Gemini Newsletter
 * #16, June 1998, p12.
 * Currently there is no provision for binning (i.e. xbin=ybin=1), and as the 
 * null positions in wfsSpecific->nulls are relative to the same full frame
 * coordinate system, the calculation of the offsets in x and y are simple 
 * differences. If binning is employed, some scaling will also be necessary 
 * when calculating the offsets in x and y of the null positions.
 *
 * The calculation of the centres[] data proceeds in one of two distinct ways
 * dependent on whether wfsSpecific->framesizeflag is 1 (indicating that a
 * full frame is used) or 0 (indicating that a reduced frame of only
 * subaperture pixels is used). If framesizeflag = 1, our calculations are 
 * complete and we have populated the centres[] array. If framesizeflag = 0, we
 * need to recalculate the subaperture coordinates to reflect that only the 
 * pixels which constitute the subapertures (including the unused subaps)
 * make up the reduced frame. The offsets of the null positions in x and y 
 * are unchanged.
 *
 * The offsets in x and y of the null positions for each subaperture
 * are checked to ensure that they are less than a quarter of the width
 * of the subaperture from its geometric centroid, and a warning flagged if this
 * is not the case- the function continues but exits with an ERROR value. 
 * An error value is also returned if the number of subapertures which are to be
 * used (which have positive coordinates for their null positions) does not 
 * equal (wfsSpecific->mp/2), which is specified when the context structure is
 * initialised. If a subaperture has negative coordinates for its null position
 * this is reported- this is not an error.
 * 
 * The data thus calculated is used by the thresholding and centroiding 
 * functions.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * That the null positions have been read in using ospReadNulls, called
 * either by itself or as a part of ospInit or ospCalibrate.
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * No provision yet for binning of pixels
 *-
 */
int /*STATUS*/ ospCalculateSubaps(struct OSP_CONTEXT * wfsSpecific)
{
    int x0;
    int y0;
    int x1;
    int y2;
    int xpitch;
    int ypitch;
    int xtail;
    int ytail;
    int mm;
    int illum;
    int index;
    int i,j;
    int ospstatus=OK;
    int side=wfsSpecific->side;
    float offlow = (float)side*0.25 + 0.5;
    float offhigh = (float)side*0.75 + 0.5;

    xpitch = wfsSpecific->side + wfsSpecific->ospxspace;
    ypitch = wfsSpecific->side + wfsSpecific->ospyspace;
    xtail = (wfsSpecific->xarraysize /2 - wfsSpecific->ospxstart -
	     wfsSpecific->ospxsubap * xpitch + wfsSpecific->ospxspace);
    ytail = (wfsSpecific->yarraysize * 2/wfsSpecific->sectors  - 
	     wfsSpecific->ospystart -wfsSpecific->ospysubap * ypitch + 
	     wfsSpecific->ospyspace);

    x0 = wfsSpecific->ospxstart+1;
    y0 = wfsSpecific->ospystart+1;
    x1 = x0 + wfsSpecific->ospxsubap * xpitch + 2 * xtail -wfsSpecific->ospxspace;

    mm=0;
    illum=0;
    for (j=0;j<wfsSpecific->ospysubap;j++)
    {
	for (i=0;i<wfsSpecific->ospxsubap;i++)
	{
	    if(wfsSpecific->nulls[2*mm] > 0.0)
	    {
		index=4*illum;
		wfsSpecific->centres[index+1]= x0 + i * xpitch;
		wfsSpecific->centres[index+2]= wfsSpecific->nulls[2*mm]-
		    wfsSpecific->centres[index+1];
		wfsSpecific->centres[index+3]= y0 + j * ypitch;
		wfsSpecific->centres[index+4]= wfsSpecific->nulls[2*mm+1]-
		    wfsSpecific->centres[index+3];
		illum++;
		if(wfsSpecific->centres[index+2] < offlow 
		   || wfsSpecific->centres[index+2] >offhigh
		   || wfsSpecific->centres[index+4] < offlow
		   || wfsSpecific->centres[index+4] >offhigh)
		{
		    printf("Badly centred null position for subap [%d][%d]\n",
			   i+1,j+1);
		    printf("...offsets are (%f, %f)\n",
			   wfsSpecific->centres[index+2],
			   wfsSpecific->centres[index+4]);
		    ospstatus=ERROR;
		}
	    }
	    else
	    {
		printf("No null position for subap [%d][%d]\n",i+1,j+1);
	    }
	    mm++;
	}
	for (i=0;i<wfsSpecific->ospxsubap;i++)
	{
	    if(wfsSpecific->nulls[2*mm] > 0.0)
	    {
		index=4*illum;
		wfsSpecific->centres[index+1]= x1 + i * xpitch;
		wfsSpecific->centres[index+2]= wfsSpecific->nulls[2*mm]-
		    wfsSpecific->centres[index+1];
		wfsSpecific->centres[index+3]= y0 + j * ypitch;
		wfsSpecific->centres[index+4]= wfsSpecific->nulls[2*mm+1]-
		    wfsSpecific->centres[index+3];
		illum++;
		if(wfsSpecific->centres[index+2] < offlow 
		   || wfsSpecific->centres[index+2] >offhigh
		   || wfsSpecific->centres[index+4] < offlow
		   || wfsSpecific->centres[index+4] >offhigh)
		{
		    printf("Badly centred null position for subap [%d][%d]\n",
			   wfsSpecific->ospxsubap+i+1,j+1);
		    printf("...offsets are (%f, %f)\n",
			   wfsSpecific->centres[index+2],
			   wfsSpecific->centres[index+4]);
		    ospstatus=ERROR;
		}
	    }
	    else
	    {
		printf("No null position for subap [%d][%d]\n",
		       wfsSpecific->ospxsubap+i+1,j+1);
	    }
	    mm++;
	}
    }

    if(wfsSpecific->sectors == 4)
    {
	y2 = x0 + wfsSpecific->ospysubap * ypitch + 2 * ytail -
	    wfsSpecific->ospyspace;

	for (j=0;j<wfsSpecific->ospysubap;j++)
	{
	    for (i=0;i<wfsSpecific->ospxsubap;i++)
	    {
	    if(wfsSpecific->nulls[2*mm] > 0.0)
	    {
		index=4*illum;
		wfsSpecific->centres[index+1]= x0 + i * xpitch;
		wfsSpecific->centres[index+2]= wfsSpecific->nulls[2*mm]-
		    wfsSpecific->centres[index+1];
		wfsSpecific->centres[index+3]= y2 + j * ypitch;
		wfsSpecific->centres[index+4]= wfsSpecific->nulls[2*mm+1]-
		    wfsSpecific->centres[index+3];
		illum++;
		if(wfsSpecific->centres[index+2] < offlow 
		   || wfsSpecific->centres[index+2] >offhigh
		   || wfsSpecific->centres[index+4] < offlow
		   || wfsSpecific->centres[index+4] >offhigh)
		{
		    printf("Badly centred null position for subap [%d][%d]\n",
			   i+1,wfsSpecific->ospysubap+j+1);
		    printf("...offsets are (%f, %f)\n",
			   wfsSpecific->centres[index+2],
			   wfsSpecific->centres[index+4]);
		    ospstatus=ERROR;
		}
	    }
	    else
	    {
		printf("No null position for subap [%d][%d]\n",
		       i+1,wfsSpecific->ospysubap+j+1);
	    }
	    mm++;
	    }
	    for (i=0;i<wfsSpecific->ospxsubap;i++)
	    {
	    if(wfsSpecific->nulls[2*mm] > 0.0)
	    {
		index = 4*illum;
		wfsSpecific->centres[index+1]= x1 + i * xpitch;
		wfsSpecific->centres[index+2]= wfsSpecific->nulls[2*mm]-
		    wfsSpecific->centres[index+1];
		wfsSpecific->centres[index+3]= y2 + j * ypitch;
		wfsSpecific->centres[index+4]= wfsSpecific->nulls[2*mm+1]-
		    wfsSpecific->centres[index+3];
		illum++;
		if(wfsSpecific->centres[index+2] < offlow 
		   || wfsSpecific->centres[index+2] >offhigh
		   || wfsSpecific->centres[index+4] < offlow
		   || wfsSpecific->centres[index+4] >offhigh)
		{
		    printf("Badly centred null position for subap [%d][%d]\n",
			   wfsSpecific->ospxsubap+i+1,
			   wfsSpecific->ospysubap+j+1);
		    printf("...offsets are (%f, %f)\n",
			   wfsSpecific->centres[index+2],
			   wfsSpecific->centres[index+4]);
		    ospstatus=ERROR;
		}
	    }
	    else
	    {
		printf("No null position for subap [%d][%d]\n",
		       wfsSpecific->ospxsubap+i+1,
		       wfsSpecific->ospysubap+j+1);
	    }
	    mm++;
	    }
	}
    }
    wfsSpecific->centres[0] = illum*4;
    printf("%d subapertures illuminated\n",(int)illum);

    if((illum*2) != wfsSpecific->mp)
    {
	fprintf(stderr,"Number of subapertures with null position inconsistent");
	fprintf(stderr," with context element mp\n");
	fprintf(stderr,"...during function ospCalculateSubaps\n");
	ospstatus=ERROR;
    }

   
    mm=0;
    illum=0;
    if(wfsSpecific->framesizeflag==0)
    {
	for (j=0;j<wfsSpecific->ospysubap*wfsSpecific->sectors/2;j++)
	{
	    for (i=0;i<wfsSpecific->ospxsubap*2;i++)
	    {
		if(wfsSpecific->nulls[2*mm] > 0.0)
		{
		    index=4*illum;
		    wfsSpecific->centres[index+1]=side*i+1;
		    wfsSpecific->centres[index+3]=side*j+1;
		    illum++;
		}
		mm++;
	    }
	}
    }

    return(ospstatus);
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME: 
 * ospThreshold
 *
 * INVOCATION:
 * ospThreshold(buffp, meanval,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) buffp     (float *) pointer to frame buffer
 * (<) meanval   (float *) pointer to array of intensity values subtracted from
 *                           above threshold pixels in each subap
 * (!) wfsSpecific (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To estimate and/or impose a threshold for each subaperture to reduce the effects of noise
 *
 * DESCRIPTION:
 * Using the readout geometry data from wfsSpecific (particularly that in 
 * wfsSpecific->centres[] which records the coordinates of the bottom left 
 * pixel of each subaperture) a threshold is applied to each subaperture which
 * is in use, modifying the frame buffer pointed to by buffp correspondingly.
 * The manner in which the threshold is determined for each subaperture depends
 * on the value of wfsSpecific->thresh: if it is -1, threshold value for each 
 * subaperture is determined by averaging the intensities of the lowest 
 * (intensity) three of the four corner pixels, calculating their standard
 * deviation and setting the threshold equal to mean + n * standard deviation, 
 * where n is the value specified by wfsSpecific->nsigma;
 * if it is -3, the threshold value for each subaperture is determined by
 * averaging three pixels at each corner, calculating their standard deviation
 * and setting the threshold equal to mean + standard deviation, where etc.;
 * if it is >=0, that value is applied as the threshold;
 * for any other value, an error is reported and returned.
 * For the first two cases, the threshold is applied by setting pixel values
 * below it to zero, and subtracting the mean calculated from the corner pixels
 * from the pixel values above zero. The actual values subtracted are returned
 * in the array pointed to by meanval. For the third
 * case, the pixel values below threshold are set to zero, the ones above 
 * threshold have threshold subtracted, and the threshold value is returned
 * in the elements of meanval.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * Would be better integrated into the centroid function.
 *-
 */
int /*STATUS*/ ospThreshold (float * buffp, float * meanval,
		   struct OSP_CONTEXT * wfsSpecific)
{
    float * indxy;       /* pointer to the current pixel */ 
    float * index[12];   /* pointer to array of corner pixels */
    float mean;          /* mean calculated from corner pixels */
    float threshold;     /* threshold calculated/imposed */
    float sigma;         /* std dev calculated from corner pixels */
    float temp=0;        /* temp storage for sorting corner pixels */
    float itotal=0;      /* total intensity in subapertures */
    int sindex = wfsSpecific->side - 1;  /* step from lhs to rhs corner pixel */
    int ii;              /* counter */
    int jj;              /* counter */
    int kk;              /* counter */
    int x0;              /* x coordinate of bottom left pixel of subap */
    int y0;              /* y coordinate of bottom left pixel of subap */
    int tempi= 0;        /* temp storage of index while sorting corner pixels*/
    int poff;            /* offset from index of bottom left pixel in subap*/
    int meanvali=1;     /* index of subaperture */
    float tflag = wfsSpecific->thresh;  /* type of thresholding */
    int ospstatus=OK;    /* status flag for function */

    * meanval = 0;

    if(tflag==-1)
    {
	for (kk=1;kk< wfsSpecific->centres[0]+1; kk=kk+4)
	{
	    x0=wfsSpecific->centres[kk];
	    y0=wfsSpecific->centres[kk+2];

	    index[0] = buffp + wfsSpecific->xframesize * (y0-1) + (x0-1);
	    index[1] = index[0] + wfsSpecific->xframesize * sindex;
	    index[2] = index[0] + sindex;
	    index[3] = index[1] + sindex;
	    
#ifdef OSP_VERBOSE
	    printf("corners: %f %f %f %f \n",*index[0], *index[1], *index[2],*index[3]);
#endif /*OSP_VERBOSE*/ 

	    temp=0;
	    tempi=0;
	    for (ii=0;ii<4;ii++)
	    {
		if(*index[ii]>temp)
		{
		    tempi = ii;
		    temp = *index[ii];
		}
	    }

	    *index[tempi]=*index[3];
#ifdef OSP_VERBOSE
	    printf("corners: %f %f %f %f tempi:%d \n",*index[0], *index[1], *index[2],*index[3],tempi);
#endif /*OSP_VERBOSE*/ 
	    
	    mean = 0.333333 *(*index[0] + *index[1] + *index[2]);

	    sigma = sqrt(((mean - *index[0])*(mean - *index[0])+
			  (mean - *index[1])*(mean - *index[1])+
			  (mean - *index[2])*(mean - *index[2]))
			 *0.5);
	    *index[tempi]=temp;

#ifdef OSP_VERBOSE
	    printf("threshold is %9.3f + %9.3f x %9.3f\n",
		   mean,wfsSpecific->nsigma,sigma);
#endif /*OSP_VERBOSE*/

	    threshold=mean+wfsSpecific->nsigma*sigma;
	    *(meanval+meanvali) = mean;
	    meanvali++;


	    for (jj = 0; jj < wfsSpecific->side; jj++)
	    {   
		for (ii = 0; ii < wfsSpecific->side; ii++)
		{
		    poff = ii + jj * wfsSpecific->xframesize;
		    indxy = index[0] + poff;
		    
		    if (*indxy > threshold)
			*indxy = *indxy - mean;
		    else 
			*indxy = 0.0;
		    itotal = itotal + *indxy;
 
		}
	    }

	}
    }

    else if(tflag==-3)
    {
	for (kk=1;kk< wfsSpecific->centres[0]+1; kk=kk+4)
	{
	    x0=wfsSpecific->centres[kk];
	    y0=wfsSpecific->centres[kk+2];

	    index[0] = buffp + wfsSpecific->xframesize * (y0-1) + (x0-1);
	    index[1] = index[0] + wfsSpecific->xframesize * sindex;
	    index[2] = index[0] + sindex;
	    index[3] = index[1] + sindex;
	    index[4] = index[0] + 1;            
	    index[5] = index[1] + 1;
	    index[6] = index[2] - 1;
	    index[7] = index[3] - 1;
	    index[8] = index[0] + wfsSpecific->xframesize;
	    index[9] = index[1] - wfsSpecific->xframesize;
	    index[10] = index[2] + wfsSpecific->xframesize;
	    index[11] = index[3] - wfsSpecific->xframesize;

	    mean = (*index[0] + *index[1] + *index[2] + *index[3] +
		*index[4] + *index[5] + *index[6] + *index[7] + *index[8]
		+ *index[9] + *index[10] + *index[11]) / 12.0 ;

	    sigma = sqrt(((mean - *index[0])*(mean - *index[0])+
			  (mean - *index[1])*(mean - *index[1])+
			  (mean - *index[2])*(mean - *index[2])+
			  (mean - *index[3])*(mean - *index[3])+
			  (mean - *index[4])*(mean - *index[4])+
			  (mean - *index[5])*(mean - *index[5])+
			  (mean - *index[6])*(mean - *index[6])+
			  (mean - *index[7])*(mean - *index[7])+
			  (mean - *index[8])*(mean - *index[8])+
			  (mean - *index[9])*(mean - *index[9])+
			  (mean - *index[10])*(mean - *index[10])+
			  (mean - *index[11])*(mean - *index[11]))
			  / 12.0);
    
#ifdef OSP_VERBOSE
	    printf("threshold is %9.3f + %9.3f x %9.3f\n",
		   mean,wfsSpecific->nsigma,sigma);
#endif /*OSP_VERBOSE*/

	    threshold=mean+wfsSpecific->nsigma*sigma;

	    *(meanval+meanvali) = mean;
	    meanvali++;


	    for (jj = 0; jj < wfsSpecific->side; jj++)
	    {   
		for (ii = 0; ii < wfsSpecific->side;ii++)
		{
		    poff = ii + jj * wfsSpecific->xframesize;
		    indxy = index[0] + poff;
		    
		    if (*indxy > threshold)
			*indxy = *indxy - mean;
		    else 
			*indxy = 0.0;
		    itotal = itotal + *indxy;
		}
	    }
	}
    }
    
    else if(tflag >= 0)
    {
	for(meanvali=1; meanvali< (wfsSpecific->centres[0]/4)+1; meanvali++)
	{
	    *(meanval+meanvali) = tflag;
	}
	    

	threshold = tflag;
	for (ii=0; ii < wfsSpecific->xframesize*wfsSpecific->yframesize; ii++)
	{
	    indxy = buffp + ii;
	    if ( *indxy > threshold)           /* Apply a threshold */
		*indxy = *indxy - threshold; 
	    else
		*indxy = 0.0;
	    itotal = itotal + *indxy;
	}
    }
    else
    {
	fprintf(stderr,"Invalid threshold supplied: no threshold applied\n");
	fprintf(stderr,"Threshold must be either\n");
	fprintf(stderr,"\tgreater than zero, or\n");
	fprintf(stderr,"\t-1 for 1 sample point per corner, or\n");
	fprintf(stderr,"\t-3 for 3 sample points per corner.\n");
	ospstatus=ERROR;
    }
#ifdef OSP_VERBOSE
    printf("Total intensity in the frame is %9.3f\n",itotal);
#endif /*OSP_VERBOSE*/

    if (ospstatus==ERROR)
	fprintf(stderr,"...during ospThreshold");
    return(ospstatus);
}


/*------------------------------------------------------------------------*/
/*
 *+
 * FUNCTION NAME: 
 * ospGlobalThreshold
 *
 * INVOCATION:
 * ospGlobalThreshold(buffp, wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) buffp          (float *)  pointer to input frame buffer
 * (!) wfsSpecific    (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * void
 *
 * PURPOSE:
 * To apply a common threshold to an entire frame
 *
 * DESCRIPTION:
 * The threshold is supplied as wfsSpecific->thresh. Pixels above
 * this threshold have it subtracted from their value. Pixels below this
 * threshold have their values set to 0.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
void ospGlobalThreshold (float * buffp, 
			 struct OSP_CONTEXT * wfsSpecific)
    /**********************************************************************/
    /* Apply a global threshold. Set below-threshold values to 0,         */
    /* subtract threshold from above threshold values. This has been      */
    /* almost wholly superceded by the more comprehensive threshold()     */
    /* function, but does allow the input of float threshold values...    */
    /*    but note that in the current main, thresh is an integer         */
    /**********************************************************************/
{
    float * indxy;           /* pointer to current pixel */
    int ii;                  /* counter */

    for (ii=0; ii < wfsSpecific->xframesize * wfsSpecific->yframesize; ii++)
    {
        indxy = buffp + ii;
        if ( *indxy > wfsSpecific->thresh)           /* Apply a threshold */
            *indxy = *indxy - wfsSpecific->thresh; 
	else
            *indxy = 0;
    }
    return;
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME: 
 * ospGlobalCentroid
 *
 * INVOCATION:
 * ospGlobalCentroid(buffp, wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) buffp          (float *)  pointer to input frame buffer
 * (!) wfsSpecific    (struct OSP_CONTEXT) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * void
 *
 * PURPOSE:
 * To determine the centroid of an entire frame, in pixels
 *
 * DESCRIPTION:
 * Uses moments to calculate the centroid of an entire frame. As ever, the
 * coordinate system for the entire frame is x-y, in units of pixels,
 * with the centre of the bottom left hand pixel in the frame (1,1).
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
void ospGlobalCentroid (float * buffp,struct OSP_CONTEXT * wfsSpecific)
    /**********************************************************************/
    /* Hastily written function to calculate the coordinates of the       */
    /* global centroid, in pixels.                                        */
    /**********************************************************************/
{
    int ii;
    int jj;
    int poff;
    float total=0;
    float mu1x=0;
    float mu1y=0;
    float * indxy;
    float distx;
    float disty; 
    float factorx; 
    float factory;

    for (jj=0;jj<wfsSpecific->yframesize;jj++)
    {
	for(ii=0;ii<wfsSpecific->xframesize;ii++)
	{
	    poff = ii + jj * wfsSpecific->xframesize;
	    indxy = buffp + poff;

	    total = *indxy  + total;

	    distx = (float)ii;
	    disty = (float)jj;
	    factorx = distx * *indxy;
	    factory = disty * *indxy;
	    mu1x = mu1x + factorx;
	    mu1y = mu1y + factory;
        }
    }
    mu1x = mu1x/total + 1;
    mu1y = mu1y/total + 1;
    
    printf("Global centroid is at (%7.3f, %7.3f) \n",mu1x,mu1y);
    return;
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME: 
 * ospCentroid 
 *
 * INVOCATION:
 * ospCentroid(buffp,x0,xdiff,y0,ydiff,disp,mean,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) buffp       (float *)  pointer to input frame buffer
 * (>) x0          (int)      x coord of bottom left pixel of subap
 * (>) xdiff       (float)    x offset of null position from x0
 * (>) y0          (int)      y coord of bottom left pixel of subap
 * (>) ydiff       (float)    y offset of null position from y0
 * (<) disp        (float *)  4 element array returning centroid displacement
 *                            and variance
 * (>) mean     (float)    value subtracted from above threshold pixels
 * (>) wfsSpecific (struct OSP_CONTEXT *) wfs context structure
 *
 * FUNCTION VALUE:
 * void
 *
 * PURPOSE:
 * To calculate the centroid position and its standard deviation for a subaperture
 *
 * DESCRIPTION:
 * The centroid position in a subaperture is calculated using moments, and
 * the standard deviation estimated, taking into consideration the 
 * uncertainty estimated in the choice of threshold value for that subaperture.
 * x0,xdiff,y0,ydiff may be obtained as four consecutive elements of the
 * wfsSpecific->centres[] array, taking care to cast x0 and y0 as ints.
 * Knowledge of (x0,y0) allows the function to index to the correct start point 
 * in the frame for the subaperture, and thence to calculate the moments for
 * each pixel relative to the null position as specified by xdiff and ydiff
 * in the subaperture, given knowledge of the subaperture geometry (e.g. size)
 * from wfsSpecific. The displacements of the centroid in x and y are given by 
 * nomalising the x and y sums of the moments by the total intensity in the 
 * subaperture. If, for whatever reason, the total intensity in a 
 * subaperture should be zero, the displacements are set to zero and the 
 * standard deviations set to 99. 
 * The displacements and variances of the spot centroid are returned
 * through the disp[] array: disp[0] = x displacement
 *                           disp[1] = variance of disp[0]
 *                           disp[2] = y displacement
 *                           disp[3] = variance of disp[2]
 * NB if a subaperture has no light in it, its centroid x and y displacements
 * are set to zero, and the variances are returned as 99.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 *
 * EXTERNAL VARIABLES:
 * none
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
void ospCentroid (float * buffp, 
		  int x0, 
		  float xdiff, 
		  int y0, 
		  float ydiff, 
		  float * disp, 
		  float mean,
		  struct OSP_CONTEXT * wfsSpecific)
{
    int ii;                   /* counter */
    int jj;                   /* counter */
    float itotal;           /* total intensity in subap */
    float mu1x;             /* unnormalised/normalised sum of x moments */
    float mu1y;             /* unnormalised/normalised sum of y moments */
    float * index00;          /* ptr to first pixel (bottom L) of subaperture */
    float * indxy;            /* pointer to current pixel of subaperture */
    float * rowinc;
    float readsqcorr, tempfactor, ireptotal,irepsqtotal;

    index00 = buffp + wfsSpecific->xframesize * (y0-1) + (x0-1);
    readsqcorr = 1 + 0.01 * wfsSpecific->readsq;
    
    itotal=0;
    mu1x = 0;
    mu1y = 0; 
    ireptotal = 0;
    irepsqtotal = 0;


    for (jj = 0; jj < wfsSpecific->side; jj++)
    {   	
	rowinc = index00 + jj * wfsSpecific->xframesize;
	for (ii = 0; ii < wfsSpecific->side;ii++)
        {
	    indxy = ii + rowinc;
	    if(*indxy > 0)
            {
		itotal += *indxy;

/*******NB the intensity used to error estimates should be the original
********value, as here */

		ireptotal += (1.0/(fabs(* indxy)+mean));
		irepsqtotal += (1.0/((fabs((* indxy)+mean) * (fabs(* indxy)+mean))));
		mu1x += (float)ii * *indxy;
		mu1y += (float)jj * *indxy;
/*		printf("x0 %d y0 %d x %d y %d intensity %f mean %f\n",
		       x0,y0,ii,jj,*indxy,mean); */
	    }
        }
    }

    if(itotal > 0)
    {
	tempfactor =  ireptotal + wfsSpecific->readsq * irepsqtotal;

	*(disp) = mu1x/itotal - xdiff;
	*(disp+2) = mu1y/itotal - ydiff;

	*(disp+1) = *(disp) * *(disp) * tempfactor;
	*(disp+3) = *(disp+2) * *(disp+2) * tempfactor;

/*	if(*(disp)==0) *(disp+1)=tempfactor;
	if(*(disp)==0) *(disp+3)=tempfactor;
	*/
	

    }
    else
    {
	*(disp)   = 0;
	*(disp+2) = 0;

        *(disp+1) = 99;
	*(disp+3) = 99;
    }
    return;
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospPrintHeaders
 *
 * INVOCATION:
 * ospPrintHeaders(infile)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) infile   (char *)  name of input file
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To print out all the header keywords in all extensions of a FITS file
 *
 * DESCRIPTION:
 * Function taken from cfitsio library and modified slightly. Prints out
 * all the keywords records from all HDUs in a FITS file until an EOF is
 * is encountered.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * fitsio.h
 *
 * DEFICIENCIES:
 * None known
 *
 *-
 */
int /*STATUS*/ ospPrintHeaders ( char * infile )

    /**********************************************************************/
    /* Print out all the header keywords in all extensions of a FITS file */
    /**********************************************************************/
{
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    int status;           /* status flag relevant to cfitsio functions */
    int nkeys;            /* number of keywords in the sequence */
    int keypos;           
    int hdutype;          /* type of HDU: image, ascii or binary table */
    int ii;               /* counter */
    int jj;               /* counter */
    char filename[80];    /* string buffer for input filename */
    int ospstatus=OK;     /* overall status flag for function */
    char card[FLEN_CARD];   /* standard string lengths defined in fitsioc.h */

    strncpy(filename,infile,79);
    status = 0;
    if ( fits_open_file(&fptr, filename, READONLY, &status) ) 
    {
	ospstatus=ospPrintError( status );  
	fprintf(stderr,"...error took place in ospPrintHeaders\n");
	return(ERROR);
    }

    /* attempt to move to next HDU, until we get an EOF error */
     for (ii = 1; !(fits_movabs_hdu(fptr, ii, &hdutype, &status) ); ii++) 
     {
	 /* get no. of keywords */
	 if (fits_get_hdrpos(fptr, &nkeys, &keypos, &status) )
	 {
	     ospstatus=ospPrintError( status );  
	     fprintf(stderr,"...error took place in ospPrintHeaders\n");
	 }
	 
	 printf("Header listing for HDU #%d:\n", ii); 
	 for (jj = 1; jj <= nkeys; jj++)  {
	     if ( fits_read_record(fptr, jj, card, &status) )
	     {
		 ospstatus=ospPrintError( status );  
		 fprintf(stderr,"...error took place in ospPrintHeaders\n");
	     }
	     
	     printf("%s\n", card); /* print the keyword card */
	 }
	 printf("END\n\n");  /* terminate listing with END */
     }
    
    if (status == END_OF_FILE)   /* status values are defined in fitsioc.h */
        status = 0;              /* got the expected EOF error; reset = 0  */
    else
    {
	ospstatus=ospPrintError( status );  
	fprintf(stderr,"...error took place in ospPrintHeaders\n");
    }
    
    if ( fits_close_file(fptr, &status) )
    {
	ospstatus=ospPrintError( status );  
	fprintf(stderr,"...error took place in ospPrintHeaders\n");
    }
    
    return(ospstatus);
}
/*--------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospReadFloatImage
 *
 * INVOCATION:
 * ospReadFloatImage(buffp,infile,buffsize)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (<) buffp     (float *) pointer to buffer for image read in
 * (>) infile    (char *)  name of input FITS file
 * (>) buffsize  (int)     size of frame buffer
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To read a float image into a buffer from a FITS file
 *
 * DESCRIPTION:
 * The FITS file is opened and the NAXIS keywords read to get the image
 * size. If the size is greater than buffsize, only enough of the image to
 * fill the buffer is read in. If the image is smaller than or equal to the
 * size of the buffer, the whole image is read in. No padding to fill any 
 * unassigned elements of the buffer takes place, as the image dimensions for
 * any subsequent processing should be strictly controlled to match the 
 * xframesize and yframesize dimensions specified in the context structure.
 * The FITS file is then closed.
 * This function was written primarily for input of test data, but is also
 * used for input of the offsets frames- it may benefit from also taking 
 * the context strtucture as an argument, to enable checking of the image 
 * dimensions.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * none
 *
 * PRIOR REQUIREMENTS:
 * That the buffer pointed to by buffp has been allocated large enough to 
 * accomodate buffsize floats
 *
 * INCLUDE FILES:
 * osp.h, fitsio.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
int /*STATUS*/ ospReadFloatImage( float * buffp, char * infile, int buffsize)

    /************************************************************************/
    /* Read a FITS image and determine the minimum and maximum pixel values */
    /************************************************************************/
{
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    int status,  nfound, anynull;
    long naxes[2], fpixel, nbuffer, npixels;
    char filename[80];
    float nullval;
    int ospstatus=OK;
    strncpy(filename,infile,79);

    status = 0;

#ifdef OSP_VERBOSE
    printf(filename);
    printf(" used as argument to ospReadFloatImage\n");
#endif /*OSP_VERBOSE*/

    if ( fits_open_file(&fptr, filename, READONLY, &status) )
    {
	ospstatus=ospPrintError( status );   
	fprintf(stderr,"...error took place in ospReadFloatImage\n");
	return(ERROR);
    }

    /* read the NAXIS1 and NAXIS2 keyword to get image size */
    if ( fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status) )
    {
	ospstatus=ospPrintError( status ); 
	fprintf(stderr,"...error took place in ospReadFloatImage\n");
    }

    npixels  = naxes[0] * naxes[1];         /* number of pixels in the image */
    fpixel   = 1;
    nullval  = 0;                /* don't check for null values in the image */

    nbuffer = npixels;
    if (npixels > buffsize)
    {
	nbuffer = buffsize;  /* read as many pixels as will fit in buffer */
	fprintf(stderr,
		"Image bigger than buffer allocated- only filling buffer.\n");
	fprintf(stderr,"...took place in ospReadFloatImage\n");
    }

	/* Note that even though the FITS images contains unsigned integer */
	/* pixel values (or more accurately, signed integer pixels with    */
	/* a bias of 32768),  this routine is reading the values into a    */
	/* float array.   Cfitsio automatically performs the datatype      */
	/* conversion in cases like this.                                  */

    if ( fits_read_img(fptr, TFLOAT, fpixel, nbuffer, &nullval,
		       buffp, &anynull, &status) )
    {
	ospstatus=ospPrintError( status );  
	fits_close_file(fptr,&status);
	fprintf(stderr,"...error took place in ospReadFloatImage\n");
	return(ERROR);
    }
    
    if ( fits_close_file(fptr, &status) )
    {
	ospstatus=ospPrintError( status );    /* call ospPrintError if error occurs */
	fprintf(stderr,"...error took place in ospReadFloatImage\n");
    }
    
    return(ospstatus);
}

/*
 *+
 * FUNCTION NAME:
 * ospReadUShortImage
 *
 * INVOCATION:
 * ospReadUshortImage(buffp,infile,buffsize)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (<) buffp     (unsigned short int *) pointer to buffer for image read in
 * (>) infile    (char *)  name of input FITS file
 * (>) buffsize  (int)     size of frame buffer
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To read an unsigned short int image into a buffer from a FITS file
 *
 * DESCRIPTION:
 * The FITS file is opened and the NAXIS keywords read to get the image
 * size. If the size is greater than buffsize, only enough of the image to
 * fill the buffer is read in. If the image is smaller than or equal to the
 * size of the buffer, the whole image is read in. No padding to fill any 
 * unassigned elements of the buffer takes place, as the image dimensions for
 * any subsequent processing should be strictly controlled to match the 
 * xframesize and yframesize dimensions specified in the context structure.
 * The FITS file is then closed.
 * This function was written primarily for input of test data, but may 
 * benefit from also taking the context strtucture as an argument, to 
 * enable checking of the image dimensions. 
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * none
 *
 * PRIOR REQUIREMENTS:
 * That the buffer pointed to by buffp has been allocated large enough to 
 * accomodate buffsize unsigned short ints
 *
 * INCLUDE FILES:
 * osp.h, fitsio.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
int /*STATUS*/ ospReadUShortImage( unsigned short int * buffp, char * infile, int buffsize)

    /************************************************************************/
    /* Read a FITS image and determine the minimum and maximum pixel values */
    /************************************************************************/
{
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    int status,  nfound, anynull;
    long naxes[2], fpixel, nbuffer, npixels;
    char filename[80];
    unsigned short int nullval;
    int ospstatus=OK;
    
    strncpy(filename,infile,79);

    status = 0;

#ifdef OSP_VERBOSE
    printf(filename);
    printf(" used as argument to ospReadShortImage\n");
#endif /*OSP_VERBOSE*/

    if ( fits_open_file(&fptr, filename, READONLY, &status) )
    {
	ospstatus=ospPrintError( status );   
	fprintf(stderr,"...error took place in ospReadUShortImage\n");
	return(ERROR);
    }

    /* read the NAXIS1 and NAXIS2 keyword to get image size */
    if ( fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status) )
    {
	    ospstatus=ospPrintError( status );  
	    fprintf(stderr,"...error took place in ospReadUShortImage\n");
    }

    npixels  = naxes[0] * naxes[1];         /* number of pixels in the image */
    fpixel   = 1;
    nullval  = 0;                /* don't check for null values in the image */

/*    while (npixels > 0) 
    {*/
    nbuffer = npixels;
    if (npixels > buffsize)
    {
	nbuffer = buffsize;  /* read as many pixels as will fit in buffer */
	fprintf(stderr,
		"Image bigger than buffer allocated- only filling buffer.\n");
	fprintf(stderr,"... took place in ospReadUShortImage\n");
    }

	/* Note that even though the FITS images contains unsigned integer */
	/* pixel values (or more accurately, signed integer pixels with    */
	/* a bias of 32768),  this routine is reading the values into a    */
	/* float array.   Cfitsio automatically performs the datatype      */
	/* conversion in cases like this.                                  */

	if ( fits_read_img(fptr, TUSHORT, fpixel, nbuffer, &nullval,
                  buffp, &anynull, &status) )
	{
	    ospstatus=ospPrintError( status );   
	    fits_close_file(fptr,&status);
	    fprintf(stderr,"...error took place in ospReadUShortImage\n");
	    return(ERROR);
	}

	if ( fits_close_file(fptr, &status) )
	{
	    ospstatus=ospPrintError( status );  
	    fprintf(stderr,"...error took place in ospReadUShortImage\n");
	}

/*    npixels -= nbuffer;  
      fpixel  += nbuffer;    
    }
*/
    return(ospstatus);
}

/*--------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospPrintError
 *
 * INVOCATION:
 * ospPrintError(status)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) status (int)  status value relevant to cfitsio functions
 *
 * FUNCTION VALUE:
 * (int) A status value equal to ERROR.
 *
 * PURPOSE:
 * To report cfitsio errors, as related to the status codes in fitsio.h
 *
 * DESCRIPTION:
 * Receives a status code from the cfitsio function which calls it, 
 * prints the appropriate error report, and returns an ERROR value.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * none
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * fitsio.h
 *
 * DEFICIENCIES:
 * None known
 *-
 */
int /*STATUS*/ ospPrintError( int status)
{
    /*****************************************************/
    /* Print out cfitsio error messages and exit program */
    /*****************************************************/


    if (status)
    {
       fits_report_error(stderr, status); /* print error report */
    }
    return(ERROR);
}

/*
 *+
 * FUNCTION NAME:
 * ospWriteFloatImage
 *
 * INVOCATION:
 * ospWriteFloatImage(buffp, outfile, xarraysize, yarraysize)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) buffp    (float *) pointer to float image to be written to FITS file
 * (>) outfile  (char *)  name of output FITS file
 * (>) xarraysize (int)   number of pixels in x of image
 * (>) yarraysize (int)   number of pixels in y of image
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To write a float image to a FITS file
 *
 * DESCRIPTION:
 * A 2-d float image is written as a FITS primary array, with a minimal
 * header. If the detector geometry is required to be added to the header
 * use ospAddContextToHeader.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * none
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * INCLUDE FILES:
 * fitsio.h
 *
 * DEFICIENCIES:
 * None known
 *
 *-
 */
int /*STATUS*/ ospWriteFloatImage( float * buffp, char * outfile,int xarraysize,int yarraysize)
{
    fitsfile *fptr;         /* pointer to the FITS file, defined in fitsio.h */
    long  fpixel;           /* index of first pixel to write */
    long nelements;         /* number of pixels to write */
    char filename[80];      /* buffer for fileneame */
    int ospstatus=OK;       /* overall status flag for function */
    int status;             /* status flag for cfitsio functions called*/
    int bitpix = FLOAT_IMG; /* bits per pixel: FLOAT_IMG = -32 */
    long naxis    =   2;    /* number of dimensions in the FITS array */  
    long naxes[2];          /* array to store the FITS array dimensions */
    
    naxes[0]=xarraysize;
    naxes[1]=yarraysize;

    strncpy(filename,outfile,79);

    remove(filename);               /* Delete old file if it already exists */

    status = 0;         /* initialize status before calling fitsio routines */

    if (fits_create_file(&fptr, filename, &status)) /* create new FITS file */
    {
	ospstatus=ospPrintError( status );    
	fprintf(stderr,"...error took place in ospWriteFloatImage\n");
	return(ERROR);
    }

    /* write the required keywords for the primary array image.     */
    /* Since bitpix = FLOAT_IMG, this will cause cfitsio to create  */
    /* a FITS image with BITPIX = -32 (float).                      */
    /* in this case.                                                */

    if ( fits_create_img(fptr,  bitpix, naxis, naxes, &status) )
    {
	ospstatus=ospPrintError( status );    
	fprintf(stderr,"...error took place in ospWriteFloatImage\n");
    }

    fpixel = 1;                               /* first pixel to write      */
    nelements = naxes[0] * naxes[1];          /* number of pixels to write */

    /* write the array of floats to the FITS file */
    if ( fits_write_img(fptr, TFLOAT, fpixel, nelements, buffp, &status) ) /*+*/
    {
	ospstatus=ospPrintError( status );   
	fprintf(stderr,"...error took place in ospWriteFloatImage\n");
    }



    if ( fits_close_file(fptr, &status) )                /* close the file */
    {
	ospstatus=ospPrintError( status );   
	fprintf(stderr,"...error took place in ospWriteFloatImage\n");
    }

    return(ospstatus);
}
/*--------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospWriteUShortImage
 *
 * INVOCATION:
 * ospWriteUShortImage(buffp, outfile, xarraysize, yarraysize)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) buffp    (unsigned short int *) pointer to unsigned short int image 
 *                                     to be written to FITS file
 * (>) outfile  (char *)  name of output FITS file
 * (>) xarraysize (int)   number of pixels in x of image
 * (>) yarraysize (int)   number of pixels in y of image
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To write an unsigned short int image to a FITS file
 *
 * DESCRIPTION:
 * A 2-d unsigned short int  image is written as a FITS primary array, with a
 * minimal header. If the detector geometry is required to be added to the 
 * header use ospAddContextToHeader.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * none
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * INCLUDE FILES:
 * fitsio.h
 *
 * DEFICIENCIES:
 * None known
 *
 *-
 */
int /*STATUS*/ ospWriteUShortImage( unsigned short int * buffp, char * outfile,int xarraysize,int yarraysize)
{
    fitsfile *fptr;         /* pointer to the FITS file, defined in fitsio.h */
    int ospstatus=OK;       /* overall status flag for function */
    int status;             /* status flag for cfitsio functions called */
    long  fpixel;           /* index of first pixel to write */
    long nelements;         /* number of pixels to write */
    char filename[80];      /* buffer for filename */
    int bitpix = USHORT_IMG;/* bits per pixel */  
    long naxis    =   2;    /* number of dimensions in the FITS array */
    long naxes[2];          /* array to store the FITS array dimensions */
    
    naxes[0]=xarraysize;
    naxes[1]=yarraysize;

    strncpy(filename,outfile,79);

    remove(filename);               /* Delete old file if it already exists */

    status = 0;         /* initialize status before calling fitsio routines */

    if (fits_create_file(&fptr, filename, &status)) /* create new FITS file */
    {
	ospstatus=ospPrintError( status );   
	fprintf(stderr,"...error took place in ospWriteUshortImage\n");
	return(ERROR);
    }


    /* write the required keywords for the primary array image.     */
    /* Since bitpix = USHORT_IMG, this will cause cfitsio to create */
    /* a FITS image with BITPIX = 16 (signed short integers) with   */
    /* BSCALE = 1.0 and BZERO = 32768.  This is the convention that */
    /* FITS uses to store unsigned integers.  Note that the BSCALE  */
    /* and BZERO keywords will be automatically written by cfitsio  */
    /* in this case.                                                */

    if ( fits_create_img(fptr,  bitpix, naxis, naxes, &status) )
    {
	ospstatus=ospPrintError( status );    
	fprintf(stderr,"...error took place in ospWriteUshortImage\n");
    }
 
    fpixel = 1;                               /* first pixel to write      */
    nelements = naxes[0] * naxes[1];          /* number of pixels to write */

    /* write the array of unsigned shorts to the FITS file */
    if ( fits_write_img(fptr, TUSHORT, fpixel, nelements, buffp, &status) ) /*+*/
    {
	ospstatus=ospPrintError( status );    
	fprintf(stderr,"...error took place in ospWriteUshortImage\n");
    }


    if ( fits_close_file(fptr, &status) )                /* close the file */
    {
	ospstatus=ospPrintError( status );    
	fprintf(stderr,"...error took place in ospWriteUshortImage\n");
    }

    return(ospstatus);
}

/*
 *+
 * FUNCTION NAME: 
 * ospFGCentroidWrapper
 *
 * INVOCATION:
 * ospFGCentroidWrapper(buffp,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) buffp    (float *) pointer to input buffer containing image of SH spots
 * (!) wfsSpecific (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int)   A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To perform offset correction, thresholding and centroiding on a data frame quickly for fast guiding
 *
 * DESCRIPTION:
 * Performs subtraction and  multiplication of offset frames, thresholding and 
 * centroiding of the resultant frame, and estimation of the variances of the 
 * centroid positions. These calculated values are stored directly in the
 * wfsSpecific->s[] and wfsSpecific->dssq[] arrays of the context structure.
 * NB if a subaperture has no light in it, the centroid position is not
 * updated, which may be advantageous for minor glitches, but the variances are
 * returned as 99 for the subaperture in this case.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * That ospCalculateSubaps has generated the centres[] array.
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * Corner averaging of subap pixels for thresholding is not available, error 
 * estimation is simplified
  *-
 */
int /*STATUS*/ ospFGCentroidWrapper(float * buffp, 
				  struct OSP_CONTEXT * wfsSpecific)
{
    int j;            /* general purpose counter */
    int i=1;          /* general purpose counter */
    int ii;                   /* counter */
    int jj;                   /* counter */
    int x0;
    int y0;
    int buffSize;
    int ospstatus=OK;
    float localbuff[OSP_BUFFMAX];
    float tempfactor;
    float readsqcorr;
    float *p, *redsubp, *redmulp, * maxp;
    float xdiff;
    float ydiff;
    float itotal=0;           /* total intensity in subap */
    float ireptotal;
    float mu1x=0;             /* unnormalised/normalised sum of x moments */
    float mu1y=0;             /* unnormalised/normalised sum of y moments */
    float * index00;          /* ptr to first pixel (bottom L) of subaperture */
    float * indxy;            /* pointer to current pixel of subaperture */
    float pixval;
    float * rowinc;
    int spotcount=0;

    buffSize = wfsSpecific->xframesize * wfsSpecific->yframesize ;
    maxp = (float *)((int)buffp + buffSize*sizeof(float)) ;
    redsubp = wfsSpecific->redsubbuff ;
    redmulp = wfsSpecific->redmultbuff ;
    readsqcorr = 1 + 0.01*wfsSpecific->readsq;

    j=0;
    /* Substract and multiply in one time */
    for ( p = buffp ; p < maxp ; )
    {
	localbuff[j++] = (*(p++) - *(redsubp ++)) * (*(redmulp ++)) ;
    }
    /* Threshold and centroid */
    for (j = 1; j <= wfsSpecific->centres[0]; j=j+4) 
    {
	itotal=0;
	ireptotal=0;
	mu1x=0;
	mu1y=0;
	x0 = (int)(wfsSpecific->centres[j]);
	xdiff = wfsSpecific->centres[j+1];
	y0 = (int)(wfsSpecific->centres[j+2]);
	ydiff = wfsSpecific->centres[j+3];
/*	index00 = buffp + wfsSpecific->xframesize * (y0-1) + (x0-1);*/
	index00 = localbuff + wfsSpecific->xframesize * (y0-1) + (x0-1);
 
	for (jj = 0; jj < wfsSpecific->side; jj++)
	{   
	    rowinc = index00 + jj * wfsSpecific->xframesize;
	    
	    for (ii = 0; ii < wfsSpecific->side;ii++)
	    {
		indxy = ii + rowinc;
		pixval = *indxy - wfsSpecific->thresh;
		if(pixval > 0)
		{		
		    itotal += pixval ;
		    ireptotal = ireptotal + (1.0 / fabs(*indxy));
		    mu1x += (float)ii * pixval ;
		    mu1y += (float)jj * pixval ;
		}
	    }
	}
	if(itotal > 0)
	{
	    tempfactor = readsqcorr* ireptotal;
	    spotcount++;

	    wfsSpecific->s[i] = (mu1x/itotal) - xdiff;
	    wfsSpecific->s[i+1] = (mu1y/itotal) - ydiff;
	    wfsSpecific->dssq[i] = wfsSpecific->s[i]*wfsSpecific->s[i]
		*tempfactor;
	    wfsSpecific->dssq[i+1] = wfsSpecific->s[i+1] * wfsSpecific->s[i+1]
		* tempfactor;
	}
	else
	{

/* NB if a subaperture has no light in it, the centroid position is not
 * updated, which may be advantageous for minor glitches, but the variance is
 * returned as 99 in this case.
 */
            wfsSpecific->s[i] = 0.0;    /* new line */
            wfsSpecific->s[i+1] = 0.0;  /* new line */
	    wfsSpecific->dssq[i] = OSP_NO_LIGHT;
	    wfsSpecific->dssq[i+1] = OSP_NO_LIGHT;
	}


	i+=2;
    }

    /* protect write to ospdiag with guard fields */

    /* set guard field */

    wfsSpecific->ospdiag[GUARD1] = 1.0;    
   
    /* write data */

    wfsSpecific->ospdiag[1] = (float)spotcount;

    /* unset guard field - write complete */

    wfsSpecific->ospdiag[GUARD1] = 0.0;    

    return(ospstatus);
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospFGMeasure
 *
 * INVOCATION:
 * ospFGMeasure(buffp,wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) buffp        (float *) input frame buffer
 * (!) wfsSpecific  (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To calculate Zernike coefficients from a frame of SH spots, and write them to the synchro bus
 *
 * DESCRIPTION:
 * Subtractive and multiplicative offset frames are applied to the input
 * pointed to by buffp, and thresholding and centroiding takes place, through
 * a call to ospFGCentroidWrapper. The Zernike coefficients are calculated
 * and stored in the wfs context structure. Measurement variances are calculated
 * and added to the fitting variances and the square root taken to give the
 * error estimates for the Zernike coefficients, which are also stored in the 
 * wfs context structure. Under vxWorks, the Zernike coefficients and their 
 * standard deviations are written to the synchro bus. The input frame buffer 
 * is unmodified, unlike when using ospMeasure.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 * Need to check the return values of the functions called. Corner averaging 
 * for thresholding is not available. Error estimation is simplified, compared
 * with ospMeasure.
 *
 *-
 */
int /*STATUS*/ ospFGMeasure(float * buffp, struct OSP_CONTEXT * wfsSpecific)
{
    int ospstatus = OK;
    int i;
    ospFGCentroidWrapper(buffp,wfsSpecific);
    ospApplyControlMatrix(wfsSpecific->c,wfsSpecific->z,wfsSpecific->s,
			  wfsSpecific->np,wfsSpecific->mp); 
    wfsSpecific->z[1] *= wfsSpecific->tipscale;
    wfsSpecific->z[2] *= wfsSpecific->tiltscale;
    wfsSpecific->z[3] *= wfsSpecific->focusscale;

    ospMeasVars(wfsSpecific);
    for (i=1;i<=wfsSpecific->np;i++)
    {
        wfsSpecific->err[i]=sqrt(wfsSpecific->fvars[i]+wfsSpecific->mvars[i]);
    }

#ifdef vxWorks
    if ( timeNow (&(wfsSpecific->time)) != OK )
    {
    	fprintf (stderr,
	         "Error: ospFGMeasure failed to take bancom time\n" ) ;
    } ;

    ospstatus=writeWfsToSynchro(wfsSpecific);
#endif /*vxWorks*/

    return(ospstatus);
}

/* Added by CB Feb 9 1999 in order to have simple tip/tilt correction */

int ospTracking ( float * buffp , 
		  struct OSP_CONTEXT * wfsSpecific )
{
    int i, j ;
    int buffSize ;
    float x ;
    float y ;
    float total ;
    float pixval ;
    float cosAngle , sinAngle ;
    float localbuff[OSP_BUFFMAX] ;
    float *p, *redsubp, *redmulp, * maxp, *localp ;

    buffSize = wfsSpecific->xframesize * wfsSpecific->yframesize ;
    maxp = (float *)((int)buffp + buffSize*sizeof(float)) ;
    redsubp = wfsSpecific->redsubbuff ;
    redmulp = wfsSpecific->redmultbuff ;
    localp = localbuff;
    cosAngle = (float) cos(wfsSpecific->angle) ;
    sinAngle = (float) sin(wfsSpecific->angle) ;


    /* Substract and multiply in one time */
    /*for ( p = buffp ; p < maxp ; )
    {
	*(localp++) = (*(p++) - *(redsubp ++)) * (*(redmulp ++)) ;
    }*/

    /* Threshold and centroid */
    total = 0 ;
    x = 0 ;
    y = 0 ;
    localp = buffp ;
    for ( j = 0 ; j < wfsSpecific->yframesize ; j ++ ) 
    {
	for ( i = 0 ; i < wfsSpecific->xframesize ; i ++)
	{   
            pixval = *(localp + wfsSpecific->xframesize*j + i) ;

            if ( pixval > wfsSpecific->thresh )
            {
               x += pixval*(i+1) ;
               y += pixval*(j+1) ;
               total += pixval ;
               /*printf ( "i=%d, j=%d, pixval=%f, x=%f, y=%f, total=%f\n" , i , j, pixval , x , y , total ) ;*/ 
            }
        }
     }

     if ( total > 0 )
     {
        /*printf ( "x=%f, y=%f, total=%f\n" , x , y , total ) ;*/ 
        wfsSpecific->s[1] = (x / total) - wfsSpecific->xcenter ;
        wfsSpecific->s[2] = (y / total) - wfsSpecific->ycenter ;
	wfsSpecific->dssq[1] = 0.0 ;
	wfsSpecific->dssq[2] = 0.0 ;
        wfsSpecific->z[1] = wfsSpecific->tipscale * ( cosAngle*(wfsSpecific->s[1]) - sinAngle*(wfsSpecific->s[2]) ) ; 
        wfsSpecific->z[2] = wfsSpecific->tiltscale * ( sinAngle *(wfsSpecific->s[1]) + cosAngle*(wfsSpecific->s[2]) ) ; 
     }
     else
     {
        wfsSpecific->s[1] = 0.0 ;
        wfsSpecific->s[2] = 0.0 ;
	wfsSpecific->dssq[1] = OSP_NO_LIGHT;
	wfsSpecific->dssq[2] = OSP_NO_LIGHT;
        wfsSpecific->z[1] = 0.0 ;
        wfsSpecific->z[2] = 0.0 ;
     }
     wfsSpecific->z[3] = 0.0 ;
     wfsSpecific->err[1] = 0.0 ;
     wfsSpecific->err[2] = 0.0 ;
     wfsSpecific->err[3] = 0.0 ;

    /* protect write to ospdiag with guard fields */

    /* set guard field */

    wfsSpecific->ospdiag[GUARD1] = 1.0;    
   
    /* write data */

    wfsSpecific->ospdiag[1] = (float)(1.0);

    /* unset guard field - write complete */

    wfsSpecific->ospdiag[GUARD1] = 0.0;    

#ifdef vxWorks
    if ( timeNow (&(wfsSpecific->time)) != OK )
    {
       fprintf (stderr,
       "Error: ospTracking failed to take bancom time\n" ) ;
    } ;
  
    if ( writeWfsToSynchro(wfsSpecific) != OK )
    {
       fprintf (stderr,
       "Error: ospTracking failed to write data to tcs\n" ) ;
    } ;
#endif /*vxWorks*/

    return (OK) ;
}

/*------------------------------------------------------------------------*/

/* Add by CB, allow to compute only averaged focus                        */
/* Fev 10 1999                             */
/* Comments to be added */

int ospCoAddFocus ( float *buffp , 
                    int N ,
                    struct OSP_CONTEXT *wfsSpecific )
{

    int   buffSize;        /* size of frame buffer */
    int   i ;
    float *p;
    float *bp;
    float *sumbuffp;
    float *maxp;
    float focusX, focusY, focus ;

#ifdef DEBUG
    if ( (N <= 0) ) 
    {
       fprintf ( stderr,
                 "Error: ospCoAddFocus called with wrong number of frames\n" ) ;
       return ( ERROR ) ;
    }
#endif
    
    buffSize = wfsSpecific->xframesize * wfsSpecific->yframesize;
    sumbuffp = wfsSpecific->sumbuff;
    maxp = (float *)((int)sumbuffp + buffSize * sizeof(float)) ;
    bp = buffp ;

    if ( wfsSpecific->coaddcounter == 0 )
    {
       for ( p = sumbuffp ; p < maxp ; )
        {
	    *(p++) = *(bp++) ;
	}
       
        wfsSpecific->coaddcounter ++;
    }
    else
    {
        if ( wfsSpecific->coaddcounter < N )
        {
	   for ( p = sumbuffp ; p < maxp ; )
	   {
	       *p = *(p++) + *(bp++);
	   }
	
           wfsSpecific->coaddcounter ++;
        }
    }

    if ( wfsSpecific->coaddcounter == N )
    {
       for ( p = sumbuffp ; p < maxp; )
       {
           * p = * (p++) / wfsSpecific->coaddcounter;
       }
       wfsSpecific->coaddcounter=0;
           
       if ( ospFGCentroidWrapper ( sumbuffp , wfsSpecific ) == ERROR )
       {
          fprintf ( stderr,
                    "Error: ospCoAddFocus centroid computation fails...\n" ) ;
          return ( ERROR ) ;
       } ;

       ospRotateCentroids ( wfsSpecific ) ;

       focusX = 0.0 ;
       focusY = 0.0 ;
       focus = 0.0 ;
       
       if ( (wfsSpecific->dssq[1] != OSP_NO_LIGHT) && (wfsSpecific->dssq[2] != OSP_NO_LIGHT) &&
            (wfsSpecific->dssq[3] != OSP_NO_LIGHT) && (wfsSpecific->dssq[4] != OSP_NO_LIGHT) &&
            (wfsSpecific->dssq[5] != OSP_NO_LIGHT) && (wfsSpecific->dssq[6] != OSP_NO_LIGHT) &&
            (wfsSpecific->dssq[7] != OSP_NO_LIGHT) && (wfsSpecific->dssq[8] != OSP_NO_LIGHT) )
       {
          focusX = (wfsSpecific->s[3] + wfsSpecific->s[7]) - 
                   (wfsSpecific->s[1] + wfsSpecific->s[5]) ;
          focusY = (wfsSpecific->s[6] + wfsSpecific->s[8]) - 
                   (wfsSpecific->s[2] + wfsSpecific->s[4]) ;

          focusX = focusX/4 ;
          focusY = focusY/4 ;

          /*printf ( "focusX = %f, focusY = %f\n" , focusX , focusY ) ;*/
          if ( (focusX > 0.0) && (focusY > 0.0) )
          {
             if ( focusX < focusY )
                focus = focusX ;
             else
                focus = focusY ;
          }
          else if ( (focusX < 0.0) && (focusY < 0.0) )
          {
             if ( focusX < focusY )
                focus = focusY ;
             else
                focus = focusX ;
          }
          else
          {
             focus = 0.0 ;
          }
       }
       else
       {
          focus = 0.0 ;
       }
       wfsSpecific->z[3] = wfsSpecific->focusscale * focus ;
    }

    return(OK);
}

int ospTrackingAndFocus ( float * buffp , 
                          int N ,
		          struct OSP_CONTEXT * wfsSpecific )
{
    int i, j ;
    int buffSize ;
    float x ;
    float y ;
    float total ;
    float pixval ;
    float cosAngle , sinAngle ;
    float localbuff[OSP_BUFFMAX] ;
    float *p, *redsubp, *redmulp, * maxp, *localp ;

    buffSize = wfsSpecific->xframesize * wfsSpecific->yframesize ;
    maxp = (float *)((int)buffp + buffSize*sizeof(float)) ;
    redsubp = wfsSpecific->redsubbuff ;
    redmulp = wfsSpecific->redmultbuff ;
    localp = localbuff;
    cosAngle = (float) cos(wfsSpecific->angle) ;
    sinAngle = (float) sin(wfsSpecific->angle) ;


    /* Substract and multiply in one time */
    /*for ( p = buffp ; p < maxp ; )
    {
	*(localp++) = (*(p++) - *(redsubp ++)) * (*(redmulp ++)) ;
    }*/

    /* Threshold and centroid */
    total = 0 ;
    x = 0 ;
    y = 0 ;
    localp = buffp ;
    for ( j = 0 ; j < wfsSpecific->yframesize ; j ++ ) 
    {
	for ( i = 0 ; i < wfsSpecific->xframesize ; i ++)
	{   
            pixval = *(localp + wfsSpecific->xframesize*j + i) ;

            if ( pixval > wfsSpecific->thresh )
            {
               x += pixval*(i+1) ;
               y += pixval*(j+1) ;
               total += pixval ;
               /*printf ( "i=%d, j=%d, pixval=%f, x=%f, y=%f, total=%f\n" , i , j, pixval , x , y , total ) ;*/ 
            }
        }
     }

     if ( total > 0 )
     {
        /*printf ( "x=%f, y=%f, total=%f\n" , x , y , total ) ; */
        wfsSpecific->s[1] = (x / total) - wfsSpecific->xcenter ;
        wfsSpecific->s[2] = (y / total) - wfsSpecific->ycenter ;
	wfsSpecific->dssq[1] = 0.0 ;
	wfsSpecific->dssq[2] = 0.0 ;
        wfsSpecific->z[1] = wfsSpecific->tipscale * ( cosAngle*(wfsSpecific->s[1]) - sinAngle*(wfsSpecific->s[2]) ) ; 
        wfsSpecific->z[2] = wfsSpecific->tiltscale * ( sinAngle *(wfsSpecific->s[1]) + cosAngle*(wfsSpecific->s[2]) ) ; 
     }
     else
     {
        wfsSpecific->s[1] = 0.0 ;
        wfsSpecific->s[2] = 0.0 ;
	wfsSpecific->dssq[1] = OSP_NO_LIGHT;
	wfsSpecific->dssq[2] = OSP_NO_LIGHT;
        wfsSpecific->z[1] = 0.0 ;
        wfsSpecific->z[2] = 0.0 ;
     }
     /*wfsSpecific->z[3] = 0.0 ;*/
     wfsSpecific->err[1] = 0.0 ;
     wfsSpecific->err[2] = 0.0 ;
     wfsSpecific->err[3] = 0.0 ;

    /* Compute Focus */

    if ( ospCoAddFocus ( buffp , N , wfsSpecific ) != OK )
    {
       fprintf (stderr,
       		"Error: ospTrackingAndFocus failed to coadd focus\n" ) ;
    };

    /* protect write to ospdiag with guard fields */

    /* set guard field */

    wfsSpecific->ospdiag[GUARD1] = 1.0;    
   
    /* write data */

    wfsSpecific->ospdiag[1] = (float)(1.0);

    /* unset guard field - write complete */

    wfsSpecific->ospdiag[GUARD1] = 0.0;    

#ifdef vxWorks
    if ( timeNow (&(wfsSpecific->time)) != OK )
    {
       fprintf (stderr,
       "Error: ospTrackingAndFocus failed to take bancom time\n" ) ;
    } ;
  
    if ( writeWfsToSynchro(wfsSpecific) != OK )
    {
       fprintf (stderr,
       "Error: ospTrackingAndFocus failed to write data to tcs\n" ) ;
    } ;
#endif /*vxWorks*/

    return (OK) ;
}


/* Added by CB 23Janv1999 in order to have simple tip/tilt correction */

int ospTtCor ( float *buffp ,
	       int output , 
               struct OSP_CONTEXT *wfsSpecific )
{
    int ospstatus = OK;
    int i ;
    int nbSubApertureUsed ;
    float meanX, meanY ;

    if ( ospFGCentroidWrapper ( buffp , wfsSpecific ) == ERROR )
    {
       fprintf ( stderr,
                 "Error: ospTtCor centroid computation fails...\n" ) ;
       return ( ERROR ) ;
    } ;
    ospRotateCentroids ( wfsSpecific ) ;

    meanX = 0.0 ;
    meanY = 0.0 ;
    nbSubApertureUsed = 0 ;
    for ( i = 1 ; i <= wfsSpecific->mp ;  i += 2 )
    {
        if ( (wfsSpecific->dssq[i] != OSP_NO_LIGHT) && (wfsSpecific->dssq[i+1] != OSP_NO_LIGHT) )
        {
           meanX += wfsSpecific->s[i] ;
           meanY += wfsSpecific->s[i+1] ;
           nbSubApertureUsed ++ ;
        } ;
    }
    if ( nbSubApertureUsed != 0 )
    { 
       meanX = meanX/nbSubApertureUsed ;
       meanY = meanY/nbSubApertureUsed ;
       wfsSpecific->z[1] = meanX * wfsSpecific->tipCor * wfsSpecific->tipscale ;
       wfsSpecific->z[2] = meanY * wfsSpecific->tiltCor * wfsSpecific->tiltscale ; 
       /*printf ("Number of subapertures used=%d, z1=%f, z2=%f\n", nbSubApertureUsed,
           wfsSpecific->z[1], wfsSpecific->z[2] );*/
    }
    else
    {
       wfsSpecific->z[1] = 0.0 ;
       wfsSpecific->z[2] = 0.0 ;
       /*printf ("Number of subapertures used=0\n");*/
    }

    wfsSpecific->z[3] = 0.0 ;

    ospMeasVars(wfsSpecific);
    for (i=1;i<=wfsSpecific->np;i++)
    {
        wfsSpecific->err[i]=sqrt(wfsSpecific->fvars[i]+wfsSpecific->mvars[i]);
    }

#ifdef vxWorks
    if ( timeNow (&(wfsSpecific->time)) != OK )
    {
    	fprintf (stderr,
	         "Error: ospTtCor failed to take bancom time\n" ) ;
    } ;

    if ( output == 1 )
       ospstatus=writeWfsToSynchro(wfsSpecific);
    else
       ospstatus=writeWfsToTcs(wfsSpecific);
#endif /*vxWorks*/

    return(ospstatus);
}
/*------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 *
 *
 * INVOCATION:
 *
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 *
 *
 * FUNCTION VALUE:
 *
 *
 * PURPOSE:
 *
 *
 * DESCRIPTION:
 *
 *
 * EXTERNAL VARIABLES:
 *
 *
 * PRIOR REQUIREMENTS:
 *
 * INCLUDE FILES:
 *
 *
 * DEFICIENCIES:
 *
 *-
 */

/*+
 * FUNCTION NAME:
 * ospNewCalibrate
 *
 * INVOCATION:
 * ospNewCalibrate(calibpath, wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) calibpath (char *) path to directory of calibration data
 * (!) wfsSpecific (struct OSP_CONTEXT *) pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To populate a reconstructor matrix and invert to obtain control matrix
 *
 * DESCRIPTION:
 * This differs from ospCalibrate in that the files and information required
 * are prompted for.
 * Calibration frames of displaced SH spots corresponding to linearly
 * idependent combinations of known Zernike
 * aberrations are used to form a reconstructor matrix with respect to the
 * basis functions defined by the linear combinations. This reconstructor matrix
 * is transformed into one with respect to the individual Zernike polynomials
 * of unit magnitude, and further inverted to provide the necessary control
 * matrix which is applied to the SH spot displacements in an arbitrary
 * input frame to yield the Zernike coefficients. The control matrix is written
 * to a file specified by the character string wfsSpecific->controlfile, and the
 * calculated fitting variances written to a file specified by the character
 * string wfsSpecific->fvarsfile. If either of these files exist, they are 
 * copied to a file with a .bak extension added, befeore the new version is 
 * created.
 * Author: Steven Heddle, UKATC, Edinburgh 18/1/1999
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 *
 *
 * INCLUDE FILES:
 * osp.h, fitsio.h
 *
 * DEFICIENCIES:
 * Not general enough-need to prompt for data input, basis functions etc.
 *
 *-
 */
int /*STATUS*/ ospNewCalibrate(char * calibpath, struct OSP_CONTEXT * wfsSpecific) 
{
    float * buffp;
    float buffer[OSP_BUFFMAX];
    float *w;
    float **a;
    float **u;
    float **v;
    float **f;
    float **ca;
    float **finv;
    int i;
    int j;
    int ospstatus=OK;
/*  int k; */
    int tempflag;
    int tempxsize;
    int tempysize;
    char controltempname[OSP_MAXSTR];
    char fvarstempname[OSP_MAXSTR];

    FILE *fp1, *fp2, *fp3;
    char ffilename[OSP_MAXSTR];
    char calimfilename[OSP_MAXSTR];
    char tempname[OSP_MAXSTR];
    char flagtext[30];
    char createflag;
    char scramflag;
    char typeflag;
    char dummy;
    float coeff;
    int basisdim; 
    int ii;
    int jj;

    buffp = buffer;
    w=vector(1,OSP_ZMAX);
    a=matrix(1,wfsSpecific->mp,1,OSP_ZMAX);
    u=matrix(1,wfsSpecific->mp,1,OSP_ZMAX);
    v=matrix(1,OSP_ZMAX,1,OSP_ZMAX);
    f=matrix(1,OSP_ZMAX,1,OSP_ZMAX);
    ca=matrix(1,OSP_ZMAX,1,OSP_ZMAX);
    finv=matrix(1,OSP_ZMAX,1,wfsSpecific->mp);

/**** Populate reconstructor, and [f], matrix showing basis function
***** representations in terms of Zernike polynomials */

/**** Do so using fits images corresponding individual Zernike coefficients 
***** of magnitude OSP_TESTMAG. In general the fits images may be made up of
***** linear combinations of Zernikes, and the f[][] matrix must be modified 
***** to take account of this. */

/**** N.B. We read a centres file that is specific to the calibration
***** data here. The test data may need a centres file with different null
***** positions, which is input at the command line. */

/*    ospSaveNullPositions("../data/z0_perf5.fits", buffp, 
                           s,ds,wfsSpecific);
			 */

    tempflag = wfsSpecific->framesizeflag;
    tempxsize = wfsSpecific->xframesize;
    tempysize = wfsSpecific->yframesize;
    wfsSpecific->framesizeflag=1;
    wfsSpecific->xframesize = wfsSpecific->xarraysize;
    wfsSpecific->yframesize = wfsSpecific->yarraysize;
    ospReadNulls(wfsSpecific->nullfile,wfsSpecific);
    ospCalculateSubaps(wfsSpecific);

    createflag = 'n';

    printf("***************Entering ospNewCalibrate*****************\n");

    while(createflag == 'n')
    {
	printf("Enter a filename where the matrix showing the Zernike\n");
	printf("composition of the basis functions will be/is stored. (The\n");
	printf("corresponding file of names and paths of the calibration\n");
	printf("images will be stored in a file of the same name but with\n");
	printf("a .ims extension\n");
	printf("IMAGES MUST BE FULL FRAME AND UNSCRAMBLED FLOATS (at present)!!!!\n[enter filename]: ");
	scanf("%s",ffilename);
	dummy = getchar(); /* mop up any loose carriage return */

	strncpy(calimfilename,ffilename,OSP_MAXSTR-5);
	strncat(calimfilename,".ims",OSP_MAXSTR-1);

	if((fp1=fopen(ffilename,"r"))==NULL)
	{
	    printf("File does not exist: Create a new file  (c)\n");
	    printf("                or,  Enter another name (n)\n");
	    printf("[enter (c|n)]: ");

	    createflag = getchar();

	    if(createflag == 'c')
	    {
		if((fp2=fopen(calimfilename,"w"))==NULL)
		{
		   printf("Error opening %s for writing: exiting ospNewCalibrate\n", calimfilename);
		   return(ERROR);
		}
/*		printf("Are image files scrambled?\n[enter (y|n)]: ");
		scramflag = getchar();*/
		scramflag='n';
		if(scramflag == 'y')
		{
		    fprintf(fp2,"scrambled\n");
		}
		else
		{
		    fprintf(fp2,"unscrambled\n");
		}
/*		printf("Are image files float (f) or unsigned short int (u)?\n");
		printf("[enter (f|u)]: ");
		typeflag = getchar();*/
		typeflag = 'f';
		if(typeflag == 'u')
		{
		    fprintf(fp2,"unsigned_short_int\n");
		}
		else
		{
		    fprintf(fp2,"float\n");
		}

		printf("How many calibration images (with basis functions)/");
		printf("how many Zernike corrections? (Assumed equal)\n");
		printf("[enter integer]: ");
		scanf("%d",&basisdim);
		fprintf(fp2,"%d\n",basisdim);

		printf("Enter data when prompted...\n");
		for (jj=1; jj<=basisdim; jj++)
		{
		    printf("...enter name of basis image [%d], inc. path (e.g. ./data/z1.fits): \n",jj);
		    scanf("%s",tempname);
		    
		    if((fp3=fopen(tempname,"r"))==NULL)
		    {
			printf("%s cannot be opened: try again.\n", tempname);
			jj--;
		    }
		    else
		    {
			fclose(fp3);

			fprintf(fp2,"%s\n",tempname);
			for(ii=1;ii<=basisdim;ii++)
			{
			    printf("Enter amount of Z%d: ",ii);
			    scanf("%f",&coeff);
			    f[ii][jj]=coeff;
			}
		    }
		}
		fclose(fp2);
		
		if(ospWriteMatrixToFile(ffilename,f,basisdim,basisdim)==ERROR)
		{
		    printf("...error occurred in ospNewCalibrate: exiting\n");
		    return(ERROR);
		}

	    }
	}
	else
	{
	    fclose(fp1);
	    createflag = 'e'; 
	}
    }
    
    printf("Reading matrix showing the Zernike composition of the basis functions\n");
    printf("from %s, and reading filenames of calibration images from %s.\n",
	   ffilename,calimfilename);

    if((fp2=fopen(calimfilename,"r"))==NULL)
    {
	printf("Error opening %s for reading: exiting ospNewCalibrate\n", calimfilename);
	return(ERROR);
    }

    scramflag = 'n';
    typeflag = 'f';

    fgets(flagtext, 30,fp2);
    if(strcmp(flagtext,"scrambled")==0) scramflag = 'y';
	

    fgets(flagtext, 30,fp2);
    if(strcmp(flagtext,"unsigned_short_int")==0) typeflag = 'u';

    fscanf(fp2,"%d\n", &basisdim);
    printf("basisdim is %d\n", basisdim);
    for(ii=1; ii<=basisdim; ii++)
    {

/*	fgets(tempname,OSP_MAXSTR,fp2);
	i=0;
	while(tempname[i] != ' ' && tempname[i] != '\n' && tempname[i] != '\0'
	      && i < OSP_MAXSTR)
	{
	    i++;
	}
	tempname[i] ='\0';
*/
	fscanf(fp2,"%s\n",tempname);

	printf("Reading image data from %s\n",tempname);
	if(scramflag == 'y')
	{
	    /* put unscrambling code here */
	}
	if(typeflag == 'u')
	{
	    /* put frame type conversion here- need another buffer */
	}

	ospReadFloatImage(buffp, tempname, wfsSpecific->buffsize);
	ospSubtractFrameFromFrame(buffp, wfsSpecific->ffsubbuff, wfsSpecific);
	ospMultiplyFrameByFrame(buffp, wfsSpecific->ffmultbuff, wfsSpecific);
	ospCentroidWrapper(buffp,wfsSpecific);
	for(jj=1; jj<=wfsSpecific->mp; jj++)
	{
	    a[jj][ii] = wfsSpecific->s[jj];
	}
    }
    fclose(fp2);
    if(ospReadMatrixFromFile(ffilename,f)==ERROR)		
    {
	printf("...error occurred in ospNewCalibrate: exiting\n");
		return(ERROR);
    }

    printf("Showing the reconstructor matrix wrt."),
    printf(" supplied basis functions \n");

    ospShowMatrix(a,wfsSpecific->mp,basisdim);
    
    wfsSpecific->framesizeflag = tempflag;
    wfsSpecific->xframesize = tempxsize;
    wfsSpecific->yframesize = tempysize;
    ospReadNulls(wfsSpecific->nullfile,wfsSpecific);
    ospCalculateSubaps(wfsSpecific);

/**** Multiply by the matrix f[][](inverse) which takes account of the basis
***** set used to calculate the reconstructor. Post multiplying the 
***** reconstructor matrix with respect to the arbitrary basis set by the
***** matrix inverse of f[][] gives a reconstructor in terms of the
***** normalised Zernike polynomials, and has the benefit that later
***** the variances are calculated wrt to the correct basis set, and that
***** uncertainties in the determination of the basis functions should be 
***** accommodated in the estimate of fitting error.  */

    svdcmp(f,basisdim,basisdim,w,v);     /* First calculate the inverse */
    ospCalculateInverse(finv,f,v,w,basisdim,basisdim);
    printf("Matrix finv: inverse of matrix made up from Zernike composition\n");
    printf("of supplied basis functions. Used to calculate reconstructor \n");
    printf("wrt. single Zernike basis functions in increasing order.\n");
    ospShowMatrix(finv,basisdim,basisdim);

    ospMatrixProduct(u,a,finv, wfsSpecific->mp,basisdim,basisdim);    /* The matrix product */

    for(i=1;i<=wfsSpecific->mp; i++)     /* Set a[][] equal to u[][] */
    {
	for(j=1;j<=basisdim;j++)
	{
	    a[i][j] = u[i][j];
	}
    }

    printf("Showing a: the reconstructor matrix wrt. ");
    printf("single Zernike basis functions, in order.\n");
    ospShowMatrix(u,wfsSpecific->mp,basisdim);

/* NB Having completed the basis function transformation,
 * from this point we use only np columns of the reconstructor *****/

/**** Invert the reconstructor matrix using svd to obtain the control 
***** matrix c[][] */

    svdcmp(u,wfsSpecific->mp,wfsSpecific->np,w,v);
    ospCalculateInverse(wfsSpecific->c,u,v,w,wfsSpecific->np,wfsSpecific->mp);

/**** Calculate and display the condition number of the matrix */
    
    printf("Zernikes which can be corrected will correspond to non-singular\n");
    printf("values in the diagonal elements of matrix w.\n");
    ospConditionOfW(w,wfsSpecific->np);

/**** Display evidence that inverse has been calculated */
    ospMatrixProduct(ca,wfsSpecific->c,a,wfsSpecific->np,wfsSpecific->mp,wfsSpecific->np);

    printf("Product i = c * a: should be identity matrix\n");
    printf("(c is the control matrix wrt. single Zernike basis functions)\n");

    ospShowMatrix(ca,wfsSpecific->np,wfsSpecific->np);

    strncpy(controltempname,wfsSpecific->controlfile,OSP_MAXSTR-5);
    strncat(controltempname,".bak",OSP_MAXSTR-1);
    if((remove(controltempname))!=0)
    {
	fprintf(stderr,"remove failed for %s in ospNewCalibrate\n",
		controltempname);
	ospstatus=ERROR;
    }
    if((rename(wfsSpecific->controlfile,controltempname))!=0)
    {
	fprintf(stderr,"rename failed for %s in ospNewCalibrate\n",
		wfsSpecific->controlfile);
	ospstatus=ERROR;
    }
    else
    {
	printf("Renaming existing control matrix file as %s\n",
	       controltempname);
    }

    ospWriteMatrixToFile(wfsSpecific->controlfile,wfsSpecific->c,
			 wfsSpecific->np, wfsSpecific->mp);
    printf("Writing control matrix to file %s\n",wfsSpecific->controlfile); 
    


    ospFitVars(v,w,wfsSpecific);

    strncpy(fvarstempname,wfsSpecific->fvarsfile,OSP_MAXSTR-5);
    strncat(fvarstempname,".bak",OSP_MAXSTR-1);
    if((remove(fvarstempname))!=0)
    {
	fprintf(stderr,"remove failed for %s in ospNewCalibrate\n",
		fvarstempname);
	ospstatus=ERROR;
    }
    if((rename(wfsSpecific->fvarsfile,fvarstempname))!=0)
    {
	fprintf(stderr,"rename failed for %s in ospNewCalibrate\n",
		wfsSpecific->fvarsfile);
	ospstatus=ERROR;
    }
    else
    {
	printf("Renaming existing file of fitting variances as %s\n",
	       fvarstempname);
    }


    ospWriteVectorToFile(wfsSpecific->fvarsfile,wfsSpecific->fvars,
			 wfsSpecific->np);
    printf("Writing fitting variances to file %s\n",wfsSpecific->fvarsfile);
    
    freeVector(w,1,OSP_ZMAX);
    freeMatrix(a,1,wfsSpecific->mp,1,OSP_ZMAX);
    freeMatrix(u,1,wfsSpecific->mp,1,OSP_ZMAX);
    freeMatrix(v,1,OSP_ZMAX,1,OSP_ZMAX);
    freeMatrix(f,1,OSP_ZMAX,1,OSP_ZMAX);
    freeMatrix(ca,1,OSP_ZMAX,1,OSP_ZMAX);
    freeMatrix(finv,1,OSP_ZMAX,1,wfsSpecific->mp);
    printf("***************Leaving ospNewCalibrate*****************\n");

    return(ospstatus);	
}
/*--------------------------------------------------------------------------*/

/*
 *+
 * FUNCTION NAME:
 * ospNullCorrection
 *
 * INVOCATION:
 * ospNullCorrection(wfsSpecific)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (!) wfsSpecific    (struct OSP_CONTEXT *)  pointer to wfs context structure
 *
 * FUNCTION VALUE:
 * (int) A status value equal to OK or ERROR
 *
 * PURPOSE:
 * To provide a simple interface by which the null positions file can be corrected using a FITS image of nulled spots
 *
 * DESCRIPTION:
 * The function issues a series of prompts for data which allows the null
 * positions file to be corrected given a FITS image of nulled spots.
 * Invocation of the function is particularly simple, and the first prompt
 * allows the functionality of this function to be bypassed in the event of 
 * of the current nullfile being known to be accurate. If null correction is to take
 * place, the readout geometry of the OSP_CONTEXT structure 
 * is changed to full frame (all calibration
 * must take place wrt full frame data to cope with all geometries of the
 * reduced frames) and the output from ospCalculateSubaps which is called as 
 * a consequence allow the next prompt (asking if the current nullfile is 
 * approximately accurate) to be answered: yes, if the subapertures with no 
 * null positions are those we have chosen to ignore, and if no 'Badly centred
 * null position' warnings are present; no, otherwise. If yes, the filename
 * of a full frame float FITS image showing the nulled SH spots is prompted for.
 * As this file may be older and have had the dark and flat field applied 
 * already, an option is given to avoid the application of the current 
 * subtractive and multiplicative offset frames. The calculation of the 
 * corrections is performed by a call to ospSaveNullPositions, which backs up
 * the current nullfile with a .bak suffix, writes the new values into a file 
 * with the same name as the original nullfile to save editing the .ini file, 
 * and applies the new null positions to the OSP_CONTEXT structure, which is 
 * reset to its original readout geometry proir to exit. This function may
 * typically be called prior to ospNewCalibrate. See also the header of
 * ospSaveNullPositions for more details of the nullfile format.
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * Existence of fitsio library
 *
 * INCLUDE FILES:
 * osp.h
 *
 * DEFICIENCIES:
 *
 *-
 */
int /*STATUS*/ ospNullCorrection(struct OSP_CONTEXT * wfsSpecific)
{
    char correctflag;
    char dummy;
    char name[OSP_MAXSTR];
    int ospstatus = OK;
    int tempflag;
    int tempxsize;
    int tempysize;

    printf("***************Entering ospNullCorrection***************\n");

    printf("Is the null file specified by %s to be corrected?\n[enter (y|n)]: ",
	   wfsSpecific->nullfile);
    correctflag = getchar();
    dummy = getchar();

    if(correctflag == 'y')
    {
	tempflag = wfsSpecific->framesizeflag;
	tempxsize = wfsSpecific->xframesize;
	tempysize = wfsSpecific->yframesize;
	wfsSpecific->framesizeflag=1;
	wfsSpecific->xframesize = wfsSpecific->xarraysize;
	wfsSpecific->yframesize = wfsSpecific->yarraysize;
	ospReadNulls(wfsSpecific->nullfile,wfsSpecific);
	ospCalculateSubaps(wfsSpecific);

	printf("Are the entries of %s approximately accurate \n",
	       wfsSpecific->nullfile);
	printf("(i.e. above, were any subaps with no null positions as expected,\n");
	printf("and were there  no 'Badly centred null position' warnings)?\n");
	printf("[enter (y|n)]: ");
	correctflag = getchar();
	dummy = getchar();
	if(correctflag == 'y')
	{
	    printf("Enter the name of the FITS file from which accurate null\n");
	    printf("positions are to be calculated\n[enter filename]: ");
	    scanf("%s",name);

	    printf("Are the following offset frames to be applied?\n");
	    printf("     subtractive    (dark)      - %s\n", wfsSpecific->subfile);
	    printf("     multiplicative (flat field)- %s\n", wfsSpecific->multfile);
	    printf("[enter (y|n)]: ");
	    correctflag = getchar();
	    dummy = getchar();
	    if(correctflag == 'n')
	    {
		printf("Calculating corrections to null positions using data from\n");
		printf("%s with no offset correction.\n", name);
		if(ospSaveNullPositions(name,NO_OFFSET_CORRECTION,
					wfsSpecific->nullfile,wfsSpecific)==ERROR)
		{
		    fprintf(stderr,"...error took place in ospNullCorrection.\n");
		    ospstatus = ERROR;
		}
	    }
	    else
	    {
		printf("Calculating corrections to null positions using data from\n");
		printf("%s with offset correction from above offset frames.\n", name);
		if(ospSaveNullPositions(name,OFFSET_CORRECTION,
					wfsSpecific->nullfile,wfsSpecific)==ERROR)
		{
		    fprintf(stderr,"...error took place in ospNullCorrection.\n");
		    ospstatus = ERROR;
		}
	    }

	}
	else
	{
	    printf("This indicates that some null positions are not\n");
	    printf("reasonably centred with respect to the specified\n");
	    printf("subaperture positions - the CCD readout geometry\n");
	    printf("and/or the null file requires editing.\n");
	    printf("...exiting ospNullCorrection with no changes made to %s\n",
	       wfsSpecific->nullfile);
	}

	wfsSpecific->framesizeflag = tempflag;
	wfsSpecific->xframesize = tempxsize;
	wfsSpecific->yframesize = tempysize;
	ospReadNulls(wfsSpecific->nullfile,wfsSpecific);
	ospCalculateSubaps(wfsSpecific);

    }
    else
    {
	printf("...exiting ospNullCorrection with no changes made to %s\n",
	       wfsSpecific->nullfile);
    }
    printf("***************Leaving  ospNullCorrection***************\n");

    return(ospstatus);
    
}

void ospChangeThreshold(float threshold, struct OSP_CONTEXT * wfsSpecific)
{
    wfsSpecific->thresh = threshold;
    return;
}

void ospChangeTipscale(float tipscale, struct OSP_CONTEXT * wfsSpecific)
{
    wfsSpecific->tipscale = tipscale;
    return;
}

void ospChangeTiltscale(float tiltscale, struct OSP_CONTEXT * wfsSpecific)
{
    wfsSpecific->tiltscale = tiltscale;
    return;
}

void ospChangeAngle(float angle, struct OSP_CONTEXT * wfsSpecific)
{
    wfsSpecific->angle = (double)angle;
    return;
}

void ospChangeFocusscale(float focusscale, struct OSP_CONTEXT * wfsSpecific)
{
    wfsSpecific->focusscale = focusscale;
    return;
}

#undef NRANSI










