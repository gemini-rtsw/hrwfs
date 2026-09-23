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
#include <math.h>

#include "errorLib.h"
#include "matrixLib.h"
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

STATUS aoFitsImageFloatRead (
   char *     pFitsFileName,
   float *    pImageBuffer,
   int        xBufferSize,
   int        yBufferSize
   )
{
   char       header[2880];
   char       line[81];
   char       restHeader[2880];
   char       keyword[8];
   char       *token;
   char       *delim1 = "=";
   char       *delim2 = "\0";

   int        flag;
   int        bufferSize;
   int        pixelsNb;
   int        bitpix;
   int        naxis;
   int        naxis1;
   int        naxis2;

   long       restSize;
   long       lineSize=80;
   long       nChar;
   long       headerSize=0;

   FILE       *pFile;

#ifdef DEBUG
   int        i;
#endif

   /* Open the FITS file */

   pFile = fopen ( pFitsFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Can't open FITS file %s", ERROR_LOG_SAVE,
                   pFitsFileName);
      return (ERROR);
   }

   /* Read the first line */

   nChar = fread ( header, sizeof (char), lineSize, pFile );

   if ( nChar != lineSize )
   {
      ERROR_SET1 ( 0, "Can't read the first line of %s", ERROR_LOG_SAVE,
                   pFitsFileName);
      fclose ( pFile );
      return (ERROR);
   }

   strncpy ( line, header, lineSize );
   line[81]='\0';

   /* Check this line contains SIMPLE keyword */

   if ( strncmp ( "SIMPLE  ", line, 8 ) != 0 )
   {
      ERROR_SET1 ( 0, "File %s doesn't contain SIMPLE keyword", ERROR_LOG_SAVE,
                   pFitsFileName);
      fclose ( pFile );
      return (ERROR);
   }

   headerSize += 80;

   flag = TRUE;
   while ( flag )
   {
      nChar = fread ( header, sizeof (char), lineSize, pFile );

      if ( nChar != lineSize )
      {
         ERROR_SET1 ( 0, "Can't read the next line of %s", ERROR_LOG_SAVE,
                      pFitsFileName);
         fclose ( pFile );
         return (ERROR);
      }

      strncpy ( line, header, lineSize );
      line[81]= '\0';

      strncpy ( keyword, line, 8 );

      token = strtok ( line, delim1);
      token = strtok ( NULL, delim2);

      if ( strncmp ( "END     ", keyword, 8) == 0 ) 
         flag = FALSE;
      if ( strncmp ( "BITPIX  ", keyword, 8) == 0 ) 
         sscanf ( token, "%d", &bitpix);
      if ( strncmp ( "NAXIS   ", keyword, 8) == 0 ) 
         sscanf ( token, "%d", &naxis);
      if ( strncmp ( "NAXIS1  ", keyword, 8) == 0 ) 
         sscanf ( token, "%d", &naxis1);
      if ( strncmp ( "NAXIS2  ", keyword, 8) == 0 ) 
         sscanf ( token, "%d", &naxis2);

      headerSize += 80;
   }

   if ( headerSize % 2880 != 0 )
   {
      restSize = 2880 - (headerSize % 2880);
      nChar = fread ( restHeader, sizeof (char), restSize, pFile );
      if ( nChar != restSize )
      {
         ERROR_SET1 ( 0, "Can't read the rest of the header of %s", 
                      ERROR_LOG_SAVE, pFitsFileName);
         fclose ( pFile );
         return (ERROR);
      }
   }

   /* Now read the data */

   bufferSize = xBufferSize * yBufferSize;
   pixelsNb = naxis1 * naxis2;

   if ( pixelsNb != bufferSize )
   {
      ERROR_SET2 ( 0, "Dark Image size %d not as expected %d",
                   ERROR_LOG_SAVE, (int)pixelsNb, bufferSize);
      fclose ( pFile );
      return (ERROR);
   }

   if ( fread ( pImageBuffer, sizeof (float), bufferSize, pFile ) != 
        bufferSize )
   {
      ERROR_SET1 ( 0, "Failed to read image from %s", ERROR_LOG_SAVE,
                   pFitsFileName);
      fclose ( pFile );
      return (ERROR);
   }

