       COMMENT *
Gemini WFS Utility Board Code
Controller: SDSU2
Revision: 1.05   (must agree with U_SW_ID in Y: memory table)
(original by Dr. Bob Leach at SDSU)

97/09/18 TDH - modified temperature control algorithm for TEC and 
               thermistor
             - modified Y: memory database to match WFS ICD v1.6
98/01/22 TDH - moved the RDC command to before the exposure and eliminated 
               the CLR command at the start of an exposure.
98/01/27 TDH - removed the IDL command to the timing board at the start of
               the power on sequence.
98/02/21 TDH - added conditional assembler directives for the code that is
               only for CCDtool compatibility and removed some extraneous
               command definitions.
98/07/01 TDH - added comments and changed Y: parameter table to match new
               WFS ICD
             - changed SBV command to timing board to INI


Assembler directives:

-d DOWNLOAD 1	To generate code for downloading to DSP memory.
-d DOWNLOAD 0	To generate code for writing to the EEPROM.

-d CCDTOOL  1	To generate code for use with CCDTool application
-d CCDTOOL  0	To generate code for use with Gemini control software

	*

; Define listing page width
	PAGE    132	; Printronix page width - 132 columns

; Name it a section so it doesn't conflict with other application programs
	SECTION	UTILAPPL

; Include definitions from header file
	INCLUDE 'utilhead.asm'

APL_NUM	EQU	1	; Application number from 1 to 10

;**************************************************************************
;                                                                         *
;    Register assignments  						  *
;	 R1 - Address of SCI receiver contents				  *
;	 R2 - Address of processed SCI receiver contents		  *
;        R3 - Pointer to current top of command buffer                    *
;        R4 - Pointer to processed contents of command buffer		  *
;	 N4 - Address for internal jumps after receiving 'DON' replies	  *
;        R0, R5, R6, A, X0, X1 - For use by program only                  *
;	 R7 - For use by SCI ISR only					  *
;        Y0, Y1, and B - For use by timer ISR only. If any of these	  *
;		registers are needed elsewhere they must be saved and	  *
;	        restored in the TIMER ISR.                        	  *
;**************************************************************************

; Specify execution and load addresses.
	ORG	P:APL_ADR,P:APL_ADR

; The TIMER addresses must be defined here and SERVICE must follow to match
;   up with the utilboot code
	JMP	<SERVICE		; Millisecond timer interrupt


; *****  Timer interrupt service routine  *****
; This routine is connected to a millisecond timer interrupt
; In the CCDTool case, this routine keeps track of the exposure time and
; opens and closes the shutter. In the non-CCDtool case, it simply sets
; a flag to say that the SERVICE routine needs executing.

TIMER	RTI				; RTI for now so downloading works
	NOP				; need to leave room for long address

	IF	CCDTOOL

	JCLR    #ST_EX,X:STATUS,NO_TIM	; Continue on if we're not exposing
	JCLR	#STRT_EX,X:<STATUS,EX_STRT ; Skip if exposure has been started

; Initialize timer at start of exposure
	BCLR	#STRT_EX,X:<STATUS	; Clear status = "not start of exposure"
	CLR     B
	MOVE    B,Y:<U_EL_TIM		; Initialize elapsed time
	MOVE	B,Y:EL_TIM_FRACTION
	JCLR	#OPT_SH,X:<OPTIONS,NO_TIM ; Don't open shutter if a dark frame
	JSR	<OSHUT 			; Open shutter if start of exposure
	JMP	<NO_TIM			; Don't increment EL_TIM at first

; Increment and check elapsed time
EX_STRT	CLR	B Y:INCR,Y0		; INCR = 0.8 seconds
	MOVE	X:<ZERO,Y1
	MOVE	Y:<U_EL_TIM,B1 		; Get elapsed time
	MOVE	Y:EL_TIM_FRACTION,B0
	ADD	Y,B   Y:<U_EXP_TIM,Y1	; EL_TIM = EL_TIM + 0.8 milliseconds	
	MOVE	B0,Y:EL_TIM_FRACTION
	SUB     Y1,B  B1,Y:<U_EL_TIM
	JLT     <NO_TIM			; If (EL .GE. TGT) we've timed out

