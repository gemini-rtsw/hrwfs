static struct {void *v; char *c;} rcsid = {&rcsid,
   "$Id: cicsCarHealth.c,v 1.3 2000-03-13 20:46:39 cboyer Exp $"};

/*+
 *   MODULE NAME:
 *   cicsCarHealth
 *
 *   FILENAME:
 *   cicsCarHealth.c
 *
 *   PURPOSE:
 *   CICS miscellaneous genSub function library
 *
 *   DESCRIPTION:
 *   This file contains the source for all the functions used to
 *   combine together multiple CAR events, multiple health
 *   values or other such things through "genSub" records.
 *
 *   FUNCTION NAME(S):
 *   cicsCarValCombine  - Combine together multiple CAR states and client IDs.
 *   cicsHealthCombine  - Combine together multiple health values.
 *   cicsStringAppend   - Append two string inputs together
 *   cicsStringFilter   - Pass a string as long as it differs from a given tag
 *   cicsStateCombine   - Combine together multiple state values.
 *
 *   EXTERNAL MODULES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   Each of these functions assumes that it has been specified in the "SNAM" 
 *   field of an EPICS genSub record, and the function is executed when the 
 *   genSub record is processed. It is assumed that the caller handles all 
 *   the genSub record processing and manages the data structure pointed to 
 *   by "pgensub".
 *
 *   INCLUDE FILES:
 *   dbDefs.h       - EPICS database definition constants
 *   recSup.h       - EPICS record support constants
 *   dbCommon.h     - Data structure and definitions common to all EPICS records
 *   genSubRecord.h - EPICS genSub record data structure and definitions
 *   car.h          - EPICS CAR record data structure and definitions
 *
 *   AUTHOR:
 *   Steven Beard  (smb@roe.ac.uk)
 *
 *   HISTORY:
 *   24-Jan-1997: Original version based on tcsCarCombine.         (smb)
 *   18-Jul-1997: cicsStringAppend added.                          (smb)
 *   18-Aug-1997: cicsStringFilter added.                          (smb)
 *   16-Jan-1988: Modified to look more like VxWorks functions and
 *                to fit in with the AGWPS system. OK/ERROR used
 *                instead of PASS/FAIL. Function types are STATUS
 *                rather than "long". Checked in to CVS.           (smb)
 *   19-Jan-1998: Module added to CVS repository.                  (smb)
 *   24-Mar-1998: All occurences of strcpy replaced by strncpy,
 *                since strcpy can sometimes lead to memory
 *                corruption.                                      (smb)
 *   28-Feb-2000: Add cicsStateCombine() routine needed by the seq (cb)
 *-
 */
/* *INDENT-OFF* */
/*
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2000/01/05 20:09:28  cboyer
 * Tidy up the directory src: remove all the not used files and tidy up
 *
 * Revision 1.1.1.1  1999/03/17 03:14:22  cboyer
 * Initial creation of the Gemini HRWFS repository
 *
 * Revision 1.5  1998/04/29 13:12:46  smb
 * Restructures to act as a Gemini coding standards example
 *
 * Revision 1.4  1998/03/24 16:14:48  smb
 * strcpy replaced by strncpy
 *
 * Revision 1.3  1998/01/30 15:30:19  smb
 * Fixed some problems uncovered by prolint
 *
 * Revision 1.2  1998/01/27 16:43:21  smb
 * DEBUG statements added
 *
 * Revision 1.1  1998/01/19 15:42:30  smb
 * Added to repository
 *
 */
/* *INDENT-ON* */

#include  <vxWorks.h>
#include  <stdioLib.h>
#include  <string.h>

#include  <dbDefs.h>
#include  <genSubRecord.h>
#include  <car.h>
#include  <dbCommon.h>
#include  <recSup.h>

