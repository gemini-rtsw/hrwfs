       COMMENT *

Gemini WFS Utility Board Boot Code
Controller: SDSU2 
Revision: 3.02  (must agree with status word U_FW_VER in P: memory)
(This code is adapted from utilboot.asm, Rev. 3.01, written by Dr. Bob Leach 
at SDSU for use with the timII board.)


98/01/27 TDH -added code to initialization section to set DACs to 0.0V on 
              reset/power up.
98/02/21 TDH -removed definitions to header file 'utilhead.asm'

	*

; Define listing page width
        PAGE    132	; Printronix page width - 132 columns

; Include definitions from header file
	INCLUDE	'utilhead.asm'


;**************************************************************************
;                                                                         *
;    Register assignments  						  *
;	 R1 - Address of SSI receiver contents				  *
;	 R2 - Address of processed SSI receiver contents		  *
;        R3 - Pointer to current top of command buffer                    *
;        R4 - Pointer to processed contents of command buffer		  *
;	 N4 - Address for internal jumps after receiving 'DON' replies	  *
;        R0, R5, R6, A, X0, and X1 - Freely available for program use     *
;        B, Y0, and Y1  - For use by timer ISR only                       *
;									  *
;**************************************************************************


; *************************  Interrupt vectors  *****************************

;  After RESET jump to initialization code
 	ORG     P:RST_ISR,P:RST_ISR+P_OFF                   
	JMP     INIT		; This is the interrupt service for RESET

; The IRQA ISR is a long interrupt keyed to the 1 millisecond timer 
        ORG     P:IRQA_ISR,P:IRQA_ISR+P_OFF
	JSR     <TIMER		; Jump to long TIMER routine for service
	NOP

; This SSI interrupt service routine receives timing board data
        ORG     P:SSI_ISR,P:SSI_ISR+P_OFF
	MOVEP   X:SSIRX,X:(R1)+  ; Put the word in the SSI receiver buffer
	NOP

; The SSI interrupts to here when there is an error.
        ORG     P:SSI_ERR,P:SSI_ERR+P_OFF       
        JSR     CLR_SSI



; *************************  Firmware ID codes  *****************************

; Put the ID words for this version of the ROM code. It is placed at
;   the address of the SWI = software interrupt, which we never use.
 
        ORG     P:ROM_ID,P:ROM_ID+P_OFF

U_FW_ID		DC	$000000	; Institution | Location | Instrument
U_FW_VER	DC	$030203	; Version 3.02, Board #3 = Utility



; **************************  Initialization  *******************************

; Initialize the DSP. Because this is executed only on DSP boot from ROM
;   it is not incorporated into any download code.

 	ORG     P:RST_OFF,P:RST_OFF                    

	MOVEC	#$02,OMR	; Normal expanded mode
	NOP			; Allow time for the remapping to occur
	JMP	INIT		; DSP resets to $E000, but we load program
				;   to EEPROM starting at RST_OFF = $6000

INIT	MOVEP   #$023B,X:PBD    ; Power enables off, shutter high
                                ;  (closed), IRQA, SYSRST, PWREN all high

        MOVEP   #$023F,X:PBDDR  ; H0 - H5 Outputs, H6 - H8 Inputs
                                ; H9 = output

	ORI     #$03,MR         ; Temporarily mask interrupts

        MOVEP   #$6000,X:CRA    ; SSI programming - no prescaling; 
				;   24 bits/word; on-demand communications; 
				;   no prescale; 5.0 MHz serial clock rate

        MOVEP   #$BD20,X:CRB    ; SSI programming - OF0, OF1 don't apply; 
				;   SC0, SC1, SC2 are inputs;  SCK is output;
				;   shift MSB first; rcv and xmt asynchronous
				;   wrt each other; gated clock; bit frame 
                                ;   sync; network mode to get on-demand; 
                                ;   RCV and its interrupts enabled; TX enabled, 
				;   TX interrrupts disabled. 

	MOVEP   #$01C8,X:PCC	; Port C implemented as enabling the SSI 
				;   function of STD, SRD, SCK, and SC0.

	MOVEP	#$0000,X:PCDDR	; Data Direction register = all inputs.

        MOVEP   #$0033,X:BCR	; Wait states for external memory accesses
				;   2 for PROM = 150 nsec
				;   2 for A/D, DAC, etc. = 150 nsec

