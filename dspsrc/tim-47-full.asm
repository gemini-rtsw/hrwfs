       COMMENT *
Gemini WFS Timing Board Code
CCD: EEV CCD47
Controller: SDSU2
Revision: 1.22   (must agree with T_SW_ID in Y: memory table)
(This code is adapted from timEEV written by Dr. Bob Leach at SDSU)

This is the full version of the timing board code. It does allow
arbitrary binning in the X (serial) direction, and there is checksum 
calculation and the option of producing simulated data.

97/07/25 BML -initial coding

97/10/21 TDH -renovated the X and YTAIL calculations. The total rasters now
              do not include a "space" after the last subaperture in either
              the X or Y directions (this space is usually split between CCD
              quadrants). Added a clear B before moving X/YSIZE into B0. 
             -pursuant to the above, changed the subap loops so that they 
              do not flush the X/YSPACE after the last subap. 
             -changed error number reporting to bits of an T_ERROR word
             -moved the underscan flush to before the XSTART flush (originally
              these were reversed).
             -moved serial flush out of frame transfer loop so that only one
              flush is performed at the end of the transfer.
97/10/23 TDH -added initialization code (DAC settings)
             -added waveform tables and CLOCK subroutine
             -inserted code to do the charge transfers and digitization
             -added code to set the gain, speed and CDS integration periods
             -removed simulated data transmission
97/10/29 TDH -added code to flush out the ADC pipeline (read an extra two 
              pixels)
97/11/05 TDH -added code to check for zero exposure time
97/11/13 TDH -added IDL and STP routines so that timing board code could
              be changed by CCDtool without resetting the controller
97/11/18 TDH -fixed bug in the code that sets the integration time (changed
              byte compilation from ADD to OR and changed the store result MOVE 
              to specify A1 as the source instead of A because otherwise it 
              will put $7f into the desired location if the MSB of A1 is a one).
              Changed the set gain/speed code the same way (to be safe).
97/12/01 TDH -added lines to set/reset the AUX3 bit of Port B which is 
              mapped to D15 in the image data (sync bit).
             -revamped gain/speed setting and combined these parameters in the
              parameter table
             -changed the integration time setting to load the switch states
              from the waveform table
             -added conditional assembler directives to remove the parameter
              flag and checksum transmission and the IDL and STP routines, and
              to adjust the command table if using CCDTool
             -combined T_USCAN and XSTART serial flush
             -added some memory overflow warnings
             -reordered the high speed serial code to put the conversion
              after the integration in order to avoid an initial garbage pixel
             -changed the video board switch state word sent in the 
              initialization code from $000FFF, which seems to cause problems.
97/12/08 TDH -added a bin x 2 serial waveform and the option to select it 
              instead of the normal bin x 1
             -added optional readout of underscan
             -removed CLOCK subroutine and put code inline
98/01/20 TDH -changed initialization to clear DUALCLK bit, i.e. do not clock
              the two halves of the clock board together
             -changed command table so that an 'SBV' (sent by the utility 
              board during power on) executes the initialization routine
              which sets the DACs
98/01/22 TDH -made it capable of receiving commands properly during exposure,
              and moved the WW change to after the exposure so that, for
              the first frame at least, it will also send proper responses
              (for compatibility with CCDtool).
             -added code to process commands received during exposure (in the
              final system, only 'RDC' and 'ABT' will be valid commands 
              during exposure).
98/01/27 TDH -changed INIT sequence so that the bias DACs and clock 
              drivers are enabled after they are set and settled.
98/02/03 TDH -fixed bug in the code that chooses the serial waveforms 
              (binning factor) - needed a CLR of the accumulator at the
              beginning.
98/02/18 TDH -shuffled the Y: memory parameter table and added a seperate
              table for on-the-fly changes
             -added LDP and WRP commands to perform on-the-fly changes. LDP
              now does the setup that was done at the beginning of RDC.
             -added code to deal with RDC received during exposure 
              (readout synchronization)
             -changed how ABT command is handled so that it performs a 
              readout and transmits one last frame. ABT removed from command
              table (since it is not valid except during readout).
             -changed the way the RDM command is handled in the CCDtool case
              (now goes through first part of the command parsing here
              rather than in the boot code).
             -removed the XTAIL and YTAIL calculations
             -added code to transmit the frame header words from ICD 1.6/1.10
             -on exposure overrun error, T_EXP_TMR is now set to zero instead
              of the uncorrected exposure time
98/03/05 TDH -changed the sync bit operation 
             -added reset of T_EXP_TMR at the end of RDC, and moved the reset of
              T_FRAMEC so that it is always executed, even with no SYNC request
             -added set default data source (sim/image) in INIT, and put default
              values in the Y: table for T_EXP_TMR, T_FRAMEC, and SER_WF
             -removed YTAIL parameter since it is not used
98/03/11 TDH -changed to full-feature version (includes arbitrary binning and
              checksum calculation)
             -added simulated data option
98/05/11 TDH -changed all waveform sending loops to REP loops
             -removed serial flush before YSTART flush
             -added extra delay between transmissions of last three pixels
              (in FL_PIPE waveform table)
             -changed command returns which went to IDLE to go to START so
              that the command buffer would be reset.
