       COMMENT *
Gemini WFS Utility Board Code
Controller: SDSU2
Revision: 0.0a   (must agree with U_SW_ID in Y: memory table)
(original by Dr. Bob Leach at SDSU)

02/01/14 TDH - version created to patch the problem with EEPROM writes


Assembler directives:

-d DOWNLOAD 1	To generate code for downloading to DSP memory.
-d DOWNLOAD 0	To generate code for writing to the EEPROM.

	*

; Define listing page width
	PAGE    132	; Printronix page width - 132 columns

; Name it a section so it doesn't conflict with other application programs
	SECTION	UTILPATCH

; Include definitions from header file
	INCLUDE 'utilhead.asm'

APL_NUM	EQU	1	; Application number from 0 to 10

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
; It simply sets
; a flag to say that the SERVICE routine needs executing.

TIMER	RTI				; RTI for now so downloading works
	NOP				; need to leave room for long address

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

	RTS			; Return from subroutine SERVICE call


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


; *****  Process DON reply  *****
; A 'DON' reply has been received in response to a command issued by
; the Utility board.
; Test if an internal program jump is needed after receiving a 'DON' reply

PR_DONE	MOVE	N4,R0		; Get internal jump address
	MOVE	#<START,N4	; Set internal jump address to default
	JMP	(R0)		; Jump to the internal jump address


; *****  Write Memory  *****
; Write data to DSP memory. These supersedes the boot code version residing
; in EEPROM, which is unable to write to locations in the EEPROM

WRMEM2	MOVE    X:(R4),R0	; Get the desired address
	MOVE	X:(R4)+,X0	; We need a 24-bit version of the address
	MOVE    X:(R4)+,X1	; Get value into X1 some MOVE works easily
	JCLR    #20,X0,WRX2	; Test address bit for Program memory
	MOVE	R0,X0		; Get 16-bit version of the address
	MOVE	X:<C512,A	; If address >= $200 then its an EEPROM write
	CMP	X0,A		;   and a delay 10 milliseconds is needed
	JSLE	<WR_ROM2	; Jump to EEPROM write routine if needed
	MOVE	X1,P:(R0)	; Write to Program memory
	JMP     <FINISH
WRX2	JCLR    #21,X0,WRY2	; Test address bit for X: memory
	MOVE    X1,X:(R0)	; Write to X: memory
	JMP     <FINISH
WRY2	JCLR    #22,X0,WRR2	; Test address bit for Y: memory
	MOVE    X1,Y:(R0)	; Write to Y: memory
	JMP	<FINISH
WRR2	JCLR    #23,X0,ERROR	; Test address bit for ROM memory
	JSR	<WR_ROM2	; Jump to EEPROM write routine
	JMP	<FINISH


; EEPROM write subroutine. This must be in DSP internal memory because 
; code cannot execute from external memory during a write operation
WR_ROM2	MOVE	X1,P:(R0)	; Write to Program memory
	DO	X:<C50000,LP_WRR2 
	MOVEP	Y:WATCH,A	; Delay 10 millisec for EEPROM write
LP_WRR2
	RTS


;  **********************    End of application    ************************

; Check for program overflow 
	IF	@CVS(N,*)>$200
        WARN    'Application P: overflow!'	; Make sure application
	ENDIF					;  fits in internal memory



; ******************************   X Data   *******************************

; Command table resident in X: data memory
;  The last part of the command table is not defined for "bootrom"
;     because it contains application-specific commands
	
	ORG	X:COM_TBL,X:COM_TBL

	DC	'INI',PWR_ON	; Initialize
	DC	'POF',PWR_OFF	; Power OFF
	DC	'DON',PR_DONE	; Process DON reply
	DC	'WR2',WRMEM2	; Replacement for WRM
	DC	0,START,0,START	; Fill up table with null commands
	DC	0,START,0,START
	DC	0,START,0,START
	DC	0,START,0,START
	DC	0,START,0,START
	DC	0,START,0,START



; *****************   Y Data (Defined in ICD 1.6/1.10)   ******************

	ORG	Y:0,Y:0		; Download address

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

U_SW_ID		DC	$000a00	; Software version number 0.0a

U_ERROR		DC	$000000	; Error code (unused)

; Locations Y:$29 to $3F reserved for future parameter and/or status words



; *************************   Y Data (Internal)   *************************

; Locations Y:$40 to $FF reserved for use by Utility DSP
	ORG	Y:$40,Y:$40	; Download address

; Define some command/reply mnemonics
INI_CMD	DC	'INI'	; Command to timing board - set bias voltages
POE_RPL	DC	'POE'	; Response to INI - power on error

; Miscellaneous
SV_SR	DC	0	; Save status register during timer processing
SV_A1	DC	0	; Save register A1 during analog processing


; During the downloading of this application program the one millisecond 
;   timer interrupts are enabled, so the utility board will attempt to execute 
;   the partially downloaded TIMER routine, and crash. A workaround is to 
;   put a RTI as the first instruction of TIMER so it doesn't execute, then 
;   write the correct instruction only after all the rest of the application 
;   program has been downloaded. Here it is -

	ORG	P:APL_ADR+1,P:APL_ADR+1

TIMER1	MOVEC	SR,Y:SV_SR 		; Save Status Register


; For similar reasons, we must not enable the replacement WRM routine
; until the end

	ORG	X:COM_TBL+6,X:COM_TBL+6

	DC	'WRM'		; Replacement for WRM


	ENDSEC		; End of SECTION UTILPATCH

; End of program
        END 
