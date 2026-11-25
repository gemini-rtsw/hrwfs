/*+
 * MODULE NAME:
 * sdsuLib
 *
 * FILENAME:
 * sdsuLib.h
 *
 * PURPOSE:
 * Include file for sdsuLib
 *
 *INDENT-OFF*
 *
 *INDENT-ON*
 *-
 */

#ifndef __INCsdsuLibh
#define __INCsdsuLibh


/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif /* vxWorks */

#include <vme.h>
#include <stdlib.h>
#include <stdio.h>
#include <wdLib.h>
#include <string.h>
#include <timers.h>
#include <semLib.h>
#include <symLib.h>
#include <msgQLib.h>
#include "gemTypes.h"
#include "gemModNum.h"


/* defines */

 /* General parameters */


#define SDSU_DSP_CLK_MHZ_VME 32.0  /* DSP clock rate (MHz), same for all DSPs */
#define SDSU_REP_BUF_ALIGN   0x100 /* 256 byte alignment for reply buffer     */
#define SDSU_NEW_FBA_FLAG    0x800000 
                                   /* Bit 23 of high word used to indicate    */
                                   /* new Frame Buffer Address (FBA)          */
#define SDSU_NBYTE_PER_PIXEL 2     /* Number of bytes transferred from the    */
                                   /* VME DSP per digitised pixel             */

#define SDSU_FULL_READOUT 1.8      /* Readout Time in sec of the full CCD     */

#define SDSU_LOW_DNLOAD_ADRS_IN_P 0x20  
                                   /* Lowest address (in "P" space) to which  */
                                   /* OMF file downloading is performed on    */
                                   /* the 1st pass through the file.          */
#define SDSU_FRAME_FIND_TIMEOUT  (20 * sysClkRateGet())
                                   /* Timeout for sdsuFrameFind() routine in  */
                                   /* clock ticks (20 seconds)                */
#define SDSU_VME_BOARD_RESET_ADDRS 4   
                                   /* Offset (in bytes) from the base address */
                                   /* the SDSU VME board to the reset register*/
                                   /* board to reset (when written to)        */
#define SDSU_VME_BOARD_RESET_TIME  1.0
                                   /* How long (seconds) to wait after reset  */
#define SDSU_RESET_VME        0x1  /* Reset VME interface & DSP bit           */
#define SDSU_RESET_CONTROLLER 0x2  /* Reset controller DSPs bit               */

/* Timing board status parameter bits */

#define SDSU_TIM_STATUS_IDLING   0x1   /* Idling bit in TIM status word.      */
#define SDSU_TIM_STATUS_EXPOSING 0x2   /* Exposing bit in TIM status word.    */
#define SDSU_TIM_STATUS_READING  0x4   /* Reading out bit in TIM status word. */
#define SDSU_TIM_STATUS_SYNC     0x8   /* Readout sync request bit in TIM     */
                                       /* status.                             */

#define SDSU_TIM_ERROR_OVERRUN   0x1   /* Data overun (exposure time<readout) */
#define SDSU_TIM_ERROR_WRP       0x2   /* WRP error - invalid parameter addr. */

#define SDSU_TIM_PARMID_SYNC  0x8000   /* Sync mode enabled bit in PARMID.    */

#define SDSU_TIM_MODE_UNDERSCAN  0x1   /* Output underscan pixels bit in TIM  */
                                       /* mode.                               */
#define SDSU_TIM_MODE_SIMULATE   0x2   /* Simulate data bit in TIM mode.      */
#define SDSU_TIM_MODE_SYNC       0x4   /* Enable sync mode bit in TIM mode.   */

 /*
  * The following parameters are conversion factors for SDSU parameters.
  * If they depend on the actual DSP code downloaded they may eventually be 
  * better off being contained in a separate header file connected with the 
  * DSP code.
  */

#define SDSU_EXPOSURE_UNIT ((double) 81.92e-6) 
                                   /* Increments of exposure time in seconds. */
#define SDSU_TEMP_BASE ((double) 0.0) 
                                   /* Temperature represented by zero ADU.    */
#define SDSU_TEMP_UNIT ((double) 0.0156) 
                                   /* Increments of temperature in Kelvin.    */

 /*
  * Some magic numbers: these go into various data structures and are used as
  * sanity checks
  */

#define SDSU_ID_MAGIC       0xCCD00CCD                   /* Context structure */
#define SDSU_DATA_EMPTY     0xDA2A    
                                  /* Used to fill unused words in data buffer */