; Load boot program into P: memory from EEPROM
	MOVE    #P_OFF,R0	; Starting P: address in EEPROM
	MOVE    #0,R1		; Put values starting at beginning of P:
	DO      #APL_ADR+2,P_MOVE ; Boot program is APL_ADR words long
				;     +2 is for SERVICE and TIMER stubs
	MOVE    P:(R0)+,A	; Get one word from EEPROM
	MOVE	A,P:(R1)+	; Write it to DSP P: memory
P_MOVE

; Load X: data memory from EEPROM
        MOVE    #X_OFF,R0	; Starting X: address in EEPROM
        MOVE    #0,R1		; Put values starting at beginning of X:
	DO      #$100,X_MOVE	; Assume 256 = $100 values exist
        MOVE    P:(R0)+,A	; Get one word from EEPROM
        MOVE    A,X:(R1)+	; Write it to DSP X: memory
X_MOVE

; Initialize various registers
        MOVE    #SSI_BUF,R1
	MOVE	#COM_BUF,R3
	MOVE	R1,R2
        MOVE    R3,R4
        MOVE    #31,M1		; Create circular buffers, modulo 32
        MOVE    M1,M2
	MOVE	M2,M3
	MOVE	M3,M4       
	MOVE	#<START,N4
	CLR	A
	CLR	B

; Initialize DACs to 0.0V
	MOVE	#$800,A
	MOVEP	A,Y:WR_DAC0
	MOVEP	A,Y:WR_DAC1
	MOVEP	A,Y:WR_DAC2
	MOVEP	A,Y:WR_DAC3

; Set interrupt priorities levels	
	MOVEP   #$2007,X:IPR    ; Write to interrupt priority register
				;   SSI = 1 = link to timing board
				;   IRQA = 2 = timer, negative edge trigerred
				;   Host, SCI, IRQB all disabled
        ANDI    #$FC,MR         ; Unmask all interrupt levels
	JMP	<XMT_CHK



; ********************  Command interpreting code  **************************

        ORG     P:START,P:START+P_OFF         
	
; Check for TIMER interrupts and go handle them if necessary
	JSSET   #ST_SRVC,X:STATUS,SERVICE ; Do all millisecond service tasks
	MOVEP	Y:WATCH,A	; Reset watchdog timer

; Test SSI receiver pointers
	MOVE    R1,A            ; Pointer to current contents of receiver
        MOVE    R2,X0           ; Pointer to processed contents
        CMP     X0,A  X:(R2),X1 ; Are they equal? Get header
        JEQ     <TST_COM	; Yes, so check the receiver stack

; Check candidate header = (S,D,N) for self-consistency
	MOVE    X:<MASK1,A1	; Test for S.LE.3 and D.LE.3 and N.LE.7
	AND     X1,A		; X1 = header from above
        JNE     <RCV_SKP        ; Test failed, skip over header
	MOVE	X:<MASK2,A1 	; Test for S.NE.0 or D.NE.0
        AND     X1,A 
       	JEQ     <RCV_SKP        ; Test failed, skip over header
	MOVE	#7,A1		; Test for N.GE.1
        AND     X1,A		; A = NWORDS in command
        JNE     <RCV_PR         ; Test suceeded - process command
RCV_SKP MOVE    (R2)+		; Header is wrong - skip over it
	JMP	<START		; Keep monitoring receiver

; Get all the words of the command before processing it
RCV_PR	MOVE	A,X1		; Number of words in command header
	DO	#<TIMEOUT,TIM_OUT
	MOVE	R1,A
	MOVE	R2,X0
	SUB	X0,A
        JGE     <RCV_L1		; X1 = Destination mask $00FF00
        MOVE    X:<C32,X0	; Correct for circular buffer
        ADD     X0,A		; No MOVE here - it isn't always executed
RCV_L1	CMP	X1,A  X1,X:<NWORDS
	JLT	<RCV_L2
	ENDDO
	JMP	<MV_COM
RCV_L2	NOP
TIM_OUT
	JMP	<RCV_SKP	; Increment R2 and BAD_HDR

; We've got the complete SSI command, so put it on the COM_BUF stack
MV_COM	DO	X:<NWORDS,SSI_WR
	MOVE	X:(R2)+,A	; R2 = SSI address
	MOVE	A,X:(R3)+	; R3 = command buffer address
