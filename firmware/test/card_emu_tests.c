#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "card_emu.h"
#include "cardemu_prot.h"
#include "tc_etu.h"
#include "req_ctx.h"

/* stub functions required by card_emu.c */

int card_emu_uart_update_fidi(uint8_t uart_chan, unsigned int fidi)
{
	printf("uart_update_fidi(uart_chan=%u, fidi=%u)\n", uart_chan, fidi);
	return 0;
}

/* a buffer in which we store those bytes send by the UART towards the card
 * reader, so we can verify in test cases what was actually written  */
static uint8_t tx_debug_buf[1024];
static unsigned int tx_debug_buf_idx;

int card_emu_uart_tx(uint8_t uart_chan, uint8_t byte)
{
	printf("UART_TX(%02x)\n", byte);
	tx_debug_buf[tx_debug_buf_idx++] = byte;
	return 1;
}

static void reader_check_and_clear(const uint8_t *data, unsigned int len)
{
	assert(len == tx_debug_buf_idx);
	assert(!memcmp(tx_debug_buf, data, len));
	tx_debug_buf_idx = 0;
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

const uint8_t atr[] = { 0x3b, 0x02, 0x14, 0x50 };

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
	verify_atr(ch);
}

static void reader_send_bytes(struct card_handle *ch, const uint8_t *bytes, unsigned int len)
{
	unsigned int i;
	for (i = 0; i < len; i++) {
		printf("UART_RX(%02x)\n", bytes[i]);
		card_emu_process_rx_byte(ch, bytes[i]);
	}
}

static void dump_rctx(struct req_ctx *rctx)
{
	struct cardemu_usb_msg_hdr *mh =
		(struct cardemu_usb_msg_hdr *) rctx->data;
	struct cardemu_usb_msg_rx_data *rxd;
	int i;

	printf("req_ctx(%p): state=%u, size=%u, tot_len=%u, idx=%u, data=%p\n",
		rctx, rctx->state, rctx->size, rctx->tot_len, rctx->idx, rctx->data);
	printf("  msg_type=%u, seq_nr=%u, data_len=%u\n",
		mh->msg_type, mh->seq_nr, mh->data_len);

	switch (mh->msg_type) {
	case CEMU_USB_MSGT_DO_RX_DATA:
		rxd = (struct cardemu_usb_msg_rx_data *)mh;
		printf("    flags=%x, data=", rxd->flags);
		for (i = 0; i < rxd->data_len; i++)
			printf(" %02x", rxd->data[i]);
		printf("\n");
		break;
	}
}

static void get_and_verify_rctx(int state, const uint8_t *data, unsigned int len)
{
	struct req_ctx *rctx;
	struct cardemu_usb_msg_tx_data *td;
	struct cardemu_usb_msg_rx_data *rd;

	rctx = req_ctx_find_get(0, state, RCTX_S_USB_TX_BUSY);
	assert(rctx);
	dump_rctx(rctx);

	/* verify the contents of the rctx */
	switch (state) {
	case RCTX_S_USB_TX_PENDING:
		td = (struct cardemu_usb_msg_tx_data *) rctx->data;
		assert(td->hdr.msg_type == CEMU_USB_MSGT_DO_RX_DATA);
		assert(td->data_len == len);
		assert(!memcmp(td->data, data, len));
		break;
#if 0
	case RCTX_S_UART_RX_PENDING:
		rd = (struct cardemu_usb_msg_rx_data *) rctx->data;
		assert(rd->data_len == len);
		assert(!memcmp(rd->data, data, len));
		break;
#endif
	default:
		assert(0);
	}

	/* free the req_ctx, indicating it has fully arrived on the host */
	req_ctx_set_state(rctx, RCTX_S_FREE);
}

