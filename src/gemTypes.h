/*+
 *   MODULE NAME:
 *   gemTypes
 *
 *   FILENAME:
 *   gemTypes.h
 *
 *   PURPOSE:
 *   Defines Gemini data types. It is expected this file might
 *   one day be replaced by something supplied by Gemini.
 *
 *   DEFICIENCIES:
 *   The typedefs in this module assume that the compiler will
 *   assign 8 bits to an object of type "char", 16 bits to an
 *   object of type "short" and 32 bits to an object of type
 *   "int". They will need to change if a new compiler/hardware
 *   combination makes different allocations.
 *
 *   MODIFICATIONS
 *   - 10 dec 1999 - cb tidy up
 *   - A lot of smb modifications before UK delivery (see CVS for previous
 *     versions)
 *   - Creation 28 Nov 1997 by UK
 *-
 */


/* includes */

#ifndef __INCgemTypesh
#define __INCgemTypesh

#ifdef   __cplusplus
extern "C" {
#endif      /* __cplusplus */

#ifndef NO_EPICS
#include "epicsTypes.h"
#include "dbDefs.h"
#endif   /* NO_EPICS */

/* macro definitions */

   /*
    * COMPILE_DATE_AND_TIME: This macro translates to a string
    * containing the date and time of compilation.
    */

#define   COMPILE_DATE_AND_TIME         (__DATE__ " @ " __TIME__)


/* defines */

#define TARGET_TYPE_MV167      0          /* Code for MV167 target type.      */
#define TARGET_TYPE_HKBAJA47   1          /* Code for HKBAJA target type.     */
#define TARGET_TYPE_MV2700     2          /* Code for HKBAJA target type.     */


#ifndef NO_EPICS                          /* Use EPICS constants.             */

#define EPICS_MAX_BYTES_STRING_ATTRIB MAX_STRING_SIZE
                                          /* defined in "epicsTypes.h".       */
#define EPICS_MAX_BYTES_FIELD_NAME FLDNAME_SZ
                                          /* defined in "dbDefs.h".           */
#define EPICS_MAX_BYTES_RECORD_NAME PVNAME_SZ
                                          /* defined in "dbDefs.h".           */

#else                                     /* Cannot use EPICS constants.      */

#define EPICS_MAX_BYTES_STRING_ATTRIB   40
#define EPICS_MAX_BYTES_FIELD_NAME      4
#define EPICS_MAX_BYTES_RECORD_NAME     28

#endif   /* NO_EPICS */


#define EPICS_MAX_NFIELD_PER_RECORD     4 /* Max. number of fields per record */
                                          /* the WFS software can deal with.  */

   /* The following constants define the message levels corresponding to
    * the various Gemini debugging levels.
    */

#define MSG_WARNING      0                /* Warning message.                 */
#define MSG_LOG          1                /* Log message.                     */
#define MSG_MINDEBUG     2                /* Minimum debugging message.       */
#define MSG_FULLDEBUG    3                /* Full debugging message.          */


/* typedefs */

typedef unsigned char    uint8;           /* 8 bit unsigned integer.          */
typedef unsigned short   uint16;          /* 16 bit unsigned integer.         */
typedef unsigned int     uint;            /* standard (32 bit?) unsigned int. */
typedef unsigned long    uint32;          /* 32 bit unsigned integer.         */
typedef long             int32;           /* 32 bit signed integer.           */
typedef volatile unsigned char  HW_REG8;  /* 8 bit hardware register.         */
typedef volatile unsigned short HW_REG16; /* 16 bit hardware register.        */
typedef volatile unsigned int   HW_REG;   /* standard (32 bit?) h/w register. */
typedef volatile unsigned long  HW_REG32; /* 32 bit hardware register.        */

#ifdef   __cplusplus
}
#endif   /* __cplusplus */

#endif   /* __INCgenTypesh */
