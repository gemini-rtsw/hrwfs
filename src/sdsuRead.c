
/* Routines to initialise the SDSU DSPs & readout a CCD */

/* NOTE: These are Tim Hardy's test functions and are not part of sdsuLib. */

/* NOTE: setInterrupts, snapN and snapNPoll functions added by SMB - 15 January 1999 */
/* NOTE: sdsuTemp added by Tim Hardy - 23 January 1999 */

#include <stdio.h>
#include <cacheLib.h>
#include <semLib.h>
#include <sysLib.h>
#include <iv.h>
#include <intLib.h>
#include <usrLib.h>	/* not included in original version and omission caused errors */
#include <math.h>
#include "sdsuLib.h"
#include "errorLib.h"

/* Constants */
#define	NUM_HEADER_BYTES	(5*4)		/* 5 32-bit header words */
#define NEW_FBA_FLAG		0x800000	/* bit 23 of high word used to indicate new FBA */


/* Some static variables to reduce the typing needed to use these routines */

LOCAL	short	sdsuVectorNumber = SDSU_INT_NUMBER_BASE;

SDSU_ID sdsuId = NULL;
volatile uint16 *imBuffer = NULL;
long imPixels = 0;
long imPackets = 0;

SEM_ID	frameSem;

char *DSPfile[] = {					/* Default DSP code */
	"",
	"bin/asm56000/vme.lod",
	"bin/asm56000/tim-39.lod",
	"bin/asm56000/util.lod"
};

void intFunction (int param);		/* Interrupt service function. */


STATUS loadDSPcode (SDSU_ID id) {
	int dsp;
	
	if (id == NULL) {
		if (sdsuId == NULL) {
			ERROR_SET (S_sdsuLib_INV_STRUCTURE, "No SDSU context ID set", ERROR_LOG_NOW);
			return (ERROR);
		}
	} else {
		sdsuId = id;
	}
	
	for (dsp = SDSU_IDENT_VME; dsp < SDSU_IDENT_INVALID; dsp++) {
		printf ("Downloading %s to DSP %d...\n", DSPfile[dsp], dsp);
		if (sdsuFileDnload(sdsuId, DSPfile[dsp], dsp, FALSE)) {
			ERROR_SET1 (0, "loadDSPcode: Error downloading %s", ERROR_LOG_NOW, DSPfile[dsp]);
			return (ERROR);
		}
	}
	
	return OK;
}

STATUS imBufSetup (int nPixels, SDSU_ID id) {
	uint32 dmaAddress;
	int	bufSize;
	uint32 packetSize;
	
	if (id == NULL) {
		if (sdsuId == NULL) {
			ERROR_SET (S_sdsuLib_INV_STRUCTURE, "No SDSU context ID set", ERROR_LOG_NOW);
			return (ERROR);
		}
	} else {
		sdsuId = id;
	}
	
	if (imBuffer != NULL) {
		cacheDmaFree(imBuffer);
		imBuffer = NULL;
	}

	if (nPixels <= 0) {
		ERROR_SET (S_sdsuLib_INV_STRUCTURE, "Number of pixels not given", ERROR_LOG_NOW);
		return (ERROR);
	}
	
	bufSize = nPixels * 2 + NUM_HEADER_BYTES;	/* image + header */	
	imBuffer = (uint16 *) cacheDmaMalloc(bufSize);
	if (imBuffer == NULL) {
		ERROR_SET (0, "Image memory allocation failed", ERROR_LOG_NOW);
		return (ERROR);
	}
	imPixels = nPixels;

	if (sdsuParamRead (sdsuId, SDSU_IDENT_VME, "V_PSIZE", &packetSize) == ERROR)
	{
		ERROR_LOG ("imBufSetup: Could not query packet size");
		return (ERROR);
	}
	imPackets = (long) ceil ((double) imPixels / (double) packetSize);

	printf ("%ld pixels and %ld packets of %ld pixels each\n", imPixels, imPackets, packetSize);

	if (sysLocalToBusAdrs(SDSU_AM_VME_MASTER_REP, (char *) imBuffer, 
						  (char **) & dmaAddress) == ERROR)
	{
		ERROR_SET (0, "imBufSetup: Couldn't map image address to VME", ERROR_LOG_NOW);
		return (ERROR);
	}
	
	if (sdsuParamWrite(sdsuId, SDSU_IDENT_VME, "V_FBALO", dmaAddress & 0xffff) ||
		sdsuParamWrite(sdsuId, SDSU_IDENT_VME, "V_FBAHI", (dmaAddress >> 16) | NEW_FBA_FLAG)) {
		ERROR_LOG ("imBufSetup: DMA address not set in VME DSP");
		return (ERROR);
	}
	
	return OK;
}