#define SDSU_CMD_WORD_EMPTY 0x000DEAD0  
                                  /* Used to fill unused words in cmd buffer  */

 /* Memory spaces identifiers */

#define SDSU_MEM_SPACE_NONE 0x000000   /* Used for symbols which are not addr */
#define SDSU_MEM_SPACE_P    0x100000   /* Upper nibble of "P" space address   */
#define SDSU_MEM_SPACE_X    0x200000   /* Upper nibble of "X" space address   */
#define SDSU_MEM_SPACE_Y    0x400000   /* Upper nibble of "Y" space address   */
#define SDSU_MEM_SPACE_E    0x800000   /* Upper nibble of "E" space address   */
#define SDSU_MEM_SPACE_MASK 0xf00000   /* Bit-mask for address-space ident    */

 /* Memory map (which is the same for each DSP) */

#define SDSU_MEM_START_P (SDSU_MEM_SPACE_P | 0x000000)
#define SDSU_MEM_START_X (SDSU_MEM_SPACE_X | 0x000000)
#define SDSU_MEM_START_Y (SDSU_MEM_SPACE_Y | 0x000000)
#define SDSU_MEM_START_E (SDSU_MEM_SPACE_E | 0x002000)
#define SDSU_MEM_END_P  (SDSU_MEM_SPACE_P | 0x001fff)
#define SDSU_MEM_END_X  (SDSU_MEM_SPACE_X | 0x001fff)
#define SDSU_MEM_END_Y  (SDSU_MEM_SPACE_Y | 0x003fff)
#define SDSU_MEM_END_E  (SDSU_MEM_SPACE_E | 0x00ffff)

 /* Parameter table definitions */

#define SDSU_SYM_NAME_LEN  21    /* Length of a parameter name     */

 /* Address modes for VME interface accesses */

#define SDSU_AM_VME_MASTER_DATA VME_AM_EXT_USR_DATA 
                                            /* VME i/f = master, data write   */
#define SDSU_AM_VME_MASTER_REP VME_AM_EXT_USR_DATA 
                                            /* VME i/f = master, reply write  */
#define SDSU_AM_VME_SLAVE_CMD VME_AM_EXT_USR_DATA 
                                            /* VME i/f = slave, command write */

 /* Interrupt details */

#define SDSU_VME_INT_LEVEL     6        /* Frame sync VME-bus interrupt level */
#define SDSU_INT_NUMBER_BASE   0xf0     /* Starting interrupt #               */
#define SDSU_INT_NUMBER_LIMIT  0xff     /* Highest interrupt # usable         */
#define SDSU_PACKET_INT_ENABLE 0x1      /* Enable packet interrupts bit       */
#define SDSU_FRAME_INT_ENABLE  0x2      /* Enable frame interrupts bit        */

 /* Readout task constants */

#define SDSU_READTASK_PRIORITY    10     /* Readout task priority-fairly high */
#define SDSU_READTASK_STACKSIZE   200000 /* Read task stack size - very large */
#define SDSU_APPLICATION_PRIORITY 190    /* Application task priority - low   */

 /* Frame status bits */

#define SDSU_FSTAT_OVERRUN   0x00000001 /* Overrun error                      */
#define SDSU_FSTAT_CHECKSUM  0x00000002 /* Checksum error                     */
#define SDSU_FSTAT_FRAMESYNC 0x00000004 /* Frame sync error                   */
#define SDSU_FSTAT_COMPLETE  0x80000000 /* Set by sdsuLib on frame sync - OK  */
#define SDSU_FSTAT_ABORTED   0x40000000 /* Set by sdsuLib when frame aborted  */
#define SDSU_FSTAT_TIMEOUT   0x20000000 /* Set by sdsuLib when frame times out*/
#define SDSU_FSTAT_NOK       0x10000000 /* Set by sdsuLib when frame overrun  */

 /*
  * Bit-mask for SDSU system tests. One bit is assigned to each test, the OR of
  * these bits defines the scope of testing performed in the sdsuTest routine.
  */

#define SDSU_TEST_LINK  0x00000001                       /* Fibre link test   */
#define SDSU_TEST_RDM   0x00000002                       /* Memory read test  */
#define SDSU_TEST_WRM   0x00000004                       /* Memory write test */

 /*
  * Error number codes used by sdsuLib.
  * These are designed to be processed using the vxWorks "makeStatTbl" utility.
  */

