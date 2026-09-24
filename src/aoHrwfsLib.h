#ifndef __INCaoHrwfsLibh
#define __INCaoHrwfsLibh

/*
 * MODULE NAME:
 * aoHrwfsLib
 *
 * FILENAME:
 * aoHrwfsLib.h
 *
 * PURPOSE:
 * Include file for the HRWFS active optics (signal processing) library.
 * Contains the types and constants for REL-845. Note: aO means active optics.
 *
 * This library is being ported from pwfs/src/aoPWLib.c/.h (author Corinne
 * Boyer). Fast-guide (fg) and circular-buffer (cb) functionality is NOT
 * included for HRWFS (see REL-845-signal-processing-plan.md). A few fg-related
 * struct fields are retained for parity with aoPWLib so the algorithm port
 * stays a close copy; they are unused for HRWFS.
 *
 * STATUS: REL-845 Part 2, initial scaffold. Geometry constants reflect the
 * documented HRWFS hardware; the context structs are ported; most algorithm
 * routines are stubs pending the open questions in the plan (HRWFS reference
 * spot file / exact active-subaperture count, rotation geometry, seeing
 * calibration, and Francois' hrwfsAO.pro reference).
 *
 * AUTHORS:
 * Ported for HRWFS from aoPWLib (Corinne Boyer).
 */

#ifdef vxWorks
#include <vxWorks.h>
#endif

/***************************************************** Constants definition ***/

#define STRING_SIZE          160       /* Size of a string                    */

/*
 * HRWFS detector geometry. The CCD is 1024 x 1024 (DET_CONTROL_HRWFS_XSIZE).
 * The Shack-Hartmann lenslet array is 18 x 18 with ~50 px spot separation,
 * aperture radius ~450 px, centred at (512, 512). See the REL-845 documents.
 */

#define CCD_XSIZE            1024      /* X size of the HRWFS CCD in pixels    */
#define CCD_YSIZE            1024      /* Y size of the HRWFS CCD in pixels    */
#define CCD_SIZE             (CCD_XSIZE * CCD_YSIZE)

#define HRWFS_LENSLET_NB     18        /* Lenslets across the SH array (nsp)    */
#define HRWFS_SPOT_SEP       50        /* Spot separation / px per subap        */
#define HRWFS_APERTURE_RADIUS 450      /* Aperture radius in pixels            */
#define HRWFS_X_CENTER       512       /* Nominal X centre of the array        */
#define HRWFS_Y_CENTER       512       /* Nominal Y centre of the array        */
#define HRWFS_OPD_SCALE      (-0.1625) /* OPD scaling factor (from doc)        */
#define HRWFS_ZERNIKE_SCALE  (-1.4625) /* Zernike scaling factor (from doc)    */
#define HRWFS_PSCALE         0.08125   /* Plate scale, arcsec/pixel (hrwfsAO)  */

/*
 * Algorithm reference: hrwfsAO.pro (F. Rigaut, v1.3, 2002). The above geometry
 * is CONFIRMED there (nsp=18, npixps=50, pscale=0.08125, tdiam=8 m). HRWFS
 * signal processing is simpler than PWFS:
 *   - The interaction matrix is computed ANALYTICALLY from Zernike slopes
 *     (zermes2), not measured on hardware; the control matrix is its
 *     pseudo-inverse. So no hardware IM-measurement (detSigMeasAoIm) is needed.
 *   - No software rotation: the Cass rotator is required at 0 deg; only a fixed
 *     x/y offset + axis flip is applied (angleWithM1/M2 are unused for HRWFS).
 *   - Centroiding is a two-pass centre-of-gravity per subaperture (getmes).
 *   - Active subapertures form a circular annulus:
 *     1.8 < sqrt((i-8.5)^2 + (j-8.5)^2) < 0.95*9.5 subaperture radii.
 *   - Modes sent to the TCS: Zernikes [4,6,5,8,7,10,9,11..19], sign-flipped.
 * See REL-845-signal-processing-plan.md sec. 6 for the full analysis.
 */

