/* Code to switch between local (physical) and remote (emulated) SIM
 *
 * (C) 2015-2017 by Harald Welte <hwelte@hmw-consulting.de>
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
#include "trace.h"
#include "led.h"
#include "sim_switch.h"

//uart12bus	2-20e pa20
//uart22bus	1-20e pa28
//usim2bus	1-10e pa0  pivot
//sim det	2-10e pa13

static const Pin pins_default[]	= {PINS_SIM_DEFAULT};
static const Pin pins_cem[]	= {PINS_SIM_CARDEM};

static int initialized = 0;

int sim_switch_use_physical(unsigned int nr, int physical)
{
	const Pin pin = PIN_MODEM_EN;// PIN_N_MODEM_PWR_ON;
	enum led led;

	if (!initialized) {
		TRACE_ERROR("Somebody forgot to call sim_switch_init()\r\n");
		sim_switch_init();
	}

	TRACE_INFO("Modem %d: %s SIM\r\n", nr,
		   physical ? "physical" : "virtual");

	switch (nr) {
	case 0:
		led = LED_USIM1;
		break;

	default:
		TRACE_ERROR("Invalid SIM%u\r\n", nr);
		return -1;
	}

	if (physical) {
		TRACE_INFO("%u: Use local/physical SIM\r\n", nr);
		PIO_Configure(pins_default, PIO_LISTSIZE(pins_default));
		led_blink(led, BLINK_ALWAYS_ON);
	} else {
		TRACE_INFO("%u: Use remote/emulated SIM\r\n", nr);
		PIO_Configure(pins_cem, PIO_LISTSIZE(pins_cem));
		led_blink(led, BLINK_5O_5F);
	}

	/* just power cycle the modem because this circumvents weird issues with unreliable signals */
	PIO_Clear(&pin);
	mdelay(200);
	PIO_Set(&pin);


	return 0;
}

int sim_switch_init(void)
{
	PIO_Configure(pins_default, PIO_LISTSIZE(pins_default));
	initialized = 1;
	return 1;
}
