#ifndef _NR_UTILS_H_
#define _NR_UTILS_H_

#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

#define NR_END 1
#define FREE_ARG char*
#define OSP_BUFFMAX 6400
#define OSP_HRBUFFMAX (1024*1024)
#define OSP_SUBAPSMAX 36
#define OSP_MAXSTR 80               /* size of text string in a matrix header */
#define OSP_TESTMAG 5
#define OSP_ZMAX 20
#define DIAG_ARRAY_SIZE 20
#define GUARD1 0
#define GUARD2 (DIAG_ARRAY_SIZE - 1)
#define MAX_WFS_SOURCES 5
#define OSP_NO_LIGHT 99

#ifndef vxWorks
#define OK 0
#define  ERROR -1
#endif /*vxWorks*/

/* #define OSP_VERBOSE */

/***** Application specific data-types ***************************/

/*typedef int STATUS; */

struct OSP_CONTEXT{
    char nullfile[OSP_MAXSTR];  /* ASCII file of origins and null positions */
    char subfile[OSP_MAXSTR];   /* FITS file of subtractive array corrections */
    char multfile[OSP_MAXSTR];  /* FITS file of multiplicative array corrections */
    char controlfile[OSP_MAXSTR];  /* ASCII file containing control matrix */
    char fvarsfile[OSP_MAXSTR];  /* ASCII file containing fitting variances */
    int np;   
    int mp;
    int xarraysize;
    int yarraysize;
    int buffsize;
    int side;
    int ospxstart;
    int ospystart;
    int ospxbin;
    int ospybin;
    int ospxraster;
    int ospyraster;
    int ospxspace;
    int ospyspace;
    int ospxsubap;
    int ospysubap;
    int defxstart;
    int defystart;
    int defxbin;
    int defybin;
    int defxraster;
    int defyraster;
    int defxspace;
    int defyspace;
    int defxsubap;
    int defysubap;
    int sectors;
    int framesizeflag;
    int xframesize;
    int yframesize;  
    int framebuffsize;
    float nsigma;
    float readsq;
    int weight;
    float thresh;
    float nulls[2*OSP_SUBAPSMAX];
    float centres[4*OSP_SUBAPSMAX +1];
    double time;
    float * ffsubbuff;
    float * redsubbuff;
    float * ffmultbuff;
    float * redmultbuff;
    float * err;
    float **c;
    float *s;
    float *dssq;
    float *fvars;
    float *mvars;
    float *z;
    float * sumbuff;
    int coaddcounter;
    clock_t coaddstart;
    int wfsSource;
    int wfsMode;
    float ospdiag[DIAG_ARRAY_SIZE];
    float tipscale;
    float tiltscale;
    float focusscale;
    float tipCor;
    float tiltCor;
    double angle ;
    float xcenter;
    float ycenter;
};

/* Temporary solution, for first light. Possibility to subtract dark image for HRWFS Add by CB */
struct OSP_HRCONTEXT{
    char subfile[OSP_MAXSTR];   /* FITS file of subtractive array corrections */
    int xarraysize;
    int yarraysize;
    int buffsize;
    float * ffsubbuff;
};
/* End of temporary solution */

struct OSP_GEOMETRY{
    int sectors;
    int xstart;
    int ystart;
    int xbin;
    int ybin;
    int xraster;
    int yraster;
    int xspace;
    int yspace;
    int xsubap;
    int ysubap;
    int xarraysize;
    int yarraysize;
    int framesizeflag;
};

/******* declare function prototypes *****/
#ifdef vxWorks
STATUS writeWfsToTcs(struct OSP_CONTEXT *pWfs);
STATUS writeWfsToSynchro(struct OSP_CONTEXT *pWfs);
#endif /*vxWorks*/

/******* enumerate wfs sources and diagnostic indices*****/

enum
{
	HRWFS = 0,
	PWFS1,
	PWFS2,
	OIWFS,
	AOWFS
};

enum
{
	AO = 0,
	FG,
	NO_OFFSET_CORRECTION = 0,
	OFFSET_CORRECTION
	
};

