static struct {void *v; char *c;} rcsid = {&rcsid,
   "$Id: epToVxLib.c,v 1.6 2012-10-23 21:20:19 mrippa Exp $"};

/*+
 * MODULE NAME:
 * epToVxLib
 *
 * FILENAME:
 * epToVxLib.c
 *
 * PURPOSE:
 * EPICS to VxWorks interface library
 *
 * DESCRIPTION:
 * The epToVxLib library provides a pipe-based interface between an EPICS
 * database running on one CPU and VxWorks tasks running on that and other
 * CPUs which share the same VME bus. A central EPICS database resides on
 * a nominated CPU in the system. Only the nominated CPU is required to
 * support EPICS. The other CPUs only require VxWorks support.
 *
 * The epToVxLib library uses the multi-processor pipe driver, mpPipeDrv,
 * for communication between CPUs, and the standard VxWorks pipe driver,
 * pipeDrv, for communication on the same CPU. Many of the epToVxLib
 * functions allow the pipe driver to be chosen by providing a pointer
 * to the appropriate "pipeCreate" function.
 *
 * It is assumed that a description of the EPICS records known by the
 * system is contained in array externals pWfsDbCadList, pWfsDbCarList and 
 * pWfsDbSirList, which are defined in wfsDb.h and wfsDb.c
 *
 * EXTERNAL MODULES:
 * mpPipeDrv       - Multi-processor pipe driver
 * timeoutLib      - Timeout library
 * errorLib        - Error handling library
 *
 * FUNCTION NAME(S):
 * epToVxInit            - initialisation routine for epToVxLib
 * epToVxDbInitCadCar    - initialise CAD/CAR records in epToVxLib symbol table
 * epToVxDbInitGensub    - initialise genSub records in epToVxLib symbol table
 * epToVxDbInitSir       - initialise SIR records in epToVxLib's symbol table
 * epToVxCaInit          - initialise channel access for specified EPICS record
 * epToVxCaInitRecords   - initialise all records for channel access operation
 * epToVxCaShow          - print contents of channel access definition structure
 * epToVxCaInitCar       - initialise channel access for all CAR records
 * epToVxCaInitSir       - initialise channel access for all SIR records
 * epToVxCaWrite         - channel-access write EPICS record
 * epToVxCaRead          - channel-access read EPICS record
 * epToVxCaWriteDaemon   - daemon task serves channel-access writes via pipes
 * epToVxPipeInit        - initialise pipe interface to EPICS records
 * epToVxPipeOpen        - open (and optionally create) a pipe
 * epToVxPipeWrite       - write EPICS record via pipe interface
 * epToVxSetHealth       - write to EPICS health record via pipe interface
 * epToVxCmdInit         - initialise control task prior to CAD command 
 *                         execution
 * epToVxCmdFree         - free resources allocated to control task
 * epToVxCmdRead         - read CAD command via pipe
 * epToVxCmdAttribGet    - get CAD command attribute from pipe data packet
 * epToVxCmdFinish       - issue CAD command response to CAR daemon via pipe 
 *                         interface
 * epToVxUpdateInit      - initialise control task prior to receiving genSub 
 *                         updates
 * epToVxUpdateRead      - read genSub data update via pipe
 * epToVxCarDaemon       - daemon task maintains CAR record status
 * epToVxCadInit         - generic CAD initialisation routine
 * epToVxCadExecute      - generic CAD execute routine
 * epToVxCadReject       - CAD execute routine used for unsupported commands
 * epToVxCadCopy         - CAD execute routine for copying input attributes 
 *                         to output
 * epToVxSetCadSimMode   - set simulation mode for all CAD records
 * epToVxGensubInit      - generic genSub initialisation routine
 * epToVxGensubInput     - execute routine for input genSub
 * epToVxGensubOutput    - execute routine for output genSub
 * epToVxRecContextGet   - get context structure for EPICS record
 * epToVxShow            - print information about an EPICS record
 * epToVxCadContextShow  - Print contents of CAD context structure
 * epToVxCarContextShow  - Print contents of CAR context structure
 * epToVxSirContextShow  - Print contents of SIR context structure
 * epToVxGsubContextShow - Print contents of genSub context structure
 *
 *
 * DEVELOPMENT NOTES:
 * There are a number of potential deadlock scenarios caused by the way in
 * which errors and messages are reported. If the EPICS database is used
 * to maintain an error count (as is done in the Gemini A&G Wavefront
 * Processing System) then problems may occur if any of the daemon tasks
 * in this library attempt to report an error or a message. The deadlocks
 * are avoided explicitly, as described in the comments for
 * epToVxCaWriteDaemon and epToVxCarDaemon.
 *
 * Many of the functions for dealing with CAD input and output attributes
 * assume that the values of these attributes are character strings exactly
 * EPICS_MAX_BYTES_STRING_ATTRIB in size and occupy consecutive slots
 * in the cadRecord data structure. The functions will fail if any changes
 * to EPICS render this assumption no longer valid.
 *
 * I think this library is a lot more complicated than it needs to be. This
 * library has suffered a lot from memory corruption bugs, caused by the fact
 * that memory is allocated dynamically in several places, pointers are heavily
 * used and arithmetic and type conversion are frequently carried out on 
 * pointers.
 * In the long term I think this library should be replaced by something based
 * around EPICS device support, and the pipe-based method of communicating 
 * database information should be replaced by a simpler shared memory technique.
 * SMB - 1 July 1998.
 *
 * BUGS:
 * The Channel Access ID structures for EPICS SIR records can get corrupted,
 * and I have found this happens most frequently when this library is used
 * with the DEBUG compiler flag defined. I have spent several months debugging
 * this library but have not yet managed to track this problem down. I suspect
 * it is related to the memory corruption bugs I have been seeing (as described
 * in the "BUGS" section of epToVxCaWrite and epToVxCaWriteDaemon).
 *
 * The memory corruption has been found to be entirely within the channel access
 * data structures. I have worked around it by commenting out the calls to 
 * channel access in epToVxCaWriteDaemon and epToVxCarDaemon and replacing 
 * them with database access calls.
 * SMB - 7 July 1998.
 *
 * ORIGINAL AUTHOR:
 * Nick Dillon (and Andy Foster?)
 *
 * DEBUGGED AND MODIFIED BY:
 * Steven Beard
 * Corinne Boyer
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.5  2001/10/26 03:28:09  cboyer
 * Port to epics3.13.4 version
 *
 * Revision 1.4  2000/02/03 01:19:13  cboyer
 * New V0-8 release: a lot of tidy up + sequencer created
 *
 * Revision 1.3  2000/01/05 20:09:39  cboyer
 * Tidy up the directory src: remove all the not used files and tidy up
 *
 * Revision 1.2  1999/11/04 04:06:59  cboyer
 * Modifications in order to have binning and windowing, short integer for dhs,
 * IT working for SDSU, observe which does everything, WCS, FITS header.
 *
 * Revision 1.1.1.1  1999/03/17 03:14:22  cboyer
 * Initial creation of the Gemini HRWFS repository
 *
 * Revision 1.32  1998/12/07 11:17:14  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.31  1998/11/30 15:54:35  cics
 * Modifications made during SMB visit to Hilo, November 1998
 *
 * Revision 1.30  1998/10/21 16:31:49  cics
 * A bit of tidying up
 *
 * Revision 1.29  1998/10/20 09:31:01  cics
 * eptovx_postEventsInputAttribs restored. printf in epToVxCadCopy wrapped in 
 * ifdef DEBUG
 *
 * Revision 1.28  1998/10/15 10:42:06  cics
 * epToVxCadCopy added.
 *
 * Revision 1.27  1998/10/12 10:45:13  cics
 * Error messages enhanced
 *
 * Revision 1.26  1998/10/01 13:53:24  cics
 * Some unchanged variables changed to const
 *
 * Revision 1.25  1998/09/28 08:56:48  cics
 * Give warning if an attempt if made to compile this file for anything other 
 * than vxWorks
 *
 * Revision 1.24  1998/09/09 14:35:25  cics
 * Global variables renamed to ensure they are unique
 *
 * Revision 1.23  1998/08/13 09:10:42  smb
 * Added author comment
 *
 * Revision 1.22  1998/07/16 16:25:02  smb
 * Removed superflous printf from epToVxSetHealth
 *
 * Revision 1.21  1998/07/15 15:30:12  smb
 * Hexadecimal to decimal conversion for LONG attributes fixed.
 *
 * Revision 1.20  1998/07/13 15:24:09  smb
 * More bugs fixed. Channel access replaced by database access.
 *
 * Revision 1.19  1998/07/01 12:51:39  smb
 * Work around memory corruption by padding command and data packets. Added more 
 * debugging functions.
 *
 * Revision 1.18  1998/06/30 14:07:12  smb
 * Ensure everything works when sysextLib and mpPipeDrv removed. Fixed mistakes.
 *
 * Revision 1.17  1998/06/29 16:28:16  smb
 * Removed references to sysextLib. Allow dependence on mpPipeDrv to be disabled 
 * by defining NO_MPPIPEDRV macro.
 *
 * Revision 1.16  1998/05/29 13:49:56  smb
 * Ability to transmit data updates added
 *
 * Revision 1.15  1998/05/13 10:37:41  smb
 * Added genSub processing functions
 *
 * Revision 1.14  1998/03/27 12:03:50  smb
 * Fixed bug in file descriptor array size
 *
 * Revision 1.13  1998/03/24 16:16:34  smb
 * Several bugs fixed and notes added
 *
 * Revision 1.12  1998/02/23 13:38:49  smb
 * Rearranged code for printability
 *
 * Revision 1.11  1998/02/18 11:15:21  smb
 * epToVxCadReject added
 *
 * Revision 1.10  1998/02/05 15:22:51  smb
 * CAD_DATA_TYPE -> EPICS_DATA_TYPE. Also fixed bugs in epToVxRecContextGet and 
 * epToVxPipeWrite
 *
 * Revision 1.9  1998/02/02 17:24:52  smb
 * Added epToVxCmdFree to free resources allocated by epToVxCmdInit
 *
 * Revision 1.8  1998/01/30 15:30:09  smb
 * Fixed some problems uncovered by prolint
 *
 * Revision 1.7  1998/01/28 10:10:38  smb
 * Improved errors and messages. epToVxShow can deal with SIR records
 *
 * Revision 1.6  1998/01/19 16:18:18  smb
 * Update individual health records
 *
 * Revision 1.5  1998/01/08 15:27:55  smb
 * Bugs noted but not yet fixed
 *
 * Revision 1.4  1997/12/15 16:49:37  smb
 * Fixed epToVxPipeWrite bug. Also increased cmd buffer size to 4
 *
 * Revision 1.3  1997/12/11 15:35:03  smb
 * Expand terse error messages - no changes to algorithms
 *
 * Revision 1.2  1997/12/05 14:30:53  smb
 * Bug in epToVxPipeWrite noted but not fixed yet
 *
 * Revision 1.1.1.1  1997/11/28 11:46:17  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif   /* vxWorks */

#include <stdio.h>
#include <stdlib.h>
#include <ioLib.h>
#include <string.h>
#include <symLib.h>
#include <taskLib.h>
#include <limits.h>
#include <float.h>
#include <iosLib.h>
#include <pipeDrv.h>
#include <math.h>
#include <selectLib.h>
#include <aio.h>
#include <semLib.h>
#include "gemTypes.h"
#include "timeoutLib.h"
#include "dbTypes.h"
#include "wfsHrwfsDb.h"
#include "cicsLib.h"

#define MP_PIPE_MAX_BYTES_NAME   80

#include "errorLib.h"
#include "epToVxLib.h"

#ifndef NO_EPICS /* START OF INCLUDES COMPILED ONLY FOR THE EPICS ENVIRONMENT */

#include <dbDefs.h>
#include <dbEvent.h>
#include <cad.h>
#include <menuCarstates.h>

#endif /* NO_EPICS - END OF INCLUDES COMPILED ONLY FOR THE EPICS ENVIRONMENT */

/* defines */

/*#define DEBUG*/          /* Define this macro to enable debug messages      */

#define NUM_FILES 100      /* Maximum number of file descriptors              */
                           /* (as defined in ${VX_DIR}/.../config.h)          */

   /*
    * The following are used when creating the symbol table used as a local 
    * database. The local database contains information about the EPICS records 
    * known to the system and is completely separate from the EPICS database.
    */

#define SYMTAB_HASHSIZE 1024                   /* Hash table size.            */
#define SYM_TYPE_MASK ((SYM_TYPE) 0xffffffff)  /* Mask for symbol types.      */


   /*
    * "Data packets" are messages transferred between certain EPICS records and 
    * VxWorks via dedicated pipes. They consist of four 4-byte words, which are:
    *
    * 1) A header word containing the mode (not used at present but reserved for 
    *    future enhancements);
    * 2) A header word containing the record ID, which consists of
    *    - 2 most significant bytes containing the record type plus
    *    - 2 least significant bytes containing the record number;
    * 3) A header word containing the number of data elements; and
    * 4) A pointer to the actual data.
    *
    * The following macros define the format of a data packet and enable the 
    * elements of the header (e.g. the Record-ID) to be accessed. They also give 
    * the arguments passed to the pipe-create routine(s) used for the 
    * communication pipe(s).
    */

#define DATA_PKT_NUM_HEADER_WORDS   3         /* Number of header words in    */
                                              /* data packet.                 */

#define DATA_PKT_HEADER_SIZE_BYTES  (DATA_PKT_NUM_HEADER_WORDS * 4)
                                              /* Size of data packet header in*/
                                              /* bytes (assuming 4 byte words)*/

#define DATA_PKT_MODE(context)      (* (uint32 *) (int) ((context)->pDataPacket))
                                              /* Macro to extract mode word   */
                                              /* from data packet.            */

#define DATA_PKT_RECORD_ID(context) (* (uint32 *) (int) ((context)->pDataPacket + 4))
                                              /* Macro to extract record ID   */
                                              /* from data packet.            */

#define DATA_PKT_N_ELEMENT(context) (* (uint32 *) (int) ((context)->pDataPacket + 8))
                                              /* Macro to extract no. of data */
                                              /* elements from data packet    */

#define DATA_PKT_VALUE_PTR(context) ((context)->pDataPacket + 12)
                                              /* Macro to extract pointer to  */
                                              /* the data in a data packet.   */

#define DATA_PKT_PIPE_WRITE_NMSGS   8         /* Number pipe message slots    */
                                              /* for writing to EPICS records */

#define DATA_PKT_PIPE_WRITE_MAX_BYTES ((DATA_PKT_HEADER_SIZE_BYTES) + \
                              (EPICS_MAX_BYTES_STRING_ATTRIB) + 1)
                                              /* Max. size of data packets    */
                                              /* that can be written to       */
                                              /* EPICS records.               */

/*
 * The following padding is added to every data packet as a guard against memory
 * corruption. SMB 1 July 1998.
 */

#define DATA_PKT_PADDING_BYTES      32        /* Padding allocated at end of  */
                                              /* data packet to guard against */
                                              /* memory corruption bug        */

#define DATA_PKT_TIMEOUT_OPENPIPE   -1.0      /* Timeout opening data pipe.   */
                                              /* (-1.0 means WAIT_FROEVER)    */

#define DATA_PKT_DELAY_OPENPIPE     0.1       /* Delay opening data pipe.     */


   /*
    * Data updates are transferred from genSub records to VxWorks tasks in 
    * "update packets" via pipes. Data update packets consist of four 4-byte 
    * words, which are:
    *
    * 1) The client ID;
    * 2) The command number;
    * 3) The number of data elements; and
    * 4) A pointer to the actual data.
    *
    * The following macros define the format of an update packet and enable the 
    * elements of the header to be accessed.
    *
    * NOTE: Can the data packet and data update packet definitions be merged? 
    * Was this Nick's original intention? SMB - 13 May 1998.
    */

#define UPDATE_PKT_NUM_HEADER_WORDS 3         /* Number of header words in    */
                                              /* update packet.               */

#define UPDATE_PKT_HEADER_SIZE_BYTES (UPDATE_PKT_NUM_HEADER_WORDS * 4)
                                              /* Size of command packet       */
                                              /* header in bytes (assuming 4  */
                                              /* byte words).                 */

#define UPDATE_PKT_CLIENT_ID(context) (* (uint32 *) (int) ((context)->pUpdatePacket))
                                              /* Macro to extract client ID   */
                                              /* from update packet.          */

#define UPDATE_PKT_ID_NUMBER(context) (* (uint32 *) (int) ((context)->pUpdatePacket + 4))
                                              /* Macro to extract update ID   */
                                              /* number from update packet.   */

#define UPDATE_PKT_N_ELEMENT(context) (* (uint32 *) (int) ((context)->pUpdatePacket + 8))
                                              /* Macro to extract no. of data */
                                              /* elements from update packet. */

#define UPDATE_PKT_VALUE_PTR(context) ((context)->pUpdatePacket + 12)
                                              /* Macro to extract pointer to  */
                                              /* the data in an update packet.*/

/*
 * The following padding is added to every update packet as a guard against 
 * memory corruption. SMB - 1 July 1998.
 */

#define UPDATE_PKT_PADDING_BYTES 32           /* Padding allocated at end of  */
                                              /* update packet to guard       */
                                              /* against memory corruption bug*/

   /*
    * CAD commands and associated responses are transferred between EPICS 
    * records and VxWorks tasks in "command packets" via pipes. The following 
    * definitions define the format of a CAD command packet, allow certain 
    * elements of the command "header" to be accessed, and define bits used 
    * to set things like: 
    * (i) whether the CAD directive was a START or a STOP, (ii) the current 
    * simulation mode for the command, (iii) whether this command packet 
    * indicates initiation of a CAD command ("begin" mode), or whether
    * the packet contains the response of a VxWorks task to a CAD command 
    * ("done" mode). A command packet consists of four 4-byte words, which are:
    *
    * 1) The client ID;
    * 2) The command number;
    * 3) The command modifier, which consists of
    *    - bits-0,1 = simulation mode (one of the macros EPTOVX_SIM_MODE_[VSM | 
    *      FAST | FULL | NONE])
    *    - bit-2    = START or STOP (CAD directive)
    *    - bit-3    = BEGIN or DONE (BEGIN corresponds to a command packet, 
    *      DONE to a response packet)
    *    (see the bit definitions below);
    * 4) Either, in a command packet: The default mask word, which consists of 
    *    16 bits where
    *    - When bit N is set this indicates that attribute N takes its default 
    *      value.
    *    Or, in a response packet: The error number.
    *
    * N.B. Any change to the macro CMD_PKT_COMMAND_MODIFIER() *MUST* be made 
    * also in the macros EPTOVX_IS_SIMULATION() and EPTOVX_IS_STOP_DIRECTIVE() 
    * given in epToVxLib.h.
    */

#define CMD_PKT_NUM_HEADER_WORDS   4          /* Number of header words in    */
                                              /* command packet.              */

#define CMD_PKT_HEADER_SIZE_BYTES  (CMD_PKT_NUM_HEADER_WORDS * 4)
                                              /* Size of command packet       */
                                              /* header in bytes (assuming 4  */
                                              /* byte words).                 */

#define CMD_PKT_CLIENT_ID(context) (* (uint32 *) (int) ((context)->pCmdPacket))
                                              /* Macro to extract client ID   */
                                              /* from command packet.         */

#define CMD_PKT_COMMAND_NUMBER(context) (* (uint32 *) (int) ((context)->pCmdPacket + 4))
                                              /* Macro to extract command     */
                                              /* number from command packet.  */

#define CMD_PKT_COMMAND_MODIFIER(context) (* (uint32 *) (int) ((context)->pCmdPacket + 8))
                                              /* Macro to extract command     */
                                              /* modifier from command packet.*/

#define CMD_PKT_DEFAULT_MASK(context) (* (uint32 *) (int) ((context)->pCmdPacket + 12))
                                              /* Macro to extract default     */
                                              /* mask from command packet.    */

#define CMD_PKT_DEFAULT_MASK_1(pCmdPkt) (* (uint32 *) (int) (pCmdPkt + 12))
                                              /* Alternative macro to extract */
                                              /* default mask.                */

#define CMD_PKT_ERROR_NUMBER(context) (CMD_PKT_DEFAULT_MASK (context))
                                              /* Macro to extract error       */
                                              /* number from response packet  */
                                              /* (same location as default    */
                                              /* mask) modifier               */

/*
 * The following padding is added to every command packet as a guard against 
 * memory corruption. SMB - 1 July 1998.
 */

#define CMD_PKT_PADDING_BYTES 32              /* Padding allocated at end of  */
                                              /* command packet to guard      */
                                              /* against memory corruption bug*/

   /*
    * The pipes used by epToVxLib are given names which are based on: (i) the 
    * name of an EPICS record or (ii) the name of a VxWorks control task 
    * associated with one or more EPICS records.
    * The following macros define name extensions (and some constant pipe names) 
    * that are used when constructing these pipe names.
    */

#define CAD_TO_TASK_PIPE_NAME_EXT  "_CadToTask" /* Extension for command pipe */
#define CAD_TO_CAR_PIPE_NAME_EXT   "_CadToCar"  /* Extension for CAD-CAR pipe */
#define TASK_TO_CAR_PIPE_NAME_EXT  "_TaskToCar" /* Extension for response pipe*/
#define GSUB_TO_TASK_PIPE_NAME_EXT "_GsubToTask" 
                                                /* Extension for genSub pipe  */
#define CA_WRITE_PIPE_NAME         "recWrite"   /* Name of record write pipe  */
#define CA_READ_PIPE_NAME          "recRead"    /* Name of record read pipe   */

   /*
    * Each CAD command has an associated "CAD structure" (typedef'd as 
    * CAD_CONTEXT). This contains all of the information needed to define the 
    * command, such as default attribute values, the permitted range(s) for each
    * attribute and the timeout period allowed for execution of the command etc.
    * Similarly, VxWorks control tasks use a "CAD command structure"
    * (typedef'd as CAD_CMD_CONTEXT) to define all of the commands to which the 
    * task responds.
    * The following macro extracts the timeout period for a specified command 
    * from a task's CAD command structure.
    */

#define CAR_PIPE_TIMEOUT(context, cmd) ((context)->ppCadContext [cmd]->timeout)

/* START OF DEFINITIONS COMPILED ONLY FOR THE EPICS ENVIRONMENT */
#ifndef NO_EPICS 

#define CAR_TIMEOUT_OPENPIPE 60.0             /* Timeout opening pipe to CAR. */
#define CAR_DELAY_OPENPIPE   0.1              /* Delay opening pipe to CAR.   */

   /*
    * Definitions of the fields of a CAR record. These *MUST* be consistent with
    * the definition & initialisation of the external array 
    * pppWfsDbRecFieldName[][]. The argument to these macros is a CAR context 
    * structure (type CAR_CONTEXT).
    */

#define CAR_FIELD_CLIENT_ID(context) (* (long *) (int) ((context)->caContext->ppFieldValue [0]))
                                              /* Macro to extract client ID.  */

#define CAR_FIELD_ERROR_NUMBER(context) (* (long *) (int) ((context)->caContext->ppFieldValue [1]))
                                              /* Macro to extract error no.   */

#define CAR_FIELD_MESSAGE(context) ( (char *) (int) ((context)->caContext->ppFieldValue [2]))
                                              /* Macro to extract error msg.  */

#define CAR_FIELD_VALUE(context) (* (long *) (int) ((context)->caContext->ppFieldValue [3]))
                                              /* Macro to extract CAR value.  */

   /*
    * The 4 byte (32 bit) client-id field for each CAR consists of the 
    * following:
    * - Bits 25-32 contain the least significant byte of the command modifier 
    *   word, so
    *   ... bits 25,26 contain the simulation mode,
    *   ... bit 27 contains START or STOP CAD directive,
    *   ... bit 28 contains BEGIN or DONE;
    * - Bits 17-24 contain the command number associated with the CAD record;
    * - Bits 0-16 contain the least significant 16 bits of the client ID (issued
    *   as a "transaction count").
    * This design results in the top 16 bits of the transaction count being 
    * unavailable at the CAR's client-id field. However, the other information 
    * such as the simulation mode and command number are more important. The 
    * macro CAR_CLIENT_ID() assembles the long value to be written to a CAR's 
    * client-id field give a CAD response packet.
    *
    * NOTE: This is a hideously complicated macro. Could this not have been done
    * in a function instead? SMB - 23 March 1998.
    */

#define CAR_CLIENT_ID(context) (((CMD_PKT_COMMAND_MODIFIER (context) & \
                                  CAD_COMMAND_MODIFIER_MASK) << 24) |  \
                                ((CMD_PKT_COMMAND_NUMBER (context) &   \
                                  0x0fff) << 16) |                     \
                                 (CMD_PKT_CLIENT_ID (context) & 0xffff))

   /*
    * The daemon task epToVxCarDaemon() uses the asynchronous I/O library 
    * (aioLib) to the response to a CAD command with a timeout specified for 
    * the pipe read. The following macro defines priority of the aio read with 
    * respect to that of the CAR daemon task
    */

#define CAR_COMMAND_DONE_AIO_PRIORITY 0       /* Assume same priority as      */
                                              /* daemon task.                 */

   /*
    * The following macros access EPICS record field values as either a long, 
    * double or string (char *)
    */

#define REC_FIELD_VALUE_LONG(caContext) (* (long *) (int) ((caContext)->ppFieldValue [0]))
                                              /* Access field value as long.  */

#define REC_FIELD_VALUE_DOUBLE(caContext) (* (double *) (int) ((caContext)->ppFieldValue [0]))
                                              /* Access field value as double.*/

#define REC_FIELD_VALUE_STRING(caContext) (  (char *) ((caContext)->ppFieldValue [0]))
                                              /* Access field value as string.*/

   /* Some more defines associated with CAD records */

#define CAD_ATTRIBUTE_DEFAULT_STRING "DEFAULT"
                                              /* A CAD attribute value which  */
                                              /* contains this string will be */
                                              /* replaced by the default      */
                                              /* value for the attribute.     */

#define CAD_MAX_TRANSACTION_NUMBER 0xffffffff /* Maximum transaction count    */
                                              /* for a CAD record.            */

#endif /* NO_EPICS - END OF DEFINITIONS COMPILED ONLY FOR THE EPICS ENVIRONMENT */


/* local function declarations and global variables */

   /*
    * Global variables have the unique prefix "epToVx" or "pEpToVx".
    */

LOCAL int      getNumberAttribRangeValues (char ** ppAttribRangeValue);
LOCAL uint32   getNumberAttribs (CAD_ATTRIB * pAttribList);
LOCAL STATUS   attribStringToUnion (uint32 type, char * pStringAttrib, 
                                    CAD_ATTRIB_VALUE * pAttrib);
LOCAL char *   getAttribSourceAddrs (CAD_CONTEXT pContext, char * pCmdPacket, 
                                     uint32 attribNumber, uint32 defaultMask);
LOCAL STATUS   waitPipeExists (char * pPipeName, const double timeoutPeriod, 
                               const double timeoutDelay);
LOCAL BOOL     pipeExists (char * pName);
LOCAL BOOL     filter (DATREC_CONTEXT pContext, char * pValue);

LOCAL int      epToVxPipeWriteFd = ERROR;  /* File descriptor used to write   */
                                           /* EPICS records (initialised to   */
                                           /* ERROR until a pipe is actually  */
                                           /* opened).                        */

LOCAL SEM_ID   epToVxPipeWriteSem = NULL;  /* Semaphore protecting pipe used  */
                                           /* to update EPICS records.        */

SYMTAB_ID      epToVxSymtab = NULL;        /* ID of symbol table to contain   */
                                           /* information about EPICS records */
                                           /* (initialised to NULL until      */
                                           /* symbol table is created).       */

#ifndef NO_EPICS   /* START OF CODE COMPILED ONLY FOR THE EPICS ENVIRONMENT */

LOCAL BOOL     eptovx_readyToAcceptCmd (CAD_CONTEXT pContext);
LOCAL STATUS   eptovx_attribPutCmdPacket (CAD_CONTEXT pContext, 
                                          uint32 attribNumber, 
                                          char * pAttribString);
LOCAL void     eptovx_loadDefaultInputAttribs (CAD_CONTEXT pContext, 
                                               struct cadRecord * pcad);
LOCAL STATUS   eptovx_checkAttribs (CAD_CONTEXT pContext, 
                                    uint32 * pOffendingAttrib,
                                    char * pReason);
LOCAL STATUS   eptovx_saveAttribs (struct cadRecord * pcad, 
                                   CAD_CONTEXT pContext,
                                   uint32 * pOffendingAttrib);
LOCAL STATUS   eptovx_copyAttrib (const char name, const char * pInput, 
                                  const int type, void * pOutput, 
                                  char * pMessage);
/*LOCAL void     eptovx_writeCadOutAttribs (CAD_CONTEXT pContext, 
                                          struct cadRecord * pcad);*/
LOCAL void     eptovx_postEventsInputAttribs (CAD_CONTEXT pContext, 
                                              struct cadRecord * pcad);
/*LOCAL void     eptovx_postEventsOutputAttribs (CAD_CONTEXT pContext, 
                                               struct cadRecord * pcad); */

LOCAL int      epToVxCadSimMode = EPTOVX_SIM_MODE_NONE; 
                                           /* Simulation mode (initialised to */
                                           /* NONE).                          */
LOCAL CA_DEF * ppEpToVxCaDefTable [N_RECORD_TYPES];
                                           /* Array of channel access         */
                                           /* definition tables for each type */
                                           /* of record (CAD, CAR, SIR).      */

                                           /* NOTE: The CAD type is never     */
                                           /* defined and the CAR type is     */
                                           /* never used. SMB - 26 Mar 1998.  */

LOCAL SEM_ID   epToVxCaDefSem = NULL;      /* Semaphore protecting channel    */
                                           /* access definition table.        */

/* -------------------------------------------------------------------------- */


/*+
 *   FUNCTION NAME:
 *   epToVxCaInit
 *
 *   INVOCATION:
 *   epToVxCaInit (recordType, pRecordName, pCaDef)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) recordType  (const int)    EPICS record type identifier
 *   (>) pRecordName (const char *) name of record to initialise
 *   (!) pCaDef      (CA_DEF)       channel-access definition structure to 
 *                                  initialise
 *
 *   FUNCTION VALUE:
 *   (STATUS)  OK, or ERROR if initialisation failed
 *
 *   PURPOSE:
 *   Initialise a record to enable channel access to it
 *
 *   DESCRIPTION:
 *   This routine initialises the structure pCaDef such that the structure
 *   can be used subsequently as a handle to the record for channel-
 *   access to it. The routine is used internally by epToVxLib and, in
 *   many applications, will not be called directly.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   (>) pppWfsDbRecFieldName (char ***) array of record field names
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEVELOPMENT NOTES:
 *   The field type is stored twice, once in the channel access ID (chid) 
 *   structure and again in the channel access definition structure. It would 
 *   be safer and more efficient if this duplication could be removed.
 *   SMB - 16 Mar 1998.
 *
 *   There is a race condition in which it is possible for this function
 *   to crash with an error message, but the crash itself prevents the
 *   error logging task from running and displaying the error message.
 *   All error messages are duplicated through "printErr" statements
 *   to ensure they appear at the console.
 *-
 */

