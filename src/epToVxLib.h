/*+
 * MODULE NAME:
 * epToVxLib
 *
 * FILENAME:
 * epToVxLib.h
 *
 * PURPOSE:
 * Include file for epToVxLib
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2001/10/26 03:28:09  cboyer
 * Port to epics3.13.4 version
 *
 * Revision 1.2  2000/01/05 20:09:43  cboyer
 * Tidy up the directory src: remove all the not used files and tidy up
 *
 * Revision 1.1.1.1  1999/03/17 03:14:26  cboyer
 * Initial creation of the Gemini HRWFS repository
 *
 * Revision 1.22  1998/12/07 11:17:15  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.21  1998/11/30 15:54:46  cics
 * Modifications made during SMB visit to Hilo, November 1998
 *
 * Revision 1.20  1998/10/15 10:42:09  cics
 * epToVxCadCopy added.
 *
 * Revision 1.19  1998/10/01 13:53:28  cics
 * Some unchanged variables changed to const
 *
 * Revision 1.18  1998/09/28 08:51:03  cics
 * Give warning if an attempt if made to compile this file for anything other 
 * than vxWorks
 *
 * Revision 1.17  1998/09/09 14:35:28  cics
 * Global variables renamed to ensure they are unique
 *
 * Revision 1.16  1998/07/13 15:24:11  smb
 * More bugs fixed. Channel access replaced by database access.
 *
 * Revision 1.15  1998/07/01 12:51:40  smb
 * Work around memory corruption by padding command and data packets. Added 
 * more debugging functions.
 *
 * Revision 1.14  1998/05/29 13:50:19  smb
 * Ability to transmit data updates added
 *
 * Revision 1.13  1998/05/13 13:37:07  smb
 * Macro EPTOVX_CAD_PIPE_FD removed. It was an unnecessary complication.
 *
 * Revision 1.12  1998/05/13 10:37:43  smb
 * Added genSub processing functions
 *
 * Revision 1.11  1998/03/27 12:03:51  smb
 * Fixed bug in file descriptor array size
 *
 * Revision 1.10  1998/03/24 16:15:56  smb
 * Simulation mode design flaw fixed
 *
 * Revision 1.9  1998/02/23 13:38:51  smb
 * Rearranged code for printability
 *
 * Revision 1.8  1998/02/18 11:15:22  smb
 * epToVxCadReject added
 *
 * Revision 1.7  1998/02/05 15:22:52  smb
 * CAD_DATA_TYPE -> EPICS_DATA_TYPE. Also fixed bugs in epToVxRecContextGet 
 * and epToVxPipeWrite
 *
 * Revision 1.6  1998/02/02 17:24:54  smb
 * Added epToVxCmdFree to free resources allocated by epToVxCmdInit
 *
 * Revision 1.5  1998/01/28 10:10:39  smb
 * Improved errors and messages. epToVxShow can deal with SIR records
 *
 * Revision 1.4  1998/01/19 16:18:19  smb
 * Update individual health records
 *
 * Revision 1.3  1997/12/15 16:49:39  smb
 * Fixed epToVxPipeWrite bug. Also increased cmd buffer size to 4
 *
 * Revision 1.2  1997/12/05 14:30:54  smb
 * Bug in epToVxPipeWrite noted but not fixed yet
 *
 * Revision 1.1.1.1  1997/11/28 11:46:17  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

#ifndef   __INCepToVxLibh
#define   __INCepToVxLibh

/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */

#include <symLib.h>
#include <time.h>
#include <string.h>
#include <symLib.h>
#include "gemTypes.h"
#include "dbTypes.h"
#include "gemModNum.h"

#ifndef NO_EPICS /* START OF INCLUDES COMPILED ONLY FOR THE EPICS ENVIRONMENT */

#include <cadef.h>
#include <cadRecord.h>
#include <genSubRecord.h>

#endif  /* NO_EPICS - END OF INCLUDES COMPILED ONLY FOR THE EPICS ENVIRONMENT */

/* defines */

