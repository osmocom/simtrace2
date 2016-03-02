/* ISO7816-3 state machine for the card side */
/* (C) 2010-2015 by Harald Welte <hwelte@hmw-consulting.de>
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

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>

#include "utils.h"
#include "trace.h"
#include "iso7816_fidi.h"
#include "tc_etu.h"
#include "card_emu.h"
#include "req_ctx.h"
#include "cardemu_prot.h"
#include "linuxlist.h"


#define NUM_SLOTS		2

#define	ISO7816_3_INIT_WTIME	9600
#define ISO7816_3_DEFAULT_WI	10
#define ISO7816_3_ATR_LEN_MAX	(1+32)	/* TS plus 32 chars */

#define ISO7816_3_PB_NULL	0x60

enum iso7816_3_card_state {
	ISO_S_WAIT_POWER,	/* waiting for power being applied */
	ISO_S_WAIT_CLK,		/* waiting for clock being applied */
	ISO_S_WAIT_RST,		/* waiting for reset being released */
	ISO_S_WAIT_ATR,		/* waiting for start of ATR */
	ISO_S_IN_ATR,		/* transmitting ATR to reader */
	ISO_S_IN_PTS,		/* transmitting ATR to reader */
	ISO_S_WAIT_TPDU,	/* waiting for data from reader */
	ISO_S_IN_TPDU,		/* inside a TPDU */
};

/* detailed sub-states of ISO_S_IN_PTS */
enum pts_state {
	PTS_S_WAIT_REQ_PTSS,
	PTS_S_WAIT_REQ_PTS0,
	PTS_S_WAIT_REQ_PTS1,
	PTS_S_WAIT_REQ_PTS2,
	PTS_S_WAIT_REQ_PTS3,
	PTS_S_WAIT_REQ_PCK,
	PTS_S_WAIT_RESP_PTSS = PTS_S_WAIT_REQ_PTSS | 0x10,
	PTS_S_WAIT_RESP_PTS0 = PTS_S_WAIT_REQ_PTS0 | 0x10,
	PTS_S_WAIT_RESP_PTS1 = PTS_S_WAIT_REQ_PTS1 | 0x10,
	PTS_S_WAIT_RESP_PTS2 = PTS_S_WAIT_REQ_PTS2 | 0x10,
	PTS_S_WAIT_RESP_PTS3 = PTS_S_WAIT_REQ_PTS3 | 0x10,
	PTS_S_WAIT_RESP_PCK = PTS_S_WAIT_REQ_PCK | 0x10,
};

#define _PTSS	0
#define _PTS0	1
#define _PTS1	2
#define _PTS2	3
#define _PTS3	4
#define _PCK	5

/* T-PDU state machine states */
enum tpdu_state {
	TPDU_S_WAIT_CLA,	/* waiting for CLA byte from reader */
	TPDU_S_WAIT_INS,	/* waiting for INS byte from reader */
	TPDU_S_WAIT_P1,		/* waiting for P1 byte from reader */
	TPDU_S_WAIT_P2,		/* waiting for P2 byte from reader */
	TPDU_S_WAIT_P3,		/* waiting for P3 byte from reader */
	TPDU_S_WAIT_PB,		/* waiting for Tx of procedure byte */
	TPDU_S_WAIT_RX,		/* waiitng for more data from reader */
	TPDU_S_WAIT_TX,		/* waiting for more data to reader */
};

#define	_CLA	0
#define	_INS	1
#define	_P1	2
#define	_P2	3
#define	_P3	4

struct card_handle {
	enum iso7816_3_card_state state;

	/* signal levels */
	uint8_t vcc_active;	/* 1 = on, 0 = off */
	uint8_t in_reset;	/* 1 = RST low, 0 = RST high */
	uint8_t clocked;	/* 1 = active, 0 = inactive */

	/* timing parameters, from PTS */
	uint8_t fi;
	uint8_t di;
	uint8_t wi;

	uint8_t tc_chan;	/* TC channel number */
	uint8_t uart_chan;	/* UART channel */