/******* enumeration of signal processing modes *****/
enum
{
        OSP_MODE_NONE = 0,    /* No signal processing.               */
		OSP_MODE_DARK,        /* Subtract DARK frame.                */
        OSP_MODE_FG,          /* Fast Guide mode.                    */
        OSP_MODE_AO,          /* Active Optics mode.                 */
        OSP_MODE_FG_AO,       /* Fast Guide and Active Optics mode.  */
        OSP_MODE_FG_COADD,    /* Fast Guide and Coadd mode.          */
        OSP_MODE_COADD,       /* Coadd Only mode.                    */
        OSP_MODE_CALIB_TT,    /* Calibrate Tip Tilt mode.            */
        OSP_MODE_CORRECT_TT,  /* Correct Tip Tilt mode.              */
        OSP_MODE_MAX          /* Maximum OSP mode marker.            */
};

/******* functions written to eliminate declaration of globals *****/
float SQR(float a);
int IMIN(int a, int b);
float FMAX(float a, float b);


/******** my matrix and vector handling functions ******************/
int /*STATUS*/ ospReadVectorFromFile(char * infile, float * vect, int n0, 
                                                     int ntot, int nstep);
int /*STATUS*/ ospWriteVectorToFile(char * outfile, float * vect, int n);
int /*STATUS*/ ospReadMatrixFromFile(char * infile, float ** matr);
int /*STATUS*/ ospWriteMatrixToFile(char * outfile, float ** matr, int rows, int cols);
void ospApplyControlMatrix(float ** c, float * z, float * s, int rows, int cols);
void ospCalculateInverse(float ** inv,float ** u,float ** v,
                                                  float *w, int m, int n);
void ospConditionOfW(float *w,int n);
void ospMatrixProduct(float ** result, float ** a, float ** b, 
                                         int rows1, int cols1, int cols2);
void ospShowMatrix(float ** matr, int rows, int cols);
void ospShowVector(float *vect, int n);


/********* cfitsio application functions **************************/
int /*STATUS*/ ospPrintHeaders( char * infile );
int /*STATUS*/ ospReadFloatImage( float * buffp, char * infile,int buffsize);
int /*STATUS*/ ospReadUShortImage( unsigned short int * buffp, char * infile,int buffsize);
int /*STATUS*/ ospWriteFloatImage( float * buffp, char * outfile,int xarraysize, int yarraysize);
int /*STATUS*/ ospWriteUShortImage( unsigned short int * buffp, char * outfile,int xarraysize, int yarraysize);
int /*STATUS*/ ospPrintError( int status);


/********** my signal processing functions ************************/

void ospGlobalCentroid (float * buffp,
			struct OSP_CONTEXT * wfsSpecific);
void ospCentroid (float *buffp,int x0,float xdiff,int y0,float ydiff,
		  float * disp,
		  float threshvar, struct OSP_CONTEXT * wfsSpecific);
