/*
*   FILENAME
*   --------
*   cicslib.h
*
*   PURPOSE
*   -------
*   This file defines the functions contained in the cicslib library.
*   It should be included by any routine wishing to use the library
*
*   REQUIREMENTS
*   ------------
*   If the definitions of the functions in cicsLib are changed, or if
*   more functions are added, this file must be kept up to date
*
*   LIMITATIONS
*   -----------
*   If this file is included in an EPICS state notation program, the state
*   notation compiler will issue a warning message claiming that the
*   functions are being used without being declared. This is a "feature"
*   of the compiler and can be ignored
*
*   AUTHOR
*   ------
*   Steven Beard  (smb@roe.ac.uk)
*
*   HISTORY
*   -------
*   04-Dec-1996: Original version.                           (smb)
*   16-Dec-1996: cicsLogFloat added.                         (smb)
*   10-Jan-1997: cicsLogDouble added.                        (smb)
*   14-Jan-1997: cicsSetDebug added.                         (smb)
*   16-Jan-1997: Header modified and Functions renamed to
*                conform to new SPS.                         (smb)
*   09-Jun-1997: More debug level constants added.           (smb)
*   17-Jun-1997: cicsInitLogging added.                      (smb)
*   26-Jun-1997: DB and CA functions added.                  (smb)
*   12-Aug-1997: Check functions added.                      (smb)
*	06-Jul-1998: Database access functions extracted.        (smb)
*/
/* *INDENT-OFF* */
/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1  1998/07/09 15:30:22  smb
 * Added to repository
 *
 */
/* *INDENT-ON* */

#ifndef CICSLIB
#define CICSLIB

/* Declare the constants used by functions in the cicslib library. */


/* Declare the functions in the cicslib library. */

long cicsDbGet( char *, char *, unsigned short, void * );
long cicsDbPut( char *, char *, unsigned short, void * );

#endif