; Close the shutter at once if needed
	JCLR    #OPT_SH,X:OPTIONS,RD_REQ ; Close the shutter only if needed
	BSET    #SHUTTER,X:PBD		; Set Port B bit #3 to close shutter
	BSET	#ST_SH,X:<STATUS	; Set status to mean shutter closed

; Wait SH_DLY milliseconds for the shutter to fully close before reading out
	MOVE	Y:SH_DLY,Y1		; Get shutter closing time
	SUB	Y1,B			; B = EL_TIM - (U_EXP_TIM + SH_DLY)
	JLT	<NO_TIM			; If (EL .GE. TGT+DEL) we've timed out

; Request a read out
RD_REQ	BSET    #ST_READ,X:<STATUS	; Set so a readout will be initiated

	ENDIF

; Return from interrupt
NO_TIM	BSET    #ST_SRVC,X:<STATUS	; SERVICE needs executing
	MOVEC	Y:SV_SR,SR		; Restore Status Register
	NOP
	RTI				; Return from TIMER interrupt


; *****  Service routine  *****
; This long subroutine is executed every millisecond, but isn't an ISR so
; that care need not be taken to preserve registers and stacks. It reads
; and updates the digital inputs/outputs, the 4 DACs and the 16 A/Ds.
; It also contains the temperature control algortihm.

SERVICE	BCLR	#ST_SRVC,X:<STATUS	; Clear request to execute SERVICE

	IF	CCDTOOL
	JCLR	#ST_READ,X:<STATUS,UPD_DIG ; Initiate readout?
	BCLR	#ST_EX,X:<STATUS	; Exposure is no longer in progress
	BCLR	#ST_READ,X:<STATUS	; Readout will be initiated
	RTS				; Return now to save time
	ENDIF

; Update all the digital input/outputs; reset watchdog timer
UPD_DIG	MOVEP   Y:RD_DIG,Y:U_DIG_IN  ; Read 16 digital inputs
	MOVEP	#1,Y:EN_DIG	   ; Enable digital outputs
	MOVEP   Y:U_DIG_OUT,Y:WR_DIG ; Write 16 digital outputs

; Update the 4 DACs
	MOVEP   Y:U_DAC0,Y:WR_DAC0	; Write to DAC0
	MOVEP   Y:U_DAC1,Y:WR_DAC1	; Write to DAC1
	MOVEP   Y:U_DAC2,Y:WR_DAC2	; Write to DAC2
	MOVEP   Y:U_DAC3,Y:WR_DAC3	; Write to DAC3

; Analog Input processor - read the 16 A/D inputs
        MOVE    X:<ONE,X0	; For incrementing accumulator to select MUX
        CLR     A  #<U_ADC0,R5	; Will contain MUX number
        DO      Y:U_NUM_AD,LOOP_AD ; Loop over each A/D converter input
        MOVEP   A,Y:WR_MUX      ; Select MUX input
        DO	#DLY_MUX,L_AD1	; Wait for the MUX to settle
	MOVE	A1,Y:SV_A1	; DO needed so SSI input can come in
L_AD1
        MOVEP   Y:STR_ADC,X1    ; Start A/D conversion - dummy read
        DO	#DLY_AD,L_AD2	; Wait for the A/D to settle
        MOVE    X:<CFFF,X1
L_AD2	
        MOVEP   Y:RD_ADC,A1     ; Get the A/D value
        AND     X1,A            ; A/D is only valid to 12 bits
        BCHG    #11,A1		; Change 12-bit 2's complement to unipolar
        MOVE    A1,Y:(R5)+      ; Put the A/D value in the table
	MOVE	Y:SV_A1,A1	; Restore A1 = MUX number
        ADD     X0,A		; Increment A = MUX number by one
LOOP_AD
	MOVEP	X:ONE,Y:WR_MUX ; Sample +5V when idle

