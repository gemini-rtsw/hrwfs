/* vmi5588.h - VMIC VMIVME5578 device driver C interface */
/* $Id: vmi5588.h,v 1.1.1.1 1999-03-17 03:14:27 cboyer Exp $
*
*	Author:		Andrew Johnson
*	Date:		10-10-94
*
* Experimental Physics and Industrial Control System (EPICS)
*
* Developed at the Royal Greenwich Observatory for the Gemini
* 8M Telescopes Project.
*/

/*
modification history
--------------------
$Log: not supported by cvs2svn $
Revision 1.1.1.1  1997/11/28 11:46:17  anj
Imported using tkCVS

Revision 1.1.1.1  1997/03/11 23:14:00  goodrich
Gem4

Revision 1.1.1.1  1996/10/21 20:56:25  jwright
Gemini EPICS


*/

#ifndef INCvmi5588h
#define INCvmi5588h

/* Status bits returned by rmStatus */
#define RM_IRQ1   0x0001	/* Interrupt 1 Pending */
#define RM_IRQ2   0x0002	/* Interrupt 2 Pending */
#define RM_IRQ3   0x0004	/* Interrupt 3 Pending */
#define RM_NOSIG  0x0010	/* No Input Signal */
#define RM_NOSYNC 0x0020	/* Input PLL unsynchronized */
#define RM_RESYNC 0x0040	/* PLL Recently unsynchronized */
#define RM_NORING 0x0400	/* Fibre Ring broken */
#define RM_BADXFR 0x0800	/* Single Transfer Error */
#define RM_TXHALF 0x2000	/* Transmit FIFO Half-full */
#define RM_RXHALF 0x4000	/* Receive FIFO Half-full */

/* Data types for symbol lookup */
#define RM_TYPE_PAGE 0x80
#define RM_TYPE_ALOG 0x01
#define RM_TYPE_LONG 0x02
#define RM_TYPE_STRG 0x03
#define RM_TYPE_ARRY 0x04
#define RM_TYPE_USER 0x10


#define RM_MAX_ATTEMPTS 10

extern int rmMaxAttempts;


/* structures for use in device support */
struct rm_data {
	unsigned char   rm_type;
	unsigned char   pad1;
	unsigned short  protect1;
	unsigned short  protect2;
	unsigned short  pad2;
	union {
		double          Alog;
		long            Long;
		char            Strg[40];
		struct {
			unsigned long	nelm;
			unsigned short	ftyp;
			unsigned short	pad3;
			char		data[4]; /* Size changes with array */
		} array;
	} value;
};

struct rmpvt {
	short           page;
	short           offset;
	struct rm_data *address;
};


/* Routines available to C applications */
long   vmi5588_report(void);
long   vmi5588_init(void);
long   rmIntConnect(int irqNumber, VOIDFUNCPTR proutine);
long   rmIntDisconnect(int irqNumber);
long   rmIntSend(int irqNumber, int nodeId);
long   rmNodeId(void);
long   rmStatus(long reset);
long   rmLoadSymbols(void);
void  *rmAddr(char *pname, int rmType);
long   rmLookup(char *pname, int rmType,
                struct rm_data **pprmData, 
                short *prmPage, short *prmOffset);
void   rmPrintSymbols(void);


#ifdef NO_EPICS
/* Define the error constants */
#define M_devSup (504 <<16) /*Device Support*/
#define M_devLib (521 <<16) /*Device Resource Registration*/

