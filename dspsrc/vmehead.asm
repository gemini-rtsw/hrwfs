	COMMENT *
SDSU2 VME Interface Board Header
Instrument: Gemini WFS
Revision: 3.09 (matches corresponding boot code version)
(This code is adapted from vmeboot v3.00 written by Dr. Bob Leach at SDSU)

    (c) 2002				(c) 2002
    National Research Council		Conseil national de recherches
    Ottawa, Canada, K1A 0R6 		Ottawa, Canada, K1A 0R6
    All rights reserved			Tous droits reserves

    NRC disclaims any warranties,	Le CNRC denie toute garantie
    expressed, implied, or statu-	enoncee, implicite ou legale,
    tory, of any kind with respect	de quelque nature que se soit,
    to the software, including		concernant le logiciel, y com-
    without limitation any war-		pris sans restriction toute
    ranty of merchantability or		garantie de valeur marchande
    fitness for a particular pur-	ou de pertinence pour un usage
    pose.  NRC shall not be liable	particulier.  Le CNRC ne
    in any event for any damages,	pourra en aucun cas etre tenu
    whether direct or indirect,		responsable de tout dommage,
    special or general, consequen-	direct ou indirect, particul-
    tial or incidental, arising		ier ou general, accessoire ou
    from the use of the software.	fortuit, resultant de l'utili-
					sation du logiciel.


Modifications:

98/05/22 TDH -initial coding
             -changed REM_RST to C80_RST
             -changed APL_ADR to $150

98/06/16 TDH -added N_W_APL, the EEPROM space allotment for each application
             -increased APL_LEN to allow for longer applications

98/10/23 TDH -no changes necessary for boot code version 3.03

99/01/06 TDH -no changes necessary for boot code version 3.04

99/03/02 TDH -no changes necessary for boot code version 3.05

99/05/11 TDH -no changes necessary for boot code version 3.06

99/10/13 TDH -added port B definitions for previously undefined pins
             -changed port B bit 6 definition to monitor interrupts (requires
              hardware modification)
             -changed port B bit 10 definition to monitor FIFO full flag
              (requires hardware modification)
             -changed APL_ADR to $155 to accomodate larger boot code

00/02/28 TDH -fixed error in port C DDR setting (CDD_SET)

2000/10/16 TDH -no changes necessary for boot code version 3.09

	*


; Common addresses
RST_ISR	EQU	$00	; Hardware reset interrupt 
ROM_ID  EQU     $06     ; Location of ROM Identification words = SWI interrupt
IRQA_ISR EQU    $08     ; Address of ISRB receiver for VMEbus commands
START   EQU     $0A	; Program starting address
BUF_STR	EQU	$50	; Starting address of buffers in X:
BUF_LEN	EQU	$20	; Length of buffers
FO_BUF	EQU	BUF_STR		; Starting address of FO buffer in X:
VME_BUF	EQU	FO_BUF+BUF_LEN	; Starting address of VMEbus buffer in X:
COM_BUF EQU     VME_BUF+BUF_LEN	; Starting address of command buffer in X:
COM_TBL EQU     COM_BUF+BUF_LEN ; Starting address of command table in X:
NUM_COM	EQU     40		; Number of entries in the command table
APL_ADR	EQU	$155	; P: memory location where application code begins
APL_LEN	EQU	$400	; Maximum length of application program
N_W_APL	EQU	$F00	; Total number of EEPROM words per application
LD_X	EQU	$4200	; Assembler loads boot X: memory into EEPROM starting
			; at this address (word address)
RD_X	EQU	$C600	; Address where X: memory is in "boot" area of EEPROM
			; (byte address)
ROM_OFF	EQU	$4000	; Boot program offset address in EEPROM