/* #define DEBUG */      /* Define this to switch on debugging messages */


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   cicsCarValCombine
 *
 *   PURPOSE:
 *   Combine together multiple CAR states
 *
 *   DESCRIPTION:
 *   This function reads the states of up to five CAR records
 *   and generates an event based on those states.
 *
 *   - An IDLE event is generated when all CAR records are idle.
 *   - A PAUSED event is generated when one of the CAR records is PAUSED
 *     and all the others are IDLE.
 *   - A BUSY event is generated when any of the CAR records is BUSY
 *     but none are in the ERROR state.
 *   - An ERROR event is generated when any of the CAR records are in
 *     the ERROR state (regardless of the other states).
 *
 *   The client ID associated with the winning CAR record is passed to
 *   the output.
 *
 *   The function is designed to be used with the EPICS "genSub" record.
 *
 *   INVOCATION:
 *   status = cicsCarValCombine( struct genSubRecord *pgensub );
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)  pgensub->a     (long)     Event delivered to CAR record 1
 *   (>)  pgensub->b     (long)     Client ID of CAR record 1
 *   (>)  pgensub->c     (long)     Event delivered to CAR record 2
 *   (>)  pgensub->d     (long)     Client ID of CAR record 2
 *   (>)  pgensub->e     (long)     Event delivered to CAR record 3
 *   (>)  pgensub->f     (long)     Client ID of CAR record 3
 *   (>)  pgensub->g     (long)     Event delivered to CAR record 4
 *   (>)  pgensub->h     (long)     Client ID of CAR record 4
 *   (>)  pgensub->i     (long)     Event delivered to CAR record 5
 *   (>)  pgensub->j     (long)     Client ID of CAR record 5
 *
 *   (<)  pgensub->vala  (long)     Resulting CAR event
 *   (<)  pgensub->valb  (long)     Winning client ID
 *   (<)  pgensub->valc  (long)     Index of winning CAR event (1-5)
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the function failed
 *
 *   EXTERNAL FUNCTIONS:
 *   None
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   This function works best if the inputs come from CAR events and not
 *   CAR states. This means connecting the A, C, E, G and I inputs of the
 *   genSub record to the .IVAL fields of the individual CAR records
 *   and not to the .VAL fields. Client ID comes from the .OCID fields.
 *
 *   DEFICIENCIES:
 *   This function does not deal with CAR error status values and messages.
 *   However, the index output to VALC can be used to extract any error
 *   status and message from the winning CAR record by using a "selection"
 *   record.
 *
 *   BUGS:
 *   None known
 *
 *   AUTHOR:
 *   Steven Beard  (smb@roe.ac.uk)
 *
 *   HISTORY:
 *   24-Jan-1997: Original version, based on tcsCarValCombine,
 *                bit with client ID included.                     (smb)
 *   25-Jan-1997: Generate index of winning CAR event.             (smb)
 *-
 */