; Control the CCD Temperature
; The algorithm assumes input from a thermistor whose A/D count  
; is inversely proportional to temperature, and output to a TE cooler.

	MOVE	Y:<U_AD_CCDT,R0
	MOVE    Y:<U_CCDT_TGT,A	; Get target CCD temperature
	MOVE    Y:(R0),X0	; Get actual CCD temperature
	SUB	X0,A
	MOVE	A,X0
	MOVE	Y:<U_TCF,X1	
	MPY	X0,X1,A		; A = (target - actual) * U_TCF
	MOVE	Y:<U_DAC2,X1	
	MOVE	Y:DAC2_LS,X0	
	ADD	X,A		; new TEC setting = current setting + A
	MOVE	Y:<U_TEC_MAX,X0	; TEC powers greater than this are not allowed
	CMP	X0,A
	JLT	<TST_LOW
	MOVE	X0,A		; Set to maximum TEC power
	JMP	<WR_DAC
TST_LOW	MOVE	Y:TEC_MIN,X0	; TEC powers less than this are not allowed	
	CMP	X0,A
	JGT	<WR_DAC
	MOVE	X0,A		; TEC off
WR_DAC	MOVEP	A,Y:WR_DAC2	; Update DAC and keep record of it
	MOVE	A,Y:<U_DAC2	
	MOVE	A0,Y:DAC2_LS

	RTS			; Return from subroutine SERVICE call


	IF	CCDTOOL

; *****  Shutter routines  *****
; Routines to open and close a mechanical shutter

OSHUT	BCLR    #SHUTTER,X:PBD  ; Clear Port B bit #3 to open shutter
        BCLR    #ST_SH,X:<STATUS ; Clear status bit to mean shutter open
        RTS

CSHUT	BSET    #SHUTTER,X:PBD  ; Set Port B bit #3 to close shutter
        BSET    #ST_SH,X:<STATUS ; Set status to mean shutter closed
        RTS

; These are called directly by command, so need to call subroutines in turn
OPEN	JSR	OSHUT		; Call open shutter subroutine
	JMP	<FINISH		; Send 'DON' reply
CLOSE	JSR	CSHUT		; Call close shutter subroutine
	JMP	<FINISH		; Send 'DON' reply

	ENDIF



;  ************************  Command  Processing  ***************************


; *****  Power off  *****
PWR_OFF	BSET	#PWRST,X:PBD	; Reset power control board
	BCLR	#PWRST,X:PBD
	JMP	<FINISH		; Reply 'DON'


; *****  Power on  *****
; Reset the power control board, ramp up the supply voltages and ensure they
; are within their tolerances, and send a command to the timing board to 
; set the CCD bias voltages.

PWR_ON	MOVEP   #$2000,X:IPR    ; Disable TIMER interrupts
	BSET	#PWRST,X:PBD	; Reset power control board
	REP	#30
	NOP
	BCLR	#PWRST,X:PBD

; Now ramp up the low voltages (+/- 15V) 
; NOTE: toggling the LVEN line is essential even if the override jumper JP1
;       is in place, in order to enable the high voltage supply.
PWR_ON1	BSET	#LVEN,X:PBD	; Make sure line is high to start with
	DO	#255,L_PON1	; The power conditioner board wants to
	BCHG    #LVEN,X:PBD	;   see 128 H --> L transitions
	NOP			; Backplane signal settling time delay
L_PON1
	MOVEP   #2,Y:WR_MUX     ; Select +15V MUX input
	DO      X:<C50000,WT_PON2 ; Wait 60 millisec for settling
        REP	#5
	MOVEP	Y:WATCH,X0	; Reset watchdog timer
WT_PON2
        MOVEP   Y:STR_ADC,X0    ; Start A/D conversion - dummy read
        DO	#DLY_AD,L_PON2	; Wait for the A/D to settle
        CLR     A  X:<CFFF,X0	; This saves some space
L_PON2
        MOVEP   Y:RD_ADC,A1     ; Get the A/D value
        AND     X0,A  Y:<U_P15_TGT,X0 ; A/D is only valid to 12 bits

