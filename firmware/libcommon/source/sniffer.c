/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support
 * ----------------------------------------------------------------------------
 * Copyright (c) 2009, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
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
/** ISO7816 pins */
static const Pin pinsISO7816_sniff[] = { PINS_SIM_SNIFF_SIM };
static const Pin pins_bus[] = { PINS_BUS_SNIFF };

static const Pin pPwr[] = {
	/* Enable power converter 4.5-6V to 3.3V; low: off */
	{SIM_PWEN, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT},

	/* Enable second power converter: VCC_PHONE to VCC_SIM; high: on */
	{VCC_FWD, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
};

static struct Usart_info usart_info = {
	.base = USART_PHONE,
	.id = ID_USART_PHONE,
	.state = USART_RCV,
};


int check_data_from_phone(void)
{
	TRACE_INFO("check data from phone\n\r");
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
	USART_DisableIt(USART_PHONE, US_IER_RXRDY);
	NVIC_DisableIRQ(USART1_IRQn);
	USART_SetReceiverEnabled(USART_PHONE, 0);
}

/* called when *Sniffer* configuration is set by host */
void Sniffer_init(void)
{
	TRACE_INFO("Sniffer Init\n\r");
	/*  Configure ISO7816 driver */
	PIO_Configure(pinsISO7816_sniff, PIO_LISTSIZE(pinsISO7816_sniff));
	PIO_Configure(pins_bus, PIO_LISTSIZE(pins_bus));

	PIO_Configure(pPwr, PIO_LISTSIZE(pPwr));

	ISO7816_Init(&usart_info, CLK_SLAVE);

	USART_SetReceiverEnabled(USART_PHONE, 1);
	USART_EnableIt(USART_PHONE, US_IER_RXRDY);
	NVIC_EnableIRQ(USART1_IRQn);
}

/* main (idle/busy) loop of this USB configuration */
void Sniffer_run(void)
{
	check_data_from_phone();
}
#endif /* HAVE_SNIFFER */
