	COMMENT *

Gemini WFS Timing Board Boot Code
Controller: SDSU2 
Revision: 3.02  (must agree with status word T_FW_VER in P: memory)
(This code is adapted from timboot.asm, Rev. 3.00, written by Dr. Bob Leach 
at SDSU for use with the TIMII board.)

98/07/01 TDH -reformatted source code and added comments 

98/07/20 TDH -changes to PBD initialization for new sync bit PALs 
              (U12/U17 Rev 4.1)

99/03/02 TDH -changed comments to FW_ID (now board serial number)
             -changed wait states for P: memory to 1, except during
              EEPROM access.
             -changed allotment of EEPROM application space in LDA


	*

; Define listing page width
	PAGE    132     ; Printronix page width - 132 columns

; Include definitions from header file
	INCLUDE 'timhead.asm'


;**************************************************************************
;                                                                         *
;    Permanent address register assignments                               *
;        R3 - Pointer to commands received pending processing             *
;        R4 - Pointer to processed commands		                  *
;        R6 - CCD clock driver address for CCD #0                         *
;                It is also the A/D address of analog board #0            *
;                                                                         *
;    Other registers 							  *
;	 R1, R2, R5 - Not used very much				  *
;        R0, R7 - Temporary registers used all over the place             *
;									  *
;**************************************************************************


; *************************  Interrupt vectors  *****************************

; After RESET jump to initialization code
	ORG     P:RST_ISR,P:RST_ISR+ROM_OFF 
	JMP	<INIT		; Initialize DSP after hardware reset
	NOP



; *************************  Firmware ID codes  *****************************

; Put the ID words for this version of the ROM code. It is placed at
;   the address of the SWI = software interrupt, which we never use. 

	ORG     P:ROM_ID,P:ROM_ID+ROM_OFF

T_FW_ID		DC	$000000	; board serial number
T_FW_VER	DC	$030202	; Version 3.02, board #2 = timing



; **************************  Initialization  *******************************

;  Initialization code is in the application area since it executes only once
	ORG	P:APL_ADR,P:APL_ADR+ROM_OFF	; Download address

; Define this as simple jump addresses so bootrom program is sure to work
;   until the application program can be loaded
IDLE	JMP	<START		; Defined so compiler has IDLE address

;  Initialization of the DSP - system register, serial link, interrupts.
;    This is executed once on DSP boot from ROM, and is not incorporated
;    into any download code since its not needed.
	
INIT	MOVEC   #$0002,OMR	; Operating Mode Register = Normal 
				;   Expanded - set after reset by hardware

	ORI     #$03,MR         ; Mask interrupts

	MOVEP	#0,X:PBC	; Port B Control Register to Parallel I/O

	MOVEP	#$01F0,X:PBD	; Port B Data Register
				;   H0=0 -> utility board SSI, WW=0 -> 24-bit
				;   STATUS 0 to 3 = 1, AUX1=1, SYNC=0,
				;   FD15 = 0, and FMODE = 0

	MOVEP	#$1DF3,X:PBDDR	; Port B Data Direction Register
				;   Set signals listed in PBD above to outputs

        MOVEP   #$6002,X:CRA    ; SSI programming - no prescaling; 
				;   24 bits/word; on-demand communications; 
				;   no prescale; 4.17 MHz serial clock rate

	MOVEP   #$3D30,X:CRB    ; SSI programming - OF0, OF1 don't apply; 
				;   SC0, SC1, SC2 are inputs; SCK is output;
				;   shift MSB first; rcv and xmt asynchronous
				;   wrt each other; gated clock; bit frame 
                                ;   sync; network mode to get on-demand; 
                                ;   RCV and TX enabled, RCV and TX interrrupts
		 		;   disabled -> Utility board SSI

	MOVEP   #$01E8,X:PCC	; Port C Control Register
				;   Implement SSI pins STD, SRD, SCK, SC2, SC0.

	MOVEP	#$0007,X:PCD	; Port C Data Register
				;   TXD = RXD = SCLK = 1

	MOVEP	#$0007,X:PCDDR	; Port C Data Direction Register
				;   Set signals listed in PCD above to outputs

	MOVEP	#$0191,X:BCR	; Wait states = X: Y: P: and Y: ext. I/O

	MOVEP   #$0000,X:IPR	; Write to interrupt priority register


