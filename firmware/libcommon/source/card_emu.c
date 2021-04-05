/* ISO7816-3 state machine for the card side
 *
 * (C) 2010-2021 by Harald Welte <laforge@gnumonks.org>
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA
 */
#include <stdio.h>
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
#include "simtrace_prot.h"
#include "usb_buf.h"
#include <osmocom/core/linuxlist.h>
#include <osmocom/core/msgb.h>


#define NUM_SLOTS		2

/* bit-mask of supported CEMU_FEAT_F_ flags */
#define SUPPORTED_FEATURES	(CEMU_FEAT_F_STATUS_IRQ)

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

const struct value_string iso7816_3_card_state_names[] = {
	{ ISO_S_WAIT_POWER,	"WAIT_POWER" },
	{ ISO_S_WAIT_CLK,	"WAIT_CLK" },
	{ ISO_S_WAIT_RST,	"WAIT_RST" },
	{ ISO_S_WAIT_ATR,	"WAIT_ATR" },
	{ ISO_S_IN_ATR,		"IN_ATR" },
	{ ISO_S_IN_PTS,		"IN_PTS" },
	{ ISO_S_WAIT_TPDU,	"WAIT_TPDU" },
	{ ISO_S_IN_TPDU,	"IN_TPDU" },
	{ 0, NULL }
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

const struct value_string pts_state_names[] = {
	{ PTS_S_WAIT_REQ_PTSS,	"WAIT_REQ_PTSS" },
	{ PTS_S_WAIT_REQ_PTS0,	"WAIT_REQ_PTS0" },
	{ PTS_S_WAIT_REQ_PTS1,	"WAIT_REQ_PTS1" },
	{ PTS_S_WAIT_REQ_PTS2,	"WAIT_REQ_PTS2" },
	{ PTS_S_WAIT_REQ_PTS3,	"WAIT_REQ_PTS3" },
	{ PTS_S_WAIT_REQ_PCK,	"WAIT_REQ_PCK" },
	{ PTS_S_WAIT_RESP_PTSS,	"WAIT_RESP_PTSS" },
	{ PTS_S_WAIT_RESP_PTS0,	"WAIT_RESP_PTS0" },
	{ PTS_S_WAIT_RESP_PTS1,	"WAIT_RESP_PTS1" },
	{ PTS_S_WAIT_RESP_PTS2,	"WAIT_RESP_PTS2" },
	{ PTS_S_WAIT_RESP_PTS3,	"WAIT_RESP_PTS3" },
	{ PTS_S_WAIT_RESP_PCK,	"WAIT_RESP_PCK" },
	{ 0, NULL }
};

/* PTS field byte index */
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
	TPDU_S_WAIT_RX,		/* waiting for more data from reader */
	TPDU_S_WAIT_TX,		/* waiting for more data to reader */
};

const struct value_string tpdu_state_names[] = {
	{ TPDU_S_WAIT_CLA,	"WAIT_CLA" },
	{ TPDU_S_WAIT_INS,	"WAIT_INS" },
	{ TPDU_S_WAIT_P1,	"WAIT_P1" },
	{ TPDU_S_WAIT_P2,	"WAIT_P2" },
	{ TPDU_S_WAIT_P3,	"WAIT_P3" },
	{ TPDU_S_WAIT_PB,	"WAIT_PB" },
	{ TPDU_S_WAIT_RX, 	"WAIT_RX" },
	{ TPDU_S_WAIT_TX,	"WAIT_TX" },
	{ 0, NULL }
};

/* TPDU field byte index */
#define	_CLA	0
#define	_INS	1
#define	_P1	2
#define	_P2	3
#define	_P3	4

struct card_handle {
	unsigned int num;

	/* bit-mask of enabled optional features (CEMU_FEAT_F_*) */
	uint32_t features;

	enum iso7816_3_card_state state;

	/* signal levels */
	bool vcc_active;	/*< if VCC is active (true = active/ON) */
	bool in_reset;	/*< if card is in reset (true = RST low/asserted, false = RST high/ released) */
	bool clocked;	/*< if clock is active ( true = active, false = inactive) */

	/* All below variables with _index suffix are indexes from 0..15 into Tables 7 + 8
	 * of ISO7816-3. */

