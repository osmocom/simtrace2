/* SIMtrace TC (Timer / Clock) code for ETU tracking
 *
 * (C) 2006-2016 by Harald Welte <laforge@gnumonks.org>
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

#include "utils.h"
#include "tc_etu.h"

#include "chip.h"

/* pins for Channel 0 of TC-block 0, we only use TCLK + TIOB */
#define PIN_TCLK0	{PIO_PA4, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT }
#define PIN_TIOA0	{PIO_PA0, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
#define PIN_TIOB0	{PIO_PA1, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
static const Pin pins_tc0[] = { PIN_TCLK0, PIN_TIOB0 };

/* pins for Channel 2 of TC-block 0, we only use TCLK + TIOB */
#define PIN_TCLK2	{PIO_PA29, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
#define PIN_TIOA2	{PIO_PA26, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
#define PIN_TIOB2	{PIO_PA27, PIOA, ID_PIOA, PIO_PERIPH_B, PIO_DEFAULT}
static const Pin pins_tc2[] = { PIN_TCLK2, PIN_TIOB2 };

struct tc_etu_state {
	/* total negotiated waiting time (default = 9600) */
	uint16_t waiting_time;
	/* how many clock cycles per ETU (default = 372) */
	uint16_t clocks_per_etu;
	/* how many ETUs does waiting time correspond ? */
	uint16_t wait_events;
	/* how many ETUs have we seen expire so far? */
	uint16_t nr_events;
	/* channel number */
	uint8_t chan_nr;
	/* Timer/Counter register pointer */
	TcChannel *chan;
	/* User reference */
	void *handle;
};

#define INIT_TE_STATE(n)	{	\
	.waiting_time = 9600,		\
	.clocks_per_etu = 372,		\
	.wait_events = 10,		\
	.nr_events = 0,			\
	.chan_nr = n,			\
}

static struct tc_etu_state te_state0 = INIT_TE_STATE(0);
static struct tc_etu_state te_state2 = INIT_TE_STATE(2);

static struct tc_etu_state *get_te(uint8_t chan_nr)
{
	if (chan_nr == 0)
		return &te_state0;
	else
		return &te_state2;
}

static void tc_etu_irq(struct tc_etu_state *te)
{
	uint32_t sr = te->chan->TC_SR;

	if (sr & TC_SR_ETRGS) {
		/* external trigger, i.e. we have seen a bit on I/O */
		te->nr_events = 0;
	}

	if (sr & TC_SR_CPCS) {
		/* Compare C event has occurred, i.e. 1 ETU expired */
		te->nr_events++;
		if (te->nr_events == te->wait_events/2) {
			/* Indicate that half the waiting tim has expired */
			tc_etu_wtime_half_expired(te->handle);
		}
		if (te->nr_events >= te->wait_events) {
			TcChannel *chan = te->chan;
			chan->TC_CMR |= TC_CMR_ENETRG;

			/* disable and re-enable clock to make it stop */
			chan->TC_CCR = TC_CCR_CLKDIS;
			chan->TC_CCR = TC_CCR_CLKEN;

			/* Indicate that the waiting tim has expired */
			tc_etu_wtime_expired(te->handle);
		}
	}
}

void TC0_IrqHandler(void)
{
	tc_etu_irq(&te_state0);
}

void TC2_IrqHandler(void)
{
	tc_etu_irq(&te_state2);
}

static void recalc_nr_events(struct tc_etu_state *te)
{
	te->wait_events = te->waiting_time / 12;
	te->chan->TC_RC = te->clocks_per_etu * 12;
}

void tc_etu_set_wtime(uint8_t chan_nr, uint16_t wtime)
{
	struct tc_etu_state *te = get_te(chan_nr);
	te->waiting_time = wtime;
	recalc_nr_events(te);
}

void tc_etu_set_etu(uint8_t chan_nr, uint16_t etu)
{
	struct tc_etu_state *te = get_te(chan_nr);
	te->clocks_per_etu = etu;
	recalc_nr_events(te);
}

void tc_etu_enable(uint8_t chan_nr)
{
	struct tc_etu_state *te = get_te(chan_nr);

	te->nr_events = 0;
	te->chan->TC_CCR = TC_CCR_CLKEN|TC_CCR_SWTRG;
}

void tc_etu_disable(uint8_t chan_nr)
{
	struct tc_etu_state *te = get_te(chan_nr);

	te->nr_events = 0;
	te->chan->TC_CCR = TC_CCR_CLKDIS;
}

void tc_etu_init(uint8_t chan_nr, void *handle)
{
	struct tc_etu_state *te = get_te(chan_nr);
	uint32_t tc_clks;

	te->handle = handle;

	switch (chan_nr) {
	case 0:
		/* Configure PA4(TCLK0), PA1(TIB0) */
		PIO_Configure(pins_tc0, ARRAY_SIZE(pins_tc0));
		PMC_EnablePeripheral(ID_TC0);
		/* route TCLK0 to XC2 */
		TC0->TC_BMR &= ~TC_BMR_TC0XC0S_Msk;
		TC0->TC_BMR |= TC_BMR_TC0XC0S_TCLK0;
		tc_clks = TC_CMR_TCCLKS_XC0;
		/* register interrupt handler */
		NVIC_EnableIRQ(TC0_IRQn);

		te->chan = &TC0->TC_CHANNEL[0];
		break;
	case 2:
		/* Configure PA29(TCLK2), PA27(TIOB2) */
		PIO_Configure(pins_tc2, ARRAY_SIZE(pins_tc2));
		PMC_EnablePeripheral(ID_TC2);
		/* route TCLK2 to XC2. TC0 really means TCA in this case */
		TC0->TC_BMR &= ~TC_BMR_TC2XC2S_Msk;
		TC0->TC_BMR |= TC_BMR_TC2XC2S_TCLK2;
		tc_clks = TC_CMR_TCCLKS_XC2;
		/* register interrupt handler */
		NVIC_EnableIRQ(TC2_IRQn);

		te->chan = &TC0->TC_CHANNEL[2];
		break;
	default:
		return;
	}

	/* enable interrupts for Compare-C and external trigger */
	te->chan->TC_IER = TC_IER_CPCS | TC_IER_ETRGS;

	te->chan->TC_CMR = tc_clks |		/* XC(TCLK) clock */
		       TC_CMR_WAVE |		/* wave mode */
		       TC_CMR_ETRGEDG_FALLING |	/* ext trig on falling edge */
		       TC_CMR_EEVT_TIOB |	/* ext trig is TIOB0 */
		       TC_CMR_ENETRG |		/* enable ext trig */
		       TC_CMR_WAVSEL_UP_RC |	/* wave mode up */
		       TC_CMR_ACPA_SET |	/* set TIOA on a compare */
		       TC_CMR_ACPC_CLEAR |	/* clear TIOA on C compare */
		       TC_CMR_ASWTRG_CLEAR;	/* Clear TIOA on sw trig */

	tc_etu_set_etu(chan_nr, 372);

	/* start with a disabled clock */
	tc_etu_disable(chan_nr);

	/* Reset to start timers */
	TC0->TC_BCR = TC_BCR_SYNC;
}
