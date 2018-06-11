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

#define BOARD_MCK     48000000

#define PIO_LED_RED	PIO_PA17
#define PIO_LED_GREEN	PIO_PA17

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
/** Console baudrate always using 115200. */
#define CONSOLE_BAUDRATE    115200
/** Usart Hw interface used by the console (UART0). */
#define CONSOLE_USART       UART0
/** Usart Hw ID used by the console (UART0). */
#define CONSOLE_ID          ID_UART0
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

#define SIM_PWEN        PIO_PA5
#define VCC_FWD         PIO_PA26


//**     USB **/
// USB pull-up control pin definition (PA16).
// Default: 1 (USB Pullup deactivated) 
#define PIN_USB_PULLUP  {1 << 16, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}

// Board has UDP controller
#define BOARD_USB_UDP
// D+ has external pull-up
#define BOARD_USB_PULLUP_EXTERNAL

#define BOARD_USB_NUMENDPOINTS      8

// FIXME: in all other cases return 0?
#define BOARD_USB_ENDPOINTS_MAXPACKETSIZE(i)    (((i == 4) || (i == 5))? 512 : 64)
#define BOARD_USB_ENDPOINTS_BANKS(i)            (((i == 0) || (i == 3)) ? 1 : 2)

#define USB_VENDOR_OPENMOKO		0x1d50
#define USB_PRODUCT_OWHW_SAM3_DFU	0x4001	/* was 0x4000 */
#define USB_PRODUCT_OWHW_SAM3		0x4001
#define USB_PRODUCT_QMOD_HUB		0x4002
#define USB_PRODUCT_QMOD_SAM3_DFU	0x4004	/* was 0x4003 */
#define USB_PRODUCT_QMOD_SAM3		0x4004
#define USB_PRODUCT_SIMTRACE2_DFU	0x60e3	/* was 0x60e2 */
#define USB_PRODUCT_SIMTRACE2		0x60e3

#define BOARD_USB_DFU
#define BOARD_DFU_BOOT_SIZE	(16 * 1024)
#define BOARD_DFU_RAM_SIZE	(2 * 1024)
#define BOARD_DFU_PAGE_SIZE	512
#define BOARD_DFU_NUM_IF	2

extern void board_exec_dbg_cmd(int ch);
extern void board_main_top(void);
extern int board_override_enter_dfu(void);
#endif