; Initialize X: data memory
	MOVE    #RD_X,R0 	; Starting X: address in EEPROM
        MOVE    #0,R1		; Put values starting at beginning of X:
	DO      #$100,X_MOVE	; Assume 256 = $100 values exist
        DO      #3,X_LOOP	; Reconstruct bytes to 24-bit words
        MOVE    P:(R0)+,A2	; Get one byte from EEPROM
	REP	#8
        ASR     A  		; Shift right 8 bits
	NOP			; DO loop restriction
X_LOOP
        MOVE    A1,X:(R1)+	; Write 24-bit words to X: memory
X_MOVE

	BCLR	#7,X:BCR	; Reduce P: wait states after X: init

; Reset the Utility board to force it to re-boot
	BCLR	#TIM_U_RST,X:<LATCH	
	MOVEP	X:LATCH,Y:WRLATCH 	; Clear reset utility board bit
	REP	#200			; Delay by RESET* low time
	NOP
	BSET	#TIM_U_RST,X:<LATCH	
	MOVEP	X:LATCH,Y:WRLATCH 	; Clear reset utility board bit
	MOVE	#25000,A		; Delay for utility boot = 1 msec
	DO	A,RST_DLY
	NOP
RST_DLY
	NOP

; Enable the SSI link to the utility board
	MOVEP	#$0000,X:PCC	; Software reset of SSI
	MOVEP   #$01E8,X:PCC	; Enable the SSI

; Initialize the permanent registers
        MOVE    #<RCV_BUF,R3	; Starting address of command buffer
        MOVE    #WRSS,R6	; Address of clock and video processor switches
	CLR	A  R3,R4
	MOVE	#31,M3		; Buffers are circular, modulo 32
	MOVE	M3,M4
	DO	#32,ZERO_X	; Zero receiver buffer
	MOVE	A,X:(R3)+
ZERO_X

; Reset the input command FIFO
	BCLR	#RST_FIFO,X:<LATCH
	MOVEP	X:LATCH,Y:WRLATCH
	BSET	#RST_FIFO,X:<LATCH
	MOVEP	X:LATCH,Y:WRLATCH

; Set interrupt priority levels
	MOVEP   #$0007,X:IPR	; Write to interrupt priority register
				;   IRQA = 2 = synchronization = edge trigerred
				;   Host, SSI, SCI, IRQB all disabled
	ANDI    #$FC,MR		; Unmask all interrupt levels

;  Reply to the host that the system re-booted
	MOVE	#$020002,A
	MOVE	A,X:(R3)+
	MOVE	#'SYR',A
	MOVE	A,X:(R3)+

; Go execute the program - initialization is over
	JMP	<PRC_RCV	; Go look for incoming commands

; Check for program space overflow
        IF	@CVS(N,*)>$1FF
        WARN    'Internal P: memory overflow!'	; Don't overflow DSP P: space
	ENDIF



; ********************  Command interpreting code  **************************

;  Start the code in the interrupt vector area that is not used
	ORG     P:START,P:START+ROM_OFF	; Program start at P:$18

; Return here after executing each command
	MOVE	#<RCV_BUF,R3
	MOVE	R3,R4
	JSET    #IDLM,X:STAT,IDLE ; See if we're idling
TST_RCV	JSR	<GET_RCV	; See if there are any pending serial words
	JCC	<TST_RCV	; If none, then keep checking

; First check for requests for service from the utility board
CHK_SSI	JSET	#ST_RCV,X:<STAT,HEADER ; Only check if its a FIFO word
	MOVE	X:(R3),Y0	; Get candidate header
	MOVE	X:<UTL_REQ,A	; Is it the utility board requesting service?
	CMP	Y0,A
	JEQ	<GET_UTL	; Yes, go get the rest of utility board words

