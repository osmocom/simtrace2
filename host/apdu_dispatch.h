#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <osmocom/sim/sim.h>

struct apdu_context {
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

enum apdu_action {
	APDU_ACT_TX_CAPDU_TO_CARD		= 0x0001,
	APDU_ACT_RX_MORE_CAPDU_FROM_READER	= 0x0002,
};


int apdu_segment_in(struct apdu_context *ac, const uint8_t *apdu_buf,
		    unsigned int apdu_len, bool new_apdu);
