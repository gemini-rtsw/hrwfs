static struct {void *v; char *c;} rcsid = {&rcsid,
	"$Id: cicsLib.c,v 1.1.1.1 1999-03-17 03:14:21 cboyer Exp $"};
/*
*   FILENAME
*   -------- 
*   cicsLib.c
*
*   PURPOSE
*   -------
*   This file contains the source for all the library functions used by the
*   Core Instrument Control System
*
*
*   FUNCTION NAME(S)
*   ----------------
*   cicsDbGet         - Get a value from an EPICS field using database access
*   cicsDbPut         - Put a value to an EPICS field using database access
*
*   DEPENDENCIES
*   ------------
*
*   LIMITATIONS
*   ------------
*
*   AUTHOR
*   ------
*   Steven Beard  (smb@roe.ac.uk)
*   Janet Tvedt   (tvedt@noao.edu)
*
*   HISTORY
*   -------
*   04-Dec-1996: Original version with just cicsLogMessage,
*                cicsLogLong and cicsLogString, based on Michelle
*                logging library (which did not conform to the
*                Gemini SPS).                                        (smb)
*   16-Dec-1996: cicsLogFloat added.                                 (smb)
*   10-Jan-1997: cicsLogDouble added.                                (smb)
*   15-Jan-1997: A means of setting and testing the debug level
*                added. Fixed some syntax and formatting problems
*                noticed by Janet Tvedt.                             (smb)
*   16-Jan-1997: Functions renamed to conform to new Gemini SPS.     (smb)
*   23-Jan-1997: Removed the "static" storage class specifier on
*                the various message string buffers. This was
*                causing messages generated simultaneously by
*                different parallel processes to be overwritten.
*                Use MAX_STRING_SIZE instead of hard-wired size for
*                log message.                                        (smb)
*   27-Jan-1997: Modified cicsLogMessage to use a ring buffer of
*                messages to get around a deficiency of logMsg().    (smb)
*   04-Jun-1997: Size of message buffer increased from 40 to 64.     (smb)
*   17-Jun-1997: Modified to use a semaphore to ensure that two
*                tasks do not attempt to use the same element of
*                the message buffer at the same time.                (smb)
*   17-Jun-1997: Set top level prefixes. Janet Tvedt's database
*                access functions added.                             (smb,tvedt)
*   26-Jun-1997: Channel Access get and put functions added, but
*                then transferred to "cicsLib2.c" because it was
*                not possible to include "dbAccess.h" and
*                "cadefs.h" in the same file..                       (smb)
*   12-Aug-1997: Added check functions used by CAD functions.        (smb)
*   06-Jul-1998: Database access functions extracted and modified
*                for use with AGWPS.                                 (smb)
*/
/* *INDENT-OFF* */
/*
 * $Log: not supported by cvs2svn $
 * Revision 1.3  1998/10/14 09:30:56  cics
 * Andy Foster's comments included
 *
 * Revision 1.2  1998/10/01 14:30:43  cics
 * rcsid initialisation changed to prevent compiler warning
 *
 * Revision 1.1  1998/07/09 15:30:22  smb
 * Added to repository
 *
 */
/* *INDENT-ON* */


/* Global Constants */

#include  <vxWorks.h>
#include  <types.h>
#include  <math.h>
#include  <time.h>
#include  <stdlib.h>
#include  <stdioLib.h>
#include  <string.h>

#include  <dbDefs.h>
#include  <dbCommon.h>
#include  <recSup.h>
#include  <dbAccess.h>

#include  <cicsLib.h>



/* ===================================================================== */

/*
 *+
 * FUNCTION NAME:
 * cicsDbGet
 *
 * INVOCATION:
 * STATUS status;
 * char *fieldName;  
 * char *errMess;
 * unsigned short type;
 * double *outVal;
 *
 * status = cicsDbGet(fieldName, errMess, type, &outVal);
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > fieldName (char *)               pointer to record_name.field_name
 * ! errMess   (char *)               pointer to string
 * > type      (unsigned short)       data type to convert output to 
 *                                    (DBF_DOUBLE, DBF_LONG, etc.)
 * < outval    (void *)               pointer to output value
 *
 * FUNCTION VALUE:
 * long  - status value returned to calling routine, a non-zero value
 *         indicates an error
 *
 * PURPOSE:
 * Function to get a value from an EPICS field by database access.
 *
 * DESCRIPTION:
 * This routine is called to obtain the value of the specified field 
 * within an EPICS database record.  It handles potential errors by
 * sending messages to the CICS logging functions and returning an
 * error emssage and status value to the calling routine.
 *
 * EXTERNAL VARIABLES:
 * dbNameToAddr    - EPICS database access routine for finding record address
 * dbGetField      - EPICS database access routine for retrieving the data
 *
 * PRIOR REQUIREMENTS:
 * None.
 * 
 * DEFICIENCIES:
 * I am informed (by Bret Goodrich) that the routine "recGblGetLinkValue" is
 * preferred to "dbGetField" by the EPICS community (because it can choose whether
 * to use Channel Access or Database Access according to circumstances),
 * but "recGblGetLinkValue" is much more complicated, and I don't understand
 * its description in the "EPICS IOC Application Developers Guide".
 *
 * I am now informed (by Andy Foster several months later) that "recGblGetLinkValue"
 * is only usuable as part of EPICS record support, and that the Channel Access
 * function "ca_array_get" is the advertised public interface to EPICS.
 *
 * HISTORY (optional):
 * 13-Mar-1997  Original version as getDbInfo.		Janet Tvedt
 * 17-Jun-1997  Imported into cicsLib.				Steven Beard
 * 06-Jul-1998: CICS logging functions removed.		Steven Beard
 *-
 */