/*
 * SUBAP_NB sizes every fixed subaperture array (WFS_VECT, AO_MATRIX,
 * subapUsedVect, ...). In aoPWLib this was 4; for an 18 x 18 HRWFS array the
 * bounding box is 18*18 = 324 subapertures, of which only those inside the
 * radius-450 aperture (~254, i.e. pi/4 * 18^2) are illuminated. We size to the
 * 324 bounding box so the buffers safely accommodate any active count.
 *
 * CONFIRMED by hrwfsAO.pro: nsp = 18, so 324 bounding-box subapertures. The
 * active set is the annulus 1.8 < r < 0.95*9.5 subap radii, which
 * tools/validate_ao.py computes to be 244 active subapertures (488 slopes).
 * That validation also confirms the 150-mode fit is well-conditioned
 * (cond ~277) and recovers modes to machine precision.
 */

#define SUBAP_NB             324       /* Max subapertures (18x18 bounding box)*/

#define FG_MODE_NB           3         /* FG modes (NOT used for HRWFS)        */

/*
 * Number of Zernike modes fitted. hrwfsAO.pro computes an analytic interaction
 * matrix for many Zernikes and fits maxz=150 of them (to limit aliasing),
 * then sends only AO_NCORR of them to the TCS. So AO_MODE_NB is the fit size,
 * not the number output (contrast PWFS, which used 19).
 */
#define AO_MODE_NB           150       /* Zernike modes fitted (hrwfsAO maxz)  */
#define AO_NCORR             16        /* Zernike modes sent to the TCS        */
#define MODE_NB              (FG_MODE_NB + AO_MODE_NB)

/*
 * Zernike model grid used to build the analytic interaction matrix (zermes2):
 * AO_NP model pixels per subaperture across an 18x18 grid, plus a 2 px border.
 */
#define AO_NP                20        /* Model pixels per subaperture         */
#define AO_ZERN_DIM          (AO_NP * HRWFS_LENSLET_NB + 2) /* = 362           */

#define AO_SUBAP_OFF         32767     /* Indicates no light on a subaperture  */
#define AO_SH_OFF            65536     /* Indicates no light on the SH array   */
#define AO_MIN_DOUBLE        1.0e-10   /* Minimum double for comparisons       */
#define AO_TIME_NOW_ERROR    -5.55e9   /* timeNow() error sentinel             */

#define SEEING_ROW_NB        6         /* Rows of the seeing coeff matrix      */
#define TELESCOPE_DIAMETER   8.0       /* Telescope diameter in metres         */
#define SEEING_LAMBDA        0.5       /* Wavelength for seeing computation     */
#define SEEING_FOCUS_KOLMO   4.98867e-3
#define RADIAN_TO_ARCSEC     4.848e-6  /* radian -> arcsec conversion          */

/*
 * NOTE on memory: with CCD_SIZE = 1024*1024, IMAGE_VECT is 4 MB. The control
 * context holds three of them (dark, flat, sum) => ~12 MB per context. This is
 * far larger than PWFS (80x80) and must be accounted for in the IOC memory
 * budget. A later optimisation may window/bin before buffering.
 */

/********************************************************************* Enum ***/

/*
 * Processing modes. The full aoPWLib enum is retained so mode indices match
 * the reference algorithms during the port; HRWFS only exercises NONE, DARK,
 * SEQ_DARK, CLOSED_LOOP and AO (see detControl.h DET_CONTROL_CMD_SIG_*).
 */

enum
{
   AO_MODE_NONE = 0,       /* No signal processing.                  */
   AO_MODE_DARK,           /* Subtract DARK frame.                   */
   AO_MODE_COADD,          /* Coadd only mode.                       */
   AO_MODE_THRESH,         /* Threshold computation mode.            */
   AO_MODE_TOTAL,          /* Average flux computation.              */
   AO_MODE_GG,             /* Global guide mode (unused for HRWFS).  */
   AO_MODE_GG_COADD,       /* Global guide and coadd (unused).       */
   AO_MODE_FG_FOCUS,       /* FG and focus (unused for HRWFS).       */
   AO_MODE_FG_FOCUS_COADD, /* FG, focus and coadd (unused).          */
   AO_MODE_MEAS_IM,        /* Interaction matrix measurement mode.   */
   AO_MODE_AO,             /* Active optics correction mode only.    */
   AO_MODE_GG_AO,          /* Global guide and aO (unused).          */
   AO_MODE_FG_FOCUS_AO,    /* FG, focus and aO (unused).             */
   AO_MODE_SEQ_DARK,       /* Sequence dark then threshold.          */
   AO_MODE_CLOSED_LOOP,    /* Sequence closed loop.                  */
   AO_MODE_MAX             /* Maximum mode marker.                   */
};

