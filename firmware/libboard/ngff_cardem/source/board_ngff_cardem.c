/* sysmocom quad-modem sysmoQMOD application code
 *
 * (C) 2021 Harald Welte <laforge@osmocom.org>
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
#include "led.h"
#include "wwan_led.h"
#include "wwan_perst.h"
#include "sim_switch.h"
#include "boardver_adc.h"
#include "card_pres.h"
#include <osmocom/core/timer.h>
#include "usb_buf.h"

/* array of generated USB Strings */
extern unsigned char *usb_strings[];

/* returns '1' in case we should break any endless loop */
void board_exec_dbg_cmd(int ch)
{
	switch (ch) {
	case '?':
		printf("\t?\thelp\n\r");
		printf("\tR\treset SAM3\n\r");
		printf("\tl\tswitch off LED 1\n\r");
		printf("\tL\tswitch on  LED 1\n\r");
		printf("\tg\tswitch off LED 2\n\r");
		printf("\tG\tswitch on  LED 2\n\r");
		printf("\tU\tProceed to USB Initialization\n\r");
		printf("\t1\tGenerate 1ms reset pulse on WWAN1\n\r");
		printf("\t!\tSwitch Channel A from physical -> remote\n\r");
		printf("\t@\tSwitch Channel B from physical -> remote\n\r");
		printf("\tt\t(pseudo)talloc report\n\r");
		break;
	case 'R':
		printf("Asking NVIC to reset us\n\r");
		USBD_Disconnect();
		NVIC_SystemReset();
		break;
	case 'l':
		led_blink(LED_GREEN, BLINK_ALWAYS_OFF);
		printf("LED 1 switched off\n\r");
		break;
	case 'L':
		led_blink(LED_GREEN, BLINK_ALWAYS_ON);
		printf("LED 1 switched on\n\r");
		break;
	case 'g':
		led_blink(LED_RED, BLINK_ALWAYS_OFF);
		printf("LED 2 switched off\n\r");
		break;
	case 'G':
		led_blink(LED_RED, BLINK_ALWAYS_ON);
		printf("LED 2 switched on\n\r");
		break;
	case '1':
		printf("Resetting Modem\n\r");
		wwan_perst_do_reset_pulse(0, 300);
		break;
	case '!':
		sim_switch_use_physical(0, 0);
		break;
	case 't':
		talloc_report(NULL, stdout);
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

	wwan_led_init();
	wwan_perst_init();
	sim_switch_init();
#endif

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