STATUS writeFits (char *filename, int x, int y, uint16 *pimBuf) {
	int i;
	FILE *fp;
	
	fp = fopen (filename, "w");
	if (fp == NULL) {
		ERROR_SET (0, "Can't create/open FITS file", ERROR_LOG_SAVE);
		return (ERROR);
	}
	
	fprintf (fp, "SIMPLE  =                    T /                                                ");
	fprintf (fp, "BITPIX  =                   16 /                                                ");
	fprintf (fp, "NAXIS   =                    2 /                                                ");
	fprintf (fp, "NAXIS1  =                %5d /                                                ", x);
	fprintf (fp, "NAXIS2  =                %5d /                                                ", y);
	fprintf (fp, "END                                                                             ");
	
	/* fill up the remaining minimum number of header records */
	for (i = 0; i < 30; i++) {
		fprintf (fp, "                                                                                ");
	}
	
	/* write image data */
	fwrite ((pimBuf+(NUM_HEADER_BYTES/2)), sizeof (uint16), x*y, fp);
	
	/* tidy */
	if (fclose (fp)) {
		ERROR_SET (0, "Problem closing FITS output file", ERROR_LOG_SAVE);
		return (ERROR);
	}
	
	printf ("writeFits: Image saved to file %s\n", filename);
	return OK;
}



STATUS snap(void) {
	return sdsuPrimitive(sdsuId, "RDC", SDSU_IDENT_VME, NULL, NULL);
}

STATUS setInterrupts ( SDSU_ID id )
{

	if (id == NULL) {
		if (sdsuId == NULL) {
			ERROR_SET (S_sdsuLib_INV_STRUCTURE, "No SDSU context ID set\n", ERROR_LOG_NOW);
			return (ERROR);
		}
	} else {
		sdsuId = id;
	}

	if (sdsuVectorNumber >= SDSU_INT_NUMBER_LIMIT)
	{
		ERROR_SET (S_sdsuLib_NO_INT_AVAILABLE, "Run out of interrupt vectors", ERROR_LOG_NOW);
		return (ERROR);
	}

	sdsuId->frameIntNum = sdsuVectorNumber++;

	printf ("Connecting interrupt number %d to vector %p\n",
		sdsuId->frameIntNum, INUM_TO_IVEC (sdsuId->frameIntNum));

	if ( intConnect (INUM_TO_IVEC (sdsuId->frameIntNum), intFunction, (int) sdsuId)
	     == ERROR
	   )
	{
		ERROR_SET (0, "Failed to connect frame interrupt service routine", ERROR_LOG_NOW);
		return (ERROR);
	}

	/*
	 * Tell the VME DSP what vector numbers to use
	 */

	if ( sdsuParamWrite (sdsuId, SDSU_IDENT_VME, "V_FIID", sdsuId->frameIntNum << 16) == ERROR)
	{
		ERROR_SET (0, "Failed to set SDSU frame interrupt vector parameter", ERROR_LOG_NOW);
		return (ERROR);
	}

	if ( sdsuParamWrite (sdsuId, SDSU_IDENT_VME, "V_PIID", 0) == ERROR)
	{
		ERROR_SET (0, "Failed to set SDSU frame interrupt vector parameter", ERROR_LOG_NOW);
		return (ERROR);
	}

	frameSem = semBCreate( SEM_Q_FIFO, SEM_EMPTY );
	if ( frameSem == NULL )
	{
		ERROR_SET (0, "Failed to create frame sync semaphore", ERROR_LOG_NOW);
		return (ERROR);
	}

	/*
	 * Finally enable interrupts.
	 */

	if ( sysIntEnable (SDSU_VME_INT_LEVEL) == ERROR )
	{
		ERROR_SET (0, "Failed to enable system interrupts", ERROR_LOG_NOW);
		return (ERROR);
	}
	if ( sdsuParamWrite (sdsuId, SDSU_IDENT_VME, "V_INT_EN", SDSU_FRAME_INT_ENABLE) == ERROR )
	{
		ERROR_SET (0, "Failed to enable SDSU interrupts", ERROR_LOG_NOW);
		return (ERROR);
	}

	return (OK);
}

