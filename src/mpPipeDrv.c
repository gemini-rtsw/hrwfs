static struct {void *v; char *c;} rcsid = {&rcsid,
	"$Id: mpPipeDrv.c,v 1.1.1.1 1999-03-17 03:14:23 cboyer Exp $"};

/*+
 *	MODULE NAME:
 *	mpPipeDrv
 *
 *	FILENAME:
 *	mpPipeDrv.c
 *
 *	DESCRIPTION:
 *	Multi-processor pipe device driver for VxWorks, used for
 *	inter-processor pipe communications over VME bus. This driver
 *	provides support for pipe devices on multiple VxWorks CPUs
 *	which share the same VME bus
 *
 *	The multi-processor pipe driver, mpPipeDrv, relies on the extended systems library,
 *	sysextLib, for the underlying VME bus communications used by the driver.
 *
 *	mpPipeDrv is, by default, installed as a VxWorks device driver and in this case
 *	the standard driver routines (create, open, close, read, write and ioctl) are
 *	only accessible via the I/O system. An option is provided to compile
 *	the driver as a VxWorks library which allows the driver routines to be
 *	available in the public interface (as mpPipeDevCreate, mpPipeOpen, mpPipeClose,
 *	mpPipeRead, mpPipeWrite and mpPipeIoctl). This option is intended to be used
 *	for debugging purposes only and is selected by defining the macro
 *	NO_INSTALL when mpPipeDrv.c is compiled
 *
 *	EXTERNAL MODULES:
 *	timeoutLib
 *	sysextLib
 *	errorLib
 *
 *	FUNCTION NAME(S):
 *	mpPipeDrv				- driver initialisation
 *	mpPipeRead				- pipe read, called by IO system
 *	mpPipeWrite				- pipe write, called by IO system
 *	mpPipeDevCreate			- pipe create, called by IO system
 *	mpPipeDevDelete			- pipe delete, called by IO system
 *	mpPipeOpen				- pipe open, called by IO system
 *	mpPipeClose				- pipe close, called by IO system
 *	mpPipeIoctl				- IO control, called by IO system
 *	mpPipeShutdown			- shutdown and de-install driver
 *	mpPipeShow				- print information on current multi-processor pipes
 *	mpDemo1					- example of buffered pipe usage
 *	mpDemo2					- example of unbuffered pipe usage
 *
 *	IGNORED FUNCTION NAME(S)
 *	mp_mailboxIsr			- mailbox interrupt service routine
 *	mp_ExecuteCommand		- execute command
 *	mp_IsRemoteProcAlive	- wait for slave to initialise
 *	mp_LocalRead			- local pipe read
 *	mp_LocalWrite			- local pipe write
 *	mp_LocalCommand			- execute remote command on local processor
 *	mp_LocalCreate			- local pipe create
 *	mp_LocalDelete			- local pipe delete
 *	mp_localopen			- local pipe open
 *	mp_localclose			- local pipe close
 *	mp_localflush			- local pipe flush
 *	mp_LocalInvalidate		- local pipe invalidate
 *	mp_PrintPipeDetails		- print details of pipes
 *	mp_PipeDaemon			- daemon task, handles mailbox interrupts
 *
 *	DEFICIENCIES
 *	For some reason this library uses a mixture of vxWorks defined types (e.g. "UINT16")
 *	and Nick Dillon defined types (e.g. "uint32"). This might cause portability problems.
 *	SMB - 29 September 1998.
 *
 *	The idea behind this library is a really good one, but it fails to be portable
 *	because it uses sysextLib, which makes very specific assumptions about the underlying
 *	hardware, to handle the interprocessor communication. A new version of this library,
 *	based around VxMP shared message queues, would be a substantial improvement.
 *	SMB - 29 September 1998.
 *
 *	INCLUDE FILE(S):
 *	mpPipeDrv.h
 *
 *	ORIGINAL AUTHOR:
 *	Nick Dillon
 *
 *	MODIFIED BY:
 *	Steven Beard
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.13  1998/12/07 11:17:21  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.12  1998/10/01 13:46:39  cics
 * Provate functions prefixed by mp_
 *
 * Revision 1.11  1998/09/28 08:54:31  cics
 * Give warning if an attempt if made to compile this file for anything other than vxWorks
 *
 * Revision 1.10  1998/09/09 14:35:31  cics
 * Global variables renamed to ensure they are unique
 *
 * Revision 1.9  1998/08/13 09:08:26  smb
 * Added author comment
 *
 * Revision 1.8  1998/07/15 13:28:42  smb
 * strncpy implemented. Code and comments rearranged to make more readable and more printable.
 *
 * Revision 1.7  1998/02/18 09:32:26  smb
 * strcpy to strncpy changes recommended but not implemented
 *
 * Revision 1.6  1998/01/30 15:30:14  smb
 * Fixed some problems uncovered by prolint
 *
 * Revision 1.5  1998/01/16 15:55:32  smb
 * Quell compiler warning about rcsid using anj's idea
 *
 * Revision 1.4  1998/01/06 14:18:59  smb
 * External modules described. Some ERROR_MSG_NONE replaced with error messages
 *
 * Revision 1.3  1997/12/16 17:33:26  smb
 * Fixed a few typos in the comments
 *
 * Revision 1.2  1997/12/11 15:35:06  smb
 * Expand terse error messages - no changes to algorithms
 *
 * Revision 1.1.1.1  1997/11/28 11:46:17  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif	/* vxWorks */

#include <symLib.h>
#include <cacheLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <vme.h>
#include <sysLib.h>
#include <timers.h>
#include <taskLib.h>
#include <vxLib.h>
#include <semLib.h>
#include <msgQLib.h>
#include <string.h>
#include <iosLib.h>
#include "mpPipeDrv.h"
#include "sysextLib.h"
#include "gemTypes.h"
#include "timeoutLib.h"
#include "errorLib.h"

/* defines */

/* #define DEBUG */							/* Define this macro to enable debug messages */

/* static variables */

LOCAL int					mpPipeDrvNumber = NULL;		/* Device driver number.				*/
LOCAL SYMTAB_ID				mpPipeSymTab = NULL;		/* Symbol table used to store pipe		*/
														/* device descriptions.					*/
LOCAL SEM_ID				mpPipeMutexSem = NULL;		/* Mutex semaphore.						*/
LOCAL SEM_ID				mpPipeMboxIrq;				/* Counting semaphore for mailbox		*/
														/* interrupts.							*/
LOCAL volatile MP_PIPE_CONTROL_MSG *
							pMasterControlMsg;			/* Pointer to Master Control Structure	*/
														/* (MCS).								*/
LOCAL int					nProcOnBus;					/* Number of processors on VME bus.		*/
LOCAL char					mpPipeJunkBuffer [MP_PIPE_MAX_BYTES_NAME + 1];
														/* Buffer used as workspace.			*/

/*
 * Each processor has a local command flag, pCmdFlag, in cache-safe RAM. This flag is set by a
 * remote processor whenever a command is issued to this (local) processor from the remote processor.
 * It is used to avoid the local processor having to poll the MCS to determine whether a command
 * has arrived every time a mailbox interrupt arrives (which will mostly occur when pipe messages
 * are written to the local processor). The Master processor reads the address of each slave's
 * command flag and write it to an array in the MCS during initialisation of the network. Each
 * slave uploads this array during initialisation and is subsequently able to write to a
 * destination proc's command flag whenever it issues a command to it. When a proc writes a data
 * message to a destination proc it DOES NOT set the command flag.
 */

LOCAL volatile char *	pCmdFlag;						/* Flag set by remote processor when	*/
														/* it issues a command.					*/
LOCAL volatile char *	ppRemoteCmdFlag [SYSEXT_MAX_N_PROC];
														/* Array of command flags.				*/

/* function declarations */

LOCAL void		mp_mailboxIsr (int argument);
LOCAL BOOL		mp_LocalRead  (char * pName, int symValue, SYM_TYPE symType, int argument,
					UINT16 group);
LOCAL BOOL		mp_LocalWrite (char * pName, int symValue, SYM_TYPE symType, int argument,
					UINT16 group);
LOCAL STATUS	mp_LocalCreate (char * pName, int  maxNMsg, int maxNBytePerMsg,
					uint32 * pErrorNumber);
LOCAL STATUS	mp_LocalDelete (char * pName, uint32 * pErrorNumber);
LOCAL STATUS	mp_localopen (char * pName, int flags, int procId, uint32 * pErrorNumber);
LOCAL STATUS	mp_localclose (char * pName, int flags, int procId, uint32 * pErrorNumber);
LOCAL STATUS	mp_localflush (char * name, uint32 * pErrorNumber);
LOCAL STATUS	mp_LocalInvalidate (char * name, uint32 * pErrorNumber);
LOCAL STATUS	mp_LocalCommand (BOOL * pIrqServiced, uint32 * pErrorNumber);
LOCAL STATUS	mp_ExecuteCommand (char * pName, uint32 function, int arg1, int arg2,
					uint32 * pErrorNumber);
LOCAL BOOL		mp_PrintPipeDetails (char * name, int symValue, SYM_TYPE symType, int argument,
					UINT16 group);
LOCAL STATUS	mp_IsRemoteProcAlive (int procNumber, struct timespec * pTimeStart,
					double timeoutSecs);
LOCAL void		mp_PipeDaemon (void);

/*
 * Only include the following functions if the driver is to be installed in the VxWorks I/O system.
 * This is the default, adopted when NO_INSTALL is not defined at compile time. If the driver isn't
 * installed, then these functions are declared as externals in mpPipeDrv.h to allow them to be
 * called as library function calls.
 */

#ifndef	NO_INSTALL
LOCAL int		mpPipeOpen (DEV_HDR * deviceHeader, char * pRemainder, int flags);
LOCAL int		mpPipeClose (int fd);
LOCAL int		mpPipeIoctl (int fd, int function, int arg);
LOCAL int		mpPipeRead (int fd, char * pBuffer, size_t maxNBytes);
LOCAL int		mpPipeWrite (int fd, char * pBuffer, size_t maxNBytes);
#endif	/* NO_INSTALL */

