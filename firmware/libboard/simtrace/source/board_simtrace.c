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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA
 */
#include "board.h"
#include "simtrace.h"
#include "utils.h"
#include "sim_switch.h"
#include <osmocom/core/timer.h>
#include "usb_buf.h"

void board_exec_dbg_cmd(int ch)
{
	switch (ch) {
	case '?':
		printf("\t?\thelp\n\r");
		printf("\tR\treset SAM3\n\r");
		break;
	case 'R':
		printf("Asking NVIC to reset us\n\r");
		USBD_Disconnect();
		NVIC_SystemReset();
		break;
	default:
		printf("Unknown command '%c'\n\r", ch);
		break;
	}
}

void board_main_top(void)
{
#ifndef APPLICATION_dfu
	usb_buf_init();

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
		//printf("BOOTLOADER switch pressed -> Force DFU\n\r");
		return 1;
	} else
		return 0;
}
