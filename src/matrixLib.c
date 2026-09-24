static struct {void *v; char *c;} rcsid= {&rcsid,
   "$Id: matrixLib.c 39188 2011-11-17 02:20:32Z aebbers $"};
/*
 * MODULE NAME:
 * matrixLib
 *
 * FILE NAME:
 * matrixLib.c
 *
 * PURPOSE:
 * This file contains vector and matrix handling functions 
 *
 * FUNCTION NAMES: 
 * readVect () - Read a vector from a file
 * readMat () - Read a matrix from a file 
 * writeVect () - Write a vector into a file
 * writeMat () - Write a vector into a file
 * copyVect () - Copy a vector
 * copyMat () - Copy a matrix
 * minMaxVect () - Find the min and max values of a vector
 * minMaxMat () - Find the min and max values of a matrix
 * multVectVect () - Multiply two vectors  
 * multMatVect - Multiply a matrix by a vector
 * multVectMat - Multiply a vector by a matrix
 * multMatMat - Multiply two matrix
 * normVect - Normalize a vector
 * exchColMat - Exchange two columns of a matrix
 * exchRowMat - Exchange two rows of a matrix
 * traceSqMat - Trace of a square matrix
 * transMat - Transposition of a matrix
 * eigenSqMat - Computation of eigen values and modes of a square matrix
 * invSqMat - Invert a square matrix
 * rectMatToSVD - SVD of a rectangular matrix
 * invRectMat - Invert a rectangular matrix using SVD method
 *
 * INCLUDE FILES:
 * matrixLib.h
 *
 * AUTHORS:
 * Corinne Boyer
 *
 * AUTHOR NOTES:
 * This matrix handling library has been written for the needs of the Active
 * Optics and Adaptive Optics System control algorithms. These functions
 * are intended to be used by non real time tasks such as the computation of 
 * the control matrix for an AO system and not intended to be used by real time
 * tasks such as the matrix multiplication needed to compute the commands of
 * the deformable mirror. It is why this library is not optimized, in terms 
 * of temporal performance. 
 * Also, vector and matrix are identical: a single array where for the matrix
 * elements are arranged row by row. Because , functions are written in C,
 * arrays are arranged as follows: array[0,N-1] for a vector with a size = N, 
 * or array[0,rowNb*colNb-1] for a matrix of size (rowNb, colNb). 
 * 
 * HISTORY:
 * Revision 1.0 1999/06/10 creation
 */

#ifdef vxWorks
#include <vxWorks.h>
#endif /*vxWorks */

#include <math.h>
#include <stdio.h>
#include "matrixLib.h"

/*
 *+
 * FUNCTION NAME:
 * readVect
 *
 * INVOCATION:
 * readVect ( pVect, vectSize, pFile, fileFormat )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (<) pVect      (double *) Pointer to the vector 
 * (>) vectSize   (int )     Size of the vector
 * (>) pFile      (FILE *)   Pointer to the file ID 
 * (>) fileFormat (int)      Text or binary file (ML_FTEXT or ML_FBIN)
 *
 * FUNCTION VALUE:
 * (int) OK if the function is run successfully 
 *       ML_E_RD_FILE if file read error occurs
 *       ML_E_FT_FILE if fileFormat is unknown
 *
 * PURPOSE:
 * Read a vector of size vectSize in a file 
 *
 * DESCRIPTION:
 * According to the file format (ascii or bin), read the content of the 
 * file pFile and put it into the vector pVect  
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int readVect 
    ( 
    double         *pVect,
    int            vectSize,
    FILE           *pFile, 
    int            fileFormat
    ) 
{
    int            i ;
    int            status ;

    status = OK ;

    if ( fileFormat == ML_FTEXT )
    {
       for ( i = 0 ; i < vectSize ; i ++ )
           if ( fscanf ( pFile , " %le" , (pVect + i) ) != 1 )
              status = ML_E_RD_FILE ;
    }
    else if ( fileFormat == ML_FBIN )
    {
       if ( fread ( (char *)pVect , sizeof(double) , vectSize , pFile ) !=
            vectSize )
          status = ML_E_RD_FILE ;
    }
    else
       status = ML_E_FT_FILE ; /* unknown format */

    return ( status ) ;
}

/*
 *+
 * FUNCTION NAME:
 * readMat
 *
 * INVOCATION:
 * readMat ( pMat, rowNb, colNb, pFile, fileFormat )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (<) pMat       (double *) Pointer to the matrix 
 * (>) rowNb      (int )     Row number
 * (>) colNb      (int )     Column number
 * (>) pFile      (FILE *)   Pointer to the file ID 
 * (>) fileFormat (int)      Text or binary file (ML_FTEXT or ML_FBIN)
 *
 * FUNCTION VALUE:
 * (int) OK if the function is run successfully 
 *       ML_E_RD_FILE if file read error occurs
 *       ML_E_FT_FILE if fileFormat is unknown
 *
 * PURPOSE:
 * Read a matrix of size (rowNb,colNb) in a file 
 *
 * DESCRIPTION:
 * According to the file format (ascii or bin), read the content of the 
 * file pFile and put it into the matrix pMat. Matrix is arranged as a vector row  
 * by row.
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int readMat 
    ( 
    double        *pMat, 
    int           rowNb, 
    int           colNb, 
    FILE          *pFile, 
    int           fileFormat
    ) 
{
    int           row, col ;
    int           status ;

    status = OK ;
   
    if ( fileFormat == ML_FTEXT )
    {
       for ( row = 0 ; row < rowNb ; row ++ )
       {
           for ( col = 0 ; col < colNb ; col ++ )
           {
               if ( fscanf ( pFile , " %le" , (pMat +row*colNb +col) ) != 1 )
                  status = ML_E_RD_FILE ;
           }
       }
    }
    else if ( fileFormat == ML_FBIN )
    {
       if ( fread ( (char *)pMat , sizeof(double) , rowNb*colNb , pFile ) !=
            rowNb*colNb )
          status = ML_E_RD_FILE ;
    }
    else
       status = ML_E_FT_FILE ; /* unknown format */

    return ( status ) ;
}

/*
 *+
 * FUNCTION NAME:
 * writeVect
 *
 * INVOCATION:
 * writeVect ( pVect, vectSize, pFile, fileFormat )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pVect      (double *) Pointer to the vector 
 * (>) vectSize   (int )     Size of the vector
 * (>) pFile      (FILE *)   Pointer to the file ID 
 * (>) fileFormat (int)      Text or binary file (ML_FTEXT or ML_FBIN)
 *
 * FUNCTION VALUE:
 * (int) OK if the function is run successfully 
 *       ML_E_WR_FILE if file write error occurs
 *       ML_E_FT_FILE if fileFormat is unknown
 *
 * PURPOSE:
 * Write a vector of size vectSize in a file 
 *
 * DESCRIPTION:
 * According to the file format (ascii or bin), write the content of 
 * vector pVect into the file pFile 
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int writeVect 
    ( 
    double         *pVect,
    int            vectSize, 
    FILE           *pFile, 
    int            fileFormat
    ) 
{
    int            i ;
    int            status ;

    status = OK ;

    if ( fileFormat == ML_FTEXT )
    {
       for ( i = 0 ; i < vectSize ; i ++ )
       {
           if ( fprintf ( pFile , " %e" , *(pVect + i) ) < 0 )
              status = ML_E_WR_FILE ;
       }
    }
    else if ( fileFormat == ML_FBIN )
    {
       if ( fwrite ( (char *)pVect , sizeof(double) , vectSize , pFile ) !=
            vectSize )
          status = ML_E_WR_FILE ;
    }
    else
       status = ML_E_FT_FILE ; /* unknown format */

    return ( status ) ;
}

/*
 *+
 * FUNCTION NAME:
 * writeMat
 *
 * INVOCATION:
 * writeMat ( pMat, rowNb, colNb, pFile, fileFormat )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pMat       (double *) Pointer to the matrix 
 * (>) rowNb      (int )     Row number
 * (>) colNb      (int )     Column number
 * (>) pFile      (FILE *)   Pointer to the file ID 
 * (>) fileFormat (int)      Text or binary file (FTEXT or FBIN)
 *
 * FUNCTION VALUE:
 * (int) OK if the function is run successfully 
 *       ML_E_WR_FILE if file write error occurs
 *       ML_E_FT_FILE if fileFormat is unknown
 *
 * PURPOSE:
 * Write a matrix of size (rowNb,colNb) in a file 
 *
 * DESCRIPTION:
 * According to the file format (ascii or bin), write the content of the 
 * the matrix pMat into the file pFile. Matrix is stored as a vector row  
 * by row.
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int writeMat 
    ( 
    double        *pMat, 
    int           rowNb, 
    int           colNb, 
    FILE          *pFile, 
    int           fileFormat
    ) 
{
    int           row, col ;
    int           status ;

    status = OK ;
   
    if ( fileFormat == ML_FTEXT )
    {
       for ( row = 0 ; row < rowNb ; row ++ )
       {
           for ( col = 0 ; col < colNb ; col ++ )
           {
               if ( fprintf ( pFile , " %e" , *(pMat +row*colNb +col) ) < 0 )
                  status = ML_E_WR_FILE ;
           }
       }
    }
    else if ( fileFormat == ML_FBIN )
    {
       if ( fwrite ( (char *)pMat , sizeof(double) , rowNb*colNb , pFile ) !=
            rowNb*colNb )
          status = ML_E_WR_FILE ;
    }
    else
       status = ML_E_FT_FILE ; /* unknown format */

    return ( status ) ;
}

