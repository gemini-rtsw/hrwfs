/* hkbaja.h - Heurikon Baja CPU board header file */

/* Copyright 1995-1996 Heurikon Corporation */

/*
modification history
--------------------
01e,30apr96,tjf  changed values for Baja68k SCID control registers
01d,30apr96,tjf  changed |'s to +'s to keep 68k assembler happy
01c,02apr96,tjf  changed the VIC IPL ACK pointers back to UINT8 *
01b,20sep95,tjf  changed VIC IPL ACK pointers to UINT32 *
01a,01sep95,tjf  written by Ted Friedl, Heurikon Corporation
*/

/*
This file contains I/O address and related constants for the
Heurikon Baja.
*/

#ifndef INChkbajah
#define INChkbajah

#if (CPU == R4000)
#define IOBASE        0xb0000000
#define PIG_BASE_ADRS (IOBASE+0x0fc00000)
#else  /* CPU == R4000 */
#define IOBASE        0xf0000000
#define K0BASE        0x00000000
#define K1BASE        0x00000000
#endif /* CPU == R4000 */

#include "drv/multi/hkpig.h"

/* -------------------- General Information --------------------------------- */

#define TARGET_HKBAJA
#define BUS                     VME_BUS

/* -------------------- RAM ------------------------------------------------- */

#ifndef _ASMLANGUAGE
#define HKBAJA_RAM_BASE_ADRS   ((UINT8 *)(K0BASE+0x00000000))
#else
#define HKBAJA_RAM_BASE_ADRS   (K0BASE+0x00000000)
#endif

/* possible RAM sizes */
#define HKBAJA_RAM_MSIZ_8M     0x00800000
#define HKBAJA_RAM_MSIZ_16M    0x01000000
#define HKBAJA_RAM_MSIZ_32M    0x02000000
#define HKBAJA_RAM_MSIZ_64M    0x04000000

/* -------------------- ROM ------------------------------------------------- */
/* note that addresses of EPROM and FLASH0 can be swapped by jumper */

#ifndef _ASMLANGUAGE
#define HKBAJA_ROM_BASE_ADRS   ((UINT8 *)(IOBASE+0x0d000000))
#else
#define HKBAJA_ROM_BASE_ADRS   (IOBASE+0x0d000000)
#endif

/* -------------------- Flash ROMs ------------------------------------------ */
/* note that addresses of EPROM and FLASH0 can be swapped by jumper */

#define HKBAJA_FLASH0          (IOBASE+0x0c000000)
#define HKBAJA_FLASH1          (IOBASE+0x0d400000) /* optional */
#define HKBAJA_FLASH2          (IOBASE+0x0d800000) /* optional */
#define HKBAJA_FLASH3          (IOBASE+0x0dc00000) /* optional */

#define HKBAJA_FLASH0_RO_SIZE  0x00080000
#define ENET_ADDR_OFFSET       0x0007fff0  /* Flash 0 Enet address here */
#if (CPU == R4000)
#define HKMON_ENTRY           (IOBASE+0x0c000008)  /* monitor entry point */
#else  /* CPU == R4000 */
#define HKMON_ENTRY           (IOBASE+0x0c000008)  /* monitor entry point */
#endif /* CPU == R4000 */

/* -------------------- Interrupt Vectors ----------------------------------- */

#define HKBAJA_ERR_VEC_BASE      ((UINT8)  0x40)
#define HKBAJA_VEC_ACFAIL        ((UINT8)  0x40)
#define HKBAJA_VEC_WRT_PST_FAIL  ((UINT8)  0x41)
#define HKBAJA_VEC_ARB_TOUT      ((UINT8)  0x42)
#define HKBAJA_VEC_SYSFAIL       ((UINT8)  0x43)
#define HKBAJA_VEC_VME_INT_IACK  ((UINT8)  0x44)
#define HKBAJA_VEC_DMA           ((UINT8)  0x45)

#define HKBAJA_VEC_ICGS_0        ((UINT8)  0x70)
#define HKBAJA_VEC_ICGS_1        ((UINT8)  0x71)
#define HKBAJA_VEC_ICGS_2        ((UINT8)  0x72)
#define HKBAJA_VEC_ICGS_3        ((UINT8)  0x73)

#define HKBAJA_VEC_ICMS_0        ((UINT8)  0x80)
#define HKBAJA_VEC_ICMS_1        ((UINT8)  0x81)
#define HKBAJA_VEC_ICMS_2        ((UINT8)  0x82)
#define HKBAJA_VEC_ICMS_3        ((UINT8)  0x83)

#define HKBAJA_VEC_MBOX          (HKBAJA_VEC_ICMS_0)

#define HKBAJA_VEC_LOCAL_BASE    ((UINT8)  0x90)
#define HKBAJA_VEC_ENET          (         0x91) /* compiler warning when cast UINT8 */
#define HKBAJA_VEC_PIG           ((UINT8)  0x92)
#define HKBAJA_VEC_PCI2          ((UINT8)  0x93)
#define HKBAJA_VEC_PCI1          ((UINT8)  0x94)
#define HKBAJA_VEC_PLX           ((UINT8)  0x95)
#define HKBAJA_VEC_RTC_SWITCH    ((UINT8)  0x96)
#define HKBAJA_VEC_PARITY_ERROR  ((UINT8)  0x97)

