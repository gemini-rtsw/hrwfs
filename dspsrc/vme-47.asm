       COMMENT *
Gemini WFS VME Interface Board Application Code
Controller: SDSU2 
Revision: 0.30  (must agree with status word V_SW_ID in Y: memory)
(This code is adapted from vme1.asm written by Dr. Bob Leach at SDSU)


22/06/98 - added ICD-compliant FBA handling
	 - changed mask operations to bit checks
	 - moved DON reply of RDC to before readout loop
	 - eliminated some redundant initialization at beginning of RDC
		because not using hardware transfers

24/06/98 - shuffled code around to put VME bus write instructions in 
		internal memory (necessary due to "feature" of VME board)
	 - added code to write packet counter and frame status at beginning
		of frame in VME memory

25/06/98 - added a masking operation to some of the frame header words 
		because of the uncertainty in b18-23 from the FIFO.
	 - added code to recognize and handle sync bit errors (not tested
		since timing board doesn't generate them properly yet).

26/06/98 - changed the buffer content computation to properly deal with the
		modulo $2000 buffer
	 - added code to check if already reading out when RDC received and
		send an ERR if so.

30/06/98 - added code to generate frame interrupts
	 - added some comments, and renamed the parameters to match the ICD
	 - adjustments to help keep up with FIFO data rate

10/07/98 - shuffled code around to make reading loop more efficient and
		keep up with the FIFO
	 - added buffer overflow handling

13/07/98 - added code for abort (ABT) command

14/07/98 - added code to handle synced RDCs (new RDC before last one is
		finished)

16/07/98 - changed to packetized reads from FIFO
	 - added check for 64k boundary when writing to VME bus 
	 - added packet interrupts and a flag word to disable interrupts

20/07/98 - shuffled code segments to ensure that all VME bus writes occurred
		below P:$200
	 - fixed bug in sync bit handling

22/07/98 - added checksum accumulation and value check

23/07/98 - changed to "burst" VME transfers instead of requesting bus for
		each pixel

24/07/98 - changed to use R6 as internal buffer pointer
	 - only change WRAM when command received and change HADDR to
		reply value at the same time instead of restoring it after 
		every VME transfer
	 - fixed bug in packet check so that now it will do the end of 
		frame read if there is exactly a packet left.

27/07/98 - changed to use R7 as internal buffer content indicator

30/07/98 - added code to jump out of packet reading loops if FIFO is empty

30/07/98 - moved transmission of frame status words to the GET_FBA routine
	 - made interrupt generation a subroutine
	 - other code shuffles to ensure the VME writes in GET_FBA were
		below P:$200
 
31/07/98 - changed overflow handling to make use of R7

03/08/98 - changed to packet-sized bursts and continuing readout while
		waiting for VMEbus mastership
	 - fixed bugs with command processing during readout by repeating
		some commmand parsing code and adding a save of the checksum
	 - fixed bug in overflow handling

05/08/98 - added comments, moved some code segments around
	 - added timeout to abort sequence and header read so it won't get 
		stuck waiting for data

13/11/98 - changed NUM_FSW from 10 to 8 to reflect latest ICD 1.6
	 - removed DEBUG statements
 
17/11/98 - added the timeout feature to the abort function where it 
		waits for the checksum of the initial partial frame

08/12/98 - corrected the processing of the frame counter in the 
		header from the timing board
	 - fixed the self-consistency check of command headers
	 - changed the abort function to expect an empty (no pixel
		data) frame as the last frame 
	 - got rid of OF_FRM flag (better to use the EOF_PXL flag to
		achieve same thing)

16/12/98 - fixed discard loop in abort function
	 - altered timeout operation and added TIM reply if there is a
		timeout
	 - added lines to reduce the wait states for external P: memory
		during time-critical sections of program code in 
		external memory (abort and overflow discard loops)

07/01/99 - added 'TIM' reply constant to Y data

02/03/99 - wait state changes removed, since they are now done in boot code
		(version 3.05)
         - removed TIM reply from command parsing
	 - removed carry bit setting from CHK_FO routine



Assembler directives:

-d DOWNLOAD 1	To generate code for downloading to DSP memory.
-d DOWNLOAD 0	To generate code for writing to the EEPROM.

	*

; Define listing page width
	PAGE	112

; Assembler options
	OPT	RC,NOPP	; Float comment column location in listing
			; Do not align fields

; Define memory limits
	IF	DOWNLOAD
	HIMEM	P:$1FFF		; P: memory $0 - $1FFF
	HIMEM	X:$1FFF		; X: memory $0 - $1FFF
	HIMEM	Y:$3FFF		; Y: memory $0 - $3FFF
	ELSE
	HIMEM	PR:$1FFF	; P: runtime memory $0 - $1FFF
	HIMEM	XR:$1FFF	; X: runtime memory $0 - $1FFF
	HIMEM	YR:$3FFF	; Y: runtime memory $0 - $3FFF
	LOMEM	PL:$4000
	HIMEM	PL:$CFFF	; EEPROM P: load memory $4000 - $CFFF
	HIMEM	XL:0		; No EEPROM X: load memory
	HIMEM	YL:0		; No EEPROM Y: load memory
	ENDIF

; Name section so it doesn't conflict with other application programs
	SECTION	VMERDFIFO

APL_NUM	EQU	1	; Application number from 1 to 4

; Include definitions from header file
	INCLUDE "vmehead.asm"


;************************************************************************
;									*
;    Address register assignments					*
;	R1 - Fibre optic receiver buffer pointer (last received entry)	*
;	   - Used for internal image buffer during readout		*
;	R2 - VMEbus receiver buffer pointer (last received entry)	*
;	R3 - Command buffer pointer (last received entry)		*
;	R4 - Command buffer pointer (last processed entry)		*
;	R5 - Used for temporary storage of VME/FO buffer pointer	*
;		during command processing (last processed entry)	*
;	R1-5 are set to modulo 32 by boot code
;									*
;	R0 - used for temporary storage all over the place, but		*
;		N0 set to 2 by boot code and must not be changed.	*
;	R6 - Internal image buffer pointer (last processed entry)	*
;	R7 - Internal image buffer content				*
;									*
;************************************************************************

; Specify execution and load addresses
	IF	DOWNLOAD
	ORG	P:APL_ADR,P:APL_ADR		; Download address
	ELSE
        ORG     P:APL_ADR,P:APL_NUM*N_W_APL	; EEPROM generation
	ENDIF


; ***** Return to readout  *****
; Restore address modifier and checksum after executing a command. The
; location P:APL_ADR is defined in the boot code as the place to jump to
; when it is finished processing a command, if in the middle of a readout.

RDING	JCLR	#DONEVME,X:PBD,* 	; Ensure last VMEbus operation finished
	MOVE	#$F0008D,X0	; Set D7 = LWORD* high for 16-bit transfers 
	MOVE	X0,X:WRAM	;   to VMEbus
	MOVE	Y:CS_SAVE,B	; Restore checksum


; *****  Main readout loop  *****
; Loop around, checking for various action items with the following priority: 
; 1) Bus mastership granted -> transfer packet of data to VME memory
; 2) Data in FIFO -> read packet of data from FIFO into the internal buffer
; 3) Complete packet accumulated in the internal buffer -> request bus
; 4) Command received from host -> process command