/*
 *+
 * FUNCTION NAME:
 * minMaxVect
 *
 * INVOCATION:
 * minMaxVect ( pVect, vectSize, pMin, pMax )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pVect    (double *) Pointer to the vector 
 * (>) vectSize (int )     Size of the vector
 * (<) pMin     (double)   pointer to the mim value of the vector
 * (<) pMax     (double)   pointer to the max value of the vector
 *
 * FUNCTION VALUE:
 * (int) always OK
 *
 * PURPOSE:
 * Find min and max values of a vector
 *
 * DESCRIPTION:
 * Min and max in terms of absolute value of a vector pVect are found
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int minMaxVect 
    ( 
    double            *pVect, 
    int               vectSize, 
    double            *pMin, 
    double            *pMax 
    )
{
    int               i ;
    double            value ;

    *pMax = fabs ( *pVect ) ;
    *pMin = fabs ( *pVect ) ;

    for ( i = 0 ; i < vectSize ; i ++ )
    {
        value = fabs ( *(pVect + i) ) ;

        if ( value > *pMax ) *pMax = value ;
        if ( value < *pMin ) *pMin = value ;
    }

    return (OK) ;
}

/*
 *+
 * FUNCTION NAME:
 * minMaxMat
 *
 * INVOCATION:
 * minMaxMat ( pMat, rowNb, colNb, pMin, pMax )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pMat  (double *) Pointer to the matrix 
 * (>) rowNb (int )     Row Number
 * (>) colNb (int )     Column Number
 * (<) pMin  (double)   pointer to the mim value of the matrix
 * (<) pMax  (double)   pointer to the max value of the matrix
 *
 * FUNCTION VALUE:
 * (int) always OK
 *
 * PURPOSE:
 * Find min and max values of a matrix
 *
 * DESCRIPTION:
 * Min and max in terms absolute value of the matrix are found
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int minMaxMat 
    ( 
    double            *pMat, 
    int               rowNb, 
    int               colNb, 
    double            *pMin, 
    double            *pMax 
    )
{
    int               row, col ;
    double            value ;

    *pMax = fabs ( *pMat ) ;
    *pMin = fabs ( *pMat ) ;

    for ( row = 0 ; row < rowNb ; row ++ )
    {
        for ( col = 0 ; col < colNb ; col ++ )
        {
            value = fabs ( *(pMat + row*colNb + col) ) ;

            if ( value > *pMax ) *pMax = value ;
            if ( value < *pMin ) *pMin = value ;
        }
    }

    return (OK) ;
}

/*
 *+
 * FUNCTION NAME:
 * copyVect
 *
 * INVOCATION:
 * copyVect ( pInVect, pOutVect, vectSize )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pInVect  (double *) Pointer to the input vector 
 * (<) pOutVect (double *) Pointer to the output vector 
 * (>) vectSize (int )     Size of the vector
 *
 * FUNCTION VALUE:
 * (int) always OK
 *
 * PURPOSE:
 * Copy a vector 
 *
 * DESCRIPTION:
 * Not much to explain: pInvect is copied into pOutVect
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * pOutVect has to be allocated before using this function. Size of the output
 * vector vectSize
 *
 * DEFICIENCIES:
 * None
 *-
 */

int copyVect 
    ( 
    double               *pInVect, 
    double               *pOutVect,
    int                  vectSize
    )
{
    int                  i ;

    for ( i = 0 ; i < vectSize ; i ++ )
        *(pOutVect + i) = *(pInVect + i) ;

    return (OK) ;
}

