/*+
 * MODULE NAME:
 * dbTypes.h
 *
 * FILENAME:
 * dbTypes.h
 *
 * DESCRIPTION:
 * Defines database type constants and structures. It is expected this file
 * might one day be replaced by something supplied by Gemini
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/03/17 03:14:26  cboyer
 * Initial creation of the Gemini HRWFS repository
 *
 * Revision 1.8  1998/12/07 11:17:13  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.7  1998/09/28 08:50:20  cics
 * Give warning if an attempt if made to compile this file for anything other 
 * than vxWorks
 *
 * Revision 1.6  1998/05/29 13:50:19  smb
 * Ability to transmit data updates added
 *
 * Revision 1.5  1998/05/13 08:42:24  smb
 * pWfsName added to genSub data structure
 *
 * Revision 1.4  1998/05/11 13:48:51  smb
 * genSub record type added
 *
 * Revision 1.3  1998/03/04 17:12:13  smb
 * Comment dates made more international
 *
 * Revision 1.2  1998/02/05 15:18:35  smb
 * Added more comments and UNKNOWN_RECORD_TYPE
 *
 * Revision 1.1.1.1  1997/11/28 11:46:18  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */


/* includes */

#ifndef   __INCdbTypesh
#define   __INCdbTypesh

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */


#include "gemTypes.h"


/* defines */

   /*
    * These codes are used to initialise the data structures describing
    * the properties of each type of EPICS record (for example the commands
    * and attributes associated with each CAD record).
    */

#define NO_LO_LIMIT                 "<<<No Low Limit On Attribute>>>"
#define NO_HI_LIMIT                 "<<<No High Limit On Attribute>>>"
#define NO_ATTRIBUTE_LIMITS         0
#define STOP_DIRECTIVE_SUPPORTED    TRUE
#define STOP_DIRECTIVE_UNSUPPORTED  FALSE
#define SIMULATION_MODE_SUPPORTED   TRUE
#define SIMULATION_MODE_UNSUPPORTED FALSE
#define CAD_MAX_N_ATTRIB_IN_RANGE   100     /* Max size of list of allowable  */
                                            /* CAD attributes                 */
#define CAD_MAX_N_ATTRIB            20      /* Max no. of CAD attributes      */
#define GSUB_MAX_INPUT_VALUES       24      /* Max no. of genSub input values */
#define GSUB_MAX_OUTPUT_VALUES      40      /* Max no. of genSub output values*/
#define GSUB_INPUT                  TRUE
#define GSUB_OUTPUT                 FALSE
#define NO_TIMEOUT                  1000000.0   /* Arbitrarily long time (sec)*/
#define NO_TASK_NAME                "<no task>" /* Constant used to declare   */
                                                /* that a record is not       */
                                                /* associated with a task     */


/* macro definitions */

   /*
    * RECORD_NAME: Construct a record name by prefixing the given string
    * with TOP (which is defined in wfsDb.h).
    */

#define   RECORD_NAME(name)            (TOP ## name)

   /*
    * TASK_NAME: Construct a full task name by concatenating the subsystem
    * name and task name, separateed by a ":".
    */

#define   TASK_NAME(subSystem, taskName)   (subSystem ## ":" ## taskName)

   /*
    * ATTRIB: ATTRIB(s) expands the macro 's' (assumed to have been
    * been defined via: '#define s something') to "something",
    * effectively converting it into a quoted string.
    */

#define   ATTRIB(s)                 (STRNG(s))
#define   STRNG(s)                  (#s)


/* data structures */

typedef union                     /* Attribute value union.                   */
   {
      char *   pStringAttrib;     /* Attribute value regarded as a string.    */
      long     longAttrib;        /* Attribute value regarded as a long.      */
      double   doubleAttrib;      /* Attribute value regarded as a double.    */
   } CAD_ATTRIB_VALUE;