; Test that the voltage is in the range abs(initial - target) < margin
        SUB     X0,A  A1,Y:<U_P15_INI
        ABS     A  Y:<U_P15_TOL,X0
        SUB     X0,A
        JGT     <PERR           ; Take corrective action

TST_M15 MOVEP   #3,Y:WR_MUX     ; Select -15v MUX input
        DO	#DLY_MUX,L_PON3	; Wait for the MUX to settle
        NOP
L_PON3
        MOVEP   Y:STR_ADC,X0    ; Start A/D conversion - dummy read
        DO	#DLY_AD,L_PON4	; Wait for the A/D to settle
        CLR     A  X:<CFFF,X0	; Clear A, so put it in DO loop
L_PON4
        MOVEP   Y:RD_ADC,A1     ; Get the A/D value
        AND     X0,A  Y:<U_M15_TGT,X0 ; A/D is only valid to 12 bits

; Test that the voltage is in the range abs(initial - target) < margin
        SUB     X0,A  A1,Y:<U_M15_INI
        ABS     A  Y:<U_M15_TOL,X0
        SUB     X0,A
        JGT     <PERR

; Now turn on the high voltage HV (nominally +36 volts)
HV_ON	BSET	#HVEN,X:PBD	; Make sure line is high to start with
	DO	#255,L_PON5	; The power conditioner board wants to
	BCHG    #HVEN,X:PBD	;   see 128 H --> L transitions
L_PON5
	MOVEP   #1,Y:WR_MUX     ; Select high voltage MUX input
	DO      X:<C50000,WT_HV ; Wait 5 millisec for settling
	NOP
WT_HV
	MOVEP   Y:STR_ADC,X0    ; Start A/D conversion - dummy read
	DO	#DLY_AD,L_PON6	; Wait for the A/D to settle
	CLR     A  X:<CFFF,X0	; Clear A, so put it in DO loop
L_PON6
	MOVEP   Y:RD_ADC,A1     ; Get the A/D value
	AND     X0,A  Y:<U_HV_TGT,X0 ; A/D is only valid to 12 bits

; Test that the voltage is in the range abs(initial - target) < margin
	SUB     X0,A  A1,Y:<U_HV_INI
	ABS     A  Y:<U_HV_TOL,X0
	SUB     X0,A
	JGT     <PERR           ; Take corrective action

; Command the timing board to turn on the analog board DC bias voltages
; and clock drivers
	MOVE	X:<TIMING,A
	MOVE	A,X:(R3)+       ; Header from Utility to timing
	MOVE	Y:INI_CMD,A
	MOVE	A,X:(R3)+       ; Set bias voltages
	MOVE	#PWR_ON3,N4	; Set internal jump address after 'DON'
	JMP	<XMT_CHK	; Send out commands to timing board

; Reply with a DONE message to the host computer
PWR_ON3	MOVE    X:<HOST,A
	MOVE    A,X:(R3)+       ; Header to host
	MOVE    X:<DON,A
	MOVE    A,X:(R3)+       ; Power is now ON
	MOVEP   #$2007,X:IPR    ; Enable TIMER interrupts
	JMP     <XMT_CHK	; Go transmit reply

; Or, return with an error message
PERR	MOVE    X:<HOST,A
	MOVE    A,X:(R3)+       ; Header to host
	MOVE	Y:POE_RPL,A
        MOVE    A,X:(R3)+	; Error in power on
	MOVEP   #$2007,X:IPR    ; Enable TIMER interrupts
	JMP     <XMT_CHK	; Go transmit reply


	IF	CCDTOOL

; *****  Start exposure  *****
START_EX
	BSET	#STRT_EX,X:<STATUS
	BSET    #ST_EX,X:<STATUS ; Exposure is in progress
	MOVE	X:<HOST,A
	MOVE	A,X:(R3)+	; Header from Utility to Host
	MOVE	X:<DON,A
	MOVE	A,X:(R3)+	; Issue a 'DON' 
	MOVE	X:<TIMING,A
	MOVE	A,X:(R3)+       ; Header from Utility to Timing
	MOVE	Y:RDC,A
	MOVE	A,X:(R3)+       ; Begin readout sequence (send 'RDC')
	JMP     <XMT_CHK	; go transmit these