/*
 *+
 * FUNCTION NAME:
 * copyMat
 *
 * INVOCATION:
 * copyMat ( pInMat, pOutMat, rowNb, colNb )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pInMat  (double *) Pointer to the input matrix 
 * (<) pOutMat (double *) Pointer to the output matrix 
 * (>) rowNb   (int )     Row number
 * (>) colNb   (int )     Column number
 *
 * FUNCTION VALUE:
 * (int) always OK
 *
 * PURPOSE:
 * Copy a matrix 
 *
 * DESCRIPTION:
 * Not much to explain: pInMat is copied into pOutMat
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * pOutMat has to be allocated before using this function. Size of the output
 * matrix (rowNb, colNb)
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int copyMat 
    ( 
    double               *pInMat, 
    double               *pOutMat,
    int                  rowNb,
    int                  colNb 
    )
{
    int                  i ;

    for ( i = 0 ; i < rowNb*colNb ; i ++ )
        *(pOutMat + i) = *(pInMat + i) ;

    return (OK) ;
}

/*
 *+
 * FUNCTION NAME:
 * multVectVect
 *
 * INVOCATION:
 * multVectVect ( pVect1, pVect2, vectSize, pProd )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pVect1   (double *) Pointer to the first vector 
 * (>) pVect2   (double *) Pointer to the second vector 
 * (>) vectSize (int )     Vectors size 
 * (<) pProd	(double *) Pointer to the scalar product
 *
 * FUNCTION VALUE:
 * (int) always OK
 *
 * PURPOSE:
 * Multiply two vectors 
 *
 * DESCRIPTION:
 * |Vect1> = [x1,x2,...,xN] is row vector
 * |Vect2> = [y1,y2,...,yN] is a column vector
 * Prod = x1*y1 + x2*y2 + ... + xN*yN
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int multVectVect 
    ( 
    double         *pVect1, 
    double         *pVect2,
    int            vectSize,
    double         *pProd  
    )
{
    int            i ;

    *pProd = 0.0 ;

    for ( i = 0 ; i < vectSize ; i ++ )
        *pProd += ( *(pVect1 + i) ) * ( *(pVect2 + i) ) ;

    return (OK) ;
}

/*
 *+
 * FUNCTION NAME:
 * multMatVect
 *
 * INVOCATION:
 * multMatVect ( pMat, rowNb, colNb, 
 *               pVect, vectSize, 
 *               pProdVect, prodVectSize )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pMat         (double *) Pointer to the input matrix
 * (>) rowNb        (int)      Row number of the input matrix
 * (>) colNb        (int)      Column number of the input matrix
 * (>) pVect        (double *) Pointer to the input vector 
 * (>) vectSize     (int)      Size of the input vector 
 * (<) pProdVect    (double *) Pointer to the output vector 
 * (>) prodVectSize (int)      Size of the output vector
 *
 * FUNCTION VALUE:
 * (int) OK if the function is run successfully 
 *       ML_E_SIZE if size error between arrays
 *
 * PURPOSE:
 * Multiply a matrix by a vector 
 *
 * DESCRIPTION:
 * Let's R be the row number
 * Let's C be the column number
 * Mat = [m11,m12,...m1C,m21,m22,...,m2C,...,mR1,mR2,...mRC]
 * |Vect> = [v1,v2,...vC]
 * |ProdVect> = [p1,p2,...pR] where pi = mi1*v1 + mi2*v2 + ... + miC*vC
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * The output vector has to be allocated before using this function. The
 * size of the output vector will be equal to the row number of the input 
 * matrix.
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int multMatVect 
    ( 
    double         *pMat, 
    int            rowNb,
    int            colNb,
    double         *pVect,
    int            vectSize,
    double         *pProdVect,
    int            prodVectSize
    )
{
    int            row, col ;
    int            base ;

    if ( (vectSize == colNb) && (prodVectSize == rowNb) )
    {
       for ( row = 0 ; row < rowNb ; row ++ )
       {
           base = row * colNb ;

           *(pProdVect + row) = 0.0 ;

           for ( col = 0 ; col < colNb ; col ++ )
               *(pProdVect + row) += 
               ( *(pMat + base + col) ) * ( *(pVect + col) ) ;
       }

       return ( OK ) ;
    }
    else 
    {
       return ( ML_E_SIZE ) ;
    }
}

/*
 *+
 * FUNCTION NAME:
 * multVectMat
 *
 * INVOCATION:
 * multVectMat ( pVect, vectSize, 
 *               pMat, rowNb, colNb, 
 *               pProdVect, prodVectSize )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pVect        (double *) Pointer to the input line vector 
 * (>) vectSize     (int)      Size of the input vector 
 * (>) pMat         (double *) Pointer to the input matrix
 * (>) rowNb        (int)      Row number of the input matrix
 * (>) colNb        (int)      Column number of the input matrix
 * (<) pProdVect    (double *) Pointer to the output vector 
 * (>) prodVectSize (int)      Size of the output vector
 *
 * FUNCTION VALUE:
 * (int) OK if the function is run successfully 
 *       ML_E_SIZE if size error between arrays
 *
 * PURPOSE:
 * Multiply a line vector by a matrix
 *
 * DESCRIPTION:
 * Let's R be the row number
 * Let's C be the column number
 * Mat = [m11,m12,...m1C,m21,m22,...,m2C,...,mR1,mR2,...mRC]
 * |Vect> = [v1,v2,...vR]
 * |ProdVect> = [p1,p2,...pC] where pj = v1*m1j + v2*m2j + ... + vR*mRj
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * The output vector has to be allocated before using this function. The
 * size of the output vector will be equal to the column number of the input 
 * matrix.
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int multVectMat 
    ( 
    double         *pVect,
    int            vectSize,
    double         *pMat, 
    int            rowNb,
    int            colNb,
    double         *pProdVect,
    int            prodVectSize
    )
{
    int            row, col ;

    if ( (vectSize == rowNb) && (prodVectSize == colNb) )
    {
       for ( col = 0 ; col < colNb ; col ++ )
       {  
           *(pProdVect + col) = 0.0 ;

           for ( row = 0 ; row < rowNb ; row ++ )
               *(pProdVect + col) += 
               ( *(pVect + row) ) * ( *(pMat + row*colNb + col) ) ;
       }

       return ( OK ) ;
    }
    else 
    {
       return ( ML_E_SIZE ) ;
    }
}

/*
 *+
 * FUNCTION NAME:
 * multMatMat
 *
 * INVOCATION:
 * multMatMat ( pMat1, rowNb1, colNb1, 
 *              pMat2, rowNb2, colNb2, 
 *              pMat, rowNb, colNb )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pMat1         (double *) Pointer to the input matrix 1
 * (>) rowNb1        (int)      Row number of the input matrix 1
 * (>) colNb1        (int)      Column number of the input matrix 1
 * (>) pMat2         (double *) Pointer to the input matrix 2
 * (>) rowNb2        (int)      Row number of the input matrix 2
 * (>) colNb2        (int)      Column number of the input matrix 2
 * (<) pMat          (double *) Pointer to the output matrix
 * (>) rowNb         (int)      Row number of the output matrix
 * (>) colNb         (int)      Column number of the output matrix
 *
 * FUNCTION VALUE:
 * (int) OK if the function is run successfully 
 *       ML_E_SIZE if size error between arrays
 *
 * PURPOSE:
 * Multiply two matrix 
 *
 * DESCRIPTION:
 * The product C of two matrix A and B is defined by:
 * cij = aik*bkj where k is summed over for all values of i and j
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * The output matrix has to be allocated before using this function. The
 * size of the output matrix will be equal to (rowNb1, colNb2)
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int multMatMat 
    ( 
    double         *pMat1, 
    int            rowNb1,
    int            colNb1,
    double         *pMat2, 
    int            rowNb2,
    int            colNb2,
    double         *pMat,
    int            rowNb,
    int            colNb
    )
{
    int            i, row, col ;
    int            base1, base2 ;
    double         sum ;

    if ( (colNb1 == rowNb2) && (rowNb == rowNb1) && (colNb == colNb2) )
    {
       for ( row = 0 ; row < rowNb1 ; row ++ )
       {
           base1 = row * colNb1 ;

           base2 = row * colNb2 ;

           for ( col = 0 ; col < colNb2 ; col ++ )
           {
               sum = 0.0 ;

               for ( i = 0 ; i < colNb1 ; i ++ )
                   sum += ( *(pMat1 + base1 + i) ) *
                          ( *(pMat2 + i*colNb2 + col) ) ;

               *(pMat + base2 + col) = sum ;
           }

       }

       return (OK) ;
    }
    else 
    {
       return ( ML_E_SIZE ) ;
    }
}

/*
 *+
 * FUNCTION NAME:
 * normVect
 *
 * INVOCATION:
 * normVect (pVect, vectSize, pNorm )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (<) pVect        (double *) Pointer to the vector 
 * (>) vectSize     (int)      Size of the vector 
 * (<) pNorm        (double *) Norm of the vector
 *
 * FUNCTION VALUE:
 * (int) always OK
 *
 * PURPOSE:
 * Normalize a vector 
 *
 * DESCRIPTION:
 * |Vect>=[v1,v2,...,vN]
 * Norm = sqrt (v1*v1 + v2*v2 + ... + vN*vN)
 * |Vect>=[v1/Norm, v2/Norm,...,vN/Norm], in case of a null vector, this step
 * is skipped 
 * Warning : the input vector is modified
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int normVect 
    ( 
    double         *pVect,
    int            vectSize,
    double         *pNorm
    )
{
    int            i ;
    double         sqNorm ;

    sqNorm = 0.0 ;
    for ( i = 0 ; i < vectSize ; i ++ )
        sqNorm += ( *(pVect + i) ) * ( *(pVect + i) ) ;

    *pNorm = sqrt ( sqNorm ) ;

    if ( *pNorm != 0 )
    {
       for ( i = 0 ; i < vectSize ; i ++ )
           *(pVect + i) = ( *(pVect + i) ) / *pNorm ;
    }

    return ( OK ) ;
}

/*
 *+
 * FUNCTION NAME:
 * exchColMat
 *
 * INVOCATION:
 * exchColMat (pMat, rowNb, colNb, col1, col2 )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (<) pMat   (double *) Pointer to the matrix 
 * (>) rowNb  (int)      Row number of the matrix
 * (>) colNb  (int)      Column number of the matrix 
 * (>) col1   (int)      First column to exchange [0 to colNb-1]
 * (>) col2   (int)      Second column to exchange [0 to colNb-1]
 *
 * FUNCTION VALUE:
 * (int) OK if function is run successfully
 *       ML_E_RANGE if col1, and/or col2 not in [0,colNb-1]
 *
 * PURPOSE:
 * Exchange two columns of a matrix
 *
 * DESCRIPTION:
 * Let's be R the row number and C the column number
 * Let's be i,j the two columns to exchange
 *
 * Before the exchange Mat=[m11,m12,...,m1i,...,m1j,...,m1C,
 *                          m21,m22,...,m2i,...,m2j,...,m2C,
 *                          ...
 *                          mR1,mR2,...,mRi,...,mRj,...,mRC]
 *
 * After the exchange Mat=[m11,m12,...,m1j,...,m1i,...,m1C,
 *                         m21,m22,...,m2j,...,m2i,...,m2C,
 *                         ...
 *                         mR1,mR2,...,mRj,...,mRi,...,mRC]
 *
 * Warning : the input matrix is modified
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int exchColMat 
    ( 
    double         *pMat,
    int            rowNb,
    int            colNb,
    int            col1,
    int            col2
    )
{
    int            row ;
    int            base ;

    double         value ;

    if ( (col1 >= 0) && (col1 < colNb) && (col2 >= 0) && (col2 < colNb) )
    {
       for ( row = 0 ; row < rowNb ; row ++ )
       {
           base = row*colNb ;

           value = *(pMat + base + col1) ;
           *(pMat + base + col1) = ( *(pMat + base + col2) ) ;
           *(pMat + base + col2) =  value ;
       }
       return ( OK ) ;
    }
    else
       return ( ML_E_RANGE ) ;
}

/*
 *+
 * FUNCTION NAME:
 * exchRowMat
 *
 * INVOCATION:
 * exchRowMat (pMat, rowNb, colNb, row1, row2 )
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (<) pMat   (double *) Pointer to the matrix 
 * (>) rowNb  (int)      Row number of the matrix
 * (>) colNb  (int)      Column number of the matrix 
 * (>) row1   (int)      First row to exchange [0 to rowNb-1]
 * (>) row2   (int)      Second row to exchange [0 to rowNb-1]
 *
 * FUNCTION VALUE:
 * (int) OK if function is run successfully
 *       ML_E_RANGE if row1, and/or row2 not in [0,rowNb-1]
 *
 * PURPOSE:
 * Exchange two rows of a matrix
 *
 * DESCRIPTION:
 * Let's be R the row number and C the column number
 * Let's be i,j the two rows to exchange
 *
 * Before the exchange Mat=[m11,m12,...,m1C,
 *                          m21,m22,...,m2C,
 *                          ...
 *                          mi1,mi2,...,miC,
 *                          ...
 *                          mj1,mj2,...,mjC,
 *                          ...
 *                          mR1,mR2,...,mRC]
 *
 * After the exchange Mat=[m11,m12,...,m1C,
 *                         m21,m22,...,m2C,
 *                         ...
 *                         mj1,mj2,...,mjC,
 *                         ...
 *                         mi1,mi2,...,miC,
 *                         ...
 *                         mR1,mR2,...,mRC]
 *
 * Warning : the input matrix is modified
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int exchRowMat 
    ( 
    double         *pMat,
    int            rowNb,
    int            colNb,
    int            row1,
    int            row2
    )
{
    int            col ;

    double         value ;

    if ( (row1 >= 0) && (row1 < rowNb) && (row2 >= 0) && (row2 < rowNb) )
    {
       for ( col = 0 ; col < colNb ; col ++ )
       {
           value = ( *(pMat + row1*colNb + col) ) ;
           *(pMat + row1*colNb + col) = ( *(pMat + row2*colNb + col) ) ;
           *(pMat + row2*colNb + col) = value ;
       }
       return ( OK ) ;
    }
    else
       return ( ML_E_RANGE ) ;
}

/*
 *+
 * FUNCTION NAME:
 * traceSqMat
 *
 * INVOCATION:
 * traceSqMat (pSqMat, matSize, pDet)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pSqMat  (double *) Pointer to the square matrix 
 * (>) matSize (int)      Size of the matrix = row number = column number
 * (<) pTrace  (double *) Trace
 *
 * FUNCTION VALUE:
 * (int) always OK
 *
 * PURPOSE:
 * Compute the trace of the square matrix
 *
 * DESCRIPTION:
 * The trace of a square matrix is defined by:
 * Tr(A) = Sum (aii) where i=0 to matSize-1
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int traceSqMat 
    ( 
    double         *pSqMat,
    int            matSize,
    double         *pTrace
    )
{
    int            i ;

    *pTrace = 0.0 ;
    for ( i = 0 ; i < matSize ; i ++ )
    {
        *pTrace += ( *(pSqMat + i*matSize + i) ) ;
    }
    return ( OK ) ;
}

/*
 *+
 * FUNCTION NAME:
 * transMat
 *
 * INVOCATION:
 * transMat (pMat, rowNb, colNb, pTransMat)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pMat      (double *) Pointer to the input matrix 
 * (>) rowNb     (int)      Row number of the matrix
 * (>) colNb     (int)      Column number of the matrix 
 * (<) pTransMat (double *) Pointer to the the transpose of the matrix
 *
 * FUNCTION VALUE:
 * (int) always OK
 *
 * PURPOSE:
 * Transpose a matrix
 *
 * DESCRIPTION:
 * Let's be R the row number and C the column number of the input matrix
 * Mat = [m11,m12,...,m1C,
 *        m21,m22,...,m2C,
 *        ...
 *        mR1,mR2,...,mRC]
 * TransMat = [m11,m21,...,mR1,
 *             m12,m22,...,mR2,
 *             ...
 *             m1C,m2C,...,mRC]
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * The transpose matrix has to be allocated before using this function. The
 * size of the transpose matrix will be equal to (colNb, rowNb)
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int transMat 
    ( 
    double         *pMat,
    int            rowNb,
    int            colNb,
    double         *pTransMat
    )
{
    int            row ;
    int            col ;

    for ( row = 0 ; row < rowNb ; row ++ )
    {
        for ( col = 0 ; col < colNb ; col ++ )
            *(pTransMat + col*rowNb + row) = ( *(pMat + row*colNb + col) ) ;
    }
    return ( OK ) ;
}

/*
 *+
 * FUNCTION NAME:
 * multSqMatRot
 *
 * INVOCATION:
 * multSqMatRot (pSqMat, matSize, row, col, A, B, C, D)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (<) pSqMat     (double *) Pointer to the square matrix 
 * (>) matSize    (int)      Size of the matrix = row number = column number
 * (>) row        (int)      Row coordinate of the element to rotate 
 * (>) col        (int)      Col coordinate of the element to rotate 
 * (>) A, B, C, D (double)   Elements of the plane rotation matrix
 *
 * FUNCTION VALUE:
 * (int) always OK 
 *
 * PURPOSE:
 * Internal function needed for the computation of the eigen values and the
 * eigen modes of a square matrix
 *
 * DESCRIPTION:
 * Product between a square matrix SqMat of dimension matSize and a rotation 
 * matrix represented by (A,B,C,D). The result matrix is the original square 
 * matrix.
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int multSqMatRot 
    ( 
    double         *pSqMat,
    int            matSize,
    int            row,
    int            col,
    double         A,
    double         B, 
    double         C, 
    double         D
    )
{
    int            index ;
    double         mki ;
    double         mkj ;

    for ( index = 0 ; index < matSize ; index ++ )
    {
        mki = ( *(pSqMat + index*matSize + row) ) ;
        mkj = ( *(pSqMat + index*matSize + col) ) ;

        *(pSqMat + index*matSize + row) = A*mki + C*mkj ;
        *(pSqMat + index*matSize + col) = B*mki + D*mkj ;
    }

    return ( OK ) ;
}

/*
 *+
 * FUNCTION NAME:
 * compRotSqMat
 *
 * INVOCATION:
 * compRotSqMat (pSqMat, matSize, row, col, pA, pB, pC, pD)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pSqMat         (double *) Pointer to the input square matrix 
 * (>) matSize        (int)      Size of the matrix = row number = column number
 * (>) row            (int)      Row coordinate of the element to annihilate 
 * (>) col            (int)      Col coordinate of the element to annihilate 
 * (<) pA, pB, pC, pD (double *) Pointer to the elements of the plane rotation
 *                               Matrix
 *
 * FUNCTION VALUE:
 * (int) always OK 
 *
 * PURPOSE:
 * Internal function needed for the computation of the eigen values and the
 * eigen modes of a square matrix
 *
 * DESCRIPTION:
 * Computation of the plane rotation matrix (A,B,C,D) necessary to annihilate 
 * the off-diagonal matrix elements pSqMat[row,col] and pSqMat[col,row], of 
 * the square matrix pSqMat of size matSize.
 * Elements of the rotation matrix (A,B,C,D) are computed and (A*D - B*C = 1)
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int compRotSqMat 
    ( 
    double         *pSqMat,
    int            matSize,
    int            row,
    int            col,
    double         *pA,
    double         *pB, 
    double         *pC,
    double         *pD 
    )
{
    double         mij, mji, mii, mjj ;
    double         prod ;
    double         diff ;
    double         delta ;
    double         sqrtDelta ;
    double         prodAD ;
    double         sum ;

    mij = ( *(pSqMat + row*matSize + col) ) ;
    mji = ( *(pSqMat + col*matSize + row) ) ;
    mii = ( *(pSqMat + row*matSize + row) ) ;
    mjj = ( *(pSqMat + col*matSize + col) ) ;

    prod = mij*mji ;
    diff = (mjj - mii)/2 ;

    delta = diff*diff + prod ;

    if ( delta < 0 )
    {
       *pA = 1.0 ;
       *pB = 0.0 ;
       *pC = 0.0 ;
       *pD = 1.0 ;
    }
    else
    {
       sqrtDelta = sqrt ( delta ) ;

       if ( diff < 0 ) sqrtDelta = - sqrtDelta ;

       sum = diff + sqrtDelta ;

       *pC = mji/sum ;
       *pB = mij/sum ;

       prodAD = 1.0/( 1 + ( *pB )*( *pC ) ) ;

       if ( prodAD > 0 )
       {
          *pA = sqrt ( prodAD ) ;
          *pD = ( *pA ) ;
       }
       else
       {
          *pA = sqrt ( - prodAD ) ;
          *pD = - ( *pA ) ;
       }

       *pC = - ( *pA )*( *pC ) ;
       *pB = ( *pD )*( *pB ) ;
    }

    return ( OK ) ;
}

/*
 *+
 * FUNCTION NAME:
 * multRotSqMatRot
 *
 * INVOCATION:
 * multRotSqMatRot (pSqMat, matSize, row, col, A, B, C, D)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (<) pSqMat     (double *) Pointer to the square matrix 
 * (>) matSize    (int)      Size of the matrix = row number = column number
 * (>) row        (int)      Row coordinate of the element to rotate 
 * (>) col        (int)      Col coordinate of the element to rotate 
 * (>) A, B, C, D (double)   Elements of the plane rotation matrix
 *
 * FUNCTION VALUE:
 * (int) always OK 
 *
 * PURPOSE:
 * Internal function needed for the computation of the eigen values and the
 * eigen modes of a square matrix
 *
 * DESCRIPTION:
 * Product of a square matrix SqMat of dimension matSize to the right 
 * by a rotation matrix represented by (A,B,C,D) and to the left 
 * by the inverse of this rotation matrix. The result matrix is the original 
 * square matrix. 
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int multRotSqMatRot 
    ( 
    double         *pSqMat,
    int            matSize,
    int            row,
    int            col,
    double         A,
    double         B, 
    double         C,
    double         D
    )
{
    int            index ;

    double         mki, mkj, mik, mjk, mij, mji, mii, mjj ;
    double         value ;

    for ( index = 0 ; index < matSize ; index ++ )
    {
        if ( ( index != row ) && ( index != col ) )
        {
           mki = ( *(pSqMat + index*matSize + row) ) ;
           mkj = ( *(pSqMat + index*matSize + col) ) ;

           *(pSqMat + index*matSize + row) = A*mki + C*mkj ;
           *(pSqMat + index*matSize + col) = B*mki + D*mkj ;

           mik = ( *(pSqMat + row*matSize + index) ) ;
           mjk = ( *(pSqMat + col*matSize + index) ) ;

           *(pSqMat + row*matSize + index) = D*mik - B*mjk ;
           *(pSqMat + col*matSize + index) = A*mjk - C*mik ;
        }
    }

    mij = ( *(pSqMat + row*matSize + col) ) ;
    mji = ( *(pSqMat + col*matSize + row) ) ;
    mii = ( *(pSqMat + row*matSize + row) ) ;
    mjj = ( *(pSqMat + col*matSize + col) ) ;

    value = C*D*mij - A*B*mji ;

    *(pSqMat + row*matSize + row) = A*D*mii + value - B*C*mjj ;

    *(pSqMat + col*matSize + col) = - B*C*mii - value + A*D*mjj  ;

    *(pSqMat + row*matSize + col) = 0 ;
    *(pSqMat + col*matSize + row) = 0 ;

    return ( OK ) ;
}

/*
 *+
 * FUNCTION NAME:
 * eigenSqMat
 *
 * INVOCATION:
 * eigenSqMat (pSqMat, matSize, pEigenValVect, pEigenModMat, prec)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (<) pSymMat       (double *) Pointer to the square matrix 
 * (>) matSize       (int)      Size of the matrix = row number = column number
 * (<) pEigenValVect (double *) Pointer to the vector of the eigen values 
 * (<) pEigenModMat  (double *) Pointer to the matrix of the eigen modes
 * (>) prec          (double)   Computation precision (1e-20)
 *
 * FUNCTION VALUE:
 * (int) always OK 
 *
 * PURPOSE:
 * Compute the eigen values and eigen modes of a square matrix pSqMat
 * with matSize size.
 *
 * DESCRIPTION:
 * The eigen values are stored into the pEigenValVect vector, and the
 * matrix pEigenModMat is a square matrix whose columns contain the 
 * orthonormalized eigen modes. 
 * The original matrix is modified.
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * The eigen mode matrix and the eigen value vector have to be allocated 
 * before using this function. The size of the eigen mode matrix will be equal 
 * to (matSize, matSize) and the size of the eigen value vector will be matSize
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int eigenSqMat 
    ( 
    double         *pSqMat,
    int            matSize,
    double         *pEigenValVect,
    double         *pEigenModMat, 
    double         prec 
    )
{
    int            row, col ;
    int            index ;

    double         threshold ;
    double         A, B, C, D ;
    double         maxBC ;
    double         absBC ;
    double         value ;

    /* Init the eigen modes matrix */

    for ( index = 0 ; index < matSize*matSize ; index ++ )
        *(pEigenModMat + index) = 0.0 ;

    for ( index = 0 ; index < matSize ; index ++ )
        *(pEigenModMat + index*matSize + index) = 1.0 ;

    /* Compute the eigen modes and the eigen values of the matrix */

    threshold = 1.0 ;

    do 
    {
       threshold = threshold/10 ;
       maxBC = 0.0 ;

       for ( col = 1 ; col < matSize ; col ++ )
       {
           for ( row = 0 ; row < col ; row ++ )
           {
               (void) compRotSqMat ( pSqMat, matSize, row, col, 
                                     &A, &B, &C, &D ) ;

               absBC = fabs(B) ;
               if ( ( fabs(C) ) > absBC ) absBC = fabs(C) ;
               if ( absBC > maxBC ) maxBC = absBC ;

               if ( absBC > threshold )
               {
                  (void) multRotSqMatRot ( pSqMat, matSize, row, col, 
                                           A, B, C, D ) ;

                  (void) multSqMatRot ( pEigenModMat, matSize, row, col, 
                                        A, B, C, D ) ;
               }
           }
       }
    } while ( maxBC > prec ) ;

    /* Put the eigen values into the pEigenValVect vector */

    for ( row = 0 ; row < matSize ; row ++ )
        *(pEigenValVect + row) = ( *(pSqMat + row*matSize + row) ) ;

    /* Sort the eigen values into growing order and rearrange the columns */
    /* of the pEigenModMat */

    for ( row = 1 ; row < matSize ; row ++ )
    {
        col = row ;

        while ( (( *(pEigenValVect + col) )
                < ( *(pEigenValVect + col - 1) )) && (col > 0) )
        {
           /* Permutation of the eigen values */

           value = ( *(pEigenValVect + col - 1) ) ;
           *(pEigenValVect + col - 1) = ( *(pEigenValVect + col) ) ;
           *(pEigenValVect + col) = value ;

           /* Permutation of the eigen modes */

           for ( index = 0 ; index < matSize ; index ++ )
           {
               value = ( *(pEigenModMat + index*matSize + col) ) ;
               *(pEigenModMat + index*matSize + col) =
               ( *(pEigenModMat + index*matSize + col - 1) ) ;
               *(pEigenModMat + index*matSize + col - 1) = value ;
           }

           col -- ;
        }
    }

    return ( OK ) ;
}