STATUS cicsCarValCombine( struct genSubRecord *pgensub ) 
{
    STATUS  status;         /* return status */
    long    outVal;         /* Output CAR event value */
    long    outClid;        /* Output client ID */
    long    outIndex;       /* Index of winning CAR event */

#ifdef DEBUG
    printf( "cicsCarValCombine: %d-%d %d-%d %d-%d %d-%d %d-%d : ",
            *(long *)pgensub->a, *(long *)pgensub->b,
            *(long *)pgensub->c, *(long *)pgensub->d,
            *(long *)pgensub->e, *(long *)pgensub->f,
            *(long *)pgensub->g, *(long *)pgensub->h,
            *(long *)pgensub->i, *(long *)pgensub->j );
#endif

/* Initialise the status */

    status = OK;

/*
 * The default output value is IDLE and the default client ID is the maximum
 * of the input client IDs. The default index is whichever of the input CAR
 * CAR records has the largest client ID.
 */

    outVal = CAR_IDLE;

    outClid = *(long *)pgensub->b;
    outIndex = 1;

    if ( *(long *)pgensub->d > outClid )
    {
        outClid = *(long *)pgensub->d;
        outIndex = 2;
    }

    if ( *(long *)pgensub->f > outClid )
    {
        outClid = *(long *)pgensub->f;
        outIndex = 3;
    }

    if ( *(long *)pgensub->h > outClid )
    {
        outClid = *(long *)pgensub->h;
        outIndex = 4;
    }

    if ( *(long *)pgensub->j > outClid )
    {
        outClid = *(long *)pgensub->j;
        outIndex = 5;
    }

/*
 * If any CAR is PAUSED this will override the IDLE state and result in a
 * PAUSED output value.
 */

    if ( *(long *)pgensub->a == CAR_PAUSED )
    {
        outVal = CAR_PAUSED;
        outClid = *(long *)pgensub->b;
        outIndex = 1;
    }

    if ( *(long *)pgensub->c == CAR_PAUSED )
    {
        outVal = CAR_PAUSED;
        outClid = *(long *)pgensub->d;
        outIndex = 2;
    }

    if ( *(long *)pgensub->e == CAR_PAUSED )
    {
        outVal = CAR_PAUSED;
        outClid = *(long *)pgensub->f;
        outIndex = 3;
    }

    if ( *(long *)pgensub->g == CAR_PAUSED )
    {
        outVal = CAR_PAUSED;
        outClid = *(long *)pgensub->h;
        outIndex = 4;
    }

    if ( *(long *)pgensub->i == CAR_PAUSED )
    {
        outVal = CAR_PAUSED;
        outClid = *(long *)pgensub->j;
        outIndex = 5;
    }

/*
 * If any CAR is BUSY this will override the IDLE or PAUSED states and result
 * in a BUSY output value.
 */

    if ( *(long *)pgensub->a == CAR_BUSY )
    {
        outVal = CAR_BUSY;
        outClid = *(long *)pgensub->b;
        outIndex = 1;
    }

    if ( *(long *)pgensub->c == CAR_BUSY )
    {
        outVal = CAR_BUSY;
        outClid = *(long *)pgensub->d;
        outIndex = 2;
    }

    if ( *(long *)pgensub->e == CAR_BUSY )
    {
        outVal = CAR_BUSY;
        outClid = *(long *)pgensub->f;
        outIndex = 3;
    }

    if ( *(long *)pgensub->g == CAR_BUSY )
    {
        outVal = CAR_BUSY;
        outClid = *(long *)pgensub->h;
        outIndex = 4;
    }

    if ( *(long *)pgensub->i == CAR_BUSY )
    {
        outVal = CAR_BUSY;
        outClid = *(long *)pgensub->j;
        outIndex = 5;
    }

/*
 * If any CAR is in the ERROR state this will override the IDLE, PAUSED
 * or BUSY states and result in an ERROR output value.
 */

    if ( *(long *)pgensub->a == CAR_ERROR )
    {
        outVal = CAR_ERROR;
        outClid = *(long *)pgensub->b;
        outIndex = 1;
    }

    if ( *(long *)pgensub->c == CAR_ERROR )
    {
        outVal = CAR_ERROR;
        outClid = *(long *)pgensub->d;
        outIndex = 2;
    }

    if ( *(long *)pgensub->e == CAR_ERROR )
    {
        outVal = CAR_ERROR;
        outClid = *(long *)pgensub->f;
        outIndex = 3;
    }

    if ( *(long *)pgensub->g == CAR_ERROR )
    {
        outVal = CAR_ERROR;
        outClid = *(long *)pgensub->h;
        outIndex = 4;
    }

    if ( *(long *)pgensub->i == CAR_ERROR )
    {
        outVal = CAR_ERROR;
        outClid = *(long *)pgensub->j;
        outIndex = 5;
    }

/* Output the resulting value, client ID and index. */

    *(long *)pgensub->vala = outVal;
    *(long *)pgensub->valb = outClid;
    *(long *)pgensub->valc = outIndex;

#ifdef DEBUG
    printf( "outVal=%d, outClid=%d, outIndex=%d\n", outVal, outClid, outIndex );
#endif

    return (status);
}


/* ---------------------------------------------------------------------------*/

