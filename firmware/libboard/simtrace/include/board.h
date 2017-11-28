#pragma once
#include "board_common.h"

/** Name of the board */
#define BOARD_NAME "SAM3S-SIMTRACE"
/** Board definition */
#define simtrace

#define BOARD_MAINOSC 18432000

/** Phone (SIM card emulator)/CCID Reader/MITM configuration    **/
/*  Normally the communication lines between phone and SIM card are disconnected    */
// Disconnect SIM card I/O, VPP line from the phone lines
// FIXME: Per default pins are input, therefore high-impedance, therefore they don not activate the bus switch, right?
#define PIN_SC_SW_DEFAULT               {PIO_PA20, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
// Disconnect SIM card RST, CLK line from the phone lines
#define PIN_IO_SW_DEFAULT               {PIO_PA19, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define PINS_BUS_DEFAULT                PIN_SC_SW_DEFAULT, PIN_IO_SW_DEFAULT

/** Sniffer configuration **/
// Connect VPP, CLK and RST lines from smartcard to the phone
#define PIN_SC_SW_SNIFF        {PIO_PA20, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
#define PIN_IO_SW_SNIFF        {PIO_PA19, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
#define PINS_BUS_SNIFF          PIN_SC_SW_SNIFF, PIN_IO_SW_SNIFF

#define PINS_SIM_SNIFF_SIM      PIN_PHONE_IO,  PIN_PHONE_CLK

#define SIM_PWEN_PIN            {PIO_PA5, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}

#define PWR_PINS                                                         \
    /* Enable power converter 4.5-6V to 3.3V; low: off */               \
    {SIM_PWEN, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT},               \
    /* Enable second power converter: VCC_PHONE to VCC_SIM; high: on */ \
    {VCC_FWD, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}

#define SW_SIM      PIO_PA8
#define SMARTCARD_CONNECT_PIN {SW_SIM, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP | PIO_DEBOUNCE | PIO_DEGLITCH | PIO_IT_EDGE }
//#define SMARTCARD_CONNECT_PIN {SW_SIM, PIOB, ID_PIOB, PIO_INPUT, PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_EDGE}

/// PIN used for resetting the smartcard
// FIXME: Card is resetted with pin set to 0 --> PIO_OUTPUT_1 as default is right?
#define PIN_ISO7816_RSTMC       {PIO_PA7, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}

/// Pins used for connect the smartcard
#define PIN_SIM_IO_INPUT    {PIO_PA1, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT}
#define PIN_SIM_IO          {PIO_PA6, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
#define PIN_SIM_CLK         {PIO_PA2, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
#define PIN_SIM_CLK_INPUT   {PIO_PA4, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT}
//#define PINS_ISO7816            PIN_USART1_TXD, PIN_USART1_SCK, PIN_ISO7816_RSTMC
#define PINS_ISO7816        PIN_SIM_IO,  PIN_SIM_CLK,  PIN_ISO7816_RSTMC // SIM_PWEN_PIN, PIN_SIM_IO2, PIN_SIM_CLK2

#define PINS_TC             PIN_SIM_IO_INPUT, PIN_SIM_CLK_INPUT

#define PIN_USIM1_VCC		{PIO_PA25, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT}
#define PIN_USIM1_nRST		{PIO_PA24, PIOA, ID_PIOA, PIO_INPUT, PIO_IT_RISE_EDGE | PIO_DEGLITCH }
#define PIN_PHONE_IO_INPUT          {PIO_PA21, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT}
#define PIN_PHONE_IO                {PIO_PA22, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
#define PIN_PHONE_CLK               {PIO_PA23A_SCK1, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}     // External Clock Input on PA28
//#define PIN_PHONE_CLK               {PIO_PA23A_SCK1, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}     // External Clock Input on PA28
#define PIN_PHONE_CLK_INPUT         {PIO_PA29, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT}
#define PINS_USIM1		PIN_PHONE_IO, PIN_PHONE_CLK, PIN_PHONE_CLK_INPUT, PIN_USIM1_VCC, PIN_PHONE_IO_INPUT, PIN_USIM1_nRST
//, VCC_PHONE


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

#define BOARD_USB_BMATTRIBUTES	USBConfigurationDescriptor_BUSPOWERED_NORWAKEUP

#define BOARD_USB_VENDOR_ID	USB_VENDOR_OPENMOKO
#define BOARD_USB_PRODUCT_ID	USB_PRODUCT_SIMTRACE2
#define BOARD_DFU_USB_PRODUCT_ID USB_PRODUCT_SIMTRACE2_DFU
#define BOARD_USB_RELEASE	0x000

//#define HAVE_SNIFFER
//#define HAVE_CCID
#define HAVE_CARDEM
//#define HAVE_MITM
