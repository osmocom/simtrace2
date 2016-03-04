#pragma once
#include "board_common.h"

/** Name of the board */
#define BOARD_NAME "OWHW"
/** Board definition */
#define owhw

/* USIM 2 interface (USART) */
#define PIN_USIM2_CLK		{PIO_PA2, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
#define PIN_USIM2_IO		{PIO_PA6, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
#define PINS_ISO7816_USIM2	PIN_USIM2_CLK, PIN_USIM2_IO

/* USIM 2 interface (TC) */
#define PIN_USIM2_IO_TC		{PIO_PA1, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
#define PIN_USIM2_CLK_TC	{PIO_PA4, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
#define PINS_TC_USIM2		PIN_USIM2_IO_TC, PIN_USIM2_CLK_TC

/* USIM 1 interface (USART) */
#define PIN_USIM1_IO		{PIO_PA22, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
#define PIN_USIM1_CLK		{PIO_PA23, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
#define PINS_ISO7816_USIM1	PIN_USIM1_CLK, PIN_USIM1_IO

/* USIM 1 interface (TC) */
#define PIN_USIM1_IO_TC		{PIO_PA27, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
#define PIN_USIM1_CLK_TC	{PIO_PA29, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
#define PINS_TC_USIM1		PIN_USIM1_IO_TC, PIN_USIM1_CLK_TC

#define PIN_SET_USIM1_PRES	{PIO_PA12, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
#define PIN_USIM1_nRST		{PIO_PA24, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT}
#define PIN_USIM1_VCC		{PIO_PB3, PIOB, ID_PIOB, PIO_INPUT, PIO_DEFAULT}

#define PIN_SET_USIM2_PRES	{PIO_PA14, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
#define PIN_USIM2_nRST		{PIO_PA7, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT}
#define PIN_USIM2_VCC		{PIO_PB2, PIOB, ID_PIOB, PIO_INPUT, PIO_DEFAULT}

#define PINS_USIM1		PINS_TC_USIM1, PINS_ISO7816_USIM1, PIN_USIM1_nRST, PIN_SET_USIM1_PRES
#define PINS_USIM2		PINS_TC_USIM2, PINS_ISO7816_USIM2, PIN_USIM2_nRST, PIN_SET_USIM2_PRES

#define PINS_CARDSIM		{ PIN_SET_USIM1_PRES, PIN_SET_USIM2_PRES }

#define SIMTRACE_VENDOR_ID          0x1d50
#define SIMTRACE_PRODUCT_ID         0x60e3	/* FIXME */
#define USB_VENDOR_ID               SIMTRACE_VENDOR_ID
#define USB_PRODUCT_ID              SIMTRACE_PRODUCT_ID

#define HAVE_CARDEM