/*
 *+
 * FUNCTION NAME:
 * invSqMat
 *
 * INVOCATION:
 * invSqMat (pSqMat, matSize, pDet)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (<) pSqMat     (double *) Pointer to the square matrix 
 * (>) matSize    (int)      Size of the matrix = row number = column number
 * (<) pDet       (double *) Determinant of the square matrix
 *
 * FUNCTION VALUE:
 * (int) always OK 
 *
 * PURPOSE:
 * Invert a square matrix
 *
 * DESCRIPTION:
 * The square matrix pSqMat of size matSize is inverted and put into 
 * the original pSqMat by using the methof of Gauss-Jordan. The determinant 
 * pDet of square matrix is also computed.
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * The inverse matrix is put into the input matrix. Before using this function,
 * allocate memory for the inverse matrix and init this inverse matrix with 
 * the matrix to invert.
 * The size of the inverse matrix will be equal to (matSize, matSize) 
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int invSqMat 
    ( 
    double         *pSqMat,
    int            matSize,
    double         *pDet 
    )
{
    void free () ;
    char *calloc () ;

    int            index ;
    int            row, col ;

    double         pivot ;
    double         value ;
    double         mij ;

    array          rowVect ;
    array          colVect ;

    /* Allocate memory for the rowVect and colVect arrays */

    rowVect.pChar = calloc ( (unsigned int)matSize , sizeof(double) ) ;
    colVect.pChar = calloc ( (unsigned int)matSize , sizeof(double) ) ;

    /* Method of Gauss-Jordan */

    *pDet = 1.0 ;

    for ( index = 0 ; index < matSize ; index ++ )
    {
        if ( ( *pDet ) != 0 )
        {
           /* Search the highest element */

           *(rowVect.pDouble + index ) = index ;
           *(colVect.pDouble + index ) = index ;

           pivot = ( *(pSqMat + index*matSize + index) ) ;

           for ( col = index ; col < matSize ; col ++ )
           {
               for ( row = index ; row < matSize ; row ++ )
               {
                   mij = ( *(pSqMat + row*matSize + col) ) ;
                   if ( (fabs(pivot) - fabs(mij)) < 0 )
                   {
                      pivot = mij ;
                      *(rowVect.pDouble + index) = row ;
                      *(colVect.pDouble + index) = col ;
                   }
               }
           }

           row = ( *(rowVect.pDouble + index ) ) ;

           /* Permutation of the rows row and index */

           if ( row != index )
           {
              for ( col = 0 ; col < matSize ; col ++ )
              {
                  value = - ( *(pSqMat + index*matSize + col) ) ;
                  mij = ( *(pSqMat + row*matSize + col) ) ;
                  *(pSqMat + index*matSize + col) = mij ;
                  *(pSqMat + row*matSize + col) = value ;
              }
           }

           col = ( *(colVect.pDouble + index) ) ;

           /* Permutation of the columns col and index */

           if ( col != index )
           {
              for ( row = 0 ; row < matSize ; row ++ )
              {
                  value = - ( *(pSqMat + row*matSize + index) ) ;
                  mij = ( *(pSqMat + row*matSize + col) ) ;
                  *(pSqMat + row*matSize + index) = mij ;
                  *(pSqMat + row*matSize + col) = value ;
              }
           }

           if ( pivot == 0 ) 
              *pDet = 0 ;
           else
           {
              for ( row = 0 ; row < matSize ; row ++ )
              {
                  if ( row != index )
                  {
                     mij = ( *(pSqMat + row*matSize + index) ) ;
                     *(pSqMat + row*matSize + index) = mij / (-pivot) ;
                  }
              }

              for ( row = 0 ; row < matSize ; row ++ )
              {
                  value = ( *(pSqMat + row*matSize + index) ) ;

                  for ( col = 0 ; col < matSize ; col ++ )
                  {
                      if ( ( row != index ) && ( col != index ) )
                      {
                         mij = ( *(pSqMat + index*matSize + col) );
                         *(pSqMat + row*matSize + col) += value * mij ;
                      }
                  }
              }

              for ( col = 0 ; col < matSize ; col ++ )
              {
                  if ( col != index )
                  {
                     mij = ( *(pSqMat + index*matSize + col) ) ;
                     *(pSqMat + index*matSize + col) = mij/pivot ;
                  }
              }

             *pDet = ( *pDet ) * pivot ;

             *(pSqMat + index*matSize + index) = 1.0 / pivot ;
           }
        }
    }

    if ( ( *pDet ) != 0 )
    {
       for ( index = (matSize - 1) ; index >= 0 ; index -- )
       {
           col = ( *(rowVect.pDouble + index) ) ;

           /* Permutation of the columns col and index */

           if ( col > index )
           {
              for ( row = 0 ; row < matSize ; row ++ )
              {
                  value = ( *(pSqMat + row*matSize + index) ) ;
                  mij = ( *(pSqMat + row*matSize + col) ) ;
                  *(pSqMat + row*matSize + index) = - mij ;
                  *(pSqMat + row*matSize + col) = value ;
              }
           }

           row = ( *(colVect.pDouble + index) ) ;

           /* Permutation of the rows row and index */

           if ( row > index )
           {
              for ( col = 0 ; col < matSize ; col ++ )
              {
                  value = ( *(pSqMat + index*matSize + col) ) ;
                  mij = ( *(pSqMat + row*matSize + col) ) ;
                  *(pSqMat + index*matSize + col) = - mij ;
                  *(pSqMat + row*matSize + col) = value ;
              }
           }
       }
    }

    /* Free memory */

    (void) free ( rowVect.pChar ) ;
    (void) free ( colVect.pChar ) ;

    return ( OK ) ;
}