	uint32_t waiting_time;	/* in clocks */

	/* ATR state machine */
	struct {
		uint8_t idx;
		uint8_t len;
		//uint8_t hist_len;
		//uint8_t last_td;
		uint8_t atr[ISO7816_3_ATR_LEN_MAX];
	} atr;

	/* PPS / PTS support */
	struct {
		enum pts_state state;
		uint8_t req[6];		/* request bytes */
		uint8_t resp[6];	/* response bytes */
	} pts;

	/* TPDU */
	struct {
		enum tpdu_state state;
		uint8_t hdr[5];		/* CLA INS P1 P2 P3 */
	} tpdu;

	struct req_ctx *uart_rx_ctx;	/* UART RX -> USB TX */
	struct req_ctx *uart_tx_ctx;	/* USB RX -> UART TX */

	struct llist_head usb_tx_queue;
	struct llist_head uart_tx_queue;

	struct {
		uint32_t tx_bytes;
		uint32_t rx_bytes;
		uint32_t pps;
	} stats;
};

struct llist_head *card_emu_get_usb_tx_queue(struct card_handle *ch)
{
	return &ch->usb_tx_queue;
}

struct llist_head *card_emu_get_uart_tx_queue(struct card_handle *ch)
{
	return &ch->uart_tx_queue;
}

static void set_tpdu_state(struct card_handle *ch, enum tpdu_state new_ts);
static void set_pts_state(struct card_handle *ch, enum pts_state new_ptss);

static void flush_rx_buffer(struct card_handle *ch)
{
	struct req_ctx *rctx;
	struct cardemu_usb_msg_rx_data *rd;

	rctx = ch->uart_rx_ctx;
	if (!rctx)
		return;

	ch->uart_rx_ctx = NULL;

	/* store length of data payload fild in header */
	rd = (struct cardemu_usb_msg_rx_data *) rctx->data;
	rd->hdr.data_len = rctx->idx;

	llist_add_tail(&rctx->list, &ch->usb_tx_queue);
	req_ctx_set_state(rctx, RCTX_S_USB_TX_PENDING);

	/* FIXME: call into USB code to see if this buffer can
	 * be transmitted now */
}

/* convert a non-contiguous PTS request/responsei into a contiguous
 * buffer, returning the number of bytes used in the buffer */
static int serialize_pts(uint8_t *out,  const uint8_t *in)
{
	int i = 0;

	out[i++] = in[_PTSS];
	out[i++] = in[_PTS0];
	if (in[_PTS0] & (1 << 4))
		out[i++] = in[_PTS1];
	if (in[_PTS0] & (1 << 5))
		out[i++] = in[_PTS2];
	if (in[_PTS0] & (1 << 6))
		out[i++] = in[_PTS3];
	out[i++] = in[_PCK];

	return i;
}

static uint8_t csum_pts(const uint8_t *in)
{
	uint8_t out[6];
	int len = serialize_pts(out, in);
	uint8_t csum = 0;
	int i;

	/* we don't include the PCK byte in the checksumming process */
	len -= 1;

	for (i = 0; i < len; i++)
		csum = csum ^ out[i];

	return csum;
}

static void flush_pts(struct card_handle *ch)
{
	struct req_ctx *rctx;
	struct cardemu_usb_msg_pts_info *ptsi;

	rctx = req_ctx_find_get(0, RCTX_S_FREE, RCTX_S_UART_RX_BUSY);
	if (!rctx)
		return;

	ptsi = (struct cardemu_usb_msg_pts_info *) rctx->data;
	ptsi->hdr.msg_type = CEMU_USB_MSGT_DO_PTS;
	ptsi->hdr.data_len = serialize_pts(ptsi->req, ch->pts.req);
	serialize_pts(ptsi->resp, ch->pts.resp);

	llist_add_tail(&rctx->list, &ch->usb_tx_queue);
	req_ctx_set_state(rctx, RCTX_S_USB_TX_PENDING);

	/* FIXME: call into USB code to see if this buffer can
	 * be transmitted now */
}

