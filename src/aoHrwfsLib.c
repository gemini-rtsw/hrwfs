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

/*+
 * aoRefRead - build the active-subaperture mask and load the reference vector.
 *
 * The active-subaperture map is the circular annulus from hrwfsAO.pro findparam:
 * a subaperture (i,j) is used when 1.8 < r < 0.95*(nsp+1)/2 where r is the
 * distance (in subaperture units) from the array centre (nsp/2 - 0.5). The
 * flux-based refinement in findparam (premask > max/8) is omitted here - the
 * geometric aperture is stable and does not need an image.
 *
 * The reference centroid vector (refWfsVect, 2*nsub values: x refs then y refs)
 * is read from pRefFileName as whitespace-separated doubles (the hrwfsAO
 * hrwfs_refmes data). If the file is absent it is left zeroed (uncalibrated,
 * so the pipeline still runs for integration testing) and a warning is logged.
 *-
 */

STATUS aoRefRead (char * pRefFileName, AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId)
{
   int    nsp, nsub, i, j, k, used;
   double centre, rin, rout, dist;
   FILE * fp;

   if ((aoCcdId == NULL) || (aoCtrlId == NULL))
   {
      ERROR_SET (0, "aoRefRead: NULL context", ERROR_LOG_SAVE);
      return (ERROR);
   }

   nsp    = aoCcdId->xSubapNb;
   nsub   = nsp * nsp;
   centre = nsp / 2.0 - 0.5;                 /* 8.5 for nsp = 18              */
   rin    = 1.8;                             /* inner radius (subap units)    */
   rout   = 0.95 * (nsp + 1.0) / 2.0;        /* 9.025 for nsp = 18            */

   /* Active-subaperture mask from the annular aperture geometry. */
   used = 0;
   for (j = 0; j < nsp; j++)
   {
      for (i = 0; i < nsp; i++)
      {
         k    = i + j * nsp;
         dist = sqrt ((i - centre) * (i - centre) + (j - centre) * (j - centre));
         if ((dist > rin) && (dist < rout))
         {
            aoCcdId->subapUsedVect[k] = 1;
            used++;
         }
         else
         {
            aoCcdId->subapUsedVect[k] = 0;
         }
      }
   }
   aoCcdId->subapUsedNb    = used;
   aoCcdId->subapNotUsedNb = nsub - used;
   aoCcdId->centroidsNb    = 2 * used;

   /* Reference centroid vector (default zero if the file is unavailable). */
   for (i = 0; i < 2 * nsub; i++)
   {
      aoCtrlId->refWfsVect[i] = 0.0;
   }

   fp = (pRefFileName != NULL) ? fopen (pRefFileName, "r") : NULL;
   if (fp != NULL)
   {
      int    n = 0;
      double val;
      while ((n < 2 * nsub) && (fscanf (fp, "%lf", &val) == 1))
      {
         aoCtrlId->refWfsVect[n++] = val;
      }
      fclose (fp);
      if (n != 2 * nsub)
      {
         printf ("aoRefRead: WARNING - reference file %s had %d of %d values\n",
                 pRefFileName, n, 2 * nsub);
      }
      strncpy (aoCtrlId->refVectFileName, pRefFileName, STRING_SIZE - 1);
      aoCtrlId->refVectFileName[STRING_SIZE - 1] = '\0';
   }
   else
   {
      printf ("aoRefRead: WARNING - reference file %s not found; using a zero "
              "reference (uncalibrated)\n",
              (pRefFileName != NULL) ? pRefFileName : "(null)");
   }

   aoCtrlId->refInitFlag = TRUE;

   printf ("aoRefRead: %d active subapertures (of %d)\n", used, nsub);

   return (OK);
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

/*+
 * aoCentroidsCompute - per-subaperture centre-of-gravity centroids.
 *
 * Ported from hrwfsAO.pro getmes: for each subaperture, threshold at max/3 and
 * take the centre of gravity (first pass), then re-take it inside a disk of
 * radius npix/4 around that estimate (second pass, to reject neighbouring
 * spots), and clamp. Assumes pImage is the recentred frame with the
 * subaperture grid aligned (geometry from aoCcdId: xSubapNb, xRaster, xPixels).
 * Centroids are written to pCentroidsVect as [x(0..nsub-1), y(0..nsub-1)],
 * each relative to its subaperture centre in pixels.
 *
 * NOTE: faithful IDL translation, not yet numerically validated.
 *-
 */

STATUS aoCentroidsCompute (float * pImage, AO_CCD_ID aoCcdId,
                           AO_CTRL_ID aoCtrlId, double * pThreshVect,
                           double * pTotalCountsVect, double * pCentroidsVect,
                           double * pErrorCentroidsVect, int * pWfsStatus)
{
   int    nsp, npx, npy, stride, i, j, a, b;
   double win;

   if ((pImage == NULL) || (aoCcdId == NULL) || (pCentroidsVect == NULL))
   {
      ERROR_SET (0, "aoCentroidsCompute: NULL argument", ERROR_LOG_SAVE);
      return (ERROR);
   }

   nsp    = aoCcdId->xSubapNb;      /* subapertures across (18)              */
   npx    = aoCcdId->xRaster;       /* pixels per subaperture in X (50)      */
   npy    = aoCcdId->yRaster;       /* pixels per subaperture in Y (50)      */
   stride = aoCcdId->xPixels;       /* image row stride (nsp * npx)          */
   win    = npx / 4.0;              /* second-pass window radius             */

   for (j = 0; j < nsp; j++)
   {
      for (i = 0; i < nsp; i++)
      {
         double smax = 0.0;
         double sum, sumx, sumy, gx, gy, v, clamp;
         int    firstpix = 1;
         int    xc0, yc0;

         /* Subaperture maximum (for the max/3 threshold). */
         for (b = 0; b < npy; b++)
         {
            for (a = 0; a < npx; a++)
            {
               v = pImage[(i * npx + a) + (j * npy + b) * stride];
               if (firstpix || (v > smax)) { smax = v; firstpix = 0; }
            }
         }

         /* First-pass centre of gravity (threshold at max/3). */
         sum = sumx = sumy = 0.0;
         for (b = 0; b < npy; b++)
         {
            for (a = 0; a < npx; a++)
            {
               v = pImage[(i * npx + a) + (j * npy + b) * stride] - smax / 3.0;
               if (v < 0.0) v = 0.0;
               sum  += v;
               sumx += v * (a - npx / 2.0 + 0.5);
               sumy += v * (b - npy / 2.0 + 0.5);
            }
         }
         gx = (sum > 0.0) ? (sumx / sum) : 0.0;
         gy = (sum > 0.0) ? (sumy / sum) : 0.0;

         /* Second pass: restrict to a disk around the first estimate. */
         xc0 = npx / 2 + (int) gx;
         yc0 = npy / 2 + (int) gy;
         sum = sumx = sumy = 0.0;
         for (b = 0; b < npy; b++)
         {
            for (a = 0; a < npx; a++)
            {
               double dx = a - xc0;
               double dy = b - yc0;
               if ((dx * dx + dy * dy) >= (win * win)) continue;
               v = pImage[(i * npx + a) + (j * npy + b) * stride] - smax / 3.0;
               if (v < 0.0) v = 0.0;
               sum  += v;
               sumx += v * (a - npx / 2.0 + 0.5);
               sumy += v * (b - npy / 2.0 + 0.5);
            }
         }
         gx = (sum > 0.0) ? (sumx / sum) : 0.0;
         gy = (sum > 0.0) ? (sumy / sum) : 0.0;

         /* Clamp with a 1.2 safety factor (as in getmes). */
         clamp = 1.2 * npx / 2.0;
         if (gx >  clamp) gx =  clamp;
         if (gx < -clamp) gx = -clamp;
         if (gy >  clamp) gy =  clamp;
         if (gy < -clamp) gy = -clamp;

         pCentroidsVect[i + j * nsp]               = gx;
         pCentroidsVect[nsp * nsp + i + j * nsp]   = gy;
         if (pTotalCountsVect != NULL)
            pTotalCountsVect[i + j * nsp] = sum;
      }
   }

   if (pWfsStatus != NULL) *pWfsStatus = 0;

   return (OK);
}

/*
 * aoContMatCompute - build the control matrix (mask-restricted pseudo-inverse
 * of the analytic interaction matrix), C = (Ma^T Ma)^-1 Ma^T, where Ma is the
 * interaction matrix restricted to the active slopes [nAct x nmodes]. C is
 * stored row-major in aoContMat as [nmodes x nAct]. Ported from the matcom
 * computation in hrwfsAO.pro hrwfs(). Computed once per mask (cached via
 * aoContMatInitFlag).
 */

LOCAL STATUS aoContMatCompute (AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId,
                               const int * activeIdx, int nAct)
{
   int      nmodes = aoCtrlId->aoModeNb;
   double * Ma;
   double * Mt;
   double * MtM;
   double * C;
   double   det;
   int      mode, s;
   STATUS   status = OK;

   Ma  = (double *) malloc ((size_t) nAct * nmodes * sizeof (double));
   Mt  = (double *) malloc ((size_t) nmodes * nAct * sizeof (double));
   MtM = (double *) malloc ((size_t) nmodes * nmodes * sizeof (double));
   C   = (double *) malloc ((size_t) nmodes * nAct * sizeof (double));

   if (!Ma || !Mt || !MtM || !C)
   {
      ERROR_SET (0, "aoContMatCompute: allocation failed", ERROR_LOG_SAVE);
      free (Ma); free (Mt); free (MtM); free (C);
      return (ERROR);
   }

   /* Ma[s][mode] = interaction response of active slope s to mode. */
   for (s = 0; s < nAct; s++)
   {
      for (mode = 0; mode < nmodes; mode++)
      {
         Ma[s * nmodes + mode] =
            aoCtrlId->aoIntMat[mode * (2 * SUBAP_NB) + activeIdx[s]];
      }
   }

   if ((transMat (Ma, nAct, nmodes, Mt) != OK) ||
       (multMatMat (Mt, nmodes, nAct, Ma, nAct, nmodes, MtM, nmodes, nmodes)
        != OK) ||
       (invSqMat (MtM, nmodes, &det) != OK) ||
       (multMatMat (MtM, nmodes, nmodes, Mt, nmodes, nAct, C, nmodes, nAct)
        != OK))
   {
      ERROR_SET (0, "aoContMatCompute: matrix operation failed (singular?)",
                 ERROR_LOG_SAVE);
      status = ERROR;
   }
   else
   {
      (void) copyMat (C, aoCtrlId->aoContMat, nmodes, nAct);
      aoCtrlId->aoContMatInitFlag = TRUE;
   }

   free (Ma); free (Mt); free (MtM); free (C);
   return (status);
}

/*+
 * aoModeCompute - compute Zernike modes from an image.
 *
 * Ported from hrwfsAO.pro hrwfs(): centroids - reference -> mask-restricted
 * pseudo-inverse -> Zernike coefficients. pZernikesVect is filled with all
 * aoModeNb fitted Zernikes (mode k = Zernike k+2). The TCS correction is
 * -pZernikesVect[m] for the AO_NCORR modes in aoCorrModes (applied where the
 * values are published). The control matrix is built once and cached.
 *
 * NOTE: faithful IDL translation, not yet numerically validated (needs
 * hrwfs_refmes.fits and a comparison against the IDL output).
 *-
 */

STATUS aoModeCompute (float * pImage, int imageStatus, AO_CCD_ID aoCcdId,
                      AO_CTRL_ID aoCtrlId, int imageNb, int pauseNb,
                      double * pZernikesVect, double * pZernikesErrorsVect,
                      double * pTime, int * pWfsStatus)
{
   int      nsp, nsub, nmodes, nAct, i, s, status = 0;
   int    * activeIdx = NULL;
   double * cent = NULL;
   double * mes  = NULL;
   double * sActive = NULL;
   STATUS   rc = OK;

   if ((pImage == NULL) || (aoCcdId == NULL) || (aoCtrlId == NULL) ||
       (pZernikesVect == NULL))
   {
      ERROR_SET (0, "aoModeCompute: NULL argument", ERROR_LOG_SAVE);
      return (ERROR);
   }
   if (!aoCtrlId->aoIntMatInitFlag)
   {
      ERROR_SET (0, "aoModeCompute: interaction matrix not computed "
                 "(call aoMatCompute)", ERROR_LOG_SAVE);
      return (ERROR);
   }
   if (aoCcdId->subapUsedNb <= 0)
   {
      ERROR_SET (0, "aoModeCompute: reference/subaperture mask not "
                 "initialised", ERROR_LOG_SAVE);
      return (ERROR);
   }

   nsp    = aoCcdId->xSubapNb;
   nsub   = nsp * nsp;
   nmodes = aoCtrlId->aoModeNb;
   nAct   = 2 * aoCcdId->subapUsedNb;

   activeIdx = (int *)    malloc ((size_t) nAct * sizeof (int));
   cent      = (double *) malloc ((size_t) 2 * nsub * sizeof (double));
   mes       = (double *) malloc ((size_t) 2 * nsub * sizeof (double));
   sActive   = (double *) malloc ((size_t) nAct * sizeof (double));

   if (!activeIdx || !cent || !mes || !sActive)
   {
      ERROR_SET (0, "aoModeCompute: allocation failed", ERROR_LOG_SAVE);
      rc = ERROR;
      goto cleanup;
   }

   /* Active slope indices: used x-slopes then used y-slopes (as ind in IDL). */
   s = 0;
   for (i = 0; i < nsub; i++)
      if (aoCcdId->subapUsedVect[i]) activeIdx[s++] = i;
   for (i = 0; i < nsub; i++)
      if (aoCcdId->subapUsedVect[i]) activeIdx[s++] = nsub + i;

   /* Centroids, then subtract the reference (mes = cent - refWfsVect). */
   if (aoCentroidsCompute (pImage, aoCcdId, aoCtrlId, NULL, NULL, cent, NULL,
                           &status) != OK)
   {
      rc = ERROR;
      goto cleanup;
   }
   for (i = 0; i < 2 * nsub; i++)
      mes[i] = cent[i] - aoCtrlId->refWfsVect[i];

   /* Control matrix (build once for this mask). */
   aoCcdId->centroidsNb = nAct;
   if (!aoCtrlId->aoContMatInitFlag)
   {
      if (aoContMatCompute (aoCcdId, aoCtrlId, activeIdx, nAct) != OK)
      {
         rc = ERROR;
         goto cleanup;
      }
   }

   /* res = C * (active measured slopes). */
   for (s = 0; s < nAct; s++)
      sActive[s] = mes[activeIdx[s]];

   if (multMatVect (aoCtrlId->aoContMat, nmodes, nAct, sActive, nAct,
                    pZernikesVect, nmodes) != OK)
   {
      ERROR_SET (0, "aoModeCompute: control-matrix multiply failed",
                 ERROR_LOG_SAVE);
      rc = ERROR;
      goto cleanup;
   }

   if (pZernikesErrorsVect != NULL)
      for (i = 0; i < nmodes; i++) pZernikesErrorsVect[i] = 0.0;
   if (pTime != NULL) *pTime = 0.0;
   if (pWfsStatus != NULL) *pWfsStatus = status;

cleanup:
   free (activeIdx);
   free (cent);
   free (mes);
   free (sActive);
   return (rc);
}

STATUS aoModeAnalyze (float * pImage, AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId,
                      double * pCentroidsVect, double * pZernikesVect,
                      double * pZernikesErrorsVect, int * pWfsStatus)
{
   /* TODO(REL-845): centroids + modes for analysis/diagnostics. */
   return (aoHrwfsNotImplemented ("aoModeAnalyze"));
}

/* --- Interaction / control matrix computation --------------------------- */

/*
 * Zernike basis + analytic interaction matrix, ported from hrwfsAO.pro
 * (zernumero / prepzernike / zernike_ext / zermes2, F. Rigaut). The model grid
 * is stored column-major (idx = i + j*dim) to match the IDL shift() semantics.
 *
 * NOTE: this is a faithful translation but is NOT yet numerically validated -
 * it compiles, but correctness needs a comparison against the IDL output and/or
 * reference data (hrwfs_refmes.fits). See REL-845-signal-processing-plan.md.
 */

/* gamma(k+1) = k! for integer k >= 0. hrwfsAO used IDL gamma(); all arguments
 * here are integers and Tornado 2.0.2 may lack C99 tgamma, so use a factorial. */
LOCAL double aoFactorial (int k)
{
   double f = 1.0;
   int    i;

   for (i = 2; i <= k; i++)
   {
      f *= (double) i;
   }
   return (f);
}

/* Radial (n) and azimuthal (m) order of Zernike number zn (hrwfsAO zernumero). */
LOCAL void aoZernumero (int zn, int * pN, int * pM)
{
   int j = 0;
   int n, m;

   for (n = 0; n <= 100; n++)
   {
      for (m = 0; m <= n; m++)
      {
         if (((n - m) % 2) == 0)
         {
            j++;
            if (j == zn) { *pN = n; *pM = m; return; }
            if (m != 0)
            {
               j++;
               if (j == zn) { *pN = n; *pM = m; return; }
            }
         }
      }
   }
   *pN = 0;
   *pM = 0;
}

/*
 * Extended, fringe-normalised Zernike zn on the prepared grid, written to z.
 * rmod/teta/maskmod are dim*dim grids from the prepzernike step. Matches
 * hrwfsAO zernike_ext(zn,/fringe): no sqrt(n+1) normalisation, result*maskmod.
 */
LOCAL void aoZernikeExt (int zn, int dim, const float * rmod,
                         const float * teta, const float * maskmod, float * z)
{
   int    n, m, i, k, npix;
   double denom, coef, val;

   aoZernumero (zn, &n, &m);
   npix = dim * dim;

   for (k = 0; k < npix; k++)
   {
      val = 0.0;
      for (i = 0; i <= (n - m) / 2; i++)
      {
         denom = aoFactorial (i) * aoFactorial ((n + m) / 2 - i)
                 * aoFactorial ((n - m) / 2 - i);
         coef = ((i % 2) ? -1.0 : 1.0) * aoFactorial (n - i) / denom;
         val += coef * pow ((double) rmod[k], (double) (n - 2 * i));
      }
      if (m != 0)
      {
         if (zn % 2 == 1)                    /* odd  -> sine term  */
            val *= sin (m * (double) teta[k]);
         else                                /* even -> cosine term*/
            val *= cos (m * (double) teta[k]);
      }
      z[k] = (float) (val * maskmod[k]);
   }
}

/*+
 * aoMatZero - zero the interaction and control matrices.
 *-
 */

STATUS aoMatZero (AO_CTRL_ID aoCtrlId)
{
   int i;

   if (aoCtrlId == NULL)
   {
      ERROR_SET (0, "aoMatZero: NULL control context", ERROR_LOG_SAVE);
      return (ERROR);
   }

   for (i = 0; i < (2 * SUBAP_NB * AO_MODE_NB); i++)
   {
      aoCtrlId->aoIntMat[i]  = 0.0;
      aoCtrlId->aoContMat[i] = 0.0;
   }
   aoCtrlId->aoIntMatInitFlag  = FALSE;
   aoCtrlId->aoContMatInitFlag = FALSE;

   return (OK);
}

/*+
 * aoMatCompute - build the analytic aO interaction matrix.
 *
 * Ported from hrwfsAO.pro zermes2: for each fitted Zernike mode the analytic
 * wavefront slopes are averaged over each subaperture, giving one row of the
 * interaction matrix (all bounding-box subapertures). The control matrix (the
 * mask-restricted pseudo-inverse) is built at observation time in aoModeCompute,
 * so this routine populates only aoIntMat. No hardware is required.
 *-
 */

STATUS aoMatCompute (AO_CCD_ID aoCcdId, AO_CTRL_ID aoCtrlId)
{
   const int    nsp   = HRWFS_LENSLET_NB;              /* 18                   */
   const int    np    = AO_NP;                         /* 20                   */
   const int    dim   = AO_ZERN_DIM;                   /* 362                  */
   const double tdiam = TELESCOPE_DIAMETER;            /* 8 m                  */
   const double app   = HRWFS_PSCALE * RADIAN_TO_ARCSEC; /* rad per pixel      */
   const double xc    = dim / 2.0 - 0.5;
   const double yc    = dim / 2.0 - 0.5;
   const double rad   = dim / 2.0 - 1.0;

   float * rmod;
   float * teta;
   float * maskmod;
   float * pup;
   float * z;
   float * zx;
   float * zy;

   int     npix = dim * dim;
   int     i, j, ii, jj, k, mode;
   double  x, y, rr;

   if (aoCtrlId == NULL)
   {
      ERROR_SET (0, "aoMatCompute: NULL control context", ERROR_LOG_SAVE);
      return (ERROR);
   }

   rmod    = (float *) malloc (npix * sizeof (float));
   teta    = (float *) malloc (npix * sizeof (float));
   maskmod = (float *) malloc (npix * sizeof (float));
   pup     = (float *) malloc (npix * sizeof (float));
   z       = (float *) malloc (npix * sizeof (float));
   zx      = (float *) malloc (npix * sizeof (float));
   zy      = (float *) malloc (npix * sizeof (float));

   if (!rmod || !teta || !maskmod || !pup || !z || !zx || !zy)
   {
      ERROR_SET (0, "aoMatCompute: grid allocation failed", ERROR_LOG_SAVE);
      free (rmod); free (teta); free (maskmod); free (pup);
      free (z); free (zx); free (zy);
      return (ERROR);
   }

   /* prepzernike: build the polar grid, aperture mask (pup) and extended mask. */

   for (j = 0; j < dim; j++)
   {
      for (i = 0; i < dim; i++)
      {
         k  = i + j * dim;
         x  = (double) i - xc;
         y  = (double) j - yc;
         rr = sqrt (x * x + y * y) / rad;
         maskmod[k] = (rr <= 1.2) ? 1.0f : 0.0f;
         rmod[k]    = (float) rr * maskmod[k];
         pup[k]     = (rr <= 1.0) ? 1.0f : 0.0f;
         teta[k]    = ((x == 0.0) && (y == 0.0)) ? 0.0f : (float) atan2 (y, x);
      }
   }

   /* For each fitted Zernike mode, average its slopes over each subaperture. */

   for (mode = 0; mode < AO_MODE_NB; mode++)
   {
      aoZernikeExt (mode + 2, dim, rmod, teta, maskmod, z);

      /* Slopes in pixels: gradient (microns/px) -> m -> rad over subap -> px. */

      for (j = 0; j < dim; j++)
      {
         for (i = 0; i < dim; i++)
         {
            int    ip = ((i + 1) % dim) + j * dim;
            int    im = ((i - 1 + dim) % dim) + j * dim;
            int    jp = i + ((j + 1) % dim) * dim;
            int    jm = i + ((j - 1 + dim) % dim) * dim;
            double gx, gy;

            k  = i + j * dim;
            gx = (z[ip] - z[im]) * pup[k] / 2.0 * 1e-6;
            gy = (z[jp] - z[jm]) * pup[k] / 2.0 * 1e-6;
            gx = gx * np / (tdiam / nsp) / app;
            gy = gy * np / (tdiam / nsp) / app;
            zx[k] = (float) gx;
            zy[k] = (float) gy;
         }
      }

      for (j = 0; j < nsp; j++)
      {
         for (i = 0; i < nsp; i++)
         {
            double tpup = 0.0;
            double sx   = 0.0;
            double sy   = 0.0;
            int    sub  = i + j * nsp;
            double mesx, mesy;

            for (jj = j * np + 1; jj <= (j + 1) * np; jj++)
            {
               for (ii = i * np + 1; ii <= (i + 1) * np; ii++)
               {
                  k     = ii + jj * dim;
                  tpup += pup[k];
                  sx   += zx[k];
                  sy   += zy[k];
               }
            }

            mesx = (tpup > 0.0) ? (sx / tpup) : 0.0;
            mesy = (tpup > 0.0) ? (sy / tpup) : 0.0;

            aoCtrlId->aoIntMat[mode * (2 * SUBAP_NB) + sub]             = mesx;
            aoCtrlId->aoIntMat[mode * (2 * SUBAP_NB) + nsp * nsp + sub] = mesy;
         }
      }
   }

   free (rmod); free (teta); free (maskmod); free (pup);
   free (z); free (zx); free (zy);

   aoCtrlId->aoIntMatInitFlag = TRUE;

   printf ("aoMatCompute: built analytic interaction matrix "
           "(%d modes x %d slopes)\n", AO_MODE_NB, 2 * SUBAP_NB);

   return (OK);
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