#ifdef DEBUG
   for ( i = 0 ; i < 10 ; i ++ )
       printf ( "pixel %d = %f\n", i, *(pImageBuffer + i) );
#endif

   /* Close the FITS file */

   fclose ( pFile );

   return (OK);
}

STATUS aoFitsImageFloatWrite (
   char *     pFitsFileName,
   float *    pImageBuffer,
   int        xBufferSize,
   int        yBufferSize
   )
{
   int        i;                /* index                                      */
   int        bufferSize;       /* Size of the buffer to write                */
   FILE       *pFile;           /* File descriptor                            */

   /* Create the FITS file */

   pFile = fopen ( pFitsFileName , "w" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Can't create FITS file %s", ERROR_LOG_SAVE,
                   pFitsFileName );
      return (ERROR);
   }

   /* Write a minimal header */

   fprintf ( pFile, "SIMPLE  =                    T /                                                " );
   fprintf ( pFile, "BITPIX  =                  -32 /                                                " );
   fprintf ( pFile, "NAXIS   =                    2 /                                                " );
   fprintf ( pFile, "NAXIS1  =                %5d /                                                ", xBufferSize );
   fprintf ( pFile, "NAXIS2  =                %5d /                                                ", yBufferSize );
   fprintf ( pFile, "BZERO   =                    0 /                                                " );
   fprintf ( pFile, "EXTEND  =                    T /                                                " );
   fprintf ( pFile, "END                                                                             ");

   /* Fill the rest of the header with blanks: header 36 * 80 char */

   for ( i = 0 ; i < 28 ; i ++ )
       fprintf ( pFile, "                                                                                " );

   /* Write the image to the Fits file */

   bufferSize = xBufferSize * yBufferSize;

   if ( fwrite ( pImageBuffer, sizeof (float), bufferSize, pFile ) != 
        bufferSize )
   {
      ERROR_SET1 ( 0, "Failed to write image into %s",
                   ERROR_LOG_SAVE, pFitsFileName );

      fclose ( pFile );
      return (ERROR);
   }

   /* Close the fits file */

   fclose ( pFile ) ;

   return ( OK ) ;
}

STATUS aoMatRead (
   char *     pMatFileName,
   int        typeExpected,
   AO_CCD_ID  aoCcdId,
   AO_CTRL_ID aoCtrlId
   )
{

   int        type;                 /* Type of the matrix         */
   int        row, col;             /* Dimension of the matrix    */
   int        i, j;                 /* Index                      */
   float      value;                /* Element of the matrix      */
   AO_MATRIX  mat;                  /* Matrix read                */
   char       comment[STRING_SIZE]; /* First line of comments     */
   FILE *     pFile;                /* File Id                    */

   /* Open the file in read mode */

   pFile = fopen ( pMatFileName, "r" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the matrix file %s",
                   ERROR_LOG_SAVE, pMatFileName );
      return (ERROR);
   }

   /* Read the first line: should be a comment line */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
                   "Failed to read line of comments from the matrix file %s",
                   ERROR_LOG_SAVE, pMatFileName );

      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoMatRead(): %s\n" , comment );
#endif

   /* The next line contains the type of the matrix */

   if ( (fscanf (pFile, "%d\n", &type)) == EOF )
   {
      ERROR_SET1 ( 0,
                   "Failed to read the type of the matrix in the file %s",
                   ERROR_LOG_SAVE, pMatFileName );
      fclose (pFile);
      return (ERROR);
   }

   if ( (type != AO_INT_MAT_TYPE) && (type != AO_CONT_MAT_TYPE) )
   {
      ERROR_SET1 ( 0,
            "Type of the matrix is unrecognized: %d (should be 0 or 1)",
            ERROR_LOG_SAVE, type );
      fclose (pFile);
      return (ERROR);
   }

   if ( type != typeExpected )
   {
      ERROR_SET2 ( 0,
            "Type of the matrix is not the one expected: %d (should be %d)",
            ERROR_LOG_SAVE, type, typeExpected);
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoMatRead(): type of the matrix %s\n" ,
            (type ? "CONTROL" : "INTERACTION") );
