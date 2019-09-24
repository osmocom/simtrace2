/* SIMtrace 2 common board pin definitions
 *
 * (C) 2015-2017 by Harald Welte <hwelte@hmw-consulting.de>
 * (C) 2018, sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA
 */
#ifndef _BOARD_
#define _BOARD_

/**     Headers     */
#include "chip.h"
/* We need this for a nop instruction in USB_HAL.c */
#define __CC_ARM

/**     Board       */
#include "board_lowlevel.h"
#include "uart_console.h"
#include "iso7816_4.h"
#include "led.h"
#include "cciddriver.h"
#include "usart.h"
#include "USBD.h"

#include "USBD_Config.h"
#include "USBDDriver.h"

/**     Highlevel   */
#include "trace.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "inttypes.h"
#include "syscalls.h"

#define MIN(a, b)       ((a < b) ? a : b)

#ifdef __GNUC__ 
#undef __GNUC__ 
#endif

/** Family definition (already defined) */
#define sam3s
/** Core definition */
#define cortexm3

#define PIO_LED_RED     PIO_PA17
#define PIO_LED_GREEN   PIO_PA18

#define PIN_LED_RED     {PIO_LED_RED, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define PIN_LED_GREEN   {PIO_LED_GREEN, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define PINS_LEDS       PIN_LED_RED, PIN_LED_GREEN 

#define LED_NUM_RED     0
#define LED_NUM_GREEN   1

/** USART0 pin RX */
#define PIN_USART0_RXD    {PIO_PA9A_URXD0, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
/** USART0 pin TX */
#define PIN_USART0_TXD    {PIO_PA10A_UTXD0, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}

#define BOARD_PIN_USART_RXD     PIN_USART0_RXD
#define BOARD_PIN_USART_TXD     PIN_USART0_TXD

#define BOARD_ID_USART          ID_USART0
#define BOARD_USART_BASE        USART0

#define PINS_UART  { PIO_PA9A_URXD0|PIO_PA10A_UTXD0, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}

/** UART0 */
/** Console baud rate in bps */
#define CONSOLE_BAUDRATE    921600
/** UART peripheral used by the console (UART0). */
#define CONSOLE_UART       UART0
/** UART peripheral ID used by the console (UART0). */
#define CONSOLE_ID          ID_UART0
/** UART ISR used by the console (UART0). */
#define CONSOLE_ISR         UART0_IrqHandler
/** UART IRQ used by the console (UART0). */
#define CONSOLE_IRQ         UART0_IRQn
/** Pins description corresponding to Rxd,Txd, (UART pins) */
#define CONSOLE_PINS        {PINS_UART}

/// Smartcard detection pin
// FIXME: add connect pin as iso pin...should it be periph b or interrupt oder input?
#define BOARD_ISO7816_BASE_USART    USART0
#define BOARD_ISO7816_ID_USART      ID_USART0

/* USART peripherals for a phone and SIM card setup */
/* USART peripheral connected to the SIM card */
#define USART_SIM       USART0
/* ID of USART peripheral connected to the SIM card */
#define ID_USART_SIM    ID_USART0
/* Interrupt request ID of USART peripheral connected to the SIM card */
#define IRQ_USART_SIM   USART0_IRQn
/* USART peripheral connected to the phone */
#define USART_PHONE     USART1
/* ID of USART peripheral connected to the phone */
#define ID_USART_PHONE  ID_USART1
/* Interrupt request ID of USART peripheral connected to the phone */
#define IRQ_USART_PHONE USART1_IRQn

// Board has UDP controller
#define BOARD_USB_UDP

#define BOARD_USB_DFU
#define BOARD_DFU_BOOT_SIZE	(16 * 1024)
#define BOARD_DFU_RAM_SIZE	(2 * 1024)
#define BOARD_DFU_PAGE_SIZE	512
#define BOARD_DFU_NUM_IF	2

extern void board_exec_dbg_cmd(int ch);
extern void board_main_top(void);
extern int board_override_enter_dfu(void);
#endif
