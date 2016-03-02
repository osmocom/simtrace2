#include "board.h"
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
static LLIST_HEAD(usb_out_queue_usim1);

#ifdef CARDEMU_SECOND_UART
static const Pin pins_usim2[]    = {PINS_USIM1};
static const Pin pin_usim2_rst	= PIN_USIM2_nRST;
static const Pin pin_usim2_vcc	= PIN_USIM2_VCC;
static LLIST_HEAD(usb_out_queue_usim2);
#endif

static struct card_handle *ch1, *ch2;
static struct ringbuf ch1_rb;

static struct Usart_info usart_info[] = {
	{
		.base = USART1,
		.id = ID_USART1,
		.state = USART_RCV
	},
#ifdef CARDEMU_SECOND_UART
	{
		.base = USART0,
		.id = ID_USART0,
		.state = USART_RCV
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
	Usart_info *ui = &usart_info[uart_chan];
	ISO7816_SendChar(byte, ui);
	return 1;
}


/* FIXME: integrate this with actual irq handler */
void usart_irq_rx(uint8_t uart)
{
	Usart *usart = get_usart_by_chan(uart);
	struct card_handle *ch = ch1;
	uint32_t csr;
	uint8_t byte = 0;

#ifdef CARDEMU_SECOND_UART
	if (uart == 1)
		ch = ch2;
#endif
	csr = usart->US_CSR;

	if (csr & US_CSR_RXRDY) {
        	byte = (usart->US_RHR) & 0xFF;
		rbuf_write(&ch1_rb, byte);
	}

	if (csr & US_CSR_TXRDY) {
		if (card_emu_tx_byte(ch) == 0)
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
	card_emu_io_statechg(ch1, CARD_IO_RST, active);
}

static void usim1_vcc_irqhandler(const Pin *pPin)
{
	int active = PIO_Get(&pin_usim1_vcc) ? 1 : 0;
	card_emu_io_statechg(ch1, CARD_IO_VCC, active);
	/* FIXME do this for real */
	card_emu_io_statechg(ch1, CARD_IO_CLK, active);
}

/* executed once at system boot for each config */
void mode_cardemu_configure(void)
{
	TRACE_ENTRY();
}

/* called if config is activated */
void mode_cardemu_init(void)
{
	TRACE_ENTRY();

	rbuf_reset(&ch1_rb);

	PIO_Configure(pins_cardsim, PIO_LISTSIZE(pins_cardsim));

	PIO_Configure(pins_usim1, PIO_LISTSIZE(pins_usim1));
	ISO7816_Init(&usart_info[0], CLK_SLAVE);
	//USART_EnableIt(USART1, US_IER_RXRDY);
	NVIC_EnableIRQ(USART1_IRQn);

	PIO_ConfigureIt(&pin_usim1_rst, usim1_rst_irqhandler);
	PIO_EnableIt(&pin_usim1_rst);
	PIO_ConfigureIt(&pin_usim1_vcc, usim1_vcc_irqhandler);
	PIO_EnableIt(&pin_usim1_vcc);

	ch1 = card_emu_init(0, 2, 0);

#ifdef CARDEMU_SECOND_UART
	PIO_Configure(pins_usim2, PIO_LISTSIZE(pins_usim2));
	ISO7816_Init(&usart_info[1], CLK_SLAVE);
	//USART_EnableIt(USART0, US_IER_RXRDY);
	NVIC_EnableIRQ(USART0_IRQn);
	ch2 = card_emu_init(1, 0, 1);
#endif
}

/* called if config is deactivated */
void mode_cardemu_exit(void)
{
	TRACE_ENTRY();

	PIO_DisableIt(&pin_usim1_rst);
	PIO_DisableIt(&pin_usim1_vcc);

	NVIC_DisableIRQ(USART1_IRQn);
	USART_SetTransmitterEnabled(USART1, 0);
	USART_SetReceiverEnabled(USART1, 0);

#ifdef CARDEMU_SECOND_UART
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

static int usb_pending_old = 0;

/* handle a single USB command as received from the USB host */
static void dispatch_usb_command(struct req_ctx *rctx, struct card_handle *ch)
{
	struct cardemu_usb_msg_hdr *hdr;
	struct cardemu_usb_msg_set_atr *atr;
	struct llist_head *queue;

	hdr = (struct cardemu_usb_msg_hdr *) rctx->data;
	switch (hdr->msg_type) {
	case CEMU_USB_MSGT_DT_TX_DATA:
		queue = card_emu_get_uart_tx_queue(ch);
		req_ctx_set_state(rctx, RCTX_S_UART_TX_PENDING);
		llist_add_tail(&rctx->list, queue);
		card_emu_have_new_uart_tx(ch);
		break;
	case CEMU_USB_MSGT_DT_SET_ATR:
		atr = (struct cardemu_usb_msg_set_atr *) hdr;
		card_emu_set_atr(ch, atr->atr, hdr->data_len);
		llist_del(&rctx->list);
		req_ctx_put(rctx);
		break;
	case CEMU_USB_MSGT_DT_GET_STATS:
	case CEMU_USB_MSGT_DT_GET_STATUS:
	default:
		/* FIXME */
		break;
	}
}

/* iterate over the queue of incoming USB commands and dispatch/execute
 * them */
static void process_any_usb_commands(struct llist_head *main_q, struct card_handle *ch)
{
	struct req_ctx *rctx, *tmp;

	llist_for_each_entry_safe(rctx, tmp, main_q, list)
		dispatch_usb_command(rctx, ch);
}

/* main loop function, called repeatedly */
void mode_cardemu_run(void)
{
	struct llist_head *queue;

	if (ch1) {
		/* drain the ring buffer from UART into card_emu */
		while (1) {
			__disable_irq();
			if (rbuf_is_empty(&ch1_rb)) {
				__enable_irq();
				break;
			}
			uint8_t byte = rbuf_read(&ch1_rb);
			__enable_irq();
			card_emu_process_rx_byte(ch1, byte);
			TRACE_ERROR("Rx%02x\r\n", byte);
		}

		queue = card_emu_get_usb_tx_queue(ch1);
		int usb_pending = llist_count(queue);
		if (usb_pending != usb_pending_old) {
//			printf("usb_pending=%d\r\n", usb_pending);
			usb_pending_old = usb_pending;
		}
		usb_refill_to_host(queue, PHONE_DATAIN);

		/* ensure we can handle incoming USB messages from the
		 * host */
		queue = &usb_out_queue_usim1;
		usb_refill_from_host(queue, PHONE_DATAOUT);
		process_any_usb_commands(queue, ch1);
	}

#ifdef CARDEMU_SECOND_UART
	if (ch2) {
		rst_active = PIO_Get(&pin_usim2_rst) ? 0 : 1;
		vcc_active = PIO_Get(&pin_usim2_vcc) ? 1 : 0;
		card_emu_io_statechg(ch2, CARD_IO_RST, rst_active);
		card_emu_io_statechg(ch2, CARD_IO_VCC, vcc_active);
		/* FIXME: clock ? */
	}
	usb_from_host(FIXME);
#endif
}
