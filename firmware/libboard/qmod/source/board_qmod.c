/* sysmocom quad-modem sysmoQMOD application code
 *
 * (C) 2016-2017 by Harald Welte <hwelte@hmw-consulting.de>
 * (C) 2018-2019, sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>
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
#include "board.h"
#include "simtrace.h"
#include "utils.h"
#include "led.h"
#include "wwan_led.h"
#include "wwan_perst.h"
#include "sim_switch.h"
#include "boardver_adc.h"
#include "card_pres.h"
#include <osmocom/core/timer.h>
#include "usb_buf.h"
#include "i2c.h"

static const Pin pin_hubpwr_override = PIN_PRTPWR_OVERRIDE;
static const Pin pin_hub_rst = {PIO_PA13, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
static const Pin pin_1234_detect = {PIO_PA14, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP};
static const Pin pin_peer_rst = {PIO_PA0, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
static const Pin pin_peer_erase = {PIO_PA11, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};

/* array of generated USB Strings */
extern unsigned char *usb_strings[];

static int qmod_sam3_is_12(void)
{
	if (PIO_Get(&pin_1234_detect) == 0)
		return 1;
	else
		return 0;
}

#if (ALLOW_PEER_ERASE > 0)
const unsigned char __eeprom_bin[256] = {
	USB_VENDOR_OPENMOKO & 0xff,
	USB_VENDOR_OPENMOKO >> 8,
	USB_PRODUCT_QMOD_HUB & 0xff,
	USB_PRODUCT_QMOD_HUB >> 8,
	                        0x00, 0x00, 0x9b, 0x20, 0x09, 0x00, 0x00, 0x00, 0x32, 0x32, 0x32, 0x32, /* 0x00 - 0x0f */
	0x32, 0x04, 0x09, 0x18, 0x0d, 0x00, 0x73, 0x00, 0x79, 0x00, 0x73, 0x00, 0x6d, 0x00, 0x6f, 0x00, /* 0x10 - 0x1f */
	0x63, 0x00, 0x6f, 0x00, 0x6d, 0x00, 0x20, 0x00, 0x2d, 0x00, 0x20, 0x00, 0x73, 0x00, 0x2e, 0x00, /* 0x20 - 0x2f */
	0x66, 0x00, 0x2e, 0x00, 0x6d, 0x00, 0x2e, 0x00, 0x63, 0x00, 0x2e, 0x00, 0x20, 0x00, 0x47, 0x00, /* 0x30 - 0x3f */
	0x6d, 0x00, 0x62, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x40 - 0x4f */
	0x00, 0x00, 0x00, 0x00, 0x71, 0x00, 0x75, 0x00, 0x61, 0x00, 0x64, 0x00, 0x20, 0x00, 0x6d, 0x00, /* 0x50 - 0x5f */
	0x6f, 0x00, 0x64, 0x00, 0x65, 0x00, 0x6d, 0x00, 0x20, 0x00, 0x76, 0x00, 0x32, 0x00, 0x00, 0x00, /* 0x60 - 0x6f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x70 - 0x7f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x80 - 0x8f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x90 - 0x9f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa0 - 0xaf */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xb0 - 0xbf */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xc0 - 0xcf */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xd0 - 0xdf */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xe0 - 0xef */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x56, 0x23, 0x71, 0x04, 0x00, /* 0xf0 - 0xff */
};

