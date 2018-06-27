/* SIMtrace specific application code */
/* (C) 2017 by Harald Welte <laforge@gnumonks.org> */

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
