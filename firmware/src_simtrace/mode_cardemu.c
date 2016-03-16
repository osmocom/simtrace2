#define TRACE_LEVEL 6

#include "board.h"
#include "simtrace.h"
#include "ringbuffer.h"
#include "card_emu.h"
#include "iso7816_fidi.h"
#include "utils.h"
#include "linuxlist.h"
#include "req_ctx.h"
#include "cardemu_prot.h"

#define TRACE_ENTRY()	TRACE_DEBUG("%s entering\n", __func__)

static const Pin pins_cardsim[] = PINS_CARDSIM;

/* UART pins */
static const Pin pins_usim1[]	= {PINS_USIM1};
static const Pin pin_usim1_rst	= PIN_USIM1_nRST;
static const Pin pin_usim1_vcc	= PIN_USIM1_VCC;

#ifdef CARDEMU_SECOND_UART
static const Pin pins_usim2[]    = {PINS_USIM1};
static const Pin pin_usim2_rst	= PIN_USIM2_nRST;
static const Pin pin_usim2_vcc	= PIN_USIM2_VCC;
#endif

struct cardem_inst {
	struct card_handle *ch;
	struct llist_head usb_out_queue;
	struct ringbuf rb;
	struct Usart_info usart_info;
	int usb_pending_old;
	uint8_t ep_out;
	uint8_t ep_in;
	uint8_t ep_int;
	const Pin pin_insert;
};

static struct cardem_inst cardem_inst[] = {
	{
		.usart_info = 	{
			.base = USART1,
			.id = ID_USART1,
			.state = USART_RCV
		},
		.ep_out = PHONE_DATAOUT,
		.ep_in = PHONE_DATAIN,
		.ep_int = PHONE_INT,
		.pin_insert = PIN_SET_USIM1_PRES,
	},
#ifdef CARDEMU_SECOND_UART
	{
		.usart_info = {
			.base = USART0,
			.id = ID_USART0,
			.state = USART_RCV
		},
		.ep_out = CARDEM_USIM2_DATAOUT,
		.ep_in = CARDEM_USIM2_DATAIN,
		.ep_int = CARDEM_USIM2_INT,
		.pin_insert = PIN_SET_USIM2_PRES,
	},
#endif
};

static Usart *get_usart_by_chan(uint8_t uart_chan)
{
	switch (uart_chan) {
	case 0:
		return USART1;
#ifdef CARDEMU_SECOND_UART
	case 1:
		return USART0;
#endif
	}
	return NULL;
}

/***********************************************************************
 * Call-Backs from card_emu.c
 ***********************************************************************/

/* call-back from card_emu.c to enable/disable transmit and/or receive */
void card_emu_uart_enable(uint8_t uart_chan, uint8_t rxtx)
{
	Usart *usart = get_usart_by_chan(uart_chan);
	switch (rxtx) {
	case ENABLE_TX:
		USART_DisableIt(usart, ~US_IER_TXRDY);
		USART_SetReceiverEnabled(usart, 0);
		usart->US_CR = US_CR_RSTSTA | US_CR_RSTIT | US_CR_RSTNACK;
		USART_EnableIt(usart, US_IER_TXRDY);
		USART_SetTransmitterEnabled(usart, 1);
		break;
	case ENABLE_RX:
		USART_DisableIt(usart, ~US_IER_RXRDY);
		USART_SetTransmitterEnabled(usart, 0);
		usart->US_CR = US_CR_RSTSTA | US_CR_RSTIT | US_CR_RSTNACK;
		USART_EnableIt(usart, US_IER_RXRDY);
		USART_SetReceiverEnabled(usart, 1);
		break;
	case 0:
	default:
		USART_SetTransmitterEnabled(usart, 0);
		USART_SetReceiverEnabled(usart, 0);
		USART_DisableIt(usart, 0xFFFFFFFF);
		usart->US_CR = US_CR_RSTSTA | US_CR_RSTIT | US_CR_RSTNACK;
		break;
	}
}

/* call-back from card_emu.c to transmit a byte */
int card_emu_uart_tx(uint8_t uart_chan, uint8_t byte)
{
	Usart *usart = get_usart_by_chan(uart_chan);
#if 0
	Usart_info *ui = &usart_info[uart_chan];
	ISO7816_SendChar(byte, ui);
#else
	int i = 0;
	while ((usart->US_CSR & (US_CSR_TXRDY)) == 0) {
		if (!(i%1000000)) {
			printf("s: %x %02X", usart->US_CSR, usart->US_RHR & 0xFF);
			usart->US_CR = US_CR_RSTTX;
			usart->US_CR = US_CR_RSTRX;
		}
	}
	usart->US_THR = byte;
	TRACE_ERROR("Sx%02x\r\n", byte);
#endif
	return 1;
}