/*+
 *	FUNCTION NAME:
 *	mpPipeDrv
 *
 *	INVOCATION:
 *	mpPipeDrv (timeoutSecs, resetBusOnTimeout)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>) timeoutSecs			(double)		allowed timeout period for initialisation
 *	(>)	resetBusOnTimeout	(const BOOL)	enables bus reset if a timeout occurs
 *
 *	FUNCTION VALUE:
 *	(STATUS)  OK, or ERROR if the driver could not be initialised or installed
 *
 *	PURPOSE:
 *	Initialisation routine for multi-processor pipe driver
 *
 *	DESCRIPTION:
 *	The multi-processor pipe driver relies on the extended systems library,
 *	sysextLib, for the underlying VME bus communications used by the driver.
 *	This routine initialises the shared-memory data structures used by each
 *	processor for these communications. If the local processor number is zero -
 *	as determined by a previous call to sysextProcNumSet() - then the processor
 *	is adopted as the "Master" by the pipe driver system and it initialises all
 *	other "Slave" processors on the bus. Each Slave is initialised by first
 *	enabling its VME interface and then writing a heartbeat counter to
 *	its local mailbox registers. When the Master detects a slave's heartbeat it
 *	then waits for the Slave to read the address of a global data structure called
 *	the Master Control Structure, or MCS, which exists in a cache-safe area of the
 *	Master's local RAM. Each Slave reads the address of the MCS from the Master's
 *	Mailbox registers and then writes the address of a single character flag,
 *	called the Command flag (which is held in cache-safe RAM by each Slave) back to
 *	the Master's mailbox registers. When the Master processor has a uploaded Command
 *	Flag pointer from every Slave on the bus it writes an array of these pointers
 *	to the MCS and instructs each Slave in turn to upload the entire array. The
 *	Command flag pointers are subsequently used by each Slave to signal to another
 *	processor when a command is being issued to it. The MCS is used as a central
 *	location for the broadcasting of commands (e.g. create, delete, open etc) to
 *	every processor on the bus.
 *
 *	If a timeout occurs whilst processor #0 attempts to initialise any other
 *	processor on the bus, then if input parameter resetBusOnTimeout is TRUE
 *	an attempt is made to assert the VME bus sysrst* signal, thus resetting
 *	all processors on the bus. Note that not all target architectures allow
 *	sysrst* to be asserted from software control (see routine sysextBusReset in
 *	sysextLib for details of supported architectures). In the event that the
 *	architecture does not support VME bus resetting, a timeout on processor #0
 *	will simply return ERROR. The timeout period, in seconds, is set by input
 *	parameter timeoutSecs; a negative timeout period forces an infinite timeout
 *	period.
 *
 *	NO INSTALL option:
 *	mpPipeDrv is, by default installed as a VxWorks device driver and in this case
 *	the standard driver routines (create, open, close, read, write and ioctl) are
 *	only accessible via the I/O system. An option is however provided to compile
 *	the driver as a VxWorks library which allows the driver routines to be
 *	available in the public interface (as mpPipeDevCreate, mpPipeOpen, mpPipeClose,
 *	mpPipeRead, mpPipeWrite and mpPipeIoctl). This option is intended to be used
 *	for debugging purposes only and is selected by defining the macro
 *	NO_INSTALL when mpPipeDrv.c is compiled.
 *
 *	EXTERNAL VARIABLES:
 *	(!)	pCmdFlag		(volatile char *)	Local command flag, may be set by other CPUs
 *	(!)	ppRemoteCmdFlag	(volatile char **)	Array of command flag pointers
 *
 *	PRIOR REQUIREMENTS:
 *	This routine is normally called immediately following a VME system reset.
 *	All processors on the bus should be initialised at the same time.
 *	The local processor's VME interface must have been intialised with
 *	a previous call to sysextProcNumSet(). VME block transfers must also have
 *	been initialised if they are to be used by the driver, e.g. the routine
 *	sysextVmeBlockInit() may be used to perform this initialisation. The real-time
 *	clock (CLOCK_REALTIME) must have been initialised with a call to clock_setres().
 *
 *	LIMITATIONS:
 *	The driver does not allow more than one processor to open a pipe in the same
 *	mode. This restricts multi-processor pipes to point-to-point connections
 *	between two processors on the bus at any instance in time. For example, an
 *	application which requries that three different processors write to a task running
 *	on a fourth processor must use three separate multi-processor pipes (one per
 *	writer); the fourth processor may then use the select library (selectLib) to
 *	effectively multiplex these three pipes into a single source of data.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS mpPipeDrv
	(
	double			timeoutSecs,
	const BOOL		resetBusOnTimeout
	)
{
	char *			pVmeAdrs;
	int				procNumber;
	int				processorNumber;
	uint32			heartbeat;
	BOOL			timeout;
	struct timespec	timeStart;

	pMasterControlMsg = NULL;

	if ((processorNumber = sysextProcNumGet ()) == -1)
	{
		ERROR_SET (0, "Invalid processor number", ERROR_LOG_SAVE);
		return (ERROR);
	}

	if ((nProcOnBus = sysextNProcGet ()) == -1)
	{
		ERROR_SET (0, "Invalid number of processors on bus", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* First clear-up leftovers from any previous incarnation of the driver */

	mpPipeShutdown ();

	/*
	 * Create a symbol table used to access mp pipes by name. Dis-allow symbols
	 * with the same name and type
	 */

	if ((mpPipeSymTab = symTblCreate ((int) MP_PIPE_SYMTAB_HASHSIZE, FALSE, memSysPartId)) == NULL)
	{
		ERROR_SET (0, "Symbol table creation failed", ERROR_LOG_SAVE);
		return (ERROR);
	}

	if ((mpPipeMutexSem = semMCreate (SEM_Q_FIFO | SEM_DELETE_SAFE)) == NULL)
	{
		ERROR_SET (0, "Failed to create mutex semaphore", ERROR_LOG_SAVE);
		return (ERROR);
	}

	if ((mpPipeMboxIrq = semCCreate (SEM_Q_FIFO, 0)) == NULL)
	{
		ERROR_SET (0, "Failed to create counting semaphore", ERROR_LOG_SAVE);
		return (ERROR);
	}

	if (sysextMboxConnect ((FUNCPTR) mp_mailboxIsr, MP_PIPE_MAILBOX) == ERROR)
	{
		ERROR_SET (0, "Failed to connect to mailbox interrupt channel", ERROR_LOG_SAVE);
		return (ERROR);
	}

	if (sysextMboxEnable (MP_PIPE_MAILBOX) == ERROR)
	{
		ERROR_SET (0, "Failed to enable mailbox interrupt", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* Allocate cache-safe memory for local command flag and initialise (clear) it */

	if ((pCmdFlag = (volatile char *) cacheDmaMalloc (1)) == NULL)
	{
		ERROR_SET (0, "Cache-safe memory allocation failed", ERROR_LOG_SAVE);
		return (ERROR);
	}

	* pCmdFlag = 0;
	CACHE_DMA_FLUSH (pCmdFlag, 1);

	/*
	 * If this is the "Master" processor (proc number 0), the MCS is held locally and is initialised
	 * then each of the other ("Slave") processors on the bus is interrogated to determine whether
	 * they are alive and to download the pointer to the MCS to them. Each slave also uploads a
	 * pointer to its local Command flag to the Master
	 */

	if (processorNumber == 0)
	{
		/* Spawn the daemon task */

		if (taskSpawn (MP_PIPE_DAEMON_NAME, MP_PIPE_DAEMON_PRIORITY, 0, MP_PIPE_DAEMON_STACK,
		               (FUNCPTR) mp_PipeDaemon, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
        {
            ERROR_SET (0, "Failed to spawn mpPipeDrv daemon task", ERROR_LOG_SAVE);
            return (ERROR);
        }

		/* Allocate cache-safe RAM for the MCS and initialise it ready for slave-initialisation */

        if ((pMasterControlMsg = (volatile MP_PIPE_CONTROL_MSG *)
             cacheDmaMalloc (sizeof (MP_PIPE_CONTROL_MSG))) == NULL)
        {
            ERROR_SET (0, "Cache-safe memory allocation failed", ERROR_LOG_SAVE);
            return (ERROR);
        }

		pMasterControlMsg->busyFlag = 1;
		pMasterControlMsg->function = MP_PIPE_FUNC_INIT_SLAVE;
		pMasterControlMsg->errorNumber = 0;

		/*
		 * Get address of MCS on VME bus and write to local mailbox register. Four byte locations
		 * in mailbox space are used to store the 32-bit address
		 */

		if (sysextLocalToBusAdrs (VME_AM_EXT_SUP_DATA, (char *) pMasterControlMsg, & pVmeAdrs)
		    == ERROR)
		{
			ERROR_SET (0, "Failed to map MCS address onto VME bus", ERROR_LOG_SAVE);
			return (ERROR);
		}
		sysextWriteLocalMboxReg32 ((uint32) pVmeAdrs);
		START_TIMEOUT (& timeStart);

#ifdef DEBUG
		printf ("mpPipeDrv: Master processor initialised control structure at 0x%x (= 0x%x on VME)\n",
				(int) pMasterControlMsg, (int) pVmeAdrs);
		printf ("mpPipeDrv: ptrs to busy, Done flags = 0x%x, 0x%x\n",
				(int) & pMasterControlMsg->busyFlag, (int) & pMasterControlMsg->doneFlag);
#endif

		/* Get VME bus address of Master's local Command flag and write it to the MCS */

		if (sysextLocalToBusAdrs (VME_AM_EXT_SUP_DATA, (char *) pCmdFlag, & pVmeAdrs) == ERROR)
		{
			ERROR_SET (0, "Failed to map local command flag address onto VME bus",
			           ERROR_LOG_SAVE);
			return (ERROR);
		}
		pMasterControlMsg->ppRemoteCmdFlag [0] = pVmeAdrs;

        /*
         * The MCS has now been initialised.
         * Probe each processor in VME extended space until it has initialised
         * its VME slave interface. Note that the library sysextLib always
         * initialises the short (mailbox) slave interface before the extended
         * interface. It is therefore safe to assume that the mailbox space is
         * ready as soon as extended space is ready
         */

		for (procNumber = 1; procNumber < nProcOnBus; procNumber++)
		{

			/*
			 * Write the ID of the destination processor for this initialisation command and
			 * clear the Done flag and the ID of the processor which replies to the command
			 * (->procIdSource). Flush out MCS from cache.
			 */

			pMasterControlMsg->procIdSource = 0;
			pMasterControlMsg->procIdDest = procNumber;
			pMasterControlMsg->doneFlag = 0;
			CACHE_DMA_FLUSH ((char *) & pMasterControlMsg->doneFlag, sizeof (MP_PIPE_CONTROL_MSG));

			/*
			 * Issue command to remote processor and wait for either a valid response or
			 * timeout error
			 */

			if (mp_IsRemoteProcAlive (procNumber, & timeStart, timeoutSecs) == ERROR)
			{
				if (resetBusOnTimeout)
					return (sysextBusReset ());
				else
					return (ERROR);
			}

			/*
			 * Reply received from Slave: cache-invalidate the MCS before reading it, then
			 * check that the reply was from the expected Slave rather than some other rogue
			 * on the bus
			 */

			CACHE_DMA_INVALIDATE ((char *) & pMasterControlMsg->doneFlag,
			                      sizeof (MP_PIPE_CONTROL_MSG));
			if (pMasterControlMsg->procIdSource != procNumber)
			{
				ERROR_SET (S_mpPipeDrv_INV_REMOTE_CPU_RESP, "Unexpected response from remote CPU",
					ERROR_LOG_SAVE);
				return (ERROR);
			}

			/* Read the remote slave's pointer its Cmd flag (bus address) and copy it into the MCS */

			if (sysextReadRemoteMboxReg32 (procNumber,
			       (uint32 *) & pMasterControlMsg->ppRemoteCmdFlag [procNumber])
			    == ERROR)
			{
				ERROR_SET (0, "Failed to read remote mailbox register", ERROR_LOG_SAVE);
				return (ERROR);
			}
#ifdef DEBUG
			printf ("mpPipeDrv: Read ptr to remote slave #%d's Cmd flag = 0x%x on bus\n",
					procNumber, pMasterControlMsg->ppRemoteCmdFlag [procNumber]);
#endif /* DEBUG */

			/*
			 * Convert the Slave's pointer to its Command flag to local address and save for
			 * future use.
			 */

			if (sysextBusToLocalAdrs (VME_AM_EXT_SUP_DATA,
				(char *) pMasterControlMsg->ppRemoteCmdFlag [procNumber],
				(char **) & ppRemoteCmdFlag [procNumber]) == ERROR)
			{
				ERROR_SET (0, "Failed to convert command flag pointer to local address",
				           ERROR_LOG_SAVE);
				return (ERROR);
			}

#ifdef DEBUG
			printf ("mpPipeDrv: master succeeded in 1st-stage initialisation of slave #%d OK\n",
			        procNumber);
#endif /* DEBUG*/
		}

		/*
		 * Now we've performed the first stage of initialising each slave, having downloaded the 
		 * pointer to the MCS to each Slave, uploaded each Slave's pointer to its Command flag,
		 * and copied these pointers into the MCS. Now issue another Mbox interrupt to each
		 * slave in turn to inform it that the array of Cmd flag pointers can now be read from
		 * the MCS.
		 */

		for (procNumber = 1; procNumber < nProcOnBus; procNumber++)
		{

			/* Prepare MCS prior to issuing command to Slave */

			pMasterControlMsg->procIdSource = 0;
			pMasterControlMsg->procIdDest = procNumber;
			pMasterControlMsg->doneFlag = 0;
			CACHE_DMA_FLUSH ((char *) & pMasterControlMsg->doneFlag, sizeof (MP_PIPE_CONTROL_MSG));

			/*
			 * Issue mailbox interrupt to Slave. It is implicit that this interrupt means that
			 * the array of Cmd flags is now ready to be loaded from the MCS: there is not
			 * specific function ID for this command.
			 *
			 * There is no need to check whether this function returns an error.
			 * It would have failed the first time if it was going to fail at all.
			 */

			sysextMboxIntGen (procNumber, MP_PIPE_MAILBOX);

			/*
			 * Invalidate the MCS then poll for the Done flag. NB: No need to re-initialise the 
			 * timeout timer since this was done earlier and this is all part of the same
			 * initialisation procedure.
			 */

			CACHE_DMA_INVALIDATE ((char *) & pMasterControlMsg->doneFlag,
			                      sizeof (MP_PIPE_CONTROL_MSG));

			timeout = FALSE;
			while ( (pMasterControlMsg->doneFlag == 0) &&
			        (pMasterControlMsg->procIdSource != procNumber) &&
				    ! (timeout = timeoutExpired (timeoutSecs, & timeStart)) )
			{
				taskDelay (SEC_TO_NTICK (MP_PIPE_DELAY_INITIALISE));
				CACHE_DMA_INVALIDATE ((char *) & pMasterControlMsg->doneFlag,
				                      sizeof (MP_PIPE_CONTROL_MSG));
			}

			if (timeout)
			{
				ERROR_SET (S_mpPipeDrv_REMOTE_CPU_TIMEOUT, "Timeout waiting for CPU (2nd stage)",
				           ERROR_LOG_SAVE);
				return (ERROR);
			}

#ifdef DEBUG
			printf ("mpPipeDrv: master succeeded in 2nd-stage initialisation of slave #%d OK\n",
			        procNumber);
#endif /* DEBUG */

		}
		pMasterControlMsg->busyFlag = 0;			/* Clear (and flush) Busy flag to release MCS */
		CACHE_DMA_FLUSH ((char *) & pMasterControlMsg->doneFlag, sizeof (MP_PIPE_CONTROL_MSG));
	}
	else
	{
		/*
		 * This is a Slave processor (proc number != 0). Start outputing a hearbeat count to the
		 * local mailbox register space (4 bytes used for a 32-bit count) and do so until a mailbox
		 * interrupt is received. This first mailbox interrupt is always interpreted as an indication 
		 * that the Master has detected the heartbeat and that its MCS is initialised and a pointer
		 * to it may be read from its mailbox space. 
		 */

		START_TIMEOUT (& timeStart);				/* Initialise the timeout timer.	*/
		heartbeat = 0;								/* Initialise the heartbeat count.	*/

#ifdef DEBUG
		printf ("mp_PipeDaemon: slave waiting for Mbox IRQ\n");
#endif /* DEBUG */

		timeout = FALSE;
        while ((semTake (mpPipeMboxIrq, NO_WAIT) == ERROR) &&
		       ! (timeout = timeoutExpired (timeoutSecs, & timeStart)) )
		{
			sysextWriteLocalMboxReg32 (heartbeat++);
			if (heartbeat >= MP_PIPE_HEARTBEAT_MAX_VALUE) heartbeat = 0;
			taskDelay (SEC_TO_NTICK (MP_PIPE_HEARTBEAT_PERIOD));
		}

		if (timeout)
		{
			ERROR_SET (S_mpPipeDrv_REMOTE_CPU_TIMEOUT, "Timeout waiting for 1st IRQ from Master",
				ERROR_LOG_SAVE);
			return (ERROR);
		}

		/* Read pointer to MCS from Master's mailbox space and convert to local address */

		if (sysextReadRemoteMboxReg32 (0, (uint32 *) & pMasterControlMsg) == ERROR)
		{
			ERROR_SET (0, "Failed to read remote mailbox register", ERROR_LOG_SAVE);
			return (ERROR);
		}
		else if (sysextBusToLocalAdrs (VME_AM_EXT_SUP_DATA, (char *) pMasterControlMsg,
		   (char **) & pMasterControlMsg) == ERROR)
		{
			ERROR_SET (0, "Failed to convert MCS pointer to local address", ERROR_LOG_SAVE);
			return (ERROR);
		}

		/*
		 * Now reply to the Master by writing the address (on VME bus) of this Slave's
		 * Command flag to the Master's mailbox space
		 */

		if (sysextLocalToBusAdrs (VME_AM_EXT_SUP_DATA, (char *) pCmdFlag, & pVmeAdrs) == ERROR)
		{
			ERROR_SET (0, "Failed to map command flag pointer onto VME bus", ERROR_LOG_SAVE);
			return (ERROR);
		}
		sysextWriteLocalMboxReg32 ((uint32) pVmeAdrs);
		pMasterControlMsg->procIdSource = processorNumber;
		pMasterControlMsg->procIdDest = 0;
		pMasterControlMsg->doneFlag = 1;
		CACHE_DMA_FLUSH ((char *) & pMasterControlMsg->doneFlag, sizeof (MP_PIPE_CONTROL_MSG));

		/*
		 * At this stage, this Slave has passed the first stage of initialisation. The Master will now
		 * proceed to go through the same procedure for any other Slaves on the bus before returning
		 * to this Slave and issuing a further mailbox interrupt which informs it that the MCS now
		 * holds a complete array of Command flag pointers (VME bus addresses) for all processors on
		 * the bus. When this interrupt is received, load the array of pointers and save for future
		 * use.
		 */

		timeout = FALSE;
		while ((semTake (mpPipeMboxIrq, NO_WAIT) == ERROR) &&
		   ! (timeout = timeoutExpired (timeoutSecs, & timeStart)))
		{
			taskDelay (SEC_TO_NTICK (MP_PIPE_DELAY_INITIALISE));
		}

		if (timeout)
		{
			ERROR_SET (S_mpPipeDrv_REMOTE_CPU_TIMEOUT, "Timeout waiting for 2nd IRQ from Master",
				ERROR_LOG_SAVE);
			return (ERROR);
		}

		/*
		 * Better check that the mailbox IRQ is indeed from the Master, just in case the networks
		 * really got in a mess. Then load each Command flag and convert from bus to local addresses
		 */

		CACHE_DMA_INVALIDATE ((char *) & pMasterControlMsg->doneFlag, sizeof (MP_PIPE_CONTROL_MSG));
		if ((pMasterControlMsg->procIdSource != 0) ||
		    (pMasterControlMsg->procIdDest != processorNumber) )
		{
			ERROR_SET (S_mpPipeDrv_UNEXPECTED_IRQ, "Unexpected IRQ received", ERROR_LOG_SAVE);
			return (ERROR);
		}

		for (procNumber = 0; procNumber < nProcOnBus; procNumber++)
		{
			if (procNumber != processorNumber)
			{
				ppRemoteCmdFlag [procNumber] =
						(volatile char *) pMasterControlMsg->ppRemoteCmdFlag [procNumber];

				if (sysextBusToLocalAdrs (VME_AM_EXT_SUP_DATA, (char *) ppRemoteCmdFlag [procNumber],
					(char **) & ppRemoteCmdFlag [procNumber]) == ERROR)
				{
					ERROR_SET (0, "Failed to convert remote command flag pointer to local address",
					           ERROR_LOG_SAVE);
					return (ERROR);
				}
#ifdef DEBUG
				printf ("mpPipeDrv: Slave uploaded ptr to Cmd flag [%d] = 0x%x (local adrs)\n",
					procNumber, ppRemoteCmdFlag [procNumber]);
#endif /* DEBUG */
			}
		}

		/* Spawn the daemon task before issuing the final DONE response to the Master */

		if (taskSpawn (MP_PIPE_DAEMON_NAME, MP_PIPE_DAEMON_PRIORITY, 0, MP_PIPE_DAEMON_STACK,
								   (FUNCPTR) mp_PipeDaemon, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
		{
			ERROR_SET (0, "Failed to spawn mp_PipeDaemon task", ERROR_LOG_SAVE);
			return (ERROR);
		}

		/* Slave initialisation complete. Set Done flag in MCS and flush it out */

		pMasterControlMsg->procIdSource = processorNumber;
		pMasterControlMsg->procIdDest = 0;
		pMasterControlMsg->doneFlag = 1;
		CACHE_DMA_FLUSH ((char *) & pMasterControlMsg->doneFlag, sizeof (MP_PIPE_CONTROL_MSG));
	}

	/* Have now done all the hard stuff. Just need to install the driver in the IO system */

#ifndef	NO_INSTALL
	if ((mpPipeDrvNumber = iosDrvInstall (mpPipeDevCreate, mpPipeDevDelete, mpPipeOpen, mpPipeClose,
										  mpPipeRead, mpPipeWrite, mpPipeIoctl)) == ERROR)
	{
		ERROR_SET (0, "Failed to install mpPipeDrv driver in I/O system", ERROR_LOG_SAVE);
		return (ERROR);
	}
#endif /* NO_INSTALL */

	return (OK);
}

/*+
 *	FUNCTION NAME:
 *	mpPipeRead
 *
 *	INVOCATION:
 *	mpPipeRead (fd, pBuffer, maxNBytes)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	fd			(int)		file descriptor, as returned by open()
 *	(<)	pBuffer		(char *)	destination for message read from pipe
 *	(>)	maxNBytes	(size_t)	number of bytes to read
 *
 *	FUNCTION VALUE:
 *	(int)  Number of bytes actually read, or ERROR if the pipe read failed.
 *
 *	PURPOSE:
 *	Read message from pipe to local buffer
 *
 *	DESCRIPTION:
 *	This routine is normally called directly by the IO system (rather than
 *	by an application program) in response to a call to read(). Its purpose
 *	is to read up to maxNBytes from a pipe which has previously been opened
 *	to read. If the first available message has a length in excess of
 *	maxNBytes then the un-read bytes are discarded. A mutex semaphore,
 *	pPipeDesc->mutexSemRead is taken for the duration of a read to ensure
 *	that once a read starts it always completes before another read is
 *	allowed to start; this avoids possible pre-emption of a task doing a
 *	read by another task.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	This routine is only available globally if the pipe driver was compiled
 *	with the NO_INSTALL option flag defined. The pipe must have been created
 *	and opened for reading.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

int mpPipeRead
	(
	int				fd,
	char *			pBuffer,
	size_t			maxNBytes
	)
{
	SYM_TYPE		symType;
	MP_PIPE_DESC *	pPipeDesc;
	int				nBytes;
	FAST int		i;

	/* Lookup pipe descriptor and make sure its the primary descriptor */

	if ((symFindByValue (mpPipeSymTab, (UINT) fd, mpPipeJunkBuffer, (int *) & pPipeDesc, & symType)
		== ERROR)
		|| (fd != (int) pPipeDesc))
	{
		ERROR_SET (0, "Can't find pipe file descriptor in symbol table", ERROR_LOG_SAVE);
		return (ERROR);
	}

	if (symType == MP_PIPE_SYM_TYPE_ALT) pPipeDesc = (MP_PIPE_DESC *) pPipeDesc->pOtherPipeDef;

	/* Check ability to perform read */

	if (! pPipeDesc->pipeValid || pPipeDesc->procIdRead != sysextProcNumGet ())
	{
		ERROR_SET (S_mpPipeDrv_CANT_READ_PIPE, "Pipe invalid or not open to read", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* Truncate size of message at maximum allowed */

	if ((int) maxNBytes > pPipeDesc->maxNBytePerMsg) maxNBytes = (size_t) pPipeDesc->maxNBytePerMsg;

	/*
	 * If the pipe mode is unbuffered, then the message queue is used only to pass the number of
	 * bytes available in the incoming message. The pipe read in this case immediately
	 * frees the pipe to be re-written, so the onus is on the reading task to extract the data from
	 * the local buffer BEFORE calling read(). The recommended way of doing this is to use ioctl() to
	 * set a semaphore to be given on receipt of data from VME bus, when the semahphore is given the
	 * destination task should read the local buffer directly and then call read() in order to releas
	 * the buffer for the next write over VME
	 */

	semTake (pPipeDesc->mutexSemRead, WAIT_FOREVER);		/* Grab mutex sem before starting read */

	if (! pPipeDesc->unbuffered)
	{
		 nBytes = msgQReceive (pPipeDesc->msgQId, pBuffer, (UINT) maxNBytes, WAIT_FOREVER);
	}
	else
	{
		if (msgQReceive (pPipeDesc->msgQId, (char *) & nBytes, (UINT) 4, WAIT_FOREVER) == ERROR)
			nBytes = ERROR;

		/*
		 * Clear the Done and Busy flags immediately, rather than have this done in mp_LocalRead(),
		 * as is the normal case for buffered pipes
		 */

		* pPipeDesc->pLocalDoneFlag = 0;
		* pPipeDesc->pLocalBuffer = 0;
		CACHE_DMA_FLUSH ((char *) pPipeDesc->pLocalBuffer,
						 (int) (pPipeDesc->pLocalDoneFlag + 1) - (int) pPipeDesc->pLocalBuffer);
	}

	if (nBytes != ERROR)
	{
		semTake (mpPipeMutexSem, WAIT_FOREVER);		/* Ensure no interference from Mbox IRQs		*/
													/* whilst manipulating the list of byte-counts.	*/

		/*
		 * Search through list of number of bytes ready to read in msgQ. Find the first
		 * valid entry (not -1) and invalidate it since we've just read the corresponding
		 * message from the message Q
		 */

		for (i = pPipeDesc->maxNMsg - 1; i >= 0; i--)
		{
			if (pPipeDesc->pNByteReady [i] != -1)
			{
				pPipeDesc->pNByteReady [i] = -1;	/* Delete entry from list.			*/
				i = -1;								/* Abort loop over entries in list.	*/
			}
		}
		semGive (mpPipeMutexSem);
	}

	/*
	 * Call mp_LocalRead(): this will ensure that any queued writes that were awaiting a read
	 * command will be de-queued. If there were no writes pending then no action will be taken.
	 * Don't do this though if the pipe is opened for both reading AND writing by this proc, since
	 * in this case there is no need for VME transfers. Ensure also that the pipe wasn't closed
	 * during the msgQReceive().
	 */

	if (pPipeDesc->procIdWrite != sysextProcNumGet () && pPipeDesc->procIdWrite >= 0 &&
		pPipeDesc->procIdRead == sysextProcNumGet () && nBytes != ERROR)
	{
		semTake (mpPipeMutexSem, WAIT_FOREVER);
		mp_LocalRead ((char *) fd, (int) pPipeDesc, MP_PIPE_SYM_TYPE_NORMAL, (int) NULL,
			(UINT16) NULL);
		semGive (mpPipeMutexSem);
	}

	if (nBytes == ERROR)
		ERROR_SET (S_mpPipeDrv_PIPE_READ_FAILED, "mpPipeDrv failed to read pipe", ERROR_LOG_SAVE);

	semGive (pPipeDesc->mutexSemRead);				/* Release mutex semaphore when done */
	return (nBytes);
}

/*+
 *	FUNCTION NAME:
 *	mpPipeWrite
 *
 *	INVOCATION:
 *	mpPipeWrite (fd, pBuffer, maxNBytes)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	fd			(int)		file descriptor, as returned by open()
 *	(<)	pBuffer		(char *)	source of message to write to pipe
 *	(>)	maxNBytes	(size_t)	maximum number of bytes to write
 *
 *	FUNCTION VALUE:
 *	(int)  Number of bytes write, or ERROR if the pipe write failed.
 *
 *	PURPOSE:
 *	Write message from local buffer to pipe
 *
 *	DESCRIPTION:
 *	This routine is normally called directly by the IO system (rather than
 *	by an application program) in response to a call to write(). Its purpose
 *	is to write up to maxNBytes to a pipe which has previously been opened
 *	to write. If the maximum length of each message (as specified when the
 *	pipe was created) is less than maxNBytes then the message length is
 *	truncated to the maximum allowed. A mutex semaphore,
 *	pPipeDesc->mutexSemWrite is taken for the duration of a write to ensure
 *	that once a write starts it always completes before another write is
 *	allowed to start; this avoids possible pre-emption of a task doing a
 *	write by another task.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	This routine is only available globally if the pipe driver was compiled
 *	with the NO_INSTALL option flag defined. The pipe must have been created
 *	and not already opened for writing.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

int	mpPipeWrite
	(
	int				fd,
	char *			pBuffer,
	size_t			nBytes
	)
{
	SYM_TYPE		symType;
	MP_PIPE_DESC *	pPipeDesc;
	FAST int		i;
	BOOL			writeDone;
	int				nByte;

	/* Lookup pipe descriptor and make sure it's the primary descriptor */

	if ((symFindByValue (mpPipeSymTab, (UINT) fd, mpPipeJunkBuffer, (int *) & pPipeDesc, & symType)
			== ERROR)
		|| (fd != (int) pPipeDesc))
	{
		ERROR_SET (0, "Can't find pipe file descriptor in symbol table", ERROR_LOG_SAVE);
		return (ERROR);
	}
	if (symType == MP_PIPE_SYM_TYPE_ALT) pPipeDesc = (MP_PIPE_DESC *) pPipeDesc->pOtherPipeDef;

	/* Truncate size of message to maximum allowed */

	if ((int) nBytes > pPipeDesc->maxNBytePerMsg) nBytes = (size_t) pPipeDesc->maxNBytePerMsg;

	/* Check ability to perform write, then attempt to write to message queue (block if necessary) */

	if (! pPipeDesc->pipeValid || (pPipeDesc->procIdWrite != sysextProcNumGet ()))
	{
		ERROR_SET (S_mpPipeDrv_CANT_WRITE_PIPE, "Pipe invalid or not open", ERROR_LOG_SAVE);
		return (ERROR);
	}

	semTake (pPipeDesc->mutexSemWrite, WAIT_FOREVER);		/* Grab mutex sem before starting write */

	/*
	 * If the pipe is set to Unbuffered mode, then the message queue is used only to pass the
	 * number of bytes available to write; the data bytes are assumed to have been written
	 * directly into the local buffer prior to the call to write() that caused entry to this
	 * routine. If the pipe is not Unbuffered, write to the message queue as usual.
	 */

	if (pPipeDesc->unbuffered)
	{
		nByte = (int) nBytes;
		if (msgQSend (pPipeDesc->msgQId, (char *) & nByte, 4, WAIT_FOREVER, MSG_PRI_NORMAL) == ERROR)
		{
			semGive (pPipeDesc->mutexSemWrite);
			ERROR_SET (0, "Failed to send message to message queue", ERROR_LOG_SAVE);
			return (ERROR);
		}
	}
	else if (msgQSend (pPipeDesc->msgQId, pBuffer, (UINT) nBytes, WAIT_FOREVER, MSG_PRI_NORMAL)
	         == ERROR)
	{
		semGive (pPipeDesc->mutexSemWrite);
		ERROR_SET (0, "Failed to send message to message queue", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* Trap the situation in which the pipe was closed for writing whilst blocked in msgQSend() */

	if (pPipeDesc->procIdWrite != sysextProcNumGet ())
	{
		semGive (pPipeDesc->mutexSemWrite);
		ERROR_SET (S_mpPipeDrv_PIPE_CLOSED_BLOCKED, "Pipe closed whilst blocked in msgQSend()",
			ERROR_LOG_SAVE); 
		return (ERROR);
	}

	/*
	 * If pipe is opened to read by a different processor than this one, or the pipe mode is
	 * unbuffered, call mp_LocalWrite() to service the write. If the pipe is opened
	 * for reading by this processor (as well as for writing) then don't call mp_LocalWrite() since
	 * writes & reads are simply buffered locally in the msgQ and there is no need for VME
	 * transfers. If the pipe mode is normal (buffered) and it is not yet opened to read by
	 * another processor, don't call mp_LocalWrite() since whichever processor does eventually
	 * open the pipe to read will at that time issue a mailbox interrupt to this one in order
	 * to flush out any messages stored in the local message queue.
	 */

	if (((pPipeDesc->procIdRead >= 0) && (pPipeDesc->procIdRead != sysextProcNumGet ()))
		|| pPipeDesc->unbuffered)
	{
		semTake (mpPipeMutexSem, WAIT_FOREVER);
		mp_LocalWrite ((char *) fd, (int) pPipeDesc, MP_PIPE_SYM_TYPE_NORMAL, (int) & writeDone,
		            (UINT16) NULL);
		semGive (mpPipeMutexSem);
	}
	else if (pPipeDesc->procIdRead == sysextProcNumGet ())
	{
		/*
		 * If pipe opened to read AND write by this processor, increment the number of transactions
		 * immediately (this normally happens in mp_LocalWrite()), since in this case mp_LocalWrite()
		 * is not called.
		 */

		semTake (mpPipeMutexSem, WAIT_FOREVER);		/* Ensure no interference from MBox IRQ's	*/
													/* whilst manipulating list of byte counts.	*/

		/*
		 * Shift count of number of bytes ready to ready in message queue down by one
		 * place and add latest byte count to bottom of queue
		 */

		for (i = pPipeDesc->maxNMsg - 1; i > 0; i--)
		{
			pPipeDesc->pNByteReady [i] = pPipeDesc->pNByteReady [i - 1];
		}
		pPipeDesc->pNByteReady [0] = (int) nBytes;
		pPipeDesc->nMsgTransactions++;

		semGive (mpPipeMutexSem);
	}
	if ((int) nBytes == ERROR)
		ERROR_SET (S_mpPipeDrv_PIPE_WRITE_FAILED, "mpPipeDrv failed to write pipe", ERROR_LOG_SAVE);

	semGive (pPipeDesc->mutexSemWrite);				/* Release mutex semaphore when done */
	return ((int) nBytes);
}

/*+
 *	FUNCTION NAME:
 *	mpPipeDevCreate
 *
 *	INVOCATION:
 *	mpPipeDevCreate (pName, nMessages, nBytes)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName		(char *)	name of pipe to create
 *	(>)	nMessages	(int)		maximum number of messages buffered in queue(s)
 *	(>)	nBytes		(int)		maximum number of bytes per message
 *
 *	FUNCTION VALUE:
 *	(STATUS)  Pipe descriptor (pointer), or ERROR if create failed.
 *
 *	PURPOSE:
 *	Create multi-processor pipe device
 *
 *	DESCRIPTION:
 *	This routine creates a multi-processor pipe device on all processors on
 *	VME bus, thus making the pipe available for subsequent opening, reading
 *	and writing by any processors on the bus. Created pipes are installed in
 *	the IO system device table via iosDevAdd(). The parameters associated with
 *	pipe creation are analogous to those of conventional pipes (see the manual
 *	entry for pipeDevCreate()), however there are two distinct types of
 *	multi-processor pipe, as follow:
 *
 *	BUFFERED PIPES:
 *	This is generally referred to as the "normal" mode for multi-processor
 *	pipes. A buffered pipe is created with the parameter nMessages (passed to
 *	mpPipeDevCreate()) greater than 0. In this case, VxWorks message queues
 *	are used at either end of the pipe to buffer data written into (or read
 *	from) the pipe. A buffered pipe therefore provides an almost identical
 *	interface as a conventional VxWorks pipe. One difference which should be
 *	recognised however is that, for multi-processor pipes, nMessages gives
 *	the number of stages of message queue buffering in EACH processor for a
 *	pipe connection which exists between two processors on VME bus. Furthermore,
 *	an additional stage of buffering occurs at the low-level VME interface so
 *	that the total number of stages of buffering between two processors for
 *	such a pipe is (2xN+1) where N is the parameter, nMessages, with which the
 *	pipe was created. It is legitimate to open a buffered multi-processor pipe
 *	for reading and writing by the same processor and in this case the
 *	number of stages of buffering is simply N (nMessages), as is the case for
 *	a conventional pipe.
 *
 *	USING MULTI-PROCESSOR PIPES IN CONJUNCTION WITH RAW VME-BUS TRANSFERS:
 *	Maximum throughput for VME bus data transfers is obtained with low-level
 *	VME block transfer operations which do not involve the VxWorks I/O system.
 *	For example, the library sysextLib provides routines sysextVmeBlockInit() and
 *	sysextVmeBlockCopy() which support raw block copies (D32/BLT or D64/BLT mode)
 *	from the local CPU to any address on the bus. The use of these routines
 *	to transfer data between two VxWorks CPUs will generally require some higher-
 *	level inter-processor communications because VxWorks allocates memory
 *	dynamically so that the destination CPU must notifiy the source CPU of the
 *	destination address (on VME bus) for the data transfer. In this case, multi-
 *	processor pipes provide the functionality required to support a simple
 *	protocol which will enable raw VME block transfers to be initiated.
 *	Furthermore, the library supports a special kind of pipe known as an
 *	"un-buffered" pipe. These pipes provide a simple wrapper around the routine
 *	sysextVmeBlockCopy() such that high VME throughput is achieved whilst the
 *	low-level protocol required to set-up and execute block transfers is hidden
 *	from application code. See the description below for more details of
 *	un-buffered pipes.
 *
 *	UN-BUFFERED PIPES:
 *	An unbuffered pipe is created by setting the parameter nMessages to
 *	zero when mpPipeDevCreate() is called. In this case, the input and
 *	output to multi-processor pipes are not buffered and an application
 *	program is required to read/write directly into an area of memory
 *	reserved by the driver when a pipe is created. This approach has two
 *	major advantages over buffered pipes. Firstly, memory usage is
 *	minimised (since memory is not allocated for internal message queues),
 *	and this can be important when transferring large blocks of data.
 *	Secondly, the overall throughput of the driver when transferring data
 *	between two tasks is increased by up to an order of magnitude
 *	since the overhead of copying data to and from the message queues is
 *	eliminated. A disadvantage of unbuffered pipes is that the external
 *	interface is slightly more complicated than that of buffered pipes (see
 *	below). Also, unbuffered pipes may NOT be opened for reading and
 *	writing by the same processor.
 *
 *	USING UN-BUFFERED PIPES:
 *	When an unbuffered pipe is opened, the driver allocates an area of
 *	memory - called the Local Buffer - which it uses for VME transfers
 *	to/from another processor. A pointer to the first available data
 *	byte within this area of memory may be obtained via ioctl() using
 *	the control function FIOFSTATGET (see manual entry for mpPipeIoctl()).
 *	A task which writes or reads an unbuffered pipe should therefore use
 *	ioctl() as an alternative to allocating memory explicitely for its
 *	input or output buffer, and this pointer passed to write() or read()
 *	when writing or reading from an unbuffered pipe. Note however, that
 *	write() and read() simply ignore the pointer actually passed to them
 *	when the pipe type is unbuffered. In view of the fact that application
 *	code makes direct access to the Local Buffer used by the driver, some
 *	form of synchronisation mechanism is necessary to ensure that these
 *	buffers are only accessed when it is safe to do so. This synchronisation
 *	is achieved using semaphores. A binary or counting semaphore (but not
 *	a mutex semaphore) may be associated with a pipe via either ioctl()
 *	or the alternative routine mpPipeIoctlByName() (see the respective
 *	manual page entries). If such an association has been made, then
 *	the driver will give the specified semaphore immediately following
 *	transfer of data from the Local Buffer to a destination on VME bus,
 *	or, immediately on receipt of data from VME bus into the Local Buffer.
 *	Since an unbuffered pipe may only be opened to either write OR read
 *	(but not both) by a single processor, the reason for the semaphore
 *	being given is always implicit in the mode in which the pipe is
 *	opened.
 *
 *	EXAMPLES OF BUFFERED AND UN-BUFFERED PIPES:
 *	Two examples of the usage of the multi-processor pipe driver are
 *	provided by routines mpDemo1() and mpDemo2(). The first
 *	of these ping-pongs a message between two different processors using
 *	buffered pipes. The second performs multiple transfers from
 *	one processor to a second using unbuffered pipes.
 *
 *	USING MULTI-PROCESSOR PIPES FOR INTER-PROCESSOR SYNCHRONISATION:
 *	A multi-processor pipe device may be used to provide synchronisation
 *	between two processors on VME bus. In this case the pipe may be created
 *	with parameter nBytes set to zero so that no information bytes are
 *	transferred when a message is written, however the communication protocol
 *	provides the synchronisation function. In this case a multi-processor
 *	pipe is analogous to a counting semaphore, with the semGive() and
 *	semTake() routines replaced by write() and read() respectively; note,
 *	however, that in this case the "semGive()" routine may potentially block
 *	if all stages of message queue buffering become full. It is also
 *	possible to specify a semaphore ID to be given whenever data is read
 *	(from VME bus) by the pipe daemon task. To do this, a task must first
 *	open the pipe and create the required type of semaphore (counting or
 *	binary, but NOT a mutex semaphore); the task then calls ioctl() with
 *	function = FIOSETOPTIONS and the argument equal to the required
 *	semaphore ID. The semGive() may be disabled by re-executing this
 *	ioctl() function with the semaphore ID set to NULL.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	The pipe driver must have been previously initialised.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS	mpPipeDevCreate
	(
	char *	pName,
	int		nMessages,
	int		nBytes
	)
{
	uint32	errorNumber;
	STATUS	returnValue;

	/* Check validity of pipe parameters */

	if ((strlen (pName) > MP_PIPE_MAX_BYTES_NAME) || (nMessages < 0) || ((nBytes %4) != 0))
	{
		ERROR_SET (S_mpPipeDrv_INVALID_PIPE_NAME, "Invalid pipe parameters", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/*
	 * Ensure max number of bytes is a multiple of 8. This allows D64 block transfers over VME bus
	 * without any odd byte or word transfers at the end of each block. For very short messages
	 * (e.g. <= about 8 bytes) its better not to add the overhead of additional padding bytes
	 */

	if (nBytes > 8) nBytes += nBytes % 8;

	semTake (mpPipeMutexSem, WAIT_FOREVER);
	returnValue = mp_ExecuteCommand (pName, MP_PIPE_FUNC_CREATE, nMessages, nBytes, & errorNumber);
	semGive (mpPipeMutexSem);

	return (returnValue);
}

/*+
 *	FUNCTION NAME:
 *	mpPipeDevDelete
 *
 *	INVOCATION:
 *	mpPipeDevDelete (pName)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName	(char *)	name of pipe device to delete
 *
 *	FUNCTION VALUE:
 *	(STATUS)  OK, or ERROR if the pipe could not be deleted.
 *
 *	PURPOSE:
 *	Delete multi-processor pipe device
 *
 *	DESCRIPTION:
 *	This routine deletes a previous-created pipe device and removes it from
 *	the IO system device table.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	The pipe must have been closed for both reading and writing before
 *	it can be deleted.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS	mpPipeDevDelete
	(
	char *	pName
	)
{
	uint32	errorNumber;
	STATUS	returnValue;

	/* Check validity of pipe name */

	if (strlen (pName) > MP_PIPE_MAX_BYTES_NAME)
	{
		ERROR_SET (S_mpPipeDrv_INVALID_PIPE_NAME, "Invalid pipe name", ERROR_LOG_SAVE);
		return (ERROR);
	}

	semTake (mpPipeMutexSem, WAIT_FOREVER);
	returnValue = mp_ExecuteCommand (pName, MP_PIPE_FUNC_DELETE, 0, 0, & errorNumber);
	semGive (mpPipeMutexSem);

	return (returnValue);
}

/*+
 *	FUNCTION NAME:
 *	mpPipeOpen
 *
 *	INVOCATION:
 *	mpPipeOpen (deviceHeader, pRemainder, flags)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	deviceHeader	(DEV_HDR)	pointer to device descriptor header
 *	(>)	pRemainder		(char *)	unrecognised part of pipe name
 *	(>)	flags			(int)		read/write mode, O_RDONLY or O_WRONLY
 *
 *	FUNCTION VALUE:
 *	(int)  File descriptor (fd) for pipe, or ERROR if the pipe open failed.
 *
 *	PURPOSE:
 *	Open pipe for reading or writing
 *
 *	DESCRIPTION:
 *	This routine is normally called directly by the IO system (rather than
 *	by an application program) in response to a call to open(). Its purpose
 *	is to open a multi-processor pipe for reading or writing and allocated a
 *	file descriptor from the IO system's "fd table".
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	This routine is only available globally if the pipe driver was compiled
 *	with the NO_INSTALL option flag defined. The pipe must have been
 *	previously created but not already opened in the requested mode by any
 *	other processor on the bus.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

int mpPipeOpen
	(
	DEV_HDR *	deviceHeader,
	char *		pRemainder,
	int			flags
	)
{
	uint32			errorNumber;
	STATUS			returnValue = ERROR;
	MP_PIPE_DESC *	pPipeDesc;
	char			pName [MP_PIPE_MAX_BYTES_NAME + 1];
	SYM_TYPE		symType;

	/*
	 * pRemainder is the part of the pipe device name which was passed to the IO system that
	 * did not match a known device. Require that there is always a perfect match so this
	 * should be a null string
	 */

	if (strlen (pRemainder) > 0)
	{
		ERROR_SET (S_mpPipeDrv_INVALID_PIPE_NAME, "Invalid pipe name", ERROR_LOG_SAVE);
		return ((int) returnValue);
	}

	semTake (mpPipeMutexSem, WAIT_FOREVER);

	/*
	 * Get pipe name associated with device descriptor from symbol table and attempt to open pipe.
	 * mp_ExecuteCommand() will return a valid file descriptor (int cast as STATUS type) if the open
	 * is successful, otherwise it returns ERROR.
	 */

	if ( (symFindByValue (mpPipeSymTab, (UINT) deviceHeader, pName, (int *) & pPipeDesc, & symType)
	      == ERROR) ||
		 ((int) deviceHeader != (int) pPipeDesc) ||
		 ((returnValue = mp_ExecuteCommand ((char *) pName, MP_PIPE_FUNC_OPEN, flags, 0,
							& errorNumber))
	      == ERROR))
	{
		ERROR_SET (0, "Unable to open pipe", ERROR_LOG_SAVE);
	}

	semGive (mpPipeMutexSem);
	return ((int) returnValue);
}

/*+
 *	FUNCTION NAME:
 *	mpPipeClose
 *
 *	INVOCATION:
 *	mpPipeClose (fd)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	fd	(int)	file descriptor
 *
 *	FUNCTION VALUE:
 *	(int)  OK, or ERROR if the pipe close failed.
 *
 *	PURPOSE:
 *	Close pipe
 *
 *	DESCRIPTION:
 *	This routine is normally called directly by the IO system (rather than
 *	by an application program) in response to a call to close(). Its purpose
 *	is to close a multi-processor pipe for reading or writing (depending on
 *	the mode in which it was previously opened when the specifed fd was
 *	allocated). The given fd is removed from the IO system's "fd table".
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	This routine is only available globally if the pipe driver was compiled
 *	with the NO_INSTALL option flag defined. The pipe must have been created
 *	and opened for reading or writing.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

int mpPipeClose
	(
	int				fd
	)
{
	uint32			errorNumber;
	char			pName [MP_PIPE_MAX_BYTES_NAME + 1];
	MP_PIPE_DESC *	pPipeDesc;
	SYM_TYPE		symType;
	STATUS			returnValue;

	semTake (mpPipeMutexSem, WAIT_FOREVER);

	/* Get pipe name associated with device descriptor from symbol table and attempt to close pipe */

	if ((symFindByValue (mpPipeSymTab, (UINT) fd, pName, (int *) & pPipeDesc, & symType) == ERROR)
		|| (fd != (int) pPipeDesc))
	{
		ERROR_SET (0, "Can't find pipe file descriptor in symbol table", ERROR_LOG_SAVE);
		returnValue = ERROR;
	}

	else if ((returnValue = mp_ExecuteCommand (pName, MP_PIPE_FUNC_CLOSE, pPipeDesc->localOpenMode,
			 0, & errorNumber)) == OK)
	{
		pPipeDesc->localOpenMode = -1;
	}

	semGive (mpPipeMutexSem);
	return (returnValue);
}

/*+
 *	FUNCTION NAME:
 *	mpPipeIoctl
 *
 *	INVOCATION:
 *	mpPipeIoctl (fd, function, arg)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	fd			(int)	device descriptor
 *	(>)	function	(int)	control function to execute on pipe
 *	(><) arg		(int)	usage depends on function, may be input or output
 *
 *	FUNCTION VALUE:
 *	(int)  OK, or ERROR if the pipe control-function failed.
 *
 *	PURPOSE:
 *	Perform an IO control function
 *
 *	DESCRIPTION:
 *	This routine is normally called directly by the IO system (rather than
 *	by an application program) in response to a call to ioctl(). Its purpose
 *	is to perform an IO control function on a multi-processor pipe. The
 *	following IO control functions are supported by the driver:
 *
 *		FIOGETNAME		=>	Get name of pipe associated with fd. The name is
 *							copied to (char *) arg, which must therefore point to
 *							a buffer large enough to receive the information.
 *		FIONREAD		=>	Get number of bytes ready to read. Number is copied
 *							to (int *) arg.
 *		FIONMSGS		=>	If arg is zero on entry, get the number of messages
 *							in the local message queue. If arg is non-zero, get
 *							the number of empty message slots in the local queue.
 *							The number of messages, or emoty slots, is copied to
 *							(int *) arg.
 *		FIOFLUSH		=>	Flush out write path for pipe. The pipe must be
 *							opened to write by the issuing processor.
 *		FIOSETOPTIONS	=>	Set pipe options. Only one option is currently defined,
 *							this is an optional semaphore which may be given
 *							whenever a message packet is read from VME bus or
 *							written to VME bus. The semaphore type may be
 *							counting or binary, but a mutex semaphore may NOT be
 *							specified. The semaphore ID is passed via arg. A SEM_ID
 *							value of NULL disables the semGive(), and this is the
 *							default SEM_ID set when a pipe is first opened.
 *		FIOATTRIBSET	=>	Set pipe attributes. Only the "Pipe Invalid" attribute
 *							may be set and no arguments are therefore taken to this
 *							function. When marked as invalid, a pipe becomes
 *							unavailable for any further use. This is normally
 *							performed only internally by the driver in the event of
 *							an error and the function should be used only for
 *							debugging.
 *		FIOFSTATGET		=>	Get "file status" for pipe. The file status structure
 *							is defined in mpPipeDrv.h and contains the ID numbers
 *							of those processors to which the pipe is
 *							currently opened to read and write, as well as the
 *							validity status flag (which should normally be TRUE)
 *							and a pointer to the local buffer allocated for the
 *							driver.
 *		FIOSELECT		=>	This function is called only by the select library via
 *						 	the IO system.
 *		FIOUNSELECT	=>		This function is called only by the select library via
 *							the IO system.
 *
 *	The above function numbers (# defines) are defined in ioLib.h.
 *
 *	SPECIFYING A PIPE BY NAME RATHER THAN FILE DESCRIPTOR:
 *	In some circumstances it may be necessary to perform ioctl() functions on a
 *	pipe which has not been opened by the local processor. For example, whilst
 *	polling for a remote processor to open a pipe; in this case there is no local
 *	file descriptor assigned to the pipe so that ioctl() cannot be used. An
 *	alternative function, mpPipeIoctlByName(), is therefore provided to meet such
 *	requirements. mpPipeIoctlByName() is passed the name of a pipe, rather than a file
 *	descriptor, and may therefore be used immediately the pipe has been created.
 *	See the manual entry for mpPipeIoctl() for further details.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	This routine is only available globally if the pipe driver was compiled
 *	with the NO_INSTALL option flag defined. The pipe must have been previously
 *	created.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

int mpPipeIoctl
	(
	int 			fd,
	int				function,
	int				arg
	)
{

	SYM_TYPE			symType;
	MP_PIPE_DESC *		pPipeDesc;
	FAST int			i;
	int					flags;
	char				pName [MP_PIPE_MAX_BYTES_NAME + 1];
	int *				pNByte;
	int *				pNMsg;
	MP_PIPE_STATUS *	pPipeStatus;
	STATUS				returnValue;
	uint32				errorNumber;

	semTake (mpPipeMutexSem, WAIT_FOREVER);
	returnValue = OK;								/* Default assumption */

	/*
	 * Get pipe descriptor from symbol table.
	 */

	if ((symFindByValue (mpPipeSymTab, (UINT) fd, pName, (int *) & pPipeDesc, & symType) == OK)
		&& (fd == (int) pPipeDesc))
	{
		flags = pPipeDesc->localOpenMode;
		if (symType == MP_PIPE_SYM_TYPE_ALT) pPipeDesc = (MP_PIPE_DESC *) pPipeDesc->pOtherPipeDef;
	}
	else
	{
		returnValue = ERROR;
		semGive (mpPipeMutexSem);
		return (returnValue);
	}

	/* Switch on the requested IO control function */

	switch (function)
	{
		case FIOGETNAME:						/* Get name of pipe device associated with fd */

			/* strcpy replaced by strncpy. SMB - 18 Feb 98. */
			strncpy ((char *) arg, pName, MP_PIPE_MAX_BYTES_NAME);
			break;

		case FIONREAD:							/* Get number bytes ready to read.. */
			pNByte = (int *) arg;
			if (flags != O_RDONLY)
			{
				returnValue = ERROR;			/* ..can only do this if the pipe is open to read */
			}
			else
			{

				/*
				 * Search through list of byte-count entries and find the oldest entry.
				 * Copy this into (* pNByte) as the number of bytes ready to read
				 */

				for (i = pPipeDesc->maxNMsg - 1; i >= 0; i--)
				{
					if ((* pNByte = pPipeDesc->pNByteReady [i]) != -1)
						i = -1;
				}
				if (* pNByte == -1) * pNByte = 0;	/* If there are no bytes ready, then set zero */
			}
			break;

		case FIONMSGS:								/* Get number of message slots in local queue */
			pNMsg = (int *) arg;

			/*
			 * If the contents of arg were non-zero on entry, then the number of empty message
			 * slots is requested, otherwise return the number of full message slots.
			 */

			if (* pNMsg != 0)
			{
				* pNMsg = pPipeDesc->maxNMsg - msgQNumMsgs (pPipeDesc->msgQId);
			}
			else
			{
				* pNMsg =  msgQNumMsgs (pPipeDesc->msgQId);
			}
			break;

		case FIOFLUSH:								/* Flush out write path on pipe */
			returnValue = mp_ExecuteCommand ((char *) pName, MP_PIPE_FUNC_FLUSH, 0, 0, & errorNumber);
			break;

		case FIOATTRIBSET:							/* Set "Pipe Invalid" attribute bit */
			returnValue = mp_ExecuteCommand ((char *) pName, MP_PIPE_FUNC_INVALIDATE, 0, 0,
			                              & errorNumber);
			break;

		case FIOSETOPTIONS:							/* Set Buffer-Ready semaphore. */
			pPipeDesc->bufferReadySem = (SEM_ID) arg;
			break;

		case FIOFSTATGET:							/* Get current status info for pipe */
			pPipeStatus = (MP_PIPE_STATUS *) arg;
			pPipeStatus->procIdRead = pPipeDesc->procIdRead;
			pPipeStatus->procIdWrite = pPipeDesc->procIdWrite;
			pPipeStatus->pipeValid = pPipeDesc->pipeValid;
			pPipeStatus->pBuffer = pPipeDesc->pLocalBuffer + OFFSET_TO_FIRST_BYTE;
			break;

		case FIOSELECT:								/* Used only by selectLib */
			if ((returnValue = selNodeAdd (& pPipeDesc->selWakeupList, (SEL_WAKEUP_NODE *) arg))
				== OK)
			{
				if (selWakeupType ((SEL_WAKEUP_NODE *) arg) == SELREAD &&
					pPipeDesc->procIdRead == sysextProcNumGet () &&
					msgQNumMsgs (pPipeDesc->msgQId) > 0)
				{
					selWakeup ((SEL_WAKEUP_NODE *) arg);
				}
				else if (selWakeupType ((SEL_WAKEUP_NODE *) arg) == SELWRITE &&
					pPipeDesc->procIdWrite == sysextProcNumGet () &&
					(pPipeDesc->maxNMsg - msgQNumMsgs (pPipeDesc->msgQId)) > 0)
				{
					selWakeup ((SEL_WAKEUP_NODE *) arg);
				}
			}
			break;

		case FIOUNSELECT:								/* Used only by selectLib */
			selNodeDelete (& pPipeDesc->selWakeupList, (SEL_WAKEUP_NODE *) arg);
			break;

		default:
			returnValue = ERROR;
	}

	if (returnValue == ERROR)
		ERROR_SET (S_mpPipeDrv_IOCTL_FAILED, "Cannot execute ioctl() function", ERROR_LOG_SAVE);

	semGive (mpPipeMutexSem);
	return (returnValue);
}

/*+
 *	FUNCTION NAME:
 *	mpPipeIoctlByName
 *
 *	INVOCATION:
 *	mpPipeIoctlByName (pName, function, arg)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName		(char *)	pipe name
 *	(>)	function	(int)		control function to execute on pipe
 *	(><) arg		(int)		usage depends on function, may be input or output
 *
 *	FUNCTION VALUE:
 *	(int)  OK, or ERROR if the pipe control-function failed.
 *
 *	PURPOSE:
 *	Perform an IO control function on a pipe specified by name
 *
 *	DESCRIPTION:
 *	This routine is used as an alternative to ioctl() (or mpPipeIoctl()) when
 *	it is necessary to perform an IO control function on a pipe which has not
 *	been opened (to read or write) by the local processor; the pipe is specified
 *	by name rather than by its file descriptor. Only three functions are currently
 *	implemented, since the other control functions supported by ioctl() (see
 *	the manual entry for mpPipeIoctl()) are only meaningful on an open pipe. The
 *	supported functions are:
 *
 *		FIOATTRIBSET	=>	Set pipe attributes. Only the "Pipe Invalid" attribute
 *							may be set and no arguments are therefore taken to this
 *							function. When marked as invalid, a pipe becomes
 *							unavailable for any further use. This is normally
 *							performed only internally by the driver in the event of
 *							an error and the function should be used only for
 *							debugging. 
 *		FIOFSTATGET		=>	Get "file status" for pipe. The file status structure
 *							is defined in mpPipeDrv.h and contains the ID numbers
 *							of those processors (if any) to which the pipe is
 *							currently opened to read and write, as well as the
 *							validity status flag (which should normally be TRUE).
 *		FIOSETOPTIONS	=>	Set pipe options. Only one option is currently defined,
 *							this is an optional semaphore which may be given
 *							whenever a message packet is read from VME bus or
 *							written to VME bus. The semaphore type may be
 *							counting or binary, but a mutex semaphore may NOT be
 *							specified. The semaphore ID is passed via arg. A SEM_ID
 *							value of NULL disables the semGive(), and this is the
 *							default SEM_ID set when a pipe is first opened.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	This routine is only available globally if the pipe driver was compiled
 *	with the NO_INSTALL option flag defined. The pipe must have been previously
 *	created.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

int mpPipeIoctlByName
	(
	char *	pName,
	int		function,
	int		arg
	)
{

	SYM_TYPE			symType;
	MP_PIPE_DESC *		pPipeDesc;
	MP_PIPE_STATUS *	pPipeStatus;
	STATUS				returnValue;
	uint32				errorNumber;

	semTake (mpPipeMutexSem, WAIT_FOREVER);
	returnValue = OK;										/* Default assumption */

	/*
	 * Get pipe descriptor from symbol table.
	 */

	if ((returnValue = symFindByNameAndType (mpPipeSymTab, pName, (char **) & pPipeDesc, & symType,
											 MP_PIPE_SYM_TYPE_NORMAL, MP_PIPE_SYM_TYPE_MASK)) == OK)
	{
		switch (function)
		{
			case FIOATTRIBSET:								/* Set "Pipe Invalid" attribute bit */
				returnValue = mp_ExecuteCommand ((char *) pName, MP_PIPE_FUNC_INVALIDATE, 0, 0,
				                              & errorNumber);
				break;

			case FIOFSTATGET:								/* Get current status info for pipe */
				pPipeStatus = (MP_PIPE_STATUS *) arg;
				pPipeStatus->procIdRead = pPipeDesc->procIdRead;
				pPipeStatus->procIdWrite = pPipeDesc->procIdWrite;
				pPipeStatus->pipeValid = pPipeDesc->pipeValid;
				break;

			case FIOSETOPTIONS:								/* Set Buffer-Ready semaphore. */
				pPipeDesc->bufferReadySem = (SEM_ID) arg;
				break;

			default:
				returnValue = ERROR;
		}
	}
	if (returnValue == ERROR)
		ERROR_SET (S_mpPipeDrv_IOCTL_FAILED, "Cannot execute ioctl() function", ERROR_LOG_SAVE);
	semGive (mpPipeMutexSem);
	return (returnValue);
}

/*+
 *	IGNORED FUNCTION NAME:
 *	mp_mailboxIsr
 *
 *	INVOCATION:
 *	mp_mailboxIsr (argument)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>) argument	(int)	mandatory argument to ISR, not used in the application
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Issue counting semaphore on receipt of mailbox interrupt
 *
 *	DESCRIPTION:
 *	This is the interrupt service routine used by the multi-processor pipe
 *	driver. On receipt of an interrupt, a counting semaphore is given thus
 *	incrementing a count of the number of interrupts received. This semaphore
 *	is monitored (taken) by the multi-processor pipe daemon task which is
 *	responsible for handling mailbox interrupt.
 *
 *	EXTERNAL VARIABLES:
 *	(>)	mpPipeMboxIrq	(SEM_ID)	ID of counting semaphore
 *
 *	PRIOR REQUIREMENTS:
 *	The interrupt service routine must have been installed, and mailbox
 *	interrupts enabled, by a previous call to mpPipeDrv().
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

void mp_mailboxIsr
	(
	int	argument
	)
{
	semGive (mpPipeMboxIrq);					/* NB. Semaphore type is Counting */
}

/*+
 *	IGNORED FUNCTION NAME:
 *	mp_ExecuteCommand
 *
 *	INVOCATION:
 *	mp_ExecuteCommand (pName, function, arg1, arg2, pErrorNumber)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName			(char *)	name of pipe
 *	(>)	function		(int)		function to broadcast
 *	(>)	arg1			(int)		argument #1 to function
 *	(>) arg2			(int)		argument #2 to function
 *	(<)	pErrorNumber	(uint32 *)	error number, 0 if no error
 *
 *	FUNCTION VALUE:
 *	(STATUS)  File descriptor if the function is pipe-create
 *			  or pipe-open and the function executed successfully. All other
 *			  functions return OK if they are successful. ERROR is returned if
 *			  the function was not executed successfully.
 *
 *	PURPOSE:
 *	Execute a multi-processor pipe control function locally and broadcast
 *	the command to all other processors on VME bus
 *
 *	DESCRIPTION:
 *	This routine executes a specified command such as a pipe create, delete,
 *	open, close or flush command then broadcasts the command to all other
 *	processors on the bus and waits for each processor to execute the
 *	command locally. The command is only deemed to have been successful if
 *	every processor generates an expected response to the command. In the
 *	event that the command fails on one or more processors (e.g. due to the
 *	failure of a processor) then the pipe is marked as invalid and thus made
 *	unavailable for further use.
 *
 *	EXTERNAL VARIABLES:
 *	(!)	ppRemoteCmdFlag	(volatile char **)	Array of command flag pointers
 *
 *	PRIOR REQUIREMENTS:
 *	The pipe driver must have been previously initialised.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS mp_ExecuteCommand
	(
	char *		pName,
	uint32		function,
	int			arg1,
	int			arg2,
	uint32 *	pErrorNumber
	)
{
	BOOL			timeout;
	BOOL			remoteCommandSuccessful;
	FAST int		procNumber;
	SYM_TYPE		symType;
	MP_PIPE_DESC *	pPipeDesc;
	int				fd;
	int				nProcToReceiveCmd;
	struct timespec	timeStart;

	/*
	 * NB. The arguments to this routine are know to be valid on entry.
	 * Initialise timeStart prior to timeout test, then grab Busy flag in MCS prior to
	 * executing the command
	 */

	timeout = FALSE;
	START_TIMEOUT (& timeStart);									/* Initialise the timeout timer */

	remoteCommandSuccessful = TRUE;
	* pErrorNumber = 0;
	nProcToReceiveCmd = nProcOnBus;									/* Assume the command will */
																	/* be broadcast to all other */
																	/* processors on the bus */

	/*
	 * Grab Busy flag in MCS before attempting to execute the command. If the flag is
	 * not available (in use by another processor), then give up the mutex semaphore
	 * whilst waiting for the flag (or timeout) since this semaphore will be needed
	 * by the daemon task in order to handle incoming interrupts. This is very
	 * important since an incoming command may be the reason why the Busy flag is
	 * not available, and we need to avoid a lock-out situation in which the daemon
	 * task cannot handle an incoming command and thus allow a remote processor to
	 * complete its command execution and release the Busy flag.
	 */

	while (! sysBusTas ((char *) & pMasterControlMsg->busyFlag) &&
		   ! (timeout = timeoutExpired (MP_PIPE_TIMEOUT_COMMAND, & timeStart)))
	{
		semGive (mpPipeMutexSem);
		taskDelay (SEC_TO_NTICK (MP_PIPE_DELAY_COMMAND));
		semTake (mpPipeMutexSem, WAIT_FOREVER);
	}

	if (timeout)
	{
		* pErrorNumber = 1;
		return (ERROR);
	}

	/* Lookup pipe descriptor only if it already exists (pipe has previously been created) */

	if (function != MP_PIPE_FUNC_CREATE)
		symFindByNameAndType (mpPipeSymTab, pName, (char **) & pPipeDesc, & symType,
							  MP_PIPE_SYM_TYPE_NORMAL, MP_PIPE_SYM_TYPE_MASK);

	/*
	 * Write ID of this processor and pipe name to MCS ready to issue command to other
	 * processors on the bus
	 */

	/* Set proc id for source of command = this proc */
	pMasterControlMsg->procIdSource = sysextProcNumGet ();

	/* strcpy replaced by strncpy. SMB - 18 Feb 98. */
	strncpy ((char *) pMasterControlMsg->pName, pName, MP_PIPE_MAX_BYTES_NAME);

#ifdef DEBUG
	printf ("mp_ExecuteCommand: about to execute command #%d locally\n", function);
#endif /* DEBUG */

	/*
	 * Switch on the required function and first attempt to execute the command locally locally. If
	 * the command is successful on this (local) processor, then set up other function-specific
	 * arguments in the MCS ready to issue it to all other processors on the bus
	 */

	switch (function)
	{
	case MP_PIPE_FUNC_CREATE:								/* Create pipe */
		if (mp_LocalCreate (pName, arg1, arg2, pErrorNumber) == ERROR)
		{
			* pErrorNumber = 1;
		}
		else
		{
			pMasterControlMsg->function = MP_PIPE_FUNC_CREATE;
			pMasterControlMsg->maxNMsg = (uint32) arg1;
			pMasterControlMsg->maxNBytePerMsg = (uint32) arg2;

			/* Get descriptor for this newly-created pipe */

			symFindByNameAndType (mpPipeSymTab, pName, (char **) & pPipeDesc, & symType,
								  MP_PIPE_SYM_TYPE_NORMAL, MP_PIPE_SYM_TYPE_MASK);
			fd = (int) pPipeDesc;
		}
		break;

	case MP_PIPE_FUNC_DELETE:								/* Delete pipe */
		if (mp_LocalDelete (pName, pErrorNumber) == ERROR)
		{
			mp_LocalInvalidate (pName, pErrorNumber);
			* pErrorNumber = 1;
		}
		else
		{
			pMasterControlMsg->function = MP_PIPE_FUNC_DELETE;
		}
		break;

	case MP_PIPE_FUNC_INVALIDATE:							/* Set Invalid flag for pipe */
		if (mp_LocalInvalidate (pName, pErrorNumber) == ERROR)
		{
			* pErrorNumber = 1;
		}
		else
		{
			pMasterControlMsg->function = MP_PIPE_FUNC_INVALIDATE;
		}
		break;

	case MP_PIPE_FUNC_OPEN:									/* Open pipe to read or write */
		if (mp_localopen (pName, arg1, sysextProcNumGet (), pErrorNumber) == ERROR)
		{
			* pErrorNumber = 1;
		}
		else
		{
			if ((arg1 == O_WRONLY) && (pPipeDesc->openCountWrite == 1))
			{
				/*
				 * If this is the first time the pipe has been opened to write by this
				 * processor, then this open command should be broadcast to all other
				 * processors on the bus.
				 */

				pMasterControlMsg->function = MP_PIPE_FUNC_OPEN_WR;
			}
			else if (pPipeDesc->openCountRead == 1)
			{
				/*
				 * If this is the first time the pipe has been opened to read by this
				 * processor, then this open command should be broadcast to all other
				 * processors on the bus.
				 */

				pMasterControlMsg->function = MP_PIPE_FUNC_OPEN_RD;

				/*
				 * If opened for reading, put the local Busy flag in the MCS to enable other 
				 * processors on the bus to get the location of destination data if/when they
				 * open for writing
				 */

				pMasterControlMsg->function = MP_PIPE_FUNC_OPEN_RD;

				if (sysextLocalToBusAdrs (VME_AM_EXT_SUP_DATA, (char *) pPipeDesc->pLocalBuffer,
										  (char **) & pMasterControlMsg->pRemoteWriteBuffer) == ERROR)
				{
					* pErrorNumber = 1;
					printf ("mp_ExecuteCommand: Error in sysextLocalToBusAdrs\n");
				}

				/*
				 * Set the local Queue flag to point to the local VME buffer. Other (remote) 
				 * processors will set their Queue flags to point to this same location when they
				 * call mp_localopen(). Then initialise the Busy, Done and Queue flags all to zero
				 * and flush these out of the local VME buffer.
				 */

				pPipeDesc->pQueueFlag = pPipeDesc->pLocalBuffer + OFFSET_TO_QUEUE_FLAG;

				* pPipeDesc->pLocalBuffer = 0;
				* pPipeDesc->pLocalDoneFlag = 0;
				* pPipeDesc->pQueueFlag = 0;

				CACHE_DMA_FLUSH ((char *) pPipeDesc->pLocalBuffer,
								 (int) (pPipeDesc->pLocalDoneFlag + 1) -
				                 (int) pPipeDesc->pLocalBuffer);
			}
			else
			{
				/*
				 * This must be the 2nd or subsequent time that the pipe has been locally
				 * opened in the given mode. There is therefore no need to re-broadcast the
				 * command to other processors on the bus.
				 */

				nProcToReceiveCmd = 0;
			}

			/*
			 * If the pipe was already opened by this processor so that it is now opened for both
			 * reading and writing by this same processor, then use the second pipe definition
			 * structure (pPipeDesc->pOtherPipeDef) as the file descriptor. This allows different file
			 * descriptors to be associated with the same pipe depending on whether it is being
			 * accessed to read or write. This situation does not arise when a pipe is opened to read
			 * and write by two different processors, in which case each processor uses the main
			 * pipeDef structure as the file descriptor.
			 */

			if (pPipeDesc->procIdRead == sysextProcNumGet () &&
				pPipeDesc->procIdWrite == sysextProcNumGet ())
			{
				((MP_PIPE_DESC *) pPipeDesc->pOtherPipeDef)->localOpenMode = arg1;
				fd = (int) pPipeDesc->pOtherPipeDef;
			}
			else
			{
				pPipeDesc->localOpenMode = arg1;
				fd = (int) pPipeDesc;
			}

		}
		break;

	case MP_PIPE_FUNC_CLOSE:									/* Close pipe */
		if (mp_localclose (pName, arg1, sysextProcNumGet (), pErrorNumber) == ERROR)
		{
			mp_LocalInvalidate (pName, pErrorNumber);
			* pErrorNumber = 1;
		}
		else
		{
			/*
			 * If the pipe has been completely closed (the open count has reached
			 * zero), then prepare to broadcast this close command to all other
			 * processors on the bus. Otherwise, prepate to skip over broadcasting
			 * the command.
			 */

			if (arg1 == O_WRONLY && pPipeDesc->openCountWrite == 0)
			{
				pMasterControlMsg->function = MP_PIPE_FUNC_CLOSE_WR;
			}
			else if (pPipeDesc->openCountRead == 0)
			{
				pMasterControlMsg->function = MP_PIPE_FUNC_CLOSE_RD;
			}
			else
			{
				nProcToReceiveCmd = 0;
			}
		}
		break;

	case MP_PIPE_FUNC_FLUSH:									/* Flush pipe */

		/* Flush command can only by initiated by the processor which has opened it for writing */

		if (pPipeDesc->procIdWrite != sysextProcNumGet () || mp_localflush (pName, pErrorNumber) == ERROR)
		{
			* pErrorNumber = 1;
		}
		else
		{
			pMasterControlMsg->function = MP_PIPE_FUNC_FLUSH;
		}
		break;

	default:													/* Unknown function */
		ERROR_SET (S_mpPipeDrv_INVALID_PIPE_FUNC, "Unknown mpPipeDrv function", ERROR_LOG_SAVE);
		* pErrorNumber = 1;
		break;
	}

	if (* pErrorNumber != 0) nProcToReceiveCmd = 0;

	/*
	 * Ready to issue the command to every other processor on the bus. nProcToReceiveCmd
	 * is normally equal to the number of processors on the bus, but if any errors were
	 * encountered above, or if the command is an Nth open or close which does not need to
	 * be broadcast to other processors, then nProcToReceiveCmd will be set to zero, thus
	 * skipping over the broadcast. Assume that a valid response from each procesor will follow.
	 * NB. If the command is anything other than FLUSH, then it is issued to all processors
	 * except for this one. If the command is FLUSH, then it is only issued to whichever
	 * processor (if any) has the pipe open to read.
	 */

	for (procNumber = 0; procNumber < nProcToReceiveCmd; procNumber++)
	{
		if ( ( (function != MP_PIPE_FUNC_FLUSH) ||
			   ( (function == MP_PIPE_FUNC_FLUSH) &&
		         (procNumber == pPipeDesc->procIdRead)
		       )
		     ) &&
			 (procNumber != sysextProcNumGet ())
		   )
		{

			/*
			 * Prepare to issue the command. Set the Done flag, destination processsor ID and
			 * clear the error number (any error is reported here by the destination proc). Also,
			 * set the destination processor's Command flag which is maintained in a cache-safe
			 * area of its local RAM. The destination processor will examine this flag on receipt
			 * of a mailbox interrupt, and, in this case, will find that the flag is set to
			 * indicate that a command is being executed by another processor (by THIS processor).
			 * Remember to flush-out the cache.
			 */

			pMasterControlMsg->doneFlag = 0;
			pMasterControlMsg->procIdDest = procNumber;					
			pMasterControlMsg->errorNumber = 0;
			CACHE_DMA_FLUSH ((char *) & pMasterControlMsg->doneFlag, sizeof (MP_PIPE_CONTROL_MSG));

			* ppRemoteCmdFlag [procNumber] = 1;

			timeout = FALSE;										/* Prepare for timeout test.	*/
			START_TIMEOUT (& timeStart);							/* Initialise the timeout timer.*/
			if (sysextMboxIntGen (procNumber, MP_PIPE_MAILBOX) == ERROR)
																	/* Issue command Mbox IRQ.		*/
			{
				* pErrorNumber = 1;
				remoteCommandSuccessful = FALSE;
			}
			else
			{

				/* Poll for a reply from the destination processor; timeout if necessary */

				CACHE_DMA_INVALIDATE ((char *) & pMasterControlMsg->doneFlag,
				                      sizeof (MP_PIPE_CONTROL_MSG));
				while (pMasterControlMsg->doneFlag == 0 &&
					   ! (timeout = timeoutExpired (MP_PIPE_TIMEOUT_COMMAND, & timeStart)))
				{
					taskDelay (SEC_TO_NTICK (MP_PIPE_DELAY_COMMAND));
					CACHE_DMA_INVALIDATE ((char *) & pMasterControlMsg->doneFlag,
					                      sizeof (MP_PIPE_CONTROL_MSG));
				}
				if (timeout)
				{
					* pErrorNumber = 1;
					remoteCommandSuccessful = FALSE;
				}
				else if (pMasterControlMsg->errorNumber != 0)
				{
					* pErrorNumber = 1;
					remoteCommandSuccessful = FALSE;
				}
			}

			/*
			 * If, for any reason, the remote processor was not successful in executing the command,
			 * mark the pipe (locally) as invalid. This will at least prevent this local processor
			 * from any further use of what must be regarded as a suspect pipe. Its up to other
			 * processors to figure out that the pipe is no use: any attempt to broadcast this
			 * Invalidate command is likely to cause all sorts of problems since we know that the
			 * multi-processor network is not very well at this stage. Better to let the other
			 * processors fend for themselves.
			 */

			if (! remoteCommandSuccessful)
			{
				mp_LocalInvalidate (pName, pErrorNumber);
				procNumber = nProcToReceiveCmd;					/* Abort loop over processors */
			}
		}
	}
#ifdef DEBUG
printf ("mp_ExecuteCommand: releasing master control structure\n");
#endif /* DEBUG */

	pMasterControlMsg->busyFlag = 0;							/* Release Master ctrl structure */
	CACHE_DMA_FLUSH ((char *) & pMasterControlMsg->busyFlag, 4);

	if (! remoteCommandSuccessful || * pErrorNumber != 0)
	{
		ERROR_SET (0, ERROR_MSG_NONE, ERROR_LOG_SAVE);
		return (ERROR);
	}
	else if (function == MP_PIPE_FUNC_CREATE || function == MP_PIPE_FUNC_OPEN)
	{
		return (fd);
	}
	else
	{
		return (OK);
	}
}

/*+
 *	FUNCTION NAME:
 *	mpPipeShow
 *
 *	INVOCATION:
 *	mpPipeShow (pName)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName	(char *)	name of pipe
 *
 *	FUNCTION VALUE:
 *	(STATUS)  OK, or ERROR if the pipe name was not recognised.
 *
 *	PURPOSE:
 *	Print summary of status of current multi-processor pipe(s)
 *	to standard output
 *
 *	DESCRIPTION:
 *	This routine prints a summary of the current status of one or more
 *	multi-processor pipes. If the specified name string is NULL, then
 *	the status of all multi-processor pipes know to the system is displayed.
 *	If the name string is that of an existing pipe then only the details of
 *	that pipe are shown and in this case further details of the pipe's local
 *	message queue are also displayed. The information shown consists of
 *v	
 *v	Column #1	- Name of pipe
 *v	Column #2	- Pipe ID = pointer to pipe descriptor structure
 *v	Column #3	- Message queue ID for local message queue
 *v	Column #4	- ID number of processors which has pipe opened to read
 *v	Column #5	- ID number of processors which has pipe opened to write
 *v	Column #6	- Number of local message queue transactions since openeing
 *v	Column #7	- Status flags - "UVBDQ" = Unbuffered, Valid, Busy, Done, Queued
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	The pipe driver must have been previously initialised.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS mpPipeShow
	(
	char *	pName
	)
{
	SYM_TYPE		symType;
	MP_PIPE_DESC *	pPipeDesc;

	if (mpPipeMutexSem == NULL)
	{
		printf ("mpPipeShow: driver not initialised\n");
		return (ERROR);
	}

	semTake (mpPipeMutexSem, WAIT_FOREVER);

	printf ("     Pipe Name          Pipe ID   Msg-Q ID   Rd Wr Transactions Flags (UVBDQ)\n");
	printf ("--------------------- ---------- ----------  ----- ------------ -------------\n");

	/*
	 * If no pipe name given print details of all pipes,
	 * otherwise only show those of the named pipe.
	 */

	if ((int) pName == 0)
	{
		symEach (mpPipeSymTab, (FUNCPTR) mp_PrintPipeDetails, 0);
	}
	else
	{							/* Get pipe descriptor from symbol table */

		if (symFindByNameAndType (mpPipeSymTab, pName, (char **) & pPipeDesc, & symType,
								  MP_PIPE_SYM_TYPE_NORMAL, MP_PIPE_SYM_TYPE_MASK) == OK)
		{
			mp_PrintPipeDetails (pName, (int) pPipeDesc, symType, 0, (UINT16) 0);

			if (pPipeDesc->pLocalBuffer == NULL)
				printf ("\nLocal buffer (data) : NULL (pipe not locally open)\n");
			else
				printf ("\nLocal buffer (data) : 0x%x\n",
				         (int) pPipeDesc->pLocalBuffer + OFFSET_TO_FIRST_BYTE);

			if (pPipeDesc->unbuffered)
				printf ("Pipe type           : UN-BUFFERED\n");
			else
				printf ("Pipe type           : BUFFERED\n");

			if (pPipeDesc->bufferReadySem != NULL)
				printf ("Buffer-Ready Sem ID : 0x%x\n", (int)  pPipeDesc->bufferReadySem);

			if (pPipeDesc->msgQId != NULL)
				msgQShow (pPipeDesc->msgQId, 1);
		}
		else
		{
			semGive (mpPipeMutexSem);
			return (ERROR);
		}
	}
	printf ("\nMaster Control Structure is located at 0x%x", (int) pMasterControlMsg);
	if (sysextProcNumGet () != 0)
		printf (" (on VME bus)\n");
	else
		printf (" (in local RAM)\n");

	semGive (mpPipeMutexSem);
	return (OK);
}