SSI_WR	

; Test the command stack too
TST_COM	MOVE    R3,A            ; Pointer to current contents of receiver
        MOVE    R4,X0           ; Pointer to processed contents
	CMP     X0,A  X:<DMASK,X1 ; Are they equal? Get destination mask
	JEQ	<START		; Go back to the top

; Process the receiver entry - is its destination number = D_BRD_ID?
	MOVE    X:(R4),A        ; Get the header ID
        MOVE    A,X:<HDR_ID	; Store it for later use
        AND     X1,A  X:<DBRDID,X1 ; Extract destination byte only; Store   
	CMP     X1,A  		; = destination number?
	JEQ	<COMMAND	; It's a command for this board
        JGT     <ERROR		; Destination byte > #DBRDID, so error

; Check the transmitter buffer for commands to send to the timing board
XMT_CHK	MOVE	R3,A
	MOVE	R4,X0
	CMP	X0,A		; R4 is incremented below
	JEQ	<START		; We're all done

; Request and receive acknowledgement from the timing board
SSI_WT0	JCLR    #SSI_TDE,X:SSISR,SSI_WT0 	; SSI XMT register must be empty
        MOVEP	X:UTL_REQ,X:SSITX		; Write to SSI XMIT register
SSI_WT1	JCLR	#SSI_RDF,X:SSISR,SSI_WT1 	; Wait for timing board ACK
	MOVEP   X:SSIRX,A
	MOVE	X:<TIM_ACK,X0
	CMP	X0,A
	JNE	<XMT_CHK

; Transmit the command to the timing board over the SSI
	MOVE	X:(R4),X0			; Read the header word
	MOVE	#>7,A1				; Extract the 3 LS bits
        AND     X0,A				; X0 = NWORDS in command
	DO	A1,SSI_XMT
SSI_WT2	JCLR    #SSI_TDE,X:SSISR,SSI_WT2  	; SSI XMT register must be empty
        MOVEP	X:(R4)+,X:SSITX			; Write to SSI buffer
SSI_XMT
	JMP	<XMT_CHK			; Is there more to transmit?

; Process the command - is it in the command table ?
COMMAND	MOVE    (R4)+           ; Increment over the header ID
        MOVE    X:(R4)+,A       ; Get the command buffer entry
	MOVE	#<COM_TBL,R0	; Get command table address
        DO      #NUM_COM,END_COM ; Loop over command table
        MOVE    X:(R0)+,X0      ; Get the command table entry
        CMP     X0,A  X:(R0),R5	; Are the receiver and table entries the same?
        JNE     <NOT_COM        ; No, keep looping
        ENDDO                   ; Restore the DO loop system registers
        JMP     (R5)            ; Jump execution to the command
NOT_COM MOVE    (R0)+           ; Increment the register past the table address
END_COM

; Step over the remaining words in the command if there's an error
	MOVE	X:<NWORDS,A 
	MOVE	X:<TWO,X0
	SUB	X0,A		; Header and command have been processed
	JEQ	<ERROR
	DO	A,INCR_R4
	MOVE	(R4)+		; Increment over unprocessed part of comamnd
INCR_R4

ERROR   MOVE    X:<ERR,X0	; Send the message - there was an error
        JMP     <FINISH1	; This protects against unknown commands

; Command execution is nearly over - generate header and message.
FINISH  MOVE    X:<DON,X0	; Send a DONE message as a reply
FINISH1	MOVE    X:<HDR_ID,A	; Get header of incoming command
	MOVE    X:<SMASK,X1	; This was the source byte, and is to 
	AND     X1,A  X:<TWO,X1	;   become the destination byte
	REP	#8		; Shift right one byte, add it to the
	LSR     A  X1,X:<NWORDS	;     header, and put 2 as the number
	ADD     X1,A  X:<SBRDID,X1 ;  of words in the string
	ADD     X1,A
	MOVE    A,X:(R3)+       ; Put header on the transmitter stack
	MOVE	X0,X:(R3)+	; Put value of X0 on the transmitter stack
	JMP	<XMT_CHK	; Go transmit

; Delay after EEPROM write in DSP internal memory because code cannot 
;   execute from EEPROM during a write operation
DLY_ROM	DO	X:<C50000,LP_WRR 
	MOVEP	Y:WATCH,A	; Delay 10 millisec for EEPROM write