; Check the header (S,D,N) for self-consistency
HEADER	MOVE	X:(R3)+,X0	; Get candidate header
	JSR	<CHK_HDR	; Go check it
	JCS	<START		; Error if carry bit is set - discard header

; Read all the words of the command before processing it
RD_COM	MOVE	R4,X0		; Header address = RCV_BUF
	ADD	X0,A		; R3 must reach this for command to be complete
	MOVE	A,X1		; X1 = header address + NWORDS

RD_WORD	JCLR	#ST_RCV,X:STAT,RD_SSI
	JSR	<WT_FIFO
	MOVE	(R3)+		; Increment register
	JMP	<TST_ADR

RD_SSI	JCLR	#SSI_RDF,X:SSISR,RD_SSI
	MOVEP   X:SSIRX,X:(R3)+	; Read the SSI word into the receiver buffer

TST_ADR	MOVE	R3,A		; Get address of last word written to buffer
	CMP	X1,A		; Has it been incremented to RCV_BUF + NWORDS?
	JLT	<RD_WORD	; No, keep looking for more

;  Process the receiver entry - is its destination number = D_BRD?
PRC_RCV	MOVE	R3,A            ; Pointer to current contents of receiver
	MOVE	R4,X0           ; Pointer to processed contents
	CMP	X0,A  X:(R4),X0	; Are they equal? Get header for later
	JEQ	<START		; If unequal, process command
	JSR	<CHK_HDR	; Check the header for self-consistency
	JCS	<START		; Reset the FIFO and start over
	MOVE	X0,X:<HDR	; Save the header
	MOVE	#7,A1
	AND	X0,A  X:<DMASK,B ; Extract NWORDS
	AND	X0,B  X:<DBRD,X1 ; Extract destination byte
	CMP	X1,B  A,X:<NWORDS ; Does header = destination number? 
	JEQ	<COMMAND	; Yes, process it as a command
	JLT	<FO_XMT		; Test to send to the fiber optic transmitter

; Transmit words to the utility board over the SSI
        DO      X:<NWORDS,DON_XMT 	; Transmit NWORDS
SSI_WT	JCLR    #SSI_TDE,X:SSISR,SSI_WT	; Continue if SSI XMT register is empty
        MOVEP	X:(R4)+,X:SSITX		; Write to SSI buffer
DON_XMT 
	JMP     <PRC_RCV		; Check command continuation

;  Transmit words to the host computer over the fiber optics link
FO_XMT	DO	X:<NWORDS,DON_FFO 	; Transmit all the words in the command
	DO	#40,DLY_FFO		; Delay for the serial transmitter
        NOP
DLY_FFO
	MOVEP	X:(R4)+,Y:WRFO		; Send each word to the fo transmitter
DON_FFO
	JMP	<PRC_RCV

;  Process the receiver entry - is it in the command table ?
COMMAND	MOVE    (R4)+           ; Increment over the header
	MOVE    X:(R4)+,A       ; Get the command buffer entry
	MOVE	#<COM_TBL,R0 	; Get command table starting address
	DO      #NUM_COM,END_COM ; Loop over command table
	MOVE    X:(R0)+,X1      ; Get the command table entry
	CMP     X1,A  X:(R0),R5	; Does receiver = table entries address?
	JNE     <NOT_COM        ; No, keep looping
	ENDDO                   ; Restore the DO loop system registers
	JMP     (R5)            ; Jump execution to the command
NOT_COM MOVE    (R0)+           ; Increment the register past the table address
END_COM

;  It's not in the command table - send an error message
ERROR   MOVE    X:<ERR,X0	; Send the message - there was an error
        JMP     <FINISH1	; This protects against unknown commands

