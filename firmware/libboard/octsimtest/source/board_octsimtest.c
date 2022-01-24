/* SIMtrace with SAM3S specific application code
 *
 * (C) 2017 by Harald Welte <laforge@gnumonks.org>
 * (C) 2018 by sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>
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
#include <stdbool.h>
#include "board.h"
#include "simtrace.h"
#include "utils.h"
#include "sim_switch.h"
#include <osmocom/core/timer.h>
#include "usb_buf.h"
#include "i2c.h"
#include "mcp23017.h"
#include "mux.h"

static bool mcp2317_present = false;

void board_exec_dbg_cmd(int ch)
{
	switch (ch) {
	case '?':
		printf("\t?\thelp\r\n");
		printf("\t0-8\tselect physical SIM slot\r\n");
		printf("\tR\treset SAM3\r\n");
		printf("\tm\trun mcp23017 test\r\n");
		printf("\ti\tset card insert via I2C\r\n");
		printf("\tI\tdisable card insert\r\n");
		break;
	case '0': mux_set_slot(0); break;
	case '1': mux_set_slot(1); break;
	case '2': mux_set_slot(2); break;
	case '3': mux_set_slot(3); break;
	case '4': mux_set_slot(4); break;
	case '5': mux_set_slot(5); break;
	case '6': mux_set_slot(6); break;
	case '7': mux_set_slot(7); break;
	case 'R':
		printf("Asking NVIC to reset us\r\n");
		USBD_Disconnect();
		NVIC_SystemReset();
		break;
	case 'm':
		mcp23017_test(MCP23017_ADDRESS);
		break;
	case 'i':
		printf("Setting card insert (slot=%u)\r\n", mux_get_slot());
		mcp23017_set_output_a(MCP23017_ADDRESS, (1 << mux_get_slot()));
		break;
	case 'I':
		printf("Releasing card insert (slot=%u)\r\n", mux_get_slot());
		mcp23017_set_output_a(MCP23017_ADDRESS, 0);
		break;
	default:
		printf("Unknown command '%c'\r\n", ch);
		break;
	}
}

void board_main_top(void)
{
#ifndef APPLICATION_dfu
	usb_buf_init();

	mux_init();
	i2c_pin_init();
	/* PORT A: all outputs, Port B0 output, B1..B7 unused */
	if (mcp23017_init(MCP23017_ADDRESS, 0x00, 0xfe) == 0) {
		mcp2317_present = true;
		mcp23017_set_output_a(MCP23017_ADDRESS, 0);
	}
	/* Initialize checking for card insert/remove events */
	//card_present_init();
#endif
}

int board_override_enter_dfu(void)
{
	const Pin bl_sw_pin = PIN_BOOTLOADER_SW;

	PIO_Configure(&bl_sw_pin, 1);

	/* Enter DFU bootloader in case the respective button is pressed */
	if (PIO_Get(&bl_sw_pin) == 0) {
		/* do not print to early since the console is not initialized yet */
		//printf("BOOTLOADER switch pressed -> Force DFU\r\n");
		return 1;
	} else
		return 0;
}

void board_set_card_insert(struct cardem_inst *ci, bool card_insert)
{
	int s = mux_get_slot();

	/* A0 .. A7 of the MCP are each connected to the gate of a FET which closes
	 * the sim-present signal of the respective slot */

	if (mcp2317_present) {
		if (card_insert) {
			/* we must enable card-presence of the active slot and disable it on all others */
			mcp23017_set_output_a(MCP23017_ADDRESS, (1 << s));
		} else {
			/* we disable all card insert signals */
			mcp23017_set_output_a(MCP23017_ADDRESS, 0);
		}
	} else {
		TRACE_WARNING("No MCP23017 present; cannot set CARD_INSERT\r\n");
	}
}
