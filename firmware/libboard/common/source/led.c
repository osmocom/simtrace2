/* LED control
 *
 * (C) 2015-2017 by Harald Welte <hwelte@hmw-consulting.de>
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
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <osmocom/core/timer.h>

#include "board.h"
#include "utils.h"
#include "led.h"

#ifdef PINS_LEDS
static const Pin pinsLeds[] = { PINS_LEDS } ;

static void led_set(enum led led, int on)
{
	ASSERT(led < PIO_LISTSIZE(pinsLeds));

	if (on)
			PIO_Clear(&pinsLeds[led]);
	else
			PIO_Set(&pinsLeds[led]);
}

/* LED blinking code */

/* a single state in a sequence of blinking */
struct blink_state {
	/* duration of the state in ms */
	uint16_t duration;
	/* brightness of LED during the state */
	uint8_t on;
} __attribute__((packed));

static const struct blink_state bs_off[] = {
	{ 0, 0 }
};

static const struct blink_state bs_on[] = {
	{ 0, 1 }
};

static const struct blink_state bs_5on_5off[] = {
	{ 500, 1 }, { 500, 0 }
};

static const struct blink_state bs_3on_5off[] = {
	{ 300, 1 }, { 500, 0 }
};

static const struct blink_state bs_3on_30off[] = {
	{ 300, 1 }, { 3000, 0 }
};

static const struct blink_state bs_3on_1off_3on_30off[] = {
	{ 300, 1 }, { 100, 0 }, { 300, 1 }, { 3000, 0 }
};

static const struct blink_state bs_3on_1off_3on_1off_3on_30off[] = {
	{ 300, 1 }, { 100, 0 }, { 300, 1 }, { 100, 0 }, { 300, 1 }, { 3000, 0 }
};

static const struct blink_state bs_2on_off[] = {
	{ 200, 1 }, { 0, 0 },
};

static const struct blink_state bs_200on_off[] = {
	{ 20000, 1 }, { 0, 0 },
};

static const struct blink_state bs_600on_off[] = {
	{ 60000, 1 }, { 0, 0 },
};

static const struct blink_state bs_2off_on[] = {
	{ 200, 0 }, { 0, 1 },
};


/* a blink pattern is an array of blink_states */
struct blink_pattern {
	const struct blink_state *states;
	uint16_t size;
};

/* compiled-in default blinking patterns */
static const struct blink_pattern patterns[] = {
	[BLINK_ALWAYS_OFF] = {
		.states = bs_off,
		.size = ARRAY_SIZE(bs_off),
	},
	[BLINK_ALWAYS_ON] = {
		.states = bs_on,
		.size = ARRAY_SIZE(bs_on),
	},
	[BLINK_5O_5F] = {
		.states = bs_5on_5off,
		.size = ARRAY_SIZE(bs_5on_5off),
	},
	[BLINK_3O_5F] = {
		.states = bs_3on_5off,
		.size = ARRAY_SIZE(bs_3on_5off),
	},
	[BLINK_3O_30F] = {
		.states = bs_3on_30off,
		.size = ARRAY_SIZE(bs_3on_30off),
	},
	[BLINK_3O_1F_3O_30F] = {
		.states = bs_3on_1off_3on_30off,
		.size = ARRAY_SIZE(bs_3on_1off_3on_30off),
	},
	[BLINK_3O_1F_3O_1F_3O_30F] = {
		.states = bs_3on_1off_3on_1off_3on_30off,
		.size = ARRAY_SIZE(bs_3on_1off_3on_1off_3on_30off),
	},
	[BLINK_2O_F] = {
		.states = bs_2on_off,
		.size = ARRAY_SIZE(bs_2on_off),
	},
	[BLINK_200O_F] = {
		.states = bs_200on_off,
		.size = ARRAY_SIZE(bs_200on_off),
	},
	[BLINK_600O_F] = {
		.states = bs_600on_off,
		.size = ARRAY_SIZE(bs_600on_off),
	},
	[BLINK_2F_O] = {
		.states = bs_2off_on,
		.size = ARRAY_SIZE(bs_2off_on),
	},

};