/*
 *+
 * FUNCTION NAME:
 * rectMatToBidiagMat
 *
 * INVOCATION:
 * rectMatToBidiagMat (pRectMat, rowNb, colNb, pUMat, pVMat, pDiagVect, 
 *                     pDiagSupVect, prec, precComp)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pRectMat     (double *) Pointer to the rectangular matrix 
 * (>) rowNb        (int)      Row number 
 * (>) colNb        (int)      Column number
 * (<) pUMat        (double *) Pointer to the rectangular U matrix
 * (<) pVMat        (double *) Pointer to the square V matrix
 * (<) pDiagVect    (double *) Pointer to the vector which contains the 
 *                             diagonal of the bidiagonal form
 * (<) pDiagSupVect (double *) Pointer to the vector which contains the 
 *                             superior diagonal of the bidiagonal form
 * (>) prec         (double)   Computation precision (1e-20)
 * (>) precComp     (double)   Smallest positive number representable on 
 *                             computer (1e-40)
 *
 * FUNCTION VALUE:
 * (int) always OK 
 *
 * PURPOSE:
 * Convert a rectangular matrix into a bidiagonal form.
 * Internal function needed to invert a rectangular matrix
 *
 * DESCRIPTION:
 * Reduction of a rectangular matrix pRectMat of size (rowNb,colNb) with
 * rowNb >= colNb into a bidiagonal form. Householder algorithm. 
 * The diagonal elements of this bidiagonal matrix are put into the vector 
 * pDiagVect of size colNb, the elements of the superior diagonal are put 
 * into a pDiadSupVect vector of dimension colNb. prec represents the 
 * computation precision and precComp the smallest positive number 
 * representable on machine. 
 * A is the rectangular pRectMat : A=U*D*tV where U is a matrix of size
 * (rowNb, colNb) of the orthonormalized eigen modes of the matrix A*tA, 
 * V is a matrix of dimension (colNb,colNb) of the eigen modes of the tA*A 
 * matrix, and D the diagonal matrix of the eigen values of the eigen modes 
 * of the tA*A matrix                                        
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * The U matrix, the V matrix, and the two vectors have to be allocated before 
 * using this function. 
 * The size of the U matrix will be equal to (rowNb, colNb) 
 * The size of the V matrix will be equal to (colNb, colNb)
 * The size of the two vectors is colNb
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int rectMatToBidiagMat 
    ( 
    double    *pRectMat,
    int       rowNb,
    int       colNb,
    double    *pUMat,
    double    *pVMat,
    double    *pDiagVect,
    double    *pDiagSupVect,
    double    prec,
    double    precComp
    )
{
    int       row, col ;
    int       index ;
    int       limit ;

    double    x, y ;
    double    sum ;
    double    value ;
    double    ujj ;
    double    prodSum ;

    /* Init U matrix */

    (void) copyMat ( pRectMat, pUMat, rowNb, colNb ) ;

    /* Others init */

    x = 0.0 ;
    value = 0.0 ;
    limit = 0 ;

    /* Reduction by Householder to a bidiagonal form */

    for ( col = 0 ; col < colNb ; col ++ )
    {
        *(pDiagSupVect + col) = value ;
        limit = col + 1 ;

        sum = 0.0 ;
        for ( row = col ; row < rowNb ; row ++ )
            sum += pow( *(pUMat + row*colNb + col) , (double)2.0 ) ;

        if ( sum < precComp ) 
           value = 0.0 ;
        else
        {
           ujj = ( *(pUMat + col*colNb + col) ) ;

           if ( ujj < 0 ) value = sqrt( sum ) ;
           else  value = ( - sqrt ( sum ) ) ;

           prodSum = ujj*value - sum ;

           *(pUMat + col*colNb + col) = ujj - value ;

           for ( row = limit ; row < colNb ; row ++ )
           {
               sum = 0.0 ;
               for ( index = col ; index < rowNb ; index ++ )
                   sum += ( *(pUMat + index*colNb + col) ) *
                          ( *(pUMat + index*colNb + row ) ) ;

               if ( prodSum != 0.0 )
                  ujj = sum / prodSum ;
               else
                  ujj = 0.0 ;

               for ( index = col ; index < rowNb ; index ++ )
                   *(pUMat + index*colNb + row ) =
                   ( *(pUMat + index*colNb + row) )
                   + ujj*( *(pUMat + index*colNb + col) ) ;
           }
        }

        *(pDiagVect + col) = value ;

        sum = 0.0 ;
        for ( row = limit ; row < colNb ; row ++ )
            sum += pow( *(pUMat + col*colNb + row) , (double)2.0 ) ;

        if ( sum < precComp ) 
           value = 0.0 ;
        else
        {
           ujj = ( *(pUMat + col*colNb + col + 1) ) ;

           if ( ujj < 0 ) value = sqrt( sum ) ;
           else value = ( - sqrt( sum ) ) ;

           prodSum = ujj*value - sum ;

           *(pUMat + col*colNb + col + 1) = ujj - value ;

           if ( prodSum != 0.0 )
              for ( row = limit ; row < colNb ; row ++ )
                  *(pDiagSupVect + row) =
                  ( *(pUMat + col*colNb + row) ) / prodSum  ;
           else
              for ( row = limit ; row < colNb ; row ++ )
                  *(pDiagSupVect + row) = 0.0 ;

           for ( row = limit ; row < rowNb ; row ++ )
           {
               sum = 0.0 ;
               for ( index = limit ; index < colNb ; index ++ )
                   sum += ( *(pUMat + row*colNb + index) )
                          * ( *(pUMat + col*colNb + index) ) ;

               for ( index = limit ; index < colNb ; index ++ )
                   *(pUMat + row*colNb + index) +=
                   ( *(pDiagSupVect + index) ) * sum ;
           }
        }

        y = fabs( *(pDiagVect + col) ) + fabs( *(pDiagSupVect + col) ) ;
        if ( y > x ) x = y ;
    }

    /* Accumulate the transformations of the superior part */

    for ( col = (colNb - 1) ; col >= 0 ; col -- )
    {
        if ( value != 0 )
        {
           prodSum = value * ( *(pUMat + col*colNb + col + 1) ) ;

           if ( prodSum != 0.0 )
              for ( row = limit ; row < colNb ; row ++ )
                  *(pVMat + row*colNb + col) =
                  ( *(pUMat + col*colNb + row) ) / prodSum ;
           else
              for ( row = limit ; row < colNb ; row ++ )
                  *(pVMat + row*colNb + col) = 0.0 ;

           for ( row = limit ; row < colNb ; row ++ )
           {
               sum = 0.0 ;
               for ( index = limit ; index < colNb ; index ++ )
                   sum += ( ( *(pUMat + col*colNb + index) ) *
                          ( *(pVMat + index*colNb + row) ) ) ;

               for ( index = limit ; index < colNb ; index ++ )
                   *(pVMat + index*colNb + row) +=
                   ( sum * ( *(pVMat + index*colNb + col) ) ) ;
           }
        }

        for ( row = limit ; row < colNb ; row ++ )
        {
            *(pVMat + col*colNb + row ) = 0.0 ;
            *(pVMat + row*colNb + col ) = 0.0 ;
        }

        *(pVMat + col*colNb + col ) = 1.0 ;
        value = ( *(pDiagSupVect + col) ) ;
        limit = col ;
    }

    /* Accumulate the transformations of the inferior part */

    for ( col = (colNb - 1) ; col >= 0 ; col -- )
    {
        limit = col + 1 ;

        value = ( *(pDiagVect + col) ) ;

        for ( row = limit ; row < colNb ; row ++ )
            *(pUMat + col*colNb + row) = 0.0 ;

        if ( value != 0 )
        {
           prodSum = value*( *(pUMat + col*colNb + col) ) ;

           for ( row = limit ; row < colNb ; row ++ )
           {
               sum = 0 ;
               for ( index = limit ; index < rowNb ; index ++ )
                   sum += ( *(pUMat + index*colNb + col) ) *
                          ( *(pUMat + index*colNb + row) ) ;

               if ( prodSum != 0.0 )
                  ujj = sum / prodSum ;
               else
                  ujj = 0.0 ;

               for ( index = col ; index < rowNb ; index ++ )
                   *(pUMat + index*colNb + row) +=
                   ujj * ( *(pUMat + index*colNb + col) ) ;
           }

           if ( value != 0.0 )
              for ( row = col ; row < rowNb ; row ++ )
                  *(pUMat + row*colNb + col) /= value ;
           else
              for ( row = col ; row < rowNb ; row ++ )
                  *(pUMat + row*colNb + col) = 0.0 ;
        }
        else
           for ( row = col ; row < rowNb ; row ++ )
               *(pUMat + row*colNb + col) = 0.0 ;

        *(pUMat + col*colNb + col) += 1.0 ;
    }

    prec *= x ;

    return ( OK ) ;
}