#define EPTOVX_CAD_CAR_PIPES_NMSGS 8  /* Number of message slots in the pipes */
                                      /* used to transfer CAD command and     */
                                      /* response packets. These slots provide*/
                                      /* a FIFO buffer which should be enough */
                                      /* to prevent CAD commands being        */
                                      /* rejected because their associated    */
                                      /* command pipe is full.                */
                                      /* Generally, VxWorks control tasks     */                                      /* should be able to read CAD command   */
                                      /* packets very quickly so that         */
                                      /* only a small number of message slots */
                                      /* is needed.                           */

#define EPTOVX_GENSUB_PIPES_NMSGS 8   /* Number of message slots in the pipe  */
                                      /* used to transfer data update packets.*/
                                      /* These slots provide a FIFO buffer    */
                                      /* which should be large enough to      */
                                      /* prevent data updates from being      */
                                      /* rejected because the associated data */
                                      /* update pipe is full. More message    */
                                      /* slots are needed here because data   */
                                      /* update packets will be received more */
                                      /* frequently than commands.            */

   /*
    * A VxWorks control task generally calls epToVxCmdInit() in order to 
    * initialise its connections to the CAD records to which the task responds.
    * The following parameters define the timeout period for which 
    * epToVxCmdInit() will wait for this initialisation to complete. 
    * This enables control tasks to be spawned before epToVxLib has completed
    * all of its internal initialisation (in epToVxCadCarInit()).
    */

#define EPTOVX_TIMEOUT_INITIALISE 60.0
#define EPTOVX_DELAY_INITIALISE   0.1

   /*
    * This timeout determines how long epToVxCaInit() and epToVxCaGet() will 
    * wait for channel access transactions to complete.
    */

#define EPTOVX_TIMEOUT_CA         20.0

   /*
    * The following simulation modes are written into a CAD command packet as 
    * part of the "command-modifier" word.
    */

#define EPTOVX_SIM_MODE_VSM       0   /* Define bits in command modifier word.*/
#define EPTOVX_SIM_MODE_FAST      1
#define EPTOVX_SIM_MODE_FULL      2
#define EPTOVX_SIM_MODE_NONE      3
#define EPTOVX_SIM_MODE_MASK      3

#define EPTOVX_DEBUG_MODE_NONE    0   /* Define debug mode constants.         */
#define EPTOVX_DEBUG_MODE_MIN     1
#define EPTOVX_DEBUG_MODE_FULL    2

   /*
    * The following lines define the format of the command-modifier word 
    * contained in a command/response packet (see definitions in epToVxLib.c 
    * for details).
    */

#define CAD_DIRECTIVE_START       0    /* Bit mask for START directive        */
#define CAD_DIRECTIVE_STOP        4    /* Bit mask for STOP directive         */
#define CAD_DIRECTIVE_MASK        4    /* Mask extracts START/STOP   dir      */
#define CAD_COMMAND_MODE_BEGIN    0    /* Bit mask for BEGIN command          */
#define CAD_COMMAND_MODE_DONE     8    /* Bit mask for DONE command           */
#define CAD_COMMAND_MODE_MASK     8    /* Mask extracts BEGIN/DONE mode       */
#define CAD_COMMAND_MODIFIER_MASK 0x0f /* Mask extracts the command-          */

   /*
    * Now define some macros that may be used by application code. Specifically,
    * VxWorks control tasks - which receive CAD command packets, act on them 
    * and issue response packets - will normally use these macros.
    *
    * The macro EPTOVX_IS_SIMULATION() enables a VxWorks control task to 
    * determine whether a CAD command that has been received (via 
    * epToVxCmdRead()) was issued in a particular simulation mode. Similarly, 
    * the macro EPTOVX_IS_STOP_DIRECTIVE() tests whether the CAD directive 
    * that was issed was a START or a STOP. The argument (context) to both 
    * macros should be of type CAD_CMD_CONTEXT. These two macros operate on 
    * the command-modifier word contained in the command packet pointed to by 
    * context->pCmdPacket. The offset to this word should agree with that given 
    * in the macro CMD_PKT_COMMAND_MODIFIER() given in epToVxLib.c.
    *
    * EPTOVX_IS_VALID_CMD_NUM() tests whether a CAD command has a valid command 
    * number. Its arguments are a CAD command context and the command number 
    * (int).
    *
    * EPTOVX_CAD_ATTRIB_GET() is used to extract a CAD attribute from a command 
    * packet. The arguments are the command context structure, the command 
    * number, the attribute numer for the required attribute (e.g. attribute 
    * "a" = number 0, attribute "b" = number 1 etc) and, finally, a pointer 
    * to the location to which the attribute should be copied.
    */

#define EPTOVX_IS_SIMULATION(context, mode) (((* (uint32 *) (int) ((context)->pCmdPacket + 8)) & EPTOVX_SIM_MODE_MASK) == mode)

#define EPTOVX_IS_STOP_DIRECTIVE(context) (((* (uint32 *) (int) ((context)->pCmdPacket + 8)) & CAD_DIRECTIVE_MASK) == CAD_DIRECTIVE_STOP)

#define EPTOVX_IS_VALID_CMD_NUM(context, number) ((number) <= (context)->highestCmdNumber ? TRUE : FALSE)

#define EPTOVX_CAD_ATTRIB_GET(context, cmd, attrib, dest) (epToVxCmdAttribGet ((context)->ppCadContext [cmd], attrib, dest, (context)->pCmdPacket))

   /*
    * Error number codes used by epToVxLib.
    * These are designed to be processed using the vxWorks "makeStatTbl" utility
    */

#define S_epToVxLib_INVALID_RECORD_TYPE      (M_epToVxLib | 1)   
                                             /* Invalid record type.          */
#define S_epToVxLib_INVALID_CAD_STRUCTURE    (M_epToVxLib | 2)   
                                             /* Invalid CAD structure.        */
#define S_epToVxLib_RECORD_DEFINITION_ERROR  (M_epToVxLib | 3)   
                                             /* Record definition error.      */
#define S_epToVxLib_RECORD_UNINITIALISED     (M_epToVxLib | 4)   
                                             /* Record not initialised.       */
#define S_epToVxLib_NO_RECS_FOUND_FOR_TASK   (M_epToVxLib | 5)   
                                             /* Task has no records defined.  */
#define S_epToVxLib_REC_INIT_TIMEOUT         (M_epToVxLib | 6)   
                                             /* Timeout initialising record.  */
#define S_epToVxLib_TIMEOUT_WAITING_FOR_PIPE (M_epToVxLib | 7)   
                                             /* Timeout waiting for pipe.     */
#define S_epToVxLib_menuDirectiveSTOP_UNSUPPORTED     (M_epToVxLib | 8)   
                                             /* STOP directive not supported  */
#define S_epToVxLib_CAD_CMD_UNSUPPORTED      (M_epToVxLib | 9)   
                                             /* Command not supported.        */
#define S_epToVxLib_INVALID_CAD_DIRECTIVE    (M_epToVxLib | 10)   
                                             /* Invalid CAD directive.        */
#define S_epToVxLib_BAD_ATTRIBUTE            (M_epToVxLib | 11)   
                                             /* Attribute failed checks.      */
#define S_epToVxLib_CAD_CAR_SYNCH_ERROR      (M_epToVxLib | 12)   
                                             /* CAD CAR synch. error.         */
#define S_epToVxLib_CONTROL_TASK_ERROR       (M_epToVxLib | 13)   
                                             /* Error in control task.        */
#define S_epToVxLib_INTERNAL_ERROR           (M_epToVxLib | 14)   
                                             /* Internal coding error.        */
#define S_epToVxLib_INVALID_PACKET_READ      (M_epToVxLib | 15)   
                                             /* Invalid packet read.          */
#define S_epToVxLib_INVALID_COMMAND_NUM      (M_epToVxLib | 16)   
                                             /* Invalid command number.       */