	/*! Index to clock rate conversion integer Fi (ISO7816-3 Table 7).
	 *  \note this represents the maximum value supported by the card, and can be indicated in TA1 */
	uint8_t Fi_index;
	/*! Current value of index to clock rate conversion integer F (ISO 7816-3 Section 7.1). */
	uint8_t F_index;

	/*! Index to baud rate adjustment factor Di (ISO7816-3 Table 8).
	 *  \note this represents the maximum value supported by the card, and can be indicated in TA1 */
	uint8_t Di_index;
	/*! Current value of index to baud rate adjustment factor D (ISO 7816-3 Section 7.1). */
	uint8_t D_index;

	/*! Waiting Integer (ISO7816-3 Section 10.2).
	 *  \note this value can be set in TA2 */
	uint8_t wi;

	/*! Waiting Time, in ETU (ISO7816-3 Section 8.1).
	 *  \note this depends on Fi, Di, and WI if T=0 is used */
	uint32_t waiting_time;	/* in etu */

	uint8_t tc_chan;	/* TC channel number */
	uint8_t uart_chan;	/* UART channel */

	uint8_t in_ep;		/* USB IN EP */
	uint8_t irq_ep;		/* USB IN EP */

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

	struct msgb *uart_rx_msg;	/* UART RX -> USB TX */
	struct msgb *uart_tx_msg;	/* USB RX -> UART TX */

	struct llist_head uart_tx_queue;

	struct {
		uint32_t tx_bytes;
		uint32_t rx_bytes;
		uint32_t pps;
	} stats;
};

/* reset all the 'dynamic' state of the card handle to the initial/default values */
static void card_handle_reset(struct card_handle *ch)
{
	struct msgb *msg;

	tc_etu_disable(ch->tc_chan);

	/* release any buffers we may still own */
	if (ch->uart_tx_msg) {
		usb_buf_free(ch->uart_tx_msg);
		ch->uart_tx_msg = NULL;
	}
	if (ch->uart_rx_msg) {
		usb_buf_free(ch->uart_rx_msg);
		ch->uart_rx_msg = NULL;
	}
	while ((msg = msgb_dequeue(&ch->uart_tx_queue))) {
		usb_buf_free(msg);
	}
}

struct llist_head *card_emu_get_uart_tx_queue(struct card_handle *ch)
{
	return &ch->uart_tx_queue;
}

static void set_tpdu_state(struct card_handle *ch, enum tpdu_state new_ts);
static void set_pts_state(struct card_handle *ch, enum pts_state new_ptss);

/* update simtrace header msg_len and submit USB buffer */
void usb_buf_upd_len_and_submit(struct msgb *msg)
{
	struct simtrace_msg_hdr *sh = (struct simtrace_msg_hdr *) msg->l1h;

	sh->msg_len = msgb_length(msg);

	usb_buf_submit(msg);
}

/* Allocate USB buffer and push + initialize simtrace_msg_hdr */
struct msgb *usb_buf_alloc_st(uint8_t ep, uint8_t msg_class, uint8_t msg_type)
{
	struct msgb *msg = NULL;
	struct simtrace_msg_hdr *sh;

	while (!msg) {
		msg = usb_buf_alloc(ep); // try to allocate some memory
		if (!msg) { // allocation failed, we might be out of memory
			struct usb_buffered_ep *bep = usb_get_buf_ep(ep);
			if (!bep) {
				TRACE_ERROR("ep %u: %s queue does not exist\n\r",
				            ep, __func__);
				return NULL;
			}
			if (llist_empty(&bep->queue)) {
				TRACE_ERROR("ep %u: %s EOMEM (queue already empty)\n\r",
				            ep, __func__);
				return NULL;
			}
			msg = msgb_dequeue_count(&bep->queue, &bep->queue_len);
			if (!msg) {
				TRACE_ERROR("ep %u: %s no msg in non-empty queue\n\r",
				            ep, __func__);
				return NULL;
			}
			usb_buf_free(msg);
			msg = NULL;
			TRACE_DEBUG("ep %u: %s queue msg dropped\n\r",
			            ep, __func__);
		}
	}

	msg->l1h = msgb_put(msg, sizeof(*sh));
	sh = (struct simtrace_msg_hdr *) msg->l1h;
	memset(sh, 0, sizeof(*sh));
	sh->msg_class = msg_class;
	sh->msg_type = msg_type;
	msg->l2h = msg->l1h + sizeof(*sh);

	return msg;
}