LP_WRR
        JMP     <FINISH

; Clear error condition and interrupt on SSI receiver
CLR_SSI MOVEP   X:SSISR,X:RCV_ERR ; Read SSI status register
        MOVEP   X:SSIRX,X:RCV_ERR ; Read receiver register to clear error
        RTI

; Check for overflow
        IF	@CVS(N,*)>APL_ADR
        WARN    'Internal P: memory overflow!'	; Make sure application code
	ENDIF					;  will not be overwritten



; *********************  Boot routines in EEPROM  ***************************

; The following boot routines execute directly from EEPROM, one 24-bit word
;   at a time. Two reasons for this - to conserve DSP P: space, and to allow
;   reading or writing to EEPROM space that overlaps with the P: internal
;   DSP memory space.

 	ORG     P:ROM_EXE,P:ROM_EXE                 


; *****  Test Data Link  *****
; Simply return value received after 'TDL'

TDL     MOVE    X:(R4)+,X0      ; Get data value
        JMP     <FINISH1	; Return from executing TDL command


; *****  Read Memory  *****
; Get the data from DSP memory and send it over the link

RDMEM	MOVE    X:(R4),R0	; Need the address in an address register
	MOVE	X:(R4)+,X0	; Need address also in a 24-bit register
        JCLR    #20,X0,RDX	; Test address bit for Program memory
        MOVE	P:(R0),X0	; Read from Program memory
        JMP     <FINISH1	; Send out a header with the value
RDX     JCLR    #21,X0,RDY	; Test address bit for X: memory
        MOVE    X:(R0),X0	; Write to X data memory
        JMP     <FINISH1	; Send out a header with the value
RDY     JCLR    #22,X0,RDR	; Test address bit for Y: memory
        MOVE    Y:(R0),X0	; Read from Y data memory
	JMP     <FINISH1	; Send out a header with the value
RDR	JCLR	#23,X0,ERROR	; Test for read of EEPROM memory
	MOVEC	#$03,OMR	; Development mode - disable internal P: memory
	NOP
	MOVE	P:(R0),X0	; Read from EEPROM
	MOVEC	#$02,OMR	; Normal mode - enable internal P: memory
	NOP
	JMP	<FINISH1


; *****  Write Memory  *****
; Write data to DSP memory.

WRMEM	MOVE    X:(R4),R0	; Get the desired address
	MOVE	X:(R4)+,X0	; We need a 24-bit version of the address
	MOVE    X:(R4)+,X1	; Get value into X1 some MOVE works easily
	JCLR    #20,X0,WRX	; Test address bit for Program memory
	MOVE	R0,X0		; Get 16-bit version of the address
	MOVE	X:<C512,A	; If address >= $200 then its an EEPROM write
	CMP	X0,A		;   and a delay 10 milliseconds is needed
	MOVE	X1,P:(R0)	; Write to Program memory
	JLE	<DLY_ROM	; Jump to delay routine if needed   
	JMP     <FINISH
WRX	JCLR    #21,X0,WRY	; Test address bit for X: memory
	MOVE    X1,X:(R0)	; Write to X: memory
	JMP     <FINISH
WRY	JCLR    #22,X0,WRR	; Test address bit for Y: memory
	MOVE    X1,Y:(R0)	; Write to Y: memory
	JMP	<FINISH
WRR	JCLR    #23,X0,ERROR	; Test address bit for ROM memory
	MOVE	#3,OMR		; Development mode - disable internal P: memory
	NOP
	MOVE	X1,P:(R0)	; Write to EEPROM
	MOVE	#2,OMR		; Normal mode - enable internal P: memory
	NOP
	JMP	<DLY_ROM	; Delay 10 milliseconds for EEPROM write


; *****  Load Application  *****
; Read EEPROM code into DSP locations starting at P:APL_ADR

LDA	ORI	#$03,MR		; Temporarily mask interrupts
	MOVE	X:(R4)+,X0	; Number of application program
	MOVE	X:<ZERO,A
	CMP	X0,A  X:<C300,X1
	JEQ	LDA_0		; Application #0 is a special case
	MPY	X0,X1,A  X:<ZERO,X1
	ASR	A  X:<C1D00,X0
	ADD	X,A  #APL_ADR,R5
	MOVE	A0,R0		; EEPROM address = # x $300 + $1D00
	DO	#$200-APL_ADR,LD_LA0  ;  Thus  ( 1 <= # <= 10 )
	MOVE	P:(R0)+,A	; Read from EEPROM
	MOVE	A,P:(R5)+	; Write to DSP
