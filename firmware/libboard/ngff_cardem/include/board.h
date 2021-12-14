/* Osmocom ngff-cardem board definition
 *
 * (C) 2021 by Harald Welte <laforge@osmocom.org>
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
 */
#pragma once
#include "board_common.h"
#include "simtrace_usb.h"

/** Name of the board */
#define BOARD_NAME "NGFF-CARDEM"
/** Board definition */
#define ngff_cardem

/** oscillator used as main clock source (in Hz) */
#define BOARD_MAINOSC 12000000
/** desired main clock frequency (in Hz, based on BOARD_MAINOSC) */
#define BOARD_MCK 58000000 // 12.000 * 29 / 6

/** MCU pin connected to red LED */
#define PIO_LED_RED     PIO_PA17
/** MCU pin connected to green LED */
#define PIO_LED_GREEN   PIO_PA18
/** red LED pin definition */
#define PIN_LED_RED     {PIO_LED_RED, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
/** green LED pin definition */
#define PIN_LED_GREEN   {PIO_LED_GREEN, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
/** LEDs pin definition */
#define PINS_LEDS       PIN_LED_RED, PIN_LED_GREEN 
/** index for red LED in LEDs pin definition array */
#define LED_NUM_RED     0
/** index for green LED in LEDs pin definition array */
#define LED_NUM_GREEN   1
/** the green LED is actually red and used as indication for USIM1 */
#define LED_USIM1	LED_GREEN

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
//#define PIN_USIM2_VCC		{PIO_PB2, PIOB, ID_PIOB, PIO_INPUT, PIO_DEFAULT}

#define PINS_USIM1		PINS_TC_USIM1, PINS_ISO7816_USIM1, PIN_USIM1_nRST
#define PINS_USIM2		PINS_TC_USIM2, PINS_ISO7816_USIM2, PIN_USIM2_nRST

/* from v3 and onwards only (!) */
#define PIN_DET_USIM1_PRES	{PIO_PA8, PIOA, ID_PIOA, PIO_INPUT, PIO_DEGLITCH | PIO_IT_EDGE}

/* inputs reading the WWAN LED level */
#define PIN_WWAN1		{PIO_PA15, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP | PIO_DEGLITCH | PIO_IT_EDGE}
#define PINS_WWAN_IN		{ PIN_WWAN1 }

#define PIN_MODEM_EN {PIO_PA11, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define PIN_N_MODEM_PWR_ON {PIO_PA26, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}

/* outputs controlling RESET input of modems */
#define PIN_PERST1		{PIO_PA25, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define PINS_PERST		{ PIN_PERST1 }

#define PIN_VERSION_DET		{PIO_PA19, PIOA, ID_PIOA, PIO_PERIPH_D, PIO_DEFAULT}

/* GPIO towards SPDT switches between real SIM and SAM3 */
//#define PIN_SIM_SWITCH1 {PIO_PA20, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
//#define PIN_SIM_SWITCH2 {PIO_PA28, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}

#define BOARD_USB_BMATTRIBUTES	USBConfigurationDescriptor_SELFPOWERED_NORWAKEUP

#define BOARD_USB_VENDOR_ID	USB_VENDOR_OPENMOKO
#define BOARD_USB_PRODUCT_ID	USB_PRODUCT_NGFF_CARDEM
#define BOARD_DFU_USB_PRODUCT_ID USB_PRODUCT_NGFF_CARDEM
#define BOARD_USB_RELEASE	0x010

#define DETECT_VCC_BY_ADC
#define VCC_UV_THRESH_1V8	1500000
#define VCC_UV_THRESH_3V	VCC_UV_THRESH_1V8

#ifdef APPLICATION_cardem
#define HAVE_CARDEM
#define HAVE_BOARD_CARDINSERT
struct cardem_inst;
void board_set_card_insert(struct cardem_inst *ci, bool card_insert);
#endif

#ifdef APPLICATION_trace
#define HAVE_SNIFFER
#endif

