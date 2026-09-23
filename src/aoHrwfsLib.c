/*+
 * MODULE NAME:
 * aoHrwfsLib
 *
 * FILENAME:
 * aoHrwfsLib.c
 *
 * PURPOSE:
 * HRWFS active optics (signal processing) library for REL-845.
 *
 * DESCRIPTION:
 * Ported from pwfs/src/aoPWLib.c (author Corinne Boyer), removing fast-guide
 * (fg) and circular-buffer (cb) functionality and adapting the geometry for
 * the 18 x 18 / 1024 x 1024 HRWFS Shack-Hartmann sensor. See
 * REL-845-signal-processing-plan.md.
 *
 * STATUS: initial scaffold (REL-845 Part 2). The context lifecycle and simple
 * geometry-independent helpers are implemented. The core algorithms
 * (centroids, mode/zernike computation, thresholds, interaction/control matrix,
 * seeing, zero-point models, reference/calibration file reading) are STUBS
 * pending the open questions in the plan:
 *   - the HRWFS SH reference-spot file and exact active-subaperture count,
 *   - the rotation geometry (angleWithM1/M2 - may differ from PWFS),
 *   - the seeing scale factor and coefficient matrix/vector derivation,
 *   - Francois' hrwfsAO.pro reference implementation.
 * Each stub logs and returns ERROR so callers fail safe until implemented.
 *
 * DEFICIENCIES:
 * Not yet compiled against the EPICS/VxWorks toolchain.
 *-
 */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif /* vxWorks */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "errorLib.h"
#include "aoHrwfsLib.h"

/* -------------------------------------------------------------------------- */

/*
 * aoHrwfsNotImplemented - shared helper for the not-yet-ported routines. Logs
 * the routine name and returns ERROR so the caller fails safe. Remove uses as
 * each routine is implemented (REL-845 Part 2).
 */

LOCAL STATUS aoHrwfsNotImplemented (const char * pRoutineName)
{
   ERROR_SET1 (0, "%s: not yet implemented (REL-845 Part 2)",
               ERROR_LOG_SAVE, pRoutineName);
   return (ERROR);
}

/* ============================ Context lifecycle =========================== */

/*+
 * aoCcdContextCreate - allocate and default-initialise a CCD geometry context.
 *-
 */

AO_CCD_ID aoCcdContextCreate (void)
{
   AO_CCD_ID aoCcdId;

   if ((aoCcdId = (AO_CCD_ID) calloc ((size_t) 1, sizeof (AO_CCD_ID_STRUCT)))
       == NULL)
   {
      ERROR_SET (0, "Memory allocation for AO CCD geometry context failed",
                 ERROR_LOG_SAVE);
      return (NULL);
   }

   /*
    * Default geometry from the documented HRWFS hardware. These are overwritten
    * by aoRefRead()/aoCtrlContextInit() once the reference file is available.
    */

   aoCcdId->outputsNb = 1;
   aoCcdId->xSize     = CCD_XSIZE;
   aoCcdId->ySize     = CCD_YSIZE;
   aoCcdId->xMax      = CCD_XSIZE;
   aoCcdId->yMax      = CCD_YSIZE;
   aoCcdId->xBin      = 1;
   aoCcdId->yBin      = 1;
   aoCcdId->xRaster   = HRWFS_SPOT_SEP;
   aoCcdId->yRaster   = HRWFS_SPOT_SEP;
   aoCcdId->xSubapNb  = HRWFS_LENSLET_NB;
   aoCcdId->ySubapNb  = HRWFS_LENSLET_NB;
   aoCcdId->subapNb   = HRWFS_LENSLET_NB * HRWFS_LENSLET_NB;
   aoCcdId->xPixels   = CCD_XSIZE;
   aoCcdId->yPixels   = CCD_YSIZE;
   aoCcdId->pixelsNb  = CCD_SIZE;
   aoCcdId->binningFlag = FALSE;

   return (aoCcdId);
}

/*+
 * aoCtrlContextCreate - allocate and default-initialise an AO control context.
 *-
 */

AO_CTRL_ID aoCtrlContextCreate (void)
{
   AO_CTRL_ID aoCtrlId;

   if ((aoCtrlId = (AO_CTRL_ID) calloc ((size_t) 1, sizeof (AO_CTRL_ID_STRUCT)))
       == NULL)
   {
      ERROR_SET (0, "Memory allocation for AO control context failed",
                 ERROR_LOG_SAVE);
      return (NULL);
   }

   /* calloc has zeroed everything; set the meaningful non-zero defaults. */

   aoCtrlId->aoModeNb          = AO_MODE_NB;
   aoCtrlId->thresholdMethod   = AO_THRESH_SPOTS;
   aoCtrlId->totalMethod       = AO_TOTAL_SPOTS;
   aoCtrlId->thresholdMultCoeff = 1.0;
   aoCtrlId->seeingScaleFactor = 1.0;

   return (aoCtrlId);
}

