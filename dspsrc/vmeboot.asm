       COMMENT *
Gemini WFS VME Interface Board Boot Code
Controller: SDSU2 
Revision: 3.03  (must agree with status word V_FW_VER in P: memory)
(This code is adapted from vmeboot v3.00 written by Dr. Bob Leach at SDSU)

98/05/22 TDH -removed common constant definitions to vmehead.asm
             -added command RRS
             -minor change to initialization of Port B DDR
             -added a check at START to jump to the readout code if
              the board is in readout mode
             -added AFE response to RDM and WRM for bad addresses
             -changed application code starting address (APL_ADR) to $150
              because of longer boot code

98/06/16 TDH -increased APL_LEN to allow for longer applications
             -changed allotment of EEPROM application space

98/07/03 TDH -reformatted source code (moved some code around) and added 
              comments 

        *

; Define listing page width
        PAGE    132             ; Printronix page width - 132 columns

; Include definitions from header file
	INCLUDE "vmehead.asm"

;************************************************************************
;									*
;    Address register assignments					*
;	R1 - Fibre optic receiver buffer pointer (last received entry)	*
;	R2 - VMEbus receiver buffer pointer (last received entry)	*
;	R3 - Command buffer pointer (last received entry)		*
;	R4 - Command buffer pointer (last processed entry)		*
;									*
;	R0 - used for temporary storage all over the place, but		*
;		N0 set to 2 and must not be changed.			*
;	R5 - used for temporary storage of VME/FO buffer pointer	*
;		during command processing (last processed entry)	*
;		M5 set to modulo 32					*
;									*
;************************************************************************


; *************************  Interrupt vectors  *****************************

; After RESET jump to initialization code
	ORG	P:RST_ISR,P:RST_ISR+ROM_OFF         
	JMP     <INIT		; This is the interrupt service for RESET
	NOP

; IRQA reads VMEbus data.
	ORG	P:IRQA_ISR,P:IRQA_ISR+ROM_OFF
	JSR	<READ_VME
	NOP



; *************************  Firmware ID codes  *****************************

; Put the ID words for this version of the ROM code. It is placed at
;   the address of the SWI = software interrupt, which we never use. 
        ORG     P:ROM_ID,P:ROM_ID+ROM_OFF
V_FW_ID		DC	$000000	; Institution | Location | Instrument
V_FW_VER	DC      $030301	; Version 3.03, board #1 = VMEINF Rev. 7A



; *************************  Application code  *****************************

; Specify the memory location where the readout program is to be loaded
	ORG	P:APL_ADR,P:APL_ADR+ROM_OFF

; Define this as simple jump addresses so bootrom program is sure to work
;   until the application program can be loaded
RDING	JMP	<CHK_CMD	; Defined so compiler has RDING address



; **************************  Initialization  *******************************

INIT				; Must define this address for all cases
	
; Initialization of the DSP - system register, serial link, interrupts.
;    This is executed once on DSP boot from ROM, and is not incorporated
;    into any download code since its not needed.

        MOVEC   #$02,OMR        ; Operating Mode Register = Normal 
                                ;    Expanded - set after reset by hwd.

; PLL - Multiplication factor = 4, disable CKOUT
	MOVEP	#$670003,X:PCTL	; MFO to 11=3, DFO to 3=0, XTLD=1, PSTP=1,
				;   PEN=1, COD0 to 1=0, CSRC=1 and CKOS=1
				;   CKOUT enabled

; Port B
        MOVEP   #$0,X:PBC       ; Port B Control Register enabling port B
                                ;   pins as general purpose I/O.
        MOVEP   #$11B4,X:PBDDR  ; Set direction of port B pins.
	MOVEP	#$0084,X:PBD	; REQBUS/I = bit 7 = 1 => not requesting bus
				; WL = bit 12 = 0 => 24 bits written to VMEbus
				; MODE = bit 2 = 1 => D32 incoming data

; Port C
	MOVEP   #$0140,X:PCC	; Enable SSI functions SCK and STD only
	MOVEP	#$0020,X:PCDDR	; PC5 is an output
	MOVEP	#$0003,X:CRA	; SSI: 64 MHz / 16 = 4 MHz output, 8 bits/word
	MOVEP   #$1338,X:CRB