; Send a reply packet - header and reply
FINISH  MOVE    X:<DON,X0	; Send a DONE message as a reply
FINISH1	MOVE    X:<HDR,A	; Get header of incoming command
        MOVE    X:<SMASK,Y0	; This was the source byte, and is to 
	AND     Y0,A  X:<TWO,Y0	;    become the destination byte
        REP	#8		; Shift right one byte, add it to the
        LSR     A  Y0,X:<NWORDS	;     header, and put 2 as the number
        ADD     Y0,A  X:<SBRD,Y0 ;    of words in the string
	ADD	Y0,A		; Add source board's header, set X1 for above
        MOVE    A,X:(R3)+       ; Put header on the transmitter stack
	MOVE	X0,X:(R3)+	; Put value of XO on the transmitter stack
	JMP	<PRC_RCV	; Go transmit these words

;  Read the FIFO data one byte at a time and construct a 3-byte word
GET_RCV	MOVE	A,P:RSTWDT 	; Reset watchdog timer
	JCLR    #EF,X:PBD,GET_SSI
WT_FIFO	CLR	A
	MOVE	A,Y1
	MOVE	X:<EIGHT,A0
	MOVE	A0,Y0
	DO	#3,L_WORD	; Process three bytes per word
	CLR	B
L_EF	JCLR    #EF,X:PBD,L_EF	; Wait until there is a byte in the FIFO
	MOVEP	Y:RDFO,B2	; Read the next byte of FIFO data
	DO	A0,L_ASR	; Shift a byte from B2 into B1
	ASR	B
	BCLR	#7,B2
L_ASR
	ADD	Y,A		; Increment the LSB of A for DO loop argument
	MOVE	B1,X0
	OR	X0,A		; Add in the current byte to the word
L_WORD
	MOVE	A1,X:(R3)	; Put the word in the RCV buffer
	BSET	#ST_RCV,X:<STAT
	JMP	<SET_RTS	; Set status register carry bit and return

GET_SSI	JCLR	#SSI_RDF,X:SSISR,CLR_RTS
	MOVEP   X:SSIRX,X:(R3)	; Put the word in the SSI receiver buffer
	BCLR	#ST_RCV,X:<STAT
SET_RTS	BSET	#0,SR		; Set status register carry bit
	RTS
CLR_RTS	BCLR	#0,SR		; Clear status register carry bit
	RTS

;  Acknowledge the utility board request and wait for normal header word
GET_UTL	JCLR    #SSI_TDE,X:SSISR,GET_UTL ; Wait for transmitter to be empty
        MOVEP	X:TIM_ACK,X:SSITX	 ; Write acknowledge to utility board

RD_UTL	JCLR	#SSI_RDF,X:SSISR,RD_UTL	; Wait for next utility board word
	MOVEP   X:SSIRX,X:(R3)		; Overwrite UTL_REQ word
	JMP	<HEADER		; Process it as normal header word

; Check the header word (S,D,N) contained in X0 for self-consistency
CHK_HDR	MOVE    X:<MASK1,A1	; Test for S.LE.3 and D.LE.3 and N.LE.7
	AND     X0,A		
	JNE     <CHK_ERR	; Test failed
	MOVE    X:<MASK2,A1
	AND     X0,A		; Test for either S.NE.0 or D.NE.0
       	JEQ     <CHK_ERR	; Test failed
	MOVE	X:<SEVEN,A1
        AND     X0,A  		; Test for NWORDS .GE. 1
	JEQ	<CHK_ERR
	BCLR	#0,SR		; Check OK - header is self-consistent
	RTS

; Clear the FIFO and restart R3 and R4
CHK_ERR	BCLR	#RST_FIFO,X:<LATCH ; Clear the FIFO
	MOVEP	X:LATCH,Y:WRLATCH
	BSET	#RST_FIFO,X:<LATCH
	MOVEP	X:LATCH,Y:WRLATCH
        MOVE    #<RCV_BUF,R3	; Starting address of command buffer
	MOVE	R3,R4
	BSET	#0,SR
	RTS


; *****  Test Data Link  *****
; Simply return value received after 'TDL'

TDL	MOVE    X:(R4)+,X0      ; Get data value
        JMP     <FINISH1	; Return from executing TDL command