#endif

   /* Read the next line of comments */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
                   "Failed to read line of comments from the matrix file %s",
                   ERROR_LOG_SAVE, pMatFileName );
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoMatRead(): %s\n", comment );
#endif

   /* The next line contains the dimensions of the matrix */

   if ( (fscanf (pFile, "%d %d\n", &row, &col)) == EOF )
   {
      ERROR_SET1 ( 0,
                   "Failed to read the dimension of the matrix in the file %s",
                   ERROR_LOG_SAVE, pMatFileName );
      fclose (pFile);
      if (type == AO_INT_MAT_TYPE)
         aoCtrlId->aoIntMatInitFlag = FALSE;
      else
         aoCtrlId->aoContMatInitFlag = FALSE;
      return (ERROR);
   }

   if ( type == AO_INT_MAT_TYPE ) /* interaction matrix */
   {
      if ( (row != aoCcdId->centroidsNb) || (col != aoCtrlId->aoModeNb) )
      {
         ERROR_SET4 ( 0,
            "Dimension of the matrix (%d,%d) are not the ones expected %d,%d)",
            ERROR_LOG_SAVE, row, col, aoCcdId->centroidsNb, aoCtrlId->aoModeNb);
         aoCtrlId->aoIntMatInitFlag = FALSE;
         fclose (pFile);
         return (ERROR);
      }
   }
   else             /* control matrix */
   {
      if ( (row != aoCtrlId->aoModeNb) || (col != aoCcdId->centroidsNb) )
      {
         ERROR_SET4 ( 0,
            "Dimension of the matrix (%d,%d) are not the ones expected %d,%d)",
            ERROR_LOG_SAVE, row, col, aoCtrlId->aoModeNb, aoCcdId->centroidsNb);
         aoCtrlId->aoContMatInitFlag = FALSE;
         fclose (pFile);
         return (ERROR);
      }
   }

#ifdef DEBUG
   printf ( "aoMatRead(): dimensions of the matrix %d, %d\n", row, col );
#endif

   /* Read the next line of comments */

   if ( fgets (comment, STRING_SIZE, pFile) == (char *)NULL )
   {
      ERROR_SET1 ( 0,
                   "Failed to read line of comments from the matrix file %s",
                   ERROR_LOG_SAVE, pMatFileName );
      if (type == AO_INT_MAT_TYPE)
         aoCtrlId->aoIntMatInitFlag = FALSE;
      else
         aoCtrlId->aoContMatInitFlag = FALSE;
      fclose (pFile);
      return (ERROR);
   }

#ifdef DEBUG
   printf ( "aoMatRead(): %s\n", comment );
#endif

   /* Now read the matrix */

   for ( i = 0 ; i < row ; i ++ )
   {
       for ( j = 0 ; j < col ; j ++ )
       {
           if ( (fscanf (pFile, "%f", &value)) != EOF )
           {
              *(mat + i*col + j) = (double)(value);
           }
           else
           {
              ERROR_SET1 ( 0, "Failed to read matrix from file %s",
                           ERROR_LOG_SAVE, pMatFileName );
              if (type == AO_INT_MAT_TYPE)
                 aoCtrlId->aoIntMatInitFlag = FALSE;
              else
                 aoCtrlId->aoContMatInitFlag = FALSE;
              fclose (pFile);
              return (ERROR);
           }
       }
   }

   /* Close the file */

   fclose (pFile);

   /* Init the aoCtrlId structure */

   if ( type == AO_INT_MAT_TYPE )
   {
      strcpy ( aoCtrlId->aoIntMatFileName, pMatFileName );
      aoCtrlId->aoIntMatInitFlag = TRUE;
      (void) copyMat ( mat, aoCtrlId->aoIntMat, row, col);
      aoCtrlId->aoContMatInitFlag = FALSE;
   }
   else
   {
      strcpy ( aoCtrlId->aoContMatFileName, pMatFileName );
      aoCtrlId->aoContMatInitFlag = TRUE;
      (void) copyMat ( mat, aoCtrlId->aoContMat, row, col);
   }