; Check if we have been granted mastership of the VME bus
CHK_BUS	JSET	#HASBUS,X:PBD,XFR_PKT	; Transfer packet if VMEbus master

; Check for data in the fibre optic receiver FIFO
CHK_EF	JSET	#EF,X:PCD,READ_FO	; Read the FO receiver data, if any

; Check if there is a packet in the buffer to send to the host
	JCLR	#REQBUS,X:PBD,CHK_BUS	; Skip check if already waiting for bus
	MOVE	R7,A		; Get buffer content
	MOVE	Y:<PSIZE,Y0
	CMP	Y0,A		; Compare content to packet size
	JGE	<PKT_RDY	; Request bus if there's a packet in buffer
	JSET	#LST_FRM,Y:<FLAGS,PKT_RDY	; Request bus if all data rxed

; Check for commands from the host
CHK_VME	JSR	<VME_TST	; Test for incoming VMEbus commands (boot code)
	JNE	<CMD_RCV	; A VMEbus command was received, so parse it

	JMP	<CHK_BUS	; Loop around again


; *****  Read data from fibre optic receiver  *****
; Read header or image data from the fibre optic receiver, first ensuring
; that there is enough room in the internal buffer. Data is read a packet
; at a time, unless there is less than a packet reamining in the frame.

READ_FO	JSET	#HD_WAIT,Y:<FLAGS,FRM_HDR	; Check if waiting for header

; Check if there is room for another packet in the buffer
	MOVE	R7,X1			; Get content of buffer
	MOVE	#IB_SIZE,A		; Get buffer size
	SUB	X1,A Y:<PSIZE,Y0	; Compute buffer space
	CMP	Y0,A			; Compare space to packet size
	JLT	<BUF_OF			; Buffer overflow if less

; Check number of pixels remaining in frame
	MOVE	Y:<PIXELC,A	; Get number of remaining pixels
	SUB	Y0,A		; Subtract packet size
	JLE	RD_EOF		; Jump if less than a packet remains
	MOVE	A,Y:<PIXELC	; Save decremented pixel counter

; Read a packet of data from FIFO
	DO	Y0,L_RDPKT
	JCLR	#EF,X:PCD,Q_MT1		; Exit packet loop if FIFO empty
	MOVE	X:RDFIFO,Y0		; Get pixel datum from the FIFO
	MOVE	X:(R7)+,X0 Y0,Y:(R1)+	; Increment buffer content (dummy move)
					; and put pixel in internal image buffer
	JCLR	#SYNC_B,Y0,NO_SB1		; Check for sync bit
	JCLR	#SYNC_MD,Y:<FLAGS,NO_SB1	; Skip if not in sync mode
	ENDDO
	JMP	<SB_RCV			; Go handle premature sync bit
Q_MT1	MOVEC	LC,Y0			; Get loop counter
	ADD	Y0,A			; Add to pixel counter
	MOVE	A,Y:<PIXELC		; Save pixel counter
	ENDDO
	JMP	<CHK_BUS
NO_SB1	ADD	Y0,B			; Accumulate checksum
L_RDPKT
	JMP	<PKT_RDY

; Read end of frame from FIFO
RD_EOF	DO	Y:<PIXELC,L_RDEOF
	JCLR	#EF,X:PCD,Q_MT2		; Exit loop if FIFO empty
	MOVE	X:RDFIFO,Y0		; Get pixel datum from the FIFO
	JSET	#DSC_PXL,Y:<FLAGS,CHK_SB	; Don't buffer data if flag set
	MOVE	X:(R7)+,X0 Y0,Y:(R1)+	; Increment buffer content (dummy move)
					; and put pixel in internal image buffer
CHK_SB	JCLR	#SYNC_B,Y0,NO_SB2		; Check for sync bit
	JCLR	#SYNC_MD,Y:<FLAGS,NO_SB2	; Skip if not in sync mode
	ENDDO
	JMP	<SB_RCV			; Go handle premature sync bit
Q_MT2	MOVEC	LC,Y:<PIXELC		; Save loop counter as pixel counter
	ENDDO
	JMP	<CHK_BUS
NO_SB2	ADD	Y0,B			; Accumulate checksum
L_RDEOF
	CLR	A
	MOVE	A,Y:<PIXELC		; Clear pixel counter
	JMP	<EOF_RCV


; *****  Transfer packet  *****
; Set up the VME bus for 16 bit image data transfers, write a packet into
; VME memory and update the packet counter.
; IMPORTANT: the VME bus write operation will NOT work if it occurs in 
; memory above P:$1FD because of external memory access in the fetch cycle