; ******  Pause exposure  *****
PAUSE   BCLR    #ST_EX,X:<STATUS ; Take out of exposing mode
        JSSET   #OPT_SH,X:<OPTIONS,CSHUT ; Close shutter if needed
        JMP     <FINISH		; Issue 'DON' and get next command


; *****  Resume exposure  *****
RESUME	BSET    #ST_EX,X:<STATUS ; Put in exposing mode
	JSSET   #OPT_SH,X:<OPTIONS,OSHUT ; Open shutter if needed
        JMP     <FINISH		; Issue 'DON' and get next command


; *****  Abort exposure  *****
ABORT	BCLR    #ST_EX,X:<STATUS ; Take out of exposing mode
	MOVE    X:<TIMING,A
	MOVE    A,X:(R3)+       ; Header from Utility to timing
	MOVE    Y:IDL,A
	MOVE    A,X:(R3)+       ; Put timing board in IDLE mode
	JSR     <CSHUT          ; To be sure
	JMP     <FINISH		; Issue 'DON' and get next command

	ENDIF


; *****  Process DON reply  *****
; A 'DON' reply has been received in response to a command issued by
; the Utility board.
; Test if an internal program jump is needed after receiving a 'DON' reply

PR_DONE	MOVE	N4,R0		; Get internal jump address
	MOVE	#<START,N4	; Set internal jump address to default
	JMP	(R0)		; Jump to the internal jump address


;  **********************    End of application    ************************

; Check for program overflow - its hard to overflow since this application
;   can be very large indeed
	IF	@CVS(N,*)>APL_XY
        WARN    'Application P: overflow!'	; Make sure next application
	ENDIF					;  will not be overwritten



; ******************************   X Data   *******************************

; Command table resident in X: data memory
;  The last part of the command table is not defined for "bootrom"
;     because it contains application-specific commands
	
	IF	DOWNLOAD 	; Memory offsets for downloading code
	ORG	X:COM_TBL,X:COM_TBL
	ELSE			; Memory offsets for generating EEPROMs
        ORG     P:COM_TBL,P:APL_XY
	ENDIF

	IF	CCDTOOL
	DC	'PON',PWR_ON	; Power ON
	DC      'POF',PWR_OFF	; Power OFF
	DC	'SEX',START_EX	; Start exposure
	DC	'PEX',PAUSE	; Pause exposure
	DC	'REX',RESUME	; Resume exposure
	DC	'AEX',ABORT	; Abort exposure
	DC	'OSH',OPEN	; Open shutter
	DC	'CSH',CLOSE	; Close shutter
	ELSE
	DC	'INI',PWR_ON	; Initialize
	ENDIF

	DC      'DON',PR_DONE	; Process DON reply
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START



; *****************   Y Data (Defined in ICD 1.6/1.10)   ******************

	IF	DOWNLOAD
	ORG	Y:0,Y:0		; Download address
	ELSE
        ORG     Y:0,P:		; EEPROM address continues from P: above
	ENDIF

; Y: parameter table definitions, containing no "bootrom" definitions
U_DIG_IN	DC      0       ; Values of 16 digital input lines
U_DIG_OUT	DC      0       ; Values of 16 digital output lines

U_DAC0		DC      $800	; Table of four DAC values to be output
U_DAC1		DC      $800               
U_DAC2		DC      $800            
U_DAC3		DC      $800   
         
