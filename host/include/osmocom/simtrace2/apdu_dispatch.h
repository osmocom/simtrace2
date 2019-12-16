/* apdu_dispatch - State machine to determine Rx/Tx phases of APDU
 *
 * (C) 2016-2019 by Harald Welte <hwelte@hmw-consulting.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <osmocom/sim/sim.h>

struct osmo_apdu_context {
	struct osim_apdu_cmd_hdr hdr;
	uint8_t dc[256];
	uint8_t de[256];
	uint8_t sw[2];
	uint8_t apdu_case;
	struct {
		uint8_t tot;
		uint8_t cur;
	} lc;
	struct {
		uint8_t tot;
		uint8_t cur;
	} le;
};

enum osmo_apdu_action {
	APDU_ACT_TX_CAPDU_TO_CARD		= 0x0001,
	APDU_ACT_RX_MORE_CAPDU_FROM_READER	= 0x0002,
};

const char *osmo_apdu_dump_context_buf(char *buf, unsigned int buf_len,
					const struct osmo_apdu_context *ac);

int osmo_apdu_segment_in(struct osmo_apdu_context *ac, const uint8_t *apdu_buf,
			 unsigned int apdu_len, bool new_apdu);