STATUS   epToVxCaInit
   (
   const int      recordType,
   const char *   pRecordName,
   CA_DEF         pCaDef
   )
{
   FAST uint32    i;
   FAST int       fieldNumber;
   int            f;
   char pName [EPICS_MAX_BYTES_RECORD_NAME + EPICS_MAX_BYTES_FIELD_NAME + 2];
                                                    /* Reserve space for      */
                                                    /* recordName + fieldName */
                                                    /* + period + null        */

   /*
    * Copy the record type into pCaDef. Init the number of fields supported
    * on this record with the maximum allowed, then search through the list of 
    * defined field names (pppWfsDbRecFieldName[recordType][]) for records of 
    * this type and set the number of fields to the number that have non-NULL 
    * name strings.
    */

   pCaDef->recordType = recordType;
   pCaDef->nField = EPICS_MAX_NFIELD_PER_RECORD;
   for (i = 0; i < EPICS_MAX_NFIELD_PER_RECORD; i++)
   {
      if (strlen (pppWfsDbRecFieldName [recordType][i]) == 0)
      {
         pCaDef->nField = i;
         break;
      }
   }

   /*
    * Allocate some memory for some structure elements, using calloc() to 
    * initialise the allocated memory to zero.
    */

   if ((pCaDef->pChannelId = (chid *) calloc ((size_t) pCaDef->nField, 
                                              sizeof (chid))) == NULL)
   {
      printErr (
      "epToVxCaInit: Memory allocation for pChannelId structures failed\n");
      ERROR_SET (0, "Memory allocation for pChannelId structures failed", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   if ((pCaDef->pFieldType = (chtype *) calloc ((size_t) pCaDef->nField, 
                                                sizeof (chtype))) == NULL)
   {
      printErr (
      "epToVxCaInit: Memory allocation for pFieldType array failed\n");
      ERROR_SET (0, "Memory allocation for pFieldType array failed", 
                 ERROR_LOG_SAVE);
      cfree ((char *) pCaDef->pChannelId);
      return (ERROR);
   }

   if ((pCaDef->ppFieldValue = (char **) calloc ((size_t) pCaDef->nField, 
                                                 sizeof (char *))) == NULL)
   {
      printErr (
      "epToVxCaInit: Memory allocation for ppFieldValue array failed\n");
      ERROR_SET (0, "Memory allocation for ppFieldValue array failed", 
                 ERROR_LOG_SAVE);
      cfree ((char *) pCaDef->pFieldType);
      cfree ((char *) pCaDef->pChannelId);
      return (ERROR);
   }

   /*
    * For each field, get the channel-access IDs, initialise the field types in 
    * pCaDef and reserve memory for each field
    */

   for (fieldNumber = 0; fieldNumber < pCaDef->nField; fieldNumber++)
   {
      strncpy (pName, pRecordName, EPICS_MAX_BYTES_RECORD_NAME);
      strncat (pName, pppWfsDbRecFieldName [recordType][fieldNumber], 
               EPICS_MAX_BYTES_FIELD_NAME+1);

      if (ca_search (pName, & pCaDef->pChannelId [fieldNumber]) != ECA_NORMAL)
      {
         printErr ("epToVxCaInit: Failed to get channel access ID for \"%s\"", 
                   pName);
         ERROR_SET1 (0, "Failed to get channel access ID for \"%s\"", 
                     ERROR_LOG_SAVE, pName);
         cfree ((char *) pCaDef->ppFieldValue);
         cfree ((char *) pCaDef->pFieldType);
         cfree ((char *) pCaDef->pChannelId);
         return (ERROR);
      }

      pCaDef->pFieldType [fieldNumber] = 
      ca_field_type (pCaDef->pChannelId [fieldNumber]);

      /* Further check added. SMB - 3 July 1998. */

      if (dbr_size [pCaDef->pFieldType [fieldNumber]] <= 0)
      {
         printErr ("epToVxCaInit: WARNING - field value %d of record \"%s\" has zero size\n",
                   fieldNumber, pName);
      }

      pCaDef->ppFieldValue [fieldNumber] =
      (char *) calloc (1, dbr_size [pCaDef->pFieldType [fieldNumber]]);

      /* The result of this calloc is now checked. SMB - 16 Mar 1998. */

      if (pCaDef->ppFieldValue [fieldNumber] == NULL)
      {
         printErr ("epToVxCaInit: Memory allocation for field value %d of record \"%s\" failed",
                   fieldNumber, pName);
         ERROR_SET1 (0, "Memory allocation for field value %d failed", 
                     ERROR_LOG_SAVE, fieldNumber);
         for (f = 0; f < fieldNumber; f++)
         {
            cfree ((char *) pCaDef->ppFieldValue[f]);      
         }
         cfree ((char *) pCaDef->ppFieldValue);
         cfree ((char *) pCaDef->pFieldType);
         cfree ((char *) pCaDef->pChannelId);
         return (ERROR);
      }

#ifdef DEBUG
      printf ("epToVxCaInit: Record \"%s\": allocated %d bytes, CA_DEF structure is @ %#x\n",
      pRecordName, dbr_size [pCaDef->pFieldType [fieldNumber]], (int) pCaDef);
#endif /* DEBUG */
   }

   /* Tidy up after channel-access routines */

   if (ca_pend_io (EPTOVX_TIMEOUT_CA) != ECA_NORMAL)
   {
      printErr ("epToVxCaInit: Channel access pend failed\n");
      ERROR_SET (0, "Channel access pend failed", ERROR_LOG_SAVE);
      for (fieldNumber = 0; fieldNumber < pCaDef->nField; fieldNumber++)
      {
         cfree ((char *) pCaDef->ppFieldValue[fieldNumber]);      
      }
      cfree ((char *) pCaDef->ppFieldValue);
      cfree ((char *) pCaDef->pFieldType);
      cfree ((char *) pCaDef->pChannelId);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxCaShow
 *
 *   INVOCATION:
 *   epToVxCaShow (pCaDef, verbose)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pCaDef  (CA_DEF)     channel-access definition structure
 *   (>) verbose (const BOOL) Verbose output flag
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Display the contents of a channel access definition structure
 *
 *   DESCRIPTION:
 *   This routine display the contents of the channel access definition
 *   structure pCaDef. In verbose mode the contents of the channel access
 *   ID structure is also displayed.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   cadef.h
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

void   epToVxCaShow
   (
   CA_DEF      pCaDef,
   const BOOL   verbose
   )
{
   FAST int   i;

   printf ("epToVxCaShow: Contents of CA def structure located at %#x.\n", 
           (int) pCaDef);

   printf ("epToVxCaShow: Record type = %d, Number of fields = %d\n",
            pCaDef->recordType, pCaDef->nField);

   printf ("epToVxCaShow: field chid     name                           type  value\n");
   printf ("epToVxCaShow: ----- -------- ------------------------------ ----- -----\n");
   for (i = 0; i < pCaDef->nField; i++)
   {
      printf ("epToVxCaShow: %-5d %#8x %-30s %-5d \"%s\"\n",
              i,
              (int) pCaDef->pChannelId [i],
              ca_name( pCaDef->pChannelId [i] ),
              (int)(pCaDef->pFieldType [i]),
              pCaDef->ppFieldValue [i]);
   }

   /* In verbose mode display the contents of all the channel access IDs. */

   if ( verbose )
   {
      for (i = 0; i < pCaDef->nField; i++)
      {
         if ( &(pCaDef->pChannelId [i]) != NULL)
         {
            printf ("\n");
            epToVxChidShow (pCaDef->pChannelId [i]);
         }
      }
   }

   return;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxChidShow
 *
 *   INVOCATION:
 *   epToVxChidShow (pChid)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pChid      (chid)   channel-access definition structure
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Display the contents of a channel access ID (chid) structure
 *
 *   DESCRIPTION:
 *   This routine display the contents of the channel access ID
 *   structure which pChid points to.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   cadef.h
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

void   epToVxChidShow
   (
   chid   pChid
   )
{
   printf ("epToVxChidShow: Contents of CHID structure located at %#x.\n", 
           (int) pChid);

   printf ("epToVxChidShow: Field name = %s\n", ca_name( pChid ) );
   printf ("epToVxChidShow: Host name = %s\n", ca_host_name( pChid ) );
   printf ("epToVxChidShow: Field type = %d, Element count = %d, Channel state = %d\n",
           (int) ca_field_type( pChid ), ca_element_count( pChid ), 
           (int) ca_state( pChid ) );
   printf ("epToVxChidShow: Access = ");
   if ( ca_read_access( pChid ) == 1 )
   {
      printf ("read + ");
   }
   else
   {
      printf ("NO read + ");
   }
   if ( ca_write_access( pChid ) == 1 )
   {
      printf ("write.\n");
   }
   else
   {
      printf ("NO write.\n");
   }

   return;
}

/* -------------------------------------------------------------------------- */


/*+
 *   FUNCTION NAME:
 *   epToVxCaInitRecords
 *
 *   INVOCATION:
 *   epToVxCaInitRecords ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the EPICS records could not be initialised
 *   for channel access
 *
 *   PURPOSE:
 *   Initialise all EPICS records for channel access operations
 *
 *   DESCRIPTION:
 *   This routine initialises all the EPICS records in the system to make
 *   them accessible through epToVxLib functions. It initialises the Channel
 *   Access library and then calls epToVxInitCar() and epToVxInitSir().
 *   The function is designed to be executed from a VxWorks startup script.
 *
 *   Note that only CAR and SIR records are initialised by this function.
 *   CAD records are initialised automatically through the epToVxCadInit
 *   specified in their INAM field.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   (!) epToVxSymtab         (SYMTAB_ID)    symbol table used internally by 
 *                                           epToVxLib
 *   (<) ppEpToVxCaDefTable   (CA_DEF *)     table of channel-access definition 
 *                                           structures
 *   (>) pWfsDbCarList        (CAR_RECORD *) array defining the system's CAR 
 *                                           records
 *   (>) wfsDbNCarRecord      (int)          number of CAR records known to the 
 *                                           system
 *   (>) pWfsDbSirList        (SIR_RECORD *) array defining the system's SIR 
 *                                           records
 *   (>) wfsDbNSirRecord      (int)          number of SIR records known to the 
 *                                           system
 *   (!) pWfsDbRecInitialised (BOOL *)       array of record-initialisation-done
 *                                           flags
 *   (>) wfsDbEpicsDbIsLocal  (BOOL)         flag indicates whether EPICS 
 *                                           database is local
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEVELOPMENT NOTES:
 *   There is a race condition in which it is possible for this function
 *   to crash with an error message, but the crash itself prevents the
 *   error logging task from running and displaying the error message.
 *   All error messages are duplicated through "printErr" statements
 *   to ensure they appear at the console.
 *-
 */

STATUS   epToVxCaInitRecords (void)
{

   /* Initialise Channel Access library */

   if (ca_task_initialize () != ECA_NORMAL)
   {
      printErr ( "epToVxCaInitRecords: Failed to initialise channel access\n" );
      ERROR_SET (0, "Failed to initialise channel access", ERROR_LOG_NOW);
      return (ERROR);
   }

   /*
    * NOTE: CAD records do not need to be initialised here.
    * They are initialised automatically by iocInit().
    */

   /* Initialise the CAR records. */

   if (epToVxCaInitCar() == ERROR)
   {
      printErr ("epToVxCaInitRecords: Error initialising CAR records\n");
      ERROR_LOG ("Error initialising CAR records");
      return(ERROR);
   }

   /* Initialise the SIR records. */

   if (epToVxCaInitSir() == ERROR)
   {
      printErr ("epToVxCaInitRecords: Error initialising SIR records\n");
      ERROR_LOG ("Error initialising SIR records");
      return(ERROR);
   }

   return (OK);
}


/* -------------------------------------------------------------------------- */


/*+
 *   FUNCTION NAME:
 *   epToVxCaInitCar
 *
 *   INVOCATION:
 *   epToVxCaInitCar ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the CAR records could not be initialised
 *   for channel access
 *
 *   PURPOSE:
 *   Initialise all CAR records for channel access operations
 *
 *   DESCRIPTION:
 *   This routine initialises all CAR records in the system in order that they
 *   can be accessed subsequently via the Channel Access routines (epToVxCa...)
 *   and by the CAR daemon task (epToVxCarDaemon). The symbol table epToVxSymtab
 *   is updated with information about the records.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   (!) epToVxSymtab         (SYMTAB_ID)     Symbol table used internally by 
 *                                            epToVxLib
 *   (<) ppEpToVxCaDefTable   (CA_DEF *)      Table of channel-access definition
 *                                            structures
 *   (>) pWfsDbCarList        (CAR_RECORD *)  Array defining the system's CAR 
 *                                            records
 *   (>) wfsDbNCarRecord      (int)           Number of CAR records known to the
 *                                            system
 *   (!) pWfsDbRecInitialised (BOOL *)        Array of record-initialisation-
 *                                            done flags
 *   (<) (pContext)           (CAR_CONTEXT *) Not strictly a global variable, 
 *                                            but added to the epToVxSymtab 
 *                                            symbol table and therefore 
 *                                            accessible outside this function.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEVELOPMENT NOTES:
 *   There is a race condition in which it is possible for this function
 *   to crash with an error message, but the crash itself prevents the
 *   error logging task from running and displaying the error message.
 *   All error messages are duplicated through "printErr" statements
 *   to ensure they appear at the console.
 *
 *   BUGS:
 *   If an error occurs during the execution of this function, the CA definition
 *   table and associated data structures may be left in a partially built 
 *   state. 
 *   The memory allocated by this function is not freed. SMB - 8 Oct 1998.
 *-
 */

STATUS epToVxCaInitCar (void)
{
   FAST int      i;
   CAR_CONTEXT   pContext;

   /*
    * Initialise the library (if necessary) and allocate memory for a table of 
    * channel-access definition tables for all CARs
    */

   if (epToVxInit () == ERROR)
   {
      printErr ("epToVxCaInitCar: Failed to initialise epToVxLib\n");
      ERROR_SET (0, "Failed to initialise epToVxLib", ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Get exclusive access to the channel access definition structures. */

   if (semTake (epToVxCaDefSem, NO_WAIT) == ERROR)
   {
      printErr ("epToVxCaInitCar: Could not take epToVxCaDefSem semaphore\n");
      ERROR_SET (0, "Could not take epToVxCaDefSem semaphore", ERROR_LOG_SAVE);
      return (ERROR);
   }

   ppEpToVxCaDefTable [CAR_RECORD_TYPE] = 
   (CA_DEF *) calloc (wfsDbNCarRecord, sizeof (CA_DEF));
   if (ppEpToVxCaDefTable [CAR_RECORD_TYPE] == NULL)
   {
      printErr ("epToVxCaInitCar: Memory allocation for CAR record table failed\n");
      ERROR_SET (0, "Memory allocation for CAR record table failed", 
                 ERROR_LOG_SAVE);
      semGive (epToVxCaDefSem);
      return (ERROR);
   }

   /*
    * For each CAR, allocate memory for its context structure (of type 
    * CAR_CONTEXT), initialise the CAR contexts with details of the record taken
    * from the declaration of pWfsDbCarList[] and initialise a channel-access 
    * definition structure (member caContext of each CAR context structure) such
    * that Channel Access may subsequently be performed to the CARs.
    */

   for (i = 0; i < wfsDbNCarRecord; i++)
   {
      if ((pContext = (CAR_CONTEXT) calloc (1, sizeof (CAR_CONTEXT_STRUCT))) 
          == NULL)
      {
         printErr ("epToVxCaInitCar: Memory allocation for CAR %d context structure failed\n", i);
         ERROR_SET1 (0, "Memory allocation for CAR %d context structure failed",
            ERROR_LOG_SAVE, i);
         semGive (epToVxCaDefSem);
         return (ERROR);
      }
      if ((pContext->caContext = (CA_DEF) calloc (1, sizeof (CA_DEF_STRUCT))) 
          == NULL)
      {
         printErr (
         "epToVxCaInitCar: Memory allocation for CAR %d CA context structure failed\n", i);
         ERROR_SET1 (0, 
            "Memory allocation for CAR %d CA context structure failed",
            ERROR_LOG_SAVE, i);
         semGive (epToVxCaDefSem);
         return (ERROR);
      }
      pContext->pTaskName = pWfsDbCarList [i].pTaskName;

      /* Add symbol for current record to symbol table */

      if (symAdd (epToVxSymtab, pWfsDbCarList [i].pRecordName, 
                  (char *) pContext, (SYM_TYPE) CAR_RECORD_TYPE, (UINT16) 0) 
                  == ERROR)
      {
         printErr (
              "epToVxCaInitCar: Failed to add CAR \"%s\" to symbol table\n",
              pWfsDbCarList [i].pRecordName);
         ERROR_SET1 (0, "Failed to add CAR \"%s\" to symbol table", 
                     ERROR_LOG_SAVE, pWfsDbCarList [i].pRecordName);
         semGive (epToVxCaDefSem);
         return (ERROR);
      }

#ifdef DEBUG
      printf ("epToVxCaInitCar: Added CAR record %d = \"%s\" to symbol table\n", 
              i, pWfsDbCarList [i].pRecordName);
#endif /* DEBUG */

      if (epToVxCaInit (CAR_RECORD_TYPE, pWfsDbCarList [i].pRecordName, 
                        pContext->caContext) == ERROR)
      {
         printErr (
             "epToVxCaInitCar: Failed to initialise CA for CAR record \"%s\"\n",
             pWfsDbCarList [i].pRecordName);
         ERROR_SET1 (0, "Failed to initialise CA for CAR record \"%s\"", 
                     ERROR_LOG_SAVE, pWfsDbCarList [i].pRecordName);
         semGive (epToVxCaDefSem);
         return (ERROR);
      }
      ppEpToVxCaDefTable [CAR_RECORD_TYPE][i] = pContext->caContext;

#ifdef DEBUG
      epToVxShow (pWfsDbCarList [i].pRecordName, 0);
#endif /* DEBUG */
   }

   /* Release the channel access definition structures. */

   semGive (epToVxCaDefSem);

   /* Mark all CARs as initialised */

   pWfsDbRecInitialised [CAR_RECORD_TYPE] = TRUE;

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxCaInitSir
 *
 *   INVOCATION:
 *   epToVxCaInitSir ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the SIR records could not be initialised
 *
 *   PURPOSE:
 *   Initialise all SIR records for channel access operations
 *
 *   DESCRIPTION:
 *   This routine initialises all SIR records in the system in
 *   order that they can be accessed subsequently via the channel-
 *   access routines (epToVxCa..) and by the SIR daemon task
 *   (epToVxCaWriteDaemon). The symbol table epToVxSymtab is updated with
 *   information about the record.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   (!) epToVxSymtab         (SYMTAB_ID)    symbol table used internally by 
 *                                           epToVxLib
 *   (<) ppEpToVxCaDefTable   (CA_DEF *)     table of channel-access definition 
 *                                           structures
 *   (>) pWfsDbSirList        (SIR_RECORD *) array defining the system's SIR 
 *                                           records
 *   (>) wfsDbNSirRecord      (int)          number of SIR records known to the 
 *                                           system
 *   (!) pWfsDbRecInitialised (BOOL *)       array of record-initialisation-done
 *                                           flags
 *   (>) wfsDbEpicsDbIsLocal  (BOOL)         flag indicates whether EPICS 
 *                                           database is local
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEVELOPMENT NOTES:
 *   There is a race condition in which it is possible for this function
 *   to crash with an error message, but the crash itself prevents the
 *   error logging task from running and displaying the error message.
 *   All error messages are duplicated through "printErr" statements
 *   to ensure they appear at the console.
 *
 *   BUGS:
 *   If an error occurs during the execution of this function, the memory 
 *   allocated by this function is not freed. SMB - 8 Oct 1998.
 *-
 */

STATUS   epToVxCaInitSir (void)
{
   FAST int         i;
   DATREC_CONTEXT   pContext;
   SYM_TYPE         symType;

   /* Initialise the library (if necessary) */

   if (epToVxInit () == ERROR)
   {
      printErr ("epToVxCaInitSir: Failed to initialise epToVxLib\n");
      ERROR_SET (0, "Failed to initialise epToVxLib", ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* If the EPICS database resides on this CPU, initialise SIR records in the 
    * local database 
    */

   if (wfsDbEpicsDbIsLocal)
   {
      if (epToVxDbInitSir () == ERROR)
      {
         printErr ("epToVxCaInitSir: Failed to initialise SIR records in local database\n");
         ERROR_SET (0, "Failed to initialise SIR records in local database", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }
   }

   /* Get exclusive access to the channel access definition structures. */

   if (semTake (epToVxCaDefSem, NO_WAIT) == ERROR)
   {
      printErr ("epToVxCaInitSir: Could not take epToVxCaDefSem semaphore\n");
      ERROR_SET (0, "Could not take epToVxCaDefSem semaphore", ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Allocate some memory for channel access definitions */

   ppEpToVxCaDefTable [SIR_RECORD_TYPE] =
      (CA_DEF *) calloc ((size_t) wfsDbNSirRecord, sizeof (CA_DEF));
   if (ppEpToVxCaDefTable [SIR_RECORD_TYPE] == NULL)
   {
      printErr ("epToVxCaInitSir: Memory allocation for SIR CA definition table failed\n");
      ERROR_SET (0, "Memory allocation for SIR CA definition table failed", 
                    ERROR_LOG_SAVE);
      semGive (epToVxCaDefSem);
      return (ERROR);
   }

   /*
    * Look-up each SIR in the local database, allocate memory for its context 
    * structure and initialise Channel Access to the record.
    */

   for (i = 0; i < wfsDbNSirRecord; i++)
   {
      if (symFindByNameAndType (epToVxSymtab, pWfsDbSirList [i].pRecordName, 
                                (char **) & pContext, & symType, 
                                (SYM_TYPE) SIR_RECORD_TYPE, SYM_TYPE_MASK) 
          == ERROR)
      {
         printErr (
              "epToVxCaInitSir: Could not find SIR \"%s\" in symbol table\n",
              pWfsDbSirList [i].pRecordName);
         ERROR_SET1 (0, "Could not find SIR \"%s\" in symbol table", 
                     ERROR_LOG_SAVE, pWfsDbSirList [i].pRecordName);
         semGive (epToVxCaDefSem);
         return (ERROR);
      }

      if ((pContext->caContext = (char *) calloc (1, sizeof (CA_DEF_STRUCT))) == 
          NULL)
      {
         printErr ("epToVxCaInitSir: Memory allocation for SIR CA context structure failed\n");
         ERROR_SET (0, "Memory allocation for SIR CA context structure failed", 
                    ERROR_LOG_SAVE);
         semGive (epToVxCaDefSem);
         return (ERROR);
      }

#ifdef DEBUG
      printf ("epicsSirInit: Initialising CA for SIR record %d = \"%s\"\n", i,
              pWfsDbSirList [i].pRecordName);
#endif /* DEBUG */

      if (epToVxCaInit (SIR_RECORD_TYPE, pWfsDbSirList [i].pRecordName, 
          (CA_DEF) (int) pContext->caContext) == ERROR)
      {
         printErr ("epToVxCaInitSir: Failed to initialise CA for SIR record \"%s\"\n",
                   pWfsDbSirList [i].pRecordName);
         ERROR_SET1 (0, "Failed to initialise CA for SIR record \"%s\"", 
                     ERROR_LOG_SAVE, pWfsDbSirList [i].pRecordName);
         semGive (epToVxCaDefSem);
         return (ERROR);
      }
      ppEpToVxCaDefTable [SIR_RECORD_TYPE][i] = 
      (CA_DEF) (int) pContext->caContext;

#ifdef DEBUG
      epToVxShow (pWfsDbSirList [i].pRecordName, 0);
#endif /* DEBUG */
   }

   /* Release the channel access definition structures. */

   semGive (epToVxCaDefSem);

   /* Mark all SIRs as initalised */

   pWfsDbRecInitialised [SIR_RECORD_TYPE] = TRUE;

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxCaWrite
 *
 *   INVOCATION:
 *   epToVxCaWrite (pCaContext)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pCaContext   (CA_DEF)   channel-access definition structure pointer
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if channel-access to the record failed
 *
 *   PURPOSE:
 *   Do a channel-access write to an EPICS record
 *
 *   DESCRIPTION:
 *   This routine calls the channel access library routines ca_array_put()
 *   and ca_flush_io() to write to a named EPICS record. The pCaContext pointer
 *   is assumed to point to a valid CA_DEF structure and this is used to extract
 *   the record field values which are written to the record.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   The pCaContext structure must have been initialised via
 *   a previous call to an appropriate record-initialisation routine, e.g.
 *   epToVxCaInitCar or epToVxCaInitSir in the case of CAR or SIR records
 *   respectively and should be set up to contain the record field
 *   values prior to calling this function.
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *   BUGS:
 *   There is an intermittent bug in this function which generates an access
 *   fault in the EPICS function dbPutfield(), which is called from 
 *   ca_array_put(). I think this may be a symptom of a memory corruption.
 *   SMB - 27 Jan 1998.
 *
 *   I have also seen "Channel access array put failed" errors being generated
 *   for no apparent reason.
 *   SMB - 19 Feb 1998.
 *
 *   I have investigated the above problems and have found that the channel 
 *   access ID structures are getting overwritten. I don't yet know why. 
 *   I am trapping the corrupted structures as a work-around.
 *   SMB - 2 Mar 1998.
 *
 *   The above bugs have been avoided by not using this function. The CICS data 
 *   access functions are used instead.
 *   SMB - 27 Jan 1999.
 *-
 */

STATUS   epToVxCaWrite
   (
   CA_DEF   pCaContext
   )
{
   FAST int      fieldNumber;

   /* Check the channel access definition structure provided */

   if (pCaContext == NULL)
   {
      ERROR_SET (S_epToVxLib_INTERNAL_ERROR, 
                 "Null channel access definition structure",
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Write record fields in the order in which they are listed in the array
    * pppWfsDbRecFieldName[][]. The last field is typically ".IVAL" in order 
    * to ensure that the record is processed AFTER all other relevant fields 
    * have been updated.
    */

   for (fieldNumber = 0; fieldNumber < pCaContext->nField; fieldNumber++)
   {
      /* Check there is a valid channel access ID structure */

      if (pCaContext->pChannelId [fieldNumber] == NULL)
      {
         ERROR_SET2 (S_epToVxLib_INTERNAL_ERROR,
            "Null channel access ID for CA context %#x field %d",
            ERROR_LOG_SAVE, (int) pCaContext, fieldNumber);
         return (ERROR);
      }

      /*
       * BUG WORK AROUND:
       *
       * Check to see if the channel access ID structure has become corrupted.
       * The field name often gets overwritten, which means it will no longer
       * contain the string "hrwfs:".
       */

      if ( strstr( ca_name(pCaContext->pChannelId [fieldNumber]), "hrwfs:") 
                   == NULL )
      {

         ERROR_SET2 (S_epToVxLib_INTERNAL_ERROR,
                     "Channel access ID at %#x for field %d corrupted!!",
                     ERROR_LOG_SAVE, (int) pCaContext->pChannelId [fieldNumber], 
                     fieldNumber);
         return (ERROR);
      }

#ifdef DEBUG
      printf ("ca_array_put: type=%d, n=%d, chid=%#x, val=%s\n",
               (int) pCaContext->pFieldType [fieldNumber], 1,
               (int) pCaContext->pChannelId [fieldNumber],
               pCaContext->ppFieldValue [fieldNumber]);
#endif
      if (ca_array_put (pCaContext->pFieldType [fieldNumber], 1,
                        pCaContext->pChannelId [fieldNumber],
                        pCaContext->ppFieldValue [fieldNumber]) != ECA_NORMAL)
      {
         ERROR_SET1 (0, "Channel access array put failed (pCaContext=%#x)", 
                     ERROR_LOG_SAVE,
                     (int) pCaContext);
         ca_flush_io ();
         return (ERROR);
      }
   }

   /* Tidy up after channel-access routines */

   if (ca_flush_io () != ECA_NORMAL)
   {
      ERROR_SET (0, "Channel access flush failed", ERROR_LOG_SAVE);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */


/*+
 *   FUNCTION NAME:
 *   epToVxCaRead
 *
 *   INVOCATION:
 *   epToVxCaRead (pCaContext)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pCaContext (CA_DEF) channel-access definition structure pointer
 *
 *   FUNCTION VALUE:
 *   (STATUS) OK, or ERROR if channel-access to the record failed
 *
 *   PURPOSE:
 *   Do a channel-access read from an EPICS record
 *
 *   DESCRIPTION:
 *   This routine calls the channel access library routines ca_array_get()
 *   and ca_flush_io() to read from a named EPICS record. The pCaContext pointer
 *   is assumed to point to a valid CA_DEF structure and this is used to extract
 *   the record field values which are to be read from the record.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   The pCaContext structure must have been initialised via
 *   a previous call to an appropriate record-initialisation routine, e.g.
 *   epToVxCaInitCar or epToVxCaInitSir in the case of CAR or SIR records
 *   respectively.
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *   BUGS:
 *   I suspect this function suffers from the same bug as epToVxCaWrite 
 *   (with the Channel access ID structures becoming corrupted). The same bug 
 *   work-around is used. SMB - 8 Oct 1998.
 *
 *   The above bug has been avoided by not using this function. The CICS data 
 *   access functions are used instead.
 *   SMB - 27 Jan 1999.
 *-
 */

STATUS   epToVxCaRead
   (
   CA_DEF   pCaContext
   )
{
   FAST int      fieldNumber;

   /* Check the channel access definition structure provided */

   if (pCaContext == NULL)
   {
      ERROR_SET (S_epToVxLib_INTERNAL_ERROR, 
                 "Null channel access definition structure",
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * Read record fields in the order in which they are listed in the array 
    * pppWfsDbRecFieldName[][].
    */

   for (fieldNumber = 0; fieldNumber < pCaContext->nField; fieldNumber++)
   {
      /* Check there is a valid channel access ID structure */

      if (ca_name(pCaContext->pChannelId [fieldNumber]) == NULL)
      {
         ERROR_SET2 (S_epToVxLib_INTERNAL_ERROR, 
                     "Null channel access ID for context %#x field %d",
                     ERROR_LOG_SAVE, (int) pCaContext, fieldNumber);
         return (ERROR);
      }

      /*
       * BUG WORK AROUND:
       *
       * Check to see if the channel access ID structure has become corrupted.
       * The field name often gets overwritten, which means it will no longer
       * contain the string "hrwfs:".
       */

      if ( strstr( ca_name(pCaContext->pChannelId [fieldNumber]), "hrwfs:") 
           == NULL )
      {

         ERROR_SET2 (S_epToVxLib_INTERNAL_ERROR,
            "Channel access ID at %#x for field %d corrupted!!",
            ERROR_LOG_SAVE, (int) pCaContext->pChannelId [fieldNumber], 
            fieldNumber);
         return (ERROR);
      }

#ifdef DEBUG
      printf ("ca_array_get: type=%d, n=%d, chid=%#x, valptr=%#x\n",
               (int) pCaContext->pFieldType [fieldNumber], 1,
               (int) pCaContext->pChannelId [fieldNumber],
               (int) pCaContext->ppFieldValue [fieldNumber]);
#endif
      if (ca_array_get (pCaContext->pFieldType [fieldNumber], 1,
                        pCaContext->pChannelId [fieldNumber],
                        pCaContext->ppFieldValue [fieldNumber]) != ECA_NORMAL)
      {
         ERROR_SET1 (0, "Channel access array get failed (pCaContext=%#x)", 
                     ERROR_LOG_SAVE,
                     (int) pCaContext);
         ca_flush_io ();
         return (ERROR);
      }
   }

   /* Tidy up after channel-access routines */

   if (ca_pend_io (EPTOVX_TIMEOUT_CA) != ECA_NORMAL)
   {
      ERROR_SET (0, "Channel access pend failed", ERROR_LOG_SAVE);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */


/*+
 *   FUNCTION NAME:
 *   epToVxCaWriteDaemon
 *
 *   INVOCATION:
 *   epToVxCaWriteDaemon (nProcOnBus)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   nProcOnBus   (const int)   total number of CPUs on VME bus
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the daemon fails to initialise or
 *   whilst attempting to write to a record
 *
 *   PURPOSE:
 *   Daemon task services channel-access writes via the pipe interface
 *
 *   DESCRIPTION:
 *   This routine is executed as a daemon task on the CPU which holds the
 *   EPICS database in what is, in general, a multi-CPU environment. The
 *   task creates, opens and then assignes one pipe to each CPU on VME bus
 *   and copies data packets arriving from any of these pipes to a destination
 *   EPICS record. The related routine epToVxCaPipeWrite() is used to write
 *   data packets to the relevant pipe on a target CPU.
 *
 *   ERROR HANDLING:
 *   This routine will return ERROR if an error occurs during the initialisation
 *   phase. However once it starts processing response packets any errors will 
 *   be logged but the routine will try to keep running. Any errors are logged
 *   immediately via the error-logging library, errorLib.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   (>) pWfsDbSirList      (SIR_RECORD *) array defining the system's SIR 
 *                                         records
 *   (!) ppEpToVxCaDefTable (CA_DEF *)     table of channel-access definition 
 *                                         structures
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   At present this routine only works with SIR record types
 *
 *   BUGS:
 *   This task was using global variables pWfsDbSirList and ppEpToVxCaDefTable 
 *   without protecting any of them with a semaphore. SMB - 23 Mar 1998.
 *
 *   I have worked around some of the memory corruption bugs by using database 
 *   access when Channel Access fails. This is only a fudge. The way this 
 *   library uses EPICS facilities needs reviewing. SMB - 7 July 1998.
 *
 *   Andy Foster has suggested that he could fix the channel access memory 
 *   corruption problems seen here. SMB - 8 Oct 1998.
 *
 *   The bugs in this task have now been avoided by using the CICS database 
 *   access functions instead of Channel Access.
 *   SMB - 27 Jan 1999.
 *-
 */

STATUS   epToVxCaWriteDaemon
   (
   const int nProcOnBus
   )
{
   char      pNameExtension [4];      /* Pipe name extension.                 */
   FAST int  procNumber;              /* Processor number.                    */
   int       pRecWriteFd [NUM_FILES]; /* File descriptor array.               */
   int       widthSelect = 0;         /* Number of file descrs for select().  */
   int       nByte;                   /* Number of bytes read from pipe.      */
   int       recordType;              /* Record type.                         */
   int       recordNumber;            /* Record number.                       */
   uint32    recordDataType;          /* Record data type.                    */
   uint16    dbfDataType=DBF_STRING;  /* Database access type.                */
   char      pRecordName [EPICS_MAX_BYTES_RECORD_NAME + 1];
                                      /* Record name.                         */
   struct   fd_set readFds;           /* File descr. structure for select().  */
   struct {
      char pDataPacket [DATA_PKT_PIPE_WRITE_MAX_BYTES + DATA_PKT_PADDING_BYTES];
   } pContext;                        /* Pointer to data context structure.   */

   char      message [EPICS_MAX_BYTES_STRING_ATTRIB * 2];
                                      /* Error msg returned by database access*/

   /*
    * First initialise error-logging for this daemon.
    */

   if (errorInit () == ERROR)
   {
      printErr (
      "%s: epToVxCaWriteDaemon: Failed to initialise error context structure\n",
               taskName (taskIdSelf()));
      return (ERROR);
   }

   /*
    * Redirect error messages for this particular task to "standard error".
    * This prevents a potential conflict between this task and the message
    * logging task. The conflict scenario is:
    * 1) The epToVxCaWriteDaemon task encounters an error and issues an
    *    error message.
    * 2) The error message is directed to the message logging task.
    * 3) The message logging task sends a message to this epToVxCaWriteDaemon
    *    task requesting that an EPICS record containing the current error
    *    count be updated.
    * 4) The epToVxCaWriteDaemon task is not responding because of the
    *    error reported at step 1. The result is deadlock.
    * Preventing error messages from going to the message logging task
    * avoids this scenario.
    */

   errorTaskFdSet( ioGlobalStdGet(2) );         /* 2 means "stderr". */

   /*
    * Check that a valid number of processors has been given.
    */

   if ( (nProcOnBus <= 0) || (nProcOnBus > NUM_FILES) )
   {
      printErr (
      "%s: epToVxCaWriteDaemon: Invalid number of processors given, %d.\n",
      taskName (taskIdSelf()), nProcOnBus);
      return (ERROR);
   }

   /* Open one record-write pipe for each CPU on VME bus.
    * Unlike epToVxPipeFd, it is not necessary to protect these pipes with a 
    * semaphore because they are read-only and only accessed by this task.
    */

   for (procNumber = 0; procNumber < nProcOnBus; procNumber++)
   {
      sprintf (pNameExtension, "%.2d", procNumber);
      if ((pRecWriteFd [procNumber] = 
           epToVxPipeOpen (FALSE, CA_WRITE_PIPE_NAME, pNameExtension,
                           NULL, 0, 0, O_RDONLY, -1, DATA_PKT_TIMEOUT_OPENPIPE,
                           DATA_PKT_DELAY_OPENPIPE)) == ERROR)
      {
         ERROR_LOG ("Error opening EPICS record write pipe");
         return (ERROR);
      }
      if ((pRecWriteFd [procNumber] + 1) > widthSelect) 
         widthSelect = pRecWriteFd [procNumber] + 1;
   }

   /*
    * Issue an initial message.
    * NOTE: The MESSAGE_LOG macro cannot be used to display this message (as is 
    * done with most other tasks) because there is a potential for deadlock 
    * between this task and the message logging task. The deadlock scenario is:
    * 1) The epToVxCaWriteDaemon issues its "entering loop waiting for data" 
    *    message.
    * 2) The message is directed to the message logging task, and a response is 
    *    waited for.
    * 3) The message logging task is still initialising, and it tries to 
    *    initialise its EPICS records by sending a message to the 
    *    epToVxCaWriteDaemon task.
    * 4) The epToVxCaWriteDaemon task is not responding because it is waiting 
    *    to send its message. The result is deadlock.
    */

   printf ("%s: epToVxCaWriteDaemon: entering loop waiting for data.\n", 
           taskName (taskIdSelf()));
                                        /* Cannot use MESSAGE_LOG. See above. */


   FOREVER                   /* Infinite loop servicing record-write requests */
   {
      /*
       * There are nProcOnBus pipes to be read. Any one of these may have a 
       * data-packet written into it by a task running on one of the CPUs in the
       * system (or indeed this CPU might write to the pipe allocated for its 
       * use). The select library is used to pend on data in any one of these 
       * pipes.
       */

      FD_ZERO (& readFds);
      for (procNumber = 0; procNumber < nProcOnBus; procNumber++)
      {
         FD_SET (pRecWriteFd [procNumber], & readFds);
      }

      if (select (widthSelect, & readFds, NULL, NULL, NULL) == ERROR)
      {
         ERROR_SET (0, "File descriptor selection function, select(), failed", 
                    ERROR_LOG_NOW);
      }

      /* At this stage, the select library has detected data in one or more of 
       * the pipes 
       */

      for (procNumber = 0; procNumber < nProcOnBus; procNumber++)
      {

         /* Check if this pipe has been written to. */

         if (FD_ISSET (pRecWriteFd [procNumber], & readFds))
         {

            /* The pipe has been written to. Read the data packet   */

            nByte = read (pRecWriteFd [procNumber], pContext.pDataPacket,
                          DATA_PKT_PIPE_WRITE_MAX_BYTES);


            /*
             * Check that a sensible number of bytes have been read from the 
             * pipe.
             */

            if ( (nByte > 0) && (nByte <= DATA_PKT_PIPE_WRITE_MAX_BYTES) )
            {

               recordType = (DATA_PKT_RECORD_ID (& pContext) >> 16) & 0xffff;
               recordNumber = DATA_PKT_RECORD_ID (& pContext) & 0xffff;
#ifdef DEBUG
               printf ("epToVxCaWriteDaemon: Read %d bytes from pipe #%d\n", 
                       nByte, procNumber);
               printf ("epToVxCaWriteDaemon: "
                       "Rec Id = %#x Rec type = %d, Rec # = %d, Value @ %#x\n",
                       DATA_PKT_RECORD_ID (& pContext), recordType, 
                       recordNumber, (int) DATA_PKT_VALUE_PTR (& pContext));
#endif /* DEBUG */
               recordDataType = EPICS_DATA_TYPE_UNDEFINED;

               /* Set the recordDataType and record name based on the contents 
                * of the data-packet header word, recordType.
                */

               switch (recordType)
               {
                  case (SIR_RECORD_TYPE):

                     recordDataType = pWfsDbSirList [recordNumber].type;
                     strncpy (pRecordName, 
                              pWfsDbSirList [recordNumber].pRecordName,
                              EPICS_MAX_BYTES_RECORD_NAME);
                     break;

                  case (CAR_RECORD_TYPE):

                     ERROR_SET (S_epToVxLib_INVALID_RECORD_TYPE,
                                "Data packets for CAR records not supported",
                                ERROR_LOG_NOW);
                     break;

                  case (CAD_RECORD_TYPE):

                     ERROR_SET (S_epToVxLib_INVALID_RECORD_TYPE,
                                "Data packets for CAD records not supported",
                                ERROR_LOG_NOW);
                     break;

                  /* Add support for other record types here. */

                  default:

                     ERROR_SET (S_epToVxLib_INVALID_RECORD_TYPE,
                           "Data packet for unrecognised record type...ignored",
                           ERROR_LOG_NOW);
               }

               /* Get exclusive access to the channel access definition 
                * structures. 
                */

               if (semTake(epToVxCaDefSem, WAIT_FOREVER) == ERROR)
               {
                  ERROR_SET (0, "Could not take epToVxCaDefSem semaphore", 
                             ERROR_LOG_NOW);

                  /* If the semaphore can't be taken, skip the remaining code */
                  recordDataType = EPICS_DATA_TYPE_UNDEFINED;
               }

               /* Extract the expected data for this type of record from the 
                * data packet 
                */

               switch (recordDataType)
               {
                  case (EPICS_DATA_TYPE_LONG):

                     REC_FIELD_VALUE_LONG (ppEpToVxCaDefTable[recordType][recordNumber]) =
                        * (long *) (int) DATA_PKT_VALUE_PTR (& pContext);
                     dbfDataType = DBF_LONG;
                     break;

                  case (EPICS_DATA_TYPE_DOUBLE):

                     REC_FIELD_VALUE_DOUBLE (ppEpToVxCaDefTable[recordType][recordNumber]) =
                        * (double *) (int) DATA_PKT_VALUE_PTR (& pContext);
                     dbfDataType = 8; /* Pb with DBF_DOUBLE, should be 8 and is 6 */
                     break;

                  case (EPICS_DATA_TYPE_STRING):

                     strncpy (
                        REC_FIELD_VALUE_STRING (ppEpToVxCaDefTable[recordType][recordNumber]),
                        DATA_PKT_VALUE_PTR (& pContext), EPICS_MAX_BYTES_STRING_ATTRIB);
                     dbfDataType = DBF_STRING;
                     break;

                  default:

                     /* Ignore data if type is EPICS_DATA_TYPE_UNDEFINED */
                     break;
               }

               /* Finally, write the record data to the EPICS record */

               if (recordDataType != EPICS_DATA_TYPE_UNDEFINED)
               {

                  /*
                   * BUG WORK-AROUND.
                   * Don't use Channel Access. Use database access instead.
                   * (See the "BUGS" section above).
                   */

                  if ( cicsDbPut (pRecordName, message, dbfDataType,
                                  DATA_PKT_VALUE_PTR (& pContext))
                       == ERROR)
                  {
                     ERROR_SET2 (0, 
                        "Database access write to \"%s\" failed: %s",
                        ERROR_LOG_NOW, pRecordName, message);
                  }
               }

               /* Release the channel access definition structures. */

               semGive (epToVxCaDefSem);
            }
            else
            {
               /*
                * The number of bytes read from the pipe is either less than 
                * or equal to zero or greater than the packet size. Something 
                * has gone wrong.
                */

               ERROR_SET1 (S_epToVxLib_INVALID_PACKET_READ,
                  "Invalid number of bytes (%d) read from data packet pipe",
                  ERROR_LOG_NOW, nByte);
            }
         }
      }
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */


/*+
 *   FUNCTION NAME:
 *   epToVxCadInit
 *
 *   INVOCATION:
 *   epToVxCadInit (pcad)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   cadRecord   (struct cadRecord *)   pointer to CAD record structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the initialisation routine failed
 *
 *   PURPOSE:
 *   Initialisation routine for CAD record
 *
 *   DESCRIPTION:
 *   This is the initialisation routine for a CAD record. It is called when the
 *   record is initialised - normally when the IOC is initialised via iocInit().
 *   The routine is generic and re-entrant so that the same initialisation 
 *   routine is used for all CAD records. Each CAD record has between one and 
 *   two pipes associated with it: a "command pipe" always exists between the 
 *   CAD record and a VxWorks task (the "control task", which is responsible 
 *   for acting on each CAD command). A second pipe exists between the CAD 
 *   record and the CAR daemon task epToVxCarDaemon(). This second pipe is 
 *   used to set the CAR associated with a given CAD to the BUSY state on 
 *   receipt of a CAD START directive. Neither of these two pipes is created 
 *   by this routine.
 *
 *   ERROR HANDLING:
 *   Any errors are logged immediately via the error-logging library, errorLib.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   (!) epToVxSymtab         (SYMTAB_ID) symbol table used internally by 
 *                                        epToVxLib
 *   (!) pWfsDbRecInitialised (BOOL *)    array of record-initialisation-done 
 *                                        flags
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   This routine extracts the definition of a CAD record from the local 
 *   database (stored in the symbol table epToVxSymtab). This definition 
 *   includes the type of each CAD attribute - LONG, DOUBLE or STRING (char *).
 *   The routine does not attempt to check that these type definitions 
 *   correspond to those in the CAD fields pcad->ftva, pcad->ftvb through to 
 *   pcad->ftvt. Such a check is clearly possible; its inclusion is left as an 
 *   exercise for enthusiastic future users of epToVxLib.
 *
 *   This function assumes that the "cadRecord" data structure reserves a 
 *   contiguous area of memory for attributes "a", "b", "c", etc... If the 
 *   "cadRecord" structure changes so this is no longer true the function will 
 *   no longer work.
 *-
 */

STATUS   epToVxCadInit
   (
   struct cadRecord *   pcad
   )
{
   SYM_TYPE   symType;
   CAD_CONTEXT   context;

#ifdef DEBUG
   printf ("epToVxCadInit: %s\n", pcad->name);
#endif /* DEBUG */

   /*
    * It is assumed that the attribute strings for CAD attributes "a", "b", 
    * etc... are stored in contiguous areas of memory such that array 
    * arithmetic can be used to access any of these attribute strings (e.g. "d" 
    * is the 4th attribute, and each attribute occupies 
    * EPICS_MAX_BYTES_STRING_ATTRIB bytes of memory) rather than having to 
    * refer to each attribute by strucure member name ("->a", "->b" etc). The 
    * following check is performed to ensure that this is indeed the case. It 
    * probably isn't necessary, but this should trap any changes to the CAD 
    * record itself that may make this assumption invalid.
    */

   if (& pcad->a [EPICS_MAX_BYTES_STRING_ATTRIB] != pcad->b)
   {
      ERROR_SET (S_epToVxLib_INVALID_CAD_STRUCTURE, "structure \"cadRecord\" not contiguous",
         ERROR_LOG_NOW);
      return (ERROR);
   }

   /*
    * Initialise all CAD & CAR records in the local data base. taskLock () 
    * ensures that initialisation is performed only once
    */

   taskLock ();
   if (! pWfsDbRecInitialised [CAD_RECORD_TYPE])
   {
      if (epToVxDbInitCadCar () == ERROR)
      {
         ERROR_LOG (
             "Error initialising all CAD and CAR records in local database");
         taskUnlock ();
         return (ERROR);
      }
   }
   taskUnlock ();

   /* Extract CAD's context structure from symbol table */

   if (symFindByNameAndType (epToVxSymtab, pcad->name, (char **) & context, 
                             (SYM_TYPE *) & symType,
                             (SYM_TYPE) CAD_RECORD_TYPE, SYM_TYPE_MASK) 
       == ERROR)
   {
      ERROR_SET1 (0, "Could not find CAD \"%s\" in symbol table", 
                  ERROR_LOG_NOW, pcad->name);
      return (ERROR);
   }

#ifdef DEBUG
   /* epToVxShow (pcad->name, 0); */
#endif /* DEBUG */

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxCadExecute
 *
 *   INVOCATION:
 *   epToVxCadExecute (pcad)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   cadRecord   (struct cadRecord *)   pointer to CAD record structure
 *
 *   FUNCTION VALUE:
 *   (long)   CAD_ACCEPT or CAD_REJECT depending on whether the directive was
 *   accepted or rejected
 *
 *   PURPOSE:
 *   Execution routine for CAD record
 *
 *   DESCRIPTION:
 *   This is the execution routine for a CAD record. It is called whenever
 *   a CAD record receives a valid directive (e.g. MARK, PRESET, START etc).
 *   The routine is generic and re-entrant so that the same execution routine
 *   can be used for most CAD records. The CAD-specific operations are 
 *   determined from the local definition of the record contained in the array 
 *   external pWfsDbCadList[]. The following action is taken on receipt of 
 *   each directive.
 *
 *      MARK  => No action taken. Always returns CAD_ACCEPT.
 *
 *      CLEAR => Copy the default attribute values (defined in the
 *               declaration & initialisation of pWfsDbCadList[]) to the CAD's
 *               attribute inputs. Always returns CAD_ACCEPT.
 *
 *      PRESET=> Open pipe(s) between the CAD record and the associated
 *               control task and the CAR daemon task if these are not
 *               already open. Return CAD_REJECT if either pipe fails
 *               to open.
 *
 *      "     => Copy CAD attributes to local (static) storage and
 *               check each attribute value against any defined limits
 *               (given in the declaration & initialisation of pWfsDbCadList[]).
 *               Return CAD_REJECT if one or more attribute is out of
 *               range.
 *
 *      "     => Test whether the pipe used to write the CAD command
 *               and its attributes has an empty message slot. If the
 *               pipe is full, return CAD_REJECT otherwise CAD_ACCEPT.
 *
 *      START => Write a "START" command to the pipe(s) associated with this
 *               CAD record. Always returns CAD_ACCEPT.
 *
 *      STOP  => If the STOP directive is not supported (as defined in
 *               the declaration & initialisation of pWfsDbCadList[]), return
 *               CAD_REJECT. If STOP is supported, write a "STOP" command
 *               to the pipe(s) associated with the CAD record. Always returns
 *               CAD_ACCEPT.
 *
 *   As shown in the table, the START and STOP CAD directives cause "START" and
 *   "STOP" commands to be written to the pipe(s) associated with each CAD 
 *   record.
 *   The commands consist of data structures which are written to the pipe(s) as
 *   atomic operations - each data structure generally consists of a command 
 *   header and, in the case of a START directive, additional elements 
 *   containing CAD attributes converted from strings to their native types. 
 *   The command header contains information such as the CAD directive, the 
 *   simulation mode, the client ID and details of any attributes which follow. 
 *   Any attributes that match the defined default value are not written as 
 *   part of a command packet but are inserted at the receiving task. In the 
 *   event that the CAD record is responsible for setting the associated CAR 
 *   BUSY on receipt of a START, directive only the command header is written 
 *   to the CAR daemon.
 *
 *   ERROR HANDLING:
 *   Any errors are logged immediately via the error-logging library, errorLib.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   (>) epToVxSymtab         (SYMTAB_ID)   symbol table used internally by 
 *                                          epToVxLib
 *   (!) epToVxCadSimMode     (int)         CAD simulation mode
 *   (>) pWfsDbRecInitialised (BOOL *)      array of record-initialisation-done 
 *                                          flags
 *   (!) (context)            (CAD_CONTEXT) Not strictly a global variable, but 
 *                                          obtained from the epToVxSymtab 
 *                                          symbol table and therefore a
 *                                          globally accessible data structure.
 *
 *   PRIOR REQUIREMENTS:
 *   The symbol table and data structures should have been initialised with
 *   epToVxCadInit.
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   Because this is a general purpose routine designed to be used with any CAD
 *   command, it is hard for the CAD record to make other than simple range
 *   checks on its inputs. For example, it would be useful if a CAD had the
 *   ability to reject a command if, say, attribute B was smaller than A.
 *   SMB - 2 Feb 1998.
 *
 *   DESIGN FLAWS:
 *   The CAR record corresponding to a command should not go BUSY when a STOP
 *   directive is received. The STOP directive is supposed to stop the action
 *   started by the START directive and make the CAR record go IDLE. The
 *   protocol between the CAD record and CAR daemon needs redesigning.
 *   SMB - 20 Feb 1998.
 *-
 */

long   epToVxCadExecute

   (
   struct cadRecord *   pcad
   )
{
   long        returnValue = CAD_ACCEPT;
   uint32      offendingAttrib;     /* ID of rejected attribute.              */
   char        pReason[(EPICS_MAX_BYTES_STRING_ATTRIB+1) * 2];
                                    /* String to hold reason for rejection.   */
   char        pMessage[(EPICS_MAX_BYTES_STRING_ATTRIB+1) * 2];
                                    /* Intermediate string to guard against   */
                                    /* memory corruption (twice as long as    */
                                    /* it needs to be).                       */
   SYM_TYPE    symType;
   CAD_CONTEXT context;
   int         nByte;
   int         status;
   int         cadSimModeUsed;

#ifdef DEBUG
   printf ("epToVxCadExecute: %s %d\n", pcad->name, pcad->dir);
#endif /* DEBUG */

   /*
    * Check that the CAD has been initialised. If it has, look up its context 
    * structure in the local database
    */

   if (! pWfsDbRecInitialised [CAD_RECORD_TYPE])
   {
      strncpy (pcad->mess, "HRWFS:CAD not initialised", 
               EPICS_MAX_BYTES_STRING_ATTRIB);
      ERROR_SET1 (S_epToVxLib_RECORD_UNINITIALISED, 
                  "CAD record \"%s\" not initialised",
                  ERROR_LOG_NOW, pcad->name);
      returnValue = CAD_REJECT;
   }
   else if (symFindByNameAndType (epToVxSymtab, pcad->name, (char **) & context,
            (SYM_TYPE *) & symType, (SYM_TYPE) CAD_RECORD_TYPE, SYM_TYPE_MASK) 
            == ERROR)
   {
      strncpy (pcad->mess, "HRWFS:CAD not implemented", 
               EPICS_MAX_BYTES_STRING_ATTRIB);
      ERROR_SET1 (0, "Could not find CAD \"%s\" in symbol table", 
                  ERROR_LOG_NOW, pcad->name);
      returnValue = CAD_REJECT;
   }

   /* Reject if the CAD is not known. */

   if (returnValue == CAD_REJECT)
   {
      return (CAD_REJECT);
   }

   switch (pcad->dir)
   {
      case menuDirectiveMARK:

         /* MARK is always accepted. */

         /*printf ( "MARK directive on the %s\n" , pcad->name ) ;*/
         returnValue = CAD_ACCEPT;
         break;

      case menuDirectiveCLEAR:

         /*
          * CLEAR causes default attribute values to be loaded into the 
          * attribute string inputs and is always accepted.
          */

         /*printf ( "CLEAR directive on the %s\n" , pcad->name ) ;*/
         eptovx_loadDefaultInputAttribs (context, pcad);
         eptovx_postEventsInputAttribs (context, pcad);   
                                         /* This is needed for the defaults   */
                                         /* to appear on a "dm" screen.       */
         returnValue = CAD_ACCEPT;
         break;

      case menuDirectivePRESET:

         /* If command pipe has not yet been opened, attempt to open it */

         /*printf ( "PRESET directive on the %s\n" , pcad->name ) ;*/
         if (context->cadToTaskPipeFd == ERROR)
         {
            if ((context->cadToTaskPipeFd = 
                epToVxPipeOpen (FALSE, context->pTaskName,
                CAD_TO_TASK_PIPE_NAME_EXT, NULL, 0, 0, O_WRONLY, -1, 0.0, 0.0)) 
                == ERROR)
            {
               strncpy (pcad->mess, "HRWFS:Error opening CAD to task pipe",
                        EPICS_MAX_BYTES_STRING_ATTRIB);
               ERROR_SET1 (0, "Error opening CAD to task pipe for %s", 
                           ERROR_LOG_NOW, pcad->name);
               returnValue = CAD_REJECT;
               break;
            }
         }

         /*
          * If CAD to CAR pipe not yet been opened (and if the protocol requires
          * that the CAD writes to this pipe), attempt to open it
          */

         if (context->cadToCarPipeFd == ERROR)
         {
            if ((context->cadToCarPipeFd = 
               epToVxPipeOpen (FALSE, context->pTaskName,
               CAD_TO_CAR_PIPE_NAME_EXT, NULL, 0, 0, O_WRONLY, FIOFLUSH, 
               0.0, 0.0)) == ERROR)
            {
               strncpy (pcad->mess, "HRWFS:Error opening CAD to CAR pipe",
                        EPICS_MAX_BYTES_STRING_ATTRIB);
               ERROR_SET1 (0, "Error opening CAD to CAR pipe for %s", 
                           ERROR_LOG_NOW, pcad->name);
               returnValue = CAD_REJECT;
               break;
            }
         }

         /*
          * Now copy the attributes from the CAD record into the CAD's context 
          * structure, then check that each attribute is within any defined 
          * ranges.
          * If a message of unknown length is written to the pcad->mess field, 
          * the intermediate string pMessage is used to ensure there isn't any 
          * accidental memory corruption.
          */

         if (eptovx_saveAttribs (pcad, context, & offendingAttrib) == ERROR)
         {
            sprintf (pMessage, "HRWFS:Attribute #%d %.*s", (int)offendingAttrib,
                     EPICS_MAX_BYTES_STRING_ATTRIB, "conversion failure");
            strncpy (pcad->mess, pMessage, EPICS_MAX_BYTES_STRING_ATTRIB);
            returnValue = CAD_REJECT;
         }
         else if (eptovx_checkAttribs (context, & offendingAttrib, pReason) 
                  == ERROR)
         {
            sprintf (pMessage, "HRWFS:Attribute #%d %.*s", (int)offendingAttrib,
                     EPICS_MAX_BYTES_STRING_ATTRIB, pReason);
            strncpy (pcad->mess, pMessage, EPICS_MAX_BYTES_STRING_ATTRIB);
            returnValue = CAD_REJECT;
         }
         else if (! eptovx_readyToAcceptCmd (context))   
                                     /* Is the CAD command pipe ready to      */
                                     /* be written with a command packet ?    */
         {
            /*
             * eptovx_readyToAcceptCmd() may return FALSE for two possible 
             * reasons:
             * (i) The routine failed due to an error getting the status of 
             * the command pipe;
             * in this case ERROR_SET() will have been called by the routine.
             * (ii) The command pipe is full, this is not regarded as an error 
             * condition (e.g. very rapid execution of CAD commands may cause 
             * the pipe to fill temporarily, but after a brief delay the pipe 
             * should get emptied by the control task); in this case 
             * ERROR_SET() will not have been called by the routine and
             * the following ERROR_LOG() should simply discard the error message
             */

            /*
             * NOTE: The reasoning in the above comment seems dodgy. By the 
             * time the code reaches this point the CAD is about to reject the 
             * command anyway, so why throw away the error message. I think 
             * the logic of this part of the code needs rethinking. Can we not 
             * wait for the command packet to become available and report an 
             * error if a timeout occurs? SMB - 8 Oct 1998.
             */

            ERROR_LOG ("CAD command pipe not ready to write");
            strncpy (pcad->mess, "HRWFS:Can't write CAD command pipe", 
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            returnValue = CAD_REJECT; /* Pipe isn't ready, reject this PRESET */
         }
         else
         {

            /* All OK.
             * Copy the attribute values to the CAD's outputs.
             */

            /*
             * I have commented out the following call because it looked like
             * it was doing something really silly. It isn't really necessary
             * anyway, since in the present design the CAD output attributes 
             * are a dead end. SMB - 16 Mar 1998.
             *
             * Further note: I have since written my own functions to copy CAD
             * attributes - epToVxCadCopy and eptovx_copyAttrib. 
             * SMB - 14 Oct 1998.
             */

            /* eptovx_writeCadOutAttribs (context, pcad); */
            returnValue = CAD_ACCEPT;       /* Accept this PRESET directive   */
         }
         break;

      case menuDirectiveSTOP:

         /*printf ( "STOP directive on the %s\n" , pcad->name ) ;*/
         if (! context->stopDirSupported) /* Fall through to menuDirectiveSTART if STOP*/
                                          /* is supported, otherwise, REJECT  */
                                          /* this STOP directive (this is an  */
                                          /* error condition)                 */
         {
            ERROR_SET1 (S_epToVxLib_menuDirectiveSTOP_UNSUPPORTED, 
                        "STOP directive not supported by %s",
                        ERROR_LOG_NOW, pcad->name);
            strncpy (pcad->mess, "HRWFS:STOP directive not supported", 
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            returnValue = CAD_REJECT;
            break;
         }

         /*
          * It is possible for a STOP directive to be issued before a PRESET 
          * or START directive, in which case no command pipe will have been 
          * opened. Trap this case and reject the STOP directive.
          */

         if (context->cadToTaskPipeFd == ERROR)
         {
            strncpy (pcad->mess, "HRWFS:No PRESET or START", 
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            ERROR_SET (S_epToVxLib_menuDirectiveSTOP_UNSUPPORTED, 
                       "No PRESET or START issued first", ERROR_LOG_NOW);
            returnValue = CAD_REJECT;
            break;
         }

      case menuDirectiveSTART:

         /*
          * START directive received.
          * Increment the client ID, wrapping the value if the number exceeds
          * CAD_MAX_TRANSACTION_NUMBER.
          */

         /*printf ( "START directive on the %s\n" , pcad->name ) ;*/
         if (context->clientId == CAD_MAX_TRANSACTION_NUMBER)
         {
            context->clientId = 0;
         }
         else
         {
            context->clientId++;
         }

         CMD_PKT_CLIENT_ID (context) = context->clientId;

         if (context->simulationSupported)
         {
            cadSimModeUsed = epToVxCadSimMode;   /* Get the simulation mode   */
         }
         else
         {
            cadSimModeUsed = EPTOVX_SIM_MODE_NONE; 
                                                 /* (disallow if not enabled) */
         }


         /*
          * At this stage, we could have received either a START or STOP 
          * directive.
          * Each is handled in a similar manner, the only differences being 
          * that the command-packet's "command-modifier" word has a START or 
          * STOP bit within it and this is set accordingly. Also, if this is a 
          * STOP directive then the attributes are not needed so that only the 
          * command header is written to the command pipe.
          */

         if (pcad->dir == menuDirectiveSTART)
         {
            nByte = context->sizeOfCmdPacket;
            CMD_PKT_COMMAND_MODIFIER (context) = 
            cadSimModeUsed | CAD_COMMAND_MODE_BEGIN | CAD_DIRECTIVE_START;
         }
         else
         {
            nByte = CMD_PKT_HEADER_SIZE_BYTES;
            CMD_PKT_COMMAND_MODIFIER (context) = 
            cadSimModeUsed | CAD_COMMAND_MODE_BEGIN | CAD_DIRECTIVE_STOP;
         }

         /*
          * Send a message to the CAR daemon in order to set the CAR to the 
          * BUSY state.
          *
          * NOTE: There is a mistake in the protocol here. This should not 
          * happen when a STOP directive is received. The system needs 
          * redesigning. SMB - 16 Mar 1998.
          */

         if (write (context->cadToCarPipeFd, context->pCmdPacket, 
                    CMD_PKT_HEADER_SIZE_BYTES) != CMD_PKT_HEADER_SIZE_BYTES)
         {
            strncpy (pcad->mess, "HRWFS:Error writing CAD to CAR pipe", 
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            ERROR_SET (0, "Error writing to CAD to CAR pipe", ERROR_LOG_NOW);
            returnValue = CAD_REJECT;
            break;
         }
#ifdef DEBUG
         printf ("epToVxCadExecute: CAD has set CAR busy\n");
#endif /* DEBUG */

         /*
          * Write the command packet to the control task, but only do this 
          * when the simulation mode is FAST, FULL or NONE (i.e. not VSM).
          *
          * In VSM simulation mode, the control task does not receive a CAD 
          * command packet, since VSM mode is handled entirely by this routine 
          * and by the CAR daemon.
          */

         if (cadSimModeUsed != EPTOVX_SIM_MODE_VSM)
         {
            status = write (context->cadToTaskPipeFd, context->pCmdPacket, 
                            nByte);
            if (status != nByte)
            {
               strncpy (pcad->mess, "HRWFS:Error writing CAD command pipe",
                        EPICS_MAX_BYTES_STRING_ATTRIB);
               ERROR_SET (0, "Error writing to CAD command pipe", 
                          ERROR_LOG_NOW);
               returnValue = CAD_REJECT;
               break;
            }
         }

#ifdef DEBUG
         if (pcad->dir == menuDirectiveSTART)
         {
            printf ("epToVxCadExecute: START accepted with client ID = %#x\n", 
                    context->clientId);
         }
         else
         {
            printf ("epToVxCadExecute: STOP accepted with client ID = %#x\n", 
                    context->clientId);
         }
#endif /* DEBUG */

         returnValue = CAD_ACCEPT;
         break;

      default:

         /* Invalid directive received   */

         strncpy (pcad->mess, "HRWFS:Invalid CAD directive", 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         ERROR_SET (S_epToVxLib_INVALID_CAD_DIRECTIVE, "Invalid CAD directive",                     ERROR_LOG_NOW);
         returnValue = CAD_REJECT;
   }

   /* All done. Call post events to update the output attributes. */

   /* I have commented out the following call because it looked like it was
    * doing something really silly. I am not sure if calling db_post_events()
    * is necessary anyway, as the CAD record support should do this anyway.
    * SMB - 17 Mar 1998.
    */

   /* eptovx_postEventsOutputAttribs (context, pcad);   */

   return (returnValue);
}

/* -------------------------------------------------------------------------- */


/*+
 *   FUNCTION NAME:
 *   epToVxCadCopy
 *
 *   INVOCATION:
 *   epToVxCadCopy (pcad)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   cadRecord   (struct cadRecord *)   pointer to CAD record structure
 *
 *   FUNCTION VALUE:
 *   (long)   CAD_ACCEPT or CAD_REJECT depending on whether the directive was
 *   accepted or rejected
 *
 *   PURPOSE:
 *   Execution routine for CAD record which simply copies its inputs to its 
 *   outputs
 *
 *   DESCRIPTION:
 *   This is the execution routine for a CAD record representing a command 
 *   implemented purely using EPICS database records. It will do nothing 
 *   except copy the input attributes; A, B, C etc... to its outputs; VALA, 
 *   VALB, VALC etc... It will not check the attributes for validity. If such 
 *   a validity check is required, then an application should supply its own 
 *   version of this function. A CAD record which uses this function does not 
 *   need to be declared in the array of pWfsDbCadList[] structures. The 
 *   following action is taken on receipt of each directive.
 *
 *      MARK    =>   No action taken. Always returns CAD_ACCEPT.
 *
 *      CLEAR   =>   No action taken. Always returns CAD_ACCEPT.
 *
 *      PRESET  =>   Copies inputs to outputs. Returns CAD_ACCEPT if successful.
 *
 *      START   =>   No action taken. Always returns CAD_ACCEPT.
 *
 *      STOP    =>   Cannot be stopped. Always returns CAD_REJECT.
 *
 *   It is assumed the response through a CAR record will be handled elsewhere.
 *
 *   ERROR HANDLING:
 *   Any errors are logged immediately via the error-logging library, errorLib.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

long   epToVxCadCopy
   (
   struct cadRecord *   pcad
   )
{
   long      returnValue = CAD_ACCEPT;

#ifdef DEBUG
   printf ("epToVxCadReject: %s\n", pcad->name);
#endif /* DEBUG */

   /* Switch according to the CAD directive. */

   switch (pcad->dir)
   {
      case menuDirectiveMARK:

         /* MARK is always accepted. */

         returnValue = CAD_ACCEPT;
         break;

      case menuDirectiveCLEAR:

         /* CLEAR is always accepted. */

         returnValue = CAD_ACCEPT;
         break;

      case menuDirectivePRESET:

         /*
          * PRESET - Copy each CAD attribute from the input to the output.
          *          Reject the directive if there is a conversion failure.
          */

         returnValue = CAD_ACCEPT;      /* Default return value. */

         /* Attributes A and B are always copied. */

         if (eptovx_copyAttrib ('A', pcad->a, (int) pcad->ftva, pcad->vala, 
                                pcad->mess) == ERROR)
         {
            returnValue = CAD_REJECT;
         }
         else if (eptovx_copyAttrib ('B', pcad->b, (int) pcad->ftvb, 
                                     pcad->valb, pcad->mess)
                  == ERROR)
         {
            returnValue = CAD_REJECT;
         }

         /* Attributes C and D are copied if this is CAD is larger than a CAD2*/

         if ( (returnValue != CAD_REJECT) && (pcad->ctyp > 2) )
         {
            if (eptovx_copyAttrib ('C', pcad->c, (int) pcad->ftvc, pcad->valc, 
                                   pcad->mess)
                == ERROR)
            {
               returnValue = CAD_REJECT;
            }
            else if (eptovx_copyAttrib ('D', pcad->d, (int) pcad->ftvd, 
                                        pcad->vald, pcad->mess)
                     == ERROR)
            {
               returnValue = CAD_REJECT;
            }
         }
         /* Attributes E, F, G and H are copied if this is CAD is larger than 
          * a CAD4. 
          */

         if ( (returnValue != CAD_REJECT) && (pcad->ctyp > 4) )
         {
            if (eptovx_copyAttrib ('E', pcad->e, (int) pcad->ftve, 
                                   pcad->vale, pcad->mess)
                == ERROR)
            {
               returnValue = CAD_REJECT;
            }
            else if (eptovx_copyAttrib ('F', pcad->f, (int) pcad->ftvf, 
                                         pcad->valf, pcad->mess)
                     == ERROR)
            {
               returnValue = CAD_REJECT;
            }
            else if (eptovx_copyAttrib ('G', pcad->g, (int) pcad->ftvg, 
                                        pcad->valg, pcad->mess)
                     == ERROR)
            {
               returnValue = CAD_REJECT;
            }
            else if (eptovx_copyAttrib ('H', pcad->h, (int) pcad->ftvh, 
                                        pcad->valh, pcad->mess)
                == ERROR)
            {
               returnValue = CAD_REJECT;
            }
         }

         /*
          * Attributes I, J, K, L, M, N, O, P, Q, R, S and T are copied
          * if this is CAD is larger than a CAD8, i.e. it is a CAD20.
          */

         if ( (returnValue != CAD_REJECT) && (pcad->ctyp > 8) )
         {
            if (eptovx_copyAttrib ('I', pcad->i, (int) pcad->ftvi, 
                                   pcad->vali, pcad->mess)
                == ERROR)
            {
               returnValue = CAD_REJECT;
            }
            else if (eptovx_copyAttrib ('J', pcad->j, (int) pcad->ftvj, 
                                        pcad->valj, pcad->mess)
                     == ERROR)
            {
               returnValue = CAD_REJECT;
               break;
            }
            else if (eptovx_copyAttrib ('K', pcad->k, (int) pcad->ftvk, 
                                        pcad->valk, pcad->mess)
                     == ERROR)
            {
               returnValue = CAD_REJECT;
            }
            else if (eptovx_copyAttrib ('L', pcad->l, (int) pcad->ftvl, 
                                        pcad->vall, pcad->mess)
                     == ERROR)
            {
               returnValue = CAD_REJECT;
            }
            else if (eptovx_copyAttrib ('M', pcad->m, (int) pcad->ftvm, 
                                        pcad->valm, pcad->mess)
                     == ERROR)
            {
               returnValue = CAD_REJECT;
            }
            else if (eptovx_copyAttrib ('N', pcad->n, (int) pcad->ftvn, 
                                        pcad->valn, pcad->mess)
                     == ERROR)
            {
               returnValue = CAD_REJECT;
            }
            else if (eptovx_copyAttrib ('O', pcad->o, (int) pcad->ftvo, 
                                        pcad->valo, pcad->mess)
                     == ERROR)
            {
               returnValue = CAD_REJECT;
            }
            else if (eptovx_copyAttrib ('P', pcad->p, (int) pcad->ftvp, 
                                        pcad->valp, pcad->mess)
                     == ERROR)
            {
               returnValue = CAD_REJECT;
            }
            else if (eptovx_copyAttrib ('Q', pcad->q, (int) pcad->ftvq, 
                                        pcad->valq, pcad->mess)
                     == ERROR)
            {
               returnValue = CAD_REJECT;
            }
            else if (eptovx_copyAttrib ('R', pcad->r, (int) pcad->ftvr, 
                                        pcad->valr, pcad->mess)
                     == ERROR)
            {
               returnValue = CAD_REJECT;
            }
            else if (eptovx_copyAttrib ('S', pcad->s, (int) pcad->ftvs, 
                                        pcad->vals, pcad->mess)
                     == ERROR)
            {
               returnValue = CAD_REJECT;
            }
            else if (eptovx_copyAttrib ('T', pcad->t, (int) pcad->ftvt, 
                                        pcad->valt, pcad->mess)
                     == ERROR)
            {
               returnValue = CAD_REJECT;
            }
         }

         if ( returnValue == CAD_REJECT )
         {
            ERROR_LOG ("Failed to copy attributes");
         }

         break;

      case menuDirectiveSTART:

         /* START is always accepted. */

         returnValue = CAD_ACCEPT;
         break;

      case menuDirectiveSTOP:

         /* Reject a STOP with an explanatory message. */

         strncpy (pcad->mess, "HRWFS:Cannot be stopped", 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         ERROR_SET1 (S_epToVxLib_menuDirectiveSTOP_UNSUPPORTED, "%s - cannot be stopped",
                     ERROR_LOG_NOW, pcad->name);
         returnValue = CAD_REJECT;
         break;

      default:

         /* Invalid directive received   */

         strncpy (pcad->mess, "HRWFS:Invalid CAD directive", 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         ERROR_SET (S_epToVxLib_INVALID_CAD_DIRECTIVE, 
                    "Invalid CAD directive", ERROR_LOG_NOW);
         returnValue = CAD_REJECT;
   }

   return (returnValue);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxCadReject
 *
 *   INVOCATION:
 *   epToVxCadReject (pcad)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   cadRecord   (struct cadRecord *)   pointer to CAD record structure
 *
 *   FUNCTION VALUE:
 *   (long)   CAD_ACCEPT or CAD_REJECT depending on whether the directive was
 *   accepted or rejected
 *
 *   PURPOSE:
 *   Execution routine for unsupported CAD record
 *
 *   DESCRIPTION:
 *   This is the execution routine for a CAD record representing an unsupported
 *   command. It will reject any attempt to execute that command. A CAD record
 *   which uses this function does not need to be declared in the array of
 *   pWfsDbCadList[] structures. The following action is taken on receipt of 
 *   each directive.
 *
 *      MARK    =>   No action taken. Always returns CAD_ACCEPT.
 *
 *      CLEAR   =>   No action taken. Always returns CAD_ACCEPT.
 *
 *      PRESET  =>   Always returns CAD_REJECT.
 *
 *      START   =>   Always returns CAD_REJECT.
 *
 *      STOP    =>   Always returns CAD_REJECT.
 *
 *   Because no action is ever started by an unsupported command, there is no
 *   response through a CAR record.
 *
 *   ERROR HANDLING:
 *   Any errors are logged immediately via the error-logging library, errorLib.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

long   epToVxCadReject
   (
   struct cadRecord *   pcad
   )
{
   long      returnValue = CAD_ACCEPT;

#ifdef DEBUG
   printf ("epToVxCadReject: %s\n", pcad->name);
#endif /* DEBUG */

   /* Switch according to the CAD directive. */

   switch (pcad->dir)
   {
      case menuDirectiveMARK:

         /* MARK is always accepted. */

         returnValue = CAD_ACCEPT;
         break;

      case menuDirectiveCLEAR:

         /* CLEAR is always accepted. */

         returnValue = CAD_ACCEPT;
         break;

      case menuDirectivePRESET:
      case menuDirectiveSTART:
      case menuDirectiveSTOP:

         /* Reject a PRESET, START or STOP with an explanatory message. */

         strncpy (pcad->mess, "HRWFS:Command not supported", 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         ERROR_SET1 (S_epToVxLib_CAD_CMD_UNSUPPORTED, 
             "%s - command not supported",
             ERROR_LOG_NOW, pcad->name);
         returnValue = CAD_REJECT;
         break;

      default:

         /* Invalid directive received   */

         strncpy (pcad->mess, "HRWFS:Invalid CAD directive", 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         ERROR_SET (S_epToVxLib_INVALID_CAD_DIRECTIVE, 
                    "Invalid CAD directive", ERROR_LOG_NOW);
         returnValue = CAD_REJECT;
   }

   return (returnValue);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxGensubInit
 *
 *   INVOCATION:
 *   epToVxGensubInit (pgensub)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pgensub (struct genSubRecord *)   pointer to genSub record structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the initialisation routine failed
 *
 *   PURPOSE:
 *   Initialisation routine for genSub record
 *
 *   DESCRIPTION:
 *   This is the init routine for a genSub record. It is called when the
 *   record is initialised - normally when the IOC is initialised via iocInit().
 *   The routine is generic and re-entrant so that the same init routine
 *   is used for all genSub records.
 *
 *   ERROR HANDLING:
 *   Any errors are logged immediately via the error-logging library, errorLib.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   (!) epToVxSymtab         (SYMTAB_ID) symbol table used internally by 
 *                                        epToVxLib
 *   (!) pWfsDbRecInitialised (BOOL *)    array of record-initialisation-done 
 *                                        flags
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   epToVxGensubInit
   (
   struct genSubRecord *   pgensub 
   )
{
   SYM_TYPE      symType;
   GSUB_CONTEXT   context;

#ifdef DEBUG
   printf ("epToVxGensubInit: %s\n", pgensub->name);
#endif /* DEBUG */

   /*
    * Initialise all genSub records in the local data base. taskLock () 
    * ensures that initialisation is performed only once
    */

   taskLock ();
   if (! pWfsDbRecInitialised [GENSUB_RECORD_TYPE])
   {
      if (epToVxDbInitGensub () == ERROR)
      {
         ERROR_LOG ("Error initialising all genSub records in local database");
         taskUnlock ();
         return (ERROR);
      }
   }
   taskUnlock ();

   /* Extract the genSub's context structure from the symbol table */

   if (symFindByNameAndType (epToVxSymtab, pgensub->name, (char **) & context,
       (SYM_TYPE *) & symType, (SYM_TYPE) GENSUB_RECORD_TYPE, SYM_TYPE_MASK) 
       == ERROR)
   {
      ERROR_SET1 (0, "Could not find genSub \"%s\" in symbol table", 
                  ERROR_LOG_NOW, pgensub->name);
      return (ERROR);
   }

   /*
    * Check that the number of values specified matches that defined in the 
    * context structure. There are NOJ input values and NOVJ output values.
    */

   if ( context->inputRecord )
   {
      if ( context->nValues != (uint32) pgensub->noj )
      {
         ERROR_SET3 (S_epToVxLib_RECORD_DEFINITION_ERROR,
                     "genSub %s: Number of values mismatch: %d %d",
                     ERROR_LOG_NOW, pgensub->name, (int)context->nValues, 
                     (int)pgensub->noj);
         return (ERROR);
      }
   }
   else
   {
      if ( context->nValues != (uint32) pgensub->novj )
      {
         ERROR_SET3 (S_epToVxLib_RECORD_DEFINITION_ERROR,
            "genSub %s: Number of values mismatch: %d %d",
            ERROR_LOG_NOW, pgensub->name, (int)context->nValues, 
            (int)pgensub->novj);
         return (ERROR);
      }
      else if ( pgensub->noj != pgensub->novj )
      {
         ERROR_SET3 (S_epToVxLib_RECORD_DEFINITION_ERROR,
            "genSub %s: Different number of inputs (%d) and outputs (%d)",
            ERROR_LOG_NOW, pgensub->name, (int)pgensub->noj, 
            (int)pgensub->novj);
         return (ERROR);
      }
   }

#ifdef DEBUG
   /* epToVxShow (pgensub->name, 0); */
#endif /* DEBUG */

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxGensubInput
 *
 *   INVOCATION:
 *   epToVxGensubInput (pgensub)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pgensub (struct genSubRecord *) pointer to genSub record structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the routine failed
 *
 *   PURPOSE:
 *   Execution routine for input genSub record
 *
 *   DESCRIPTION:
 *   This is the execution routine for a genSub record used for data input.
 *
 *   ERROR HANDLING:
 *   Any errors are logged immediately via the error-logging library, errorLib.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   I am not sure how efficient it will be to send data updates at 20Hz. If 
 *   we switch from multi-processor pipes to VxMP I would communicate these 
 *   data updates via shared memory instead. SMB - 14 May 1998.
 *
 *   BUGS:
 *   The number of values stored in the .NOJ and .NOVJ fields can become 
 *   corrupted.
 *   As a work around, these fields are examined when the record is first 
 *   initialised (in epToVxGensubInit) and then not looked at again. 
 *   SMB - 27 Apr 1998.
 *-
 */

STATUS   epToVxGensubInput
   (
   struct genSubRecord *   pgensub
   )
{
   SYM_TYPE     symType;
   GSUB_CONTEXT context;
   char         pName [EPICS_MAX_BYTES_RECORD_NAME];
                             /* Name of input pipe.                           */
   uint32       nValues;     /* Number of input values.                       */
   int          nByte;       /* Number of bytes.                              */
   int          status;      /* Status.                                       */
   double *     inputArray;  /* Pointer acting as input array of double values*/
   double *     outputArray; /* Pointer acting as output array of double      */
                             /* values.                                       */
   FAST int     i;           /* Element number.                               */

#ifdef DEBUG
   printf ("epToVxGensubInput: %s\n", pgensub->name);
#endif /* DEBUG */

   /*
    * Check that the genSub has been initialised. If it has, look up its context
    * structure in the local database.
    */

   if (! pWfsDbRecInitialised [GENSUB_RECORD_TYPE])
   {
      ERROR_SET1 (S_epToVxLib_RECORD_UNINITIALISED, 
         "genSub record \"%s\" not initialised",
         ERROR_LOG_NOW, pgensub->name);
      return (ERROR);
   }
   else if (symFindByNameAndType (epToVxSymtab, pgensub->name, 
            (char **) & context,
            (SYM_TYPE *) & symType, (SYM_TYPE) GENSUB_RECORD_TYPE, 
            SYM_TYPE_MASK) == ERROR)
   {
      ERROR_SET1 (0, "Could not find genSub \"%s\" in symbol table", 
                  ERROR_LOG_NOW, pgensub->name);
      return (ERROR);
   }

   /*
    * Find out how many values this genSub record has. This number has already 
    * been verified to be sensible in epToVxGensubInit.
    */

   nValues = context->nValues;

   /*
    * If the genSub input pipe has not yet been opened, attempt to open it; 
    * constructing the pipe name from a combination of wavefront sensor name 
    * and task name, followed by the GSUB_TO_TASK_PIPE_NAME_EXT string.
    */

   sprintf (pName, "%.8s:%.30s", context->pWfsName, context->pTaskName);

   if (context->gensubToTaskPipeFd == ERROR)
   {
      if ((context->gensubToTaskPipeFd = epToVxPipeOpen (FALSE, pName,
           GSUB_TO_TASK_PIPE_NAME_EXT, NULL, 0, 0, O_WRONLY, -1, 0.0, 0.0)) 
           == ERROR)
      {
         ERROR_SET (0, "Error opening genSub to task pipe", ERROR_LOG_NOW);
         return (ERROR);
      }
   }

   /*
    * Define the input array pointer to point to the J input values in the 
    * pgensub structure. Using this pointer will access the J structure as if 
    * it were an array of double values.
    */

   inputArray = (double *)pgensub->j;

   /* In debug mode, list the values obtained. */

#ifdef DEBUG
   printf ("Gensub input:");
   for ( i = 0; i < (int)nValues; i++ )
   {
      printf (" J[%d] = %f", i, inputArray[i] );
   }
   printf ("\n");
#endif /* DEBUG */

   /*
    * Increment the client ID, wrapping the value if the number exceeds
    * CAD_MAX_TRANSACTION_NUMBER.
    * NOTE: Client IDs for data update packets are currently ignored, and may
    * be removed.
    */

   if (context->clientId == CAD_MAX_TRANSACTION_NUMBER)
   {
      context->clientId = 0;
   }
   else
   {
      context->clientId++;
   }
   UPDATE_PKT_CLIENT_ID (context) = context->clientId;

   /*
    * Define the output array pointer to point to the data values in the 
    * context->pUpdatePacket structure. Using this pointer will access the 
    * data update packet as if it were an array of double values.
    */

   outputArray = (double *) (int) UPDATE_PKT_VALUE_PTR(context);

   /* Copy the data values from the J structure to the data update packet. */

   for ( i = 0; i < (int)nValues; i++ )
   {
      outputArray[i] = inputArray[i];
   }

   /*
    * Write the data update packet to the data update pipe. The number of bytes
    * to be transmitted has already been calculated from the header size and
    * number of values in epToVxDbInitGensub().
    */

   nByte = context->sizeOfUpdatePacket;

   status = write (context->gensubToTaskPipeFd, context->pUpdatePacket, nByte);
   if (status != nByte)
   {
      ERROR_SET (0, "Error writing to data update pipe", ERROR_LOG_NOW);
      return (ERROR);
   }

   return (OK);
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxGensubOutput
 *
 *   INVOCATION:
 *   epToVxGensubOutput (pgensub)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pgensub (struct genSubRecord *)   pointer to genSub record structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the routine failed
 *
 *   PURPOSE:
 *   Execution routine for output genSub record
 *
 *   DESCRIPTION:
 *   This is the execution routine for a genSub record used for data output.
 *   The input J values are simply copied to the output VALJ values.
 *
 *   ERROR HANDLING:
 *   Any errors are logged immediately via the error-logging library, errorLib.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *   BUGS:
 *   The number of values stored in the .NOJ and .NOVJ fields can become 
 *   corrupted. As a work around, these fields are examined when the record 
 *   is first initialised (in epToVxGensubInit) and then not looked at again. 
 *   SMB - 27 Apr 1998.
 *-
 */

STATUS   epToVxGensubOutput
   (
   struct genSubRecord *   pgensub
   )
{
   SYM_TYPE      symType;
   GSUB_CONTEXT  context;
   double *      inputArray;  
                          /* Pointer acting as input array of double values.  */
   double *      outputArray; 
                          /* Pointer acting as output array of double values. */
   uint32        nValues; /* Number of input and output values.               */
   FAST int      i;       /* Element number.                                  */

#ifdef DEBUG
   printf ("epToVxGensubOutput: %s\n", pgensub->name);
#endif /* DEBUG */

   /*
    * Check that the genSub has been initialised. If it has, look up its context
    * structure in the local database.
    */

   if (! pWfsDbRecInitialised [GENSUB_RECORD_TYPE])
   {
      ERROR_SET1 (S_epToVxLib_RECORD_UNINITIALISED, 
                  "genSub record \"%s\" not initialised",
                  ERROR_LOG_NOW, pgensub->name);
      return (ERROR);
   }
   else if (symFindByNameAndType (epToVxSymtab, pgensub->name, 
            (char **) & context, (SYM_TYPE *) & symType, 
            (SYM_TYPE) GENSUB_RECORD_TYPE, SYM_TYPE_MASK) == ERROR)
   {
      ERROR_SET1 (0, "Could not find genSub \"%s\" in symbol table", 
                  ERROR_LOG_NOW, pgensub->name);
      return (ERROR);
   }

   /*
    * Find out how many values this genSub record has. This number has already 
    * been verified to be sensible in epToVxGensubInit.
    */

   nValues = context->nValues;

   /*
    * Define the input array pointer to point to the J values in the pgensub 
    * structure. Using this pointer will access the J values as if they were 
    * an array of double values.
    */

   inputArray = (double *)pgensub->j;

   /*
    * Define the output array pointer to point to the VALJ values in the 
    * pgensub structure. Using this pointer will access the VALJ structure as 
    * if it were an array of double values.
    */

   outputArray = (double *)pgensub->valj;

   /*
    * Now copy the input values in the J structure to the output values in 
    * the VALJ structure.
    */

   for ( i = 0; i < (int)nValues; i++ )
   {
      outputArray[i] = inputArray[i];
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxCarDaemon
 *
 *   INVOCATION:
 *   epToVxCarDaemon (pRecordName)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pRecordName   (char *)   name of CAR record to maintain
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the daemon task fails
 *
 *   PURPOSE:
 *   Daemon task maintains an EPICS CAR record
 *
 *   DESCRIPTION:
 *   This routine executes as a daemon task which is responsible for maintaining
 *   the status of an EPICS CAR record. It executes on the CPU which holds the
 *   EPICS database in which the CAR resides and a separate daemon must be 
 *   spawned for each CAR. This CAR daemon task creates and opens a pipe to 
 *   which each CAD record associated with the CAR record can write 
 *   command-header packets during its execution routine (see epToVxCadExecute()
 *   ). The "command-header" packets contain the header associated with a CAD 
 *   command. The CAR daemon also opens a "response" pipe via which the 
 *   response of VxWorks control tasks to CAD commmands are received in the 
 *   form of "response" data packets. These command-header and response packets 
 *   determine the states through which the CAR record is driven.
 *
 *   SIMULATION MODES:
 *   The CAR daemon supports the VSM simulation mode as follows. If a command 
 *   header is received with VSM simulation mode enabled, then the daemon will 
 *   set the CAR to the "BUSY" and then "IDLE" states, simulating the CAD/CAR 
 *   interface without involving any other application tasks.
 *
 *   ERROR HANDLING:
 *   This routine will return ERROR if an error occurs during the initialisation
 *   phase. However once it starts processing response packets any errors will 
 *   be logged but the routine will try to keep running. Any errors are logged
 *   immediately via the error-logging library, errorLib.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   (>) epToVxSymtab (SYMTAB_ID)     symbol table used internally by epToVxLib
 *   (!) (pContext)   (CAR_CONTEXT *) Not strictly a global variable, but 
 *                                    obtained from the epToVxSymtab symbol 
 *                                    table and therefore a pointer to a 
 *                                    globally accessible data structure.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   Only a limited amount of information can be exchanged between command
 *   tasks and the CAR daemon due to the fixed size of the command packets.
 *
 *   BUGS:
 *   The same memory corruption bugs in the Channel Access ID structures 
 *   described in epToVxCaWriteDaemon are seen in this function also. The same 
 *   bug work around has been applied. SMB - 7 Jul 1998. This bug has been 
 *   avoided by using the CICS database access functions instead of Channel 
 *   Access. SMB - 27 Jan 1999.
 *
 *   If an error occurs during the initialisation of this function, the memory 
 *   allocated by this function is not freed. This will lead to a memory leak 
 *   if the initialisation of the function is repeated. SMB - 8 Oct 1998.
 *
 *   DEVELOPMENT NOTES:
 *   Memory is allocated for the command packets but never freed. This is
 *   acceptable because the daemon task is expected to loop forever.
 *
 *   When a command fails the daemon task writes the message "Command failed" to
 *   the CAR record. I find this message unhelpful, and it would be better if
 *   the actual error message reported by the command task could be written.
 *   Doing this would involve increasing the size of the command done packet.
 *
 *   This function does not distinguish between START and STOP directives, and
 *   it will therefore assume that a STOP directive will result in the same 
 *   changes to the CAR record as a START directive (i.e. BUSY then IDLE).
 *-
 */

STATUS   epToVxCarDaemon
   (
   char * pRecordName
   )
{
   int             commandBeginFd = ERROR; /* File descriptor for command     */
                                           /* begin pipe.                     */
   int             commandDoneFd = ERROR;  /* File descriptor for command done*/
                                           /* pipe.                           */
   int             nByte;                  /* Number of bytes read/written.   */
   struct {char * pCmdPacket;}   beginContext;
   struct {char * pCmdPacket;} doneContext;
   SYM_TYPE        symType;
   CAR_CONTEXT     pContext;
   struct aiocb    aioControlBlock;        /* Asynchronous I/O control block. */
   const struct aiocb * aioList [1];       /* List of aio blocks. There is    */
                                           /* only one.                       */
   CAD_CMD_CONTEXT pCadCmdContext;         /* Command context for CADs        */
                                           /* associated with this CAR record */

   char            ivalField [EPICS_MAX_BYTES_RECORD_NAME + 1];
   char            icidField [EPICS_MAX_BYTES_RECORD_NAME + 1];
   char            ierrField [EPICS_MAX_BYTES_RECORD_NAME + 1];
   char            imssField [EPICS_MAX_BYTES_RECORD_NAME + 1];
                                           /* Field names used by database    */
                                           /* access.                         */

   char            message [EPICS_MAX_BYTES_STRING_ATTRIB * 2];
                                           /* Error message returned by       */
                                           /* database access                 */

   /*
    * First initialise error-logging for this daemon.
    */

   if (errorInit () == ERROR)
   {
      printf ("%s: epToVxCarDaemon: Failed to initialise error context structure\n",
          taskName (taskIdSelf()));
      return (ERROR);
   }

   /* Look-up the CAR record assigned to this invocation of the daemon in the 
    * local database 
    */

   if (symFindByNameAndType (epToVxSymtab, pRecordName, (char **) & pContext,
      (SYM_TYPE *) & symType, (SYM_TYPE) CAR_RECORD_TYPE, SYM_TYPE_MASK) 
      == ERROR)
   {
      ERROR_SET1 (0, "Could not find CAR \"%s\" in symbol table", 
                  ERROR_LOG_NOW, pRecordName);
      return (ERROR);
   }

   /*
    * Determine the field names to be used by database access (in case Channel 
    * Access fails).
    */

   sprintf (ivalField, "%s%s", pRecordName, ".IVAL");
   sprintf (icidField, "%s%s", pRecordName, "ID.VAL");
   sprintf (ierrField, "%s%s", pRecordName, ".IERR");
   sprintf (imssField, "%s%s", pRecordName, ".IMSS");

   /*
    * Set CAR busy and initialise the client ID and error number
    */

   CAR_FIELD_VALUE (pContext) = menuCarstatesBUSY;
   CAR_FIELD_CLIENT_ID (pContext) = 0;
   CAR_FIELD_ERROR_NUMBER (pContext) = 0;

   /*
    * BUG WORK-AROUND.
    * Don't use Channel Access. Use database access instead.
    * (See the "BUGS" section above).
    */

   if ( (cicsDbPut (icidField, message, DBF_LONG, 
                    &CAR_FIELD_CLIENT_ID (pContext)) == ERROR) ||
        (cicsDbPut (ivalField, message, DBF_LONG, 
                    &CAR_FIELD_VALUE (pContext)) == ERROR)
      )
   {
      ERROR_SET2 (0, "Database access write to %s failed: %s", ERROR_LOG_NOW,
         pRecordName, message);
   }

   /*
    * Open the pipe via which a control task will write "command done" packets 
    * to this daemon. These packets terminate a previously-initialised (via 
    * a "command begin" packet) command sequence.
    */

   commandDoneFd = 
   epToVxPipeOpen (FALSE, pContext->pTaskName, TASK_TO_CAR_PIPE_NAME_EXT,
                   NULL, 0, 0, O_RDONLY, -1, CAR_TIMEOUT_OPENPIPE,
                   CAR_DELAY_OPENPIPE);
   if (commandDoneFd == ERROR)
   {
      ERROR_LOG ("Failed to open command-done pipe");
      return (ERROR);
   }

   /*
    * Create and open for reading the pipe via which "command begin" packets 
    * are to be read.
    */

   commandBeginFd = 
   epToVxPipeOpen (FALSE, pContext->pTaskName, CAD_TO_CAR_PIPE_NAME_EXT,
                   pipeDevCreate, EPTOVX_CAD_CAR_PIPES_NMSGS,
                   CMD_PKT_HEADER_SIZE_BYTES, O_RDONLY, -1, 0.0, 0.0);
   if (commandBeginFd == ERROR)
   {
      ERROR_LOG ("Failed to create/open command-begin pipe");
      return (ERROR);
   }

   /* Allocate memory for command packets to be read from the pipe(s) */

   if (((beginContext.pCmdPacket = 
         calloc (1, CMD_PKT_HEADER_SIZE_BYTES + CMD_PKT_PADDING_BYTES))
          == NULL) ||
       ((doneContext.pCmdPacket = 
         calloc (1, CMD_PKT_HEADER_SIZE_BYTES + CMD_PKT_PADDING_BYTES))
         == NULL))
   {
      ERROR_SET (0, "Memory allocation for command packets failed", 
                 ERROR_LOG_NOW);
      return (ERROR);
   }

   /*
    * The asynchronous I/O library, aioLib, is used to read "command done" 
    * packets. This allows a timeout to be specified for each read() operation, 
    * thus allowing the daemon to detect when a command has timed-out. 
    * Initialise the asynchronous IO control block.
    */

   bzero ((char *) & aioControlBlock, sizeof (aioControlBlock));
   aioControlBlock.aio_fildes = commandDoneFd;
   aioControlBlock.aio_offset = 0;
   aioControlBlock.aio_buf = (void *) doneContext.pCmdPacket;
   aioControlBlock.aio_nbytes = CMD_PKT_HEADER_SIZE_BYTES;
   aioControlBlock.aio_reqprio = CAR_COMMAND_DONE_AIO_PRIORITY;
   aioList [0] = & aioControlBlock;

   /*
    * Initialise the CAR state to IDLE.
    */

   CAR_FIELD_VALUE (pContext) = menuCarstatesIDLE;

   /*
    * BUG WORK-AROUND.
    * Don't use Channel Access. Use database access instead.
    * (See the "BUGS" section above).
    */

   if ( cicsDbPut (ivalField, message, DBF_LONG, 
                   &CAR_FIELD_VALUE (pContext)) == ERROR )
   {
      ERROR_SET2 (0, "Database access write to %s failed: %s", ERROR_LOG_NOW,
         pRecordName, message);
   }

   /*
    * Get the CAD command structure - this defines all of the CADs that this CAR
    * is responsible for.
    */

   pCadCmdContext = epToVxCmdInit (pContext->pTaskName, NULL);
   if (pCadCmdContext == NULL)
   {
      ERROR_LOG ("Error getting CAD command context");
      return (ERROR);
   }

   /*
    * Issue an initial message.
    * NOTE: The MESSAGE_LOG macro cannot be used to display this message 
    * (as is done with most other tasks) because there is a potential for 
    * deadlock between this task and the message logging task. The deadlock 
    * scenario is:
    * 1) The epToVxCarDaemon issues its "entering loop waiting for data" 
    *    message.
    * 2) The message is directed to the message logging task, and a response 
    *    is waited for.
    * 3) The message logging task is still initialising, and it tries to 
    *    initialise its EPICS CAR record by sending a message to the 
    *    epToVxCarDaemon task.
    * 4) The epToVxCarDaemon task is not responding because it is waiting to 
    *    send its message.
    *    The result is deadlock.
    */

   printf ("%s: epToVxCarDaemon: Ready: waiting for commands.\n", 
           taskName (taskIdSelf()));
                                      /* Cannot use MESSAGE_LOG. See above.   */

   FOREVER                            /* Infinite loop maintaining the CAR.   */
   {
      /*
       * Read the next "command begin" packet.
       * This will block indefinitely until a packet arrives.
       */

      nByte = 
      read (commandBeginFd, beginContext.pCmdPacket, CMD_PKT_HEADER_SIZE_BYTES);
      if (nByte != CMD_PKT_HEADER_SIZE_BYTES)
      {
         ERROR_SET (0, "Error reading command-begin pipe", ERROR_LOG_NOW);
      }

      /*
       * Extract the current client ID and command number from command header.
       * If the command number is valid, set the CAR BUSY.
       */

      CAR_FIELD_CLIENT_ID (pContext) = CAR_CLIENT_ID (& beginContext);

      if (EPTOVX_IS_VALID_CMD_NUM (pCadCmdContext, 
          (int) CMD_PKT_COMMAND_NUMBER (& beginContext)))
      {
         CAR_FIELD_ERROR_NUMBER (pContext) = 0;
         CAR_FIELD_VALUE (pContext) = menuCarstatesBUSY;
         strncpy (CAR_FIELD_MESSAGE (pContext), " ", 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         ERROR_SET (S_epToVxLib_INVALID_COMMAND_NUM, "Invalid command number", 
                    ERROR_LOG_NOW);
         CAR_FIELD_VALUE (pContext) = menuCarstatesERROR;
         CAR_FIELD_ERROR_NUMBER (pContext) = S_epToVxLib_INVALID_COMMAND_NUM;
         strncpy (CAR_FIELD_MESSAGE (pContext), "Invalid command number",
                  EPICS_MAX_BYTES_STRING_ATTRIB);
      }

      /*
       * BUG WORK-AROUND.
       * Don't use Channel Access. Use database access instead.
       * (See the "BUGS" section above).
       */

      if ( (cicsDbPut (icidField, message, DBF_LONG, 
           &CAR_FIELD_CLIENT_ID (pContext)) == ERROR) ||
           (cicsDbPut (ierrField, message, DBF_LONG, 
            &CAR_FIELD_ERROR_NUMBER (pContext)) == ERROR) ||
           (cicsDbPut (imssField, message, DBF_STRING, 
            CAR_FIELD_MESSAGE (pContext)) == ERROR) ||
           (cicsDbPut (ivalField, message, DBF_LONG, 
            &CAR_FIELD_VALUE (pContext)) == ERROR)              
         )
      {
         ERROR_SET2 (0, "Database access write to %s failed: %s", 
                     ERROR_LOG_NOW, pRecordName, message);
      }

#ifdef DEBUG
      printf ("epToVxCarDaemon: Command BEGIN with client ID = %d\n",
              CMD_PKT_CLIENT_ID (& beginContext));
#endif /* DEBUG */

      switch (CMD_PKT_COMMAND_MODIFIER (& beginContext) & EPTOVX_SIM_MODE_MASK)
      {
         case EPTOVX_SIM_MODE_VSM:

            /*
             * In VSM simulation mode the CAR is set IDLE immediately.
             */

            CAR_FIELD_VALUE (pContext) = menuCarstatesIDLE;
            strncpy (CAR_FIELD_MESSAGE (pContext), " ", 
                     EPICS_MAX_BYTES_STRING_ATTRIB);

            /*
             * BUG WORK-AROUND.
             * Don't use Channel Access. Use database access instead.
             * (See the "BUGS" section above).
             */

            if ( cicsDbPut (ivalField, message, DBF_LONG, 
                            &CAR_FIELD_VALUE (pContext)) == ERROR )
            {
               ERROR_SET2 (0, "Database access write to %s failed: %s", 
                           ERROR_LOG_NOW, pRecordName, message);
            }

            break;

         default:

            /*
             * The simulation mode is either FAST, FULL or NONE (simulation is 
             * disabled). Treat all these cases identically:
             * First read the "command done" packet for this command, with a 
             * timeout period specified which is dependent on the CAD that 
             * is currently being executed (the macro CAR_PIPE_TIMEOUT() 
             * returns the timeout period for the current command).
             */

            if (aio_read (& aioControlBlock) == ERROR)
            {
               ERROR_SET (0, "Asynchronous read of command-done pipe failed", 
                          ERROR_LOG_NOW);
            }

            if (aio_suspend (aioList, 1, & CAR_PIPE_TIMEOUT (pCadCmdContext,
               CMD_PKT_COMMAND_NUMBER (& beginContext))) == ERROR)
            {
               /* Timeout reading "command done" packet */

               ERROR_SET (0, "Timeout waiting for command-done packet", 
                          ERROR_LOG_NOW);
               CAR_FIELD_VALUE (pContext) = menuCarstatesERROR;
               CAR_FIELD_ERROR_NUMBER (pContext) = 
               S_epToVxLib_TIMEOUT_WAITING_FOR_PIPE;
               strncpy (CAR_FIELD_MESSAGE (pContext), "Timeout", 
                        EPICS_MAX_BYTES_STRING_ATTRIB);

               /*
                * BUG WORK-AROUND.
                * Don't use Channel Access. Use database access instead.
                * (See the "BUGS" section above).
                */

               if ( (cicsDbPut (icidField, message, DBF_LONG,
                           &CAR_FIELD_CLIENT_ID (pContext)) == ERROR) ||
                    (cicsDbPut (ierrField, message, DBF_LONG,
                           &CAR_FIELD_ERROR_NUMBER (pContext)) == ERROR) ||
                    (cicsDbPut (imssField, message, DBF_STRING,
                           CAR_FIELD_MESSAGE (pContext)) == ERROR) ||
                    (cicsDbPut (ivalField, message, DBF_LONG,
                           &CAR_FIELD_VALUE (pContext)) == ERROR)
                  )
               {
                  ERROR_SET2 (0, "Database access write to %s failed: %s", 
                              ERROR_LOG_NOW, pRecordName, message);
               }
            }
            else if (aio_return (& aioControlBlock) != 
                     CMD_PKT_HEADER_SIZE_BYTES)
            {

               /*
                * The "command done" packet was read OK, but an error occurred 
                * when terminating the aio read. Try to handle this error.
                */

               ERROR_SET (0, "Unexpected asynchronous I/O return status", 
                          ERROR_LOG_NOW);
               CAR_FIELD_VALUE (pContext) = menuCarstatesERROR;
               CAR_FIELD_ERROR_NUMBER (pContext) = 
               S_epToVxLib_INVALID_PACKET_SIZE;
               strncpy (CAR_FIELD_MESSAGE (pContext), 
                        "Invalid message packet size",
                        EPICS_MAX_BYTES_STRING_ATTRIB);
               /*
                * BUG WORK-AROUND.
                * Don't use Channel Access. Use database access instead.
                * (See the "BUGS" section above).
                */

               if ( (cicsDbPut (icidField, message, DBF_LONG,
                           &CAR_FIELD_CLIENT_ID (pContext)) == ERROR) ||
                    (cicsDbPut (ierrField, message, DBF_LONG,
                           &CAR_FIELD_ERROR_NUMBER (pContext)) == ERROR) ||
                    (cicsDbPut (imssField, message, DBF_STRING,
                           CAR_FIELD_MESSAGE (pContext)) == ERROR) ||
                    (cicsDbPut (ivalField, message, DBF_LONG,
                           &CAR_FIELD_VALUE (pContext)) == ERROR)
                  )
               {
                  ERROR_SET2 (0, "Database access write to %s failed: %s", 
                              ERROR_LOG_NOW, pRecordName, message);
               }
            }
            else if ((CMD_PKT_CLIENT_ID (& beginContext) != 
                      CMD_PKT_CLIENT_ID (& doneContext)) || 
                     (CMD_PKT_COMMAND_NUMBER (& beginContext) !=
                      CMD_PKT_COMMAND_NUMBER (& doneContext)))
            {

               /*
                * The header information in the "command begin" and 
                * "command done" packets is inconsistent. Some form of 
                * synchronisation error has occurred. Ignore this
                * packet and report the error.
                */

               ERROR_SET (S_epToVxLib_CAD_CAR_SYNCH_ERROR,
                  "Unsynchronised command begin/done packets", ERROR_LOG_NOW);

               CAR_FIELD_VALUE (pContext) = menuCarstatesERROR;
               CAR_FIELD_CLIENT_ID (pContext) = CAR_CLIENT_ID (& doneContext);
               CAR_FIELD_ERROR_NUMBER (pContext) = 
               S_epToVxLib_CAD_CAR_SYNCH_ERROR;
               strncpy (CAR_FIELD_MESSAGE (pContext), "Synchronisation error",
                        EPICS_MAX_BYTES_STRING_ATTRIB);
               /*
                * BUG WORK-AROUND.
                * Don't use Channel Access. Use database access instead.
                * (See the "BUGS" section above).
                */


               if ( (cicsDbPut (icidField, message, DBF_LONG,
                                &CAR_FIELD_CLIENT_ID (pContext)) == ERROR) ||
                    (cicsDbPut (ierrField, message, DBF_LONG,
                                &CAR_FIELD_ERROR_NUMBER (pContext)) == ERROR) ||
                    (cicsDbPut (imssField, message, DBF_STRING,
                                CAR_FIELD_MESSAGE (pContext)) == ERROR) ||
                    (cicsDbPut (ivalField, message, DBF_LONG,
                                &CAR_FIELD_VALUE (pContext)) == ERROR)
                  )
               {
                  ERROR_SET2 (0, "Database access write to %s failed: %s", 
                              ERROR_LOG_NOW, pRecordName, message);
               }
            }
            else if (CMD_PKT_ERROR_NUMBER (& doneContext) != 0)
            {

               /*
                * This is a relatively "mild" error condition: The 
                * error-number returned by the control task (in the "command 
                * done" packet) is non-zero, this indicating that the command 
                * failed to execute successfully. Report this error.
                *
                * Reduce this uninformative message to a warning, so it does 
                * not overwrite the "real" error in the errorLog records.
                */

               MESSAGE_LOG (MSG_WARNING, 
                            "WARNING: Error returned by control task");
               CAR_FIELD_VALUE (pContext) = menuCarstatesERROR;
               CAR_FIELD_ERROR_NUMBER (pContext) = 
               CMD_PKT_ERROR_NUMBER (& doneContext);

               /* NOTE: The following message is unhelpful. Can a command done
                * message be returned in the command done packet? - 
                * SMB 15 Dec 97.
                */

               strncpy (CAR_FIELD_MESSAGE (pContext), "Command failed",
                        EPICS_MAX_BYTES_STRING_ATTRIB);
               /*
                * BUG WORK-AROUND.
                * Don't use Channel Access. Use database access instead.
                * (See the "BUGS" section above).
                */

               if ( (cicsDbPut (icidField, message, DBF_LONG,
                                &CAR_FIELD_CLIENT_ID (pContext)) == ERROR) ||
                    (cicsDbPut (ierrField, message, DBF_LONG,
                                &CAR_FIELD_ERROR_NUMBER (pContext)) == ERROR) ||
                    (cicsDbPut (imssField, message, DBF_STRING,
                                CAR_FIELD_MESSAGE (pContext)) == ERROR) ||
                    (cicsDbPut (ivalField, message, DBF_LONG,
                                &CAR_FIELD_VALUE (pContext)) == ERROR)
                  )
               {
                  ERROR_SET2 (0, "Database access write to %s failed: %s", 
                              ERROR_LOG_NOW, pRecordName, message);
               }
            }
            else
            {

               /* All OK - set the CAR IDLE */

               CAR_FIELD_VALUE (pContext) = menuCarstatesIDLE;
               CAR_FIELD_ERROR_NUMBER (pContext) = 0;
               strncpy (CAR_FIELD_MESSAGE (pContext), " ", 
                        EPICS_MAX_BYTES_STRING_ATTRIB);

               /*
                * BUG WORK-AROUND.
                * Don't use Channel Access. Use database access instead.
                * (See the "BUGS" section above).
                */

               if ( cicsDbPut (ivalField, message, DBF_LONG,
                               &CAR_FIELD_VALUE (pContext)) == ERROR )
               {
                  ERROR_SET2 (0, "Database access write to %s failed: %s", 
                              ERROR_LOG_NOW, pRecordName, message);
               }
            }
#ifdef DEBUG
            printf ("epToVxCarDaemon: Command DONE\n");
#endif /* DEBUG */
      }
   }

   close (commandDoneFd);
   if (commandBeginFd != commandDoneFd) close (commandBeginFd);

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxSetCadSimMode
 *
 *   INVOCATION:
 *   epToVxSetCadSimMode (simMode)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   simMode   (const int)   new simulation mode
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Set the CAD simulation mode for all CADs in the system
 *
 *   DESCRIPTION:
 *   This routine sets the simulation mode for all CADs. The simulation
 *   mode can be VSM, FAST, FULL or NONE, as defined in the first table,
 *
 *      VSM  => Virtual Simulation Mode, in which only the
 *              high level command interface is simulated.
 *      FAST => Fast mode, in which commands are simulated
 *              but the response time is unrealistic.
 *      FULL => Full simulation, in which commands are simulated
 *              and the response time is realistic.
 *      NONE => No simulation.
 *
 *   and this mode is identified by one of the encoded values 0, 1, 2 or 3
 *   tabulated in the second table.
 *
 *      VSM    =>   0
 *      FAST   =>   1
 *      FULL   =>   2
 *      NONE   =>   3
 *
 *   The specified argument is ANDed (bitwise) with 0x03 such that the
 *   adopted value is always valid irrespecive of the value of the argument.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   The simulation mode is stored as a static variable on the same CPU
 *   as that on which the EPICS database is loaded. It can only be used on
 *   a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   (<)   epToVxCadSimMode   (int)   CAD simulation mode
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

void epToVxSetCadSimMode
   (
   const int   simMode
   )
{

   /*
    * Mask the given simMode before writing it to epToVxCadSimMode,
    * so the result is always valid.
    */

   epToVxCadSimMode = simMode & EPTOVX_SIM_MODE_MASK;
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   eptovx_writeCadOutAttribs
 *
 *   INVOCATION:
 *   eptovx_writeCadOutAttribs (pContext, pcad)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pContext  (CAD_CONTEXT)         CAD context structure
 *   (>)   pcad      (struct cadRecord *)  CAD record structure
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Copy CAD attributes from context structure to CAD record's outputs
 *
 *   DESCRIPTION:
 *   This routine will copy CAD attribute values from static storage in
 *   the CAD context structure pointed to by pContext to the attribute
 *   outputs (e.g. pointed to by pcad->vala, pcad->valb etc) for the
 *   associated CAD record.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   It is assumed that the CAD attribute values were previously saved
 *   in the context structure, e.g. by the routine eptovx_saveAttribs().
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   This routine is not complete - check the source code.
 *   The function also assumes that the output attributes of a CAD
 *   record occupy consecutive slots in the cadRecord structure.
 *
 *   BUGS:
 *   I do not like this function's use of ppAttrib[i] at all.
 *   It is treating the contents of the cadRecord data structure
 *   as if the vala, valb, etc... were unioned to an array of character
 *   strings, whereas in reality they are a set void pointers which
 *   just happen to occupy consecutive slots in the data structure
 *   (but even this can't be guaranteed).
 *   I suggest this function be commented out for now. SMB - 16 Mar 1998.
 *
 *   Further to the above comment, I have now written my own functions
 *   to copy CAD attributes - epToVxCadCopy and eptovx_copyAttrib.
 *   If it becomes necessary, this function could be rewritten to use those.
 *   SMB - 14 October 1998.
 *-
 */

void   eptovx_writeCadOutAttribs
   (
   CAD_CONTEXT         pContext,
   struct cadRecord *  pcad
   )
{
   char **      ppAttrib;
   FAST uint32   i;

   /* Initialise the array of pointers to output attribute "vala" */

   ppAttrib = (char **) & pcad->vala;

   /*
    * For each attribute, copy attribute from the structure pContext to the 
    * CAD record (ppAttrib[])
    */

   for (i = 0; i < pContext->nAttrib; i++)
   {
      epToVxCmdAttribGet (pContext, i, ppAttrib [i], NULL);
   }

   /* Insert code to copy the attribute retrieved above to the CAD attribute 
    * output fields here 
    */
}

/* -------------------------------------------------------------------------- */


/*+
 *   IGNORED FUNCTION NAME:
 *   eptovx_loadDefaultInputAttribs
 *
 *   INVOCATION:
 *   eptovx_loadDefaultInputAttribs (pContext, pcad)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pContext   (CAD_CONTEXT)          CAD context structure
 *   (>)   pcad       (struct cadRecord *)   CAD record structure
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Copy CAD attribute default values to CAD record's inputs
 *
 *   DESCRIPTION:
 *   This routine will copy any default attribute values defined for
 *   a CAD record to the attribute inputs of the record. Default
 *   values are defined in the declaration & initialisation of the
 *   array pWfsDbCadList[], and subsequently loaded into the local database
 *   epToVxSymtab. The CAD_CONTEXT structure for each CAD record also
 *   holds the default values in their native data types (long, double
 *   or char*).
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   This function assumes that the attribute value strings contained in the
 *   pcad structure are all contiguous and exactly EPICS_MAX_BYTES_STRING_ATTRIB
 *   bytes in size. The function will fail if the CAD record is changed so this
 *   assumption is no longer true. SMB - 16 Mar 1998.
 *-
 */

void   eptovx_loadDefaultInputAttribs
   (
   CAD_CONTEXT         pContext,
   struct cadRecord *  pcad
   )
{
   char *      pAttrib;
   FAST uint32   i;

   /* Initialise to input attribute "a" */

   pAttrib = pcad->a;

   /*
    * For each attribute, copy the default attribute to the CAD input string.
    * The attribute type may be string, long or double.
    */

   for (i = 0; i < pContext->nAttrib; i++)
   {
      switch (pContext->pType [i])
      {
         case EPICS_DATA_TYPE_STRING:

            strncpy (& pAttrib [i * EPICS_MAX_BYTES_STRING_ATTRIB],
                     pContext->pDefault [i].pStringAttrib,
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            break;

         case EPICS_DATA_TYPE_LONG:

            sprintf (& pAttrib [i * EPICS_MAX_BYTES_STRING_ATTRIB], "%d",
                     (int)(pContext->pDefault [i].longAttrib));
            break;

         case EPICS_DATA_TYPE_DOUBLE:

            sprintf (& pAttrib [i * EPICS_MAX_BYTES_STRING_ATTRIB], "%g",
                     pContext->pDefault [i].doubleAttrib);
      }
   }
}

/* -------------------------------------------------------------------------- */


/*+
 *   IGNORED FUNCTION NAME:
 *   eptovx_checkAttribs
 *
 *   INVOCATION:
 *   eptovx_checkAttribs (pContext, pOffendingAttrib, pReason)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pContext         (CAD_CONTEXT) CAD context structure
 *   (>) pOffendingAttrib (uint32 *)    where to put the ID # for
 *                                      any offending attribute
 *   (<) pReason          (char *)      String to contain the
 *                                      reason for rejection of
 *                                      an attribute
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if any attributes are outside the
 *   permitted range
 *
 *   PURPOSE:
 *   Check that CAD attributes are within an allowed range
 *
 *   DESCRIPTION:
 *   This routine checks the CAD attribute values stored in the CAD context
 *   structure (via a previous call to eptovx_saveAttribs()) against a set of
 *   permitted ranges for each attribute. Each attribute type may be either
 *   long, double or char*; the permitted range definitions for long and
 *   double data types may consists of either - (i) a lower-limit; (ii) an
 *   upper limit; (iii) lower- and upper-limits; or (iv) no limits. String
 *   attributes (of type char*) may also have "range" definitions, but in this
 *   case the definition consists of a list of allowed values that the string
 *   may take (a null-length list corresponds to a string attribute that may
 *   take any value). The definition of attribute data types and of the
 *   permitted ranges is contained in the declaration & initialisation of the
 *   array pWfsDbCadList[].
 *
 *   The routine will return ERROR immediately it encounters an attribute
 *   that is out of range, and will in this case set *pOffendingAttrib to
 *   the number of the offending attribute (e.g. attribute "a" is number 1,
 *   attribute "b" is number 2 etc). On exit, the string "pReason" (which must
 *   be at least EPICS_MAX_BYTES_STRING_ATTRIB in size) will contain the
 *   reason for the rejection of the attribute.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   It is assumed that the CAD attribute values were previously saved
 *   in the context structure, e.g. by the routine eptovx_saveAttribs().
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS   eptovx_checkAttribs
   (
   CAD_CONTEXT   pContext,
   uint32 *      pOffendingAttrib,
   char *        pReason
   )
{
   FAST uint32   i;
   FAST uint32   j;
   BOOL          attribValid;
   long          longAttrib;
   double        doubleAttrib;
   char          pStringAttrib [EPICS_MAX_BYTES_STRING_ATTRIB];


   /* Sort through all defined attributes   */

   for (i = 0; i < pContext->nAttrib; i++)
   {

#ifdef DEBUG
      printf ("epToVxLib: eptovx_checkAttribs: checking validity of attribute #%d\n", i);
#endif /* DEBUG */

      /*
       * Check if there are any limits on the values that the current 
       * attribute [i] may take.
       */   


      if (pContext->pNumberRangeValues [i] > 0)
      {

         /*
          * Within the following switch statement, return ERROR if an invalid 
          * attribute is detected, but this should not be logged as a run-time 
          * error - it is entirely legitimate for an invalid attribute value 
          * to be given (e.g. perhaps someone was testing the ability of this 
          * library to reject invalid attributes !).
          * Hence no call to ERROR_SET().
          */

         switch (pContext->pType [i])
         {
            case EPICS_DATA_TYPE_STRING:

               /*
                * Get string attribute, then check that its value matches 
                * one of those given in the list of range values. Abort search 
                * through the range list immediately a match is found.
                */

               epToVxCmdAttribGet (pContext, i, pStringAttrib, NULL);

               for (j = 0, attribValid = FALSE; 
                    j < pContext->pNumberRangeValues [i]; 
                    j++)
               {
                  if (strcmp (pStringAttrib, 
                      pContext->ppAllowedRange [i][j].pStringAttrib) == 0)
                  {
                     attribValid = TRUE;
                     j = pContext->pNumberRangeValues [i];   
                                             /* Attrib OK - abort search      */
                  }
               }

               if (! attribValid)            /* Attrib not OK - return ERROR  */
               {
                  strncpy (pReason, "not in list", 
                           EPICS_MAX_BYTES_STRING_ATTRIB);
                  * pOffendingAttrib = i + 1;
                  return (ERROR);
               }
               break;

            case EPICS_DATA_TYPE_LONG:

               /*
                * Get long attribute, then check that its value is within the 
                * defined range. If only one range value is defined then the 
                * attribute value must take on this value exactly.
                */

               epToVxCmdAttribGet (pContext, i, (char *) & longAttrib, NULL);

               if ((pContext->pNumberRangeValues [i] == 1) &&
                   (longAttrib != pContext->ppAllowedRange [i][0].longAttrib)
                  )
               {
                  sprintf (pReason, "!= %d", 
                           (int)(pContext->ppAllowedRange [i][0].longAttrib));
                  * pOffendingAttrib = i + 1;
                  return (ERROR);
               }

               /* If two ranges values are defined, check that the attrib is 
                * between these values.
                */

               if (longAttrib < pContext->ppAllowedRange [i][0].longAttrib)
               {
                  sprintf (pReason, "< %d", 
                           (int)(pContext->ppAllowedRange [i][0].longAttrib));
                  * pOffendingAttrib = i + 1;
                  return (ERROR);
               }
               else if (longAttrib > 
                        pContext->ppAllowedRange [i][1].longAttrib)
               {
                  sprintf (pReason, "> %d", 
                           (int)(pContext->ppAllowedRange [i][1].longAttrib));
                  * pOffendingAttrib = i + 1;
                  return (ERROR);
               }
               break;

            case EPICS_DATA_TYPE_DOUBLE:

               /*
                * Get double attribute, then check that its value is within 
                * the defined range.
                * If only one range value is defined then the attribute value 
                * must take on this value exactly.
                */

               epToVxCmdAttribGet (pContext, i, (char *) & doubleAttrib, NULL);

               if ((pContext->pNumberRangeValues [i] == 1) &&
                   (doubleAttrib != 
                    pContext->ppAllowedRange [i][0].doubleAttrib))
               {
                  sprintf (pReason, "!= %g", 
                           pContext->ppAllowedRange [i][0].doubleAttrib);
                  * pOffendingAttrib = i + 1;
                  return (ERROR);
               }

               /* If two ranges values are defined, check that the attrib is 
                * between these values.
                */

               if (doubleAttrib < pContext->ppAllowedRange [i][0].doubleAttrib)
               {
                  sprintf (pReason, "< %g", 
                           pContext->ppAllowedRange [i][0].doubleAttrib);
                  * pOffendingAttrib = i + 1;
                  return (ERROR);
               }
               else if (doubleAttrib > 
                        pContext->ppAllowedRange [i][1].doubleAttrib)
               {
                  sprintf (pReason, "> %g", 
                           pContext->ppAllowedRange [i][1].doubleAttrib);
                  * pOffendingAttrib = i + 1;
                  return (ERROR);
               }
               break;

            default:

               /*
                * Internal coding error - should never get here. If we do, log 
                * an error message immediately, because whatever calls this 
                * routine will not normally call ERROR_LOG() if ERROR is 
                * returned since the most likely cause is that the attribute 
                * value was out of range.
                */

               ERROR_SET1 (S_epToVxLib_INTERNAL_ERROR,
                           "Unrecognised EPICS data type for attribute %d", 
                           ERROR_LOG_NOW, (int)(i+1));
               strncpy (pReason, "Bad data type", 
                        EPICS_MAX_BYTES_STRING_ATTRIB);
               * pOffendingAttrib = i + 1;
               return (ERROR);
         }
      }
   }

#ifdef DEBUG
   printf ("epToVxLib: eptovx_checkAttribs: All attributes within defined ranges - returning OK\n");
#endif /* DEBUG */

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   eptovx_saveAttribs
 *
 *   INVOCATION:
 *   eptovx_saveAttribs (pcad, pContext, pOffendingAttrib)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pcad             (struct cadRecord *)  CAD record structure
 *   (>) pContext         (CAD_CONTEXT)         CAD context structure
 *   (>) pOffendingAttrib (uint32 *)            where to put the ID # for
 *                                              any offending attribute
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if any attributes cannot be converted
 *
 *   PURPOSE:
 *   Save CAD attributes in CAD context structure
 *
 *   DESCRIPTION:
 *   This routine copies CAD attribute values from an EPICS CAD record to
 *   static storage in a CAD context structure. Each attribute is converted
 *   from a string to its native type (long, double or char*) during the
 *   copy procedure.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   This function assumes that the attribute value strings contained in the
 *   pcad structure are all contiguous and exactly EPICS_MAX_BYTES_STRING_ATTRIB
 *   bytes in size. The function will fail if the CAD record is changed so this
 *   assumption is no longer true. SMB - 16 Mar 1998.
 *-
 */

STATUS   eptovx_saveAttribs
   (
   struct cadRecord *   pcad,
   CAD_CONTEXT          pContext,
   uint32 *             pOffendingAttrib
   )
{
   FAST uint32   i;
   char *        pAttribSource;
   STATUS        returnValue = OK;

   /* Initialise to attribute "a" */

   pAttribSource = pcad->a;

   /*
    * Reset the Default Mask Word in the command packet header.
    */

   CMD_PKT_DEFAULT_MASK (pContext) = 0;

   /*
    * Initially assume only a header in the command packet - this is especially 
    * necessary for CADs which have no attributes since the routine 
    * eptovx_attribPutCmdPacket() which sets the total size of the command 
    * packet will not be called in this case.
    */

   pContext->sizeOfCmdPacket = CMD_PKT_HEADER_SIZE_BYTES;

   /*
    * Copy each attribute from the CAD structure (pcad) into the command 
    * packet. This must be done in the order attribute #0, #1 etc. The total 
    * size of the command packet is incremented accordingly as each attribute 
    * is added.
    */

   for (i = 0; i < pContext->nAttrib; i++)
   {
      if (eptovx_attribPutCmdPacket (pContext, i,
          & pAttribSource [i * EPICS_MAX_BYTES_STRING_ATTRIB])
          == ERROR)
      {
         /* Only record the number of the first attribute to fail. */
         if (returnValue == OK) * pOffendingAttrib = i + 1;
         returnValue = ERROR;
      }

#ifdef DEBUG
      else
      {
         printf ("epToVxLib: eptovx_saveAttribs: Attrib string [%d] = \"%s\", (Attrib A = %s)\n",
                 i, & pAttribSource [i * EPICS_MAX_BYTES_STRING_ATTRIB], 
                 pcad->a);
      }
#endif /* DEBUG */

   }
   return (returnValue);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   eptovx_copyAttrib
 *
 *   INVOCATION:
 *   eptovx_copyAttrib (name, pInput, type, pOutput, pMessage)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) name     (const char)   Name of attribute (single character)
 *   (>) pInput   (const char *) String containing input value
 *   (>) type     (const int)    EPICS data type of value (DBR_xxx)
 *   (<) pOutput  (void *)       Pointer to output
 *   (>) pMessage (char *)       Pointer to string to contain message
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the attribute cannot be converted
 *
 *   PURPOSE:
 *   Copy a CAD attribute value from the given input to the given output
 *
 *   DESCRIPTION:
 *   This routine copies a CAD attribute value from an input field (such as 
 *   pcad->a) to an output field (such as pcad->vala), converting the value 
 *   from a string to the data type indicated by the type field (such as 
 *   pcad->ftva). A typical invocation is
 *v
 *v  eptovx_copyAttrib('A', pcad->a, (int) pcad->ftva, pcad->vala, pcad->mess);
 *v
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *   cadRecord.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   eptovx_copyAttrib
   (
   const char     name,         /* Name of atribute.                          */
   const char *   pInput,       /* Pointer to CAD input attribute.            */
   const int      type,         /* EPICS data type (DBR_xxx).                 */
   void *         pOutput,      /* Pointer to CAD output attribute.           */
   char *         pMessage      /* Pointer to string to contain message.      */
   )
{
   long           lvalue;       /* Input value converted to long.             */
   double         dvalue;       /* Input value converted to double.           */
   char *         pHexString1;  /* Pointer to "0x" in input (if any).         */
   char *         pHexString2;  /* Pointer to "0X" in input (if any).         */
   char *         pHexString3;  /* Pointer to "$" in input (if any).          */      

#ifdef DEBUG
   printf ("Attribute %c: Type=%d, Input=%s\n", name, type, pInput);
#endif   /* DEBUG */

   /* Check the input and output pointers are sensible. */

   if ( (pInput == NULL) || (pOutput == NULL) )
   {
      ERROR_SET (S_epToVxLib_INTERNAL_ERROR, 
         "Invalid input and output pointers provided",
         ERROR_LOG_SAVE);
      if ( pMessage != NULL )
      {
         sprintf (pMessage, "Attrib %c invalid pointers", name);
      }
      return (ERROR);
   }

   /* The conversion method depends on the data type of the attribute. */

   switch (type)
   {
      case (DBR_LONG):

         /*
          * long integer attribute.
          * If the input attribute is completely blank it is probably unused,
          * so don't copy it at all.
          */

         if ( (strlen(pInput) != 0) && (strspn(pInput," ") != strlen(pInput)) )
         {

            /*
             * Check whether the string value for the attribute contains "0x",
             * indicating the start of a hexadecimal value.
             */

            if ( (pHexString1 = strstr (pInput, "0x")) != NULL )
            {
               /*
                * The string contains "0x". Assume a hexadecimal value starts 
                * at pHexString1+2.
                * Decode the value using sscanf and trap a conversion error.
                */

               if (sscanf (pHexString1+2, "%lx", &lvalue) == 1)
               {
                  *(long *) pOutput = lvalue;
               }
               else
               {
                  ERROR_SET2 (S_epToVxLib_BAD_ATTRIBUTE,
                     "Failed to convert attribute %c from %s to long (hex)",
                     ERROR_LOG_SAVE, name, pInput);
                  if ( pMessage != NULL )
                  {
                     sprintf (pMessage, "Attrib %c conversion failure", name);
                  }
                  return (ERROR);
               }
            }
            else if ( (pHexString2 = strstr (pInput, "0X")) != NULL )
            {
               /*
                * The string contains "0X". Assume a hexadecimal value starts 
                * at pHexString2+2.
                * Decode the value using sscanf and trap a conversion error.
                */

               if (sscanf (pHexString2+2, "%lx", &lvalue) == 1)
               {
                  *(long *) pOutput = lvalue;
               }
               else
               {
                  ERROR_SET2 (S_epToVxLib_BAD_ATTRIBUTE,
                     "Failed to convert attribute %c from %s to long (hex)",
                     ERROR_LOG_SAVE, name, pInput);
                  if ( pMessage != NULL )
                  {
                     sprintf (pMessage, "Attrib %c conversion failure", name);
                  }
                  return (ERROR);
               }
            }
            else if ( (pHexString3 = strstr (pInput, "$")) != NULL )
            {
               /*
                * The string contains "$". Assume a hexadecimal value starts 
                * at pHexString3+2.
                * Decode the value using sscanf and trap a conversion error.
                */

               if (sscanf (pHexString3+2, "%lx", &lvalue) == 1)
               {
                  *(long *) pOutput = lvalue;
               }
               else
               {
                  ERROR_SET2 (S_epToVxLib_BAD_ATTRIBUTE,
                     "Failed to convert attribute %c from %s to long (hex)",
                     ERROR_LOG_SAVE, name, pInput);
                  if ( pMessage != NULL )
                  {
                     sprintf (pMessage, "Attrib %c conversion failure", name);
                  }
                  return (ERROR);
               }
            }
            else
            {
               /*
                * The string contains neither "0x", "0X" nor "$". Assume the 
                * value is decimal.
                * Decode the value using sscanf and trap a conversion error.
                */

               if (sscanf (pInput, "%ld", &lvalue) == 1)
               {
                  *(long *) pOutput = lvalue;
               }
               else
               {
                  ERROR_SET2 (S_epToVxLib_BAD_ATTRIBUTE,
                     "Failed to convert attribute %c from %s to long",
                     ERROR_LOG_SAVE, name, pInput);
                  if ( pMessage != NULL )
                  {
                     sprintf (pMessage, "Attrib %c conversion failure", name);
                  }
                  return (ERROR);
               }
            }
         }
         break;

      case (DBR_DOUBLE):

         /*
          * double attribute.
          * If the input attribute is completely blank it is probably unused,
          * so don't copy it at all.
          */

         if ( (strlen(pInput) != 0) && (strspn(pInput," ") != strlen(pInput)) )
         {
            /*
             * Decode the value using sscanf and trap a conversion error.
             */

            if (sscanf (pInput, "%lf", &dvalue) == 1)
            {
               *(double *) pOutput = dvalue;
            }
            else
            {
               ERROR_SET2 (S_epToVxLib_BAD_ATTRIBUTE,
                  "Failed to convert attribute %c from %s to double",
                  ERROR_LOG_SAVE, name, pInput);
               if ( pMessage != NULL )
               {
                  sprintf (pMessage, "Attrib %c conversion failure", name);
               }
               return (ERROR);
            }
         }
         break;

      case (DBR_STRING):

         /* String attribute. Copy it even if it is blank. */

         strncpy (pOutput, pInput, EPICS_MAX_BYTES_STRING_ATTRIB);
         break;

      default:

         ERROR_SET1 (S_epToVxLib_BAD_ATTRIBUTE, 
            "Unrecognised EPICS data type for attribute %c",
            ERROR_LOG_SAVE, name);
         if ( pMessage != NULL )
         {
            sprintf (pMessage, "Attrib %c invalid type", name);
         }
         return (ERROR);
         break;
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */


/*+
 *   IGNORED FUNCTION NAME:
 *   eptovx_readyToAcceptCmd
 *
 *   INVOCATION:
 *   eptovx_readyToAcceptCmd (pContext)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pContext   (CAD_CONTEXT)         CAD context structure
 *
 *   FUNCTION VALUE:
 *   (BOOL)   TRUE if epToVxLib is ready accept a CAD command, FALSE if
 *   not ready or if ioctl() fails on the CAD record's command pipe
 *
 *   PURPOSE:
 *   Test whether the pipe connected to a CAD record is ready to accept a 
 *   command
 *
 *   DESCRIPTION:
 *   This routine examines the pipe which connects a CAD record to a VxWorks
 *   control task in order to determine whether the pipe has space for a
 *   command packet to be written into it.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *   DEVELOPMENT NOTE:
 *   The logic of this function needs rethinking. It is all too easy for a 
 *   command to be rejected simply because the CAD command pipe is full. Why 
 *   not wait for the pipe to become ready and only report an error if a 
 *   timeout occurs? Urgent commands can have a short timeout.
 *   SMB - 8 Oct 1998.
 *-
 */

BOOL   eptovx_readyToAcceptCmd
   (
   CAD_CONTEXT pContext
   )
{
   int   numberMessagesInBuffer = 0;

   /*
    * Call ioctl() with function=FIONMSGS and numberMessagesInBuffer set to 
    * zero. This ensures that, if the pipe driver is mpPipeDrv (rather than 
    * the vxWorks standard pipe driver pipeDrv), the number of message-queue 
    * slots which remain occupied in the pipe is returned. 
    * If numberMessagesInBuffer were != 0 then the number of AVAILABLE 
    * message-queue slots would have been returned by mpPipeDrv, however 
    * pipeDrv ALWAYS returns the number of occupied slots irrespective of the 
    * value of numberMessagesInBuffer passed to ioctl().
    * Pipe driver mpPipeDrv is normally used only where the command-destination 
    * task resides on a different processor on VME bus than the one on which 
    * the EPICS data base resides, but the idea is to make this fact 
    * transparent at the level of the EPICS interface.
    */

   if (ioctl (pContext->cadToTaskPipeFd, FIONMSGS, 
              (int) & numberMessagesInBuffer) == ERROR)
   {
      ERROR_SET (0, "ioctl() execution failed while testing for pipe ready", 
                 ERROR_LOG_SAVE);
      return (FALSE);
   }

   if ((EPTOVX_CAD_CAR_PIPES_NMSGS - numberMessagesInBuffer) < 1)
   {

      /* The following line added by SMB. Nick was not reporting an error 
       * at this point. 
       */

      ERROR_SET (0, "Insufficient slots available in CAD command pipe", 
                 ERROR_LOG_SAVE);
      return (FALSE);
   }
   else
   {
      return (TRUE);
   }
}

/* -------------------------------------------------------------------------- */


/*+
 *   IGNORED FUNCTION NAME:
 *   eptovx_attribPutCmdPacket
 *
 *   INVOCATION:
 *   eptovx_attribPutCmdPacket (pContext, attribNumber, pAttribString)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pContext      (CAD_CONTEXT) CAD context structure
 *   (>) attribNumber  (uint32)      ID # for attribute to put
 *   (>) pAttribString (char *)      pointer to string in EPICS record
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the attribute cannot be converted
 *
 *   PURPOSE:
 *   Copy CAD attribute string into the command packet in a CAD context 
 *   structure
 *
 *   DESCRIPTION:
 *   This routine copies a CAD attribute (string) from an EPICS record into
 *   the command packet in a CAD context structure. On each call to this routine
 *   the size of the command packet (pContext->sizeOfCmdPacket) is incremented 
 *   by that of the attribute which is added to the command packet (or by zero 
 *   if the attribute value matches the its defined default). When this 
 *   routine is called with attribNumber=0, the size of the command packet is 
 *   first preset to that of the command-packet header, 
 *   CMD_PKT_HEADER_SIZE_BYTES. It is therefore assumed that, when copying 
 *   attributes into a command packet, attribute #0 is written first, then all 
 *   other attributes from #1 through to the number defined.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   It is assumed that pContext->sizeOfCmdPacket has already been initialised
 *   to CMD_PKT_HEADER_SIZE_BYTES and all the bits in the default mask have 
 *   already been cleared with CMD_PKT_DEFAULT_MASK (pContext) = 0.
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None
 *-
 */

STATUS eptovx_attribPutCmdPacket
   (
   CAD_CONTEXT pContext,
   uint32      attribNumber,
   char *      pAttribString
   )
{
   char *   pDestination;
   long     longValue;
   double   doubleValue;
   STATUS   returnValue = OK;
   char *   pHexString;

   /*
    * Test whether the string value for the attribute (*pAttribString) matches 
    * the magic string CAD_ATTRIBUTE_DEFAULT_STRING. If it is, then set the 
    * default bit for this attribute.
    */

   if (strcmp (pAttribString, CAD_ATTRIBUTE_DEFAULT_STRING) == 0)
   {
      CMD_PKT_DEFAULT_MASK (pContext) |= 1 << attribNumber;
   }
   else
   {
      /* Assume the destination address for this attribute is at the
       * current end of the command packet. (Note that pContext->sizeOfCmdPacket
       * will contain the command packet header size plus the space allocated to
       * all the previous attributes.
       */

      pDestination = pContext->pCmdPacket + pContext->sizeOfCmdPacket;

      /*
       * The attribute string does not match the magic string 
       * CAD_ATTRIBUTE_DEFAULT_STRING.
       * Copy the attribute, with appropriate type conversion, to the command 
       * packet. But, also test for the attribute value (after conversion) 
       * matching the default value (which may occur even though the attribute 
       * string does not match CAD_ATTRIBUTE_DEFAULT_STRING).
       * If a match is found, then set the attribute's bit in the Default Mask 
       * Word in the command header to indicate that the attribute may be 
       * overwritten by the next one. Also set the size of the command packet 
       * accordingly.
       */

      switch (pContext->pType [attribNumber])
      {
         case EPICS_DATA_TYPE_STRING:

            if (strcmp (pAttribString, 
                pContext->pDefault [attribNumber].pStringAttrib) != 0)
            {
               strncpy (pDestination, pAttribString, 
                        EPICS_MAX_BYTES_STRING_ATTRIB);
               pContext->sizeOfCmdPacket += EPICS_MAX_BYTES_STRING_ATTRIB;
            }
            else
            {
               CMD_PKT_DEFAULT_MASK (pContext) |= 1 << attribNumber;
            }
            break;

         case EPICS_DATA_TYPE_LONG:

            /*
             * Check whether the string value for the attribute contains "0x",
             * indicating the start of a hexadecimal value.
             */

            if ( (pHexString = strstr (pAttribString, "0x")) == NULL )
            {

               /* The string does not contain "0x". Assume the value is 
                * decimal. 
                */

               if (sscanf (pAttribString, "%ld", &longValue) != 1)
               {
                  /* Conversion failure. */
                  longValue = 0;
                  returnValue = ERROR;
               }
            }
            else
            {

               /* The string contains "0x". Assume a hexadecimal value starts 
                * at pHexString+2. 
                */

               if (sscanf (pHexString+2, "%lx", &longValue) != 1)
               {
                  /* Conversion failure. */
                  longValue = 0;
                  returnValue = ERROR;
               }
            }

            if (longValue != pContext->pDefault [attribNumber].longAttrib)
            {
               * (long *) (int) pDestination = longValue;
               pContext->sizeOfCmdPacket += sizeof (long);
            }
            else
            {
               CMD_PKT_DEFAULT_MASK (pContext) |= 1 << attribNumber;
            }
            break;

         case EPICS_DATA_TYPE_DOUBLE:

            if (sscanf (pAttribString, "%lf", &doubleValue) != 1)
            {
               /* Conversion failure. */
               doubleValue = 0.0;
               returnValue = ERROR;
            }
            if (doubleValue != pContext->pDefault [attribNumber].doubleAttrib)
            {
               * (double *) (int) pDestination = doubleValue;
               pContext->sizeOfCmdPacket += sizeof (double);
            }
            else
            {
               CMD_PKT_DEFAULT_MASK (pContext) |= 1 << attribNumber;
            }
      }
   }
   return (returnValue);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   eptovx_postEventsInputAttribs
 *
 *   INVOCATION:
 *   eptovx_postEventsInputAttribs (pContext, pcad)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pContext   (CAD_CONTEXT)         CAD context structure
 *   (>)   pcad      (struct cadRecord *)   CAD record structure
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Post events for the attribute inputs of a CAD record
 *
 *   DESCRIPTION:
 *   This routine calls the EPICS routine db_post_events() for each
 *   attribute input for a CAD record. It should be called after the
 *   input fields have been written to in order to update the record.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   This function assumes that the attribute value strings contained in the
 *   pcad structure are all contiguous and exactly EPICS_MAX_BYTES_STRING_ATTRIB
 *   bytes in size. The function will fail if the CAD record is changed so this
 *   assumption is no longer true. I am not sure if this function needed at all,
 *   since the record support of the CAD record ought to deal with this.
 *   SMB - 16 Mar 1998.
 *   
 *+
 */

void eptovx_postEventsInputAttribs
   (
   CAD_CONTEXT         pContext,
   struct cadRecord *   pcad
   )
{
   FAST uint32   i;
   char *      pAttrib;

   /* Initialise to attribute input "a" */

   pAttrib = pcad->a;

   /* Call db_post_events() for each attribute */

   for (i = 0; i < pContext->nAttrib; i++)
   {
      db_post_events (pcad, & pAttrib [i * EPICS_MAX_BYTES_STRING_ATTRIB], 1);
   }
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   eptovx_postEventsOutputAttribs
 *
 *   INVOCATION:
 *   eptovx_postEventsOutputAttribs (pContext, pcad)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pContext   (CAD_CONTEXT)         CAD context structure
 *   (>)   pcad      (struct cadRecord *)   CAD record structure
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Post events for the attribute outputs of a CAD record
 *
 *   DESCRIPTION:
 *   This routine calls the EPICS routine db_post_events() for each
 *   attribute output for a CAD record. It should be called after the
 *   output fields have been written to in order to update the record.
 *
 *   SUPPORT FOR THIS ROUTINE:
 *   This routine makes use of one or more EPICS libraries and can
 *   therefore only be used on a CPU which can call those EPICS libraries.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   This function assumes that the attribute value strings contained in the
 *   pcad structure are all contiguous and exactly EPICS_MAX_BYTES_STRING_ATTRIB
 *   bytes in size. The function will fail if the CAD record is changed so this
 *   assumption is no longer true. I am not sure if this function needed at all,
 *   since the record support of the CAD record ought to deal with this.
 *   SMB - 16 Mar 1998.
 *-
 */

void eptovx_postEventsOutputAttribs
   (
   CAD_CONTEXT         pContext,
   struct cadRecord *   pcad
   )
{
   FAST uint32   i;
   void **      ppAttrib;

   /* Initialise to attribute output "a" */

   ppAttrib = & pcad->vala;

   /* Call db_post_events() for each attribute   */

   for (i = 0; i < pContext->nAttrib; i++)
   {
      db_post_events (pcad, ppAttrib [i], 1);
   }
}

#endif /* NO_EPICS - END OF CODE COMPILED ONLY FOR THE EPICS ENVIRONMENT */

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxInit
 *
 *   INVOCATION:
 *   epToVxInit ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the symbol table could not be created
 *
 *   PURPOSE:
 *   Initialisation routine for epToVxLib. Must be called first
 *
 *   DESCRIPTION:
 *   This is the initialisation routine for the epToVxLib library. It
 *   creates a symbol table (epToVxSymtab) which is used by the library
 *   as a local database containing information about the EPICS records
 *   that the system is capable of interfacing to. This database should
 *   not be confused with the EPICS database which is normally loaded via
 *   the EPICS routine dbLoadRecords(), although some of the information
 *   in the local database will parallel that in the EPICS records. In the
 *   case of a multi-CPU system a local database is required on each CPU
 *   in the system, including any CPUs which do not support EPICS but which
 *   need to connect to EPICS records loaded on another CPU.
 *   The function also creates the mutual exclusion semaphore (epToVxCaDefSem) 
 *   which protects the channel access definition table.
 *
 *   EXTERNAL VARIABLES:
 *   (<) epToVxSymtab   (SYMTAB_ID) symbol table used internally by epToVxLib
 *   (<) epToVxCaDefSem (SEM_ID)    semaphore protecting channel access 
 *                                  definition table
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *   semLib.h
 *
 *   DEFICIENCIES:
 *   None
 *
 *   NOTE:
 *   This function calls taskLock() and taskUnlock(), but it so happens that 
 *   epToVxCadInit() also calls taskLock() before calling epToVxDbInitCadCar(), 
 *   which then calls this  function. Fortunately, VxWorks allows multiple 
 *   nested calls to taskLock() and taskUnlock().
 *   It would be nice to simplify this situation, though.
 *   SMB - 13 Mar 1998.
 *-
 */

STATUS epToVxInit (void)
{
   /*
    * Create a symbol table used to access EPICS records globally by name. 
    * Also create the semaphore used to control access to the channel access 
    * definition structure. taskLock() ensures that the table and semaphore 
    * are only created once on each processor.
    * (This function is potentially called from any of several different 
    * record-initialisation routines, but the table and semaphore should only 
    * be created during the first record-init routine which happens to get 
    * called).
    */

   taskLock ();
   if (epToVxSymtab == NULL)
   {
      if ((epToVxSymtab = 
           symTblCreate ((int) SYMTAB_HASHSIZE, FALSE, memSysPartId)) == NULL)
      {
         ERROR_SET (0, "epToVxSymtab symbol table creation failed", 
                    ERROR_LOG_SAVE);
         taskUnlock ();
         return (ERROR);
      }
   }

#ifndef NO_EPICS         /* Code compiled only for the EPICS environment */
   if (epToVxCaDefSem == NULL)
   {
      if ((epToVxCaDefSem = semMCreate( SEM_Q_FIFO | SEM_DELETE_SAFE )) == NULL)
      {
         ERROR_SET (0, "epToVxCaDefSem semaphore creation failed", 
                    ERROR_LOG_SAVE);
         taskUnlock ();
         return (ERROR);
      }
   }
#endif /* NO_EPICS */

   taskUnlock ();

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxDbInitCadCar
 *
 *   INVOCATION:
 *   epToVxDbInitCadCar ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if initialisation failed
 *
 *   PURPOSE:
 *   Initialise CAD and CAR records in the local database
 *
 *   DESCRIPTION:
 *   This routine initialises the local database (epToVxSymtab) with
 *   information about CAD and CAR records such that those CAD/CAR
 *   records listed in the array pWfsDbCadList[] may be accessed via epToVxLib.
 *   Memory is reserved for various data structures associated with each
 *   record and for the data packets used over the pipe interfaces between
 *   the records and VxWorks tasks. The validity of the record definitions
 *   given in the declaration of pWfsDbCadList[] is checked and CAD attribute
 *   default values are converted from strings to their native types
 *   (long, double or char*) for storage in this form.
 *
 *   EXTERNAL VARIABLES:
 *   (!) epToVxSymtab         (SYMTAB_ID)     symbol table used internally by 
 *                                            epToVxLib
 *   (>) pWfsDbCadList        (CAD_RECORD *)  array defining the system's CAD 
 *                                            records
 *   (>) wfsDbNCadRecord      (int)           number of CAD records known to 
 *                                            the system
 *   (!) pWfsDbRecInitialised (BOOL *)        array of record-initialisation-
 *                                            done flags
 *   (<) (ppCadContext)       (CAD_CONTEXT *) Not strictly a global variable, 
 *                                            but added to the epToVxSymtab 
 *                                            symbol table and therefore
 *                                            accessible outside this function.
 *
 *   PRIOR REQUIREMENTS:
 *   A list of CAD records should already have been defined in the 
 *   pWfsDbCadList[] data structures.
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *   wfsDb.h
 *
 *   DEFICIENCIES:
 *   It is possible for this function to screw up without reporting an error 
 *   if a data structure in pCadlist[] incorrectly defines the type of an 
 *   attribute. It is therefore important that pCadlist[] is checked carefully.
 *
 *   There is a race condition in which it is possible for this function
 *   to crash with an error message, but the crash itself prevents the
 *   error logging task from running and displaying the error message.
 *   All error messages are duplicated through "printErr" statements
 *   to ensure they appear at the console.
 *
 *   BUGS:
 *   If an error occurs during the execution of this function, the memory 
 *   allocated by this function is not freed. SMB - 8 Oct 1998.
 *-
 */

STATUS   epToVxDbInitCadCar (void)
{
   FAST int         i;
   FAST uint32      j;
   FAST uint32      k;
   int              temp;
   long             tempLong;
   double           tempDouble;
   CAD_CONTEXT *    ppCadContext;

   /* Initialise the library (if necessary) */

   if (epToVxInit () == ERROR)
   {
      printErr ("epToVxDbInitCadCar: Error initialising epToVxLib\n");
      ERROR_SET (0, "Error initialising epToVxLib", ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Allocate memory for an array of pointers to CAD context structures */

   ppCadContext = (CAD_CONTEXT *) calloc ((size_t) wfsDbNCadRecord, 
                                          sizeof (CAD_CONTEXT_STRUCT));
   if (ppCadContext == NULL)
   {
      printErr ("epToVxDbInitCadCar: Memory allocation for CAD context structure array failed\n");
      ERROR_SET (0, "Memory allocation for CAD context structure array failed",
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * For each CAD, extract the CAD definition from the array pWfsDbCadList[] 
    * and initialise the CAD's context structure.
    */

   for (i = 0; i < wfsDbNCadRecord; i++)
   {
      /* First, allocate memory for the context structure. */

      ppCadContext [i] = (CAD_CONTEXT) calloc (1, sizeof (CAD_CONTEXT_STRUCT));
      if (ppCadContext [i] == NULL)
      {
         printErr ("epToVxDbInitCadCar: "
            "Memory allocation for CAD context structure element failed\n");
         ERROR_SET (0, 
            "Memory allocation for CAD context structure element failed",
            ERROR_LOG_SAVE);
         return (ERROR);
      }

      /* Get the number of attributes defined for this CAD. */

      ppCadContext [i]->nAttrib = getNumberAttribs (pWfsDbCadList [i].pAttrib);

      /*
       * Add a symbol for the current CAD to the symbol table. This corresponds 
       * to an entry in the local database.
       */

      if (symAdd (epToVxSymtab, pWfsDbCadList [i].pRecordName, 
                  (char *) ppCadContext [i],
                  (SYM_TYPE) CAD_RECORD_TYPE, (UINT16) 0) == ERROR)
      {
         printErr (
            "epToVxDbInitCadCar: Failed to add CAD \"%s\" to symbol table\n",
            pWfsDbCadList [i].pRecordName);
         ERROR_SET1 (0, "Failed to add CAD \"%s\" to symbol table", 
                     ERROR_LOG_SAVE, pWfsDbCadList [i].pRecordName);
         return (ERROR);
      }

#ifdef DEBUG
      printf (
      "epToVxDbInitCadCar: Added CAD record %d = \"%s\" to symbol table\n", i,
      pWfsDbCadList [i].pRecordName);
#endif /* DEBUG */

      /* Copy the CAD definition from pWfsDbCadList[i] to its context 
         structure */

      ppCadContext [i]->pCadRecord = & pWfsDbCadList [i];
      ppCadContext [i]->clientId = 0;
      ppCadContext [i]->cadToTaskPipeFd = ERROR;  
                                /* Initialise assuming pipe doesn't exist yet */
      ppCadContext [i]->cadToCarPipeFd = ERROR;   
                                /* Initialise assuming pipe doesn't exist yet */
      ppCadContext [i]->timeout.tv_sec = 
      (time_t) ((int) pWfsDbCadList [i].timeoutSecs);
      ppCadContext [i]->timeout.tv_nsec = 
      (long) (1.0e09 * (pWfsDbCadList [i].timeoutSecs
                        - (double) ppCadContext [i]->timeout.tv_sec));
      ppCadContext [i]->presetDone = FALSE;
      ppCadContext [i]->stopDirSupported = pWfsDbCadList [i].stopDirSupported;
      ppCadContext [i]->simulationSupported = 
      pWfsDbCadList [i].simulationSupported;

      /* Allocate more memory for the context structure */

      ppCadContext [i]->pType =
      (uint32 *) calloc ((size_t) ppCadContext [i]->nAttrib, sizeof (uint32));
      if (ppCadContext [i]->pType == NULL)
      {
         printErr ("epToVxDbInitCadCar: Memory allocation for attribute types array failed\n");
         ERROR_SET (0, "Memory allocation for attribute types array failed", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }
      if ((ppCadContext [i]->pNumberRangeValues =
           (uint32 *) calloc ((size_t) ppCadContext [i]->nAttrib, 
                              sizeof (uint32))) == NULL)
      {
         printErr (
            "epToVxDbInitCadCar: Memory allocation for # attrib. range vals array failed\n");
         ERROR_SET (0, 
            "Memory allocation for # attrib. range vals array failed",
            ERROR_LOG_SAVE);
         return (ERROR);
      }
      if ((ppCadContext [i]->pDefault =
           (CAD_ATTRIB_VALUE *) calloc ((size_t) ppCadContext [i]->nAttrib,
                                        sizeof (CAD_ATTRIB_VALUE))) == NULL)
      {
         printErr ("epToVxDbInitCadCar: Memory allocation for attribute defaults union failed\n");
         ERROR_SET (0, "Memory allocation for attribute defaults union failed",
            ERROR_LOG_SAVE);
         return (ERROR);
      }
      if ((ppCadContext [i]->ppAllowedRange =
           (CAD_ATTRIB_VALUE **) calloc ((size_t) ppCadContext [i]->nAttrib,
                                         sizeof (CAD_ATTRIB_VALUE *))) == NULL)
      {
         printErr (
            "epToVxDbInitCadCar: Memory allocation for attribute range union array failed\n");
         ERROR_SET (0, 
            "Memory allocation for attribute range union array failed",
            ERROR_LOG_SAVE);
         return (ERROR);
      }

      /*
       * Obtain the name of the VxWorks task which is responsible for executing 
       * this CAD command.
       * This is referred to elsewhere as the "control task".
       * NOTE: The string itself is not copied, only a pointer to it.
       */

      ppCadContext [i]->pTaskName = pWfsDbCadList [i].pTaskName;

      /*
       * Now go through the list of attributes defined for this CAD 
       * (pWfsDbCadList[i].pAttrib[]) in order to calculate the total maximum 
       * size of the CAD command packet which is needed to contain the command 
       * header and attributes. Initialise the size of the command packet
       * to size of the command header. The size of each attrib is then added 
       * on to this initial value in the loop over all attributes.
       */

      ppCadContext [i]->maxSizeCmdPacket = CMD_PKT_HEADER_SIZE_BYTES;

      for (j = 0; j < ppCadContext [i]->nAttrib; j++)
      {

         /* Copy over the attribute type, then increment the command packet size
          * by the number of bytes required to store an attribute of this 
          * particular type.
          */

         ppCadContext [i]->pType [j] = pWfsDbCadList [i].pAttrib [j].type;

         switch (ppCadContext [i]->pType [j])
         {
            case (EPICS_DATA_TYPE_STRING):

               ppCadContext [i]->maxSizeCmdPacket += sizeof (char) *
                  EPICS_MAX_BYTES_STRING_ATTRIB;
               break;

            case (EPICS_DATA_TYPE_LONG):

               ppCadContext [i]->maxSizeCmdPacket += sizeof (long);
               break;

            case (EPICS_DATA_TYPE_DOUBLE):

               ppCadContext [i]->maxSizeCmdPacket += sizeof (double);
               break;

            default:

               printErr ("epToVxDbInitCadCar: Invalid attribute type for CAD record\n");
               ERROR_SET (S_epToVxLib_RECORD_DEFINITION_ERROR,
                          "Invalid attribute type for CAD record", 
                          ERROR_LOG_SAVE);
         }

         /* If the attribute type is a string, allocate memory for it */

         if (ppCadContext [i]->pType [j] == EPICS_DATA_TYPE_STRING)
         {
            if ((ppCadContext [i]->pDefault [j].pStringAttrib = 
                (char *) calloc ((size_t)
                (EPICS_MAX_BYTES_STRING_ATTRIB+1), sizeof (char))) == NULL)
            {
               printErr ("epToVxDbInitCadCar: Memory allocation for string attribute failed\n");
               ERROR_SET (0, "Memory allocation for string attribute failed", 
                          ERROR_LOG_SAVE);
               return (ERROR);
            }
         }

         /*
          * Store the default attribute value in the union used to hold 
          * attributes of different native types.
          */

         if (attribStringToUnion (ppCadContext [i]->pType [j],
                                  pWfsDbCadList [i].pAttrib [j].pDefault,
                             & ppCadContext [i]->pDefault [j])
             == ERROR)
         {
            printErr ("epToVxDbInitCadCar: CAD %d attribute %d - conversion failure\n", i, j);
            ERROR_SET2 (S_epToVxLib_BAD_ATTRIBUTE,
                        "CAD %d attribute %d - conversion failure", 
                        ERROR_LOG_SAVE, (int)i, (int)j);
            return (ERROR);
         }


         /*
          * Get the number of defined range values for the current attribute 
          * and check that only string attributes have more than 2 range 
          * values. This restriction applies because a long (or double) may 
          * have 1 range value if the attribute is only to have a single value,
          * or a long (or double) may have 2 range values if the attribute is 
          * restricted to lie between these 2 values. Alternatively, a
          * string attribute may often take one one of many different values 
          * and in this case the range values are set to allowed values for the 
          * attribute string rather than defining upper and lower limits on 
          * the string.
          */

         if ((temp = getNumberAttribRangeValues (pWfsDbCadList [i].pAttrib [j].pAllowedRange)) < 0)
         {
            ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);   
                                                 /* Error already reported.   */
            return (ERROR);
         }
         ppCadContext [i]->pNumberRangeValues [j] = temp;

         if ((ppCadContext [i]->pType [j] != EPICS_DATA_TYPE_STRING) &&
             (ppCadContext [i]->pNumberRangeValues [j] > 2))
         {
            printErr ("epToVxDbInitCadCar: Non-string CAD attrib has > 2 range values\n");
            ERROR_SET (S_epToVxLib_RECORD_DEFINITION_ERROR,
                       "Non-string CAD attrib. has > 2 range values", 
                       ERROR_LOG_SAVE);
            return (ERROR);
         }

         if (ppCadContext [i]->pNumberRangeValues [j] > 0)
         {
            /* Allocate memory for attribute range values */

            if ((ppCadContext [i]->ppAllowedRange [j] = 
               (CAD_ATTRIB_VALUE *) calloc ((size_t)
               ppCadContext [i]->pNumberRangeValues [j], 
               sizeof (CAD_ATTRIB_VALUE))) == NULL)
            {
               printErr (
                  "epToVxDbInitCadCar: Memory allocation for attribute range unions failed\n");
               ERROR_SET (0, 
                     "Memory allocation for attribute range unions failed",
                     ERROR_LOG_SAVE);
               return (ERROR);
            }

            /* If the attribute type is a string, allocate memory for its 
               range values */

            if (ppCadContext [i]->pType [j] == EPICS_DATA_TYPE_STRING)
            {
               for (k = 0; k < ppCadContext [i]->pNumberRangeValues [j]; k++)
               {
                  if ((ppCadContext [i]->ppAllowedRange [j][k].pStringAttrib =
                       (char *)calloc((size_t)(EPICS_MAX_BYTES_STRING_ATTRIB+1),
                                      sizeof (char))) == NULL)
                  {
                     printErr ("epToVxDbInitCadCar: "
                     "Memory allocation for attrib. range string vals failed\n");
                     ERROR_SET (0,
                     "Memory allocation for attrib. range string vals failed",
                     ERROR_LOG_SAVE);
                     return (ERROR);
                  }
               }
            }

            /*
             * Store the range values in the union used to hold attributes of 
             * different native types.
             */

            for (k = 0; k < ppCadContext [i]->pNumberRangeValues [j]; k++)
            {
               if (attribStringToUnion (ppCadContext [i]->pType [j],
                         pWfsDbCadList [i].pAttrib [j].pAllowedRange [k],
                         & ppCadContext [i]->ppAllowedRange [j][k])
                   == ERROR)
               {
                  printErr ("epToVxDbInitCadCar: "
                     "CAD %d attrib. %d range val. %d - conversion failure\n", 
                     i, j, k);
                  ERROR_SET3 (S_epToVxLib_BAD_ATTRIBUTE,
                     "CAD %d attrib. %d range val. %d - conversion failure",
                     ERROR_LOG_SAVE, (int)i, (int)j, (int)k);
                  return (ERROR);
               }
            }

            /*
             * If the attribute type is LONG or DOUBLE, and there are two 
             * permitted range values, swap the order in which they occur if 
             * necessary to ensure that they are listed in the order: low-range 
             * value followed by high-range value. This makes range-value 
             * checking on receipt of a CAD PRESET directive routine a bit 
             * faster.
             */

            if ((ppCadContext [i]->pType [j] == EPICS_DATA_TYPE_LONG) &&
                (ppCadContext [i]->pNumberRangeValues [j] == 2))
            {
               if (ppCadContext [i]->ppAllowedRange [j][0].longAttrib >
                  ppCadContext [i]->ppAllowedRange [j][1].longAttrib)
               {
                  tempLong = ppCadContext [i]->ppAllowedRange [j][0].longAttrib;

                  ppCadContext [i]->ppAllowedRange [j][0].longAttrib =
                        ppCadContext [i]->ppAllowedRange [j][1].longAttrib;

                  ppCadContext [i]->ppAllowedRange [j][1].longAttrib = tempLong;
               }
            }
            else if ((ppCadContext [i]->pType [j] == EPICS_DATA_TYPE_DOUBLE) &&
                    (ppCadContext [i]->pNumberRangeValues [j] == 2))
            {
               if (ppCadContext [i]->ppAllowedRange [j][0].doubleAttrib >
                  ppCadContext [i]->ppAllowedRange [j][1].doubleAttrib)
               {
                  tempDouble = 
                  ppCadContext [i]->ppAllowedRange [j][0].doubleAttrib;

                  ppCadContext [i]->ppAllowedRange [j][0].doubleAttrib =
                        ppCadContext [i]->ppAllowedRange [j][1].doubleAttrib;

                  ppCadContext [i]->ppAllowedRange [j][1].doubleAttrib = 
                  tempDouble;
               }
            }
         }
      }

      /*
       * Now allocate memory for the CAD command packet. This must be large 
       * enough to hold the largest possible command packet for this CAD record.
       */

      if ((ppCadContext [i]->pCmdPacket = (char *) calloc ((size_t) 1,
         (size_t) ppCadContext [i]->maxSizeCmdPacket + CMD_PKT_PADDING_BYTES)) 
         == NULL)
      {
         printErr ("epToVxDbInitCadCar: Memory allocation for CAD command packet failed\n");
         ERROR_SET (0, "Memory allocation for CAD command packet failed", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }

      /* Set the command number for this CAD in the header of the command 
         packet */

      if (pWfsDbCadList [i].commandNumber < 0)
      {
         printErr ("epToVxDbInitCadCar: Invalid CAD command number, %d\n",
            pWfsDbCadList [i].commandNumber);
         ERROR_SET1 (S_epToVxLib_RECORD_DEFINITION_ERROR, 
            "Invalid CAD command number, %d",
            ERROR_LOG_SAVE, pWfsDbCadList [i].commandNumber);
         return (ERROR);
      }
      CMD_PKT_COMMAND_NUMBER (ppCadContext [i]) = 
                              pWfsDbCadList [i].commandNumber;
   }

   /* All done, mark the CAD records as initialised OK */

   pWfsDbRecInitialised [CAD_RECORD_TYPE] = TRUE;
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxDbInitGensub
 *
 *   INVOCATION:
 *   epToVxDbInitGensub ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if initialisation failed
 *
 *   PURPOSE:
 *   Initialise genSub records in the local database
 *
 *   DESCRIPTION:
 *   This routine initialises the local database (epToVxSymtab) with
 *   information about genSub records such that those genSub
 *   records listed in the array pWfsDbGsubList[] may be accessed via epToVxLib.
 *
 *   EXTERNAL VARIABLES:
 *   (!) epToVxSymtab         (SYMTAB_ID)    symbol table used internally by 
 *                                           epToVxLib
 *   (>) pWfsDbGsubList       (CAD_RECORD *) array defining the system's genSub 
 *                                           records
 *   (>) wfsDbNGsubRecord     (int)          number of genSUb records known to 
 *                                           the system
 *   (!) pWfsDbRecInitialised (BOOL *)       array of record-initialisation-
 *                                           done flags
 *
 *   PRIOR REQUIREMENTS:
 *   A list of genSub records should already have been defined in the 
 *   pWfsDbGsubList[] data structures.
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *   wfsDb.h
 *
 *   DEFICIENCIES:
 *   There is a race condition in which it is possible for this function
 *   to crash with an error message, but the crash itself prevents the
 *   error logging task from running and displaying the error message.
 *   All error messages are duplicated through "printErr" statements
 *   to ensure they appear at the console.
 *-
 */

STATUS   epToVxDbInitGensub (void)
{
   FAST int         i;
   GSUB_CONTEXT *   ppGsubContext;

   /* Initialise the library (if necessary) */

   if (epToVxInit () == ERROR)
   {
      printErr ("epToVxDbInitGensub: Error initialising epToVxLib\n");
      ERROR_SET (0, "Error initialising epToVxLib", ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Allocate memory for an array of pointers to genSub context structures */

   if ((ppGsubContext = (GSUB_CONTEXT *) calloc ((size_t) wfsDbNGsubRecord,
         sizeof (GSUB_CONTEXT_STRUCT)))
       == NULL)
   {
      printErr ("epToVxDbInitGensub: "
         "Memory allocation for genSub context structure array failed\n");
      ERROR_SET (0, 
      "Memory allocation for genSub context structure array failed", 
      ERROR_LOG_SAVE);
      return (ERROR);
   }

   /*
    * For each genSub, extract the genSub definition from the array 
    * pWfsDbGsubList[] and initialise the genSub's context structure.
    */

   for (i = 0; i < wfsDbNGsubRecord; i++)
   {
      /* First, allocate memory for the context structure. */

      if ((ppGsubContext [i] = (GSUB_CONTEXT) calloc (1, 
          sizeof (GSUB_CONTEXT_STRUCT))) == NULL)
      {
         printErr ("epToVxDbInitGensub: Memory allocation for genSub context structure failed\n");
         ERROR_SET (0, "Memory allocation for genSub context structure failed",
                    ERROR_LOG_SAVE);
         return (ERROR);
      }

      /*
       * Add a symbol for the current genSub to the symbol table. 
       * This corresponds to an entry in the local database.
       */

      if (symAdd (epToVxSymtab, pWfsDbGsubList [i].pRecordName, 
                  (char *) ppGsubContext [i],
                  (SYM_TYPE) GENSUB_RECORD_TYPE, (UINT16) 0) == ERROR)
      {
         printErr ("epToVxDbInitGensub: Failed to add genSub \"%s\" to symbol table",
            pWfsDbGsubList [i].pRecordName);
         ERROR_SET1 (0, "Failed to add genSub \"%s\" to symbol table", 
            ERROR_LOG_SAVE, pWfsDbGsubList [i].pRecordName);
         return (ERROR);
      }

#ifdef DEBUG
      printf ("epToVxDbInitGensub: Added genSub record %d = \"%s\" to symbol table\n", i,
            pWfsDbGsubList [i].pRecordName);
#endif /* DEBUG */

      /* Copy the genSub definition from pWfsDbGsubList[i] to its context 
         structure */

      ppGsubContext [i]->pGsubRecord = & pWfsDbGsubList [i];
      ppGsubContext [i]->gensubToTaskPipeFd = ERROR; 
                                             /* Assume pipe doesn't exist yet */
      ppGsubContext [i]->timeout.tv_sec = 
      (time_t) ((int) pWfsDbGsubList [i].timeoutSecs);
      ppGsubContext [i]->timeout.tv_nsec = 
      (long) (1.0e09 * (pWfsDbGsubList [i].timeoutSecs
                        - (double) ppGsubContext [i]->timeout.tv_sec));

      /*
       * Obtain the name of the VxWorks task which is responds to changes in 
       * this genSub record, and the wavefront sensor associated with the 
       * record.
       * NOTE: The strings themselves are not copied, only a pointer to each 
       * string.
       */

      ppGsubContext [i]->pTaskName = pWfsDbGsubList [i].pTaskName;
      ppGsubContext [i]->pWfsName = pWfsDbGsubList [i].pWfsName;


      /*
       * Find out whether this is an input or output record, then obtain the 
       * number of values and check this is within the expected range.
       */

      ppGsubContext [i]->inputRecord = pWfsDbGsubList [i].inputRecord;

      if ( pWfsDbGsubList [i].inputRecord )
      {
         if ( (pWfsDbGsubList [i].nValues < 0) ||
              (pWfsDbGsubList [i].nValues > GSUB_MAX_INPUT_VALUES)
            )
         {
            ERROR_SET2 (S_epToVxLib_RECORD_DEFINITION_ERROR,
               "Invalid number of input values (%d) for genSub \"%s\"", 
               ERROR_LOG_SAVE,
               pWfsDbGsubList [i].nValues, pWfsDbGsubList [i].pRecordName);
            return (ERROR);
         }
      }
      else
      {
         if ( (pWfsDbGsubList [i].nValues < 0) ||
              (pWfsDbGsubList [i].nValues > GSUB_MAX_OUTPUT_VALUES)
            )
         {
            ERROR_SET2 (S_epToVxLib_RECORD_DEFINITION_ERROR,
               "Invalid number of output values (%d) for genSub \"%s\"", 
               ERROR_LOG_SAVE, pWfsDbGsubList [i].nValues, 
               pWfsDbGsubList [i].pRecordName);
            return (ERROR);
         }
      }

      ppGsubContext [i]->nValues = pWfsDbGsubList [i].nValues;

      /*
       * Now allocate memory for the data update packet. This must be large 
       * enough to hold the all the data values.
       */

      ppGsubContext [i]->sizeOfUpdatePacket = UPDATE_PKT_HEADER_SIZE_BYTES +
                    (ppGsubContext [i]->nValues * sizeof(double));

      if ((ppGsubContext [i]->pUpdatePacket = (char *) calloc ((size_t) 1,
         (size_t) ppGsubContext [i]->sizeOfUpdatePacket + 
         UPDATE_PKT_PADDING_BYTES)) == NULL)
      {
         printErr ("epToVxDbInitGensub: Memory allocation for data update packet failed\n");
         ERROR_SET (0, "Memory allocation for data update packet failed", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }

      /* Set the update number for this CAD in the header of the command 
         packet */

      if (pWfsDbGsubList [i].updateNumber < 0)
      {
         printErr ("epToVxDbInitGensub: Invalid data update number, %d\n",
            pWfsDbGsubList [i].updateNumber);
         ERROR_SET1 (S_epToVxLib_RECORD_DEFINITION_ERROR, 
            "Invalid data update number, %d",
            ERROR_LOG_SAVE, pWfsDbGsubList [i].updateNumber);
         return (ERROR);
      }
      UPDATE_PKT_ID_NUMBER (ppGsubContext [i])=pWfsDbGsubList [i].updateNumber;

   }

   /* All done, mark the genSub records as initialised OK */

   pWfsDbRecInitialised [GENSUB_RECORD_TYPE] = TRUE;
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxDbInitSir
 *
 *   INVOCATION:
 *   epToVxDbInitSir ()
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   None
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if initialisation failed
 *
 *   PURPOSE:
 *   Initialise SIR records in the local database
 *
 *   DESCRIPTION:
 *   This routine initialises the local database (epToVxSymtab) with
 *   information about SIR records such that those SIR
 *   records listed in the array pWfsDbSirList[] may be accessed via epToVxLib.
 *   Memory is reserved for various data structures associated with each
 *   record and for the data packets used over the pipe interfaces between
 *   the records and VxWorks tasks. The validity of the record definitions
 *   given in the declaration of pWfsDbSirList[] is checked.
 *
 *   EXTERNAL VARIABLES:
 *   (!) epToVxSymtab         (SYMTAB_ID)        symbol table used internally 
 *                                               by epToVxLib
 *   (>) pWfsDbSirList        (SIR_RECORD *)     array defining the system's 
 *                                               SIR records
 *   (>) wfsDbNSirRecord      (int)              number of SIR records known 
 *                                               to the system
 *   (!) pWfsDbRecInitialised (BOOL *)           array of record-initialisation-
 *                                               done flags
 *   (>) wfsDbEpicsDbIsLocal  (BOOL)             flag indicates whether EPICS 
 *                                               database is local
 *   (<) (pContext)           (DATREC_CONTEXT *) Not strictly a global variable, *                                               but added to the epToVxSymtab 
 *                                               symbol table and therefore
 *                                               accessible outside this 
 *                                               function.
 *
 *   PRIOR REQUIREMENTS:
 *   A list of SIR records should already have been defined in the 
 *   pWfsDbSirList[] data structures.
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *   wfsDb.h
 *
 *   DEFICIENCIES:
 *   There is a race condition in which it is possible for this function
 *   to crash with an error message, but the crash itself prevents the
 *   error logging task from running and displaying the error message.
 *   All error messages are duplicated through "printErr" statements
 *   to ensure they appear at the console.
 *-
 */

STATUS   epToVxDbInitSir (void)
{
   FAST int         i;
   int              dataPktNByte;
   DATREC_CONTEXT   pContext;

   /* Initialise the library (if necessary) */

   if (epToVxInit () == ERROR)
   {
      printErr ("epToVxDbInitSir: Error initialising epToVxLib\n");
      ERROR_SET (0, "Error initialising epToVxLib", ERROR_LOG_SAVE);
      return (ERROR);
   }

   /* Work through list of SIRs, pWfsDbSirList[] */

   for (i = 0; i < wfsDbNSirRecord; i++)
   {
      /*
       * Allocate memory for each SIR's "data-record" context structure and 
       * set the size of its data packet.
       */

      if ((pContext = (DATREC_CONTEXT) calloc (1, 
                      sizeof (DATREC_CONTEXT_STRUCT))) == NULL)
      {
         printErr ("epToVxDbInitSir: Memory allocation for SIR context structure failed\n");
         ERROR_SET (0, "Memory allocation for SIR context structure failed", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }

      switch (pWfsDbSirList [i].type)
      {
         case EPICS_DATA_TYPE_LONG:

            dataPktNByte = sizeof (long);
            break;

         case EPICS_DATA_TYPE_DOUBLE:

            dataPktNByte = sizeof (double);
            break;

         case EPICS_DATA_TYPE_STRING:

            dataPktNByte = EPICS_MAX_BYTES_STRING_ATTRIB;
            break;

         default:

            ERROR_SET (S_epToVxLib_RECORD_DEFINITION_ERROR, 
                       "Invalid EPICS data type for SIR",
                       ERROR_LOG_SAVE);
            return (ERROR);
      }

      dataPktNByte += DATA_PKT_HEADER_SIZE_BYTES;

      if ((pContext->pDataPacket =
           (char *) calloc (1, 
           (size_t) dataPktNByte + DATA_PKT_PADDING_BYTES)) == NULL)
      {
         printErr ("epToVxDbInitSir: Memory allocation for data packet failed\n");
         ERROR_SET (0, "Memory allocation for data packet failed", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }

      /*
       * Note the fact that this particular data record is a SIR type.
       * (Only SIR records are supported at the moment, but other types may 
       * be added).
       */

      pContext->recordType = SIR_RECORD_TYPE;

      /*
       * Copy the definition of the SIR from pWfsDbSirList[] to the 
       * data-record's context structure.
       */

      pContext->type = pWfsDbSirList [i].type;
      pContext->nElement = 1;
      pContext->recordId = (SIR_RECORD_TYPE & 0xffff) << 16 | (i & 0xffff);
      pContext->filterEnable = FALSE;
      pContext->filterParam1 = 0.0;
      pContext->caContext = NULL;
      pContext->hysteresisOnWrite = fabs (pWfsDbSirList [i].hysteresis);
      pContext->firstWriteDone = FALSE;

      /*
       * Initialise the header in the data packet for this record. NB The 
       * word DATA_PKT_MODE() is currently not used but is provided to 
       * support possible future requirements.
       */

      DATA_PKT_MODE (pContext) = 0;
      DATA_PKT_RECORD_ID (pContext) = pContext->recordId;
      DATA_PKT_N_ELEMENT (pContext) = pContext->nElement;

      /* Add symbol for current SIR to symbol table in local database */

      if (symAdd (epToVxSymtab, pWfsDbSirList [i].pRecordName, 
               (char *) pContext,
               (SYM_TYPE) SIR_RECORD_TYPE, (UINT16) 0) == ERROR)
      {
         printErr ("epToVxDbInitSir: Failed to add SIR \"%s\" to symbol table",
            pWfsDbSirList [i].pRecordName);
         ERROR_SET1 (0, "Failed to add SIR \"%s\" to symbol table", 
               ERROR_LOG_SAVE,
               pWfsDbSirList [i].pRecordName);
         return (ERROR);
      }

#ifdef DEBUG
      printf (
         "epToVxDbInitSir: Added SIR record %d = \"%s\" to symbol table\n", i,
         pWfsDbSirList [i].pRecordName);
#endif /* DEBUG */

   }

   /* All done, mark the SIR records as initialised OK */

   if (! wfsDbEpicsDbIsLocal) pWfsDbRecInitialised [SIR_RECORD_TYPE] = TRUE;
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxPipeInit
 *
 *   INVOCATION:
 *   epToVxPipeInit (procNumber)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   procNumber   (const int)   processor number for this CPU
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if initialisation failed
 *
 *   PURPOSE:
 *   Initialise the pipe(s) used to connect to EPICS data records
 *
 *   DESCRIPTION:
 *   Initialise the pipe(s) used to connect VxWorks tasks to data
 *   records (e.g. SIRs). Each CPU opens has one or more pipes dedicated
 *   to it for input/output from/to EPICS records. This routine creates
 *   and opens those pipes required by the CPU identified by procNumber.
 *   In most applications the argument, procNumber, will be the CPU
 *   number for CPU which calls this routine.
 *
 *   EXTERNAL VARIABLES:
 *   (!) epToVxPipeWriteFd   (int)  file descriptor used to write to EPICS 
 *                                  records
 *   (>) wfsDbEpicsDbIsLocal (BOOL) flag indicates whether EPICS database is 
 *                                  local
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   This routine currently only deals with the single pipe per CPU
 *   which is used to input data from EPICS records to VxWorks tasks.
 *   A separate pipe is likely to be needed to allow data to be output
 *   from VxWorks to EPICS records, but support for this latter pipe
 *   is not currently provided.
 *-
 */

STATUS   epToVxPipeInit
   (
   const int   procNumber
   )
{
   STATUS    (* pipeCreate) ();
   char      pNameExtension [4];


   /*
    * If it hasn't already been created, create the semaphore used to protect 
    * pipe used to communicate with the EPICS record daemon.
    */

   if (epToVxPipeWriteSem == NULL)
   {
      if ((epToVxPipeWriteSem = 
          semMCreate( SEM_Q_FIFO | SEM_DELETE_SAFE )) == NULL)
      {
         ERROR_SET (0, "epToVxPipeWriteSem semaphore creation failed", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }
   }

   /*
    * If it hasn't already been created, create the pipe used to communicate 
    * with the EPICS record daemon.
    */

   if (epToVxPipeWriteFd == ERROR)
   {

      /* Get exclusive access to the pipe (and prevent other tasks from 
       * trying to use it until it has been successfully created.
       */

      if (semTake(epToVxPipeWriteSem, NO_WAIT) == ERROR)
      {
         ERROR_SET (0, "Could not take epToVxPipeWriteSem semaphore", 
                    ERROR_LOG_SAVE);
         return (ERROR);
      }

      /*
       * If the EPICS database is local (not to be confused with the "local 
       * database"), then use the convention VxWorks pipe driver. Otherwise, 
       * the EPICS database is assumed to reside on a different CPU and the 
       * multi-processor pipe driver (mpPipeDrv) is used.
       */

      pipeCreate = pipeDevCreate;      


      /*
       * Each CPU is assigned a pipe with a name of the form "pipenameNN" 
       * where NN is the CPU number (00, 01, 02 etc..). Use sprintf() to 
       * construct the required name extension into a string such that it 
       * can be passed to the pipe-open routine (epToVxPipeOpen()).
       */

      sprintf (pNameExtension, "%.2d", procNumber);

      if ((epToVxPipeWriteFd = epToVxPipeOpen (FALSE, CA_WRITE_PIPE_NAME, 
                               pNameExtension, pipeCreate, 
                               DATA_PKT_PIPE_WRITE_NMSGS, 
                               DATA_PKT_PIPE_WRITE_MAX_BYTES, O_WRONLY,
                               -1, 0.0, 0.0)) == ERROR)
      {
         ERROR_SET (0, "Failed to open EPICS record daemon pipe", 
                    ERROR_LOG_NOW);
         semGive (epToVxPipeWriteSem);
         return (ERROR);
      }

      /* Release the pipe, allowing other tasks to use it. */

      semGive (epToVxPipeWriteSem);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */


/*+
 *   FUNCTION NAME:
 *   epToVxPipeOpen
 *
 *   INVOCATION:
 *   epToVxPipeOpen (fullNameProvided, pName, pNameExtension, pipeCreate,
 *                nMsgSlots, maxMsgSize, mode, ioctlFunction,
 *                timeoutPeriod, timeoutDelay)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) fullNameProvided (const BOOL)   TRUE if the full pipe name is provided
 *   (>) pName            (const char *) full pipe name or EPICS record name
 *   (<) pNameExtension   (const char *) optional name extension if full pipe
 *                                       name not provided
 *   (<) pipeCreate       (STATUS * ())  pipe-creation routine to use
 *                                       (or NULL if pipe already exists)
 *   (<) nMsgSlots        (const int)    # message slots for pipe create
 *   (<) maxMsgSize       (const int)    max message-slot size for pipe create
 *   (<) mode             (const int)    mode in which to open pipe
 *   (<) ioctlFunction    (const int)    optional ioctl() function
 *                                       (< 0 if none required)
 *   (<) timeoutPeriod    (const double) timeout period in seconds when opening
 *                                       pipe (=0 for no timeout, < 0 for 
 *                                       infinite timeout)
 *   (<) timeoutDelay     (const double) delay in sec between attempts to open 
 *                                       pipe
 *
 *   FUNCTION VALUE:
 *   (int) File descriptor on which pipe has been opened.
 *         ERROR if pipe could not be created or opened, or if
 *         the optional ioctl() function failed
 *
 *   PURPOSE:
 *   Open a pipe with a timeout and optionally create the pipe first
 *
 *   DESCRIPTION:
 *   This routine is a general-purpose utililty for opening pipes used in
 *   the epToVxLib system. If fullNameProvided is TRUE, the pipe name is
 *   given by pName, if fullNameProvided is FALSE then pName is assumed to
 *   contain the name of an EPICS record and the pipe name is generated
 *   according to this simple rule
 *v
 *v      pPipeName = "/pipe/" + pName + pNameExtension
 *v
 *   The pipe is created using the routine pointed to by pipeCreate via the
 *   invocation pipeCreate(pPipeName, nMsgSlots, maxMsgSize) where pPipeName
 *   is the adopted pipe name. If pipeCreate is NULL then the pipe is assumed
 *   to exist and the create routine is not called.
 *
 *   Once created, the pipe is opened in the specified mode (normally either
 *   O_RDONLY or O_WRONLY). Multiple attempts to open the pipe are made until
 *   the specified timeout period, timeoutPeriod, has expired with a pause
 *   of timeoutDelay in between each call to the open() routine.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *   mpPipeDrv.h
 *
 *   DEFICIENCIES:
 *   This function returns either a file descriptor or ERROR, and makes the 
 *   assumption that ERROR will never be a valid file descriptor.
 *
 *   NOTE:
 *   Is this function ever called with fullNameProvided=TRUE? If not it could be
 *   simplified by removing this option. SMB - 20 Mar 1998.
 *-
 */

int   epToVxPipeOpen
   (
   const BOOL     fullNameProvided,
   const char *   pName,
   const char *   pNameExtension,
   STATUS         (* pipeCreate) (),
   const int      nMsgSlots,
   const int      maxMsgSize,
   const int      mode,
   const int      ioctlFunction,
   const double   timeoutPeriod,
   const double   timeoutDelay
   )
{
   char         pPipeName[MP_PIPE_MAX_BYTES_NAME];
   int          fd = ERROR;

#ifdef DEBUG
   printf ("epToVxPipeOpen: %d \"%s\" \"%s\" %#x %d %d %d %#x %f %f\n", 
           fullNameProvided, pName, pNameExtension, 
           (int) pipeCreate, nMsgSlots, maxMsgSize, mode,
           (int) ioctlFunction, timeoutPeriod, timeoutDelay);
#endif


   /*
    * If the full pipe name is given, get on and use it. Otherwise, 
    * construct the name based on the EPICS record name and any 
    * pipe-name extension in pNameExtension.
    */

   if (fullNameProvided)
   {
      strncpy (pPipeName, pName, MP_PIPE_MAX_BYTES_NAME);
   }
   else
   {
      sprintf (pPipeName, "/pipe/%.*s%.16s", EPICS_MAX_BYTES_RECORD_NAME,
               pName, pNameExtension);
   }

   /* Create the pipe if required */

   if (pipeCreate != NULL)
   {
#ifdef DEBUG
      printf ( "epToVxPipeOpen: Creating pipe \"%s\" in %d mode\n", 
               pPipeName, mode );
#endif
      if (pipeCreate (pPipeName, nMsgSlots, maxMsgSize) == ERROR)
      {
         ERROR_SET3 (0, 
            "Failed to create pipe \"%s\" with nMsgSlots=%d, maxMsgSize=%d",
            ERROR_LOG_SAVE, pPipeName, nMsgSlots, maxMsgSize);
         return (ERROR);
      }
   }
#ifdef DEBUG
   else
   {
      printf ( "epToVxPipeOpen: Opening pipe \"%s\" in %d mode\n", 
               pPipeName, mode );
   }
#endif

   /* Wait, with timeout, until the pipe exists */

   if (waitPipeExists (pPipeName, timeoutPeriod, timeoutDelay) == ERROR)
   {
      ERROR_SET1 (0, "Timeout waiting for pipe \"%s\" to exist", 
                  ERROR_LOG_SAVE, pPipeName);
      return (ERROR);
   }

   /* Open the pipe in required mode. */

   if ((fd = open (pPipeName, mode, 0)) == ERROR)
   {
      ERROR_SET2 (0, "Error opening pipe \"%s\" in mode %d", 
                  ERROR_LOG_SAVE, pPipeName, mode);
      return (ERROR);
   }

   /* Execute any ioctl() function that was specified, if any */

   if (ioctlFunction >= 0)
   {
      if (ioctl (fd, ioctlFunction, 0) == ERROR)
      {
         ERROR_SET1 (0, "ioctl() execution failed after opening pipe \"%s\"", 
                     ERROR_LOG_SAVE, pPipeName);
         return (ERROR);
      }
   }

   return (fd);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxPipeWrite
 *
 *   INVOCATION:
 *   epToVxPipeWrite (pRecordName, pValue, pContextKnown)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordName   (char *)         name of EPICS record to write
 *   (!) pValue        (char *)         pointer to value to write
 *                                      (modified if filtering enabled)
 *   (>) pContextKnown (DATREC_CONTEXT) pointer to context structure for record
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the record has not been initialised or if
 *   the associated pipe could not be written
 *
 *   PURPOSE:
 *   Write to EPICS data record via the pipe-based interface
 *
 *   DESCRIPTION:
 *   This routine writes data to an EPICS record via the pipe-based interface
 *   to the record, and using the daemon task (see epToVxCaWriteDaemon()) which
 *   is responsible for maintaining all such records. A typical example of a
 *   "data record" is a SIR, to which a single value of type long, double or
 *   char* may be written. However the routine will support arbitrary data types
 *   (including arrays) providing an initialisation routine is provided
 *   for the record type and that the daemon task epToVxCaWriteDaemon() supports
 *   the record. The parameter pValue is points to the first element of data to
 *   write to the record and may, in general, point to an array of whatever type
 *   is relevant for the named record.
 *
 *   The record to write may be identified in one of two ways. Firstly, if 
 *   pRecordName is non-NULL, then it points to a string containing the EPICS 
 *   record name.
 *   In this case the name string may be either the full record name - including
 *   the name-prefix given by the macro TOP - or alternatively the name prefix 
 *   may be omitted. e.g. A record with the full name "system:recName" and 
 *   with the macro TOP defined as "system:", may be accessed with either of 
 *   *pRecordName="system:recName" or *pRecordName="recName". 
 *   In this mode the parameter pContextKnown should be NULL.
 *
 *   The second way of identifying the record to write is via its context
 *   structure, pContextKnown. If this is non-NULL (and pRecordName is NULL), 
 *   then pContextKnown is assumed to have been obtained previously via a 
 *   call to epToVxRecContextGet(). This mode is provided to avoid the overhead 
 *   of looking-up a record's context structure on successive calls to this 
 *   routine in applications in which a task is devoted to maintaining an 
 *   EPICS record via repeated calls to this routine.
 *
 *   HYSTERESIS ON RECORD UPDATES:
 *   Each data record has an associated hysteresis value. This determines the
 *   minimum change in the record value (for single-valued records only) that
 *   will trigger an update to the record. This feature allows the data traffic
 *   associated with record updates to be minimised in applications in which
 *   a record may potentially be updated by insignificant amounts at a 
 *   relatively high rate. In multi-CPU applications, each record update 
 *   requires a VME data transfer and it may then be desirable to limit 
 *   the data traffic in this way.
 *
 *   DATA FILTERING OPTION:
 *   Each data record may be assigned a set of filter parameters which define
 *   a filter to be applied to the data values that are passed via this routine.
 *
 *   EXTERNAL VARIABLES:
 *   (>) epToVxPipeWriteFd (int) file descriptor used to write to EPICS records
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   The filtering option described above is not currently implemented.
 *
 *   This function can currently only be used to write to SIR records.
 *
 *   DEVELOPMENT NOTE:
 *   This function is often called with pValue pointing at a constant.
 *   The fact that filtering can cause this value to be updated needs to
 *   be reconsidered. SMB - 17 Mar 1998.
 *-
 */

STATUS   epToVxPipeWrite
   (
   char *         pRecordName,
   char *         pValue,
   DATREC_CONTEXT pContextKnown
   )
{
   int            recordType;   /* Record type.                           */
   int            nByte=0;      /* Number of bytes written to pipe.       */
   long           valueLong;
   double         valueDouble;
   DATREC_CONTEXT pContext;     /* Record context structure.              */
   BOOL           updateFilteredValue = TRUE; 
                            /* Can only be set FALSE if filter is enabled */
   BOOL           hysteresisExceeded = FALSE;

   /* Get the context structure for the specified data record and obtain 
      the record type. */

   if (pContextKnown != NULL)
   {
      pContext = pContextKnown;
      recordType = pContext->recordType;
   }
   else if (epToVxRecContextGet (pRecordName, & pContext, & recordType) 
            == ERROR)
   {
      ERROR_SET1 (0, "Failed to get context structure for record, %s", 
                  ERROR_LOG_SAVE, pRecordName);
      return (ERROR);
   }

   /* The processing depends on the type of record.
    * At present only SIR records are supported.
    */

   switch (recordType)
   {
      case CAD_RECORD_TYPE:

         ERROR_SET (S_epToVxLib_INVALID_RECORD_TYPE, 
            "CAD record type not supported",
            ERROR_LOG_SAVE);
         return (ERROR);
         break;

      case CAR_RECORD_TYPE:

         ERROR_SET (S_epToVxLib_INVALID_RECORD_TYPE, 
            "CAR record type not supported",
            ERROR_LOG_SAVE);
         return (ERROR);
         break;

      case SIR_RECORD_TYPE:

         /* SIR records are supported. */

         break;

      /* Add support for other types of records here. */

      default:

         ERROR_SET (S_epToVxLib_INVALID_RECORD_TYPE, 
                    "Unrecognised record type", ERROR_LOG_SAVE);
         return (ERROR);
   }

   /* Do any filtering specified for this record */

   if (pContext->filterEnable) updateFilteredValue = filter (pContext, pValue);

   /*
    * Determine whether the EPICS record should be updated and get the 
    * number of bytes to write.
    */

   if ((pContext->type == EPICS_DATA_TYPE_STRING) || updateFilteredValue)
   {
      switch (pContext->type)
      {
         case EPICS_DATA_TYPE_LONG:

            valueLong = * (long *) (int) pValue;

            if ((! pContext->firstWriteDone) ||
                (fabs ((double) valueLong - pContext->lastWriteValue) >
                 pContext->hysteresisOnWrite))
            {
               hysteresisExceeded = TRUE;
               pContext->lastWriteValue = (double) valueLong;
               pContext->firstWriteDone = TRUE;
               * (long *) (int) DATA_PKT_VALUE_PTR (pContext) = valueLong;
               nByte = sizeof (long);
            }
            else
            {
               hysteresisExceeded = FALSE;
            }
            break;

         case EPICS_DATA_TYPE_DOUBLE:

            valueDouble = * (double *) (int) pValue;

            if ((! pContext->firstWriteDone) ||
                (fabs (valueDouble - pContext->lastWriteValue) > 
                 pContext->hysteresisOnWrite))
            {
               hysteresisExceeded = TRUE;
               pContext->lastWriteValue = (double) valueDouble;
               pContext->firstWriteDone = TRUE;
               * (double *) (int) DATA_PKT_VALUE_PTR (pContext) = valueDouble;
               nByte = sizeof (double);
            }
            else
            {
               hysteresisExceeded = FALSE;
            }
            break;

         case EPICS_DATA_TYPE_STRING:

            hysteresisExceeded = TRUE;

            strncpy (DATA_PKT_VALUE_PTR (pContext), pValue, 
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            nByte = strlen (DATA_PKT_VALUE_PTR (pContext));

            /*
             * Ensure the number of bytes never exceeds 
             * EPICS_MAX_BYTES_STRING_ATTRIB.
             * (I am not sure if this is necessary. It was added in an attempt 
             * to get around memory corruption problems). SMB - 6 July 1998
             */

            if ( nByte > EPICS_MAX_BYTES_STRING_ATTRIB ) 
               nByte = EPICS_MAX_BYTES_STRING_ATTRIB;

            /* Include EOS in byte count if the string is shorter than 
               the maximum permitted */

            if ( nByte < EPICS_MAX_BYTES_STRING_ATTRIB ) nByte++;
            break;

         default:

            ERROR_SET (S_epToVxLib_INTERNAL_ERROR, 
               "Unrecognised EPICS data type",
               ERROR_LOG_SAVE);
            return (ERROR);
      }

      /*
       * Write to the pipe which connects this routine to the daemon task 
       * epToVxCaWriteDaemon().
       */

      if (hysteresisExceeded)
      {
         nByte += DATA_PKT_HEADER_SIZE_BYTES;

         /* Get exclusive access to the pipe (and prevent other tasks from 
          * trying to write to it simultaneously).
          *
          * NOTE: I freely admit to ignorance on whether a pipe will look 
          * after itself if more than one task tries to write to it 
          * simultaneously. I have added a semaphore because all EPICS record 
          * updates are channeled through the one dameon, and simultaneous
          * accesses to this pipe are therefore very likely. The semaphore 
          * provides extra protection over and above that provided by the 
          * pipe itself. SMB - 26 Mar 1998.
          */

         if (semTake(epToVxPipeWriteSem, WAIT_FOREVER) == ERROR)
         {
            ERROR_SET (0, "Could not take epToVxPipeWriteSem semaphore", 
                       ERROR_LOG_SAVE);
            return (ERROR);
         }

         /*
          * N.B. The following write() could be replaced by an aioLib write, 
          * to ensure that it is non-blocking. N.Dillon.
          */

         if (write (epToVxPipeWriteFd, pContext->pDataPacket, nByte) != nByte)
         {
            ERROR_SET (0, "Number of bytes written to pipe does not match", 
                       ERROR_LOG_SAVE);
            semGive (epToVxPipeWriteSem);
            return (ERROR);
         }

         /* Release the pipe. */

         semGive (epToVxPipeWriteSem);
      }
   }
   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxSetHealth
 *
 *   INVOCATION:
 *   epToVxSetHealth (pRecordPrefix, pValue)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordPrefix (const char *) Prefix for "health" record name.
 *   (>) pValue        (char *)       Health value ("GOOD", "WARNING" or "BAD").
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the record name is not recognised or if
 *   channel-access to the record failed
 *
 *   PURPOSE:
 *   Update an EPICS health record
 *
 *   DESCRIPTION:
 *   This routine writes a new value to an EPICS health record. The name of
 *   the record is constructed from pRecordPrefix. If pRecordPrefix is NULL
 *   the record name is assumed to be "TOP:health". If pRecordPrefix contains
 *   the string "health" or "Health" the record name is "TOP:pRecordPrefix",
 *   otherwise the record name is constructed from "TOP:pRecordPrefix:health".
 *   TOP is the top level prefix defined in wfsDb.h. pValue contains a new
 *   health value, which should take one of the following values
 *
 *      GOOD     =>   The system is functioning normally.
 *      WARNING  =>   The system can still function, but there is a
 *                     problem which may affect the system's performance.
 *      BAD      =>   A problem has rendered the system inoperable.
 *
 *   The function is designed to be called when reporting an error, if
 *   that error affects the health of a system.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   The EPICS health record must have been initialised via a
 *   previous call to epToVxDbInitSir, and there should be an
 *   EPICS record daemon process running in processor 0.
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *   wfsDb.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   epToVxSetHealth
   (
   const char *   pRecordPrefix,
   char *         pValue
   )
{
   char   pRecordNameFull [EPICS_MAX_BYTES_RECORD_NAME + 1];

   /* Construct the full name of the health record.
    * If the record prefix is given as NULL use the name of the top level
    * health record, making the record name:
    *
    *   <TOP>:health
    *
    * If the string "health" or "Health" is found in the prefix, append
    * the given string given to the top level prefix, making the record name:
    *
    *   <TOP>:<pRecordPrefix>
    *
    * In other cases append the string ":health", making the record name:
    *
    *   <TOP>:<pRecordPrefix>:health
    *
    */

   if ( pRecordPrefix == NULL )
   {
      sprintf (pRecordNameFull, TOP "health" );
   }
   else if ( (strstr(pRecordPrefix, "Health") != NULL) || 
             (strstr(pRecordPrefix, "health") != NULL) )
   {
      sprintf (pRecordNameFull, TOP "%.*s", 
               (int) (EPICS_MAX_BYTES_RECORD_NAME - strlen(TOP)),
               pRecordPrefix);
   }
   else
   {
      sprintf (pRecordNameFull, TOP "%.*s:health",
               (int) (EPICS_MAX_BYTES_RECORD_NAME - strlen(TOP) - 7),
               pRecordPrefix);
   }
      
   /* Write the health value given to the record. */

#ifdef DEBUG
   printf( "epToVxSetHealth: Setting health %s to %s\n", pRecordNameFull, 
           pValue);
#endif /* DEBUG */

   if ( epToVxPipeWrite (pRecordNameFull, pValue, NULL) == ERROR )
   {
      ERROR_SET1 (0, "Error updating health record, %s", 
                  ERROR_LOG_SAVE, pRecordNameFull);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxCmdInit
 *
 *   INVOCATION:
 *   epToVxCmdInit (pTaskName, pipeCreate)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pTaskName   (const char *)  name of control task
 *   (>)   pipeCreate  (STATUS * ())   pipe-creation routine to use
 *
 *   FUNCTION VALUE:
 *   (CAD_CMD_CONTEXT)   Context structure to use subsequently as a handle
 *                       for CAD commands, or NULL if the function failed
 *
 *   PURPOSE:
 *   Initialise a control task prior to CAD command execution
 *
 *   DESCRIPTION:
 *   This is the initialisation routine for a control task in the epToVxLib
 *   system. It creates two pipes using the specified pipe-creation routine
 *   and subsequently uses these to read CAD-command packets and to write
 *   associated command-response packets to a CAR daemon task. The control task
 *   is a VxWorks task which handles the overall coordination of commands
 *   associated with a single CAR record; a control task may process commands
 *   from any number of CADs. The name of the control task, pTaskName, forms
 *   the basis of the command and response pipes and must agree with the task
 *   name given in the definition of the relevant CAD/CAR records in the
 *   declaration of the array pWfsDbCadList[]. The pipe names used are
 *v
 *v      Command pipe name   =   "/pipe/taskName_CadToTask"
 *v      Response pipe name   =   "/pipe/taskName_TaskToCar"
 *v
 *   where "taskName" is the string pointed to by pTaskName. If pTaskName is 
 *   NULL, this routine determines the name of the parent task (via taskLib) 
 *   and adopts this as "taskName".
 *
 *   EXTERNAL VARIABLES:
 *   (>) epToVxSymtab         (SYMTAB_ID)     symbol table used internally by 
 *                                            epToVxLib
 *   (>) pWfsDbCadList        (CAD_RECORD *)  array defining the system's CAD 
 *                                            records
 *   (>) wfsDbNCadRecord      (int)           number of CAD records known to the
 *                                            system
 *   (>) pWfsDbRecInitialised (BOOL *)        array of record-initialisation-
 *                                            done flags
 *   (>) (pOldContext)        (CAD_CONTEXT *) Not strictly a global variable, 
 *                                            but obtained from the 
 *                                            epToVxSymtab symbol table and 
 *                                            therefore a pointer to a globally 
 *                                            accessible data structure.
 *
 *   PRIOR REQUIREMENTS:
 *   A list of CAD records should already have been defined in the 
 *   pWfsDbCadList[] data structures.
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *   wfsDb.h
 *
 *   DEFICIENCIES:
 *   The fact that this function creates the command and response pipes means 
 *   that application tasks cannot easily be restarted if they crash.
 *-
 */

CAD_CMD_CONTEXT epToVxCmdInit
   (
   const char *   pTaskName,
   STATUS         (* pipeCreate) ()
   )
{
   FAST int            cadRecNum;
   int                 cmdNum;
   int                 highestCmdNum = -1;
   int                 nCommandsFound;
   uint32              maxSizeCmdPacket = 0;
   const char *        pTaskName1;
   CAD_CMD_CONTEXT     pCadCmdContext;
   CAD_CONTEXT         pOldContext;
   SYM_TYPE            symType;
   struct timespec     timeStart;

   /*
    * Get the name of this task.
    * If a name is not specified it is assumed to be the current task.
    */

   if (pTaskName == NULL)
   {
      pTaskName1 = taskName (taskIdSelf ());
   }
   else
   {
      pTaskName1 = pTaskName;
   }

   /*
    * For every CAD record known to the system (defined in pWfsDbCadList[]), 
    * test whether the control task assigned to the CAD matches the name of 
    * this task. Determine the highest command number assigned to this task.
    */

   for (cadRecNum = 0; cadRecNum < wfsDbNCadRecord; cadRecNum++)
   {
      if (strcmp (pWfsDbCadList [cadRecNum].pTaskName, pTaskName1) == 0)
      {
         if (pWfsDbCadList [cadRecNum].commandNumber > highestCmdNum)
            highestCmdNum = pWfsDbCadList [cadRecNum].commandNumber;
      }
   }

   if (highestCmdNum < 0)
   {
      ERROR_SET1 (S_epToVxLib_NO_RECS_FOUND_FOR_TASK, 
         "No CADs found for task %s",
         ERROR_LOG_SAVE, pTaskName1);
      return (NULL);
   }

   /*
    * Wait, with a timeout period specified, for the local EPICS data base 
    * to be initialised with CAD records.
    */

   START_TIMEOUT (& timeStart);
   while (! pWfsDbRecInitialised [CAD_RECORD_TYPE] &&
          ! timeoutExpired (EPTOVX_TIMEOUT_INITIALISE, & timeStart))
   {
      taskDelay (SEC_TO_NTICK (EPTOVX_DELAY_INITIALISE));
   }

   if (! pWfsDbRecInitialised [CAD_RECORD_TYPE])
   {
      ERROR_SET (S_epToVxLib_REC_INIT_TIMEOUT, 
         "Timeout waiting for CADs to initialise",
         ERROR_LOG_SAVE);
      return (NULL);
   }

   /*
    * Allocate memory for a CAD command context structure to be used by the 
    * calling task, and for each CAD context structure within the command 
    * structure.
    */
   pCadCmdContext = (CAD_CMD_CONTEXT) calloc ((size_t) 1, 
                                      sizeof (CAD_CMD_CONTEXT_STRUCT));
   if (pCadCmdContext == NULL)
   {
      ERROR_SET (0, "Memory allocation for CAD cmd context structure failed", 
                 ERROR_LOG_SAVE);
      return (NULL);
   }

   pCadCmdContext->ppCadContext =
      (CAD_CONTEXT *) calloc ((size_t) (highestCmdNum + 1), 
                              sizeof (CAD_CONTEXT_STRUCT));
   if (pCadCmdContext->ppCadContext == NULL)
   {
      ERROR_SET (0, "Memory allocation for CAD context structures failed", 
                 ERROR_LOG_SAVE);
      cfree ((char *) pCadCmdContext);
      return (NULL);
   }

   /* Set the highest command number */

   pCadCmdContext->highestCmdNumber = highestCmdNum;

#ifdef DEBUG
      printf ("epToVxCmdInit: Highest command for %s task is %d\n",
               pTaskName1, highestCmdNum);
#endif /* DEBUG */

   /*
    * Now sort through all CADs in the local database. For those whose 
    * assigned task name match that of the calling task, copy required parts 
    * of the CAD context structure from the local database to another copy of 
    * the context structure held in the CAD command structure.
    * This is done because the CAD context structure held in the local database 
    * is used by the CAD subroutine epToVxCadExecute() to hold a CAD command 
    * packet corresponding to a given invocation of the record. The CAD 
    * context structure held within the CAD command structure is used by 
    * a control task which is responsible for reading CAD command packets.
    */

   cmdNum = -1;
   nCommandsFound = 0;
   for (cadRecNum = 0; cadRecNum < wfsDbNCadRecord; cadRecNum++)
   {
#ifdef DEBUG
      printf ("epToVxCmdInit: record %d of %d = %s\n",
              cadRecNum+1, wfsDbNCadRecord,pWfsDbCadList[cadRecNum].pRecordName);
#endif /* DEBUG */

      if (strcmp (pWfsDbCadList [cadRecNum].pTaskName, pTaskName1) == 0)
      {
         nCommandsFound++;
         cmdNum = pWfsDbCadList [cadRecNum].commandNumber;

         if (symFindByNameAndType (epToVxSymtab, 
             pWfsDbCadList [cadRecNum].pRecordName,
             (char **) & pOldContext, (SYM_TYPE *) & symType, 
             (SYM_TYPE) CAD_RECORD_TYPE,
             SYM_TYPE_MASK) == ERROR)
         {
            ERROR_SET1 (0, "Could not find CAD \"%s\" in symbol table", 
                        ERROR_LOG_SAVE,
                        pWfsDbCadList [cadRecNum].pRecordName);
            cfree ((char *) pCadCmdContext->ppCadContext);
            cfree ((char *) pCadCmdContext);
            return (NULL);
         }

         pCadCmdContext->ppCadContext [cmdNum] = pOldContext;   
                                                   /* Why? SMB - 18 Mar 1998. */
         pCadCmdContext->ppCadContext [cmdNum]->nAttrib = pOldContext->nAttrib;
         pCadCmdContext->ppCadContext [cmdNum]->pType = pOldContext->pType;
         pCadCmdContext->ppCadContext [cmdNum]->pDefault = 
         pOldContext->pDefault;
         pCadCmdContext->ppCadContext [cmdNum]->timeout = pOldContext->timeout;

         if (pOldContext->maxSizeCmdPacket > maxSizeCmdPacket)
            maxSizeCmdPacket = pOldContext->maxSizeCmdPacket;
      }

      /* Abort the "for" loop when all the CADs for the calling task have 
         been found. */

      if (nCommandsFound == (highestCmdNum + 1)) break;
   }

   /*
    * Determine the size of the largest command packet that can be received by
    * the calling task.
    * Create a command pipe that can be written with such a packet by
    * epToVxCadExecute(). Also, create a response pipe that can be used to
    * write "command done" packets (response packets) to the CAR daemon.
    */

   pCadCmdContext->maxSizeCmdPacket = maxSizeCmdPacket;
   if (pipeCreate != NULL)
   {

#ifdef DEBUG
      printf ("epToVxCmdInit: Creating CAD_to_TASK and TASK_to_CAR pipes\n");
#endif /* DEBUG */

      if (((pCadCmdContext->cadPipeFd =
            epToVxPipeOpen (FALSE, pTaskName1, CAD_TO_TASK_PIPE_NAME_EXT,
                            pipeCreate, EPTOVX_CAD_CAR_PIPES_NMSGS, 
                            (int) maxSizeCmdPacket,
                            O_RDONLY, -1, 0.0, 0.0)) == ERROR) ||
      ((pCadCmdContext->carPipeFd =
        epToVxPipeOpen (FALSE, pTaskName1, TASK_TO_CAR_PIPE_NAME_EXT,
                        pipeCreate, EPTOVX_CAD_CAR_PIPES_NMSGS, 
                        CMD_PKT_HEADER_SIZE_BYTES,
                        O_WRONLY, -1, 0.0, 0.0)) == ERROR))
      {
         ERROR_SET (0, "Failed to create command/response pipes", 
                    ERROR_LOG_SAVE);
         cfree ((char *) pCadCmdContext->ppCadContext);
         cfree ((char *) pCadCmdContext);
         return (NULL);
      }
   }

   pCadCmdContext->pCmdPacket =
        (char *) calloc ((size_t) 1, 
                 (size_t) maxSizeCmdPacket + CMD_PKT_PADDING_BYTES);
   if (pCadCmdContext->pCmdPacket == NULL)
   {
      ERROR_SET (0, "Memory allocation for command done packet failed", 
                 ERROR_LOG_SAVE);
      cfree ((char *) pCadCmdContext->ppCadContext);
      cfree ((char *) pCadCmdContext);
      return (NULL);
   }

   return (pCadCmdContext);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxCmdFree
 *
 *   INVOCATION:
 *   epToVxCmdFree (pCadCmdContext)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pCadCmdContext (CAD_CMD_CONTEXT) Pointer to CAD command context 
 *                                        structure
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK if successful, ERROR if the resources could not be freed.
 *
 *   PURPOSE:
 *   Free the resources allocated to a control task
 *
 *   DESCRIPTION:
 *   This function frees the resources allocated to a control task by 
 *   epToVxCmdInit() and closes the pipes used by that task.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   This function does not delete the pipes created by a task.
 *-
 */

STATUS epToVxCmdFree
   (
   CAD_CMD_CONTEXT   pCadCmdContext
   )
{
   STATUS               returnValue;

   /*
    * Initialise the return value.
    */

   returnValue = OK;

   /*
    * Close the command and response pipes opened for this task.
    */

#ifdef DEBUG
   printf ("epToVxCmdFree: Closing CAD_to_TASK and TASK_to_CAR pipes\n");
#endif /* DEBUG */

   if ((close (pCadCmdContext->cadPipeFd) == ERROR) || 
       (close (pCadCmdContext->carPipeFd) == ERROR))
   {
      ERROR_SET (0, "Failed to close command/response pipes", ERROR_LOG_SAVE);
      returnValue = ERROR;
   }

   /*
    * Free the memory allocated for the command context structures.
    */

#ifdef DEBUG
   printf ("epToVxCmdFree: Freeing memory pCmdPacket=%#x ppCadContext=%#x pCadCmdContext=%#x\n",
      (int) pCadCmdContext->pCmdPacket, (int) pCadCmdContext->ppCadContext,
      (int) pCadCmdContext);
#endif /* DEBUG */

   if (cfree ((char *) pCadCmdContext->pCmdPacket) == ERROR)
   {
      ERROR_SET (0, "Failed to free memory allocated for command packets", 
                 ERROR_LOG_SAVE);
      returnValue = ERROR;
   }

   if (cfree ((char *) pCadCmdContext->ppCadContext) == ERROR)
   {
      ERROR_SET (0, "Failed to free memory allocated for CAD context", 
                 ERROR_LOG_SAVE);
      returnValue = ERROR;
   }

   if (cfree ((char *) pCadCmdContext) == ERROR)
   {
      ERROR_SET (0, "Failed to free memory allocated for CAD cmd context", 
                 ERROR_LOG_SAVE);
      returnValue = ERROR;
   }

   return (returnValue);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxCmdRead
 *
 *   INVOCATION:
 *   epToVxCmdRead (pCadCmdContext)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pCadCmdContext (CAD_CMD_CONTEXT)   context structure for CAD commands
 *
 *   FUNCTION VALUE:
 *   (int) command number, or -1 if an error occurred reading the command pipe
 *
 *   PURPOSE:
 *   Read the next CAD command packet from the command pipe
 *
 *   DESCRIPTION:
 *   This routine reads a CAD command packet from the pipe which connects
 *   the epToVxCadExecute() routine to a VxWorks control task. The routine
 *   will block indefinetely until a command packet is available to read
 *   in the pipe. The CAD attribute values are copied into the context
 *   structure for subsequent retreival via the following macro
 *v
 *v      EPTOVX_CAD_ATTRIB_GET(pCadCmdContext, cmdNum, attrib, pValue)
 *v
 *   where cmdNum is a valid command number (as returned by this routine),
 *   attrib is the number of the attribute to return (e.g. 0 for attribute "a",
 *   1 for attribute "b" etc) and pValue points to the destination address for
 *   the attribute. Alternatively, the routine epToVxCadAttribGet() provides
 *   a lower-level way of accessing CAD attributes.
 *
 *   CAD attribute values are converted from string format to the native type
 *   which is specified for the attribute in the definition of the array
 *   pWfsDbCadList[]. The types are supported by this library are long, double 
 *   and string (char*).
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   The routine epToVxCmdInit() must have been used to initialise 
 *   pCadCmdContext.
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

int   epToVxCmdRead
   (
   CAD_CMD_CONTEXT   pCadCmdContext
   )
{
   int nByte;
   int   cmdNumber;

   /* Read the command pipe */

   nByte = read (pCadCmdContext->cadPipeFd, pCadCmdContext->pCmdPacket,
                 pCadCmdContext->maxSizeCmdPacket);

   /* Get the current command number */

   cmdNumber = CMD_PKT_COMMAND_NUMBER (pCadCmdContext);

   /* Handle any errors */

   if (nByte < CMD_PKT_HEADER_SIZE_BYTES)
   {
      ERROR_SET1 (S_epToVxLib_INVALID_PACKET_READ, 
         "Unexpected command packet size, %d",
         ERROR_LOG_SAVE, nByte);
      cmdNumber = -1;
   }
   else if (! EPTOVX_IS_VALID_CMD_NUM (pCadCmdContext, cmdNumber))
   {
      ERROR_SET1 (S_epToVxLib_INVALID_COMMAND_NUM, 
         "Invalid command number, %d", ERROR_LOG_SAVE,
         cmdNumber);
      cmdNumber = -1;
   }

   return (cmdNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxCmdAttribGet
 *
 *   INVOCATION:
 *   epToVxCmdAttribGet (pContext, attribNumber, pAttribDest, pCmdPacket)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pContext     (CAD_CONTEXT)  CAD context structure
 *   (>)   attribNumber (uint32)       number of attribute to get
 *   (!)   pAttribDest  (char *)       where to put the attribute value
 *   (>)   pCmdPacket   (const char *) pointer to CAD command packet
 *                                     (NULL means use pContext->pCmdPacket).
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Get CAD attribute value from CAD command packet
 *
 *   DESCRIPTION:
 *   This routine extracts a CAD attribute value from a CAD command packet.
 *   Command packets are normally read from a pipe which has been written by
 *   the routine epToVxCadExecute() and contain information about the
 *   mode in which the command is to be executed (e.g. any simulation mode)
 *   as well as associated attribute values. In this case, he parameter
 *   pCmdPacket points to such a command packet. The routine is also called
 *   from the CAD subroutine epToVxCadExecute() whilst assembling a command
 *   packet and in this case pCmdPacket should be NULL and the pointer to the
 *   command packet will be extracted from the CAD context structure, pContext.
 *   The parameter attribNumber identifies the required attribute.
 *   The attribute value is copied to pAttribDest in the native type for the
 *   attribute (long, double or char*), as defined in the initialisation of
 *   the array pWfsDbCadList[].
 *
 *   See also the macro EPTOVX_CAD_ATTRIB_GET() which is described in the manual
 *   entry for epToVxCmdRead().
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   The routine epToVxCmdInit() must have been used to initialise 
 *   pCadCmdContext.
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *   DEVELOPMENT NOTE:
 *   This function was discovered to be updating pCmdPacket even though it was
 *   documented as input only and is often called with the constant NULL.
 *   Modified to use a local copy, pLocalCmdPacket. SMB - 16 Mar 1998.
 *-
 */

void epToVxCmdAttribGet
   (
   CAD_CONTEXT    pContext,
   uint32         attribNumber,
   char *         pAttribDest,
   const char *   pCmdPacket
   )
{
   static char *  pSource = NULL;
   uint32         defaultMask;
   char *         pLocalCmdPacket;

   /* Get pointer to the command packet */

   /* Why does this have to be so complicated? Why the two possible options 
    * and two variations
    * on CMD_PKT_DEFAULT_MASK? Can this be simplified? SMB - 16 Mar 1998.
    */

   if (pCmdPacket == NULL)
   {
      /* Get default mask from pContext->pCmdPacket */

      pLocalCmdPacket = pContext->pCmdPacket;
      defaultMask = CMD_PKT_DEFAULT_MASK (pContext);
   }
   else
   {
      /* Get default mask directly from specified pCmdPacket */

      pLocalCmdPacket = (char *) pCmdPacket;
      defaultMask = CMD_PKT_DEFAULT_MASK_1 (pLocalCmdPacket);
   }

   /* Check whether the default bit is set for this attribute. */

   if ((defaultMask & (1 << attribNumber)) == 0)
   {
      /*
       * Default attribute bit is not set: Copy attribute from command packet 
       * to destination.
       * pSource is the location where the attribute string is stored.
       */

      pSource = getAttribSourceAddrs (pContext, pLocalCmdPacket, attribNumber, 
                                      defaultMask);

      /* Copy attribute from the source, with appropriate type conversion */

      switch (pContext->pType [attribNumber])
      {
         case EPICS_DATA_TYPE_STRING:

            strncpy (pAttribDest, pSource, EPICS_MAX_BYTES_STRING_ATTRIB);
            break;

         case EPICS_DATA_TYPE_LONG:

            * (long *) (int) pAttribDest = * (long *) (int) pSource;
            break;

         case EPICS_DATA_TYPE_DOUBLE:

            * (double *) (int) pAttribDest = * (double *) (int) pSource;
            break;
      }
   }
   else
   {
      /*
       * The default bit is set for this attribute: Copy value from the 
       * default array rather than from the command packet, with appropriate 
       * type conversion
       */

      switch (pContext->pType [attribNumber])
      {
         case EPICS_DATA_TYPE_STRING:

            strncpy (pAttribDest, 
                     pContext->pDefault[attribNumber].pStringAttrib,
                     EPICS_MAX_BYTES_STRING_ATTRIB);
            break;

         case EPICS_DATA_TYPE_LONG:

            * (long *) (int) pAttribDest = 
            pContext->pDefault[attribNumber].longAttrib;
            break;

         case EPICS_DATA_TYPE_DOUBLE:

            * (double *) (int) pAttribDest = 
            pContext->pDefault[attribNumber].doubleAttrib;
            break;
      }
   }
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxCmdFinish
 *
 *   INVOCATION:
 *   epToVxCmdFinish (pCadCmdContext, errorNumber)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pCadCmdContext (CAD_CMD_CONTEXT) context structure for CAD commands
 *   (!) errorNumber    (uint32)          error number resulting from command 
 *                                        execution
 *                                        (modified in simulation mode)
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the response pipe could not be written.
 *
 *   PURPOSE:
 *   Generate a response to indicate the completion status of a CAD command
 *
 *   DESCRIPTION:
 *   This routine completes a CAD command/response sequence. It assembles a CAD
 *   response packet based on the last CAD record that was read via the last
 *   call to epToVxCmdRead() and the value of errorNumber. An errorNumber of
 *   zero indicates successful command completion whilst any non-zero value
 *   indicates an error. The response packet is written to a pipe which connects
 *   to the CAR daemon task, epToVxCarDaemon().
 *
 *   SIMULATION MODE:
 *   Commands received in FULL simulation mode (as indicated in
 *   the command packet), this routine will delay for an interval which is
 *   normally 1/10th of the specified timeout period for the relevant CAD
 *   (timeout periods are defined in the declaration and initialisation of the
 *   array pWfsDbCadList[]). A response message packet will then be written
 *   with the error number set to zero. thus simulating successful command
 *   completion. For those CADs that have an infinite timeout period the
 *   simulated time delay is zero seconds
 *
 *   Commands received in FAST simulation mode are treated in the same way
 *   as FULL simulation mode, except there is no simulated time delay.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   The routine epToVxCmdRead() must have been used to read the CAD
 *   command to which a response is generated.
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   epToVxCmdFinish
   (
   CAD_CMD_CONTEXT pCadCmdContext,
   uint32          errorNumber
   )
{
   int   cmdNumber;
   int   nTickDelay;


   /*
    * If the command was issued in FULL simulation mode, calculate a suitable
    * delay period that gives a reasonable approximation to the expected
    * command-execution time. (1/10th of the specified timeout period for
    * the command seems reasonable). Delay for this time and set the error
    * number to 0 (command successful).
    *
    * If the command was issued in FAST simulation mode, there is no delay
    * but the error number is still set to 0.
    */

   if (EPTOVX_IS_SIMULATION (pCadCmdContext, EPTOVX_SIM_MODE_FULL))
   {
      cmdNumber = CMD_PKT_COMMAND_NUMBER (pCadCmdContext);
      nTickDelay = 
      (int) ((pCadCmdContext->ppCadContext [cmdNumber]->timeout.tv_sec
      + pCadCmdContext->ppCadContext [cmdNumber]->timeout.tv_sec * 1.0e-09)
      * sysClkRateGet () / 10.0);
      if (nTickDelay >= (NO_TIMEOUT * sysClkRateGet () / 10)) nTickDelay = 0;

#ifdef DEBUG
      printf ("epToVxCmdFinish: simulating CAD command with delay = %d ticks\n",
              nTickDelay);
#endif /* DEBUG */

      taskDelay (nTickDelay);
      errorNumber = 0;
   }
   else if (EPTOVX_IS_SIMULATION (pCadCmdContext, EPTOVX_SIM_MODE_FAST))
   {

#ifdef DEBUG
      printf ("epToVxCmdFinish: simulating CAD command with no delay\n");
#endif /* DEBUG */

      errorNumber = 0;
   }

   /*
    * Overwrite the CAD command header's command-modifier word with the
    * "command done" bit set rather than with the "command begin" bit set.
    * Then copy the error number into the header and write the "command done"
    * packet to the CAR daemon via the allocated pipe.
    */

   CMD_PKT_COMMAND_MODIFIER (pCadCmdContext) = 
   (CMD_PKT_COMMAND_MODIFIER (pCadCmdContext)
   & ~CAD_COMMAND_MODE_MASK) | CAD_COMMAND_MODE_DONE;
   CMD_PKT_ERROR_NUMBER (pCadCmdContext) = errorNumber;

   if (write (pCadCmdContext->carPipeFd, 
              pCadCmdContext->pCmdPacket, CMD_PKT_HEADER_SIZE_BYTES)
      != CMD_PKT_HEADER_SIZE_BYTES)
   {
      ERROR_SET (0, "Failed to write command done packet to CAR pipe", 
                 ERROR_LOG_SAVE);
      return (ERROR);
   }

   return (OK);
}


/* ------------------------------------------------------------------------- */


/*+
 *   FUNCTION NAME:
 *   epToVxRecContextGet
 *
 *   INVOCATION:
 *   epToVxRecContextGet (pRecordName, pContext, pRecordType)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pRecordName  (char *)           record name
 *   (!) pContext     (DATREC_CONTEXT *) where to put the record's context 
 *                                       structure
 *   (!) pRecordType  (int *)            where to put the record type
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the record has not been previously initialised
 *
 *   PURPOSE:
 *   Get the context structure associated with a data record
 *
 *   DESCRIPTION:
 *   This routine is used to initialise the context structure associated with
 *   an EPICS data record. It is used internally by epToVxLib but is available
 *   in the public interface as an aid to debugging. Data records are those
 *   EPICS records used to transfer data (excluding commands and responses)
 *   between the EPICS and pure VxWorks environments; e.g. a SIR record is an
 *   example of a data record.  
 *
 *   The record name given in parameter pRecordName may be either the full EPICS
 *   record name, including the name prefix which is set in the macro TOP
 *   (TOP is normally defined in a file xxxDb.h where "xxx" identifies the
 *   system -  e.g. wfsDb.h in the case of the Gemini wavefront sensor system).
 *   Alternatively, the record's name prefix may be omitted.
 *   For example, an EPICS SIR record named "pwfs2:status" with the macro
 *   TOP = "pwfs2:" can legitimately be referred to via pRecordName) as
 *   either "pwfs2:status" or "status".
 *
 *   If pRecordType is non-NULL, then the type identifier for the record is
 *   written to this location.
 *
 *   EXTERNAL VARIABLES:
 *   (>) epToVxSymtab (SYMTAB_ID) symbol table used internally by epToVxLib
 *
 *   PRIOR REQUIREMENTS:
 *   Memory must have been previously allocated for the structure type-defined
 *   as DATREC_CONTEXT which is pointed to by the parameter pContext.
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   epToVxRecContextGet
   (
   char *           pRecordName,
   DATREC_CONTEXT * pContext,
   int *            pRecordType
   )
{
   SYM_TYPE  type;
   char      pRecordNameFull [EPICS_MAX_BYTES_RECORD_NAME + 1];

   /*
    * Search for the record name prefix, TOP, in the given record name. 
    * If not found concatenate TOP with pRecordName, otherwise just use 
    * pRecordName
    */

   if (strstr (pRecordName, TOP) == NULL)
   {
      sprintf (pRecordNameFull, TOP "%.*s", 
               (int) (EPICS_MAX_BYTES_RECORD_NAME - strlen(TOP)),
               pRecordName);
   }
   else
   {
      strncpy (pRecordNameFull, pRecordName, EPICS_MAX_BYTES_RECORD_NAME);
   }

#ifdef DEBUG
   printf ("epToVxRecContextGet: Looking up %s in symbol table.\n", 
           pRecordNameFull);
#endif

   /* Look-up the record */

   if (symFindByName (epToVxSymtab, pRecordNameFull, 
                      (char **) pContext, &type) == ERROR)
   {
      ERROR_SET1 (0, "Could not find \"%s\" in symbol table", 
                  ERROR_LOG_SAVE, pRecordNameFull);
      return (ERROR);
   }

   /* Only copy the record type to pRecordType if the pointer is non-NULL */

   if (pRecordType != NULL) *pRecordType = (int) type;

   return (OK);
}


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxUpdateInit
 *
 *   INVOCATION:
 *   epToVxUpdateInit (pWfsName, pTaskName, pipeCreate)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pWfsName   (const char *)  name of associated WFS
 *   (>)   pTaskName  (char *)        name of control task
 *   (>)   pipeCreate (STATUS * ())   pipe-creation routine to use
 *
 *   FUNCTION VALUE:
 *   (GSUB_DATA_CONTEXT) Context structure to use subsequently as a handle
 *                       for genSub data updates, or NULL if the function failed
 *
 *   PURPOSE:
 *   Initialise a control task prior to receiving genSub data updates
 *
 *   DESCRIPTION:
 *   This is the initialisation routine for a control task in the epToVxLib
 *   system which intends to receive continuous data updates from one or more
 *   genSub records. It creates a pipe using the specified pipe-creation routine
 *   and subsequently uses it to read genSub data update packets. The control
 *   task is a VxWorks task which handles the overall coordination of commands
 *   may receive data updates from any number of genSub records. The names of
 *   the wavefront sensor and control task, pWfsName and pTaskName, form the
 *   basis of the data update pipe and must agree with the WFS name and task
 *   name given in the definition of the relevant genSub records in the
 *   declaration of the array pWfsDbGsubList[]. The pipe name used is
 *v
 *v      Data update pipe name   =   "/pipe/wfsName:taskName_GsubToTask"
 *v
 *   where "wfsName" is the string pointed to by pWfsName and "taskName" is
 *   the string pointed to by pTaskName. If pTaskName is NULL, this routine
 *   determines the name of the parent task (via taskLib) and adopts this as
 *   "taskName". If pWfsName is NULL this routine assumes that all wavefront
 *   sensors are to be matched and constructs a pipe name without wfsName.
 *
 *   EXTERNAL VARIABLES:
 *   (>) epToVxSymtab         (SYMTAB_ID)        symbol table used internally
 *                                               by epToVxLib
 *   (>) pWfsDbGsubList       (GENSUB_RECORD *)  array defining the system's
 *                                               CAD records
 *   (>) wfsDbNGsubRecord     (int)              number of genSub records known
 *                                               to the system
 *   (>) pWfsDbRecInitialised (BOOL *)           array of record-initialisation-
 *                                               done flags
 *   (>) (pOldContext)        (GENSUB_CONTEXT *) Not strictly a global variable,
 *                                               but obtained from the epToVxSym
 *                                               tab symbol table and therefore
 *                                               a pointer to a globally
 *                                               accessible data structure.
 *
 *   PRIOR REQUIREMENTS:
 *   A list of genSub records should already have been defined in the 
 *   pWfsDbGsubList[] data structures.
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *   wfsDb.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

GSUB_DATA_CONTEXT epToVxUpdateInit
   (
   const char *   pWfsName,
   char *         pTaskName,
   STATUS         (* pipeCreate) ()
   )
{
   FAST int             gsubRecNum;
   int                  updateNum;
   int                  highestUpdateNum = -1;
   int                  nUpdatesFound;
   uint32               maxSizeUpdatePacket = 0;
   char *               pTaskName1;
   char                 pName[ EPICS_MAX_BYTES_STRING_ATTRIB + 1 ];
   GSUB_DATA_CONTEXT    pGsubUpdateContext;
   GSUB_CONTEXT         pOldContext;
   SYM_TYPE             symType;
   struct timespec      timeStart;

   /*
    * Get the name of this task.
    * If a name is not specified it is assumed to be the current task.
    */

   if (pTaskName == NULL)
   {
      pTaskName1 = taskName (taskIdSelf ());
   }
   else
   {
      pTaskName1 = pTaskName;
   }

#ifdef DEBUG
   printf ("epToVxUpdateInit: pWfsName=%s, pTaskName1=%s.\n", 
           pWfsName, pTaskName1);
#endif

   /*
    * For every genSub record known to the system (defined in pWfsDbGsubList[]),
    * test whether the WFS name and control task assigned to the genSub matches
    * WFS name provided and the name of this task. If the WFS name is NULL find
    * all the genSub records associated with this task. Determine the highest
    * data update ID number assigned to this task.
    */

   for (gsubRecNum = 0; gsubRecNum < wfsDbNGsubRecord; gsubRecNum++)
   {
      if ( pWfsName == NULL )
      {
         if (strcmp (pWfsDbGsubList [gsubRecNum].pTaskName, pTaskName1) == 0)
         {
            if (pWfsDbGsubList [gsubRecNum].updateNumber > highestUpdateNum)
               highestUpdateNum = pWfsDbGsubList [gsubRecNum].updateNumber;
         }
      }
      else
      {
         if ( (strcmp (pWfsDbGsubList [gsubRecNum].pTaskName, pTaskName1) == 0)
              && (strcmp (pWfsDbGsubList [gsubRecNum].pWfsName, pWfsName) == 0)
            )
         {
            if (pWfsDbGsubList [gsubRecNum].updateNumber > highestUpdateNum)
               highestUpdateNum = pWfsDbGsubList [gsubRecNum].updateNumber;
         }
      }
   }

   if (highestUpdateNum < 0)
   {
      ERROR_SET1 (S_epToVxLib_NO_RECS_FOUND_FOR_TASK, 
         "No genSubs found for task %s",
         ERROR_LOG_SAVE, pTaskName1);
      return (NULL);
   }

   /*
    * Wait, with a timeout period specified, for the local EPICS data base 
    * to be initialised with genSub records.
    */

   START_TIMEOUT (& timeStart);
   while (! pWfsDbRecInitialised [GENSUB_RECORD_TYPE] &&
          ! timeoutExpired (EPTOVX_TIMEOUT_INITIALISE, & timeStart))
   {
      taskDelay (SEC_TO_NTICK (EPTOVX_DELAY_INITIALISE));
   }

   if (! pWfsDbRecInitialised [GENSUB_RECORD_TYPE])
   {
      ERROR_SET (S_epToVxLib_REC_INIT_TIMEOUT, 
          "Timeout waiting for genSubs to initialise",
          ERROR_LOG_SAVE);
      return (NULL);
   }

   /*
    * Allocate memory for a genSub data update context structure to be used 
    * by the calling task, and for each genSub context structure within the 
    * data update structure.
    */

   pGsubUpdateContext = (GSUB_DATA_CONTEXT) calloc ((size_t) 1, 
                        sizeof (GSUB_DATA_CONTEXT_STRUCT));
   if (pGsubUpdateContext == NULL)
   {
      ERROR_SET (0, "Memory allocation for genSub data update structure failed",
                 ERROR_LOG_SAVE);
      return (NULL);
   }

   pGsubUpdateContext->ppGsubContext =
      (GSUB_CONTEXT *) calloc ((size_t) (highestUpdateNum + 1), 
      sizeof (GSUB_CONTEXT_STRUCT));
   if (pGsubUpdateContext->ppGsubContext == NULL)
   {
      ERROR_SET (0, "Memory allocation for genSub context structures failed", 
                 ERROR_LOG_SAVE);
      cfree ((char *) pGsubUpdateContext);
      return (NULL);
   }

   /* Set the highest data update ID number */

   pGsubUpdateContext->highestUpdateNumber = highestUpdateNum;

#ifdef DEBUG
      printf ("epToVxUpdateInit: Highest update ID for %s task is %d\n", 
         pTaskName1,
         highestUpdateNum);
#endif /* DEBUG */

   /*
    * Now sort through all genSubs in the local database. For those whose
    * assigned task name match that of the calling task, copy required parts
    * of the genSub context structure from the local database to another copy
    * of the context structure held in the genSub data update structure.
    * This is done because the genSub context structure held in the local
    * database is used by the genSub subroutine epToVxGensubInput() to hold
    * a data update packet corresponding to a given invocation of the record.
    * The genSub context structure held within the data update structure
    * is used by a control task which is responsible for reading data update
    * packets.
    */

   updateNum = -1;
   nUpdatesFound = 0;
   for (gsubRecNum = 0; gsubRecNum < wfsDbNGsubRecord; gsubRecNum++)
   {
#ifdef DEBUG
      printf ("epToVxUpdateInit: record %d of %d = %s\n",
            gsubRecNum+1, 
            wfsDbNGsubRecord,pWfsDbGsubList[gsubRecNum].pRecordName);
#endif /* DEBUG */

      if (strcmp (pWfsDbGsubList [gsubRecNum].pTaskName, pTaskName1) == 0)
      {
         nUpdatesFound++;
         updateNum = pWfsDbGsubList [gsubRecNum].updateNumber;

         if (symFindByNameAndType (epToVxSymtab, 
             pWfsDbGsubList [gsubRecNum].pRecordName,
             (char **) & pOldContext, (SYM_TYPE *) & symType, 
             (SYM_TYPE) GENSUB_RECORD_TYPE,
             SYM_TYPE_MASK) == ERROR)
         {
            ERROR_SET1 (0, "Could not find genSub \"%s\" in symbol table", 
               ERROR_LOG_SAVE,
               pWfsDbGsubList [gsubRecNum].pRecordName);
            cfree ((char *) pGsubUpdateContext->ppGsubContext);
            cfree ((char *) pGsubUpdateContext);
            return (NULL);
         }

         pGsubUpdateContext->ppGsubContext [updateNum] = pOldContext;
                                                   /* Why? SMB - 18 Mar 1998. */
         pGsubUpdateContext->ppGsubContext [updateNum]->inputRecord = 
         pOldContext->inputRecord;
         pGsubUpdateContext->ppGsubContext [updateNum]->nValues = 
         pOldContext->nValues;
         pGsubUpdateContext->ppGsubContext [updateNum]->timeout = 
         pOldContext->timeout;

         if (pOldContext->sizeOfUpdatePacket > maxSizeUpdatePacket)
            maxSizeUpdatePacket = pOldContext->sizeOfUpdatePacket;
      }

      /* Abort the "for" loop when all the genSubs for the calling task 
         have been found. */

      if (nUpdatesFound > highestUpdateNum) break;
   }

   /*
    * Determine the size of the largest data update packet that can be
    * received by the calling task.
    * Create a data update pipe that can be written with such a packet by
    * epToVxGensubInput().
    */

   pGsubUpdateContext->maxSizeUpdatePacket = maxSizeUpdatePacket;
   if (pipeCreate != NULL)
   {

      /* Construct a pipe name from the WFS name and task name. */

      if ( pWfsName == NULL)
      {
         strncpy (pName, pTaskName1, EPICS_MAX_BYTES_STRING_ATTRIB);
      }
      else
      {
         sprintf (pName, "%.8s:%.30s", pWfsName, pTaskName1);
      }

#ifdef DEBUG
      printf ("epToVxUpdateInit: Creating GENSUB_to_TASK pipe for %s\n", pName);
#endif /* DEBUG */

      if ((pGsubUpdateContext->gensubPipeFd =
            epToVxPipeOpen (FALSE, pName, GSUB_TO_TASK_PIPE_NAME_EXT,
                            pipeCreate, EPTOVX_GENSUB_PIPES_NMSGS, 
                            (int) maxSizeUpdatePacket,
                            O_RDONLY, -1, 0.0, 0.0)) == ERROR)
      
      {
         ERROR_SET (0, "Failed to create data update pipe", ERROR_LOG_SAVE);
         cfree ((char *) pGsubUpdateContext->ppGsubContext);
         cfree ((char *) pGsubUpdateContext);
         return (NULL);
      }
   }

   if ((pGsubUpdateContext->pUpdatePacket =
        (char *) calloc ((size_t) 1, 
        (size_t) maxSizeUpdatePacket + UPDATE_PKT_PADDING_BYTES)) == NULL)
   {
      ERROR_SET (0, "Memory allocation for data update packet failed", 
                 ERROR_LOG_SAVE);
      cfree ((char *) pGsubUpdateContext->ppGsubContext);
      cfree ((char *) pGsubUpdateContext);
      return (NULL);
   }

   return (pGsubUpdateContext);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxUpdateRead
 *
 *   INVOCATION:
 *   epToVxUpdateRead (pDataUpdateContext)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pDataUpdateContext (GSUB_DATA_CONTEXT) context structure for genSub 
 *                                              data updates
 *
 *   FUNCTION VALUE:
 *   (int) update ID number, or -1 if an error occurred reading the data 
 *         update pipe
 *
 *   PURPOSE:
 *   Read the next data update packet from a data update pipe
 *
 *   DESCRIPTION:
 *   This routine reads a data update packet from the pipe which connects
 *   the epToVxGensubInput() routine to a VxWorks control task. The routine
 *   will block indefinetely until a data update packet is available to read
 *   in the pipe.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   The routine epToVxUpdateInit() must have been used to initialise 
 *   pDataUpdateContext.
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

int   epToVxUpdateRead
   (
   GSUB_DATA_CONTEXT   pDataUpdateContext
   )
{
   int nByte;
   int updateNumber;

   /* Read the data update pipe */

   nByte = read (pDataUpdateContext->gensubPipeFd, 
                 pDataUpdateContext->pUpdatePacket,
                 pDataUpdateContext->maxSizeUpdatePacket);

   /* Get the current data update ID number */

   updateNumber = UPDATE_PKT_ID_NUMBER (pDataUpdateContext);

   /* Handle any errors */

   if (nByte < UPDATE_PKT_HEADER_SIZE_BYTES)
   {
      ERROR_SET1 (S_epToVxLib_INVALID_PACKET_READ, 
         "Unexpected update packet size, %d",
         ERROR_LOG_SAVE, nByte);
      updateNumber = -1;
   }

   return (updateNumber);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxShow
 *
 *   INVOCATION:
 *   epToVxShow (pRecordName, verbose)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pRecordName  (const char *) Record name
 *   (>)   verbose      (const BOOL)   Verbose output flag
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if the record has not been previously initialised
 *
 *   PURPOSE:
 *   Show information about an EPICS record
 *
 *   DESCRIPTION:
 *   This routine is used to print a summary of the definition of an EPICS
 *   record as contained in epToVxLib's local database (contained in the
 *   symbol table epToVxSymtab). Note that the information displayed reflects
 *   the definition of the record as it is known to epToVxLib; this is
 *   independent of any EPICS data structures such as those displayed via
 *   the EPICS utility routines dbl() and dbpr().
 *
 *   EXTERNAL VARIABLES:
 *   (>) epToVxSymtab  (SYMTAB_ID)     symbol table used internally by epToVxLib
 *   (>) (pCadContext) (CAD_CONTEXT *) Not strictly a global variable, but
 *                                     obtained from the epToVxSymtab symbol
 *                                     table and therefore a pointer to a
 *                                     globally accessible data structure.
 *   (>) (pGsubContext) (GSUB_CONTEXT *) Not strictly a global variable, but
 *                                     obtained from the epToVxSymtab symbol
 *                                     table and therefore a pointer to a
 *                                     globally accessible data structure.
 *   (>) (pCarContext) (CAR_CONTEXT *) Not strictly a global variable, but
 *                                     obtained from the epToVxSymtab symbol
 *                                     table and therefore a pointer to a
 *                                     globally accessible data structure.
 *   (>) (pSirContext) (DATREC_CONTEXT *) Not strictly a global variable, but
 *                                     obtained from the epToVxSymtab symbol
 *                                     table and therefore a pointer to a
 *                                     globally accessible data structure.
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   This routine supports only EPICS CAD, CAR, SIR and genSub records.
 *-
 */

STATUS   epToVxShow
   (
   const char *      pRecordName,
   const BOOL        verbose
   )
{
   SYM_TYPE         symType;
   CAD_CONTEXT      pCadContext;
   GSUB_CONTEXT     pGsubContext;

#ifndef NO_EPICS      /* Code compiled only for EPICS environment */
   CAR_CONTEXT      pCarContext;
#endif /* NO_EPICS */

   DATREC_CONTEXT   pSirContext;
   char             pRecordNameFull [EPICS_MAX_BYTES_RECORD_NAME + 1];

   /*
    * Search for the record name prefix, TOP, in the given record name. If
    * not found concatenate TOP with pRecordName, otherwise just use
    * pRecordName
    */

   if (strstr (pRecordName, TOP) == NULL)
   {
      sprintf (pRecordNameFull, TOP "%.*s", 
               (int) (EPICS_MAX_BYTES_RECORD_NAME - strlen(TOP)),
               pRecordName);
   }
   else
   {
      strncpy (pRecordNameFull, pRecordName, EPICS_MAX_BYTES_RECORD_NAME);
   }

   /*
    * Attempt to search for the record in the CAD, CAR, SIR and genSub symbol 
    * tables.
    */

   if (symFindByNameAndType (epToVxSymtab, pRecordNameFull, 
       (char **) & pCadContext,
       (SYM_TYPE *) & symType, (SYM_TYPE) CAD_RECORD_TYPE, SYM_TYPE_MASK) 
       != ERROR)
   {

      /*
       * The record has been found in the CAD symbol table. 
       * Print some information about it.
       */

      printf ("epToVxShow: CAD name = \"%s\"\n", pRecordNameFull);
      epToVxCadContextShow (pCadContext, verbose);
   }

#ifndef NO_EPICS      /* Code compiled only for EPICS environment */

   else if (symFindByNameAndType (epToVxSymtab, pRecordNameFull,
            (char **) & pCarContext, (SYM_TYPE *) & symType,
            (SYM_TYPE) CAR_RECORD_TYPE, SYM_TYPE_MASK) != ERROR)
   {
      /*
       * The record has been found in the CAR symbol table. 
       * Print some information about it.
       */

      printf ("epToVxShow: CAR name = \"%s\"\n", pRecordNameFull);
      epToVxCarContextShow (pCarContext, verbose);
   }
#endif /* NO_EPICS */

   else if (symFindByNameAndType (epToVxSymtab, pRecordNameFull,
            (char **) & pSirContext, (SYM_TYPE *) & symType,
            (SYM_TYPE) SIR_RECORD_TYPE, SYM_TYPE_MASK) != ERROR)
   {

      /*
       * The record has been found in the SIR symbol table. 
       * Print some information about it.
       */

      printf ("epToVxShow: SIR name = \"%s\"\n",  pRecordNameFull);
      epToVxSirContextShow (pSirContext, verbose);
   }

   else if (symFindByNameAndType (epToVxSymtab, pRecordNameFull,
            (char **) & pGsubContext, (SYM_TYPE *) & symType,
            (SYM_TYPE) GENSUB_RECORD_TYPE, SYM_TYPE_MASK) != ERROR)
   {

      /*
       * The record has been found in the genSub symbol table. 
       * Print some information about it.
       */

      printf ("epToVxShow: genSub name = \"%s\", used for ", pRecordNameFull);
      if ( pGsubContext->inputRecord )
      {
         printf ("INPUT\n");
      }
      else
      {
         printf ("OUTPUT\n");
      }

      printf ("epToVxShow: Number of values = %d\n", 
              (int)pGsubContext->nValues);

      printf ("epToVxShow: Associated task name = %s, timeout period = %e seconds\n",
      pGsubContext->pTaskName,
      pGsubContext->timeout.tv_sec + pGsubContext->timeout.tv_nsec * 1.0e-09);
      printf ("epToVxShow: genSub context structure located at: %#x\n", 
              (int) pGsubContext);
   }
   else
   {
      ERROR_SET1 (0, 
       "Could not find \"%s\" in CAD, CAR, SIR or genSub symbol tables",
       ERROR_LOG_NOW, pRecordNameFull);
      return (ERROR);
   }

   return (OK);
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxCadContextShow
 *
 *   INVOCATION:
 *   epToVxCadContextShow (pCadContext, verbose)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pCadContext  (CAD_CONTEXT)  Pointer to CAD context structure
 *   (>)   verbose      (const BOOL)   Verbose output flag
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Show information contained in CAD context structure
 *
 *   DESCRIPTION:
 *   This routine prints a summary of the contents of a CAD context structure.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known.
 *-
 */

void   epToVxCadContextShow
   (
   CAD_CONTEXT        pCadContext,
   const BOOL         verbose
   )
{
   FAST uint32         i;
   FAST uint32         j;


   printf (
   "epToVxCadContextShow: Contents of CAD context structure located at: %#x\n",
   (int) pCadContext);

   printf ("epToVxCadContextShow: Number of attributes = %d\n", 
           (int)pCadContext->nAttrib);
   printf ("epToVxCadContextShow: STOP directive is %ssupported, Simulation mode is %ssupported\n",
      pCadContext->stopDirSupported ? "" : "not ",
      pCadContext->simulationSupported ? "" : "not ");
   printf ("epToVxCadContextShow: Associated task name = %s, timeout period = %e seconds\n",
      pCadContext->pTaskName,
      (pCadContext->timeout.tv_sec + pCadContext->timeout.tv_nsec * 1.0e-09));

   /* Print a summary of the definition of each attribute */

   for (i = 0; i < pCadContext->nAttrib; i++)
   {
      switch (pCadContext->pType [i])
      {
         case EPICS_DATA_TYPE_STRING:

            printf ("epToVxCadContextShow: "
              "Attribute #%d: Type = STRING, Default Value = \"%s\"\n", (int)i,
              pCadContext->pDefault [i].pStringAttrib);

            if (pCadContext->pNumberRangeValues [i] > 0)
            {
               printf ("epToVxCadContextShow: Permitted string values:\n");

               for (j = 0; j < pCadContext->pNumberRangeValues [i]; j++)
               {
                  printf ("\t\"%s\"\n", 
                          pCadContext->ppAllowedRange [i][j].pStringAttrib);
               }
            }
            else
            {
               printf ("epToVxCadContextShow: "
                 "Permitted string values: not defined (any string allowed)\n");
            }
            break;

         case EPICS_DATA_TYPE_LONG:

            printf ("epToVxCadContextShow: Attribute #%d: Type = LONG, Default Value = %d, ", 
            (int)i, (int)(pCadContext->pDefault [i].longAttrib));

            if (pCadContext->pNumberRangeValues [i] == 0)
            {
               printf ("Permitted range is UNLIMITED\n");
            }
            else if (pCadContext->pNumberRangeValues [i] == 1)
            {
               printf ("Permitted range = %d to %d\n",
                  (int)(pCadContext->ppAllowedRange [i][0].longAttrib),
                  (int)(pCadContext->ppAllowedRange [i][0].longAttrib));
            }
            else if (pCadContext->pNumberRangeValues [i] == 2)
            {
               printf ("Permitted range = %d to %d\n",
                  (int)(pCadContext->ppAllowedRange [i][0].longAttrib),
                  (int)(pCadContext->ppAllowedRange [i][1].longAttrib));
            }
            break;

         case EPICS_DATA_TYPE_DOUBLE:

            printf ("epToVxCadContextShow: Attribute #%d: Type = DOUBLE, Default Value = %g, ", 
                    (int)i, pCadContext->pDefault [i].doubleAttrib);

            if (pCadContext->pNumberRangeValues [i] == 0)
            {
               printf ("Permitted range is UNLIMITED\n");
            }
            else if (pCadContext->pNumberRangeValues [i] == 1)
            {
               printf ("Permitted range = %g to %g\n\n",
                  pCadContext->ppAllowedRange [i][0].doubleAttrib,
                  pCadContext->ppAllowedRange [i][0].doubleAttrib);
            }
            else if (pCadContext->pNumberRangeValues [i] == 2)
            {
               printf ("Permitted range = %g to %g\n\n",
                  pCadContext->ppAllowedRange [i][0].doubleAttrib,
                  pCadContext->ppAllowedRange [i][1].doubleAttrib);
            }
      }
   }

   /*
    * In verbose mode show the contents of the channel access definition 
    * structure, if one exists.
    * N.B. At the moment this structure is not used with CAD records, so it will
    * always be NULL.
    * (The structure has to be cast to (CA_DEF) because it is actually
    * defined as (char *) in the pCadContext data structure definition.
    */

#ifndef NO_EPICS      /* Code compiled only for EPICS environment */

   if ( (verbose) && (pCadContext->caContext != NULL) )
   {
      epToVxCaShow ((CA_DEF) (int) pCadContext->caContext, verbose);
   }
#endif /* NO_EPICS */

   return;
}

#ifndef NO_EPICS      /* Code compiled only for EPICS environment */

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxCarContextShow
 *
 *   INVOCATION:
 *   epToVxCarContextShow (pCarContext, verbose)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pCarContext  (CAD_CONTEXT)  Pointer to CAR context structure
 *   (>)   verbose      (const BOOL)   Verbose output flag
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Show information contained in CAR context structure
 *
 *   DESCRIPTION:
 *   This routine prints a summary of the contents of a CAR context structure.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known.
 *-
 */

void   epToVxCarContextShow
   (
   CAR_CONTEXT        pCarContext,
   const BOOL         verbose
   )
{

   printf ("epToVxCarContextShow: Contents of CAR context structure located at: %#x\n",
           (int) pCarContext);

   printf ("epToVxCarContextShow: Associated task name = \"%s\"\n", 
           pCarContext->pTaskName);

   /*
    * In verbose mode show the contents of the channel access definition 
    * structure, if one exists.
    */

   if ( (verbose) && (pCarContext->caContext != NULL) )
   {
      epToVxCaShow ((CA_DEF) pCarContext->caContext, verbose);
   }

   return;
}

#endif /* NO_EPICS */


/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxSirContextShow
 *
 *   INVOCATION:
 *   epToVxSirContextShow (pSirContext, verbose)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pSirContext  (DATREC_CONTEXT)  Pointer to SIR context structure
 *   (>)   verbose      (const BOOL)      Verbose output flag
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Show information contained in SIR context structure
 *
 *   DESCRIPTION:
 *   This routine prints a summary of the contents of a SIR context structure.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known.
 *-
 */

void   epToVxSirContextShow
   (
   DATREC_CONTEXT     pSirContext,
   const BOOL         verbose
   )
{

   printf ("epToVxSirContextShow: Contents of SIR context structure located at: %#x\n",
           (int) pSirContext);

   printf ("epToVxSirContextShow: Record type = %d, data type = ", 
           pSirContext->recordType);
   if (pSirContext->type == EPICS_DATA_TYPE_STRING)
   {
      printf ("STRING\n");
   }
   else if (pSirContext->type == EPICS_DATA_TYPE_LONG)
   {
      printf ("LONG\n");
   }
   else if (pSirContext->type == EPICS_DATA_TYPE_DOUBLE)
   {
      printf ("DOUBLE\n");
   }
   else
   {
      printf ("UNDEFINED\n");
   }

   printf ("epToVxSirContextShow: Number of elements = %d, record ID = %d\n",
            (int)pSirContext->nElement, (int)pSirContext->recordId);

   if ( pSirContext->filterEnable )
   {
      printf ("epToVxSirContextShow: Filtering is ENABLED with parameters = %g\n",
              pSirContext->filterParam1);
   }
   else
   {
      printf ("epToVxSirContextShow: Filtering is DISABLED\n");
   }

   printf ("epToVxSirContextShow: Data packet structure located at: %#x.\n",
           (int) pSirContext->pDataPacket);

   if ( (pSirContext->type == EPICS_DATA_TYPE_LONG) ||
        (pSirContext->type == EPICS_DATA_TYPE_DOUBLE) )
   {

      printf ("epToVxSirContextShow: Hysteresis value = %g, ", 
              pSirContext->hysteresisOnWrite);

      if ( pSirContext->firstWriteDone )
      {
         printf ("The record has been written to with %g.\n", 
                 pSirContext->lastWriteValue);
      }
      else
      {
         printf ("The record has NOT been written to.\n");
      }
   }

   /*
    * In verbose mode show the contents of the channel access definition 
    * structure, if one exists.
    * (The structure has to be cast to (CA_DEF) because it is actually
    * defined as (char *) in the pSirContext data structure definition.
    */

#ifndef NO_EPICS      /* Code compiled only for EPICS environment */

   if ( (verbose) && (pSirContext->caContext != NULL) )
   {
      epToVxCaShow ((CA_DEF) (int) pSirContext->caContext, verbose);
   }
#endif /* NO_EPICS */

   return;
}

/* -------------------------------------------------------------------------- */

/*+
 *   FUNCTION NAME:
 *   epToVxGsubContextShow
 *
 *   INVOCATION:
 *   epToVxGsubContextShow (pSirContext, verbose)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pGsubContext (GSUB_CONTEXT)  Pointer to genSub context structure
 *   (>) verbose      (const BOOL)    Verbose output flag
 *
 *   FUNCTION VALUE:
 *   None
 *
 *   PURPOSE:
 *   Show information contained in genSub context structure
 *
 *   DESCRIPTION:
 *   This routine prints a summary of the contents of a genSub context structure
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   dbTypes.h
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known.
 *-
 */

void   epToVxGsubContextShow
   (
   GSUB_CONTEXT      pGsubContext,
   const BOOL        verbose
   )
{

   printf ("epToVxGsubContextShow: Contents of genSub context structure located at: %#x\n",
           (int) pGsubContext);

   printf ("epToVxGsubContextShow: Record used for ");
   if ( pGsubContext->inputRecord )
   {
      printf ("INPUT\n");
   }
   else
   {
      printf ("OUTPUT\n");
   }

   printf ("epToVxGsubContextShow: Number of values = %d\n", 
           (int)pGsubContext->nValues);

   printf ("epToVxGsubContextShow: Associated task name = \"%s\", timeout period = %e seconds\n",
      pGsubContext->pTaskName,
      (pGsubContext->timeout.tv_sec + pGsubContext->timeout.tv_nsec * 1.0e-09));

   return;
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   getAttribSourceAddrs
 *
 *   INVOCATION:
 *   getAttribSourceAddrs (pContext, pCmdPacket, attribNumber, defaultMask)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pContext      (CAD_CONTEXT) CAD context structure
 *   (>)   pCmdPacket    (char *)      pointer to CAD command packet
 *   (>)   attribNumber  (uint32)      number of attribute pointer to get
 *   (>)   defaultMask   (uint32)      default-value mask for the attributes
 *
 *   FUNCTION VALUE:
 *   (char *)   A pointer to the specified CAD attribute in the command packet
 *   pointed to by pCmdPacket, or NULL if the attribute is not contained within
 *   the command packet
 *
 *   PURPOSE:
 *   Get the address of a CAD attribute from a CAD command packet
 *
 *   DESCRIPTION:
 *   This routine returns the address at which a CAD attribute value is
 *   stored within a CAD command packet. The command packet will, in most
 *   applications have been read from the CAD's command pipe.
 *   An attribute will only be contained within a command packet if its
 *   value is not equal to the specified default value for the attribute.
 *   Where the attribute value does equal the default, this will be
 *   indicated in the parameter defaultMask which forms part of the command-
 *   packet header information. In this case this routine will return NULL.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

char *   getAttribSourceAddrs
   (
   CAD_CONTEXT   pContext,
   char *   pCmdPacket,
   uint32   attribNumber,
   uint32   defaultMask
   )
{
   char *      pSource;
   FAST uint32 attribute;


   /*
    * Test for the presence of the requested attribute in the command packet by
    * first examining the default mask word, defaultMask. This is a
    * 32-bit bit-field for which each bit corresponds to a CAD attribute
    * (e.g. bit #0 corresponds to attribute "a", bit #1 corresponds to
    * attribute "b" etc). When one of these bits is set for a given attibute
    * then the attribute value is equal to the default
    * defined for that attribute. The return value is NULL if the attribute is
    * not present. Otherwise, initialise the return value pSource to point
    * to the first attribute in the command packet then step through each
    * attribute, incrementing pSource by the size (in bytes) of those
    * attributes which are present and which occur before the requested
    * attribute in the command packet. pSource points to
    * the requested attribute when all preceeding ones have been stepped over.
    */

   if ((defaultMask & (1 << attribNumber)) == 1)
   {
      pSource = NULL;
   }
   else
   {
      pSource = pCmdPacket + CMD_PKT_HEADER_SIZE_BYTES;
      for (attribute = 0; attribute < attribNumber; attribute++)
      {
         if ((defaultMask & (1 << attribute)) == 0)
         {
            switch (pContext->pType [attribute])
            {
               case EPICS_DATA_TYPE_STRING:

                  pSource += EPICS_MAX_BYTES_STRING_ATTRIB;
                  break;

               case EPICS_DATA_TYPE_LONG:

                  pSource += sizeof (long);
                  break;

               case EPICS_DATA_TYPE_DOUBLE:

                  pSource += sizeof (double);
            }
         }
      }
   }

   return (pSource);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   filter
 *
 *   INVOCATION:
 *   filter (pContext, pValue)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pContext (DATREC_CONTEXT) pointer to data-record context structure
 *   (!) pValue   (char *)         pointer to data value
 *
 *   FUNCTION VALUE:
 *   (BOOL)   TRUE if the filter output has changed, FALSE if the output
 *   did not change
 *
 *   PURPOSE:
 *   Filter data prior to writing it to an EPICS data record
 *
 *   DESCRIPTION:
 *   This routine applies a generalised filter to the input data pValue
 *   and copies the filter output to pValue. The filter type is defined
 *   in the definition of the data record, e.g. in the array pWfsDbSirList[],
 *   which holds the filtered data. In the case of decimating filters
 *   (which are used to reduce the rate at which a variable changes),
 *   the filter output will not necessarily change each time a new input
 *   value arrives. The return value for the routine therefore indicates
 *   whether the filter output changed as a result of the applied input.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   Filtering is not currently implemented. This routine always returns
 *   TRUE in the current version of epToVxLib.
 *-
 */

BOOL   filter
   (
   DATREC_CONTEXT pContext,
   char *         pValue
   )
{

   /* Insert filtering code here. */

   return (TRUE);
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   getNumberAttribs
 *
 *   INVOCATION:
 *   getNumberAttribs (pAttribList)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pAttribList (CAD_ATTRIB *) pointer to list of CAD attribute definitions
 *
 *   FUNCTION VALUE:
 *   (uint32)   Number of attributes defined for the CAD record
 *
 *   PURPOSE:
 *   Get the number of attributes defined for a CAD record
 *
 *   DESCRIPTION:
 *   This routine returns the number of attributes that are defined for
 *   a CAD record. The list of attributes, pAttribList, is normally a member
 *   of the structure array pWfsDbCadList[] - the declaration & initialisation 
 *   of which serves as the definition of each CAD record in the system.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

uint32   getNumberAttribs
   (
   CAD_ATTRIB *   pAttribList
   )
{
   FAST uint32   i;
   FAST uint32   j;

   /*
    * The array of structures pAttribList[] is initialised to zero
    * (all elements) unless the declaration of the array in the
    * initialisation/declaration of pWfsDbCadList[] contains one or more
    * attribute definitions, each definition being identified by the structure
    * member "number" which acts as a tag to identify each attribute.
    * The first attribute defined in such a declaration is attribute "a",
    * with number = CAD_ATTRIB_A; the next is "b"
    * with number = CAD_ATTRIB_B (= CAD_ATTRIB_A + 1) etc..
    *
    * Sort through the attribute list until the first un-initialised entry
    * (number = 0) is found. This locates the number of defined attributes.
    */

   for (i = 0, j = CAD_ATTRIB_A; i < CAD_MAX_N_ATTRIB; i++, j++)
   {
      if (pAttribList [i].number != j) return (j - 1);
   }

   return (i);
}

/* ------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   attribStringToUnion
 *
 *   INVOCATION:
 *   attribStringToUnion (type, pStringAttrib, pAttrib)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) type          (uint32)             attribute type identifier
 *   (>) pStringAttrib (char *)             pointer to attribute string
 *   (>) pAttrib       (CAD_ATTRIB_VALUE *) pointer to union of attribute values
 *
 *   FUNCTION VALUE:
 *   (STATUS)   OK, or ERROR if a conversion failure occurs
 *
 *   PURPOSE:
 *   Convert an attribute string to its native type and store in a union
 *
 *   DESCRIPTION:
 *   This routine converts a CAD attribute string from pStringAttrib to its 
 *   native type (long, double or char*) and stores the result in the union 
 *   pAttrib.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *
 *   DEVELOPMENT NOTES:
 *   There may be problems in setting attribute values to their "most negative
 *   value". prolint has reported a potential overflow problem when attempting
 *   to set an attribute value to LONG_MIN. Unfortunately VxWorks doesn't
 *   provide a constant for "most negative double value", and attempting to
 *   use -DBL_MAX makes an assumption about how the hardware architecture
 *   handles doubles. Although this is a potential problem, the function does
 *   seem to work. SMB - 29 Jan 1998.
 *
 *   The success or failure of the conversion from string to long or double is
 *   not checked. strtol() and strtod() could be replaced by sscanf(),
 *   which returns a status. SMB - 13 Mar 1998.
 *-
 */

STATUS attribStringToUnion
   (
   uint32               type,
   char *               pStringAttrib,
   CAD_ATTRIB_VALUE *   pAttrib
   )
{
   long   longValue;
   double doubleValue;
   long   returnValue = OK;
   char * pHexString;

   switch (type)
   {
      /*
       * Copy the attribute string into the relevent member of the attribute
       * union, pAttrib. If the attribute is a long or double, and the
       * attribute string matches one of the magic values used to identify
       * no lower or lower limit on the attribute, then adopt
       * the minimum or maximum value allowed for this particular data type.
       */

      case EPICS_DATA_TYPE_STRING:

         strncpy (pAttrib->pStringAttrib, pStringAttrib, 
                  EPICS_MAX_BYTES_STRING_ATTRIB);
         break;

      case EPICS_DATA_TYPE_LONG:

         if (strcmp (pStringAttrib, NO_LO_LIMIT) == 0)
         {
            pAttrib->longAttrib = LONG_MIN;      
            /* prolint suggests this results in overflow */
         }
         else if (strcmp (pStringAttrib, NO_HI_LIMIT) == 0)
         {
            pAttrib->longAttrib = LONG_MAX;
         }
         else
         {

            /*
             * Check whether the attribute string contains "0x",
             * indicating the start of a hexadecimal value.
             */

            if ( (pHexString = strstr (pStringAttrib, "0x")) == NULL )
            {
               /* The string does not contain "0x". Assume the value is 
                  decimal. */

               if (sscanf (pStringAttrib, "%ld", &longValue) != 1)
               {
                  /* Conversion failure. */
                  pAttrib->longAttrib = 0;
                  returnValue = ERROR;
               }
               else
               {
                  pAttrib->longAttrib = longValue;
               }
            }
            else
            {

               /* The string contains "0x". Assume a hexadecimal value 
                  starts at pHexString+2. */

               if (sscanf (pHexString+2, "%lx", &longValue) != 1)
               {
                  /* Conversion failure. */
                  pAttrib->longAttrib = 0;
                  returnValue = ERROR;
               }
               else
               {
                  pAttrib->longAttrib = longValue;
               }
            }
         }
         break;

      case EPICS_DATA_TYPE_DOUBLE:

         if (strcmp (pStringAttrib, NO_LO_LIMIT) == 0)
         {
            pAttrib->doubleAttrib = -DBL_MAX;   
                                  /* Assuming -DBL_MAX is the most negative   */
                                  /* double may be a dodgy thing to do.       */
                                  /* SMB - 29 Jan 1998.                       */
         }
         else if (strcmp (pStringAttrib, NO_HI_LIMIT) == 0)
         {
            pAttrib->doubleAttrib = DBL_MAX;
         }
         else
         {
            if (sscanf (pStringAttrib, "%lf", & doubleValue) != 1)
            {
               /* Conversion failure. */
               pAttrib->doubleAttrib = 0.0;
               returnValue = ERROR;
            }
            else
            {
               pAttrib->doubleAttrib = doubleValue;
            }
         }
         break;
   }
   return (returnValue);
}

/* -------------------------------------------------------------------------- */


/*+
 *   IGNORED FUNCTION NAME:
 *   getNumberAttribRangeValues
 *
 *   INVOCATION:
 *   getNumberAttribRangeValues (pAttribList)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) ppAttribRangeValue (char **) pointer to list of table of range values
 *
 *   FUNCTION VALUE:
 *   (int)   Number of attributes range values defined for the CAD record, or
 *   -1 if the number of range values in the declaration of the external array
 *   pWfsDbCadList[] exceeds the maximum allowed (as defined in dbTypes.h)
 *
 *   PURPOSE:
 *   Get the number of attribute range values defined for a CAD record
 *
 *   DESCRIPTION:
 *   This routine returns the number of attribute range values that are defined
 *   for a CAD record. The table of range values, ppAttribRangeValue, is
 *   normally a member of the structure array pWfsDbCadList[].pAttrib[] - the
 *   declaration & initialisation of which serves as the definition of each
 *   CAD record in the system.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEVELOPMENT NOTE:
 *   This function assumes that the first uninitialised attribute range value
 *   contains zero. 
 *-
 */

int getNumberAttribRangeValues
   (
   char **   ppAttribRangeValue
   )
{
   FAST int   k;

   /*
    * The first uninitialised attribute range value corresponds to the end of 
    * the list of range values.
    */

   for (k = 0; k < (CAD_MAX_N_ATTRIB_IN_RANGE + 1); k++)
   {
      if (ppAttribRangeValue [k] == 0) return (k);
   }

   ERROR_SET (S_epToVxLib_RECORD_DEFINITION_ERROR, 
      "Invalid CAD attribute range definition",
      ERROR_LOG_SAVE);
   return (-1);
}

/* -------------------------------------------------------------------------- */


/*+
 *   IGNORED FUNCTION NAME:
 *   waitPipeExists 
 *
 *   INVOCATION:
 *   waitPipeExists (pPipeName, timeoutPeriod, timeoutDelay)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>) pPipeName     (char *)       name of pipe
 *   (>) timeoutPeriod (const double) timeout period in seconds
 *                                    (=0 for no timeout, < 0 for infinite
 *                                    timeout)
 *   (>) timeoutDelay  (const double) interval in seconds between test for
 *                                    pipe's existence
 *   FUNCTION VALUE:
 *   (STATUS)   OK if the pipe exists, or ERROR if the pipe is not
 *   found to created within the specified timeout period
 *
 *   PURPOSE:
 *   Wait (with timeout) until a named pipe exists
 *
 *   DESCRIPTION:
 *   This routine waits until a named pipe device is created or until
 *   the specified timeout period expires. It tests for the existence
 *   of the pipe at intervals of timeoutDelay seconds.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

STATUS   waitPipeExists
   (
   char *         pPipeName,
   const double   timeoutPeriod,
   const double   timeoutDelay
   )
{
   struct timespec   timeStart;
   BOOL              timeout = FALSE;

#ifdef DEBUG
   printf ( "waitPipeExists: Waiting for pipe %s to exist ... ", pPipeName );
#endif /* DEBUG */

   /* Check the pipe every timeoutDelay seconds.
    * Wait for the pipe to exist, or a timeout, whichever occurs first.
    */

   if (! pipeExists (pPipeName) && (timeoutPeriod != 0.0))
   {
      START_TIMEOUT (& timeStart);
      while (! pipeExists (pPipeName) && 
             ! (timeout = timeoutExpired (timeoutPeriod, & timeStart)))
      {
         taskDelay (SEC_TO_NTICK (timeoutDelay));
      }
   }

#ifdef DEBUG
   printf ("done.\n");
#endif /* DEBUG */

   if (timeout)
   {
      ERROR_SET1 (S_epToVxLib_TIMEOUT_WAITING_FOR_PIPE, 
         "Timeout awaiting creation of pipe %s",
         ERROR_LOG_SAVE, pPipeName);
      return (ERROR);
   }
   else
   {
      return (OK);
   }
}

/* -------------------------------------------------------------------------- */

/*+
 *   IGNORED FUNCTION NAME:
 *   pipeExists
 *
 *   INVOCATION:
 *   pipeExists (pName)
 *
 *   PARAMETERS: (">" input, "!" modified, "<" output)
 *   (>)   pName      (char *)   name of pipe
 *
 *   FUNCTION VALUE:
 *   (BOOL)   TRUE if the pipe exists, FALSE if it does not
 *
 *   PURPOSE:
 *   Test for the existence of a named pipe device
 *
 *   DESCRIPTION:
 *   This routine tests for the existence a named pipe.
 *
 *   EXTERNAL VARIABLES:
 *   None
 *
 *   PRIOR REQUIREMENTS:
 *   None
 *
 *   INCLUDE FILES:
 *   epToVxLib.h
 *
 *   DEFICIENCIES:
 *   None known
 *-
 */

BOOL   pipeExists
   (
   char * pName
   )
{
   char * pNameFound;

   iosDevFind (pName, & pNameFound);
   if (strlen (pNameFound) > 0)
   {
      return (FALSE);
   }
   else
   {
      return (TRUE);
   }
}


/* ------------------------------------------------------------------------- */

/*
 * These are a collection of undocumented debugging functions for use at 
 * the VxWorks console.
 */

void epToVxCmdPacketShow (const char * ptr)      
                                        /* Display contents of command packet */
{
   uint32   clientId; /* Client ID (see def of command packet at beginning)   */
   uint32   cmdNum;   /* Command number                                       */
   uint32   cmdMod;   /* Command modifier                                     */
   uint32   defMask;  /* Default mask   or error number                       */
   uint32 * argPtr;   /* Pointer to command arguments                         */

   printf ("epToVxCmdPacketShow: Contents of command packet at %#x\n", 
           (int) ptr);

   clientId = *((uint32 *)(int)ptr);
   printf ("epToVxDataPacketShow: Client ID = %d, ", (int)clientId);

   cmdNum = *((uint32 *)(int)(ptr + 4));
   printf ("command # = %d ", (int)cmdNum);

   cmdMod = *((uint32 *)(int)(ptr + 8));
   printf ("modifier = %#x, ", (int)cmdMod);

   defMask = *((uint32 *)(int)(ptr + 12));
   printf ("default mask/error number = %#x/%d\n", (int)defMask, (int)defMask);

   argPtr = (uint32 *)(int)(ptr + 15);
   printf ("Command arguments begin at %#x\n", (int) argPtr);

}

void epToVxDataPacketShow (const char * ptr)   
                                 /* Display contents of data or update packet */
{
   uint32   mode;     /* Mode   (see definition of data packet at beginning)  */
   uint32   recordId; /* Record ID                                            */
   uint32   nElement; /* Number of data elements                              */
   uint32 * dataPtr;  /* Pointer to data element                              */
   uint32   data;     /* Contents of data element as a 4-byte integer value   */
   int      i;


   printf ("epToVxDataPacketShow: Contents of data/update packet at %#x\n", 
           (int) ptr);

   mode = *((uint32 *)(int)ptr);
   printf ("epToVxDataPacketShow: Mode/Client ID = %d, ", (int)mode);

   recordId = *((uint32 *)(int)(ptr + 4));
   printf ("record/update ID = %d, ", (int)recordId);

   nElement = *((uint32 *)(int)(ptr + 8));
   printf ("N elements = %d\n", (int)nElement);

   dataPtr = (uint32 *)(int)(ptr + 12);

   printf ("Data:");
   for (i=0; i<nElement; i++)
   {
      data = *(dataPtr);
      printf (" [%d]=%#x/%d", (int)i, (int)data, (int)data);
      dataPtr++;
   }
   printf (".\n");
}