; Initialize some board functions
	MOVE	X:RDCOM,A	; Reset the IRQA flip flop
	CLR	A
	MOVE	A,X:CLRRDFIFO	; Clear to non-automatic RDFIFO cycling
	JSR	<XMIT	
	REP	#50		; Wait for serial data transmission
	NOP
	MOVE	A,X:RSTFIFO	; Reset FIFO
	CLR	A
	MOVE	A,X:SELBLT	; Make sure BLT = block transfer line is cleared

	MOVEP	#$1181,X:BCR	; Wait states for external memory accesses
				;   Ext. X:, Ext. Y, Ext. P:, Ext. I/O
	MOVEP   #$0007,X:IPR    ; IRQA = priority 2, VME command

; Initialize X: data memory
	MOVE    #RD_X,R0	; Starting X: address in PROM
	MOVE    #0,R1           ; Put values starting at beginning of X:
	DO      #$100,XMOVE     ; Assume 256 = $100 values exist
	DO      #3,XLOOP        ; Reconstruct bytes to 24-bit words
	MOVE    P:(R0)+,A2      ; Get one byte from ROM
	REP     #8
	ASR     A               ; Shift right 8 bits
XLOOP
	MOVE    A1,X:(R1)+      ; Write 24-bit words to X: memory
XMOVE

; Initialize the permanent registers
	MOVE	#FO_BUF,R1	; Starting address of FIFO buffer
	MOVE	#VME_BUF,R2	; Starting address of VMEbus buffer
	MOVE	#COM_BUF,R3	; Starting address of command buffer
	MOVE	R1,X:<R1PROC
	MOVE	R2,X:<R2PROC
	MOVE	R3,R4
	MOVE	#31,M1
	MOVE	M1,M2
	MOVE	M2,M3
	MOVE	M3,M4
	MOVE	M4,M5		; Create circular buffers, all modulo 32
	MOVE	#2,N0		; Post-increment address by 4 bytes
	CLR	B		; B1 is used for the serial receiver
	REP	#32		; Zero out the command buffer for ease
	MOVE	B,X:(R3)+	;   of debugging

; Unmask all interrupts now that the system has been initialized
        ANDI	#$FC,MR		; Unmask all interrupts

; All done with initialization
	JMP	<START

; Check for overflow
        IF	@CVS(N,*)>$1FF
        WARN    'Internal P: memory overflow!'	; Make sure readout code
	ENDIF					;  will not be overwritten



; ********************  Command interpreting code  **************************

; Start the command interpreting code
        ORG     P:START,P:START+ROM_OFF

; Return here after executing each command
	MOVE	X0,X:RSTWDT	; Assert reset watch dog timer line
;	MOVE	#COM_BUF,R3	; Starting address of command buffer
;	MOVE	R3,R4		; Reset command buffer
	JSET    #RD,X:STATUS,RDING ; See if we're reading out

; Test the VMEbus receiver buffer contents
CHK_CMD	JSR	<VME_TST	; Test for incoming VMEbus commands
	JNE	<CHK_HDR	; A VMEbus command was received, so check it

; Test the FO receiver buffer contents
FO_TST	JSSET	#EF,X:PCD,READ_FIFO ; Read FIFO if there's anything there
	MOVE	X:<R1PROC,X0	; Get pointer to processed contents of buffer
	MOVE	R1,A		; Get current command buffer pointer
	CMP	X0,A  X0,R5
	JEQ	<START		; Keep trying if R1 .EQ. X:<R1PROC
	BSET	#ST_ISR,X:<STATUS

; Check header (S,D,N) for self-consistency
CHK_HDR	MOVE	X:(R5),Y0	; Get candidate header
	MOVE    X:<MASK1,A1	; Test for S.LE.3 and D.LE.3 and N.LE.7
	AND     Y0,A  X:<MASK2,B1
        JNE     <RCV_ERR	; Test failed 
        AND     Y0,B  #7,A1	; Test for S.NE.0 or D.NE.0
       	JEQ     <RCV_ERR	; Test failed
        AND     Y0,A  		; Test for N.GE.1
        JNE     <RCV_PR         ; Test suceeded - process command

