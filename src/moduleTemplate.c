/******************************************************************************
* N.B. These comments down to but excluding the line                         *
*                                                                            *
* static struct {void *v; char *c;} rcsid = {&rcsid, "$Id: moduleTemplate.c,v 1.1.1.1 1999-03-17 03:14:23 cboyer Exp $"};                *
*                                                                            *
* must be deleted after this template file is used to generate a new source  *
* file.                                                                      *
*                                                                            *
* 1. If this file is used as a template for a state notation program i.e.    *
*    one ending with the file extension .st or .stpp, the line declaring     *
*    rcsid MUST be moved to after the snc program statement and then         *
*    escaped using the %% construct.                                         *
*    Failure to do this will cause the snc compiler to fail.                 *
*                                                                            *
******************************************************************************/

static struct {void *v; char *c;} rcsid = {&rcsid,
	"$Id: moduleTemplate.c,v 1.1.1.1 1999-03-17 03:14:23 cboyer Exp $"};

/*+
 *	MODULE NAME:
 *	Put module name here
 *
 *	FILENAME:
 *	Put file name here
 *
 *	PURPOSE:
 *	Brief description of the purpose of the module
 *
 *	DESCRIPTION:
 *	Introductory paragraph explaining what the module does
 *
 *	FUNCTION NAME(S):
 *	The name of each function in the module goes here along with a single line
 *	comment on what it does e.g.
 *	function1 - intialises the other functions, must be called first.
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.4  1998/12/07 11:17:21  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.3  1998/02/23 10:08:13  smb
 * Updated templates
 *
 * Revision 1.2  1998/01/16 15:55:31  smb
 * Quell compiler warning about rcsid using anj's idea
 *
 * Revision 1.1.1.1  1997/11/28 11:46:17  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */


