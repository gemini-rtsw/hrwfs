static struct {void *v; char *c;} rcsid = {&rcsid,
   "$Id: sdsuLib.c,v 1.4 2000-07-24 20:28:03 cboyer Exp $"};

/*+
 *   MODULE NAME:
 *   sdsuLib
 *
 *   FILENAME:
 *   sdsuLib.c
 *
 *   PURPOSE:
 *   SDSU Detector Controller Interface Library
 *
 *   DESCRIPTION:
 *   The SDSU detector controller VxWorks library, which provides an interface
 *   to the San Diego State University (SDSU) detector controller system.
 *
 *   This library provides functions for sending commands to the SDSU 
 *   controller, reading back the responses and data, defining various detector 
 *   control parameters, reading and writing DSP memory, downloading and 
 *   uploading configuration files, plus various diagnostic functions, as 
 *   listed below. Further details of the SDSU controller hardware, firmware 
 *   and DSP interface are contained in the "SDSU CCD Controller User's Manual",
 *   (Bob Leach and Frank Beale, SDSU, 1994). The low level SDSU detector
 *   controller interface used by the AGWPS is described in ICD 1.6/1.10.
 *
 *   Portability
 *   -----------
 *   The library has been written to be as portable as is reasonably possible.
 *   However, certain function calls make use of target-specific hardware
 *   resources which are not fully supported by the VxWorks BSP (e.g. some
 *   accesses to VME and on-board peripherals such as the high-resolution
 *   timers).
 *   It was originally claimed that portability had been maximised through the
 *   use of the extended system library, sysextLib (an extension to the mv167
 *   and hk4700 BSP sysLib libraries). However, sysextLib itself has proved to 
 *   be highly non-portable and more trouble than it is worth, and it can been
 *   removed from this library by defining the NO_SYSEXTLIB macro when 
 *   compiling it.
 *
 *   Shared Code and Re-Entrancy
 *   ---------------------------
 *   It is a requirement that a single processor can control an arbitrary number
 *   of SDSU controllers in the Gemini system. This requires that care be taken
 *   to ensure that the sdsuLib library can be shared by a number of tasks on
 *   the same processor, and that the code is re-entrant. Section 3.3.9 of the
 *   "VxWorks Programmer's Guide (version 5.2)" contains a general discussion
 *   on shared code and reentrancy in VxWorks.
 *
 *   In order to meet this requirement, each SDSU controller has an associated
 *   data structure used to store all static variables associated with the
 *   controller. The structure is called the "context" (typedef SDSU_CONTEXT).
 *   A single context structure is usually declared only once for each 
 *   controller and a handle to the structure may subsequently be used by any 
 *   tasks which make calls to sdsuLib.
 *
 *   It is also possible to declare multiple contexts for a single controller
 *   system, and thereby allow a number of independent application programs to
 *   share access to a single set of controller hardware. An example of the
 *   way in which multiple contexts might be required is to allow both an
 *   EPICS-based application program and a pure VxWorks application to control
 *   a single SDSU controller. It should be noted that the use of multiple
 *   contexts requires appropriate calls to sdsuLib in order to ensure that each
 *   context structure is maintained correctly.
 *
 *   The sdsuLib library provides mutual-excusion mechanisms which may be used
 *   to ensure that only one process can make physical access to an SDSU
 *   controller at any instance. The use of these functions is generally
 *   recommended whenever more than once task is required to access the same
 *   controller hardware. The exclusion functions can be used by multiple tasks
 *   on a single processor or by multiple tasks on multiple processors.
 *
 *   DSP Code
 *   --------
 *   The SDSU system makes extensive use of internal Digital Signal Processor
 *   (DSP) firmware and software. A number of downloadable DSP program modules
 *   will be made available as extensions to the basic system, and it is assumed
 *   that these are installed on the VxWorks host machine and are available to
 *   the sdsuLib library
 *
 *   EXTERNAL MODULES:
 *   timeoutLib             - Timeout library
 *   errorLib               - error handling library
 *
 *   FUNCTION NAME(S):
 *   sdsuLibInit            - initialise library, must be called first
 *   sdsuProbe              - check for presence of SDSU controller on VME bus
 *   sdsuContextCreate      - create & initialise context structure
 *   sdsuContextDelete      - delete context structure
 *   sdsuVersionGet         - get version numbers for sdsuLib and DSP firmware
 *   sdsuReset              - reset controller
 *   sdsuTest               - test controller
 *   sdsuShow               - display summary information about controller
 *   sdsuD                  - display internal controller memory
 *   sdsuM                  - modify internal controller memory
 *   sdsuPrintCmdBuf        - print command buffer
 *   sdsuPrintRepBuf        - print reply buffer
 *   sdsuPrimitiveWrite     - write primitive command to controller
 *   sdsuPrimitiveRead      - read controller response to primitive command
 *   sdsuPrimitive          - execute primitive command
 *   sdsuPrimitiveRDM       - read SDSU memory
 *   sdsuPrimitiveWRM       - write SDSU memory
 *   sdsuPrimitiveMultiRDM  - read block of SDSU memory
 *   sdsuPrimitiveMultiWRM  - write block of SDSU memory
 *   sdsuMemoryDnload       - download from local memory to controller
 *   sdsuMemoryUpload       - upload from controller to local memory
 *   sdsuFileDnload         - download OMF file to controller
 *   sdsuFileUpload         - upload OMF file from controller
 *   sdsuBufferCreate       - create frame buffers for detector readout
 *   sdsuBufferDelete       - destroy frame buffers previously allocated for 
 *                            readout
 *   sdsuFrameFind          - find an unused frame buffer
 *   sdsuFrameReserve       - reserve an existing frame of data
 *   sdsuFrameRelease       - release a previously reserved frame
 *   sdsuFrameShow          - display information about a frame
 *   sdsuFrameSetFBA        - send frame address to VME card
 *   sdsuIntConnect         - connect to controller interrupts
 *   sdsuSimulatePacketSync - simulate packet sync interrupt
 *   sdsuSimulateFrameSync  - simulate frame sync interrupt
 *   sdsuIntDisable         - disable controller interrupts
 *   sdsuReadoutOpen        - create readout task and set callbacks
 *   sdsuReadoutStart       - begin detector readout process
 *   sdsuReadoutAbort       - cancel remaining detector readouts
 *   sdsuReadoutClose       - shut down detector readout task
 *   sdsuParamWrite         - write DSP parameter to controller
 *   sdsuParamRead          - read DSP parameter value
 *   sdsuParamPrint         - print DSP parameter value
 *   sdsuParamDnload        - download parameter values from file
 *   sdsuParamUpload        - upload parameter values to file
 *   sdsuStatusShow         - print SDSU status parameters
 *   sdsuTempShow           - print SDSU temperature parameters
 *
 *
 *   DEFICIENCIES:
 *   This library uses a polling method for detecting a reply from the SDSU 
 *   controller, and the algorithm used can get confused if the controller is 
 *   reset unexpectedly and the timing board generates an SYR. The algorithm 
 *   can also get confused if some legitimate data just happens to contain ERR 
 *   or SYR byte sequences.
 *
 *   Many of the timing parameters used by this library, such as the wait time 
 *   while polling for a reply, the time to wait for a controller reset, and 
 *   the timeouts used for the various commands, have been set arbitrarily.
 *
 *   I think the algorithm for reading out data from an SDSU controller, 
 *   involving a message queue, an interrupt, various semaphores, a readout 
 *   task and a signal handler, is more complicated than it needs to be. Having 
 *   several threads makes handling errors difficult.
 *
 *   SMB - 16 December 1998.
 *
 *   ORIGINAL AUTHORS:
 *   Nick Dillon
 *   Andrew Johnson
 *
 *   MODIFIED BY:
 *   Steven Beard
 *   Tim Hardy
 *   Brian Leckie
 *   Corinne Boyer
 *
 *INDENT-OFF*
 *   10 dec 99 cb: tidy up
 *   6 oct 99 cb: add Chris Tierney code sdsu_checkRepBuf into sdsuPrimitiveRead
 *                or sdsuPrimitiveWrite + routine sdsu_checkRepBuf -> doesn't 
 *                work
 *                add sdsu_initRepBuf used in detControl.c 
 *
 *INDENT-ON*
 *-
 */

/****************************************************************** Includes***/

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif 

#include <stdio.h>
#include <stdlib.h>
#include <vme.h>
#include <memLib.h>
#include <cacheLib.h>
#include <taskLib.h>
#include <ioLib.h>
#include <string.h>
#include <symLib.h>
#include <lstLib.h>
#include <sysLib.h>
#include <iv.h>
#include <intLib.h>
#include <usrLib.h>
#include <sigLib.h>
#include <taskVarLib.h>
#include <ctype.h>
#include <vxLib.h>
#include <logLib.h>
#include <ppc.h>
#include "gemTypes.h"
#include "sdsuLib.h"
#include "errorLib.h"
#include "timeoutLib.h"
/*#include "xycom.h"*/


/****************************************************************** Defines ***/
/*#define CHECK_REPLY_BUFFER*/

/*#define DEBUG*/               /* Define this macro to enable debug messages   */

#ifdef DEBUG
BOOL sdsuFullDebug = FALSE;   /* This additional global flag prevents a flood */
                              /* of debug messages appearing while the OMF    */
                              /* files are being downloaded. Set the flag     */
                              /* TRUE from the VxWorks shell after the OMF    */
                              /* download.                                    */
#endif 

/******************************************************* General parameters ***/

#define OMF_MAX_CHARS_PER_LINE 80      /* Max # chars per line in OMF file    */
#define MAX_CHAR_FILENAME      256     /* Max # chars in file names           */
#define MAX_CHAR_STRING        256     /* Maximum string size                 */
#define MAX_NUM_CMD_ARG        2       /* Max # of command arguments allowed  */
#define CMD_BUF_NWORD (2 + MAX_NUM_CMD_ARG) /* # words in command buffer is   */
                                            /* one header word, one command   */
                                            /* word and up to MAX_NUM_CMD_ARG */
                                            /* argument words                 */
#define REP_BUF_NWORD          32      /* # words in circular reply buffer:   */
                                       /* must agree with what is implemented */
                                       /* in the VME DSP code                 */
#define PRINT_FUNC_WORDS_PER_LINE 8    /* # words/line for sdsuPrint[Rep|Cmd] */
                                       /* Buf()                               */
#define D_FUNC_WORDS_PER_LINE  4       /* # words/line for sdsuD()            */

/****************************************** File download/upload parameters ***/

#define FILE_LOAD_BUFFER_SIZE  1024    /* Working buffer size (# 32-bit words)*/
                                       /* used during memory up- and down-    */
                                       /* loading                             */

#define OMF_FIELD_START     "_START"     /* Object Module Format (OMF) record */
#define OMF_FIELD_END       "_END"       /* definitions. There are six record */
#define OMF_FIELD_DATA      "_DATA"      /* types defined. See also the enums */
#define OMF_FIELD_BLOCKDATA "_BLOCKDATA" /* OMF_FIELD_IDENT_name which are    */
#define OMF_FIELD_SYMBOL    "_SYMBOL"    /* numeric identifiers assigned to   */
#define OMF_FIELD_COMMENT   "_COMMENT"   /* each record type and used         */
                                         /* internally by sdsuLib.            */

/******************************************************* Timeout parameters ***/

#define TIMEOUT_READ_REPLY_SEC 2.0e-01  /* Timeout (s) reading reply header   */
#define DELAY_READ_REPLY_SEC   10.0e-06 /* Sleep time (s) polling for reply   */
#define TIMEOUT_READ_DATA_SEC  1.0e-2   /* Timeout (s) polling for data per   */
                                        /* packet                             */
#define DELAY_READ_DATA_TICKS  1        /* Sleep time (ticks) polling for data*/

#ifdef COMMENTED_OUT_BY_SOMEONE
/*
 * N O T E:
 *
 * Below is an old value of this parameter which someone (not me) has commented 
 * out and replaced with the 10.0e-06 values above. I am not sure what is going 
 * on here. On an mv167 it is not possible to wait for shorter than 1.6667e-02 
 * seconds because the system clock rate is 60 Hz, and 10.0e-06 seconds is in 
 * the realm of fantasy. I think both these sets of values will result in a 
 * taskDelay(0), which effectively means no wait at all but allow a task 
 * context switch. SMB - 11 December 1998.
 */

#define DELAY_READ_REPLY_SEC 1.0e-02      /* Sleep time (s) polling for reply */

#endif /* COMMENTED OUT BY SOMEONE */


/*********************************** Command/reply symbol-table definitions ***/

#define SYMTAB_HASHSIZE  1024            /* Hash-table size for symbol table  */
#define SYMBOL_GROUP     ((UINT16) 0)    /* Symbol group is arbitrary (groups */
                                         /* are not used by sdsuLib)          */
#define SYMBOL_TYPE_CMD  ((SYM_TYPE) 0)  /* Symbol type for commands          */
#define SYMBOL_TYPE_REP  ((SYM_TYPE) 1)  /* Symbol type for replies           */
#define SYMBOL_TYPE_MASK ((SYM_TYPE) 1)  /* Mask for symbol types             */

/*
 * Bit field identifiers for each possible reply to an SDSU command. These bits
 * get OR'd together in the command-definition table (sdsuCmdTable[]) to 
 * generate a set of valid replies to each command.
 */

#define BIT_FIELD_REPLY_NONE 0x00000000     /* ID for "no reply" must be zero */
#define BIT_FIELD_REPLY_AFE  0x00000001
#define BIT_FIELD_REPLY_DAT  0x00000002
#define BIT_FIELD_REPLY_DON  0x00000004
#define BIT_FIELD_REPLY_ERR  0x00000008
#define BIT_FIELD_REPLY_POE  0x00000010
#define BIT_FIELD_REPLY_TIM  0x00000020
#define BIT_FIELD_REPLY_SYR  0x80000000

/***************** Numeric identifiers used internally for OMF record types ***/

enum             
   {
      OMF_FIELD_IDENT_START,
      OMF_FIELD_IDENT_END,
      OMF_FIELD_IDENT_DATA,
      OMF_FIELD_IDENT_BLOCKDATA,
      OMF_FIELD_IDENT_SYMBOL,
      OMF_FIELD_IDENT_COMMENT,
      OMF_FIELD_IDENT_INVALID
   };

/******************************************************* External variables ***/

/*extern int swapFlag ;
extern xycomCard *xycom_ptr ;*/

/*************************************************** Module local variables ***/

LOCAL SYMTAB_ID sdsuSymTab = NULL;     /* Symbol table for defined cmds &     */
                                       /* replies                             */
LOCAL SEM_ID    sdsuAtomicSem = NULL;  /* Ensures atomicity for certain       */
                                       /* operations which would otherwise    */
                                       /* conflict with the need for sdsuLib  */
                                       /* to be re-entrant                    */

/*
 * The following locals are used with sdsuD() routine. The fact that these are 
 * static symbols means that sdsuD is NOT re-entrant. However, by making them
 * static they can be "remembered" between successive calls to sdsuD, thus 
 * allowing the arguments to this routine to be ommitted following the first 
 * call (analogous to the VxWorks d() routine - see usrLib).
 */

LOCAL SDSU_ID   sdsuDcontext       = NULL;
LOCAL uint32    sdsuDdestId        = SDSU_IDENT_VME;
LOCAL char      sdsuDpMemSpace [2] = "P";
LOCAL uint32    sdsuDptr           = SDSU_MEM_START_P;
LOCAL int       sdsuDnWord         = 4 * D_FUNC_WORDS_PER_LINE;

/*
 * The following locals are used with sdsuM() routine. Tbe fact that these are
 * static symbols means that sdsuM is NOT re-entrant. However, by making them
 * static they can be "remembered" between successive calls to sdsuM, thus 
 * allowing the arguments to this routine to be ommitted following the first 
 * call (analogous to the VxWorks m() routine - see usrLib).
 */

LOCAL SDSU_ID   sdsuMcontext       = NULL;
LOCAL uint32    sdsuMdestId        = SDSU_IDENT_VME;
LOCAL char      sdsuMpMemSpace [2] = "P";
LOCAL uint32    sdsuMptr           = SDSU_MEM_START_P;

/*
 * The following table defines all valid primitive commands recognised by 
 * sdsuLib. New commands can be added as and when required by adding an 
 * additional array element of type SDSU_CMD_DEF = {pString, nArg, nRep, 
 * timeout, replyExpected} where pString is the command mnemonic, nArg the 
 * number of command arguments, nRep the number of arguments expected in the 
 * reply, timeout is the time allowed in microseconds for the command to 
 * execute before sdsuLib will generate a time-out error and replyExpected 
 * defines the replies which the controller is expected to produce in response 
 * to each command (obtained by ORing together each possible reply).
 *
 * NOTE: It is possible for an "SYR" to be generated in reply to any of these
 * commands (for example if someone presses the reset button while a command
 * is executing). This is not a normal response and is therefore not included
 * in the table. sdsuLib will issue an "unexpected reply" error if this
 * happens.
 *
 * It is recommended that commands are kept in strict alphabetical order in this
 * table in order to make it easy to find the definition for a given command.
 *
 * NB. The timeouts specified below have not been fine-tuned (ND, October 1997).
 * Somone should replace these somewhat arbitrary values with more appropriate
 * values.
 *
 * I have increased some of the really small timeouts and have adopted the 
 * policy of increasing a timeout whenever it turns out to be a problem. 
 * SMB - 13 November 1998.
 */

LOCAL SDSU_CMD_DEF sdsuCmdTable [] =
{
   {"ABT", 0, 0, 2000000, BIT_FIELD_REPLY_DON}, /* Abort the previous RDC     */
                                                /* command.                   */
   {"IDL", 0, 0, 100000, BIT_FIELD_REPLY_DON }, /* IDL command used for       */
                                                /* debugging.                 */
   {"INI", 0, 0, 10000000, BIT_FIELD_REPLY_DON | BIT_FIELD_REPLY_POE},
                                                /* Initialise.                */
   {"LDA", 1, 0, 200000, BIT_FIELD_REPLY_DON},  /* Load application.          */
   {"LDP", 0, 0, 0, BIT_FIELD_REPLY_NONE},      /* Load parameters.           */
   {"RDC", 0, 0, 2000000, BIT_FIELD_REPLY_DON}, /* Readout CCD.               */
   {"RDM", 1, 0, 1000000, BIT_FIELD_REPLY_DAT | BIT_FIELD_REPLY_AFE},
                                                /* Read memory.               */
   {"RRS", 0, 0, 1000000, BIT_FIELD_REPLY_SYR}, /* Reset remote system.       */
   {"SRA", 2, 0, 500000, BIT_FIELD_REPLY_DON},  /* Set reply address.         */
   {"TDL", 1, 0, 500000, BIT_FIELD_REPLY_DAT},  /* Test data link.            */
   {"WRM", 2, 0, 1000000, BIT_FIELD_REPLY_DON | BIT_FIELD_REPLY_AFE},
                                                /* Write memory.              */
   {"WRP", 2, 0, 0, BIT_FIELD_REPLY_NONE}       /* Write parameter.           */
};

/*
 * The following table defines all valid replies to primitive commands 
 * recognised by sdsuLib. New replies can be added as and when required by 
 * adding an additional
 * array element of type SDSU_REP_DEF = {pString, nArg, replyType} where 
 * pString is the reply mnemonic, nArg the number of reply arguments and 
 * replyType identifies the type of reply.
 * It is recommended that replies are kept in strict alphabetical order in this
 * table in order to make it easy to find the definition for a given reply.
 */

LOCAL SDSU_REP_DEF sdsuRepTable [] =
{
   {"AFE", 0, BIT_FIELD_REPLY_AFE},   /* Address format error.                */
   {"DON", 0, BIT_FIELD_REPLY_DON},   /* Done. Command processed successfully */
   {"ERR", 0, BIT_FIELD_REPLY_ERR},   /* Error. Unrecognised command.         */
   {"POE", 0, BIT_FIELD_REPLY_POE},   /* Power on error.                      */
   {"TIM", 0, BIT_FIELD_REPLY_TIM},   /* Timeout while receiving command.     */
   {"SYR", 0, BIT_FIELD_REPLY_SYR}    /* System reset.                        */
};

/*
 * The next variable is used for allocating interrupt vector numbers for the
 * different controllers.  Each readout needs two vectors, for the frame and
 * packet sync interrupts.
 */

short sdsuVectorNumber = SDSU_INT_NUMBER_BASE;

/*
 * The following is used within the readout task to communicate the SDSU
 * context to the signal handler routine.  Each readout task makes this a
 * task variable, thus although it looks like a static here, vxWorks acts to
 * make it look like a different variable to each readout task.
 */

LOCAL SDSU_ID sdsuReadContext = NULL;

int sdsuFrameLost ;

/******************************************** Private function declarations ***/

STATUS       sdsu_incRepBufCounter (SDSU_ID pContext, int nIncrement);
LOCAL BOOL   sdsu_isAddressValid (uint32 address, const uint32 destId);
LOCAL BOOL   sdsu_areStringsSame (const char * pString1, const char * pString2);
LOCAL BOOL   sdsu_isRecordType (char * pField);
LOCAL uint32 sdsu_atoiBase16 (char * string);
LOCAL uint32 sdsu_getVersion (char * string);
LOCAL int    sdsu_makeHeader (SDSU_CMD_DEF * pCmdDef, uint32 sourceId, 
                              const uint32 destId, const BOOL cmdHeader);
LOCAL int    sdsu_makeCmdWord (SDSU_CMD_DEF * pCmdDef);
LOCAL BOOL   sdsu_isReplyValid (uint32 reply, SDSU_CMD_DEF * pCmdDef);
LOCAL STATUS sdsu_makeCmd (SDSU_ID pContext, SDSU_CMD_DEF * pCmdDef, 
                           const uint32 sourceId, const uint32 destId, 
                           uint32 * pCmdArg);
LOCAL int    sdsu_getRecordType (char * pField);
LOCAL STATUS sdsu_testTDL (SDSU_ID context, const uint32 destId, 
                           const BOOL verbose);
LOCAL STATUS sdsu_testRDM (SDSU_ID context, const uint32 destId, 
                           uint32 * pMemSpace, uint32 * pAddress, 
                           const BOOL verbose);
LOCAL STATUS sdsu_testWRM (SDSU_ID context, const uint32 destId, 
                           uint32 * pMemSpace, uint32 * pAddress, 
                           const BOOL verbose);