/* FIXME: integrate this with actual irq handler */
void usart_irq_rx(uint8_t uart)
{
	Usart *usart = get_usart_by_chan(uart);
	struct cardem_inst *ci = &cardem_inst[0];
	uint32_t csr;
	uint8_t byte = 0;

#ifdef CARDEMU_SECOND_UART
	if (uart == 1)
		ci = &cardem_inst[1];
#endif
	csr = usart->US_CSR;

	if (csr & US_CSR_RXRDY) {
        	byte = (usart->US_RHR) & 0xFF;
		rbuf_write(&ci->rb, byte);
	}

	if (csr & US_CSR_TXRDY) {
		if (card_emu_tx_byte(ci->ch) == 0)
			USART_DisableIt(usart, US_IER_TXRDY);
	}

	if (csr & (US_CSR_OVRE|US_CSR_FRAME|US_CSR_PARE|
		   US_CSR_TIMEOUT|US_CSR_NACK|(1<<10))) {
		usart->US_CR = US_CR_RSTSTA | US_CR_RSTIT | US_CR_RSTNACK;
		TRACE_ERROR("e 0x%x st: 0x%x\n", byte, csr);
	}
}

/* call-back from card_emu.c to change UART baud rate */
int card_emu_uart_update_fidi(uint8_t uart_chan, unsigned int fidi)
{
	int rc;
	Usart *usart = get_usart_by_chan(uart_chan);

	usart->US_CR |= US_CR_RXDIS | US_CR_RSTRX;
	usart->US_FIDI = fidi & 0x3ff;
	usart->US_CR |= US_CR_RXEN | US_CR_STTTO;
	return 0;
}

/***********************************************************************
 * Core USB  / mainloop integration
 ***********************************************************************/

static void usim1_rst_irqhandler(const Pin *pPin)
{
	int active = PIO_Get(&pin_usim1_rst) ? 0 : 1;
	card_emu_io_statechg(cardem_inst[0].ch, CARD_IO_RST, active);
}

static void usim1_vcc_irqhandler(const Pin *pPin)
{
	int active = PIO_Get(&pin_usim1_vcc) ? 1 : 0;
	card_emu_io_statechg(cardem_inst[0].ch, CARD_IO_VCC, active);
	/* FIXME do this for real */
	card_emu_io_statechg(cardem_inst[0].ch, CARD_IO_CLK, active);
}

#ifdef CARDEMU_SECOND_UART
static void usim2_rst_irqhandler(const Pin *pPin)
{
	int active = PIO_Get(&pin_usim2_rst) ? 0 : 1;
	card_emu_io_statechg(cardem_inst[1].ch, CARD_IO_RST, active);
}

static void usim2_vcc_irqhandler(const Pin *pPin)
{
	int active = PIO_Get(&pin_usim2_vcc) ? 1 : 0;
	card_emu_io_statechg(cardem_inst[1].ch, CARD_IO_VCC, active);
	/* FIXME do this for real */
	card_emu_io_statechg(cardem_inst[1].ch, CARD_IO_CLK, active);
}
#endif

/* executed once at system boot for each config */
void mode_cardemu_configure(void)
{
	TRACE_ENTRY();
}

/* called if config is activated */
void mode_cardemu_init(void)
{
	int i;

	TRACE_ENTRY();

	PIO_Configure(pins_cardsim, PIO_LISTSIZE(pins_cardsim));

	INIT_LLIST_HEAD(&cardem_inst[0].usb_out_queue);
	rbuf_reset(&cardem_inst[0].rb);
	PIO_Configure(pins_usim1, PIO_LISTSIZE(pins_usim1));
	ISO7816_Init(&cardem_inst[0].usart_info, CLK_SLAVE);
	NVIC_EnableIRQ(USART1_IRQn);
	PIO_ConfigureIt(&pin_usim1_rst, usim1_rst_irqhandler);
	PIO_EnableIt(&pin_usim1_rst);
	PIO_ConfigureIt(&pin_usim1_vcc, usim1_vcc_irqhandler);
	PIO_EnableIt(&pin_usim1_vcc);
	cardem_inst[0].ch = card_emu_init(0, 2, 0);

#ifdef CARDEMU_SECOND_UART
	INIT_LLIST_HEAD(&cardem_inst[1].usb_out_queue);
	rbuf_reset(&cardem_inst[1].rb);
	PIO_Configure(pins_usim2, PIO_LISTSIZE(pins_usim2));
	ISO7816_Init(&cardem_inst[1].usart_info, CLK_SLAVE);
	NVIC_EnableIRQ(USART0_IRQn);
	PIO_ConfigureIt(&pin_usim2_rst, usim2_rst_irqhandler);
	PIO_EnableIt(&pin_usim2_rst);
	PIO_ConfigureIt(&pin_usim2_vcc, usim2_vcc_irqhandler);
	PIO_EnableIt(&pin_usim2_vcc);
	cardem_inst[1].ch = card_emu_init(1, 0, 1);
#endif
}

