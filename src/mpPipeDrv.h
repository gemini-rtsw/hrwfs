/* mpPipeDrv.h - multi-processor pipe driver header
 *
 *	Define macro NO_INSTALL if the driver is to be used as a VxWorks library directly, in which
 *	case it will not be installed in the IO system. This mode is intended primarily for debugging.
 */

/*
modification history
--------------------
01a,02Dec96,nd	Written.
*/

#ifndef	__INCmpPipeDrvh
#define	__INCmpPipeDrvh

/* includes */

#ifdef vxWorks
#include <vxWorks.h>
#else
#error This code only runs under VxWorks
#endif	/* vxWorks */

#include <msgQLib.h>
#include <semLib.h>
#include <symLib.h>
#include <ioLib.h>
#include <iosLib.h>
#include <selectLib.h>
#include <timers.h>
#include "sysextLib.h"
#include "gemTypes.h"

/* defines */

#define	MP_PIPE_SYMTAB_MAX_N		1024		/* Max number of pipe devices allowed */
#define	MP_PIPE_SYMTAB_HASHSIZE		((MP_PIPE_SYMTAB_MAX_N) * 2)
#define	MP_PIPE_MAX_BYTES_NAME		80			/* Max length of pipe name, excluding null termination */
#define	MP_PIPE_NAME_PREFIX			"/pipe/"	/* Recommend same name prefix as for normal pipes */
#define	MP_PIPE_TIMEOUT_COMMAND		60.0		/* Timeout (s), proc executing command over VME */
#define	MP_PIPE_TIMEOUT_UNBLOCK		0.1			/* Timeout (s), unblocking tasks when deleting pipe */
#define	MP_PIPE_DELAY_VME_ENABLE	1.0			/* Delay period (s), waiting for VME i/face to enable */
#define	MP_PIPE_DELAY_INITIALISE	0.1			/* Delay period (s), waiting for slave to initialise */
#define	MP_PIPE_DELAY_COMMAND		0.1			/* Delay period (s), whilst executing command over VME */
#define	MP_PIPE_DELAY_UNBUF_WRITE	0.0			/* Delay period (s), whilst writing unbuffered pipe */
#define	MP_PIPE_DELAY_TAS			0.001		/* Delay period (s), whilst TAS'ing busy flag */
#define	MP_PIPE_DELAY_UNBLOCK		0.0			/* Delay period (s), unblocking tasks when deleting pipe */
#define	MP_PIPE_HEARTBEAT_PERIOD	1.0			/* Period (s) of heartbeat output by slave */
#define	MP_PIPE_MAILBOX				3			/* Number of mailbox used by driver */

	/*
	 * Pipe command function codes. These are written to the Master Control Structure (MCS)
	 * by any processor when it attempts to broadcast a command to the network
	 */

#define	MP_PIPE_FUNC_IDLE			0			/* Do nothing */
#define	MP_PIPE_FUNC_INIT_SLAVE		1			/* Master initialising slave processor */
#define	MP_PIPE_FUNC_CREATE			2			/* Create pipe */
#define	MP_PIPE_FUNC_DELETE			3			/* Delete pipe */
#define	MP_PIPE_FUNC_OPEN			4			/* Open pipe (read or write, only used locally) */
#define	MP_PIPE_FUNC_OPEN_RD		5			/* Open pipe to read */
#define	MP_PIPE_FUNC_OPEN_WR		6			/* Open pipe to write */
#define	MP_PIPE_FUNC_CLOSE			7			/* Close pipe (read or write, only used locally) */
#define	MP_PIPE_FUNC_CLOSE_RD		8			/* Close pipe for reading */
#define	MP_PIPE_FUNC_CLOSE_WR		9			/* Close pipe for writing */
#define	MP_PIPE_FUNC_INVALIDATE		10			/* Mark pipe invalid (not available for further use) */
#define	MP_PIPE_FUNC_FLUSH			11			/* Flush out pipe's write path */

	/*
	 * Error number codes used by mpPipeDrv.
	 * These are designed to be processed using the vxWorks "makeStatTbl" utility.
	 */