enum
{
   AO_THRESH_SPOTS = 0,    /* Threshold computed with spots.         */
   AO_THRESH_NOSPOTS,      /* Threshold computed without spots.      */
   AO_THRESH_VALUE         /* Use given value, no computation.       */
};

enum
{
   AO_TOTAL_SPOTS = 0,     /* Average flux computed with spots.      */
   AO_TOTAL_VALUE,         /* Use given value.                       */
   AO_TOTAL_FORMULA        /* Use function(rms, N).                  */
};

enum
{
   AO_INT_MAT_TYPE = 0,    /* Interaction matrix type.               */
   AO_CONT_MAT_TYPE        /* Control matrix type.                   */
};

/***************************************** Definition of vectors and matrix ***/

typedef float  IMAGE_VECT   [ CCD_SIZE ];
typedef double WFS_VECT     [ (2 * SUBAP_NB) ];   /* 2 centroids per subap    */
typedef double GUIDE_VECT   [ 2 ];                /* whole-CCD guide           */
typedef double FG_VECT      [ FG_MODE_NB ];       /* unused for HRWFS          */
typedef double AO_VECT      [ AO_MODE_NB ];
typedef double AO_MATRIX    [ 2 * SUBAP_NB * AO_MODE_NB ];
typedef double FG_MATRIX    [ 2 * SUBAP_NB * FG_MODE_NB ]; /* unused for HRWFS */
typedef double SEEING_VECT  [ SEEING_ROW_NB ];
typedef double SEEING_MATRIX[ SEEING_ROW_NB * 2 * SUBAP_NB ];

typedef struct                         /* Column of an interaction matrix      */
{
   double     posAmplitude;            /* Positive amplitude of measured mode  */
   double     negAmplitude;            /* Negative amplitude of measured mode  */
   WFS_VECT   posCentroidsVect;        /* Centroids for a positive amplitude   */
   WFS_VECT   negCentroidsVect;        /* Centroids for a negative amplitude   */
} CIM_STRUCT;

/*************************** Definition of the structure describing the WFS ***/

typedef struct
{
   unsigned long outputsNb;            /* Number of CCD sectors (T_OUTPUTS)    */
   int        xSize;                   /* Columns per output (T_XSIZE)         */
   int        ySize;                   /* Rows per output (T_YSIZE)            */
   int        xMax;                    /* Max columns (xSize*outputsNb/2)      */
   int        yMax;                    /* Max rows (ySize*outputsNb/2)         */
   int        xStart;                  /* Columns discarded before subap 1     */
   int        yStart;                  /* Rows discarded before subap 1        */
   int        xBin;                    /* X binning factor (T_XBIN)            */
   int        yBin;                    /* Y binning factor (T_YBIN)            */
   int        xRaster;                 /* Subap X size in binned px (T_XRAS)   */
   int        yRaster;                 /* Subap Y size in binned px (T_YRAS)   */
   int        xSpace;                  /* Columns discarded between subaps     */
   int        ySpace;                  /* Rows discarded between subaps        */
   int        xSubapNb;                /* Subaperture columns (T_XSUBAP)       */
   int        ySubapNb;                /* Subaperture rows (T_YSUBAP)          */
   int        subapNb;                 /* xSubapNb*ySubapNb*outputsNb          */
   int        subapNotUsedNb;          /* Subapertures not used                */
   int        subapUsedNb;             /* subapNb - subapNotUsedNb             */
   int        subapUsedVect [ SUBAP_NB ]; /* Per-subap used flag (TRUE/FALSE)  */
   int        centroidsNb;             /* subapUsedNb * 2                      */
   int        xPixels;                 /* Image columns (xRaster*xSubapNb*2)   */
   int        yPixels;                 /* Image rows (yRaster*ySubapNb*2)      */
   int        pixelsNb;                /* xPixels*yPixels (T_NPIXEL)           */
   int        uscanNb;                 /* Underscan pixels (T_USCAN)           */
   int        xTail;                   /* Remaining px to discard per row      */
   int        packetSize;             /* Packet size in pixels (V_PSIZE)      */
   int        packetNb;                /* pixelsNb / packetSize                */
   int        binningFlag;             /* TRUE if binning                      */
   int        unused;                  /* Pad to a multiple of a double        */
} AO_CCD_ID_STRUCT, * AO_CCD_ID;

/******************************************* Definition of the aO structure ***/

