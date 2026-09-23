#ifndef __INCmatrixLibh
#define __INCmatrixLibh

/*
 * MODULE NAME:
 * matrixLib
 *
 * FILENAME:
 * matrixLib.h
 *
 * PURPOSE:
 * Include file for the matrix and vector handling function library
 *
 * AUTHORS:
 * Corinne Boyer
 *
 * HISTORY:
 * Revision 1.0 1999/06/10 creation
 */

/* Constant definitions */
#ifndef ML_FTEXT          /* File format: text */ 
#define ML_FTEXT 0
#endif

#ifndef ML_FBIN           /* File format: binary */
#define ML_FBIN 1
#endif

/* Vector or matrix type */
typedef union _array
{
    char   *pChar ;
    double *pDouble ;
} array ;

/* Function return error codes */
typedef enum
{
    ML_E_RD_FILE         = -10, /* File read error */
    ML_E_WR_FILE         = -11, /* File write error */
    ML_E_FT_FILE         = -12, /* Unknown format file */
    ML_E_SIZE            = -20, /* Size error when multiplying two arrays */
    ML_E_RANGE           = -30, /* Col or row of an array out of range */
    ML_E_INV_RECT        = -40  /* ColNb>rowNb when inverting a rect. matrix*/
} ML_STATUS;

/* Function prototypes */
int readVect ( double *pVect, int vectSize, FILE *pFile, int fileFormat ) ;
int readMat ( double *pMat, int rowNb, int colNb, FILE *pFile, 
              int fileFormat ) ;
int writeVect ( double *pVect, int vectSize, FILE *pFile, int fileFormat ) ;
int writeMat ( double *pMat, int rowNb, int colNb, FILE *pFile, 
               int fileFormat ) ;
int minMaxVect ( double *pVect, int vectSize, double *pMin, double *pMax ) ;
int minMaxMat ( double *pMat, int rowNb, int colNb, double *pMin, 
               double *pMax ) ;
int copyVect ( double *pInVect, double *pOutVect, int vectSize ) ;
int copyMat ( double *pInMat, double *pOutMat, int rowNb, int colNb ) ;
int multVectVect ( double *pVect1, double *pVect2, int vectSize, 
                   double *pProd ) ;
int multMatVect ( double *pMat, int rowNb, int colNb, 
                  double *pVect, int vectSize, 
                  double *pProdVect, int prodVectSize ) ;
int multVectMat ( double *pVect, int vectSize,
                  double *pMat, int rowNb, int colNb,
                  double *pProdVect, int prodVectSize ) ;
int multMatMat ( double *pMat1, int rowNb1, int colNb1,
                 double *pMat2, int rowNb2, int colNb2,
                 double *pMat, int rowNb, int colNb ) ;
int normVect ( double *pVect, int vectSize, double *pNorm ) ;
int exchColMat ( double *pMat, int rowNb, int colNb, int col1, int col2 ) ;
int exchRowMat ( double *pMat, int rowNb, int colNb, int row1, int row2 ) ;
int traceSqMat ( double *pSqMat, int matSize, double *pTrace ) ;
int transMat ( double *pMat, int rowNb, int colNb, double *pTransMat ) ;
int eigenSqMat ( double *pSqMat, int matSize, double *pEigenValVect, 
                 double *pEigenModMat, double prec ) ;
int invSqMat ( double *pSqMat, int matSize, double *pDet ) ;
double myHypot ( double x, double y ) ;
int rectMatToSVD ( double *pRectMat, int rowNb, int colNb, double *pUMat, 
                   double *pVMat, double *pSingValVect, double prec, 
                   double precComp ) ;
int invRectMat ( double *pRectMat, int rowNb, int colNb, double *pInvRectMat, 
                 double prec, double precComp ) ;
#endif /* __INCmatrixLibh */