/* called if config is deactivated */
void mode_cardemu_exit(void)
{
	TRACE_ENTRY();

	/* FIXME: stop tc_fdt */
	/* FIXME: release all rctx, unlink them from any queue */

	PIO_DisableIt(&pin_usim1_rst);
	PIO_DisableIt(&pin_usim1_vcc);

	NVIC_DisableIRQ(USART1_IRQn);
	USART_SetTransmitterEnabled(USART1, 0);
	USART_SetReceiverEnabled(USART1, 0);

#ifdef CARDEMU_SECOND_UART
	PIO_DisableIt(&pin_usim2_rst);
	PIO_DisableIt(&pin_usim2_vcc);

	NVIC_DisableIRQ(USART0_IRQn);
	USART_SetTransmitterEnabled(USART0, 0);
	USART_SetReceiverEnabled(USART0, 0);
#endif
}

static int llist_count(struct llist_head *head)
{
	struct llist_head *list;
	int i = 0;

	llist_for_each(list, head)
		i++;

	return i;
}

/* handle a single USB command as received from the USB host */
static void dispatch_usb_command(struct req_ctx *rctx, struct cardem_inst *ci)
{
	struct cardemu_usb_msg_hdr *hdr;
	struct cardemu_usb_msg_set_atr *atr;
	struct cardemu_usb_msg_cardinsert *cardins;
	struct llist_head *queue;

	hdr = (struct cardemu_usb_msg_hdr *) rctx->data;
	switch (hdr->msg_type) {
	case CEMU_USB_MSGT_DT_TX_DATA:
		queue = card_emu_get_uart_tx_queue(ci->ch);
		req_ctx_set_state(rctx, RCTX_S_UART_TX_PENDING);
		llist_add_tail(&rctx->list, queue);
		card_emu_have_new_uart_tx(ci->ch);
		break;
	case CEMU_USB_MSGT_DT_SET_ATR:
		atr = (struct cardemu_usb_msg_set_atr *) hdr;
		card_emu_set_atr(ci->ch, atr->atr, hdr->data_len);
		req_ctx_put(rctx);
		break;
	case CEMU_USB_MSGT_DT_CARDINSERT:
		cardins = (struct cardemu_usb_msg_cardinsert *) hdr;
		if (cardins->card_insert)
			PIO_Set(&ci->pin_insert);
		else
			PIO_Clear(&ci->pin_insert);
		req_ctx_put(rctx);
		break;
	case CEMU_USB_MSGT_DT_GET_STATS:
	case CEMU_USB_MSGT_DT_GET_STATUS:
	default:
		/* FIXME */
		req_ctx_put(rctx);
		break;
	}
}

/* iterate over the queue of incoming USB commands and dispatch/execute
 * them */
static void process_any_usb_commands(struct llist_head *main_q, struct cardem_inst *ci)
{
	struct req_ctx *rctx, *tmp;

	llist_for_each_entry_safe(rctx, tmp, main_q, list) {
		llist_del(&rctx->list);
		dispatch_usb_command(rctx, ci);
	}
}

/* main loop function, called repeatedly */
void mode_cardemu_run(void)
{
	struct llist_head *queue;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(cardem_inst); i++) {
		struct cardem_inst *ci = &cardem_inst[i];

		/* drain the ring buffer from UART into card_emu */
		while (1) {
			__disable_irq();
			if (rbuf_is_empty(&ci->rb)) {
				__enable_irq();
				break;
			}
			uint8_t byte = rbuf_read(&ci->rb);
			__enable_irq();
			card_emu_process_rx_byte(ci->ch, byte);
			TRACE_ERROR("Rx%02x\r\n", byte);
		}

		queue = card_emu_get_usb_tx_queue(ci->ch);
		int usb_pending = llist_count(queue);
		if (usb_pending != ci->usb_pending_old) {
//			printf("usb_pending=%d\r\n", usb_pending);
			ci->usb_pending_old = usb_pending;
		}
		usb_refill_to_host(queue, ci->ep_in);

		/* ensure we can handle incoming USB messages from the
		 * host */
		queue = &ci->usb_out_queue;
		usb_refill_from_host(queue, ci->ep_out);
		process_any_usb_commands(queue, ci);
	}

}