/* -------------------- VIC chip -------------------------------------------- */

#define VIC_BASE_ADRS             (IOBASE+0x0f000000)
#define VIC_REG_INTERVAL          4

#if (CPU == R4000)

#define VIC_VME_SYS_BUS_TAS_DISABLE

#define HKBAJA_VIC_IPL1_ACK_ADRS ((volatile UINT8 *)(IOBASE+0x0fff0005))
#define HKBAJA_VIC_IPL2_ACK_ADRS ((volatile UINT8 *)(IOBASE+0x0fff0009))

#define HKBAJA_BUSERR_LATCH      ((volatile UINT32 *)(IOBASE+0x0ffe0000))

#endif /* CPU == R4000 */

/* -------------------- Intel 82596 Chip ------------------------------------ */
#define HKBAJA_EI_CA             ((volatile ULONG *)(IOBASE+0x0f0103a0))
#define HKBAJA_EI_PORT           ((volatile ULONG *)(IOBASE+0x0f0103a4))
#define HKBAJA_EI_ABTCLR         ((volatile ULONG *)(IOBASE+0x0f0103ac))

/* -------------------- Dallas DS1286 Watchdog Timekeeper Chip -------------- */
#define DS1286_BASE              (IOBASE+0x0f010600)
#define DS1286_INTERVAL          8

#define HKBAJA_SWITCH_ADDR       (IOBASE+0x0f010000) /* switch latches as D1 here */
#define HKBAJA_SWITCH_MASK       0x02                /* bit D1 has switch information */
#define HKBAJA_SWITCH_CLEAR      (IOBASE+0x0fe00000) /* long word access clear switch */

/* -------------------- Xicor X24C16 serial EEPROM (NVRAM) Chip ------------- */
#define X24C16_CLOCK_ADDR         ((ULONG *)(IOBASE+0x0fe20000))
#define X24C16_DATA_ADDR          ((ULONG *)(IOBASE+0x0fe10000))
#define X24C16_OUTPUT_ADDR        ((ULONG *)(IOBASE+0x0f010000))

/* -------------------- PCI9060 PCI Bridge Chip ----------------------------- */
#define PCI9060_BASE_ADRS         (IOBASE+0x0f020000)

/* -------------------- SCID Chip ------------------------------------------- */
#if (CPU == R4000)
#define HKBAJA_SCID_CONTROL_0    (IOBASE+0x0f010520)
#define HKBAJA_SCID_CONTROL_1    (IOBASE+0x0f010560)
#else /* (CPU == R4000) */
#define HKBAJA_SCID_CONTROL_0    (IOBASE+0x0f010518)
#define HKBAJA_SCID_CONTROL_1    (IOBASE+0x0f010538)
#endif /* (CPU == R4000) */

/* -------------------- VMEBus Control -------------------------------------- */

#define HKBAJA_VME_LOCAL_ADRS      (unsigned char *)(IOBASE+0x0f010400)
#define HKBAJA_VME_STD_MAP         (unsigned char *)(IOBASE+0x0f010408)
#define HKBAJA_VME_BASE_ADRS       (unsigned char *)(IOBASE+0x0f010410)
#define HKBAJA_VME_EXT_ENABLE      (unsigned long *)PIG_CNTL_BIT11
#define HKBAJA_VME_STD_ENABLE      (unsigned long *)PIG_CNTL_BIT10
#define HKBAJA_VME_SHT_ENABLE      (unsigned long *)PIG_CNTL_BIT9
#define HKBAJA_MBOX_ADRS_PLACEMT   ((unsigned char *)(IOBASE+0x0f010418))
#define HKBAJA_NUMBER_MBOXES       8
#define HKBAJA_DEFAULT_BP_MBOX     0x0021
#define HKBAJA_MBOX_ADDR_BIAS      0xc000
#define HKBAJA_VME_STANDARD_ADRS   ((UINT8 *)(IOBASE+0x0e000000))
#define HKBAJA_VME_STANDARD        ((ULONG)0x00ffffff)
#define HKBAJA_VME_SHORT_ADRS      ((UINT8 *)(IOBASE+0x0f040000))
#define HKBAJA_VME_SHORT           ((ULONG)0x0000ffff)

/* -------------------- Default Baud Rate ----------------------------------- */
#define HKBAJA_BAUD_RATE           9600

/* -------------------- User LEDs ------------------------------------------- */
#define HKBAJA_LED                 ((UINT8)0xff)
#define HKBAJA_LED_ADRS            ((UINT8 *)(IOBASE+0x0f010100))
#define HKBAJA_LED_OFF             ((UINT8)0x00)
#define HKBAJA_LED_ON              ((UINT8)0xff)
#define HKBAJA_LED_POWERUP         HKBAJA_LED_ON

#endif /* INChkbajah */