LOCAL void   sdsu_packetSyncIsr (int mailbox);
LOCAL void   sdsu_frameSyncIsr (int mailbox);
LOCAL void   sdsu_simpleSyncIsr (int mailbox);
LOCAL void   sdsu_readSignal (int sigNum);
LOCAL void   sdsu_readTask (int iContext, int iPacket, int iFrame, int iUseInt);
LOCAL void   sdsu_simpleTask (int iContext, int iPacket, int iFrame, 
                              int iUseInt);


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuLibInit
 *
 *   INVOCATION:
 *   sdsuLibInit ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the library could not be initialised.
 *
 *   PURPOSE:
 *   Initialisation routine for SDSU detector controller library
 *
 *   DESCRIPTION:
 *   Create and initialise symbol table used for commands and replies, allocate
 *   memory for internal data structures and create semaphores used by sdsuLib.
 *   The calling task is locked (with taskLock() and taskUnlock()) for the
 *   duration of this routine. The routine will be called automatically by
 *   sdsuContextCreate() if it has not previously been called.
 *
 *   EXTERNAL VARIABLES:
 *   (!) sdsuSymTab    (SYMTAB_ID) Symbol table ID.
 *   (!) sdsuAtomicSem (SEM_ID)    Semaphore ensures atomicity of certain 
 *                                 operations.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS sdsuLibInit ( void )
{
   int   index;

   /****************** taskLock() ensures init can only happen once per CPU ***/

#ifdef DEBUG
   sdsuFullDebug = FALSE ;
#endif

   taskLock ();     

   if (sdsuAtomicSem == NULL)       /* Only initialise if have not previously */
                                    /* done so                                */
   {

      /*
       * Create a symbol table used to store definitions of SDSU commands 
       * (/responses). Add a symbol for each command found in the arrays 
       * sdsuCmdTable[] and sdsuRepTable[].
       */

      if ((sdsuSymTab = symTblCreate ((int) SYMTAB_HASHSIZE, FALSE, 
                                      memSysPartId)) == NULL)
      {
         ERROR_SET (0, "Failed to create sdsuSymTab symbol table", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }

      for (index = 0; index < NELEMENTS (sdsuCmdTable); index++)
      {
         if (symAdd (sdsuSymTab, sdsuCmdTable [index].pCmdString, 
                     (char *) & sdsuCmdTable [index], SYMBOL_TYPE_CMD, 
                     SYMBOL_GROUP) == ERROR)
         {
            ERROR_SET1 (0, "Failed to add symbol %s to command table", 
                        ERROR_LOG_SAVE, sdsuCmdTable [index].pCmdString);
            return (ERROR);
         }
      }

      for (index = 0; index < NELEMENTS (sdsuRepTable); index++)
      {
         if (symAdd (sdsuSymTab, sdsuRepTable [index].pRepString, 
                     (char *) & sdsuRepTable [index], SYMBOL_TYPE_REP, 
                     SYMBOL_GROUP) == ERROR)
         {
            ERROR_SET1 (0, "Failed to add symbol %s to response table", 
                        ERROR_LOG_SAVE, sdsuRepTable [index].pRepString);
            return (ERROR);
         }
         sdsuRepTable [index].repWord = 
         SDSU_STRING_TO_UINT (sdsuRepTable [index].pRepString);
      }

      /*
       * Create a semaphore which allows certain sdsuLib operations to be 
       * atomic. Each SDSU context structure has another mutex semaphore which 
       * ensures that once an SDSU primitive command has been executed a reply 
       * must be read into the same context structure. However, sdsuAtomicSem 
       * is used for global things like connecting to interrupt sources etc. 
       * The existence of this semaphore also acts as a flag which indicates 
       * when the library has been initialised.
       */

      if ((sdsuAtomicSem = semMCreate (SEM_Q_FIFO | SEM_DELETE_SAFE)) == NULL)
      {
         ERROR_SET (0, "Failed to create sdsuAtomicSem mutex semaphore", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }

      /***************************************** Initialise timeout library ***/

      if (timeoutInit() == ERROR)
      {
         ERROR_SET (0, "Failed to initialise timeout library", ERROR_LOG_SAVE);
         return (ERROR);
      }

   }
   taskUnlock ();

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuProbe
 *
 *   INVOCATION:
 *   sdsuProbe (vmeAddress)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) vmeAddress (const uint32) Base address of SDSU interface on VME bus
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the given address could not be mapped to the 
 *             VME bus.
 *
 *   PURPOSE:
 *   Probes an address to see if an SDSU controller is present
 *
 *   DESCRIPTION:
 *   This function provides a non-intrusive way of testing for the presence of 
 *   an SDSU VME card. Unlike sdsuContextCreate, it does not create a context 
 *   structure or symbol table and so call be called at will. The function is 
 *   designed for engineering testing only.
 *
 *   SEE ALSO:
 *   sdsuContextCreate
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS sdsuProbe ( const uint32 vmeAddress )
{
   volatile uint32 *localAddress;   /* local address of SDSU interface.       */
   uint32          i = 0;           /* Test value to be poked into controller */
                                    /* registers.                             */

   /************* Map the address of the SDSU/VME interface card on the bus ***/

   if ( sysBusToLocalAdrs (SDSU_AM_VME_SLAVE_CMD,
                           (char *) vmeAddress,
                           (char **) &localAddress) == ERROR )
   {
      ERROR_SET (0, "Failed to map address of SDSU interface", ERROR_LOG_SAVE);
      return (ERROR);
   }

   printf ("SDSU interface: Bus address = %#x, Local address = %p\n", 
           vmeAddress, localAddress);
      
   /***** Check that the card is present by writing to the command register ***/

   if (vxMemProbe ((char *) localAddress, VX_WRITE, sizeof(uint32), 
                   (char *) &i) == ERROR)
   {
      printf ("SDSU interface card is not present.\n");
      return (OK);
   }

   printf ("SDSU interface card is present.\n");
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuContextCreate
 *
 *   INVOCATION:
 *   sdsuContextCreate (vmeAddress, simulate)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) vmeAddress (const uint32) Base address of SDSU interface on VME bus
 *   (>) simulate   (const BOOL)   Set TRUE to simulate the SDSU controller
 *
 *   FUNCTION VALUE:
 *   (SDSU_ID)   Context ID, or NULL if unsuccessful.
 *
 *   PURPOSE:
 *   Create and initialise an SDSU context structure
 *
 *   DESCRIPTION:
 *   This routine must be called before sdsuLib can be used to execute any
 *   commands which involve communication with SDSU controller hardware. It 
 *   returns the context ID (a pointer to a structure) which is subsequently 
 *   used to maintain information about the status of the controller, including
 *   the circular reply buffer and various configuration & status data. The 
 *   context ID ensures that sdsuLib is re-entrant since it holds all data 
 *   specific to a given controller: multiple controllers may therefore use 
 *   sdsuLib on a single CPU under VxWorks providing that each has a unique 
 *   context ID.
 *   After initialising the context ID with this routine it is necessary to
 *   reset the SDSU controller and issue a "Set Reply Address" (SRA) command
 *   to the SDSU controller at the specified VME address, which resets the VME
 *   DSP's reply address to point to the circular buffer that was allocated by
 *   this routine. This may be done by calling
 *v
 *v      sdsuReset (context, SDSU_RESET_VME | SDSU_RESET_CONTROLLER);
 *v
 *
 *   The "simulate" flag may be set TRUE to test the operation of sdsuLib in
 *   the absence of the SDSU controller hardware.
 *
 *   SEE ALSO:
 *   sdsuContextDelete
 *   sdsuReset
 *   sdsuSetReplyAddress
 *
 *   EXTERNAL VARIABLES:
 *   (>)   sdsuAtomicSem   (SEM_ID)   Symbol table ID.
 *
 *   PRIOR REQUIREMENTS:
 *   None. sdsuLibInit() will be called automatically.
 *
 *   POST REQUIREMENTS:
 *   IMPORTANT. After calling this function sdsuSetReplyAddress() needs to be 
 *   called to set the reply address before the controller can be used. 
 *   Alternately, sdsuReset() may be called to reset the controller and set 
 *   the reply address.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

SDSU_ID sdsuContextCreate ( const uint32 vmeAddress,
                            const BOOL   simulate )
{
   SDSU_ID context;       /* Pointer to SDSU context structure.               */
   uint32  i = 0;         /* Test value to be poked into controller registers */

   /************************************* Ensure the library is initialised ***/

   if (sdsuLibInit () == ERROR)
   {
      ERROR_SET (0, "sdsuLib initialisation failed", ERROR_LOG_SAVE);
      return (NULL);
   }

   /***************************** Allocate memory for the context structure ***/

   if ((context = (SDSU_ID) calloc ((size_t) 1, sizeof (SDSU_ID_STRUCT))) 
       == NULL)
   {
      ERROR_SET (0,"Memory allocation for SDSU context failed", ERROR_LOG_SAVE);
      return (NULL);
   }
   context->magic = SDSU_ID_MAGIC; /* Magic number (valid context identifier) */

   /*
    * Check whether the controller is to be simulated. In simulation mode no 
    * attempt will be made to use the specified address on the VME bus, and 
    * the local address of the SDSU controller will be set to NULL.
    */

   if (!simulate)
   {
      /********** Map the address of the SDSU/VME interface card on the bus ***/

      if (sysBusToLocalAdrs (SDSU_AM_VME_SLAVE_CMD,
                             (char *) vmeAddress,
                             (char **) &(context->pVmeAddress)) == ERROR)
      {
         ERROR_SET (0, "Failed to map address of SDSU interface", 
                    ERROR_LOG_SAVE);
         cfree ((char *) context);
         return (NULL);
      }

#ifdef DEBUG
      printf ("sdsuContextCreate: Testing SDSU command register at %p\n", 
              context->pVmeAddress);
#endif

      /** Check that the card is present by writing to the command register ***/

      if (vxMemProbe((char *) context->pVmeAddress, VX_WRITE, sizeof(uint32), 
                     (char *) &i) == ERROR)
      {
         ERROR_SET (S_sdsuLib_INV_CARD_ADDRESS, "SDSU interface not present", 
                    ERROR_LOG_SAVE);
         cfree ((char *) context);
         return (NULL);
      }

#ifdef DEBUG
      printf ("sdsuContextCreate: Writing to SDSU reset register at %p\n",
              ((char *) context->pVmeAddress) + SDSU_VME_BOARD_RESET_ADDRS);
#endif 

      /*
       * Write to the card reset register to initialise it.  This can cause a
       * bus error even with working hardware, so we ignore the return value.
       */

      vxMemProbe(((char *) context->pVmeAddress) + SDSU_VME_BOARD_RESET_ADDRS, 
                 VX_WRITE, sizeof(uint32) , (char *) &i);

      /*********************************** Wait for the board reset to work ***/

      taskDelay(SDSU_VME_BOARD_RESET_TIME * sysClkRateGet());

      /***** Record the fact that we are not simulating the SDSU controller ***/

      context->simulate = FALSE;
   }
   else
   {
      /********* Record the fact that we are simulating the SDSU controller ***/

      context->pVmeAddress = NULL;
      context->simulate = TRUE;
   }
   
   /*
    * Allocate memory for the command buffer, initialising its contents to zero.
    * This is just some local memory in which SDSU commands get assembled as 
    * a command header, command mnemonic and any associated command arguments.
    */

   context->pCmdBuffer = (uint32 *) calloc ((size_t) CMD_BUF_NWORD, 
                                            sizeof (uint32));
   if (context->pCmdBuffer == NULL)
   {
      ERROR_SET (0, "Command buffer memory allocation failed", ERROR_LOG_SAVE);
      cfree ((char *) context);
      return (NULL);
   }

   /*
    * Allocate memory for the reply buffer (the size of which - CMD_BUF_NWORD -
    * MUST agree with that programmed in the firmware for the SDSU VME DSP). 
    * The reply buffer is probably cached so we have to make sure we invalidate 
    * the cache whenever we're about the read it. The reply buffer must start 
    * on a 256-byte boundary.
    */

   context->pRepBuffer = (uint32 *) memalign (SDSU_REP_BUF_ALIGN, 
                                              REP_BUF_NWORD * sizeof (uint32));
   if (context->pRepBuffer == NULL)
   {
      ERROR_SET (0, "Reply buffer memory allocation failed", ERROR_LOG_SAVE);
      cfree ((char *) context->pCmdBuffer);
      cfree ((char *) context);
      return (NULL);
   }

   /*
    * Initialise reply buffer by filling it with zeros (since calloc was not 
    * used to allocate it), then flush the cache to ensure these values are up 
    * to date.
    */

   for (i = 0; i < REP_BUF_NWORD; i++)
       context->pRepBuffer [i] = 0;

   if (cacheFlush (DATA_CACHE, context->pRepBuffer, 
                   REP_BUF_NWORD * sizeof (uint32)) == ERROR)
   {
      ERROR_SET (0, "Cache flush for reply buffer failed", ERROR_LOG_SAVE);
      cfree ((char *) context->pRepBuffer);
      cfree ((char *) context->pCmdBuffer);
      cfree ((char *) context);
      return (NULL);      
   }

   /******************* Create symbol table for the DSP parameter addresses ***/

   if (((context->paramSyms = symTblCreate ((int) SYMTAB_HASHSIZE, FALSE, 
                                            memSysPartId)) == NULL))
   {
      ERROR_SET (0, "Parameter symbol table creation failed", ERROR_LOG_SAVE);
      cfree ((char *) context->pRepBuffer);
      cfree ((char *) context->pCmdBuffer);
      cfree ((char *) context);
      return (NULL);
   }

   /*
    * Create a semaphore to ensure atomic command-execution/completion. This 
    * semaphore needs SEM_Q_PRIORITY and SEM_INVERSION_SAFE options to ensure 
    * that the higher priority SDSU readout task (spawned by sdsuReadoutOpen) 
    * has priority access to the SDSU controller when it wants it.
    */

   context->commandSem = semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE | 
                                     SEM_INVERSION_SAFE);
   if (context->commandSem == NULL)
   {
      ERROR_SET (0, "Failed to create command semaphore", ERROR_LOG_SAVE);
      symTblDelete (context->paramSyms);
      cfree ((char *) context->pRepBuffer);
      cfree ((char *) context->pCmdBuffer);
      cfree ((char *) context);
      return (NULL);
   }
   
   /*
    * Create the packet sync semaphore
    */

   if (((context->packetSem = semBCreate (SEM_Q_FIFO, SEM_EMPTY)) == NULL))
   {
      ERROR_SET (0, "Failed to create packet sync semaphore", ERROR_LOG_SAVE);
      semDelete (context->commandSem);
      symTblDelete (context->paramSyms);
      cfree ((char *) context->pRepBuffer);
      cfree ((char *) context->pCmdBuffer);
      cfree ((char *) context);
      return (NULL);
   }
   
   /*
    * At this point the "Set Reply Address" (SRA) command needs to be issued
    * to the VME DSP. This is not done automatically because if it fails the
    * only way to report an error is to delete the SDSU context structure and
    * return NULL, but the caller may need the contect structure to correct
    * the error - a catch 22 situation. Instead it is recommended that the
    * caller calls sdsuSetReplyAddress() or sdsuReset() immediately after
    * calling this function.
    * See the "POST REQUIREMENTS" section above.
    */

   return (context);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuContextDelete
 *
 *   INVOCATION:
 *   sdsuContextDelete (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context (SDSU_ID) SDSU context ID
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the context ID is invalid.
 *
 *   PURPOSE:
 *   Free resources associated with an SDSU context ID
 *
 *   DESCRIPTION:
 *   This routine removes resources associated with a context structure
 *   and de-allocates memory used by the context ID. It should be the
 *   last routine called when terminating sdsuLib operations.
 *
 *   SEE ALSO:
 *   sdsuContextCreate
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   It is not possible to tidy up the SDSU interrupt vectors, since VxWorks 
 *   does not have an intDisconnect function. Repeated cycles of 
 *   sdsuContextCreate and sdsuContextDelete will eventually run out of 
 *   interrupt vectors.
 *-
 */

STATUS sdsuContextDelete ( SDSU_ID   context )
{
   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Abort any current observation, disable SDSU interrupts and delete the 
    * SDSU readout task if one has been spawned.
    * If any errors occur while doing this they are reported to the user, 
    * but they will not prevent the context being deleted.
    */

   if ( sdsuReadoutAbort (context) == ERROR )
   {
      ERROR_LOG ("Readout abort failed, but deleting context anyway");
   }

   if ( sdsuIntDisable (context) == ERROR )
   {
      ERROR_LOG("Failed to disable SDSU interrupts, at least deleting context");
   }

   if ( sdsuReadoutClose (context) == ERROR )
   {
      ERROR_LOG ("Failed to delete readout task, at least deleting context");
   }

   /****************** Free the SDSU frame buffer if one has been allocated ***/

   if (context->pDataBuffer != NULL)   sdsuBufferDelete (context);

   /* 
    * Free memory previously allocated in sdsuContextCreate() and delete 
    * the semaphores 
    */

   semDelete (context->commandSem);
   symTblDelete (context->paramSyms);
   cfree ((char *) context->pRepBuffer);
   cfree ((char *) context->pCmdBuffer);
   cfree ((char *) context);

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuVersionGet
 *
 *   INVOCATION:
 *   sdsuVersionGet (SDSU_ID context, const uint32 destId, BOOL firmware)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context   (SDSU_ID)      Context ID
 *   (>)   destId    (const uint32) Target processor ID
 *   (>)   firmware  (const BOOL)   Gets firmware rather than software version
 *
 *   FUNCTION VALUE:
 *   (uint32) Software or firmware version number, 0x00XXYYZZ in hexadecimal
 *            where 'XX', 'YY' and 'ZZ' are version codes.
 *
 *   PURPOSE:
 *   Get the software version number for sdsuLib or for the SDSU firmware
 *
 *   DESCRIPTION:
 *   This routine returns the software or firmware version number for a
 *   target controller DSP or for sdsuLib. The target is selected by destId, 
 *   whose value should correspond to one of the enumerated variables given in
 *   the table below.
 *
 *      SDSU_IDENT_HST   =>   Returns sdsuLib (host CPU) version number
 *      SDSU_IDENT_VME   =>   Returns VME DSP version number
 *      SDSU_IDENT_TIM   =>   Returns Timing DSP version number
 *      SDSU_IDENT_UTL   =>   Returns Utility DSP version number
 *
 *   DSP-code version numbers are derived from integer values, xxyy,
 *   defined in the code. If firmware is TRUE, the firmware DSP-code version
 *   number is returned, otherwise, the software version number is returned.
 *   
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32 sdsuVersionGet ( SDSU_ID         context,
                        const uint32    destId,
                        const BOOL      firmware )
{
   uint32   version = 0;
   char *   paramName;

   /*
    * The SDSU context is not required to query the sdsuLib version number.
    */

   if (destId == SDSU_IDENT_HST)
      return (sdsu_getVersion ("$Revision: 1.4 $"));
   
   /*
    * The SDSU context must be valid if the code gets this far, as the version 
    * is to be queried from a DSP.
    */

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   switch (destId)
   {
      case (SDSU_IDENT_VME):
         paramName = firmware ? "V_FW_VER" : "V_SW_ID";
         break;

      case (SDSU_IDENT_TIM):
         paramName = firmware ? "T_FW_VER" : "T_SW_ID";
         break;

      case (SDSU_IDENT_UTL):
         paramName = firmware ? "U_FW_VER" : "U_SW_ID";
         break;

      default:
         ERROR_SET (S_sdsuLib_INV_PROC_ID, "Invalid target ID", ERROR_LOG_SAVE);
         return (ERROR);
   }
   
   if (sdsuParamRead (context, destId, paramName, &version) == ERROR)
   {
      ERROR_SET (0, "Failed to read version number", ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   return (version);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuReset
 *
 *   INVOCATION:
 *   sdsuReset (context, what)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context   (SDSU_ID)        Context ID
 *   (>)   what      (const uint32)   Indicates what to reset
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the reset was unsuccessful.
 *
 *   PURPOSE:
 *   Reset VME interface and/or SDSU controller
 *
 *   DESCRIPTION:
 *   This routine examines 'what' to determine what is to be reset, and acts
 *   accordingly.  If the SDSU_RESET_VME bit of 'what' is set then it writes
 *   to the interface reset address which is 4 bytes above the base address
 *   of the board.  An "SRA" command is then issued to re-program the reply 
 *   address in the VME interface. Finally (and irrespective of whether a VME 
 *   board reset has been issued), if the SDSU_RESET_CONTROLLER bit of 'what' 
 *   is set then an "RRS" command is issued to the VME DSP.  This is a Gemini
 *   custom boot command which causes a special bit-pattern to be sent down 
 *   the fibre and results in the remote DSPs being reset.
 *
 *   NOTE:
 *   The DSP code will need to be redownloaded after executing this function.
 *   The timing board also needs to receive an LDP command after being reset,
 *   and the utility board needs to receive a INI command.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS sdsuReset ( SDSU_ID       context,
                   const uint32  what )
{
   SDSU_CMD_DEF *pCmdDef;
   SYM_TYPE     symType;
   uint32       itmp;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /******* Only perform the reset if we are not running in simulation mode ***/

   if (!context->simulate)
   {
      /************************************** Reset the VME interface first ***/

      if (what & SDSU_RESET_VME)
      {

#ifdef DEBUG
         printf ("sdsuReset: Resetting VME board.\n");
#endif
         /*
          * Write to the card reset register to initialise it.  This can cause a
          * bus error even with working hardware, so we ignore the return value.
          */
         vxMemProbe(((char *) context->pVmeAddress)+SDSU_VME_BOARD_RESET_ADDRS, 
                    VX_WRITE, sizeof(uint32) , (char *) &itmp);

         /*************************** Wait for the reset to work and finish ***/

         taskDelay(SDSU_VME_BOARD_RESET_TIME * sysClkRateGet());

         /*********** Then need to re-set the reply address for the VME DSP ***/

         if (sdsuSetReplyAddress (context) == ERROR)
         {
            ERROR_SET (0, "Failed to set reply address for VME DSP", 
                       ERROR_LOG_SAVE);
            return (ERROR);
         }
      }

      /* 
       * Reset remote DSPs by sending an RRS command to VME, but reply comes 
       * from TIM.
       */

      if (what & SDSU_RESET_CONTROLLER)
      {
#ifdef DEBUG
         printf ("sdsuReset: Resetting SDSU controller.\n");
#endif
         if (symFindByNameAndType (sdsuSymTab, "RRS", (char **) &pCmdDef, 
                                   &symType, SYMBOL_TYPE_CMD, SYMBOL_TYPE_MASK)              == ERROR)
         {
            ERROR_SET (S_sdsuLib_INV_COMMAND, 
                       "Could not find RRS command definition symbol",
                       ERROR_LOG_SAVE);
            return (ERROR);
         }

         if ((sdsuPrimitiveWrite (context, pCmdDef, SDSU_IDENT_HST, 
                                  SDSU_IDENT_VME, NULL)
              == ERROR) ||
            (sdsuPrimitiveRead (context, pCmdDef, SDSU_IDENT_TIM, 
                                SDSU_IDENT_HST, NULL)
              == ERROR)
            )
         {
            ERROR_SET (0, "Remote reset failed", ERROR_LOG_SAVE);
            return (ERROR);
         }
      }
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuTest
 *
 *   INVOCATION:
 *   sdsuTest (context, verbose, pTestMask, dspIdentMask)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context      (SDSU_ID)      Context ID
 *   (>) verbose      (BOOL)         Enable verbose print-out of test result
 *   (!) pTestMask    (uint32 *)     Pointer to mask for types of test required
 *   (>) dspIdentMask (const uint32) Mask for DSPs to test
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if any of the required tests was not successsful.
 *
 *   PURPOSE:
 *   Execute diagnostic tests of SDSU controller
 *
 *   DESCRIPTION:
 *   This routine executes a suite of pre-programmed tests on an SDSU controller
 *   system. The tests may be performed on one or more of the installed DSPs in
 *   the controller, according to the bit-field dspIdentMask as shown in the
 *   following table.
 *   
 *      Bit-0   =>   VME DSP
 *      Bit-1   =>   Timing DSP
 *      Bit-2   =>   Utility DSP
 *
 *   For example, if dspIdentMask is 0x05 then the Utility and VME DSPs will be
 *   tested; if dspIdentMask is 0x02 then only the Timing DSP will be tested.
 *
 *   The scope of the tests performed on each DSP is determined by the bit-field
 *   pointed to by pTestMask. Application code should generate this bit-field by
 *   ORing together any combination of the following macros.
 *
 *      SDSU_TEST_LINK  =>   Enables TDL test with varying data pattern
 *      SDSU_TEST_RDM   =>   Enables RDM from the first and last location in
 *                           each memory space
 *      SDSU_TEST_WRM   =>   Enables WRM to the first and last location in
 *                           X and Y (but not P) memory space
 *
 *   The contents of pTestMask on return from this routine reflect the result 
 *   of the tests performed - in the event of successful tests then *pTestMask 
 *   is set to zero, while if either test fails then the corresponding bit in 
 *   *pTestMask (as defined in the above table) will be set. Note that the 
 *   tests are aborted immediately on detection of an error so that only one 
 *   bit in *pTestMask should ever be set on return. Also, in the event that 
 *   more than one DSP is tested (more than one bit in dspIdentMask set on 
 *   entry to this routine), then if an error is detected in one of the tests 
 *   there is no way of determining which DSP failed the given test.
 *   In this case it is necessary to separately test each DSP in order to locate
 *   the error.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS sdsuTest ( SDSU_ID      context,
                  BOOL         verbose,
                  uint32 *     pTestMask,
                  const uint32 dspIdentMask )
{
   uint32         ident;
   uint32         memSpace;
   uint32         address;
   int            testMaskIn;
   int            sdsuTestCnt = 4; /* Number of consecutive link tests to try */
   int            i;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   testMaskIn = * pTestMask;               /* Save the required tests         */
   * pTestMask = 0;                        /* Assume all tests will be passed */

   /*
    * For each DSP, determine whether the requested tests are to be performed, 
    * then execute each test in turn. Return at the first test to fail or when 
    * all have been completed. If a test does fail, set the returned test mask,     * *pTestMask, to identify the failed test.
    */

   for (ident = SDSU_IDENT_VME; ident < SDSU_IDENT_INVALID; ident++)
   {
      if ((dspIdentMask & (1 << (ident - 1))) != 0)
      {
         if (verbose) printf ("Testing DSP %lu\n", ident);
         if ((testMaskIn & SDSU_TEST_LINK) != 0)
         {
            for (i = 0; i < sdsuTestCnt; i++)
            {
               if (sdsu_testTDL (context, ident, verbose) == ERROR)
               {
                  * pTestMask |= SDSU_TEST_LINK;
                  return (ERROR);
               }
            }
            if (verbose) printf (" - TDL test OK\n");
         }
         if ((testMaskIn & SDSU_TEST_RDM) != 0)
         {
            if (sdsu_testRDM (context, ident, & memSpace, & address, verbose) 
                == ERROR)
            {
               * pTestMask |= SDSU_TEST_RDM;
               return (ERROR);
            }
            if (verbose) printf (" - RDM test OK\n");
         }
         if ((testMaskIn & SDSU_TEST_WRM) != 0)
         {
            if (sdsu_testWRM (context, ident, & memSpace, & address, verbose) 
                == ERROR)
            {
               * pTestMask |= SDSU_TEST_WRM;
               return (ERROR);
            }
            if (verbose) printf (" - WRM test OK\n");
         }

         /*
          * More tests can be added here as necessary.
          */
      }
   }

   if (verbose) printf ("sdsuLib: SDSU system test completed with no errors\n");
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuShow
 *
 *   INVOCATION:
 *   sdsuShow (context, verbose)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context   (SDSU_ID)      Context ID.
 *   (>)   verbose   (const BOOL)   Enable verbose printout
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the context ID is invalid.
 *
 *   PURPOSE:
 *   Print summary information from an SDSU context structure
 *
 *   DESCRIPTION:
 *   This routine prints (to the VxWorks console) a summary of the information
 *   contained in the context ID structure. If verbose is TRUE it will also
 *   display the current contents of the circular reply buffer and of the
 *   command buffer - see the manual entries for sdsuPrintRepBuf() and
 *   sdsuPrintCmdBuf() respectively.
 *
 *   NOTE:
 *   This function is designed to be invoked from the VxWorks shell
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuShow ( SDSU_ID    context,
                    const BOOL verbose )
{
   int            i;
   SDSU_FRAME *   pFrame;
   const char *   readStatus[5] =
      {
         "CLOSED", "OPENING", "IDLE", "BUSY", "ERROR"
      };
   
   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Print a summary of the information contained in the context structure ***/

   printf ("Contents of SDSU context structure at %p:\n", context);
   printf ("------------------------------------------------\n");
   printf ("Simulation?                      : %s\n", 
           (context->simulate ? "YES" : "NO"));
   printf ("Fast camera?                     : %s\n", 
           (context->fastCamera ? "YES" : "NO"));
   printf ("VME interface bus address        : %p\n", context->pVmeAddress);
   printf ("Reply buffer (local address)     : %p\n", context->pRepBuffer);
   printf ("Reply-buffer counter             : %u\n", context->repBufCounter);
   printf ("Command buffer (local address)   : %p\n", context->pCmdBuffer);
   printf ("Command issue/reply semaphore ID : %p\n", context->commandSem);
   printf ("Parameter address symbol table   : %p\n", context->paramSyms);
   printf ("Max pixels per frame             : %d\n", 
           context->maxPixelsPerFrame);
   printf ("Total size of each frame in bytes: %u\n", context->frameSize);
   printf ("Number of frames in data buffer  : %d\n", context->nFrames);
   printf ("Number of packets per frame      : %d\n", context->packetsPerFrame);
   printf ("Exposure time in ticks           : %d\n", context->exposureTicks);
   printf ("Aborted?                         : %s\n", 
           (context->aborted ? "YES" : "NO"));
   printf ("Frame error count                : %d\n", context->frameErrors);
   printf ("Data buffer (local address)      : %p\n", context->pDataBuffer);
   printf ("Data buffer semaphore ID         : %p\n", context->bufferSem);

   printf ("Frames on free list (local)      : ");
   pFrame = context->pFreeList;

   /******************************************** Do not allow infinite loop ***/

   for (i=0; ((pFrame != NULL) && (i <= context->nFrames)); i++)   
   {
      /**************************** Only show the first 4 and last 4 frames ***/

      if ( (i < 4) || ((context->nFrames-i) < 4) )
         printf ("%p ", pFrame);
      else
         printf (". ");
      pFrame = pFrame->pNext;
   }
   printf ("\n");

   printf ("  Number of frames on free list  : %d", i);
   if ( i > context->nFrames ) printf (" !!");
   printf ("\n");

   printf ("Packet interrupt vector number   : %d\n", context->packetIntNum);
   printf ("Frame interrupt vector number    : %d\n", context->frameIntNum);
   printf ("Readout task ID                  : %#x\n", context->readTask);

   printf ("Readout task status              : ");
   if ( (context->readStatus >= 0) && (context->readStatus < 5) )
   {
      printf ("%s\n", readStatus[context->readStatus]);
   }
   else
   {
      printf ("INVALID !!\n");
   }

   printf ("Frame currently reading out      : %p\n", context->readFrame);
   printf ("Frame processing message queue   : %p\n", context->frameQueue);
   if (context->frameQueue != 0)
      printf ("  Number of frames on queue      : %d\n", 
              msgQNumMsgs(context->frameQueue));
   printf ("Readout packet semaphore ID      : %p\n", context->packetSem);
   printf ("Signal mask value                : %#x\n", context->sigMask);
   printf ("Application private pointer      : %p\n", context->appPrivate);
   printf ("Timeout start time (seconds)     : %ld.%09ld\n", 
           context->timeoutStart.tv_sec,
           context->timeoutStart.tv_nsec);
   printf ("\n");         /* Blank line for spacing. */

   /*********************** If verbose show the current frame contents also ***/

   if ( (verbose) && (context->readFrame != NULL) )
   {
      sdsuFrameShow (context->readFrame);
   }

   /*********************** If verbose print the command/reply buffers also ***/

   if (verbose)
   {
      sdsuPrintCmdBuf (context, FALSE);
      sdsuPrintRepBuf (context);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuD
 *
 *   INVOCATION:
 *   sdsuD (ptr, pMemSpace, nWord, destId, context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) ptr       (uint32)  Address of first DSP location to display
 *   (>) pMemSpace (char *)  String identifies memory space ("x","y","p" or "e")
 *   (>) nWord     (int)     Number of 24-bit DSP words to display
 *   (>) destId    (uint32)  DSP ID or pointer to string ID ("v", "t" or "u")
 *   (>) context   (SDSU_ID) Context ID
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the memory could not be displayed.
 *
 *   PURPOSE:
 *   Read DSP memory and display at the VxWorks console
 *
 *   DESCRIPTION:
 *   This routine is analogous to the VxWorks routine d() provided in usrLib. It
 *   allows a range of DSP memory locations to be read and displayed in
 *   hexadecimal and ascii format at the VxWorks console. The routine accepts
 *   its arguments in a versatile way - ptr specifies the starting address to
 *   display, and, if ptr includes a valid address-space identifier in the upper
 *   nibble of the 24-bit address (see the SDSU hardware manual), then 
 *   pMemSpace is ignored. If the upper nibble of ptr is zero, then the string 
 *   at pMemSpace identifies the required memory space and should be one of 
 *   "x", "y", "p" or "e" corresponding to X, Y, program and EEPROM memory 
 *   spaces respectively. nWord gives the number of 24-bit DSP words to display 
 *   starting from address ptr. destId identifies the target DSP to access - 
 *   this can be a valid DSP ID (see table below) or may point to a string 
 *   containing one of "v", "t" or "u" corresponding to the VME, Timing or 
 *   Utility DSP respectively.
 *
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   The routine stores the last set of parameters which were successfully 
 *   employed in a set of static variables and substitutes these for any 
 *   entry parameters which are either zero or NULL. This allows the second 
 *   and subsequent calls to the routine to advance through successive "pages" 
 *   of DSP memory, since the address (ptr) is incremented by nWord locatations 
 *   before being saved. Similarly, in many
 *   cases parameters such as the context ID need only be specified the first 
 *   time the routine is invoked. Note, however, that in order to allow
 *   addresses starting from ptr=0 to be displayed the value of ptr is only 
 *   substituted with that used on the last call to the routine if both ptr=0 
 *   and pMemSpace=0.
 *
 *   NOTE:
 *   This function is designed to be invoked from the VxWorks shell
 *
 *   EXTERNAL VARIABLES:
 *   (!) sdsuDcontext   (SDSU_ID)  Last context used
 *   (!) sdsuDdestId    (uint32)   Last destination ID used
 *   (!) sdsuDpMemSpace (char[2])  Last memory-space identifier used
 *   (!) sdsuDptr       (uint32)   Last display address used
 *   (!) sdsuDnWord     (int)      Last number words to display used
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   The output produce by this function can be a little confusing if nWord 
 *   is less than 4. The address of each word displayed is a combination of 
 *   the address shown at the left and the column number the word appears in. 
 *   If one word is displayed the address shown on the left may not correspond 
 *   to the address of the word displayed.
 *-
 */

STATUS   sdsuD ( uint32      ptr,
                 char *      pMemSpace,
                 int         nWord,
                 uint32      destId,
                 SDSU_ID     context )
{
   int         i;
   int         j;
   int         k;
   uint32 *    pData;
   uint32      addr;
   uint32      addr1;
   uint32      addr2;
   char        pString [D_FUNC_WORDS_PER_LINE * 3 + 1];

   pString [D_FUNC_WORDS_PER_LINE * 3] = 0;         /* Null-terminate string. */

   /************************************* Print a help message if requested ***/

   if ((ptr != NULL) && (sdsu_areStringsSame ((char *) ptr, "-help")))
   {
      printf ("sdsuD: Display SDSU controller DSP memory\n");
      printf ("usage: sdsuD ([addr] [,space] [,nWord] [,dsp] [,context])\n");
      printf ( "where: addr  = DSP address (with or without mem-space identifier in top nibble)\n");
      printf (
      "       space = [\"x\"|\"y\"|\"p\"|\"e\"] memory-space to display\n");
      printf (
      "       nWord = number of words (24-bit word length) to display\n");
      printf ("       dsp   = [\"v\"|\"t\"|\"u\"|%d|%d|%d] VME, Timing or Utility DSP\n",
              SDSU_IDENT_VME, SDSU_IDENT_TIM, SDSU_IDENT_UTL);
      printf ("       ctxt  = pointer to valid SDSU context structure\n");
      return (OK);
   }

   /*
    * Get any command-line arguments that were not given (or for which the 
    * value given was 0) from the static variables sdsuD__
    */

   if (context == NULL) context = sdsuDcontext;
   if (destId == 0) destId = sdsuDdestId;
   if (nWord == 0) nWord = sdsuDnWord;
   if (ptr == 0 && pMemSpace == NULL) ptr = sdsuDptr;
   if (pMemSpace == NULL) pMemSpace = sdsuDpMemSpace;

   /************************************** Determine the destination DSP ID ***/

   if ((destId != SDSU_IDENT_VME) && (destId != SDSU_IDENT_TIM) &&
       (destId != SDSU_IDENT_UTL))
   {
      if (sdsu_areStringsSame ((char *) destId, "v")) 
         destId = SDSU_IDENT_VME;
      else if (sdsu_areStringsSame ((char *) destId, "t"))   
         destId = SDSU_IDENT_TIM;
      else if (sdsu_areStringsSame ((char *) destId, "u"))   
         destId = SDSU_IDENT_UTL;
   }

   if (SDSU_ID_IS_INVALID (context)) /* Ensure the context structure is valid */
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /******************** Allocate a temporary buffer to store the data read ***/

   if ((pData = (uint32 *) calloc (nWord, sizeof(uint32))) == NULL)
   {
      ERROR_SET (0, "Memory allocation for temporary buffer failed", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * If pMemSpace identifies a valid DSP memory space ("x","Y","p" or "e" - 
    * case-insensitive) then OR the mem-space identifier with the starting 
    * address. Otherwise, simply ignore the mem-space identifier string. Also 
    * ignore the string if ptr already contains a DSP ident.
    */

   switch (ptr & SDSU_MEM_SPACE_MASK)
   {
      case (0):
         if      (sdsu_areStringsSame (pMemSpace, "X")) ptr |= SDSU_MEM_SPACE_X;
         else if (sdsu_areStringsSame (pMemSpace, "Y")) ptr |= SDSU_MEM_SPACE_Y;
         else if (sdsu_areStringsSame (pMemSpace, "P")) ptr |= SDSU_MEM_SPACE_P;
         else if (sdsu_areStringsSame (pMemSpace, "E")) ptr |= SDSU_MEM_SPACE_E;
         break;

      case (SDSU_MEM_SPACE_X):
         strcpy (pMemSpace, "X");
         break;

      case (SDSU_MEM_SPACE_Y):
         strcpy (pMemSpace, "Y");
         break;

      case (SDSU_MEM_SPACE_P):
         strcpy (pMemSpace, "P");
         break;
   }

   /********************************************** Read the data to display ***/

   if (sdsuPrimitiveMultiRDM (context, destId, ptr, pData, nWord) == ERROR)
   {
      ERROR_SET1 (0, "Failed to read block of DSP memory at address %#x", 
                  ERROR_LOG_SAVE, ptr);
      cfree ((char *) pData);
      return (ERROR);
   }

   /*************** Print a nicely-formatted display of the data just read. ***/

   addr1 = ptr & ~(D_FUNC_WORDS_PER_LINE - 1);
   addr2 = (ptr + nWord - 1) | (D_FUNC_WORDS_PER_LINE - 1);

   for (addr = addr1, i = 0, j = 0; addr <= addr2; addr++)
   {
      if ((addr % D_FUNC_WORDS_PER_LINE) == 0)
      {

         /********************************* Print DSP identification string ***/

         if      (destId == SDSU_IDENT_VME) printf ("VME-");
         else if (destId == SDSU_IDENT_TIM) printf ("TIM-");
         else if (destId == SDSU_IDENT_UTL) printf ("UTL-");
         else
         {
            ERROR_SET (S_sdsuLib_INV_PROC_ID, "Invalid DSP target", 
                       ERROR_LOG_SAVE);
            cfree ((char *) pData);
            return (ERROR);
         }

         if ((addr & SDSU_MEM_SPACE_MASK) == SDSU_MEM_SPACE_X)   
            printf ("X:");
         else if ((addr & SDSU_MEM_SPACE_MASK) == SDSU_MEM_SPACE_Y)   
            printf ("Y:");
         else if ((addr & SDSU_MEM_SPACE_MASK) == SDSU_MEM_SPACE_P)   
            printf ("P:");
         else printf ("?:");

         printf ("%06x: ", addr);
         j = 0;
      }

      if ((addr < ptr) || (addr > (ptr + nWord - 1)))
      {
         printf ("       ");
         for (k = 2; k >= 0; k--) pString [j++] = ' ';
      }
      else
      {
         printf (" %06x", pData [i++] & 0xffffff);
         for (k = 16; k >= 0; k-=8)
         {
            pString [j++] = (pData [i - 1] >> k) & 0xff;
            if (pString [j - 1] < ' ' || pString [j - 1] > '~')
               pString [j - 1] = '.';
         }
      }
      if (((addr + 1) % D_FUNC_WORDS_PER_LINE) == 0) 
         printf ("   *%s*\n", pString);
   }

   /********************************************* Free the temporary buffer ***/

   cfree ((char *) pData);

   /*
    * Save the parameters adopted on this invocation of the routine in the 
    * static variables sdsuD__, for optional use on the next invocation
    */

   sdsuDcontext = context;
   sdsuDdestId = destId;
   sdsuDnWord = nWord;
   strncpy (sdsuDpMemSpace, pMemSpace, 1);
   sdsuDptr = ptr + nWord;
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuM
 *
 *   INVOCATION:
 *   sdsuM (ptr, pMemSpace, destId, context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) ptr       (uint32)  Address of first DSP location to modify
 *   (>) pMemSpace (char *)  String identifies memory space ("x","y","p" or "e")
 *   (>) destId    (uint32)  DSP ID number or pointer to string ID ("v", "t" 
 *                           or "u")
 *   (>) context   (SDSU_ID) Context ID
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the memory could not be modified.
 *
 *   PURPOSE:
 *   Read DSP memory and modify at the VxWorks console
 *
 *   DESCRIPTION:
 *   This routine is analogous to the VxWorks routine m() provided in usrLib. It
 *   allows a range of DSP memory locations to be read, displayed and modified
 *   The routine accepts
 *   its arguments in a versatile way - ptr specifies the starting address to
 *   modify, and, if ptr includes a valid address-space identifier in the upper
 *   nibble of the 24-bit address (see the SDSU hardware manual), then pMemSpace
 *   is ignored. If the upper nibble of ptr is zero, then the string at 
 *   pMemSpace identifies the required memory space and should be one of "x", 
 *   "y", "p" or "e"
 *   corresponding to X, Y, program and EEPROM memory spaces respectively. 
 *   destId identifies the target DSP to access - this can be a valid DSP ID 
 *   (see table below) or may point to a string containing one of "v", "t" or 
 *   "u" corresponding to the VME, Timing or Utility DSP respectively.
 *
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   The routine stores the last set of parameters which were successfully 
 *   employed in a set of static variables and substitutes these for any entry 
 *   parameters which are either zero or NULL. This allows the second and 
 *   subsequent calls to the routine to advance through DSP memory. Similarly, 
 *   in many cases parameters such as the context ID need only be specified the 
 *   first time the routine is invoked. Note, however, that in order to allow 
 *   addresses starting from ptr=0 to be displayed the value of ptr is only 
 *   substituted with that used on the last call to the routine if ptr=0 and 
 *   pMemSpace=0.
 *
 *   NOTE:
 *   This function is designed to be invoked from the VxWorks shell
 *
 *   EXTERNAL VARIABLES:
 *   (!)   sdsuMcontext     (SDSU_ID)     Last context used
 *   (!)   sdsuMdestId      (uint32)      Last destination ID used
 *   (!)   sdsuMpMemSpace   (char[2])     Last memory-space identifier used
 *   (!)   sdsuMptr         (uint32)      Last display address used
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuM ( uint32      ptr,
                 char *      pMemSpace,
                 uint32      destId,         /* 1, 2, 3 or "v", "t", "u" */
                 SDSU_ID     context )
{
   int       i;
   BOOL      done = FALSE;
   uint32    data;
   char      pString [80];

   /************************************* Print a help message if requested ***/

   if ((ptr != NULL) && (sdsu_areStringsSame ((char *) ptr, "-help")))
   {
      printf ("sdsuM: Modify SDSU controller DSP memory\n");
      printf ("usage: sdsuM ([addr] [,space] [,dsp] [,context])\n");
      printf ("where: addr  = DSP address (with or without mem-space identifier in top nibble)\n");
      printf (
      "       space = [\"x\"|\"y\"|\"p\"|\"e\"] memory-space to modify\n");
      printf ("       dsp   = [\"v\"|\"t\"|\"u\"|%d|%d|%d] VME, Timing or Utility DSP\n",
              SDSU_IDENT_VME, SDSU_IDENT_TIM, SDSU_IDENT_UTL);
      printf ("       ctxt  = pointer to valid SDSU context structure\n");
      return (OK);
   }

   /*
    * Get any command-line arguments that were not given (or for which the 
    * value given was 0) from the static variables sdsuM__
    */

   if (context == NULL) context = sdsuMcontext;
   if (destId == 0) destId = sdsuMdestId;
   if (ptr == 0 && pMemSpace == NULL) ptr = sdsuMptr;
   if (pMemSpace == NULL) pMemSpace = sdsuMpMemSpace;

   /************************************** Determine the destination DSP ID ***/

   if (destId != SDSU_IDENT_VME && destId != SDSU_IDENT_TIM &&
       destId != SDSU_IDENT_UTL)
   {
      if (sdsu_areStringsSame ((char *) destId, "v"))   
         destId = SDSU_IDENT_VME;
      else if (sdsu_areStringsSame ((char *) destId, "t"))   
         destId = SDSU_IDENT_TIM;
      else if (sdsu_areStringsSame ((char *) destId, "u"))   
         destId = SDSU_IDENT_UTL;
   }

   if (SDSU_ID_IS_INVALID (context))     /* Ensure context structure is valid */
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * If pMemSpace identifies a valid DSP memory space ("x","Y","p" or "e" - 
    * case-insensitive) then or the mem-space identifier with the starting 
    * address. Otherwise, simply ignore the mem-space identifier string. Also 
    * ignore the string if ptr already contains a DSP ident.
    */

   switch (ptr & SDSU_MEM_SPACE_MASK)
   {
      case (0):
         if      (sdsu_areStringsSame (pMemSpace, "X")) ptr |= SDSU_MEM_SPACE_X;
         else if (sdsu_areStringsSame (pMemSpace, "Y")) ptr |= SDSU_MEM_SPACE_Y;
         else if (sdsu_areStringsSame (pMemSpace, "P")) ptr |= SDSU_MEM_SPACE_P;
         else if (sdsu_areStringsSame (pMemSpace, "E")) ptr |= SDSU_MEM_SPACE_E;
         break;

      case (SDSU_MEM_SPACE_X):
         strcpy (pMemSpace, "X");
         break;

      case (SDSU_MEM_SPACE_Y):
         strcpy (pMemSpace, "Y");
         break;

      case (SDSU_MEM_SPACE_P):
         strcpy (pMemSpace, "P");
         break;
   }

   while (! done)        /* Keep going until a non-numeric character is input */
   {
      /* Print DSP ID string. */

      if      (destId == SDSU_IDENT_VME) printf ("VME-");
      else if (destId == SDSU_IDENT_TIM) printf ("TIM-");
      else if (destId == SDSU_IDENT_UTL) printf ("UTL-");
      else
      {
         ERROR_SET (S_sdsuLib_INV_PROC_ID, "Invalid DSP target", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }

      /******************************************* Read current DSP address ***/

      if (sdsuPrimitiveRDM (context, destId, ptr, & data) == ERROR)
      {
         ERROR_SET1 (0, "Failed to read DSP memory at address %#x", 
                     ERROR_LOG_SAVE, ptr);
         return (ERROR);
      }

      /* Print mem-space ID. */

      if      ((ptr & SDSU_MEM_SPACE_MASK) == SDSU_MEM_SPACE_X)   printf ("X:");
      else if ((ptr & SDSU_MEM_SPACE_MASK) == SDSU_MEM_SPACE_Y)   printf ("Y:");
      else if ((ptr & SDSU_MEM_SPACE_MASK) == SDSU_MEM_SPACE_P)   printf ("P:");
      else    printf ("?:");

      printf ("%06x:  %06x-", ptr, data & 0xffffff);

      /* Get the next user input. */

      if (isxdigit (pString [0] = getchar ()) == 0)
      {
         if (pString [0] == 0x0a)
         {
            ptr++;                             /* Keep going if car'ge return */
         }
         else
         {
            getchar ();                       /* If carriage return, read the */
                                              /* next line-feed character to  */
                                              /* remove it from the stdin     */
                                              /* buffer                       */
            done = TRUE;
         }
      }
      else                                          /* Process numberic input */
      {
         i = 1;
         while ((pString [i++] = getchar ()) != 0x0a) ;     /* get all input  */
                                                            /* characters     */
                                                            /* Null loop body */

         data = strtoul (pString, NULL, 16) & 0xffffff;     /* then convert to*/
                                                            /* a digit        */

         /* Modify the DSP data. */

         if (sdsuPrimitiveWRM (context, destId, ptr++, data) == ERROR)
         {
            ptr--;
            ERROR_SET1 (0, "Failed to write DSP memory at address %#x", 
                        ERROR_LOG_SAVE, ptr);
            return (ERROR);
         }
      }
   }

   /*
    * Save the parameters adopted on this invocation of the routine in the 
    * static variables sdsuM__, for optional use on the next invocation
    */

   sdsuMcontext = context;
   sdsuMdestId = destId;
   strncpy (sdsuMpMemSpace, pMemSpace, 1);
   sdsuMptr = ptr + 1;

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuPrintCmdBuf
 *
 *   INVOCATION:
 *   sdsuPrintCmdBuf (context, printAll)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context  (SDSU_ID)    Context ID
 *   (>) printAll (const BOOL) Forces the entire command buffer to be printed
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the context ID is invalid.
 *
 *   PURPOSE:
 *   Print contents of SDSU command buffer
 *
 *   DESCRIPTION:
 *   This routine prints the SDSU command buffer at the VxWorks console. 
 *   The buffer should hold the last sequence of commands issued to the 
 *   controller. Those buffer locations which contain recognisable SDSU 
 *   command words are displayed as ASCII strings, all other words are in 
 *   hexadecimal format. If printAll is TRUE the entire buffer is printed 
 *   (typically 32 words), otherwise only the last command is displayed.
 *
 *   EXAMPLES:
 *v      sdsuPrintCmdBuf ctx
 *v      SDSU Command Buffer(:)
 *v         000103     RDM  100042  
 *v
 *v      sdsuPrintCmdBuf ctx,1
 *v      SDSU Command Buffer(:)
 *v         000103     RDM  100042  0dead0  0dead0  0dead0  0dead0  0dead0 
 *v         0dead0  0dead0  0dead0  0dead0  0dead0  0dead0  0dead0  0dead0 
 *v         0dead0  0dead0  0dead0  0dead0  0dead0  0dead0  0dead0  0dead0 
 *v         0dead0  0dead0  0dead0  0dead0  0dead0  0dead0  0dead0  0dead0 
 *v
 *
 *   NOTE:
 *   This function is useful when invoked from the VxWorks shell
 *
 *   EXTERNAL VARIABLES:
 *   (>)   sdsuSymTab    (SYMTAB_ID)    Symbol table ID.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuPrintCmdBuf ( SDSU_ID    context,
                           const BOOL printAll )
{
   int          n;
   char         pString [4];
   uint32       cmd;
   SDSU_CMD_DEF *pCmdDef;
   SYM_TYPE     symType;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /********************************* Print each word of the command buffer ***/

   printf ("Contents of SDSU Command Buffer at %p:\n", context->pCmdBuffer);
   printf ("----------------------------------------------\n");

   for (n = 1; n <= CMD_BUF_NWORD; n++)
   {
      printf (" ");
      cmd = context->pCmdBuffer [n - 1];
      SDSU_UINT_TO_STRING (cmd, pString);

      if (symFindByNameAndType (sdsuSymTab, pString, (char **) & pCmdDef, 
                                & symType, SYMBOL_TYPE_CMD, SYMBOL_TYPE_MASK) 
          == OK)
      {
         printf ("%6s", pString);
      }
      else if (((context->pCmdBuffer [n - 1] & 0xffffff) == 
                SDSU_CMD_WORD_EMPTY) && (! printAll))
      {
         printf ("\n");
         break;
      }
      else
      {
         printf ("%06x", context->pCmdBuffer [n - 1] & 0xffffff);
      }

      printf (" ");
      if (n % PRINT_FUNC_WORDS_PER_LINE == 0) printf ("\n");
   }
   printf ("\n");
   printf ("\n");                                  /* Blank line for spacing. */

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuPrintRepBuf
 *
 *   INVOCATION:
 *   sdsuPrintRepBuf (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context (SDSU_ID) Context ID
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the context ID is invalid.
 *
 *   PURPOSE:
 *   Print contents of SDSU circular reply buffer
 *
 *   DESCRIPTION:
 *   This routine prints the SDSU circular reply buffer at the VxWorks console.
 *   The buffer is cleared by sdsuLib when it wraps over and the current buffer
 *   pointer, i.e. the location to which the next reply word is expected to be  
 *   written bracketed by left- and right-angle brackets ('>' and '<'). Those 
 *   buffer locations which contain recognisable SDSU reply words are displayed 
 *   as ASCII strings, all other words are in hexadecimal format.
 *
 *   The most significant byte of each reply word (which is not used by the SDSU
 *   hardware) is set to 0xff whenever the word contains data written by the 
 *   SDSU DSP which has not yet been read by the VxWorks software. These words 
 *   are bracketed by '!' characters. If the word at the reply buffer pointer 
 *   is flagged in this way it is bracketed by '*' instead of '>' and '<'.
 *
 *   EXAMPLE:
 *v      sdsuPrintRepBuf ctx
 *v
 *v      SDSU Reply Buffer
 *v         010002     DON  010002  060400  010002  000048  010002  DON 
 *v         010002  053fa0 {>}000000{<} 000000  000000  000000  000000  000000 
 *v         000000  000000  000000  000000  000000  000000  000000  000000 
 *v         000000  000000  000000  000000  000000  000000  000000  000000
 *v
 *
 *   NOTE:
 *   This function is useful when invoked from the VxWorks shell
 *
 *   EXTERNAL VARIABLES:
 *   (>)   sdsuSymTab    (SYMTAB_ID)    Symbol table ID.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuPrintRepBuf ( SDSU_ID context )
{
   int          n;
   uint32       reply;
   char         pName [4];
   SYM_TYPE     type;
   SDSU_REP_DEF *pRepDef;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*********************************** Print each word of the reply buffer ***/

   printf ("Contents of SDSU Reply Buffer at %p:\n", context->pRepBuffer);
   printf ("--------------------------------------------\n");

   if (cacheInvalidate (DATA_CACHE, context->pRepBuffer, 
                        REP_BUF_NWORD * sizeof (uint32)) == ERROR)
   {
      ERROR_SET (0, "Cache invalidate for reply buffer failed", ERROR_LOG_SAVE);
      return (ERROR);
   }

   for (n = 1; n <= REP_BUF_NWORD; n++)
   {
      if ((n-1 == context->repBufCounter) && 
          ((context->pRepBuffer) [n - 1] & 0xff000000))
         printf ("*");
      else if (n-1 == context->repBufCounter)
         printf (">");
      else if ((context->pRepBuffer) [n - 1] & 0xff000000)
         printf("!");
      else
         printf (" ");

      reply = (context->pRepBuffer) [n - 1] & 0xffffff; /* Get reply word from*/
                                                        /* the buffer. Convert*/
      SDSU_UINT_TO_STRING (reply, pName);               /* it to a string     */

      /******************************************** Is it a known command ? ***/
      if (symFindByNameAndType (sdsuSymTab, pName, (char **) & pRepDef, & type,
          SYMBOL_TYPE_REP, SYMBOL_TYPE_MASK) == OK)
      {
         printf ("%6s", pRepDef->pRepString);      /* ...if yes, print string */
      }
      else
      {
         printf ("%06x", (context->pRepBuffer) [n - 1] & 0xffffff);
                                                   /* ...if no, print numeric */
      }

      if ((n-1 == context->repBufCounter) && 
          ((context->pRepBuffer) [n - 1] & 0xff000000))
         printf ("*");
      else if (n-1 == context->repBufCounter)
         printf ("<");
      else if ((context->pRepBuffer) [n - 1] & 0xff000000)
         printf("!");
      else
         printf (" ");

      if (n % PRINT_FUNC_WORDS_PER_LINE == 0) printf ("\n");
   }
   printf ("\n");                                  /* Blank line for spacing. */

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuPrimitiveWrite
 *
 *   INVOCATION:
 *   sdsuPrimitiveWrite (context, pCmdDef, sourceId, destId, pCmdArg)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context    (SDSU_ID)          Context ID
 *   (>)   pCmdDef    (SDSU_CMD_DEF *)   Command definition structure
 *   (>)   sourceId   (const uint32)     Source of command (e.g. host ID)
 *   (>)   destId     (const uint32)     Destination for command (e.g. DSP ID)
 *   (>)   pCmdArg    (uint32 *)         Array of command arguments
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK, or ERROR if an error was encountered while writing the command
 *
 *   PURPOSE:
 *   Issue SDSU primitive command to controller
 *
 *   DESCRIPTION:
 *   This routine issues a primitive command to a specified target processor. 
 *   The source will normally be the host CPU and the destination will normally 
 *   be one of the DSPs - VME, Timing or Utility, as specified in the table.
 *
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   The command is determined by the structure pCmdDef (the definition of 
 *   which can be found in sdsuLib.h). pCmdArg points to an array of command 
 *   arguments and these must occur in the order they are written to the DSP. 
 *   A NULL pointer may be passed if there are no arguments. This routine is 
 *   not normally used by application programs and is included in the public 
 *   interface as an aid to system diagnostics.
 *
 *   ATOMICITY OF PRIMITIVE COMMANDS:
 *   This routine calls semTake() on a mutex semaphore contained within the 
 *   SDSU context structure; the related routine sdsuPrimitiveRead() calls 
 *   semGive() on the same samaphore. This semaphore ensures that a primitive 
 *   command must be completed before a new one can be issued to a given 
 *   controller.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   It is possible for this function to wait forever to take the SDSU command 
 *   semaphore, although this is very unlikely to happen.
 *-
 */

STATUS sdsuPrimitiveWrite ( SDSU_ID      context,
                            SDSU_CMD_DEF *pCmdDef,
                            const uint32 sourceId,
                            const uint32 destId,
                            uint32       *pCmdArg )
{
   FAST int      i;
   int           nCmdWord;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Check the SDSU reply buffer - CODE ADDED BY CHRIS TIERNEY */


   /************************************************** Assemble the command ***/

   if (sdsu_makeCmd (context, pCmdDef, sourceId, destId, pCmdArg) == ERROR)
   {
      ERROR_SET (0, "Failed to assemble command words", ERROR_LOG_SAVE);
      return (ERROR);
   }
   nCmdWord = pCmdDef->cmdArgCount + 2;

#ifdef   DEBUG
   if (sdsuFullDebug)
      printf ("sdsuPrimitiveWrite: Writing the following command words to VME address %p:",
              context->pVmeAddress);
#endif 

   /*
    * The following sigblock and semTake() ensure that no other accesses to
    * the SDSU hardware can be made until the current command has been
    * completed via a call to sdsuPrimitiveRead().
    *
    * SIGBLOCK IS NOT NEEDED AS LONG AS THE INTERRUPTS AND SIGNAL HANDLER
    * MECHANISM FOR DATA READOUT IS NOT BEING USED.
    * SMB - 21 JAN 1999.
    */

   context->sigMask = sigblock (SIGMASK(SIGUSR1));
   semTake (context->commandSem, WAIT_FOREVER);

   /* 
    * Only write the command to the SDSU hardware if we are not running in 
    * simulation mode.
    */

   if (!context->simulate)
   {
      for (i = 0; i < nCmdWord; i++)    /* Write command to the SDSU hardware */
      {
         *(context->pVmeAddress) = (context->pCmdBuffer) [i];

#ifdef DEBUG
         if (sdsuFullDebug) printf (" %#x", (context->pCmdBuffer) [i]);
#endif 
      }
   }
#ifdef   DEBUG
   else
   {
      /* In simulation mode when debugging, just print out the command words. */

      if (sdsuFullDebug)
      {
         printf (" (simulation mode)" );
         for (i = 0; i < nCmdWord; i++)
         {
            printf (" %#x", (context->pCmdBuffer) [i]);
         }
      }
   }
#endif

#ifdef   DEBUG
   if (sdsuFullDebug) printf ("\n");
#endif

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuPrimitiveRead
 *
 *   INVOCATION:
 *   sdsuPrimitiveRead (context, pCmdDef, sourceId, destId, pRepArg)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context    (SDSU_ID)         Context ID
 *   (>)   pCmdDef    (SDSU_CMD_DEF *)  Command definition structure
 *   (>)   sourceId   (const uint32)    Source of reply (e.g. DSP ID)
 *   (>)   destId     (const uint32)    Destination for reply (e.g. host ID)
 *   (>)   pRepArg    (uint32 *)        Array of reply arguments
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if an error was encountered while reading the reply.
 *
 *   PURPOSE:
 *   Read the reply issued by the controller in response to a primitive command
 *
 *   DESCRIPTION:
 *   This routine reads the reply to a previously-executed primitive command. 
 *   The source will normally be one of the three DSPs - VME, Timing or Utility,
 *   and the destination will normally be the host CPU, as specified in the 
 *   table.
 *
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   The type of reply expected is determined by the structure pCmdDef 
 *   issues a primitive command to a specified target processor. The command
 *   is determined by the structure pCmdDef (the definition of which can
 *   be found in sdsuLib.h). pRepArg points to an array to which any arguments 
 *   associated with the reply will be copied. A NULL pointer may be passed
 *   if there are no arguments expected. This routine is not normally used by 
 *   application programs and is included in the public interface as an aid to 
 *   system diagnostics.
 *
 *   ATOMICITY OF PRIMITIVE COMMANDS:
 *   This routine calls semGive() on a mutex semaphore contained within the SDSU
 *   context structure, and also clears the SIGUSR1 bit in the task's signal
 *   mask.  The related routine sdsuPrimitiveWrite() is assumed to have
 *   previously called semTake() on the same samaphore and blocked the SIGUSR1
 *   signal. This ensures that a primitive command must be completed before a
 *   new one can be issued to a given controller by a different task or from
 *   the SIGUSR1 signal handler of the same task.
 *   
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   timeoutLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   If the reply contains data which happens to contain the string "ERR" or 
 *   "SYR" for some innocent reason, this function will assume, incorrectly, 
 *   that an error has occurred.
 *   
 *   This code is horrible, and is unable to cope with an unexpected SYR reply 
 *   from the timing DSP (as generated by an RRS command, or by someone 
 *   pressing the reset button on the controller). It needs a rewrite it to
 *   cope with this, but I don't think it's essential right now. ANJ - 15July98.
 *
 *   I agree with ANJ's comment. I have seen the timing DSP reply with SYR just 
 *   after it has been switched on, which can make the first attempt to download
 *   DSP code to it fail.
 *   The VME board can sometimes respond with a TIM reply if a timeout occurs 
 *   during an observation, and this is not handled properly either.
 *   SMB - 17 September 98.
 *-
 */

STATUS sdsuPrimitiveRead ( SDSU_ID      context,
                           SDSU_CMD_DEF *pCmdDef,
                           uint32       sourceId,
                           uint32       destId,
                           uint32       *repArg )
{
   uint32         header;
   uint32         headerExpected;
   uint32         reply;
   uint32         repArgCounter = 0;
   uint32         nArgToReturn;
   int            nWordExpected;
   double         timeoutSec;
   BOOL           validReplyFlag = TRUE;
   char           pReplyString[4];

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /***************** Do nothing if the command does not generate any reply ***/

   if (pCmdDef->repType == BIT_FIELD_REPLY_NONE)
   {
      semGive (context->commandSem);
      sigsetmask (context->sigMask);
      return (OK);
   }

#ifdef CHECK_REPLY_BUFFER
   if (sdsu_checkRepBuf (context) == ERROR)
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, 
                 "Reply buffer error - write to controller aborted",
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
#endif /* CHECK_REPLY_BUFFER */
   /******************************** Get the number of reply argument words ***/

   if (pCmdDef->repType == BIT_FIELD_REPLY_DAT)
   {
      nArgToReturn = pCmdDef->repArgCount + 1;
   }
   else
   {
      nArgToReturn = pCmdDef->repArgCount;
   }

   /****************************** Get total number of reply words expected ***/

   nWordExpected = 2 + (int) nArgToReturn;

   /****************************************** Get the expected header word ***/

   headerExpected = sdsu_makeHeader (pCmdDef, sourceId, destId, FALSE);

   if ( headerExpected == 0 )
   {
      ERROR_SET (0, "Failed to construct the expected header word", 
                 ERROR_LOG_SAVE);
      semGive (context->commandSem);
      sigsetmask (context->sigMask);
      return (ERROR);
   }

   timeoutSec = (pCmdDef->timeoutHeaderUsec) / 1.0e6;

#ifdef   DEBUG
   if (sdsuFullDebug)
      printf ("sdsuPrimitiveRead: "
         "Expecting reply header (%#x) and %lu arguments within %f seconds\n",
         headerExpected, nArgToReturn, timeoutSec);
#endif 

   /*
    * A reply will only be received if we are dealing with real SDSU hardware, 
    * so only check for a reply if we are not running in simulation mode.
    */

   if (!context->simulate)
   {

      /*
       * Enter a loop polling for the reply-header word. Only break from this 
       * loop when a valid header word has been read or after the specified 
       * timeout period for the command. But, before entering this loop poll 
       * just once for the reply word since in most cases a header will already 
       * be present and there is no need to fuss around setting up for timeouts        * etc.
       */

      if (cacheInvalidate (DATA_CACHE, context->pRepBuffer, 
          REP_BUF_NWORD * sizeof (uint32)) == ERROR)
      {
         ERROR_SET (0, "Cache invalidate for reply buffer failed", 
                    ERROR_LOG_SAVE);
         semGive (context->commandSem);
         sigsetmask (context->sigMask);
         return (ERROR);
      }

      if ((context->pRepBuffer [context->repBufCounter] & 0xff000000)   == 0)
      {
                                /* The header wasn't valid on the first poll, */
                                /* so enter the timeout loop                  */

         START_TIMEOUT (& context->timeoutStart);
         while (((context->pRepBuffer [context->repBufCounter] & 0xff000000) 
                == 0) &&
                (! timeoutExpired (timeoutSec, & context->timeoutStart)))
         {
            taskDelay (SEC_TO_NTICK (DELAY_READ_REPLY_SEC));   
            if (cacheInvalidate (DATA_CACHE, context->pRepBuffer, 
                                 REP_BUF_NWORD * sizeof (uint32))
                == ERROR)
            {
               ERROR_SET (0, "Cache invalidate for reply buffer failed", 
                          ERROR_LOG_SAVE);
               semGive (context->commandSem);
               sigsetmask (context->sigMask);
               return (ERROR);
            }
         }
      }

      if ((context->pRepBuffer [context->repBufCounter] & 0xff000000)   == 0)
      {
         /********** Nothing was written to the reply buffer before timeout ***/
         ERROR_SET3 (S_sdsuLib_REPLY_TIMEOUT,
               "Timeout reading header word, expected=%#x, RepBuffer=%p+%d", 
               ERROR_LOG_SAVE, headerExpected, context->pRepBuffer, 
               context->repBufCounter);

         semGive (context->commandSem);             /* Done with this command */
         sigsetmask (context->sigMask);
         return (ERROR);
      }
      
      header = context->pRepBuffer [context->repBufCounter] & 0xffffff;
      if ((header & 0xffff00)   != (headerExpected & 0xffff00))
      {
         /*
          * The wrong header word was received.
          * This might be an SYR from TIM or a TIM from VME (if we were 
          * expecting a response from some other DSP just now), but there's no 
          * code in here to check that and deal with the consequences.
          */
         
         ERROR_SET4 (S_sdsuLib_REPLY_TIMEOUT,
            "Unexpected header word, expected=%#x, actual=%#x, RepBuffer=%p+%d",
            ERROR_LOG_SAVE, headerExpected, header, context->pRepBuffer,
            context->repBufCounter);

                        /* Skip the header word and any reply argument words. */
                        /* BUG FIX: LOGIC REVERSED - SMB 15 Jan 99 */
         while ((context->pRepBuffer [context->repBufCounter] & 0xff000000) 
                != 0)
            sdsu_incRepBufCounter (context, 1);

         semGive (context->commandSem);             /* Done with this command */
         sigsetmask (context->sigMask);
         return (ERROR);
      }

      /*
       * A valid reply-header word has been received.
       * Increment the reply-buffer pointer to skip over it.
       */

      sdsu_incRepBufCounter (context, 1);

#ifdef DEBUG
      if (sdsuFullDebug)
      printf (
      "sdsuPrimitiveRead: Read expected reply header(%#x) successfully\n", 
      header);
#endif 

      if ((context->pRepBuffer [context->repBufCounter] & 0xff000000)   == 0)
      {
          /* The reply word wasn't valid on the first poll. Enter the timeout */
          /* loop                                                             */

         START_TIMEOUT (& context->timeoutStart);
         if (cacheInvalidate (DATA_CACHE, context->pRepBuffer, 
             REP_BUF_NWORD * sizeof (uint32)) == ERROR)
         {
            ERROR_SET (0, "Cache invalidate for reply buffer failed", 
                       ERROR_LOG_SAVE);
            semGive (context->commandSem);
            sigsetmask (context->sigMask);
            return (ERROR);
         }

         while (((context->pRepBuffer [context->repBufCounter] & 0xff000000) 
                == 0) && ! timeoutExpired (TIMEOUT_READ_REPLY_SEC, 
                & context->timeoutStart))
         {
            taskDelay (SEC_TO_NTICK (DELAY_READ_REPLY_SEC)); 
            if (cacheInvalidate (DATA_CACHE, context->pRepBuffer, 
                REP_BUF_NWORD * sizeof (uint32)) == ERROR)
            {
               ERROR_SET (0, "Cache invalidate for reply buffer failed", 
                          ERROR_LOG_SAVE);
               semGive (context->commandSem);
               sigsetmask (context->sigMask);
               return (ERROR);
            }
         }
      }

      if ((context->pRepBuffer [context->repBufCounter] & 0xff000000)   == 0)
      {
         /* Nothing was written to the reply buffer before timeout */
         ERROR_SET2 (S_sdsuLib_REPLY_TIMEOUT, 
                     "Timeout reading reply word, RepBuffer=%p+%d",
                     ERROR_LOG_SAVE, context->pRepBuffer, 
                     context->repBufCounter);

         semGive (context->commandSem);             /* Done with this command */
         sigsetmask (context->sigMask);
         return (ERROR);
      }

      /* Read reply from reply buffer */
      reply = (context->pRepBuffer) [context->repBufCounter] & 0xffffff;

      /*
       * If the command can return data, then check that an ERR or SYR are not 
       * returned.
       * (NOTE: It is possible for valid data to contain these values for 
       * innocent reasons).
       */

      if ((pCmdDef->repType & BIT_FIELD_REPLY_DAT) != 0)
      {
         if (reply == SDSU_STRING_TO_UINT ("ERR"))
         {
            ERROR_SET (S_sdsuLib_ERROR_RETURNED, 
                       "ERR returned in reply data argument",
                       ERROR_LOG_SAVE);

            sdsu_incRepBufCounter (context, 1);
            repArg [repArgCounter] = reply;
            
            semGive (context->commandSem);
            sigsetmask (context->sigMask);
            return (ERROR);
         }
         else if (reply == SDSU_STRING_TO_UINT ("SYR"))
         {
            ERROR_SET (S_sdsuLib_ERROR_RETURNED, 
                       "SYR returned in reply data argument",
                       ERROR_LOG_SAVE);

            sdsu_incRepBufCounter (context, 1);
            repArg [repArgCounter] = reply;
            
            semGive (context->commandSem);
            sigsetmask (context->sigMask);
            return (ERROR);
         }

#ifdef   DEBUG
         if (sdsuFullDebug)
         printf ("sdsuPrimitiveRead: Read 24-bit dataword (%#x) successfully\n",
                 repArg [repArgCounter] & 0xffffff);
#endif

         validReplyFlag = TRUE;
      }
      else
      {
         /* A non-data reply is expected. Confirm its validity */
         validReplyFlag = sdsu_isReplyValid (reply, pCmdDef);
      }

      /* 
       * Finished reading the SDSU reply-header and reply word: handle any 
       * errors
       */

      if (! validReplyFlag)
      {
         SDSU_UINT_TO_STRING (reply,pReplyString)
         ERROR_SET2 (S_sdsuLib_REPLY_TIMEOUT, 
                     "Invalid SDSU reply, returned value=%#x=%s",
                     ERROR_LOG_SAVE, reply, pReplyString);

         /* BUG FIX: REPLACE SINGLE INCREMENT WITH WHILE LOOP - SMB 15 Jan 99 */
         while ((context->pRepBuffer [context->repBufCounter] & 0xff000000) 
                != 0)
            sdsu_incRepBufCounter (context, 1);

         semGive (context->commandSem);
         sigsetmask (context->sigMask);
         return (ERROR);
      }
#ifdef   DEBUG
      if (sdsuFullDebug)
      {
         SDSU_UINT_TO_STRING (reply,pReplyString)
         printf (
         "sdsuPrimitiveRead: Read expected reply word (%#x=%s) successfully\n",
         reply, pReplyString);
      }
#endif 

      repArg [repArgCounter++] = reply;
      sdsu_incRepBufCounter (context, 1);
      while (repArgCounter < nArgToReturn)         /* Get any reply arguments */
      {
         repArg [repArgCounter++] = 
         (context->pRepBuffer) [context->repBufCounter] & 0xffffff;

#ifdef DEBUG
         if (sdsuFullDebug)
         printf ("sdsuPrimitiveRead: Read reply argument (%#x) successfully\n",
                 repArg [repArgCounter - 1]);
#endif 

         sdsu_incRepBufCounter(context,1); /* Skip over each arg as it is read*/
      }
   }
   semGive (context->commandSem);           /* Finally, give-up the semaphore */
   sigsetmask (context->sigMask);
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuPrimitive
 *
 *   INVOCATION:
 *   sdsuPrimitive (context, pCommand, destId, pCmdArg, pRepArg)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context    (SDSU_ID)        Context ID
 *   (>)   pCommand   (char *)         Command mnemonic string
 *   (>)   destId     (const uint32)   Destination for command (DSP ID)
 *   (>)   pCmdArg    (uint32 *)       Array of command arguments
 *   (>)   pRepArg    (uint32 *)       Array of reply arguments
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if an error was encountered while executing the 
 *             command.
 *
 *   PURPOSE:
 *   Execute SDSU primitive command
 *
 *   DESCRIPTION:
 *   This routine executes an SDSU primitive command. It combines the routines
 *   sdsuPrimitiveWrite() and sdsuPrimitiveRead(), and takes as the command 
 *   definition a 3-character (plus null) string which holds the command 
 *   mnemonic, e.g. "RDM" to read DSP memory. The source of the command is by 
 *   definition always the host processor but the destination is determined by 
 *   destId which must be a valid DSP ID taken from the table below. Command 
 *   arguments are contained in the array pCmdArg and any returned values are 
 *   copied to pRepArg. Either of these two pointers may be NULL if there are 
 *   no arguments.
 *
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   ATOMICITY OF PRIMITIVE COMMANDS:
 *   This routine uses a mutex semaphore contained within the SDSU context 
 *   strucuture to ensure that primitive commands are executed atomically. 
 *   This is potentially important where, for example, more than one VxWorks 
 *   task may issue commands to the same controller. Such a situation is somwhat
 *   dangerous, however a more likely scenario is one in which a user needs to 
 *   issue commands from a VxWorks console (e.g. in order to determine the 
 *   state of a controller) while a separate VxWorks task is also issuing 
 *   commands. Note that there are no exclusion mechanisms for applications in 
 *   which primitive commands are issued to a single SDSU controller from 
 *   different VxWorks CPUs, since in this case each CPU will have a separate
 *   mutex semaphore within its own context structure. Such an application 
 *   requires inter-CPU communications to provide exclusive to each CPU at 
 *   any instant.
 *
 *   PRIMITIVE COMMANDS:
 *   A primitive command must be recognised by sdsuLib for it to send it. The 
 *   fact that a command is recognised does not, however, guarantee that the
 *   command is supported by the downloaded SDSU software or firmware - this 
 *   must be determined from examination of the specific system installation.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   sdsuSymTab   (SYMTAB_ID)      Symbol table ID.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   REFERENCES:
 *   See The Gemini ICD1.6/1.10 for a full description of the primitive
 *   commands, including definition of any assoicated arguments and expected 
 *   return values.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS sdsuPrimitive ( SDSU_ID      context,
                       char         *pCommand,
                       const uint32 destId,
                       uint32       *pCmdArg,
                       uint32       *pRepArg )
{
   SDSU_CMD_DEF *pCmdDef;
   SYM_TYPE     symType;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /********************** Get the command-definition from the symbol table ***/

   if (symFindByNameAndType (sdsuSymTab, pCommand, (char **) & pCmdDef, 
                             & symType, SYMBOL_TYPE_CMD, SYMBOL_TYPE_MASK) 
       == ERROR)
   {
      ERROR_SET1 (S_sdsuLib_INV_COMMAND, 
                  "sdsuPrimitive: Could not find command definition symbol, %s",
                  ERROR_LOG_SAVE, pCommand);
      return (ERROR);
   }

#ifdef DEBUG
   if (sdsuFullDebug)
   {
      printf ("sdsuPrimitive: Issuing command \"%s\" with %d arguments.\n",
              pCmdDef->pCmdString, pCmdDef->cmdArgCount);
      printf (
      "sdsuPrimitive: %d reply arguments of type %#x are expected within %d microseconds.\n",
      pCmdDef->repArgCount, pCmdDef->repType, pCmdDef->timeoutHeaderUsec);
   }
#endif 

   /*************** Write the command to SDSU hardware, then read the reply ***/

   if ((sdsuPrimitiveWrite (context, pCmdDef, SDSU_IDENT_HST, destId, pCmdArg)
       == ERROR) ||
      (sdsuPrimitiveRead (context, pCmdDef, destId, SDSU_IDENT_HST, pRepArg) 
       == ERROR))
   {
      ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE); 
      return (ERROR);
   }
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuPrimitiveRDM
 *
 *   INVOCATION:
 *   sdsuPrimitiveRDM (context, destId, address, pData)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context   (SDSU_ID)      Context ID
 *   (>) destId    (const uint32) ID of DSP from which to read data
 *   (>) address   (uint32)       Memory address to read
 *   (!) pData     (uint32 *)     Where to put data read from DSP
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the DSP address could not be read.
 *
 *   PURPOSE:
 *   Read a 24-bit data word from an address in DSP memory space
 *
 *   DESCRIPTION:
 *   This routine executes the SDSU primitive command 'RDM'. It reads a 24-
 *   bit word from the memory location address for the specified destination
 *   DSP. destId can take one of the values specified in the table below.
 *
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   The data word is copied to the location pointed to by pData. The
 *   most-significant byte written to pData is not defined.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS sdsuPrimitiveRDM ( SDSU_ID      context,
                          const uint32 destId,
                          uint32       address,
                          uint32       *pData )
{
   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   if (! sdsu_isAddressValid (address, destId))
   {
      ERROR_SET1 (0, "Invalid address, %#x", ERROR_LOG_SAVE, address);
      return (ERROR);
   }

   /***************************************************** Issue RDM command ***/

   if (sdsuPrimitive (context, "RDM", destId, & address, pData) == ERROR)
   {
      ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);   
      return (ERROR);
   }
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuPrimitiveWRM
 *
 *   INVOCATION:
 *   sdsuPrimitiveWRM (context, destId, address, data)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context   (SDSU_ID)      Context ID
 *   (>) destId    (const uint32) ID of DSP to which data will be written
 *   (>) address   (uint32)       Memory address to write
 *   (!) data      (uint32)       Data to write
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the DSP address could not be written.
 *
 *   PURPOSE:
 *   Write a 24-bit data word to an address in DSP memory space
 *
 *   DESCRIPTION:
 *   This routine executes the SDSU primitive command 'WRM'. It writes
 *   the least significant 24 bits of data to DSP memory location address,
 *   for the specified destination DSP. destId can take one of the values
 *   specified in the table below.
 *
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuPrimitiveWRM ( SDSU_ID        context,
                            const uint32   destId,
                            uint32         address,
                            uint32         data )
{
   uint32   pAddressData [2];

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   if (! sdsu_isAddressValid (address, destId))
   {
      ERROR_SET1 (0, "Invalid address, %#x", ERROR_LOG_SAVE, address);
      return (ERROR);
   }
   pAddressData [0] = address;
   pAddressData [1] = data;

   /***************************************************** Issue WRM command ***/

   if (sdsuPrimitive (context, "WRM", destId, pAddressData, NULL) == ERROR)
   {
      ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);    
      return (ERROR);
   }
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuPrimitiveMultiRDM
 *
 *   INVOCATION:
 *   sdsuPrimitiveMultiRDM (context, destId, address, pData, wordCount)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context   (SDSU_ID)      Context ID
 *   (>) destId    (const uint32) ID of DSP from which to read data
 *   (>) address   (uint32)       Starting memory address to read
 *   (!) pData     (uint32 *)     Array in which to store data
 *   (>) wordCount (const uint32) Number of words to read
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if one or more DSP address could not be read.
 *
 *   PURPOSE:
 *   Read a block of DSP memory
 *
 *   DESCRIPTION:
 *   This routine performs multiple execution of the SDSU primitive
 *   command 'RDM' in order to read a number of consecutive DSP memory
 *   locations. destId can take one of the values specified in the table below.
 *
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   The number of 24-bit words to read is given by wordCount,
 *   starting from memory location address. The data are copied to the
 *   array pointed to by pData and the most significant byte of each word
 *   written to pData[] is not defined.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuPrimitiveMultiRDM ( SDSU_ID        context,
                                 const uint32   destId,
                                 uint32         address,
                                 uint32         *pData,
                                 const uint32   wordCount )
{
   int            i;

   for (i = 0; i < wordCount; i++)               /* RDM each location in turn */
   {
      if (sdsuPrimitiveRDM (context, destId, address++, & pData [i]) == ERROR)
      {
         ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);   
         return (ERROR);
      }
   }
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuPrimitiveMultiWRM
 *
 *   INVOCATION:
 *   sdsuPrimitiveMultiWRM (context, destId, address, pData, wordCount)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context    (SDSU_ID)        Context ID
 *   (>) destId     (const uint32)   ID of DSP to which data will be written
 *   (>) address    (uint32)         Starting memory address to write
 *   (!) pData      (uint32 *)       Array in which data to write are stored
 *   (>) wordCount  (const uint32)   Number of words to wrote
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if one or more DSP address could not be written.
 *
 *   PURPOSE:
 *   Write a block of DSP memory
 *
 *   DESCRIPTION:
 *   This routine performs multiple execution of the SDSU primitive command
 *   'WRM' in order to write a number of consecutive DSP memory locations.
 *   destId can take one of the values specified in the table below.
 *
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   The number of 24-bit words to write is given by wordCount, starting from
 *   memory location address. The data are copied from the array pointed to
 *   by pData, the most significant byte of which is discarded.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS sdsuPrimitiveMultiWRM ( SDSU_ID       context,
                               const uint32  destId,
                               uint32        address,
                               uint32        *pData,
                               const uint32  wordCount )
{
   int            i;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   for (i = 0; i < wordCount; i++)               /* WRM each location in turn */
   {
      if (sdsuPrimitiveWRM (context, destId, address++, pData [i]) == ERROR)
      {
         ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);
         return (ERROR);
      }
   }
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuMemoryDnload
 *
 *   INVOCATION:
 *   sdsuMemoryDnload (context, fd, destId, address, pStart, pFinish)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context (SDSU_ID)      Context ID
 *   (>) fd      (FILE *)       Pointer to OMF file stream
 *   (>) destId  (const uint32) ID of DSP to which data will be downloaded
 *   (>) pStart  (uint32 *)     Start address to download for each memory space
 *   (>) pFinish (uint32 *)     Final address to download for each memory space
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if an error occured while downloading.
 *
 *   PURPOSE:
 *   Download data from a OMF file (".lod" format) to an SDSU DSP
 *
 *   DESCRIPTION:
 *   This routine downloads DSP56k object code from a OMF file to
 *   a specified DSP in the SDSU controller system. destId can take one
 *   of the values specified in the table below.
 *
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   The OMF file will normally be the output from the Motorola utility
 *   program "cldlod". The arrays pStart and pFinish define the starting and
 *   finishing DSP addresses to which data may be downloaded for each of the
 *   DSP56k's memory spaces (X, Y and P space); these address ranges therefore
 *   allow a mask to be applied to the OMF file such that only certain
 *   memory spaces, and/or a limited range of addresses within each space
 *   are over-written.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   The OMF file must have been opened before this routine is called.
 *
 *   REFERENCES:
 *   A description of the OMF file structure can be found in Chapter 6
 *   of the Motorola DSP Simulator Reference Manual, "DSP Object Module
 *   Format", Published by Motorola 1992.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS sdsuMemoryDnload ( SDSU_ID       context,
                          FILE          *fd,
                          const uint32  destId,
                          uint32        *pStart,
                          uint32        *pFinish )
{
   char      pField [OMF_MAX_CHARS_PER_LINE + 1];
   uint32    memSpace;
   uint32    address;
   uint32    start;
   uint32    finish;
   uint32    startAddress;
   uint32    blockSize;
   uint32    endBlock;
   uint32    value;
   uint32    *pBuffer;
   int       memIndex;
   int       i;
   int       recordType;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   /*
    * Read the first field of the first record in the OMF file and
    * check that this is the "start" record.
    */

   if (fscanf (fd, "%s", pField) == EOF)
   {
      ERROR_SET (0, "Failed to read first field of OMF file", ERROR_LOG_SAVE);
      return (ERROR);
   }

   if ((recordType = sdsu_getRecordType (pField)) != OMF_FIELD_IDENT_START)
   {
      ERROR_SET (S_sdsuLib_OMF_PARSE_ERROR, 
                 "START record not found in OMF file", ERROR_LOG_SAVE);
      return (ERROR);
   }
   else
   {
      /*
       * "start" record found. A start record has this format:
       * "_START <Module id> <Version> <Rev #> <Device #> <Asm Version> 
       * <Comment>"
       * Skip over the 6 unwanted parts of the start record.
       */

      for (i = 0; i < 6; i++)
      {
         if (fscanf (fd, "%s", pField) == EOF)
         {
            ERROR_SET (0, "Failed to skip over start record in OMF file", 
                       ERROR_LOG_SAVE);
            return (ERROR);
         }
      }
   }

   /*
    * Allocate some memory for a working buffer used during downloading. 
    * This size of this buffer is not critical: it should be large enough to 
    * avoid too many file accesses.
    */

   if ((pBuffer = (uint32 *) calloc ((size_t) FILE_LOAD_BUFFER_SIZE, 
                                     sizeof (uint32))) == NULL)
   {
      ERROR_SET (0, "Memory allocation for file load working buffer failed", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Get the first field of the record, then enter a loop parsing each field 
    * in turn until an "end" record is read.
    */

   if (fscanf (fd, "%s", pField) == EOF)
   {
      ERROR_SET (0, "Failed to read first field from OMF file", ERROR_LOG_SAVE);
      return (ERROR);
   }

   while ((recordType = sdsu_getRecordType (pField)) != OMF_FIELD_IDENT_END)
   {
      switch (recordType = sdsu_getRecordType (pField))
      {
         case (OMF_FIELD_IDENT_DATA):

            /*
             * This is a data record. A data record looks like:
             * "_DATA <Memory space> <Address> <Code/data> ..."
             * Read and determine the memory space.
             */

            if (fscanf (fd, "%s", pField) == EOF)
            {
               ERROR_SET (0, "Failed to read field from OMF file", 
                          ERROR_LOG_SAVE);
               cfree ((char *) pBuffer);
               return (ERROR);
            }

            if (sdsu_areStringsSame (pField, "P"))
            {
               memSpace = SDSU_MEM_SPACE_P;
               memIndex = SDSU_MEM_INDEX_P;
            }
            else if (sdsu_areStringsSame (pField, "X"))
            {
               memSpace = SDSU_MEM_SPACE_X;
               memIndex = SDSU_MEM_INDEX_X;
            }
            else if (sdsu_areStringsSame (pField, "Y"))
            {
               memSpace = SDSU_MEM_SPACE_Y;
               memIndex = SDSU_MEM_INDEX_Y;
            }
            else
            {
               ERROR_SET1 (S_sdsuLib_INV_MEM_SPACE,
                           "Invalid memory space, %s, read from OMF file", 
                           ERROR_LOG_SAVE, pField);
               cfree ((char *) pBuffer);
               return (ERROR);
            }

            /********************************************* Read the address ***/

            if (fscanf (fd, "%s", pField) == EOF)
            {
               ERROR_SET (0, "Failed to read field from OMF file", 
                          ERROR_LOG_SAVE);
               cfree ((char *) pBuffer);
               return (ERROR);
            }

            /************ We don't expect another record type at this stage ***/

            if (sdsu_isRecordType (pField))
            {
               ERROR_SET1 (S_sdsuLib_INV_OMF_FIELD,
               "Field %s read from OMF file should be address, not record type",
               ERROR_LOG_SAVE, pField);
               cfree ((char *) pBuffer);
               return (ERROR);
            }

            address = 
            (uint32) sdsu_atoiBase16 (pField);  /* Convert address to numeric */
            if ((memSpace == SDSU_MEM_SPACE_P) && 
               (address >= (SDSU_MEM_START_E & ~SDSU_MEM_SPACE_MASK)) &&
               (address <= (SDSU_MEM_END_E & ~SDSU_MEM_SPACE_MASK)))
            {
               memSpace = SDSU_MEM_SPACE_E;              /* Mark as P/E space */
               memIndex = SDSU_MEM_INDEX_P;
            }
            address |= memSpace;            /* OR mem space ID with addresses */
            start = pStart [memIndex] | memSpace;
            finish = pFinish [memIndex] | memSpace;

            /*********************************** Read first Code/Data value ***/

            if (fscanf (fd, "%s", pField) == EOF)
            {
               ERROR_SET (0, "Failed to read field from OMF file", 
                          ERROR_LOG_SAVE);
               cfree ((char *) pBuffer);
               return (ERROR);
            }
            if (sdsu_isRecordType (pField))
            {
               ERROR_SET1 (S_sdsuLib_INV_OMF_FIELD,
               "Field %s read from OMF file should be code/data value, not record type",
               ERROR_LOG_SAVE, pField);
               cfree ((char *) pBuffer);
               return (ERROR);
            }

            /*
             * Enter a loop reading Code/Data values and processing each until 
             * another field-type is encountered. Another field-type indicates 
             * the end of this data field.
             */

            i = 0;                        /* Reached end of this data record? */
            while (! sdsu_isRecordType (pField))
            {
               if (address >= start &&         /* ...no, save Code/Data value */
                   address <= finish)
               {
                  if (i == 0) startAddress = address;

                  pBuffer [i++] = sdsu_atoiBase16 (pField);
                  if (i == FILE_LOAD_BUFFER_SIZE)
                  {
                     if (sdsuPrimitiveMultiWRM (context, destId, startAddress, 
                                                pBuffer, i) == ERROR)
                     {
                        ERROR_SET1 (0, 
                        "Failed to write block of DSP memory at address %#x",
                        ERROR_LOG_SAVE, startAddress);
                        cfree ((char *) pBuffer);
                        return (ERROR);
                     }
                     i = 0;
                  }
               }
               else if (i != 0)        /* Download Code/Data from the working */
                                       /* buffer whenever the counter, i,     */
                                       /* reaches the end of the buffer       */
               {
                  if (sdsuPrimitiveMultiWRM (context, destId, startAddress, 
                                             pBuffer, i) == ERROR)
                  {
                     ERROR_SET1 (0, 
                     "Failed to write block of DSP memory at address %#x",
                     ERROR_LOG_SAVE, startAddress);
                     cfree ((char *) pBuffer);
                     return (ERROR);
                  }
                  i = 0;
               }
               address++;             /* Increment address and get next       */
                                      /* Code/Data or the next record-type if */
                                      /* reached the end of this data record  */
               if (fscanf (fd, "%s", pField) == EOF)
               {
                  ERROR_SET (0, "Failed to read field from OMF file", 
                             ERROR_LOG_SAVE);
                  cfree ((char *) pBuffer);
                  return (ERROR);
               }
            }
            if (i != 0)                      /* Download anything left in the */
                                             /* working buffer                */
            {
               if (sdsuPrimitiveMultiWRM (context, destId, startAddress, 
                                          pBuffer, i) == ERROR)
               {
                  ERROR_SET1 (0, 
                  "Failed to write block of DSP memory at address %#x",
                  ERROR_LOG_SAVE, startAddress);
                  cfree ((char *) pBuffer);
                  return (ERROR);
               }
            }
            break;

         case (OMF_FIELD_IDENT_SYMBOL):
         
            /* This is a symbol record, which looks like:
             * _SYMBOL <memory space> <symbol definition> ...
             * where each symbol definition consists of
             * <name> I <address>
             * (no idea what the 'I' is for)
             * Save the symbol information for (eventual) use with parameters.
             */
             
            if (fscanf (fd, "%s", pField) == EOF)
            {
               ERROR_SET (0, "Failed to read field from OMF file", 
                          ERROR_LOG_SAVE);
               cfree ((char *) pBuffer);
               return (ERROR);
            }

            if (sdsu_areStringsSame (pField, "P")) 
               memSpace = SDSU_MEM_SPACE_P;
            else if (sdsu_areStringsSame (pField, "X")) 
               memSpace = SDSU_MEM_SPACE_X;
            else if (sdsu_areStringsSame (pField, "Y")) 
               memSpace = SDSU_MEM_SPACE_Y;
            else if (sdsu_areStringsSame (pField, "N")) 
               memSpace = SDSU_MEM_SPACE_NONE;
            else
            {
               ERROR_SET1 (S_sdsuLib_INV_MEM_SPACE, 
                           "Invalid memory space, %s", ERROR_LOG_SAVE,
                           pField);
               cfree ((char *) pBuffer);
               return (ERROR);
            }
            
            /******** The next field is the start of the symbol definitions ***/
            if (fscanf (fd, "%s", pField) == EOF)
            {
               ERROR_SET (S_sdsuLib_INV_OMF_FIELD, 
                          "Failed to read field from OMF file",
                          ERROR_LOG_SAVE);
               cfree ((char *) pBuffer);
               return (ERROR);
            }

            /** As long as it's not a record type, it must be a symbol name ***/
            while (!sdsu_isRecordType (pField))
            {
               char symName[SDSU_SYM_NAME_LEN + 1];
               
               /* Save the name for a moment */
               strncpy(symName, pField, SDSU_SYM_NAME_LEN);
               
               /* Skip the 'I' */
               if (fscanf (fd, "%s", pField) == EOF)
               {
                  ERROR_SET (S_sdsuLib_INV_OMF_FIELD, 
                             "Failed to read field from OMF file",
                             ERROR_LOG_SAVE);
                  cfree ((char *) pBuffer);
                  return (ERROR);
               }

               if (strcmp (pField, "I"))
               {
                  ERROR_SET1 (S_sdsuLib_INV_OMF_FIELD,
                  "Invalid symbol, %s, read from OMF file - expecting \"I\"",
                  ERROR_LOG_SAVE, pField);
                  cfree ((char *) pBuffer);
                  return (ERROR);
               }

               /****************************** Read and convert the address ***/

               if (fscanf (fd, "%s", pField) == EOF)
               {
                  ERROR_SET (S_sdsuLib_INV_OMF_FIELD, 
                             "Failed to read field from OMF file",
                             ERROR_LOG_SAVE);
                  cfree ((char *) pBuffer);
                  return (ERROR);
               }

               address = sdsu_atoiBase16 (pField);
               
               /* If it's P but in the E range ...*/
               if ((memSpace == SDSU_MEM_SPACE_P) && 
                  (address >= (SDSU_MEM_START_E & ~SDSU_MEM_SPACE_MASK)) &&
                  (address <= (SDSU_MEM_END_E & ~SDSU_MEM_SPACE_MASK)))
               {
                  address |= SDSU_MEM_SPACE_E;     /* then mark it as E space */
               }
               else
               {
                  address |= memSpace;   /* OR memory space ID with addresses */
               }
               
               /* Finally make a (unique) entry in the symbol table */
               symRemove(context->paramSyms, symName, (SYM_TYPE) destId);
               if (symAdd(context->paramSyms, symName, (char *) address, 
                          (SYM_TYPE) destId, 0))
               {
                  ERROR_SET1 (0, "Error adding parameter symbol, %s", 
                              ERROR_LOG_SAVE, symName);
                  cfree ((char *) pBuffer);
                  return (ERROR);
               }
               
               /* Read the next symbol name and try again */
               if (fscanf (fd, "%s", pField) == EOF)
               {
                  ERROR_SET (S_sdsuLib_INV_OMF_FIELD, 
                             "Failed to read field from OMF file", 
                             ERROR_LOG_SAVE);
                  cfree ((char *) pBuffer);
                  return (ERROR);
               }
            }
            break;
             
         case (OMF_FIELD_IDENT_BLOCKDATA):

            /*
             * This is a block data record, which looks like:
             * "_BLOCKDATA <Memory space> <Address> <Block size> <Value>"
             * This means fill the memory block with the given Value.
             */

            if (fscanf (fd, "%s", pField) == EOF)
            {
               ERROR_SET (0, "Failed to read field from OMF file", 
                             ERROR_LOG_SAVE);
               cfree ((char *) pBuffer);
               return (ERROR);
            }
            if (sdsu_areStringsSame (pField, "P"))
            {
               memSpace = SDSU_MEM_SPACE_P;
               memIndex = SDSU_MEM_INDEX_P;
            }
            else if (sdsu_areStringsSame (pField, "X"))
            {
               memSpace = SDSU_MEM_SPACE_X;
               memIndex = SDSU_MEM_INDEX_X;
            }
            else if (sdsu_areStringsSame (pField, "Y"))
            {
               memSpace = SDSU_MEM_SPACE_Y;
               memIndex = SDSU_MEM_INDEX_Y;
            }
            else
            {
               ERROR_SET1 (S_sdsuLib_INV_MEM_SPACE, "Invalid memory space, %s",
                           ERROR_LOG_SAVE, pField);
               cfree ((char *) pBuffer);
               return (ERROR);
            }

            /* Read the address */
            if (fscanf (fd, "%s", pField) == EOF)
            {
               ERROR_SET (0, "Failed to read field from OMF file", 
                          ERROR_LOG_SAVE);
               cfree ((char *) pBuffer);
               return (ERROR);
            }
            if (sdsu_isRecordType (pField))
            {
               ERROR_SET1 (S_sdsuLib_INV_OMF_FIELD, 
               "Field %s read from OMF file should be address, not record type",
               ERROR_LOG_SAVE, pField);
               cfree ((char *) pBuffer);
               return (ERROR);
            }

            /* Convert address to numeric */
            address = (uint32) sdsu_atoiBase16 (pField);
            if ((memSpace == SDSU_MEM_SPACE_P) &&
               (address >= (SDSU_MEM_START_E & ~SDSU_MEM_SPACE_MASK)) &&
               (address <= (SDSU_MEM_END_E & ~SDSU_MEM_SPACE_MASK)) )
            {
               memSpace = SDSU_MEM_SPACE_E;              /* Mark as P/E space */
               memIndex = SDSU_MEM_INDEX_P;
            }
            address |= memSpace;            /* OR mem space ID with addresses */
            start = pStart [memIndex] | memSpace;
            finish = pFinish [memIndex] | memSpace;

            if (fscanf (fd, "%s", pField) == EOF)          /* Read block size */
            {
               ERROR_SET (0, "Failed to read field from OMF file", 
                          ERROR_LOG_SAVE);
               cfree ((char *) pBuffer);
               return (ERROR);
            }
            if (sdsu_isRecordType (pField))   
            {
               ERROR_SET1 (S_sdsuLib_INV_OMF_FIELD,
               "Field %s read from OMF file should be block size, not record type",
               ERROR_LOG_SAVE, pField);
               cfree ((char *) pBuffer);
               return (ERROR);
            }
            blockSize = (uint32) sdsu_atoiBase16 (pField);

            if (fscanf (fd, "%s", pField) == EOF)               /* Read value */
            {
               ERROR_SET (0, "Failed to read field from OMF file", 
                          ERROR_LOG_SAVE);
               cfree ((char *) pBuffer);
               return (ERROR);
            }
            if (sdsu_isRecordType (pField))
            {
               ERROR_SET1 (S_sdsuLib_INV_OMF_FIELD,
               "Field %s read from OMF file should be value, not record type",
               ERROR_LOG_SAVE, pField);
               cfree ((char *) pBuffer);
               return (ERROR);
            }
            value = (uint32) sdsu_atoiBase16 (pField);
            endBlock = address + blockSize;

            for (i = 0; (address < endBlock) && (address <= finish); address++)
            {
               if (address >= start)
               {
                  if (i == 0) startAddress = address;

                  pBuffer [i++] = value;
                  if (i == FILE_LOAD_BUFFER_SIZE)
                  {
                     if (sdsuPrimitiveMultiWRM (context, destId, startAddress,
                          pBuffer, i) == ERROR)
                     {
                        ERROR_SET1 (0, 
                        "Failed to write block of DSP memory at address %#x",
                        ERROR_LOG_SAVE, startAddress);
                        cfree ((char *) pBuffer);
                        return (ERROR);
                     }
                     i = 0;
                  }
               }
            }
            if (i != 0)                      /* Download anything left in the */
                                             /* working buffer                */
            {
               if (sdsuPrimitiveMultiWRM (context, destId, startAddress, 
                                          pBuffer, i) == ERROR)
               {
                  ERROR_SET (0, "Failed to write block of DSP memory", 
                             ERROR_LOG_SAVE);
                  cfree ((char *) pBuffer);
                  return (ERROR);
               }
            }

            /* Skip to next record */
            while (!sdsu_isRecordType(pField))
               if (fscanf (fd, "%s", pField) == EOF)
               {
                  ERROR_SET (0, "Failed to read field from OMF file", 
                             ERROR_LOG_SAVE);
                  cfree ((char *) pBuffer);
                  return (ERROR);
               }
            break;

         case (OMF_FIELD_IDENT_START):

            /*
             * Another "start" record was encountered. We don't expect one 
             * at this stage 
             */

            ERROR_SET (S_sdsuLib_OMF_PARSE_ERROR, 
                       "Too many START records in OMF file", ERROR_LOG_SAVE);
            cfree ((char *) pBuffer);
            return (ERROR);

         /*
          * Ignore any other record types that are encountered
          * (as long as they are valid field types)
          */

         case (OMF_FIELD_IDENT_INVALID):

            ERROR_SET (S_sdsuLib_INV_OMF_FIELD, "Invalid record in OMF file", 
                       ERROR_LOG_SAVE);
            cfree ((char *) pBuffer);
            return (ERROR);

         default:

            /* Read and dump any un-supported records */
            if (fscanf (fd, "%s", pField) == EOF)
            {
               ERROR_SET (0, "Failed to read field from OMF file", 
                          ERROR_LOG_SAVE);
               cfree ((char *) pBuffer);
               return (ERROR);
            }
            break;
      }
   }

   /* Finally, free the temporary buffer. */

   cfree ((char *) pBuffer);
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuMemoryUpload
 *
 *   INVOCATION:
 *   sdsuMemoryUpload (context, fd, destId, address, pStart, pFinish)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context (SDSU_ID)      Context ID
 *   (>) fd      (FILE *)       Pointer to OMF file stream
 *   (>) destId  (const uint32) ID of DSP from which data will be uploaded
 *   (>) pStart  (uint32 *)     Start address to upload for each memory space
 *   (>) pFinish (uint32 *)     Final address to upload for each memory space
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if an error occured while uploading.
 *
 *   PURPOSE:
 *   Upload data from an SDSU DSP to a OMF file (".lod" format)
 *
 *   DESCRIPTION:
 *   This routine uploads DSP56k object code from a specified DSP
 *   in the SDSU controller system and writes the code to a file
 *   in OMF format. This file format is compatible with that produced
 *   by the Motorola utility program "cldlod". The arrays pStart and
 *   pFinish define the starting and finishing addresses from which
 *   data will be uploaded for each of the DSP56k's memory spaces (X,
 *   Y and P space). No data will be uploaded from any memory spaces
 *   for which the start address is larger than the finish address.
 *   destId can take one of the values specified in the table below.
 *
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   The OMF file must have been opened before this routine is called.
 *
 *   REFERENCES:
 *   A description of the OMF file structure can be found in Chapter 6
 *   of the Motorola DSP Simulator Reference Manual, "DSP Object Module
 *   Format", Published by Motorola 1992.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS sdsuMemoryUpload ( SDSU_ID      context,
                          FILE         *fd,
                          const uint32 destId,
                          uint32       *pStart,
                          uint32       *pFinish )
{
   int            i;
   int            j;
   uint32         *pBuffer;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Get the range of address to upload in each memory space. Then process 
    * each block of memory as a separate data record.
    */

   for (i = 0; i < SDSU_MEM_INDEX_NDEF; i++)
   {
      switch (i)
      {
         /* 
          * Write the "type" word and the memory-space for the current data 
          * record 
          */

         case (SDSU_MEM_INDEX_P):
            fprintf (fd, "_DATA P %x\n", pStart [i]);
            pStart [i] |= SDSU_MEM_SPACE_P;
            pFinish [i] |= SDSU_MEM_SPACE_P;
            break;

         case (SDSU_MEM_INDEX_X):
            fprintf (fd, "_DATA X %x\n", pStart [i]);
            pStart [i] |= SDSU_MEM_SPACE_X;
            pFinish [i] |= SDSU_MEM_SPACE_X;
            break;

         case (SDSU_MEM_INDEX_Y):
            fprintf (fd, "_DATA Y %x\n", pStart [i]);
            pStart [i] |= SDSU_MEM_SPACE_Y;
            pFinish [i] |= SDSU_MEM_SPACE_Y;
            break;

         default:
            ERROR_SET1 (S_sdsuLib_INV_MEM_SPACE, 
                        "Invalid memory space, %d", ERROR_LOG_SAVE, i);
            return (ERROR);
      }

      if (pStart [i] <= pFinish [i])
      {
         /* Write the start address for the current data record */

         switch (i)
         {
            case (SDSU_MEM_INDEX_P):
               fprintf (fd, "_DATA P %x\n", pStart [i]);
               break;

            case (SDSU_MEM_INDEX_X):
               fprintf (fd, "_DATA X %x\n", pStart [i]);
               break;

            case (SDSU_MEM_INDEX_Y):
               fprintf (fd, "_DATA Y %x\n", pStart [i]);
               break;
         }

         /*
          * Allocate a working buffer for the data (initialising its contents 
          * to zero). 
          */

         if ((pBuffer = 
              (uint32 *) calloc ((size_t) pFinish [i] - pStart [i] + 1,
                                 sizeof (uint32))) == NULL)
         {
            ERROR_SET (0, "Memory allocation for working buffer failed", 
                       ERROR_LOG_SAVE);
            return (ERROR);
         }

         /* Read the data. */   

         if (sdsuPrimitiveMultiRDM (context, destId, pStart [i], pBuffer, 
                                    pFinish [i] - pStart [i] + 1) == ERROR)
         {
            ERROR_SET1 (0, "Failed to read block of DSP memory at address, %#x",
                        ERROR_LOG_SAVE, pStart [i]);
            cfree ((char *) pBuffer);
            return (ERROR);
         }

         /* Write the data to the data-record (file) - formatted nicely */

         for (j = 0; j < pFinish [i] - pStart [i] + 1; j++)
         {
            if (fprintf (fd, "%6x ", pBuffer [j]) < 0)
            {
               ERROR_SET (0, "Failed to write to data record file", 
                          ERROR_LOG_SAVE);
               cfree ((char *) pBuffer);
               return (ERROR);
            }

            if ((j + 1) % 8 == 0)
            {
               if (fprintf (fd, "\n") < 0)
               {
                  ERROR_SET (0, "Failed to write to data record file", 
                             ERROR_LOG_SAVE);
                  cfree ((char *) pBuffer);
                  return (ERROR);
               }
            }
         }
         if ((j % 8) != 0)
         {
            if (fprintf (fd, "\n") < 0)
            {
               ERROR_SET (0, "Failed to write to data record file", 
                          ERROR_LOG_SAVE);
               cfree ((char *) pBuffer);
               return (ERROR);
            }
         }

         cfree ((char *) pBuffer);            /* Done with the working buffer */
      }
   }
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuFileDnload
 *
 *   INVOCATION:
 *   sdsuFileDnload (context, pFileName, destId, limitAdrsRange)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context    (SDSU_ID)      Context ID
 *   (>) pFileName  (char *)       OMF file name
 *   (>) destId     (const uint32) ID of DSP to which data will be downloaded
 *   (>) limitAdrsRange (const BOOL) Switch enables limited address range
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if an error occured while downloading.
 *
 *   PURPOSE:
 *   Download data from a OMF file (".lod" format) to an SDSU DSP
 *
 *   DESCRIPTION:
 *   This routine downloads DSP56k object code from a OMF file to
 *   a specified DSP in the SDSU controller system. The OMF file will
 *   normally be the output from the Motorola utility program "cldlod".
 *   The routine provides similar functionality to sdsuMemoryDnload()
 *   but with a slightly higher-level interface. In this case the
 *   OMF file is opened by the routine so that only the file name is
 *   required. If limitAdrsRange is FALSE the entire OMF file will be
 *   downloaded, while if limitAdrsRange is TRUE the user will be
 *   prompted to enter (at standard input) the starting and finishing
 *   addresses for each DSP memory space (see sdsuMemoryDnload()). A
 *   further difference between this routine and sdsuMemoryDnload() is
 *   that in this case the P memory space is always downloaded in two
 *   stages - during the first pass through the file only those P-space
 *   addresses larger than  minimum value, Pmin, are downloaded, then,
 *   on a second pass through the file any P-space addresses which lie
 *   below Pmin are downloaded. This feature is provided to cope with
 *   situations in which a DSP's interrupt vector table (which occurs
 *   at addresses < Pmin) may need to be over-written only AFTER
 *   one or more corresponding interrupt service routines have been
 *   installed at addresses >= Pmin. Pmin is given by the macro
 *   SDSU_LOW_DNLOAD_ADRS_IN_P, which is defined in the file sdsuLib.h
 *   destId can take one of the values specified in the table below.
 *
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   REFERENCES:
 *   A description of the OMF file structure can be found in Chapter 6
 *   of the Motorola DSP Simulator Reference Manual, "DSP Object Module
 *   Format", Published by Motorola 1992.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS sdsuFileDnload ( SDSU_ID      context,
                        char         *pFileName,
                        const uint32 destId,
                        const BOOL   limitAdrsRange )
{
   char pIoFileName [MAX_CHAR_FILENAME + 1];
   FILE *pIoFile;
   int  i;
   int  j;
   uint32  start [2][SDSU_MEM_INDEX_NDEF];
   uint32  finish [2][SDSU_MEM_INDEX_NDEF];

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Get the OMF file name & open the file */

   strncpy (pIoFileName, pFileName, MAX_CHAR_FILENAME);
   if (strstr (pIoFileName, ".lod") == NULL && 
       strstr (pIoFileName, ".LOD") == NULL)
      strcat (pIoFileName, ".lod");

   if ((pIoFile = fopen (pIoFileName, "r")) == NULL)
   {
      ERROR_SET1 (0, "Failed to open OMF file, %s", ERROR_LOG_SAVE, 
                  pIoFileName);
      return (ERROR);
   }

   /*
    * If the range of addresses to be downloaded to is limited then read 
    * the required ranges into the 1st element of the start[][] and finish[][] 
    * arrays. If the range is un-limited, load the defined start and end 
    * addresses for each memory space.
    */

   if (limitAdrsRange)
   {
      printf ("sdsuFileDnload: Enter address ranges (hex)\n");

      printf ("         P:Start P:End: ");
      scanf ("%x %x", & start [0][SDSU_MEM_INDEX_P],
         &finish [0][SDSU_MEM_INDEX_P]);

      printf ("         X:Start X:End: ");
      scanf ("%x %x", & start [0][SDSU_MEM_INDEX_X],
         &finish [0][SDSU_MEM_INDEX_X]);

      printf ("         Y:Start Y:End: ");
      scanf ("%x %x", & start [0][SDSU_MEM_INDEX_Y],
         &finish [0][SDSU_MEM_INDEX_Y]);
   }
   else
   {
      start [0][SDSU_MEM_INDEX_X] = SDSU_MEM_START_X;
      start [0][SDSU_MEM_INDEX_Y] = SDSU_MEM_START_Y;
      start [0][SDSU_MEM_INDEX_P] = SDSU_MEM_START_P;
      finish [0][SDSU_MEM_INDEX_X] = SDSU_MEM_END_X;
      finish [0][SDSU_MEM_INDEX_Y] = SDSU_MEM_END_Y;
      finish [0][SDSU_MEM_INDEX_P] = SDSU_MEM_END_P;
   }

   /*
    * Mask off SDSU memory-space identifier bits: the dnload routine doesn't
    * need use them.
    */

   for (i = 0; i < 2; i++)
   {
      for (j = 0; j < SDSU_MEM_INDEX_NDEF; j++)
      {
         start  [i][j] &= ~SDSU_MEM_SPACE_MASK;
         finish [i][j] &= ~SDSU_MEM_SPACE_MASK;
      }
   }

   /*
    * Initialise the 2nd element of start[][], finish[][] arrays such that 
    * the start address > finish address, thus disabling any downloading 
    * on the seconds pass through the download routine.
    */

   start [1][SDSU_MEM_INDEX_X] = 1;
   start [1][SDSU_MEM_INDEX_Y] = 1;
   start [1][SDSU_MEM_INDEX_P] = 1;
   finish [1][SDSU_MEM_INDEX_X] = 0;
   finish [1][SDSU_MEM_INDEX_Y] = 0;
   finish [1][SDSU_MEM_INDEX_P] = 0;

   /*
    * The macro SDSU_LOW_DNLOAD_ADRS_IN_P defines a critical address
    * in "P" memory space which must not be downloaded to prior to
    * loading to higher address locations. This may be necessary in
    * some cases because some existing SDSU DSP code makes use of memory
    * locations which should have been reserved for interrupt vectors.
    * If new applications need to use the relevant interrupts, then
    * it will be necessary to install "patches" which branch around
    * the interrupt vector table. To cope with this situation,
    * downloading is performed as a 2-stage process:
    * (i) addresses >= SDSU_LOW_DNLOAD_ADRS_IN_P are first downloaded,
    * (ii) addresses < SDSU_LOW_DNLOAD_ADRS_IN_P are then downloaded.
    *
    * The address range defined by start[0][] to finish[0][] define
    * the range of addresses downloaded on the first pass; the range
    * start[1][] to finish[1][] define the second pass. Note that if
    * on either pass start[n][]>finish[n][] then no downloading to
    * the relevant memory space will be performed. Note also that only
    * "P" memory space is handled in these 2 stages; "X" and "Y"
    * memory are always downloaded in the first pass only.
    */

   if (start [0][SDSU_MEM_INDEX_P] < SDSU_LOW_DNLOAD_ADRS_IN_P)
   {
      /*
       * 2-stage download required, set 1st-pass address range
       * to start & finish no lower than P:SDSU_LOW_DNLOAD_ADRS_IN_P.
       */

      start [1][SDSU_MEM_INDEX_P] = start [0][SDSU_MEM_INDEX_P];
      start [0][SDSU_MEM_INDEX_P] = SDSU_LOW_DNLOAD_ADRS_IN_P;
      finish [1][SDSU_MEM_INDEX_P] = SDSU_LOW_DNLOAD_ADRS_IN_P - 1;

      if (finish [0][SDSU_MEM_INDEX_P] < (SDSU_LOW_DNLOAD_ADRS_IN_P - 1))
      {
         finish [1][SDSU_MEM_INDEX_P] = finish [0][SDSU_MEM_INDEX_P];
         finish [0][SDSU_MEM_INDEX_P] = SDSU_LOW_DNLOAD_ADRS_IN_P - 1;
      }
   }

   for (i = 0; i < 2; i++)                                 /* Do the download */
   {
      if (sdsuMemoryDnload (context, pIoFile, destId, & start [i][0], 
                            & finish [i][0]) == ERROR)
      {
         ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);
         fclose (pIoFile);
         return (ERROR);
      }
      rewind (pIoFile);
   }
   printf ("%s downloaded OK\n", pIoFileName);

   if (fclose (pIoFile) == ERROR)
   {
      ERROR_SET (0, "Failed to close OMF file", ERROR_LOG_SAVE);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuFileUpload
 *
 *   INVOCATION:
 *   sdsuFileUpload (context, pFileName, destId, limitAdrsRange)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context   (SDSU_ID)         Context ID
 *   (>) pFileName (char *)          OMF file name
 *   (>) destId    (const uint32)    ID of DSP from which data will be uploaded
 *   (>) limitAdrsRange (const BOOL) Switch enables limited address range
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if an error occured while uploading.
 *
 *   PURPOSE:
 *   Upload data from an SDSU DSP to a OMF file (".lod" format)
 *
 *   DESCRIPTION:
 *   This routine uploads DSP56k object code from a OMF file to
 *   a specified DSP in the SDSU controller system and writes the code
 *   to a file in OMF format. This file format is compatible with that
 *   produced by the Motorola utility program "cldlod". The routine
 *   provides similar functionality to sdsuMemoryUpload() but with a
 *   slightly higher-level interface. In this case the OMF file is
 *   created and opened by the routine so that only the file name is
 *   required. If limitAdrsRange is FALSE the entire OMF file will be
 *   downloaded, while if limitAdrsRange is TRUE the user will be
 *   prompted to enter (at standard input) the starting and finishing
 *   addresses for each DSP memory space (see sdsuMemoryUpload()).
 *   destId can take one of the values specified in the table below.
 *
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   REFERENCES:
 *   A description of the OMF file structure can be found in Chapter 6
 *   of the Motorola DSP Simulator Reference Manual, "DSP Object Module
 *   Format", Published by Motorola 1992.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS sdsuFileUpload ( SDSU_ID         context,
                        char            *pFileName,
                        const uint32    destId,
                        const BOOL      limitAdrsRange )
{
   int      j;
   char     pIoFileName [MAX_CHAR_FILENAME + 1];
   FILE     *pIoFile;
   uint32   start [SDSU_MEM_INDEX_NDEF];
   uint32   finish [SDSU_MEM_INDEX_NDEF];

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   strncpy (pIoFileName, pFileName, MAX_CHAR_FILENAME);
   if ((strstr (pIoFileName, ".lod") == NULL) 
       && (strstr (pIoFileName, ".LOD") == NULL))
   {
      strcat (pIoFileName, ".lod");
   }

   if ((pIoFile = fopen (pIoFileName, "w")) == NULL)
   {
      ERROR_SET1 (0, "Failed to open OMF file, %s", ERROR_LOG_SAVE, 
                  pIoFileName);
      return (ERROR);
   }

   /*
    * If the range of addresses to be downloaded to is limited then read 
    * the required ranges into the 1st element of the start[][] and finish[][] 
    * arrays. If the range is un-limited, load the defined start and end 
    * addresses for each memory space.
    */

   if (limitAdrsRange)
   {
      printf ("sdsuFileDnload: Enter address ranges (hex)\n");

      printf ("         P:Start P:End: ");
      scanf ("%x %x", & start [SDSU_MEM_INDEX_P], & finish [SDSU_MEM_INDEX_P]);

      printf ("         X:Start X:End: ");
      scanf ("%x %x", & start [SDSU_MEM_INDEX_X], & finish [SDSU_MEM_INDEX_X]);

      printf ("         Y:Start Y:End: ");
      scanf ("%x %x", & start [SDSU_MEM_INDEX_Y], & finish [SDSU_MEM_INDEX_Y]);
   }
   else
   {
      start [SDSU_MEM_INDEX_X] = SDSU_MEM_START_X;
      start [SDSU_MEM_INDEX_Y] = SDSU_MEM_START_Y;
      start [SDSU_MEM_INDEX_P] = SDSU_MEM_START_P;
      finish [SDSU_MEM_INDEX_X] = SDSU_MEM_END_X;
      finish [SDSU_MEM_INDEX_Y] = SDSU_MEM_END_Y;
      finish [SDSU_MEM_INDEX_P] = SDSU_MEM_END_P;
   }

   /*
    * Mask off SDSU memory-space identifier bits: the upload routine 
    * doesn't need use them.
    */

   for (j = 0; j < SDSU_MEM_INDEX_NDEF; j++)
   {
      start  [j] &= ~SDSU_MEM_SPACE_MASK;
      finish [j] &= ~SDSU_MEM_SPACE_MASK;
   }

   /* Do the upload */

   if (sdsuMemoryUpload (context, pIoFile, destId, & start [0], & finish [0]) 
       == ERROR)
   {
      fclose (pIoFile);
      ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);
      return (ERROR);
   }
   printf ("Uploaded OK\n");

   if (fclose (pIoFile) == ERROR)
   {
      ERROR_SET (0, "Failed to close OMF file", ERROR_LOG_SAVE);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuBufferCreate
 *
 *   INVOCATION:
 *   sdsuBufferCreate (context, pixelsPerFrame, nFrames)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) context        (SDSU_ID)     SDSU context structure
 *   (>) pixelsPerFrame (uint32)      Maximum number of pixels per frame
 *   (>) nFrames        (int)         Number of frames
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if an error occurred or the buffer could not be 
 *              allocated.
 *
 *   PURPOSE:
 *   Allocate a buffer capable of holding SDSU data
 *
 *   DESCRIPTION:
 *   This routine allocates a block of memory capable of holding nFrames frames
 *   of SDSU data of up to pixelsPerFrame pixels each, and writes information
 *   describing the buffer into the SDSU context structure.
 *   This routine must not be used while readouts are taking place on the given
 *   controller.
 *
 *   SEE ALSO:
 *   sdsuBufferDelete
 *   sdsuFrameFind
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   REFERENCES:
 *   See Gemini Interface Control Document ICD 1.6/1.10 for a description of 
 *   the SDSU data packet format.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS sdsuBufferCreate ( SDSU_ID context,
                          uint32  pixelsPerFrame,
                          int     nFrames )
{
   uint32         frameSize;                /* Number of bytes for each frame */
   uint32         bufferSize;               /* Size of data buffer in bytes   */
   int            frame;                    /* Frame number                   */
   char           *pBuffer;                 /* Address calculation pointer    */
   SDSU_FRAME     *pFrame = 0;              /* Pointer to a frame             */

   /* Check whether the SDSU context provided is valid. */

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Check if a data buffer has already been allocated.
    */

   if (context->pDataBuffer != NULL)
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Data buffer already allocated", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Calculate the size of the data buffer needed to hold the specified number
    * of frames. The data header size is calculated by subtracting the pointer 
    * to the beginning of the SDSU_FRAME structure from the pointer to the 
    * beginning of the data, assuming that sizeof(char) is one byte.
    */

   frameSize = 
   ((char *) & pFrame->pixel[0]) - ((char *) pFrame) +         /* Header size */
   (SDSU_NBYTE_PER_PIXEL * pixelsPerFrame);                 /* plus data size */

   frameSize += frameSize % 4;               /* Frames must be 4-byte aligned */

   bufferSize = frameSize * nFrames;

#ifdef DEBUG
   printf ("sdsuBufferCreate: Allocating %lu bytes of memory for data buffer (%d frames of %lu bytes).\n",
           bufferSize, nFrames, frameSize);
#endif

   /*
    * Allocate cache-safe memory for the data buffer.
    */

   pBuffer = cacheDmaMalloc (bufferSize);
   if (pBuffer == NULL)
   {
      ERROR_SET (0, "Memory allocation for data buffer failed", ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf ("sdsuBufferCreate: Local address of data buffer = %p\n", pBuffer);
#endif

   /*
    * Create a counting semaphore to look after the data buffer as a whole.
    * This allows a task to wait until a frame becomes available.
    * The semaphore is initialised with the number of free frames.
    */

   context->bufferSem = semCCreate (SEM_Q_FIFO, nFrames);
   if (context->bufferSem == NULL)
   {
      ERROR_SET (0, "Buffer semaphore creation failed", ERROR_LOG_SAVE);
      cacheDmaFree (pBuffer);
      return (ERROR);
   }

   /*
    * Create a message queue for communication within the readout task, giving 
    * the message queue a buffer of sufficient size to allow all the available 
    * frames to be queued.
    */
   
   context->frameQueue = msgQCreate (nFrames, sizeof (SDSU_FRAME *), 
                                     MSG_Q_FIFO);
   if (context->frameQueue == NULL)
   {
      ERROR_SET (0, "Frame message queue creation failed", ERROR_LOG_SAVE);
      semDelete (context->bufferSem);
      cacheDmaFree (pBuffer);
      return (ERROR);
   }

   /*
    * Update the context structure and create the frameSem semaphores.
    */

   context->pDataBuffer       = pBuffer;
   context->maxPixelsPerFrame = pixelsPerFrame;
   context->frameSize         = frameSize;
   context->nFrames           = nFrames;
   context->pFreeList         = NULL;      /* Start empty */

   for (frame = 0; frame < nFrames; frame++, pBuffer += frameSize)
   {
      pFrame = (SDSU_FRAME *) (int) pBuffer;
      
      /*
       * Each frame has an associated counting semaphore which holds its
       * current reservation count.  This starts at zero.
       */
      
      pFrame->frameSem = semCCreate (SEM_Q_FIFO, 0);
      if (pFrame->frameSem == NULL)
      {
         /* Bother, it failed. Got to release everything again */
         ERROR_SET (0, "Frame semaphore creation failed", ERROR_LOG_SAVE);
         
         while (--frame >= 0)
         {
            pBuffer -= frameSize;
            pFrame = (SDSU_FRAME *) (int) pBuffer;
            
            semDelete (pFrame->frameSem);
         }
         msgQDelete (context->frameQueue);
         semDelete (context->bufferSem);
         cacheDmaFree (context->pDataBuffer);
         
         context->frameQueue = NULL;
         context->pDataBuffer = NULL;
         context->pFreeList  = NULL;
         
         return (ERROR);
      }

#ifdef DEBUG
      /* Fill the frame with recognisable values for debugging purposes */

      {
         int   pixel;
         for ( pixel=0; pixel < pixelsPerFrame; pixel++ )
         {
            pFrame->pixel[pixel] = (uint16) SDSU_DATA_EMPTY;
         }
      }
#endif /* DEBUG */
      
      /* Add this frame to the free list */
      pFrame->pNext = context->pFreeList;
      context->pFreeList = pFrame;
   }
   
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuBufferDelete
 *
 *   INVOCATION:
 *   sdsuBufferDelete( context )
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) context (SDSU_ID) SDSU context structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the data-definition structure is invalid.
 *
 *   PURPOSE:
 *   Free local memory previously allocated by sdsuBufferCreate()
 *
 *   DESCRIPTION:
 *   This routine de-allocates memory which has been previously allocated
 *   for detector readout (with a previous call to sdsuBufferCreate()). The
 *   data-buffer pointers are marked as invalid and cannot be re-used until
 *   a new buffer is allocated with a subsequent call to sdsuBufferCreate().
 *
 *   SEE ALSO:
 *   sdsuBufferCreate
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   The data-buffer pointers must have been initialised with a previous call 
 *   to sdsuBufferCreate().
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   All frames reserved with sdsuFrameReserve will be lost when this function 
 *   is executed.
 *-
 */

STATUS   sdsuBufferDelete ( SDSU_ID context )
{
   char *         pBuffer;                     /* Address calculation pointer */
   SDSU_FRAME *   pFrame = 0;                  /* Pointer to a frame          */
   int            frame;

   /* Check whether the SDSU context provided is valid. */

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Check that a data buffer has already been allocated.
    */

   if (context->pDataBuffer == NULL)
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Data buffer not allocated", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Delete the data buffer semaphore.
    */

   semDelete (context->bufferSem);

   /*
    * Delete all the frame semaphores
    */

   pBuffer = context->pDataBuffer;
   
   for (frame = 0; frame < context->nFrames; 
        frame++, pBuffer += context->frameSize)
   {
      pFrame = (SDSU_FRAME *) (int) pBuffer;
      semDelete (pFrame->frameSem);
   }

   /*
    * Free the data buffer and clear the contents of the context structure.
    */

#ifdef DEBUG
   printf (
   "sdsuBufferDelete: Freeing data buffer at %p\n", context->pDataBuffer);
#endif 

   msgQDelete (context->frameQueue);
   cacheDmaFree (context->pDataBuffer);
   
   context->frameQueue = NULL;
   context->pDataBuffer = NULL;
   context->maxPixelsPerFrame = 0;
   context->nFrames = 0;

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuFrameFind
 *
 *   INVOCATION:
 *   sdsuFrameFind (context, timeout, ppFrame)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context (SDSU_ID)       SDSU context structure
 *   (>) timeout (const int)     Timeout in system clock ticks
 *   (<) ppFrame (SDSU_FRAME **) Where to return the frame buffer pointer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if an error occurred or a frame could not be found.
 *
 *   PURPOSE:
 *   Finds a spare image frame from the data buffer pool
 *
 *   DESCRIPTION:
 *   This routine finds a spare frame from the previously created data buffer 
 *   and returns a pointer to it.  Frames are managed on a free-list, so the
 *   next free frame is obtained without having to perform a linear search of
 *   the pool.  If a timeout is specified the function will wait until a spare
 *   frame becomes available, or until the timeout elapses, if it does not 
 *   find a frame free immediately. The function will not wait if NO_WAIT is 
 *   specified and will wait forever if WAIT_FOREVER is specified.
 *
 *   sdsuFrameFind is available for applications which use their own custom
 *   readout task. Applications which use the standard sdsuLib readout task,
 *   (started by calling sdsuReadoutOpen) do not need to use sdsuFrameFind at 
 *   all, as it will be called automatically by the readout task before each 
 *   frame callback.
 *
 *   EXAMPLE:
 *v
 *v  sdsuFrameFind    - Finds a frame (called by readout task)
 *v  sdsuFrameReserve - Reserves the frame (application code)
 *v  sdsuFrameReserve - Reserves the frame again (application code)
 *v  sdsuFrameRelease - Reduces the reservation count (application code)
 *v  sdsuFrameRelease - Reduces the reservation count again (application code)
 *v  sdsuFrameRelease - Only now is the frame returned to the free list 
 *v                     (readout task)
 *v
 *
 *   SEE ALSO:
 *   sdsuBufferCreate
 *   sdsuFrameReserve
 *   sdsuFrameRelease
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   A data buffer must have been allocated with sdsuBufferCreate before calling
 *   this function.
 *
 *   REFERENCES:
 *   See Gemini Interface Control Document ICD 1.6/1.10 for a description of 
 *   the SDSU data packet format.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS sdsuFrameFind ( SDSU_ID    context,
                       const int  timeout,
                       SDSU_FRAME **ppFrame )
{
   int        key;
   SDSU_FRAME *pFrame;
   
   /* Check whether the SDSU context provided is valid. */

#ifdef DEBUG
   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Check that a data buffer has already been allocated.
    */

   if (context->pDataBuffer == NULL)
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Data buffer not allocated", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
#endif

#ifdef USE_FRAME_FIND_SEMAPHORE              /* Commented out due to problems */
   /*
    * Attempt to take, with the specified timeout. the counting semaphore 
    * controlling the free list.
    * This semaphore can be taken as long as there is at least one frame on 
    * the free list.
    */

   if ( semTake (context->bufferSem, timeout) == ERROR )
   {
      ERROR_SET (S_sdsuLib_SYNC_ERROR, "No unreserved frames available", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
#endif /* USE_FRAME_FIND_SEMAPHORE */

   /*
    * Double check that there is at least one frame on the free list.
    * If there isn't then something has gone wrong with the bufferSem semaphore
    * protocol.
    */

   if (context->pFreeList == NULL)
   {
      ERROR_SET (S_sdsuLib_SYNC_ERROR,
         "Took buffer semaphore but still no unreserved frames available",
         ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   /*
    * Take the frame from the free list and change the free list pointer 
    * to point to the next frame in the list.
    * This next bit must be uninterruptible, so interrupts and task scheduling
    * are disabled.
    */

   taskLock();
   key = intLock();
   
   pFrame = context->pFreeList;
   context->pFreeList = pFrame->pNext;
   
   /* Done, interrupts and task rescheduling allowed again */
   intUnlock (key);
   taskUnlock();

   /*
    * Initialise the frame header and set the pNext pointer to NULL to show 
    * this frame is no longer part of the free list.
    */

   pFrame->pNext = NULL;
   pFrame->header.packetCount = 0;
   pFrame->header.status      = 0;
   pFrame->header.parameterId = 0;
   pFrame->header.frameCount  = 0;

#ifdef DEBUG
   printf ("sdsuFrameFind: Found frame at local address %p\n", pFrame);
#endif

   *ppFrame = pFrame;
   
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuFrameReserve
 *
 *   INVOCATION:
 *   sdsuFrameReserve (context, pFrame)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context (SDSU_ID)      SDSU context structure
 *   (>)   pFrame  (SDSU_FRAME *) Pointer to frame buffer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if parameter checks fail
 *
 *   PURPOSE:
 *   Reserve the frame which has been obtained from sdsuFrameFind
 *
 *   DESCRIPTION:
 *   This routine reserves the frame which has already been selected from 
 *   the allocated data buffer by a previous call to sdsuFrameFind. 
 *   The purpose of this is to flag the data as valuable to prevent the frame 
 *   from being re-used until other operations on it are completed. The 
 *   function can be called as many times as required by different tasks. 
 *   The effect of the function is reversed by calls to sdsuFrameRelease.
 *
 *   The function is designed to be used within application code executed as 
 *   a callback by sdsu_readTask. sdsu_readTask will automatically return 
 *   a frame to the free list unless it is specifically reserved by the 
 *   application code.
 *
 *   EXAMPLE:
 *v
 *v      sdsuFrameFind      - Finds a frame
 *v      sdsuFrameReserve   - Reserves the frame
 *v      sdsuFrameReserve   - Reserves the frame again
 *v      sdsuFrameRelease   - Reduces the reservation count
 *v      sdsuFrameRelease   - Reduces the reservation count again
 *v      sdsuFrameRelease   - Only now is the frame returned to the free list
 *v
 *
 *   SEE ALSO:
 *   sdsuFrameFind
 *   sdsuFrameRelease
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   A data buffer should be allocated with sdsuBufferCreate, and the frame
 *   obtained from sdsuFrameFind before calling this function.
 *
 *   REFERENCES:
 *   See Gemini Interface Control Document ICD 1.6/1.10 for a description of 
 *   the SDSU data packet format.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   We don't actually need the context, but we take it anyway to keep the user
 *   aware that it's important to associate the two.
 *-
 */

STATUS sdsuFrameReserve ( SDSU_ID    context,
                          SDSU_FRAME *pFrame )
{
#ifdef DEBUG
   /* Check whether the SDSU context provided is valid. */

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Check that a data buffer has already been allocated.
    */

   if (context->pDataBuffer == NULL)
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Data buffer not allocated", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   
#endif
   /*
    * The frame pointer is in error if its NULL.
    */
   
   if (pFrame == NULL)
   {
      ERROR_SET (S_sdsuLib_INV_PARAM_VAL, "Invalid frame pointer", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Increment the frame usage count by one.
    * If semGive returns ERROR the semaphore was not a proper SEM_ID.
    */
   
   if (semGive (pFrame->frameSem) == ERROR)
   {
      ERROR_SET (S_sdsuLib_INV_PARAM_VAL, "Invalid frame semaphore", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf ("sdsuFrameReserve: Reserved frame at local address %p\n", pFrame);
#endif

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuFrameRelease
 *
 *   INVOCATION:
 *   sdsuFrameRelease (context, pFrame)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context  (SDSU_ID)        SDSU context structure
 *   (>)   pFrame   (SDSU_FRAME *)   Frame pointer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if an error occurred or the frame could not be 
 *              released.
 *
 *   PURPOSE:
 *   Releases frame reservation and returns it to free list if no longer in use
 *
 *   DESCRIPTION:
 *   This routine releases a frame previously obtained from sdsuFrameFind and
 *   possibly reserved any number of times by sdsuFrameReserve.  The reservation
 *   count is reduced by one, and if it reaches zero the frame is added onto
 *   the free list for the controller.
 *
 *   EXAMPLE:
 *v
 *v      sdsuFrameFind      - Finds a frame
 *v      sdsuFrameReserve   - Reserves the frame
 *v      sdsuFrameReserve   - Reserves the frame again
 *v      sdsuFrameRelease   - Reduces the reservation count
 *v      sdsuFrameRelease   - Reduces the reservation count again
 *v      sdsuFrameRelease   - Only now is the frame returned to the free list
 *v
 *
 *   SEE ALSO:
 *   sdsuFrameFind
 *   sdsuFrameReserve
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   The frame must have been obtained from sdsuFrameFind before calling this
 *   function.
 *
 *   REFERENCES:
 *   See Gemini Interface Control Document ICD 1.6/1.10 for a description of 
 *   the SDSU data packet format.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   No check is done that this frame was originally obtained from this context
 *   structure and data buffer.  Application software must ensure that the 
 *   correct context is used when releasing frames as otherwise extreme 
 *   confusion and loss of data may result!
 *
 *   This function assumes that any error from semTake() means that the 
 *   counting semaphore is empty and the frame can be returned to the free list. *   On rare occasions this error might be due to an invalid semaphore ID. 
 *   Ideally, this function should check the errno variable to distinguish the 
 *   kind of error that has happened. SMB - 14 December 1998.
 *-
 */

STATUS   sdsuFrameRelease ( SDSU_ID    context,
                            SDSU_FRAME *pFrame )
{
   STATUS         status;

   int            i;
   BOOL           found;
   SDSU_FRAME     *pTestFrame;

   /* Check whether the SDSU context provided is valid. */

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Check that a data buffer has already been allocated and we were given a
    * non-NULL frame pointer.
    */

   if (context->pDataBuffer == NULL)
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Data buffer not allocated", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   if (pFrame == NULL)
   {
      ERROR_SET (S_sdsuLib_INV_PARAM_VAL, "Invalid frame pointer", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf ("sdsuFrameRelease: Releasing frame at local address %p\n", pFrame);
#endif /* DEBUG */

   /*
    * Reduce the frame usage count by one.
    * The call below will return ERROR when the count has reached zero, 
    * indicating that the frame is not reserved and can be returned to the 
    * free list.
    */

   status = semTake (pFrame->frameSem, NO_WAIT);

   if (status == ERROR)
   {
      /* Not reserved, we can return this frame to the free list */

      /*
       * First check this frame is not already present on the free list, 
       * which might happen if sdsuFrameRelease is called too many times.
       */

      found = FALSE;
      pTestFrame = context->pFreeList;
      for (i=0; ( (!found) && (pTestFrame != NULL) && (i < context->nFrames)); 
           i++)
      {
         if ( pTestFrame == pFrame ) found = TRUE;            
         pTestFrame = pTestFrame->pNext;
      }

      if ( !found )
      {
         int key;

#ifdef DEBUG
         printf ("sdsuFrameRelease: Returning frame to the free list\n");
#endif 

         /* Increase the frame available count by one. */
         semGive (context->bufferSem);

         /* Do this next bit atomically */
         taskLock();
         key = intLock();
      
         pFrame->pNext = context->pFreeList;
         context->pFreeList = pFrame;
      
         /* All complete */
         intUnlock (key);
         taskUnlock();
      }
      else
      {
         ERROR_SET (S_sdsuLib_INV_STRUCTURE, 
                    "Frame is already on the free list", ERROR_LOG_SAVE);
         return (ERROR);
      }
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuFrameShow
 *
 *   INVOCATION:
 *   sdsuFrameShow (pFrame)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pFrame (SDSU_FRAME *)   Frame pointer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if an error occurred.
 *
 *   PURPOSE:
 *   Display the contents of an SDSU frame structure
 *
 *   DESCRIPTION:
 *   This routine displays the contents of the frame structure pointed to by 
 *   pFrame.
 *
 *   NOTE:
 *   This function is useful when invoked from the VxWorks shell
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   The frame buffer must have been allocated using sdsuFrameAlloc.
 *
 *   REFERENCES:
 *   See Gemini Interface Control Document ICD 1.6/1.10 for a description of 
 *   the SDSU data packet format.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS sdsuFrameShow ( SDSU_FRAME *pFrame )
{
   int            pixel;
   
   /*
    * Check that we have been given a non-NULL frame pointer.
    */
   
   if (pFrame == NULL)
   {
      ERROR_SET (S_sdsuLib_INV_PARAM_VAL, "Invalid frame pointer", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Display the contents of this frame. */

   printf ("Contents of SDSU frame at %p:\n", pFrame);
   printf ("-------------------------------------\n");
   printf ("Next frame            : %p\n",  pFrame->pNext);
   printf ("Packet count          : %lu\n",  pFrame->header.packetCount);
   printf ("Frame count           : %lu\n",  pFrame->header.frameCount);
   printf ("Frame status          : %#x\n", pFrame->header.status);
   printf ("Parameter set ID      : %lu\n",  pFrame->header.parameterId);
   printf ("Frame semaphore ID    : %#x\n", (uint32) pFrame->frameSem);
   printf ("Local address of data : %p\n",  pFrame->pixel);

   printf ("First 8 pixels: ");
   for (pixel=0; pixel<8; pixel++) printf ("%d ", pFrame->pixel[pixel]);
   printf ("\n");

   printf ("\n");         /* Blank line for spacing. */

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuFrameSetFBA
 *
 *   INVOCATION:
 *   sdsuFrameSetFBA (context, pFrame)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context   (SDSU_ID)        SDSU context structure
 *   (>)   pFrame    (SDSU_FRAME *)   Pointer to frame
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if an error occurred
 *
 *   PURPOSE:
 *   Sets VME card the Frame Buffer Address (FBA) to the given frame
 *
 *   DESCRIPTION:
 *   Converts the frame pointer into an address suitable for the SDSU VME card
 *   and write it to the relevent DSP parameter locations.
 *   Also writes pFrame into context->readFrame.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   REFERENCES:
 *   See Gemini Interface Control Document ICD 1.6/1.10 for a description of 
 *   the SDSU data packet format.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuFrameSetFBA ( SDSU_ID         context,
                           SDSU_FRAME *   pFrame )
{
   uint32          dmaAddress;                 /* Bus address of image buffer */


   /* 
    * Check whether the SDSU context and frame buffer address 
    * provided are valid
    */
#ifdef DEBUG
   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, 
                 "Invalid SDSU context", ERROR_LOG_SAVE);
      return (ERROR);
   }

   if (pFrame == NULL)
   {
      ERROR_SET (S_sdsuLib_INV_PARAM_VAL, 
                 "Frame pointer not provided", ERROR_LOG_SAVE);
      return (ERROR);
   }
#endif
   /*
    * Convert the address of the frame header into a VMEbus address.
    * Note that the address given is the start of the data header in the 
    * SDSU_FRAME structure.
    * When the SDSU controller writes its data it will automatically overwrite 
    * the pFrame->header structure as well as the memory pointed to by 
    * pFrame->pixel.
    */

   if (sysLocalToBusAdrs (SDSU_AM_VME_MASTER_DATA, (char *) & pFrame->header, 
                          (char **) & dmaAddress) == ERROR)
   {
      ERROR_SET (0, "Failed to map frame buffer address to VME bus",
                 ERROR_LOG_SAVE);
      return ERROR;
   }

#ifdef DEBUG
   printf ("sdsuFrameSetFBA: Frame buffer at local address %p maps to VME bus address %#x.\n",
           pFrame, dmaAddress);
#endif 

   /*
    * Load the high and low parts of the frame buffer address into 
    * the VME DSP, setting the SDSU_NEW_FBA_FLAG bit to make the DSP 
    * recognise that a new address has been loaded.
    */

   if ( (sdsuParamWrite (context, SDSU_IDENT_VME, "V_FBALO", 
                         dmaAddress & 0xffff) == ERROR) ||
       (sdsuParamWrite (context, SDSU_IDENT_VME, "V_FBAHI",
                       ((dmaAddress >> 16) | SDSU_NEW_FBA_FLAG)) == ERROR))
   {
      ERROR_SET (0, "Frame buffer address could not be set in VME DSP", 
                 ERROR_LOG_SAVE);
      return ERROR;
   }

   /* Finally record the new frame in the context structure */
   context->readFrame = pFrame;
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuIntConnect
 *
 *   INVOCATION:
 *   sdsuIntConnect (context, intEnable)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context   (SDSU_ID)  SDSU Context structure
 *   (>) intEnable (uint32)   Interrupts to enable mask
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if an error occurred
 *
 *   PURPOSE:
 *   Connect to SDSU controller interrupts 
 *
 *   DESCRIPTION:
 *   This routine allocates two interrupt vectors and connects the packet and
 *   frame synchronisation interrupts of an SDSU controller up to the relevent
 *   internal routines for correct operation of the sdsuReadout process.  The
 *   VME DSP is told what vector numbers to use, and interrupts are enabled on
 *   both the CPU (for the level indicated by SDSU_VME_INT_LEVEL in sdsuLib.h)
 *   and the SDSU VME Interface card. The intEnable parameter determines which
 *   interrupts are enabled and can be constructed by ORing together any
 *   combination of the following macros.
 *
 *      SDSU_FRAME_INT_ENABLE   =>   Enables frame interrupts
 *      SDSU_PACKET_INT_ENABLE  =>   Enables packet interrupts
 *
 *   EXTERNAL VARIABLES:
 *   (>) sdsuAtomicSem      (SEM_ID)   Semaphore for exclusive access
 *   (!) sdsuVectorNumber   (int)      Vector number allocation
 *
 *   PRIOR REQUIREMENTS:
 *   VME DSP application code revision 0.17 or later.
 *
 *   REFERENCES:
 *   See Gemini Interface Control Document ICD 1.6/1.10 for a description of the
 *   SDSU controller software interface.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   There is no way of confirming that the interrupt level SDSU_VME_INT_LEVEL 
 *   is correct for this particular SDSU VME card, because it's set by jumper.
 *   
 *   It is possible (in fact quite easy) to run out of interrupt vectors, as
 *   these are not recovered for re-use when an SDSU context is deleted. Even if
 *   they were, there would still be a memory leak because the vxWorks
 *   intConnect routine allocates memory for the interrupt handler, but there is
 *   no way to release this.
 *
 *   I have modified this function so that interrupt vectors are not allocated 
 *   for interrupts that are not enabled. This should delay the running out 
 *   of vectors. SMB - 3 December 1998.
 *
 *   It is possible for this function to wait forever for the sdsuAtomicSem 
 *   semaphore. A timeout should be used if this ever becomes a problem.
 *-
 */

STATUS   sdsuIntConnect ( SDSU_ID    context,
                          uint32     intEnable )
{
   /*
    * Check whether the SDSU context provided is valid.
    */

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Check we haven't run out of interrupts.
    */

   if (sdsuVectorNumber >= SDSU_INT_NUMBER_LIMIT)
   {
      ERROR_SET (S_sdsuLib_NO_INT_AVAILABLE, "Run out of interrupt vectors", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Ensure exclusive access to the sdsuLib global variables.
    */

   semTake (sdsuAtomicSem, WAIT_FOREVER);

   /*
    * Allocate a vector number and connect handlers for packet interrupts,
    * if requested and we have not already done so.
    */

   if ( ((intEnable & SDSU_PACKET_INT_ENABLE) != 0) && 
        (context->packetIntNum == 0) )
   {

      /*
       * Get an interrupt vector number from sdsuVectorNumber, ensuring
       * exclusive access to this variable during allocation.
       */
      
      context->packetIntNum = sdsuVectorNumber++;

#ifdef DEBUG
      printf ("sdsuIntConnect: Connecting packet interrupts to vector %d.\n",
         context->packetIntNum);
#endif   

      /*
       * Connect the appropriate interrupt service routine to this vector number
       */
      
      if ( intConnect (INUM_TO_IVEC (context->packetIntNum), 
                       sdsu_packetSyncIsr, (int) context) == ERROR )
      {
         ERROR_SET (0, "Failed to connect packet interrupt service routine", 
                    ERROR_LOG_SAVE);

         /* Cancel vector number allocation if the connection failed */

         sdsuVectorNumber = context->packetIntNum;
         semGive (sdsuAtomicSem);
         context->packetIntNum = 0;
         return (ERROR);
      }
   }

   /*
    * Allocate a vector number and connect handlers for frame interrupts,
    * if requested and we have not already done so.
    */

   if ( ((intEnable & SDSU_FRAME_INT_ENABLE) != 0) && 
        (context->frameIntNum == 0) )
   {

      /*
       * Get an interrupt vector number from sdsuVectorNumber, ensuring
       * exclusive access to this variable during allocation.
       */
      
      context->frameIntNum = sdsuVectorNumber++;

#ifdef DEBUG
      printf (
      "sdsuIntConnect: Connecting frame interrupts to vector %d (%p).\n",
      context->frameIntNum, INUM_TO_IVEC (context->frameIntNum));
#endif

      /*
       * Connect the appropriate interrupt service routine to this vector number
       */
      
      if ( intConnect (INUM_TO_IVEC (context->frameIntNum), sdsu_frameSyncIsr, 
                       (int) context) == ERROR )
      {
         ERROR_SET (0, "Failed to connect frame interrupt service routine", 
                    ERROR_LOG_SAVE);

         /* Cancel vector number allocation if the connection failed */

         sdsuVectorNumber = context->frameIntNum;
         semGive (sdsuAtomicSem);
         context->frameIntNum = 0;
         return (ERROR);
      }
   }

   /*
    * Exclusive access to the global variables is no longer needed.
    */

   semGive (sdsuAtomicSem);

   /*
    * Tell the VME DSP what vector numbers to use
    */

   if ( (intEnable & SDSU_PACKET_INT_ENABLE) != 0)
   {
      if (sdsuParamWrite (context, SDSU_IDENT_VME, "V_PIID", 
                          context->packetIntNum << 16) == ERROR)
      {
         ERROR_SET (0, "Failed to set SDSU packet interrupt vector parameter", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }
   }
   else
   {
      if (sdsuParamWrite (context, SDSU_IDENT_VME, "V_PIID", 0) == ERROR)
      {
         ERROR_SET (0, "Failed to zero SDSU packet interrupt vector parameter",
            ERROR_LOG_SAVE);
         return (ERROR);
      }
   }

   if ( ((intEnable & SDSU_FRAME_INT_ENABLE) != 0) &&
        (sdsuParamWrite (context, SDSU_IDENT_VME, "V_FIID", 
                         context->frameIntNum << 16) == ERROR) )
   {
      ERROR_SET (0, "Failed to set SDSU frame interrupt vector parameter", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Finally enable interrupts.
    */

#ifdef DEBUG
   printf ("sdsuIntConnect: Enabling interrupts at level %d (same as jumpers on SDSU board).\n",
      SDSU_VME_INT_LEVEL);
#endif

   if ( sysIntEnable (SDSU_VME_INT_LEVEL) == ERROR )
   {
      ERROR_SET (0, "Failed to enable system interrupts", ERROR_LOG_SAVE);
      return (ERROR);
   }
   if ( sdsuParamWrite (context, SDSU_IDENT_VME, "V_INT_EN", intEnable) 
        == ERROR )
   {
      ERROR_SET (0, "Failed to enable SDSU interrupts", ERROR_LOG_SAVE);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuSimpleIntConnect
 *
 *   INVOCATION:
 *   sdsuSimpleIntConnect (context, intEnable)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context   (SDSU_ID)  SDSU Context structure
 *   (>) intEnable (uint32)   Interrupts to enable mask
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if an error occurred
 *
 *   PURPOSE:
 *   Connect to SDSU controller interrupts - simple version
 *
 *   DESCRIPTION:
 *   This routine allocates one interrupt vectors and connects the
 *   frame synchronisation interrupts of an SDSU controller up to the relevent
 *   internal routines for correct operation of the sdsuReadout process.  The
 *   VME DSP is told what vector numbers to use, and interrupts are enabled on
 *   both the CPU (for the level indicated by SDSU_VME_INT_LEVEL in sdsuLib.h)
 *   and the SDSU VME Interface card. The intEnable parameter is ignored.
 *
 *   EXTERNAL VARIABLES:
 *   (>) sdsuAtomicSem      (SEM_ID)   Semaphore for exclusive access
 *   (!) sdsuVectorNumber   (int)      Vector number allocation
 *
 *   PRIOR REQUIREMENTS:
 *   VME DSP application code revision 0.17 or later.
 *
 *   REFERENCES:
 *   See Gemini Interface Control Document ICD 1.6/1.10 for a description of the
 *   SDSU controller software interface.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   There is no way of confirming that the interrupt level SDSU_VME_INT_LEVEL 
 *   is correct for this particular SDSU VME card, because it's set by jumper.
 *   
 *   It is possible (in fact quite easy) to run out of interrupt vectors, as
 *   these are not recovered for re-use when an SDSU context is deleted. Even if
 *   they were, there would still be a memory leak because the vxWorks
 *   intConnect routine allocates memory for the interrupt handler, but there is
 *   no way to release this.
 *
 *   I have modified this function so that interrupt vectors are not allocated 
 *   for interrupts that are not enabled. This should delay the running 
 *   out of vectors.  SMB - 3 December 1998.
 *
 *   It is possible for this function to wait forever for the sdsuAtomicSem 
 *   semaphore. A timeout should be used if this ever becomes a problem.
 *-
 */

STATUS   sdsuSimpleIntConnect ( SDSU_ID    context,
                                uint32     intEnable )
{
   /*
    * Check whether the SDSU context provided is valid.
    */

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Check we haven't run out of interrupts.
    */

   if (sdsuVectorNumber >= SDSU_INT_NUMBER_LIMIT)
   {
      ERROR_SET (S_sdsuLib_NO_INT_AVAILABLE, "Run out of interrupt vectors", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Ensure exclusive access to the sdsuLib global variables.
    */

   semTake (sdsuAtomicSem, WAIT_FOREVER);

   /*
    * Allocate a vector number and connect handlers for frame interrupts,
    * if requested and we have not already done so.
    */

   if ( ((intEnable & SDSU_FRAME_INT_ENABLE) != 0) && 
        (context->frameIntNum == 0) )
   {

      /*
       * Get an interrupt vector number from sdsuVectorNumber, ensuring
       * exclusive access to this variable during allocation.
       */
      
      context->frameIntNum = sdsuVectorNumber++;

#ifdef DEBUG
      printf (
      "sdsuSimpleIntConnect: Connecting frame interrupts to vector %d (%p).\n",
      context->frameIntNum, INUM_TO_IVEC (context->frameIntNum));
#endif   

      /*
       * Connect the appropriate interrupt service routine to this vector number
       */
      
      if ( intConnect (INUM_TO_IVEC (context->frameIntNum), 
                       sdsu_simpleSyncIsr, (int) context) == ERROR )
      {
         ERROR_SET (0, "Failed to connect frame interrupt service routine", 
                    ERROR_LOG_SAVE);

         /* Cancel vector number allocation if the connection failed */

         sdsuVectorNumber = context->frameIntNum;
         semGive (sdsuAtomicSem);
         context->frameIntNum = 0;
         return (ERROR);
      }
   }

   /*
    * Exclusive access to the global variables is no longer needed.
    */

   semGive (sdsuAtomicSem);

   /*
    * Tell the VME DSP what vector numbers to use
    */

   if ( sdsuParamWrite (context, SDSU_IDENT_VME, "V_FIID", 
                        context->frameIntNum << 16) == ERROR)
   {
      ERROR_SET (0, "Failed to set SDSU frame interrupt vector parameter", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   if ( sdsuParamWrite (context, SDSU_IDENT_VME, "V_PIID", 0) == ERROR)
   {
      ERROR_SET (0, "Failed to zero SDSU packet interrupt vector parameter", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Finally enable interrupts.
    */

#ifdef DEBUG
   printf ("sdsuIntConnect: Enabling interrupts at level %d (same as jumpers on SDSU board).\n",
      SDSU_VME_INT_LEVEL);
#endif

   if ( sysIntEnable (SDSU_VME_INT_LEVEL) == ERROR )
   {
      ERROR_SET (0, "Failed to enable system interrupts", ERROR_LOG_SAVE);
      return (ERROR);
   }
   if ( sdsuParamWrite (context, SDSU_IDENT_VME, "V_INT_EN", 
                        SDSU_FRAME_INT_ENABLE) == ERROR )
   {
      ERROR_SET (0, "Failed to enable SDSU interrupts", ERROR_LOG_SAVE);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_packetSyncIsr
 *
 *   INVOCATION:
 *   sdsu_packetSyncIsr (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context   (int)   SDSU_ID for this controller
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Interrupt service routine for packet-sync interrupt from SDSU controller
 *
 *   DESCRIPTION:
 *   This is the interrupt service routine used for packet-synchronisation 
 *   during readout of the SDSU controller.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

void   sdsu_packetSyncIsr ( int   iContext )
{
   int     key;
   SDSU_ID context = (SDSU_ID) iContext;

#ifdef DEBUG
   logMsg ("sdsu_packetSyncIsr (%p)\n",iContext,0,0,0,0,0);
#endif /* DEBUG */

   if ( context != NULL )
   {

      /* Give the packet sync semaphore with interrupts disabled. */

      key = intLock();
      semGive (context->packetSem);
      intUnlock (key);
   }
   else
   {
      logMsg ("sdsu_packetSyncIsr: Null context.\n",0,0,0,0,0,0);
   }
}


/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_frameSyncIsr
 *
 *   INVOCATION:
 *   sdsu_frameSyncIsr (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context   (int)   SDSU_ID for this controller
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Interrupt service routine for frame-sync interrupt from SDSU controller
 *
 *   DESCRIPTION:
 *   This is the interrupt service routine used for frame-synchronisation during
 *   readout of the SDSU controller.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

void   sdsu_frameSyncIsr ( int   iContext )
{
   int     key;
   SDSU_ID context = (SDSU_ID) iContext;
   
#ifdef DEBUG
   logMsg ("sdsu_frameSyncIsr (%p)\n",iContext,0,0,0,0,0);
#endif /* DEBUG */

   if ( context != NULL )
   {

      /* Disable interrupts. */
      key = intLock();

      /*
       * We can't send a new FBA here, so we tell the task to do it as a
       * matter of priority by sending it a SIGUSR1 signal. This should cause
       * the signal handler defined by sdsu_readTask (sdsu_readSignal) to be 
       * executed.
       */

      if (kill (context->readTask, SIGUSR1))
      {
         intUnlock (key);
         logMsg ("sdsu_frameSyncIsr: signal to sdsu_readTask %d failed\n",
                 context->readTask, 0, 0, 0, 0, 0);
         return;
      }

      /* 
       * Trigger the processing of the last packet by giving the packet sync 
       * semaphore. 
       */
   
      semGive (context->packetSem);

      /* Reenable interrupts */
      intUnlock (key);
   }
   else
   {
      logMsg ("sdsu_frameSyncIsr: Null context.\n",0,0,0,0,0,0);
   }
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_simpleSyncIsr
 *
 *   INVOCATION:
 *   sdsu_simpleSyncIsr (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context   (int)   SDSU_ID for this controller
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Interrupt service routine for frame-sync interrupt from SDSU controller 
 *   Simple version
 *
 *   DESCRIPTION:
 *   This is the simple version of the interrupt service routine used for 
 *   frame-synchronisation during readout of the SDSU controller.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

void   sdsu_simpleSyncIsr ( int   iContext )
{
   int     key;
   SDSU_ID context = (SDSU_ID) iContext;
   
#ifdef DEBUG
   logMsg ("sdsu_simpleSyncIsr (%p)\n",iContext,0,0,0,0,0);
#endif

   if ( context != NULL )
   {

      /* 
       * Trigger the processing of the frame by giving the packet sync 
       * semaphore with interrupts disabled 
       */
   
      key = intLock();
      semGive (context->packetSem);
      sdsuFrameLost ++ ;
      intUnlock (key);

   }
   else
   {
      logMsg ("sdsu_frameSyncIsr: Null context.\n",0,0,0,0,0,0);
   }
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuSimulatePacketSync
 *
 *   INVOCATION:
 *   sdsuSimulatePacketSync (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context   (SDSU_ID)   SDSU_ID for this controller
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if an error occurred
 *
 *   PURPOSE:
 *   Simulate a packet-sync interrupt from the SDSU controller
 *
 *   DESCRIPTION:
 *   This function simulates a packet sync interrupt from the SDSU controller, 
 *   and may be used to trigger a packet sync when the controller is being 
 *   simulated.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuSimulatePacketSync ( SDSU_ID   context )
{

   /*
    * Check the SDSU context is valid and give a warning if this function is 
    * called with packet sync interrupts enabled when not in simulation mode.
    */

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   if ( (!context->simulate) && (context->packetIntNum != 0) )
   {
   MESSAGE_LOG (MSG_WARNING,
   "WARNING: Packet interrupts enabled, SDSU controller not being simulated");
   }

   /* Call the private packet sync interrupt service function directly. */

   sdsu_packetSyncIsr( (int) context );

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuSimulateFrameSync
 *
 *   INVOCATION:
 *   sdsuSimulateFrameSync (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context   (SDSU_ID)   SDSU_ID for this controller
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if an error occurred
 *
 *   PURPOSE:
 *   Simulate a frame-sync interrupt from the SDSU controller
 *
 *   DESCRIPTION:
 *   This function simulates a frame sync interrupt from the SDSU controller, 
 *   and may be used to trigger a frame sync when the controller is being 
 *   simulated.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuSimulateFrameSync ( SDSU_ID   context )
{
   /*
    * Check the SDSU context is valid and give a warning if this function is 
    * called with frame sync interrupts enabled when not in simulation mode.
    */

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   if ( (!context->simulate) && (context->frameIntNum != 0) )
   {
   MESSAGE_LOG (MSG_WARNING,
   "WARNING: Frame interrupts enabled and SDSU controller not being simulated");
   }

   /* Call the private frame sync interrupt service function directly. */

   sdsu_frameSyncIsr( (int) context );

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuSimulateSimpleSync
 *
 *   INVOCATION:
 *   sdsuSimulateSimpleSync (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context   (SDSU_ID)   SDSU_ID for this controller
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if an error occurred
 *
 *   PURPOSE:
 *   Simulate a simple frame-sync interrupt from the SDSU controller
 *
 *   DESCRIPTION:
 *   This function simulates a simple frame sync interrupt from the SDSU 
 *   controller, and may be used to trigger a frame sync when the controller 
 *   is being simulated.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuSimulateSimpleSync ( SDSU_ID   context )
{
   /*
    * Check the SDSU context is valid and give a warning if this function is 
    * called with frame sync interrupts enabled when not in simulation mode.
    */

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   if ( (!context->simulate) && (context->frameIntNum != 0) )
   {
   MESSAGE_LOG (MSG_WARNING,
   "WARNING: Frame interrupts enabled and SDSU controller not being simulated");
   }

   /* Call the private frame sync interrupt service function directly. */

   sdsu_simpleSyncIsr( (int) context );

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuIntDisable
 *
 *   INVOCATION:
 *   sdsuIntDisable (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context    (SDSU_ID)   SDSU Context structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if an error occurred
 *
 *   PURPOSE:
 *   Disable SDSU controller interrupts
 *
 *   DESCRIPTION:
 *   This routine disables both packet and frame sync interrupts from the 
 *   selected SDSU controller.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   REFERENCES:
 *   See Gemini Interface Control Document ICD 1.6/1.10 for a description of the
 *   SDSU image-readout communications protocol.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   It is not really feasible to disconnect interrupt vectors (vxWorks doesn't
 *   make its default vector available, and there would still be a memory leak
 *   due to storage allocated to the interrupt handler), hence this is not the
 *   exact reverse of the sdsuIntConnect routine, but this is really the best
 *   which is worth doing.  It is not a good idea to call sysIntDisable()
 *   here as we can't really be sure that some other driver (possibly even
 *   another SDSU VME card) isn't still using the same interrupt level.
 *-
 */

STATUS   sdsuIntDisable ( SDSU_ID context )
{

   /* Disable interrupts by clearing all the bits in the V_INT_EN parameter */

   if (sdsuParamWrite (context, SDSU_IDENT_VME, "V_INT_EN", 0) == ERROR)
   {
      ERROR_SET (0, "Failed to disable SDSU interrupts", ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   return OK;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuReadoutOpen
 *
 *   INVOCATION:
 *   sdsuReadoutOpen (context, packetCall, frameCall, useInterrupts)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context       (SDSU_ID)       Context ID
 *   (>) packetCall    (SDSU_CALLBACK) Packet sync callback routine
 *   (>) frameCall     (SDSU_CALLBACK) Frame sync callback routine
 *   (>) useInterrupts (const BOOL)    Should SDSU interrupts be used
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if readout could not be aborted.
 *
 *   PURPOSE:
 *   Start a detector readout task for this controller
 *
 *   DESCRIPTION:
 *   This routine will start a readout task for the indicated controller,
 *   setting the application callback routines to the packetCall and frameCall
 *   functions. These may be passed as NULL if not required by the application.
 *   The useInterrupts parameter should normally be TRUE but can be set FALSE
 *   to operate the SDSU controller without interrupts (in which case the
 *   application will not be interrupted when an exposure is complete but
 *   instead should wait a sufficient length of time and then call one of
 *   the interrupt simulation functions).
 *
 *   SEE ALSO:
 *   sdsuReadoutClose
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   The readout task priority is currently a fixed constant. It might be a
 *   good idea for the different readout tasks to have different priorities
 *   or their relative execution priorities may be random.
 *-
 */

STATUS   sdsuReadoutOpen
   (
   SDSU_ID         context,
   SDSU_CALLBACK   packetCall,
   SDSU_CALLBACK   frameCall,
   const BOOL      useInterrupts
   )
{
   uint32         intEnable;
   char *         readName;
   const char *   myName;
   
   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   /* Complain if the readout task already exists */
   
   if (context->readTask != 0)
   {
      ERROR_SET (S_sdsuLib_READOUT_ACTIVE, "Readout task already exists", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   /*
    * What name shall I use? Make one up based on who we're called from.
    * An extra 9 bytes is allocated to allow the ":readout" string to be 
    * appended.
    */
   
   myName = taskName(0);
   if ((readName = (char *) malloc(strlen(myName) + 9)) == NULL)
   {
      ERROR_SET(0, "Memory allocation for readout task name failed", 
                ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   strcpy(readName, myName);
   strcat(readName, ":readout");
   
   /* Set the task status to OPENING, then fire it up */

#ifdef DEBUG
   printf ("sdsuReadoutOpen: Spawning readout task %s\n", readName);
#endif /* DEBUG */

   context->readStatus = SDSU_READ_OPENING;
   context->readTask = taskSpawn (readName, SDSU_READTASK_PRIORITY, 
      VX_FP_TASK, SDSU_READTASK_STACKSIZE,
      (FUNCPTR) sdsu_readTask, (int) context, (int) packetCall, (int) frameCall,
      (int) useInterrupts, 0, 0, 0, 0, 0, 0);
   
   if (context->readTask == ERROR)
   {
      ERROR_SET1 (0, "Failed to spawn readout task, %s", ERROR_LOG_SAVE, 
                  readName);
      context->readTask = 0;
      free (readName);
      return (ERROR);
   }

   /* Free the temporary name buffer. */

   free (readName);

   /*
    * If requested, connect interrupts to the card. If a packet callback is not
    * provided we don't bother to enable packet interrupts as they'd just be a
    * waste of CPU.
    */

   if ( useInterrupts )
   {
      intEnable = SDSU_FRAME_INT_ENABLE;
      if (packetCall != NULL)
         intEnable |= SDSU_PACKET_INT_ENABLE;

      if (sdsuIntConnect (context, intEnable) == ERROR)
      {
         ERROR_SET (0, "Readout open failed - failed to connect interrupts", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuSimpleReadoutOpen
 *
 *   INVOCATION:
 *   sdsuSimpleReadoutOpen (context, packetCall, frameCall, nice, useInterrupts)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context        (SDSU_ID)         Context ID
 *   (>) packetCall     (SDSU_CALLBACK)   Packet sync callback routine
 *   (>) frameCall      (SDSU_CALLBACK)   Frame sync callback routine
 *   (>) nice           (const int)       Priority increment
 *   (>) useInterrupts  (const BOOL)      Should SDSU interrupts be used
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if readout could not be aborted.
 *
 *   PURPOSE:
 *   Start a detector readout task for this controller - simple version
 *
 *   DESCRIPTION:
 *   This routine will start a SIMPLE VERSION OF THE readout task for the 
 *   indicated controller, setting the application callback routines to the 
 *   packetCall and frameCall functions. These may be passed as NULL if not 
 *   required by the application.
 *   The useInterrupts parameter should normally be TRUE but can be set FALSE
 *   to operate the SDSU controller without interrupts (in which case the
 *   application will not be interrupted when an exposure is complete but
 *   instead should wait a sufficient length of time and then call one of
 *   the interrupt simulation functions).
 *
 *   SEE ALSO:
 *   sdsuReadoutClose
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   The readout task priority is currently a fixed constant. It might be a
 *   good idea for the different readout tasks to have different priorities
 *   or their relative execution priorities may be random.
 *-
 */

STATUS   sdsuSimpleReadoutOpen
   (
   SDSU_ID         context,
   SDSU_CALLBACK   packetCall,
   SDSU_CALLBACK   frameCall,
   const int       nice,
   const BOOL      useInterrupts
   )
{
   uint32         intEnable;
   char *         readName;
   const char *   myName;
   
   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   /* Complain if the readout task already exists */
   
   if (context->readTask != 0)
   {
      ERROR_SET (S_sdsuLib_READOUT_ACTIVE, "Readout task already exists", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   /*
    * What name shall I use? Make one up based on who we're called from.
    * An extra 9 bytes is allocated to allow the ":readout" string to be 
    * appended.
    */
   
   myName = taskName(0);
   if ((readName = (char *) malloc(strlen(myName) + 9)) == NULL)
   {
      ERROR_SET(0, "Memory allocation for readout task name failed", 
                ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   strcpy(readName, myName);
   strcat(readName, ":readout");
   
   /* Set the task status to OPENING, then fire it up */

#ifdef DEBUG
   printf ("sdsuReadoutOpen: Spawning simple readout task %s\n", readName);
#endif /* DEBUG */

   context->readStatus = SDSU_READ_OPENING;
   context->readTask = taskSpawn (readName, SDSU_READTASK_PRIORITY, 
      VX_FP_TASK, SDSU_READTASK_STACKSIZE,
      (FUNCPTR) sdsu_simpleTask, (int) context, (int) packetCall, 
      (int) frameCall, (int) useInterrupts, 0, 0, 0, 0, 0, 0);
   
   if (context->readTask == ERROR)
   {
      ERROR_SET1 (0, "Failed to spawn simple readout task, %s", 
                  ERROR_LOG_SAVE, readName);
      context->readTask = 0;
      free (readName);
      return (ERROR);
   }

   /* Free the temporary name buffer. */

   free (readName);

   /*
    * If requested, connect interrupts to the card. The packet callback is 
    * ignored.
    */

   if ( useInterrupts )
   {
      intEnable = SDSU_FRAME_INT_ENABLE;

      if (sdsuSimpleIntConnect (context, intEnable) == ERROR)
      {
         ERROR_SET (0, 
         	    "Readout open failed - failed to connect simple interrupts",
                    ERROR_LOG_SAVE);
         return (ERROR);
      }
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_readTask
 *
 *   INVOCATION:
 *   sdsu_readTask (context, packetSync, frameSync, useInterrupts)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context       (int)   SDSU_ID for this controller
 *   (>) packetCall    (int)   Application packet sync callback routine
 *   (>) frameCall     (int)   Application frame sync callback routine
 *   (>) useInterrupts (int)   Set TRUE when interrupts are being used
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Entry routine for controller readout task
 *
 *   DESCRIPTION:
 *   This is the task entry point for the readout task which handles all the
 *   image processing and synchronisation functions during the readout of the
 *   SDSU controller.
 *   
 *   If not NULL, the packetCall is executed by the task whenever more data
 *   has been received, until the whole frame is in memory.  Application code is
 *   allowed to spend as much elapsed time as it likes processing the packet 
 *   data, although on average the processing time must not exceed the time 
 *   between readouts or the system will run out of image buffers. When the 
 *   frame has been received the frameCall routine is run (omitted if this is 
 *   NULL).
 *   
 *   NB if a frame has been completely received before the readout task starts
 *   to process it, the packetCall routine will never be run for that frame,
 *   thus the frameCall must be able to completely handle the processing of the
 *   frame data.  The application is responsible for checking the parameterId
 *   field of the frame and working out how much data has been received at any
 *   time, based on the packetCount and its knowledge of the packet size.
 *
 *   SEE ALSO:
 *   sdsu_readSignal
 *
 *   EXTERNAL VARIABLES:
 *   (>)   sdsuReadContext    (SDSU_ID)   task variable holding SDSU context
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   The task priority is reduced each time application code is executed.
 *   This also lowers the priority of the packet and frame signal handlers, 
 *   which could delay the response to packet and frame interrupts. A better 
 *   solution is needed which allows the signals to remain at high priority.
 *-
 */

void   sdsu_readTask ( int    iContext,
                       int    iPacket,
                       int    iFrame,
                       int    iUseInt )
{
   SDSU_CALLBACK   packetCall      = (SDSU_CALLBACK) iPacket;
   SDSU_CALLBACK   frameCall      = (SDSU_CALLBACK) iFrame;

   SDSU_FRAME      *pFrame;

   if (errorInit () == ERROR)
   {
      printErr (
      "sdsu_readTask: Failed to initialise error context structure.\n");       
      return;
   }
   
   /*
    * Set up the sdsuReadContext taskVar and register our signal handler, 
    * sdsu_readSignal 
    */
   
   if (taskVarAdd(0, (int *) &sdsuReadContext) == ERROR)
   {
      ERROR_SET (0, "Failed to create sdsuReadContext task variable", 
                 ERROR_LOG_NOW);
      return;
   }
   sdsuReadContext = (SDSU_ID) iContext;
   signal (SIGUSR1, sdsu_readSignal);
   
   /* Now we loop round coping with frame and packet callbacks */

   sdsuReadContext->readStatus = SDSU_READ_IDLE;

   while (TRUE)
   {
      /*
       * Get the address of the next frame from the queue, transmitted by 
       * sdsu_readSignal. The message should contain a pointer to an 
       * SDSU_FRAME structure.
       */
      
      if (msgQReceive (sdsuReadContext->frameQueue, (char *) &pFrame, 
          sizeof (SDSU_FRAME *), WAIT_FOREVER) != sizeof (SDSU_FRAME *))
      {
         /* 
          * msgQReceive has returned ERROR or has received a message of the 
          * wrong size 
          */

         ERROR_SET (0, "Failed to receive message from frame queue correctly",
                    ERROR_LOG_NOW);
      }
      else if ( pFrame != NULL )
      {

         /*
          * NOTE: You might wish to put here a callback to some additional
          * routine that indicates there is a pFrame at a certain address.
          * There may be fewer packet interrupts than packets, since several
          * packets may have arrived since the last interrupt. ANJ.
          */

         sdsuReadContext->readStatus = SDSU_READ_BUSY;
         
         while ((pFrame->header.status & SDSU_FSTAT_COMPLETE) == 0)
         {
            semTake (sdsuReadContext->packetSem, WAIT_FOREVER);

#ifdef DEBUG
            printf ("sdsu_readTask: Packet callback %p %p %p\n", 
                    sdsuReadContext, sdsuReadContext->appPrivate, pFrame);
#endif 
            
            /*
             * Next packet available, tell the processing software.
             * The task priority is not reduced here, since the application code
             * to handle a packet interrupt is assumed to do very little.
             */

            if (packetCall != NULL)
            {
               (*packetCall) (sdsuReadContext, sdsuReadContext->appPrivate, 
                              pFrame);
            }
         }
         
         /*
          * This frame is complete. In debug mode check the header status.
          */

#ifdef DEBUG
         if ( (pFrame->header.status & SDSU_FSTAT_OVERRUN) != 0 )
         {
            printf ("sdsu_readTask: Overrun error bit set in frame header\n");
         }
         if ( (pFrame->header.status & SDSU_FSTAT_CHECKSUM) != 0 )
         {
            printf ("sdsu_readTask: Checksum error bit set in frame header\n");
         }
         if ( (pFrame->header.status & SDSU_FSTAT_FRAMESYNC) != 0 )
         {
            printf ("sdsu_readTask: Framesync error bit set in frame header\n");
         }
         if ( (pFrame->header.status & SDSU_FSTAT_ABORTED) != 0 )
         {
            printf ("sdsu_readTask: Frame aborted bit set in frame header\n");
         }
#endif

         /*
          * Do the frame callback if the frame has not been aborted.
          * The task priority is temporarily reduced while the application code
          * (which may be CPU intensive) executes.
          * NOTE: This is only a temporary solution. Reducing the priority 
          * here also reduces the priority of the signal handler, which could 
          * delay the response to frame interrupts.
          */

         if ( (pFrame->header.status & SDSU_FSTAT_ABORTED) == 0 )
         {

#ifdef DEBUG
            printf ("sdsu_readTask: Frame callback %p %p %p\n", sdsuReadContext,
               sdsuReadContext->appPrivate, pFrame);
#endif /* DEBUG */

            if (frameCall != NULL)
            {
               taskPrioritySet ( taskIdSelf(), SDSU_APPLICATION_PRIORITY);
               (*frameCall) (sdsuReadContext, sdsuReadContext->appPrivate, 
               pFrame);
               taskPrioritySet ( taskIdSelf(), SDSU_READTASK_PRIORITY);
            }
         }
#ifdef DEBUG
         else
         {
            printf ("sdsu_readTask: No frame callback since frame aborted\n");
         }
#endif /* DEBUG */

         /*
          * We're done with this frame now. This call will return the frame 
          * to the free list unless the application code has reserved the 
          * frame with sdsuFrameReserve.
          */

         if (sdsuFrameRelease (sdsuReadContext, pFrame) == ERROR)
         {
            ERROR_SET (0, "Problem releasing frame after readout", 
                       ERROR_LOG_NOW);
         }

         sdsuReadContext->readStatus = SDSU_READ_IDLE;
      }
      else
      {
         /*
          * Something has gone wrong with the signal handler and it has 
          * reported an error by sending a NULL frame pointer.
          */

         sdsuReadContext->readStatus = SDSU_READ_ERROR;
      }
   }
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_simpleTask
 *
 *   INVOCATION:
 *   sdsu_simpleTask (context, packetSync, frameSync, useInterrupts)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context       (int)   SDSU_ID for this controller
 *   (>) packetCall    (int)   Application packet sync callback routine
 *   (>) frameCall     (int)   Application frame sync callback routine
 *   (>) useInterrupts (int)   Set TRUE when interrupts are being used
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Entry routine for controller readout task
 *
 *   DESCRIPTION:
 *   This is the task entry point for a simplified version of the readout 
 *   task which handles all the image processing and synchronisation functions 
 *   during the readout of the SDSU controller.
 *
 *   SEE ALSO:
 *   sdsu_readSignal
 *
 *   EXTERNAL VARIABLES:
 *   (>)   sdsuReadContext    (SDSU_ID)   task variable holding SDSU context
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   This function is a quick fudge only. It needs to be redesigned properly.
 *-
 */

void   sdsu_simpleTask
   (
   int    iContext,
   int    iPacket,
   int    iFrame,
   int    iUseInt
   )
{
   SDSU_ID         context         = (SDSU_ID) iContext;

   SDSU_CALLBACK   frameCall       = (SDSU_CALLBACK) iFrame;
   BOOL            useInterrupts   = (BOOL) iUseInt;

   SDSU_FRAME      *pFrame;

   int             frame;
   int             failures;
   int             oldPriority;
   int             lastFrame;
   uint32          dmaAddress;

   double          timeout;
   int             i ;

   /* Turn off floating point exception errors */

   setFPE() ;

   if (errorInit () == ERROR)
   {
      printErr (
      "sdsu_readTask: Failed to initialise error context structure.\n");       
      return;
   }
   
   /* Set up the sdsuReadContext taskVar. */
   
   if (taskVarAdd(0, (int *) &sdsuReadContext) == ERROR)
   {
      ERROR_SET (0, "Failed to create sdsuReadContext task variable", 
                 ERROR_LOG_NOW);
      return;
   }
   sdsuReadContext = (SDSU_ID) iContext;
   

   /* Now we loop round coping with frame callbacks */

   sdsuReadContext->readStatus = SDSU_READ_IDLE;

   while (TRUE)
   {
      /*
       * Get the address of the next frame from the queue.
       * The message should contain a pointer to an SDSU_FRAME structure.
       */

      if (msgQReceive (sdsuReadContext->frameQueue, (char *) &pFrame, 
          sizeof (SDSU_FRAME *), WAIT_FOREVER) != sizeof (SDSU_FRAME *))
      {
         /* 
          * msgQReceive has returned ERROR or has received a message of the 
          * wrong size 
          */

         ERROR_SET (0, "Failed to receive message from frame queue correctly",
                    ERROR_LOG_NOW);
         sdsuReadContext->readStatus = SDSU_READ_ERROR;
      }
      else if ( pFrame == NULL )
      {
         /*
          * NULL frame pointer. Something has gone wrong.
          */

         ERROR_SET (0, "NULL pointer received from frame queue", ERROR_LOG_NOW);
         sdsuReadContext->readFrame = NULL;
         sdsuReadContext->readStatus = SDSU_READ_ERROR;
      }
      else
      {

#ifdef DEBUG
         if ( pFrame->totalFrames>0 )
         {
            printf (
            "readTask: Got mess to process frame %p with %d frames\n",
            pFrame, pFrame->totalFrames);
         }
         else
         {
            printf ("readTask: Got mess to process frame %p with INF frames\n",
            pFrame);
         }
#endif

         sdsuReadContext->readStatus = SDSU_READ_BUSY;

         if ( sdsuReadContext->readMethod == 1 )
         {
            /* 
             * Set the SDSU Frame Buffer Address to the address of the next 
             * frame. 
             */
#ifdef DEBUG
            printf ("sdsu_simpleTask: Set FBA \n");
#endif

            if (sdsuFrameSetFBA (sdsuReadContext, pFrame) == ERROR)
            {
               ERROR_SET (0, 
               "Failed to set up frame buffer address for frame DMA",
               ERROR_LOG_NOW);
               sdsuReadContext->readStatus = SDSU_READ_ERROR;
               sdsuReadContext->fatal = TRUE;     /* Abandon the observation. */
            };

            if ( !sdsuReadContext->fatal )
            {
               if (sysLocalToBusAdrs (SDSU_AM_VME_MASTER_DATA, 
                                      (char *) & pFrame->header,
                                      (char **) & dmaAddress) == ERROR)
               {
                  ERROR_SET (0, "Failed to map frame buffer address to VME bus",
                             ERROR_LOG_NOW);
                  sdsuReadContext->readStatus = SDSU_READ_ERROR;
                  sdsuReadContext->fatal = TRUE;     
               }
            };

            if ( useInterrupts )
            {
               if ( semTake (sdsuReadContext->packetSem, NO_WAIT) == OK )
               {
#ifdef DEBUG
                  printf ( "readTask: clear IT semaphore, sdsuFrameLost=%d\n" , 
                           sdsuFrameLost) ;
#endif
               }
               else
               {
#ifdef DEBUG
                  printf ( "readTask: No sem IT to clear, sdsuFrameLost=%d\n" , 
                           sdsuFrameLost) ;
#endif
               }

            }
               
            sdsuFrameLost = 0; 
#ifdef DEBUG
            printf ( "sdsu_simpleTask: sdsuFrameLost=%d\n" , sdsuFrameLost) ;
#endif

            if ( !sdsuReadContext->fatal )
            {
               if (sdsuPrimitive (sdsuReadContext, "RDC", SDSU_IDENT_VME, NULL, 
                                  NULL) == ERROR)
               {
                  ERROR_SET (0, "Readout CCD (RDC) command failed", 
                             ERROR_LOG_NOW);
                  sdsuReadContext->readStatus = SDSU_READ_ERROR;
                  sdsuReadContext->fatal = TRUE;  /* Abandon the observation. */
               }
            };

            if ( sdsuReadContext->fatal )
            {
               /* Observation abandoned due to fatal error. */
               if (sdsuPrimitive (sdsuReadContext, "ABT", SDSU_IDENT_VME, 
                                  NULL, NULL) == ERROR )
               {
                  ERROR_SET (0, "Failed to abort observation after fatal error",
                             ERROR_LOG_NOW);
               }
            }
            else
            {
    
               /*
                * Process each frame in turn.
                * Loop forever if the total number of frames is specified as 0 
                * or negative. Terminate the loop if the frame status becomes 
                * non-zero (which means the SDSU controller has detected an 
                * error).
                */

               sdsuReadContext->aborted = FALSE;
               sdsuReadContext->fatal = FALSE;
               lastFrame = FALSE ;
               failures = 0; 
               for (frame=0; (((pFrame->totalFrames <= 0) || 
                    (frame < pFrame->totalFrames)) &&
                    (!lastFrame) &&
                    (!sdsuReadContext->aborted) &&
                    (!sdsuReadContext->fatal));
                    frame++)
               {
                  /* Ensure the packet count in the data buffer begins at zero*/
                  pFrame->header.packetCount = 0;
                  pFrame->header.status = 0 ;

#ifdef DEBUG
                  sdsuFrameShow ( pFrame );
#endif 

                  if ( useInterrupts )
                  {
#ifdef DEBUG
                     printf ("readTask: Waiting for frame sync semaphore ...");
#endif 
                     if ( semTake (sdsuReadContext->packetSem, 
                                   sdsuReadContext->frameTimeout) == ERROR)
                     {
#ifdef DEBUG
                        printf (
                           "WARNING: frame sync sem timed out at frame %d\n",
                           (frame+1));
#endif 
                        failures++;
                     }
#ifdef DEBUG
                     else
                     {
                        printf (" ... got frame sync semaphore\n");
                     }
#endif 
                     sdsuFrameLost -- ;
                     if ( pFrame->header.packetCount < 
                          context->packetsPerFrame ) 
                        pFrame->header.status |= SDSU_FSTAT_NOK ;
                  
                  }
                  else
                  {
#ifdef DEBUG
                     printf ( "readTask: Polling for packet count reaching %d",
                              context->packetsPerFrame);
#endif
                     if ( sdsuReadContext->exposureTicks < 2 )
                        taskDelay (1) ;
                     else
                        taskDelay (sdsuReadContext->exposureTicks - 1) ;

                     while ( (pFrame->header.packetCount < 
                              context->packetsPerFrame) 
                             && (pFrame->header.status == 0))
                     { 
                        for (i=0;i<1000;i++) ;
                     }
                  }

                  if (pFrame->header.status != 0)
                  {
                     failures++;
                  }

#ifdef DEBUG
                  if (pFrame->header.status == 0)
                     printf (" ... all packets received.\n");
                  else
                     printf (" ... FRAME COMPLETE WITH LOSS OF PACKETS.\n");

                  sdsuFrameShow ( pFrame );
#endif

                  /*
                   * Fudge the frame number in the header so that detControl 
                   * behaves correctly.
                   */

                  if ( pFrame->header.frameCount == 1 )
                     lastFrame = TRUE ;
                  else
                     pFrame->header.frameCount = pFrame->totalFrames - frame;
#ifdef DEBUG
                  printf ( "header.frameCount=%d, lastFrame = %d\n",
                           pFrame->header.frameCount, lastFrame);
#endif
                  /*
                   * Ensure the aborted bit is set in the frame header if the 
                   * frame has been aborted.
                   */

                  if ( sdsuReadContext->aborted )
                  {
                     pFrame->header.status |= SDSU_FSTAT_ABORTED;
                  }

                  /*
                   * Record the number of frame failures.
                   */

                  sdsuReadContext->frameErrors = failures;

                  /*
                   * The frame is complete. In debug mode check the header 
                   * status.
                   */

#ifdef DEBUG
                  if ( (pFrame->header.status & SDSU_FSTAT_OVERRUN) != 0 )
                  {
                     printf (
                     "readTask: Overrun error bit set in frame header\n");
                  }
                  if ( (pFrame->header.status & SDSU_FSTAT_CHECKSUM) != 0 )
                  {
                     printf (
                     "readTask: Checksum error bit set in frame header\n");
                  }
                  if ( (pFrame->header.status & SDSU_FSTAT_FRAMESYNC) != 0 )
                  {
                     printf (
                     "readTask: Framesync error bit set in frame header\n");
                  }
                  if ( (pFrame->header.status & SDSU_FSTAT_ABORTED) != 0 )
                  {
                     printf (
                     "readTask: Frame aborted bit set in frame header\n");
                  }
                  if ( (pFrame->header.status & SDSU_FSTAT_TIMEOUT) != 0 )
                  {
                     printf ("readTask: Timeout bit set in frame header\n");
                  }
                  if ( (pFrame->header.status & SDSU_FSTAT_NOK) != 0 )
                  {
                     printf ("readTask: Overwritten bit set in frame header\n");
                  }
#endif

                  /* 
                   * If the observation has not been aborted, call the frame 
                   * callback function. 
                   */

#ifdef DEBUG
                  printf ("readTask: Frame callback %p %p %p\n", 
                          sdsuReadContext, sdsuReadContext->appPrivate, pFrame);
#endif

                  if ((!sdsuReadContext->aborted) && (frameCall != NULL))
                  {
                     (*frameCall) (sdsuReadContext, sdsuReadContext->appPrivate,
                                   pFrame);
                  }
                  else
                  {
#ifdef DEBUG
                     printf (
                     "readTask: No frame callback since frame aborted\n");
#endif 
                  }

               }
            }

            /*
             * Observation complete.
             * Release the frame and go back and wait for the next observation.
             */

            if (sdsuPrimitive (sdsuReadContext, "ABT", SDSU_IDENT_VME, 
                               NULL, NULL) == ERROR )
            {
               ERROR_SET (0, 
                          "Failed to abort observation when stop observation",
                          ERROR_LOG_NOW);
            }
            sdsuFrameRelease (sdsuReadContext, pFrame);
            sdsuReadContext->readFrame = NULL;
            if ( sdsuReadContext->readStatus != SDSU_READ_ERROR )
            {
               sdsuReadContext->readStatus = SDSU_READ_IDLE;
            }
         }
         else
         {
            if ( useInterrupts )
            {
               if ( semTake (sdsuReadContext->packetSem, NO_WAIT) == OK )
               {
#ifdef DEBUG
                  printf ( "readTask: clear IT semaphore, sdsuFrameLost=%d\n" , 
                           sdsuFrameLost) ;
#endif
               }
               else
               {
#ifdef DEBUG
                  printf ( "readTask: No sem IT to clear, sdsuFrameLost=%d\n" , 
                           sdsuFrameLost) ;
#endif
               }

            }
               
            sdsuFrameLost = 0; 
#ifdef DEBUG
            printf ( "sdsu_simpleTask: sdsuFrameLost=%d\n" , sdsuFrameLost) ;
#endif

            /*
             * Process each frame in turn.
             * Loop forever if the total number of frames is specified as 0 
             * or negative. Terminate the loop if the frame status becomes 
             * non-zero (which means the SDSU controller has detected an 
             * error).
             */

            sdsuReadContext->aborted = FALSE;
            sdsuReadContext->fatal = FALSE;
            lastFrame = FALSE ;
            failures = 0; 
            for (frame=0; (((pFrame->totalFrames <= 0) || 
                 (frame < pFrame->totalFrames)) &&
                 (!lastFrame) &&
                 (!sdsuReadContext->aborted) &&
                 (!sdsuReadContext->fatal));
                 frame++)
            {
#ifdef DEBUG
               printf ("sdsu_simpleTask: Set FBA \n");
#endif

               if (sdsuFrameSetFBA (sdsuReadContext, pFrame) == ERROR)
               {
                  ERROR_SET (0, 
                  "Failed to set up frame buffer address for frame DMA",
                  ERROR_LOG_NOW);
                  sdsuReadContext->readStatus = SDSU_READ_ERROR;
                  sdsuReadContext->fatal = TRUE;  /* Abandon the observation. */
               };

               /* Ensure the packet count in the data buffer begins at zero*/
               pFrame->header.packetCount = 0;
               pFrame->header.status = 0 ;

#ifdef DEBUG
               sdsuFrameShow ( pFrame );
#endif 

               if ( !sdsuReadContext->fatal )
               {
                  if (sdsuPrimitive (sdsuReadContext, "RDC", SDSU_IDENT_VME, 
                                     NULL, NULL) == ERROR)
                  {
                     ERROR_SET (0, "Readout CCD (RDC) command failed", 
                                ERROR_LOG_NOW);
                     sdsuReadContext->readStatus = SDSU_READ_ERROR;
                     sdsuReadContext->fatal = TRUE;  
                                                  /* Abandon the observation. */
                  }
               };

               if ( sdsuReadContext->fatal )
               {
                  /* Observation abandoned due to fatal error. */
                  if (sdsuPrimitive (sdsuReadContext, "ABT", SDSU_IDENT_VME, 
                                     NULL, NULL) == ERROR )
                  {
                     ERROR_SET (0, 
                             "Failed to abort observation after fatal error",
                             ERROR_LOG_NOW);
                  }
               }
               else if ( useInterrupts )
               {
#ifdef DEBUG
                  printf ("readTask: Waiting for frame sync semaphore ...");
#endif 
                  if ( semTake (sdsuReadContext->packetSem, 
                                sdsuReadContext->frameTimeout) == ERROR)
                  {
#ifdef DEBUG
                     printf (
                        "WARNING: frame sync sem timed out at frame %d\n",
                        (frame+1));
#endif 
                     failures++;
                  }
#ifdef DEBUG
                  else
                  {
                     printf (" ... got frame sync semaphore\n");
                  }
#endif 
                  sdsuFrameLost -- ;
                  if ( pFrame->header.packetCount < 
                       context->packetsPerFrame ) 
                     pFrame->header.status |= SDSU_FSTAT_NOK ;
                  
               }
               else
               {
#ifdef DEBUG
                  printf ( "readTask: Polling for packet count reaching %d",
                           context->packetsPerFrame);
#endif
                  if ( sdsuReadContext->exposureTicks < 2 )
                     taskDelay (1) ;
                  else
                     taskDelay (sdsuReadContext->exposureTicks - 1) ;

                  while ( (pFrame->header.packetCount < 
                           context->packetsPerFrame) 
                          && (pFrame->header.status == 0))
                  { 
                     for (i=0;i<1000;i++) ;
                  }
               }

               if (pFrame->header.status != 0)
               {
                  failures++;
               }

#ifdef DEBUG
               if (pFrame->header.status == 0)
                  printf (" ... all packets received.\n");
               else
                  printf (" ... FRAME COMPLETE WITH LOSS OF PACKETS.\n");

               sdsuFrameShow ( pFrame );
#endif

               /*
                * Fudge the frame number in the header so that detControl 
                * behaves correctly.
                */

               /*if ( pFrame->header.frameCount == 1 )
                  lastFrame = TRUE ;
               else*/
                  pFrame->header.frameCount = pFrame->totalFrames - frame;
#ifdef DEBUG
               printf ( "header.frameCount=%d, lastFrame = %d\n",
                        pFrame->header.frameCount, lastFrame);
#endif
               /*
                * Ensure the aborted bit is set in the frame header if the 
                * frame has been aborted.
                */

               if ( sdsuReadContext->aborted )
               {
                  pFrame->header.status |= SDSU_FSTAT_ABORTED;
               }

               /*
                * Record the number of frame failures.
                */

               sdsuReadContext->frameErrors = failures;

               /*
                * The frame is complete. In debug mode check the header 
                * status.
                */

#ifdef DEBUG
               if ( (pFrame->header.status & SDSU_FSTAT_OVERRUN) != 0 )
               {
                  printf (
                  "readTask: Overrun error bit set in frame header\n");
               }
               if ( (pFrame->header.status & SDSU_FSTAT_CHECKSUM) != 0 )
               {
                  printf (
                  "readTask: Checksum error bit set in frame header\n");
               }
               if ( (pFrame->header.status & SDSU_FSTAT_FRAMESYNC) != 0 )
               {
                  printf (
                  "readTask: Framesync error bit set in frame header\n");
               }
               if ( (pFrame->header.status & SDSU_FSTAT_ABORTED) != 0 )
               {
                  printf (
                  "readTask: Frame aborted bit set in frame header\n");
               }
               if ( (pFrame->header.status & SDSU_FSTAT_TIMEOUT) != 0 )
               {
                  printf ("readTask: Timeout bit set in frame header\n");
               }
               if ( (pFrame->header.status & SDSU_FSTAT_NOK) != 0 )
               {
                  printf ("readTask: Overwritten bit set in frame header\n");
               }
#endif

               /* 
                * If the observation has not been aborted, call the frame 
                * callback function. 
                */

#ifdef DEBUG
               printf ("readTask: Frame callback %p %p %p\n", 
                       sdsuReadContext, sdsuReadContext->appPrivate, pFrame);
#endif

               if ((!sdsuReadContext->aborted) && (frameCall != NULL))
               {
                  (*frameCall) (sdsuReadContext, sdsuReadContext->appPrivate,
                                pFrame);
               }
               else
               {
#ifdef DEBUG
                  printf (
                  "readTask: No frame callback since frame aborted\n");
#endif 
               }

            }

            /*
             * Observation complete.
             * Release the frame and go back and wait for the next observation.
             */

            if (sdsuPrimitive (sdsuReadContext, "ABT", SDSU_IDENT_VME, 
                               NULL, NULL) == ERROR )
            {
               ERROR_SET (0, 
                          "Failed to abort observation when stop observation",
                          ERROR_LOG_NOW);
            }
            sdsuFrameRelease (sdsuReadContext, pFrame);
            sdsuReadContext->readFrame = NULL;
            if ( sdsuReadContext->readStatus != SDSU_READ_ERROR )
            {
               sdsuReadContext->readStatus = SDSU_READ_IDLE;
            }
         }
      }          /* else if pFrame == NULL */
   }
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_readSignal
 *
 *   INVOCATION:
 *   sdsu_readSignal (sigNumber)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   sigNumber      (int)   SDSU_ID for this controller
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Signal handler for readout task
 *
 *   DESCRIPTION:
 *   The readout task is sent a SIGUSR1 signal by sdsu_frameSyncIsr. On receipt
 *   of this the sdsu_readSignal routine flags the current frame as complete.
 *   If there are further exposures to be read out, another frame is found, 
 *   passed to the VME DSP and queued for the image processing to work on.
 *
 *   NOTE:
 *   This code executes in the context of the task executing sdsu_readTask, 
 *   and therefore inherits that task's priority and task variables. This is 
 *   how sdsuReadContext is shared.
 *
 *   SEE ALSO:
 *   sdsu_readTask
 *   
 *   EXTERNAL VARIABLES:
 *   (>)   sdsuReadContext    (SDSU_ID)   task variable holding SDSU context
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   It is possible for the execution of the signal handler to be blocked if 
 *   the CPU is busy executing a packet or frame callback at the time when a 
 *   frame sync interrupt happens. Unfortunately, setting task priorities does 
 *   not solve this problem since the signal handler inherits the same priority
 *   as the readout task.
 *
 *   If an error occurs in the signal handler it is difficult for the detector 
 *   control task to discover that anything has gone wrong.
 *-
 */

void sdsu_readSignal ( int sigNumber )
{
   SDSU_FRAME *pFrame = sdsuReadContext->readFrame; /* Current SDSU frame ptr */

   const SDSU_FRAME *pBadFrame = NULL;                   /* Bad frame pointer */

   BOOL             skipFrame;

   /* Ensure the signal handler runs at high priority. */ 

   taskPrioritySet ( taskIdSelf(), SDSU_READTASK_PRIORITY );

#ifdef DEBUG
   printf ("sdsu_readSignal: %d, pFrame=%p\n", sigNumber, pFrame);
#endif /* DEBUG */

   /* Check there's something to do */
   if (pFrame == NULL)
   {
      ERROR_SET (S_sdsuLib_INTERNAL_ERROR, 
                 "Frame sync signal received with no frame",
                 ERROR_LOG_NOW);
      return;
   }

   /* Flag the frame as complete */
   pFrame->header.status |= SDSU_FSTAT_COMPLETE;
   if (pFrame->header.frameCount == 1)
   {
      /* No more frames to come */
      sdsuReadContext->readFrame = NULL;
      return;
   }

   /*
    * Find a spare frame in the frame buffer.
    * Ideally a frame should be grabbed immediately from the buffer, but if 
    * a frame is not available immediately the function will wait for 
    * SDSU_FRAME_FIND_TIMEOUT ticks until one becomes free. It is very 
    * important to keep synchronised with the SDSU controller, and any delay 
    * in finding a new frame here may cause the controller to lose 
    * synchronisation and throw away data. (The SDSU_FSTAT_OVERRUN bit and 
    * possibly also the SDSU_FSTAT_FRAMESYNC bit in the data header status 
    * word will be set if this happens).
    */

#ifdef DEBUG
   printf ("sdsu_readSignal: Getting free frame ...\n");
#endif /* DEBUG */

   skipFrame = FALSE;
   if (sdsuFrameFind (sdsuReadContext, NO_WAIT, &pFrame) == ERROR)
   {
      /* 
       * If a new frame could not be obtained, reuse the previous frame, which 
       * will result in data loss.
       */

      pFrame = sdsuReadContext->readFrame;
      skipFrame = TRUE;
#ifdef DEBUG
      printf ("sdsu_readSignal: Reusing %p\n", pFrame);
#endif /* DEBUG */
   }

   /*
    * Set the frame buffer address (FBA) in the SDSU controller to match the 
    * address of the frame just found.
    */

   if (sdsuFrameSetFBA (sdsuReadContext, pFrame) == ERROR)
   {
   ERROR_SET (0, 
   "sdsu_readSignal: Failed to set up frame buffer address for next frame DMA",
   ERROR_LOG_NOW);
   sdsuFrameRelease (sdsuReadContext, pFrame);
   sdsuReadContext->readFrame = NULL;
   goto ERROR_EXIT;
   }

   /* Send a message to sdsu_readTask giving it the address of the new frame. */

   if ( !skipFrame )
   {
      if (msgQSend (sdsuReadContext->frameQueue, (char *) &pFrame, 
                sizeof(SDSU_FRAME *), NO_WAIT, MSG_PRI_NORMAL) == ERROR)
      {
        ERROR_SET (0, 
        "sdsu_readSignal: Unable to send frame to message queue for processing",
        ERROR_LOG_NOW);
      }
   }

   return;

ERROR_EXIT:

   /* Send a message to sdsu_readTask giving it the address of a bad frame. */

   if (msgQSend (sdsuReadContext->frameQueue, (char *) &pBadFrame, 
             sizeof(SDSU_FRAME *), NO_WAIT, MSG_PRI_NORMAL) == ERROR)
   {
    ERROR_SET (0, 
    "sdsu_readSignal: Unable to send bad frame to message queue for processing",
    ERROR_LOG_NOW);
    /* PROBLEM: WE ARE NOW STUFFED AND CAN'T TELL THE OUTSIDE WORLD AN ERROR 
       HAS HAPPENED */
    return;
   }

   return;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuReadoutStart
 *
 *   INVOCATION:
 *   sdsuReadoutStart (context, totalFrames, pPrivate)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context      (SDSU_ID)  Context ID
 *   (>) totalFrames  (int)      Number of frames to read out
 *   (>) pPrivate     (void *)   Application private pointer for callbacks
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if readout could not be aborted.
 *
 *   PURPOSE:
 *   Start detector readout
 *
 *   DESCRIPTION:
 *   This routine starts readout of data from the SDSU controller. 
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   The readout task must have been created by calling sdsuReadoutOpen() for
 *   this context, but there must be no active readout in progress.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuReadoutStart
   (
   SDSU_ID     context,
   int         totalFrames,
   void *      pPrivate
   )
{
   SDSU_FRAME *pFrame;
   
   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf ("sdsuReadoutStart: context=%p, pPrivate=%p\n", context, pPrivate);
#endif /* DEBUG */   

   /* Ensure the readout task is idle */
   
   if (context->readStatus != SDSU_READ_IDLE)
   {
      ERROR_SET (S_sdsuLib_READOUT_ACTIVE, "Readout task not idle", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   /*
    * Get sdsu_readTask ready to receive the data.
    * Find a spare frame in the frame buffer.
    * No timeout is specified because at the start of an observation all the 
    * frames should be available.
    */

#ifdef DEBUG
   printf ("sdsuReadoutStart: Getting free frame ...\n");
#endif /* DEBUG */

   if (sdsuFrameFind (context, NO_WAIT, &pFrame) == ERROR)
   {
      ERROR_SET (0, "sdsuReadoutStart: Failed to get frame for first readout", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* 
    * Set the SDSU Frame Buffer Address to the address of the frame just 
    * obtained. 
    */
   
#ifdef DEBUG
   printf ("sdsuReadoutStart: Set FBA\n");
#endif /* DEBUG */

   if (sdsuFrameSetFBA (context, pFrame) == ERROR)
   {
      ERROR_SET (0, 
      "Failed to set up frame buffer address for first frame DMA", 
      ERROR_LOG_SAVE);
      sdsuFrameRelease(context, pFrame);
      context->readFrame = NULL;
      return (ERROR);
   }

   /*
    * Send an initial message to the frame queue indicating the location of 
    * the frame buffer and the number of frames to be processed.
    */

#ifdef DEBUG
   printf ("sdsuReadoutStart: Write message queue\n");
#endif /* DEBUG */

   pFrame->totalFrames = totalFrames;
   if (msgQSend (context->frameQueue, (char *) &pFrame, 
       sizeof(SDSU_FRAME *), NO_WAIT, MSG_PRI_NORMAL) == ERROR)
   {
      ERROR_SET (0, "Unable to send frame to message queue for processing", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   /* Give callbacks the right application context */
   context->appPrivate = pPrivate;
   
   /* Do it */

#ifdef DEBUG
   printf ("sdsuReadoutStart: RDC\n");
#endif /* DEBUG */

   if (sdsuPrimitive (context, "RDC", SDSU_IDENT_VME, NULL, NULL) == ERROR)
   {
      ERROR_SET (0, "Readout CCD (RDC) command failed", ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuSimpleReadoutStart
 *
 *   INVOCATION:
 *   sdsuSimpleReadoutStart (context, totalFrames, pPrivate)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context      (SDSU_ID)  Context ID
 *   (>) totalFrames  (int)      Number of frames to read out
 *   (>) pPrivate     (void *)   Application private pointer for callbacks
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if readout could not be aborted.
 *
 *   PURPOSE:
 *   Start detector readout
 *
 *   DESCRIPTION:
 *   This routine starts readout of data from the SDSU controller. 
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   The readout task must have been created by calling sdsuReadoutOpen() for
 *   this context, but there must be no active readout in progress.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuSimpleReadoutStart
   (
   SDSU_ID     context,
   int         totalFrames,
   void *      pPrivate
   )
{
   SDSU_FRAME *pFrame;
   
#ifdef DEBUG
   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   printf ("sdsuReadoutStart: context=%p, pPrivate=%p\n", context, pPrivate);
#endif /* DEBUG */   

   /* Ensure the readout task is idle */
   
   if (context->readStatus != SDSU_READ_IDLE)
   {
      ERROR_SET (S_sdsuLib_READOUT_ACTIVE, "Readout task not idle", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   /*
    * Get sdsu_readTask ready to receive the data.
    * Find a spare frame in the frame buffer.
    * No timeout is specified because at the start of an observation all 
    * the frames should be available.
    */

#ifdef DEBUG
   printf ("sdsuReadoutStart: Getting free frame ...\n");
#endif /* DEBUG */

   if (sdsuFrameFind (context, NO_WAIT, &pFrame) == ERROR)
   {
      ERROR_SET (0, "sdsuReadoutStart: Failed to get frame for first readout", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Give callbacks the right application context */
   context->appPrivate = pPrivate;

   /*
    * Send an initial message to the frame queue indicating the location of 
    * the frame buffer and the number of frames to be processed.
    */

#ifdef DEBUG
   printf ("sdsuReadoutStart: Write message queue\n");
#endif /* DEBUG */

   pFrame->totalFrames = totalFrames;
   if (msgQSend (context->frameQueue, (char *) &pFrame, 
             sizeof(SDSU_FRAME *), NO_WAIT, MSG_PRI_NORMAL) == ERROR)
   {
      ERROR_SET (0, "Unable to send frame to message queue for processing", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* The readout thread will now execute and handle the observation. */
      
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuReadoutAbort
 *
 *   INVOCATION:
 *   sdsuReadoutAbort (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context         (SDSU_ID)      Context ID
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if readout could not be aborted.
 *
 *   PURPOSE:
 *   Abort detector readout at the earliest opportunity
 *
 *   DESCRIPTION:
 *   This routine will abort readout of data from the SDSU controller. There
 *   will be a delay before this action finishes, and the calling task will
 *   block until abortion completes or until a timeout error occurs; the timeout
 *   period is that of the "ABT" entry of sdsuCmdTable, and is fixed.  
 *   No action is
 *   taken and the routine returns OK if the readout task is not active when
 *   this routine is executed.  The image processing readout task will continue
 *   to run and process all the images received.  When the abort frame arrives it
 *   will turn quiescent until another readout starts or the task is stopped by
 *   a call to sdsuReadoutClose().
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuReadoutAbort ( SDSU_ID   context )
{

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf ("sdsuReadoutAbort: context=%p\n", context);
#endif /* DEBUG */

   /* We're happy if the readout task doesn't exist or is idle */

   /*
    * THE FOLLOWING COMMENTED OUT. AN ABT COMMAND SHOULD BE ISSUED ANYWAY,
    * AS IT CAN OFTEN FIX AN SDSU HANGUP WHEN AN OBSERVATION IS NOT APPARENTLY
    * TAKING PLACE. SMB - 17 JAN 1999.
    */

/*
   if ((context->readStatus == SDSU_READ_CLOSED) ||
      (context->readStatus == SDSU_READ_IDLE))
      return (OK);
*/
   
   /* Send an ABT command, which should stop it all */
   
   if (sdsuPrimitive(context, "ABT", SDSU_IDENT_VME, NULL, NULL) == ERROR)
   {
      ERROR_SET (0, "Readout abort command (ABT) failed", ERROR_LOG_SAVE);
      return (ERROR);
   }
   /* add 14 oct 99 for testing slow stop pb */
   printf ( "ABT sent and DON returned \n" ) ;

   /*
    * Ensure any current frame is completed and the readout task
    * returns to its IDLE state by flushing the packet semaphore and setting
    * the "frame completed" and "frame aborted" bits in the frame header.
    * Also set the "aborted" flag in the context structure.
    */

   context->aborted = TRUE;
   if ( context->readFrame != NULL )
   {
      context->readFrame->header.status |= SDSU_FSTAT_ABORTED;
      context->readFrame->header.status |= SDSU_FSTAT_COMPLETE;
      semFlush (context->packetSem);
      /* add 14 oct 99 for testing slow stop pb */
      printf ( "readFrame and packetSem reset \n" ) ;
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuReadoutClose
 *
 *   INVOCATION:
 *   sdsuReadoutClose (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context         (SDSU_ID)      Context ID
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if readout could not be aborted.
 *
 *   PURPOSE:
 *   Shut down detector readout task
 *
 *   DESCRIPTION:
 *   This call stops the readout task for the given context without attempting
 *   to communicate to the SDSU controller, thus it can be used in an emergency
 *   situation to recover control in the event of the task taking up too much
 *   CPU time.  The effect of this on an active readout process has yet to be
 *   worked out in detail, but it should just result in frames after the one
 *   currently reading out being dropped by the VME DSP.  In normal operation
 *   any readout which is active should be aborted before calling this routine.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuReadoutClose ( SDSU_ID   context )
{

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf ("sdsuReadoutClose: context=%p\n", context);
#endif /* DEBUG */


   /* We're happy if the readout task doesn't exist */
   
   if (context->readTask == 0)
      return (OK);
   
   if (taskDelete(context->readTask) == ERROR)
   {
      ERROR_SET (0, "Readout task could not be deleted", ERROR_LOG_SAVE);
      return (ERROR);
   }
   
   context->readStatus = SDSU_READ_CLOSED;
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuParamWrite
 *
 *   INVOCATION:
 *   sdsuParamWrite (context, destId, paramName, paramValue)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context      (SDSU_ID)        Context ID
 *   (>)   destId       (const uint32)   ID of destination DSP for parameters
 *   (>)   paramName    (char *)         Name of parameter to change
 *   (>)   paramValue   (uint32)         Parameter value to write
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the parameters could not be written to the DSP.
 *
 *   PURPOSE:
 *   Write parameter value to SDSU DSP
 *
 *   DESCRIPTION:
 *   This routine writes the parameter paramValue to the address identified by 
 *   the paramName in the DSP identified by ID destId, which can take one of the
 *   values specified in the table below.
 *
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   The parameters for each of the three DSPs (VME, Timing and Utility) are 
 *   defined by the symbols found in the object file which has been downloaded 
 *   to the DSP. Parameter values are written using the WRM command, which 
 *   means that this call cannot be used to communicate with the Timing or 
 *   Utility DSPs during exposure readout operation.
 *
 *   SEE ALSO:
 *   sdsuParamRead
 *   sdsuParamWRP
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   This routine can only update parameters one at a time.
 *-
 */

STATUS   sdsuParamWrite
   ( 
   SDSU_ID        context,
   const uint32   destId,
   char *         paramName,
   uint32         paramValue
   )
{
   uint32          address;


   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf ("sdsuParamWrite: Setting parameter %s on DSP %lu to %lu (%#x)\n", 
           paramName, destId, paramValue, paramValue);
#endif /* DEBUG */

   if (symFindByNameAndType(context->paramSyms, paramName, (char **) &address, 
       NULL, (SYM_TYPE) destId, (SYM_TYPE) ~0) == ERROR)
   {
      ERROR_SET1 (S_sdsuLib_INV_PARAM_NAME, "Parameter %s not found", 
                  ERROR_LOG_SAVE, paramName);
      return (ERROR);
   }

   /* Write the data. */

   if (sdsuPrimitiveWRM (context, destId, address, paramValue) == ERROR)
   {
      ERROR_SET2 (0, 
      "Failed to write %s parameter to DSP memory at address %#x", 
      ERROR_LOG_SAVE,
      paramName, address);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuParamWRP
 *
 *   INVOCATION:
 *   sdsuParamWRP (context, destId, paramName, paramValue)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context     (SDSU_ID)        Context ID
 *   (>)   destId      (const uint32)   ID of destination DSP for parameters
 *   (>)   paramName   (char *)         Name of parameter to change
 *   (>)   paramValue  (uint32)         Parameter value to write
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the parameters could not be written to the DSP.
 *
 *   PURPOSE:
 *   Write parameter value to SDSU DSP
 *
 *   DESCRIPTION:
 *   This routine writes the parameter paramValue to the Y address identified 
 *   by the paramName in the DSP identified by ID destId, which can take one 
 *   of the values specified in the table below. Note however that only the 
 *   Timing DSP currently implements the WRP command, thus this routine cannot 
 *   be used to send parameter updates to the other DSPs with the current DSP 
 *   software.
 *
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   The parameters for each of the three DSPs (VME, Timing and Utility) are 
 *   defined by the symbols found in the object file which has been downloaded 
 *   to the DSP. Parameter values are written using the WRP command, which 
 *   means that this call *can* be used to communicate with the Timing DSP 
 *   during an exposure/readout operation, and after all parameter changes 
 *   have been written an LDP command must be sent by the application to cause 
 *   these changes to take effect.
 *
 *   SEE ALSO:
 *   sdsuParamRead
 *   sdsuParamWRP
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   This routine can only update parameters one at a time.
 *-
 */

STATUS   sdsuParamWRP
   ( 
   SDSU_ID        context,
   const uint32   destId,
   char *         paramName,
   uint32         paramValue
   )
{
   uint32         pAddressData[2];

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf ("sdsuParamWRP: Requesting %s on DSP %lu becomes %lu\n", 
           paramName, destId, paramValue);
#endif

   if (symFindByNameAndType(context->paramSyms, paramName, 
       (char **) &pAddressData[0], NULL, (SYM_TYPE) destId, (SYM_TYPE) ~0) 
       == ERROR)
   {
      ERROR_SET1 (S_sdsuLib_INV_PARAM_NAME, "Parameter %s not found", 
                  ERROR_LOG_SAVE, paramName);
      return (ERROR);
   }
   
   pAddressData [1] = paramValue;
   
   if ((pAddressData[0] & SDSU_MEM_SPACE_MASK) != SDSU_MEM_SPACE_Y)
   {
      ERROR_SET1 (S_sdsuLib_INV_DSP_ADDRESS, "Parameter %s not in Y memory", 
                  ERROR_LOG_SAVE, paramName);
      return (ERROR);
   }

   /* Write the parameter. */

   if (sdsuPrimitive (context, "WRP", destId, pAddressData, NULL) == ERROR)
   {
      ERROR_SET1 (0, "Failed to write %s parameter to DSP memory using WRP", 
                  ERROR_LOG_SAVE, paramName);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuParamRead
 *
 *   INVOCATION:
 *   sdsuParamRead (context, destId, paramName, pValue)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context     (SDSU_ID)      Context ID
 *   (>)   destId      (const uint32) ID of source DSP for parameters
 *   (>)   paramName   (char *)       Parameter name
 *   (!)   pValue      (uint32 *)     Where to put the parameter value
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the parameters could not be read.
 *
 *   PURPOSE:
 *   Read parameter value from an SDSU DSP
 *
 *   DESCRIPTION:
 *   This routine reads a named location paramName from a particular SDSU
 *   DSP indicated by destId into the local variable pointed to by pValue.
 *   destId can take one of the values specified in the table below.
 *
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   The single parameter value is written to the location pointed to by pValue.
 *
 *   SEE ALSO:
 *   sdsuParamWrite
 *   sdsuParamWRP
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   This routine only reads a single parameter value.
 *   It cannot be used while the controller is performing expose/readout cycles.
 *-
 */

STATUS   sdsuParamRead
   ( 
   SDSU_ID        context,
   const uint32   destId,
   char *         paramName,
   uint32 *       pValue
   )
{
   uint32          address;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef DEBUG
   printf ("sdsuParamRead: Reading %s from DSP %lu ...", paramName, destId);
#endif

   if (symFindByNameAndType(context->paramSyms, paramName, (char **) &address, 
                      NULL, (SYM_TYPE) destId, (SYM_TYPE) ~0) == ERROR)
   {
      ERROR_SET1 (S_sdsuLib_INV_PARAM_NAME, "Parameter %s not found", 
                  ERROR_LOG_SAVE, paramName);
      return (ERROR);
   }

   /* Read the parameter   */

   if (sdsuPrimitiveRDM (context, destId, address, pValue) == ERROR)
   {
      ERROR_SET2 (0, 
      "Failed to read %s parameter from DSP memory at address %#x", 
       ERROR_LOG_SAVE, paramName, address);
      return (ERROR);
   }

#ifdef DEBUG
   printf (" got %d\n ", *pValue);
#endif

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuParamPrint
 *
 *   INVOCATION:
 *   sdsuParamPrint (context, destId, name)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context  (SDSU_ID)        Context ID
 *   (>)   destId   (const uint32)   ID for DSP parameter table to print
 *   (>)   name     (const char *)   Parameter name
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the parameter could not be read.
 *
 *   PURPOSE:
 *   Print DSP parameter value to VxWorks console
 *
 *   DESCRIPTION:
 *   This routine prints out the value of the named parameter for a specified 
 *   DSP, whose ID is specified in destId. Valid DSP IDs are listed in the 
 *   table below.
 *
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   The parameter name is looked up in the parameter symbol table given by the
 *   symbol addresses in the download file, and the value at this address is 
 *   read from the DSP and printed.
 *
 *   SEE ALSO:
 *   sdsuParamRead
 *
 *   NOTE:
 *   This function is intended only for debugging
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   This routine only prints a single parameter value.
 *-
 */

STATUS   sdsuParamPrint
   (
   SDSU_ID        context,
   const uint32   destId,
   char *         paramName
   )
{
   uint32         value;
   char *         dspName;
   
   /* Look up DSP name string */
   switch (destId) {
      case SDSU_IDENT_VME:
         dspName = "VME";
         break;
      
      case SDSU_IDENT_TIM:
         dspName = "TIM";
         break;
      
      case SDSU_IDENT_UTL:
         dspName = "UTL";
         break;
      
      default:
         ERROR_SET (S_sdsuLib_INV_PROC_ID, "Invalid DSP target", 
                    ERROR_LOG_SAVE);
         return (ERROR);
   }
   
   if (sdsuParamRead (context, destId, paramName, &value) == ERROR)
   {
      ERROR_LOG ("Failed to read parameter for printing");
      return (ERROR);
   }
   printf ("%s: %s = %lu (%#x)\n", dspName, paramName, value, value);
   
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuParamDnload
 *
 *   INVOCATION:
 *   sdsuParamDnload (context, pFileName, destId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context   (SDSU_ID)    Context ID
 *   (>) pFileName (char *)     Name of file containing parameter list
 *   (>) destId    (const long) ID of DSP to which parameters will be downloaded
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if an error occured while downloading.
 *
 *   PURPOSE:
 *   Download a list of parameter values from a file to an SDSU DSP
 *
 *   DESCRIPTION:
 *   This routine reads as ASCII file containing a list of parameters
 *   and values of the form
 *
 *      Param1 name   =>   Param1 value
 *      Param2 name   =>   Param2 value
 *      etc...        =>   etc...
 *
 *   The file can begin with a comment block containing a ";" in column 1.
 *   The parameter names and values are separated by white space. Each 
 *   name/value pair is terminated by a newline. The parameters are directed 
 *   to the DSP specified by destId, which can take one of the values specified
 *   in the table below.
 *
 *      -1               =>   Determine DSP from each parameter name
 *      SDSU_IDENT_HST   =>   Refers to the host CPU
 *      SDSU_IDENT_VME   =>   Refers to VME DSP
 *      SDSU_IDENT_TIM   =>   Refers to Timing DSP
 *      SDSU_IDENT_UTL   =>   Refers to Utility DSP
 *
 *   If the DSP is determined from the parameter name, the first two characters
 *   of the each parameter name are used according to the this table.
 *
 *      V_   =>   Refers to VME DSP
 *      T_   =>   Refers to Timing DSP
 *      U_   =>   Refers to Utility DSP
 *
 *   This allows parameters to be downloaded from one file and directed to the
 *   appropriate DSPs.
 *
 *   NOTE:
 *   Parameters are set in the order they are found in the file. Parameters may
 *   not be set "on-the-fly" by downloading them from a file. 
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   REFERENCES:
 *   A list and description of the SDSU parameters may be found in ICD 1.6/1.10.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   The function is not very flexible when it comes to parsing the parameter 
 *   file. Comments can only occur at the beginning of the file and have to 
 *   have a ";" in column 1. In-line comments are not currently supported. 
 *   The remainder of the file after the comment block is assumed to consist 
 *   of parameter name/value pairs only.
 *
 *   At present, files uploaded using this sdsuParamUpload cannot be downloaded
 *   without first editing the parameter file. Allowing in-line comments 
 *   beginning with a ";" will solve this deficiency.
 *-
 */

STATUS sdsuParamDnload
   (
   SDSU_ID        context,
   char *         pFileName,
   const long     destId
   )
{
   char    pParamFileName [MAX_CHAR_FILENAME + 1];
                      /* Full name of parameter file, including ".par" suffix */
   FILE    *pParamFile;     /* File descriptor associated with parameter file */
   int     nread;                          /* Number of items read from file  */
   int     nextChar;                       /* Character sensed from next line */
   int     failures;                       /* Number of conversion failures   */
   char    pParamString[OMF_MAX_CHARS_PER_LINE + 1];
                                           /* String read from parameter file */
   char    *pHexString; /* Pointer to hexadecminal portion of string (if any) */
   char    pParamName[SDSU_SYM_NAME_LEN + 1];
                                  /* Parameter name read from parameter file  */
   uint32  paramValue;            /* Parameter value read from parameter file */
   uint32  dspId;                 /* Actual DSP ID for parameter              */

   /*
    * Check the SDSU context structure.
    */

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Append the string ".par" if it is not already present in the file name.
    */

   strncpy (pParamFileName, pFileName, MAX_CHAR_FILENAME);
   if ((strstr (pParamFileName, ".par") == NULL) && 
       (strstr (pParamFileName, ".PAR") == NULL))
   {
      strcat (pParamFileName, ".par");
   }

   /*
    * Open the file for read access.
    */

   if ((pParamFile = fopen (pParamFileName, "r")) == NULL)
   {
      ERROR_SET1 (0, "Failed to open parameter file, %s", 
                  ERROR_LOG_SAVE, pParamFileName);
      return (ERROR);
   }

   /*
    * Skip over any comment lines at the beginning of the file denoted by a 
    * ";" in column 1.
    * First test the next character for ';'
    */
   while ( (nextChar = fgetc (pParamFile)) == ';' ) 
   {
      ungetc (nextChar, pParamFile);            /* Undo the effect of fgetc() */
      fgets (pParamString, OMF_MAX_CHARS_PER_LINE, pParamFile);
                                                       /* Skip the whole line */
   }
   ungetc (nextChar, pParamFile);                   /* Undo the final fgetc() */

   /*
    * Read through the rest of the file, reading pairs of tokens and assuming
    * these are pairs of parameter names and values.
    */

   failures = 0;
   while ((nread = fscanf (pParamFile, "%s %s", pParamName, pParamString)) 
          != EOF)
   {
      if ( nread == 2 )
      {

         /*
          * If the DSP destination, destId, is specified as -1, extract the 
          * destination DSP identifier from first 2 characters of the parameter 
          * name; otherwise use the DSP identifier specified. (Specifying -1 
          * allows each parameter from a single file to be individually 
          * directed to the appropriate DSP).
          */

         if ( destId == -1 )
         {
            if ( strncmp (pParamName, "V_", 2) == 0 )
            {
               dspId = SDSU_IDENT_VME;
            }
            else if ( strncmp (pParamName, "T_", 2) == 0 )
            {
               dspId = SDSU_IDENT_TIM;
            }
            else if ( strncmp (pParamName, "U_", 2) == 0 )
            {
               dspId = SDSU_IDENT_UTL;
            }
            else
            {
               dspId = SDSU_IDENT_HST;
            }
         }
         else
         {
            dspId = (uint32) destId;
         }

         /*
          * If the parameter value begins with "0x" or "$" assume it is a 
          * hexadecimal integer, otherwise assume it is decimal integer.
          */

         if ( (pHexString = strstr (pParamString, "0x")) == NULL )
         {
            /* The string does not contain "0x". Try "$". */

            if ( (pHexString = strstr (pParamString, "$")) == NULL )
            {
               /* 
                * The string contains neither "0x" nor "$". Assume it is 
                * decimal
                */

               if (sscanf (pParamString, "%lu", &paramValue) != 1)
               {
                  /*
                   * Conversion failure. This is not fatal, so issue a warning
                   * and increment counter.
                   */

                  MESSAGE_LOG2 (MSG_WARNING,
                     "Failed to evaluate parameter %s from %s (decimal)",
                       pParamName, pParamString);
                  failures++;
                  continue;        /* Abort this iteration of the while loop. */ 
               }
            }
            else
            {
               /* 
                * The string contains "$". Assume a hexadecimal value starts 
                * at pHexString+1
                */

               if (sscanf (pHexString+1, "%lx", &paramValue) != 1)
               {
                  /*
                   * Conversion failure. This is not fatal, so issue a warning
                   * and increment counter.
                   */

                  MESSAGE_LOG2 (MSG_WARNING,
                     "Failed to evaluate parameter %s from %s (hexadecimal)",
                     pParamName, pParamString);
                  failures++;
                  continue;        /* Abort this iteration of the while loop. */ 
               }
            }
         }
         else
         {
            /* 
             * The string contains "0x". Assume a hexadecimal value starts at 
             * pHexString+2
             */

            if (sscanf (pHexString+2, "%lx", &paramValue) != 1)
            {
               /*
                * Conversion failure. This is not fatal, so issue a warning
                * and increment counter.
                */

               MESSAGE_LOG2 (MSG_WARNING,
                  "Failed to evaluate parameter %s from %s (hexadecimal)",
                  pParamName, pParamString);
               failures++;
               continue;            /* Abort this iteration of the while loop */
            }
         }

         MESSAGE_LOG3 (MSG_FULLDEBUG, "Setting parameter %s in DSP %lu to %lu.",
            pParamName, dspId, paramValue);

         if ( sdsuParamWrite (context, dspId, pParamName, paramValue) == ERROR )
         {
            ERROR_SET1 (0, "Error setting %s parameter", ERROR_LOG_SAVE, 
                        pParamName);
            fclose (pParamFile);
            return (ERROR);
         }
      }
      else
      {
         ERROR_SET2 (0, 
            "Problem reading parameter file at line containing %s %s",
            ERROR_LOG_SAVE, pParamName, pParamString);
         fclose (pParamFile);
         return (ERROR);
      }
   }

   if ( failures > 0 )
   {
      ERROR_SET2 (0, 
          "%d conversion failures occurred reading parameter file %s", 
          ERROR_LOG_SAVE,
          failures, pParamFileName);
      fclose (pParamFile);
      return (ERROR);
   }
      
   /*
    * Close the file and tidy up.
    */

   MESSAGE_LOG1 (MSG_MINDEBUG, "Downloaded parameters from %s OK.", 
                 pParamFileName);

   if (fclose (pParamFile) == ERROR)
   {
      ERROR_SET (0, "Failed to close parameter file", ERROR_LOG_SAVE);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuParamUpload
 *
 *   INVOCATION:
 *   sdsuParamUpload (context, pFileName, destId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context    (SDSU_ID)     Context ID
 *   (>) pFileName  (char *)      Name of file to contain parameter list
 *   (>) sourceId   (const long)  ID of DSP from which parameters will be 
 *                                uploaded
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if an error occured while uploading.
 *
 *   PURPOSE:
 *   Download a list of parameter values from a file to an SDSU DSP
 *
 *   DESCRIPTION:
 *   This routine writes an ASCII file containing a list of parameters
 *   and values of the form
 *
 *      Param1 name   =>   Param1 value
 *      Param2 name   =>   Param2 value
 *      etc...        =>   etc...
 *
 *   The parameters are uploaded from the DSP specified by sourceId,
 *   which can take one of the values specified in the table below.
 *
 *      -1               =>   Upload from all DSPs
 *      SDSU_IDENT_HST   =>   Upload from the host CPU
 *      SDSU_IDENT_VME   =>   Upload from VME DSP
 *      SDSU_IDENT_TIM   =>   Upload from Timing DSP
 *      SDSU_IDENT_UTL   =>   Upload from Utility DSP
 *
 *   NOTE:
 *   Only those parameters defined in ICD 1.6/1.10 are uploaded. 
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   REFERENCES:
 *   A list and description of the SDSU parameters may be found in ICD 1.6/1.10.
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   Only parameters known to this function will be uploaded. That means this 
 *   function has to be kept up to date with ICD 1.6/1.10.
 *
 *   An alternative but more complex implementation would download all the
 *   parameters contained in the context->paramSyms symbol table, using the
 *   symEach() function.
 *-
 */

STATUS sdsuParamUpload
   (
   SDSU_ID         context,
   char *          pFileName,
   const long      sourceId
   )
{
   char      pParamFileName [MAX_CHAR_FILENAME + 1];
                      /* Full name of parameter file, including ".par" suffix */
   FILE      *pParamFile;   /* File descriptor associated with parameter file */
   uint32    paramValue;    /* Parameter value obtained from DSP              */
   int       i;             /* Parameter name index                           */


   /* List of documented VME parameter names. */

   char *vmeParamNames[] = {"V_FW_ID",  "V_FW_VER", "V_SW_ID", "V_PSIZE",
                            "V_INT_EN", "V_PIID",   "V_FIID",  "V_FBAHI",
                            "V_FBALO"};
   int         vmeNparams = NELEMENTS (vmeParamNames);

   /* List of documented TIMING parameter names. */

   char *timParamNames[] = {"T_FW_ID",   "T_FW_VER", "T_SW_ID",   "T_STATUS",
                            "T_ERROR",   "T_USCAN",  "T_XSIZE",   "T_YSIZE",
                            "T_OUTPUTS", "T_RO_TIM", "T_EXP_TMR", "T_FRAMEC",
                            "T_EXP_TIM", "T_NFRAME", "T_GAIN_SP", "T_PARMID",
                            "T_MODE",    "T_SAMPLES","T_XSUBAP",  "T_YSUBAP",
                            "T_XSTART",  "T_YSTART", "T_XRAS",    "T_YRAS",
                            "T_XSPACE",  "T_YSPACE", "T_XBIN",    "T_YBIN",
                            "T_XTAIL",   "T_NPIXEL", "T_INT_TIM"};
   int timNparams = NELEMENTS (timParamNames);

   /* List of documented UTILITY parameter names. */

   char *utlParamNames[] = {"U_FW_ID",   "U_FW_VER",   "U_SW_ID",   "U_DIG_IN",
                            "U_DIG_OUT", "U_DAC0",     "U_DAC1",    "U_DAC2",
                            "U_DAC3",    "U_NUM_AD",   "U_ADC0",    "U_ADC1",
                            "U_ADC2",    "U_ADC3",     "U_ADC4",    "U_ADC5",
                            "U_ADC6",    "U_ADC7",     "U_ADC8",    "U_ADC9",
                            "U_ADC10",   "U_ADC11",    "U_ADC12",   "U_ADC13",
                            "U_ADC14",   "U_ADC15",    "U_AD_CCDT", "U_AD_TECV",
                            "U_TCF",     "U_CCDT_TGT", "U_TEC_MAX", "U_HV_TGT",
                            "U_HV_TGT",  "U_HV_TOL",   "U_P15_TGT", "U_P15_TOL",
                            "U_M15_TGT", "U_M15_TOL",  "U_HV_INI",  "U_P15_INI",
                            "U_M15_INI", "U_ERROR"};
   int utlNparams = NELEMENTS (utlParamNames);

   /*
    * Check the SDSU context structure.
    */

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Append the string ".par" if it is not already present in the file name.
    */

   strncpy (pParamFileName, pFileName, MAX_CHAR_FILENAME);
   if (strstr (pParamFileName, ".par") == NULL && 
       strstr (pParamFileName, ".PAR") == NULL)
      strcat (pParamFileName, ".par");

   /*
    * Create and open the parameter file for write access.
    */

   if ((pParamFile = fopen (pParamFileName, "w")) == NULL)
   {
      ERROR_SET1 (0, "Failed to open parameter file, %s", ERROR_LOG_SAVE, 
                  pParamFileName);
      return (ERROR);
   }

   /*
    * Upload all documented parameters from the VME DSP, if required.
    * If an error occurs while attempting to read a parameter, ignore
    * the error and write "********" into the file as a parameter value.
    */

   if ( (sourceId == SDSU_IDENT_VME) || (sourceId == -1) )
   {
      for ( i=0; i<vmeNparams; i++ )
      {
         if (sdsuParamRead (context, SDSU_IDENT_VME, vmeParamNames[i], 
             &paramValue) == ERROR)
         {
            errorClear();                  /* Ignore any error */
            fprintf (pParamFile, 
                     "%12s\t******** \t;\tFailed to read parameter\n",
                     vmeParamNames[i]);
         }
         else
         {
            fprintf (pParamFile, "%12s\t%8u \t;\t= 0x%08x\n",
                     vmeParamNames[i], paramValue, paramValue);
         }
      }
   }

   /*
    * Upload all documented parameters from the TIMING DSP, if required.
    * If an error occurs while attempting to read a parameter, ignore
    * the error and write "********" into the file as a parameter value.
    */

   if ( (sourceId == SDSU_IDENT_TIM) || (sourceId == -1) )
   {
      for ( i=0; i<timNparams; i++ )
      {
         if (sdsuParamRead (context, SDSU_IDENT_TIM, timParamNames[i], 
                            &paramValue) == ERROR)
         {
            errorClear();                  /* Ignore any error */
            fprintf (pParamFile, 
                     "%12s\t******** \t;\tFailed to read parameter\n",
                     timParamNames[i]);
         }
         else
         {
            fprintf (pParamFile, "%12s\t%8u \t;\t= 0x%08x\n",
                     timParamNames[i], paramValue, paramValue);
         }
      }
   }

   /*
    * Upload all documented parameters from the UTILITY DSP, if required.
    * If an error occurs while attempting to read a parameter, ignore
    * the error and write "********" into the file as a parameter value.
    */

   if ( (sourceId == SDSU_IDENT_UTL) || (sourceId == -1) )
   {
      for ( i=0; i<utlNparams; i++ )
      {
         if (sdsuParamRead (context, SDSU_IDENT_UTL, utlParamNames[i], 
                            &paramValue) == ERROR)
         {
            errorClear();                  /* Ignore any error */
            fprintf (pParamFile, 
                     "%12s\t******** \t;\tFailed to read parameter\n",
                     utlParamNames[i]);
         }
         else
         {
            fprintf (pParamFile, "%12s\t%8u \t;\t= 0x%08x\n",
                     utlParamNames[i], paramValue, paramValue);
         }
      }
   }

   /*
    * Close the file and tidy up.
    */

   MESSAGE_LOG1 (MSG_MINDEBUG, "Uploaded parameters to %s OK.", pParamFileName);

   if (fclose (pParamFile) == ERROR)
   {
      ERROR_SET (0, "Failed to close parameter file", ERROR_LOG_SAVE);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuStatusShow
 *
 *   INVOCATION:
 *   sdsuStatusShow (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context      (SDSU_ID)      Context ID
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the parameter could not be read.
 *
 *   PURPOSE:
 *   Print values of SDSU status parameters to VxWorks console
 *
 *   DESCRIPTION:
 *   This routine reads and prints out the values of all the known SDSU status 
 *   parameters, translating the contents of those parameters into a status 
 *   report.
 *
 *   SEE ALSO:
 *   sdsuParamRead
 *
 *   NOTE:
 *   This function is intended only for debugging
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   This function makes assumptions about the names of the SDSU status 
 *   parameters described in ICD 1.6/1.10 and needs to be changed each time 
 *   the names of those parameters change.
 *-
 */

STATUS sdsuStatusShow ( SDSU_ID context )
{
   char         paramName[SDSU_SYM_NAME_LEN + 1];
   uint32       value;
   STATUS       returnValue;

   /*
    * Initialise the return value. An attempt will be made to display every 
    * status parameter even if failures occur, so the return value is 
    * remembered until the end.
    */

   returnValue = OK;

   /* VME board status parameters */

   printf ("VME board status parameters\n");
   printf ("---------------------------\n");

   strcpy (paramName, "V_INT_EN");
   if (sdsuParamRead (context, SDSU_IDENT_VME, paramName, &value) == ERROR)
   {
      ERROR_LOG ("Failed to read status parameter");
      returnValue = ERROR;
   }
   else
   {
      printf ("VME: %s = %lu (%#x) [", paramName, value, value);

      if ( (value & SDSU_PACKET_INT_ENABLE) != 0 )
      {
         printf ("packet interrupts enabled, ");
      }
      else
      {
         printf ("packet interrupts disabled, ");
      }
      if ( (value & SDSU_FRAME_INT_ENABLE) != 0 )
      {
         printf ("frame interrupts enabled");
      }
      else
      {
         printf ("frame interrupts disabled");
      }
      printf ("]\n");
   }

   strcpy (paramName, "V_FIID");
   if (sdsuParamRead (context, SDSU_IDENT_VME, paramName, &value) == ERROR)
   {
      ERROR_LOG ("Failed to read status parameter");
      returnValue = ERROR;
   }
   else
   {
      printf ("VME: %s = %lu (%#x)\n", paramName, value, value);
   }

   strcpy (paramName, "V_PIID");
   if (sdsuParamRead (context, SDSU_IDENT_VME, paramName, &value) == ERROR)
   {
      ERROR_LOG ("Failed to read status parameter");
      returnValue = ERROR;
   }
   else
   {
      printf ("VME: %s = %lu (%#x)\n", paramName, value, value);
   }

   strcpy (paramName, "V_FBALO");
   if (sdsuParamRead (context, SDSU_IDENT_VME, paramName, &value) == ERROR)
   {
      ERROR_LOG ("Failed to read status parameter");
      returnValue = ERROR;
   }
   else
   {
      printf ("VME: %s = %lu (%#x)\n", paramName, value, value);
   }

   strcpy (paramName, "V_FBAHI");
   if (sdsuParamRead (context, SDSU_IDENT_VME, paramName, &value) == ERROR)
   {
      ERROR_LOG ("Failed to read status parameter");
      returnValue = ERROR;
   }
   else
   {
      printf ("VME: %s = %lu (%#x) [", paramName, value, value);

      if ( (value & SDSU_NEW_FBA_FLAG) == 0 )
      {
         printf ("old FBA value");
      }
      else
      {
         printf ("new FBA value");
      }
      printf ("]\n\n");
   }

   strcpy (paramName, "V_PSIZE");
   if (sdsuParamRead (context, SDSU_IDENT_VME, paramName, &value) == ERROR)
   {
      ERROR_LOG ("Failed to read status parameter");
      returnValue = ERROR;
   }
   else
   {
      printf ("VME: %s = %lu (%#x)\n", paramName, value, value);
   }

   /* TIMING board status parameters */

   printf ("TIMING board status parameters\n");
   printf ("------------------------------\n");

   strcpy (paramName, "T_STATUS");
   if (sdsuParamRead (context, SDSU_IDENT_TIM, paramName, &value) == ERROR)
   { 
      /* Force error message to be displayed. */
      ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_NOW);  
      returnValue = ERROR;
   }
   else
   {
      printf ("TIM: %s = %lu (%#x) [", paramName, value, value);

      if ( (value & SDSU_TIM_STATUS_IDLING) != 0 )
      {
         printf ("idling, ");
      }
      if ( (value & SDSU_TIM_STATUS_EXPOSING) != 0 )
      {
         printf ("exposing, ");
      }
      if ( (value & SDSU_TIM_STATUS_READING) != 0 )
      {
         printf ("reading out, ");
      }
      if ( (value & SDSU_TIM_STATUS_SYNC) != 0 )
      {
         printf ("readout sync requested ");
      }
      printf ("]\n");
   }

   strcpy (paramName, "T_ERROR");
   if (sdsuParamRead (context, SDSU_IDENT_TIM, paramName, &value) == ERROR)
   {
      /* Force error message to be displayed. */
      ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_NOW);   
      returnValue = ERROR;
   }
   else
   {
      printf ("TIM: %s = %lu (%#x) [", paramName, value, value);

      if ( (value & SDSU_TIM_ERROR_OVERRUN) != 0 )
      {
         printf ("overrun error, ");
      }
      if ( (value & SDSU_TIM_ERROR_WRP) != 0 )
      {
         printf ("WRP error ");
      }
      if ( value ==  0 )
      {
         printf ("no error");
      }
      printf ("]\n");
   }

   strcpy (paramName, "T_MODE");
   if (sdsuParamRead (context, SDSU_IDENT_TIM, paramName, &value) == ERROR)
   {   
      /* Force error message to be displayed. */
      ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_NOW);
      returnValue = ERROR;
   }
   else
   {
      printf ("TIM: %s = %lu (%#x) [", paramName, value, value);

      if ( (value & SDSU_TIM_MODE_UNDERSCAN) != 0 )
      {
         printf ("output underscan pixels,  ");
      }
      if ( (value & SDSU_TIM_MODE_SIMULATE) != 0 )
      {
         printf ("simulate data, ");
      }
      if ( (value & SDSU_TIM_MODE_SYNC) != 0 )
      {
         printf ("sync mode");
      }
      printf ("]\n");
   }

   strcpy (paramName, "T_PARMID");
   if (sdsuParamRead (context, SDSU_IDENT_TIM, paramName, &value) == ERROR)
   {   
      /* Force error message to be displayed. */
      ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_NOW);
      returnValue = ERROR;
   }
   else
   {
      printf ("TIM: %s = %lu (%#x) [", paramName, value, value);

      if ( (value & SDSU_TIM_PARMID_SYNC) == 0 )
      {
         printf ("sync mode disabled");
      }
      else
      {
         printf ("sync mode enabled");
      }
      printf ("]\n\n");
   }


   /* UTILITY board status parameters */

   printf ("UTILITY board status parameters\n");
   printf ("-------------------------------\n");

   strcpy (paramName, "U_ERROR");
   if (sdsuParamRead (context, SDSU_IDENT_UTL, paramName, &value) == ERROR)
   {
      /* Force error message to be displayed. */
      ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_NOW);   
      returnValue = ERROR;
   }
   else
   {
      printf ("UTL: %s = %lu (%#x) [", paramName, value, value);
      if ( value ==  0 )
      {
         printf ("no error");
      }
      printf ("]\n\n");
   }

   return (returnValue);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuTempShow
 *
 *   INVOCATION:
 *   sdsuTempShow (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   context      (SDSU_ID)      Context ID
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if the parameter could not be read.
 *
 *   PURPOSE:
 *   Print values of SDSU temperature parameters to VxWorks console
 *
 *   DESCRIPTION:
 *   This routine reads and prints out the values of all the known SDSU 
 *   temperature parameters. The chip temperature is sampled several times 
 *   and the mean value calculated.
 *
 *   SEE ALSO:
 *   sdsuParamRead
 *
 *   NOTE:
 *   This function is intended only for debugging
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   This function makes assumptions about the names of the SDSU status 
 *   parameters described in ICD 1.6/1.10 and needs to be changed each time 
 *   the names of those parameters change.
 *
 *   The coefficients used in the calculations within this function have been 
 *   copied from a similar function written by Tim Hardy. These do not seem to 
 *   correspond to the coefficients documented in ICD 1.6/1.10, which I find 
 *   confusing. SMB - 28 Jan 1999.
 *-
 */

STATUS   sdsuTempShow
   (
   SDSU_ID         context
   )
{
   uint32   value;
   double   meanValue6, meanValue7;
   double   sdsuTemp0, sdsuTemp6, sdsuTemp7, sdsuTargetTemp, sdsuTecVoltage;

   int      sample;


   printf ("Approximate SDSU temperature values\n");
   printf ("-----------------------------------\n");

   if (sdsuParamRead (context, SDSU_IDENT_UTL, "U_ADC0", &value) == ERROR)
   {
      ERROR_LOG ("Failed to read utility board temperature parameter");
      return (ERROR);
   }
   sdsuTemp0 = 25.0 + 0.38 * ((double) value - 2789.0);      /* 0xae5 = 2789 */
   printf ("Utility board = %2.2f C (U_ADC0 = %lu)\n", sdsuTemp0, value);

   meanValue6 = meanValue7 = 0.0;
   for ( sample=0; sample<20; sample++)
   {
      if (sdsuParamRead (context, SDSU_IDENT_UTL, "U_ADC6", &value) == ERROR)
      {
         ERROR_LOG ("Failed to read thermistor 1 temperature parameter");
         return (ERROR);
      }
      else
      {
         meanValue6 += (double) value;
      }

      if (sdsuParamRead (context, SDSU_IDENT_UTL, "U_ADC7", &value) == ERROR)
      {
         ERROR_LOG ("Failed to read thermistor 2 temperature parameter");
         return (ERROR);
      }
      else
      {
         meanValue7 += (double) value;
      }
   }

   meanValue6 /= 20.0;
   meanValue7 /= 20.0;

   sdsuTemp6 = meanValue6 * (-0.01545); /* 0.01545 is not quite SDSU_TEMP_UNIT*/
   sdsuTemp7 = meanValue7 * (-0.01545); /* 0.01545 is not quite SDSU_TEMP_UNIT*/

   printf (" Thermistor 1 = %2.2f C (mean U_ADC6 = %d)\n", sdsuTemp6, 
           (int) meanValue6);
   printf (" Thermistor 2 = %2.2f C (mean U_ADC7 = %d)\n", sdsuTemp7, 
           (int) meanValue7);

   if (sdsuParamRead (context, SDSU_IDENT_UTL, "U_CCDT_TGT", &value) == ERROR)
   {
      ERROR_LOG ("Failed to target temperature parameter");
      return (ERROR);
   }
   sdsuTargetTemp = ((double) value) * -1.0 * SDSU_TEMP_UNIT;
   printf (" Target temp  = %2.2f C (U_CCDT_TGT = %lu)\n", sdsuTargetTemp, 
           value);

   if (sdsuParamRead (context, SDSU_IDENT_UTL, "U_DAC2", &value) == ERROR)
   {
      ERROR_LOG ("Failed to TEC voltage parameter");
      return (ERROR);
   }
   sdsuTecVoltage = ((double) value - 2048.0) * (5.0/2048.0);
   printf (" TEC voltage  = %2.2f V (U_DAC2 = %lu)\n", sdsuTecVoltage, value);

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_testTDL
 *
 *   INVOCATION:
 *   sdsu_testTDL (context destId, verbose)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context    (SDSU_ID)         context ID
 *   (>) destId     (const uint32)    ID of DSP to test
 *   (>) verbose    (const BOOL)      enable verbose printout of test status
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if any of the RDL executions failed.
 *
 *   PURPOSE:
 *   Test ability to execute TDL command on SDSU DSP
 *
 *   DESCRIPTION:
 *   This routine attempts to execute a number of TDL commands on an SDSU
 *   controller in order to verify correct operation of the hardare and
 *   firmware. For the specified DSP, the routine will execute TDL with
 *   a variety of different arguments in order to test for the usual things
 *   like bits stuck high/low and adjacent bits shorted together.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsu_testTDL
   (
   SDSU_ID        context,
   const uint32   destId,
   const BOOL     verbose
   )
{
   uint32 dataOut[] = 
   {0x000000, 0xffffff, 0x555555, 0xaaaaaa, 0x5c5c5c, 0x123456};
   uint32 dataIn;
   int    i;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   if (verbose) printf ("Testing data link for DSP %d\n", destId);

   /* Do TDL for each test pattern      */
   for (i = 0; i < NELEMENTS (dataOut); i++) 
   {
      if (verbose) printf ("Test pattern %#x\n", dataOut [i]);

      if (sdsuPrimitive (context, "TDL", destId, & dataOut [i], & dataIn) 
          == ERROR)
      {
         ERROR_SET2 (0, "TDL failed writing %#x to DSP %d", ERROR_LOG_SAVE,
            dataOut [i], destId);
         return (ERROR);
      }

      /* 
       * The data mis-match test is not necessary if the controller is being 
       * simulated
       */

      if ((!context->simulate) && (dataOut [i] != dataIn))
      {
         ERROR_SET3 (S_sdsuLib_TEST_FAIL,
            "TDL data mis-match, wrote %#x but read %#x from DSP %d", 
            ERROR_LOG_SAVE, dataOut [i], dataIn, destId);
         return (ERROR);
      }

      if (verbose)
      {
         if (((context->pRepBuffer[context->repBufCounter]) & 0xff000000) != 0)
         printf(
         "****************************************************************\n");
         sdsuPrintRepBuf (context);
      }
   }
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_testRDM
 *
 *   INVOCATION:
 *   sdsu_testRDM (context, destId, pMemSpace, pAddress, verbose)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context    (SDSU_ID)      context ID
 *   (>) destId     (const uint32) ID of DSP to test
 *   (!) pMemSpace  (uint32 *)     points to memory-space identifier
 *   (!) pAddress   (uint32 *)     points to address to test
 *   (>) verbose    (const BOOL)   enable verbose printout of test status
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if any of the RDM executions failed.
 *
 *   PURPOSE:
 *   Test ability to execute RDM command on SDSU DSP
 *
 *   DESCRIPTION:
 *   This routine attempts to execute a number of RDM command on an SDSU
 *   controller in order to verify correct operation of the hardare and
 *   firmware. For the specified DSP, the routine will read the first and
 *   last address which define the range of valid addresses for each of the
 *   DSP's memory spaces. The pointers pMemSpace and pAddress are written with
 *   the memory-space and address being tested at any instant so that, in the
 *   event of an error, they can be examined by the calling routine in order
 *   to determine the cause of the failure.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsu_testRDM
   (
   SDSU_ID       context,
   const uint32  destId,
   uint32 *      pMemSpace,
   uint32 *      pAddress,
   const BOOL    verbose
   )
{
   uint32         address;
   uint32         data;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   switch (destId)
   {

      /*
       * Test the ability to execute an RDM command at the specified address 
       * in each of the DSP's memory spaces. Each DSP is handled in exactly 
       * the same way.
       */

      case SDSU_IDENT_VME:
      case SDSU_IDENT_UTL:
      case SDSU_IDENT_TIM:

         if (verbose) printf ("Testing RDM for DSP %d at start of X space\n", 
                              destId);
         address = SDSU_MEM_START_X;
         * pAddress = address;
         * pMemSpace = SDSU_MEM_SPACE_X;

         if (sdsuPrimitive (context, "RDM", destId, & address, & data) == ERROR)
         {
            ERROR_SET2 (0, "RDM failed at start of X space (%#x) for DSP %d", 
                        ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }

         if (verbose) printf ("Testing RDM for DSP %d at end of X space\n", 
                              destId);
         address = SDSU_MEM_END_X;
         * pAddress = address;
         if (sdsuPrimitive (context, "RDM", destId, & address, & data) == ERROR)
         {
            ERROR_SET2 (0, "RDM failed at end of X space (%#x) for DSP %d", 
                        ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }

         if (verbose) printf ("Testing RDM for DSP %d at start of Y space\n", 
                              destId);
         address = SDSU_MEM_START_Y;
         * pAddress = address;
         * pMemSpace = SDSU_MEM_SPACE_Y;
         if (sdsuPrimitive (context, "RDM", destId, & address, & data) == ERROR)
         {
            ERROR_SET2 (0, "RDM failed at start of Y space (%#x) for DSP %d", 
                        ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }

         if (verbose) printf ("Testing RDM for DSP %d at end of Y space\n", 
                              destId);
         address = SDSU_MEM_END_Y;
         * pAddress = address;
         if (sdsuPrimitive (context, "RDM", destId, & address, & data) == ERROR)
         {
            ERROR_SET2 (0, "RDM failed at end of Y space (%#x) for DSP %d", 
                        ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }

         if (verbose) printf ("Testing RDM for DSP %d at start of P space\n", 
                              destId);
         address = SDSU_MEM_START_P;
         * pAddress = address;
         * pMemSpace = SDSU_MEM_SPACE_P;
         if (sdsuPrimitive (context, "RDM", destId, & address, & data) == ERROR)
         {
            ERROR_SET2 (0, "RDM failed at start of P space (%#x) for DSP %d", 
                        ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }

         if (verbose) printf ("Testing RDM for DSP %d at end of P space\n", 
                              destId);
         address = SDSU_MEM_END_P;
         * pAddress = address;
         if (sdsuPrimitive (context, "RDM", destId, & address, & data) == ERROR)
         {
            ERROR_SET2 (0, "RDM failed at end of P space (%#x) for DSP %d", 
                        ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }

         if (verbose) printf ("Testing RDM for DSP %d at start of E space\n", 
                              destId);
         address = SDSU_MEM_START_E;
         * pAddress = address;
         * pMemSpace = SDSU_MEM_SPACE_E;
         if (sdsuPrimitive (context, "RDM", destId, & address, & data) == ERROR)
         {
            ERROR_SET2 (0, "RDM failed at start of E space (%#x) for DSP %d", 
                        ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }

         if (verbose) printf ("Testing RDM for DSP %d at end of E space\n", 
                              destId);
         address = SDSU_MEM_END_E;
         * pAddress = address;
         if (sdsuPrimitive (context, "RDM", destId, & address, & data) == ERROR)
         {
            ERROR_SET2 (0, "RDM failed at end of E space (%#x) for DSP %d", 
                        ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }
         break;

      default:
         ERROR_SET (S_sdsuLib_INV_PROC_ID, "Invalid DSP target", 
                    ERROR_LOG_SAVE);
         return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_testWRM
 *
 *   INVOCATION:
 *   sdsu_testWRM (context, destId, pMemSpace, pAddress, verbose)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context    (SDSU_ID)      context ID
 *   (>) destId     (const uint32) ID of DSP to test
 *   (!) pMemSpace  (uint32 *)     points to memory-space identifier
 *   (!) pAddress   (uint32 *)     points to address to test
 *   (>) verbose    (const BOOL)   enable verbose printout of test status
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if any of the RDM executions failed.
 *
 *   PURPOSE:
 *   Test ability to execute WRM command on SDSU DSP
 *
 *   DESCRIPTION:
 *   This routine attempts to execute a number of WRM command on an SDSU
 *   controller in order to verify correct operation of the hardare and
 *   firmware. For the specified DSP, the routine will check the first and
 *   last address which define the range of valid addresses for each of the
 *   DSP's Y memory space. (P and X space are untouched to avoid trashing
 *   any DSP code). The routine will read the value contained
 *   in a particular address, complement it and write the modified value back.
 *   It will then attempt to read back the modified value it has written and
 *   check that the same value is obtained. Finally it will restore the old 
 *   value. The pointers pMemSpace and pAddress are written with the 
 *   memory-space and address being tested at any instant so that, in the event 
 *   of an error, they can be examined by the calling routine in order to 
 *   determine the cause of the failure.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   The read back test (where this function checks that when a value is written
 *   to DSP memory and read back that the same value is returned) does not work
 *   and has been commented out pending further investigation. The test was 
 *   failing even with correctly working SDSU hardware. SMB - 14 October 1998.
 *-
 */

STATUS   sdsu_testWRM
   (
   SDSU_ID       context,
   const uint32  destId,
   uint32 *      pMemSpace,
   uint32 *      pAddress,
   const BOOL    verbose
   )
{
   uint32         address;
   uint32         oldData;
   uint32         newData;
   uint32         readBackData;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   switch (destId)
   {

      /*
       * Test the ability to execute an WRM command at the specified address 
       * in the DSP's Y memory space. Each DSP is handled in exactly the same 
       * way.
       */

      case SDSU_IDENT_VME:
      case SDSU_IDENT_UTL:
      case SDSU_IDENT_TIM:

         if (verbose) printf ("Testing WRM for DSP %d at start of Y space\n", 
                              destId);
         address = SDSU_MEM_START_Y;
         * pAddress = address;
         * pMemSpace = SDSU_MEM_SPACE_Y;

         if (sdsuPrimitive (context, "RDM", destId, & address, & oldData) 
             == ERROR)
         {
            ERROR_SET2 (0, "RDM 1 failed at start of Y space (%#x) for DSP %d",
                        ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }
         newData = ~oldData;
         if (sdsuPrimitive (context, "WRM", destId, & address, & newData) 
             == ERROR)
         {
            ERROR_SET2 (0, "WRM 1 failed at start of Y space (%#x) for DSP %d", 
                       ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }
         if (sdsuPrimitive (context, "RDM", destId, & address, & readBackData) 
             == ERROR)
         {
            ERROR_SET2 (0, "RDM 2 failed at start of Y space (%#x) for DSP %d", 
                        ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }
/* Read back test commented out - see "DEFICIENCIES" section above */
/*
         if ( (!context->simulate) && (readBackData != newData) )
         {
            ERROR_SET2 (S_sdsuLib_TEST_FAIL, "Wrote %lu, Read back %lu", 
                        ERROR_LOG_SAVE, newData, readBackData);
            ERROR_SET2 (0,
            "Read back test failed at start of Y space (%#x) for DSP %d", 
            ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }
*/

         if (sdsuPrimitive (context, "WRM", destId, & address, & oldData) 
             == ERROR)
         {
            ERROR_SET2 (0, "WRM 2 failed at start of Y space (%#x) for DSP %d", 
               ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }

         if (verbose) printf ("Testing WRM for DSP %d at end of Y space\n", 
                              destId);
         address = SDSU_MEM_END_Y;
         * pAddress = address;

         if (sdsuPrimitive (context, "RDM", destId, & address, & oldData) 
             == ERROR)
         {
            ERROR_SET2 (0, "RDM 1 failed at end of Y space (%#x) for DSP %d", 
                 ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }
         newData = ~oldData;
         if (sdsuPrimitive (context, "WRM", destId, & address, & newData) 
             == ERROR)
         {
            ERROR_SET2 (0, "WRM 1 failed at end of Y space (%#x) for DSP %d", 
               ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }
         if (sdsuPrimitive (context, "RDM", destId, & address, & readBackData) 
             == ERROR)
         {
            ERROR_SET2 (0, "RDM 2 failed at end of Y space (%#x) for DSP %d", 
                        ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }
/* Read back test commented out - see "DEFICIENCIES" section above */
/*
         if ( (!context->simulate) && (readBackData != newData) )
         {
            ERROR_SET2 (S_sdsuLib_TEST_FAIL, "Wrote %lu, Read back %lu", 
               ERROR_LOG_SAVE, newData, readBackData);
            ERROR_SET2 (0,
               "read back test failed at end of Y space (%#x) for DSP %d", 
               ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }
*/
         if (sdsuPrimitive (context, "WRM", destId, & address, & oldData) 
             == ERROR)
         {
            ERROR_SET2 (0, "WRM 2 failed at end of Y space (%#x) for DSP %d", 
                    ERROR_LOG_SAVE, (* pAddress | * pMemSpace), destId);
            return (ERROR);
         }
         break;

      default:
         ERROR_SET (S_sdsuLib_INV_PROC_ID, "Invalid DSP target", 
                    ERROR_LOG_SAVE);
         return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_makeCmd
 *
 *   INVOCATION:
 *   sdsu_makeCmd (context, pCmdDef, sourceId, destId, pCmdArg)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) context    (SDSU_ID)         context ID
 *   (>) pCmdDef    (SDSU_CMD_DEF *)  command-definition structure
 *   (>) sourceId   (const uint32)    DSP ID for source of command
 *   (>) destId     (const uint32)    DSP ID for command destination
 *   (>) pCmdArg    (uint32 *)        points to any command arguments
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the command could not be generated.
 *
 *   PURPOSE:
 *   Generate the sequence of words that make up an SDSU command
 *
 *   DESCRIPTION:
 *   This routine generates an command and writes the sequence of bytes that
 *   make up the command into the context structure. The required command is
 *   defined by pCmdDef. sourceId and destId give the source and destination
 *   for the command while pCmdArg points to any arguments associated with the
 *   command.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsu_makeCmd
   (
   SDSU_ID          context,
   SDSU_CMD_DEF     *pCmdDef,
   const uint32     sourceId,
   const uint32     destId,
   uint32 *         pCmdArg
   )
{
   int              argIndex;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Check destination DSP valid   */

   if ((sourceId >= SDSU_IDENT_INVALID) || (destId >= SDSU_IDENT_INVALID))   
   {
      ERROR_SET2 (S_sdsuLib_INV_PROC_ID, 
          "Invalid source (%lu) and target (%lu) IDs", ERROR_LOG_SAVE,
          sourceId, destId);
      return (ERROR);
   }

   (context->pCmdBuffer) [0] = 
   sdsu_makeHeader (pCmdDef, sourceId, destId, TRUE);
                                                 /* Get the cmd-header word   */
   (context->pCmdBuffer) [1] = sdsu_makeCmdWord (pCmdDef);         
                                                /* ... and the cmd mnemonic   */
                                                 /* (i.e. the command word)   */

   if ( (context->pCmdBuffer) [0] == 0 )         /* Check header word ok      */
   {
      ERROR_SET (0, "Failed to construct the header word", ERROR_LOG_SAVE);
      return (ERROR);
   }

   if (pCmdDef->cmdArgCount > MAX_NUM_CMD_ARG)   /* Check # arguments valid   */
   {
      ERROR_SET (S_sdsuLib_INV_CMD_ARG_COUNT, "Too many command arguments", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Copy any arguments      */
   for (argIndex = 0; argIndex < pCmdDef->cmdArgCount; argIndex++)  
      (context->pCmdBuffer) [2 + argIndex] = pCmdArg [argIndex];

   /*
    * Following the command header word, the command-word and any arguments 
    * there may be unused words in the command buffer. Fill these with a 
    * recognisable value to make them readily identifiable for debugging 
    * purposes (e.g. when examining the contents of context->pCmdBuffer[]).
    */

   for (argIndex = pCmdDef->cmdArgCount; argIndex < CMD_BUF_NWORD - 2; 
        argIndex++)
   {
      (context->pCmdBuffer) [2 + argIndex] = SDSU_CMD_WORD_EMPTY;
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_makeCmdWord
 *
 *   INVOCATION:
 *   sdsu_makeCmdWord (pCmdDef)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *
 *   FUNCTION VALUE:
 *   (int)   Command word.
 *
 *   PURPOSE:
 *   Generate an SDSU command word
 *
 *   DESCRIPTION:
 *   This routine generates a command word for an SDSU command. The command
 *   word contains the 3-character command mnemonic converted into a 32-bit
 *   integer (the least-significant 24 bits of which are used by the SDSU
 *   controller).
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

int   sdsu_makeCmdWord
   (
   SDSU_CMD_DEF *   pCmdDef
   )
{
   int      i;
   int      j;
   uint32   cmdWord;

   /* Convert the 3-byte command-mnemonic (string) into a 24-bit word */

   for (i = 0, j = 16, cmdWord = 0; i < 3; i++, j -= 8)
   {
      cmdWord |= pCmdDef->pCmdString [i] << j;
   }
   return (cmdWord);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_makeHeader
 *
 *   INVOCATION:
 *   sdsu_makeHeader (pCmdDef, sourceId, destId, cmdHeader)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pCmdDef    (SDSU_CMD_DEF *) command-definition structure
 *   (>) sourceId   (const uint32)   DSP ID for source of command
 *   (>) destId     (const uint32)   DSP ID for command destination
 *   (>) cmdHeader  (const BOOL)     make command or reply header word
 *
 *   FUNCTION VALUE:
 *   (int)   Command- or reply-header word. 0 if an error occurred.
 *
 *   PURPOSE:
 *   Generate a header word for an SDSU command or reply
 *
 *   DESCRIPTION:
 *   This routine generates a header word for an SDSU command or reply 
 *   (if cmdHeader is TRUE, a command header is generated, otherwise a reply 
 *   header is generated).
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

int   sdsu_makeHeader
   (
   SDSU_CMD_DEF *pCmdDef,
   const uint32 sourceId,
   const uint32 destId,
   const BOOL   cmdHeader
   )
{
   int   nWord = 2;

   if (cmdHeader)                            /* Is this a command header word */
   {
      /* if a command, get the number of arguments*/
      nWord += pCmdDef->cmdArgCount;  
   }
   else                            /* ... or is it a reply header word ?      */
   {
      if (pCmdDef->repType == BIT_FIELD_REPLY_NONE)
      {
         /*
          * A reply for which the reply type is BIT_FIELD_REPLY_NONE cannot, 
          * by definition, have a reply-header word. This is a serious error 
          * condition.
          */

         ERROR_SET (S_sdsuLib_INTERNAL_ERROR, 
                   "BIT_FIELD_REPLY_NONE cannot have reply-header word",
                   ERROR_LOG_SAVE);
         return (0);
      }
      else
      {          
         /* if a reply, get the number of arguments   */
         nWord += pCmdDef->repArgCount;
      }
   }

   /* Assemble and return the reply header */
   return (sourceId << 16 | destId << 8 | nWord);   
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_isReplyValid
 *
 *   INVOCATION:
 *   sdsu_isReplyValid (pField)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) reply   (uint32)         reply word to test validity of
 *   (>) pCmdDef (SDSU_CMD_DEF *) definition of command associated with reply
 *
 *   FUNCTION VALUE:
 *   (BOOL)   TRUE if reply is valid, FALSE if it is not.
 *
 *   PURPOSE:
 *   Test the validity of a reply to an SDSU command
 *
 *   DESCRIPTION:
 *   This routine tests whether the reply read from an SDSU controller in 
 *   response to a previously-issued command is valid. In order to be valid, 
 *   the reply must be one of the values defined in the definition of the SDSU 
 *   command.
 *
 *   EXTERNAL VARIABLES:
 *   (>)   sdsuSymTab      (SYMTAB_ID)      Symbol table ID.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

BOOL   sdsu_isReplyValid
   (
   uint32       reply,
   SDSU_CMD_DEF *pCmdDef
   )
{
   char         pName [4];
   SDSU_REP_DEF *pRepDef;
   SYM_TYPE     type;

   SDSU_UINT_TO_STRING (reply, pName);

   /* Look-up the reply definition in the symbol table */

   if (symFindByNameAndType (sdsuSymTab, pName, (char **) & pRepDef, & type,
       SYMBOL_TYPE_REP, SYMBOL_TYPE_MASK) == ERROR) return (FALSE);

   /*
    * Check that the reply is one of those identified in the bit-field 
    * (pRepDef->repBitMask) which defines valid replies to this command. Note 
    * that this routine is only called if a reply is expected for the command, 
    * so that pRepDef->repBitMask should always be non-zero in this routine.
    */

   if ((pCmdDef->repType & pRepDef->repBitMask) == 0) return (FALSE);

   return (TRUE);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_getRecordType
 *
 *   INVOCATION:
 *   sdsu_getRecordType (pField)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pField   (char *)   pointer to string holding OMF field
 *
 *   FUNCTION VALUE:
 *   (int)   OMF record type identifier.
 *
 *   PURPOSE:
 *   Convert string to a OMF record-type identifier
 *
 *   DESCRIPTION:
 *   This routine converts the string pField[] to one of the OMF record-type
 *   identifiers, OMF_FIELD_[START | END | DATA | BLOCKDATA | SYMBOL | COMMENT].
 *   If the string is not recognised as a valid record-type identifier, then the
 *   return value is OMF_FIELD_IDENT_INVALID.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

int   sdsu_getRecordType
   (
   char *   pField
   )
{
   if (sdsu_areStringsSame (pField, OMF_FIELD_START))      
      return (OMF_FIELD_IDENT_START);
   else if (sdsu_areStringsSame (pField, OMF_FIELD_END))      
      return (OMF_FIELD_IDENT_END);
   else if (sdsu_areStringsSame (pField, OMF_FIELD_DATA))      
      return (OMF_FIELD_IDENT_DATA);
   else if (sdsu_areStringsSame (pField, OMF_FIELD_BLOCKDATA))   
      return (OMF_FIELD_IDENT_BLOCKDATA);
   else if (sdsu_areStringsSame (pField, OMF_FIELD_SYMBOL))   
      return (OMF_FIELD_IDENT_SYMBOL);
   else if (sdsu_areStringsSame (pField, OMF_FIELD_COMMENT))   
      return (OMF_FIELD_IDENT_COMMENT);
   else
   {
      ERROR_SET1 (S_sdsuLib_INV_OMF_REC_TYPE, "Invalid OMF record-type, %s", 
                  ERROR_LOG_SAVE, pField);
      return (OMF_FIELD_IDENT_INVALID);
   }
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_isRecordType
 *
 *   INVOCATION:
 *   sdsu_isRecordType (pField)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pField   (char *)   pointer to string holding OMF field
 *
 *   FUNCTION VALUE:
 *   (BOOL)   TRUE if the string is a OMF record identifier.
 *
 *   PURPOSE:
 *   Test whether a string holds a OMF record identifier
 *
 *   DESCRIPTION:
 *   This routine tests whether the string pField[] is a OMF record
 *   identifier. Identifiers are character strings prefixed by the
 *   underscore character, "_". This routine simply tests for the presence
 *   of this character in the first byte of the string.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

BOOL   sdsu_isRecordType
   (
   char *   pField
   )
{
   /* Record-type strings are prefixed by "_"   */

   if (pField [0] == '_')
      return (TRUE);
   else
      return (FALSE);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_areStringsSame
 *
 *   INVOCATION:
 *   sdsu_areStringsSame (pString1, pString2)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pString1   (const char *)      pointer to first string
 *   (>) pString2   (const char *)      pointer to second string
 *
 *   FUNCTION VALUE:
 *   (BOOL)   TRUE if the strings are the same, FALSE if they are not.
 *
 *   PURPOSE:
 *   Perform case-insensitive string comparison
 *
 *   DESCRIPTION:
 *   This routine performs a case-insensitive comparison of the two strings
 *   pString1 and pString2.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *
 *   DEFICIENCIES:
 *   The case-insensitive comparison only works for strings consisting
 *   entirely of alphabetic characters. Strings containing other
 *   characters will not be matched unless they are identical. Examples
 *
 *   "ABCDEF"   =>   "ABCDEF"   =>   matched
 *   "ABCDEF"   =>   "ABCdef"   =>   matched
 *   "ABC42"    =>   "ABC42"    =>   matched
 *   "ABC42"    =>   "abc42"    =>   NOT matched
 *
 *   It is assumed no string longer than MAX_CHAR_STRING will be tested
 *   with this function.
 *-
 */

BOOL   sdsu_areStringsSame
   (
   const char *   pString1,
   const char *   pString2
   )
{
   int      i;
   char   pCopy1 [MAX_CHAR_STRING + 1];
   char   pCopy2 [MAX_CHAR_STRING + 1];

   /*
    * If the strings are already the same, then return TRUE.
    * Otherwise take a copy of both strings, convert them both to the same case
    * and try the test again.
    */

   if (strcmp (pString1, pString2) == 0)
   {
      return (TRUE);
   }
   else
   {
      strncpy (pCopy1, pString1, MAX_CHAR_STRING);
      strncpy (pCopy2, pString2, MAX_CHAR_STRING);

      for (i = 0; i < strlen (pCopy1); i++) pCopy1 [i] &= ~0x20;
      for (i = 0; i < strlen (pCopy2); i++) pCopy2 [i] &= ~0x20;

      if (strcmp (pCopy1, pCopy2) == 0)
         return (TRUE);
      else
         return (FALSE);
   }
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_atoiBase16
 *
 *   INVOCATION:
 *   sdsu_atoiBase16 (pString)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pString   (char *)   pointer to string holding hex number
 *
 *   FUNCTION VALUE:
 *   (uint32)   numeric value of hex character string.
 *
 *   PURPOSE:
 *   Convert a hex number from string to numeric format
 *
 *   DESCRIPTION:
 *   This routine interprets pString as a hex number and returns the numeric
 *   value.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32   sdsu_atoiBase16
   (
   char *   pString
   )
{
   int    i;
   uint32 hexResult = 0;
   char   nibble;

   for (i = 0; i < strlen (pString); i++)
   {
      hexResult = hexResult << 4;
      nibble = pString [i];

      if ((nibble >= 0x30) && (nibble <= 0x39))   hexResult |= nibble - 0x30;
      else if ((nibble >= 0x41) && (nibble <= 0x46))   
              hexResult |= nibble - 0x37;
      else if ((nibble >= 0x61) && (nibble <= 0x66))   
              hexResult |= nibble - 0x57;
   }
   return (hexResult);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_getVersion
 *
 *   INVOCATION:
 *   sdsu_getVersion (pString)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pString   (char *)   pointer to string holding version code
 *
 *   FUNCTION VALUE:
 *   (uint32)   version number (0xffffffff if a version number could not be 
 *              extracted)
 *
 *   PURPOSE:
 *   Convert a CVS version string into numeric version number
 *
 *   DESCRIPTION:
 *   This routine interprets pString as a CVS version number aa[.bb[.cc[.dd]]] 
 *   and returns a 4-byte integer numeric equivalent, with each set of digits 
 *   in their own byte starting at the MSB. It scans pString for the first 
 *   digit character, then converts the first series of digits into the top 
 *   byte. Any digits after a '.' are converted into the next byte and so on.
 *   If no digits are found 0xffffffff is returned.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *
 *   DEFICIENCIES:
 *   Version segments greater than 255 cannot be represented.
 *   Version number 255.255.255.255 looks like a conversion error.
 *-
 */

uint32   sdsu_getVersion
   (
   char *   pString
   )
{
   int      shift = 24;      /* Bit shift */
   uint32   byte;            /* Version segment */
   char     *pNext;          /* Pointer to next character */
   uint32   version = 0;     /* Return value */
   
   /* Scan through for the first digit */
   while ((*pString != '\0') && !isdigit(*pString))
      pString++;

   if (*pString == '\0') return (0xffffffff);   /* No digits */
   
   while (shift >= 0)
   {
      byte = strtoul(pString, &pNext, 10);
      if (byte > 255) return (0xffffffff);   /* Conversion error */
   
      version |= (byte << shift);
      shift -= 8;
   
      if (*pNext != '.') return (version);
      pString = pNext + 1;
   }
   return (version);
}

/* -------------------------------------------------------------------------- */
/*+
 *   FUNCTION NAME:
 *   sdsu_checkRepBuf
 *
 *   INVOCATION:
 *   sdsu_checkRepBuf (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) context (SDSU_ID) Context ID
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if unread words were detected in the reply buffer.
 *
 *   PURPOSE:
 *   Check the Reply buffer for unread words - caused perhaps by a system reset
 *   or timeout from a previous command. If detected, flag an error and read 
 *   all unread words from the buffer
 *
 *   DESCRIPTION:
 *   This routine reads the reply buffer at the current read address. If an 
 *   unread word is detected: an error is logged, the buffer printed, then 
 *   cleared, then printed again. Any system reset detected will cause an 
 *   additional error to be logged.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   timeoutLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *-
 */

STATUS sdsu_checkRepBuf 
   (
   SDSU_ID context
   )
{

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

#ifdef  DEBUG
   if (sdsuFullDebug)
      printf ("sdsu_checkRepBuf: Examining reply buffer for unread data\n");
#endif /* DEBUG */

   if (cacheInvalidate (DATA_CACHE, context->pRepBuffer, 
                        REP_BUF_NWORD * sizeof (uint32)) == ERROR)
   {
      ERROR_SET (0, "Cache invalidate for reply buffer failed", ERROR_LOG_SAVE);
      return (ERROR);
   }

   if ((context->pRepBuffer [context->repBufCounter] & 0xff000000) != 0)
   {
      /* There is an unread word at the current read point */
      ERROR_SET (S_sdsuLib_REPLY_BUFFER_ERROR, 
                 "Unexpected data found in Reply buffer", ERROR_LOG_SAVE);

      sdsuPrintRepBuf (context); /*Display the buffer*/

      printf("\nCleaning Reply Buffer...\n");

      while ((context->pRepBuffer [context->repBufCounter] & 0xff000000) != 0)
      {
            if (cacheInvalidate (DATA_CACHE, context->pRepBuffer, 
                                 REP_BUF_NWORD * sizeof (uint32)) == ERROR)
            {
               ERROR_SET (0, "Cache invalidate for reply buffer failed", 
                          ERROR_LOG_SAVE);
               return (ERROR);
            }

            /* Check if unread words contain a system reset message-
             * this will not work if we are unlucky enough
             * to have unread DATA which looks like an 'SYR'
             */

            if (((context->pRepBuffer) [context->repBufCounter] & 0xffffff)
                                        == SDSU_STRING_TO_UINT ("SYR"))
            {
               printf("\nUnexpected System Reset detected ! - SDSU controller will require initialising\n\n");
               ERROR_SET (S_sdsuLib_REPLY_BUFFER_ERROR, 
                          "Unexpected System reset detected",
                          ERROR_LOG_SAVE);
            }

            /* move on to check the next word */
            sdsu_incRepBufCounter (context, 1);
      }

      printf("\n...done\n\n");
      sdsuPrintRepBuf (context);
      return(ERROR);
   }

   /* If the buffer was fine then...*/
   return(OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   sdsu_incRepBufCounter
 *
 *   INVOCATION:
 *   sdsu_incRepBufCounter (context, nIncrement)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) context     (SDSU_ID) SDSU context ID
 *   (>) nIncrement  (int)     number of words to increment reply-buffer counter
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if not context structure is invalid.
 *
 *   PURPOSE:
 *   Increment the pointer in the local SDSU reply-buffer
 *
 *   DESCRIPTION:
 *   This routine increments the pointer in the local SDSU reply buffer. This 
 *   buffer gets written by the SDSU VME DSP, and the DSP maintains its own 
 *   copy of the buffer pointer and increments this pointer as it issues 
 *   replies to the VME CPU. The local VME CPU (the thing running sdsuLib under 
 *   VxWorks) is responsible for keeping the reply-buffer pointer in 
 *   synchronisation with that maintained by the VME DSP. Parameter nIncrement 
 *   gives the number of words by which the pointer should be incremented. The 
 *   pointer wraps around to the start of the buffer every REP_BUF_NWORD words.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   It is possible for the DSP and host to lose synchronisation and end up 
 *   with different reply buffer pointers. Care should be taken to call this 
 *   function to step over any new data in the reply buffer.
 *-
 */

STATUS   sdsu_incRepBufCounter
   (
   SDSU_ID   context,
   int      nIncrement
   )
{
   int      i;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Increment the reply buffer, modulo the buffer size.
    */

   for (i = 0; i < nIncrement; i++)
   {
      context->pRepBuffer [context->repBufCounter] &= 0x00ffffff;
      if (cacheFlush (DATA_CACHE, 
                      & (context->pRepBuffer [context->repBufCounter]), 4) 
          == ERROR)
      {
         ERROR_SET (0, "Cache flush for reply buffer failed", ERROR_LOG_SAVE);      
         return (ERROR);
      }
      context->repBufCounter = (context->repBufCounter + 1) % REP_BUF_NWORD;
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   sdsuSetReplyAddress
 *
 *   INVOCATION:
 *   sdsuSetReplyAddress (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) context (SDSU_ID) SDSU context ID
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if the reply address was set successfully, or ERROR if not.
 *
 *   PURPOSE:
 *   Set the reply address used by the SDSU VME DSP
 *
 *   DESCRIPTION:
 *   This routine extracts the address of a previously-allocated SDSU reply 
 *   buffer from the context structure, convert it to a VME bus address (i.e. 
 *   to the addres to which the VME DSP should write its replies), and then 
 *   issues an SRA command to the VME DSP in order to set this reply address.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   sdsuSetReplyAddress
   (
   SDSU_ID   context
   )
{
   uint32     pCmdArg [2];
   uint32     *pHostAddress;
   uint32     oldRepBufCounter;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Map local reply buffer address to VME bus, ready to be
    * passed to the VME board to set the host's base address
    */

   if (sysLocalToBusAdrs (SDSU_AM_VME_MASTER_REP, (char *) context->pRepBuffer,
                          (char **) & pHostAddress) == ERROR)
   {
      ERROR_SET (0, "Failed to map reply address to VME bus", ERROR_LOG_SAVE);
      return (NULL);
   }

   pCmdArg [0] = 
   ((uint32) pHostAddress >> 16) & 0xffff;      /* Get top 16 bits of address */
   pCmdArg [1] = 
   (uint32) pHostAddress & 0xffff;           /* and bottom 16 bits of address */

#ifdef DEBUG
   printf ("sdsuSetReplyAddress: Setting reply address top bits to %#lx and bottom bits to %#lx.\n",
      pCmdArg[0], pCmdArg[1]);
#endif   /* DEBUG */

   oldRepBufCounter = context->repBufCounter;
   context->repBufCounter = 0;
   if (sdsuPrimitive (context, "SRA", SDSU_IDENT_VME, pCmdArg, NULL) == ERROR)
   {
      /* Restore counter if SRA failed      */
      context->repBufCounter = oldRepBufCounter; 
      ERROR_SET (0, "Set reply address (SRA) command failed", ERROR_LOG_SAVE);
      return (ERROR);
   }

   return (OK);
}


/* -------------------------------------------------------------------------- */

/*-
 *   IGNORED FUNCTION NAME:
 *   sdsu_isAddressValid
 *
 *   INVOCATION:
 *   sdsu_isAddressValid (address, destId)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   address     (uint32)         address in DSP memory
 *   (>)   destId      (const uint32)   DSP ID
 *
 *   FUNCTION VALUE:
 *   (BOOL)   TRUE if the address is valid, FALSE if it is not.
 *
 *   PURPOSE:
 *   Test the validity of a DSP 56k address for a particular DSP
 *
 *   DESCRIPTION:
 *   This routine tests whether a DSP address lies within the defined range of
 *   valid addresses for that DSP.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *+
 */

BOOL   sdsu_isAddressValid ( uint32         address,
                             const uint32   destId )
{
   BOOL         valid;

   switch ((int) destId)
   {

      /*
       * Check that the address is within the defined range for one of the 
       * memory spaces 
       */

      case SDSU_IDENT_VME:
      case SDSU_IDENT_UTL:
      case SDSU_IDENT_TIM:
         if (((address >= SDSU_MEM_START_X) && (address <= SDSU_MEM_END_X)) ||
            ((address >= SDSU_MEM_START_Y) && (address <= SDSU_MEM_END_Y)) ||
            ((address >= SDSU_MEM_START_P) && (address <= SDSU_MEM_END_P)) ||
            ((address >= SDSU_MEM_START_E) && (address <= SDSU_MEM_END_E)))
         {
            valid = TRUE;
         }
         else
         {
            ERROR_SET1 (S_sdsuLib_INV_DSP_ADDRESS, "Invalid DSP address, %#x", 
                        ERROR_LOG_SAVE,
               address);
            valid = FALSE;
         }
         break;

      case SDSU_IDENT_HST:
         ERROR_SET (S_sdsuLib_INV_DSP_ADDRESS, "Host specified as DSP", 
                    ERROR_LOG_SAVE);
         valid = FALSE;
         break;

      default:
         ERROR_SET1 (S_sdsuLib_INV_DSP_ADDRESS, "Unknown DSP ID, %lu", 
                     ERROR_LOG_SAVE, destId);
         valid = FALSE;
   }
   return (valid);
}

/* -------------------------------------------------------------------------- */

/*-
 *   FUNCTION NAME:
 *   sdsu_initRepBuf
 *
 *   INVOCATION:
 *   sdsu_initRepBuf (context)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (!) context (SDSU_ID) SDSU context ID
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if the reset to zero of the reply buffer set successfully,
 *              or ERROR if not.
 *
 *   PURPOSE:
 *   Reset to zero the reply buffer
 *
 *   DESCRIPTION:
 *   Initialise reply buffer by filling it with zeros then flush the cache
 *   to ensure these values are up to date.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   sdsuLib.h
 *   errorLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *+
 */

STATUS sdsu_initRepBuf
   (
   SDSU_ID context
   )
{
   uint32 i=0 ;

   if (SDSU_ID_IS_INVALID (context))
   {
      ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Invalid SDSU context",
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Initialise reply buffer by filling it with zeros 
    * then flush the cache to ensure these values are up to date.
    */

   for (i = 0; i < REP_BUF_NWORD; i++)
       context->pRepBuffer [i] = 0;

   if (cacheFlush (DATA_CACHE, context->pRepBuffer,
                   REP_BUF_NWORD * sizeof (uint32)) == ERROR)
   {
      ERROR_SET (0, "Cache flush for reply buffer failed", ERROR_LOG_SAVE);
      return (ERROR);
   }

   return ( OK ) ;
}