typedef struct
{
   /* Everything needed to go from centroids to zernike modes.                */
   int           initFlag;             /* Context initialised                  */
   int           darkInitFlag;         /* Dark initialised                     */
   int           flatInitFlag;         /* Flat initialised                     */
   int           refInitFlag;          /* Reference initialised                */
   int           aoScaleInitFlag;      /* aO scale-factor vector initialised   */
   int           aoIntMatInitFlag;     /* aO interaction matrix initialised    */
   int           aoContMatInitFlag;    /* aO control matrix initialised        */
   int           fgContMatInitFlag;    /* FG control matrix (unused for HRWFS) */
   int           seeingCoeffMatInitFlag;
   int           seeingCoeffVectInitFlag;
   int           allowedSubapOff;      /* Subapertures allowed off             */
   int           focusCounter;         /* Focus computation counter            */
   int           coaddCounter;         /* Coadd counter                        */
   int           seeingCounter;        /* Seeing computation counter           */
   int           aoModeNb;             /* Number of aO modes to correct        */
   int           aoModeNotUsedNb;      /* aO modes not used                    */
   int           aoModeUsedNb;         /* aoModeNb - aoModeNotUsedNb           */
   int           aoModeUsedVect [ AO_MODE_NB ]; /* Per-mode used flag          */
   int           fgModeNb;             /* FG modes (unused for HRWFS)          */
   int           thresholdMethod;      /* AO_THRESH_*                          */
   int           totalMethod;          /* AO_TOTAL_*                           */
   int           unused;               /* Pad to a multiple of a double        */

   char          darkFileName [ STRING_SIZE ];
   char          flatFileName [ STRING_SIZE ];
   char          refVectFileName [ STRING_SIZE ];
   char          aoScaleFileName [ STRING_SIZE ];
   char          aoIntMatFileName [ STRING_SIZE ];
   char          aoContMatFileName [ STRING_SIZE ];
   char          fgContMatFileName [ STRING_SIZE ]; /* unused for HRWFS        */
   char          seeingCoeffMatFileName [ STRING_SIZE ];
   char          seeingCoeffVectFileName [ STRING_SIZE ];

   IMAGE_VECT    darkVect;             /* Dark image for the whole CCD         */
   IMAGE_VECT    flatVect;             /* Flat-field image for the whole CCD   */
   IMAGE_VECT    sumVect;              /* Coadd image                          */
   GUIDE_VECT    refGuideVect;         /* Centre of the whole CCD              */
   WFS_VECT      refWfsVect;           /* Centre of each subaperture           */
   WFS_VECT      thresholdVect;        /* Per-subaperture threshold            */
   WFS_VECT      averageThreshVect;    /* Per-subaperture average threshold    */
   SEEING_VECT   seeingCoeffVect;      /* Seeing coefficient vector            */
   SEEING_VECT   averageSeeingVect;    /* Average seeing vector                */
   SEEING_VECT   varianceSeeingVect;   /* Variance seeing vector               */
   AO_VECT       aoScaleFactorVect;    /* Scale factor per aO mode             */
   FG_VECT       fgScaleFactorVect;    /* FG scale factor (unused for HRWFS)   */
   /*
    * HRWFS interaction matrix is analytic (aoMatCompute), not measured, so the
    * PWFS aoIntMatStruct[] measurement buffer is not needed. Layout of aoIntMat
    * is mode-major: aoIntMat[mode*(2*SUBAP_NB) + slope], mode 0..AO_MODE_NB-1
    * (Zernike mode+2), slope 0..2*SUBAP_NB-1 (all bounding-box subaps; the
    * active-subaperture restriction and pseudo-inverse happen in aoModeCompute).
    */
   AO_MATRIX     aoIntMat;             /* aO interaction matrix (analytic)     */
   AO_MATRIX     aoContMat;            /* aO control matrix                    */
   FG_MATRIX     fgContMat;            /* FG control matrix (unused for HRWFS) */
   SEEING_MATRIX seeingCoeffMat;       /* Seeing coefficient matrix            */

   double        rms;                  /* RMS for threshold computation        */
   double        threshold;            /* Threshold for centroid computation   */
   double        thresholdRate;        /* Bright-pixel rate for threshold      */
   double        thresholdMultCoeff;   /* Multiplier for threshold             */
   double        thresholdDarkFull;    /* Sequence-dark threshold (no binning) */
   double        thresholdDarkBin;     /* Sequence-dark threshold (binning)    */
   double        rmsDarkFull;          /* Sequence-dark RMS (no binning)       */
   double        rmsDarkBin;           /* Sequence-dark RMS (binning)          */
   double        averageTotal;         /* Average total counts (whole CCD)     */
   double        totalThreshold;       /* Total-counts threshold (whole CCD)   */
   double        multCoeffTotal;       /* Multiplier for totalThreshold        */
   double        angleWithM2;          /* Angle between M2 and HRWFS coords    */
   double        cosAngleWithM2;
   double        sinAngleWithM2;
   double        angleWithM1;          /* Angle between M1 and HRWFS coords    */
   double        cosAngleWithM1;
   double        sinAngleWithM1;
   double        slidingFocusGain;     /* Gain for sliding focus average       */
   double        one_slidingFocusGain; /* 1 - slidingFocusGain                 */
   double        previousFocus;        /* Previous focus mode value            */
   double        r0;                   /* r0                                   */
   double        seeing;               /* Seeing                               */
   double        jitter;               /* X/Y average residual jitter          */
   double        seeingScaleFactor;    /* Seeing scale factor                  */
   double        aoThreshold;          /* Threshold above which gain increases */
   double        aoMaxThreshold;       /* Threshold above which values clamp   */
} AO_CTRL_ID_STRUCT, * AO_CTRL_ID;

