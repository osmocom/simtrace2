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

int card_emu_uart_tx(uint8_t uart_chan, uint8_t byte)
{
	printf("TX: 0x%02x\n", byte);
	return 1;
}

void card_emu_uart_enable(uint8_t uart_chan, uint8_t rxtx)
{
	printf("uart_enable(uart_chan=%u, rxtx=0x%02x)\n", uart_chan, rxtx);
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
}




#if 0
/* process a single byte received from the reader */
void card_emu_process_rx_byte(struct card_handle *ch, uint8_t byte);

/* return a single byte to be transmitted to the reader */
int card_emu_get_tx_byte(struct card_handle *ch, uint8_t *byte);

/* hardware driver informs us that a card I/O signal has changed */
void card_emu_io_statechg(struct card_handle *ch, enum card_io io, int active);

/* User sets a new ATR to be returned during next card reset */
int card_emu_set_atr(struct card_handle *ch, const uint8_t *atr, uint8_t len);
#endif


static int verify_atr(struct card_handle *ch)
{
	uint8_t atr[4];
	uint8_t byte;
	unsigned int i;

	printf("receiving + verifying ATR:");
	for (i = 0; i < sizeof(atr); i++) {
		assert(card_emu_get_tx_byte(ch, &atr[i]) == 1);
		printf(" %02x", atr[i]);
	}
	printf("\n");
	assert(card_emu_get_tx_byte(ch, &byte) == 0);

	return 1;
}

static void io_start_card(struct card_handle *ch)
{
	uint8_t byte;

	/* bring the card up from the dead */
	card_emu_io_statechg(ch, CARD_IO_VCC, 1);
	assert(card_emu_get_tx_byte(ch, &byte) == 0);
	card_emu_io_statechg(ch, CARD_IO_CLK, 1);
	assert(card_emu_get_tx_byte(ch, &byte) == 0);
	card_emu_io_statechg(ch, CARD_IO_RST, 1);
	assert(card_emu_get_tx_byte(ch, &byte) == 0);

	/* release from reset and verify th ATR */
	card_emu_io_statechg(ch, CARD_IO_RST, 0);
	verify_atr(ch);
}

static void send_bytes(struct card_handle *ch, const uint8_t *bytes, unsigned int len)
{
	unsigned int i;
	for (i = 0; i < len; i++)
		card_emu_process_rx_byte(ch, bytes[i]);
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
		for (i = 0; i < mh->data_len; i++)
			printf(" %02x", rxd->data[i]);
		printf("\n");
		break;
	}
}

static void send_tpdu_hdr(struct card_handle *ch, const uint8_t *tpdu_hdr)
{
	struct req_ctx *rctx;

	/* we don't want a receive context to become available during
	 * the first four bytes */
	send_bytes(ch, tpdu_hdr, 4);
	assert(!req_ctx_find_get(1, RCTX_S_USB_TX_PENDING, RCTX_S_USB_TX_BUSY));

	send_bytes(ch, tpdu_hdr+4, 1);
	/* but then after the final byte of the TPDU header, we want a
	 * receive context to be available for USB transmission */
	rctx = req_ctx_find_get(1, RCTX_S_USB_TX_PENDING, RCTX_S_USB_TX_BUSY);
	assert(rctx);
	dump_rctx(rctx);
}

const uint8_t tpdu_hdr_sel_mf[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02 };

int main(int argc, char **argv)
{
	struct card_handle *ch;

	req_ctx_init();

	ch = card_emu_init(0, 23, 42);
	assert(ch);

	io_start_card(ch);

	send_tpdu_hdr(ch, tpdu_hdr_sel_mf);

	exit(0);
}