XFR_PKT	MOVE	Y:<FBAHISV,A
	MOVE	A,X:WRHADR		; Set HADDR to FBA value
	MOVE	Y:<FBALOSV,R0		; Set up R0 as VME address pointer

	DO	Y:<PSIZE,L_BURST	; Loop through packet burst transfer
	MOVE	Y:(R6)+,Y0		; Get datum from internal buffer
	MOVE	X:(R7)-,A Y0,Y:(R0)+	; Decrement buffer content (dummy move)
					; and 16-bit write pixel datum to VMEbus
	NOP				; Keep DSP address around for awhile
	JSCLR	#15,R0,CHG_HI		; If 64k boundary increment HIADDR
	JCLR	#EOF_PXL,Y0,NOT_EOF	; Check if it was last pixel of frame
	ENDDO				; Cancel burst DO loop
NOT_EOF	NOP				; DO loop restriction
L_BURST

	BSET	#REQBUS,X:PBD		; Release VMEbus mastership
	MOVE	R0,Y:<FBALOSV		; Save last VME frame buffer location	

; Increment and send packet counter
	JCLR	#DONEVME,X:PBD,* ; Make sure the last VME operation has finished
	MOVE	Y:<PCHIADD,A
	MOVE	A,X:WRHADR	; Set HADDR to packet counter value
	MOVE	Y:<PCLOADD,R0	; Set R0 to low word of packet counter address
	MOVE	X:<ONE,X0 
	MOVE	Y:<PKTC,A
	ADD	X0,A		; Increment packet counter
	MOVE	A,Y:<PKTC	; Save packet counter
	MOVE	A,Y:(R0)+	; Send packet counter to VME buffer
	NOP			; Keep DSP address around for awhile

	JSET	#EOF_PXL,Y0,EOF_XMT	; Check if it was last packet of frame

; Send packet interrupt
	MOVE	Y:<V_PIID,A			; Get packet interrupt ID
	JSSET	#PI_EN,Y:<V_INT_EN,GEN_INT	; Only if packet int. enabled

	JMP	<CHK_EF			; Return to loop


; *****  Complete frame transmitted  *****
; When a complete frame has been transferred to the VME frame buffer, send the
; frame status, generate a frame interrupt, and set up for the next frame.

; Send frame status
EOF_XMT	JSCLR	#15,R0,CHG_HI	; If 64k boundary increment HIADDR
	CLR	A
	MOVE	A,Y:<PKTC	; Reset packet counter
	MOVE	(R0)+		; Skip high word (b16-31) of frame status
	JSCLR	#15,R0,CHG_HI	; If 64k boundary increment HIADDR
	MOVE	Y:(R6)+,Y0	; Get frame status word
	JCLR	#DONEVME,X:PBD,* 	; Make sure the VMEbus is available
	MOVE	X:(R7)-,A Y0,Y:(R0)	; Decrement buffer content (dummy move)
					; and xfer frame status to VME memory
	NOP			; Keep DSP address around for awhile

; Send frame interrupt
	MOVE	Y:<V_FIID,A			; Get frame interrupt ID
	JSSET	#FI_EN,Y:<V_INT_EN,GEN_INT	; Only if frame int. enabled

	BCLR	#FBA_VLD,Y:<FLAGS	; Clear flag: FBA is no longer valid

; Check if last frame has been transferred to VME memory
	JCLR	#LST_FRM,Y:<FLAGS,CHK_VME ; Keep reading if last frame not rxed
	MOVE	R7,A		; Get buffer content
	TST	A		; Compute buffer content	
	JGT	<CHK_VME	; Keep reading if internal buffer not empty
	JMP	<RD_DONE	; Finish readout 


; *****  Get new FBA  *****
; Check if FBA has been updated by host. If so, save an adjusted version, 
; transmit the initial frame status words and set the appropriate flags.

GET_FBA	JCLR	#NEW_FBA,Y:<V_FBAHI,CHK_VME	; Skip if no new FBA 
;	BCLR	#NEW_FBA,Y:<V_FBAHI		; Clear new FBA flag bit

	MOVE	Y:<V_FBAHI,A	; Get high word of FBA
	MOVE	A,Y:FBAHISV	; Save high word
	JCLR	#DONEVME,X:PBD,* ; Make sure the last VME operation has finished
	MOVE	A,X:WRHADR	; Set HADDR to FBA value

	MOVE	Y:<V_FBALO,A	; Get low word of FBA
	LSR	A		; Convert VMEbus byte address to word address
	BSET	#15,A		; Put address in top of Y: memory space
	MOVE	A,R0		; Set up R0 as VME address pointer

; Transmit frame status words to VME bus
	JSET	#EOF_PXL,Y:(R6),CLR_OF	; Skip if FSWs were discarded
	BCLR	#REQBUS,X:PBD		; Request to be VMEbus master
	JCLR	#HASBUS,X:PBD,* 	; Wait for VMEbus

	DO	#NUM_FSW,L_XFSW		; Loop through status words
	MOVE	Y:(R6)+,Y0		; Get datum from internal buffer
	MOVE	X:(R7)-,A Y0,Y:(R0)+	; Decrement buffer content (dummy move)
					; and 16-bit write pixel datum to VMEbus
	NOP				; Keep DSP address around for awhile
	JSCLR	#15,R0,CHG_HI		; If 64k boundary increment HIADDR
	NOP				; DO loop restriction
L_XFSW
	BSET	#REQBUS,X:PBD		; Release VMEbus mastership
	JMP	<SAVE_LO

; Check for external memory problem
	IF 	@CVS(N,*)>=$200
        WARN    'VME bus write operations will not work at > P:$1FD'	
	ENDIF	

CLR_OF	MOVE	#NUM_FSW,A 	; get number of frame status words
	MOVE	R0,Y0		; get frame buffer pointer
	ADD	Y0,A
	MOVE	A,R0		; skip over status words in frame buffer
	JSCLR	#15,R0,CHG_HI		; If 64k boundary increment HIADDR

SAVE_LO	MOVE	R0,Y:<FBALOSV		; Save last VME frame buffer location	
	BSET	#FBA_VLD,Y:<FLAGS	; Set flag that FBA is now valid