/*+
 *   FUNCTION NAME:
 *   cicsHealthCombine
 *
 *   PURPOSE:
 *   Combine together multiple health states
 *
 *   DESCRIPTION:
 *   This function reads the states of up to five health records
 *   and generates a combined health.
 *
 *   - The result will be GOOD if none of the inputs are WARNING or BAD.
 *   - The result will be WARNING if any of the inputs are WARNING
 *     but none of the inputs are BAD.
 *   - The result will be BAD if any of the inputs are BAD.
 *
 *   The message contained in the winning health record is passed to
 *   the output.
 *
 *   The function is designed to be used with the EPICS "genSub" record.
 *
 *   INVOCATION:
 *   status = cicsHealthCombine( struct genSubRecord *pgensub );
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pgensub->a    (char *)   Value of health record 1
 *   (>) pgensub->b    (char *)   Message contained in health record 1
 *   (>) pgensub->c    (char *)   Value of health record 2
 *   (>) pgensub->d    (char *)   Message contained in health record 2
 *   (>) pgensub->e    (char *)   Value of health record 3
 *   (>) pgensub->f    (char *)   Message contained in health record 3
 *   (>) pgensub->g    (char *)   Value of health record 4
 *   (>) pgensub->h    (char *)   Message contained in health record 4
 *   (>) pgensub->i    (char *)   Value of health record 5
 *   (>) pgensub->j    (char *)   Message contained in health record 5
 *
 *   (<) pgensub->vala (char *)   Resulting health
 *   (<) pgensub->valb (char *)   Winning message
 *   (<) pgensub->valc (long)     Index of winning health input
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the function failed
 *
 *   EXTERNAL FUNCTIONS:
 *   None
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   It is assumed that the value and message inputs to the genSub record are
 *   connected respectively to the .VAL and .OMSS fields of the the SIR records
 *   containing the health values to be combined.
 *
 *   DEFICIENCIES:
 *   If any or all the input health values are silly the result is still
 *   GOOD. Only the strings "WARNING" and "BAD" are recognised.
 *
 *   I expect this function will be very inefficient, since it deals with
 *   lots of character strings. It would be much more efficient to combine
 *   health values as integers by taking the maximum. It should be ok if
 *   the function is executed only when the health changes and is not
 *   executed continuously.
 *
 *   BUGS:
 *   None known
 *
 *   AUTHOR:
 *   Steven Beard  (smb@roe.ac.uk)
 *
 *   HISTORY:
 *   24-Jan-1997: Original version.                                (smb)
 *   25-Jan-1997: Generate index of winning health input.          (smb)
 *-
 */