#define	S_mpPipeDrv_REMOTE_CPU_TIMEOUT		(M_mpPipeDrv | 1)
#define	S_mpPipeDrv_PIPE_NOT_OPEN			(M_mpPipeDrv | 2)
#define	S_mpPipeDrv_INV_REMOTE_CPU_RESP		(M_mpPipeDrv | 3)
#define	S_mpPipeDrv_CANT_READ_PIPE			(M_mpPipeDrv | 4)
#define	S_mpPipeDrv_CANT_WRITE_PIPE			(M_mpPipeDrv | 5)
#define	S_mpPipeDrv_PIPE_READ_FAILED		(M_mpPipeDrv | 6)
#define	S_mpPipeDrv_PIPE_WRITE_FAILED		(M_mpPipeDrv | 7)
#define	S_mpPipeDrv_PIPE_CLOSED_BLOCKED		(M_mpPipeDrv | 8)
#define	S_mpPipeDrv_UNEXPECTED_IRQ			(M_mpPipeDrv | 9)
#define	S_mpPipeDrv_INVALID_PIPE_NAME		(M_mpPipeDrv | 10)
#define	S_mpPipeDrv_IOCTL_FAILED			(M_mpPipeDrv | 11)
#define	S_mpPipeDrv_TIMEOUT_AWAITING_MCS	(M_mpPipeDrv | 12)
#define	S_mpPipeDrv_INVALID_PIPE_MODE		(M_mpPipeDrv | 13)
#define	S_mpPipeDrv_NO_REMOTE_HEARTBEAT		(M_mpPipeDrv | 14)
#define	S_mpPipeDrv_INVALID_PIPE_FUNC		(M_mpPipeDrv | 15)

	/* Assorted other definitions */

#define	MP_PIPE_DAEMON_NAME			"mpPiped"	/* Name of daemon task which handles mailbox interrupts */
#define	MP_PIPE_DAEMON_PRIORITY		50			/* Priority of daemon task */
#define	MP_PIPE_DAEMON_STACK		2000		/* Stack size for daemon task */
#define	MP_PIPE_HEARTBEAT_MAX_VALUE	0x1000000	/* Max value of heartbeat count before roll-over to zero */
#define	MP_PIPE_SYM_TYPE_NORMAL		((SYM_TYPE) 0) /* Symbol type for pipe descriptor */
#define	MP_PIPE_SYM_TYPE_ALT		((SYM_TYPE) 1) /* Symbol type for alternative pipe desc */
#define	MP_PIPE_SYM_TYPE_MASK		((SYM_TYPE) 1) /* Symbol type mask: only use lsb */
#define	MP_PIPE_SYMBOL_GROUP		((UINT16) 0)   /* Symbol group is arbitrary: not used by driver */
#define	OFFSET_TO_QUEUE_FLAG		1			/* uint32 offset (4 byte) to queue flag in local buffer */
#define	OFFSET_TO_NBYTES			2			/* uint32 offset to number of bytes word in local buffer */
#define	OFFSET_TO_FIRST_BYTE		3			/* uint32 offset to first data byte in local buffer */
#define	MP_PIPE_VME_BLT_MODE		VME_BLT_D64	/* VME block transfer mode: use D64 whenever possible */
#define	MP_PIPE_BLT_ALIGN			2048		/* Byte alignment for data memory transfers over VME */

/* typedefs */

typedef struct									/* Master Control Structure (in Master proc's local RAM) */
	{
	char				doneFlag;				/* Set when MCS finished not in use */
	char				busyFlag;				/* Set (normally via TAS) when MCS in use */
	uint16				pad16bits;				/* Padding to next 4-byte boundary */
	int32				procIdSource;			/* Proc ID for source of command in MCS */
	int32				procIdDest;				/* Proc ID for destination of command in MCS */
	uint32				function;				/* Function number for command in MCS */
	uint32				errorNumber;			/* Error number returned by destination proc */
	uint32				maxNMsg;				/* Argument to pipe create function */
	uint32				maxNBytePerMsg;			/* Argument to pipe create function */
	volatile uint32 *	pRemoteWriteBuffer;		/* Argument to pipe open-to-read function */
	volatile char *		ppRemoteCmdFlag [SYSEXT_MAX_N_PROC]; /* Arguments to init function */
	char				pName [MP_PIPE_MAX_BYTES_NAME + 1]; /* Argument for all pipe functions */
	} MP_PIPE_CONTROL_MSG;