#ifdef DEBUG
   printf ( "aoMatRead(): matrix\n" );
   for ( i = 0 ; i < row ; i ++ )
   {
       for ( j = 0 ; j < col ; j ++ )
           printf ( "%f ", mat[i*col +j]);
       printf ( "\n" );
   }
#endif
   return (OK);
}

STATUS aoMatWrite (
   char *     pMatFileName,
   double *   pMat,
   int        rowNb,
   int        colNb,
   int        type
   )
{
   int      i, j;                 /* Index                      */
   FILE *   pFile;                /* File Id                    */

   /* Open the file in write mode */

   pFile = fopen ( pMatFileName, "w" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the matrix file %s",
                   ERROR_LOG_SAVE, pMatFileName );
      return (ERROR);
   }

   /* Write the first line: should be a comment line */

   (void) fprintf (pFile,
          "# Type of the matrix (0: Interaction, 1: Control)\n");

   /* Write the type of the matrix */

   (void) fprintf (pFile, "%d\n", type);

   /* Write the next line of comment */

   (void) fprintf (pFile, "# Dimensions\n" );

   /* Write the dimensions */

   (void) fprintf (pFile, "%d %d\n", rowNb, colNb );

   /* Write the next line of comments */

   (void) fprintf (pFile, "# Matrix\n" );

   /* Now write the matrix */

   for ( i = 0 ; i < rowNb ; i ++ )
   {
       for ( j = 0 ; j < colNb ; j ++ )
           (void) fprintf (pFile, "%f ", *(pMat + i*colNb + j) );
       (void) fprintf (pFile, "\n" );
   }
     
   /* Close the file */

   fclose (pFile);


#ifdef DEBUG
   printf ( "aoMatWrite: Write matrix into %s done \n" , pMatFileName );
#endif

   return (OK);
}

STATUS aoImageFloatAverage (
   float *      pImage,
   AO_CCD_ID    aoCcdId,
   AO_CTRL_ID   aoCtrlId,
   int          imageNb
   )
{
   int          imageSize;
   float *      p;
   float *      pi;
   float *      ps;
   float *      pMax;

   /* Some initialisations */

   imageSize = aoCcdId->pixelsNb;
   pi = pImage;
   ps = aoCtrlId->sumVect;
   pMax = (float *)((int)ps + imageSize*sizeof(float));
  
   /* Coadd images */

   if ( aoCtrlId->coaddCounter == 0 )
   {
      for ( p = ps ; p < pMax ; )
      {
          *(p++) = *(pi++);
      }
      aoCtrlId->coaddCounter ++;
#ifdef DEBUG
      printf ( "Pixel[0]=%f, Sum[0]=%f\n" , *pImage, aoCtrlId->sumVect[0]);
#endif

   }
   else
   {
      if ( aoCtrlId->coaddCounter < imageNb )
      {
         for ( p = ps ; p < pMax ; p ++ )
         {
             *p = ( *(p) + *(pi++) );
         }
         aoCtrlId->coaddCounter ++;
#ifdef DEBUG
         printf ( "Pixel[0]=%f, Sum[0]=%f\n" , *pImage, aoCtrlId->sumVect[0]);
#endif

      }

      if  ( aoCtrlId->coaddCounter == imageNb )
      {
          for ( p = ps ; p < pMax; p ++ )
          {
               *p = (*(p) / imageNb);
          }
          aoCtrlId->coaddCounter = 0;
#ifdef DEBUG
          printf ( "Sum[0]=%f\n" , aoCtrlId->sumVect[0]);
#endif

      }
   }

   return (OK);
}

