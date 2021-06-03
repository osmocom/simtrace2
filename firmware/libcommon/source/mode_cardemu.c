/* card emulation mode
 *
 * (C) 2015-2017 by Harald Welte <laforge@gnumonks.org>
 * (C) 2018-2019 by sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>
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
#include "board.h"
#include "boardver_adc.h"
#include "simtrace.h"
#include "ringbuffer.h"
#include "card_emu.h"
#include "iso7816_fidi.h"
#include "utils.h"
#include <osmocom/core/linuxlist.h>
#include <osmocom/core/msgb.h>
#include "llist_irqsafe.h"
#include "usb_buf.h"
#include "simtrace_usb.h"
#include "simtrace_prot.h"
#include "sim_switch.h"

#define TRACE_ENTRY()	TRACE_DEBUG("%s entering\r\n", __func__)

#ifdef PINS_CARDSIM
static const Pin pins_cardsim[] = PINS_CARDSIM;
#endif

/* UART pins */
static const Pin pins_usim1[]	= {PINS_USIM1};
static const Pin pin_usim1_rst	= PIN_USIM1_nRST;
static const Pin pin_usim1_vcc	= PIN_USIM1_VCC;
#ifdef PIN_USIM1_IO_DIR
static const Pin pin_io_dir 	= PIN_USIM1_IO_DIR;
#endif


#ifdef CARDEMU_SECOND_UART
static const Pin pins_usim2[]	= {PINS_USIM2};
static const Pin pin_usim2_rst	= PIN_USIM2_nRST;
static const Pin pin_usim2_vcc	= PIN_USIM2_VCC;
#endif

struct cardem_inst {
	unsigned int num;
	struct card_handle *ch;
	struct llist_head usb_out_queue;
	struct ringbuf rb;
	struct Usart_info usart_info;
	struct {
		/*! receiver waiting time to trigger timeout (0 to deactivate it) */
		uint32_t total;
		/*! remaining waiting time (we may need multiple timer runs to reach total */
		uint32_t remaining;
		/*! did we already notify about half the time having expired? */
		bool half_time_notified;
	} wt;
	int usb_pending_old;
	uint8_t ep_out;
	uint8_t ep_in;
	uint8_t ep_int;
	const Pin pin_insert;
#ifdef DETECT_VCC_BY_ADC
	uint32_t vcc_uv;
#endif
	bool vcc_active;
	bool vcc_active_last;
	bool rst_active;
	bool rst_active_last;
};

struct cardem_inst cardem_inst[] = {
	{
		.num = 0,
		.usart_info = 	{
			.base = USART1,
			.id = ID_USART1,
			.state = USART_RCV
		},
		.ep_out = SIMTRACE_CARDEM_USB_EP_USIM1_DATAOUT,
		.ep_in = SIMTRACE_CARDEM_USB_EP_USIM1_DATAIN,
		.ep_int = SIMTRACE_CARDEM_USB_EP_USIM1_INT,
#ifdef PIN_SET_USIM1_PRES
		.pin_insert = PIN_SET_USIM1_PRES,
#endif
	},
#ifdef CARDEMU_SECOND_UART
	{
		.num = 1,
		.usart_info = {
			.base = USART0,
			.id = ID_USART0,
			.state = USART_RCV
		},
		.ep_out = SIMTRACE_CARDEM_USB_EP_USIM2_DATAOUT,
		.ep_in = SIMTRACE_CARDEM_USB_EP_USIM2_DATAIN,
		.ep_int = SIMTRACE_CARDEM_USB_EP_USIM2_INT,
#ifdef PIN_SET_USIM2_PRES
		.pin_insert = PIN_SET_USIM2_PRES,
#endif
	},
#endif
};

static Usart *get_usart_by_chan(uint8_t uart_chan)
{
	if (uart_chan < ARRAY_SIZE(cardem_inst)) {
		return cardem_inst[uart_chan].usart_info.base;
	} else {
		return NULL;
	}
}

/***********************************************************************
 * Call-Backs from card_emu.c
 ***********************************************************************/