long cicsDbGet(char *fieldName, char *errMess, unsigned short type, void *outVal)
{
	struct dbAddr addr;
	long ret, options=0L, nRq = 1L;
	STATUS status;
    
	status = OK;

	/* Get the address of the data structure  and handle any errors */

	if( (ret = dbNameToAddr (fieldName,&addr)) != 0L)
	{
		status = ERROR;
		sprintf(errMess, "dbNameToAddr error = %ld", ret);
	}

	/* If address found, get the data.  Handle any errors. */

	if( status == OK )
	{
		if( (ret = dbGetField(&addr, type, outVal, &options, &nRq, NULL)) != 0L)
		{
			status = ERROR;
			sprintf(errMess, "dbGet error = %ld", ret);
		}
	}

	/* Return error status */
	return status;
}


/* ===================================================================== */

/*
 *+
 * FUNCTION NAME:
 * cicsDbPut
 *
 * INVOCATION:
 * char *fieldName;
 * char *errMess;
 * unsigned short type;
 * double outVal;
 * STATUS status;
 *
 * status = cicsDbPut(fieldName, errMess, DBF_DOUBLE, &outVal)
 *
 * PARAMETERS: (">" input, "!" modified, "<" output)
 * > fieldName (char *)           pointer to record+field name
 * ! errMess   (char *)           pointer to string
 * > type      (unsigned short)   data type of value to put
 * > outVal    (void *)           pointer of data to put
 *
 * FUNCTION VALUE:
 * long  Status value returned to calling routine, a non-zero value indicates
 *       an error
 *
 * PURPOSE:
 * Function to put a value into an EPICS field by database access
 *
 * DESCRIPTION:
 * This routine may be called by a user subroutine to put a value
 * into an EPICS databaser record field.  The complete field name must
 * be specified (for example: sytem:subsystem:recordx.FIELDY).  This function
 * will also handle errors by logging them through the CICS logging functions
 * and by returning an error message and status value to the calling routine.
 *
 * EXTERNAL VARIABLES:
 * dbNameToAddr    - EPICS database access routine for finding record address
 * dbPutField      - EPICS database access routine for putting the data
 *
 * PRIOR REQUIREMENTS:
 * None
 *
 * DEFICIENCIES:
 * I am informed that the routine "recGblPutLinkValue" is preferred
 * to "dbGetField" by the EPICS community (because it can choose whether
 * to use Channel Access or Database Access according to circumstances),
 * but "recGblGetLinkValue" is much more complicated, and I don't understand
 * its description in the "EPICS IOC Application Developers Guide".
 *
 * I am now informed (by Andy Foster several months later) that "recGblPutLinkValue"
 * is only usuable as part of EPICS record support, and that the Channel Access
 * function "ca_array_put" is the advertised public interface to EPICS.
 *
 * HISTORY (optional):
 * 19-Mar-1997  Original version as putDbInfo.			Janet Tvedt
 * 17-Jun-1997  Imported into cicsLib					Steven Beard
 * 06-Jul-1998: CICS logging functions removed.			Steven Beard
 *-
 */


long cicsDbPut(char *fieldName, char *errMess, unsigned short type, void *outVal)
{
	struct dbAddr addr;
	long ret, nRq = 1L;
	STATUS status;
       
	status = OK;

	/* Get the address of the data structure  and handle any errors */

	if( (ret = dbNameToAddr (fieldName,&addr)) != 0L)
	{
		status = ERROR;
		sprintf(errMess, "dbNameToAddr error = %ld", ret);

	}

	/* If address found, write the data.  Handle any errors. */

	if( status == OK )
	{
		if( (ret = dbPutField(&addr, type, outVal, nRq)) != 0L)
		{
			status = ERROR;
			sprintf(errMess, "dbPutField error = %ld", ret);
		}
	}

	/* Return error status */
	return status;
}