; Save address of packet counter
	MOVE	Y:<V_FBAHI,Y0	; Get high word of FBA
	MOVE	Y:<V_FBALO,A	; Get low word of FBA
	LSR	A X:<ONE,X0	; Convert VMEbus byte address to word address
	BSET	#15,A		; Put address in top of Y: memory space
	ADD	X0,A		; Increment address past zero high word
	CLR	A A,Y:PCLOADD	; Save low address
	JSET	#15,Y:PCLOADD,CMP_HI	; Check for 64k boundary
	BSET	#15,Y:PCLOADD	; LADR = $8000
	MOVE	X0,A
CMP_HI	ADD	Y0,A		; Compute high address
	MOVE	A,Y:PCHIADD	; Save high address

	JMP	<CHK_EF



; *****  Packet ready  *****
; Make sure we have a valid FBA to write to, then request mastership of the
; VME bus.

PKT_RDY	JCLR	#FBA_VLD,Y:<FLAGS,GET_FBA	; Ensure current FBA is valid
	JCLR	#DONEVME,X:PBD,* ; Make sure the last VME operation has finished
	BCLR	#REQBUS,X:PBD			; Request to be VMEbus master
	JMP	<CHK_BUS


; *****  Change high address  *****
; Increment high word of FBA if a 64k boundary is encountered

CHG_HI	MOVE	Y:<FBAHISV,A	; Get high word of address
	MOVE	X:<ONE,X1
	ADD	X1,A		; Increment
	MOVE	A,Y:<FBAHISV	; Save new address
	JCLR	#DONEVME,X:PBD,*	; Make sure the VMEbus is available
	MOVE	A,X:WRHADR	; Set high word of current VMEbus address
	BSET	#15,R0		; LADR = $8000
	RTS


; ***** Generate VME Interrupt *****
; Generate an interrupt on the VME IRQ (level determined by jumpers) with
; the ID given in accumulator A

GEN_INT	JCLR	#DONEVME,X:PBD,* ; Make sure the last VME operation has finished
	MOVE	Y:<AM_16B,Y0
	OR	Y0,A
	MOVE	A,X:WRAM	; Set interrupt ID
	MOVE	A,X:RQINTR	; Request interrupt
	RTS


; *****  Process frame header  *****
; Read the frame header values from the FIFO, set up the associated 
; parameters and discard the pipeline garbage values.

; Insert zeroes for the initial values of packet counter and frame status
FRM_HDR	CLR	A
	REP	#NUM_FSW-4
	MOVE	X:(R7)+,X0 A,Y:(R1)+	; Increment buffer content (dummy move)
					; and put datum in internal image buffer

; Word 0 = parameter ID
	MOVE	Y:<MSK_16B,Y1	; Get mask for incoming data
	CLR	A X:RDFIFO,Y0	; Get parameter ID from the FIFO
	ADD	Y0,B X:(R7)+,X1 A,Y:(R1)+	; Accumulate checksum, 
						; increment buffer content
						; and zero high word of PARMID
	MOVE	X:(R7)+,A Y0,Y:(R1)+	; Increment buffer content (dummy move)
					; and put datum in internal image buffer
	BCLR	#SYNC_MD,Y:<FLAGS
	JSET	#SYNC_B,Y0,GET_FC	; Set sync mode flag if Msb of PARMID=0
	BSET	#SYNC_MD,Y:<FLAGS

; Word 1|2 = frame counter
GET_FC	JSCLR	#EF,X:PCD,CHK_FO		; Wait for datum
	JSET	#TIM_OUT,Y:<FLAGS,ABT_DON	; Timeout
	MOVE	X:RDFIFO,A	; Get frame count high byte (b15-23) from FIFO
	AND	Y1,A		; Mask off lower 16 bits
	ADD	A,B		; Accumulate checksum
	ASR	A		; Shift right to align bytes
	MOVE	X:(R7)+,X1 A,Y:(R1)+	; Increment buffer content (dummy move)
					; and put datum in internal image buffer
	REP	#16
	ASL	A		; Shift into b15-23
	MOVE	A1,X0		; Temporary store of high byte
	JSCLR	#EF,X:PCD,CHK_FO		; Wait for datum
	JSET	#TIM_OUT,Y:<FLAGS,ABT_DON	; Timeout
	MOVE	X:RDFIFO,A	; Get frame count low word (b0-14) from FIFO
	AND	Y1,A		; Mask off lower 16 bits
	ADD	A,B		; Accumulate checksum
	OR	X0,A1		; Insert high byte
	MOVE	A1,Y:<FRAMEC	; Save frame counter
	AND	Y1,A		; Mask off lower 16 bits
	MOVE	X:(R7)+,X1 A,Y:(R1)+	; Increment buffer content (dummy move)
					; and put datum in internal image buffer

; Word 3 = number of outputs
	JSCLR	#EF,X:PCD,CHK_FO		; Wait for datum
	JSET	#TIM_OUT,Y:<FLAGS,ABT_DON	; Timeout
	MOVE	X:RDFIFO,A	; Get number of outputs from the FIFO
	AND	Y1,A		; Mask off lower 16 bits
	ADD	A,B		; Accumulate checksum
	MOVE	A,Y:<NOUTS	; Save number of outputs

; Word 4|5 = total number of pixels 
	JSCLR	#EF,X:PCD,CHK_FO		; Wait for datum
	JSET	#TIM_OUT,Y:<FLAGS,ABT_DON	; Timeout
	MOVE	X:RDFIFO,A	; Get pixel count high byte from the FIFO
	AND	Y1,A		; Mask off lower 16 bits
	ADD	A,B		; Accumulate checksum
	REP	#15
	LSL	A		; Shift into b15-22
	MOVE	A1,X0		; Temporary store of high byte
	JSCLR	#EF,X:PCD,CHK_FO		; Wait for datum
	JSET	#TIM_OUT,Y:<FLAGS,ABT_DON	; Timeout
	MOVE	X:RDFIFO,A	; Get pixel count low word from the FIFO
	AND	Y1,A		; Mask off lower 16 bits
	ADD	A,B		; Accumulate checksum
	OR	X0,A1		; Insert high byte
	MOVE	A1,Y:<PIXELC 	; Initialize pixel counter

