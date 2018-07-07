/* apdu_dispatch - State machine to determine Rx/Tx phases of APDU
 *
 * (C) 2016 by Harald Welte <hwelte@hmw-consulting.de>
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>

#include <osmocom/core/utils.h>
#include <osmocom/sim/sim.h>
#include <osmocom/sim/class_tables.h>

#include "apdu_dispatch.h"

/*! \brief Has the command-data phase been completed yet? */
static inline bool is_dc_complete(struct apdu_context *ac)
{
	return (ac->lc.tot == ac->lc.cur);
}

/*! \brief Has the expected-data phase been completed yet? */
static inline bool is_de_complete(struct apdu_context *ac)
{
	return (ac->le.tot == ac->le.cur);
}

static const char *dump_apdu_hdr(const struct osim_apdu_cmd_hdr *h)
{
	static char buf[256];
	sprintf(buf, "CLA=%02x INS=%02x P1=%02x P2=%02x P3=%02x",
		h->cla, h->ins, h->p1, h->p2, h->p3);

	return buf;
}

static void dump_apdu_ctx(const struct apdu_context *ac)
{
	printf("%s; case=%d, lc=%d(%d), le=%d(%d)\n",
		dump_apdu_hdr(&ac->hdr), ac->apdu_case,
		ac->lc.tot, ac->lc.cur,
		ac->le.tot, ac->le.cur);
}

/*! \brief input function for APDU segmentation
 *  \param ac APDU context accross successive calls
 *  \param[in] apdu_buf APDU inpud data buffer
 *  \param[in] apdu_len Length of apdu_buf
 *  \param[in] new_apdu Is this the beginning of a new APDU?
 *
 *  The function returns APDU_ACT_TX_CAPDU_TO_CARD once there is
 *  sufficient data of the APDU received to transmit the command-APDU to
 *  the actual card.
 *
 *  The function retunrs APDU_ACT_RX_MORE_CAPDU_FROM_READER when there
 *  is more data to be received from the card reader (GSM Phone).
 */
int apdu_segment_in(struct apdu_context *ac, const uint8_t *apdu_buf,
		    unsigned int apdu_len, bool new_apdu)
{
	int rc = 0;

	if (new_apdu) {
		/* initialize the apdu context structure */
		memset(ac, 0, sizeof(*ac));
		/* copy APDU header over */
		memcpy(&ac->hdr, apdu_buf, sizeof(ac->hdr));
		ac->apdu_case = osim_determine_apdu_case(&osim_uicc_sim_cic_profile, apdu_buf);
		switch (ac->apdu_case) {
		case 1: /* P3 == 0, No Lc/Le */
			ac->le.tot = ac->lc.tot = 0;
			break;
		case 2: /* P3 == Le */
			ac->le.tot = ac->hdr.p3;
			break;
		case 3: /* P3 = Lc */
			ac->lc.tot = ac->hdr.p3;
			/* copy Dc */
			ac->lc.cur = apdu_len - sizeof(ac->hdr);
			memcpy(ac->dc, apdu_buf + sizeof(ac->hdr),
				ac->lc.cur);
			break;
		case 4: /* P3 = Lc; SW with Le */
			ac->lc.tot = ac->hdr.p3;
			/* copy Dc */
			ac->lc.cur = apdu_len - sizeof(ac->hdr);
			memcpy(ac->dc, apdu_buf + sizeof(ac->hdr),
				ac->lc.cur);
			break;
		case 0:
		default:
			fprintf(stderr, "Unknown APDU case %d\n", ac->apdu_case);
			return -1;
		}
	} else {
		/* copy more data, if available */
		int cpy_len;
		switch (ac->apdu_case) {
		case 1:
		case 2:
			break;
		case 3:
		case 4:
			cpy_len = ac->lc.tot - ac->lc.cur;
			if (cpy_len > apdu_len)
				cpy_len = apdu_len;
			memcpy(ac->dc+ac->lc.cur, apdu_buf, cpy_len);
			ac->lc.cur += cpy_len;
			break;
		default:
			fprintf(stderr, "Unknown APDU case %d\n", ac->apdu_case);
			break;
		}
	}

	/* take some decisions... */
	switch (ac->apdu_case) {
	case 1: /* P3 == 0, No Lc/Le */
		/* send C-APDU to card */
		/* receive SW from card, forward to reader */
		rc |= APDU_ACT_TX_CAPDU_TO_CARD;
		break;
	case 2: /* P3 == Le */
		/* send C-APDU to card */
		/* receive Le bytes + SW from card, forward to reader */
		rc |= APDU_ACT_TX_CAPDU_TO_CARD;
		break;
	case 3: /* P3 = Lc */
		if (!is_dc_complete(ac)) {
			/* send PB + read further Lc bytes from reader */
			rc |= APDU_ACT_RX_MORE_CAPDU_FROM_READER;
		} else {
			/* send C-APDU to card */
			/* receive SW from card, forward to reader */
			rc |= APDU_ACT_TX_CAPDU_TO_CARD;
		}
		break;
	case 4: /* P3 = Lc; SW with Le */
		if (!is_dc_complete(ac)) {
			/* send PB + read further Lc bytes from reader */
			rc |= APDU_ACT_RX_MORE_CAPDU_FROM_READER;
		} else {
			/* send C-APDU to card */
			/* receive SW from card, forward to reader */
			rc |= APDU_ACT_TX_CAPDU_TO_CARD;
		}
		break;
	case 0:
	default:
		fprintf(stderr, "Unknown APDU case %d\n", ac->apdu_case);
		break;
	}

	dump_apdu_ctx(ac);

	return rc;
}