; If a header value is wrong then reset the FIFO and buffer pointers
RCV_ERR	MOVE	A,X:RSTFIFO	; Reset FIFO
	MOVE	#FO_BUF,R1	; Starting address of FIFO buffer
	MOVE	#VME_BUF,R2	; Starting address of VMEbus buffer
	MOVE	R1,X:<R1PROC
	MOVE	R2,X:<R2PROC
	JMP	<START		; Wait for the next command

; Get all the words of the command before processing it
RCV_PR	MOVE	A,Y0		; Number of words in command header
	DO	X:<TIMEOUT,TIM_OUT
	JSSET	#EF,X:PCD,READ_FIFO ; Read FIFO if there's anything there
	MOVE	R1,A
	JSET	#ST_ISR,X:STATUS,RCV_WT1
	MOVE	R2,A		
RCV_WT1	SUB	X0,A		; X0 = R#PROC from VME_TST or FO_TST
        JGE     <RCV_L1		; X1 = Destination mask $00FF00
        MOVE    X:<C32,X1	; Correct for circular buffer
        ADD     X1,A		; No MOVE here - it isn't always executed
RCV_L1	CMP	Y0,A  Y0,X:<NWORDS ; Y0 = NWORDS from above
	JLT	<RCV_L2
	ENDDO
	JMP	<MV_COM
RCV_L2	NOP
TIM_OUT
	MOVE	(R5)+		; Increment R5 past header
	MOVE	X:<TIM,X0	; Reply will be Timeout
	JMP	<FINISH1	; Send reply

; We've got the complete command, so put it on the COM_BUF stack
MV_COM	DO	X:<NWORDS,XFER
	MOVE	X:(R5)+,A	; R5 = R#PROC from VME_TST or FO_TST
	MOVE	A,X:(R3)+
XFER
	JSR	<R_PROC

; Process the receiver entry - is its destination number = D_BRD_HDR?
PRC_RCV MOVE    R3,A            ; Pointer to current contents of receiver
        MOVE    R4,X0           ; Pointer to processed contents
	CMP     X0,A  X:<DMASK,X1 ; Are they equal? Get destination mask
	JEQ	<START		; If unequal, there's a command to be processed
        MOVE    X:(R4),A        ; Get the header
        MOVE    A1,X:<HDR
        AND     X1,A  X:<DBRDHDR,X1 ; Extract destination byte only, Store HDR
        CMP     X1,A            ; It is the same as the destination number?
        JEQ     <COMMAND	; Yes, process it as a command
        JGT     <XMT_TIM

; Send the command over the fiber optic transmitter to the timing board or to
;   the VMEbus, depending on what the destination byte of the header is.

; Write to the VMEbus
	JCLR    #SRA_EX,X:STATUS,SKP_RPY ; Reply address must be valid 
	MOVE    X:<LRPLADR,R0	 
	MOVE	#63,M0		; Make R0 a circular buffer modulo 64
	DO      X:<NWORDS,L3	; Loop over all words in command string
	MOVE    X:(R4)+,A
	JCLR    #DONEVME,X:PBD,* ; Wait here until VMEbus is available
	MOVE    A,Y:(R0)+N0     ; Write to VMEbus
L3
	MOVE	R0,X:<LRPLADR	; Save VMEbus lower address for future use
	MOVE	X:<CFFFF,M0	; Restore linear addressing to R0

; Generate a VMEbus interrupt if OPTIONS bit INTR is set
 	JCLR    #INTR,X:OPTIONS,START 	; Optional interrupt generation
        MOVE	A,X:RQINTR		; Request a VMEbus interrupt
	JMP	<START

SKP_RPY	REP	X:<NWORDS	; Skip over this command because reply
	MOVE	(R4)+		;   address has not yet been specified
	JMP	<START

; Send the command to the timing board
XMT_TIM DO      X:NWORDS,L4     ; Transmit to timing board
	MOVE	X:(R4)+,A	; Get the next word for transmission
	JSR	<XMT_WRD	; Transmit word
	NOP			; Do loop restriction
L4
	JMP     <START

; Transmit one complete word to the timing board
XMT_WRD	JCLR	#SSI_TDE,X:SSISR,*
	MOVEP	#$000000,X:SSITX
	JCLR	#SSI_TDE,X:SSISR,*	; Start bit
	MOVEP	#$010000,X:SSITX
	JCLR	#SSI_TDE,X:SSISR,*
	MOVEP	#$AC0000,X:SSITX	; Preamble byte
	JSR	<XMIT			; transmit 3 bytes
	JCLR	#SSI_TDE,X:SSISR,*
	MOVEP	#$000000,X:SSITX	; Stop bit (?)
	RTS