STATUS aoRmsNoiseImageCompute (
   float *      pImage,
   AO_CCD_ID    aoCcdId,
   double *     pRmsNoise,
   double *     pMeanNoise
   )
{
   int          imageSize;
   float *      p;
   float *      pi;
   float *      pMax;
   double       value;
   double       meanPixel;
   double       variance;
   double       rmsrms;

   /* Some initialisations */

   imageSize = aoCcdId->pixelsNb;
   pi = pImage;
   pMax = (float *)((int)pi + imageSize*sizeof(float));

   /* Compute mean and variance */

   meanPixel = 0.0;
   variance = 0.0;

   for ( p = pi ; p < pMax ; p ++ )
   {
       value = (double)(*p);

       meanPixel += value;
   
       variance += (value * value);
   }

   meanPixel = meanPixel / (double)(aoCcdId->pixelsNb);

   *pMeanNoise = meanPixel;

   variance = variance / (double)(aoCcdId->pixelsNb);

   rmsrms = variance - (meanPixel*meanPixel);

   /* Compute the rms of the noise */

   if ( rmsrms < 0.0 )
   {
      ERROR_SET (0, "Variance of the noise is negative" , ERROR_LOG_SAVE);
      *pRmsNoise = 0.0;
      return (ERROR);
   }

   *pRmsNoise = sqrt ( rmsrms );

#ifdef DEBUG
   printf ( "Mean=%f, rms=%f\n", (float)*pMeanNoise, (float)*pRmsNoise );
#endif

   return (OK);
}

STATUS aoScaleRead (char * pAoScaleFileName, AO_CTRL_ID aoCtrlId)
{
   /* TODO(REL-845): read the aO scale-factor vector file. */
   return (aoHrwfsNotImplemented ("aoScaleRead"));
}

STATUS aoScaleUpdate (
   double *   pAoScaleVect,
   AO_CTRL_ID aoCtrlId
   )
{

   int      i;                            /* Index                            */

   /* Update the aoCtrlId structure */

   strcpy ( aoCtrlId->aoScaleFileName, "Through dm" );

   aoCtrlId->aoModeNotUsedNb = 0;
   for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
   {
       aoCtrlId->aoScaleFactorVect[i] = pAoScaleVect[i];
       if ( pAoScaleVect[i] == 0.0 )
       {
          aoCtrlId->aoModeUsedVect[i] = FALSE;
          aoCtrlId->aoModeNotUsedNb += 1;
       }
       else
       {
          aoCtrlId->aoModeUsedVect[i] = TRUE;
       }
   }

   aoCtrlId->aoModeUsedNb = aoCtrlId->aoModeNb - aoCtrlId->aoModeNotUsedNb;

   aoCtrlId->aoScaleInitFlag = TRUE;

#ifdef DEBUG
   printf ( "aoScaleUpdate(): \n" );
   for ( i = 0 ; i < aoCtrlId->aoModeNb ; i ++ )
       printf ( "aO mode %d: %f\n" , i+1, aoCtrlId->aoScaleFactorVect[i]);
#endif

   return (OK);
}

STATUS aoDarkUpdate (char * pDarkFileName, AO_CCD_ID aoCcdId,
                     AO_CTRL_ID aoCtrlId)
{
   /* TODO(REL-845): read a dark FITS into aoCtrlId->darkVect. */
   return (aoHrwfsNotImplemented ("aoDarkUpdate"));
}

STATUS aoCentroidsWrite (
   char *     pCentroidsFileName,
   double *   pCentroids,
   int        centNb,
   char *     pComment
   )
{
   int      i;                    /* Index                      */
   FILE *   pFile;                /* File Id                    */

   /* Open the file in write mode */

   pFile = fopen ( pCentroidsFileName, "w" );

   if ( pFile == (FILE *)NULL )
   {
      ERROR_SET1 ( 0, "Failed to open the centroids file %s",
                   ERROR_LOG_SAVE, pCentroidsFileName );
      return (ERROR);
   }

   /* Write the first line: should be a comment line */

   (void) fprintf (pFile,
          "# Comments \n");

   /* Write the comment line */

   (void) fprintf (pFile, "%s\n", pComment);

   /* Write the next line of comment */

   (void) fprintf (pFile, "# Dimension\n" );

   /* Write the dimension */

   (void) fprintf (pFile, "%d\n", centNb );

   /* Write the next line of comments */

   (void) fprintf (pFile, "# Centroids\n" );

   /* Now write the centroids */

   for ( i = 0 ; i < centNb ; i ++ )
   {
       (void) fprintf (pFile, "%f\n", *(pCentroids + i) );
   }

   /* Close the file */

   fclose (pFile);

#ifdef DEBUG
   printf ( "aoCentroidsWrite: Write centroids into %s done \n" ,
            pCentroidsFileName );
#endif

   return (OK);
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