/*+
 *	FUNCTION NAME:
 *	mp_PipeDaemon
 *
 *	INVOCATION:
 *	mp_PipeDaemon ()
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	None
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Multi-processor pipe daemon task. Handles mailbox interrupts
 *	from other processors on VME bus
 *
 *	DESCRIPTION:
 *	This routine executes as a daemon task with the multi-processor
 *	pipe driver. It is spawned by the driver's initialisation routine
 *	and immediately enters a "forever" loop in which it waits for mailbox
 *	interrupts and, on receipt of an interrupt (semaphore), it first
 *	assumes that the interrupt is due to an incoming command (from another
 *	processor and executes any such command. If the interrupt is not due
 *	to an incoming command the daemon then searches through all known pipes
 *	and examines each to see whether incoming data has been writtten from
 *	an external processor. Finally, if the cause of the interrupt remains
 *	unknown the daemon examines all known pipes to see whether outgoing data
 *	may be written by one of them to an external processor. This latter case
 *	arises, for example, when an external processor which was previously
 *	blocked for writing one a particular pipe (message queue full) un-blocks
 *	thus allowing this processor to de-queue any write requests that it may
 *	have previously queued.
 *
 *	ERROR HANDLING:
 *	This routine will report run-time errors via the error-logging library,
 *	errorLib. In the event of an such an error, the routine will not exit
 *	or return.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

void mp_PipeDaemon (void)
{
	BOOL	IRQServiced;
	uint32	errorNumber;

	/*
	 * First initialise error-logging for this daemon.
	 */

	if (errorInit () == ERROR)
	{
		printErr ("%s: mp_PipeDaemon: Failed to initialise error context structure\n",
		         taskName (taskIdSelf()));
		exit (ERROR);
	}

	/*
	 * Enter a loop handling post-initialisation IRQs. These occur when a remote processor reads
	 * or writes a pipe on this processor or if a remote processor issues a command (e.g. to
	 * create or open a pipe) to this processor
	 */

	FOREVER
	{
		semTake (mpPipeMboxIrq, WAIT_FOREVER);	/* Wait for mailbox interrupt semaphore.			*/
		semTake (mpPipeMutexSem, WAIT_FOREVER);	/* Exclude other tasks whilst servicing interrupt.	*/
		IRQServiced = FALSE;

		mp_LocalCommand (& IRQServiced, & errorNumber);
												/* Test for an incoming command.					*/
		if (errorNumber != 0)
			ERROR_LOG ("mpPipeDrv (daemon): local command error");

		if (! IRQServiced)
		{

			/*
			 * If no incoming command found, search through all known pipes and test for data being
			 * written to it from a remote processor
			 */

			symEach (mpPipeSymTab, (FUNCPTR) mp_LocalRead, (int) & IRQServiced);
			if (! IRQServiced)
			{

				/*
				 * If the IRQ is due to neither an incoming command or incoming data, test for
				 * data available to be written by this processor. This will be the case when
				 * a remote processor de-queues a write request which was previously queued by
				 * this processor 
				 */

				symEach (mpPipeSymTab, (FUNCPTR) mp_LocalWrite, (int) & IRQServiced);
			}
		}
		semGive (mpPipeMutexSem);
	}
}