; Serially transmit 3 bytes to timing board
XMIT	DO	#3,L_XMIT
	JCLR	#SSI_TDE,X:SSISR,*		; Three data bytes	
	MOVEP	A1,X:SSITX
	REP	#8
	LSL	A
L_XMIT
	RTS

; Update the pointer to the already processed commands
R_PROC	JCLR	#ST_ISR,X:STATUS,LBL1 ; Does VME or FO have an entry?
	MOVE	R5,X:<R1PROC	; Update FO pointer
	RTS
LBL1	MOVE	R5,X:<R2PROC	; Update VMEbus pointer
	RTS

; Check the VMEbus for incoming commands
VME_TST	MOVE	X:<R2PROC,X0	; Get pointer to processed contents of buffer
	MOVE	R2,A		; Get current command buffer pointer
	CMP	X0,A  X0,R5
	JEQ	<VME_RTS	; Process header if R2 .NE. X:R2PROC
	BCLR	#ST_ISR,X:<STATUS
VME_RTS	RTS

; Process the receiver entry - is it in the command table ?
COMMAND	MOVE    (R4)+           ; Increment over the header
        MOVE    X:(R4)+,A	; Get the command buffer entry
	MOVE    #COM_TBL,R0	; Get command table starting address
        DO      #NUM_COM,END_COM ; Loop over command table
        MOVE    X:(R0)+,X0      ; Get the command table entry
        CMP     X0,A  		; Are the receiver and table entries the same?
        JNE     <NOT_COM        ; No, keep looping
	MOVE	X:(R0),R0	; Get jump address
        ENDDO                   ; Restore the DO loop system registers
        JMP     (R0)            ; Jump execution to the command
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
FINISH1	MOVE    X:<HDR,A	; Get header of incoming command
	MOVE    X:<SMASK,Y0	; This was the source byte, and is to 
	AND     Y0,A  X:<TWO,Y0	;   become the destination byte
	REP	#8		; Shift right one byte, add it to the
	LSR     A  Y0,X:<NWORDS	;     header, and put 2 as the number
	ADD     Y0,A  X:<SBRDHDR,Y0 ;  of words in the string
	ADD     Y0,A
	MOVE    A,X:(R3)+       ; Put header on the transmitter stack
	MOVE	X0,X:(R3)+	; Put value of XO on the transmitter stack
	JMP	<PRC_RCV	; Process this reply

; Long interrupt service routine for reading commands from the VMEbus
READ_VME
	MOVE	Y1,X:<SV_Y1	; Save Y1
	MOVE	X:RDCOM,Y1	; Read the command from the register
	MOVE	Y1,X:(R2)+	; Put the command on the VME command stack
	MOVE	X:<SV_Y1,Y1	; Restore Y1
	RTI			; Return from interrupt

; Routine for reading commands and replies from the timing board via the FIFO
READ_FIFO
	MOVE	X:RDFIFO,A	; Get most significant word
	MOVE	A,B		; Test that the header byte = $AC
	MOVE	X:<DMASK,X1	; DMASK = $00FF00
	AND	X1,B  
	MOVE	X:<RCV_HDR,X1	; RCV_HDR = $00AC00
	CMP	X1,B  X:RDFIFO,X1
	JNE	<FO_TST		; If byte does not equal $AC then re-read FIFO
	REP	#16		;   if there's still anything there
	LSL	A
	OR	X1,A		; Add the two words together
	MOVE	A1,X:(R1)+	; Put it onto the fiber optic stack
	RTS


; *****  Test Data Link  *****
; Simply return value received after 'TDL'

TDL	MOVE    X:(R4)+,X0	; Get data value
        JMP     <FINISH1	; Return from executing TDL command


; *****  Set Reply Address  *****
; Set VMEbus reply address (header 'SRA' Haddr Laddr)