static void wait_tx_idle(Usart *usart)
{
	int i = 1;

	/* wait until last char has been fully transmitted */
	while ((usart->US_CSR & (US_CSR_TXEMPTY)) == 0) {
		if (!(i%1000000)) {
			TRACE_ERROR("s: %lx \r\n", usart->US_CSR);
		}
		i++;
	}
}

void card_emu_uart_wait_tx_idle(uint8_t uart_chan)
{
	Usart *usart = get_usart_by_chan(uart_chan);
	wait_tx_idle(usart);
}

static void card_emu_uart_set_direction(uint8_t uart_chan, bool tx)
{
	/* only on some boards (octsimtest) we hae an external level
	 * shifter that requires us to switch the direction between RX and TX */
#ifdef PIN_USIM1_IO_DIR
	if (uart_chan == 0) {
		if (tx)
			PIO_Set(&pin_io_dir);
		else
			PIO_Clear(&pin_io_dir);
	}
#endif
}

/* call-back from card_emu.c to enable/disable transmit and/or receive */
void card_emu_uart_enable(uint8_t uart_chan, uint8_t rxtx)
{
	Usart *usart = get_usart_by_chan(uart_chan);
	switch (rxtx) {
	case ENABLE_TX:
		card_emu_uart_set_direction(uart_chan, true);
		USART_DisableIt(usart, ~(US_IER_TXRDY | US_IER_TIMEOUT));
		/* as irritating as it is, we actually want to keep the
		 * receiver enabled during transmit */
		USART_SetReceiverEnabled(usart, 1);
		usart->US_CR = US_CR_RSTSTA | US_CR_RSTIT | US_CR_RSTNACK;
		USART_EnableIt(usart, US_IER_TXRDY | US_IER_TIMEOUT);
		USART_SetTransmitterEnabled(usart, 1);
		break;
	case ENABLE_TX_TIMER_ONLY:
		/* enable the transmitter without generating TXRDY interrupts
		 * just so that the timer can run */
		USART_DisableIt(usart, ~US_IER_TIMEOUT);
		/* as irritating as it is, we actually want to keep the
		 * receiver enabled during transmit */
		USART_SetReceiverEnabled(usart, 1);
		usart->US_CR = US_CR_RSTSTA | US_CR_RSTIT | US_CR_RSTNACK;
		USART_EnableIt(usart, US_IER_TIMEOUT);
		USART_SetTransmitterEnabled(usart, 1);
		break;
	case ENABLE_RX:
		USART_DisableIt(usart, ~US_IER_RXRDY);
		/* as irritating as it is, we actually want to keep the
		 * transmitter enabled during receive */
		USART_SetTransmitterEnabled(usart, 1);
		wait_tx_idle(usart);
		card_emu_uart_set_direction(uart_chan, false);;
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
	int i = 1;
	while ((usart->US_CSR & (US_CSR_TXRDY)) == 0) {
		if (!(i%1000000)) {
			TRACE_ERROR("%u: s: %lx %02lX\r\n",
				uart_chan, usart->US_CSR,
				usart->US_RHR & 0xFF);
			usart->US_CR = US_CR_RSTTX;
			usart->US_CR = US_CR_RSTRX;
		}
		i++;
	}
	usart->US_THR = byte;
	//TRACE_ERROR("Sx%02x\r\n", byte);
#endif
	return 1;
}

static uint16_t compute_next_timeout(struct cardem_inst *ci)
{
	uint32_t want_to_expire;

	if (ci->wt.total == 0)
		return 0;

	if (!ci->wt.half_time_notified) {
		/* we need to make sure to expire after half the total waiting time */
		OSMO_ASSERT(ci->wt.remaining > (ci->wt.total / 2));
		want_to_expire = ci->wt.remaining - (ci->wt.total / 2);
	} else
		want_to_expire = ci->wt.remaining;
	/* if value exceeds the USART TO range, use the maximum possible value for one round */
	return OSMO_MIN(want_to_expire, 0xffff);
}

/*! common handler if interrupt was received.
 *  \param[in] inst_num Instance number, range 0..1 (some boards only '0' permitted) */
static void usart_irq_rx(uint8_t inst_num)
{
	Usart *usart = get_usart_by_chan(inst_num);
	struct cardem_inst *ci = &cardem_inst[inst_num];
	uint32_t csr;
	uint8_t byte = 0;

	/* get one atomic snapshot of state/flags before they get changed */
	csr = usart->US_CSR & usart->US_IMR;

	/* check if one byte has been completely received and is now in the holding register */
	if (csr & US_CSR_RXRDY) {
		/* read the bye from the holding register */
		byte = (usart->US_RHR) & 0xFF;
		/* append it to the buffer */
		if (rbuf_write(&ci->rb, byte) < 0)
			TRACE_ERROR("rbuf overrun\r\n");
	}

	/* check if the transmitter is ready for the next byte */
	if (csr & US_CSR_TXRDY) {
		/* transmit next byte and check if more bytes are to be transmitted */
		if (card_emu_tx_byte(ci->ch) == 0) {
			/* stop the TX ready interrupt of no more bytes to transmit */
			USART_DisableIt(usart, US_IER_TXRDY);
		}
	}

	/* check if any error flags are set */
	if (csr & (US_CSR_OVRE|US_CSR_FRAME|US_CSR_PARE|US_CSR_NACK|(1<<10))) {
		/* clear any error flags */
		usart->US_CR = US_CR_RSTSTA | US_CR_RSTIT | US_CR_RSTNACK;
		TRACE_ERROR("%u USART error on 0x%x status: 0x%lx\n", ci->num, byte, csr);
	}

	/* check if the timeout has expired. We "abuse" the receive timer for tracking
	 * how many etu have expired since we last sent a byte.  See section
	 * 33.7.3.11 "Receiver Time-out" of the SAM3S8 Data Sheet */
	if (csr & US_CSR_TIMEOUT) {
		/* clear timeout flag (and stop timeout until next character is received) */
		usart->US_CR |= US_CR_STTTO;

		/* RX has been inactive for some time */
		if (ci->wt.remaining <= (usart->US_RTOR & 0xffff)) {
			/* waiting time is over; will stop the timer */
			ci->wt.remaining = 0;
		} else {
			/* subtract the actual timeout since the new might not have been set and
			 * reloaded yet */
			ci->wt.remaining -= (usart->US_RTOR & 0xffff);
		}
		if (ci->wt.remaining == 0) {
			/* let the FSM know that WT has expired */
			card_emu_wtime_expired(ci->ch);
			/* don't automatically re-start in this case */
		} else {
			bool half_time_just_reached = false;

			if (ci->wt.remaining <= ci->wt.total / 2 && !ci->wt.half_time_notified) {
				ci->wt.half_time_notified = true;
				/* don't immediately call card_emu_wtime_half_expired(), as that
				 * in turn may calls card_emu_uart_update_wt() which will change
				 * the timeout but would be overridden 4 lines below */
				half_time_just_reached = true;
			}

			/* update the counter no matter if we reached half time or not */
			usart->US_RTOR = compute_next_timeout(ci);
			/* restart the counter (if wt is 0, the timeout is not started) */
			usart->US_CR |= US_CR_RETTO;

			if (half_time_just_reached)
				card_emu_wtime_half_expired(ci->ch);
		}
	}
}

/*! ISR called for USART0 */
void mode_cardemu_usart0_irq(void)
{
	/* USART0 == Instance 1 == USIM 2 */
	usart_irq_rx(1);
}

/*! ISR called for USART1 */
void mode_cardemu_usart1_irq(void)
{
	/* USART1 == Instance 0 == USIM 1 */
	usart_irq_rx(0);
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

/*! Update WT on USART peripheral.  Will automatically re-start timer with new value.
 *  \param[in] usart USART peripheral to configure
 *  \param[in] wt inactivity Waiting Time before card_emu_wtime_expired is called (0 to disable) */
void card_emu_uart_update_wt(uint8_t uart_chan, uint32_t wt)
{
	OSMO_ASSERT(uart_chan < ARRAY_SIZE(cardem_inst));
	struct cardem_inst *ci = &cardem_inst[uart_chan];
	Usart *usart = get_usart_by_chan(uart_chan);

	if (ci->wt.total != wt) {
		TRACE_DEBUG("%u: USART WT changed from %lu to %lu ETU\r\n", uart_chan,
			    ci->wt.total, wt);
	}

	ci->wt.total = wt;
	/* reset and start the timer */
	card_emu_uart_reset_wt(uart_chan);
}

/*! Reset and re-start waiting timeout count down on USART peripheral.
 *  \param[in] usart USART peripheral to configure */
void card_emu_uart_reset_wt(uint8_t uart_chan)
{
	OSMO_ASSERT(uart_chan < ARRAY_SIZE(cardem_inst));
	struct cardem_inst *ci = &cardem_inst[uart_chan];
	Usart *usart = get_usart_by_chan(uart_chan);

	/* FIXME: guard against race with interrupt handler */
	ci->wt.remaining = ci->wt.total;
	ci->wt.half_time_notified = false;
	usart->US_RTOR = compute_next_timeout(ci);
	/* restart the counter (if wt is 0, the timeout is not started) */
	usart->US_CR |= US_CR_RETTO;
}

/* call-back from card_emu.c to force a USART interrupt */
void card_emu_uart_interrupt(uint8_t uart_chan)
{
	Usart *usart = get_usart_by_chan(uart_chan);
	if (!usart) {
		return;
	}
	if (USART0 == usart) {
		NVIC_SetPendingIRQ(USART0_IRQn);
	} else if (USART1 == usart) {
		NVIC_SetPendingIRQ(USART1_IRQn);
	}
}

/***********************************************************************
 * ADC for VCC voltage detection
 ***********************************************************************/

#ifdef DETECT_VCC_BY_ADC
#if !defined(VCC_UV_THRESH_1V8) || !defined(VCC_UV_THRESH_3V)
#error "You must define VCC_UV_THRESH_{1V1,3V} if you use ADC VCC detection"
#endif

static volatile int adc_triggered = 0;
static int adc_sam3s_reva_errata = 0;

static int card_vcc_adc_init(void)
{
	uint32_t chip_arch = CHIPID->CHIPID_CIDR & CHIPID_CIDR_ARCH_Msk;
	uint32_t chip_ver = CHIPID->CHIPID_CIDR & CHIPID_CIDR_VERSION_Msk;

	PMC_EnablePeripheral(ID_ADC);

	ADC->ADC_CR |= ADC_CR_SWRST;
	if (chip_ver == 0 &&
	    (chip_arch == CHIPID_CIDR_ARCH_SAM3SxA ||
	     chip_arch == CHIPID_CIDR_ARCH_SAM3SxB ||
	     chip_arch == CHIPID_CIDR_ARCH_SAM3SxC)) {
		TRACE_INFO("Enabling Rev.A ADC Errata work-around\r\n");
		adc_sam3s_reva_errata = 1;
	}

	if (adc_sam3s_reva_errata) {
		/* Errata Work-Around to clear EOCx flags */
		volatile uint32_t foo;
		int i;
		for (i = 0; i < 16; i++)
			foo = ADC->ADC_CDR[i];
	}

	/* Initialize ADC for AD7 / AD6, fADC=48/24=2MHz */
	ADC->ADC_MR = ADC_MR_TRGEN_DIS | ADC_MR_LOWRES_BITS_12 |
		      ADC_MR_SLEEP_NORMAL | ADC_MR_FWUP_OFF |
		      ADC_MR_FREERUN_OFF | ADC_MR_PRESCAL(23) |
		      ADC_MR_STARTUP_SUT8 | ADC_MR_SETTLING(3) |
		      ADC_MR_ANACH_NONE | ADC_MR_TRACKTIM(4) |
		      ADC_MR_TRANSFER(1) | ADC_MR_USEQ_NUM_ORDER;
	/* enable AD6 + AD7 channels */
	ADC->ADC_CHER = ADC_CHER_CH7;
	ADC->ADC_IER = ADC_IER_EOC7;
#ifdef CARDEMU_SECOND_UART
	ADC->ADC_CHER |= ADC_CHER_CH6;
	ADC->ADC_IER |= ADC_IER_EOC6;
#endif
	NVIC_EnableIRQ(ADC_IRQn);
	ADC->ADC_CR |= ADC_CR_START;

	return 0;
}

static void process_vcc_adc(struct cardem_inst *ci)
{
	if (ci->vcc_uv >= VCC_UV_THRESH_3V)
		ci->vcc_active = true;
	else
		ci->vcc_active = false;
}

void ADC_IrqHandler(void)
{
#ifdef CARDEMU_SECOND_UART
	if (ADC->ADC_ISR & ADC_ISR_EOC6) {
		uint16_t val = ADC->ADC_CDR[6] & 0xFFF;
		cardem_inst[1].vcc_uv = adc2uv(val);
		process_vcc_adc(&cardem_inst[1]);
		if (adc_sam3s_reva_errata) {
			/* Errata: START doesn't start a conversion
			 * sequence, but only a single conversion */
			ADC->ADC_CR |= ADC_CR_START;
		}
	}
#endif

	if (ADC->ADC_ISR & ADC_ISR_EOC7) {
		uint16_t val = ADC->ADC_CDR[7] & 0xFFF;
		cardem_inst[0].vcc_uv = adc2uv(val);
		process_vcc_adc(&cardem_inst[0]);
		ADC->ADC_CR |= ADC_CR_START;
		adc_triggered = 1;
	}
}
#endif /* DETECT_VCC_BY_ADC */


/* called from main loop; dispatches card I/O state changes to card_emu from main loop */
static void process_io_statechg(struct cardem_inst *ci)
{
	if (ci->vcc_active != ci->vcc_active_last) {
		card_emu_io_statechg(ci->ch, CARD_IO_VCC, ci->vcc_active);
		/* FIXME do this for real */
		card_emu_io_statechg(ci->ch, CARD_IO_CLK, ci->vcc_active);
		ci->vcc_active_last = ci->vcc_active;
	}

	if (ci->rst_active != ci->rst_active_last) {
		card_emu_io_statechg(ci->ch, CARD_IO_RST, ci->rst_active);
		ci->rst_active_last = ci->rst_active;
	}
}

/***********************************************************************
 * Core USB  / main loop integration
 ***********************************************************************/

static void usim1_rst_irqhandler(const Pin *pPin)
{
	cardem_inst[0].rst_active = PIO_Get(&pin_usim1_rst) ? false : true;
}

#ifndef DETECT_VCC_BY_ADC
static void usim1_vcc_irqhandler(const Pin *pPin)
{
	cardem_inst[0].vcc_active = PIO_Get(&pin_usim1_vcc) ? true : false;
}
#endif /* !DETECT_VCC_BY_ADC */

#ifdef CARDEMU_SECOND_UART
static void usim2_rst_irqhandler(const Pin *pPin)
{
	cardem_inst[1].rst_active = PIO_Get(&pin_usim2_rst) ? false : true;
}

#ifndef DETECT_VCC_BY_ADC
static void usim2_vcc_irqhandler(const Pin *pPin)
{
	cardem_inst[1].vcc_active = PIO_Get(&pin_usim2_vcc) ? true : false;
}
#endif /* !DETECT_VCC_BY_ADC */
#endif /* CARDEMU_SECOND_UART */

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

#ifdef PINS_CARDSIM
	PIO_Configure(pins_cardsim, PIO_LISTSIZE(pins_cardsim));
#endif
#ifdef DETECT_VCC_BY_ADC
	card_vcc_adc_init();
#endif /* DETECT_VCC_BY_ADC */

	INIT_LLIST_HEAD(&cardem_inst[0].usb_out_queue);
	rbuf_reset(&cardem_inst[0].rb);
	PIO_Configure(pins_usim1, PIO_LISTSIZE(pins_usim1));

	/* configure USART as ISO-7816 slave (e.g. card) */
	ISO7816_Init(&cardem_inst[0].usart_info, CLK_SLAVE);
	NVIC_EnableIRQ(USART1_IRQn);
	PIO_ConfigureIt(&pin_usim1_rst, usim1_rst_irqhandler);
	PIO_EnableIt(&pin_usim1_rst);

	/* obtain current RST state */
	usim1_rst_irqhandler(&pin_usim1_rst);
#ifndef DETECT_VCC_BY_ADC
	PIO_ConfigureIt(&pin_usim1_vcc, usim1_vcc_irqhandler);
	PIO_EnableIt(&pin_usim1_vcc);

	/* obtain current VCC state */
	usim1_vcc_irqhandler(&pin_usim1_vcc);
#else
	do {} while (!adc_triggered); /* wait for first ADC reading */
#endif /* DETECT_VCC_BY_ADC */

	cardem_inst[0].ch = card_emu_init(0, 0, SIMTRACE_CARDEM_USB_EP_USIM1_DATAIN,
					  SIMTRACE_CARDEM_USB_EP_USIM1_INT, cardem_inst[0].vcc_active,
					  cardem_inst[0].rst_active, cardem_inst[0].vcc_active);
	sim_switch_use_physical(0, 1);

#ifdef CARDEMU_SECOND_UART
	INIT_LLIST_HEAD(&cardem_inst[1].usb_out_queue);
	rbuf_reset(&cardem_inst[1].rb);
	PIO_Configure(pins_usim2, PIO_LISTSIZE(pins_usim2));
	ISO7816_Init(&cardem_inst[1].usart_info, CLK_SLAVE);
	/* TODO enable timeout */
	NVIC_EnableIRQ(USART0_IRQn);
	PIO_ConfigureIt(&pin_usim2_rst, usim2_rst_irqhandler);
	PIO_EnableIt(&pin_usim2_rst);
	usim2_rst_irqhandler(&pin_usim2_rst); /* obtain current RST state */
#ifndef DETECT_VCC_BY_ADC
	PIO_ConfigureIt(&pin_usim2_vcc, usim2_vcc_irqhandler);
	PIO_EnableIt(&pin_usim2_vcc);
	usim2_vcc_irqhandler(&pin_usim2_vcc); /* obtain current VCC state */
#else
	do {} while (!adc_triggered); /* wait for first ADC reading */
#endif /* DETECT_VCC_BY_ADC */

	cardem_inst[1].ch = card_emu_init(1, 1, SIMTRACE_CARDEM_USB_EP_USIM2_DATAIN,
					  SIMTRACE_CARDEM_USB_EP_USIM2_INT, cardem_inst[1].vcc_active,
					  cardem_inst[1].rst_active, cardem_inst[1].vcc_active);
	sim_switch_use_physical(1, 1);
	/* TODO check RST and VCC */
#endif /* CARDEMU_SECOND_UART */
}

/* called if config is deactivated */
void mode_cardemu_exit(void)
{
	TRACE_ENTRY();

	/* FIXME: stop tc_fdt */
	/* FIXME: release all msg, unlink them from any queue */

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

/* handle a single USB command as received from the USB host */
static void dispatch_usb_command_generic(struct msgb *msg, struct cardem_inst *ci)
{
	struct simtrace_msg_hdr *hdr;

	hdr = (struct simtrace_msg_hdr *) msg->l1h;
	switch (hdr->msg_type) {
	case SIMTRACE_CMD_BD_BOARD_INFO:
		break;
	default:
		break;
	}
	usb_buf_free(msg);
}

static void process_card_insert(struct cardem_inst *ci, bool card_insert)
{
	TRACE_INFO("%u: set card_insert to %s\r\n", ci->num, card_insert ? "INSERTED" : "REMOVED");

#ifdef HAVE_BOARD_CARDINSERT
	board_set_card_insert(ci, card_insert);
#else
	if (!ci->pin_insert.pio) {
		TRACE_INFO("%u: skipping unsupported card_insert to %s\r\n",
			   ci->num, card_insert ? "INSERTED" : "REMOVED");
		return;
	}

	if (card_insert)
		PIO_Set(&ci->pin_insert);
	else
		PIO_Clear(&ci->pin_insert);
#endif
}

/* handle a single USB command as received from the USB host */
static void dispatch_usb_command_cardem(struct msgb *msg, struct cardem_inst *ci)
{
	struct simtrace_msg_hdr *hdr;
	struct cardemu_usb_msg_set_atr *atr;
	struct cardemu_usb_msg_cardinsert *cardins;
	struct cardemu_usb_msg_config *cfg;
	struct llist_head *queue;

	hdr = (struct simtrace_msg_hdr *) msg->l1h;
	switch (hdr->msg_type) {
	case SIMTRACE_MSGT_DT_CEMU_TX_DATA:
		queue = card_emu_get_uart_tx_queue(ci->ch);
		llist_add_tail(&msg->list, queue);
		card_emu_have_new_uart_tx(ci->ch);
		break;
	case SIMTRACE_MSGT_DT_CEMU_SET_ATR:
		atr = (struct cardemu_usb_msg_set_atr *) msg->l2h;
		card_emu_set_atr(ci->ch, atr->atr, atr->atr_len);
		usb_buf_free(msg);
		break;
	case SIMTRACE_MSGT_DT_CEMU_CARDINSERT:
		cardins = (struct cardemu_usb_msg_cardinsert *) msg->l2h;
		process_card_insert(ci, cardins->card_insert);
		usb_buf_free(msg);
		break;
	case SIMTRACE_MSGT_BD_CEMU_STATUS:
		card_emu_report_status(ci->ch, false);
		usb_buf_free(msg);
		break;
	case SIMTRACE_MSGT_BD_CEMU_CONFIG:
		cfg = (struct cardemu_usb_msg_config *) msg->l2h;
		card_emu_set_config(ci->ch, cfg, msgb_l2len(msg));
		break;
	case SIMTRACE_MSGT_BD_CEMU_STATS:
	default:
		/* FIXME: Send Error */
		usb_buf_free(msg);
		break;
	}
}

#ifdef PINS_PERST
#include "wwan_perst.h"
#endif

static int usb_command_modem_reset(struct msgb *msg, struct cardem_inst *ci)
{
	struct st_modem_reset *mr = (struct st_modem_reset *) msg->l2h;

	if (msgb_l2len(msg) < sizeof(*mr))
		return -1;

	switch (mr->asserted) {
#ifdef PINS_PERST
	case 0:
		wwan_perst_set(ci->num, 0);
		break;
	case 1:
		wwan_perst_set(ci->num, 1);
		break;
	case 2:
		wwan_perst_do_reset_pulse(ci->num, mr->pulse_duration_msec);
		break;
#endif
	default:
		return -1;
	}

	return 0;
}

static int usb_command_sim_select(struct msgb *msg, struct cardem_inst *ci)
{
	struct st_modem_sim_select *mss = (struct st_modem_sim_select *) msg->l2h;

	if (msgb_l2len(msg) < sizeof(*mss))
		return -1;

	if (mss->remote_sim)
		sim_switch_use_physical(ci->num, 0);
	else
		sim_switch_use_physical(ci->num, 1);

	return 0;
}

/* handle a single USB command as received from the USB host */
static void dispatch_usb_command_modem(struct msgb *msg, struct cardem_inst *ci)
{
	struct simtrace_msg_hdr *hdr;

	hdr = (struct simtrace_msg_hdr *) msg->l1h;
	switch (hdr->msg_type) {
	case SIMTRACE_MSGT_DT_MODEM_RESET:
		usb_command_modem_reset(msg, ci);
		break;
	case SIMTRACE_MSGT_DT_MODEM_SIM_SELECT:
		usb_command_sim_select(msg, ci);
		break;
	case SIMTRACE_MSGT_BD_MODEM_STATUS:
		break;
	default:
		break;
	}
	usb_buf_free(msg);
}

/* handle a single USB command as received from the USB host */
static void dispatch_usb_command(struct msgb *msg, struct cardem_inst *ci)
{
	struct simtrace_msg_hdr *sh = (struct simtrace_msg_hdr *) msg->l1h;

	if (msgb_length(msg) < sizeof(*sh)) {
		/* FIXME: Error */
		usb_buf_free(msg);
		return;
	}
	msg->l2h = msg->l1h + sizeof(*sh);

	switch (sh->msg_class) {
	case SIMTRACE_MSGC_GENERIC:
		dispatch_usb_command_generic(msg, ci);
		break;
	case SIMTRACE_MSGC_CARDEM:
		dispatch_usb_command_cardem(msg, ci);
		break;
	case SIMTRACE_MSGC_MODEM:
		/* FIXME: Find out why this fails if used for !=
		 * MSGC_MODEM ?!? */
		dispatch_usb_command_modem(msg, ci);
		break;
	default:
		/* FIXME: Send Error */
		usb_buf_free(msg);
		break;
	}
}

static void dispatch_received_msg(struct msgb *msg, struct cardem_inst *ci)
{
	struct msgb *segm;
	struct simtrace_msg_hdr *mh;

	/* check if we have multiple concatenated commands in
	 * one message.  USB endpoints are streams that don't
	 * preserve the message boundaries */
	mh = (struct simtrace_msg_hdr *) msg->data;
	if (mh->msg_len == msgb_length(msg)) {
		/* fast path: only one message in buffer */
		dispatch_usb_command(msg, ci);
		return;
	}

	/* slow path: iterate over list of messages, allocating one new
	 * reqe_ctx per segment */
	while (1) {
		mh = (struct simtrace_msg_hdr *) msg->data;

		segm = usb_buf_alloc(ci->ep_out);
		if (!segm) {
			TRACE_ERROR("%u: ENOMEM during msg segmentation\r\n",
				    ci->num);
			break;
		}

		if (mh->msg_len > msgb_length(msg)) {
			TRACE_ERROR("%u: Unexpected large message (%u bytes)\r\n",
					ci->num, mh->msg_len);
			usb_buf_free(segm);
			break;
		} else {
			uint8_t *cur = msgb_put(segm, mh->msg_len);
			segm->l1h = segm->head;
			memcpy(cur, mh, mh->msg_len);
			dispatch_usb_command(segm, ci);
		}
		/* pull this message */
		msgb_pull(msg, mh->msg_len);
		/* abort if we're done */
		if (msgb_length(msg) <= 0)
			break;
	}

	usb_buf_free(msg);
}

/* iterate over the queue of incoming USB commands and dispatch/execute
 * them */
static void process_any_usb_commands(struct llist_head *main_q,
				     struct cardem_inst *ci)
{
	struct llist_head *lh;
	struct msgb *msg;
	int i;

	/* limit the number of iterations to 10, to ensure we don't get
	 * stuck here without returning to main loop processing */
	for (i = 0; i < 10; i++) {
		/* de-queue the list head in an irq-safe way */
		lh = llist_head_dequeue_irqsafe(main_q);
		if (!lh)
			break;
		msg = llist_entry(lh, struct msgb, list);
		dispatch_received_msg(msg, ci);
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
			//TRACE_ERROR("%uRx%02x\r\n", i, byte);
		}

		process_io_statechg(ci);

		/* first try to send any pending messages on IRQ */
		usb_refill_to_host(ci->ep_int);

		/* then try to send any pending messages on IN */
		usb_refill_to_host(ci->ep_in);

		/* ensure we can handle incoming USB messages from the
		 * host */
		usb_refill_from_host(ci->ep_out);
		queue = usb_get_queue(ci->ep_out);
		process_any_usb_commands(queue, ci);
	}
}
