/*
 * This dummy function is required by the VxWorks DHS library.
 * The library fails to load if this function is not accessible,
 * but I am assured that the function is not actually used.
 * Nevertheless, I have included some code just in case.
 *
 * Steven Beard (24 July 1998).
 */


#include "vxWorks.h"
#include "taskLib.h"
#include "sysLib.h"
#include "string.h"
#include "stdio.h"

int strcasecmp(
	const char *	pString1,
	const char *	pString2
)
{
	int		i;
	char	pCopy1 [256];
	char	pCopy2 [256];

	strncpy (pCopy1, pString1, 256);
	strncpy (pCopy2, pString2, 256);

	for (i = 0; i < strlen (pCopy1); i++) pCopy1 [i] &= ~0x20;
	for (i = 0; i < strlen (pCopy2); i++) pCopy2 [i] &= ~0x20;

	return (strcmp (pCopy1, pCopy2));
}