SRADDR  MOVE	X:(R4)+,X0	; Get 16-bits of high address
        MOVE	X0,X:WRHADR	; Write high word of VMEbus address to latch
        MOVE	X0,X:<HRPLADR	; Store for later use
        MOVE	X:(R4)+,A 	; Get 16-bits of low address
	LSR	A  X:<VME_HDR,Y0 ; Convert VMEbus byte address to word address
        BSET	#15,A		; Put address in top of Y: memory space
        MOVE	A,X:<LRPLADR	; Write low word of VMEbus address
	MOVE	A1,R0 		; Get low VME address for write below
        BSET	#SRA_EX,X:<STATUS ; SRA command has been executed
	MOVE	X:<AM_REG,X0	; Interrupt vector + address modifiers + control
				; AM = $09 or $0D - Extended supervisory or
				;   non-priveledged data access.
				; WRITE = LWORD = 0 => write 32-bit data
				; $0D - for a32d32 (default)
				; $3D - for a24d32
				; $F0 = Interrupt service address in VME space
	JCLR    #DONEVME,X:PBD,* ; Make sure the VMEbus is available
	MOVE	Y0,Y:(R0)	; Write a dummy header to the VMEbus
	MOVE	X0,X:WRAM	; Extended data access
        JMP	<FINISH


; *****  Read Memory  *****
; Read contents of DSP or external ROM memory

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
RDR	JCLR	#23,A,ADDERR	; Test address bit for read from EEPROM memory
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
L1RDR
	MOVE    A1,X0           ; FINISH1 transmits X0 as its reply
	JMP     <FINISH1


; *****  Write Memory  *****
; Program WRMEM ('WRM' address datum): write to memory, reply 'DON'

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
WRR	JCLR	#23,A,ADDERR	; Test address bit for write to EEPROM
	MOVE	X:<THREE,X1	; Convert to word address to a byte address
	MOVE	R0,Y0		; Get 16-bit address in a data register
	MPY	X1,Y0,A		; Multiply	
	ASR	A  X0,B1	; Eliminate zero fill of fractional multiply
	MOVE	A0,R0		; Need to address memory
	BSET	#15,R0		; Set bit so its in EEPROM space
	DO      #3,L1WRR	; Loop over three bytes of the word
	MOVE    B1,P:(R0)+      ; Write each EEPROM byte
	REP     #8
	ASR     B  X:<C50000,Y0 ; Move right one byte, enter delay
	DO      Y0,L2WRR	; Delay by 10 milliseconds for EEPROM write
	REP	#4		; Assume 50 MHz DSP56002
	NOP
L2WRR
	NOP                     ; DO loop nesting restriction
L1WRR
	JMP     <FINISH

; Send 'AFE' response if the RDM or WRM address is bad
ADDERR	MOVE    X:<AFE,X0	; Send the message - there was an error
        JMP     <FINISH1


; *****  Load Application  *****
; Read EEPROM code into DSP memory starting at P:APL_ADR.

LDAPPL	MOVE	X:(R4)+,X0	; Number of application program
	MOVE	#N_W_APL,Y0 	; Space allowed per application
	MPY	X0,Y0,A  #APL_ADR,R7
	ASR	A		; Correct for 24-bit multiply
	MOVE	A0,R0		; EEPROM address = # x N_W_APL
	BSET	#15,R0		; All EEPROM accesses are with A15=1
	DO	#APL_LEN,LD_LA2	; Loop through application program
	DO	#3,LD_LA1
	MOVE	P:(R0)+,A2	; Read from EEPROM
	REP	#8
	ASR	A
LD_LA1
	MOVE	A1,P:(R7)+	; Write to DSP P: memory
LD_LA2

; Splice the application and boot command tables together
	MOVE	#COM_TBL,R7	; Leave most of X: memory alone
	DO	#64,LD_LA4 	; 32 commands, 2 DSP words per command
	DO	#3,LD_LA3
	MOVE	P:(R0)+,A2	; Read from EEPROM
	REP	#8
	ASR	A
LD_LA3
	MOVE	A1,X:(R7)+	; Write to DSP X: memory
LD_LA4

; Transfer to Y: memory, containing application program waveforms and 
;   readout parameters
	MOVE	#0,R7		; Start at bottom of Y: memory
	DO	#N_W_APL-APL_LEN-64,LD_LA6	; Update Y: DSP memory
	DO	#3,LD_LA5
	MOVE	P:(R0)+,A2	; Read from EEPROM
	REP	#8
	ASR	A