STATUS cicsHealthCombine( struct genSubRecord *pgensub ) 
{
    STATUS  status;                          /* return status */
    char    outHealth[MAX_STRING_SIZE + 1];  /* Output health value */
    char    inMess[MAX_STRING_SIZE + 1];     /* Input message */
    char    outMess[MAX_STRING_SIZE + 1];    /* Output message */
    long    outIndex;                        /* Index of winning health input */

#ifdef DEBUG
    printf( "cicsHealthCombine: %s %s %s %s %s : ",
            (char *)pgensub->a,
            (char *)pgensub->c,
            (char *)pgensub->e,
            (char *)pgensub->g,
            (char *)pgensub->i );
#endif

/* Initialise the status */

    status = OK;

/*
 * The default output value is GOOD and the default message is the first
 * non-null and non-blank message (or a null message if there are no non-null
 * and non-blank messages). The default index corresponds to the message
 * chosen (or is 1 if there are no messages found).
 */

    strncpy( outHealth, "GOOD", MAX_STRING_SIZE );
    strncpy( outMess, "", MAX_STRING_SIZE );
    outIndex = 1;

    strncpy( inMess, (char *)pgensub->j, MAX_STRING_SIZE );
    if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
    {
        strncpy( outMess, inMess, MAX_STRING_SIZE );
        outIndex = 5;
    }

    strncpy( inMess, (char *)pgensub->h, MAX_STRING_SIZE );
    if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
    {
        strncpy( outMess, inMess, MAX_STRING_SIZE );
        outIndex = 4;
    }

    strncpy( inMess, (char *)pgensub->f, MAX_STRING_SIZE );
    if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
    {
        strncpy( outMess, inMess, MAX_STRING_SIZE );
        outIndex = 3;
    }

    strncpy( inMess, (char *)pgensub->d, MAX_STRING_SIZE );
    if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
    {
        strncpy( outMess, inMess, MAX_STRING_SIZE );
        outIndex = 2;
    }

    strncpy( inMess, (char *)pgensub->b, MAX_STRING_SIZE );
    if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
    {
        strncpy( outMess, inMess, MAX_STRING_SIZE );
        outIndex = 1;
      }

/*
 * If any health value is WARNING this will override the GOOD value and result 
 * in a WARNING output value. If the message associated with this WARNING
 * value is non-null and non-blank it will override the default message.
 */

    if ( (strcmp( (char *)pgensub->a, "WARNING") == 0) ||
         (strcmp( (char *)pgensub->a, "warning") == 0) )
    {
        strncpy( outHealth, "WARNING", MAX_STRING_SIZE );
        outIndex = 1;

        strncpy( inMess, (char *)pgensub->b, MAX_STRING_SIZE );
        if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
        {
            strncpy( outMess, inMess, MAX_STRING_SIZE );
        }
    }

    if ( (strcmp( (char *)pgensub->c, "WARNING") == 0) ||
         (strcmp( (char *)pgensub->c, "warning") == 0) )
    {
        strncpy( outHealth, "WARNING", MAX_STRING_SIZE );
        outIndex = 2;

        strncpy( inMess, (char *)pgensub->d, MAX_STRING_SIZE );
        if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
        {
            strncpy( outMess, inMess, MAX_STRING_SIZE );
        }
    }

    if ( (strcmp( (char *)pgensub->e, "WARNING") == 0) ||
         (strcmp( (char *)pgensub->e, "warning") == 0) )
    {
        strncpy( outHealth, "WARNING", MAX_STRING_SIZE );
        outIndex = 3;

        strncpy( inMess, (char *)pgensub->f, MAX_STRING_SIZE );
        if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
        {
            strncpy( outMess, inMess, MAX_STRING_SIZE );
        }
    }

    if ( (strcmp( (char *)pgensub->g, "WARNING") == 0) ||
         (strcmp( (char *)pgensub->g, "warning") == 0) )
    {
        strncpy( outHealth, "WARNING", MAX_STRING_SIZE );
        outIndex = 4;

        strncpy( inMess, (char *)pgensub->h, MAX_STRING_SIZE );
        if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
        {
            strncpy( outMess, inMess, MAX_STRING_SIZE );
        }
    }

    if ( (strcmp( (char *)pgensub->i, "WARNING") == 0) ||
         (strcmp( (char *)pgensub->i, "warning") == 0) )
    {
        strncpy( outHealth, "WARNING", MAX_STRING_SIZE );
        outIndex = 5;

        strncpy( inMess, (char *)pgensub->j, MAX_STRING_SIZE );
        if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
        {
            strncpy( outMess, inMess, MAX_STRING_SIZE );
        }
    }

/*
 * If any health value is BAD this will override any GOOD or WARNING values
 * and result in a BAD output value. If the message associated with this BAD
 * value is non-null and non-blank it will override the default message.
 */

    if ( (strcmp( (char *)pgensub->a, "BAD") == 0) ||
         (strcmp( (char *)pgensub->a, "bad") == 0) )
    {
        strncpy( outHealth, "BAD", MAX_STRING_SIZE );
        outIndex = 1;

        strncpy( inMess, (char *)pgensub->b, MAX_STRING_SIZE );
        if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
        {
            strncpy( outMess, inMess, MAX_STRING_SIZE );
        }
    }

    if ( (strcmp( (char *)pgensub->c, "BAD") == 0) ||
         (strcmp( (char *)pgensub->c, "bad") == 0) )
    {
        strncpy( outHealth, "BAD", MAX_STRING_SIZE );
        outIndex = 2;

        strncpy( inMess, (char *)pgensub->d, MAX_STRING_SIZE );
        if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
        {
            strncpy( outMess, inMess, MAX_STRING_SIZE );
        }
    }

    if ( (strcmp( (char *)pgensub->e, "BAD") == 0) ||
         (strcmp( (char *)pgensub->e, "bad") == 0) )
    {
        strncpy( outHealth, "BAD", MAX_STRING_SIZE );
        outIndex = 3;

        strncpy( inMess, (char *)pgensub->f, MAX_STRING_SIZE );
        if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
        {
            strncpy( outMess, inMess, MAX_STRING_SIZE );
        }
    }

    if ( (strcmp( (char *)pgensub->g, "BAD") == 0) ||
         (strcmp( (char *)pgensub->g, "bad") == 0) )
    {
        strncpy( outHealth, "BAD", MAX_STRING_SIZE );
        outIndex = 4;

        strncpy( inMess, (char *)pgensub->h, MAX_STRING_SIZE );
        if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
        {
            strncpy( outMess, inMess, MAX_STRING_SIZE );
        }
    }

    if ( (strcmp( (char *)pgensub->i, "BAD") == 0) ||
         (strcmp( (char *)pgensub->i, "bad") == 0) )
    {
        strncpy( outHealth, "BAD", MAX_STRING_SIZE );
        outIndex = 5;

        strncpy( inMess, (char *)pgensub->j, MAX_STRING_SIZE );
        if ( (strlen(inMess) != 0) && (strspn(inMess," ") != strlen(inMess)) )
        {
            strncpy( outMess, inMess, MAX_STRING_SIZE );
        }
    }

/* Output the resulting health value, message and index. */

    strncpy( (char *)pgensub->vala, outHealth, MAX_STRING_SIZE );
    strncpy( (char *)pgensub->valb, outMess, MAX_STRING_SIZE );
    *(long *)pgensub->valc = outIndex;

#ifdef DEBUG
    printf( "outHealth=%s, outMess=\"%s\", outIndex=%d\n", outHealth, outMess,
            outIndex );
#endif

    return (status);
}