; Words 6 -> (3*NOUTS)+6 = pipeline garbage values
	MOVE	X:<THREE,X0	; Get number of garbage pixels per output
	MOVE	Y:<NOUTS,Y0	; Get number of outputs
	MPY	X0,Y0,A		; Calculate number of garbage pixels
	ASR	A		; Correct for 24-bit multiply
	DO	A0,L_GRBG
	JSCLR	#EF,X:PCD,CHK_FO		; Wait for datum
	JCLR	#TIM_OUT,Y:<FLAGS,RD_GBG	; Read datum if available
	ENDDO			; Timeout - exit loop
	JMP	<ABT_DON
RD_GBG	MOVE	X:RDFIFO,X0	; Get garbage pixel from the FIFO
	ADD	X0,B		; Accumulate checksum
L_GRBG
	BCLR	#HD_WAIT,Y:<FLAGS	; Complete header received - clear flag
	JMP	<CHK_BUS		; Return to readout loop


; *****  Sync bit received  *****
; A sync bit has been received before the pixel counter reached zero. If in 
; sync mode, flag the error and assume the frame has finished. If in discard
; pixels mode, this is the end of the discarded pixels. 

SB_RCV	JSET	#DSC_PXL,Y:<FLAGS,REC_FS	; End of discarded pixels
	BSET	#SYN_ERR,Y:<FSTAT	; Set error flag
	MOVE	X:(R7)-,A Y:(R1)-,Y0	; Decrement buffer pointer and content
	BSET	#EOF_PXL,Y:-(R1)	; Flag last pixel
	CLR	A (R1)+			; Restore buffer pointer
	MOVE	A,Y:<PIXELC		; Clear pixel counter
	JMP	<REC_FS			; Do end of frame processing


; *****  Buffer overflow  *****
; The internal buffer has overflowed. Discard whatever remains of the oldest
; frame and mark the frame status.

BUF_OF	MOVE	Y:(R6)+,Y0	; Discard at least one pixel
	MOVE	X:(R7)-,X0	; Decrement buffer content (dummy move)
 
	DO	R7,L_DSC
	JCLR	#EOF_PXL,Y:(R6)+,NOT_LST	; Look for last pixel of frame
	BSET	#OF_ERR,Y:(R6)-			; Set error flag
	MOVEC	LC,R7		; Save loop counter as new buffer content
	ENDDO			; Abort discard loop
	JMP	<CHK_BUS	; Return to reading loop
NOT_LST	NOP
L_DSC
	CLR	A
	MOVE	A,R7			; Clear content indicator
	BSET	#OF_ERR,Y:<FSTAT	; Set error flag
	JMP	<CHK_BUS		; Return to reading loop


; *****  Last pixel received  *****
; The pixel counter has reached zero. Flag the last pixel in the internal 
; buffer, read and verify the checksum, record the frame status, and set up
; for the next frame.

EOF_RCV	BSET	#EOF_PXL,Y:-(R1)	; Flag last pixel
	MOVE	(R1)+			; Restore buffer pointer

; Get checksum	
	JCLR	#EF,X:PCD,*	; Wait for the datum to be ready
	MOVE	X:RDFIFO,A	; Get checksum from the FIFO

; Check sync bit of checksum
	JCLR	#SYNC_MD,Y:<FLAGS,CHK_CS	; Skip if not in sync mode
	JSET	#SYNC_B,A,CHK_CS	; Continue if sync bit set 
	BSET	#SYN_ERR,Y:<FSTAT	; Set error flag
	BSET	#DSC_PXL,Y:<FLAGS	; Set flag to discard remaining pixels
	JMP	<CHK_BUS		; Return to reading loop

; Check value of checksum
CHK_CS	MOVE	Y:MSK_15B,Y0		; Get bit mask ($7FFF)
	AND	Y0,B
	AND	Y0,A B1,X1
	JEQ	REC_FS			; Allow checksum=0 (not calculated)
	CMP	X1,A			; Compare checksum
	JEQ	REC_FS			; Skip if equal
	BSET	#CS_ERR,Y:<FSTAT	; Set error flag

; Record frame status
REC_FS	BCLR	#DSC_PXL,Y:<FLAGS	; Clear discard pixels flag
	CLR	B Y:<FSTAT,A	; Clear checksum, get frame status
	CLR	A A,Y:(R1)+	; Store frame status at end of frame in buffer
	MOVE	X:(R7)+,X0	; Increment buffer content (dummy move)
	MOVE	A,Y:<FSTAT	; Reset frame status

; Check frame counter
	MOVE	X:<ONE,X0
	MOVE	Y:<FRAMEC,A		; Get frame counter
	CMP	X0,A
	JNE	<NEW_HD			; Check if last frame (counter = 1)
	JSET	#RD_SYNC,Y:<FLAGS,NEW_RDC	; Keep reading if synced RDC
	BSET	#LST_FRM,Y:<FLAGS	; Set flag that last frame received
	JMP	<PKT_RDY

NEW_HD	BSET	#HD_WAIT,Y:<FLAGS	; Set flag to wait for new frame header
	JMP	<PKT_RDY

NEW_RDC	BCLR	#RD_SYNC,Y:<FLAGS	; Clear synced RDC flag
	BSET	#HD_WAIT,Y:<FLAGS	; Set flag to wait for new frame header
	JMP	<PKT_RDY


; *****  Begin CCD Readout  *****
; Put VME interface board in CCD readout mode, send an RDC to the timing board 
; and send a DON to the host.

RDCCD	BCLR	#MODE,X:PBD	; Fiber optic shift register set for 16-bit 

