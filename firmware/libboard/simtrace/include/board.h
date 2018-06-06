#pragma once
#include "board_common.h"

/* Name of the board */
#define BOARD_NAME "SAM3S-SIMTRACE"
/* Board definition */
#define simtrace

/* Board main oscillator frequency (in Hz) */
#define BOARD_MAINOSC 18432000

/** Pin configuration **/
/* Button to force bootloader start (shorted to ground when pressed */
#define PIN_BOOTLOADER_SW      {PIO_PA31, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP}
/* Enable powering the card using the second 3.3 V output of the LDO (active high) */
#define SIM_PWEN_PIN           {SIM_PWEN, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
/* Card presence pin */
#define SW_SIM                  PIO_PA8
/* Pull card presence pin high (shorted to ground in card slot when card is present) */
#define SMARTCARD_CONNECT_PIN  {SW_SIM, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP | PIO_DEBOUNCE | PIO_DEGLITCH | PIO_IT_EDGE }

/** Smart card connection **/
/* Card RST reset signal input (active low; RST_SIM in schematic) */
#define PIN_ISO7816_RSTMC      {PIO_PA7, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
/* Card I/O data signal input/output (I/O_SIM in schematic) */
#define PIN_SIM_IO             {PIO_PA6A_TXD0, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
/* Card CLK clock input (CLK_SIM in schematic) */
#define PIN_SIM_CLK            {PIO_PA2B_SCK0, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
/* ISO7816-communication related pins */
#define PINS_ISO7816            PIN_SIM_IO,  PIN_SIM_CLK,  PIN_ISO7816_RSTMC // SIM_PWEN_PIN, PIN_SIM_IO2, PIN_SIM_CLK2
/* Pin to measure card I/O timing (to start measuring the ETU on I/O activity; connected I/O_SIM in schematic) */
#define PIN_SIM_IO_INPUT       {PIO_PA1B_TIOB0, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
/* Pin used as clock input (to measure the ETU duration; connected to CLK_SIM in schematic) */
#define PIN_SIM_CLK_INPUT      {PIO_PA4B_TCLK0, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
/* Pins used to measure ETU timing (using timer counter) */ 
#define PINS_TC                 PIN_SIM_IO_INPUT, PIN_SIM_CLK_INPUT

/** Phone connection **/
/* Phone USIM slot 1 VCC pin (VCC_PHONE in schematic) */
#define PIN_USIM1_VCC          {PIO_PA25, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT}
/* Phone USIM slot 1 RST pin (active low; RST_PHONE in schematic) */
#define PIN_USIM1_nRST         {PIO_PA24, PIOA, ID_PIOA, PIO_INPUT, PIO_IT_RISE_EDGE | PIO_DEGLITCH }
/* Phone I/O data signal input/output (I/O_PHONE in schematic) */
#define PIN_PHONE_IO           {PIO_PA22A_TXD1, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
/* Phone CLK clock input (CLK_PHONE in schematic) */
#define PIN_PHONE_CLK          {PIO_PA23A_SCK1, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
/* Pin used for phone USIM slot 1 communication */
#define PINS_USIM1              PIN_PHONE_IO, PIN_PHONE_CLK, PIN_PHONE_CLK_INPUT, PIN_USIM1_VCC, PIN_PHONE_IO_INPUT, PIN_USIM1_nRST
/* Phone I/O data signal input/output (unused USART RX input; connected to I/O_PHONE in schematic) */
#define PIN_PHONE_IO_INPUT     {PIO_PA21A_RXD1, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
/* Pin used as clock input (to measure the ETU duration; connected to CLK_PHONE in schematic) */
#define PIN_PHONE_CLK_INPUT    {PIO_PA29B_TCLK2, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}

