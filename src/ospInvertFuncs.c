#ifdef VXWORKS
#include <vxWorks.h>
#endif /* VXWORKS */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#define NRANSI
#include "osp.h"

/******************* Following defined in mynrutil.h*******************
void nrerror(char error_text[]);
float *vector(long nl, long nh);
float **matrix(long nrl, long nrh, long ncl, long nch);
void free_vector(float *v, long nl, long nh);
void free_matrix(float **m, long nrl, long nrh, long ncl, long nch);
*/


void ospShowMatrix(float ** matr, int rows, int cols)
/*****************************************************************************/
/* View a matrix on standard output                                          */
/*****************************************************************************/
{
    int k;
    int l;

    for (k=1;k<=rows;k++) {
	for (l=1;l<=cols;l++) printf("%9.3f",matr[k][l]);
	printf("\n");
    }
    return;
}
/*--------------------------------------------------------------------------*/

void ospShowVector(float *vect, int n)
/*****************************************************************************/
/* View a vector on standard output in row format                            */
/*****************************************************************************/
{
    int l;
    for (l=1;l<=n;l++) printf("%9.3f",vect[l]);
    printf("\n");
    return;
}
/*--------------------------------------------------------------------------*/

void ospMatrixProduct(float ** result, float ** a, float ** b, 
                                         int rows1, int cols1, int cols2)
/*****************************************************************************/
/* calculate matrix product   [result] = [a] [b]                             */
/* [a] is rows1 by cols1, [b] is cols1 by cols2                              */
/*****************************************************************************/
{
    int j;
    int k;
    int l;

    for (k=1;k<=rows1;k++) {
	for (l=1;l<=cols2;l++) {
		result[k][l]=0.0;
		for (j=1;j<=cols1;j++)
			result[k][l] += a[k][j]*b[j][l];
	}
    }
    return;
}

void ospConditionOfW(float *w, int n)
/*****************************************************************************/
/* Output information about the condition of the diagonal matrix w, which    */
/* is calculated as part of the singular valued decomposition                */
/*****************************************************************************/
{
    int k;
    float wmin = 100.0;
    float wmax = 0;

    printf("Diagonal of matrix w\n");
    for (k=1;k<=n;k++){
        printf("%9.3f",w[k]);
	if (w[k]>wmax) wmax= w[k];
	if (w[k]<wmin) wmin= w[k];
    }
    printf("\n");
    if (wmin < 0.0001)
 	printf("The matrix is singular (wmin= %f)\n",wmin);
    else
        printf("Condition number is %f\n",wmax/wmin);
    printf("\n");

    return;
}
/*--------------------------------------------------------------------------*/

void ospCalculateInverse(float ** inv,float ** u,float ** v,
                                              float *w, int rows, int cols)
/*****************************************************************************/
/* Evaluate [v]*[1/w]*[u-transpose], the inverse of the matrix supplied as   */
/* the first argument to svdcmp on input, which is itself equal to           */
/* [u]*[w]*[v-transpose].                                                    */
/*****************************************************************************/
{
    int j;
    int k;
    int l;
    for (k=1;k<=rows;k++) {
	for (l=1;l<=cols;l++) {
	    inv[k][l]=0.0;
	    for (j=1;j<=rows;j++){
		if (w[j] <= 0.0001) inv[k][l] += 0.0;
		else
		inv[k][l] += v[k][j]*(1.0/w[j])*u[l][j];
	    }
	}
    }

	return;
}

/*--------------------------------------------------------------------------*/

int /*STATUS*/ ospWriteMatrixToFile(char * outfile, float ** matr, int rows, int cols)
/*****************************************************************************/
/* Write a (rows)*(cols) matrix to a file as specified. The data is formatted*/
/* as a 2-d array with the row data spaced by a single space. This array is  */
/* preceded by two lines of text less than MAXSTR long, a line with two space*/
/* separated integers denoting the row and column dimensions of the matrix,  */
/* and a third line of text with the same length restriction. The lines of   */
/* are ignored by read_matrix_from_file, but are necessary for compatibility */
/* and are useful for storing information about the matrix.                  */
/*****************************************************************************/
{
    int k;
    int l;

    FILE *fp;

    if((fp = fopen(outfile,"w"))==NULL)
    {
	fprintf(stderr,"Error in opening file %s ",outfile);
	fprintf(stderr,"for writing during function writeMatrixToFile\n");
	return(ERROR);
    }
    fprintf(fp,"Written by write_matrix_to_file.\n");
    fprintf(fp,"Next line shows the row and column dimensions.\n");
    fprintf(fp,"%d %d\n",rows,cols);
    fprintf(fp,"Remember row data should be separated by a single space\n");

    for (k=1;k<=rows;k++){
	for (l=1;l<=cols;l++) {
		fprintf(fp,"%f ",matr[k][l]);
	}
	fprintf(fp,"\n");
    }
    fclose(fp);
    return(OK);
}
/*--------------------------------------------------------------------------*/

