	COMMENT *
Gemini WFS VME Interface Board Header
Controller: SDSU2 (preproduction PALs)
Revision: 3.03 (matches corresponding boot code version)
(This code is adapted from vmeboot v3.00 written by Dr. Bob Leach at SDSU)

98/05/22 TDH -initial coding
             -changed REM_RST to C80_RST
             -changed APL_ADR to $150

98/06/16 TDH -added N_W_APL, the EEPROM space allotment for each application
             -increased APL_LEN to allow for longer applications

98/10/23 TDH -no changes necessary for boot code version 3.03


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
APL_ADR	EQU	$150	; P: memory location where application code begins
APL_LEN	EQU	$400	; Maximum length of application program
N_W_APL	EQU	$F00	; Total number of EEPROM words per application
LD_X	EQU	$4200	; Assembler loads boot X: memory into EEPROM starting at this address (word address)
RD_X	EQU	$C600	; Address where X: memory is in "boot" area of EEPROM (byte address)
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
BLT	EQU	0	; Set to enable block transfer mode to the VME bus
C80_RST	EQU	1	; Remote reset of 'C80 port (mapped to pin 17 of CON24)
MODE	EQU	2	; Clear for 16-bit image data, set for 24-bit 
			; command/reply data
REQBUS	EQU	7	; Clear to request ownership of VMEbus
HASBUS	EQU	11	; Set if board is bus master
WL	EQU	12	; Set to write 32-bit VMEbus data
DONEVME EQU	13	; Set if the VME transfer is complete
HF	EQU	14	; Cleared to indicate the input FIFO is half full

; Port C bits
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

