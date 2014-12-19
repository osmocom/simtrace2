#ifndef _BOARD_
#define _BOARD_

/**     Headers     */
#include "chip.h"

/**     Board       */
#include "board_lowlevel.h"
#include "uart_console.h"
#include "iso7816_4.h"
#include "led.h"

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

#define BOARD_MAINOSC 12000000
#define BOARD_MCK     48000000

#define LED_RED PIO_PA17
#define LED_GREEN PIO_PA18

#define PIN_LED_RED     {LED_RED, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define PIN_LED_GREEN   {LED_GREEN, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
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

#define SIM_PWEN_PIN            {PIO_PA5, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}

#define SW_SIM      PIO_PA8
#define SMARTCARD_CONNECT_PIN {SW_SIM, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_EDGE }
//#define SMARTCARD_CONNECT_PIN {SW_SIM, PIOB, ID_PIOB, PIO_INPUT, PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_EDGE}

/// PIN used for reset the smartcard
//#define RST_SIM     (1 << 7)
#define RST_SIM     (1 << 7)
// FIXME: Card is resetted with pin set to 0 --> PIO_OUTPUT_1 as default is right?
#define PIN_ISO7816_RSTMC       {RST_SIM, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_PULLUP}

/// Pins used for connect the smartcard
#define PIN_SIM_IO      {PIO_PA1, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
#define PIN_SIM_IO2      {PIO_PA6, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
#define PIN_SIM_CLK     {PIO_PA2, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
#define PIN_SIM_CLK2     {PIO_PA4, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
//#define PINS_ISO7816            PIN_USART1_TXD, PIN_USART1_SCK, PIN_ISO7816_RSTMC
#define PINS_ISO7816            PIN_SIM_IO, PIN_SIM_IO2, PIN_SIM_CLK, PIN_SIM_CLK2, PIN_ISO7816_RSTMC, SIM_PWEN_PIN

#endif
