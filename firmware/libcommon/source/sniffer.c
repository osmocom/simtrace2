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

/** Maximum ATR ucSize in bytes.*/
#define MAX_ATR_SIZE            55

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
/* Pin configuration to sniff communication (using USART connection to SIM card) */
static const Pin pins_sniff[] = { PINS_SIM_SNIFF };
/* Connect phone to card using bus switch */
static const Pin pins_bus[] = { PINS_BUS_SNIFF };
/* Power card using phone VCC */
static const Pin pins_power[] = { PWR_PINS };
/* Timer Counter pins to measure ETU timing */
static const Pin pins_tc[] = { PINS_TC };
/* USART peripheral used to sniff communication */
static struct Usart_info sniff_usart = {
	.base = USART_SIM,
	.id = ID_USART_SIM,
	.state = USART_RCV,
};
/* Ring buffer to store sniffer communication data */
static struct ringbuf sniff_buffer;

/*------------------------------------------------------------------------------
 *         Global functions
 *------------------------------------------------------------------------------*/

void Sniffer_usart0_irq(void)
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
	/* Enable interrupt requests for the USART peripheral (warning: use IRQ corresponding to USART) */
	NVIC_EnableIRQ(USART0_IRQn);
}

/* main (idle/busy) loop of this USB configuration */
void Sniffer_run(void)
{
	check_sniffed_data();
}
#endif /* HAVE_SNIFFER */