int /*STATUS*/ ospWriteVectorToFile(char * outfile, float * vect, int n)
/*****************************************************************************/
/* write a vector to file, one data item per line                            */
/*****************************************************************************/
{
    int j;

    FILE *fp;

    if((fp = fopen(outfile,"w"))==NULL)
    {
	fprintf(stderr,"Error in opening file %s ",outfile);
	fprintf(stderr,"for writing during function writeVectorToFile");
	return(ERROR);
    }
    for(j=1;j<=n;j++) 
	fprintf(fp, "%f\n",vect[j]);
    
    fclose(fp);
    return(OK);
}
/*--------------------------------------------------------------------------*/

int /*STATUS*/ ospReadMatrixFromFile(char * infile, float ** matr)
/*****************************************************************************/
/* Read a matrix from a file as specified. The data should be formatted      */
/* as a 2-d array with the row data spaced by a single space. This array is  */
/* preceded by two lines of text less than MAXSTR long, a line with two space*/
/* separated integers denoting the row and column dimensions of the matrix,  */
/* and a third line of text with the same length restriction. The text lines */
/* are ignored by read_matrix_from_file, but are useful for storing          */
/* information about the matrix.                                             */
/*****************************************************************************/
{
    int k;
    int l;
    int n;
    int m;
    char dummy[OSP_MAXSTR];
    int ospstatus=OK;
    FILE *fp;

    if ((fp = fopen(infile,"r")) == NULL)
    {
	fprintf(stderr,"Error in opening file %s ",infile);
	fprintf(stderr,"for reading during function readMatrixFromFile\n");
	return(ERROR);
    }

    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	fprintf(stderr,"Error reading first line of %s ",infile);
	fprintf(stderr,"during function readMatrixFromFile\n");
	fprintf(stderr,"...should be text comment\n");
	ospstatus=ERROR;
    }
    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	fprintf(stderr,"Error reading second line of %s ",infile);
	fprintf(stderr,"during function readMatrixFromFile\n");
	fprintf(stderr,"...should be text comment\n");
	ospstatus=ERROR;
    }
    if((fscanf(fp,"%d %d ",&m,&n))==EOF)
    {
	fprintf(stderr,"Error reading third line of %s ",infile);
	fprintf(stderr,"during function readMatrixFromFile\n");
	fprintf(stderr,"...should be (space separated) matrix dimensions\n");
	ospstatus=ERROR;
    }
    if((fgets(dummy,OSP_MAXSTR,fp))==NULL)
    {
	fprintf(stderr,"Error reading fourth line of %s ",infile);
	fprintf(stderr,"during function readMatrixFromFile\n");
	fprintf(stderr,"...should be text comment\n");
	ospstatus=ERROR;
    }
    for (k=1;k<=m;k++)
	for (l=1;l<=n;l++) 
	{
	    if((fscanf(fp,"%f ",&matr[k][l]))==EOF)
	    {
		fprintf(stderr,"Error reading element [%d][%d] of matrix stored in %s ", k,l,infile);
		fprintf(stderr,"during function readMatrixFromFile\n");
		ospstatus=ERROR;
	    }
	}
    fclose(fp);
    return(ospstatus);
}
/*--------------------------------------------------------------------------*/


int /*STATUS*/ ospReadVectorFromFile(char * infile, float * vect, int n0, int ntot, int nstep)
/*****************************************************************************/
/* Reads vectors from files. If the data required is interleaved in a        */
/* regular manner with unrequired data, this is dealt with by selection of   */
/* a start point (n0) and step (nstep).                                      */
/*****************************************************************************/
{
    int i;
    FILE *fp;
    int ospstatus=OK;
    if ((fp = fopen(infile,"r")) == NULL)
    {
	fprintf(stderr,"Error in opening file %s ",infile);
	fprintf(stderr,"for reading during function readVectorFromFile\n");
	return(ERROR);
    }

    for(i=n0;i<=ntot;i=i+nstep)
	if((fscanf(fp,"%f ",&vect[i]))==EOF)
	{
		fprintf(stderr,"Error reading element [%d] of vector stored in %s\n", i,infile);
		fprintf(stderr,"during function readVectorFromFile\n");
		ospstatus=ERROR;
	}
    fclose(fp);
    return(ospstatus);
}
/*--------------------------------------------------------------------------*/