static int write_hub_eeprom(void)
{
	int i;

	/* wait */
	mdelay(100);

	TRACE_INFO("Writing EEPROM...\r\n");
	/* write the EEPROM once */
	for (i = 0; i < ARRAY_SIZE(__eeprom_bin); i++) {
		int rc = eeprom_write_byte(0x50, i, __eeprom_bin[i]);
		if (rc < 0) {
			TRACE_ERROR("Writing EEPROM failed at byte %u: 0x%02x\r\n",
				i, __eeprom_bin[i]);
			return 1;
		}
	}

	/* then pursue re-reading it again and again */
	TRACE_INFO("Verifying EEPROM...\r\n");
	for (i = 0; i < ARRAY_SIZE(__eeprom_bin); i++) {
		int byte = eeprom_read_byte(0x50, i);
		TRACE_DEBUG("0x%02x: %02x\r\n", i, byte);
		if (byte != __eeprom_bin[i])
			TRACE_ERROR("Byte %u is wrong, expected 0x%02x, found 0x%02x\r\n",
					i, __eeprom_bin[i], byte);
	}
	TRACE_INFO("EEPROM written\r\n");

	/* FIXME: Release PIN_PRTPWR_OVERRIDE after we know the hub is
	 * again powering us up */

	return 0;
}

static int erase_hub_eeprom(void)
{
	int i;

	/* wait */
	mdelay(100);

	TRACE_INFO("Erasing EEPROM...\r\n");
	/* write the EEPROM once */
	for (i = 0; i < 256; i++) {
		int rc = eeprom_write_byte(0x50, i, 0xff);
		if (rc < 0) {
			TRACE_ERROR("Erasing EEPROM failed at byte %u: 0x%02x\r\n",
				i, __eeprom_bin[i]);
			return 1;
		}
	}
	TRACE_INFO("EEPROM erased\r\n");

	return 0;
}
#endif /* ALLOW_PEER_ERASE */

static void board_exec_dbg_cmd_st12only(int ch)
{
	uint32_t addr, val;

	/* functions below only work on primary (ST12) */
	if (!qmod_sam3_is_12())
		return;

	switch (ch) {
#if (ALLOW_PEER_ERASE > 0)
	case 'E':
		write_hub_eeprom();
		break;
	case 'e':
		erase_hub_eeprom();
		break;
#endif /* ALLOW_PEER_ERASE */
	case 'O':
		printf("Setting PRTPWR_OVERRIDE\r\n");
		PIO_Set(&pin_hubpwr_override);
		break;
	case 'o':
		printf("Clearing PRTPWR_OVERRIDE\r\n");
		PIO_Clear(&pin_hubpwr_override);
		break;
#if (ALLOW_PEER_ERASE > 0)
	case 'H':
		printf("Clearing _HUB_RESET -> HUB_RESET high (inactive)\r\n");
		PIO_Clear(&pin_hub_rst);
		break;
	case 'h':
		/* high level drives transistor -> HUB_RESET low */
		printf("Asserting _HUB_RESET -> HUB_RESET low (active)\r\n");
		PIO_Set(&pin_hub_rst);
		break;
	case 'w':
		if (PIO_GetOutputDataStatus(&pin_hub_rst) == 0)
			printf("WARNING: attempting EEPROM access while HUB not in reset\r\n");
		printf("Please enter EEPROM offset:\r\n");
		UART_GetIntegerMinMax(&addr, 0, 255);
		printf("Please enter EEPROM value:\r\n");
		UART_GetIntegerMinMax(&val, 0, 255);
		printf("Writing value 0x%02lx to EEPROM offset 0x%02lx\r\n", val, addr);
		eeprom_write_byte(0x50, addr, val);
		break;
#endif /* ALLOW_PEER_ERASE */
	case 'r':
		printf("Please enter EEPROM offset:\r\n");
		UART_GetIntegerMinMax(&addr, 0, 255);
		printf("EEPROM[0x%02lx] = 0x%02x\r\n", addr, eeprom_read_byte(0x50, addr));
		break;
	default:
		printf("Unknown command '%c'\r\n", ch);
		break;
	}
}