/* Update cardemu_usb_msg_rx_data length + submit buffer */
static void flush_rx_buffer(struct card_handle *ch)
{
	struct msgb *msg;
	struct cardemu_usb_msg_rx_data *rd;
	uint32_t data_len;

	msg = ch->uart_rx_msg;
	if (!msg)
		return;

	ch->uart_rx_msg = NULL;

	/* store length of data payload field in header */
	rd = (struct cardemu_usb_msg_rx_data *) msg->l2h;
	rd->data_len = msgb_l2len(msg) - sizeof(*rd);

	TRACE_INFO("%u: %s (%u)\n\r",
			ch->num, __func__, rd->data_len);

	usb_buf_upd_len_and_submit(msg);
}

/* convert a non-contiguous PTS request/response into a contiguous
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
	struct msgb *msg;
	struct cardemu_usb_msg_pts_info *ptsi;

	msg = usb_buf_alloc_st(ch->in_ep, SIMTRACE_MSGC_CARDEM, SIMTRACE_MSGT_DO_CEMU_PTS);
	if (!msg)
		return;

	ptsi = (struct cardemu_usb_msg_pts_info *) msgb_put(msg, sizeof(*ptsi));
	ptsi->pts_len = serialize_pts(ptsi->req, ch->pts.req);
	serialize_pts(ptsi->resp, ch->pts.resp);

	usb_buf_upd_len_and_submit(msg);
}

static void emu_update_fidi(struct card_handle *ch)
{
	int rc;

	rc = iso7816_3_compute_fd_ratio(ch->F_index, ch->D_index);
	if (rc > 0 && rc < 0x400) {
		TRACE_INFO("%u: computed F(%u)/D(%u) ratio: %d\r\n", ch->num,
			   ch->F_index, ch->D_index, rc);
		/* make sure UART uses new F/D ratio */
		card_emu_uart_update_fidi(ch->uart_chan, rc);
		/* notify ETU timer about this */
		tc_etu_set_etu(ch->tc_chan, rc);
	} else
		TRACE_INFO("%u: computed F/D ratio %d unsupported\r\n",
			   ch->num, rc);
}

/* Update the ISO 7816-3 TPDU receiver state */
static void card_set_state(struct card_handle *ch,
			   enum iso7816_3_card_state new_state)
{
	if (ch->state == new_state)
		return;

	TRACE_DEBUG("%u: 7816 card state %s -> %s\r\n", ch->num,
		    get_value_string(iso7816_3_card_state_names, ch->state),
		    get_value_string(iso7816_3_card_state_names, new_state));
	ch->state = new_state;

	switch (new_state) {
	case ISO_S_WAIT_POWER:
	case ISO_S_WAIT_CLK:
	case ISO_S_WAIT_RST:
		/* disable Rx and Tx of UART */
		card_emu_uart_enable(ch->uart_chan, 0);
		break;
	case ISO_S_WAIT_ATR:
		/* Reset to initial Fi / Di ratio */
		ch->Fi_index = ch->F_index = 1;
		ch->Di_index = ch->D_index = 1;
		ch->wi = ISO7816_3_DEFAULT_WI;
		ch->waiting_time = ISO7816_3_INIT_WTIME;
		emu_update_fidi(ch);
		/* the ATR should only be sent 400 to 40k clock cycles after the RESET.
		 * we use the tc_etu mechanism to wait this time.
		 * since the initial ETU is Fd=372/Dd=1 clock cycles long, we have to wait 2-107 ETU.
		 */
		tc_etu_set_wtime(ch->tc_chan, 2);
		/* enable the TC/ETU counter once reset has been released */
		tc_etu_enable(ch->tc_chan);
		break;
	case ISO_S_IN_ATR:
		/* initialize to default WI, this will be overwritten if we
		 * send TC2, and it will be programmed into hardware after
		 * ATR is finished */
		ch->wi = ISO7816_3_DEFAULT_WI;
		/* update waiting time to initial waiting time */
		ch->waiting_time = ISO7816_3_INIT_WTIME;
		/* set initial waiting time */
		tc_etu_set_wtime(ch->tc_chan, ch->waiting_time);
		/* Set ATR sub-state to initial state */
		ch->atr.idx = 0;
		/* enable USART transmission to reader */
		card_emu_uart_enable(ch->uart_chan, ENABLE_TX);
		/* trigger USART TX IRQ to sent first ATR byte TS */
		card_emu_uart_interrupt(ch->uart_chan);
		break;
	case ISO_S_WAIT_TPDU:
		/* enable the receiver, disable transmitter */
		set_tpdu_state(ch, TPDU_S_WAIT_CLA);
		card_emu_uart_enable(ch->uart_chan, ENABLE_RX);
		break;
	case ISO_S_IN_PTS:
	case ISO_S_IN_TPDU:
		/* do nothing */
		break;
	}
}