void intFunction ( int param )
{
	int key;
	logMsg ("--- Interrupt --- %d\n", param, 0, 0, 0, 0, 0);
	key = intLock();
	semGive ( frameSem );
	intUnlock (key);
}

STATUS multiTDL ( SDSU_ID id, int nTimes )
{
	int loop;
	uint32 input, output;
	int failures;

	if (id == NULL)
	{
		ERROR_SET (S_sdsuLib_INV_STRUCTURE, "No SDSU context ID set\n", ERROR_LOG_NOW);
		return (ERROR);
	}

	failures = 0;
	for ( loop=0; loop<nTimes; loop++ )
	{
		input = (uint32) loop;
		if (sdsuPrimitive (id, "TDL", SDSU_IDENT_TIM, &input, &output) == ERROR)
		{
			ERROR_SET1 (0, "sdsuPrimitive failed at step %d", loop, ERROR_LOG_NOW);
			failures++;
		}
		else
		{
			if ( output == input )
			{
				printf ("Step %d: Sent %ld, received %ld - OK\n", loop, input, output);
			}
			else
			{
				printf ("Step %d: Sent %ld, received %ld - * DATA MISMATCH *\n", loop, input, output);
				failures++;
			}

		}
	}

	printf ("Completed with %d failures\n", failures);
	return (OK);
}


STATUS snapN (int nFrames) {

	int frame;
	uint32 dmaAddress;

	if ( imBuffer == NULL )
	{
		ERROR_SET (0, "snapN: No image buffer defined", ERROR_LOG_NOW);
		return (ERROR);
	}

	if ( frameSem == NULL )
	{
		ERROR_SET (0, "snapN: Interrupts not connected", ERROR_LOG_NOW);
		return (ERROR);
	}
	
	if (sysLocalToBusAdrs(SDSU_AM_VME_MASTER_REP, (char *) imBuffer, 
						  (char **) & dmaAddress) == ERROR) {
		ERROR_SET (0, "snapN: Couldn't map image address to VME", ERROR_LOG_NOW);
		return (ERROR);
	}

	for (frame=0; frame<nFrames; frame++)
	{
		printf ("Frame %d of %d. FBA.\n", frame, nFrames);

		if (sdsuParamWrite(sdsuId, SDSU_IDENT_VME, "V_FBALO", dmaAddress & 0xffff) ||
			sdsuParamWrite(sdsuId, SDSU_IDENT_VME, "V_FBAHI", (dmaAddress >> 16) | NEW_FBA_FLAG))
		{
			ERROR_LOG ("snapN: DMA address not set in VME DSP");
			return (ERROR);
		}

		/* printf ("Frame %d RDC.\n", frame); */

		if ( sdsuPrimitive(sdsuId, "RDC", SDSU_IDENT_VME, NULL, NULL) == ERROR )
		{
			ERROR_LOG ("snapN: RDC command failed");
			return (ERROR);
		}

		printf ("Waiting for frame sync interrupt...");
		semTake ( frameSem, WAIT_FOREVER );
		printf ("interrupt received.\n");
	}

	printf ("Observation completed.\n");

	return (OK);
}