/*
 *+
 * FUNCTION NAME:
 * myHypot
 *
 * INVOCATION:
 * myHypot (x, y)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) x (double) length side of the triangle 
 * (>) y (double) length side of the triangle
 *
 * FUNCTION VALUE:
 * (double) length of the hypotenuse of a right angled triangle
 *
 * PURPOSE:
 * Euclidean distance function
 * Internal function needed to invert a rectangular matrix
 *
 * DESCRIPTION:
 *    
 *   |\
 * y | \ hypot = sqrt (x*x+y*y)
 *   |  \
 *   |___\
 *    x
 *
 * This function calls the hypot function of the math library except for 
 * PPC and MC68040 architectures.
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * None known
 *-
 */

double myHypot 
       (
       double x,
       double y 
       )
{
       double length ;
       double X, Y, Z ;

#if defined(PPC) || defined(MC68040)
       /* length = sqrt ( x*x + y*y ) ; leads to overflow and underflow errors */
       length = ((X=fabs(x)) > (Y=fabs(y)) ? (Z=Y/X,X*sqrt(1.0+Z*Z)) : (Y ? (Z=X/Y,Y*sqrt(1.0+Z*Z)):0.0)) ;
#else
       length = hypot ( x, y ) ;
#endif

return ( length ) ;
}