static void emu_update_fidi(struct card_handle *ch)
{
	int rc;

	rc = compute_fidi_ratio(ch->fi, ch->di);
	if (rc > 0 && rc < 0x400) {
		TRACE_DEBUG("computed Fi(%u) Di(%u) ratio: %d\r\n",
			    ch->fi, ch->di, rc);
		/* make sure UART uses new F/D ratio */
		card_emu_uart_update_fidi(ch->uart_chan, rc);
		/* notify ETU timer about this */
		tc_etu_set_etu(ch->tc_chan, rc);
	} else
		TRACE_DEBUG("computed FiDi ration %d unsupported\r\n", rc);
}

/* Update the ISO 7816-3 TPDU receiver state */
static void card_set_state(struct card_handle *ch,
			   enum iso7816_3_card_state new_state)
{
	switch (new_state) {
	case ISO_S_WAIT_POWER:
	case ISO_S_WAIT_CLK:
	case ISO_S_WAIT_RST:
		/* disable Rx and Tx of UART */
		card_emu_uart_enable(ch->uart_chan, 0);
		break;
	case ISO_S_WAIT_ATR:
		set_pts_state(ch, PTS_S_WAIT_REQ_PTSS);
		/* Reset to initial Fi / Di ratio */
		ch->fi = 1;
		ch->di = 1;
		emu_update_fidi(ch);
		/* initialize todefault WI, this will be overwritten if we
		 * receive TC2, and it will be programmed into hardware after
		 * ATR is finished */
		ch->wi = ISO7816_3_DEFAULT_WI;
		/* update waiting time to initial waiting time */
		ch->waiting_time = ISO7816_3_INIT_WTIME;
		tc_etu_set_wtime(ch->tc_chan, ch->waiting_time);
		/* Set ATR sub-state to initial state */
		ch->atr.idx = 0;
		//set_atr_state(ch, ATR_S_WAIT_TS);
		/* Notice that we are just coming out of reset */
		//ch->sh.flags |= SIMTRACE_FLAG_ATR;
		card_emu_uart_enable(ch->uart_chan, ENABLE_TX);
		break;
		break;
	case ISO_S_WAIT_TPDU:
		/* enable the receiver, disable transmitter */
		set_tpdu_state(ch, TPDU_S_WAIT_CLA);
		card_emu_uart_enable(ch->uart_chan, ENABLE_RX);
		break;
	case ISO_S_IN_ATR:
	case ISO_S_IN_PTS:
	case ISO_S_IN_TPDU:
		/* do nothing */
		break;
	}

	if (ch->state == new_state)
		return;

	TRACE_DEBUG("7816 card state %u -> %u\r\n", ch->state, new_state);
	ch->state = new_state;
}


/**********************************************************************
 * PTS / PPS handling
 **********************************************************************/

/* Update the ATR sub-state */
static void set_pts_state(struct card_handle *ch, enum pts_state new_ptss)
{
	TRACE_DEBUG("7816 PTS state %u -> %u\r\n", ch->pts.state, new_ptss);
	ch->pts.state = new_ptss;
}

/* Determine the next PTS state */
static enum pts_state next_pts_state(struct card_handle *ch)
{
	uint8_t is_resp = ch->pts.state & 0x10;
	uint8_t sstate = ch->pts.state & 0x0f;
	uint8_t *pts_ptr;

	if (!is_resp)
		pts_ptr = ch->pts.req;
	else
		pts_ptr = ch->pts.resp;

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