/*********************** Zero-point model structures (astig/trefoil/coma/foc) */

typedef struct
{
   double a1, a2, a3;                  /* cos(theta), cos(2t), cos(4t) scale   */
   double p1, p2, p3;                  /* cos phases                           */
   double c;                           /* Constant term for astig0             */
   double b1, b2, b3;                  /* sin(theta), sin(2t), sin(4t) scale   */
   double pp1, pp2, pp3;               /* sin phases                           */
   double d;                           /* Constant term for astig45            */
   double astig0;                      /* Zero-point model for astig0          */
   double astig45;                     /* Zero-point model for astig45         */
   int    applyModel;                  /* Apply model TRUE/FALSE               */
   double gain0;
   double gain45;
   double offsetAstig0;
   double offsetAstig45;
} AST_ZP_MODEL_ID_STRUCT, * AST_ZP_MODEL_ID;

typedef struct
{
   double a;                           /* cos(3*theta) scale                   */
   double p;                           /* cos phase                            */
   double c;                           /* Constant term for cos trefoil        */
   double b;                           /* sin(3*theta) scale                   */
   double pp;                          /* sin phase                            */
   double d;                           /* Constant term for sin trefoil        */
   double costref;                     /* Zero-point model for cos trefoil     */
   double sintref;                     /* Zero-point model for sin trefoil     */
   int    applyModel;                  /* Apply model TRUE/FALSE               */
} TREF_ZP_MODEL_ID_STRUCT, * TREF_ZP_MODEL_ID;

typedef struct
{
   double a;                           /* cos(theta) scale                     */
   double p;                           /* cos phase                            */
   double c;                           /* Constant term for comaX              */
   double b;                           /* sin(theta) scale                     */
   double pp;                          /* sin phase                            */
   double d;                           /* Constant term for comaY              */
   double comaX;                       /* Zero-point model for comaX           */
   double comaY;                       /* Zero-point model for comaY           */
   int    applyModel;                  /* Apply model TRUE/FALSE               */
} COMA_ZP_MODEL_ID_STRUCT, * COMA_ZP_MODEL_ID;

typedef struct
{
   double a1, a2;                      /* cos(theta), cos(2*theta) scale       */
   double p1, p2;                      /* cos phases                           */
   double c;                           /* Constant term for focus              */
   double focus;                       /* Zero-point model for focus           */
   int    applyModel;                  /* Apply model TRUE/FALSE               */
} FOCUS_ZP_MODEL_ID_STRUCT, * FOCUS_ZP_MODEL_ID;

/**************************************************************** Functions ***/

/* Context lifecycle (implemented). */
AO_CCD_ID  aoCcdContextCreate (void);
AO_CTRL_ID aoCtrlContextCreate (void);
STATUS     aoCcdContextShow (AO_CCD_ID aoCcdId);
STATUS     aoCtrlContextShow (AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId,
                              int verbose);

/* File and image utilities (geometry-independent; being ported). */
STATUS aoFitsImageFloatRead (char * pFitsFileName, float * pImageBuffer,
                             int xBufferSize, int yBufferSize);
STATUS aoFitsImageFloatWrite (char * pFitsFileName, float * pImageBuffer,
                              int xBufferSize, int yBufferSize);
