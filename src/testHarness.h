/*+
 *	MODULE NAME:
 *	testHarness
 *
 *	FILENAME:
 *	testHarness.h
 *
 *	PURPOSE:
 *	Include file for testHarness library
 *
 *	DESCRIPTION:
 *	This file contains the constants used by the test harness functions.
 *	At the moment it just contains function templates.
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.2  1998/12/07 11:17:25  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.1  1998/05/13 10:22:52  smb
 * Test harness functions added to repository
 *
 *
 *INDENT-ON*
 *-
 */

#ifndef	__INCtestHarnessh
#define	__INCtestHarnessh


/* includes */

#include  <dbDefs.h>
#include  <genSubRecord.h>
#include  <dbCommon.h>
#include  <recSup.h>

#include  "gemModNum.h"


/* defines */


	/* function declarations */

IMPORT STATUS	testInit( struct genSubRecord * pgensub );
IMPORT STATUS	testTtfZero( struct genSubRecord * pgensub );
IMPORT STATUS	testAoZero( struct genSubRecord * pgensub );
IMPORT STATUS	testProbeOffset( struct genSubRecord * pgensub );

#endif /* __INCtestHarnessh */