#define S_sdsuLib_INV_STRUCTURE       (M_sdsuLib | 1)  /* Invalid structure   */
#define S_sdsuLib_REPLY_TIMEOUT       (M_sdsuLib | 2)  /* Timeout reading     */
                                                       /* SDSU reply          */
#define S_sdsuLib_INV_PROC_ID         (M_sdsuLib | 3)  /* Invalid DSP ident   */
#define S_sdsuLib_INV_DSP_ADDRESS     (M_sdsuLib | 4)  /* Invalid DSP memory  */
                                                       /* address             */
#define S_sdsuLib_TEST_FAIL           (M_sdsuLib | 5)  /* Test failed         */
#define S_sdsuLib_ERROR_RETURNED      (M_sdsuLib | 6)  /* ERR returned as     */
                                                       /* reply to command    */
#define S_sdsuLib_INV_OMF_REC_TYPE    (M_sdsuLib | 7)  /* Invalid OMF record  */
                                                       /* type                */
#define S_sdsuLib_INV_OMF_FIELD       (M_sdsuLib | 8)  /* Invalid OMF field   */
                                                       /* read from file      */
#define S_sdsuLib_INV_MEM_SPACE       (M_sdsuLib | 9)  /* Invalid DSP memory  */
                                                       /*  space ident        */
#define S_sdsuLib_OMF_PARSE_ERROR     (M_sdsuLib | 10) /* Parse error reading */
                                                       /* OMF file            */
#define S_sdsuLib_INTERNAL_ERROR      (M_sdsuLib | 11) /* Internal coding     */
                                                       /* error               */
#define S_sdsuLib_INV_COMMAND         (M_sdsuLib | 12) /* Un-supported SDSU   */
                                                       /* command             */
#define S_sdsuLib_INV_CMD_ARG_COUNT   (M_sdsuLib | 13) /* Invalid # of command*/
                                                       /* arguments           */
#define S_sdsuLib_INV_CARD_ADDRESS    (M_sdsuLib | 14) /* Interface not       */
                                                       /* present at address  */
#define S_sdsuLib_INV_NUM_FRAME       (M_sdsuLib | 16) /* Invalid # frames to */
                                                       /* read                */
#define S_sdsuLib_INV_INT_LEVEL       (M_sdsuLib | 17) /* Invalid interrupt   */
                                                       /* vector #            */
#define S_sdsuLib_INV_PARAM_NAME      (M_sdsuLib | 18) /* Invalid parameter   */
                                                       /* name                */
#define S_sdsuLib_NO_INT_AVAILABLE    (M_sdsuLib | 19) /* No interrupt        */
                                                       /* channels available  */
#define S_sdsuLib_READOUT_ACTIVE      (M_sdsuLib | 20) /* Readout task is     */
                                                       /* already running     */
#define S_sdsuLib_REMOTE_SYSTEM_RESET (M_sdsuLib | 21) /* Received SYR        */
                                                       /* response from TIM   */
#define S_sdsuLib_INV_PARAM_VAL       (M_sdsuLib | 22) /* Invalid parameter   */
                                                       /* value               */
#define S_sdsuLib_SYNC_ERROR          (M_sdsuLib | 23) /* Synchronisation     */
                                                       /* error               */
#define S_sdsuLib_REPLY_BUFFER_ERROR  (M_sdsuLib | 24) /* Unread reply in     */
                                                       /* reply buffer        */


/* macros */

 /* Convert 3-character command string to 32-bit integer */

#define SDSU_STRING_TO_UINT(x)  ((uint32) (((char) ((x) [0])) << 16 | \
          ((char) ((x) [1])) << 8  |    \
          ((char) ((x) [2]))))

 /* Convert 32-bit command word to 3-character string (plus null) */

#define SDSU_UINT_TO_STRING(x,y)           \
{                   \
         (y) [0] = (char) ((x) >> 16) & 0xff; \
         (y) [1] = (char) ((x) >> 8) & 0xff;  \
         (y) [2] = (char) (x) & 0xff;   \
         (y) [3] = 0;       \
}

 /*
  * Check validity of context structure. The macro NO_CONTEXT_CHECK disables
  * this sanity check and will enable very slightly faster performance, e.g.
  * when executing SDSU commands.
  */

#ifdef NO_CONTEXT_CHECK
#define SDSU_ID_IS_INVALID(context) FALSE
#else
#define SDSU_ID_IS_INVALID(context)      \
 (context == NULL || context->magic != SDSU_ID_MAGIC)
#endif /* NO_CONTEXT_CHECK */


/* typedefs */

