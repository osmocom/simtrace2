/* card presence utilities
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
 */
#include <osmocom/core/timer.h>
#include "board.h"
#include "utils.h"
#include "card_pres.h"

#define NUM_CARDPRES	2

#define TIMER_INTERVAL_MS	500

static const Pin pin_cardpres[NUM_CARDPRES] = { PIN_DET_USIM1_PRES, PIN_DET_USIM2_PRES };
static int last_state[NUM_CARDPRES] = { -1, -1 };
static struct osmo_timer_list cardpres_timer;

/* Determine if a SIM card is present in the given slot */
int is_card_present(int port)
{
	const Pin *pin;
	int present;

	if (port < 0 || port >= NUM_CARDPRES)
		return -1;
	pin = &pin_cardpres[port];

	/* Card present signals are low-active, as we have a switch
	 * against GND and an internal-pull-up in the SAM3 */
	present = PIO_Get(pin) ? 0 : 1;

	return present;
}

static void cardpres_tmr_cb(void *data)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(pin_cardpres); i++) {
		int state = is_card_present(i);
		if (state != last_state[i]) {
			TRACE_INFO("%u: Card Detect Status %d -> %d\r\n", i, last_state[i], state);
			/* FIXME: report to USB host */
			last_state[i] = state;
		}
	}

	osmo_timer_schedule(&cardpres_timer, 0, TIMER_INTERVAL_MS*1000);
}

int card_present_init(void)
{
	unsigned int i;

	PIO_Configure(pin_cardpres, ARRAY_SIZE(pin_cardpres));

	/* start timer */
	cardpres_timer.cb = cardpres_tmr_cb;
	osmo_timer_schedule(&cardpres_timer, 0, TIMER_INTERVAL_MS*1000);

	return 2;
}