STATUS aoMatRead (char * pMatFileName, int typeExpected, AO_CCD_ID aoCcdId,
                  AO_CTRL_ID aoCtrlId);
STATUS aoMatWrite (char * pMatFileName, double * pMat, int rowNb, int colNb,
                   int type);
STATUS aoDarkSubtract (float * pImage, float * pDark, int xPixels,
                       int yPixels);
STATUS aoImageFloatAverage (float * pImage, AO_CCD_ID aoCcdId,
                            AO_CTRL_ID aoCtrlId, int imageNb);
STATUS aoRmsNoiseImageCompute (float * pImage, AO_CCD_ID aoCcdId,
                               double * pRmsNoise, double * pMeanNoise);
STATUS aoScaleRead (char * pAoScaleFileName, AO_CTRL_ID aoCtrlId);
STATUS aoScaleUpdate (double * pAoScaleVect, AO_CTRL_ID aoCtrlId);
STATUS aoDarkUpdate (char * pDarkFileName, AO_CCD_ID aoCcdId,
                     AO_CTRL_ID aoCtrlId);
STATUS aoCentroidsWrite (char * pCentroidsFileName, double * pCentroids,
                         int centNb, char * pComment);

/* Geometry / calibration setup (needs the HRWFS reference file & rotations). */
STATUS aoRefRead (char * pRefFileName, AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId);
STATUS aoFindParam (float * pRawImage, int xSize, int ySize, AO_CCD_ID aoCcdId,
                    AO_CTRL_ID aoCtrlId, int recentering, float * pOutImage);
STATUS aoCtrlContextInit (char * pInitFileName, AO_CCD_ID aoCcdId,
                          AO_CTRL_ID aoCtrlId);
STATUS aoCtrlFileRead (char * pInitFileName, char * pPath, char * pDarkFileName,
                       char * pFlatFileName, char * pRefFileName,
                       double * pRefX, double * pRefY, char * pAoImFileName,
                       char * pAoCmFileName, char * pSeeingCmFileName,
                       char * pSeeingCvFileName, double * pRms, double * pThresh,
                       double * pTotalThresh, double * pAngleM2,
                       double * pAngleM1, double * pSeeingGain,
                       double * pSlidingFocusGain);

/* Core algorithms (blocked on open questions - stubs for now). */
STATUS aoThresholdCompute (float * pImage, AO_CCD_ID aoCcdId,
                           AO_CTRL_ID aoCtrlId, double ratePixel,
                           double * pThreshold);
STATUS aoThresholdPerSubapCompute (float * pImage, AO_CCD_ID aoCcdId,
                                   AO_CTRL_ID aoCtrlId, double ratePixel);
STATUS aoTotalThresholdCompute (float * pImage, AO_CCD_ID aoCcdId,
                                AO_CTRL_ID aoCtrlId);
STATUS aoCentroidsCompute (float * pImage, AO_CCD_ID aoCcdId,
                           AO_CTRL_ID aoCtrlId, double * pThreshVect,
                           double * pTotalCountsVect, double * pCentroidsVect,
                           double * pErrorCentroidsVect, int * pWfsStatus);
STATUS aoModeCompute (float * pImage, int imageStatus, AO_CCD_ID aoCcdId,
                      AO_CTRL_ID aoCtrlId, int imageNb, int pauseNb,
                      double * pZernikesVect, double * pZernikesErrorsVect,
                      double * pTime, int * pWfsStatus);
STATUS aoModeAnalyze (float * pImage, AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId,
                      double * pCentroidsVect, double * pZernikesVect,
                      double * pZernikesErrorsVect, int * pWfsStatus);

/* Interaction / control matrix computation. */
STATUS aoMatZero (AO_CTRL_ID aoCtrlId);
STATUS aoMatCompute (AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId);

/* Zero-point models (astig/trefoil/coma). */
STATUS aoModInit (AST_ZP_MODEL_ID astModelId, TREF_ZP_MODEL_ID trefModelId,
                  COMA_ZP_MODEL_ID comaModelId);
STATUS aoModAstFileRead (char * pInitFileName, AST_ZP_MODEL_ID astModelId);
STATUS aoModTrefFileRead (char * pInitFileName, TREF_ZP_MODEL_ID trefModelId);
STATUS aoModComaFileRead (char * pInitFileName, COMA_ZP_MODEL_ID comaModelId);

#endif /* __INCaoHrwfsLibh */