typedef volatile struct /* Defines the header which preceeds each image frame */
                        /* NOTE: This data structure must exactly match the   */
                        /* header provided by the SDSU controller and         */
                        /* described in ICD 1.6/1.10.                         */
 {
  uint32 packetCount;  /* # packets of this frame now in memory               */
  uint32 status;       /* Frame Status                                        */
  uint32 parameterId;  /* Parameter set ID word; gives readout mode employed  */
  uint32 frameCount;   /* # frames remaining to be read out                   */
 } SDSU_FRAME_HDR;

typedef struct sdsu_frame_s        /* Defines the image frame buffer format   */
 {
  struct sdsu_frame_s *pNext;      /* Linked list                             */
  SEM_ID              frameSem;    /* Counting semaphore holding reservation  */
                                   /* information                             */
  int                 totalFrames; /* Number of frames expected in total (**) */
  /*int                 notUsed;*/     /* Not Used */
  SDSU_FRAME_HDR      header;      /* Control information for the VME DSP     */
  volatile uint16     pixel[1];    /* Image data, actually a variable size    */
                                   /* array                                   */
 } SDSU_FRAME;

typedef struct             /* Context structure used as handle to controller  */
                           /* (**) ITEMS ADDED FOR GEMINI FIRST LIGHT FUDGE.  */
 {
  uint32          magic;          /* Magic number used as sanity check        */
  BOOL            simulate;       /* TRUE when the controller is being        */
                                  /* simulated                                */
  volatile uint32 *pVmeAddress;   /* Base address of VME interface card on bus*/
  uint32          *pRepBuffer;    /* Properly aligned address of reply buffer */
  uint32          repBufCounter;  /* Reply-buffer counter                     */
  uint32          *pCmdBuffer;    /* Local address of command buffer          */
  BOOL            fastCamera;     /* Is context associated with fast camera?  */
                                  /* (**)                                     */
  SEM_ID          commandSem;     /* Ensures command issue/reply is atomic    */
  SYMTAB_ID       paramSyms;      /* Parameter location symbol table          */
  uint32          maxPixelsPerFrame;
                                  /* Max number of pixels in frame            */
  uint32          frameSize;      /* Number of bytes allocated to each frame  */
  int             nFrames;        /* Number of frames available               */
  int             packetsPerFrame;/* Number of packets expected per frame (**)*/
  int             exposureTicks;  /* Exposure time in ticks (**).             */
  BOOL            aborted;        /* Signals when an exposure is aborted (**) */
  BOOL            fatal;          /* Signals when a fatal error is encountered*/
  int             frameErrors;    /* Records number of frame errors (**).     */
  void            *pDataBuffer;   /* Pointer to data buffer                   */
  SDSU_FRAME      *pFreeList;     /* Head of list of free frames              */
  SEM_ID          bufferSem;      /* Counting semaphore for data buffer       */
                                  /* resource                                 */
  short           packetIntNum;   /* Interrupt vector number for packet sync  */
  short           frameIntNum;    /* Interrupt vector number for frame sync   */
  int             frameTimeout;   /* Frame timeout in ticks.                  */
  int             readTask;       /* Task ID of the readout task              */
  int             readStatus;     /* What the readout task is doing right now */
  SDSU_FRAME      *readFrame;     /* Frame which is currently being read      */
  MSG_Q_ID        frameQueue;     /* Queue of frames to process               */
  SEM_ID          packetSem;      /* Packet interrupt semaphore for readout   */
                                  /* task                                     */
  int             sigMask;        /* Old signal mask setting while SIGUSR1    */
                                  /* blocked                                  */
  void            *appPrivate;    /* Application pointer passed to sync       */
                                  /* callbacks                                */
  struct timespec timeoutStart;   /* Start time used for command timeout      */
  int             readMethod;     /* Flag to indicate how to read the CCD     */
  double          readoutTime;    /* Readout Time of the CCD - depends of nb  */
                                  /* of pixels reads                          */
  int             interFrameDelayTicks;
                                  /* Settle delay inserted between two        */
                                  /* consecutive frames of an observation, in */
                                  /* clock ticks (0 = no pause). Used to let   */
                                  /* M1/M2 settle during tuning.              */
 } SDSU_ID_STRUCT, * SDSU_ID;

