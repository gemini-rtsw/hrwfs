       COMMENT *
SDSU2 Utility Board Header
Instrument: Gemini WFS
Revision: 3.03  (matches corresponding boot code version)
(This code is adapted from code written by Dr. Bob Leach at SDSU)

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

98/02/21 TDH -initial coding
2002/01/17 TDH - no changes necessary for boot code version 3.03

	*


;  Common addresses
RST_ISR	EQU	$00	; Hardware reset interrupt 
ROM_ID  EQU     $06     ; Location of ROM Identification words = SWI interrupt
IRQA_ISR EQU    $08     ; Address of ISRA for 1 kHz timer interrupts
SSI_ISR EQU     $0C	; SSI serial receiver interrupt address
SSI_ERR EQU     $0E	; SSI interrupt with exception (error)
START	EQU     $10     ; Address for beginning of code

BUF_STR	EQU	$80	; Starting address of buffers in X:
BUF_LEN	EQU	$20	; Length of each buffer
SSI_BUF	EQU	BUF_STR		; Starting address of SSI buffer in X:
COM_BUF EQU     SSI_BUF+BUF_LEN	; Starting address of command buffer in X:
COM_TBL EQU     COM_BUF+BUF_LEN	; Starting address of command table in X:
NUM_COM EQU     24	; Number of entries in the command table

APL_ADR	EQU	$80	; Starting address of application program
APL_XY	EQU	$1EE0	; Start of application data tables
RST_OFF	EQU	$6000	; Reset code offset in EEPROM
P_OFF	EQU	$6040	; P: memory offset into EEPROM
X_OFF	EQU	$6100	; X: memory offset into EEPROM
ROM_EXE	EQU	$6200	; P: start address for routines that execute from ROM

;  Define some useful constants
TIMEOUT	EQU	1666	; Timeout for receiving complete command = 1 millisec
DLY_MUX EQU     70      ; Number of DSP cycles to delay for MUX settling
DLY_AD  EQU     100     ; Number of DSP cycles to delay for A/D settling

; Now assign addresses to on-chip functions
BCR     EQU     $FFFE   ; Bus (=Port A) Control Register -> Wait States
PBC     EQU     $FFE0   ; Port B Control Register
PBDDR   EQU     $FFE2   ; Port B Data Direction Register
PBD     EQU     $FFE4   ; Port B Data Register
PCC     EQU     $FFE1   ; Port C Control Register
PCDDR	EQU	$FFE5	; PortC Data Direction Register
IPR     EQU     $FFFF   ; Interrupt Priority Register
SSITX	EQU	$FFEF	; SSI Transmit and Receive data register
SSIRX	EQU	$FFEF	; SSI Transmit and Receive data register
SSISR	EQU	$FFEE	; SSI Status Register
CRA     EQU     $FFEC   ; SSI Control Register A
CRB     EQU     $FFED   ; SSI Control Register B

; SSI status register bits
SSI_TDE	EQU	6	; SSI Transmitter data register empty
SSI_RDF	EQU	7	; SSI Receiver data register full

;  Addresses of memory mapped components in Y: data memory space
;  Write addresses first
WR_DIG  EQU     $FFF0   ; was $FFFF  Write Digital output values D00-D15
WR_MUX  EQU     $FFF1   ; Select MUX connected to A/D input - one of 16
EN_DIG	EQU	$FFF2	; Enable digital outputs
WR_DAC3 EQU     $FFF7   ; Write to DAC#3 D00-D11
WR_DAC2 EQU     $FFF6   ; Write to DAC#2 D00-D11
WR_DAC1 EQU     $FFF5   ; Write to DAC#1 D00-D11
WR_DAC0 EQU     $FFF4   ; Write to DAC#0 D00-D11

;  Read addresses next
RD_DIG  EQU     $FFF0   ; Read Digital input values D00-D15
STR_ADC EQU     $FFF1   ; Start ADC conversion, ignore data
RD_ADC  EQU     $FFF2   ; Read A/D converter value D00-D11
WATCH   EQU     $FFF7   ; Watch dog timer - tell it that DSP is alive

;  Bit definitions of STATUS word
ST_SRVC	EQU     0       ; Set if ADC routine needs executing
ST_EX   EQU     1       ; Set if timed exposure is in progress
ST_SH   EQU     2       ; Set if shutter is open
ST_READ EQU     3	; Set if a readout needs to be initiated
STRT_EX	EQU	4	; Set to indicate start of exposure

; Bit definitions of software OPTIONS word
OPT_SH  EQU     0       ; Set to open and close shutter

;  Bit definitions of Port B = Host Processor Interface
HVEN	EQU     0       ; Enable high voltage PS (+36V nominal)- Output
LVEN	EQU     1       ; Enable low voltage PS (+/-15 volt nominal) - Output
PWRST	EQU     2       ; Reset power conditioner counter - Output
SHUTTER EQU     3       ; Control shutter - Output
IRQ_T   EQU     4       ; Request interrupt service from timing board - Output
SYS_RST EQU     5       ; Reset entire system - Output
WATCH_T EQU     8       ; Processed watchdog signal from timing board - Input
PWREN	EQU	9	; Enable power conditioner board - Output

