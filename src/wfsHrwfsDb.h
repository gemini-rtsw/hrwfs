/*+
 *	MODULE NAME:
 *	wfsHrwfsDb
 *
 *	FILENAME:
 *	wfsHrwfsDb.h
 *
 *	PURPOSE:
 *	Include file for wfsHrwfsDb.
 *
 *	DEFICIENCIES:
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.8  1998/12/07 11:17:52  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.7  1998/10/12 14:12:03  cics
 * Define __INCwfsHrwfsDbh
 *
 * Revision 1.6  1998/09/28 08:56:28  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks. detGeometry command changed.
 *
 * Revision 1.5  1998/09/09 14:35:36  cics
 * Global variables renamed to ensure they are unique
 *
 * Revision 1.4  1998/07/03 15:10:39  smb
 * Include deficiencies
 *
 * Revision 1.3  1998/05/13 10:36:30  smb
 * genSub records added
 *
 * Revision 1.2  1998/02/05 15:33:10  smb
 * Added observing and initialising records
 *
 * Revision 1.1.1.1  1997/11/28 11:46:17  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

#ifndef	__INCwfsHrwfsDbh
#define	__INCwfsHrwfsDbh

/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif	/* vxWorks */

#include "dbTypes.h"


/* defines */

#ifndef	TOP
#define	TOP	"hrwfs:"						/* Top level prefix for HRWFS EPICS record names.	*/
#endif	/* TOP */

	/*
	 * Declare the data structures to contain EPICS record information.
	 * The CAD_RECORD, CAR_RECORD and SIR_RECORD data types are declared
	 * in "dbTypes.h". The structures are initialised in wfsDb.c.
	 */

IMPORT char			pppWfsDbRecFieldName [N_RECORD_TYPES][EPICS_MAX_NFIELD_PER_RECORD][EPICS_MAX_BYTES_FIELD_NAME + 2];
												/* Recognised field names for each		*/
												/* type of record. (The "+2" in the		*/
												/* string length allows for the "."		*/
												/* and the null terminator).			*/

IMPORT CAD_RECORD	pWfsDbCadList [];			/* Array of CAD record structures.		*/
IMPORT GSUB_RECORD	pWfsDbGsubList [];			/* Array of genSub record structures.	*/
IMPORT CAR_RECORD	pWfsDbCarList [];			/* Array of CAR record structures.		*/
IMPORT SIR_RECORD	pWfsDbSirList [];			/* Array of SIR record structures.		*/

IMPORT int			wfsDbNCadRecord;			/* Total number of CAD records.			*/
IMPORT int			wfsDbNGsubRecord;			/* Total number of genSub records.		*/
IMPORT int			wfsDbNCarRecord;			/* Total number of CAR records.			*/
IMPORT int			wfsDbNSirRecord;			/* Total number of SIR records.			*/

IMPORT char 		pWfsDbRecNamePrefix [];		/* Record name prefix string.			*/
IMPORT BOOL			pWfsDbRecInitialised [];	/* Array of flags indicating when		*/
												/* the set of records of each type		*/
												/* have been initialised.				*/
IMPORT BOOL			wfsDbEpicsDbIsLocal;		/* Flag indicating whether the			*/
												/* EPICS database resides on the		*/
												/* local processor.						*/

#endif	/* ifndef __INCwfsHrwfsDbh */