typedef struct									/* Pipe device descriptor */
	{
	DEV_HDR				deviceHeader;			/* Device header, used by iosLib */
	SEL_WAKEUP_LIST		selWakeupList;			/* Wakeup list used by selectLib */
	void *				pOtherPipeDef;			/* Pointer to "alternative" pipe descriptor */
	int					localOpenMode;			/* Local open mode (read or write) for descriptor */
	BOOL				pipeValid;				/* True when pipe is valid */
	int32				procIdWrite;			/* ID of proc which has opened pipe to write */
	int32				procIdRead;				/* ID of proc which has opened pipe to read */
	uint32				localQueueFlag;			/* Local queue flag, set when a write has been queued */
	MSG_Q_ID			msgQId;					/* ID of pipe's msgQ */
	int					maxNMsg;				/* Max num msgs in msgQ */
	int					maxNBytePerMsg;			/* Max num bytes per msg in msgQ */
	char *				pLocalBufferAlloc;		/* Pointer to first byte allocated for local buffer */
	volatile uint32 *	pLocalBuffer;			/* Adopted pointer to local buffer after re-alignment */
	volatile uint32 *	pLocalDoneFlag;			/* Pointer to done flag in local buffer */
	volatile uint32 *	pRemoteWriteBuffer;		/* Pointer to remote buffer (destination for pipe write) */
	volatile uint32 *	pRemoteDoneFlag;		/* Pointer to remote done flag */
	volatile uint32 *	pQueueFlag;				/* Pointer to remote queue flag */
	uint32				nMsgTransactions;		/* Count of number of local msg transactions on pipe */
	int	*				pNByteReady;			/* Array of number of bytes stored in each slot of msgQ */
	SEM_ID				bufferReadySem;			/* Semaphore optionally given when data read from VME bus */
	SEM_ID				mutexSemWrite;			/* Semaphore used to ensure only one task writes at a time */
	SEM_ID				mutexSemRead;			/* Semaphore used to ensure only one task reads at a time */
	int					openCountRead;			/* Count of the number of opens locally performed on a pipe */
	int					openCountWrite;			/* Count of the number of opens locally performed on a pipe */
	BOOL				unbuffered;				/* Unbuffered option set if pipe created with 0 msg slots */
	} MP_PIPE_DESC;

typedef struct									/* Pipe status structure returned by ioctl (FIOFSTATGET) */
	{
		int					procIdRead;			/* ID of proc which has opened pipe to write */
		int					procIdWrite;		/* ID of proc which has opened pipe to read */
		BOOL				pipeValid;			/* True when pipe is valid */
		volatile uint32 *	pBuffer;			/* Pointer to 1st data byte in allocated data buffer */
	} MP_PIPE_STATUS;

/* static variables */

/* function declarations */

IMPORT STATUS	mpPipeDrv (double timeoutSecs, const BOOL resetBusOnTimeout);
IMPORT STATUS	mpPipeDevCreate (char * pName, int nMessages, int nByte);
IMPORT STATUS	mpPipeDevDelete (char *	pName);
IMPORT STATUS	mpPipeShow (char * pName);
IMPORT void		mpPipeShutdown (void);
IMPORT int		mpPipeIoctlByName (char * pName, int function, int arg);
IMPORT STATUS	mpDemo1 (char * pName, int nByteXfer, int nBuffer, int nLoop);
IMPORT STATUS	mpDemo2 (char * pName, int nByteXfer, int nLoop);

/*
 * Following function should really be declared in timers.h, but it isn't (in VxWorks Vn 5.2).
 * The declaration below is included to stop the compiler from issuing a warning message. May
 * need to remove this declaration if/when timers.h gets fixed.
 *
 * NOTE: This function is also declared in timeoutLib.h.
 */

IMPORT int		clock_setres (clockid_t clock_id, struct timespec * res);

/*
 * Only include the following functions if the driver is NOT to be installed in the VxWorks I/O
 * system (which is a special option requested by defining NO_INSTALL). If the driver is
 * installed, then these functions are declared static in mpPipeDrv.c and are not accessible
 * as library function calls.
 */

#ifdef	NO_INSTALL
IMPORT int		mpPipeOpen (DEV_HDR * deviceHeader, char * pRemainder, int flags);
IMPORT int		mpPipeClose (int fd);
IMPORT int		mpPipeIoctl (int fd, int function, int arg);
IMPORT int		mpPipeRead (int fd, char * pBuffer, size_t maxNBytes);
IMPORT int		mpPipeWrite (int fd, char * pBuffer, size_t maxNBytes);
#endif	/* NO_INSTALL */

#endif	/* __INCmpPipeDrvh */