/*+
 * aoCcdContextShow - print the CCD geometry context for diagnostics.
 *-
 */

STATUS aoCcdContextShow (AO_CCD_ID aoCcdId)
{
   if (aoCcdId == NULL)
   {
      ERROR_SET (0, "aoCcdContextShow: NULL context", ERROR_LOG_SAVE);
      return (ERROR);
   }

   printf ("HRWFS AO CCD geometry context:\n");
   printf ("  CCD size      : %d x %d\n", aoCcdId->xSize, aoCcdId->ySize);
   printf ("  subapertures  : %d x %d (total %d)\n",
           aoCcdId->xSubapNb, aoCcdId->ySubapNb, aoCcdId->subapNb);
   printf ("  used / off    : %d / %d\n",
           aoCcdId->subapUsedNb, aoCcdId->subapNotUsedNb);
   printf ("  raster (px)   : %d x %d\n", aoCcdId->xRaster, aoCcdId->yRaster);
   printf ("  image (px)    : %d x %d (%d total)\n",
           aoCcdId->xPixels, aoCcdId->yPixels, aoCcdId->pixelsNb);
   printf ("  binning       : %s\n", aoCcdId->binningFlag ? "yes" : "no");

   return (OK);
}

/*+
 * aoCtrlContextShow - print the AO control context for diagnostics.
 *-
 */

STATUS aoCtrlContextShow (AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId, int verbose)
{
   if (aoCtrlId == NULL)
   {
      ERROR_SET (0, "aoCtrlContextShow: NULL context", ERROR_LOG_SAVE);
      return (ERROR);
   }

   printf ("HRWFS AO control context:\n");
   printf ("  initialised   : %s\n", aoCtrlId->initFlag ? "yes" : "no");
   printf ("  dark / flat   : %s / %s\n",
           aoCtrlId->darkInitFlag ? "yes" : "no",
           aoCtrlId->flatInitFlag ? "yes" : "no");
   printf ("  ref / scale   : %s / %s\n",
           aoCtrlId->refInitFlag ? "yes" : "no",
           aoCtrlId->aoScaleInitFlag ? "yes" : "no");
   printf ("  int / cont mat: %s / %s\n",
           aoCtrlId->aoIntMatInitFlag ? "yes" : "no",
           aoCtrlId->aoContMatInitFlag ? "yes" : "no");
   printf ("  aO modes      : %d (used %d)\n",
           aoCtrlId->aoModeNb, aoCtrlId->aoModeUsedNb);
   printf ("  threshold     : %f (rms %f)\n",
           aoCtrlId->threshold, aoCtrlId->rms);
   printf ("  seeing / r0   : %f / %f\n", aoCtrlId->seeing, aoCtrlId->r0);

   if (verbose)
   {
      printf ("  dark file     : %s\n", aoCtrlId->darkFileName);
      printf ("  flat file     : %s\n", aoCtrlId->flatFileName);
      printf ("  ref file      : %s\n", aoCtrlId->refVectFileName);
      printf ("  aoIntMat file : %s\n", aoCtrlId->aoIntMatFileName);
      printf ("  aoContMat file: %s\n", aoCtrlId->aoContMatFileName);
   }

   return (OK);
}

/* ===================== Geometry-independent helpers ======================= */

/*+
 * aoDarkSubtract - subtract a dark frame from an image, clamping at zero.
 * Geometry-independent: operates over xPixels * yPixels pixels.
 *-
 */