void ospApplyControlMatrix(float ** c, float * z, float * s, int rows, int cols)
/*****************************************************************************/
/* Apply the control matrix c to the vector of displacements s, to achieve   */
/* the vector of coefficients of the basis functions z. If the control matrix*/
/* has been premultiplied by the matrix representation of the basis functions*/
/* in terms of the Zernike polynomials, z is the Zernike coefficients        */
/*****************************************************************************/
{
    int j;
    int k;

    for(j=1;j<=rows;j++){
	z[j]=0.0;
	for (k=1;k<=cols;k++)
	    z[j] += c[j][k]*s[k];
	}
    return;
}
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* Routines moved from nrutil.c, prototyped in osp.h                  */
/*--------------------------------------------------------------------------*/

void ospnrerror(char error_text[])
/* Numerical Recipes standard error handler */
{
	fprintf(stderr,"Numerical Recipes run-time error...\n");
	fprintf(stderr,"%s\n",error_text);
	return;
}

float *vector(long nl, long nh)
/* allocate a float vector with subscript range v[nl..nh] */
{
	float *v;

	v=(float *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(float)));
	if (!v) ospnrerror("allocation failure in vector()");
	return v-nl+NR_END;
}

float **matrix(long nrl, long nrh, long ncl, long nch)
/* allocate a float matrix with subscript range m[nrl..nrh][ncl..nch] */
{
	long i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
	float **m;

	/* allocate pointers to rows */
	m=(float **) malloc((size_t)((nrow+NR_END)*sizeof(float*)));
	if (!m) ospnrerror("allocation failure 1 in matrix()");
	m += NR_END;
	m -= nrl;

	/* allocate rows and set pointers to them */
	m[nrl]=(float *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(float)));
	if (!m[nrl]) ospnrerror("allocation failure 2 in matrix()");
	m[nrl] += NR_END;
	m[nrl] -= ncl;

	for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;

	/* return pointer to array of pointers to rows */
	return m;
}

void freeVector(float *v, long nl, long nh)
/* free a float vector allocated with vector() */
{
	free((FREE_ARG) (v+nl-NR_END));
}

void freeMatrix(float **m, long nrl, long nrh, long ncl, long nch)
/* free a float matrix allocated by matrix() */
{
	free((FREE_ARG) (m[nrl]+ncl-NR_END));
	free((FREE_ARG) (m+nrl-NR_END));
}


float pythag(float a, float b)
{
        float absa,absb;
        absa=fabs(a);
        absb=fabs(b);
        if (absa > absb) return absa*sqrt(1.0+SQR(absb/absa));
        else return (absb == 0.0 ? 0.0 : absb*sqrt(1.0+SQR(absa/absb)));
}

