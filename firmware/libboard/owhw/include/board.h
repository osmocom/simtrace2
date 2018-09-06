/* OWHW board definition
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
#pragma once
#include "board_common.h"
#include "simtrace_usb.h"

/** Name of the board */
#define BOARD_NAME "OWHW"
/** Board definition */
#define owhw

/** oscillator used as main clock source (in Hz) */
#define BOARD_MAINOSC 18432000
/** desired main clock frequency (in Hz, based on BOARD_MAINOSC) */
#define BOARD_MCK 58982400 // 18.432 * 16 / 5

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

#define BOARD_USB_BMATTRIBUTES	USBConfigurationDescriptor_SELFPOWERED_NORWAKEUP

#define BOARD_USB_VENDOR_ID	USB_VENDOR_OPENMOKO
#define BOARD_USB_PRODUCT_ID	USB_PRODUCT_OWHW_SAM3
#define BOARD_DFU_USB_PRODUCT_ID USB_PRODUCT_OWHW_SAM3_DFU
#define BOARD_USB_RELEASE	0x010

#define CARDEMU_SECOND_UART
/* Disable VCC/ADC detection, as OWHWv2 has no ADCVREF */
//#define DETECT_VCC_BY_ADC

#define HAVE_CARDEM