typedef struct      /* Defines an SDSU primitive command */
 {
  char   pCmdString [4];     /* Command mnemonic string (3 chars + null)      */
  uint32 cmdArgCount;        /* # of arguments to command                     */
  uint32 repArgCount;        /* # of arguments to expected reply              */
  int    timeoutHeaderUsec;  /* Timeout (microsecs) for expected reply header */
  uint32 repType;            /* Types of valid reply expected (bit fields)    */
 } SDSU_CMD_DEF;

typedef struct      /* Defines an SDSU primitive reply */
 {
  char   pRepString [4];  /* Reply mnemonic string (3 chars + null)           */
  uint32 repWord;         /* Mnemonic converted into 32-bit word              */
                          /* INCONSISTENT! A comment in sdsuLib.c claims this */
                          /* is the number of reply arguments. Which is       */
                          /* ccorrect ? SMB - 6 January 1999                  */
  uint32 repBitMask;      /* Bit mask for this reply (only expect 1 bit set)  */
 } SDSU_REP_DEF;


/* enumerated types */

 /* Numeric identifiers for each DSP memory space    */
enum
 {
  SDSU_MEM_INDEX_P = 0,   /* P space     */
  SDSU_MEM_INDEX_X,       /* X space     */
  SDSU_MEM_INDEX_Y,       /* Y space     */
  SDSU_MEM_INDEX_NDEF     /* not defined */
 };

 /* Numeric identifiers for each DSP (and the host CPU) */
enum
 {
  SDSU_IDENT_HST = 0,   /* Host CPU (VxWorks) */
  SDSU_IDENT_VME,       /* VME DSP            */
  SDSU_IDENT_TIM,       /* Timing DSP         */
  SDSU_IDENT_UTL,       /* Utility DSP        */
  SDSU_IDENT_INVALID    /* not specified      */
 };

 /*
  * Readout task status values.
  * These must agree with the readStatus array in sdsuShow()
  */
enum
 {
  SDSU_READ_CLOSED = 0,   /* Closed (when task finished)           */
  SDSU_READ_OPENING,      /* Opening (when task being created)     */
  SDSU_READ_IDLE,         /* Idle (when task waiting for commands) */
  SDSU_READ_BUSY,         /* Busy (when task reading out)          */
  SDSU_READ_ERROR         /* Error (when something has gone wrong) */
 };


/* function declarations */

typedef
 void (*SDSU_CALLBACK)(SDSU_ID context, void *pPrivate, SDSU_FRAME *pFrame);

IMPORT STATUS sdsuLibInit (void);
IMPORT STATUS sdsuProbe (const uint32 vmeAddress);
IMPORT SDSU_ID sdsuContextCreate (const uint32 vmeAddress, const BOOL simulate);
IMPORT STATUS sdsuContextDelete (SDSU_ID context);
IMPORT STATUS sdsuSetReplyAddress (SDSU_ID context);
IMPORT uint32 sdsuVersionGet (SDSU_ID context, const uint32 destId, 
                              const BOOL firmware);
IMPORT STATUS sdsuReset (SDSU_ID context, uint32 what);
IMPORT STATUS sdsuTest (SDSU_ID context, const BOOL verbose, uint32 * pTestMask,
                        const uint32 dspIdentMask);
IMPORT STATUS sdsuShow (SDSU_ID context, const BOOL verbose);
IMPORT STATUS sdsuD (uint32 ptr, char * pMemSpace, int nWord, uint32 destId,
                     SDSU_ID context);
IMPORT STATUS sdsuM (uint32 ptr, char * pMemSpace, uint32 destId, 
                     SDSU_ID context);
IMPORT STATUS sdsuPrintCmdBuf (SDSU_ID context, const BOOL printAll);
IMPORT STATUS sdsuPrintRepBuf (SDSU_ID context);
IMPORT STATUS sdsuClear1RepBuf (SDSU_ID context);
IMPORT STATUS sdsuPrimitiveWrite (SDSU_ID context, SDSU_CMD_DEF * pCmdDef,
       const uint32 sourceId, const uint32 destId, uint32 * pCmdArg);
IMPORT STATUS sdsuPrimitiveRead (SDSU_ID context, SDSU_CMD_DEF * pCmdDef,
       const uint32 sourceId, const uint32 destId, uint32 * pReplyArg);
IMPORT STATUS sdsuPrimitive (SDSU_ID context, char * pCommand, 
       const uint32 destId, uint32 * pCmdArg, uint32 * pRepArg);
IMPORT STATUS sdsuPrimitiveRDM (SDSU_ID context, const uint32 destId, 
       uint32 address, uint32 * data);
