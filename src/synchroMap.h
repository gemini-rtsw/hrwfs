enum
{
	INT1 = 1,		/* synchro bus interrupt numbers */
	INT2,
	INT3
};

enum
{
	SCS_NODE = 0,		/* synchro bus node identifiers */
	M2_NODE,
	AG_NODE
};



/* structure of Gemini Synchro Bus pages */

#define	SYNCHROBASE	(0xf0a00040)	/* base address of synchro bus card vmic5588		*/

typedef union
{
	unsigned all;	/* name to refer to the whole structure */
	char byte[4];	/* individual bytes of whole structure	*/

	struct
	{
		unsigned offloaders		: 1;	/* bit 31 */
		unsigned mirrorControl		: 1;
		unsigned mirrorMoving		: 1;
		unsigned mirrorCommanded	: 1;
		unsigned mirrorResponding	: 1;
		unsigned sensorLimit		: 1;
		unsigned actuatorLimit		: 1;
		unsigned thermalLimit		: 1;
		unsigned mirrorDspInt		: 1;
		unsigned vibDspInt		: 1;
		unsigned blank			: 10;	/* bits 12 through 21 currently unused */
		unsigned space			: 1;
		unsigned decsFrozen		: 1;
		unsigned decsPaused		: 1;
		unsigned decsOn			: 1;
		unsigned chopOn			: 1;
		unsigned vibControlOn		: 1;
		unsigned testInProgress		: 1;
		unsigned resetInProgress	: 1;
		unsigned initInProgress		: 1;
		unsigned powerEnabled		: 1;
		unsigned diagnosticsAvailable	: 1;
		unsigned health			: 1;	/* bit 0 */
	} flags;

} bitFieldM2;

/* SCS to M2 command block */

typedef struct
{
	long		checksum;
	long		NS;
	long		commandCode;
	float		xTiltGuide;
	float		yTiltGuide;
	float		zFocusGuide;
	float		AxTilt;
	float		AyTilt;
	float		BxTilt;
	float		ByTilt;
	float		CxTilt;
	float		CyTilt;
	float		actuator1;
	float		actuator2;
	float		actuator3;
	long		heartbeat;
	float		xDemand;
	float		yDemand;
	long		centralBaffle;
	long		deployBaffle;
	long		chopProfile;
	float		chopFrequency;
	float		chopDutyCycle;
	float		xTiltTolerance;
	float		yTiltTolerance;
	float		zFocusTolerance;
	float		xPositionTolerance;
	float		yPositionTolerance;
	float		bandwidth;
	float		xTiltGain;
	float		yTiltGain;
	float		zFocusGain;
	float		xTiltShift;
	float		yTiltShift;
	float		zFocusShift;
	float		xTiltSmooth;
	float		yTiltSmooth;
	float		zFocusSmooth;
	float		pad[218];
}commandBlock;

/* M2 to SCS status block */

typedef struct
{
	long		checksum;
	long		NR;
	float		xTilt;
	float		yTilt;
	float		zFocus;
	float		actuator1;
	float		actuator2;
	float		actuator3;
	long		inPosition;
	long		chopTransition;
	bitFieldM2	statusWord;
	long		heartbeat;
	long		beamPosition;
	float		xPosition;
	float		yPosition;
	long		deployBaffle;
	long		centralBaffle;
	float		baffleEncoderA;
	float		baffleEncoderB;
	float		baffleEncoderC;
	long		topEnd;
	float		enclosureTemp;
	float		pad[234];
}statusBlock;

/* structure of diagnostic codes */

typedef struct
{
	char index;
	char system;
	char subsystem;
	char code;
}fault;

typedef struct
{
	long	checksum;
	long	number;
	fault	faults[30];
	float	pad[224];
}diagBlock;

typedef struct
{
	float	z1;
	float	z2;
	float	z3;
	float	err1;
	float	err2;
	float	err3;
	char	name[16];
	float	interval;
	double	time;
}wfs;

typedef struct
{
	long	currentBeam;
	long	inPosition;	
	float	xTilt;
	float	yTilt;
	float	zFocus;
	float	xPosition;
	float	yPosition;
	double	time;
}eventBlock;

typedef struct
{
	commandBlock	page0;
	statusBlock	page1;
	diagBlock	testResults;
	float		pad1[1024];
	eventBlock	eventData;
	float		pad2[247];
	wfs		pwfs1;
	float		pad3[243];
	wfs		pwfs2;
	float		pad4[243];
	wfs		oiwfs;
	float		pad5[243];
	wfs		gaos;
	float		pad6[243];
	wfs		gyro;
}memMap;



