U_NUM_AD	DC      16      ; Number of inputs to A/D converter
U_ADC0		DC      0	; Analog input 0 (utility board temp)
U_ADC1		DC      0	; Analog input 1 (PSU + high voltage)
U_ADC2		DC      0	; Analog input 2 (PSU + low voltage)
U_ADC3		DC      0	; Analog input 3 (PSU - low voltage)
U_ADC4		DC      0	; Analog input 4 (PSU logic voltage)
U_ADC5		DC      0	; Analog input 5 (unused)
U_ADC6		DC      0	; Analog input 6 (CCD thermistor 1)
U_ADC7		DC      0	; Analog input 7 (CCD thermistor 2)
U_ADC8		DC      0	; Analog input 8 (unused)
U_ADC9		DC      0	; Analog input 9 (unused)
U_ADC10		DC      0	; Analog input 10 (unused)
U_ADC11		DC      0	; Analog input 11 (unused)
U_ADC12		DC      0	; Analog input 12 (unused)
U_ADC13		DC      0	; Analog input 13 (unused)
U_ADC14		DC      0	; Analog input 14 (unused)
U_ADC15		DC      0	; Analog input 15 (unused)

U_EL_TIM	DC	0	; Number of milliseconds elapsed
U_EXP_TIM	DC	0	; Number of milliseconds desired in exposure
U_AD_CCDT	DC	U_ADC6	; Address of CCD temperature in A/D table
U_AD_TECV	DC	U_DAC2	; Address of TEC voltage in DAC table
U_TCF		DC	$080	; Coeff. for temp. control algorithm (fraction)
U_CCDT_TGT	DC	$000	; Target CCD temperature
U_TEC_MAX	DC	$FFF	; Maximum TEC setting (+5V)

; Define power supply turn-on variables (12-bit 2's complement values)
U_HV_TGT	DC	$4D0	; Target HV supply voltage
U_HV_TOL	DC	$100	; Tolerance of HV supply voltage = LARGE
U_P15_TGT	DC	$580	; Target +15 volts supply voltage
U_P15_TOL	DC	$80	; Tolerance of +15 volts supply voltage = 0.5v
U_M15_TGT	DC	$A60	; Target -15 volts supply voltage
U_M15_TOL	DC	$80	; Tolerance of -15 volts supply voltage
U_HV_INI	DC	0	; Initial value of HV
U_P15_INI	DC	0	; Initial value of +15 volts
U_M15_INI	DC	0	; Initial value of -15 volts

U_SW_ID		DC	$010500	; Software version number 1.05

U_ERROR		DC	$000000	; Error code (unused)

; Locations Y:$29 to $3F reserved for future parameter and/or status words



; *************************   Y Data (Internal)   *************************

; Locations Y:$40 to $FF reserved for use by Utility DSP
	IF	DOWNLOAD
	ORG	Y:$40,Y:$40	; Download address
	ELSE
	ORG     Y:$40,P:	; EEPROM address continues from P: above
	ENDIF

; Define some command/reply mnemonics
INI_CMD	DC	'INI'	; Command to timing board - set bias voltages
POE_RPL	DC	'POE'	; Response to INI - power on error

	IF	CCDTOOL
RDC     DC      'RDC'   ; Command to timing board - readout CCD
IDL	DC	'IDL'	; Command to timing board - put camera in idle mode
	ENDIF

; Miscellaneous
SV_SR	DC	0	; Save status register during timer processing
SV_A1	DC	0	; Save register A1 during analog processing
EL_TIM_FRACTION DC 0	; Fraction of a millisecond of elapsed exposure time
INCR	DC	$CCCCCC	; Exposure time increment = 0.8 milliseconds
SH_DLY	DC	0	; Shutter closing time
TEC_MIN	DC	$810	; Minumum TEC power (0V)
DAC2_LS	DC	0	; LSP (fractional part) of TEC power



; During the downloading of this application program the one millisecond 
;   timer interrupts are enabled, so the utility board will attempt to execute 
;   the partially downloaded TIMER routine, and crash. A workaround is to 
;   put a RTI as the first instruction of TIMER so it doesn't execute, then 
;   write the correct instruction only after all the rest of the application 
;   program has been downloaded. Here it is - 

	ORG	P:APL_ADR+1,P:APL_ADR+1
TIMER1	MOVEC	SR,Y:SV_SR 		; Save Status Register


	ENDSEC		; End of SECTION UTILAPPL

; End of program
        END 
