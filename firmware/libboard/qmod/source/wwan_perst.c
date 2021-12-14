/* Code to control the PERST lines of attached modems
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
/* Depending on the board this is running on,  it might be possible
 * for the controller to set the status of the PERST input line of
 * the cellular modem.  If the board supports this, it sets the
 * PIN_PERST1 and/or PIN_PERST2 defines in its board.h file.
 */
#include "board.h"
#include "trace.h"
#include "wwan_perst.h"
#include <osmocom/core/timer.h>

struct wwan_perst {
	uint8_t idx;
	const Pin pin;
	struct osmo_timer_list timer;
};

#ifdef PIN_PERST1
static struct wwan_perst perst1 = {
	.idx = 0,
	.pin = PIN_PERST1,
};
#endif

#ifdef PIN_PERST2
static struct wwan_perst perst2 = {
	.idx = 1,
	.pin = PIN_PERST2,
};
#endif

static int initialized = 0;

static void perst_tmr_cb(void *data)
{
	struct wwan_perst *perst = data;
	/* release the (low-active) reset */
	TRACE_INFO("%u: De-asserting modem reset\r\n", perst->idx);
	PIO_Clear(&perst->pin);
}

static struct wwan_perst *get_perst_for_modem(int modem_nr)
{
	if (!initialized) {
		TRACE_ERROR("Somebody forgot to call wwan_perst_init()\r\n");
		wwan_perst_init();
	}

	switch (modem_nr) {
#ifdef PIN_PERST1
	case 0:
		return &perst1;
#endif
#ifdef PIN_PERST2
	case 1:
		return &perst2;
#endif
	default:
		return NULL;
	}
}

int wwan_perst_do_reset_pulse(int modem_nr, unsigned int duration_ms)
{
	struct wwan_perst *perst = get_perst_for_modem(modem_nr);
	if (!perst)
		return -1;

	TRACE_INFO("%u: Asserting modem reset\r\n", modem_nr);
	PIO_Set(&perst->pin);
	osmo_timer_schedule(&perst->timer, duration_ms/1000, (duration_ms%1000)*1000);

	return 0;
}

int wwan_perst_set(int modem_nr, int active)
{
	struct wwan_perst *perst = get_perst_for_modem(modem_nr);
	if (!perst)
		return -1;

	osmo_timer_del(&perst->timer);
	if (active) {
		TRACE_INFO("%u: Asserting modem reset\r\n", modem_nr);
		PIO_Set(&perst->pin);
	} else {
		TRACE_INFO("%u: De-asserting modem reset\r\n", modem_nr);
		PIO_Clear(&perst->pin);
	}

	return 0;
}

int wwan_perst_init(void)
{
	int num_perst = 0;
#ifdef PIN_PERST1
	PIO_Configure(&perst1.pin, 1);
	perst1.timer.cb = perst_tmr_cb;
	perst1.timer.data = (void *) &perst1;
	num_perst++;
#endif

#ifdef PIN_PERST2
	PIO_Configure(&perst2.pin, 1);
	perst2.timer.cb = perst_tmr_cb;
	perst2.timer.data = (void *) &perst2;
	num_perst++;
#endif
	initialized = 1;
	return num_perst;
}