static void get_and_verify_rctx_pps(const uint8_t *data, unsigned int len)
{
	struct req_ctx *rctx;
	struct cardemu_usb_msg_pts_info *ptsi;

	rctx = req_ctx_find_get(0, RCTX_S_USB_TX_PENDING, RCTX_S_USB_TX_BUSY);
	assert(rctx);
	dump_rctx(rctx);

	ptsi = (struct cardemu_usb_msg_pts_info *) rctx->data;
	/* FIXME: verify */
	assert(ptsi->hdr.msg_type == CEMU_USB_MSGT_DO_PTS);
	assert(!memcmp(ptsi->req, data, len));
	assert(!memcmp(ptsi->resp, data, len));

	/* free the req_ctx, indicating it has fully arrived on the host */
	req_ctx_set_state(rctx, RCTX_S_FREE);
}

/* emulate a TPDU header being sent by the reader/phone */
static void rdr_send_tpdu_hdr(struct card_handle *ch, const uint8_t *tpdu_hdr)
{
	/* we don't want a receive context to become available during
	 * the first four bytes */
	reader_send_bytes(ch, tpdu_hdr, 4);
	assert(!req_ctx_find_get(0, RCTX_S_USB_TX_PENDING, RCTX_S_USB_TX_BUSY));

	reader_send_bytes(ch, tpdu_hdr+4, 1);
	/* but then after the final byte of the TPDU header, we want a
	 * receive context to be available for USB transmission */
	get_and_verify_rctx(RCTX_S_USB_TX_PENDING, tpdu_hdr, 5);
}

/* emulate a CEMU_USB_MSGT_DT_TX_DATA received from USB */
static void host_to_device_data(const uint8_t *data, uint16_t len, unsigned int flags)
{
	struct req_ctx *rctx;
	struct cardemu_usb_msg_tx_data *rd;

	/* allocate a free req_ctx */
	rctx = req_ctx_find_get(0, RCTX_S_FREE, RCTX_S_USB_RX_BUSY);
	assert(rctx);

	/* initialize the header */
	rd = (struct cardemu_usb_msg_tx_data *) rctx->data;
	rctx->tot_len = sizeof(*rd) + len;
	cardemu_hdr_set(&rd->hdr, CEMU_USB_MSGT_DT_TX_DATA);
	rd->flags = flags;
	/* copy data and set length */
	rd->data_len = len;
	memcpy(rd->data, data, len);
	rd->hdr.msg_len = sizeof(*rd) + len;

	/* hand the req_ctx to the UART transmit code */
	req_ctx_set_state(rctx, RCTX_S_UART_TX_PENDING);
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
	host_to_device_data(hdr+1, 1, CEMU_DATA_F_FINAL | CEMU_DATA_F_PB_AND_RX);
	/* card actually sends that single PB */
	card_tx_verify_chars(ch, hdr+1, 1);

	/* emulate more characters from reader to card */
	reader_send_bytes(ch, body, body_len);

	/* check if we have received them on the USB side */
	get_and_verify_rctx(RCTX_S_USB_TX_PENDING, body, body_len);

	/* ensure there is no extra data received on usb */
	assert(!req_ctx_find_get(0, RCTX_S_USB_TX_PENDING, RCTX_S_USB_TX_BUSY));

	/* card emulator sends SW via USB */
	host_to_device_data(tpdu_pb_sw, sizeof(tpdu_pb_sw),
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
	host_to_device_data(hdr+1, 1, CEMU_DATA_F_PB_AND_TX);

	/* card actually sends that PB */
	card_tx_verify_chars(ch, hdr+1, 1);

	/* emulate more characters from card to reader */
	host_to_device_data(body, body_len, 0);
	/* obtain those bytes as they arrvive on the card */
	card_tx_verify_chars(ch, body, body_len);

	/* ensure there is no extra data received on usb */
	assert(!req_ctx_find_get(0, RCTX_S_USB_TX_PENDING, RCTX_S_USB_TX_BUSY));

	/* card emulator sends SW via USB */
	host_to_device_data(tpdu_pb_sw, sizeof(tpdu_pb_sw), CEMU_DATA_F_FINAL);

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

	req_ctx_init();

	ch = card_emu_init(0, 23, 42);
	assert(ch);

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