LD_LA0
	JMP	LD_X		; Keep R0 value

; Special case - application #0 can spill from internal P: DSP to EEPROM memory
LDA_0	MOVE	#APL_ADR,R0
	DO	#$200-APL_ADR,LD_LA1
	MOVE	#3,OMR		; Development mode - disable internal P: memory
	NOP
	MOVE	P:(R0),A	; Read from EEPROM
	MOVE	#2,OMR		; Normal mode - enable internal P: memory
	NOP
	MOVE	A,P:(R0)+	; Write to DSP
LD_LA1

; Load in the application command table into X:CMD_TBL
	MOVE	#APL_XY,R0
LD_X	MOVE	#COM_TBL,R5
	DO	#32,LD_LA2	; 16 application commands
	MOVE	P:(R0)+,A
	MOVE	A,X:(R5)+
LD_LA2

; Load the Y: data memory contents 
	MOVE	#0,R5		; Start at bottom of Y: memory
	DO	#$100,LD_LA3	; Read from EEPROM and write
	MOVE	P:(R0)+,A	;   them to Y: memory
	MOVE	A,Y:(R5)+
LD_LA3
	ANDI    #$FC,MR         ; Unmask interrupts
	JMP	<FINISH		; Send 'DON' message



; ************************  Application program  ****************************

	ORG	P:APL_ADR,P:APL_ADR+P_OFF

; Define TIMER as a simple jump addresses so the "bootrom" program 
;   can work until the application program can be loaded
SERVICE	RTS			; Just return from subroutine call
TIMER	RTI			; Just return from interrupt



; ******************************   X Data   *******************************

; Parameter definitions in X: memory space
	ORG	X:0,P:X_OFF
STATUS  DC      0       ; Status word
OPTIONS DC      0       ; Software options
NWORDS  DC      0	; Number of words in destination command packet
HDR_ID  DC      0       ; 24-bit header containing board ID's

; Constant definitions, useful for saving program memory space
ZERO	DC	0	
ONE     DC      1
TWO	DC	2
C32	DC	32
C512	DC	512	; Boundary between DSP and EEPROM P: memory
C1D00	DC	$1D00	; Offset for loading application programs
MASK1	DC	$FCFCF8	; Mask for checking header ID
MASK2	DC	$030300	; Mask for checking header ID
CFFF    DC      $FFF    ; Mask for 12-bit A/D converter
C300    DC      $300	; Constant for resetting the DSP
C50000	DC      50000   ; Delay for +/- 15v and EEPROM settling
SBRDID  DC	$030000 ; Source Identification number
DBRDID  DC	$000300 ; Destination Identification number
DMASK   DC	$00FF00 ; Mask to get destination board number out
SMASK   DC	$FF0000 ; Mask to get source board number out
HOST    DC      $030002 ; Header to host computer
TIMING  DC      $030202 ; Header to timing board
UTIL	DC	$030302	; Header to utility board
ERR	DC	'ERR'	; For sending error messages
DON	DC	'DON'	; For sending completion messages
RCV_ERR DC      0	; Dummy location for receiver clearing

; Miscellaneous
UTL_REQ	DC	$555555	 ; Word for utility requesting SSI service
TIM_ACK	DC	$AAAAAA	 ; Word for timing acknowledging SSI service


; *****  Command table  *****
; The command table is resident in X: data memory; 32 entries maximum

        ORG     X:COM_TBL,P:COM_TBL+X_OFF

	DC	0,START,0,START,0,START,0,START ; This is where application
	DC	0,START,0,START,0,START,0,START ;    commands go
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	DC      'TDL',TDL       ; Test Data Link        
	DC      'RDM',RDMEM	; Read DSP or EEPROM memory
	DC	'WRM',WRMEM	; Write DSP or EEPROM memory 
	DC	'LDA',LDA	; Load application program from EEPROM      
        DC      'ERR',START	; Do nothing
	DC	0,START,0,START,0,START
	
; End of program
        END 