void ospGlobalThreshold (float * buffp,
			 struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospThreshold (float * buffp, float * meanval,
		   struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospNewReadCentres(char * nullfile, float * centres,int side,
			      struct OSP_CONTEXT * wfsSpecific);
void ospTestData ( float * buffp, struct OSP_CONTEXT * wfsSpecific);
void ospGetBasisFunction(float **f, int i, int znum, float mag);
int /*STATUS*/ ospCentroidWrapper(float * buffp,
				  struct OSP_CONTEXT * wfsSpecific);
void ospSimulateCentroids(float ** matr, float * vect, int np, int mp);
int /*STATUS*/ ospFitVars(float **v, float *w, 
			  struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospMeasVars(struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospCoAdd(float * buffp, int N, float deltaT,
			struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospAddFrameToFrame(float *buffp1, float *buffp2,
				  struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospSubtractFrameFromFrame(float *buffp1, float *buffp2,
					 struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospNewSubtractFrameFromFrame(float *buffp1, float *buffp2,
					    int buffSize );
int /*STATUS*/ ospMultiplyFrameByFrame(float *buffp1, float *buffp2,
				       struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospMultiplyFrameByConstant(float *buffp1, float constname,
					  struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospAddConstantToFrame(float *buffp1, float constname,
				     struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospSaveNullPositions(char * nullname,int oflag,char * outfile, 
				    struct OSP_CONTEXT * wfsSpecific);
void ospError(char error_text[]);
struct OSP_CONTEXT * ospInit(char * wfsName, struct OSP_GEOMETRY * ospGeom);
struct OSP_HRCONTEXT * ospInitHr(char * hrwfsName);
int /*STATUS*/ ospMeasure(float * buffp, struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospCalibrate(char * calibpath,struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospNewCalibrate(char * calibpath,struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospTidyUp(struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospTidyUpHr(struct OSP_HRCONTEXT * hrwfsSpecific);
int /*STATUS*/ ospFrameTypeConvert(unsigned short int * usbuffp, float * fbuffp, char * mode,int buffsize);
int /*STATUS*/ ospFrameScramble(const int xPixels, const int yPixels, 
				const int outputs, float * inBuffer,
				unsigned short int * outBuffer);
int /*STATUS*/ ospConvertAndScrambleFITS(char * infile, char * outfile, 
					 struct OSP_CONTEXT *wfsSpecific);
int /*STATUS*/ ospShow(struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospAddContextToHeader(char * filename,
			     struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospReadHeaderInt(char * filename, int numelem, char ** keynames,                                 int * values);
int /*STATUS*/ ospReduceFITS(char * infile, char * outfile, struct
			     OSP_CONTEXT * wfsSpecific, int buffsize);
int /*STATUS*/ ospReduceFrame(float * buffp, float * newbuffp, struct 
			      OSP_CONTEXT * wfsSpecific, int buffsize);
int /*STATUS*/ ospReadNulls(char * nullfile, struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospCalculateSubaps(struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospReadCentres( char *centfile, float * centres, int side);
int /*STATUS*/ ospChangeGeometry(struct OSP_GEOMETRY * ospGeom,
				 struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospNullCorrection(struct OSP_CONTEXT * wfsSpecific);
void ospChangeThreshold(float threshold, struct OSP_CONTEXT * wfsSpecific);
void ospChangeTipscale(float tipscale, struct OSP_CONTEXT * wfsSpecific);
void ospChangeTiltscale(float tiltscale, struct OSP_CONTEXT * wfsSpecific);
void ospChangeFocusscale(float focusscale, struct OSP_CONTEXT * wfsSpecific);

void vxtest();
/********* Fast versions **************************************************/
int /*STATUS*/ ospFGMeasure(float * buffp, struct OSP_CONTEXT * wfsSpecific);
int /*STATUS*/ ospFGCentroidWrapper(float * buffp,
				  struct OSP_CONTEXT * wfsSpecific);
void ospFGCentroid (float *buffp,int x0,float xdiff,int y0,float ydiff,
		  float * disp, float threshvar,
		  struct OSP_CONTEXT * wfsSpecific);

int ospCalibrateTip ( float *buffp , 
                      int N ,
                      float amplitude,
                      struct OSP_CONTEXT *wfsSpecific );
int ospCalibrateTilt ( float *buffp , 
                       int N ,
                       float amplitude,
                       struct OSP_CONTEXT *wfsSpecific );
int ospCoAddOnly ( float *buffp , 
                   int N ,
                   struct OSP_CONTEXT *wfsSpecific );

int ospTtCor ( float *buffp , 
               int output ,
               struct OSP_CONTEXT *wfsSpecific );
int ospTracking ( float *buffp ,
                  struct OSP_CONTEXT *wfsSpecific );

int ospTrackingAndFocus ( float *buffp , int N ,
                  struct OSP_CONTEXT *wfsSpecific );

int ospCoAddFocus ( float *buffp , int N ,
                  struct OSP_CONTEXT *wfsSpecific );


/********* Numerical recipes functions ************************************/

float pythag(float a, float b);
void svdcmp(float **a, int m, int n, float w[], float **v);
void ospnrerror(char error_text[]);
float *vector(long nl, long nh);
float **matrix(long nrl, long nrh, long ncl, long nch);
void freeVector(float *v, long nl, long nh);
void freeMatrix(float **m, long nrl, long nrh, long ncl, long nch);

#endif /* _NR_UTILS_H_ */