#define S_epToVxLib_INVALID_PACKET_SIZE      (M_epToVxLib | 17)    
                                             /* Invalid packet size.          */

/* typedefs */

typedef struct                        /* CAD context structure                */
{
  CAD_RECORD *    pCadRecord;         /* Pointer to CAD's record-definition   */
                                      /* structure.                           */
  char *          pTaskName;          /* VxWorks control task that services   */
                                      /* this CAD.                            */
  struct timespec timeout;            /* Timeout period (sec) for execution of*/
                                      /* command.                             */
  int             cadToTaskPipeFd;    /* FD for CAD record to control task    */
                                      /* pipe.                                */
  int             cadToCarPipeFd;     /* FD for CAD record to CAR daemon pipe.*/
  char *          pCmdPacket;         /* Pointer to memory reserved for       */
                                      /* command packet.                      */
  size_t          maxSizeCmdPacket;   /* Maximum size of CAD's command packet.*/
  int             sizeOfCmdPacket;    /* Size of current command packet       */
                                      /* (depends on attributes).             */
  uint32          clientId;           /* Transaction id for each command      */
  uint32          nAttrib;            /* Number of attributes used in this CAD*/
  uint32 *        pType;              /* Array of types for each attribute    */
                                      /* (long, double or string).            */
  uint32 *        pNumberRangeValues; /* Array giving the number of attribute */
                                      /* range                                */
                                      /* Values defined for each of the       */
                                      /* nAttrib attributes.                  */
  CAD_ATTRIB_VALUE *  pDefault;       /* Array of default attribute values.   */
  CAD_ATTRIB_VALUE ** ppAllowedRange; /* Multi-dim. array of permitted        */
                                      /* attribute ranges.                    */
  BOOL            presetDone;         /* Set when the CAD has been            */
                                      /* successfully PRESET                  */
  BOOL            stopDirSupported;   /* Set if command supports the CAD STOP */
                                      /* directive.                           */
  BOOL            simulationSupported;/* Set if the command supports          */
                                      /* simulation mode(s).                  */
  char *          caContext;          /* Pointer to channel-access definition */
                                      /* structure, if any.                   */
                                      /* NOTE: This is defined as "CA_DEF"    */
                                      /* below. I expect Nick has used char * */
                                      /* to allow this bit to compile without */
                                      /* EPICS. SMB - 27 Jan 98.              */
} CAD_CONTEXT_STRUCT, * CAD_CONTEXT;

typedef struct                        /* CAD command structure                */
{
  CAD_CONTEXT *   ppCadContext;       /* Array of CAD context structures      */
                                      /* handled by control task.             */
  char *          pCmdPacket;         /* Pointer to memory reserved to read   */
                                      /* incoming command packets.            */
  int             maxSizeCmdPacket;   /* Size of the largest possible command */
                                      /* packet that can be read by control   */
                                      /* task.                                */
  int             cadPipeFd;          /* FD for CAD record(s) to control task */
                                      /* pipe.                                */
  int             carPipeFd;          /* FD for control task to CAR daemon    */
                                      /* pipe.                                */
  int             highestCmdNumber;   /* Highest command number to which      */
                                      /* control task will respond.           */
} CAD_CMD_CONTEXT_STRUCT, * CAD_CMD_CONTEXT;

typedef struct                        /* genSub context structure             */
{
  GSUB_RECORD *   pGsubRecord;        /* Pointer to genSub's record-definition*/
                                      /* structure.                           */
  char *          pTaskName;          /* VxWorks control task which services  */
                                      /* this genSub.                         */
  char *          pWfsName;           /* Wavefront sensor associated with this*/
                                      /* genSub.                              */
  BOOL            inputRecord;        /* TRUE if this genSub is used for input*/
  struct timespec timeout;            /* Timeout period (sec) for update of   */
                                      /* parameters.                          */
  int             gensubToTaskPipeFd; /* FD for genSub record to control task */
                                      /* pipe.                                */
  char *          pUpdatePacket;      /* Pointer to memory reserved for update*/
                                      /* packet.                              */
  int             sizeOfUpdatePacket; /* Size of the data update packet for   */
                                      /* this genSub                          */
  uint32          clientId;           /* Transaction id for each update       */
  uint32          nValues;            /* Number of values.                    */
  double *        pValues;            /* Pointer to array of values.          */
} GSUB_CONTEXT_STRUCT, * GSUB_CONTEXT;