STATUS snapNPoll (int nFrames) {

	int frame;
	uint32 dmaAddress;

	volatile uint32* pPacketCount;

	pPacketCount = (uint32*) imBuffer;

	if ( imBuffer == NULL )
	{
		ERROR_SET (0, "snapN: No image buffer defined", ERROR_LOG_NOW);
		return (ERROR);
	}

	if (sysLocalToBusAdrs(SDSU_AM_VME_MASTER_REP, (char *) imBuffer, 
						  (char **) & dmaAddress) == ERROR) {
		ERROR_SET (0, "snapN: Couldn't map image address to VME", ERROR_LOG_NOW);
		return (ERROR);
	}

	for (frame=0; frame<nFrames; frame++)
	{
		printf ("Frame %d of %d. FBA.\n", frame, nFrames);

		if (sdsuParamWrite(sdsuId, SDSU_IDENT_VME, "V_FBALO", dmaAddress & 0xffff) ||
			sdsuParamWrite(sdsuId, SDSU_IDENT_VME, "V_FBAHI", (dmaAddress >> 16) | NEW_FBA_FLAG))
		{
			ERROR_LOG ("snapN: DMA address not set in VME DSP");
			return (ERROR);
		}

		/* Initialise the packet count. */
		*pPacketCount = 0;

		printf ("Frame %d RDC.\n", frame);

		if ( sdsuPrimitive(sdsuId, "RDC", SDSU_IDENT_VME, NULL, NULL) == ERROR )
		{
			ERROR_LOG ("snapN: RDC command failed");
			return (ERROR);
		}

		/* Poll for completion of the frame. */

		printf ("Polling for packet count...");
		while ( *pPacketCount < imPackets )
		{
			taskDelay(0.01 * sysClkRateGet());
		}
		printf ("all packets received.\n");
	}

	printf ("Observation completed.\n");

	return (OK);
}

STATUS save (char *filename) {
	return writeFits(filename, 160, 40, imBuffer);
}

STATUS setExposure (double seconds) {
	uint32 exp;

	exp = (uint32) (seconds/ (double) 81.92e-6);

	printf ("Exposure time of %f seconds translates to T_EXP_TIM=%ld\n", seconds, exp);

	return sdsuParamWrite (sdsuId, SDSU_IDENT_TIM, "T_EXP_TIM", exp);
}


void codeVersions(SDSU_ID id) {
	printf("VME interface board firmware = %lx\n",sdsuVersionGet(id,1,1));
	printf("                    software = %lx\n\n",sdsuVersionGet(id,1,0));
	printf("       Timing board firmware = %lx\n",sdsuVersionGet(id,2,1));
	printf("                    software = %lx\n\n",sdsuVersionGet(id,2,0));
	printf("      Utility board firmware = %lx\n",sdsuVersionGet(id,3,1));
	printf("                    software = %lx\n\n",sdsuVersionGet(id,3,0));
}


STATUS sdsuTemp(SDSU_ID id) {
	int	i,t0,t1,t2;
	float	temp0,temp1,temp2;
	uint32	data;

	if (sdsuParamRead(id, 3, "U_ADC0", &data) == ERROR)
		return (ERROR);
	t0 = data;

	t1 = 0;
	t2 = 0;

	for (i=0; i<20; i++) {
		if (sdsuParamRead(id, 3, "U_ADC6", &data) == ERROR)
			return (ERROR);
		t1 = t1 + data;
		if (sdsuParamRead(id, 3, "U_ADC7", &data) == ERROR)
			return (ERROR);
		t2 = t2 + data;
	}

	temp0 = 25.0 + 0.38*( (float)t0 - 2789.0);
	temp1 = ( (float)t1 / 20.0 ) * (-0.01545);
	temp2 = ( (float)t2 / 20.0 ) * (-0.01545);

	printf(" Approximate values....  \n");
	printf("Utility board = %2.2f C\n",temp0);
	printf("           T1 = %2.2f C\n",temp1);
	printf("           T2 = %2.2f C\n",temp2);

	if (sdsuParamRead(id, 3, "U_CCDT_TGT", &data) == ERROR)
		return (ERROR);
	printf(" Target temp = %2.2f C\n",(float)data *(-0.01545) );

	if (sdsuParamRead(id, 3, "U_DAC2", &data) == ERROR)
		return (ERROR);
	printf(" TEC voltage = %2.2f V\n",((float)data - 2048) * (5.0/2048.0) );

	return (OK);

}
