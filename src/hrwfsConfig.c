static char rcsid[]="$Id: hrwfsConfig.c,v 1.3 2001-10-26 03:28:09 cboyer Exp $";
/*
*   FILENAME
*   hrwfsConfig.c
*
*   FUNCTION NAME(S)
*   hrwfsCarCombine  - combine configC, agCommSentC and activeC CAR records
*   hrwfsConfigBegin - routine to execute at start of configuration checking
*   hrwfsConfigEnd   - routine to execute at end of configuration checking
*   hrwfsInterlocked - check if HRWFS is interlocked
*   hrwfsMechCad     - simple CAD routine for passing parameters to A&G
*
*/
/* *INDENT-OFF* */
/*
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/06/24 18:43:01  gemvx
 * Add standard CAD routine and extend CAR combination to include more CAR records
 *
 * Revision 1.1  2001/06/11 08:11:00  cjm
 * Source code for hrwfs configuration checking
 *
 */
/* *INDENT-ON* */

#include <string.h>
#include <cad.h>
#include <menuCarstates.h>
#include <genSubRecord.h>
#include <cadRecord.h>

static int hrwfsInterlocked (char *message) ;

/*+
 *   Function name:
 *   hrwfsCarCombine
 *
 *   Purpose:
 *   Combines the configC, agCommSentC and activeC CARs to generate applyC
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
 *   c => CAR value of activeC in A&G
 *   d => Message field of activeC in A&G
 *   e => CAR value of agCommSentC
 *   f => Message field of agCommSentC
 *   g => CAR value of configC
 *   h => Message field of configC
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
  char mess[MAX_STRING_SIZE] ;

/* Make output IDLE by default */
  outval = menuCarstatesIDLE ;
  strcpy(mess, "" ) ;

/* Check for BUSY first */
  if ( *(long *)pgsub->a == menuCarstatesBUSY ||
       *(long *)pgsub->c == menuCarstatesBUSY ||
       *(long *)pgsub->e == menuCarstatesBUSY ||
       *(long *)pgsub->g == menuCarstatesBUSY ) {
    outval = menuCarstatesBUSY ;
  } else if (*(long *)pgsub->a == menuCarstatesERROR ||
             *(long *)pgsub->c == menuCarstatesERROR ||
             *(long *)pgsub->e == menuCarstatesERROR ||
             *(long *)pgsub->g == menuCarstatesERROR) {
    outval = menuCarstatesERROR ;

/* Copy the error message to the output. If there is more than one error
*  message then the last will get output
*/
    if (*(long *)pgsub->a == menuCarstatesERROR)  
      strcpy(pgsub->vala, pgsub->b) ;
    if (*(long *)pgsub->c == menuCarstatesERROR)  
      strcpy(pgsub->vala, pgsub->d) ;
    if (*(long *)pgsub->e == menuCarstatesERROR)  
      strcpy(pgsub->vala, pgsub->f) ;
    if (*(long *)pgsub->g == menuCarstatesERROR)  
      strcpy(pgsub->vala, pgsub->h) ;
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
 *   the OCS or by an engineering screen. It is tied to a cad record
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
 *      (!)    pcad     (struct cadRecord *)  Pointer to cad structure
 *
 *   Function value:
 *   (<)  status  (long)  Return status, 0 = OK
 *
 *-
 */

long hrwfsConfigBegin(struct cadRecord *pcad )
{
    long status;                      /* return status */

/* Check for interlocks */

    status = CAD_ACCEPT ;

    if (hrwfsInterlocked(pcad->mess))
        status = CAD_REJECT;
    else {
        switch (pcad->dir) {
        case menuDirectivePRESET:

            break;

        case menuDirectiveSTART:
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
 *   the OCS or by an engineering screen. It is tied to a cad record
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
 *      (!)    pcad     (struct cadRecord *)  Pointer to cad structure
 *
 *   Function value:
 *   (<)  status  (long)  Return status, 0 = OK
 *
 *-
 */

long hrwfsConfigEnd(struct cadRecord *pcad)
{

    long status;             /* return status */

    status = CAD_ACCEPT;

    switch (pcad->dir) {

    case menuDirectivePRESET:

        break;

    case menuDirectiveSTART:

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

/*+
 *   Function name:
 *   hrwfsMechCad
 *
 *   Purpose:
 *   Implements CAD command from HRWFS to A&G 
 *
 *   Description:
 *    General puprose subroutine to implement an A&G HRWFS CAD being
 *    triggered from a HRWFS CAD. The input strings are simply copied
 *    to the output strings and a MARK & START directive is output to the
 *    subsystem CAD via STLK. This routine has been copied with minor 
 *    modifications from the TCS.
 *
 *   Invocation:
 *    hrwfsMechCad (pcad)
 *
 *   Parameters: (">" input, "!" modified, "<" output)
 *      (!)    pcad     (struct cadRecord *) Pointer to CAD record structure
 *
 *   Function value:
 *   (<)  status  (long) Return status, 0 = OK
 *
 *   Prior requirements:
 *   None
 *
 *-
 */

long hrwfsMechCad (struct cadRecord *pcad)
{
   int status;

/* Check for any interlocks */

  if (pcad->dir != menuDirectiveCLEAR)
    if (hrwfsInterlocked (pcad->mess)) return CAD_REJECT;

  status = CAD_REJECT ;

  switch (pcad->dir)
  {

   case menuDirectivePRESET :

   status = CAD_ACCEPT;
   break ;

   case menuDirectiveSTART :

/* Copy over the parameters for the A&G CAD */

    strncpy(pcad->vala, pcad->a, MAX_STRING_SIZE) ;
    strncpy(pcad->valb, pcad->b, MAX_STRING_SIZE) ;
    strncpy(pcad->valc, pcad->c, MAX_STRING_SIZE) ;
    strncpy(pcad->vald, pcad->d, MAX_STRING_SIZE) ;
    strncpy(pcad->vale, pcad->e, MAX_STRING_SIZE) ;
    strncpy(pcad->valf, pcad->f, MAX_STRING_SIZE) ;
    status = CAD_ACCEPT;
   break ;

   case menuDirectiveMARK :
   status = CAD_ACCEPT;
   break ;

   case menuDirectiveSTOP :
   status = CAD_ACCEPT;
   break ;

   case menuDirectiveCLEAR :
   status = CAD_ACCEPT;
   break ;

  }

  return(status) ;

}