/* ---------------------------------------------------------------------------*/

/*+
 *   FUNCTION NAME:
 *   cicsStringAppend
 *
 *   PURPOSE:
 *   Append two strings together
 *
 *   DESCRIPTION:
 *   This function appends two input strings together to make one
 *   output string.
 *
 *   The function is designed to be used with the EPICS "genSub" record.
 *
 *   INVOCATION:
 *   status = cicsStringAppend( struct genSubRecord *pgensub );
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pgensub->a    (char *)  Input string 1
 *   (>) pgensub->b    (char *)  Input string 2
 *
 *   (<) pgensub->vala (char *)  Resulting output string
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the function failed
 *
 *   EXTERNAL FUNCTIONS:
 *   None
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   DEFICIENCIES:
 *   None known
 *
 *   BUGS:
 *   None known
 *
 *   AUTHOR:
 *   Steven Beard  (smb@roe.ac.uk)
 *
 *   HISTORY:
 *   18-Jul-1997: Original version.                                (smb)
 *-
 */

STATUS cicsStringAppend( struct genSubRecord *pgensub ) 
{
    STATUS  status;                             /* return status */
    char    string[(MAX_STRING_SIZE+1) * 2];    /* Resulting string */

/* Initialise the status */

    status = OK;

/* Build up the output string */

    sprintf( string, "%.*s + %.*s", MAX_STRING_SIZE, (char *)pgensub->a,
             MAX_STRING_SIZE, (char *)pgensub->b );

/*
 * Copy as much of the string as possible to the output, truncating it
 * if it is longer than MAX_STRING_SIZE.
 */

    strncpy( (char *)pgensub->vala, string, MAX_STRING_SIZE );

    return (status);
}


/* ---------------------------------------------------------------------------*/

/*+
 *   FUNCTION NAME:
 *   cicsStringFilter
 *
 *   PURPOSE:
 *   Filter out any string matching a given tag
 *
 *   DESCRIPTION:
 *   This function passes a string from input A to output A, but only if
 *   it does not match a tag provided in input B.
 *
 *   The function is designed to be used with the EPICS "genSub" record.
 *
 *   INVOCATION:
 *   status = cicsStringFilter( struct genSubRecord *pgensub );
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pgensub->a    (char *)  Input string
 *   (>) pgensub->b    (char *)  Tag to be filtered out
 *
 *   (<) pgensub->vala (char *)  Output string
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the function failed
 *
 *   EXTERNAL FUNCTIONS:
 *   None
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   DEFICIENCIES:
 *   None known
 *
 *   BUGS:
 *   None known
 *
 *   AUTHOR:
 *   Steven Beard  (smb@roe.ac.uk)
 *
 *   HISTORY:
 *   18-Aug-1997: Original version.                                (smb)
 *-
 */