; *****  Read Memory  *****
; Get the data from DSP memory and send it over the link

RDMEM	MOVE    X:(R4),R0	; Need the address in an address register
	MOVE	X:(R4)+,A	; Need address also in a 24-bit register
        JCLR    #20,A,RDX 	; Test address bit for Program memory
	MOVE	P:(R0),X0	; Read from Program Memory
        JMP     <FINISH1	; Send out a header with the value
RDX     JCLR    #21,A,RDY 	; Test address bit for X: memory
        MOVE    X:(R0),X0	; Write to X data memory
        JMP     <FINISH1	; Send out a header with the value
RDY     JCLR    #22,A,RDR	; Test address bit for Y: memory
        MOVE    Y:(R0),X0	; Read from Y data memory
	JMP     <FINISH1	; Send out a header with the value
RDR	JCLR	#23,A,ERROR	; Test address bit for read from EEPROM memory
	BSET	#7,X:BCR	; Slow down P: accesses to EEPROM speed
	MOVE	X:<THREE,X0	; Convert to word address to a byte address
	MOVE	R0,Y0		; Get 16-bit address in a data register
	MPY	X0,Y0,A		; Multiply	
	ASR	A		; Eliminate zero fill of fractional multiply
	MOVE	A0,R0		; Need to address memory
	BSET	#15,R0		; Set bit so its in EEPROM space
	DO      #3,L1RDR
	MOVE    P:(R0)+,A2      ; Read each ROM byte
	REP     #8
	ASR     A               ; Move right into A1
	NOP
L1RDR
	MOVE    A1,X0           ; FINISH1 transmits X0 as its reply
	BCLR	#7,X:BCR	; Restore P: speed to fast
	JMP     <FINISH1


; *****  Write Memory  *****
; Write data to DSP memory.

WRMEM	MOVE    X:(R4),R0	; Get the desired address
	MOVE	X:(R4)+,A	; We need a 24-bit version of the address
        MOVE    X:(R4)+,X0	; Get datum into X0 so MOVE works easily
        JCLR    #20,A,WRX	; Test address bit for Program memory
        MOVE	X0,P:(R0)	; Write to Program memory
        JMP     <FINISH
WRX     JCLR    #21,A,WRY	; Test address bit for X: memory
        MOVE    X0,X:(R0)	; Write to X: memory
        JMP     <FINISH
WRY     JCLR    #22,A,WRR	; Test address bit for Y: memory
        MOVE    X0,Y:(R0)	; Write to Y: memory
	JMP	<FINISH
WRR	JCLR	#23,A,ERROR	; Test address bit for write to EEPROM
	BSET	#7,X:BCR	; Slow down P: accesses to EEPROM speed
	MOVE	X:<THREE,X1	; Convert to word address to a byte address
	MOVE	R0,Y0		; Get 16-bit address in a data register
	MPY	X1,Y0,A		; Multiply	
	ASR	A		; Eliminate zero fill of fractional multiply
	MOVE	A0,R0		; Need to address memory
	BSET	#15,R0		; Set bit so its in EEPROM space
	MOVE    X0,A1           ; Get data from command string
	DO      #3,L1WRR	; Loop over three bytes of the word
	MOVE    A1,P:(R0)+      ; Write each EEPROM byte
	REP     #8
	ASR     A  X:<C50000,Y0 ; Move right one byte, enter delay
	DO      Y0,L2WRR	; Delay by 12 milliseconds for EEPROM write
	REP	#4		; Assume 50 MHz DSP56002
	NOP
L2WRR
	NOP                     ; DO loop nesting restriction
L1WRR
	BCLR	#7,X:BCR	; Restore P: accesses speed
	JMP     <FINISH


; *****  Load Application  *****
; Read EEPROM code into DSP locations starting at P:APL_ADR

