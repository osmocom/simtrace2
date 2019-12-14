#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "card_emu.h"
#include "simtrace_prot.h"
#include "tc_etu.h"
#include "usb_buf.h"

#define PHONE_DATAIN	1
#define PHONE_INT	2
#define PHONE_DATAOUT	3

/***********************************************************************
 * stub functions required by card_emu.c
 ***********************************************************************/

void card_emu_uart_wait_tx_idle(uint8_t uart_chan)
{
}

int card_emu_uart_update_fidi(uint8_t uart_chan, unsigned int fidi)
{
	printf("uart_update_fidi(uart_chan=%u, fidi=%u)\n", uart_chan, fidi);
	return 0;
}

/* a buffer in which we store those bytes send by the UART towards the card
 * reader, so we can verify in test cases what was actually written  */
static uint8_t tx_debug_buf[1024];
static unsigned int tx_debug_buf_idx;

/* the card emulator wants to send some data to the host [reader] */
int card_emu_uart_tx(uint8_t uart_chan, uint8_t byte)
{
	printf("UART_TX(%02x)\n", byte);
	tx_debug_buf[tx_debug_buf_idx++] = byte;
	return 1;
}

void card_emu_uart_enable(uint8_t uart_chan, uint8_t rxtx)
{
	char *rts;
	switch (rxtx) {
	case 0:
		rts = "OFF";
		break;
	case ENABLE_TX:
		rts = "TX";
		break;
	case ENABLE_RX:
		rts = "RX";
		break;
	default:
		rts = "unknown";
		break;
	};

	printf("uart_enable(uart_chan=%u, %s)\n", uart_chan, rts);
}

void card_emu_uart_interrupt(uint8_t uart_chan)
{
	printf("uart_interrupt(uart_chan=%u)\n", uart_chan);
}

void tc_etu_set_wtime(uint8_t tc_chan, uint16_t wtime)
{
	printf("tc_etu_set_wtime(tc_chan=%u, wtime=%u)\n", tc_chan, wtime);
}

void tc_etu_set_etu(uint8_t tc_chan, uint16_t etu)
{
	printf("tc_etu_set_etu(tc_chan=%u, etu=%u)\n", tc_chan, etu);
}

void tc_etu_init(uint8_t chan_nr, void *handle)
{
	printf("tc_etu_init(tc_chan=%u)\n", chan_nr);
}

void tc_etu_enable(uint8_t chan_nr)
{
	printf("tc_etu_enable(tc_chan=%u)\n", chan_nr);
}

void tc_etu_disable(uint8_t chan_nr)
{
	printf("tc_etu_disable(tc_chan=%u)\n", chan_nr);
}



/***********************************************************************
 * test helper functions
 ***********************************************************************/


static void reader_check_and_clear(const uint8_t *data, unsigned int len)
{
	assert(len == tx_debug_buf_idx);
	assert(!memcmp(tx_debug_buf, data, len));
	tx_debug_buf_idx = 0;
}

static const uint8_t atr[] = { 0x3b, 0x02, 0x14, 0x50 };

static int verify_atr(struct card_handle *ch)
{
	unsigned int i;

	printf("receiving + verifying ATR:\n");
	for (i = 0; i < sizeof(atr); i++) {
		assert(card_emu_tx_byte(ch) == 1);
	}
	assert(card_emu_tx_byte(ch) == 0);
	reader_check_and_clear(atr, sizeof(atr));

	return 1;
}

static void io_start_card(struct card_handle *ch)
{
	card_emu_set_atr(ch, atr, sizeof(atr));

	/* bring the card up from the dead */
	card_emu_io_statechg(ch, CARD_IO_VCC, 1);
	assert(card_emu_tx_byte(ch) == 0);
	card_emu_io_statechg(ch, CARD_IO_CLK, 1);
	assert(card_emu_tx_byte(ch) == 0);
	card_emu_io_statechg(ch, CARD_IO_RST, 1);
	assert(card_emu_tx_byte(ch) == 0);

	/* release from reset and verify th ATR */
	card_emu_io_statechg(ch, CARD_IO_RST, 0);
	/* simulate waiting time before ATR expired */
	tc_etu_wtime_expired(ch);
	verify_atr(ch);
}

