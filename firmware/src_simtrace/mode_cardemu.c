#include "board.h"
#include "card_emu.h"
#include "iso7816_fidi.h"

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

static struct card_handle *ch1, *ch2;

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
		USART_SetReceiverEnabled(usart, 0);
		USART_SetTransmitterEnabled(usart, 1);
		break;
	case ENABLE_RX:
		USART_SetTransmitterEnabled(usart, 0);
		USART_SetReceiverEnabled(usart, 1);
		break;
	case 0:
	default:
		USART_SetTransmitterEnabled(usart, 0);
		USART_SetReceiverEnabled(usart, 0);
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
void usart_irq_rx(uint8_t uart, uint32_t csr, uint8_t byte)
{
	struct card_handle *ch = ch1;
#ifdef CARDEMU_SECOND_UART
	if (uart == 0)
		ch = ch2;
#endif

	if (csr & US_CSR_TXRDY)
		card_emu_tx_byte(ch);

	if (csr & US_CSR_RXRDY)
		card_emu_process_rx_byte(ch, byte);

	if (csr & (US_CSR_OVRE|US_CSR_FRAME|US_CSR_PARE|
		   US_CSR_TIMEOUT|US_CSR_NACK|(1<<10))) {
		TRACE_DEBUG("e 0x%x st: 0x%x\n", byte, csr);
	}
}

/* call-back from card_emu.c to change UART baud rate */
int card_emu_uart_update_fidi(uint8_t uart_chan, unsigned int fidi)
{
	int rc;
        uint8_t fi = fidi >> 4;
        uint8_t di = fidi & 0xf;
	Usart *usart = get_usart_by_chan(uart_chan);

	rc = compute_fidi_ratio(fi, di);
	if (rc > 0 && rc < 0x400) {
		TRACE_INFO("computed Fi(%u) Di(%u) ratio: %d", fi, di, rc);
		usart->US_CR |= US_CR_RXDIS | US_CR_RSTRX;
		usart->US_FIDI = rc & 0x3ff;
		usart->US_CR |= US_CR_RXEN | US_CR_STTTO;
		return 0;
	} else {
		TRACE_INFO("computed FiDi ratio %d unsupported", rc);
		return -1;
	}
}

/***********************************************************************
 * Core USB  / mainloop integration
 ***********************************************************************/

/* executed once at system boot for each config */
void mode_cardemu_configure(void)
{
	TRACE_ENTRY();
}

/* called if config is activated */
void mode_cardemu_init(void)
{
	TRACE_ENTRY();

	PIO_Configure(pins_cardsim, PIO_LISTSIZE(pins_cardsim));

	PIO_Configure(pins_usim1, PIO_LISTSIZE(pins_usim1));
	ISO7816_Init(&usart_info[0], CLK_SLAVE);
	USART_EnableIt(USART1, US_IER_RXRDY);
	NVIC_EnableIRQ(USART1_IRQn);
	ch1 = card_emu_init(0, 2, 0);

#ifdef CARDEMU_SECOND_UART
	PIO_Configure(pins_usim2, PIO_LISTSIZE(pins_usim2));
	ISO7816_Init(&usart_info[1], CLK_SLAVE);
	USART_EnableIt(USART0, US_IER_RXRDY);
	NVIC_EnableIRQ(USART0_IRQn);
	ch2 = card_emu_init(1, 0, 1);
#endif
}

/* called if config is deactivated */
void mode_cardemu_exit(void)
{
	TRACE_ENTRY();

	NVIC_DisableIRQ(USART1_IRQn);
	USART_SetTransmitterEnabled(USART1, 0);
	USART_SetReceiverEnabled(USART1, 0);

#ifdef CARDEMU_SECOND_UART
	NVIC_DisableIRQ(USART0_IRQn);
	USART_SetTransmitterEnabled(USART0, 0);
	USART_SetReceiverEnabled(USART0, 0);
#endif
}

/* main loop function, called repeatedly */
void mode_cardemu_run(void)
{
	int rst_active, vcc_active;

	/* usb_to_host() is handled by main() */

	if (ch1) {
		rst_active = PIO_Get(&pin_usim1_rst) ? 0 : 1;
		vcc_active = PIO_Get(&pin_usim1_vcc) ? 1 : 0;
		card_emu_io_statechg(ch1, CARD_IO_RST, rst_active);
		card_emu_io_statechg(ch1, CARD_IO_VCC, vcc_active);
		/* FIXME: clock ? */
	}
	usb_from_host(PHONE_DATAOUT);

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
