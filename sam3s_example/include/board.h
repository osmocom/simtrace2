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
#include "USBD.h"

/**     Highlevel   */
#include "trace.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#ifdef __GNUC__ 
#undef __GNUC__ 
#endif

/** Name of the board */
#define BOARD_NAME "SAM3S-SIMTRACE"
/** Board definition */
#define simtrace
/** Family definition (already defined) */
#define sam3s
/** Core definition */
#define cortexm3

#define BOARD_MAINOSC 18432000 
#define BOARD_MCK     48000000

#define LED_RED PIO_PA17
#define LED_GREEN PIO_PA18

#define PIN_LED_RED     {LED_RED, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define PIN_LED_GREEN   {LED_GREEN, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define PINS_LEDS       PIN_LED_RED, PIN_LED_GREEN 

#define LED_NUM_RED     0
#define LED_NUM_GREEN   1

#define IO_SW               PIO_PA19
#define PIN_PUSHBUTTON      {IO_SW, PIOA, ID_PIOA, PIO_INPUT, PIO_DEGLITCH | PIO_PULLUP}

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

#define SIM_PWEN        PIO_PA5
#define VCC_FWD         PIO_PA26

#define SIM_PWEN_PIN            {PIO_PA5, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}

#define SW_SIM      PIO_PA8
#define SMARTCARD_CONNECT_PIN {SW_SIM, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP | PIO_DEBOUNCE | PIO_DEGLITCH | PIO_IT_EDGE }
//#define SMARTCARD_CONNECT_PIN {SW_SIM, PIOB, ID_PIOB, PIO_INPUT, PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_EDGE}

/// PIN used for reset the smartcard
//#define RST_SIM     (1 << 7)
#define RST_SIM     (1 << 7)
// FIXME: Card is resetted with pin set to 0 --> PIO_OUTPUT_1 as default is right?
#define PIN_ISO7816_RSTMC       {RST_SIM, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}

/// Pins used for connect the smartcard
//#define PIN_SIM_IO      {PIO_PA1, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
#define PIN_SIM_IO      {PIO_PA6, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
#define PIN_SIM_CLK     {PIO_PA2, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
//#define PIN_SIM_CLK2     {PIO_PA4, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
//#define PINS_ISO7816            PIN_USART1_TXD, PIN_USART1_SCK, PIN_ISO7816_RSTMC
#define PINS_ISO7816            PIN_SIM_IO,  PIN_SIM_CLK,  PIN_ISO7816_RSTMC // SIM_PWEN_PIN, PIN_SIM_IO2, PIN_SIM_CLK2

//**     SPI interface   **/
/// SPI MISO pin definition (PA12).
#define PIN_SPI_MISO   {1 << 12, PIOA, PIOA, PIO_PERIPH_A, PIO_PULLUP}
/// SPI MOSI pin definition (PA13).
#define PIN_SPI_MOSI   {1 << 13, PIOA, PIOA, PIO_PERIPH_A, PIO_DEFAULT}
/// SPI SPCK pin definition (PA14).
#define PIN_SPI_SPCK   {1 << 14, PIOA, PIOA, PIO_PERIPH_A, PIO_DEFAULT}
/// SPI pins definition. Contains MISO, MOSI & SPCK (PA12, PA13 & PA14).
#define PINS_SPI       PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_SPCK
/// SPI chip select 0 pin definition (PA11).
#define PIN_SPI_NPCS0  {1 << 11, PIOA, PIOA, PIO_PERIPH_A, PIO_DEFAULT}

//**     USB **/
/// USB pull-up control pin definition (PA16).
#define PIN_USB_PULLUP  {1 << 16, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}

// Board has UDP controller
#define BOARD_USB_UDP
// D+ has external pull-up
#define BOARD_USB_PULLUP_EXTERNAL

#define BOARD_USB_NUMENDPOINTS      8

// FIXME: in all other cases return 0?
#define BOARD_USB_ENDPOINTS_MAXPACKETSIZE(i)    (((i == 4) || (i == 5))? 512 : 64)
#define BOARD_USB_ENDPOINTS_BANKS(i)            (((i == 0) || (i == 3)) ? 1 : 2)

/// USB attributes configuration descriptor (bus or self powered, remote wakeup)
//#define BOARD_USB_BMATTRIBUTES                  USBConfigurationDescriptor_SELFPOWERED_NORWAKEUP
#define BOARD_USB_BMATTRIBUTES                  USBConfigurationDescriptor_BUSPOWERED_NORWAKEUP
//#define BOARD_USB_BMATTRIBUTES                  USBConfigurationDescriptor_SELFPOWERED_RWAKEUP

#define ATMEL_VENDOR_ID             0x03EB
#define SIMTRACE_PRODUCT_ID         0x6004
//#define OPENPCD_VENDOR_ID           0x16c0
//#define SIMTRACE_PRODUCT_ID         0x0762
#define USB_VENDOR_ID               OPENPCD_VENDOR_ID
#define USB_PRODUCT_ID              SIMTRACE_PRODUCT_ID


#endif