/*
 *+
 * FUNCTION NAME:
 * diagBidiagMat
 *
 * INVOCATION:
 * diagBidiagMat (rowNb, colNb, pUMat, pVMat, pSingValVect, pDiagSupVect, prec)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) rowNb        (int)      Row number 
 * (>) colNb        (int)      Column number
 * (<) pUMat        (double *) Pointer to the rectangular U matrix
 * (<) pVMat        (double *) Pointer to the square V matrix
 * (<) pSingValVect (double *) Pointer to the vector which contains the 
 *                             singular values 
 * (<) pDiagSupVect (double *) Pointer to the vector which contains the 
 *                             superior diagonal of the bidiagonal form
 * (>) prec         (double)   Computation precision (1e-20)
 *
 * FUNCTION VALUE:
 * (int) always OK 
 *
 * PURPOSE:
 * Diagonalize a bidiagonal matrix.
 * Internal function needed to invert a rectangular matrix
 *
 * DESCRIPTION:
 * Diagonalize of a bidiagonal matrix by using the QR algorithm.
 * The pSingValVect od dimension colNb, contains at the beginning the 
 * diagonal of the bidiagonal matrix obtained with rectMatToBidiagForm() 
 * and at the end of the function it contains the singular values of the 
 * original rectangular A matrix.
 * prec represents the computation precision 
 * A is the rectangular pRectMat : A=U*D*tV where U is a matrix of size
 * (rowNb, colNb) of the orthonormalized eigen modes of the matrix A*tA, 
 * V is a matrix of dimension (colNb,colNb) of the eigen modes of the tA*A 
 * matrix, and D the diagonal matrix of the eigen values of the eigen modes 
 * of the tA*A matrix. rowNb >= colNb                                        
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * The U matrix, the V matrix, and the two vectors have to be allocated before 
 * using this function. 
 * The size of the U matrix will be equal to (rowNb, colNb) 
 * The size of the V matrix will be equal to (colNb, colNb)
 * The size of the two vectors is colNb
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int diagBidiagMat 
    ( 
    int       rowNb,
    int       colNb,
    double    *pUMat,
    double    *pVMat,
    double    *pSingValVect,
    double    *pDiagSupVect,
    double    prec
    )
{
    int       row, col ;
    int       index ;
    int       limit ;
    double    dsi, dsj ;
    double    total ;
    double    value ;
    double    factor ;
    double    x, y, z ;

    /* Compute x (prec) */

    z = 0.0 ;
    x = 0.0 ;

    for ( col = 0 ; col < colNb ; col ++ )
    {
        y = fabs( *(pSingValVect + col) ) + fabs( *(pDiagSupVect + col) ) ;

        if ( y > x )   x = y ;
    }

    prec *= x ;

    /* Compute the QR transformations */

    for ( index = (colNb - 1) ; index >= 0 ; index -- )
    {
        limit = index ;
        do
        {
           if ( limit != index )
           {
              x = ( *(pSingValVect + limit) ) ;
              y = ( *(pSingValVect + index - 1) ) ;

              dsi = ( *(pDiagSupVect + index - 1) ) ;
              dsj = ( *(pDiagSupVect + index) ) ;

              if ( (dsj * y) != 0.0 )
                 total = ( (y - z)*(y + z) + (dsi - dsj)*(dsi + dsj) )
                         / ( 2.0 * dsj * y ) ;
              else
                 total = 0.0 ;

              dsi = myHypot ( total , 1.0 ) ;

              total = ( (x - z)*(x + z) + 
                        dsj*(y/( total < 0 ? total - dsi : total + dsi ) - dsj)                       ) / x ;

              /* Following QR transformation */

              value = 1.0 ;
              factor = 1.0 ;

              for ( col = limit + 1 ; col  <= index ; col ++ )
              {
                  dsi = ( *(pDiagSupVect + col) ) ;
                  y = ( *(pSingValVect + col) ) ;
                  dsj = factor * dsi ;
                  dsi = value * dsi ;
                  z = myHypot ( total , dsj ) ;
                  *(pDiagSupVect + col  - 1) = z ;

                  if ( z != 0.0 )
                     value = total / z ;
                  else
                     value = 0.0 ;

                  if ( z != 0.0 )
                     factor = dsj / z ;
                  else
                     factor = 0.0 ;

                  total = x*value + dsi*factor ;
                  dsi = ( (-x)*factor + dsi*value ) ;
                  dsj = y * factor ;
                  y *= value ;

                  for ( row = 0  ; row < colNb ; row ++ )
                  {
                      x = ( *(pVMat + row*colNb + col - 1) ) ;
                      z = ( *(pVMat + row*colNb + col) ) ;
                      *(pVMat + row*colNb + col - 1) = x*value + z*factor ;
                      *(pVMat + row*colNb + col) = ( - x*factor + z*value ) ;
                  }

                  z = myHypot( total , dsj ) ;
                  *(pSingValVect + col - 1) = z  ;

                  if ( z != 0.0 )
                     value = total / z ;
                  else
                     value = 0.0 ;

                  if ( z != 0.0 )
                     factor = dsj / z ;
                  else
                     factor = 0.0 ;

                  total = value*dsi + factor*y ;
                  x = ( - factor*dsi + value*y ) ;

                  for ( row = 0 ; row < rowNb ; row ++ )
                  {
                      y = ( *(pUMat + row*colNb + col - 1) ) ;
                      z = ( *(pUMat + row*colNb + col) ) ;
                      *(pUMat + row*colNb + col - 1) = y*value + z*factor ;
                      *(pUMat + row*colNb + col) = ( - y*factor + z*value ) ;
                  }
              }

              *(pDiagSupVect + limit) = 0 ;
              *(pDiagSupVect + index) = total ;
              *(pSingValVect + index) = x ;
           } ;

           for ( limit = index ; limit >= 0 ; limit -- )
           {
               if ( fabs( *(pDiagSupVect + limit) ) <= prec ) break ;

               if ( fabs( *(pSingValVect + limit - 1) ) <= prec )
               {
                  value = 0.0 ;
                  factor = 1.0 ;

                  for ( col = limit ; col <= index ; col ++ )
                  {
                      total = factor * ( *(pDiagSupVect + col) ) ;
                      *(pDiagSupVect + col) *= value ;

                      if ( fabs( total ) <= prec ) break ;

                      dsi = ( *(pSingValVect + col) ) ;
                      *(pSingValVect + col) = myHypot( total , dsi ) ;
                      dsj = ( *(pSingValVect + col) ) ;

                      if ( dsj != 0.0 )
                      {
                         value = dsi / dsj ;
                         factor = ( - total/dsj ) ;
                      }
                      else
                      {
                         value = 0.0 ;
                         factor = 0.0 ;
                      }

                      for ( row = 0 ; row < rowNb ; row ++ )
                      {
                          y = ( *(pUMat + row*colNb + limit - 1) ) ;
                          z = ( *(pUMat + row*colNb + col) ) ;
                          *(pUMat + row*colNb + limit-1) = y*value + z*factor;
                          *(pUMat + row*colNb + col) = (- y*factor + z*value);
                      }
                  }

                  break ;
               }
           }
           z = ( *(pSingValVect + index) ) ;
        } while ( limit != index ) ;

        if ( z < 0 ) /* pSingValVect(k) positive */
        {
           *(pSingValVect + index) = ( - z ) ;

           for ( row = 0 ; row < colNb ; row ++ )
               *(pVMat + row*colNb + index) =
               ( - ( *(pVMat + row*colNb + index) ) ) ;
        }
    }
    return ( OK ) ;
}

