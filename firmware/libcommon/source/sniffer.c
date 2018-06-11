/*
 * (C) 2010-2017 by Harald Welte <hwelte@sysmocom.de>
 * (C) 2018 by Kevin Redon <kredon@sysmocom.de>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/* This code implement the Sniffer mode to sniff the communication between a SIM card and a phone.
 * For historical reasons (i.e. SIMtrace hardware) the USART peripheral connected to the SIM card is used.
 */
#include "board.h"
#include "simtrace.h"

#ifdef HAVE_SNIFFER

/*------------------------------------------------------------------------------
 *          Headers
 *------------------------------------------------------------------------------*/

#include <string.h>

/*------------------------------------------------------------------------------
 *         Internal definitions
 *------------------------------------------------------------------------------*/

/** Maximum ucSize in bytes of the smartcard answer to a command.*/
#define MAX_ANSWER_SIZE         10

/*! Maximum Answer-To-Reset (ATR) size in bytes ucSize in bytes
 *  @note defined in ISO/IEC 7816-3:2006(E) section 8.2.1 as 32, on top the initial character TS of section 8.1
 *  @remark technical there is no size limitation since Yi present in T0,TDi will indicate if more interface bytes are present, including TDi+i
 */
#define MAX_ATR_SIZE 33

/*! ISO 7816-3 states relevant to the sniff mode */
enum iso7816_3_sniff_state {
	ISO7816_S_RESET, /*!< in Reset */
	ISO7816_S_WAIT_ATR, /*!< waiting for ATR to start */
	ISO7816_S_IN_ATR, /*!< while we are receiving the ATR */
	ISO7816_S_WAIT_APDU, /*!< waiting for start of new APDU */
	ISO7816_S_IN_APDU, /*!< inside a single APDU */
	ISO7816_S_IN_PTS, /*!< while we are inside the PTS / PSS */
};

/*! Answer-To-Reset (ATR) sub-states of ISO7816_S_IN_ATR
 *  @note defined in ISO/IEC 7816-3:2006(E) section 8
 */
enum atr_sniff_state {
	ATR_S_WAIT_TS, /*!< initial byte */
	ATR_S_WAIT_T0, /*!< format byte */
	ATR_S_WAIT_TA, /*!< first sub-group interface byte */
	ATR_S_WAIT_TB, /*!< second sub-group interface byte */
	ATR_S_WAIT_TC, /*!< third sub-group interface byte */
	ATR_S_WAIT_TD, /*!< fourth sub-group interface byte */
	ATR_S_WAIT_HIST, /*!< historical byte */
	ATR_S_WAIT_TCK, /*!< check byte */
	ATR_S_DONE, /*!< to indicated all ATR bytes have been received */
};

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/

/* note: the sniffer code is currently designed to support only one sniffing interface, but the hardware would support a second one.
 * to support a second sniffer interface the code should be restructured to use handles.
 */
/* Pin configurations */
/* Pin configuration to sniff communication (using USART connection card) */
static const Pin pins_sniff[] = { PINS_SIM_SNIFF };
static const Pin pins_bus[] = { PINS_BUS_SNIFF };
static const Pin pins_power[] = { PINS_PWR_SNIFF };
static const Pin pins_tc[] = { PINS_TC };
/* USART related variables */
/* USART peripheral used to sniff communication */
static struct Usart_info sniff_usart = {
	.base = USART_SIM,
	.id = ID_USART_SIM,
	.state = USART_RCV,
};
/* Ring buffer to store sniffer communication data */
static struct ringbuf sniff_buffer;

/*------------------------------------------------------------------------------
 *         Internal functions
 *------------------------------------------------------------------------------*/

static void check_sniffed_data(void)
{
	/* Display sniffed data */
	while (!rbuf_is_empty(&sniff_buffer)) {
		uint8_t byte = rbuf_read(&sniff_buffer);
		TRACE_INFO_WP("0x%02x ", byte);
	}
}

/*! Interrupt Service Routine called on USART activity */
void Sniffer_usart_irq(void)
{
	/* Read channel status register */
	uint32_t csr = sniff_usart.base->US_CSR & sniff_usart.base->US_IMR;
	/* Verify if character has been received */
	if (csr & US_CSR_RXRDY) {
		/* Read communication data byte between phone and SIM */
		uint8_t byte = sniff_usart.base->US_RHR;
		/* Store sniffed data into buffer (also clear interrupt */
		rbuf_write(&sniff_buffer, byte);
	}
}

/*------------------------------------------------------------------------------
 *         Global functions
 *------------------------------------------------------------------------------*/

void Sniffer_usart1_irq(void)
{
	if (ID_USART1==sniff_usart.id) {
		Sniffer_usart_irq();
	}
}

void Sniffer_usart0_irq(void)
{
	if (ID_USART0==sniff_usart.id) {
		Sniffer_usart_irq();
	}
}

/*-----------------------------------------------------------------------------
 *          Initialization routine
 *-----------------------------------------------------------------------------*/

/* Called during USB enumeration after device is enumerated by host */
void Sniffer_configure(void)
{
	TRACE_INFO("Sniffer config\n\r");
}

/* called when *different* configuration is set by host */
void Sniffer_exit(void)
{
	TRACE_INFO("Sniffer exit\n\r");
	USART_DisableIt(sniff_usart.base, US_IER_RXRDY);
	/* NOTE: don't forget to set the IRQ according to the USART peripheral used */
	NVIC_DisableIRQ(USART0_IRQn);
	USART_SetReceiverEnabled(sniff_usart.base, 0);
}

/* called when *Sniffer* configuration is set by host */
void Sniffer_init(void)
{
	TRACE_INFO("Sniffer Init\n\r");

	/* Configure pins to sniff communication between phone and card */
	PIO_Configure(pins_sniff, PIO_LISTSIZE(pins_sniff));
	/* Configure pins to connect phone to card */
	PIO_Configure(pins_bus, PIO_LISTSIZE(pins_bus));
	/* Configure pins to forward phone power to card */
	PIO_Configure(pins_power, PIO_LISTSIZE(pins_power));

	/* Clear ring buffer containing the sniffed data */
	rbuf_reset(&sniff_buffer);
	/* Configure USART to as ISO-7816 slave communication to sniff communication */
	ISO7816_Init(&sniff_usart, CLK_SLAVE);
	/* Only receive data when sniffing */
	USART_SetReceiverEnabled(sniff_usart.base, 1);
	/* Enable interrupt to indicate when data has been received */
	USART_EnableIt(sniff_usart.base, US_IER_RXRDY);
	/* Enable interrupt requests for the USART peripheral */
	NVIC_EnableIRQ(IRQ_USART_SIM);

	/* TODO configure RST pin ISR */
}

/* main (idle/busy) loop of this USB configuration */
void Sniffer_run(void)
{
	check_sniffed_data();
}
#endif /* HAVE_SNIFFER */
