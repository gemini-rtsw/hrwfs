       COMMENT *
Gemini WFS Timing Board Header
Controller: SDSU2 (preproduction PALs)
Revision: 3.01 (matches corresponding boot code version)
(This code is adapted from timEEV written by Dr. Bob Leach at SDSU)

97/10/08 BML -initial coding

98/07/20 TDH -changes for new sync bit PALs (U12/U17 Rev 4.1)

	*


; Common addresses
RST_ISR	EQU	$00	; Hardware reset interrupt 
ROM_ID  EQU     $06	; Location of program Identification = SWI interrupt
START	EQU	$08	; Starting address of program
RCV_BUF EQU     $60	; Starting address of receiver buffer in X:
COM_TBL EQU     $80     ; Starting address of command table in X: memory
NUM_COM EQU     24      ; Number of entries in command table
APL_ADR	EQU	$100	; P: memory location where application code begins
APL_LEN	EQU	$200-APL_ADR ; Maximum length of application program

ROM_OFF	EQU	$4000	; Boot program offset address in EEPROM
LD_X	EQU	$4200	; Assembler loads X: starting at this EEPROM address
RD_X	EQU	$C600	; DSP reads X: from this EEPROM address

; X: stat bits
IDLM	EQU	0	; Set if in idle mode => clocking out
ST_RCV	EQU	2	; Set if FO, cleared if SSI

; Timing board hardware addresses
RSTWDT	EQU	$6000	; Address to reset the timing board watchdog timer
WRSS	EQU	$FF80	; Write clock driver and VP switch states
RDAD0	EQU	$FFA0	; Address for reading A/D #0
RDAD1	EQU	$FFA1	; Address for reading A/D #1
RDAD2	EQU	$FFA2	; Address for reading A/D #2
RDAD3	EQU	$FFA3	; Address for reading A/D #3
WRFO	EQU	$FFC0	; Write to fiber optic transmitter
RDFO	EQU	$FFC0	; Read serial receiver fiber optic contents
WRLATCH	EQU	$FFC1	; Write to timing board latch
TCSR	EQU	$FFDE	; Timer control/status register
TCR	EQU	$FFDF	; Timer count register
PBC	EQU	$FFE0	; Port B Control Register
PCC     EQU     $FFE1   ; Port C Control Register
PBDDR	EQU	$FFE2	; Port B Data Direction Register
PCDDR	EQU	$FFE3	; Port C Data Direction Register
PBD	EQU	$FFE4	; Port B Data Register
PCD     EQU     $FFE5   ; Port C Data Register
CRA     EQU     $FFEC   ; SSI Control Register A
CRB     EQU     $FFED   ; SSI Control Regsiter B
SSISR	EQU	$FFEE	; SSI Status Register
SSITX	EQU	$FFEF	; SSI Transmit and Receive data register
SSIRX	EQU	$FFEF	; SSI Transmit and Receive data register
BCR     EQU     $FFFE   ; Bus (=Port A) Control Register -> Wait States
IPR     EQU     $FFFF   ; Interrupt Priority Register

; Port B bits
SSI_CON	EQU	0	; SSI connection (0 = utility, 1 = analog)
WW	EQU	1	; Word width (0 = 24-bit, 1 = 16-bit)
LVEN	EQU     2       ; Low voltage enable (+/-15 volt nominal)
HVEN	EQU     3       ; High voltage enable (+32V nominal)
TIO	EQU	4	; STATUS0: jumpered to Timer I/O pin
EF	EQU	9	; FIFO empty flag, low true
FD15	EQU	10	; AUX3: forced data bit (d15) value
FMODE	EQU	12	; HEN: Enable forced data (sync) bit in 16-bit WW mode
PWRST	EQU     13      ; Power control board reset

; SSI status register bits
SSI_TDE	EQU	6	; SSI Transmitter data register empty
SSI_RDF	EQU	7	; SSI Receiver data register full

; SSI control register B bits
GCK	EQU	10	; Gated clock control (0 = continuous, 1 = gated)

; Control latch (U25) bits
CDAC	EQU	0	; Set to disable clearing of DACs
DUALCLK	EQU	1	; Set to clock two halves of clock driver board together
ENCK	EQU	2	; Enable clock and DAC output switches 
TIM_U_RST EQU	5	; Timing to utility board reset
RST_FIFO EQU	7	; Reset FIFO

; Timer control/status register bits
TMR_EN	EQU	0	; Timer enable (0 = disabled, 1 = enabled)
TMR_ST	EQU	7	; Timer status (1 = timer reached zero)

; Addresses of other boards 
VIDSS	EQU	$000000	; Video processor board switch-state select = 0
VID1	EQU	$000000	; Video processor board ADC and DAC select = 0
VID2	EQU	$001000	; Video processor board ADC and DAC select = 1
CLK	EQU	$002000	; Clock driver board DAC select = 2
CLKA	EQU	$002000	; Clock driver switch state (lower 12) select = 2
CLKB	EQU	$003000	; Clock driver switch state (upper 12) select = 3