/*+
 *	IGNORED FUNCTION NAME:
 *	mp_LocalRead
 *
 *	INVOCATION:
 *	mp_LocalRead (pName, symValue, symType, argument, group)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName		(char *)	name of pipe
 *	(>)	symValue	(int)		pipe descriptor (pointer, cast as int)
 *	(>)	symType		(SYM_TYPE)	symbol type associated with pipe descriptor
 *	(>)	argument	(int)		arbitrary argument passed from symEach()
 *	(>)	group		(UINT16)	symbol group type associated with pipe desc
 *
 *	FUNCTION VALUE:
 *	(BOOL)	FALSE if the pipe could be read, or TRUE if it could not.
 *
 *	PURPOSE:
 *	Attempt to read data from VME buffer into pipe message queue
 *
 *	DESCRIPTION:
 *	This routine examines a local pipe and determines whether it is ready
 *	to read data, and, if it is, whether there is data available (from a
 *	remote processor) in its VME buffer. If ready, the incoming data is copied
 *	into the pipe's message queue.
 *
 *	This routine is invoked either by the local processor or in response
 *	to a mailbox interrupt from a remote processor
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	The pipe must have been opened for reading by this processor and
 *	for writing by any processor on the bus.
 *
 *	DEFICIENCIES:
 *	None known
 *
 *	BUGS:
 *	Arguments "pName" and "group" are not referenced by this function.
 *	Are they present for future exhancement, to allow debugging, or
 *	simply for compatability with the other "local" functions, or is
 *	this a bug? SMB - 30 Jan 98.
 *-
 */

