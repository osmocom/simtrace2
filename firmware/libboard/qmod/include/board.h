/* sysmocom quad-modem sysmoQMOD board definition
 *
 * (C) 2016-2017 by Harald Welte <hwelte@hmw-consulting.de>
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
#pragma once
#include "board_common.h"
#include "simtrace_usb.h"

#define LED_USIM1	LED_GREEN
#define LED_USIM2	LED_RED

/** Name of the board */
#define BOARD_NAME "QMOD"
/** Board definition */
#define qmod

/** oscillator used as main clock source (in Hz) */
#define BOARD_MAINOSC 12000000
/** desired main clock frequency (in Hz, based on BOARD_MAINOSC) */
#define BOARD_MCK 58000000 // 18.432 * 29 / 6

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

#define PIN_USIM1_nRST		{PIO_PA24, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT}
#define PIN_USIM1_VCC		{PIO_PB3, PIOB, ID_PIOB, PIO_INPUT, PIO_DEFAULT}

#define PIN_USIM2_nRST		{PIO_PA7, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT}
#define PIN_USIM2_VCC		{PIO_PB2, PIOB, ID_PIOB, PIO_INPUT, PIO_DEFAULT}

#define PINS_USIM1		PINS_TC_USIM1, PINS_ISO7816_USIM1, PIN_USIM1_nRST
#define PINS_USIM2		PINS_TC_USIM2, PINS_ISO7816_USIM2, PIN_USIM2_nRST

/* from v3 and onwards only (!) */
#define PIN_DET_USIM1_PRES	{PIO_PA12, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP | PIO_DEGLITCH | PIO_IT_EDGE}
#define PIN_DET_USIM2_PRES	{PIO_PA8, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP | PIO_DEGLITCH | PIO_IT_EDGE}

/* only in v2 and lower (!) */
#define PIN_PRTPWR_OVERRIDE	{PIO_PA8, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}

/* inputs reading the WWAN LED level */
#define PIN_WWAN1		{PIO_PA15, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP | PIO_DEGLITCH | PIO_IT_EDGE}
#define PIN_WWAN2		{PIO_PA16, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP | PIO_DEGLITCH | PIO_IT_EDGE}
#define PINS_WWAN_IN		{ PIN_WWAN1, PIN_WWAN2 }

/* outputs controlling RESET input of modems */
#define PIN_PERST1		{PIO_PA25, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_PULLUP}
#define PIN_PERST2		{PIO_PA26, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_PULLUP}
#define PINS_PERST		{ PIN_PERST1, PIN_PERST2 }

#define PIN_VERSION_DET		{PIO_PA19, PIOA, ID_PIOA, PIO_PERIPH_D, PIO_DEFAULT}

/* GPIO towards SPDT switches between real SIM and SAM3 */
#define PIN_SIM_SWITCH1 {PIO_PA20, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
#define PIN_SIM_SWITCH2 {PIO_PA28, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}

#define BOARD_USB_BMATTRIBUTES	USBConfigurationDescriptor_SELFPOWERED_NORWAKEUP

#define BOARD_USB_VENDOR_ID	USB_VENDOR_OPENMOKO
#define BOARD_USB_PRODUCT_ID	USB_PRODUCT_QMOD_SAM3
#define BOARD_DFU_USB_PRODUCT_ID USB_PRODUCT_QMOD_SAM3_DFU
#define BOARD_USB_RELEASE	0x010

#define CARDEMU_SECOND_UART
#define DETECT_VCC_BY_ADC

/** sysmoQMOD only supports card emulation */
#ifdef APPLICATION_cardem
#define HAVE_CARDEM
#endif