	if (ch->pts.state == PTS_S_WAIT_REQ_PCK)
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


static enum iso7816_3_card_state
process_byte_pts(struct card_handle *ch, uint8_t byte)
{
	switch (ch->pts.state) {
	case PTS_S_WAIT_REQ_PTSS:
		ch->pts.req[_PTSS] = byte;
		break;
	case PTS_S_WAIT_REQ_PTS0:
		ch->pts.req[_PTS0] = byte;
		break;
	case PTS_S_WAIT_REQ_PTS1:
		ch->pts.req[_PTS1] = byte;
		break;
	case PTS_S_WAIT_REQ_PTS2:
		ch->pts.req[_PTS2] = byte;
		break;
	case PTS_S_WAIT_REQ_PTS3:
		ch->pts.req[_PTS3] = byte;
		break;
	case PTS_S_WAIT_REQ_PCK:
		ch->pts.req[_PCK] = byte;
		if (ch->pts.req[_PCK] != csum_pts(ch->pts.req)) {
			TRACE_DEBUG("Error in PTS Checksum!\r\n");
			/* Wait for the next TPDU */
			set_pts_state(ch, PTS_S_WAIT_REQ_PTSS);
			return ISO_S_WAIT_TPDU;
		}
		/* FIXME: check if proposal matches capabilities in ATR */
		memcpy(ch->pts.resp, ch->pts.req, sizeof(ch->pts.resp));
		break;
	default:
		TRACE_DEBUG("process_byte_pts() in invalid state %u\r\n",
			ch->pts.state);
		break;
	}
	/* calculate the next state and set it */
	set_pts_state(ch, next_pts_state(ch));

	if (ch->pts.state == PTS_S_WAIT_RESP_PTSS) {
		flush_pts(ch);
		/* activate UART TX to transmit PTS response */
		card_emu_uart_enable(ch->uart_chan, ENABLE_TX);
	}

	return ISO_S_IN_PTS;
}

/* return a single byte to be transmitted to the reader */
static int tx_byte_pts(struct card_handle *ch)
{
	uint8_t byte;

	/* 1: Determine the next transmit byte */
	switch (ch->pts.state) {
	case PTS_S_WAIT_RESP_PTSS:
		byte = ch->pts.resp[_PTSS];
		break;
	case PTS_S_WAIT_RESP_PTS0:
		byte = ch->pts.resp[_PTS0];
		break;
	case PTS_S_WAIT_RESP_PTS1:
		byte = ch->pts.resp[_PTS1];
		/* This must be TA1 */
		ch->fi = byte >> 4;
		ch->di = byte & 0xf;
		TRACE_DEBUG("found Fi=%u Di=%u\r\n", ch->fi, ch->di);
		break;
	case PTS_S_WAIT_RESP_PTS2:
		byte = ch->pts.resp[_PTS2];
		break;
	case PTS_S_WAIT_RESP_PTS3:
		byte = ch->pts.resp[_PTS3];
		break;
	case PTS_S_WAIT_RESP_PCK:
		byte = ch->pts.resp[_PCK];
		/* update baud rate generator with Fi/Di */
		emu_update_fidi(ch);
		break;
	default:
		TRACE_DEBUG("get_byte_pts() in invalid state %u\r\n",
			ch->pts.state);
		return 0;
	}

	/* 2: Transmit the byte */
	card_emu_uart_tx(ch->uart_chan, byte);

	/* 3: Update the state */

	switch (ch->pts.state) {
	case PTS_S_WAIT_RESP_PCK:
		/* Wait for the next TPDU */
		card_set_state(ch, ISO_S_WAIT_TPDU);
		set_pts_state(ch, PTS_S_WAIT_REQ_PTSS);
		break;
	default:
		/* calculate the next state and set it */
		set_pts_state(ch, next_pts_state(ch));
		break;
	}

	/* return number of bytes transmitted */
	return 1;
}


/**********************************************************************
 * TPDU handling
 **********************************************************************/


/* compute number of data bytes according to Chapter 10.3.2 of 7816-3 */
static unsigned int t0_num_data_bytes(uint8_t p3, int reader_to_card)
{
	if (reader_to_card) {
		return p3;
	} else {
		if (p3 == 0)
			return 256;
		else
			return p3;
	}
}

/* add a just-received TPDU byte (from reader) to USB buffer */
static void add_tpdu_byte(struct card_handle *ch, uint8_t byte)
{
	struct req_ctx *rctx;
	struct cardemu_usb_msg_rx_data *rd;
	unsigned int num_data_bytes = t0_num_data_bytes(ch->tpdu.hdr[_P3], 0);

	/* ensure we have a buffer */
	if (!ch->uart_rx_ctx) {
		rctx = ch->uart_rx_ctx = req_ctx_find_get(0, RCTX_S_FREE, RCTX_S_UART_RX_BUSY);
		if (!ch->uart_rx_ctx) {
			TRACE_DEBUG("Received UART byte but unable to allocate Rx Buf\r\n");
			return;
		}
		rd = (struct cardemu_usb_msg_rx_data *) ch->uart_rx_ctx->data;
		cardemu_hdr_set(&rd->hdr, CEMU_USB_MSGT_DO_RX_DATA);
		rctx->tot_len = sizeof(*rd);
		rctx->idx = 0;
	} else
		rctx = ch->uart_rx_ctx;

	rd = (struct cardemu_usb_msg_rx_data *) rctx->data;

	rd->data[rctx->idx++] = byte;
	rctx->tot_len++;

	/* check if the buffer is full. If so, send it */
	if (rctx->tot_len >= sizeof(*rd) + num_data_bytes) {
		rd->flags |= CEMU_DATA_F_FINAL;
		flush_rx_buffer(ch);
		/* We need to transmit the SW now, */
		set_tpdu_state(ch, TPDU_S_WAIT_TX);
	} else if (rctx->tot_len >= rctx->size)
		flush_rx_buffer(ch);
}

static void set_tpdu_state(struct card_handle *ch, enum tpdu_state new_ts)
{
	if (ch->tpdu.state == new_ts)
		return;

	TRACE_DEBUG("7816 TPDU state %u -> %u\r\n", ch->tpdu.state, new_ts);

	switch (new_ts) {
	case TPDU_S_WAIT_CLA:
	case TPDU_S_WAIT_RX:
		card_emu_uart_enable(ch->uart_chan, ENABLE_RX);
		break;
	case TPDU_S_WAIT_PB:
		/* we just completed the TPDU header from reader to card
		 * and now need to disable the receiver, enable the
		 * transmitter and transmit the procedure byte */
		card_emu_uart_enable(ch->uart_chan, ENABLE_TX);
		break;
	}

	ch->tpdu.state = new_ts;
}

static enum tpdu_state next_tpdu_state(struct card_handle *ch)
{
	switch (ch->tpdu.state) {
	case TPDU_S_WAIT_CLA:
		return TPDU_S_WAIT_INS;
	case TPDU_S_WAIT_INS:
		return TPDU_S_WAIT_P1;
	case TPDU_S_WAIT_P1:
		return TPDU_S_WAIT_P2;
	case TPDU_S_WAIT_P2:
		return TPDU_S_WAIT_P3;
	case TPDU_S_WAIT_P3:
		return TPDU_S_WAIT_PB;
	/* simply stay in Rx or Tx by default */
	case TPDU_S_WAIT_PB:
		return TPDU_S_WAIT_PB;
	case TPDU_S_WAIT_RX:
		return TPDU_S_WAIT_RX;
	case TPDU_S_WAIT_TX:
		return TPDU_S_WAIT_TX;
	}
	/* we should never reach here */
	assert(0);
	return -1;
}

static void send_tpdu_header(struct card_handle *ch)
{
	struct req_ctx *rctx;
	struct cardemu_usb_msg_rx_data *rd;

	TRACE_DEBUG("%s: %02x %02x %02x %02x %02x\r\n", __func__,
			ch->tpdu.hdr[0], ch->tpdu.hdr[1],
			ch->tpdu.hdr[2], ch->tpdu.hdr[3],
			ch->tpdu.hdr[4]);

	/* if we already/still have a context, send it off */
	if (ch->uart_rx_ctx) {
		TRACE_DEBUG("have old buffer\r\n");
		if (ch->uart_rx_ctx->idx) {
			TRACE_DEBUG("flushing old buffer\r\n");
			flush_rx_buffer(ch);
		}
	} else {
		TRACE_DEBUG("allocating new buffer\r\n");
		/* ensure we have a new buffer */
		ch->uart_rx_ctx = req_ctx_find_get(0, RCTX_S_FREE, RCTX_S_UART_RX_BUSY);
		if (!ch->uart_rx_ctx) {
			TRACE_ERROR("%s: ENOMEM\r\n", __func__);
			return;
		}
	}
	rctx = ch->uart_rx_ctx;
	rd = (struct cardemu_usb_msg_rx_data *) rctx->data;

	/* initializ header */
	cardemu_hdr_set(&rd->hdr, CEMU_USB_MSGT_DO_RX_DATA);
	rd->flags = CEMU_DATA_F_TPDU_HDR;
	rctx->tot_len = sizeof(*rd) + sizeof(ch->tpdu.hdr);
	rctx->idx = sizeof(ch->tpdu.hdr);

	/* copy TPDU header to data field */
	memcpy(rd->data, ch->tpdu.hdr, sizeof(ch->tpdu.hdr));
	/* rd->data_len is set in flush_rx_buffer() */

	flush_rx_buffer(ch);
}

static enum iso7816_3_card_state
process_byte_tpdu(struct card_handle *ch, uint8_t byte)
{
	switch (ch->tpdu.state) {
	case TPDU_S_WAIT_CLA:
		ch->tpdu.hdr[_CLA] = byte;
		set_tpdu_state(ch, next_tpdu_state(ch));
		break;
	case TPDU_S_WAIT_INS:
		ch->tpdu.hdr[_INS] = byte;
		set_tpdu_state(ch, next_tpdu_state(ch));
		break;
	case TPDU_S_WAIT_P1:
		ch->tpdu.hdr[_P1] = byte;
		set_tpdu_state(ch, next_tpdu_state(ch));
		break;
	case TPDU_S_WAIT_P2:
		ch->tpdu.hdr[_P2] = byte;
		set_tpdu_state(ch, next_tpdu_state(ch));
		break;
	case TPDU_S_WAIT_P3:
		ch->tpdu.hdr[_P3] = byte;
		set_tpdu_state(ch, next_tpdu_state(ch));
		/* FIXME: start timer to transmit further 0x60 */
		/* send the TPDU header as part of a procedure byte
		 * request to the USB host */
		send_tpdu_header(ch);
		break;
	case TPDU_S_WAIT_RX:
		add_tpdu_byte(ch, byte);
		break;
	default:
		TRACE_DEBUG("process_byte_tpdu() in invalid state %u\r\n",
			    ch->tpdu.state);
	}

	/* ensure we stay in TPDU ISO state */
	return ISO_S_IN_TPDU;
}

/* tx a single byte to be transmitted to the reader */
static int tx_byte_tpdu(struct card_handle *ch)
{
	struct req_ctx *rctx;
	struct cardemu_usb_msg_tx_data *td;
	uint8_t byte;

	/* ensure we are aware of any data that might be pending for
	 * transmit */
	if (!ch->uart_tx_ctx) {
		if (llist_empty(&ch->uart_tx_queue))
			return 0;

		/* dequeue first at head */
		ch->uart_tx_ctx = llist_entry(ch->uart_tx_queue.next,
					      struct req_ctx, list);
		llist_del(&ch->uart_tx_ctx->list);
		req_ctx_set_state(ch->uart_tx_ctx, RCTX_S_UART_TX_BUSY);

		/* start with index zero */
		ch->uart_tx_ctx->idx = 0;

	}
	rctx = ch->uart_tx_ctx;
	td = (struct cardemu_usb_msg_tx_data *) rctx->data;

	/* take the next pending byte out of the rctx */
	byte = td->data[rctx->idx++];

	card_emu_uart_tx(ch->uart_chan, byte);

	/* this must happen _after_ the byte has been transmittd */
	switch (ch->tpdu.state) {
	case TPDU_S_WAIT_PB:
		/* if we just transmitted the procedure byte, we need to decide
		 * if we want to continue to receive or transmit */
		if (td->flags & CEMU_DATA_F_PB_AND_TX)
			set_tpdu_state(ch, TPDU_S_WAIT_TX);
		else if (td->flags & CEMU_DATA_F_PB_AND_RX)
			set_tpdu_state(ch, TPDU_S_WAIT_RX);
		break;
	}

	/* check if the buffer has now been fully transmitted */
	if ((rctx->idx >= td->hdr.data_len) ||
	    (td->data + rctx->idx >= rctx->data + rctx->tot_len)) {
		if (td->flags & CEMU_DATA_F_PB_AND_RX) {
			/* we have just sent the procedure byte and now
			 * need to continue receiving */
			set_tpdu_state(ch, TPDU_S_WAIT_RX);
		} else {
			/* we have transmitted all bytes */
			if (td->flags & CEMU_DATA_F_FINAL) {
				/* this was the final part of the APDU, go
				 * back to state one*/
				card_set_state(ch, ISO_S_WAIT_TPDU);
			} else {
				/* FIXME: call into USB code to chec if we need
				 * to submit a free buffer to accept
				 * further data on bulk out endpoint */
			}
		}
		req_ctx_set_state(rctx, RCTX_S_FREE);
		ch->uart_tx_ctx = NULL;
	}

	return 1;
}

/**********************************************************************
 * Public API
 **********************************************************************/

/* process a single byte received from the reader */
void card_emu_process_rx_byte(struct card_handle *ch, uint8_t byte)
{
	int new_state = -1;

	ch->stats.rx_bytes++;

	switch (ch->state) {
	case ISO_S_WAIT_POWER:
	case ISO_S_WAIT_CLK:
	case ISO_S_WAIT_RST:
	case ISO_S_WAIT_ATR:
		TRACE_DEBUG("Received UART char in 7816 state %u\r\n",
				ch->state);
		/* we shouldn't receive any data from the reader yet! */
		break;
	case ISO_S_WAIT_TPDU:
		if (byte == 0xff) {
			new_state = process_byte_pts(ch, byte);
			ch->stats.pps++;
			goto out_silent;
		}
		/* fall-through */
	case ISO_S_IN_TPDU:
		new_state = process_byte_tpdu(ch, byte);
		break;
	case ISO_S_IN_PTS:
		new_state = process_byte_pts(ch, byte);
		goto out_silent;
	}

out_silent:
	if (new_state != -1)
		card_set_state(ch, new_state);
}

/* transmit a single byte to the reader */
int card_emu_tx_byte(struct card_handle *ch)
{
	int rc = 0;

	switch (ch->state) {
	case ISO_S_IN_ATR:
		if (ch->atr.idx < ch->atr.len) {
			uint8_t byte;
			byte = ch->atr.atr[ch->atr.idx++];
			rc = 1;

			card_emu_uart_tx(ch->uart_chan, byte);

			/* detect end of ATR */
			if (ch->atr.idx >= ch->atr.len)
				card_set_state(ch, ISO_S_WAIT_TPDU);
		}
		break;
	case ISO_S_IN_PTS:
		rc = tx_byte_pts(ch);
		break;
	case ISO_S_IN_TPDU:
		rc = tx_byte_tpdu(ch);
		break;
	}

	if (rc)
		ch->stats.tx_bytes++;

	/* if we return 0 here, the UART needs to disable transmit-ready
	 * interrupts */
	return rc;
}

void card_emu_have_new_uart_tx(struct card_handle *ch)
{
	switch (ch->state) {
	case ISO_S_IN_TPDU:
		switch (ch->tpdu.state) {
		case TPDU_S_WAIT_TX:
		case TPDU_S_WAIT_PB:
			card_emu_uart_enable(ch->uart_chan, ENABLE_TX);
			break;
		default:
			break;
		}
	default:
		break;
	}
}

/* hardware driver informs us that a card I/O signal has changed */
void card_emu_io_statechg(struct card_handle *ch, enum card_io io, int active)
{
	switch (io) {
	case CARD_IO_VCC:
		if (active == 0 && ch->vcc_active == 1) {
			TRACE_DEBUG("VCC deactivated\r\n");
			tc_etu_disable(ch->tc_chan);
			card_set_state(ch, ISO_S_WAIT_POWER);
		} else if (active == 1 && ch->vcc_active == 0) {
			TRACE_DEBUG("VCC activated\r\n");
			card_set_state(ch, ISO_S_WAIT_CLK);
		}
		ch->vcc_active = active;
		break;
	case CARD_IO_CLK:
		if (active == 1 && ch->clocked == 0) {
			TRACE_DEBUG("CLK activated\r\n");
			if (ch->state == ISO_S_WAIT_CLK)
				card_set_state(ch, ISO_S_WAIT_RST);
		} else if (active == 0 && ch->clocked == 1) {
			TRACE_DEBUG("CLK deactivated\r\n");
		}
		ch->clocked = active;
		break;
	case CARD_IO_RST:
		if (active == 0 && ch->in_reset) {
			TRACE_DEBUG("RST released\r\n");
			if (ch->vcc_active && ch->clocked) {
				/* enable the TC/ETU counter once reset has been released */
				tc_etu_enable(ch->tc_chan);
				card_set_state(ch, ISO_S_WAIT_ATR);
				/* FIXME: wait 400 to 40k clock cycles before sending ATR */
				card_set_state(ch, ISO_S_IN_ATR);
			}
		} else if (active && !ch->in_reset) {
			TRACE_DEBUG("RST asserted\r\n");
			tc_etu_disable(ch->tc_chan);
		}
		ch->in_reset = active;
		break;
	}
}

/* User sets a new ATR to be returned during next card reset */
int card_emu_set_atr(struct card_handle *ch, const uint8_t *atr, uint8_t len)
{
	if (len > sizeof(ch->atr.atr))
		return -1;

	memcpy(ch->atr.atr, atr, len);
	ch->atr.len = len;
	ch->atr.idx = 0;

	/* FIXME: race condition with trasmitting ATR to reader? */

	return 0;
}

/* hardware driver informs us that one (more) ETU has expired */
void tc_etu_wtime_half_expired(void *handle)
{
	struct card_handle *ch = handle;
	/* transmit NULL procedure byte well before waiting time expires */
	switch (ch->state) {
	case ISO_S_IN_TPDU:
		switch (ch->tpdu.state) {
		case TPDU_S_WAIT_PB:
		case TPDU_S_WAIT_TX:
			putchar('N');
			card_emu_uart_tx(ch->uart_chan, ISO7816_3_PB_NULL);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

/* hardware driver informs us that one (more) ETU has expired */
void tc_etu_wtime_expired(void *handle)
{
}

/* shortest ATR found in smartcard_list.txt */
static const uint8_t default_atr[] = { 0x3B, 0x02, 0x14, 0x50 };

static struct card_handle card_handles[NUM_SLOTS];

struct card_handle *card_emu_init(uint8_t slot_num, uint8_t tc_chan, uint8_t uart_chan)
{
	struct card_handle *ch;

	if (slot_num >= ARRAY_SIZE(card_handles))
		return NULL;

	ch = &card_handles[slot_num];

	memset(ch, 0, sizeof(*ch));

	INIT_LLIST_HEAD(&ch->usb_tx_queue);
	INIT_LLIST_HEAD(&ch->uart_tx_queue);

	/* initialize the card_handle with reasonabe defaults */
	ch->state = ISO_S_WAIT_POWER;
	ch->vcc_active = 0;
	ch->in_reset = 1;
	ch->clocked = 0;

	ch->fi = 0;
	ch->di = 1;
	ch->wi = ISO7816_3_DEFAULT_WI;

	ch->tc_chan = tc_chan;
	ch->uart_chan = uart_chan;
	ch->waiting_time = ISO7816_3_INIT_WTIME;

	ch->atr.idx = 0;
	ch->atr.len = sizeof(default_atr);
	memcpy(ch->atr.atr, default_atr, ch->atr.len);

	ch->pts.state = PTS_S_WAIT_REQ_PTSS;
	ch->tpdu.state = TPDU_S_WAIT_CLA;

	tc_etu_init(ch->tc_chan, ch);

	return ch;
}