LDAPPL	MOVE	X:(R4)+,X0	; Number of application program
	MOVE	#N_W_APL,Y0 	; Space allowed per application
	MPY	X0,Y0,A  #APL_ADR,R7
	ASR	A		; Correct for 24-bit multiply
	MOVE	A0,R0		; EEPROM address = # x N_W_APL
	BSET	#15,R0		; All EEPROM accesses are with A15=1
	BSET	#7,X:BCR	; Slow down P: accesses to EEPROM speed
	DO	#APL_LEN,LD_LA2	; Loop through application program
	DO	#3,LD_LA1
	MOVE	P:(R0)+,A2	; Read from EEPROM
	REP	#8
	ASR	A
	NOP			; DO loop restriction
LD_LA1
	MOVE	A1,P:(R7)+	; Write to DSP P: memory
LD_LA2	

; Splice the application and boot command tables together
	MOVE	#COM_TBL,R7	; Leave most of X: memory alone
	DO	#32,LD_LA4 	; 16 commands, 2 entries per command
	DO	#3,LD_LA3
	MOVE	P:(R0)+,A2	; Read from EEPROM
	REP	#8
	ASR	A
	NOP			; DO loop restriction
LD_LA3
	MOVE	A1,X:(R7)+	; Write to DSP X: memory
LD_LA4

; Transfer Y: memory, containing waveforms and readout parameters
	MOVE	#0,R7		; Start at bottom of Y: memory
	DO	#N_W_APL-APL_LEN-32,LD_LA6	; Update Y: DSP memory
	DO	#3,LD_LA5
	MOVE	P:(R0)+,A2	; Read from EEPROM
	REP	#8
	ASR	A
	NOP			; DO loop restriction
LD_LA5
	MOVE	A1,Y:(R7)+	; Write to DSP Y: memory
LD_LA6
	BCLR	#7,X:BCR	; Restore P: accesses speed
	JMP	<FINISH


; Check that the boot code is not too big
        IF	@CVS(N,*)>APL_ADR
        WARN    'Boot program is too big!'	; Make sure application code
	ENDIF					;  will not be overwritten



; ******************************   X Data   *******************************

; Status and header processing words
        ORG     X:0,P:LD_X
STAT	DC      0       ; Status word 
LATCH	DC      $E0	; Value in latch chip U25  --> $E2 for parallel mode
HDR	DC	0	; Header for all commands
NWORDS	DC	0	; Number of words in command

;  Miscellaneous constant definitions
ZERO    DC      0
ONE	DC	1
TWO	DC	2
THREE	DC	3
SEVEN	DC	7
EIGHT	DC	8
C300    DC      $300		; Constant for resetting the DSP
C600	DC	$600		; EEPROM space per application program
C50000	DC	50000		; Delay for WRROM = 12 millisec
MASK1	DC	$FCFCF8		; Mask for checking header
MASK2	DC	$030300		; Mask for checking header
SBRD	DC	$020000 	; Source Identification number
DBRD	DC	$000200 	; Destination Identification number
DMASK   DC	$00FF00 	; Mask to get destination board number out
SMASK   DC	$FF0000 	; Mask to get source board number out
ERR	DC	'ERR'		; An error occurred
DON	DC	'DON'		; Command was fully processed
UTL_REQ	DC	$555555		; Word for utility requesting SSI service
TIM_ACK	DC	$AAAAAA		; Word for timing acknowledging SSI service


; *****  Command table  *****
; The command table is resident in X: data memory; 32 entries maximum
; The first part of the command table will be loaded with application commands

	ORG     X:COM_TBL,P:COM_TBL+LD_X

	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	DC      'TDL',TDL	; Test Data Link
	DC      'RDM',RDMEM	; Read from DSP or EEPROM memory         
	DC      'WRM',WRMEM	; Write to DSP memory        
	DC	'LDA',LDAPPL	; Load application progam from EEPROM to DSP
	DC      'STP',FINISH	; Put it here as a no op
	DC	'DON',PRC_RCV	; Nothing special
	DC      'ERR',PRC_RCV	; Nothing special
	DC	0,START		; Space for one more instruction

;  End of program
	END