; Send RDC command to the timing board
	MOVE	Y:<TIM_HDR,A	; Header from VME to timing
	JSR	XMT_WRD		; send to timing board
	MOVE	Y:<RDC,A	; RDC mnemonic
	JSR	XMT_WRD		; send to timing board

	JSET	#RD,X:<STATUS,RDC_SNC	; Check if already reading out
	BSET    #RD,X:<STATUS		; Set status to reading out

; Clear checksum accumulator, flags, pixel, packet and frame counters
	CLR	B
	MOVE	B,Y:<FLAGS
	MOVE	B,Y:<PIXELC
	MOVE	B,Y:<PKTC
	MOVE	B,Y:<FRAMEC

; Initialize internal image buffer
	MOVE	#IB_MOD,M1		; Set pointer moduli
	MOVE	#IB_MOD,M6
	MOVE	#INT_BUF,R1		; Initialize pointers
	MOVE	R1,R6
	MOVE	B,R7			; Initialize content indicator

; Set wait for frame header flag
	BSET	#HD_WAIT,Y:<FLAGS	; Set flag

; Store a copy of the packet size so that it is fixed for duration of readout
	MOVE	Y:<V_PSIZE,A	; Get packet size
	MOVE	A,Y:<PSIZE	; Save size

; Send DON to host
RET_DON	MOVE	X:<HST_RPL,A
	MOVE	A,X:<HDR	; Host computer header	
	JMP	<FINISH		; Issue a nice 'DON' message

; Synced RDC (a second RDC received during readout)
RDC_SNC	JSET	#LST_FRM,Y:<FLAGS,CLR_LF	; Check if last read finished
	BSET	#RD_SYNC,Y:<FLAGS	; Set flag for synced RDC
	JMP	<RET_DON		; Send DON to host
CLR_LF	BCLR	#LST_FRM,Y:<FLAGS	; Clear flag that last frame received
	BSET	#HD_WAIT,Y:<FLAGS	; Set flag to wait for new header
	JMP	<RET_DON		; Send DON to host


; *****  Readout done  *****
; All done with the readout, so clean up and return

RD_DONE	BCLR    #RD,X:<STATUS   ; Clear status to not reading out
	BSET	#MODE,X:PBD	; Fiber optic shift register set for 24-bit 
	BCLR	#LST_FRM,Y:<FLAGS	; Clear flag that last frame received

; Restore R1 for use as FIFO command buffer
	MOVE	#FO_BUF,R1	; Starting address of FIFO buffer
	MOVE	R1,X:<R1PROC
	MOVE	#31,M1

; Restore VME bus to command and reply mode
	JCLR	#DONEVME,X:PBD,* ; Make sure the last VME operation has finished
	MOVE	X:<AM_REG,X0	; Restore normal 32-bit transfers to VMEbus 
	MOVE	X0,X:WRAM   
	MOVE	X:HRPLADR,A
	MOVE	A,X:WRHADR	; Set HADDR to reply value

	JMP	<START


; *****  Command received  *****
; A command has been received during readout. Save registers, setup VME
; bus, do first part of command parsing. First bit of parsing is done here
; because the boot code version accesses the FIFO.

CMD_RCV	MOVE	B,Y:CS_SAVE	; Save checksum
	JCLR	#DONEVME,X:PBD,* ; Make sure the last VME operation has finished
	MOVE	X:<AM_REG,X0	; Restore normal 32-bit transfers to VMEbus 
	MOVE	X0,X:WRAM   
	MOVE	X:HRPLADR,A
	MOVE	A,X:WRHADR	; Set HADDR to reply value

; Check header (S,D,N) for self-consistency. Only commands that come 
; from the host are valid, since the FO receiver is busy with image data.
	MOVE	X:(R5),Y0	; Get candidate header
	MOVE    Y:<HDR_MSK,A1	; Test for S=0 and D<=3 and N<=7
	AND     Y0,A  X:<MASK2,B1
        JNE     <HDR_ERR	; Test failed 
        AND     Y0,B  #7,A1	; Test for S>0 or D>0
       	JEQ     <HDR_ERR	; Test failed (S and D both zero)
        AND     Y0,A  		; Test for N>=1
        JNE     <GET_CMD	; Test succeeded - process command

; If a header value is wrong then reset the buffer pointers
HDR_ERR	MOVE	#VME_BUF,R2	; Starting address of VMEbus buffer
	MOVE	R2,X:<R2PROC
	JMP	<START		; Return to START (in boot code)

; Get all the words of the command before processing it
GET_CMD	MOVE	A,Y0		; Number of words in command header
	DO	X:<TIMEOUT,L_TO
	MOVE	R2,A			
	SUB	X0,A		; X0 = R#PROC from VME_TST or FO_TST
        JGE     <CMP_NW		; X1 = Destination mask $00FF00
        MOVE    X:<C32,X1	; Correct for circular buffer
        ADD     X1,A		; No MOVE here - it isn't always executed
CMP_NW	CMP	Y0,A  Y0,X:<NWORDS ; Y0 = NWORDS from above
	JLT	<NOT_ALL	; Keep looping if not all words received
	ENDDO
	JMP	<MV_COM		; Go process the command (boot code)
NOT_ALL	NOP
L_TO
	MOVE	(R5)+		; Increment R5 past header
SND_TIM	JMP	<START		; Timeout - ignore command


; *****  Abort  *****
; Abort readout in progress. Discard any internally buffered data, send an
; ABT command to the timing board, wait for timing board to send a frame 
; numbered 1, then cleanup and send a DON to the host.

; Make sure a readout is in progress
ABORT	JCLR	#RD,X:<STATUS,RET_DON

; Send abort command to timing board
	MOVE	Y:<TIM_HDR,A	; Header from VME to timing
	JSR	XMT_WRD		; send to timing board
	MOVE	Y:<ABT,A	; Abort mnemonic
	JSR	XMT_WRD		; send to timing board