/** Default configuration **/
/* Disconnect VPP, CLK, and RST lines between card and phone using bus switch (high sets bus switch to high-impedance) */
#define PIN_SC_SW_DEFAULT      {PIO_PA20, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
/* Disconnect I/O line between card and phone using bus switch (high sets bus switch to high-impedance) */
#define PIN_IO_SW_DEFAULT      {PIO_PA19, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
/* Disconnect all lines (VPP, CLK, RST, and I/O) between card and phone */
#define PINS_BUS_DEFAULT        PIN_SC_SW_DEFAULT, PIN_IO_SW_DEFAULT

/** Sniffer configuration **/
/* Connect VPP, CLK, and RST lines between card and phone using bus switch (low connects signals on bus switch) */
#define PIN_SC_SW_SNIFF        {PIO_PA20, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
/* Connect I/O line between card and phone using bus switch (low connects signals on bus switch) */
#define PIN_IO_SW_SNIFF        {PIO_PA19, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
/* Connect all lines (VPP, CLK, RST, and I/O) between card and phone */
#define PINS_BUS_SNIFF          PIN_SC_SW_SNIFF, PIN_IO_SW_SNIFF
/* Pins used to sniff phone-card communication */
#define PINS_SIM_SNIFF_SIM      PIN_PHONE_IO,  PIN_PHONE_CLK
/* Disable power converter 4.5-6V to 3.3V; (active high) */
#define PIN_SIM_PWEN_SNIFF     {SIM_PWEN, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
/* Enable power switch: VCC_PHONE to VCC_SIM; (active high) */
#define PIN_VCC_FWD_SNIFF      {VCC_FWD, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
/* Use phone VCC to power card */
#define PWR_PINS                PIN_SIM_PWEN_SNIFF, PIN_VCC_FWD_SNIFF

/** External SPI flash interface   **/
/* SPI MISO pin definition */
#define PIN_SPI_MISO  {PIO_PA12A_MISO, PIOA, PIOA, PIO_PERIPH_A, PIO_PULLUP}
/* SPI MOSI pin definition */
#define PIN_SPI_MOSI  {PIO_PA13A_MOSI, PIOA, PIOA, PIO_PERIPH_A, PIO_DEFAULT}
/* SPI SCK pin definition */
#define PIN_SPI_SCK   {PIO_PA14A_SPCK, PIOA, PIOA, PIO_PERIPH_A, PIO_DEFAULT}
/* SPI pins definition. Contains MISO, MOSI & SCK */
#define PINS_SPI       PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_SCK
/* SPI chip select 0 pin definition */
#define PIN_SPI_NPCS0 {PIO_PA11A_NPCS0, PIOA, PIOA, PIO_PERIPH_A, PIO_DEFAULT}
/* SPI flash write protect pin (active low, pulled low) */
#define PIN_SPI_WP    {PA15, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}

/** USB definitions */
/* OpenMoko SIMtrace 2 USB vendor ID */
#define BOARD_USB_VENDOR_ID	USB_VENDOR_OPENMOKO
/* OpenMoko SIMtrace 2 USB product ID (main application/runtime mode) */
#define BOARD_USB_PRODUCT_ID	USB_PRODUCT_SIMTRACE2
/* OpenMoko SIMtrace 2 DFU USB product ID (DFU bootloader/DFU mode) */
#define BOARD_DFU_USB_PRODUCT_ID USB_PRODUCT_SIMTRACE2_DFU
/* USB release number (bcdDevice, shown as 0.00) */
#define BOARD_USB_RELEASE	0x000
/* Indicate SIMtrace is bus power in USB attributes */
#define BOARD_USB_BMATTRIBUTES	USBConfigurationDescriptor_BUSPOWERED_NORWAKEUP

/** Supported modes */
/* SIMtrace board supports sniffer mode */
#define HAVE_SNIFFER
/* SIMtrace board supports CCID mode */
#define HAVE_CCID
/* SIMtrace board supports card emulation mode */
#define HAVE_CARDEM
/* SIMtrace board supports man-in-the-middle mode */
//#define HAVE_MITM