BOOL mp_LocalRead
	(
	char *		pName,
	int			symValue,
	SYM_TYPE	symType,
	int			argument,
	UINT16		group
	)
{
	MP_PIPE_DESC *	pPipeDesc;
	BOOL *			pIrqServiced;
	BOOL			localIrqServiced;
	uint32			nBytes;
	FAST int		i;

	/* Skip over non-primary symbol table entries */
	if (symType == MP_PIPE_SYM_TYPE_ALT) return (TRUE);

	pPipeDesc = (MP_PIPE_DESC *) symValue;
	pIrqServiced = (BOOL *) argument;
	if (pIrqServiced == NULL) pIrqServiced = & localIrqServiced;

	/*
	 * Test for pipe opened for reading by this proc, and writing by any processor (mp_localopen()
	 * ensures that the same processor cannot open a pipe for both reading and writing)
	 */

	if (pPipeDesc->procIdRead == sysextProcNumGet () && pPipeDesc->procIdWrite >= 0)
	{

		/* If opened, test for incoming data available and for an empty slot in the msg Q */

		CACHE_DMA_INVALIDATE ((char *) pPipeDesc->pLocalBuffer,
							  (int) (pPipeDesc->pLocalDoneFlag + 1) - (int) pPipeDesc->pLocalBuffer);

		if (* pPipeDesc->pLocalDoneFlag != 0 && msgQNumMsgs (pPipeDesc->msgQId) < pPipeDesc->maxNMsg)
		{
			if (pPipeDesc->bufferReadySem != NULL)		/* Give semaphore if defined (via ioctl()) */
				semGive (pPipeDesc->bufferReadySem);

			* pIrqServiced = TRUE;						/* Have detected reason for the Mbox IRQ */
			nBytes = * (pPipeDesc->pLocalBuffer + OFFSET_TO_NBYTES);

			/*
			 * If the pipe mode is Unbuffered, then the message queue is pass the number of bytes
			 * available in the incoming message. Otherwise, copy the incoming message to the queue.
			 */

			if (pPipeDesc->unbuffered)
			{
				if (msgQSend (pPipeDesc->msgQId, (char *) & nBytes, (UINT) 4, NO_WAIT, MSG_PRI_NORMAL)
				    == ERROR)
				{
					ERROR_SET (0, "Failed to send message to message queue", ERROR_LOG_SAVE);
					return (! * pIrqServiced);
				}
			}
			else
			{
				if (msgQSend (pPipeDesc->msgQId,
				              (char *) (pPipeDesc->pLocalBuffer + OFFSET_TO_FIRST_BYTE),
				              (UINT) nBytes, NO_WAIT, MSG_PRI_NORMAL)
				    == ERROR)
				{
					ERROR_SET (0, "Failed to send message to message queue", ERROR_LOG_SAVE);
					return (! * pIrqServiced);
				}
			}

#ifndef	NO_INSTALL
			selWakeupAll (& pPipeDesc->selWakeupList, SELREAD);
														/* Only call selectLib if driver installed */
#endif /* NO_INSTALL */

			/*
			 * Shift count of number of bytes ready to ready in message queue down by one
			 * place and add latest byte count to bottom of queue
			 */

			for (i = pPipeDesc->maxNMsg - 1; i > 0; i--)
			{
				pPipeDesc->pNByteReady [i] = pPipeDesc->pNByteReady [i - 1];
			}
			pPipeDesc->pNByteReady [0] = (int) nBytes;
			pPipeDesc->nMsgTransactions++;						/* Increment transaction count */

			/*
			 * If the pipe mode is normal (buffered), then clear the Done and Busy flags
			 * to allow the local buffer to be re-written by a remote processor now that the
			 * data in it has been copied to the message queue. If the pipe mode is unbuffered,
			 * then these flags get cleared in mpPipeRead() as soon as the message queue is read.
			 */

			if (! pPipeDesc->unbuffered)
			{
				* pPipeDesc->pLocalDoneFlag = 0;
				* pPipeDesc->pLocalBuffer = 0;
				CACHE_DMA_FLUSH ((char *) pPipeDesc->pLocalBuffer,
							 (int) (pPipeDesc->pLocalDoneFlag + 1) - (int) pPipeDesc->pLocalBuffer);
			}
		}

		/*
		 * Now test for a write pending on the Queue flag. If found send Mbox IRQ to source
		 * processor (the one waiting to write to this one) so that it can do its write. The
		 * source processor will issue a Mbox IRQ to this processor when it does the write,
		 * thus causing this proc to re-call this read routine in order to copy the newly-arrived
		 * data into the msgQ (if there is space available).
		 */

		if (* pPipeDesc->pLocalBuffer == 0 && * pPipeDesc->pQueueFlag != 0)
		{
			if (sysextMboxIntGen (pPipeDesc->procIdWrite, MP_PIPE_MAILBOX) == ERROR)
			{
				ERROR_SET (0, "Failed to send mailbox interrupt", ERROR_LOG_SAVE);
				return (! * pIrqServiced);
			}
		}
	}

	/*
	 *  If IRQ serviced, return FALSE to discontinue search through symbol table. If
	 *	not serviced, return FALSE to continue search
	 */

	return (! * pIrqServiced);
}


/*+
 *	IGNORED FUNCTION NAME:
 *	mp_LocalWrite
 *
 *	INVOCATION:
 *	mp_LocalWrite (pName, symValue, symType, argument, group)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName		(char *)	name of pipe
 *	(>)	symValue	(int)		pipe descriptor (pointer, cast as int)
 *	(>)	symType		(SYM_TYPE)	symbol type associated with pipe descriptor
 *	(>)	argument	(int)		arbitrary argument passed from symEach()
 *	(>)	group		(UINT16)	symbol group type associated with pipe desc
 *
 *	FUNCTION VALUE:
 *	(BOOL)	FALSE if the pipe could be written, or TRUE if it could not.
 *
 *	PURPOSE:
 *	Attempt to write from local pipe message queue to remote processor's
 *	VME buffer (via VME bus)
 *
 *	DESCRIPTION:
 *	This routine examines a local pipe and determines whether it is ready
 *	to write data, and, if it is, whether the destination processor is
 *	ready to accept the data. If data can be written, then it is written
 *	over VME bus into the destination processor's local VME buffer memory.
 *	If data is available locally but the destination is not ready to read it
 *	then the data is queued; in this latter case a Queue flag on the remote
 *	processor will be set unless it has already been set.
 *
 *  This routine is invoked either by the local processor or in response to
 *  a mailbox interrupt from a remote processor
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	The pipe must have been opened for writing by this processor and
 *	for reading by any processor on the bus.
 *
 *	DEFICIENCIES:
 *	None known
 *
 *	BUGS:
 *	Arguments "pName" and "group" are not referenced by this function.
 *	Are they present for future exhancement, to allow debugging, or
 *	simply for compatability with the other "local" functions, or is
 *	this a bug? SMB - 30 Jan 98.
 *-
 */

BOOL mp_LocalWrite
	(
	char *		pName,
	int			symValue,
	SYM_TYPE	symType,
	int			argument,
	UINT16		group
	)
{
	MP_PIPE_DESC *	pPipeDesc;
	BOOL *			pIrqServiced;
	int				nBytes;

	/* Skip over non-primary symbol table entries */
	if (symType == MP_PIPE_SYM_TYPE_ALT) return (TRUE);

	pPipeDesc = (MP_PIPE_DESC *) symValue;
	pIrqServiced = (BOOL *) argument;			/* Success/failure of write is returned in *argument */

	/* Test for pipe opened for writing by this proc, and reading by any processor */

	if (pPipeDesc->procIdWrite == sysextProcNumGet () && pPipeDesc->procIdRead >= 0)
	{
		if (msgQNumMsgs (pPipeDesc->msgQId) > 0)				/* Test for something to write		*/
																/* waiting in the message queue.	*/
		{
			if (sysBusTas ((char *) pPipeDesc->pRemoteWriteBuffer))
																/* Grab remote Busy flag.			*/
			{
				* pIrqServiced = TRUE;							/* Have detected the reason for IRQ */

				/*
				 * If the pipe mode is unbuffered, the data to write should already have been written
				 * directly into the local buffer starting at the offset to the first data byte. In
				 * this case, the message queue is only used to pass the number of bytes to write.
				 * For normal (buffered) pipes, copy the data to write from the message queue to the
				 * local buffer
				 */

				if (pPipeDesc->unbuffered)
				{
					if (msgQReceive (pPipeDesc->msgQId, (char *) & nBytes, (UINT) 4, NO_WAIT)
						== ERROR)
						nBytes = ERROR;
				}
				else
				{
					nBytes = msgQReceive (pPipeDesc->msgQId, (char *) (pPipeDesc->pLocalBuffer +
					                      OFFSET_TO_FIRST_BYTE),
					                      (UINT) pPipeDesc->maxNBytePerMsg, NO_WAIT);
				}

#ifndef	NO_INSTALL
				/* Only call selectLib if driver is installed. */
				selWakeupAll (& pPipeDesc->selWakeupList, SELWRITE);
#endif /* NO_INSTALL */

				if ((nBytes == ERROR) || (nBytes > pPipeDesc->maxNBytePerMsg))
				{
					ERROR_SET (0, "Invalid read from message queue", ERROR_LOG_SAVE);
					return (! * pIrqServiced);
				}

				/*
				 * Prepare local write buffer ready to write msg over bus. The format of the buffer
				 * is as follows:
				 *
				 *		[pLocalBuffer + 0]:				Busy flag (4 bytes)
				 *		[pLocalBuffer + 4]:				Queue flag (4 bytes)
				 *		[pLocalBuffer + 8]:				Number of bytes in following message (4 bytes)
				 *		[pLocalBuffer + 12]:			Data byte #1 (1 byte)
				 *		[pLocalBuffer + 13]:			Data byte #2 (1 byte)
				 *		[pLocalBuffer + ..]:			..
				 *		[pLocalBuffer + ..]:			..
				 *		[pLocalBuffer + 12 + N - 1]:	Data byte #N (1 byte) (N always divisible by 4)
				 *		[pLocalBuffer + 12 + N]:		Done flag (4 bytes)
				 *
				 * Set number of bytes field and Done flag in buffer. Also, test for one or more
				 * writes previously queued by this processor, but now no longer in the queue
				 * (local msgQ empty). If one or more writes were queued, then clear the Queue
				 * flag in the remote destination buffer and clear the local copy of this flag.
				 */

				* (pPipeDesc->pLocalBuffer + OFFSET_TO_NBYTES) = (uint32) nBytes;
				* (pPipeDesc->pLocalBuffer + OFFSET_TO_FIRST_BYTE + pPipeDesc->maxNBytePerMsg / 4) = 1;

				if (pPipeDesc->localQueueFlag == TRUE && msgQNumMsgs (pPipeDesc->msgQId) < 1)
				{
					pPipeDesc->localQueueFlag = FALSE;
					* (pPipeDesc->pLocalBuffer + OFFSET_TO_QUEUE_FLAG) = 0;
				}

				CACHE_DMA_FLUSH ((char *) pPipeDesc->pLocalBuffer,
								 (int) (pPipeDesc->pLocalDoneFlag + 1) -
				                    (int) pPipeDesc->pLocalBuffer);

				/*
				 * If nByte is within 16 bytes of the maximum allowed (which is always a multiple
				 * of 8), then pad the data up to fill the maximum allowed. This ensures that the
				 * data + number-of-bytes word + doneFlag word can all be transferred with a single
				 * D64 block transfer.
				 */

				if ((pPipeDesc->maxNBytePerMsg - nBytes) < 16)
				{
					nBytes = pPipeDesc->maxNBytePerMsg;
				}
				if (nBytes == pPipeDesc->maxNBytePerMsg)
				{
					/*
					 * Write nBytes, data bytes and doneFlag all in one go (Don't bother to re-write
					 * Busy flag and Queue flag). In the event that the pipe was created with a
					 * maximum message length of zero bytes (e.g. if the pipe is used only for inter-
					 * processor synchronisation rather than to transfer data), then the following
					 * block copy will just transfer two 32-bit words: the first is the count of the
					 * number of bytes to follow (zero in this case) and the second is the done flag.
					 */

					if (sysextVmeBlockCopy (
					   		(const char *) CACHE_DMA_VIRT_TO_PHYS (pPipeDesc->pLocalBuffer
							+ OFFSET_TO_NBYTES),
							(char *) (pPipeDesc->pRemoteWriteBuffer + OFFSET_TO_NBYTES),
							nBytes + (OFFSET_TO_FIRST_BYTE - OFFSET_TO_NBYTES + 1) * 4, TRUE,
							MP_PIPE_VME_BLT_MODE) == ERROR)
					{
						ERROR_SET (0, "Error doing block copy", ERROR_LOG_SAVE);
#ifdef DEBUG
						printf (
							"mp_LocalWrite: Error doing block copy. src = 0x%x dst = 0x%x n = 0x%x\n",
							(int) (pPipeDesc->pLocalBuffer + OFFSET_TO_NBYTES),
							(int) (pPipeDesc->pRemoteWriteBuffer + OFFSET_TO_NBYTES),
							nBytes + (OFFSET_TO_FIRST_BYTE - OFFSET_TO_NBYTES + 1) * 4);
#endif /* DEBUG */
						return (! * pIrqServiced);
					}
					pPipeDesc->nMsgTransactions++;
				}
				else
				{

					/*
					 * If nBytes > 8 but not within 16 bytes of the maximum allowed, pad such that
					 * the number-of-bytes word + data can all be sent with a single D64 BLT. The
					 * doneFlag word will then be sent separately with a single D32 word write. If
					 * nByte is very small (<= 8 bytes seems like a reasonable threshold value),
					 * then just get on with block copy over VME bus. The DMA driver routines in
					 * sysextLib won't bother with DMA (VME block transfers) in this case since it's
					 * not worth the overhead of setting up the transfer.
					 */

					if (nBytes > 8)
					{
						nBytes += (nBytes + 4) % 8;
					}

					/*
					 * Write nBytes and data bytes, then write doneFlag separately. This
					 * approach avoids having to pad each write over the bus with unused data bytes in
					 * the case that the number of bytes to written does not fill entire message Q
					 */

					if (sysextVmeBlockCopy (
							(const char *) CACHE_DMA_VIRT_TO_PHYS (pPipeDesc->pLocalBuffer
							+ OFFSET_TO_NBYTES),
							(char *) (pPipeDesc->pRemoteWriteBuffer + OFFSET_TO_NBYTES),
							nBytes + (OFFSET_TO_FIRST_BYTE - OFFSET_TO_NBYTES) * 4, TRUE,
							MP_PIPE_VME_BLT_MODE) == ERROR)
					{
						ERROR_SET (0, "Error doing block copy", ERROR_LOG_SAVE);
						return (! * pIrqServiced);
					}

					* (pPipeDesc->pRemoteDoneFlag) = 1;
					CACHE_DMA_FLUSH ((char *) pPipeDesc->pRemoteDoneFlag, 4);
					pPipeDesc->nMsgTransactions++;
				}

				/*
				 * If the buffer ready semaphore ID is non-null, then give the semaphore to
				 * signal the availability of the local buffer. The main reason for providing
				 * this semaphore is to allow tasks which uses unbuffered pipes to determine
				 * when a submitted write request (call to mpPipeWrite(), via write()) has
				 * been serviced.
				 */

				if (pPipeDesc->bufferReadySem != NULL) semGive (pPipeDesc->bufferReadySem);

				/*
				 * Issue Mbox IRQ to destination processor to enable it to read the data we've
				 * just written into its local buffer
				 */

				if (sysextMboxIntGen (pPipeDesc->procIdRead, MP_PIPE_MAILBOX) == ERROR)
				{
					ERROR_SET (0, "Failed to send mailbox interrupt", ERROR_LOG_SAVE);
					return (! * pIrqServiced);
				}
			}
			else
			{

				/*
				 * Unable to write to destination (Busy flag set). Test whether the Queue flag
				 * has been set by testing the local copy of it. If set, do nothing, otherwise
				 * set the Queue flag over the bus and set the local copy. Note that the local
				 * copy of the Queue flag avoids the remote Queue flag having to be tested over
				 * the bus every time we want to test whether writes have been queued.
				 */

				if (pPipeDesc->localQueueFlag == FALSE)
				{
					* pIrqServiced = TRUE;						/* Have detected the reason for IRQ */
					* pPipeDesc->pQueueFlag = 1;
					CACHE_DMA_FLUSH ((char *) pPipeDesc->pQueueFlag, 4);
					pPipeDesc->localQueueFlag = TRUE;
				}
			}
		}
	}

	/*
	 *  If IRQ serviced, return FALSE to discontinue search through symbol table. If
	 *	not serviced, return FALSE to continue search
	 */

	return (! * pIrqServiced);
}

/*+
 *	IGNORED FUNCTION NAME:
 *	mp_LocalCommand
 *
 *	INVOCATION:
 *	mp_LocalCommand (pIrqServiced, pErrorNumber)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(<)	pIrqServiced	(BOOL *)	contents set TRUE if command was executed
 *	(<)	pErrorNumber	(uint32 *)	error number, 0 if no error
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if the command was executed, or ERROR if it was not.
 *
 *	PURPOSE:
 *	Attempt to execute incoming command from a remote processor
 *
 *	DESCRIPTION:
 *	This routine examines the local command flag to determine whether
 *	a remote processor is currently attempting to issue a command to it.
 *	If the command flag is set, the MCS (on processor #0) is examined to
 *	determine the type of command and any assoicated parameters. The command
 *	is then execute locally and the success or failure of it is reported
 *	back to the remote processor via the MCS.
 *
 *	EXTERNAL VARIABLES:
 *	(>!)	pCmdFlag	(volatile char *)	Local command flag, may be set by other CPUs
 *
 *	PRIOR REQUIREMENTS:
 *	The pipe driver must have been initialised.
 *
 *	DEFICIENCIES:
 *	None known
 *
 *	BUGS:
 *	The return value from this function is never actually tested in any
 *	of the mpPipeDrv code. The pErrorNumber variable is tested instead, yet
 *	I found this function didn't always return that variable. I have modified
 *	the function so pErrorNumber is always returned. I find the discrepancy
 *	suspicious. SMB - 30 Jan 98.
 *-
 */