typedef struct                        /* genSub data update structure         */
{
  GSUB_CONTEXT *  ppGsubContext;      /* Array of genSub context structures   */
                                      /* handled by control task.             */
  char *          pUpdatePacket;      /* Pointer to memory reserved to read   */
                                      /* incoming data update packets.        */
  int             maxSizeUpdatePacket;/* Size of the largest possible date    */
                                      /* update packet that can be read by    */
                                      /* control task.                        */
  int             gensubPipeFd;       /* FD for genSub record(s) to control   */
                                      /* task pipe.                           */
  int             highestUpdateNumber;/* Highest update number to which       */
                                      /* control task will respond.           */
} GSUB_DATA_CONTEXT_STRUCT, * GSUB_DATA_CONTEXT;

typedef struct                        /* Data-record context structure        */
{
  int             recordType;         /* Type of record (e.g. SIR_RECORD_TYPE)*/
  uint32          type;               /* Type of data to write (e.g. long,    */
                                      /* double, string etc...).              */
  uint32          nElement;           /* Number of data elements to transfer  */
  uint32          recordId;           /* ID number for data record.           */
  BOOL            filterEnable;       /* Set to enable data filtering.        */
  double          filterParam1;       /* Filter parameter 1                   */
                                      /* - add/delete further parameters when */
                                      /* filtering is implemented.            */
  char *          pDataPacket;        /* Pointer to memory reserved for data  */
                                      /* packet                               */
  char *          caContext;          /* Pointer to channel-access definition */
                                      /* structure                            */
                                      /* NOTE: This is defined as "CA_DEF"    */
                                      /* below.                               */
                                      /* I expect Nick has used "char *" to   */
                                      /* allow this bit to compile without    */
                                      /* EPICS. SMB - 27 Jan 98.              */
  double          hysteresisOnWrite;  /* Amount of hysteresis on record writes*/
                                      /* (numerical types only).              */
  double          lastWriteValue;     /* Last value written to record (used   */
                                      /* with hysteresis).                    */
  BOOL            firstWriteDone;     /* Set when record first written (used  */
                                      /* with hysteresis).                    */
} DATREC_CONTEXT_STRUCT, * DATREC_CONTEXT;

#ifndef NO_EPICS /* START OF TYPEDEFS COMPILED ONLY FOR THE EPICS ENVIRONMENT */

typedef struct                        /* Channel-access definition structure  */
{
  int             recordType;         /* Type of record                       */
                                      /* (e.g. [CAD|CAR|SIR]_RECORD_TYPE)     */
  int             nField;             /* Number of fields written for this    */
                                      /* record                               */
  chid *          pChannelId;         /* Array of field channel IDs.          */
  chtype *        pFieldType;         /* Array of field types.                */
  char **         ppFieldValue;       /* Array of field values.               */
} CA_DEF_STRUCT, * CA_DEF;

typedef struct                        /* CAR context structure                */
{
  char *          pTaskName;          /* name of control task which maintains */
                                      /* this CAR                             */
  CA_DEF          caContext;          /* channel-access definition structure  */
                                      /* for CAR                              */
} CAR_CONTEXT_STRUCT, * CAR_CONTEXT;

#endif /* NO_EPICS - END OF TYPEDEFS COMPILED ONLY FOR THE EPICS ENVIRONMENT */


/* function declarations */

IMPORT STATUS epToVxInit (void);
IMPORT STATUS epToVxDbInitCadCar (void);
IMPORT STATUS epToVxDbInitGensub (void);
IMPORT STATUS epToVxDbInitSir (void);
IMPORT STATUS epToVxPipeInit (const int procNumber);
IMPORT int    epToVxPipeOpen (const BOOL fullNameProvided, const char * pName,
                              const char * pNameExtension,
                              STATUS (* pipeCreate) (), const int nMsgSlots, 
                              const int maxMsgSize, const int mode,
                              const int ioctlFunction, double timeoutPeriod, 
                              double timeoutDelay);