/* Card I/O data signal input/output (I/O_SIM in schematic) */
#define PIN_SIM_IO             {PIO_PA6A_TXD0, PIOA, ID_PIOA, PIO_PERIPH_A, PIO_DEFAULT}
/* Card CLK clock input (CLK_SIM in schematic) */
#define PIN_SIM_CLK            {PIO_PA2B_SCK0, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
/* Card RST reset signal input (use as input since the phone will drive it) */
#define PIN_SIM_RST_SNIFF      {PIO_PA7, PIOA, ID_PIOA, PIO_INPUT, PIO_DEGLITCH | PIO_IT_EDGE}
/* Pins used to sniff phone-card communication */
#define PINS_SIM_SNIFF          PIN_SIM_IO, PIN_SIM_CLK, PIN_SIM_RST_SNIFF

/* Pin to measure card I/O timing (to start measuring the ETU on I/O activity; connected I/O_SIM in schematic) */
#define PIN_SIM_IO_INPUT       {PIO_PA1B_TIOB0, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
/* Pin used as clock input (to measure the ETU duration; connected to CLK_SIM in schematic) */
#define PIN_SIM_CLK_INPUT      {PIO_PA4B_TCLK0, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
/* Pins used to measure ETU timing (using timer counter) */ 
#define PINS_TC                 PIN_SIM_IO_INPUT, PIN_SIM_CLK_INPUT

//default state: NO uart connected, modem connected to physical sim, NO sim presence override, modem powers sim slot
#define pin_conn_usim1_default {PIO_PA20, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define pin_conn_usim2_default {PIO_PA28, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define pin_conn_mdm_sim_default {PIO_PA0, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
#define pin_conn_set_sim_det_default {PIO_PA13, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define pin_en_st_sim_vdd_default {PIO_PB2, PIOB, ID_PIOB, PIO_OUTPUT_0, PIO_DEFAULT}
#define pin_en_mdm_sim_vdd_default {PIO_PA16, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}

//cardem state: uart2 (!) connected, NO modem connected to physical sim, sim presence override, NOTHING powers sim slot
#define pin_conn_usim1_cem {PIO_PA20, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define pin_conn_usim2_cem {PIO_PA28, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
#define pin_conn_mdm_sim_cem {PIO_PA0, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define pin_en_st_sim_vdd_cem {PIO_PB2, PIOB, ID_PIOB, PIO_OUTPUT_0, PIO_DEFAULT}
#define pin_en_mdm_sim_vdd_cem {PIO_PA16, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}

//trace state: uart2 (!) connected, modem connected to physical sim, st powers sim slot
#define pin_conn_usim1_trace {PIO_PA20, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define pin_conn_usim2_trace {PIO_PA28, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
#define pin_conn_mdm_sim_trace {PIO_PA0, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
#define pin_conn_set_sim_det_trace {PIO_PA13, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
#define pin_en_st_sim_vdd_trace {PIO_PB2, PIOB, ID_PIOB, PIO_OUTPUT_1, PIO_DEFAULT}
#define pin_en_mdm_sim_vdd_trace {PIO_PA16, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}

#define PIN_MODEM_EN_off {PIO_PA11, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}

#define PINS_SIM_DEFAULT pin_conn_usim1_default, pin_conn_usim2_default, pin_conn_mdm_sim_default, pin_conn_set_sim_det_default, PIN_N_MODEM_PWR_ON, PIN_MODEM_EN, pin_en_st_sim_vdd_default, pin_en_mdm_sim_vdd_default
#define PINS_SIM_CARDEM pin_conn_usim1_cem, pin_conn_usim2_cem, pin_conn_mdm_sim_cem, pin_en_mdm_sim_vdd_cem, pin_en_st_sim_vdd_cem// , pin_conn_set_sim_det_cem

#define PINS_BUS_SNIFF pin_conn_usim1_trace, pin_conn_usim2_trace, pin_conn_mdm_sim_trace, pin_conn_set_sim_det_trace,PIN_MODEM_EN_off
#define PINS_PWR_SNIFF PIN_N_MODEM_PWR_ON, PIN_MODEM_EN, pin_en_st_sim_vdd_trace, pin_en_mdm_sim_vdd_trace