STATUS mp_LocalCommand
	(
	BOOL *		pIrqServiced,
	uint32 *	pErrorNumber
	)
{
	STATUS	returnValue;
	char *	pName;

	/*
	 * Initialise the error number returned by this function.
	 */

	* pErrorNumber = 0;

	/*
	 * First test local Cmd flag. This will be set if a Mbox IRQ has been issued by a remote
	 * processor which is issuing a command to this processor
	 */

	CACHE_DMA_INVALIDATE (pCmdFlag, 1);
	if (* pCmdFlag == 0) return (OK);

	/*
	 * Only get here if the local Command flag is set, indicating that a remote processor has
	 * issued a command to this one. Only then will we go to the bother of examining the MCS.
	 * If the Busy flag is set, and this processor has the same ID as the destination ID in
	 * the MCS, then a command is being issued to us: switch on it and go deal with it.
	 */

	CACHE_DMA_INVALIDATE ((char *) & pMasterControlMsg->doneFlag, sizeof (MP_PIPE_CONTROL_MSG));

	if (pMasterControlMsg->busyFlag != 0 && pMasterControlMsg->procIdDest == sysextProcNumGet ())
	{
		* pIrqServiced = TRUE;						/* Reason for IRQ determined. IRQ serviced. */
		pName = (char *) pMasterControlMsg->pName;

		switch (pMasterControlMsg->function)
		{
			case MP_PIPE_FUNC_CREATE:
				returnValue = mp_LocalCreate (pName, (int) pMasterControlMsg->maxNMsg,
					(int) pMasterControlMsg->maxNBytePerMsg, pErrorNumber);
				break;

			case MP_PIPE_FUNC_DELETE:
				returnValue = mp_LocalDelete (pName, pErrorNumber);
				break;

			case MP_PIPE_FUNC_INVALIDATE:
				returnValue = mp_LocalInvalidate (pName, pErrorNumber);
				break;

			case MP_PIPE_FUNC_OPEN_WR:
				returnValue = mp_localopen (pName, O_WRONLY, pMasterControlMsg->procIdSource,
				                         pErrorNumber);
				break;

			case MP_PIPE_FUNC_OPEN_RD:
				returnValue = mp_localopen (pName, O_RDONLY, pMasterControlMsg->procIdSource,
				                         pErrorNumber);
				break;

			case MP_PIPE_FUNC_CLOSE_WR:
				returnValue = mp_localclose (pName, O_WRONLY, pMasterControlMsg->procIdSource,
                                          pErrorNumber);
				break;

			case MP_PIPE_FUNC_CLOSE_RD:
				returnValue = mp_localclose (pName, O_RDONLY, pMasterControlMsg->procIdSource,
                                          pErrorNumber);
				break;

			case MP_PIPE_FUNC_FLUSH:
				returnValue = mp_localflush (pName, pErrorNumber);
				break;

			default:
				pMasterControlMsg->errorNumber = 1;	/* Error, invalid function rxd from Master */
				printf ("mp_LocalCommand: ERROR - invalid command interrupt\n");
				* pErrorNumber = 1;
				returnValue = ERROR;
		}

		if (returnValue == ERROR)
		{
			pMasterControlMsg->errorNumber = 1;					/* Error, local command failed.	*/
		}
		* pCmdFlag = 0;											/* Clear local command flag.	*/
		CACHE_DMA_FLUSH (pCmdFlag, 1);
		pMasterControlMsg->doneFlag = 1;
		CACHE_DMA_FLUSH ((char *) & pMasterControlMsg->doneFlag, sizeof (MP_PIPE_CONTROL_MSG));
	}

	return (returnValue);
}