/*
 *+
 * FUNCTION NAME:
 * rectMatToSVD
 *
 * INVOCATION:
 * rectMatToSVD (pRectMat, rowNb, colNb, pUMat, pVMat, pSingValVect, 
 *               prec, precComp)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pRectMat     (double *) Pointer to the rectangular matrix 
 * (>) rowNb        (int)      Row number 
 * (>) colNb        (int)      Column number
 * (<) pUMat        (double *) Pointer to the rectangular U matrix
 * (<) pVMat        (double *) Pointer to the square V matrix
 * (<) pSingValVect (double *) Pointer to the vector which contains 
 *                             the singular values
 * (>) prec         (double)   Computation precision (1e-20)
 * (>) precComp     (double)   Smallest positive number representable on 
 *                             computer (1e-40)
 *
 * FUNCTION VALUE:
 * (int) OK if function is run successfully
 *       ML_E_INV_RECT if colNb > rowNb
 *
 * PURPOSE:
 * Convert a rectangular matrix into SVD 
 * Internal function needed to invert a rectangular matrix
 *
 * DESCRIPTION:
 * Singular value decomposition of the rectangular pRectMat, size 
 * (rowNb, colNb) where rowNb >= colNb .
 * A is the rectangular pRectMat : A=U*D*tV where U is a matrix of size
 * (rowNb, colNb) of the colNb orthonormalized eigen modes of the matrix
 * A*tA (associated to the highest eigen values), V is a matrix of size
 * (colNb,colNb) of the eigen modes of the tA*A matrix, and D is the diagonal
 * matrix of size colNb of the singular values of the A matrix. prec
 * represents the computation precision and precComp the smallest positive 
 * number representable on machine.                       *
 * Reference : Singular value decomposition and least squares solutions,
 * G.H. Golub, C. Reinsh - Handbook Series Linear Algebra -
 * Numer. Math. 14,421_434 (1970): pretty recent, isn't it ?
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * The U matrix, the V matrix, and the vector which contains the singular 
 * values have to be allocated before using this function. 
 * The size of the U matrix will be equal to (rowNb, colNb) 
 * The size of the V matrix will be equal to (colNb, colNb)
 * The size of the vector will be equal to colNb
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int rectMatToSVD 
    ( 
    double    *pRectMat,
    int       rowNb,
    int       colNb,
    double    *pUMat,
    double    *pVMat,
    double    *pSingValVect,
    double    prec,
    double    precComp
    )
{
    void free () ;
    char *calloc() ;

    array     diagSupVect ;

    if ( rowNb >= colNb )
    {
       diagSupVect.pChar = calloc ( (unsigned int)colNb , sizeof(double) ) ;

       (void) rectMatToBidiagMat ( pRectMat, rowNb, colNb, pUMat, pVMat, 
                                   pSingValVect, diagSupVect.pDouble, 
                                   prec, precComp ) ;

       (void) diagBidiagMat ( rowNb, colNb, pUMat, pVMat, pSingValVect , 
                              diagSupVect.pDouble, prec ) ;

       (void) free ( diagSupVect.pChar ) ;
       return ( OK ) ;
    }
    else
       return ( ML_E_INV_RECT ) ;
}

/*
 *+
 * FUNCTION NAME:
 * invRectMat
 *
 * INVOCATION:
 * invRectMat (pRectMat, rowNb, colNb, pInvRectMat, prec, precComp)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * (>) pRectMat     (double *) Pointer to the rectangular matrix 
 * (>) rowNb        (int)      Row number 
 * (>) colNb        (int)      Column number
 * (<) pInvRectMat  (double *) Pointer to the inverse matrix
 * (>) prec         (double)   Computation precision (1e-20)
 * (>) precComp     (double)   Smallest positive number representable on 
 *                             computer (1e-40)
 *
 * FUNCTION VALUE:
 * (int) OK if function is run successfully
 *       ML_E_SIZE if size error when multiplying arrays
 *       ML_E_INV_RECT if colNb > rowNb
 *
 * PURPOSE:
 * Invert a rectangular matrix using SVD method
 *
 * DESCRIPTION:
 * Invert a rectangular matrix pRectMat of size (rowNb, colNb) where 
 * rowNb >= colNb.
 * The method used is the singular value decomposition :
 * A is the rectangular matrix, then A = U*D*tV and the pseudo-inverse 
 * A+ = V*D-1*tU, the size of the pseudo inverse is (colNb, rowNb). prec 
 * represents the computation precision and precComp the smallest positive 
 * number representable on machine.                       
 *
 * EXTERNAL VARIABLES:
 * None
 *
 * PRIOR REQUIREMENTS:
 * The pseudo inverse matrix has to be allocated before using this function. 
 * The size of the pseudo inverse matrix will be equal to (colNb, rowNb) 
 *
 * DEFICIENCIES:
 * None Known
 *-
 */

int invRectMat 
    ( 
    double    *pRectMat,
    int       rowNb,
    int       colNb,
    double    *pInvRectMat,
    double    prec,
    double    precComp
    )
{
    void free () ;
    char *calloc () ;

    int       index ;
    int       status ;

    array     singValVect ;
    array     UMat ;
    array     VMat ;
    array     diagMat ;
    array     invDiagMat ;
    array     transUMat ;
    array     prodMat ;

    /* Singular value decomposition */

    UMat.pChar = calloc ( (unsigned int)(rowNb*colNb) , sizeof (double) ) ;
    VMat.pChar = calloc ( (unsigned int)(colNb*colNb) , sizeof (double) ) ;
    singValVect.pChar = calloc ( (unsigned int)(colNb) , sizeof (double) ) ;

    status = rectMatToSVD ( pRectMat, rowNb, colNb, 
                            UMat.pDouble, VMat.pDouble, singValVect.pDouble, 
                            prec, precComp ) ;

    if ( status == ML_E_INV_RECT ) 
    {
       (void) free ( UMat.pChar ) ;
       (void) free ( VMat.pChar ) ;
       (void) free ( singValVect.pChar ) ;
       return ( ML_E_INV_RECT ) ;
    } 

    /* Compute the pseudo-inverse */

    transUMat.pChar = calloc ( (unsigned int)(colNb*rowNb) , sizeof (double) ) ;
    prodMat.pChar = calloc ( (unsigned int)(colNb*rowNb) , sizeof (double) ) ;

    diagMat.pChar = calloc ( (unsigned int)(colNb*colNb) , sizeof (double) ) ;
    invDiagMat.pChar = calloc ( (unsigned int)(colNb*colNb) , sizeof (double) );

    for ( index = 0 ; index < colNb*colNb ; index ++ ) 
    {
        *(diagMat.pDouble + index) = 0.0 ;
        *(invDiagMat.pDouble + index) = 0.0 ;
    }

    for ( index = 0 ; index < colNb ; index ++ )
        *(diagMat.pDouble + index*colNb + index) =
        ( *(singValVect.pDouble + index) ) ;

    (void) transMat ( UMat.pDouble, rowNb, colNb, transUMat.pDouble ) ;

    for ( index = 0 ; index < colNb ; index ++ )
    {
        if ( *(diagMat.pDouble +index*colNb + index) != 0.0 )
           *(invDiagMat.pDouble + index*colNb + index) =
           1.0 / ( *(diagMat.pDouble + index*colNb + index) ) ;
        else
           *(invDiagMat.pDouble + index*colNb + index) = 0.0 ;
    }

    status = multMatMat ( invDiagMat.pDouble , colNb , colNb ,
                          transUMat.pDouble , colNb , rowNb ,
                          prodMat.pDouble , colNb , rowNb ) ;

    if ( status == ML_E_SIZE )
    {
       (void) free ( singValVect.pChar ) ;
       (void) free ( UMat.pChar ) ;
       (void) free ( VMat.pChar ) ;
       (void) free ( diagMat.pChar ) ;
       (void) free ( invDiagMat.pChar ) ;
       (void) free ( transUMat.pChar ) ;
       (void) free ( prodMat.pChar ) ;
       return ( ML_E_SIZE ) ;
    }

    status = multMatMat ( VMat.pDouble , colNb , colNb ,
                          prodMat.pDouble , colNb , rowNb ,
                          pInvRectMat , colNb , rowNb ) ;

    if ( status == ML_E_SIZE )
    {
       (void) free ( singValVect.pChar ) ;
       (void) free ( UMat.pChar ) ;
       (void) free ( VMat.pChar ) ;
       (void) free ( diagMat.pChar ) ;
       (void) free ( invDiagMat.pChar ) ;
       (void) free ( transUMat.pChar ) ;
       (void) free ( prodMat.pChar ) ;
       return ( ML_E_SIZE ) ;
    }

    /* Free the memory */

    (void) free ( singValVect.pChar ) ;
    (void) free ( UMat.pChar ) ;
    (void) free ( VMat.pChar ) ;
    (void) free ( diagMat.pChar ) ;
    (void) free ( invDiagMat.pChar ) ;
    (void) free ( transUMat.pChar ) ;
    (void) free ( prodMat.pChar ) ;

    return ( OK ) ;
}