IMPORT STATUS sdsuPrimitiveWRM (SDSU_ID context, const uint32 destId, 
       uint32 address, uint32 data);
IMPORT STATUS sdsuPrimitiveMultiRDM (SDSU_ID context, const uint32 destId,
     uint32 address, uint32 * pData, const uint32 wordCount);
IMPORT STATUS sdsuPrimitiveMultiWRM (SDSU_ID context, const uint32 destId,
     uint32 address, uint32 * data, const uint32 wordCount);
IMPORT STATUS sdsuMemoryDnload (SDSU_ID context, FILE * fd, const uint32 destId,
     uint32 * pStart, uint32 * pFinish);
IMPORT STATUS sdsuMemorySymbolDnload (SDSU_ID context, FILE * fd,
                                      const uint32 destId);
IMPORT STATUS sdsuMemoryUpload (SDSU_ID context, FILE * fd, const uint32 destId,
     uint32 * pStart, uint32 * pFinish);
IMPORT STATUS sdsuFileDnload (SDSU_ID context, char * pFileName, 
       const uint32 destId, const BOOL limitAdrsRange);
IMPORT STATUS sdsuFileSymbolDnload (SDSU_ID context, char * pFileName,
       const uint32 destId);
IMPORT STATUS sdsuFileUpload (SDSU_ID context, char * pFileName, 
       const uint32 destId, const BOOL limitAdrsRange);

IMPORT STATUS sdsuBufferCreate (SDSU_ID context, uint32 pixelsPerFrame, 
              int dummy);
IMPORT STATUS sdsuBufferDelete (SDSU_ID context);
IMPORT STATUS sdsuFrameFind (SDSU_ID context, const int timeout, 
       SDSU_FRAME **ppFrame);
IMPORT STATUS sdsuFrameReserve (SDSU_ID context, SDSU_FRAME *pFrame);
IMPORT STATUS sdsuFrameRelease (SDSU_ID context, SDSU_FRAME *pFrame);
IMPORT STATUS sdsuFrameSetFBA (SDSU_ID context, SDSU_FRAME *pFrame);
IMPORT STATUS sdsuFrameShow (SDSU_FRAME *pFrame);

IMPORT STATUS sdsuIntConnect (SDSU_ID context, uint32 intEnable);
IMPORT STATUS sdsuSimpleIntConnect (SDSU_ID context, uint32 intEnable);
IMPORT STATUS sdsuIntDisable (SDSU_ID context);

IMPORT STATUS sdsuSimulatePacketSync (SDSU_ID context);
IMPORT STATUS sdsuSimulateFrameSync (SDSU_ID context);
IMPORT STATUS sdsuSimulateSimpleSync (SDSU_ID context);

IMPORT STATUS sdsuReadoutOpen (SDSU_ID context, SDSU_CALLBACK packetCall,
     SDSU_CALLBACK frameCall, const BOOL useInterrupts);
IMPORT STATUS sdsuSimpleReadoutOpen (SDSU_ID context, SDSU_CALLBACK packetCall,
     SDSU_CALLBACK frameCall, const int nice, const BOOL useInterrupts);
IMPORT STATUS sdsuReadoutStart (SDSU_ID context, int totalFrames, 
       void *pPrivate);
IMPORT STATUS sdsuSimpleReadoutStart (SDSU_ID context, int totalFrames, 
        void *pPrivate);
IMPORT STATUS sdsuReadoutAbort (SDSU_ID context);
IMPORT STATUS sdsuReadoutClose (SDSU_ID context);

IMPORT STATUS sdsuParamWrite (SDSU_ID context, const uint32 destId, 
       char * paramName, uint32 paramValue);
IMPORT STATUS sdsuParamWRP (SDSU_ID context, const uint32 destId, 
       char * paramName, uint32 paramValue);
IMPORT STATUS sdsuParamRead (SDSU_ID context, const uint32 destId, 
       char * paramName, uint32 * pValue);
IMPORT STATUS sdsuParamPrint (SDSU_ID context, const uint32 destId, 
       char * paramName);
IMPORT STATUS sdsuParamDnload (SDSU_ID context, char * pFileName, 
       const long destId);
IMPORT STATUS sdsuParamUpload (SDSU_ID context, char * pFileName, 
       const long sourceId);
IMPORT STATUS sdsuStatusShow (SDSU_ID context);
IMPORT STATUS sdsuTempShow (SDSU_ID context);
IMPORT STATUS sdsu_initRepBuf (SDSU_ID context);

#endif     /* ifndef __INCsdsuLibh */