/* returns '1' in case we should break any endless loop */
void board_exec_dbg_cmd(int ch)
{
#if (ALLOW_PEER_ERASE > 0)
	/* this variable controls if it is allowed to assert/release the ERASE line.
	   this is done to prevent accidental ERASE on noisy serial input since only one character can trigger the ERASE.
	 */
	static bool allow_erase = false;
#endif /* ALLOW_PEER_ERASE */

	switch (ch) {
	case '?':
		printf("\t?\thelp\r\n");
		printf("\tR\treset SAM3\r\n");
		printf("\tl\tswitch off LED 1\r\n");
		printf("\tL\tswitch off LED 1\r\n");
		printf("\tg\tswitch off LED 2\r\n");
		printf("\tG\tswitch off LED 2\r\n");
		if (qmod_sam3_is_12()) {
#if (ALLOW_PEER_ERASE > 0)
			printf("\tE\tprogram EEPROM\r\n");
			printf("\te\tErase EEPROM\r\n");
#endif /* ALLOW_PEER_ERASE */
			printf("\tO\tEnable PRTPWR_OVERRIDE\r\n");
			printf("\to\tDisable PRTPWR_OVERRIDE\r\n");
#if (ALLOW_PEER_ERASE > 0)
			printf("\tH\tRelease HUB RESET (high)\r\n");
			printf("\th\tAssert HUB RESET (low)\r\n");
			printf("\tw\tWrite single byte in EEPROM\r\n");
#endif /* ALLOW_PEER_ERASE */
			printf("\tr\tRead single byte from EEPROM\r\n");
		}
		printf("\tX\tRelease peer SAM3 from reset\r\n");
		printf("\tx\tAssert peer SAM3 reset\r\n");
#if (ALLOW_PEER_ERASE > 0)
		printf("\tY\tRelease peer SAM3 ERASE signal\r\n");
		printf("\ta\tAllow asserting peer SAM3 ERASE signal\r\n");
		printf("\ty\tAssert peer SAM3 ERASE signal\r\n");
#endif /* ALLOW_PEER_ERASE */
		printf("\tU\tProceed to USB Initialization\r\n");
		printf("\t1\tGenerate 1ms reset pulse on WWAN1\r\n");
		printf("\t2\tGenerate 1ms reset pulse on WWAN2\r\n");
		printf("\t!\tSwitch Channel A from physical -> remote\r\n");
		printf("\t@\tSwitch Channel B from physical -> remote\r\n");
		printf("\tt\t(pseudo)talloc report\r\n");
		break;
	case 'R':
		printf("Asking NVIC to reset us\r\n");
		USBD_Disconnect();
		NVIC_SystemReset();
		break;
	case 'l':
		led_blink(LED_GREEN, BLINK_ALWAYS_OFF);
		printf("LED 1 switched off\r\n");
		break;
	case 'L':
		led_blink(LED_GREEN, BLINK_ALWAYS_ON);
		printf("LED 1 switched on\r\n");
		break;
	case 'g':
		led_blink(LED_RED, BLINK_ALWAYS_OFF);
		printf("LED 2 switched off\r\n");
		break;
	case 'G':
		led_blink(LED_RED, BLINK_ALWAYS_ON);
		printf("LED 2 switched on\r\n");
		break;
	case 'X':
		printf("Clearing _SIMTRACExx_RST -> SIMTRACExx_RST high (inactive)\r\n");
		PIO_Clear(&pin_peer_rst);
		break;
	case 'x':
		printf("Setting _SIMTRACExx_RST -> SIMTRACExx_RST low (active)\r\n");
		PIO_Set(&pin_peer_rst);
		break;
#if (ALLOW_PEER_ERASE > 0)
	case 'Y':
		printf("Clearing SIMTRACExx_ERASE (inactive)\r\n");
		PIO_Clear(&pin_peer_erase);
		break;
	case 'a':
		printf("Asserting SIMTRACExx_ERASE allowed on next command\r\n");
		allow_erase = true;
		break;
	case 'y':
		if (allow_erase) {
			printf("Setting SIMTRACExx_ERASE (active)\r\n");
			PIO_Set(&pin_peer_erase);
		} else {
			printf("Please first allow setting SIMTRACExx_ERASE\r\n");
		}
		break;
#endif /* ALLOW_PEER_ERASE */
	case '1':
		printf("Resetting Modem 1 (of this SAM3)\r\n");
		wwan_perst_do_reset_pulse(0, 300);
		break;
	case '2':
		printf("Resetting Modem 2 (of this SAM3)\r\n");
		wwan_perst_do_reset_pulse(1, 300);
		break;
	case '!':
		sim_switch_use_physical(0, 0);
		break;
	case '@':
		sim_switch_use_physical(0, 0);
		break;
	case 't':
		talloc_report(NULL, stdout);
		break;
	default:
		if (!qmod_sam3_is_12())
			printf("Unknown command '%c'\r\n", ch);
		else
			board_exec_dbg_cmd_st12only(ch);
		break;
	}

#if (ALLOW_PEER_ERASE > 0)
	// set protection back so it can only run for one command
	if ('a' != ch) {
		allow_erase = false;
	}
#endif /* ALLOW_PEER_ERASE */
}