/**********************************************************************
 * ATR handling
 **********************************************************************/

/*! Transmit ATR data to reader
 *  @param[in] ch card interface connected to reader
 *  @return numbers of bytes transmitted
 */
static int tx_byte_atr(struct card_handle *ch)
{
	if (NULL == ch) {
		TRACE_ERROR("ATR TX: no card handle provided\n\r");
		return 0;
	}
	if (ISO_S_IN_ATR != ch->state) {
		TRACE_ERROR("%u: ATR TX: no in ATR state\n\r", ch->num);
		return 0;
	}

	/* Transmit ATR */
	if (ch->atr.idx < ch->atr.len) {
		uint8_t byte = ch->atr.atr[ch->atr.idx++];
		card_emu_uart_tx(ch->uart_chan, byte);
		return 1;
	} else { /* The ATR has been completely transmitted */
		/* search for TC2 to updated WI */
		ch->wi = ISO7816_3_DEFAULT_WI;
		if (ch->atr.len >= 2 && ch->atr.atr[1] & 0xf0) { /* Y1 has some data */
			uint8_t atr_td1 = 2;
			if (ch->atr.atr[1] & 0x10) { /* TA1 is present */
				atr_td1++;
			}
			if (ch->atr.atr[1] & 0x20) { /* TB1 is present */
				atr_td1++;
			}
			if (ch->atr.atr[1] & 0x40) { /* TC1 is present */
				atr_td1++;
			}
			if (ch->atr.atr[1] & 0x80) { /* TD1 is present */
				if (ch->atr.len > atr_td1 && ch->atr.atr[atr_td1] & 0xf0) { /* Y2 has some data */
					uint8_t atr_tc2 = atr_td1+1;
					if (ch->atr.atr[atr_td1] & 0x10) { /* TA2 is present */
						atr_tc2++;
					}
					if (ch->atr.atr[atr_td1] & 0x20) { /* TB2 is present */
						atr_tc2++;
					}
					if (ch->atr.atr[atr_td1] & 0x40) { /* TC2 is present */
						if (ch->atr.len > atr_tc2 && ch->atr.atr[atr_tc2]) { /* TC2 encodes WI */
							ch->wi = ch->atr.atr[atr_tc2]; /* set WI */
						}
					}
				}
			}
		}
		/* update waiting time (see ISO 7816-3 10.2) */
		ch->waiting_time = ch->wi * 960 * iso7816_3_fi_table[ch->F_index];
		tc_etu_set_wtime(ch->tc_chan, ch->waiting_time);
		/* go to next state */
		card_set_state(ch, ISO_S_WAIT_TPDU);
		return 0;
	}

	/* return number of bytes transmitted */
	return 1;
}

/**********************************************************************
 * PTS / PPS handling
 **********************************************************************/