void svdcmp(float **a, int m, int n, float w[], float **v)
{
        float pythag(float a, float b);
        int flag,i,its,j,jj,k,l,nm;
        float anorm,c,f,g,h,s,scale,x,y,z,*rv1;

        rv1=vector(1,n);
        g=scale=anorm=0.0;
        for (i=1;i<=n;i++) {
                l=i+1;
                rv1[i]=scale*g;
                g=s=scale=0.0;
                if (i <= m) {
                        for (k=i;k<=m;k++) scale += fabs(a[k][i]);
                        if (scale) {
                                for (k=i;k<=m;k++) {
                                        a[k][i] /= scale;
                                        s += a[k][i]*a[k][i];
                                }
                                f=a[i][i];
                                g = -SIGN(sqrt(s),f);
                                h=f*g-s;
                                a[i][i]=f-g;
                                for (j=l;j<=n;j++) {
                                        for (s=0.0,k=i;k<=m;k++) s += a[k][i]*a[k][j];
                                        f=s/h;
                                        for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
                                }
                                for (k=i;k<=m;k++) a[k][i] *= scale;
                        }
                }
                w[i]=scale *g;
                g=s=scale=0.0;
                if (i <= m && i != n) {
                        for (k=l;k<=n;k++) scale += fabs(a[i][k]);
                        if (scale) {
                                for (k=l;k<=n;k++) {
                                        a[i][k] /= scale;
                                        s += a[i][k]*a[i][k];
                                }

                                f=a[i][l];
                                g = -SIGN(sqrt(s),f);
                                h=f*g-s;
                                a[i][l]=f-g;
                                for (k=l;k<=n;k++) rv1[k]=a[i][k]/h;
                                for (j=l;j<=m;j++) {
                                        for (s=0.0,k=l;k<=n;k++) s += a[j][k]*a[i][k];
                                        for (k=l;k<=n;k++) a[j][k] += s*rv1[k];
                                }
                                for (k=l;k<=n;k++) a[i][k] *= scale;
                        }
                }
                anorm=FMAX(anorm,(fabs(w[i])+fabs(rv1[i])));
        }
        for (i=n;i>=1;i--) {
                if (i < n) {
                        if (g) {
                                for (j=l;j<=n;j++)
                                        v[j][i]=(a[i][j]/a[i][l])/g;
                                for (j=l;j<=n;j++) {
                                        for (s=0.0,k=l;k<=n;k++) s += a[i][k]*v[k][j];
                                        for (k=l;k<=n;k++) v[k][j] += s*v[k][i];
                                }
                        }
                        for (j=l;j<=n;j++) v[i][j]=v[j][i]=0.0;
                }
                v[i][i]=1.0;
                g=rv1[i];
                l=i;
        }
        for (i=IMIN(m,n);i>=1;i--) {
                l=i+1;
                g=w[i];
                for (j=l;j<=n;j++) a[i][j]=0.0;
                if (g) {
                        g=1.0/g;
                        for (j=l;j<=n;j++) {
                                for (s=0.0,k=l;k<=m;k++) s += a[k][i]*a[k][j];
                                f=(s/a[i][i])*g;
                                for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
                        }
                        for (j=i;j<=m;j++) a[j][i] *= g;
                } else for (j=i;j<=m;j++) a[j][i]=0.0;
                ++a[i][i];
        }
        for (k=n;k>=1;k--) {
                for (its=1;its<=30;its++) {
                        flag=1;
                        for (l=k;l>=1;l--) {
                                nm=l-1;
                                if ((float)(fabs(rv1[l])+anorm) == anorm) {
                                        flag=0;
                                        break;
                                }
                                if ((float)(fabs(w[nm])+anorm) == anorm) break;
                        }
                        if (flag) {
                                c=0.0;
                                s=1.0;
                                for (i=l;i<=k;i++) {
                                        f=s*rv1[i];
                                        rv1[i]=c*rv1[i];
                                        if ((float)(fabs(f)+anorm) == anorm) break;
                                        g=w[i];
                                        h=pythag(f,g);
                                        w[i]=h;
                                        h=1.0/h;
                                        c=g*h;
                                        s = -f*h;
                                        for (j=1;j<=m;j++) {
                                                y=a[j][nm];
                                                z=a[j][i];
                                                a[j][nm]=y*c+z*s;
                                                a[j][i]=z*c-y*s;
                                        }
                                }
                        }
                        z=w[k];
                        if (l == k) {
                                if (z < 0.0) {
                                        w[k] = -z;
                                        for (j=1;j<=n;j++) v[j][k] = -v[j][k];
                                }
                                break;
                        }
                        if (its == 30) ospnrerror("no convergence in 30 svdcmp iterations");
                        x=w[l];
                        nm=k-1;
                        y=w[nm];
                        g=rv1[nm];
                        h=rv1[k];
                        f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
                        g=pythag(f,1.0);
                        f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
                        c=s=1.0;
                        for (j=l;j<=nm;j++) {
                                i=j+1;
                                g=rv1[i];
                                y=w[i];
                                h=s*g;
                                g=c*g;
                                z=pythag(f,h);
                                rv1[j]=z;
                                c=f/z;
                                s=h/z;
                                f=x*c+g*s;
                                g = g*c-x*s;
                                h=y*s;
                                y *= c;
                                for (jj=1;jj<=n;jj++) {
                                        x=v[jj][j];
                                        z=v[jj][i];
                                        v[jj][j]=x*c+z*s;
                                        v[jj][i]=z*c-x*s;
                                }
                                z=pythag(f,h);
                                w[j]=z;
                                if (z) {
                                        z=1.0/z;
                                        c=f*z;
                                        s=h*z;
                                }
                                f=c*g+s*y;
                                x=c*y-s*g;
                                for (jj=1;jj<=m;jj++) {
                                        y=a[jj][j];
                                        z=a[jj][i];
                                        a[jj][j]=y*c+z*s;
                                        a[jj][i]=z*c-y*s;
                                }
                        }
                        rv1[l]=0.0;
                        rv1[k]=f;
                        w[k]=x;
                }
        }
        freeVector(rv1,1,n);
}

float SQR(float a)
{
    float sqrarg = a;
    return(sqrarg == 0.0 ? 0.0 : sqrarg * sqrarg);
}

int IMIN(int a, int b)
{
    int iminarg1 = a;
    int iminarg2 = b;
    return(iminarg1 < iminarg2 ? iminarg1 : iminarg2);
}

float FMAX(float a, float b)
{
    float fmaxarg1 = a;
    float fmaxarg2 = b;
    return(fmaxarg1 > fmaxarg2 ? fmaxarg1 : fmaxarg2);
}





#undef NRANSI