STATUS cicsStringFilter( struct genSubRecord *pgensub ) 
{
    STATUS status;                         /* return status */

/* Initialise the status */

    status = OK;

/*
 * Test the string against the tag provided, and only copy it to the output
 * if the string is non blank and does not match the tag.
 */

    if ( (strlen((char *)pgensub->a) != 0) &&
         (strspn((char *)pgensub->a," ") != strlen((char *)pgensub->a)) )
    {

       if ( strncmp((char *)pgensub->a, (char *)pgensub->b, MAX_STRING_SIZE) 
            != 0 )
       {

/* Match with tag not found. Copy the input to the output. */

          strncpy( (char *)pgensub->vala, (char *)pgensub->a, MAX_STRING_SIZE );
       }
    }

    return (status);
}

/* -------------------------------------------------------------------------- */

/*+
 *   Function name:
 *   cicsStateCombine
 *
 *   Purpose:
 *   Generate a state value from the states of the CC and DC
 *
 *   Description:
 *   This routine generates an overall "state" value from the combined
 *   states of the Components Controller and Detector Controller for hrwfs.
 *   The input states are expressed as strings and the output state by
 *   a number (see ICD 7b). As far as the output state is concerned,
 *   the input states "CONFIGURING" are treated as being equivalent to
 *   "RUNNING". 
 *
 *   Invocation:
 *   cicsStateCombine (pgensub)
 *
 *   Parameters: (">" input, "!" modified, "<" output)  
 *      (!)   pgensub  (struct genSubRecord *pgsub)  Pointer to gensub structure

 *
 *   Epics inputs:
 *   a => Input state of CC
 *   b => Input state of DC 
 *   c => Input state of sequencer 
 *
 *   Epics outputs:
 *   vala => Overall state of hrwfs seq as string
 *   valb => Overall state of hrwfs seq as a number
 *
 *   Function value:
 *   (<)  status  (long)  Return status, 0 = OK
 * 
 *-
 */

long cicsStateCombine (struct genSubRecord *pgensub) 
{
     int ccState ;
     int dcState ;
     int seqState ;
     int overallState ;
     int status ;

     char ccStateStr[MAX_STRING_SIZE] ;
     char dcStateStr[MAX_STRING_SIZE] ;
     char seqStateStr[MAX_STRING_SIZE] ;
     char *states[] = {"BOOTING", "INITIALISING", "RUNNING"} ;

     /* Some init */

     status = OK ;

     /* Read input state strings for CC and DC */

     strncpy ( ccStateStr, (char *)pgensub->a, MAX_STRING_SIZE ) ;
     strncpy ( dcStateStr, (char *)pgensub->b, MAX_STRING_SIZE ) ;
     strncpy ( seqStateStr, (char *)pgensub->c, MAX_STRING_SIZE ) ;

     /* Convert CC string state to a number */

     ccState = 0 ;      /* Assume it is BOOTING by default */

     if (!strncmp (ccStateStr, "INITIALISING", 12))
        ccState = 1 ;

     if (!strncmp (ccStateStr, "RUNNING", 7) ||
         !strncmp (ccStateStr, "CONFIGURING", 11))
        ccState = 2 ;

     /* Convert DC string state to a number */

     dcState = 0 ;      /* Assume it is BOOTING by default */

     if (!strncmp (dcStateStr, "INITIALISING", 12))
        dcState = 1 ;

     if (!strncmp (dcStateStr, "RUNNING", 7) ||
         !strncmp (dcStateStr, "CONFIGURING", 11))
        dcState = 2 ;

     /* Convert seq string state to a number */

     seqState = 0 ;      /* Assume it is BOOTING by default */

     if (!strncmp (seqStateStr, "INITIALISING", 12))
        seqState = 1 ;

     if (!strncmp (seqStateStr, "RUNNING", 7) ||
         !strncmp (seqStateStr, "CONFIGURING", 11))
        seqState = 2 ;

     /* Set overall state to the lowest individual states */

     overallState = 2 ;

     if ( seqState < overallState ) overallState = seqState ;
     if ( ccState < overallState ) overallState = ccState ;
     if ( dcState < overallState ) overallState = dcState ;

     strcpy ((char *)pgensub->vala, states[overallState]) ;
     *(long *)pgensub->valb = (long)overallState ;

#ifdef DEBUG
     printf ( "cicsStateCombine: dcState=%d, ccState=%d, seqState=%d, overall=%d\n",
              dcState, ccState, seqState, overallState ) ;
     printf ( "states[%d]=%s\n" , overallState , states[overallState]) ;
#endif

     return (OK);
}