/* Update the PTS sub-state */
static void set_pts_state(struct card_handle *ch, enum pts_state new_ptss)
{
	TRACE_DEBUG("%u: 7816 PTS state %s -> %s\r\n", ch->num,
		    get_value_string(pts_state_names, ch->pts.state),
		    get_value_string(pts_state_names, new_ptss));
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


static int
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
			TRACE_ERROR("%u: Error in PTS Checksum!\r\n",
				    ch->num);
			/* Wait for the next TPDU */
			set_pts_state(ch, PTS_S_WAIT_REQ_PTSS);
			return ISO_S_WAIT_TPDU;
		}
		/* FIXME: check if proposal matches capabilities in ATR */
		memcpy(ch->pts.resp, ch->pts.req, sizeof(ch->pts.resp));
		break;
	default:
		TRACE_ERROR("%u: process_byte_pts() in invalid PTS state %s\r\n", ch->num,
			    get_value_string(pts_state_names, ch->pts.state));
		break;
	}
	/* calculate the next state and set it */
	set_pts_state(ch, next_pts_state(ch));

	if (ch->pts.state == PTS_S_WAIT_RESP_PTSS) {
		flush_pts(ch);
		/* activate UART TX to transmit PTS response */
		card_emu_uart_enable(ch->uart_chan, ENABLE_TX);
		/* don't fall-through to the 'return ISO_S_IN_PTS'
		 * below, rather keep ISO7816 state as-is, it will be
		 * further updated by the tx-completion handler */
		return -1;
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
		ch->F_index = byte >> 4;
		ch->D_index = byte & 0xf;
		TRACE_DEBUG("%u: found F=%u D=%u\r\n", ch->num,
			    iso7816_3_fi_table[ch->F_index], iso7816_3_di_table[ch->D_index]);
		/* FIXME: if F or D are 0, become unresponsive to signal error condition */
		break;
	case PTS_S_WAIT_RESP_PTS2:
		byte = ch->pts.resp[_PTS2];
		break;
	case PTS_S_WAIT_RESP_PTS3:
		byte = ch->pts.resp[_PTS3];
		break;
	case PTS_S_WAIT_RESP_PCK:
		byte = ch->pts.resp[_PCK];
		break;
	default:
		TRACE_ERROR("%u: get_byte_pts() in invalid PTS state %s\r\n", ch->num,
			    get_value_string(pts_state_names, ch->pts.state));
		return 0;
	}

	/* 2: Transmit the byte */
	card_emu_uart_tx(ch->uart_chan, byte);

	/* 3: Update the state */

	switch (ch->pts.state) {
	case PTS_S_WAIT_RESP_PCK:
		card_emu_uart_wait_tx_idle(ch->uart_chan);
		/* update baud rate generator with F/D */
		emu_update_fidi(ch);
		/* Wait for the next TPDU */
		card_set_state(ch, ISO_S_WAIT_TPDU);
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
	struct msgb *msg;
	struct cardemu_usb_msg_rx_data *rd;
	unsigned int num_data_bytes = t0_num_data_bytes(ch->tpdu.hdr[_P3], 0);

	/* ensure we have a buffer */
	if (!ch->uart_rx_msg) {
		msg = ch->uart_rx_msg = usb_buf_alloc_st(ch->in_ep, SIMTRACE_MSGC_CARDEM,
							 SIMTRACE_MSGT_DO_CEMU_RX_DATA);
		if (!ch->uart_rx_msg) {
			TRACE_ERROR("%u: Received UART byte but ENOMEM\r\n",
				    ch->num);
			return;
		}
		msgb_put(msg, sizeof(*rd));
	} else
		msg = ch->uart_rx_msg;

	rd = (struct cardemu_usb_msg_rx_data *) msg->l2h;
	msgb_put_u8(msg, byte);

	/* check if the buffer is full. If so, send it */
	if (msgb_l2len(msg) >= sizeof(*rd) + num_data_bytes) {
		rd->flags |= CEMU_DATA_F_FINAL;
		flush_rx_buffer(ch);
		/* We need to transmit the SW now, */
		set_tpdu_state(ch, TPDU_S_WAIT_TX);
	} else if (msgb_tailroom(msg) <= 0)
		flush_rx_buffer(ch);
}