STATUS aoDarkSubtract (float * pImage, float * pDark, int xPixels, int yPixels)
{
   int   i;
   int   nPixels;

   if ((pImage == NULL) || (pDark == NULL))
   {
      ERROR_SET (0, "aoDarkSubtract: NULL image or dark pointer",
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   nPixels = xPixels * yPixels;

   for (i = 0; i < nPixels; i++)
   {
      pImage[i] -= pDark[i];
      if (pImage[i] < 0.0)
      {
         pImage[i] = 0.0;
      }
   }

   return (OK);
}

/* ======================= Stubs (REL-845 Part 2 TODO) ===================== */
/*
 * The routines below are ported next, once the open questions are resolved.
 * They are grouped so the remaining work is easy to see.
 */

/* --- FITS / matrix / file I/O (geometry-independent; port from aoPWLib) --- */

STATUS aoFitsImageFloatRead (char * pFitsFileName, float * pImageBuffer,
                             int xBufferSize, int yBufferSize)
{
   /* TODO(REL-845): port from aoPWLib (uses fitsio.h). */
   return (aoHrwfsNotImplemented ("aoFitsImageFloatRead"));
}

STATUS aoFitsImageFloatWrite (char * pFitsFileName, float * pImageBuffer,
                              int xBufferSize, int yBufferSize)
{
   /* TODO(REL-845): port from aoPWLib (uses fitsio.h). */
   return (aoHrwfsNotImplemented ("aoFitsImageFloatWrite"));
}

STATUS aoMatRead (char * pMatFileName, int typeExpected, AO_CCD_ID aoCcdId,
                  AO_CTRL_ID aoCtrlId)
{
   /* TODO(REL-845): port matrix file reader; sizes depend on SUBAP_NB. */
   return (aoHrwfsNotImplemented ("aoMatRead"));
}

STATUS aoMatWrite (char * pMatFileName, double * pMat, int rowNb, int colNb,
                   int type)
{
   /* TODO(REL-845): port matrix file writer. */
   return (aoHrwfsNotImplemented ("aoMatWrite"));
}

STATUS aoImageFloatAverage (float * pImage, AO_CCD_ID aoCcdId,
                            AO_CTRL_ID aoCtrlId, int imageNb)
{
   /* TODO(REL-845): accumulate/average into aoCtrlId->sumVect. */
   return (aoHrwfsNotImplemented ("aoImageFloatAverage"));
}

STATUS aoRmsNoiseImageCompute (float * pImage, AO_CCD_ID aoCcdId,
                               double * pRmsNoise, double * pMeanNoise)
{
   /* TODO(REL-845): compute RMS/mean of the noise. */
   return (aoHrwfsNotImplemented ("aoRmsNoiseImageCompute"));
}

STATUS aoScaleRead (char * pAoScaleFileName, AO_CTRL_ID aoCtrlId)
{
   /* TODO(REL-845): read the aO scale-factor vector file. */
   return (aoHrwfsNotImplemented ("aoScaleRead"));
}

STATUS aoScaleUpdate (double * pAoScaleVect, AO_CTRL_ID aoCtrlId)
{
   /* TODO(REL-845): copy a scale-factor vector into the control context. */
   return (aoHrwfsNotImplemented ("aoScaleUpdate"));
}

STATUS aoDarkUpdate (char * pDarkFileName, AO_CCD_ID aoCcdId,
                     AO_CTRL_ID aoCtrlId)
{
   /* TODO(REL-845): read a dark FITS into aoCtrlId->darkVect. */
   return (aoHrwfsNotImplemented ("aoDarkUpdate"));
}

STATUS aoCentroidsWrite (char * pCentroidsFileName, double * pCentroids,
                         int centNb, char * pComment)
{
   /* TODO(REL-845): write centroids to a text file. */
   return (aoHrwfsNotImplemented ("aoCentroidsWrite"));
}

/* --- Geometry / calibration setup (needs HRWFS reference file & rotations) - */

STATUS aoRefRead (char * pRefFileName, AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId)
{
   /* TODO(REL-845): read the SH reference-spot file; this defines the active
    * subaperture map and reference centroids. Blocked on the reference file. */
   return (aoHrwfsNotImplemented ("aoRefRead"));
}

STATUS aoCtrlContextInit (char * pInitFileName, AO_CCD_ID aoCcdId,
                          AO_CTRL_ID aoCtrlId)
{
   /* TODO(REL-845): read the AO control file and populate the context. */
   return (aoHrwfsNotImplemented ("aoCtrlContextInit"));
}

STATUS aoCtrlFileRead (char * pInitFileName, char * pPath, char * pDarkFileName,
                       char * pFlatFileName, char * pRefFileName,
                       double * pRefX, double * pRefY, char * pAoImFileName,
                       char * pAoCmFileName, char * pSeeingCmFileName,
                       char * pSeeingCvFileName, double * pRms, double * pThresh,
                       double * pTotalThresh, double * pAngleM2,
                       double * pAngleM1, double * pSeeingGain,
                       double * pSlidingFocusGain)
{
   /* TODO(REL-845): parse the AO control parameter file. */
   return (aoHrwfsNotImplemented ("aoCtrlFileRead"));
}

/* --- Core algorithms (blocked on open questions) ------------------------- */

STATUS aoThresholdCompute (float * pImage, AO_CCD_ID aoCcdId,
                           AO_CTRL_ID aoCtrlId, double ratePixel,
                           double * pThreshold)
{
   /* TODO(REL-845): compute the detection threshold. */
   return (aoHrwfsNotImplemented ("aoThresholdCompute"));
}

STATUS aoThresholdPerSubapCompute (float * pImage, AO_CCD_ID aoCcdId,
                                   AO_CTRL_ID aoCtrlId, double ratePixel)
{
   /* TODO(REL-845): compute a per-subaperture threshold. */
   return (aoHrwfsNotImplemented ("aoThresholdPerSubapCompute"));
}

STATUS aoTotalThresholdCompute (float * pImage, AO_CCD_ID aoCcdId,
                                AO_CTRL_ID aoCtrlId)
{
   /* TODO(REL-845): compute the total-counts threshold. */
   return (aoHrwfsNotImplemented ("aoTotalThresholdCompute"));
}

STATUS aoCentroidsCompute (float * pImage, AO_CCD_ID aoCcdId,
                           AO_CTRL_ID aoCtrlId, double * pThreshVect,
                           double * pTotalCountsVect, double * pCentroidsVect,
                           double * pErrorCentroidsVect, int * pWfsStatus)
{
   /* TODO(REL-845): compute per-subaperture centroids. Needs the reference
    * map from aoRefRead. */
   return (aoHrwfsNotImplemented ("aoCentroidsCompute"));
}

STATUS aoModeCompute (float * pImage, int imageStatus, AO_CCD_ID aoCcdId,
                      AO_CTRL_ID aoCtrlId, int imageNb, int pauseNb,
                      double * pZernikesVect, double * pZernikesErrorsVect,
                      double * pTime, int * pWfsStatus)
{
   /* TODO(REL-845): centroids -> control matrix -> zernikes. Core routine
    * called from detObserveEnd for the sequence/AO modes. Needs the control
    * matrix and rotation geometry. */
   return (aoHrwfsNotImplemented ("aoModeCompute"));
}

STATUS aoModeAnalyze (float * pImage, AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId,
                      double * pCentroidsVect, double * pZernikesVect,
                      double * pZernikesErrorsVect, int * pWfsStatus)
{
   /* TODO(REL-845): centroids + modes for analysis/diagnostics. */
   return (aoHrwfsNotImplemented ("aoModeAnalyze"));
}

/* --- Interaction / control matrix computation --------------------------- */

STATUS aoIntMatStructZero (AO_CTRL_ID aoCtrlId)
{
   /* TODO(REL-845): zero the interaction-matrix measurement structure. */
   return (aoHrwfsNotImplemented ("aoIntMatStructZero"));
}

STATUS aoIntMatStructShow (AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId)
{
   /* TODO(REL-845): display the interaction-matrix measurement structure. */
   return (aoHrwfsNotImplemented ("aoIntMatStructShow"));
}

STATUS aoMatZero (AO_CTRL_ID aoCtrlId)
{
   /* TODO(REL-845): zero the interaction and control matrices. */
   return (aoHrwfsNotImplemented ("aoMatZero"));
}

STATUS aoMatCompute (AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId)
{
   /* TODO(REL-845): compute interaction + control matrix (SVD via matrixLib). */
   return (aoHrwfsNotImplemented ("aoMatCompute"));
}

/* --- Zero-point models -------------------------------------------------- */

STATUS aoModInit (AST_ZP_MODEL_ID astModelId, TREF_ZP_MODEL_ID trefModelId,
                  COMA_ZP_MODEL_ID comaModelId)
{
   /* TODO(REL-845): initialise the astig/trefoil/coma zero-point models. */
   return (aoHrwfsNotImplemented ("aoModInit"));
}

STATUS aoModAstFileRead (char * pInitFileName, AST_ZP_MODEL_ID astModelId)
{
   /* TODO(REL-845): read the astigmatism zero-point model file. */
   return (aoHrwfsNotImplemented ("aoModAstFileRead"));
}

STATUS aoModTrefFileRead (char * pInitFileName, TREF_ZP_MODEL_ID trefModelId)
{
   /* TODO(REL-845): read the trefoil zero-point model file. */
   return (aoHrwfsNotImplemented ("aoModTrefFileRead"));
}

STATUS aoModComaFileRead (char * pInitFileName, COMA_ZP_MODEL_ID comaModelId)
{
   /* TODO(REL-845): read the coma zero-point model file. */
   return (aoHrwfsNotImplemented ("aoModComaFileRead"));
}