/* emulate the host/reader sending some bytes to the [emulated] card */
static void reader_send_bytes(struct card_handle *ch, const uint8_t *bytes, unsigned int len)
{
	unsigned int i;
	for (i = 0; i < len; i++) {
		printf("UART_RX(%02x)\n", bytes[i]);
		card_emu_process_rx_byte(ch, bytes[i]);
	}
}

static void dump_rctx(struct msgb *msg)
{
	struct simtrace_msg_hdr *mh = (struct simtrace_msg_hdr *) msg->l1h;
	struct cardemu_usb_msg_rx_data *rxd;
	int i;
#if 0

	printf("req_ctx(%p): state=%u, size=%u, tot_len=%u, idx=%u, data=%p\n",
		rctx, rctx->state, rctx->size, rctx->tot_len, rctx->idx, rctx->data);
	printf("  msg_type=%u, seq_nr=%u, msg_len=%u\n",
		mh->msg_type, mh->seq_nr, mh->msg_len);
#endif
	printf("%s\n", msgb_hexdump(msg));

	switch (mh->msg_type) {
	case SIMTRACE_MSGT_DO_CEMU_RX_DATA:
		rxd = (struct cardemu_usb_msg_rx_data *) msg->l2h;
		printf("    flags=%x, data=", rxd->flags);
		for (i = 0; i < rxd->data_len; i++)
			printf(" %02x", rxd->data[i]);
		printf("\n");
		break;
	}
}

static void get_and_verify_rctx(uint8_t ep, const uint8_t *data, unsigned int len)
{
	struct usb_buffered_ep *bep = usb_get_buf_ep(ep);
	struct msgb *msg;
	struct cardemu_usb_msg_tx_data *td;
	struct cardemu_usb_msg_rx_data *rd;
	struct simtrace_msg_hdr *mh;

	assert(bep);
	msg = msgb_dequeue_count(&bep->queue, &bep->queue_len);
	assert(msg);
	dump_rctx(msg);
	assert(msg->l1h);
	mh = (struct simtrace_msg_hdr *) msg->l1h;

	/* verify the contents of the rctx */
	switch (mh->msg_type) {
	case SIMTRACE_MSGT_DO_CEMU_RX_DATA:
		rd = (struct cardemu_usb_msg_rx_data *) msg->l2h;
		assert(rd->data_len == len);
		assert(!memcmp(rd->data, data, len));
		break;
#if 0
	case RCTX_S_UART_RX_PENDING:
		rd = (struct cardemu_usb_msg_rx_data *) msg->l2h;
		assert(rd->data_len == len);
		assert(!memcmp(rd->data, data, len));
		break;
#endif
	default:
		assert(0);
	}

	/* free the req_ctx, indicating it has fully arrived on the host */
	usb_buf_free(msg);
}

static void get_and_verify_rctx_pps(const uint8_t *data, unsigned int len)
{
	struct usb_buffered_ep *bep = usb_get_buf_ep(PHONE_DATAIN);
	struct msgb *msg;
	struct simtrace_msg_hdr *mh;
	struct cardemu_usb_msg_pts_info *ptsi;

	assert(bep);
	msg = msgb_dequeue_count(&bep->queue, &bep->queue_len);
	assert(msg);
	dump_rctx(msg);
	assert(msg->l1h);
	mh = (struct simtrace_msg_hdr *) msg->l1h;
	ptsi = (struct cardemu_usb_msg_pts_info *) msg->l2h;

	/* FIXME: verify */
	assert(mh->msg_type == SIMTRACE_MSGT_DO_CEMU_PTS);
	assert(!memcmp(ptsi->req, data, len));
	assert(!memcmp(ptsi->resp, data, len));

	/* free the req_ctx, indicating it has fully arrived on the host */
	usb_buf_free(msg);
}