static void set_tpdu_state(struct card_handle *ch, enum tpdu_state new_ts)
{
	if (ch->tpdu.state == new_ts)
		return;

	TRACE_DEBUG("%u: 7816 TPDU state %s -> %s\r\n", ch->num,
		get_value_string(tpdu_state_names, ch->tpdu.state),
		get_value_string(tpdu_state_names, new_ts));
	ch->tpdu.state = new_ts;

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
	default:
		break;
	}
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
	struct msgb *msg;
	struct cardemu_usb_msg_rx_data *rd;
	uint8_t *cur;

	TRACE_INFO("%u: %s: %02x %02x %02x %02x %02x\r\n",
			ch->num, __func__,
			ch->tpdu.hdr[0], ch->tpdu.hdr[1],
			ch->tpdu.hdr[2], ch->tpdu.hdr[3],
			ch->tpdu.hdr[4]);

	/* if we already/still have a context, send it off */
	if (ch->uart_rx_msg) {
		TRACE_DEBUG("%u: have old buffer\r\n", ch->num);
		if (msgb_l2len(ch->uart_rx_msg)) {
			TRACE_DEBUG("%u: flushing old buffer\r\n", ch->num);
			flush_rx_buffer(ch);
		}
	}
	TRACE_DEBUG("%u: allocating new buffer\r\n", ch->num);
	/* ensure we have a new buffer */
	ch->uart_rx_msg = usb_buf_alloc_st(ch->in_ep, SIMTRACE_MSGC_CARDEM,
					   SIMTRACE_MSGT_DO_CEMU_RX_DATA);
	if (!ch->uart_rx_msg) {
		TRACE_ERROR("%u: %s: ENOMEM\r\n", ch->num, __func__);
		return;
	}
	msg = ch->uart_rx_msg;
	rd = (struct cardemu_usb_msg_rx_data *) msgb_put(msg, sizeof(*rd));

	/* initialize header */
	rd->flags = CEMU_DATA_F_TPDU_HDR;

	/* copy TPDU header to data field */
	cur = msgb_put(msg, sizeof(ch->tpdu.hdr));
	memcpy(cur, ch->tpdu.hdr, sizeof(ch->tpdu.hdr));
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
		TRACE_ERROR("%u: process_byte_tpdu() in invalid TPDU state %s\r\n", ch->num,
			    get_value_string(tpdu_state_names, ch->tpdu.state));
	}

	/* ensure we stay in TPDU ISO state */
	return ISO_S_IN_TPDU;
}

