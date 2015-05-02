/* Driver for AT91SAM7 USART0 in ISO7816-3 mode for passive sniffing
 * (C) 2010 by Harald Welte <hwelte@hmw-consulting.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "board.h"
#include "errno.h"

#define _PTSS	0
#define _PTS0	1
#define _PTS1	2
#define _PTS2	3
#define _PTS3	4
#define _PCK	5

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static struct Usart_info usart_info = {.base = USART_PHONE, .id = ID_USART_PHONE, .state = USART_RCV};

/* Table 6 from ISO 7816-3 */
static const uint16_t fi_table[] = {
	0, 372, 558, 744, 1116, 1488, 1860, 0,
	0, 512, 768, 1024, 1536, 2048, 0, 0
};

/* Table 7 from ISO 7816-3 */
static const uint8_t di_table[] = {
	0, 1, 2, 4, 8, 16, 0, 0,
	0, 0, 2, 4, 8, 16, 32, 64,
};

/* compute the F/D ratio based on Fi and Di values */
static int compute_fidi_ratio(uint8_t fi, uint8_t di)
{
	uint16_t f, d;
	int ret;

	if (fi >= ARRAY_SIZE(fi_table) ||
	    di >= ARRAY_SIZE(di_table))
		return -EINVAL;

	f = fi_table[fi];
	if (f == 0)
		return -EINVAL;

	d = di_table[di];
	if (d == 0)
		return -EINVAL;

	if (di < 8) 
		ret = f / d;
	else
		ret = f * d;

	return ret;
}

static void update_fidi(struct iso7816_3_handle *ih)
{
	int rc;

	rc = compute_fidi_ratio(ih->fi, ih->di);
	if (rc > 0 && rc < 0x400) {
		TRACE_INFO("computed Fi(%u) Di(%u) ratio: %d", ih->fi, ih->di, rc);
/*              make sure UART uses new F/D ratio */
		USART_PHONE->US_CR |= US_CR_RXDIS | US_CR_RSTRX;
		USART_PHONE->US_FIDI = rc & 0x3ff;
		USART_PHONE->US_CR |= US_CR_RXEN | US_CR_STTTO;
	} else
		TRACE_INFO("computed FiDi ratio %d unsupported", rc);
}

/* Update the ATR sub-state */
static void set_pts_state(struct iso7816_3_handle *ih, enum pts_state new_ptss)
{
	//DEBUGPCR("PTS state %u -> %u", ih->pts_state, new_ptss);
	ih->pts_state = new_ptss;
}

/* Determine the next PTS state */
static enum pts_state next_pts_state(struct iso7816_3_handle *ih)
{
	uint8_t is_resp = ih->pts_state & 0x10;
	uint8_t sstate = ih->pts_state & 0x0f;
	uint8_t *pts_ptr;

        if (ih->pts_state == PTS_END) {
            return PTS_END;
        }
	if (!is_resp)
		pts_ptr = ih->pts_req;
	else
		pts_ptr = ih->pts_resp;

	switch (sstate) {
	case PTS_S_WAIT_REQ_PTSS:
		goto from_ptss;
	case PTS_S_WAIT_REQ_PTS0:
		goto from_pts0;
	case PTS_S_WAIT_REQ_PTS1:
		goto from_pts1;
	case PTS_S_WAIT_REQ_PTS2:
		goto from_pts2;
	case PTS_S_WAIT_REQ_PTS3:
		goto from_pts3;
	}

	if (ih->pts_state == PTS_S_WAIT_REQ_PCK)
		return PTS_S_WAIT_RESP_PTSS;

from_ptss:
	return PTS_S_WAIT_REQ_PTS0 | is_resp;
from_pts0:
	if (pts_ptr[_PTS0] & (1 << 4))
		return PTS_S_WAIT_REQ_PTS1 | is_resp;
from_pts1:
	if (pts_ptr[_PTS0] & (1 << 5))
		return PTS_S_WAIT_REQ_PTS2 | is_resp;
from_pts2:
	if (pts_ptr[_PTS0] & (1 << 6))
		return PTS_S_WAIT_REQ_PTS3 | is_resp;
from_pts3:
	return PTS_S_WAIT_REQ_PCK | is_resp;
}

enum pts_state process_byte_pts(struct iso7816_3_handle *ih, uint8_t byte)
{
        printf("PTS: %x, stat: %x\n", byte, ih->pts_state);
	switch (ih->pts_state) {
	case PTS_S_WAIT_REQ_PTSS:
		ih->pts_req[_PTSS] = byte;
		break;
	case PTS_S_WAIT_REQ_PTS0:
		ih->pts_req[_PTS0] = byte;
		break;
	case PTS_S_WAIT_REQ_PTS1:
		ih->pts_req[_PTS1] = byte;
		break;
	case PTS_S_WAIT_REQ_PTS2:
		ih->pts_req[_PTS2] = byte;
		break;
	case PTS_S_WAIT_REQ_PTS3:
		ih->pts_req[_PTS3] = byte;
		break;
	case PTS_S_WAIT_REQ_PCK:
		/* FIXME: check PCK */
		ih->pts_req[_PCK] = byte;
		break;
	case PTS_S_WAIT_RESP_PTSS:
		ih->pts_resp[_PTSS] = byte;
		break;
	case PTS_S_WAIT_RESP_PTS0:
		ih->pts_resp[_PTS0] = byte;
		break;
	case PTS_S_WAIT_RESP_PTS1:
		/* This must be TA1 */
		ih->fi = byte >> 4;
		ih->di = byte & 0xf;
		TRACE_DEBUG("found Fi=%u Di=%u", ih->fi, ih->di);
		ih->pts_resp[_PTS1] = byte;
		break;
	case PTS_S_WAIT_RESP_PTS2:
		ih->pts_resp[_PTS2] = byte;
		break;
	case PTS_S_WAIT_RESP_PTS3:
		ih->pts_resp[_PTS3] = byte;
		break;
	case PTS_S_WAIT_RESP_PCK:
		ih->pts_resp[_PCK] = byte;
		/* FIXME: check PCK */
                for (int i = 0; ih->pts_resp != 0; i++)
                    ISO7816_SendChar(ih->pts_req[i], &usart_info);
		/* update baud rate generator with Fi/Di */
		update_fidi(ih);
		//set_pts_state(ih, PTS_S_WAIT_REQ_PTSS);
		/* Wait for the next APDU */
		ih->pts_state = PTS_END;
        case PTS_END:
                TRACE_INFO("PTS state PTS_END reached");
	}
	/* calculate the next state and set it */
	set_pts_state(ih, next_pts_state(ih));
        printf("stat: %x\n", ih->pts_state);
	return ih->pts_state;
}