/*+
 *	IGNORED FUNCTION NAME:
 *	mp_LocalCreate
 *
 *	INVOCATION:
 *	mp_LocalCreate (pName, maxNMsg, maxNBytePerMsg, pErrorNumber)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName			(char *)	name of pipe
 *	(>)	maxNMsg			(int)		number of slots in local message queue
 *	(>)	maxNBytePerMsg	(int)		maximum number of bytes per message
 *	(<)	pErrorNumber	(uint32 *)	error number, 0 if no error
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if the pipe was created, or ERROR if the creation failed.
 *
 *	PURPOSE:
 *	Create local pipe device
 *
 *	DESCRIPTION:
 *	This routine creates a local pipe device in response to a command
 *	issued either locally or by a remote processor.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	The pipe driver must have been initialised.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS mp_LocalCreate
	(
	char *		pName,
	int			maxNMsg,
	int			maxNBytePerMsg,
	uint32 *	pErrorNumber
	)
{
	SYM_TYPE		symType;
	MP_PIPE_DESC *	pPipeDesc;
	MP_PIPE_DESC *	pOtherPipeDef;
	FAST int		i;
	DEV_HDR *		pOldDevHdr;
	char *			pNameTail;

	* pErrorNumber = 0;

	/*
	 * Create pipe device. Note that each pipe always has associated with it a symbol of
 	 * type MP_PIPE_SYM_TYPE_NORMAL and this corresponds to the "primary" device descriptor.
	 * A second descriptor is also pointed to by element pOtherPipeDef within the primary
	 * descriptor and this "secondary" descriptor is used to differentate between the mode
	 * (O_RDONLY or O_WRONLY) in which the given pipe has been opened. The secondary
	 * descriptor has a symbol type MP_PIPE_SYM_TYPE_ALT. The first time a pipe
	 * is opened, the primary descriptor is used and element mp_localopenMode is set to the
	 * requested mode (O_RDONLY or O_WRONLY). If the pipe is subsequently opened in the
	 * other mode BY THE SAME PROCESSOR, then the element mp_localopenMode in the secondary
	 * descriptor is set to this mode and the pointer to this secondary structure is returned
	 * as the device descriptor. This approach allows the driver to allocate different fd's
	 * depending on the open mode for a pipe.
	 *
	 * NB. Params are always valid on entry to this function. Only need to check that the pipe
	 * doesn't already exist in the symbol table, then get on with creating new pipe if it doesn't.
	 */

	if (symFindByNameAndType (mpPipeSymTab, pName, (char **) & pPipeDesc, & symType,
							  MP_PIPE_SYM_TYPE_NORMAL, MP_PIPE_SYM_TYPE_MASK) == OK)
	{
		* pErrorNumber = 1;
	}
	else if ((pPipeDesc = (MP_PIPE_DESC *) malloc (sizeof (MP_PIPE_DESC))) == NULL)
	{
		* pErrorNumber = 1;
	}
	else if ((pOtherPipeDef = (MP_PIPE_DESC *) malloc (sizeof (MP_PIPE_DESC))) == NULL)
	{
		free (pPipeDesc);
		* pErrorNumber = 1;
	}
	else if ((pPipeDesc->pNByteReady = (int *) malloc (maxNMsg * sizeof (int))) == NULL)
	{
		free (pPipeDesc);
		free (pOtherPipeDef);
		* pErrorNumber = 1;
	}
	else if (symAdd (mpPipeSymTab, pName, (char *) pPipeDesc, MP_PIPE_SYM_TYPE_NORMAL,
					 MP_PIPE_SYMBOL_GROUP) == ERROR)
	{
		free (pPipeDesc->pNByteReady);
		free (pPipeDesc);
		free (pOtherPipeDef);
		* pErrorNumber = 1;
	}

	if (* pErrorNumber != 0)
	{
		ERROR_SET (0, "Failed to create pipe device", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* Initialise primary pipe descriptor structure */

	if (maxNMsg == 0)									/* Pipe is unbuffered if maxNMsg = 0 */
	{
		maxNMsg = 1;
		pPipeDesc->unbuffered = TRUE;
	}
	else
	{
		pPipeDesc->unbuffered = FALSE;
	}

	pPipeDesc->pOtherPipeDef = (void *) pOtherPipeDef;
	pPipeDesc->localOpenMode = -1;
	pPipeDesc->localQueueFlag = FALSE;
	pPipeDesc->pQueueFlag = NULL;
	pPipeDesc->pRemoteWriteBuffer = NULL;
	pPipeDesc->pLocalBufferAlloc = NULL;
	pPipeDesc->pLocalBuffer = NULL;
	pPipeDesc->msgQId = NULL;
	pPipeDesc->pRemoteDoneFlag = NULL;
	pPipeDesc->procIdWrite = -1;
	pPipeDesc->procIdRead = -1;
	pPipeDesc->maxNMsg = maxNMsg;
	pPipeDesc->maxNBytePerMsg = maxNBytePerMsg;
	pPipeDesc->pipeValid = TRUE;
	pPipeDesc->nMsgTransactions = 0;
	pPipeDesc->bufferReadySem = NULL;
	pPipeDesc->mutexSemWrite = NULL;
	pPipeDesc->mutexSemRead = NULL;
	pPipeDesc->openCountWrite = 0;
	pPipeDesc->openCountRead = 0;

	/*
	 * Initialise list containing number of bytes ready to read in message queue
	 * (-1 means no bytes ready).
	 */

	for (i = 0; i < maxNMsg; i++)
		pPipeDesc->pNByteReady [i] = -1;

	/* Initialise secondary pipe descriptor structure */

	pOtherPipeDef->pOtherPipeDef = (void *) pPipeDesc;
	pOtherPipeDef->localOpenMode = -1;
	if (symAdd (mpPipeSymTab, pName, (char *) pOtherPipeDef, MP_PIPE_SYM_TYPE_ALT,
				MP_PIPE_SYMBOL_GROUP) == ERROR)
	{
		free (pPipeDesc->pNByteReady);
		free (pPipeDesc);
		free (pOtherPipeDef);
		symRemove (mpPipeSymTab, pName, MP_PIPE_SYM_TYPE_NORMAL);
		* pErrorNumber = 1;
		ERROR_SET (0, "Failed to add symbol for secondary pipe descriptor structure", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/*
	 * If the pipe driver is installed, then add this pipe to the device list and initialise the
	 * select library's wakeup list
	 */

#ifndef	NO_INSTALL

	/*
	 * Before adding the device to the IO system, search to see if it already exists. This
	 * may occur if the device driver is re-loaded into the system since the pipe symbol
	 * table will then be re-initialised (with no existing pipes), however any pre-existing
	 * pipes will still exist in the IO system. Any attempt to re-create any such defunct
	 * pipes will fail in iosDevAdd unless they are first deleted. Search for given pipe name
	 * in IO system and delete if found, then add device to IO system with the current device
	 * header.
	 */

	pOldDevHdr = iosDevFind (pName, & pNameTail);			/* Lookup pipe with given name. */
															/* pNameTail will be returned as */
															/* a null string if the pipe already */
															/* exists in the IO system */

	if (strlen (pNameTail) == 0)
	{
		iosDevDelete (pOldDevHdr);							/* Delete if not the default device */
	}
	if (iosDevAdd (& pPipeDesc->deviceHeader, pName, mpPipeDrvNumber) == ERROR)
	{
		free (pPipeDesc->pNByteReady);
		free (pPipeDesc);
		free (pOtherPipeDef);
		symRemove (mpPipeSymTab, pName, MP_PIPE_SYM_TYPE_NORMAL);
		symRemove (mpPipeSymTab, pName, MP_PIPE_SYM_TYPE_ALT);
		* pErrorNumber = 1;
		ERROR_SET (0, "Failed to add pipe device to IO system", ERROR_LOG_SAVE);
		return (ERROR);
	}
	selWakeupListInit (& pPipeDesc->selWakeupList);
#endif	/* NO_INSTALL */

	return (OK);
}

/*+
 *	IGNORED FUNCTION NAME:
 *	mp_LocalDelete
 *
 *	INVOCATION:
 *	mp_LocalDelete (pName, pErrorNumber)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName			(char *)	name of pipe
 *	(<)	pErrorNumber	(uint32 *)	error number, 0 if no error
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if the pipe was deleted, or ERROR if the deletion failed.
 *
 *	PURPOSE:
 *	Delete local pipe device
 *
 *	DESCRIPTION:
 *	This routine deletes a local pipe device in response to a command
 *	issued either locally or by a remote processor.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	The pipe driver must have been initialised and the pipe must be closed
 *	for both reading and writing by all processor on the bus.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS mp_LocalDelete
	(
	char *		pName,
	uint32 *	pErrorNumber
	)
{
	SYM_TYPE		symType;
	MP_PIPE_DESC *	pPipeDesc;

	/* NB. The pipe name is always valid on entry. Get pipe descriptor from the symbol table */

	if (symFindByNameAndType (mpPipeSymTab, pName, (char **) & pPipeDesc, & symType,
							  MP_PIPE_SYM_TYPE_NORMAL, MP_PIPE_SYM_TYPE_MASK) == ERROR)
	{
		* pErrorNumber = 1;
		ERROR_SET (0, "Could not find pipe descriptor in symbol table", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/* Only allow deletion if the pipe is either un-opened or has previously been marked as invalid */

	if ((pPipeDesc->procIdRead >= 0 || pPipeDesc->procIdWrite >= 0) && pPipeDesc->pipeValid)
	{
		* pErrorNumber = 1;
		ERROR_SET (0, "Cannot delete an open and valid pipe", ERROR_LOG_SAVE);
		return (ERROR);
	}

#ifndef	NO_INSTALL
	iosDevDelete (& pPipeDesc->deviceHeader);		/* Remove pipe from IO system's device list.. */
#endif /* NO_INSTALL */

	free (pPipeDesc->pNByteReady);
	free (pPipeDesc->pOtherPipeDef);
	free (pPipeDesc);
	if (symRemove (mpPipeSymTab, pName, MP_PIPE_SYM_TYPE_NORMAL) == ERROR ||
		symRemove (mpPipeSymTab, pName, MP_PIPE_SYM_TYPE_ALT) == ERROR)
	{
		* pErrorNumber = 1;
		ERROR_SET (0, "Failed to remove pipe descriptor from symbol table", ERROR_LOG_SAVE);
		return (ERROR);
	}

	return (OK);
}

/*+
 *	IGNORED FUNCTION NAME:
 *	mp_localopen
 *
 *	INVOCATION:
 *	mp_localopen (pName, flags, procId, pErrorNumber)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName			(char *)	name of pipe
 *	(>) flags			(int)		read/write mode, O_RDONLY or O_WRONLY
 *	(>)	procId			(int)		ID of processor opening pipe
 *	(<)	pErrorNumber	(uint32 *)	error number, 0 if no error
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if the pipe was opened, or ERROR if the open failed.
 *
 *	PURPOSE:
 *	Open local pipe device
 *
 *	DESCRIPTION:
 *	This routine opens a local pipe device in response to a command
 *	issued either locally or by a remote processor. If the pipe is
 *	being opened by this processor (rather than a remote processor on
 *	the bus), then a message queue and local buffer used by the pipe
 *	are allocated.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	The pipe driver must have been initialised and the pipe must have
 *	been created (by any processor on the bus). The pipe must not be
 *	currently opened in the requested mode by any other processor on
 *	the bus.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS mp_localopen
	(
	char *		pName,
	int			flags,
	int			procId,
	uint32 *	pErrorNumber
	)
{
	SYM_TYPE		symType;
	MP_PIPE_DESC *	pPipeDesc;
	FAST int		n;
	BOOL			firstTimeOpen;
	int				nBytePerMsg;
	int				remainder;
	int				thisProcId;
	char *			ptr;

	/* NB. Pipe params are always valid on entry. Get pipe descriptor from symbol table */

	if (symFindByNameAndType (mpPipeSymTab, pName, (char **) & pPipeDesc, & symType,
							  MP_PIPE_SYM_TYPE_NORMAL, MP_PIPE_SYM_TYPE_MASK) == ERROR)
	{
		* pErrorNumber = 1;
		ERROR_SET (0, "Could not find pipe descriptor in symbol table", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/*
	 * firstTimeOpen is TRUE unless this is the 2nd or subsequent time that the pipe is
 	 * locally opened, in which case it is set FALSE and there is no need to go through all
	 * steps necessary to open a pipe; all that is done in this case is to check that the open
	 * command can be performed and increment a count of the number of times the pipe has been
	 * opened
 	 */

	firstTimeOpen = TRUE;									/* Assume 1st-time open */
	thisProcId = sysextProcNumGet ();
	switch (flags)
	{
	case O_RDONLY:											/* Open pipe for reading */
		if (procId != thisProcId)
		{
			/*
			 * The pipe is being opened to read by some processor other than this one. This
			 * can only occur when the pipe open command is broadcast to all processors on the
			 * bus immediately after it has been opened by the opening processor.
			 * Get the pointer to the opening pipe's VME buffer from the MCS and convert to a local
			 * address for potential future use by this processor should we open the pipe for
			 * writing, in which case we'll need to know the destination address for data. Also
			 * set up pointers to the Queue and Done flags on whichever processor is opening the pipe.
			 */

			CACHE_DMA_INVALIDATE ((char *) & pMasterControlMsg->doneFlag,
			                      sizeof (MP_PIPE_CONTROL_MSG));
			if (sysextBusToLocalAdrs (VME_AM_EXT_SUP_DATA,
			                          (char *) pMasterControlMsg->pRemoteWriteBuffer,
			                          (char **) & pPipeDesc->pRemoteWriteBuffer)
			    == ERROR)
			{
				ERROR_SET (0, "Failed to convert VME buffer pointer to local address",
					ERROR_LOG_SAVE);
				return (ERROR);
			}
			pPipeDesc->pQueueFlag = pPipeDesc->pRemoteWriteBuffer + OFFSET_TO_QUEUE_FLAG;
			pPipeDesc->pRemoteDoneFlag = pPipeDesc->pRemoteWriteBuffer +
			                             pPipeDesc->maxNBytePerMsg / 4 +
			                             OFFSET_TO_FIRST_BYTE;

			/*
			 * If This is an Open to READ command by a remote processor, and this processor
			 * has already opened the pipe to WRITE, then give the ISR semaphore once for
			 * every message which (may potentially) have been queued in the local message Q
			 * by this processor (whilst awaiting definition of the READ destination).
			 */

			if (pPipeDesc->procIdWrite == thisProcId)
			{
				for (n = 0; n < msgQNumMsgs (pPipeDesc->msgQId); n++)
				{
					semGive (mpPipeMboxIrq);
				}
			}
		}
		else
		{
			/*
			 * The pipe is being opened to read by this processor. Check that, if the
			 * pipe mode is Unbuffered, it hasn't already been opened to write by this same
			 * processor. Unbuffered pipes can ONLY be used between two different processors
			 */

			if (pPipeDesc->procIdWrite == thisProcId && pPipeDesc->unbuffered)
			{
				ERROR_SET (S_mpPipeDrv_INVALID_PIPE_MODE, "Cannot open pipe in this mode",
				           ERROR_LOG_SAVE);
				return (ERROR);
			}

			/*
			 * Increment the count of the number of times the pipe has been opened to
			 * read by this processor. If this is the first open, then create a mutex a
			 * mutex semaphore used to ensure that only one local task can read
			 * a pipe at a time. Once a task starts to read a pipe, the semaphore is held
			 * until the read has completed, thus avoiding pre-emptive messing-up of a read.
			 */

			if (++pPipeDesc->openCountRead == 1)
			{
				if ((pPipeDesc->mutexSemRead = semMCreate (SEM_Q_FIFO | SEM_DELETE_SAFE)) == NULL)
				{
					ERROR_SET (0, "Failed to create mutex semaphore", ERROR_LOG_SAVE);
					return (ERROR);
				}
			}
			else
			{
				firstTimeOpen = FALSE;					/* This is a 2nd or subsequent local open */
			}
		}
		break;

	case O_WRONLY:										/* Open pipe for writing */
		if (procId == thisProcId)
		{
			/*
			 * Check that, if the pipe is being opened to write by this processor, and the
			 * pipe mode is Unbuffered, it hasn't already been opened to read by this same
			 * processor. Unbuffered pipes can ONLY be used between two different processors
			 */

			if (pPipeDesc->procIdRead == thisProcId && pPipeDesc->unbuffered)
			{
				ERROR_SET (S_mpPipeDrv_INVALID_PIPE_MODE, "Cannot open pipe in this mode",
				           ERROR_LOG_SAVE);
				return (ERROR);
			}

			/*
			 * Increment the count of the number of times the pipe has been opened to
			 * write by this processor. If this is the first open, then create a mutex a
			 * mutex semaphore used to ensure that only one local task can write
			 * a pipe at a time. Once a task starts to write a pipe, the semaphore is held
			 * write the read has completed, thus avoiding pre-emptive messing-up of a write.
			 */

			if (++pPipeDesc->openCountWrite == 1)
			{
				if ((pPipeDesc->mutexSemWrite = semMCreate (SEM_Q_FIFO | SEM_DELETE_SAFE)) == NULL)
				{
					ERROR_SET (0, "Failed to create mutex semaphore", ERROR_LOG_SAVE);
					return (ERROR);
				}
			}
			else
			{
				firstTimeOpen = FALSE;				/* This is a 2nd or subsequent local open */
			}
		}
		break;

	default:										/* Unknown read/write mode */
		ERROR_SET (S_mpPipeDrv_INVALID_PIPE_MODE, "Unknown read/write mode", ERROR_LOG_SAVE);
		return (ERROR);
		break;
	}

	/*
	 * Have now done almost everything necessary to open the processor to read or write.
	 * Now need to allocate memory for the local buffer and create the message queue
	 * if this processor is the one issuing the open command, if the pipe is being
	 * opened by some other processor, then it will allocate memory locally.
	 */

	if (procId == thisProcId && firstTimeOpen)
	{
		/*
		 * If the pipe mode is Unbuffered, then the message queue is used only to pass the
		 * number of bytes in each message and therefore need only be 4 bytes (int byte-
		 * count) in length. For buffered pipes, the message queue must be long enough to
		 * hold an entire message of up to pPipeDesc->maxNBytePerMsg bytes.
		 */

		if (pPipeDesc->unbuffered)
			nBytePerMsg = 4;
		else
			nBytePerMsg = pPipeDesc->maxNBytePerMsg;

		if ((pPipeDesc->msgQId = msgQCreate (pPipeDesc->maxNMsg, nBytePerMsg, MSG_Q_FIFO)) == NULL)
		{
			ERROR_SET (0, "Failed to create message queue", ERROR_LOG_SAVE);
			return (ERROR);
		}

		if ((pPipeDesc->pLocalBuffer = (volatile uint32 *) cacheDmaMalloc ((OFFSET_TO_FIRST_BYTE + 1)
			* 4 + pPipeDesc->maxNBytePerMsg + MP_PIPE_BLT_ALIGN)) == NULL)
		{
			msgQDelete (pPipeDesc->msgQId);
			ERROR_SET (0, "Cache-safe memory allocation failed", ERROR_LOG_SAVE);
			return (ERROR);
		}

		/*
		 * Fix pPipeDesc->pLocalBuffer such that the first word written over VME bus (at
		 * pPipeDesc->pLocalBuffer[OFFSET_TO_NBYTES], in which the word length is 4 bytes)
		 * is aligned on an MP_PIPE_BLT_ALIGN address boundary. The value previously
		 * allocated to pPipeDesc->pLocalBuffer is saved in pPipeDesc->pLocalBufferAlloc
		 * for future use by cacheDmaFree() when freeing this region of memory
		 */

		pPipeDesc->pLocalBufferAlloc = (char *) pPipeDesc->pLocalBuffer;

		ptr = (char *) pPipeDesc->pLocalBuffer;

		remainder = (int) ptr % MP_PIPE_BLT_ALIGN;
		if (remainder < 0) remainder += MP_PIPE_BLT_ALIGN;

		ptr += MP_PIPE_BLT_ALIGN - OFFSET_TO_NBYTES * 4 - remainder;
		if (ptr < pPipeDesc->pLocalBufferAlloc) ptr += MP_PIPE_BLT_ALIGN;

		pPipeDesc->pLocalBuffer = (uint32 *) ptr;

		pPipeDesc->pLocalDoneFlag = pPipeDesc->pLocalBuffer + OFFSET_TO_FIRST_BYTE +
			pPipeDesc->maxNBytePerMsg / 4;
	}

	/* Finally, set the opening proc ID and initialise the number of message transactions */

	if (firstTimeOpen)
	{
		if (flags == O_RDONLY)
			pPipeDesc->procIdRead  = procId;
		else
			pPipeDesc->procIdWrite = procId;

		pPipeDesc->nMsgTransactions = 0;
	}

	return (OK);
}

/*+
 *	IGNORED FUNCTION NAME:
 *	mp_localclose
 *
 *	INVOCATION:
 *	mp_localclose (pName, flags, procId, pErrorNumber)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName			(char *)	name of pipe
 *	(>) flags			(int)		read/write mode, O_RDONLY or O_WRONLY
 *	(>)	procId			(int)		ID of processor closing pipe
 *	(<)	pErrorNumber	(uint32 *)	error number, 0 if no error
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if the pipe was closed, or ERROR if the close failed.
 *
 *	PURPOSE:
 *	Close local pipe device
 *
 *	DESCRIPTION:
 *	This routine closes a local pipe device in response to a command
 *	issued either locally or by a remote processor. If the pipe was
 *	previously opened by this processor, then the message queue and
 *	local buffer allocated for it are de-allocated. 
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	The pipe driver must have been initialised and the pipe must have
 *	been created and currently opened (in the requested mode) by any
 *	any processor on the bus.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS mp_localclose
	(
	char *		pName,
	int			flags,
	int			procId,
	uint32 *	pErrorNumber
	)
{
	SYM_TYPE		symType;
	MP_PIPE_DESC *	pPipeDesc;
	MSG_Q_INFO		msgQInfo;
	char			dummy;
	BOOL			timeout = FALSE;
	BOOL			lastTimeClose;
	FAST int		i;
	int				nTaskBlocked;
	int				nTaskBlockedInitially;
	struct timespec	timeStart;

	/* NB. Pipe name is always valid on entry. Get pipe descriptor from symbol table */

	if (symFindByNameAndType (mpPipeSymTab, pName, (char **) & pPipeDesc, & symType,
							  MP_PIPE_SYM_TYPE_NORMAL, MP_PIPE_SYM_TYPE_MASK) == ERROR)
	{
		* pErrorNumber = 1;
		ERROR_SET (0, "Could not find pipe descriptor in symbol table", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/*
	 * lastTimeClose is TRUE if the count of the number of times the pipe has been
	 * locally closed reaches zero. Only in this case is the pipe really closed.
	 */

	lastTimeClose = TRUE;										/* Assume last-time close */

	/*
	 * Before closing the pipe, ensure that any local tasks which are locally blocked
	 * on the msgQ in mpPipeRead() or mpPipeWrite() fail gracefully rather than crashing the
	 * system by, e.g., accessing data structures which are deleted here. A pipe should always
	 * be invalidated before deleting it but this is done here just to be sure.
	 * Then determine how many tasks are currently blocked. For each blocked task read/write
	 * (depending on whether the pipe is opened to read or write) a single byte from/to the msgQ
	 * and following each read/write wait until the number of blocked tasks has decremented by
	 * at least one. (NB. A read/write from an external processor could potentially un-blocked
	 * the task(s) so we only want to test that AT LEAST ONE, rather than EXACTLY ONE, un-block
	 * has occurred). When there are no more tasks blocked, proceed to delete the pipe. Note
	 * that as each mpPipeRead() or mpPipeWrite() un-blocks it will examine the pipe's validity
	 * and return with a "pipe invalid" error. Also, any subsequent attempts to read or write
	 * this pipe will fail because it has been marked as invalid.
	 */

	msgQInfo.msgListMax = 0;
	msgQInfo.taskIdListMax = 0;
	msgQInfo.taskIdList = NULL;
	msgQInfo.msgPtrList = NULL;
	msgQInfo.msgLenList = NULL;

	if (pPipeDesc->procIdRead == procId && flags == O_RDONLY)		/* Close pipe for reading */
	{
		/* Only need to un-block locally-blocked tasks (on THIS processor) */

		if (procId == sysextProcNumGet ())
		{
			/* Decrement close count and only proceed with close if the count has reached zero */

			if (--pPipeDesc->openCountRead > 0)
			{
				lastTimeClose = FALSE;
			}
			else
			{
				/* Determine the number of tasks blocked waiting to read from the pipe */

				msgQInfo.numMsgs = 0;
				msgQInfoGet (pPipeDesc->msgQId, & msgQInfo);
				nTaskBlocked = msgQInfo.numTasks;
				nTaskBlockedInitially = nTaskBlocked;

				for (i = 0; i < nTaskBlockedInitially; i++)
				{
					msgQSend (pPipeDesc->msgQId, (char *) & dummy, (UINT) 1, NO_WAIT, MSG_PRI_NORMAL);
					nTaskBlocked--;

					START_TIMEOUT (& timeStart);				/* Initialise the timeout timer */
					while (msgQInfo.numTasks > nTaskBlocked &&
							! (timeout = timeoutExpired (MP_PIPE_TIMEOUT_UNBLOCK, & timeStart)))
					{
						msgQInfo.numMsgs = 0;					/* Test for task still blocked on Rx */
						msgQInfoGet (pPipeDesc->msgQId, & msgQInfo);
						taskDelay (SEC_TO_NTICK (MP_PIPE_DELAY_UNBLOCK));
																/* Wait whilst un-blocking */
					}
					if (timeout)
					{
						* pErrorNumber = 1;
						ERROR_SET (S_mpPipeDrv_REMOTE_CPU_TIMEOUT, "Remote CPU error closing pipe",
							ERROR_LOG_SAVE);
						return (ERROR);
					}
				}
			}
		}

		/*
		 * Now its OK to really close the pipe. Delete the message queue and free the local
		 * buffer if they were created & allocated previously by this processor. Also
		 * delete the mutex semaphore used during pipe reads
		 */

		if (lastTimeClose)
		{
			if (procId == sysextProcNumGet ())
			{
				msgQDelete (pPipeDesc->msgQId);
				cacheDmaFree ((void *) pPipeDesc->pLocalBufferAlloc);
				pPipeDesc->msgQId = NULL;
				pPipeDesc->pLocalBufferAlloc = NULL;
				pPipeDesc->pLocalBuffer = NULL;
				semDelete (pPipeDesc->mutexSemRead);
				pPipeDesc->mutexSemRead = NULL;
			}
			pPipeDesc->procIdRead = -1;
			pPipeDesc->pRemoteWriteBuffer = NULL;
			pPipeDesc->pQueueFlag = NULL;
			pPipeDesc->pRemoteDoneFlag = NULL;
			pPipeDesc->nMsgTransactions = 0;
			pPipeDesc->localQueueFlag = FALSE;
		}
	}
	else if (pPipeDesc->procIdWrite == procId && flags == O_WRONLY)		/* Close pipe for writing */
	{
		/* Only need to un-block locally-blocked tasks (on THIS processor) */

		if (procId == sysextProcNumGet ())
		{
			/* Decrement close count and only proceed with close if the count has reached zero */

			if (--pPipeDesc->openCountWrite > 0)
			{
				lastTimeClose = FALSE;
			}
			else
			{
				/* Determine the number of tasks blocked waiting to write to the pipe */

				msgQInfo.numMsgs = pPipeDesc->maxNMsg;
				msgQInfoGet (pPipeDesc->msgQId, & msgQInfo);
				nTaskBlocked = msgQInfo.numTasks;
				nTaskBlockedInitially = nTaskBlocked;

				for (i = 0; i < nTaskBlockedInitially; i++)
				{
					msgQReceive (pPipeDesc->msgQId, (char *) & dummy, (UINT) 1, NO_WAIT);
					nTaskBlocked--;

					START_TIMEOUT (& timeStart);				/* Initialise the timeout timer */
					while (msgQInfo.numTasks > nTaskBlocked &&
						! (timeout = timeoutExpired (MP_PIPE_TIMEOUT_UNBLOCK, & timeStart)))
					{
						msgQInfo.numMsgs = pPipeDesc->maxNMsg;	/* Test for task still blocked on Tx */
						msgQInfoGet (pPipeDesc->msgQId, & msgQInfo);
						taskDelay (SEC_TO_NTICK (MP_PIPE_DELAY_UNBLOCK));
																/* Wait whilst un-blocking */
					}
					if (timeout)
					{
						* pErrorNumber = 1;
						ERROR_SET (S_mpPipeDrv_REMOTE_CPU_TIMEOUT, "Remote CPU error closing pipe",
							ERROR_LOG_SAVE);
						return (ERROR);
					}
				}
			}
		}

		/*
		 * Now its OK to really close the pipe. Delete the message queue and free the local
		 * buffer if they were created & allocated previously by this processor. Also
		 * delete the mutex semaphore used during pipe writes
		 */

		if (lastTimeClose)
		{
			if (procId == sysextProcNumGet ())
			{
				msgQDelete (pPipeDesc->msgQId);
				cacheDmaFree ((void *) pPipeDesc->pLocalBufferAlloc);
				pPipeDesc->msgQId = NULL;
				pPipeDesc->pLocalBufferAlloc = NULL;
				pPipeDesc->pLocalBuffer = NULL;
				semDelete (pPipeDesc->mutexSemWrite);
				pPipeDesc->mutexSemWrite = NULL;
			}
			pPipeDesc->procIdWrite = -1;
			pPipeDesc->nMsgTransactions = 0;
		}

		/* If the driver is installed, wakeup any tasks that may have been blocked */

#ifndef	NO_INSTALL
		if (lastTimeClose)
		{
			selWakeupAll (& pPipeDesc->selWakeupList, SELWRITE);
			selWakeupAll (& pPipeDesc->selWakeupList, SELREAD);
		}
#endif /* NO_INSTALL */
	}
	else
	{
		/*
		 * The pipe has not been opened by this processor, which is an error.
		 * NB. When called from Mbox IRQ, the procId should be that of the issuing command, not
		 * the ID number of this processor.
		 */

		* pErrorNumber = 1;
		ERROR_SET (S_mpPipeDrv_PIPE_NOT_OPEN, "Cannot close un-opened pipe", ERROR_LOG_SAVE);
		return (ERROR);
	}

	return (OK);
}

/*+
 *	IGNORED FUNCTION NAME:
 *	mp_localflush
 *
 *	INVOCATION:
 *	mp_localflush (pName, pErrorNumber)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName			(char *)	name of pipe
 *	(<)	pErrorNumber	(uint32 *)	error number, 0 if no error
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if the pipe was flushed, or ERROR if the flush failed.
 *
 *	PURPOSE:
 *	Flush local pipe's message queue and empty its VME buffer
 *
 *	DESCRIPTION:
 *	This routine flushes the message queue for a local pipe device and
 *	discards any incoming data which is currently available for reading
 *	in its VME buffer.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	The pipe driver must have been initialised and the pipe must have
 *	been created and currently opened (in the requested mode) by any
 *	any processor on the bus.
 *
 *	DEFICIENCIES:
 *	It is only possible to flush a pipe on its "write path". i.e. Only
 *	a processor which has opened the pipe to write may flush the pipe,
 *	and the file descriptor (passed to mpPipeIoctl()) must be that issued
 *	when the pipe was opened for writing. If a processor needs to flush
 *	out the "read path" of a pipe, then it should do so by simply reading
 *	the pipe until no further messages are available (as may be determined
 *	with the mpPipeIoctl() routine). Note that this routine uses taskLock()
 *	to disable pre-emption during part of the flushing procedure.
 *-
 */

STATUS	mp_localflush
	(
	char *		pName,
	uint32 *	pErrorNumber
	)
{
	SYM_TYPE		symType;
	MP_PIPE_DESC *	pPipeDesc;
	char			placeToDumpMessage;

	* pErrorNumber = 0;

	/* NB. Pipe name is always valid on entry. Get pipe descriptor from symbol table */

	if (symFindByNameAndType (mpPipeSymTab, pName, (char **) & pPipeDesc, & symType,
							  MP_PIPE_SYM_TYPE_NORMAL, MP_PIPE_SYM_TYPE_MASK) == ERROR)
	{
		* pErrorNumber = 1;
		ERROR_SET (0, "Could not find pipe descriptor in symbol table", ERROR_LOG_SAVE);
		return (ERROR);
	}

	/*
	 * Lock-out task pre-emption whilst flushing pipe to ensure that the pipe daemon
	 * doesn't attempt to read/write the pipe whilst it is being flushed
	 */

	taskLock ();
	if (pPipeDesc->procIdRead == sysextProcNumGet ())
	{
		* pPipeDesc->pLocalBuffer = 0;						/* Clear local Busy flag. This will */
															/* discard any data in the local buffer */
		CACHE_DMA_FLUSH ((char *) pPipeDesc->pLocalBuffer, 4);

		/*
		 * Clear local message queue by reading 1 byte from it until the queue is empty.
		 * Each 1-byte read will discard all other bytes written for each message in the queue,
		 * e.g. if the queue was created with N = maximum number of messages in queue, then
		 * msgQReceive() will be called up to N times (irrespective of the length of each of the
		 * N messages)
		 */

		while (msgQReceive (pPipeDesc->msgQId, & placeToDumpMessage, (UINT) 1, NO_WAIT) == OK)
			;												/* Loop body has null statement. */
	}

	if (pPipeDesc->procIdWrite == sysextProcNumGet ())
	{

		/*
		 * If there are any queued writes pending from another processor then de-queue them
		 * now to prevent the other processor attempting to de-queue them sometime in the
		 * future after this proc's write connection has been closed
		 */

		if ((pPipeDesc->procIdRead >= 0) && (pPipeDesc->procIdRead != sysextProcNumGet ()) &&
			(pPipeDesc->localQueueFlag))
		{
			pPipeDesc->localQueueFlag = FALSE;
			* pPipeDesc->pQueueFlag = 0;
			CACHE_DMA_FLUSH ((char *) pPipeDesc->pQueueFlag, 4);
		}

		/* Clear local message queue and, if the driver is installed,
		 * wake up any write-blocked tasks.
		 */

		while (msgQReceive (pPipeDesc->msgQId, & placeToDumpMessage, (UINT) 1, NO_WAIT) == OK)
			;												/* Loop body has null statement. */

#ifndef	NO_INSTALL
		selWakeupAll (& pPipeDesc->selWakeupList, SELWRITE);
#endif /* NO_INSTALL */
	}

	taskUnlock ();											/* Re-enable normal task scheduling */
	return (OK);
}

/*+
 *	IGNORED FUNCTION NAME:
 *	mp_LocalInvalidate
 *
 *	INVOCATION:
 *	mp_LocalInvalidate (pName, pErrorNumber)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName			(char *)	name of pipe
 *	(<)	pErrorNumber	(uint32 *)	error number, 0 if no error
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if the pipe was invalidated, or ERROR if the pipe
 *				name is invalid.
 *
 *	PURPOSE:
 *	Mark a pipe as invalid and therefore unavailable for further use
 *
 *	DESCRIPTION:
 *	This routine may be used to clear the VALID flag associated with
 *	a pipe. It is normally called only in the event of an internal
 *	error with the multi-processor pipe driver, but may be invoked
 *	via the routine mpPipeIoctl().
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	The pipe driver must have been initialised and the pipe must have
 *	been created.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS mp_LocalInvalidate
	(
	char *		pName,
	uint32 *	pErrorNumber
	)
{
	SYM_TYPE		symType;
	MP_PIPE_DESC *	pPipeDesc;

	/* NB. Pipe name is always valid on entry. Get pipe descriptor from symbol table */

	if (symFindByNameAndType (mpPipeSymTab, pName, (char **) & pPipeDesc, & symType,
							  MP_PIPE_SYM_TYPE_NORMAL, MP_PIPE_SYM_TYPE_MASK) == ERROR)
	{
		* pErrorNumber = 1;
		ERROR_SET (0, "Could not find pipe descriptor in symbol table", ERROR_LOG_SAVE);
		return (ERROR);
	}

	pPipeDesc->pipeValid = FALSE;						/* Mark pipe invalid */

	return (OK);
}

/*+
 *	IGNORED FUNCTION NAME:
 *	mp_IsRemoteProcAlive
 *
 *	INVOCATION:
 *	mp_IsRemoteProcAlive (processorNumber, pTimeStart, timeoutSecs)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	processorNumber	(int)				processor ID to interrogate
 *	(<) pTimeStart		(struct timespec *)	start time for timeout tests
 *	(>) timeoutSecs		(double)			timeout period in seconds
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK if the processor is alive and responding.
 *
 *	PURPOSE:
 *	Examine a remote processor's VME mailbox interface and determine
 *	whether the processor is ready for use by the driver
 *
 *	DESCRIPTION:
 *	This routine first waits for a remote processor to initialise its
 *	VME interface, then examines the processor's mailbox registers in
 *	search of a "heartbeat" count which should be output by each Slave
 *	processor during initialisation of their multi-processor pipe
 *	drivers. A timeout is enabled whilst the remote processor is being
 *	interrogated and an error returned if a valid response is not
 *	detected within the specified timeout period.
 *
 *	mp_IsRemoteProcAlive() is used by the multi-processor pipe driver
 *  initialisation routine
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	The local processor's VME interface must have been intialised with
 *	a previous call to sysextProcNumSet().
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS	mp_IsRemoteProcAlive
	(
	int					procNumber,
	struct timespec *	pTimeStart,
	double				timeoutSecs
	)
{
	char *			pProbeAdrs;
	char			probeData;
	uint32			heartbeat;
	uint32			heartbeatLast;
	BOOL			heartbeatDetected = FALSE;
	BOOL			timeout;

	START_TIMEOUT (pTimeStart);										/* Initialise the timeout timer */
	timeout = FALSE;

	/*
	 * Compute local address of the destination processor's RAM (first byte) as seen by this
	 * processor. This address is used to probe for the presence of the remote processor on the bus
	 */

	pProbeAdrs = (char *) CACHE_DMA_VIRT_TO_PHYS ((SYSEXT_EXT_VME_BASE +
		procNumber * SYSEXT_EXT_NBYTE_PER_PROC));
	if (sysextBusToLocalAdrs (VME_AM_EXT_SUP_DATA, pProbeAdrs, & pProbeAdrs) == ERROR)
	{
		ERROR_SET (0, "Failed to convert destination RAM pointer to local address", ERROR_LOG_SAVE);
		return (ERROR);
	}

#ifdef	DEBUG
	printf ("mpPipeDrv: waiting for processor #%d to enable VME interface\n", procNumber);
#endif

	/* Probe VME bus for remote processor; timeout if it isn't found */

	while ((vxMemProbe ((char *) pProbeAdrs, VX_READ, 1, & probeData) == ERROR) &&
		   ! (timeout = timeoutExpired (timeoutSecs, pTimeStart)))
	{
		taskDelay (SEC_TO_NTICK (MP_PIPE_DELAY_VME_ENABLE));
	}
	if (timeout)
	{
		ERROR_SET (S_mpPipeDrv_REMOTE_CPU_TIMEOUT, "Timeout probing VME for remote CPU",
		           ERROR_LOG_SAVE);
		return (ERROR);
	}

	/*
	 * Remote processor is now available on the bus. Read its heartbeat count (written to
	 * its local mailbox registers) and wait for a valid (changing) heartbeat count
	 */

	if (sysextReadRemoteMboxReg32 (procNumber, (uint32 *) & heartbeatLast) == ERROR)
	{
		ERROR_SET (0, "Failed to read heartbeat from remote mailbox register",
		           ERROR_LOG_SAVE);
		return (ERROR);
	}
#ifdef	DEBUG
	printf ("mpPipeDrv: waiting for processor #%d's heartbeat\n", procNumber);
#endif

	while ((heartbeatDetected == FALSE) && ! timeoutExpired (timeoutSecs, pTimeStart))
	{
		sysextReadRemoteMboxReg32 (procNumber, (uint32 *) & heartbeat);
		if (heartbeat == heartbeatLast)
		{
			taskDelay (SEC_TO_NTICK (MP_PIPE_HEARTBEAT_PERIOD));
		}
		else
		{
			heartbeatDetected = TRUE;
		}
	}
	if (! heartbeatDetected)
	{
		ERROR_SET (S_mpPipeDrv_NO_REMOTE_HEARTBEAT, "No heartbeat from remote CPU", ERROR_LOG_SAVE);
		return (ERROR);
	}

#ifdef	DEBUG
	printf ("mpPipeDrv: Heartbeat detected... now probing processor #%d\n", procNumber);
#endif

	/*
	 * Heartbeat has been detected. Now issue a mailbox IRQ to the remote processor and poll
	 * for a valid response (Done flag set and ID of remote processor) in the MCS
	 */

	if (sysextMboxIntGen (procNumber, MP_PIPE_MAILBOX) == ERROR)
	{
		ERROR_SET (0, "Failed to send mailbox interrupt to remote CPU", ERROR_LOG_SAVE);
		return (ERROR);
	}
	CACHE_DMA_INVALIDATE ((char *) & pMasterControlMsg->doneFlag, sizeof (MP_PIPE_CONTROL_MSG));

	while ((pMasterControlMsg->doneFlag == 0) && (pMasterControlMsg->procIdSource != procNumber) &&
		   ! (timeout = timeoutExpired (timeoutSecs, pTimeStart)))
	{
		taskDelay (SEC_TO_NTICK (MP_PIPE_DELAY_INITIALISE));
		CACHE_DMA_INVALIDATE ((char *) & pMasterControlMsg->doneFlag, sizeof (MP_PIPE_CONTROL_MSG));
	}

	if (timeout)
	{
		ERROR_SET (S_mpPipeDrv_REMOTE_CPU_TIMEOUT, "Timeout interrogating remote CPU", ERROR_LOG_SAVE);
		return (ERROR);
	}

	return (OK);
}

/*+
 *	FUNCTION NAME:
 *	mpPipeShutdown
 *
 *	INVOCATION:
 *	mpPipeShutdown ()
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	None
 *
 *	FUNCTION VALUE:
 *	None
 *
 *	PURPOSE:
 *	Shutdown and de-install multi-processor pipe driver and
 *	release system resources allocated to it
 *
 *	DESCRIPTION:
 *	This routine is used to shutdown a multi-processor pipe driver and
 *	clean-up resources after it. It is normally only required during
 *	debugging when re-initialisation of the driver may be needed. It
 *	is called by the driver initialisation routine, mpPipeDrv(), at the
 *	start of the initialisation procedure.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	DEFICIENCIES:
 *	Not all memory allocated by the driver is not currently freed by
 *	this routine.
 *-
 */

void mpPipeShutdown (void)
{
	int			daemonTask;
	DEV_HDR *	pDefunctDevHdr;
	char		pNamePrefix [MP_PIPE_MAX_BYTES_NAME + 1];
	char *		pNameTail;

	/*
	 * Don't bother with error checking here. This is a somewhat desparate attempt to clean-
	 * up after a previous incarnation of the pipe driver. There's not much we can do in
	 * the event of and error, and the chances are that we'd just want to plough on with
	 * re-initialisation of the driver (e.g. when this routine is called from mpPipeDrv()).
	 * If a really serious error does occur, it will be detected if/when the driver is
	 * re-initialised anyway.
	 */

	sysextMboxDisable (MP_PIPE_MAILBOX);
	if ((daemonTask = taskNameToId (MP_PIPE_DAEMON_NAME)) != ERROR)
		taskDelete (daemonTask);
	if (mpPipeSymTab != NULL) symTblDelete (mpPipeSymTab);
	if (mpPipeMutexSem != NULL) semDelete (mpPipeMutexSem);

#ifndef	NO_INSTALL

	/*
	 * Search for any IO devices in the system with names starting with the recommended
	 * prefix for multi-path pipes. Remove any such devices from the system.
	 */

	/* strcpy replace by strncpy. SMB - 4 Mar 98. */
	strncpy (pNamePrefix, MP_PIPE_NAME_PREFIX, MP_PIPE_MAX_BYTES_NAME);
																/* Pipe names to search for */
	pNameTail = NULL;
	while (pNameTail != pNamePrefix)
	{
		pDefunctDevHdr = iosDevFind (pNamePrefix, & pNameTail);	/* Lookup pipe with name prefix */

		/*
		 * Only delete the device found if it is not the default device (pNameTail will have
		 * been set equal to pNamePrefix if it is the default device).
		 */

		if ((pDefunctDevHdr != NULL) && (pNameTail != pNamePrefix))
		{
			iosDevDelete (pDefunctDevHdr);						/* Delete if not the default device */
		}
		else
		{
			pNameTail = pNamePrefix;							/* Abort search if no pipe dev found */
		}
	}

	if (mpPipeDrvNumber != NULL)
		iosDrvRemove (mpPipeDrvNumber, TRUE);	/* Remove driver with forced shutdown of open files */
#endif /* NO_INSTALL */
}

/*+
 *	IGNORED FUNCTION NAME:
 *	mp_PrintPipeDetails
 *
 *	INVOCATION:
 *	mp_PrintPipeDetails (pName, symValue, symType, argument, group)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName		(char *)	name of pipe
 *	(>)	symValue	(int)		pipe descriptor (pointer, cast as int)
 *	(>)	symType		(SYM_TYPE)	symbol type associated with pipe descriptor
 *	(>)	argument	(int)		arbitrary argument passed from symEach()
 *	(>)	group		(UINT16)	symbol group type associated with pipe desc
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK always.
 *
 *	PURPOSE:
 *	Print summary of multi-processor pipe status
 *
 *	DESCRIPTION:
 *	This routine prints a formatted display which summarises the current
 *	status of a multi-processor pipe. It is invoked from symEach() in the
 *	routine mpPipeShow().
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	None
 *
 *	DEFICIENCIES:
 *	None known
 *
 *	BUGS:
 *	Arguments "argument" and "group" are not referenced by this function.
 *	Do they exist for future exhancement, to allow debugging, or
 *	are they simply a left-overs that haven't been removed?
 *	SMB - 30 Jan 98.
 *-
 */

BOOL mp_PrintPipeDetails
	(
	char *		pName,
	int			symValue,
	SYM_TYPE	symType,
	int			argument,
	UINT16		group
	)
{
	MP_PIPE_DESC *	pPipeDesc;
	char			littleName [16];

	/*
	 * Skip over secondary pipe descriptor entries in the symbol table, since these are
	 * may be regarded as aliases of the primary descriptors and contain no additional
	 * information of relevance to this print-out
	 */

	if (symType == MP_PIPE_SYM_TYPE_ALT) return (TRUE);

	pPipeDesc = (MP_PIPE_DESC *) symValue;

	/*
	 * If the pipe name string is to large to fit in the 20-character space available,
	 * only print the first 15 and last 3 characters, with sub-string ".." between them
	 */

	if (strlen (pName) > 20)
	{
		strncpy (littleName, pName, 15);
		littleName [15] = 0;
		printf ("%.15s..", littleName);
		strncpy (littleName, & pName [strlen (pName) - 3], 3);
		printf ("%.3s  ", littleName);
	}
	else
	{
		printf ("%-20s  ", pName);
	}

	printf ("0x%08x 0x%08x  %2d %2d", (int) symValue, (int) pPipeDesc->msgQId,
			pPipeDesc->procIdRead, pPipeDesc->procIdWrite);

	if ((pPipeDesc->procIdRead == sysextProcNumGet ()) ||
	    (pPipeDesc->procIdWrite == sysextProcNumGet ()))
	{
		printf ("  %010d    %1d %1d ", pPipeDesc->nMsgTransactions, pPipeDesc->unbuffered,
		        pPipeDesc->pipeValid);
	}
	else
	{
		printf ("  XXXXXXXXXX    %1d %1d ", pPipeDesc->unbuffered, pPipeDesc->pipeValid);
	}

	if (pPipeDesc->procIdRead >= 0 && pPipeDesc->procIdRead != sysextProcNumGet ())
	{
		CACHE_DMA_INVALIDATE ((char *) pPipeDesc->pRemoteWriteBuffer, 4);
		CACHE_DMA_INVALIDATE ((char *) pPipeDesc->pRemoteDoneFlag, 4);
		CACHE_DMA_INVALIDATE ((char *) pPipeDesc->pQueueFlag, 4);
		printf ("%1d ", ((int) * pPipeDesc->pRemoteWriteBuffer) != 0);
		printf ("%1d ", ((int) * pPipeDesc->pRemoteDoneFlag) != 0);
		printf ("%1d\n", ((int) * pPipeDesc->pQueueFlag) != 0);
	}
	else if (pPipeDesc->procIdRead >= 0 && pPipeDesc->procIdRead == sysextProcNumGet ())
	{
		CACHE_DMA_INVALIDATE ((char *) pPipeDesc->pLocalBuffer, 4);
		CACHE_DMA_INVALIDATE ((char *) pPipeDesc->pLocalDoneFlag, 4);
		CACHE_DMA_INVALIDATE ((char *) pPipeDesc->pQueueFlag, 4);
		printf ("%1d ", ((int) * pPipeDesc->pLocalBuffer) != 0);
		printf ("%1d ", ((int) * pPipeDesc->pLocalDoneFlag) != 0);
		printf ("%1d\n", ((int) * pPipeDesc->pQueueFlag) != 0);
	}
	else
	{
		printf ("X X X\n");
	}

	return (TRUE);
}

/*+
 *	FUNCTION NAME:
 *	mpDemo1
 *
 *	INVOCATION:
 *	mpDemo1 (pName, nByteXfer, nBuffer, nLoop)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName		(char *)	name of pipe
 *	(>)	nByteXfer	(int)		number of bytes per transfer
 *	(>)	nBuffer		(int)		number of buffer stages
 *	(>)	nLoop		(int)		arbitrary argument passed from symEach()
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK or ERROR.
 *
 *	PURPOSE:
 *	Simple demonstration of buffered multi-processor pipes
 *
 *	DESCRIPTION:
 *	This routine provides an example of how buffered (normal) multi-
 *	processor pipes may be used to ping-pong a message between two
 *	processors. The routine should be spawned as a task on the two
 *	processors with an identical set of arguments for each task.
 *	The two processors will then "race" to attempt to generate the
 *	specified pipe and, whichever processor loses this race, will
 *	generate a second pipe with the extension "__" appended to the
 *	specified pipe name. A message of nByteXfer bytes
 *	in length is then written from the first processor to the
 *	second, then re-written from the second processor back to the
 *	first. This transfer is repeated nLoop times. An estimate of the
 *	achieved throughput between the two tasks is printed by each
 *	processor on completion of the demonstration.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	The multi-processor pipe driver must have been initialised.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS mpDemo1
	(
	char *	pNameR,
	int		nByteXfer,
	int		nBuffer,
	int		nLoop
	)
{
	FAST int		n;
	MP_PIPE_STATUS	status;
	uint32 *		pData;
	int				fdRead;
	int				fdWrite;
	struct timespec	t1;
	struct timespec	t2;
	float			timeInterval;
	BOOL			goFirst = TRUE;
	char			pName [MP_PIPE_MAX_BYTES_NAME + 1];
	char			pNameW [MP_PIPE_MAX_BYTES_NAME + 1];

	/* strcpy replaced by strncpy. strcat replaced by strncat. SMB - 18 Feb 98. */
	strncpy (pNameW, pNameR, MP_PIPE_MAX_BYTES_NAME - 2);		/* Make second pipe name */
	strncat (pNameW, "__", 2);

	t1.tv_sec = 0;												/* Initialise timer */
	t1.tv_nsec = (long) ((1.0e09) / sysClkRateGet ());
	if (t1.tv_nsec < 1) t1.tv_nsec = 1;
	if (clock_setres ((clockid_t) CLOCK_REALTIME, & t1) == ERROR)
	{
		printf ("mpDemo1: Error setting clock resolution\n");
		return (ERROR);
	}

	if (nBuffer < 1)
	{
		printf ("mpDemo1: Error, must have nBuffer > 0 for buffered pipes with this demo\n");
		return (ERROR);
	}
	if ((pData = (uint32 *) calloc ((size_t) nByteXfer, 1)) == NULL)
	{
		printf ("mpDemo1: Failed to allocate memory for buffers\n");
		return (ERROR);
	}

	/* Attempt to create the specified pipe */

	if (mpPipeDevCreate (pNameR, nBuffer, nByteXfer) == ERROR)
	{

		/*
		 * This processor lost the race to create the pipe (ERROR assumed returned
		 * because the pipe already existed; if ERROR was returned for some
		 * other reason we'll find out soon enough)
 		 */

		/* strcpy replace by strncpy. SMB - 18 Feb 98. */
		strncpy (pName, pNameR, MP_PIPE_MAX_BYTES_NAME);		/* Swap first and second pipe names */
		strncpy (pNameR, pNameW, MP_PIPE_MAX_BYTES_NAME);
		strncpy (pNameW, pName, MP_PIPE_MAX_BYTES_NAME);

		goFirst = FALSE;
		if (mpPipeDevCreate (pNameR, nBuffer, nByteXfer) == ERROR)
																/* Create the second pipe */
		{
			printf ("mpDemo1: Error creating pipe \"%s\"\n", pNameR);
			return (ERROR);
		}
	}

	/*
	 * Open the pipe to which this processor will read, and wait for the other
 	 * processor on the bus to create the pipe that we'll be writing. When the
	 * write pipe exists, open it to write
	 */

	if ((fdRead = open (pNameR, O_RDONLY, 0)) == ERROR)
	{
		printf ("mpDemo1: Error opening pipe \"%s\" to read\n", pNameR);
		return (ERROR);
	}

	printf ("mpDemo1: Waiting for remote processor to create pipe \"%s\"...", pNameW);
	while (mpPipeIoctlByName (pNameW, FIOFSTATGET, (int) & status) == ERROR)
		taskDelay (1);
	printf ("done\n");

	if ((fdWrite = open (pNameW, O_WRONLY, 0)) == ERROR)
	{
		printf ("mpDemo1: Error opening pipe \"%s\" to write\n", pNameW);
		return (ERROR);
	}

	if (goFirst)
	{
		/*
		 * Initialise the data to copy with a simple counting sequence, then loop
		 * doing write() then read() on this message
		 */

		for (n = 0; n < nByteXfer / 4; n++)
			* (pData + n) = n;

		printf ("mpDemo1: Test starting...\n");
		clock_gettime ((clockid_t) CLOCK_REALTIME, & t1);
		for (n = 0; n < nLoop; n++)
		{
			if (write (fdWrite, (char *) pData, nByteXfer) == ERROR)
			{
				printf ("mpDemo1: Error writing pipe\n");
				exit (ERROR);
			}
			if (read (fdRead, (char *) pData, nByteXfer) == ERROR)
			{
				printf ("mpDemo1: Error reading pipe\n");
				exit (ERROR);
			}
		}
		clock_gettime ((clockid_t) CLOCK_REALTIME, & t2);
	}
	else
	{
		/*
		 * Evidently we lost the race to create the specified pipe. That means
		 * we should read() BEFORE write()ing the pipe. No need to initialise
		 * the message either, since that was done by the other processor
		 */

		printf ("mpDemo1: Test starting...\n");
		clock_gettime ((clockid_t) CLOCK_REALTIME, & t1);
		for (n = 0; n < nLoop; n++)
		{
			if (read (fdRead, (char *) pData, nByteXfer) == ERROR)
			{
				printf ("mpDemo1: Error reading pipe\n");
				exit (ERROR);
			}
			if (write (fdWrite, (char *) pData, nByteXfer) == ERROR)
			{
				printf ("mpDemo1: Error writing pipe\n");
				exit (ERROR);
			}
		}
		clock_gettime ((clockid_t) CLOCK_REALTIME, & t2);
	}

	/* Estimate the data rate and print out the results */

	timeInterval = (float) (t2.tv_sec - t1.tv_sec + (t2.tv_nsec - t1.tv_nsec) * 1.0e-09);
	printf ("mpDemo1: Total time for data transfers was %e s\n", timeInterval);

	printf ("mpDemo1: Finished %d byte transfers to address 0x%x OK\n",
		nLoop * nByteXfer * 2, (int) pData);
	printf ("mpDemo1: Average throughput was %e Byte/s\n", (float) nLoop * nByteXfer * 2 / timeInterval);

	/* Free allocated memory, and close all pipes opened by this processor */

	cfree ((void *) pData);
	close (fdRead);
	close (fdWrite);
	printf ("mpDemo1: Waiting for remote processor to close pipe \"%s\"...", pNameR);

	/*
	 * Wait until the other processor closes the pipe that we created, then delete this pipe
 	 * once we're sure that it isn't still opened by anyone
	 */

	status.procIdWrite = 0;
	while (status.procIdWrite != -1)
	{
		mpPipeIoctlByName (pNameR, FIOFSTATGET, (int) & status);
		taskDelay (1);
	}
	printf ("done\n");

	if (mpPipeDevDelete (pNameR) == ERROR)
	{
		printf ("mpDemo1: Error deleting pipe \"%s\"\n", pNameR);
		return (ERROR);
	}

	return (OK);
}

/*+
 *	FUNCTION NAME:
 *	mpDemo2
 *
 *	INVOCATION:
 *	mpDemo2 (pName, nByteXfer, nLoop)
 *
 *	PARAMETERS: (">" input, "!" modified, "<" output)
 *	(>)	pName		(char *)	name of pipe
 *	(>)	nByteXfer	(int)		number of bytes per transfer
 *	(>)	nLoop		(int)		arbitrary argument passed from symEach()
 *
 *	FUNCTION VALUE:
 *	(STATUS)	OK or ERROR.
 *
 *	PURPOSE:
 *	Simple demonstration of un-buffered multi-processor pipes
 *
 *	DESCRIPTION:
 *	This routine provides an example of how un-buffered pipes are
 *	used. The routine should be spawned separately (but with
 *	identical arguments) on two different processors. Whichever
 *	processor starts up first will be adopted as the source of data,
 *	and the other processor will be adopted as the destination. A
 *	message of nByteXfer bytes in length is then transferred nLoop
 *	times from the source to the destination using an un-buffered
 *	pipe. An estimate of the achieved throughput between the two
 *	tasks is printed by each processor on completion of the
 *	demonstration.
 *
 *	EXTERNAL VARIABLES:
 *	None
 *
 *	PRIOR REQUIREMENTS:
 *	The multi-processor pipe driver must have been initialised.
 *
 *	DEFICIENCIES:
 *	None known
 *-
 */

STATUS mpDemo2
	(
	char *	pName,
	int		nByteXfer,
	int		nLoop
	)
{
	FAST int			n;
	MP_PIPE_STATUS		status;
	volatile uint32 *	pData;
	int					fd;
	struct timespec		t1;
	struct timespec		t2;
	float				timeInterval;
	BOOL				goFirst;
	SEM_ID				dataReadySem;

	t1.tv_sec = 0;										/* Initialise timer */
	t1.tv_nsec = (long) ((1.0e09) / sysClkRateGet ());
	if (t1.tv_nsec < 1) t1.tv_nsec = 1;
	if (clock_setres ((clockid_t) CLOCK_REALTIME, & t1) == ERROR)
	{
		printf ("mpDemo2: Error setting clock resolution\n");
		return (ERROR);
	}

	/* Attempt to create un-buffered pipe */

	if (mpPipeDevCreate (pName, 0, nByteXfer) != ERROR)
	{
		/*
		 * This processor was successful in creating the pipe. It is therefore
 		 * adopted as the source of data for this demo
		 */

		goFirst = TRUE;

		/*
		 * Open the pipe, create a binary semaphore used to mediate access to
		 * the local (write) buffer, then set this semaphore using ioctl()
		 */

		if ((fd = open (pName, O_WRONLY, 0)) == ERROR)
		{
			printf ("mpDemo2: Error opening pipe \"%s\" to write\n", pName);
			return (ERROR);
		}

		if ((dataReadySem = semBCreate (SEM_Q_FIFO, SEM_FULL)) == NULL)
		{
			printf ("mpDemo2: Error creating semaphore\n");
			return (ERROR);
		}

		if (ioctl (fd, FIOSETOPTIONS, (int) dataReadySem) == ERROR)
		{
			printf ("mpDemo2: Error in ioctl()\n");
			return (ERROR);
		}

		/*
		 * NB. There is no need to wait for the second processor to open
		 * the pipe before this one starts to write it. Any writes
		 * performed before the destination is opened are queued locally
		 * by the writing processor
		 */
	}
	else
	{
		/*
		 * This processor was unsuccessful in creating the pipe. It is
		 * therefore adopted as the destination for data during this demo
		 */

		goFirst = FALSE;

		/*
		 * Open the pipe, create a binary semaphore used to mediate access to
		 * the local (write) buffer, then set this semaphore using
		 * mpPipeIoctlByName(). Note that ioctl() cannot be used to set the
		 * semaphore ID because the pipe has not yet been opened, so the
		 * file descriptor is undefined. We don't want to open the pipe
		 * and THEN use ioctl() to set the semaphore ID since the processor
		 * which writes to the pipe is generally waiting to write as soon as
 		 * the pipe has been opened. It could therefore write to the pipe
		 * BEFORE the semaphore ID has been set. This way, we are sure that
		 * the pipe won't be written until the semaphore has been set up.
		 */

		if ((dataReadySem = semCCreate (SEM_Q_FIFO, 0)) == NULL)
		{
			printf ("mpDemo2: Error creating semaphore\n");
			return (ERROR);
		}
		if (mpPipeIoctlByName (pName, FIOSETOPTIONS, (int) dataReadySem) == ERROR)
		{
			printf ("mpDemo2: Error in mpPipeIoctlByName()\n");
			return (ERROR);
		}
		if ((fd = open (pName, O_RDONLY, 0)) == ERROR)
		{
			printf ("mpDemo2: Error opening pipe \"%s\" to read\n", pName);
			return (ERROR);
		}
	}

	/* Get pointer to the local buffer used for writing or reading */

	if (ioctl (fd, FIOFSTATGET, (int) & status) == ERROR)
	{
		printf ("mpDemo2: Error in ioctl()\n");
		return (ERROR);
	}
	pData = status.pBuffer;

	if (goFirst)
	{
		/*
		 * Initialise the data to copy with a simple counting sequence, then
		 * loop waiting on the semaphore to write*(). Note that the
		 * binary semaphore was created initially full to ensure that
		 * we can start off with a write()
		 */

		for (n = 0; n < nByteXfer / 4; n++)
			* (pData + n) = n;

		printf ("mpDemo2: Test starting...\n");
		clock_gettime ((clockid_t) CLOCK_REALTIME, & t1);
		for (n = 0; n < nLoop; n++)
		{
			/* Wait for the local buffer to become available */

			semTake (dataReadySem, WAIT_FOREVER);

			/*
			 * At this stage, following the semTake(), the local buffer
			 * (pointed to by pData) is free to be accessed.
			 * It would typically be filled with data destined for
			 * the second processor on the bus. Once the buffer is
			 * ready to be transferred, the following call to write()
			 * despatches the data to the destination.
			 */

			if (write (fd, NULL, nByteXfer) == ERROR)
			{
				printf ("mpDemo2: Error writing pipe\n");
				exit (ERROR);
			}
		}
		clock_gettime ((clockid_t) CLOCK_REALTIME, & t2);
	}
	else
	{
		/*
		 * Enter loop taking the semaphore then doing a read() once
		 * the data is available. Note that the semaphore was created
		 * initially empty to ensure that we always wait for the
		 * first write() before doing a read()
		 */

		printf ("mpDemo2: Test starting...\n");
		clock_gettime ((clockid_t) CLOCK_REALTIME, & t1);
		for (n = 0; n < nLoop; n++)
		{
			/* Wait for the local buffer to become available */

			semTake (dataReadySem, WAIT_FOREVER);

			/*
			 * At this stage, following the semTake() the local buffer
			 * (pointer to by pData) has been released by the driver and
			 * is free to be accessed. Data in the buffer would typically
			 * be read from it. Once the data is no longer needed, the
			 * following call to read() passes control of the buffer
			 * back to the driver thus enabling the next write from
			 * a remote processor into it.
			 */

			if (read (fd, NULL, nByteXfer) == ERROR)
			{
				printf ("mpDemo2: Error reading pipe\n");
				exit (ERROR);
			}
		}
		clock_gettime ((clockid_t) CLOCK_REALTIME, & t2);
	}

	/* Estimate throughput and print out the results */

	timeInterval = (float) (t2.tv_sec - t1.tv_sec + (t2.tv_nsec - t1.tv_nsec) * 1.0e-09);
	printf ("mpDemo2: Total time for data transfers was %e s\n", timeInterval);

	if (goFirst)
	{
		printf ("mpDemo2: Finished %d byte transfers from address 0x%x OK\n",
				nLoop * nByteXfer, (int) pData);
	}
	else
	{
		printf ("mpDemo2: Finished %d byte transfers to address 0x%x OK\n",
				nLoop * nByteXfer, (int) pData);
	}
	printf ("mpDemo2: Average throughput was %e Byte/s\n", (float) nLoop * nByteXfer / timeInterval);

	if (goFirst)
	{
		/*
		 * Wait until the pipe has been closed to read before closing it to write
		 * and deleting it
		 */

		status.procIdRead = 0;
		while (status.procIdRead != -1)
		{
			if (ioctl (fd, FIOFSTATGET, (int) & status) == ERROR)
			{
				printf ("mpDemo2: Error in ioctl()\n");
				return (ERROR);
			}
		}
		close (fd);
		if (mpPipeDevDelete (pName) == ERROR)
		{
			printf ("mpDemo2: Error deleting pipe \"%s\"\n", pName);
			return (ERROR);
		}
	}
	else
	{
		close (fd);
	}

	return (OK);
}