struct led_state {
	/* which led are we handling */
	enum led led;

	/* timer */
	struct osmo_timer_list timer;

	/* pointer and size of blink array */
	const struct blink_pattern *pattern;

	unsigned int cur_state;
	unsigned int illuminated;

	/* static allocated space for custom blinking pattern */
	struct blink_pattern pattern_cust;
	struct blink_state blink_cust[10];
};

static unsigned int cur_state_inc(struct led_state *ls)
{
	ls->cur_state = (ls->cur_state + 1) % ls->pattern->size;
	return ls->cur_state;
}

static const struct blink_state *
next_blink_state(struct led_state *ls)
{
	return &ls->pattern->states[cur_state_inc(ls)];
}

/* apply the next state to the LED */
static void apply_blinkstate(struct led_state *ls,
			     const struct blink_state *bs)
{
	led_set(ls->led, bs->on);
	ls->illuminated = bs->on;

	/* re-schedule the timer */
	if (bs->duration) {
		uint32_t us = bs->duration * 1000;
		osmo_timer_schedule(&ls->timer, us / 1000000, us % 1000000);
	}
}

static void blink_tmr_cb(void *data)
{
	struct led_state *ls = data;
	const struct blink_state *next_bs = next_blink_state(ls);

	/* apply the next state to the LED */
	apply_blinkstate(ls, next_bs);
}

static struct led_state led_state[] = {
	[LED_RED] = {
		.led = LED_RED,
		.timer.cb = blink_tmr_cb,
		.timer.data = &led_state[LED_RED],
	},
	[LED_GREEN] = {
		.led = LED_GREEN,
		.timer.cb = blink_tmr_cb,
		.timer.data = &led_state[LED_GREEN],
	},
};
#endif /* PINS_LEDS */

void led_blink(enum led led, enum led_pattern blink)
{
#ifdef PINS_LEDS
	struct led_state *ls;

	if (led >= ARRAY_SIZE(led_state))
		return;
	ls = &led_state[led];

	/* stop previous blinking, if any */
	osmo_timer_del(&ls->timer);
	led_set(led, 0);
	ls->illuminated = 0;
	ls->pattern = NULL;
	ls->cur_state = 0;

	switch (blink) {
	case BLINK_CUSTOM:
		ls->pattern = &ls->pattern_cust;
		break;
	default:
		if (blink >= ARRAY_SIZE(patterns))
			return;
		ls->pattern = &patterns[blink];
		break;
	}

	if (ls->pattern && ls->pattern->size > 0)
		apply_blinkstate(ls, &ls->pattern->states[0]);
#endif
}

enum led_pattern led_get(enum led led)
{
#ifdef PINS_LEDS
	struct led_state *ls;
	unsigned int i;

	if (led >= ARRAY_SIZE(led_state))
		return -1;
	ls = &led_state[led];

	if (ls->pattern == &ls->pattern_cust)
		return BLINK_CUSTOM;

	for (i = 0; i < ARRAY_SIZE(patterns); i++) {
		if (ls->pattern == &patterns[i])
			return i;
	}
#endif
	/* default case, shouldn't be reached */
	return -1;
}

void led_start(void)
{
	led_set(LED_GREEN, led_state[LED_GREEN].illuminated);
	led_set(LED_RED, led_state[LED_RED].illuminated);
}

void led_stop(void)
{
	led_set(LED_GREEN, 0);
	led_set(LED_RED, 0);
}

void led_init(void)
{
#ifdef PINS_LEDS
	PIO_Configure(pinsLeds, PIO_LISTSIZE(pinsLeds));
	led_set(LED_GREEN, 0);
	led_set(LED_RED, 0);
#endif
}

void led_fini(void)
{
#ifdef PINS_LEDS
	/* we don't actually need to do this, but just in case... */
	osmo_timer_del(&led_state[LED_RED].timer);
	osmo_timer_del(&led_state[LED_GREEN].timer);
	led_set(LED_GREEN, 0);
	led_set(LED_RED, 0);
#endif
}