void board_main_top(void)
{
#ifndef APPLICATION_dfu
	usb_buf_init();

	wwan_led_init();
	wwan_perst_init();
	sim_switch_init();
#endif

	/* make sure we can detect whether running in ST12 or ST34 */
	PIO_Configure(&pin_1234_detect, 1);

	if (qmod_sam3_is_12()) {
		/* set PIN_PRTPWR_OVERRIDE to output-low to avoid the internal
		* pull-up on the input to keep SIMTRACE12 alive */
		PIO_Configure(&pin_hubpwr_override, 1);
		PIO_Configure(&pin_hub_rst, 1);
	}
	PIO_Configure(&pin_peer_rst, 1);
	PIO_Configure(&pin_peer_erase, 1);

#ifndef APPLICATION_dfu
	i2c_pin_init();
#endif

	if (qmod_sam3_is_12()) {
		TRACE_INFO("Detected Quad-Modem ST12\r\n");
	} else {
		TRACE_INFO("Detected Quad-Modem ST34\r\n");
#ifndef APPLICATION_dfu
		/* make sure we use the second set of USB Strings
		 * calling the interfaces "Modem 3" and "Modem 4" rather
		 * than 1+2 */
		usb_strings[7] = usb_strings[9];
		usb_strings[8] = usb_strings[10];
#endif
	}

	/* Obtain the circuit board version (currently just prints voltage */
	get_board_version_adc();
#ifndef APPLICATION_dfu
	/* Initialize checking for card insert/remove events */
	card_present_init();
#endif
}

static int uart_has_loopback_jumper(void)
{
	unsigned int i;
	const Pin uart_loopback_pins[] = {
		{PIO_PA9A_URXD0, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT},
		{PIO_PA10A_UTXD0, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
	};

	/* Configure UART pins as I/O */
	PIO_Configure(uart_loopback_pins, PIO_LISTSIZE(uart_loopback_pins));

	/* Send pattern over UART TX and check if it is received on RX
	 * If the loop doesn't get interrupted, RxD always follows TxD and thus a
	 * loopback jumper has been placed on RxD/TxD, and we will boot
	 * into DFU unconditionally
	 */
	int has_loopback_jumper = 1;
	for (i = 0; i < 10; i++) {
		/* Set TxD high; abort if RxD doesn't go high either */
		PIO_Set(&uart_loopback_pins[1]);
		if (!PIO_Get(&uart_loopback_pins[0])) {
			has_loopback_jumper = 0;
			break;
		}
		/* Set TxD low, abort if RxD doesn't go low either */
		PIO_Clear(&uart_loopback_pins[1]);
		if (PIO_Get(&uart_loopback_pins[0])) {
			has_loopback_jumper = 0;
			break;
		}
	}

	/* Put pins back to UART mode */
	const Pin uart_pins[] = {PINS_UART};
	PIO_Configure(uart_pins, PIO_LISTSIZE(uart_pins));

	return has_loopback_jumper;
}

int board_override_enter_dfu(void)
{
	/* If the loopback jumper is set, we enter DFU mode */
	if (uart_has_loopback_jumper())
		return 1;

	return 0;
}