/* emulate a TPDU header being sent by the reader/phone */
static void rdr_send_tpdu_hdr(struct card_handle *ch, const uint8_t *tpdu_hdr)
{
	struct llist_head *queue = usb_get_queue(PHONE_DATAIN);

	/* we don't want a receive context to become available during
	 * the first four bytes */
	reader_send_bytes(ch, tpdu_hdr, 4);
	assert(llist_empty(queue));

	reader_send_bytes(ch, tpdu_hdr+4, 1);
	/* but then after the final byte of the TPDU header, we want a
	 * receive context to be available for USB transmission */
	get_and_verify_rctx(PHONE_DATAIN, tpdu_hdr, 5);
}

/* emulate a SIMTRACE_MSGT_DT_CEMU_TX_DATA received from USB */
static void host_to_device_data(struct card_handle *ch, const uint8_t *data, uint16_t len,
				unsigned int flags)
{
	struct msgb *msg;
	struct simtrace_msg_hdr *mh;
	struct cardemu_usb_msg_tx_data *rd;
	struct llist_head *queue;

	/* allocate a free req_ctx */
	msg = usb_buf_alloc(PHONE_DATAOUT);
	assert(msg);
	/* initialize the common header */
	msg->l1h = msg->head;
	mh = (struct simtrace_msg_hdr *) msgb_put(msg, sizeof(*mh));
	mh->msg_class = SIMTRACE_MSGC_CARDEM;
	mh->msg_type = SIMTRACE_MSGT_DT_CEMU_TX_DATA;

	/* initialize the tx_data message */
	msg->l2h = msgb_put(msg, sizeof(*rd) + len);
	rd = (struct cardemu_usb_msg_tx_data *) msg->l2h;
	rd->flags = flags;
	/* copy data and set length */
	rd->data_len = len;
	memcpy(rd->data, data, len);

	mh->msg_len = sizeof(*mh) + sizeof(*rd) + len;

	/* hand the req_ctx to the UART transmit code */
	queue = card_emu_get_uart_tx_queue(ch);
	assert(queue);
	msgb_enqueue(queue, msg);
}

/* card-transmit any pending characters */
static int card_tx_verify_chars(struct card_handle *ch, const uint8_t *data, unsigned int data_len)
{
	int count = 0;

	while (card_emu_tx_byte(ch)) {
		count++;
	}

	assert(count == data_len);
	reader_check_and_clear(data, data_len);

	return count;
}

const uint8_t tpdu_hdr_sel_mf[] = { 0xA0, 0xA4, 0x00, 0x00, 0x00 };
const uint8_t tpdu_pb_sw[] = { 0x90, 0x00 };

static void
test_tpdu_reader2card(struct card_handle *ch, const uint8_t *hdr, const uint8_t *body, uint8_t body_len)
{
	printf("\n==> transmitting APDU (HDR + PB + card-RX)\n");

	/* emulate the reader sending a TPDU header */
	rdr_send_tpdu_hdr(ch, hdr);
	/* we shouldn't have any pending card-TX yet */
	card_tx_verify_chars(ch, NULL, 0);

	/* card emulator PC sends a singly byte PB response via USB */
	host_to_device_data(ch, hdr+1, 1, CEMU_DATA_F_FINAL | CEMU_DATA_F_PB_AND_RX);
	/* card actually sends that single PB */
	card_tx_verify_chars(ch, hdr+1, 1);

	/* emulate more characters from reader to card */
	reader_send_bytes(ch, body, body_len);

	/* check if we have received them on the USB side */
	get_and_verify_rctx(PHONE_DATAIN, body, body_len);

	/* ensure there is no extra data received on usb */
	assert(llist_empty(usb_get_queue(PHONE_DATAOUT)));

	/* card emulator sends SW via USB */
	host_to_device_data(ch, tpdu_pb_sw, sizeof(tpdu_pb_sw),
			    CEMU_DATA_F_FINAL | CEMU_DATA_F_PB_AND_TX);
	/* obtain any pending tx chars */
	card_tx_verify_chars(ch, tpdu_pb_sw, sizeof(tpdu_pb_sw));

	/* simulate some clock stop */
	card_emu_io_statechg(ch, CARD_IO_CLK, 0);
	card_emu_io_statechg(ch, CARD_IO_CLK, 1);
}