; Discard rest of frame
DSC_FRM	MOVE	Y:<PIXELC,A	; Get number of remaining pixels
	TST	A
	JEQ	<CHK_FC
	MOVE	Y:MAX_LP,Y0	; Get maximum loop count
	SUB	Y0,A X:<ONE,X0	; Subtract loop count
	JGT	<SAV_PC		; Jump if more than MAX_LP remains
	MOVE	Y:<PIXELC,A	; Get number of remaining pixels
	ADD	X0,A		; Add one for checksum
	MOVE	A,Y0
	CLR	A		; Set pixel count to zero
SAV_PC	MOVE	A,Y:<PIXELC	; Save decremented pixel counter

; Read a bunch of data from FIFO and discard
	DO	Y0,L_ADSC
	JSCLR	#EF,X:PCD,CHK_FO		; Wait for datum
	JCLR	#TIM_OUT,Y:<FLAGS,RD_PXL1	; Read datum if available
	ENDDO			; Timeout - exit loop
	JMP	<ABT_DON
RD_PXL1	MOVE	X:RDFIFO,X0	; Get discard pixel from the FIFO
	JCLR	#SYNC_B,X0,NO_SB3		; Check for sync bit
	JCLR	#SYNC_MD,Y:<FLAGS,NO_SB3	; Skip if not in sync mode
	ENDDO
	JMP	<CHK_FC		; frame finished - check frame counter
NO_SB3	NOP
L_ADSC
	JMP	<DSC_FRM	; continue discarding

; Check frame counter
CHK_FC	MOVE	X:<ONE,X0
	MOVE	Y:<FRAMEC,A	; Get frame counter
	CMP	X0,A
	JEQ	<ABT_DON	; Check if last frame (counter = 1)

; Read header of next frame
	JSCLR	#EF,X:PCD,CHK_FO		; Wait for datum
	JSET	#TIM_OUT,Y:<FLAGS,ABT_DON	; Timeout
	MOVE	Y:<MSK_16B,Y1	; Get mask for incoming data
	CLR	A X:RDFIFO,X0	; Get parameter ID from the FIFO

	JSCLR	#EF,X:PCD,CHK_FO		; Wait for datum
	JSET	#TIM_OUT,Y:<FLAGS,ABT_DON	; Timeout
	MOVE	X:RDFIFO,A	; Get frame count high byte from the FIFO
	AND	Y1,A		; Mask off lower 16 bits
	REP	#15
	LSL	A		; Shift into b15-23
	MOVE	A1,X0		; Temporary store of high byte
	JSCLR	#EF,X:PCD,CHK_FO		; Wait for datum
	JSET	#TIM_OUT,Y:<FLAGS,ABT_DON	; Timeout
	MOVE	X:RDFIFO,A	; Get frame count low word from the FIFO
	AND	Y1,A		; Mask off lower 16 bits
	OR	X0,A1		; Insert high byte
	MOVE	A,Y:<FRAMEC	; Save frame counter

	JSCLR	#EF,X:PCD,CHK_FO		; Wait for datum
	JSET	#TIM_OUT,Y:<FLAGS,ABT_DON	; Timeout
	MOVE	X:RDFIFO,A	; Get number of outputs from the FIFO
	AND	Y1,A		; Mask off lower 16 bits
	MOVE	A,Y:<NOUTS	; Save number of outputs

	JSCLR	#EF,X:PCD,CHK_FO		; Wait for datum
	JSET	#TIM_OUT,Y:<FLAGS,ABT_DON	; Timeout
	MOVE	X:RDFIFO,A	; Get pixel count high byte from the FIFO
	AND	Y1,A		; Mask off lower 16 bits
	REP	#15
	LSL	A		; Shift into b15-22
	MOVE	A1,X0		; Temporary store of high byte
	JSCLR	#EF,X:PCD,CHK_FO		; Wait for datum
	JSET	#TIM_OUT,Y:<FLAGS,ABT_DON	; Timeout
	MOVE	X:RDFIFO,A	; Get pixel count low word from the FIFO
	AND	Y1,A		; Mask off lower 16 bits
	OR	X0,A1		; Insert high byte
	MOVE	A,Y:<PIXELC 	; Initialize pixel counter
	TST	A
	JEQ	<ABT_DON	; Last frame is empty 

	MOVE	X:<THREE,X0	; Get number of garbage pixels per output
	MOVE	Y:<NOUTS,Y0	; Get number of outputs
	MPY	X0,Y0,A		; Calculate number of garbage pixels
	ASR	A		; Correct for 24-bit multiply
	DO	A0,L_GBGDSC
	JSCLR	#EF,X:PCD,CHK_FO		; Wait for datum
	JCLR	#TIM_OUT,Y:<FLAGS,DSC_GBG	; Read datum if available
	ENDDO			; Timeout - exit loop
	JMP	<ABT_DON
DSC_GBG	MOVE	X:RDFIFO,X0	; Get garbage pixel from the FIFO
L_GBGDSC
	JMP	<DSC_FRM	; Go discard image data

; Cleanup and return
ABT_DON	BCLR    #RD,X:<STATUS   ; Clear status to not reading out
	BSET	#MODE,X:PBD	; Fiber optic shift register set for 24-bit 
	BCLR	#FBA_VLD,Y:<FLAGS	; Clear flag: FBA is no longer valid
	BCLR	#LST_FRM,Y:<FLAGS	; Clear flag that last frame received

; Restore R1 for use as FIFO command buffer
	MOVE	#FO_BUF,R1	; Starting address of FIFO buffer
	MOVE	R1,X:<R1PROC
	MOVE	#31,M1

; Restore VME bus to command and reply mode
	JCLR	#DONEVME,X:PBD,* ; Make sure the last VME operation has finished
	MOVE	X:<AM_REG,X0	; Restore normal 32-bit transfers to VMEbus 
	MOVE	X0,X:WRAM   
	MOVE	X:HRPLADR,A
	MOVE	A,X:WRHADR	; Set HADDR to reply value

; Send DON to host
	MOVE	X:<HST_RPL,A
	MOVE	A,X:<HDR	; Host computer header	
	JCLR	#TIM_OUT,Y:<FLAGS,FINISH	; Issue a DON message
	BCLR	#TIM_OUT,Y:<FLAGS		; clear timeout flag
	MOVE	Y:<TIM,X0	; Reply will be Timeout
	JMP	<FINISH1	; Send reply