#define S_dev_vectorInUse (M_devLib| 1) /*Interrupt vector in use*/
#define S_dev_vxWorksVecInstlFail (M_devLib| 2) /*vxWorks interrupt vector install failed*/
#define S_dev_uknIntType (M_devLib| 3) /*Unrecognized interrupt type*/ 
#define S_dev_vectorNotInUse (M_devLib| 4) /*Interrupt vector not in use by caller*/
#define S_dev_badA16 (M_devLib| 5) /*Invalid VME A16 address*/
#define S_dev_badA24 (M_devLib| 6) /*Invalid VME A24 address*/
#define S_dev_badA32 (M_devLib| 7) /*Invalid VME A32 address*/
#define S_dev_uknAddrType (M_devLib| 8) /*Unrecognized address space type*/
#define S_dev_addressOverlap (M_devLib| 9) /*Specified device address overlaps another device*/ 
#define S_dev_identifyOverlap (M_devLib| 10) /*This device already owns the address range*/ 
#define S_dev_vxWorksAddrMapFail (M_devLib| 11) /*vxWorks refused address map*/ 
#define S_dev_intDisconnect (M_devLib| 12) /*Interrupt at vector disconnected from an EPICS device*/ 
#define S_dev_internal (M_devLib| 13) /*Internal failure*/ 
#define S_dev_vxWorksIntEnFail (M_devLib| 14) /*vxWorks interrupt enable failure*/ 
#define S_dev_vxWorksIntDissFail (M_devLib| 15) /*vxWorks interrupt disable failure*/ 
#define S_dev_noMemory (M_devLib| 16) /*Memory allocation failed*/ 
#define S_dev_addressNotFound (M_devLib| 17) /*Specified device address unregistered*/ 
#define S_dev_noDevice (M_devLib| 18) /*No device at specified address*/
#define S_dev_wrongDevice (M_devLib| 19) /*Wrong device type found at specified address*/
#define S_dev_badSignalNumber (M_devLib| 20) /*Signal number (offset) to large*/
#define S_dev_badSignalCount (M_devLib| 21) /*Signal count to large*/
#define S_dev_badRequest (M_devLib| 22) /*Device does not support requested operation*/
#define S_dev_highValue (M_devLib| 23) /*Parameter to high*/
#define S_dev_lowValue (M_devLib| 24) /*Parameter to low*/
#define S_dev_multDevice (M_devLib| 25) /*Specified address is ambiguous (more than one device responds)*/
#define S_dev_badSelfTest (M_devLib| 26) /*Device self test failed*/
#define S_dev_badInit (M_devLib| 27) /*Device failed during initialization*/
#define S_dev_hdwLimit (M_devLib| 28) /*Input exceeds Hardware Limit*/
#define S_dev_deviceDoesNotFit (M_devLib| 29) /*Unable to locate address space for device*/
#define S_dev_deviceTMO (M_devLib| 30) /*device timed out*/

#define S_dev_noDevSup      (M_devSup| 1) /*SDR_DEVSUP: Device support missing*/
#define S_dev_noDSET        (M_devSup| 3) /*Missing device support entry table*/
#define S_dev_missingSup    (M_devSup| 5) /*Missing device support routine*/
#define S_dev_badInpType    (M_devSup| 7) /*Bad INP link type*/
#define S_dev_badOutType    (M_devSup| 9) /*Bad OUT link type*/
#define S_dev_badInitRet    (M_devSup|11) /*Bad init_rec return value */
#define S_dev_badBus        (M_devSup|13) /*Illegal bus type*/
#define S_dev_badCard       (M_devSup|15) /*Illegal or nonexistant module*/
#define S_dev_badSignal     (M_devSup|17) /*Illegal signal*/
#define S_dev_NoInit        (M_devSup|19) /*No init*/
#define S_dev_Conflict      (M_devSup|21) /*Multiple records accessing same signal*/

#else
/* Routines for use with EPICS device support only */
long vmi5588_pageInit(short rmPage);
void vmi5588_pvtInit(struct rmpvt **ppdpvt, short rmPage, short rmOffset,
                     struct rm_data *prmData);
long vmi5588_getIoscanpvt(struct rmpvt *pdpvt, void **pscanpvt);
long vmi5588_trigger(short rmPage);
#endif

#endif /* INCvmi5588h */