/* tx a single byte to be transmitted to the reader */
static int tx_byte_tpdu(struct card_handle *ch)
{
	struct msgb *msg;
	struct cardemu_usb_msg_tx_data *td;
	uint8_t byte;

	/* ensure we are aware of any data that might be pending for
	 * transmit */
	if (!ch->uart_tx_msg) {
		/* uart_tx_queue is filled from main loop, so no need
		 * for irq-safe operations */
		if (llist_empty(&ch->uart_tx_queue))
			return 0;

		/* dequeue first at head */
		ch->uart_tx_msg = msgb_dequeue(&ch->uart_tx_queue);
		ch->uart_tx_msg->l1h = ch->uart_tx_msg->head;
		ch->uart_tx_msg->l2h = ch->uart_tx_msg->l1h + sizeof(struct simtrace_msg_hdr);
		msg = ch->uart_tx_msg;
		/* remove the header */
		msgb_pull(msg, sizeof(struct simtrace_msg_hdr) + sizeof(*td));
	}
	msg = ch->uart_tx_msg;
	td = (struct cardemu_usb_msg_tx_data *) msg->l2h;

	/* take the next pending byte out of the msgb */
	byte = msgb_pull_u8(msg);

	card_emu_uart_tx(ch->uart_chan, byte);

	/* this must happen _after_ the byte has been transmitted */
	switch (ch->tpdu.state) {
	case TPDU_S_WAIT_PB:
		/* if we just transmitted the procedure byte, we need to decide
		 * if we want to continue to receive or transmit */
		if (td->flags & CEMU_DATA_F_PB_AND_TX)
			set_tpdu_state(ch, TPDU_S_WAIT_TX);
		else if (td->flags & CEMU_DATA_F_PB_AND_RX)
			set_tpdu_state(ch, TPDU_S_WAIT_RX);
		break;
	default:
		break;
	}

	/* check if the buffer has now been fully transmitted */
	if (msgb_length(msg) == 0) {
		if (td->flags & CEMU_DATA_F_PB_AND_RX) {
			/* we have just sent the procedure byte and now
			 * need to continue receiving */
			set_tpdu_state(ch, TPDU_S_WAIT_RX);
		} else {
			/* we have transmitted all bytes */
			if (td->flags & CEMU_DATA_F_FINAL) {
				/* this was the final part of the APDU, go
				 * back to state one */
				card_set_state(ch, ISO_S_WAIT_TPDU);
			}
		}
		usb_buf_free(msg);
		ch->uart_tx_msg = NULL;
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
	case ISO_S_WAIT_TPDU:
		if (byte == 0xff) {
			/* reset PTS to initial state */
			set_pts_state(ch, PTS_S_WAIT_REQ_PTSS);
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
	default:
		TRACE_ERROR("%u: Received UART char in invalid 7816 state %s\r\n", ch->num,
			    get_value_string(iso7816_3_card_state_names, ch->state));
		break;
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
		rc = tx_byte_atr(ch);
		break;
	case ISO_S_IN_PTS:
		rc = tx_byte_pts(ch);
		break;
	case ISO_S_IN_TPDU:
		rc = tx_byte_tpdu(ch);
		break;
	default:
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

void card_emu_report_status(struct card_handle *ch, bool report_on_irq)
{
	struct msgb *msg;
	struct cardemu_usb_msg_status *sts;
	uint8_t ep = ch->in_ep;

	if (report_on_irq)
		ep = ch->irq_ep;

	msg = usb_buf_alloc_st(ep, SIMTRACE_MSGC_CARDEM, SIMTRACE_MSGT_BD_CEMU_STATUS);
	if (!msg)
		return;

	sts = (struct cardemu_usb_msg_status *) msgb_put(msg, sizeof(*sts));
	sts->flags = 0;
	if (ch->vcc_active)
		sts->flags |= CEMU_STATUS_F_VCC_PRESENT;
	if (ch->clocked)
		sts->flags |= CEMU_STATUS_F_CLK_ACTIVE;
	if (ch->in_reset)
		sts->flags |= CEMU_STATUS_F_RESET_ACTIVE;
	/* FIXME: voltage + card insert */
	sts->F_index = ch->F_index;
	sts->D_index = ch->D_index;
	sts->wi = ch->wi;
	sts->waiting_time = ch->waiting_time;

	usb_buf_upd_len_and_submit(msg);
}

static void card_emu_report_config(struct card_handle *ch)
{
	struct msgb *msg;
	struct cardemu_usb_msg_config *cfg;
	uint8_t ep = ch->in_ep;

	msg = usb_buf_alloc_st(ch->in_ep, SIMTRACE_MSGC_CARDEM, SIMTRACE_MSGT_BD_CEMU_CONFIG);
	if (!msg)
		return;

	cfg = (struct cardemu_usb_msg_config *) msgb_put(msg, sizeof(*cfg));
	cfg->features = ch->features;

	usb_buf_upd_len_and_submit(msg);
}

/* hardware driver informs us that a card I/O signal has changed */
void card_emu_io_statechg(struct card_handle *ch, enum card_io io, int active)
{
	uint32_t chg_mask = 0;

	switch (io) {
	case CARD_IO_VCC:
		if (active == 0 && ch->vcc_active == 1) {
			TRACE_INFO("%u: VCC deactivated\r\n", ch->num);
			card_handle_reset(ch);
			card_set_state(ch, ISO_S_WAIT_POWER);
			chg_mask |= CEMU_STATUS_F_VCC_PRESENT;
		} else if (active == 1 && ch->vcc_active == 0) {
			TRACE_INFO("%u: VCC activated\r\n", ch->num);
			card_set_state(ch, ISO_S_WAIT_CLK);
			chg_mask |= CEMU_STATUS_F_VCC_PRESENT;
		}
		ch->vcc_active = active;
		break;
	case CARD_IO_CLK:
		if (active == 1 && ch->clocked == 0) {
			TRACE_INFO("%u: CLK activated\r\n", ch->num);
			if (ch->state == ISO_S_WAIT_CLK)
				card_set_state(ch, ISO_S_WAIT_RST);
			chg_mask |= CEMU_STATUS_F_CLK_ACTIVE;
		} else if (active == 0 && ch->clocked == 1) {
			TRACE_INFO("%u: CLK deactivated\r\n", ch->num);
			chg_mask |= CEMU_STATUS_F_CLK_ACTIVE;
		}
		ch->clocked = active;
		break;
	case CARD_IO_RST:
		if (active == 0 && ch->in_reset) {
			TRACE_INFO("%u: RST released\r\n", ch->num);
			if (ch->vcc_active && ch->clocked) {
				/* enable the TC/ETU counter once reset has been released */
				tc_etu_enable(ch->tc_chan);
				/* prepare to send the ATR */
				card_set_state(ch, ISO_S_WAIT_ATR);
			}
			chg_mask |= CEMU_STATUS_F_RESET_ACTIVE;
		} else if (active && !ch->in_reset) {
			TRACE_INFO("%u: RST asserted\r\n", ch->num);
			card_handle_reset(ch);
			chg_mask |= CEMU_STATUS_F_RESET_ACTIVE;
		}
		ch->in_reset = active;
		break;
	}

	switch (ch->state) {
	case ISO_S_WAIT_POWER:
	case ISO_S_WAIT_CLK:
	case ISO_S_WAIT_RST:
		/* check end activation state (even if the reader does
		 * not respect the activation sequence) */
		if (ch->vcc_active && ch->clocked && !ch->in_reset) {
			/* prepare to send the ATR */
			card_set_state(ch, ISO_S_WAIT_ATR);
		}
		break;
	default:
		break;
	}

	/* notify the host about the state change */
	if ((ch->features & CEMU_FEAT_F_STATUS_IRQ) && chg_mask)
		card_emu_report_status(ch, true);
}

/* User sets a new ATR to be returned during next card reset */
int card_emu_set_atr(struct card_handle *ch, const uint8_t *atr, uint8_t len)
{
	if (len > sizeof(ch->atr.atr))
		return -1;

	memcpy(ch->atr.atr, atr, len);
	ch->atr.len = len;
	ch->atr.idx = 0;

#if TRACE_LEVEL >= TRACE_LEVEL_INFO 
	uint8_t i;
	TRACE_INFO("%u: ATR set: ", ch->num);
	for (i = 0; i < ch->atr.len; i++) {
		TRACE_INFO_WP("%02x ", atr[i]);
	}
	TRACE_INFO_WP("\n\r");
#endif
	/* FIXME: race condition with transmitting ATR to reader? */

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
	struct card_handle *ch = handle;
	switch (ch->state) {
	case ISO_S_WAIT_ATR:
		/* ISO 7816-3 6.2.1 time tc has passed, we can now send the ATR */
		card_set_state(ch, ISO_S_IN_ATR);
		break;
	default:
		TRACE_ERROR("%u: wtime_exp\r\n", ch->num);
		break;
	}
}

/* reasonable ATR offering all protocols and voltages
 * smartphones might not care, but other readers do
 *
 * TS =		0x3B	Direct Convention
 * T0 =		0x80	Y(1): b1000, K: 0 (historical bytes)
 * TD(1) =	0x80	Y(i+1) = b1000, Protocol T=0
 * ----
 * TD(2) =	0x81	Y(i+1) = b1000, Protocol T=1
 * ----
 * TD(3) =	0x1F	Y(i+1) = b0001, Protocol T=15
 * ----
 * TA(4) =	0xC7	Clock stop: no preference - Class accepted by the card: (3G) A 5V B 3V C 1.8V
 * ----
 * Historical bytes
 * TCK =	0x59 	correct checksum
 */
static const uint8_t default_atr[] = { 0x3B, 0x80, 0x80, 0x81 , 0x1F, 0xC7, 0x59 };

static struct card_handle card_handles[NUM_SLOTS];

int card_emu_set_config(struct card_handle *ch, const struct cardemu_usb_msg_config *scfg,
			unsigned int scfg_len)
{
	if (scfg_len >= sizeof(uint32_t))
		ch->features = (scfg->features & SUPPORTED_FEATURES);

	/* send back a report of our current configuration */
	card_emu_report_config(ch);

	return 0;
}

struct card_handle *card_emu_init(uint8_t slot_num, uint8_t tc_chan, uint8_t uart_chan, uint8_t in_ep, uint8_t irq_ep, bool vcc_active, bool in_reset, bool clocked)
{
	struct card_handle *ch;

	if (slot_num >= ARRAY_SIZE(card_handles))
		return NULL;

	ch = &card_handles[slot_num];

	memset(ch, 0, sizeof(*ch));

	INIT_LLIST_HEAD(&ch->uart_tx_queue);

	ch->num = slot_num;
	ch->irq_ep = irq_ep;
	ch->in_ep = in_ep;
	ch->state = ISO_S_WAIT_POWER;
	ch->vcc_active = vcc_active;
	ch->in_reset = in_reset;
	ch->clocked = clocked;

	ch->Fi_index = ch->F_index = 1;
	ch->Di_index = ch->D_index = 1;
	ch->wi = ISO7816_3_DEFAULT_WI;

	ch->tc_chan = tc_chan;
	ch->uart_chan = uart_chan;
	ch->waiting_time = ISO7816_3_INIT_WTIME;

	ch->atr.idx = 0;
	ch->atr.len = sizeof(default_atr);
	memcpy(ch->atr.atr, default_atr, ch->atr.len);

	card_handle_reset(ch);

	tc_etu_init(ch->tc_chan, ch);

	return ch;
}