; *****  Check FIFO  *****
; Check the fibre optic receiver for data, but give up if it remains empty for
; a long time.

CHK_FO	DO	Y:FO_TO,L_FTO	; Check FO_TO times for data 
	JCLR	#EF,X:PCD,FO_MT	; Wait for the datum to be ready
	ENDDO			; Abort loop
	RTS
FO_MT	DO	#250,DLY	 ; Wait 20 usec
	NOP
	NOP
DLY	NOP
L_FTO
	BSET	#TIM_OUT,Y:<FLAGS	; Set timeout bit
	RTS


; *****  Debug routines  *****
; Use IDL command to execute various debugging routines

IDLE	JMP	<FINISH		; send DON


;  **********************    End of application    ************************

; Check that the application program memory space P: is not too large
	IF 	@CVS(N,*)>=(APL_ADR+APL_LEN)
	WARN    'Application P: is too large!'	; Prevent program overflow
	ENDIF



; ******************************   X Data   *******************************

; Command table resident in X: data memory
	IF	DOWNLOAD	; Memory offsets for downloading code
	ORG	X:COM_TBL,X:COM_TBL
	ELSE
        ORG     X:COM_TBL,P:APL_NUM*N_W_APL+APL_LEN
	ENDIF
	DC      'RDC',RDCCD     ; Put VME board in CCD readout mode
        DC      'ABT',ABORT     ; Abort Readout
        DC      'DON',START     ; Do nothing
        DC      'IDL',IDLE      ; send IDL to timing (for debug purposes only)
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	DC	0,START,0,START,0,START,0,START
	


; *****************   Y Data (Defined in ICD 1.6/1.10)   ******************

; Parameter definitions for application specific Y: memory
	IF	DOWNLOAD
	ORG	Y:0,Y:0	; Download address
	ELSE
        ORG     Y:0,P:		; EEPROM address continues from P: above
	ENDIF


; *****  Status values  *****

V_SW_ID	DC	$003000	; Software version number (00.30)


; *****  Readout parameters  *****

V_PSIZE	DC	1024	; packet size

V_INT_EN	DC	$000000	; Interrupt enable flags
PI_EN		EQU	0	; Packet interrupt enable
FI_EN		EQU	1	; Frame interrupt enable

V_PIID		DC	$F10000	; Packet interrupt ID
V_FIID		DC	$F20000	; Frame interrupt ID

V_FBAHI	DC	0	; Frame Buffer address high word
V_FBALO	DC	0	; Frame Buffer address low word



; *************************   Y Data (Internal)   *************************

PSIZE	DC	1024	; Packet size (in use copy)

PIXELC	DC	0	; Pixel counter
PKTC	DC	0	; Packet counter
FRAMEC	DC	0	; Frame counter

FBAHISV	DC	0	; FBA high word currently in use for image transfer
FBALOSV	DC	0	; FBA low word (after bit adjustments)

PCHIADD	DC	0	; VME address of packet counter (beginning of frame)
PCLOADD	DC	0

NUM_FSW	EQU	8	; # of (16 bit) frame status words at start of frame

FLAGS	DC	0	; Flag word in memory
; Bit definitions
HD_WAIT	EQU	0	; Set if waiting for a frame header
SYNC_MD	EQU	1	; Set if in sync bit mode
FBA_VLD	EQU	2	; Set if current frame buffer address is valid
LST_FRM	EQU	3	; Set if last frame has been received from timing board
DSC_PXL	EQU	4	; Set if remaining pixels of frame are to be discarded
RD_SYNC	EQU	5	; Set if an RDC is received during readout (synced RDC)
TIM_OUT	EQU	6	; Set if timeout occurred during abort

FSTAT	DC	0	; Frame status word
; Bit definitions
OF_ERR	EQU	0	; Internal buffer overflow error
CS_ERR	EQU	1	; Checksum error
SYN_ERR	EQU	2	; Frame sync error

; Other bit definitions
SYNC_B	EQU	15	; Sync bit in image data (b15)
NEW_FBA	EQU	23	; b23 of V_FBAHI is used to indicate a new FBA
EOF_PXL	EQU	16	; b16 is used to mark the last pixel of a frame


; Miscellaneous
NOUTS	DC	0	; Number of outputs (current frame)

MSK_16B	DC	$FFFF	; Mask for incoming 16-bit data
MSK_15B	DC	$7FFF	; Mask off lower 15 bits

AM_16B	DC	$8D	; Address modifier settings for 16 bit transfers
AM_32B	DC	$0D	; Address modifier settings for 32 bit transfers

CS_SAVE	DC	0	; Temporary storage of checksum

HDR_MSK	DC	$FFFCF8	; Mask to check command header

FO_TO	DC	$2710	; Timeout: number of times to look for data in FIFO

MAX_LP	DC	$FFFE	; Maximum loop counter value

; Constants for communication with timing board and host
TIM_HDR	DC	$010202		; header VME -> timing, 2 word command
RDC	DC	'RDC'		; mnemonic for CCD readout command
IDL	DC	'IDL'		; mnemonic for CCD idle command
ABT	DC	'ABT'		; mnemonic for CCD abort command
TIM	DC	'TIM'		; mnemonic for timeout reply


; *****  Internal image buffer  *****

IB_SIZE	EQU	$2000		; Buffer size
IB_MOD	EQU	IB_SIZE-1	; Buffer pointer modulus
INT_BUF	BSM	IB_SIZE		; Allocate and initialize memory for buffer



; Check for overflow in the EEPROM case
	IF !DOWNLOAD
		IF @CVS(N,@LCV(L))>(APL_NUM+1)*N_W_APL
        	WARN    'EEPROM overflow!'	; Make sure next application
		ENDIF				;  will not be overwritten
	ENDIF


; End of SECTION VMERDFIFO
	ENDSEC			


;  End of program
        END