; DSP register addresses
BCR     EQU     $FFFE   ; Bus (=Port A) Control Register -> Wait States
PCC     EQU     $FFE1   ; Port C Control Register
PCDDR   EQU     $FFE3	; Port C Data Direction Register
PCD     EQU     $FFE5	; Port C Data Register
CRA     EQU     $FFEC   ; SSI Control Register A
CRB     EQU     $FFED   ; SSI Control Register B
SSISR   EQU     $FFEE   ; SSI Status Register
SSITX	EQU     $FFEF   ; SSI Receive Transmitter
PBC     EQU     $FFE0   ; Port B Control Register
PBDDR   EQU     $FFE2   ; Port B Data Direction Register
PBD     EQU     $FFE4   ; Port B Data Register
PCTL	EQU	$FFFD	; PLL control register
IPR     EQU     $FFFF   ; Interrupt Priority Register

; Port B bits
BDD_SET	EQU	%001000110110100	; Data direction (1 = output)
BLT	EQU	0	; Set if VMEbus block transfer is in progress
C80_RST	EQU	1	; Remote reset of 'C80 port (mapped to pin 17 of CON24)
MODE	EQU	2	; Clear for 16-bit image data, set for 24-bit 
			; command/reply data
AUX1	EQU	3	; Unused
H4	EQU	4	; Value of data strobe signal DS0/I on VME bus cycles
H5	EQU	5	; Value of data strobe signal DS1/I on VME bus cycles
IRQ_TST	EQU	6	; Interrupt request status (mapped to pin5 of U22)
REQBUS	EQU	7	; Clear to request ownership of VMEbus
AUX2	EQU	8	; Set to use frame sync bit
AUX3	EQU	9	; Unused
FF	EQU	10	; Cleared to indicate the input FIFO is full
HASBUS	EQU	11	; Set if board is bus master
WL	EQU	12	; Set to write 32-bit VMEbus data
DONEVME EQU	13	; Set if the VME transfer is complete
HF	EQU	14	; Cleared to indicate the input FIFO is half full

; Port C bits
CDD_SET	EQU	%000100000	; Data direction (1 = output)
BCLR	EQU	0	; VMEbus arbitration signal BCLR
BR0	EQU	1	; VMEbus arbitration signal BR0
BR1	EQU	2	; VMEbus arbitration signal BR1
BR2	EQU	3	; VMEbus arbitration signal BR2
BR3	EQU	4	; VMEbus arbitration signal BR3
FSTART	EQU	5	; Set for start of frame (mapped to pin 23 of CON24)
EF	EQU	7	; Cleared to indicate the input FIFO is empty 

; SSI status register bits
SSI_TDE	EQU	6	; Transmitter data register empty
SSI_RDF	EQU	7	; SSI Receiver data register full

; Board status bits, defined at X:<STATUS = X:0
RD      EQU     0       ; Set if reading out
SRA_EX  EQU     2       ; Set if SRA has been executed
ST_ISR	EQU	3	; Set if fiber optic, cleared if VMEbus

; Board software options bits, defined at X:<OPTIONS = X:1
INTR    EQU     0       ; Set if program is to generate a VMEbus interrupt 
                        ;  at the end of reply and image buffer service
TWO_CMP EQU     1       ; Set to change all input data to two's complement

; DSP addresses for executing various functions - mapped to X: memory
WRAM	EQU	$FFB0   ; Write memory mapped numbers
RDFIFO	EQU	$FFB1	; Read 16-bit word from FIFO
WRHADR	EQU	$FFB2   ; Write high VME address bits A16-A31
WRHDATA EQU     $FFB3   ; Write high data bits D16-D31 if in D32 mode
RDCOM	EQU	$FFB4   ; Read data D00-D23 from VMEbus to DSP
RQINTR  EQU     $FFB5   ; Request VMEbus interrupt service
RSTWDT	EQU	$FFB6	; Write to reset watch dog timer
RSTFIFO	EQU	$FFB7	; Reset FIFO
SELBLT	EQU	$FFB8	; Select block transfer mode
SETRDFIFO EQU	$FFB9	; Set to execute automatic read FIFO cycle
CLRRDFIFO EQU	$FFBA	; Clear automatic read FIFO cycling mode