typedef struct                    /* CAD attribute definition structure.      */
   {
      uint32   number;            /* Attribute number.                        */
      uint32   type;              /* Attribute type (string, long or double). */
      char *   pDefault;          /* Default value (encoded as a string).     */
      char *   pAllowedRange [CAD_MAX_N_ATTRIB_IN_RANGE + 1];
                                  /* Allowed range, which can be a single     */
                                  /* flag, two limit values, or a whole list  */
                                  /* of allowed values.                       */
   } CAD_ATTRIB;

typedef struct                    /* CAD record definition structure.         */
   {
      char *   pRecordName;       /* CAD record name.                         */
      char *   pTaskName;         /* Name of task which receives commands     */
                                  /* when this record is activated.           */
      int      commandNumber;
                                  /* Number of command activated by this      */
                                  /* CAD record.                              */
      BOOL     stopDirSupported;
                                  /* Is the STOP directive supported?         */
      BOOL     simulationSupported;
                                  /* Is simulation is supported?              */
      double   timeoutSecs;       /* Command timeout in seconds.              */
      CAD_ATTRIB    pAttrib [CAD_MAX_N_ATTRIB + 1];
                                  /* Array of CAD attribute structures        */
                                  /* (defined above).                         */
                                  /* (Add 1 because attribute numbers start   */
                                  /* at 1 and not 0. SMB - 3 Feb 98).         */
   } CAD_RECORD;

typedef struct                    /* genSub record definition structure.      */
   {
      char *   pRecordName;       /* genSub record name.                      */
      char *   pTaskName;         /* Name of task associated with this record */
      char *   pWfsName;          /* Name of WFS associated with this record  */
      BOOL     inputRecord;       /* Flag indicating whether this is an input */
                                  /* record (TRUE) or output record (FALSE).  */
      int      updateNumber;      /* If this is an input genSub record, the   */
                                  /* ID number of the data update activated.  */
      double   timeoutSecs;       /* Data update timeout in seconds.          */
      int      nValues;           /* Number of values.                        */
   } GSUB_RECORD;

typedef struct                    /* CAR record definition structure.         */
   {
      char *   pRecordName;       /* CAR record name.                         */
      char *   pTaskName;         /* Name of task whose state this CAR record */
                                  /* describes.                               */
   } CAR_RECORD;

typedef struct                    /* SIR record definition structure.         */
   {
      char *   pRecordName;       /* SIR record name.                         */
      uint32   type;              /* Data type (string, long or double).      */
      double   hysteresis;        /* Hysteresis value. (The record will only  */
                                  /* be updated when the value changes by     */
                                  /* more than this amount).                  */
   } SIR_RECORD;

enum
   {
      CAD_RECORD_TYPE = 0,        /* EPICS record type codes.                 */
      GENSUB_RECORD_TYPE,
      CAR_RECORD_TYPE,
      SIR_RECORD_TYPE,
      /* Insert additional EPICS record types here. */
      N_RECORD_TYPES,             /* This is automatically set to the total   */
                                  /* number of recognised record types.       */
      UNKNOWN_RECORD_TYPE
   };

enum
   {
   EPICS_DATA_TYPE_STRING = 0,    /* EPICS data type codes.                   */
   EPICS_DATA_TYPE_LONG,
   EPICS_DATA_TYPE_DOUBLE,
   EPICS_DATA_TYPE_UNDEFINED
   };

enum
   {
   CAD_ATTRIB_A = 1,              /* Numerical codes for CAD attributes.      */
   CAD_ATTRIB_B,
   CAD_ATTRIB_C,
   CAD_ATTRIB_D,
   CAD_ATTRIB_E,
   CAD_ATTRIB_F,
   CAD_ATTRIB_G,
   CAD_ATTRIB_H,
   CAD_ATTRIB_I,
   CAD_ATTRIB_J,
   CAD_ATTRIB_K,
   CAD_ATTRIB_L,
   CAD_ATTRIB_M,
   CAD_ATTRIB_N,
   CAD_ATTRIB_O,
   CAD_ATTRIB_P,
   CAD_ATTRIB_Q,
   CAD_ATTRIB_R,
   CAD_ATTRIB_S,
   CAD_ATTRIB_T
   };

#endif   /*   ifndef __INCdbTypesh */
