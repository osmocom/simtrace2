/* sysmoOCTSIMTEST support for multiplexers
 *
 * (C) 2021 by Harald Welte <laforge@gnumonks.org>
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
#include "mux.h"
#include <stdbool.h>

/* 3-bit S0..S2 signal for slot selection */
static const Pin pin_in_sel[3] = {
	{ PIO_PA1, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT },
	{ PIO_PA2, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT },
	{ PIO_PA3, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT },
};

/* 3-bit S0..S2 signal for frequency divider selection */
static const Pin pin_freq_sel[3] = {
	{ PIO_PA16, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT },
	{ PIO_PA17, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT },
	{ PIO_PA18, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT },
};

/* low-active output enable for all muxes */
static const Pin pin_oe = { PIO_PA19, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT };

/* initialize the external 1:8 multiplexers */
void mux_init(void)
{
	PIO_Configure(&pin_oe, PIO_LISTSIZE(pin_oe));
	PIO_Configure(pin_in_sel, PIO_LISTSIZE(pin_in_sel));
	PIO_Configure(pin_freq_sel, PIO_LISTSIZE(pin_freq_sel));

	mux_set_slot(0);
}

/* set the slot selection mux */
void mux_set_slot(uint8_t s)
{
	printf("%s(%u)\r\n", __func__, s);

	/* !OE = H: disconnect input and output of muxes */
	PIO_Set(&pin_oe);

	if (s & 1)
		PIO_Set(&pin_in_sel[0]);
	else
		PIO_Clear(&pin_in_sel[0]);
	if (s & 2)
		PIO_Set(&pin_in_sel[1]);
	else
		PIO_Clear(&pin_in_sel[1]);
	if (s & 4)
		PIO_Set(&pin_in_sel[2]);
	else
		PIO_Clear(&pin_in_sel[2]);

	/* !OE = L: (re-)enable the output of muxes */
	PIO_Clear(&pin_oe);
}

/* set the frequency divider mux */
void mux_set_freq(uint8_t s)
{
	printf("%s(%u)\r\n", __func__, s);

	/* no need for 'break before make' here, this would also affect
	 * the SIM card I/O signals which we don't want to disturb */

	if (s & 1)
		PIO_Set(&pin_freq_sel[0]);
	else
		PIO_Clear(&pin_freq_sel[0]);
	if (s & 2)
		PIO_Set(&pin_freq_sel[1]);
	else
		PIO_Clear(&pin_freq_sel[1]);
	if (s & 4)
		PIO_Set(&pin_freq_sel[2]);
	else
		PIO_Clear(&pin_freq_sel[2]);

	/* !OE = L: ensure enable the output of muxes */
	PIO_Clear(&pin_oe);
}
