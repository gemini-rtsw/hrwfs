/*+
 *	MODULE NAME:
 *	wfsWcs
 *
 *	FILENAME:
 *	wfsWcs.h
 *
 *	PURPOSE:
 *	Include file for wfsWcs
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.3  1998/12/11 09:21:41  cics
 * Extra error checking
 *
 * Revision 1.2  1998/12/07 11:17:32  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.1  1998/11/30 15:54:48  cics
 * Modifications made during SMB visit to Hilo, November 1998
 *
 *INDENT-ON*
 *-
 */

#ifndef	__INCwfsWcsh
#define	__INCwfsWcsh


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#endif	/* vxWorks */

#include "gemTypes.h"
#include "gemModNum.h"


/* function declarations */

IMPORT void	wfsSetTrackFrame (FRAMETYPE frame, char type, double year, double wavelength,
								double RA, double Dec, char epochType, double epochYear);
IMPORT void	wfsGetTrackFrame (FRAMETYPE * pFrame, char * pType, double * pYear, double * pWavelength,
								double * pRA, double * pDec, char * pEpochType, double * pEpochYear);

#endif /* __INCwfsWcsh */
