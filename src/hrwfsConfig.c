static char rcsid[]="$Id: hrwfsConfig.c,v 1.1 2001-06-11 08:11:00 cjm Exp $";
/*
*   FILENAME
*   hrwfsConfig.c
*
*   FUNCTION NAME(S)
*   hrwfsCarCombine  - combine configC and activeC CAR records
*   hrwfsConfigBegin - routine to execute at start of configuration checking
*   hrwfsConfigEnd   - routine to execute at end of configuration checking
*   hrwfsInterlocked - check if HRWFS is interlocked
*
*/
/* *INDENT-OFF* */
/*
 * $Log: not supported by cvs2svn $
 */
/* *INDENT-ON* */

#include <string.h>
#include <subCadRecord.h>
#include <subcad.h>
#include <cad.h>
#include <car.h>
#include <genSubRecord.h>

static int hrwfsInterlocked (char *message) ;

/*+
 *   Function name:
 *   hrwfsCarCombine
 *
 *   Purpose:
 *   Combines the configC and activeC CAR records to generate applyC
 *
 *   Description:
 *   Simply pull in the values and messages (if any) and then set applyC
 *   to BUSY if any of the inputs are BUSY. Set applyC to ERROR if none
 *   of the inputs are BUSY but at least one is in ERROR. Otherwise set
 *   applyC to IDLE.
 *
 *   Invocation:
 *   hrwfsCarCombine(pgsub)
 *
 *   Parameters: (">" input, "!" modified, "<" output)  
 *      (!)  pgsub  (struct genSubRecord *)  Pointer to gensub structure
 *
 *   Function value:
 *   (<)  status  (long)  Return status, 0 = OK
 * 
 *   Epics inputs:
 *   a => CAR value of activeC
 *   b => Message field of activeC
 *   c => CAR value of configC
 *   d => Message field of configC
 *
 *   Epics outputs:
 *   vala => Input message for applyC
 *   valb => Input value for applyC
 *
 *-
 */

long hrwfsCarCombine(struct genSubRecord *pgsub)
{
  long outval ;

/* Make output IDLE by default */
  outval = CAR_IDLE ;

/* Check for BUSY first */
  if ( *(long *)pgsub->a == CAR_BUSY || *(long *)pgsub->c == CAR_BUSY) {
    outval = CAR_BUSY ;
  } else if (*(long *)pgsub->a == CAR_ERROR ||
             *(long *)pgsub->c == CAR_ERROR ) {
    outval = CAR_ERROR ;
  }

  *(long *)pgsub->valb = outval ;

  return 0 ;

}


/*+
 *   Function name:
 *   hrwfsConfigBegin
 *
 *   Purpose:
 *   Begin a new HRWFS configuration.
 *
 *   Description:
 *   This routine is called as soon as a new configuration is started by
 *   the OCS or by an engineering screen. It is tied to a subcad record
 *   which is in turn the first record triggered by the top level Apply
 *   record. It will get called the top level Apply both when preset and
 *   start are issued. Any code that needs to be executed before preset
 *   or start is issued to any commands should be executed from this
 *   routine.
 *
 *   Invocation:
 *   hrwfsConfigBegin (pcad)
 *
 *   Parameters: (">" input, "!" modified, "<" output)
 *      (!)    pcad     (struct subCadRecord *)  Pointer to subcad structure
 *
 *   Function value:
 *   (<)  status  (long)  Return status, 0 = OK
 *
 *-
 */

long hrwfsConfigBegin(struct subCadRecord *pcad )
{
    long status;                      /* return status */

/* Check for interlocks */

    status = CAD_ACCEPT ;

    if (hrwfsInterlocked(pcad->mess))
        status = CAD_REJECT;
    else {
        switch (pcad->dir) {
        case CAD_PRESET:

            break;

        case CAD_START:
            break;

        default:
            break;
        }
    }
    return status;

}

/*+
 *   Function name:
 *   hrwfsConfigEnd
 *
 *   Purpose:
 *   End a new HRWFS configuration and tidy up.
 *
 *   Description:
 *   This routine is called once a new configuration has been started by
 *   the OCS or by an engineering screen. It is tied to a subcad record
 *   which is in turn the last record triggered by the top level Apply
 *   record. It will get called by the top level Apply both when preset and
 *   start are issued. Any code that needs to be executed after preset
 *   or start is issued to any commands should be executed from this
 *   routine.
 *
 *   Invocation:
 *   hrwfsConfigEnd (pcad)
 *
 *   Parameters: (">" input, "!" modified, "<" output)
 *      (!)    pcad     (struct subCadRecord *)  Pointer to subcad structure
 *
 *   Function value:
 *   (<)  status  (long)  Return status, 0 = OK
 *
 *-
 */

long hrwfsConfigEnd(struct subCadRecord *pcad)
{

    long status;             /* return status */

    status = CAD_ACCEPT;

    switch (pcad->dir) {

    case CAD_PRESET:

        break;

    case CAD_START:

        break;

    default:
        break;

    }

    return status;

}


/*+
 *   Function name:
 *   hrwfsInterlocked
 *
 *   Purpose:
 *   Queries the interlock state of the HRWFS 
 *
 *   Description:
 *   Currently this is a dummy routine but it could check if the GIS had
 *   issued an interlock or if the HRWFS was not in a state to allow
 *   commands to be issued e.g. it hadn't reached the running state.
 *
 *   Invocation:
 *   interlock = hrwfsInterlocked(message)
 *
 *   Parameters: (">" input, "!" modified, "<" output)
 *      (<)    mess     (char * )  message string
 *
 *   Function value:
 *   (<)  interlock  (int)  0 = no interlock, 1 = interlock in place
 *
 *-
 */

int hrwfsInterlocked (char *message)

{

  strcpy (message, " ") ;
  return 0 ;


}