IMPORT STATUS epToVxPipeWrite (char * RecordName, char * pValue,
                               DATREC_CONTEXT pContextKnown);
IMPORT STATUS epToVxSetHealth (const char * pRecordPrefix, char * pValue);
IMPORT CAD_CMD_CONTEXT epToVxCmdInit (const char * pTaskName, 
                                      STATUS (* pipeCreate) ());
IMPORT STATUS epToVxCmdFree (CAD_CMD_CONTEXT pCadCmdContext);
IMPORT int    epToVxCmdRead (CAD_CMD_CONTEXT pCadCmdContext);
IMPORT void   epToVxCmdAttribGet (CAD_CONTEXT pContext, uint32 attribNumber,
                                  char * pAttribDest, const char * pCmdPacket);
IMPORT STATUS epToVxCmdFinish (CAD_CMD_CONTEXT pCadCmdContext, 
                               uint32 errorNumber);
IMPORT STATUS epToVxRecContextGet (char * pRecordName, 
                                   DATREC_CONTEXT * pContext,
                                   int * pRecordType);
IMPORT GSUB_DATA_CONTEXT epToVxUpdateInit (const char * pWfsName, 
                                           char * pTaskName, 
                                           STATUS (* pipeCreate) ());
IMPORT int    epToVxUpdateRead (GSUB_DATA_CONTEXT pDataUpdateContext);
IMPORT STATUS epToVxShow (const char * pRecordName, const BOOL verbose);
IMPORT void   epToVxCadContextShow (CAD_CONTEXT pCadContext, 
                                    const BOOL verbose);
IMPORT void   epToVxSirContextShow (DATREC_CONTEXT pSirContext, 
                                    const BOOL verbose);
IMPORT void   epToVxGsubContextShow (GSUB_CONTEXT pGsubContext, 
                                     const BOOL verbose);

/* START OF FUNCTION DEFINITIONS COMPILED ONLY FOR THE EPICS ENVIRONMENT */
#ifndef NO_EPICS   

IMPORT void   epToVxCarContextShow (CAR_CONTEXT pCarContext, 
                                    const BOOL verbose);
IMPORT STATUS epToVxCaInit (int recordType, const char * pRecordName, 
                            CA_DEF pCaDef);
IMPORT void   epToVxCaShow (CA_DEF pCaDef, const BOOL verbose);
IMPORT void   epToVxChidShow (chid pChid);
IMPORT STATUS epToVxCaInitRecords (void);
IMPORT STATUS epToVxCaInitCar (void);
IMPORT STATUS epToVxCaInitSir (void);
IMPORT STATUS epToVxCaWrite (CA_DEF pContextKnown);
IMPORT STATUS epToVxCaRead (CA_DEF pContextKnown);
IMPORT STATUS epToVxCaWriteDaemon (const int nProcOnBus);
IMPORT STATUS epToVxCadInit (struct cadRecord * pcad);
IMPORT long   epToVxCadExecute (struct cadRecord * pcad);
IMPORT long   epToVxCadReject (struct cadRecord * pcad);
IMPORT long   epToVxCadCopy (struct cadRecord * pcad);
IMPORT void   epToVxSetCadSimMode (const int simMode);
IMPORT STATUS epToVxGensubInit (struct genSubRecord * pgensub);
IMPORT STATUS epToVxGensubInput (struct genSubRecord * pgensub);
IMPORT STATUS epToVxGensubOutput (struct genSubRecord * pgensub);
IMPORT STATUS epToVxCarDaemon (char * pRecordName);

#endif /* NO_EPICS - END OF FUNCTION DEFINITIONS COMPILED ONLY FOR THE EPICS ENVIRONMENT */

#endif   /*   ifndef __INCepToVxLibh */