static void
test_tpdu_card2reader(struct card_handle *ch, const uint8_t *hdr, const uint8_t *body, uint8_t body_len)
{
	printf("\n==> transmitting APDU (HDR + PB + card-TX)\n");

	/* emulate the reader sending a TPDU header */
	rdr_send_tpdu_hdr(ch, hdr);
	card_tx_verify_chars(ch, NULL, 0);

	/* card emulator PC sends a response PB via USB */
	host_to_device_data(ch, hdr+1, 1, CEMU_DATA_F_PB_AND_TX);

	/* card actually sends that PB */
	card_tx_verify_chars(ch, hdr+1, 1);

	/* emulate more characters from card to reader */
	host_to_device_data(ch, body, body_len, 0);
	/* obtain those bytes as they arrvive on the card */
	card_tx_verify_chars(ch, body, body_len);

	/* ensure there is no extra data received on usb */
	assert(llist_empty(usb_get_queue(PHONE_DATAOUT)));

	/* card emulator sends SW via USB */
	host_to_device_data(ch, tpdu_pb_sw, sizeof(tpdu_pb_sw), CEMU_DATA_F_FINAL);

	/* obtain any pending tx chars */
	card_tx_verify_chars(ch, tpdu_pb_sw, sizeof(tpdu_pb_sw));

	/* simulate some clock stop */
	card_emu_io_statechg(ch, CARD_IO_CLK, 0);
	card_emu_io_statechg(ch, CARD_IO_CLK, 1);
}

const uint8_t pps[] = {
	/* PPSS identifies the PPS request or response and is set to
	 * 'FF'. */
	0xFF,		// PPSS
	/* In PPS0, each bit 5, 6 or 7 set to 1 indicates the presence
	 * of an optional byte PPS 1 , PPS 2 , PPS 3 ,
	 * respectively. Bits 4 to 1 encode a type T to propose a
	 * transmission protocol. Bit 8 is reserved for future
	 * use and shall be set to 0. */
	0b00010000,	// PPS0: PPS1 present
	0x00,		// PPS1 proposed Fi/Di value
	0xFF ^ 0b00010000// PCK
};

static void
test_ppss(struct card_handle *ch)
{
	reader_send_bytes(ch, pps, sizeof(pps));
	get_and_verify_rctx_pps(pps, sizeof(pps));
	card_tx_verify_chars(ch, pps, sizeof(pps));
}

/* READ RECORD (offset 0, 10 bytes) */
const uint8_t tpdu_hdr_read_rec[] = { 0xA0, 0xB2, 0x00, 0x00, 0x0A };
const uint8_t tpdu_body_read_rec[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };

/* WRITE RECORD */
const uint8_t tpdu_hdr_write_rec[] = { 0xA0, 0xD2, 0x00, 0x00, 0x07 };
const uint8_t tpdu_body_write_rec[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };

int main(int argc, char **argv)
{
	struct card_handle *ch;
	unsigned int i;

	ch = card_emu_init(0, 23, 42, PHONE_DATAIN, PHONE_INT, false, true, false);
	assert(ch);

	usb_buf_init();

	/* start up the card (VCC/RST, ATR) */
	io_start_card(ch);
	card_tx_verify_chars(ch, NULL, 0);

	test_ppss(ch);

	for (i = 0; i < 2; i++) {
		test_tpdu_reader2card(ch, tpdu_hdr_write_rec, tpdu_body_write_rec, sizeof(tpdu_body_write_rec));

		test_tpdu_card2reader(ch, tpdu_hdr_read_rec, tpdu_body_read_rec, sizeof(tpdu_body_read_rec));
	}

	exit(0);
}