98/06/29 TDH -added individual ADC offsets for each channel
             -added comments
             -added another delay instruction to the end of FL_PIPE waveform
              (not sure why this is necessary, but it won't work otherwise)
             -changed the SYNC routine to use the ABORT sequence instead of
              repeating the code
             -moved application code specific definitions from header file
              (T_STATUS, T_ERROR, and T_MODE bit definitions)
98/07/15 TDH -fixed bug in WRP command (problem with address verification) and 
              changed it so that it could be used to write to any address in
              Y: memory if it is executed while idling.
             -changed sync bit operation to work with old sync bit PALs
98/07/20 TDH -changes for new sync bit PALs (U12/U17 Rev 4.1)
             -minor change to checksum transmission
             -fixed bug in frame counter transmission
98/07/27 TDH -adapted for CCD47 (changed waveforms, DAC settings, default Y:  
              parameters; changed serial flushes to use dump gate; removed 
              gain/speed setting of second video board)
98/09/08 TDH -reversed serial clocking directions.
             -added non-IMO clocking option
             -leave SR1LR and SR2LR on throughout the vertical transfer process
             -pickup and leave charge under one pixel in serial transfer process
             -added workaround for bug in new sync bit PAL (U17 Rev 4.1)
             -moved internal P: memory overflow warning to before pipeline
              pixel transmission. 
              -moved the clear of IDLING in T_STATUS from the idling code to
              the beginning of the readout.
98/09/10 TDH -made the ADC input offsets parameters
             -added an assembler directive for IMO/non-IMO clocking
98/11/13 TDH -fixed bug in on-the-fly command processing: needed extra 
              command buffer pointer increment
             -fixed bug in ABORT function: changed to use stored constant
              (X:<ONE) instead of immediate value (#1)
99/01/06 TDH -implemented infinite series readout (if T_NFRAME=0)
             -changed abort function so that it sends an empty frame
              (no pixel data) as the last frame
             -made the minimum exposure time one tick (81.92 us)
             -added reset of simulated data pixel counter between 
              successive frames in a series
             -added delay to last entry in parallel clock waveform tables


Assembler directives:

-d DOWNLOAD 1	To generate code for downloading to DSP memory.
-d DOWNLOAD 0	To generate code for writing to the EEPROM.

-d CCDTOOL  1	To generate code for use with CCDTool application
-d CCDTOOL  0	To generate code for use with Gemini control software

-d IMOCLK   1   To generate code for IMO clocking.
-d IMOCLK   0   To generate code for nonIMO clocking.

	*

	PAGE    132     ; Printronix page width - 132 columns
	OPT	CEX	; print DC evaluations

; Define a section name so it doesn't conflict with other application programs
	SECTION	TIM
	INCLUDE 'timhead.asm'

APL_NUM	EQU	1	; Application number from 1 to 10

;**************************************************************************
;                   	                                                  *
;    Permanent address register assignments                               *
;	 R1 - Pointer to data transmission subroutine			  *
;	 R2 - Pixel counter for simulated data				  *
;        R3 - Pointer to current top of command buffer                    *
;        R4 - Pointer to processed contents of command buffer		  *
;        R6 - CCD clock driver address for CCD #0 = $FF80                 *
;                It is also the A/D address of analog board #0            *
;                                                                         *
;    Other registers                                                      *
;        R0, R7 - Temporary registers used all over the place.            *
;        R5 - Can be used as a temporary register but is circular,        *
;               modulo 32.       					  *
;**************************************************************************

;  Specify execution and load addresses
	IF	DOWNLOAD
	ORG	P:APL_ADR,P:APL_ADR		; Download address
	ELSE
	ORG     P:APL_ADR,P:(2*APL_NUM-1)*$100	; EEPROM generation
	ENDIF


; *****  Idle  *****
; Keep the CCD idling when not reading out. Continuously flush device,
; stopping after each serial transfer to check for received commands

	IF	CCDTOOL
IDLE	JSET	#EXPING,Y:<T_STATUS,CHK_TMR	; Check if we were exposing
						; and return to exposure if so
	BSET    #IDLING,Y:<T_STATUS	; Revise status

	ELSE
IDLE	BSET    #IDLING,Y:<T_STATUS	; Revise status

	ENDIF

IDLE2	DO      Y:<T_XSIZE,IDL1	; Loop over number of pixels per line
	MOVE    #<XCLOCK,R0	; Address of serial (skip) clocking waveform
	NOP			; register access restriction
	MOVE    Y:(R0)+,X0	; # of waveform entries 
	MOVE    Y:(R0)+,A       ; Start the pipeline
	DO      X0,CLK0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
CLK0
	MOVE    A,X:(R6)        ; Flush out the pipeline
	JSR     <GET_RCV        ; Check for FO or SSI commands and reset WDT
        JCC     <NO_COM         ; Continue IDLE if no commands received
        ENDDO                   ; Cancel the DO loop system stack numbers
        JMP     <CHK_SSI        ; Go process header and command
NO_COM  NOP
IDL1
	MOVE    #<YISCLOCK,R0	; Address of parallel (S&I) clocking waveform
	NOP			; register access restriction
	MOVE    Y:(R0)+,X0	; # of waveform entries 
	MOVE    Y:(R0)+,A       ; Start the pipeline
	DO      X0,CLK00		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
CLK00
	JMP     <IDLE2


; *****  Image data transmission subroutine  *****
; Read ADCs, write image data to serial transmitter, and update the checksum.
; In sync bit mode, a bit mask is used so that data added to checksum is the 
; same as the data transmitted

X_NORM	MOVE	Y:XMT_MSK,Y1	; get bit mask

	MOVE	Y:RDAD0,A	; get pixel value (output 0)
	AND	Y1,A1		; mask off lowest 15 bits (if sync mode)
	MOVEP	A1,Y:WRFO	; transmit value
	ADD	A,B		; add to checksum
	REP	#2
	NOP			; delay for transmission

	MOVE	Y:RDAD1,A	; get pixel value (output 1)
	AND	Y1,A1		; mask off lowest 15 bits (if sync mode)
	MOVEP	A1,Y:WRFO	; transmit value
	ADD	A,B		; add to checksum
	REP	#2
	NOP			; delay for transmission

	RTS


; *****  Readout CCD  *****
; This routine performs the exposure timing, readout clocking and image
; data transmission. For optimum speed, the main section should be in 
; internal memory. This avoids extra delays due to external memory access.
; For this reason, the initial setup code and chip clear has been placed
; after the core readout loops.

RDCCD	BCLR    #IDLING,Y:<T_STATUS	; Revise status
	JMP	<IMG_CLR	; clear image area before first exposure

; Start exposure
EXPOSE	BSET	#EXPING,Y:<T_STATUS	; Set status to expose
	MOVEP	#$800,X:TCR	; timer counts to zero every 81.92us
	MOVEP	#1,X:TCSR	; enable hardware timer in mode 0
CHK_COM	JSR	<GET_RCV	; check for another command and reset WDT
	JCS	<PRC_CMD	; if yes, process command 

CHK_TMR	JCLR	#TMR_ST,X:TCSR,CHK_COM	; check hardware timer
	CLR	A		
	MOVE	Y:<T_EXP_TMR,A0	; if zero, decrement exposure timer
	DEC	A
	MOVE	A0,Y:<T_EXP_TMR
	JNE	<CHK_COM	; if exposure timer is not zero, loop back

; End of exposure, start readout
	BCLR	#EXPING,Y:<T_STATUS	; clear expose status
	BCLR	#TMR_EN,X:TCSR	; disable hardware timer
	BSET	#RDING,Y:<T_STATUS	; set status to readout
	BSET	#WW,X:PBD	; Set word width = 1 for 16-bit image data
	JCLR	#ESYNC,Y:MODE,SET_R1	; skip if not sync mode
	BCLR	#FD15,X:PBD	; Set sync bit value to 0
	BSET	#FMODE,X:PBD	; Enable sync bit

SET_R1	MOVE	Y:SXMIT,R1	; put data transmission routine address into R1

; Do frame transfer
	DO      Y:<T_YSIZE,LFT	; transfer image region to storage region
	MOVE    #<YISCLOCK,R0	; Address of parallel (S&I) clocking waveform
	NOP			; register access restriction
	MOVE    Y:(R0)+,X0	; # of waveform entries 
	MOVE    Y:(R0)+,A	; Start the pipeline
	REP	X0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline
LFT

; Start readout timer
	MOVEP	#$25,X:TCSR	; enable timer in mode 4 for low pulse width
	BCLR	#TIO,X:PBD	; TIO = 0

; Clear checksum
	CLR	B		; clear checksum

; Transmit frame header words
	IF	!CCDTOOL
	MOVE	Y:B_0_14,Y1	; get bit mask ($007FFF)
	MOVE	Y:<PARMID,A	; get parameter identifier word
	AND	Y1,A1		; mask off lowest 15 bits
	JSET	#ESYNC,Y:<MODE,XMT_PID
	BSET	#15,A1		; set bit 15 if not sync mode
XMT_PID	MOVEP	A1,Y:WRFO	; transmit parameter ID
	ADD	A,B		; add to checksum
	CLR	A	; clear A
	MOVE	Y:<T_FRAMEC,A0	; get frame counter
	REP	#9
	ASL	A		; shift high byte (b15-23) into A1
	AND	Y1,A1 #0,A0	; mask off lowest 15 bits, clear A0
	MOVEP	A1,Y:WRFO	; transmit high word of frame counter
	ADD	A,B		; add to checksum
	REP	#8		; delay for serial data transmission
	NOP
	MOVE	Y:<T_FRAMEC,A 	; get frame counter
	AND	Y1,A1		; mask off low word (b0-14)
	MOVEP	A1,Y:WRFO	; transmit low word of frame counter
	ADD	A,B		; add to checksum
	REP	#8		; delay for serial data transmission
	NOP
	MOVE	Y:T_OUTPUTS,A	; get number of outputs
	AND	Y1,A1		; mask off low word (b0-14)
	MOVEP	A1,Y:WRFO	; transmit number of outputs
	ADD	A,B		; add to checksum
	CLR	A 
	MOVE	Y:<N_PIXEL,A0	; get number of pixels per output
	REP	#9
	ASL	A		; shift high byte (b15-23) into A1
	AND	Y1,A1 #0,A0	; mask off lowest 15 bits, clear A0
	MOVEP	A1,Y:WRFO	; transmit high byte of number of pixels
	ADD	A,B		; add to checksum
	REP	#8		; delay for serial data transmission
	NOP
	MOVE	Y:<N_PIXEL,A	; get number of pixels per output
	AND	Y1,A1		; mask off lowest 15 bits
	MOVEP	A1,Y:WRFO	; transmit low word of number of pixels
	ADD	A,B		; add to checksum

	ENDIF

; Skip readout if abort command received
	JSET	#ABT_EXP,Y:<T_STATUS,END_RD	; skip readout if aborted

; Discard initial unread rows (YSTART)
	MOVE	Y:<YSTART,A
	TST	A
	JEQ	LDROWS
	DO      A,LDROWS
	MOVE    #<YSCLOCK,R0	; Address of parallel (S only) clocking waveform
	MOVE	(R2)+N2		; increment pixel counter
	MOVE    Y:(R0)+,X0	; # of waveform entries 
	MOVE    Y:(R0)+,A	; Start the pipeline
	REP	X0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)	; Flush out the pipeline
LDROWS

; Flush serial register
	MOVE    #<XDUMP,R0	; Address of serial dump clocking waveform
	NOP			; register access restriction
	MOVE    Y:(R0)+,X0	; # of waveform entries 
	MOVE    Y:(R0)+,A       ; Start the pipeline
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline

; Start the loop for reading the number of Y subapertures
	DO      Y:<YSUBAP,LYSUB

; Read one Y subaperture
	DO	Y:<YRAS,LYRAS
	MOVE	A,P:RSTWDT 	; Reset watchdog timer

; Bin together rows 
	MOVE	(R2)-N2		; predecrement pixel counter
	DO	Y:<YBIN,LYBIN
	MOVE    #<YSCLOCK,R0	; Address of parallel (S only) clocking waveform
	MOVE	(R2)+N2		; increment pixel counter
	MOVE    Y:(R0)+,X0	; # of waveform entries 
	MOVE    Y:(R0)+,A	; Start the pipeline
	REP	X0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline
LYBIN

; Check if underscan output has been requested.
	MOVE	Y:<T_USCAN,A
	JCLR	#OUSCN,Y:<MODE,FL_STRT

; Read the underscan pixels
	DO	Y:<T_USCAN,LUSCN
	JSR	(R1)		; Retrieve and transmit image data
	MOVE    #<INT_RST,R0	; address of reset integration waveform
	MOVE	(R2)+		; increment pixel counter
	MOVE    Y:(R0)+,X0      ; # of waveform entries 
	MOVE    Y:(R0)+,A       ; Start the pipeline
	REP	X0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline

	MOVE    #<INT_SIG,R0	; address of signal integration waveform
	NOP
	MOVE    Y:(R0)+,X0      ; # of waveform entries 
	MOVE    Y:(R0)+,A       ; Start the pipeline
	REP	X0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline
LUSCN
	CLR	A

; Flush out the underscan (if not read) and initial space (XSTART) 
FL_STRT	MOVE	Y:<XSTART,Y0
	ADD	Y0,A		; A = (T_USCAN | 0) + XSTART
	TST	A
	JEQ	LFLUSH3		; skip if no pixels to flush
	DO	A,LFLUSH3
	MOVE    #<XCLOCK,R0	; Address of serial (skip) clocking waveform
	MOVE	(R2)+		; increment pixel counter
	MOVE    Y:(R0)+,X0	; # of waveform entries 
	MOVE    Y:(R0)+,A       ; Start the pipeline
	REP	X0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline
LFLUSH3

; Start the loop for the X subapertures
	DO	Y:<XSUBAP,LXSUB

; Read out one subaperture in the X direction
	DO	Y:<XRAS,LXRAS

	JSR	(R1)		; Retrieve and transmit image data

	MOVE    #<INT_RST,R0	; address of reset integration waveform
	MOVE	(R2)+		; increment pixel counter
	MOVE    Y:(R0)+,X0      ; # of waveform entries 
	MOVE    Y:(R0)+,A       ; Start the pipeline
	REP	X0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline

	MOVE	Y:XBIN1,A
	TST	A
	JEQ	LXBIN		; skip if binning = 1 (XBIN1=0)
	DO	A,LXBIN		; bin pixels together
	MOVE    #<XBINCLK,R0	; address of serial (binning) clocking waveform
	NOP
	MOVE    Y:(R0)+,X0      ; # of waveform entries 
	MOVE    Y:(R0)+,A       ; Start the pipeline
	REP	X0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline
LXBIN

	MOVE    #<INT_SIG,R0	; address of signal integration waveform
	NOP
	MOVE    Y:(R0)+,X0      ; # of waveform entries 
	MOVE    Y:(R0)+,A       ; Start the pipeline
	REP	X0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline

LXRAS	; End of one X subaperture

; Flush out space between subapertures (XSPACE)
	CLR	A
	MOVEC	LC,A0		; check loop counter
	DEC	A
	JEQ	LXSPA		; skip flush if last subaperture
	MOVE	Y:<XSPACE,A	; check for zero length space
	TST	A
	JEQ	LXSPA		; skip flush if no space
	DO	A,LXSPA		; flush inter-subaperture space
	MOVE    #<XCLOCK,R0	; Address of serial (skip) clocking waveform
	MOVE	(R2)+		; increment pixel counter
	MOVE    Y:(R0)+,X0	; # of waveform entries 
	MOVE    Y:(R0)+,A       ; Start the pipeline
	REP	X0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline

LXSPA

	NOP
LXSUB	; End of all X subapertures

; Flush out remaining pixels in serial register (XTAIL)
	MOVE	Y:<XTAIL,A
	TST	A
	JEQ	LXTAIL
	DO	A,LXTAIL
	MOVE    #<XCLOCK,R0	; Address of serial (skip) clocking waveform
	MOVE	(R2)+		; increment pixel counter
	MOVE    Y:(R0)+,X0	; # of waveform entries 
	MOVE    Y:(R0)+,A       ; Start the pipeline
	REP	X0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline

LXTAIL

	NOP
LYRAS	; End of one Y subaperture

; Flush out space between subapertures (YSPACE)
	CLR	A
	MOVEC	LC,A0		; check loop counter
	DEC	A
	JEQ	LYSPA		; skip flush if last subaperture
	MOVE	Y:<YSPACE,A	; check for zero length space
	TST	A
	JEQ	LYSPA		; skip flush if no space
	DO	A,LYSPA		; flush inter-subaperture space
	MOVE    #<YSCLOCK,R0	; Address of parallel (S only) clocking waveform
	MOVE	(R2)+N2		; increment pixel counter
	MOVE    Y:(R0)+,X0	; # of waveform entries 
	MOVE    Y:(R0)+,A	; Start the pipeline
	REP	X0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline
LYSPA

; Flush serial register
	MOVE    #<XDUMP,R0	; Address of serial dump clocking waveform
	NOP			; register access restriction
	MOVE    Y:(R0)+,X0	; # of waveform entries 
	MOVE    Y:(R0)+,A       ; Start the pipeline
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline
LYSUB	; End of all Y subapertures

;	JMP	<LDROWS		; debug (continuous readout)

; Digitize two more pixels to clear ADC pipeline
	DO	#2,LPLFLSH
	JSR	(R1)		; Retrieve and transmit image data
	MOVE    #<INT_SIG,R0	; address of signal integration waveform
	MOVE	(R2)+		; increment pixel counter
	MOVE    Y:(R0)+,X0	; # of waveform entries 
	MOVE    Y:(R0)+,A	; Start the pipeline
	REP	X0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline
LPLFLSH
	JSR	(R1)		; Retrieve and transmit image data

; Finish readout
	IF	!CCDTOOL
	JCLR	#ESYNC,Y:<MODE,XMIT_CS	; Skip if not sync bit mode
	BSET	#FD15,X:PBD		; Set sync bit value to 1
	MOVE	Y:B_0_14,Y0		; Get mask ($7fff)
	AND	Y0,B1			; Mask out b15 to avoid saturation
XMIT_CS	MOVEP	B1,Y:WRFO		; transmit checksum
	ENDIF

END_RD	BCLR	#RDING,Y:<T_STATUS	; clear readout status
	MOVE	Y:PCINIT,R2	; Reset simulated data pixel counter
	BSET	#TIO,X:PBD	; TIO = 1
	BCLR	#TMR_EN,X:TCSR	; disable hardware timer
	CLR	A
	CLR	B Y:<T_EXP_TIM,A0	; get exposure time
	MOVEP	X:TCR,B0	; get elapsed time
	MOVE	B0,Y:<T_RO_TIM	; save as a status value
	REP	#11		; divide by 2048 = 2^11
	ASR	B
	SUB	B,A		; subtract
	JGT	CONT
 	BSET	#E_OVR,Y:<T_ERROR	; flag exposure overrun error
	MOVE	X:<ONE,A0		; set exposure time to one
CONT	MOVE	A0,Y:<T_EXP_TMR		; copy to exposure timer
	JSET	#INF_FRM,Y:T_STATUS,EXPOSE	; if inf. series, do next exp.
	CLR	A
	MOVE	Y:<T_FRAMEC,A0	; get frame counter
	DEC	A
	MOVE	A0,Y:<T_FRAMEC
	JNE	EXPOSE
	MOVE	Y:<T_NFRAME,A
	TST	A			; check if infinite series is requested
	JNE	<RST_FRC		; skip if not
	BSET	#INF_FRM,Y:<T_STATUS	; set bit for infinite series
RST_FRC	MOVE	A,Y:<T_FRAMEC		; reset frame counter
	JCLR	#RDSYNC,Y:<T_STATUS,RDCDON	; if no new RDC, finish
	BCLR	#RDSYNC,Y:<T_STATUS	; Clear RDC sync request
	JMP	EXPOSE
RDCDON	BCLR	#WW,X:PBD	; Clear word width for 32-bit commands
	BCLR	#FMODE,X:PBD	; Disable sync bit
	BCLR	#INF_FRM,Y:<T_STATUS	; Clear infinite series request
	JCLR	#ABT_EXP,Y:<T_STATUS,START	; finished if no ABT request
	BCLR	#ABT_EXP,Y:<T_STATUS	; Clear ABT request
	MOVE	Y:NP_SAV,A		; get saved copy of N_PIXEL
	MOVE	A,Y:<N_PIXEL		; reset N_PIXEL
	JMP	<START		; reset command buffer and return to idling

; Do frame transfer to flush out image area before first exposure
IMG_CLR	DO      Y:<T_YSIZE,LFT1	; transfer image region to storage region
	MOVE    #<YISCLOCK,R0	; Address of parallel (S&I) clocking waveform
	NOP			; register access restriction
	MOVE    Y:(R0)+,X0	; # of waveform entries 
	MOVE    Y:(R0)+,A	; Start the pipeline
	REP	X0		; Repeat X0 times
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline
LFT1

; Flush serial register
	MOVE    #<XDUMP,R0	; Address of serial dump clocking waveform
	NOP			; register access restriction
	MOVE    Y:(R0)+,X0	; # of waveform entries 
	MOVE    Y:(R0)+,A       ; Start the pipeline
	MOVE    A,X:(R6) Y:(R0)+,A	; Send out the waveform
	MOVE    A,X:(R6)        ; Flush out the pipeline

; Setup frame counter and exposure timer
	MOVE	Y:<T_NFRAME,A	; get number of frames
	TST	A			; check if infinite series is requested
	JNE	<SET_FRC		; skip if not
	BSET	#INF_FRM,Y:<T_STATUS	; set bit for infinite series
SET_FRC MOVE	A,Y:<T_FRAMEC	; copy number of frames to frame counter
	MOVE	Y:<T_EXP_TIM,A	; get exposure time
	TST	A		; check if zero
	JGT	<SET_ET		; skip if greater than zero
	MOVE	X:<ONE,A	; minimum exposure timer count is one	
SET_ET	MOVE	A,Y:<T_EXP_TMR	; set exposure timer

; Reset simulated data pixel counter
	MOVE	Y:PCINIT,R2
	
	JMP	<EXPOSE		; begin first exposure



; *****  Process command received during exposure  *****
; This is mostly a copy of the command processing section of the boot code. 
; It has been repeated here so that only commands valid during an exposure
; will be accepted and no commands or replies from the utility board will 
; be processed (except in the CCDtool case).
 
PRC_CMD	JSET	#ST_RCV,X:<STAT,GET_HDR ; check if its a FIFO word

	IF	!CCDTOOL
	JMP	<CMD_ERR

	ELSE	; Must allow utility board RDMs (and responses) for CCDTool
	MOVE	X:(R3),Y0
	MOVE	X:<UTL_REQ,A	; Is it the utility board requesting service?
	CMP	Y0,A
	JNE	<GET_HDR

	JCLR    #SSI_TDE,X:SSISR,*	; Wait for transmitter to be empty
        MOVEP	X:TIM_ACK,X:SSITX	; Write acknowledge to utility board
	JCLR	#SSI_RDF,X:SSISR,*	; Wait for next utility board word
	MOVEP   X:SSIRX,X:(R3)		; Overwrite UTL_REQ word

	ENDIF

GET_HDR	MOVE	X:(R3)+,X0	; Get candidate header
	JSR	<CHK_HDR	; check for valid header
	JCS	<CMD_ERR	; if error, go back to exposure
	MOVE	A,X:<NWORDS	; save NWORDS

; Read all the words of the command before processing it
	MOVE	R4,X0		; Header address = RCV_BUF
	ADD	X0,A		; R3 must reach this for command to be complete
	MOVE	A,X1		; X1 = header address + NWORDS

RD_IN	JCLR	#ST_RCV,X:<STAT,WT_SSI ; check if its a FIFO word
	JSR	<WT_FIFO
	MOVE	(R3)+		; Increment register
	JMP	<ADR_CHK

WT_SSI	JCLR	#SSI_RDF,X:SSISR,*	; wait for next SSI word
	MOVEP	X:SSIRX,X:(R3)+	; read the SSI word into the buffer

ADR_CHK	MOVE	R3,A		; Get address of last word written to buffer
	CMP	X1,A		; Has it been incremented to RCV_BUF + NWORDS?
	JLT	<RD_IN		; No, keep looking for more

;  Process the receiver entry - is its destination number = D_BRD?
CHK_DST	MOVE	X:(R4),X0	; Get header
	MOVE	X:<DMASK,A
	AND	X0,A  X:<DBRD,X1 ; Extract destination byte
	CMP	X1,A		; Does header = destination number? 

; Must allow utility board RDMs (and responses) for CCDTool
	IF	CCDTOOL		
	JEQ 	<LKP_CMD	; Check for valid command string
	JLT	<XMT_FO		; send to FO transmitter

; Transmit words to the utility board over the SSI
        DO      X:<NWORDS,SSI_LP 	; Transmit NWORDS
	JCLR    #SSI_TDE,X:SSISR,*	; Wait until SSI XMT register is empty
        MOVEP	X:(R4)+,X:SSITX		; Write to SSI buffer
SSI_LP	
	JMP	<RET_EXP	; go back to exposure

;  Transmit words to the host computer over the fiber optics link
XMT_FO	DO	X:<NWORDS,DON_FO 	; Transmit all the words in the command
	DO	#40,DLY_FO		; Delay for the serial transmitter
        NOP
DLY_FO
	MOVEP	X:(R4)+,Y:WRFO		; Send each word to the FO transmitter
DON_FO
	JMP	<RET_EXP	; go back to exposure

	ELSE
	JNE	<CMD_ERR	; If not timing board command, ignore
	ENDIF

;  Process the receiver entry - is it a valid comand during exposure ?
LKP_CMD	MOVE	(R4)+		; increment past header
	MOVE    X:(R4)+,A       ; Get the command buffer entry
	MOVE    Y:ABT,X1	; Compare to 'ABT'
	CMP     X1,A
	JEQ	ABORT		; If 'ABT' go to ABORT
	MOVE    Y:RDC,X1	; Compare to 'RDC'
	CMP     X1,A
	JEQ	SYNC		; If 'RDC' go to SYNC
	MOVE    Y:WRP,X1	; Compare to 'WRP'
	CMP     X1,A
	JEQ	WRITEP		; If 'WRP' go to WRITEP
	MOVE    Y:LDP,X1	; Compare to 'LDP'
	CMP     X1,A
	JEQ	LOADP		; If 'LDP' go to LOADP

; Invalid command - Reset FIFO and return to exposure
CMD_ERR	JSR	CHK_ERR
	JMP	<CHK_TMR	; Return to exposure

; Restart R3 and R4 and return
RET_EXP	MOVE	#<RCV_BUF,R3
	MOVE	R3,R4
	JMP	<CHK_TMR	; Return to exposure


; *****  Abort readout  *****
; An abort is accomplished by forcing the exposure timer and frame counter
; to a value of one. This will halt the exposure on the next timer tick and
; force the readout to finish when the current frame is complete.

ABORT	MOVE	X:<ONE,A
	MOVE	A,Y:T_EXP_TMR	; Force exposure timer to end
	MOVE	A,Y:T_FRAMEC	; Force frame count to end
	MOVE	Y:<N_PIXEL,A	; get number of pixels
	MOVE	A,Y:NP_SAV	; save number of pixels
	CLR	A
	MOVE	A,Y:<N_PIXEL		; set number of pixels to zero
	BCLR	#INF_FRM,Y:<T_STATUS	; clear request for infinite series 
	BSET	#ABT_EXP,Y:<T_STATUS	; set flag that abort command received 
	JMP	<RET_EXP		; Finish readout


; *****  Synchronize readouts  *****
; If an RDC is received before the previous one has finished, the current 
; frame set is aborted and a new one started immediately.

SYNC	BSET	#RDSYNC,Y:<T_STATUS	; Set flag to request RDC sync
	MOVE	X:<ONE,A
	MOVE	A,Y:T_EXP_TMR	; Force exposure timer to end
	MOVE	A,Y:T_FRAMEC	; Force frame count to end
	BCLR	#INF_FRM,Y:<T_STATUS	; clear request for infinite series 
	JMP	<RET_EXP	; Finish readout


; *****  Write parameter (on-the-fly changes)  *****
; The WRP command is essentially the same as the WRM command, except that it
; only allows writing to the parameter buffer memory locations, and does
; not send a reply so that it does not cause problems during readout. If
; an invalid address is specified, an error flag is set.
 
WRITEP	MOVE    X:(R4),R0	; get the desired address
	MOVE	X:(R4)+,A	; we need a 24-bit version of the address
	MOVE    X:(R4)+,X0	; get datum into X0 so MOVE works easily
	JCLR    #22,A,FWRPERR	; Error: not Y: memory
	JCLR	#EXPING,Y:<T_STATUS,DO_WRP	; Skip addr. check if not exping
	MOVE	R0,A		; get 16 bit address
	MOVE	#<P_BUF,X1	; get address of parameter buffer
	SUB	X1,A Y:<P_BUF,X1
	JLE	<FWRPERR	; Error: address is <= P_BUF
	SUB	X1,A
	JGT	<FWRPERR	; Error: address >= ENDP_BUF
DO_WRP	MOVE    X0,Y:(R0)	; write to Y: memory
WRPRET	JSET	#EXPING,Y:<T_STATUS,RET_EXP	; return to exposure
	JMP	<START		; reset command buffer and return to idling

FWRPERR	BSET	#WRP_ERR,Y:<T_ERROR	; flag error
	JMP	<WRPRET


; *****  Load new parameter set from table  *****
; The LDP command copies the parameters from the input buffer to the working 
; parameter table and puts them into effect. Like WRP, it does not send a reply
; so that it can execute during a readout. The gain and integrator speed 
; parameters are only changed if the command is executed when the board is not
; in readout mode.

LOADP	MOVE    #<P_BUF,R0	; address of parameter table
	MOVE    #<PARMID,R7	; address of current parameter set
	MOVE    Y:(R0)+,X0	; # of parameter table entries 
	DO      X0,LDPLP	; Repeat X0 times
	MOVE    Y:(R0)+,A	; Get new parameter
	MOVE    A,Y:(R7)+	; Write new parameter
LDPLP

; Calculate decremented X binning factor
XB_CALC	CLR	A
	MOVE	Y:<XBIN,A0	; get X binning factor
	DEC	A
	MOVE	A0,Y:XBIN1	; store decremented value

; Determine address of data transmission subroutine
	MOVE	#X_SIM,A
	JSET	#SIMD,Y:<MODE,SETSXMT	; use simulated data
	MOVE	#X_NORM,A		; use raw data
SETSXMT	MOVE	A,Y:SXMIT

; Set mask for transmitted data depending on sync mode
	MOVE	Y:B_0_14,Y1	; get bit mask ($007FFF)
	JSET	#ESYNC,Y:<MODE,SETXMSK
	BSET	#15,Y1		; set bit 15 of mask ($00FFFF)
SETXMSK	MOVE	Y1,Y:XMT_MSK

; Set CDS integration periods
	MOVE	Y:DLYMASK,Y1	; mask for clearing integration time
	MOVE	Y:R_INT,A	; reset integration waveform entry
	AND	Y1,A1 Y:<INT_TIM,Y0	; mask out old integration time
	OR	Y0,A1		; insert new integration time
	MOVE	A1,Y:R_INT	; update waveform entry
	MOVE	Y:S_INT,A	; signal integration waveform entry
	AND	Y1,A1		; mask out old integration time
	OR	Y0,A1		; insert new integration time
	MOVE	A1,Y:S_INT	; update waveform entry

	JSET	#EXPING,Y:<T_STATUS,RET_EXP	; return to exposure

; Load remaining parameters 

; Set gain and integrator speed
	JSR	<SER_ANA	; Set SSI to analog board communication
	MOVE	Y:GSDAC1,A	; address of DAC for gain and speed, board #1
	MOVE	Y:<T_GAIN_SP,Y1	; gain/speed value parameter
	OR	Y1,A1
	MOVEP	A1,X:SSITX	; send 
	JSR	<PAL_DLY	; delay for transmit

; Set A/D input offsets
	MOVE	#ADC_OS0,R0	; address of first DAC for A/D offset
	MOVE	#T_ADC_OS0,R7	; first A/D offset value parameter
	DO	Y:<T_OUTPUTS,L_ADCOS
	MOVE	Y:(R0)+,A1
	MOVE	Y:(R7)+,Y1
	OR	Y1,A1
	MOVEP	A1,X:SSITX	; send 
	JSR	<PAL_DLY	; delay for transmit
	NOP			; DO loop restriction
L_ADCOS

	JSR	<SER_UTL	; Return SSI to utility board communication

	JMP	<START		; reset command buffer and return to idling


; *****  Initialize  *****
; Set control voltages and configuration of video and clock boards

INIT	BSET	#IDLM,X:<STAT	; force into idling mode
	CLR	A
 	MOVE	A,Y:<T_ERROR	; clear error word
	JSR	<SER_ANA	; Set SSI to analog board communication
	BSET	#CDAC,X:<LATCH		; Disable clearing of DACs
	BCLR	#DUALCLK,X:<LATCH	; Don't clk 2 halves of clk bd together
	MOVEP	X:LATCH,Y:WRLATCH 	; Write to latch
	JSR	<PAL_DLY	; Delay for all this to happen
	JSR	<PAL_DLY	; Delay for all this to happen

; Read DAC values from a table, and set DACs
	MOVE	#DACS,R0	; Get starting address of DAC values
	NOP			; Register data access restriction
	MOVE	Y:(R0)+,X0	; Get the number of table entries
        DO      X0,SET_L0	; Repeat X0 times
        MOVEP	Y:(R0)+,X:SSITX	; Send out the waveform
	JSR	<PAL_DLY	; Wait for SSI and PAL to be empty
	NOP			; Do loop restriction
SET_L0

; Set all video processor analog switches to initial values (1 => OFF)
	MOVE	#VIDSS+$000000+%0010100,A 
	MOVE    A,X:(R6)	; Send out the waveform
	NOP

; Let the DAC voltages all ramp up before exiting
	MOVE	#400,A		; Delay 4 millisec
	DO	A,L_SBV1
	JSR	<PAL_DLY 	; Delay for all this to happen
	NOP
L_SBV1

; Enable clock and DAC output switches 
	BSET	#ENCK,X:<LATCH		
	MOVEP	X:LATCH,Y:WRLATCH 	; Write to latch
	JSR	<PAL_DLY	; Delay for all this to happen
	JSR	<PAL_DLY	; Delay for all this to happen

	JSR	<SER_UTL	; Return SSI to utility board communication

; Setup pixel counter row increment
	MOVE	Y:<T_XSIZE,N2

; Disable sync bit
	BCLR	#FMODE,X:PBD	

	JMP	<FINISH		; send DON


	IF	CCDTOOL

; *****  Set software to IDLE mode  *****
; Causes the timing board to continuously clock the CCD while waiting for
; commands (the usual state when not reading out).

IDL	BSET    #IDLM,X:<STAT
	JMP     <FINISH	


; *****  Take software out of IDLE mode  *****
; Stops the timing board from clocking the CCD while waiting for commands.
; It is important to do so before downloading new timing board application code
; so that the board does not attempt to execute half-loaded code.

STP     BCLR    #IDLM,X:<STAT
	JMP     <FINISH

	ENDIF


;  *************************    Subroutines    ***************************

; Enable serial communication to the analog boards
SER_ANA	BSET	#SSI_CON,X:PBD	; Set H0 for analog boards SSI
	MOVEP	#$0000,X:PCC	; Software reset of SSI
	BCLR	#GCK,X:CRB	; Change SSI to continuous clock for analog 
	MOVEP   #$0160,X:PCC	; Re-enable the SSI
	RTS


; Enable serial communication to the utility board
SER_UTL	MOVEP	#$0000,X:PCC	; Software reset of SSI
	BSET	#GCK,X:CRB	; Change SSI to gated clock for utility board 
	MOVEP   #$0160,X:PCC	; Enable the SSI
	BCLR	#SSI_CON,X:PBD	; Clear H0 for utility board SSI
	RTS


; Delay for serial writes to the PALs and DACs by 8 microsec
PAL_DLY	DO	#250,DLY	 ; Wait 8 usec for serial data transmission
	NOP
	NOP
DLY	NOP
	RTS


; Write simulated data to serial transmitter
X_SIM	MOVE	Y:B_0_11,Y1	; get bit mask ($000FFF)
	MOVE	R2,A		; get simulated pixel value
	AND	Y1,A1 Y:SIMDATA0,Y0	; mask off lowest 12 bits
	OR	Y0,A1		; add output header (output 0)
	MOVEP	A1,Y:WRFO	; transmit value
	ADD	A,B		; add to checksum

	AND	Y1,A1 Y:SIMDATA1,Y0	; mask off lowest 12 bits
	OR	Y0,A1		; add output header (output 1)
	MOVEP	A1,Y:WRFO	; transmit value
	ADD	A,B		; add to checksum

	RTS


;  **********************    End of application    ************************

; Check for program overflow
        IF	@CVS(N,*)>=$20000
        WARN    'Application P: program is too large!'
	ENDIF


; ******************************   X Data   *******************************

;  Command table
	IF	DOWNLOAD 	; Memory offsets for downloading code
	ORG	X:COM_TBL,X:COM_TBL
	ELSE			; Memory offsets for generating EEPROMs
        ORG     P:COM_TBL,P:(2*APL_NUM-1)*$100+APL_LEN
	ENDIF

	DC	'RDC',RDCCD 	; Begin CCD readout    
	DC	'INI',INIT 	; Initialize
  	DC	'DON',START	; Ignore
	DC	'LDP',LOADP	; Load new parameter set
	DC	'WRP',WRITEP	; Write parameter

	DC	'CLR',FINISH	; Do nothing (timboot compatibility)

	IF	CCDTOOL
	DC	'IDL',IDL	; Set to IDLE mode (timboot/CCDtool compat.)
	DC	'STP',STP	; Unset from IDLE mode (CCDtool compatibility)
	DC	'SBV',INIT	; Initialize (CCDtool/utilappl compatibility)
	ELSE
	DC	'IDL',FINISH	; Do nothing (timboot compatibility)
	ENDIF



; *****************   Y Data (Defined in ICD 1.6/1.10)   ******************

	IF	DOWNLOAD
	ORG	Y:0,Y:0		; Download address
	ELSE
	ORG     Y:0,P:		; EEPROM address continues from P: above
	ENDIF

	IF	CCDTOOL
DUM0	DC	0		; Not used (for compatibility with CCDtool)
DUM1	DC      0		; Not used (for compatibility with CCDtool)
DUM2	DC      0		; Not used (for compatibility with CCDtool)
	ENDIF


; ***** Status values *****

T_SW_ID		DC	$012204	; Software version 01.22 (CCD47, full-feature)

T_STATUS	DC	0	; Status word
; Bit definitions
IDLING  	EQU     0	; Set if idling
EXPING		EQU	1	; Set if exposing
RDING		EQU	2	; Set if reading out
RDSYNC		EQU	3	; Set if RDC received during exposure
INF_FRM		EQU	4	; Set if infinite series of frames is requested
ABT_EXP		EQU	5	; Set if abort command received

T_ERROR		DC	0	; Error word
; Bit definitions
E_OVR		EQU	0	; overrun error (exposure time < readout time)
WRP_ERR		EQU	1	; WRP error - invalid address specified

T_USCAN		DC	8	; Number of underscan pixels
T_XSIZE		DC	536	; Total number of pixels per row per output
T_YSIZE		DC	1032	; Total number of pixels per column per output
T_OUTPUTS	DC	2	; Number of outputs

T_RO_TIM	DC	0	; Measured readout time (units of 40ns)
T_EXP_TMR	DC	12207	; Exposure timer (units of 81.92us)
T_FRAMEC	DC	1	; Frame counter


; ***** Readout parameters *****

; Parameters not changeable on the fly
T_EXP_TIM	DC	1221	; Exposure time (units of 81.92us)
T_NFRAME	DC	1	; Number of frames to readout
T_GAIN_SP	DC	$FEE	; Amplifier gain / integrator speed

T_ADC_OS0	DC	$A00	; A/D input offset voltage, ch0 
T_ADC_OS1	DC	$A00	; A/D input offset voltage, ch1 


; Parameters changeable on-the-fly
; The following locations are an input buffer for parameters which can be
; changed on-the-fly with WRP. Seperate copies of these values are kept
; internally.  

P_BUF		DC	ENDP_BUF-P_BUF-1	; Length of input buffer
						; (not a parameter)

T_PARMID	DC	$47	; Parameter set identifier

T_MODE		DC	0	; Readout mode
; Bit definitions
OUSCN		EQU	0	; Output underscan pixels
SIMD		EQU	1	; Simulate data (data value = pixel #)
ESYNC		EQU	2	; Enable sync mode

T_SAMPLES	DC	0	; Not used for CCD's

T_XSUBAP	DC	1	; Number of subapertures in X direction
T_YSUBAP	DC	1	; Number of subapertures in Y direction
T_XSTART	DC	16	; Offset to first column of pixels to digitize
T_YSTART	DC	1	; Offset to first row of pixels to digitize
T_XRAS		DC	512	; Number of X super-pixels per subaperture
T_YRAS		DC	1024	; Number of Y super-pixels per subaperture
T_XSPACE	DC	0	; Number of X pixels to skip between subaps
T_YSPACE	DC	0	; Number of Y pixels to skip between subaps
T_XBIN		DC	1	; Number of X pixels per super-pixel 
T_YBIN		DC	1	; Number of Y pixels per super-pixel
T_XTAIL		DC	0	; Remaining pixels per row per output
T_NPIXEL	DC	1048576	; Total number of pixels

T_INT_TIM	DC	$470000	; CDI integration time = 1.5us

ENDP_BUF


; *************************   Y Data (Internal)   *************************

; *****  Readout parameters  ***** 
; These are the in-use values of the on-the-fly parameters and should not be 
; changed directly with WRM. Instead, write the new values to the parameter 
; input buffer using WRM or WRP, and then send an LDP.

PARMID		DC	0	; Parameter set identifier

MODE		DC	0	; Readout mode
SAMPLES		DC	0	; Not used for CCD's

XSUBAP		DC	1	; Number of subapertures in X direction
YSUBAP		DC	1	; Number of subapertures in Y direction
XSTART		DC	16	; Offset to first column of pixels to digitize
YSTART		DC	1	; Offset to first row of pixels to digitize
XRAS		DC	512	; Number of X super-pixels per subaperture
YRAS		DC	1024	; Number of Y super-pixels per subaperture
XSPACE		DC	0	; Number of X pixels to skip between subaps
YSPACE		DC	0	; Number of Y pixels to skip between subaps
XBIN		DC	1	; Number of X pixels per super-pixel 
YBIN		DC	1	; Number of Y pixels per super-pixel
XTAIL		DC	0	; Remaining pixels per row per output
N_PIXEL		DC	1048576	; Total number of pixels

INT_TIM		DC	$2E0000	; CDI integration time = 1us


; ***** Clock waveforms *****
; For optimum speed, the waveform tables should be located below Y:$FF

; Define switch state bits for the CCD clocks of the WFS CCD
; Lower 12 clocks
RG	EQU	$1	; Reset output node - right/left
SR1R	EQU	$2	; Serial shift register, phase #1 right
SR2R	EQU	$4	; Serial shift register, phase #2 right
SR1L	EQU	$8	; Serial shift register, phase #1 left
SR2L	EQU	$10	; Serial shift register, phase #2 left
SR3	EQU	$20	; Serial shift register, phase #3
I1	EQU	$40	; Image area, phase #1
I2	EQU	$80	; Image area, phase #2
I3	EQU	$100	; Image area, phase #3
S1	EQU	$200	; Storage area, phase #1
S2	EQU	$400	; Storage area, phase #2
S3	EQU	$800	; Storage area, phase #3
; Upper 12 clocks
DG	EQU	$1	; Dump gate

; Define clock timing
P_DELAY	EQU	$600000	; Parallel delay (96x20+80=2000ns)
S_DELAY	EQU	$010000	; Serial delay (1x20+80=100ns)

	IF IMOCLK
; IMO parallel (storage and image) clocking: 0-1-2-3-0
YISCLOCK DC	YSCLOCK-YISCLOCK-2
	DC	CLKA+P_DELAY+I1+00+00+S1+00+00+0000+SR2R+SR1L+0000+000+RG
	DC	CLKA+P_DELAY+I1+I2+00+S1+S2+00+0000+SR2R+SR1L+0000+000+RG
	DC	CLKA+P_DELAY+00+I2+00+00+S2+00+0000+SR2R+SR1L+0000+000+RG
	DC	CLKA+P_DELAY+00+I2+I3+00+S2+S3+0000+SR2R+SR1L+0000+000+RG
	DC	CLKA+P_DELAY+00+00+I3+00+00+S3+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+P_DELAY+00+00+00+00+00+00+SR1R+SR2R+SR1L+SR2L+000+RG

; IMO parallel (storage only) clocking: 0-1-2-3-0
YSCLOCK DC	XCLOCK-YSCLOCK-2
	DC	CLKA+P_DELAY+00+00+00+S1+00+00+0000+SR2R+SR1L+0000+000+RG
	DC	CLKA+P_DELAY+00+00+00+S1+S2+00+0000+SR2R+SR1L+0000+000+RG
	DC	CLKA+P_DELAY+00+00+00+00+S2+00+0000+SR2R+SR1L+0000+000+RG
	DC	CLKA+P_DELAY+00+00+00+00+S2+S3+0000+SR2R+SR1L+0000+000+RG
	DC	CLKA+P_DELAY+00+00+00+00+00+S3+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+P_DELAY+00+00+00+00+00+00+SR1R+SR2R+SR1L+SR2L+000+RG

	ELSE
; non-IMO parallel (storage and image) clocking: 1-2-3-1
YISCLOCK DC	YSCLOCK-YISCLOCK-2
	DC	CLKA+P_DELAY+I1+I2+00+S1+S2+00+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+P_DELAY+00+I2+00+00+S2+00+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+P_DELAY+00+I2+I3+00+S2+S3+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+P_DELAY+00+00+I3+00+00+S3+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+P_DELAY+I1+00+I3+S1+00+S3+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+P_DELAY+I1+00+00+S1+00+00+SR1R+SR2R+SR1L+SR2L+000+RG

; non-IMO parallel (storage only) clocking: 1-2-3-1
YSCLOCK DC	XCLOCK-YSCLOCK-2
	DC	CLKA+P_DELAY+I1+00+00+S1+S2+00+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+P_DELAY+I1+00+00+00+S2+00+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+P_DELAY+I1+00+00+00+S2+S3+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+P_DELAY+I1+00+00+00+00+S3+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+P_DELAY+I1+00+00+S1+00+S3+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+P_DELAY+I1+00+00+S1+00+00+SR1R+SR2R+SR1L+SR2L+000+RG

	ENDIF

	IF IMOCLK
; IMO serial clocking and charge dumping: R: 1-2-3-1, L: 2-1-3-2 
XCLOCK	DC	XDUMP-XCLOCK-2
	DC	CLKA+S_DELAY+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+S_DELAY+0000+SR2R+SR1L+0000+000
	DC	CLKA+S_DELAY+0000+SR2R+SR1L+0000+SR3
	DC	CLKA+S_DELAY+0000+0000+0000+0000+SR3
	DC	CLKA+S_DELAY+SR1R+0000+0000+SR2L+SR3
	DC	CLKA+0000000+SR1R+0000+0000+SR2L+000

	ELSE
; nonIMO serial clocking and charge dumping: R: 1-2-3-1, L: 2-1-3-2 
XCLOCK	DC	XDUMP-XCLOCK-2
	DC	CLKA+S_DELAY+I1+S1+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+S_DELAY+I1+S1+0000+SR2R+SR1L+0000+000
	DC	CLKA+S_DELAY+I1+S1+0000+SR2R+SR1L+0000+SR3
	DC	CLKA+S_DELAY+I1+S1+0000+0000+0000+0000+SR3
	DC	CLKA+S_DELAY+I1+S1+SR1R+0000+0000+SR2L+SR3
	DC	CLKA+0000000+I1+S1+SR1R+0000+0000+SR2L+000

	ENDIF

; Serial register dump using dump gate
XDUMP	DC	END_WAVE1-XDUMP-2
	DC	CLKB+P_DELAY+DG
	DC	CLKB+0000000+00

END_WAVE1

; Define switch state bits for the video boards
;	%1000000	latch ADC output on leading edge
;	%0100000	start conversion on leading edge
;	%0010000	integrate when low
;	%0001000	polarity = pos when low
;	%0000100	polarity = neg when low
;	%0000010	DC restore when low
;	%0000001	reset integrator when low

	IF IMOCLK
; IMO serial waveforms
; Integrate the reset level
INT_RST	DC	XBIN1-INT_RST-2
	DC	CLKA+S_DELAY+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+S_DELAY+0000+SR2R+SR1L+0000+000
	DC	CLKA+S_DELAY+0000+SR2R+SR1L+0000+SR3
	DC	CLKA+S_DELAY+0000+0000+0000+0000+SR3
	DC	CLKA+0000000+SR1R+0000+0000+SR2L+SR3
	DC	VIDSS+$000000+%0010111  ; remove integ. reset and dc-restore
R_INT	DC	VIDSS+$2E0000+%0000111  ; Integrate reset level for t_int
	DC      VIDSS+$000000+%0011011  ; Stop Integrate
	DC	CLKA+0000000+SR1R+0000+0000+SR2L+000 ; transfer charge to output

; Serial binning clocking (no reset/charge dumping): 1-2-3-1
XBIN1	DC	0	; binning factor minus one
XBINCLK	DC	INT_SIG-XBINCLK-2
	DC	CLKA+S_DELAY+SR1R+SR2R+SR1L+SR2L+000
	DC	CLKA+S_DELAY+0000+SR2R+SR1L+0000+000
	DC	CLKA+S_DELAY+0000+SR2R+SR1L+0000+SR3
	DC	CLKA+S_DELAY+0000+0000+0000+0000+SR3
	DC	CLKA+0000000+SR1R+0000+0000+SR2L+SR3
	DC	CLKA+0000000+SR1R+0000+0000+SR2L+000 ; transfer charge to output

	ELSE
; non-IMO serial waveforms
; Integrate the reset level
INT_RST	DC	XBIN1-INT_RST-2
	DC	CLKA+S_DELAY+I1+S1+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+S_DELAY+I1+S1+0000+SR2R+SR1L+0000+000
	DC	CLKA+S_DELAY+I1+S1+0000+SR2R+SR1L+0000+SR3
	DC	CLKA+S_DELAY+I1+S1+0000+0000+0000+0000+SR3
	DC	CLKA+0000000+I1+S1+SR1R+0000+0000+SR2L+SR3
	DC	VIDSS+$000000+%0010111  ; remove integ. reset and dc-restore
R_INT	DC	VIDSS+$2E0000+%0000111  ; Integrate reset level for t_int
	DC      VIDSS+$000000+%0011011  ; Stop Integrate
	DC	CLKA+0000000+I1+S1+SR1R+0000+0000+SR2L+000 ; xfer to output

; Serial binning clocking (no reset/charge dumping): 1-2-3-1
XBIN1	DC	0	; binning factor minus one
XBINCLK	DC	INT_SIG-XBINCLK-2
	DC	CLKA+S_DELAY+I1+S1+SR1R+SR2R+SR1L+SR2L+000+RG
	DC	CLKA+S_DELAY+I1+S1+0000+SR2R+SR1L+0000+000
	DC	CLKA+S_DELAY+I1+S1+0000+SR2R+SR1L+0000+SR3
	DC	CLKA+S_DELAY+I1+S1+0000+0000+0000+0000+SR3
	DC	CLKA+0000000+I1+S1+SR1R+0000+0000+SR2L+SR3
	DC	CLKA+0000000+I1+S1+SR1R+0000+0000+SR2L+000 ; xfer to output

	ENDIF

; Integrate the signal level
INT_SIG	DC	END_WAVE-INT_SIG-2
S_INT	DC	VIDSS+$2E0000+%0001011  ; Integrate video level for t_int
	DC	VIDSS+$000000+%0011011  ; Stop integrate, A/D is sampling
	DC	VIDSS+$000000+%1110100  ; start conversion n and transfer n-2

END_WAVE


; Check for Y: data memory overflow
        IF	@CVS(N,*)>$FF
        WARN    'Waveform tables go beyond Y:$FF (end of internal memory)'
	ENDIF						  


; *****  DAC settings  *****

; CCD clock voltage definitions
RS_HI	EQU	$B48	; Reset and Serial clocks High (+4.0 V)
RS_LO	EQU	$160	; Reset and Serial clocks Low  (-8.0 V)
SI_HI	EQU	$B48	; Storage and Image High (+4.0 V)
SI_LO	EQU	$160	; Storage and Image Low  (-8.0 V)
SI2_HI	EQU	$CBE	; Implanted phase (S2,I2) High (+6.0 V)

;  CCD DC bias voltages
VOD	EQU	$D0C03	; 24.0 V, pin #1
VRD	EQU	$D4405	; 12.0 V, pin #2
VABD	EQU	$D8FFF	; 20.0 V, pin #3
VOG	EQU	$F0000	; -5.0 V, pin #9
VABG	EQU	$F4000	; -5.0 V, pin #10

; Input offset voltage for DC coupling
INP_OS	EQU	$800	; 24.0V

; Initialization of clock driver DACs
DACS	DC	END_DACS-DACS-1
	DC	(CLK<<8)+(0<<14)+RS_HI		; RG High
	DC	(CLK<<8)+(1<<14)+RS_LO		; RG Low
	DC	(CLK<<8)+(2<<14)+RS_HI		; SR1R High
	DC	(CLK<<8)+(3<<14)+RS_LO		; SR1R Low
	DC	(CLK<<8)+(4<<14)+RS_HI		; SR2R High
	DC	(CLK<<8)+(5<<14)+RS_LO		; SR2R Low
	DC	(CLK<<8)+(6<<14)+RS_HI		; SR1L High
	DC	(CLK<<8)+(7<<14)+RS_LO		; SR1L Low
	DC	(CLK<<8)+(8<<14)+RS_HI		; SR2L High
	DC	(CLK<<8)+(9<<14)+RS_LO		; SR2L Low
	DC	(CLK<<8)+(10<<14)+RS_HI		; SR3 High
	DC	(CLK<<8)+(11<<14)+RS_LO		; SR3 Low
	DC	(CLK<<8)+(12<<14)+SI_HI		; I1 High
	DC	(CLK<<8)+(13<<14)+SI_LO		; I1 Low
	DC	(CLK<<8)+(14<<14)+SI2_HI	; I2 High
	DC	(CLK<<8)+(15<<14)+SI_LO		; I2 Low
	DC	(CLK<<8)+(16<<14)+SI_HI		; I3 High
	DC	(CLK<<8)+(17<<14)+SI_LO		; I3 Low
	DC	(CLK<<8)+(18<<14)+SI_HI		; S1 High
	DC	(CLK<<8)+(19<<14)+SI_LO		; S1 Low
	DC	(CLK<<8)+(20<<14)+SI2_HI	; S2 High
	DC	(CLK<<8)+(21<<14)+SI_LO		; S2 Low
	DC	(CLK<<8)+(22<<14)+SI_HI		; S3 High
	DC	(CLK<<8)+(23<<14)+SI_LO		; S3 Low
	DC	(CLK<<8)+(24<<14)+SI_HI		; DG High
	DC	(CLK<<8)+(25<<14)+SI_LO		; DG Low


; Initialization of video processor DACs

; Input offset voltages for DC coupling.
	DC	(VID1<<8)+$C0000+INP_OS	; channel A, board #1
	DC	(VID1<<8)+$C8000+INP_OS	; channel B, board #1

; CCD DC bias voltages
	DC	(VID1<<8)+VOD		; board #1
	DC	(VID1<<8)+VRD		; board #1
	DC	(VID1<<8)+VABD		; board #1
	DC	(VID1<<8)+VOG		; board #1
	DC	(VID1<<8)+VABG		; board #1

END_DACS

; DAC addresses for gain and integrator speed.
GSDAC1	DC	(VID1<<8)+$C3000	; board #1

; DAC addresses for A/D input offset voltages
ADC_OS0	DC	(VID1<<8)+$C4000	; channel A, board #1 (ch0)
ADC_OS1	DC	(VID1<<8)+$CC000	; channel B, board #1 (ch1)


; *****  Miscellaneous internal data  *****

; Simulated data output headers
SIMDATA0	DC	$000000
SIMDATA1	DC	$001000
SIMDATA2	DC	$002000
SIMDATA3	DC	$003000

; Delay mask (used to set integration times in high-speed serial code)
DLYMASK	DC	$00FFFF	; Mask for clearing the delay portion of an entry

; Masks for clearing b15 in sync mode
B_0_14	DC	$007FFF	; Mask off bits 0 - 14
XMT_MSK	DC	$007FFF	; mask used on transmitted data

; Mask for clearing MS nibble of simulated data 
B_0_11	DC	$000FFF	; Mask off bits 0 - 11

; Image data transmission subroutine address
SXMIT	DC	X_NORM

; Commands valid during exposure
ABT	DC	'ABT'
RDC	DC	'RDC'
WRP	DC	'WRP'
LDP	DC	'LDP'

; Temporary store of N_PIXEL
NP_SAV	DC	1048576

; Constants
PCINIT	DC	1-3	; Initial value for pixel counter 
			; (offset by three for garbage pixels)

; Check for Y: data memory overflow
        IF	@CVS(N,*)>$20000
        WARN    'Application Y: data memory is too large!' 
	ENDIF						  

; Check for overflow in the EEPROM case
	IF !DOWNLOAD
		IF	@CVS(N,@LCV(L))>(2*APL_NUM+1)*$100
	WARN    'EEPROM overflow!'	; Make sure next application
		ENDIF			;  will not be overwritten
	ENDIF

	ENDSEC		; End of section TIM


;  End of program
	END