LD_LA5
	MOVE	A1,Y:(R7)+	; Write to DSP Y: memory
LD_LA6
	JMP	<FINISH


; ***** Remote Reset  *****
; Send code to cause a reset of the timing and utility boards

RMT_RST	JCLR	#SSI_TDE,X:SSISR,*
	MOVEP	#$000000,X:SSITX
	JCLR	#SSI_TDE,X:SSISR,*	; Start bit
	MOVEP	#$010000,X:SSITX
	JCLR	#SSI_TDE,X:SSISR,*
	MOVEP	#$530000,X:SSITX	; Preamble byte (reset code)
	JSR	<XMIT			; transmit 3 more bytes (ignored)
	JCLR	#SSI_TDE,X:SSISR,*
	MOVEP	#$000000,X:SSITX	; stop bit (?)
	JMP	<START


; Check that the boot code is not too big
        IF	@CVS(N,*)>APL_ADR
        WARN    'Boot program is too big!'	; Make sure application code
	ENDIF					;  will not be overwritten



; ******************************   X Data   *******************************

; Status and header processing words

	ORG	X:0,P:LD_X

STATUS  DC      0	; Current execution status word of this board 
OPTIONS DC      4	; Software options for this program to execute
HRPLADR	DC      0	; High word of VME reply address
LRPLADR	DC	0	; Low word of VMEbus reply address
NWORDS  DC      0	; Number of words in destination command packet
HDR	DC      0	; 24-bit header containing board's header
R1PROC	DC	0	; Last processed value of VMEbus pointer R1
R2PROC	DC	0	; Last processed value of fiber link pointer R2

; Interrupt service routine register values
SV_Y1	DC	0

;  Constants from here on
ZERO    DC      0
ONE	DC	1
TWO     DC      2               ; Two words in each Header
THREE	DC	3		; Used for translating word to byte addresses
C32     DC      32              ; Constant for using circular buffer
C300    DC      $300		; Constant for resetting the DSP
TWOS    DC      32768           ; For two's complement conversion
C50000	DC	50000		; Delay for WRROM = 5 millisec
CFFFF   DC      $FFFF           ; Constant for resetting the DSP
TIMEOUT	DC	5000		; Timeout period for commands
COM_DLY	DC	500		; Time for serial transmitter and timing bd.
HST_RPL	DC	$000102		; Header for host computer, before LSR's
TIMING	DC	$010202         ; Header for a timing board command
MASK1   DC      $FCFCF8         ; Mask for Header test
MASK2   DC      $030300         ; Mask for Header test
SBRDHDR	DC      $010000         ; Source Identification number
DBRDHDR	DC      $000100         ; Destination Identification number
SMASK   DC      $FF0000         ; Mask to get source board number out
DMASK   DC      $00FF00         ; Mask to get destination board number out
RCV_HDR	DC	$00AC00		; Header on words received from host computer
AM_REG	DC	$F0000D		; Address modifier contents
VME_HDR	DC	$010002		; Header to host from VME on startup
ERR     DC      'ERR'           ; Error message (unrecognized command)
DON     DC      'DON'           ; Done message
ABR	DC	'ABR'		; Abort readout command to timing board
TIM	DC	'TIM'		; Complete command not received withing timeout
AFE	DC	'AFE'		; Address format error message


; *****  Command table  *****
;  Command table resident in X: data memory
;  The first part of the command table is not defined for "bootrom"
;     because it contains application-specific commands

	ORG	X:COM_TBL,P:COM_TBL+LD_X

	DC	0,START,0,START,0,START,0,START	; Space for 32 application 
	DC	0,START,0,START,0,START,0,START	;   commands
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	DC	'SRA',SRADDR	; Set reply address
	DC      'TDL',TDL	; Test Data Link               
	DC      'RDM',RDMEM	; Read DSP or EEPROM memory
	DC	'WRM',WRMEM	; Write DSP or EEPROM memory 
	DC	'LDA',LDAPPL	; Load application program from EEPROM      
	DC      'ERR',START	; Nothing special
	DC	'RRS',RMT_RST	; Remote reset of timing and utility boards
	DC	0,START		; Room for one more "boot" command

;  End of program
        END
