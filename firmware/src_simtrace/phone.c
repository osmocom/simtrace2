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

/*------------------------------------------------------------------------------
 *          Headers
 *------------------------------------------------------------------------------*/

#include "board.h"

#include <string.h>

/*------------------------------------------------------------------------------
 *         Internal definitions
 *------------------------------------------------------------------------------*/

/** Maximum ucSize in bytes of the smartcard answer to a command.*/
#define MAX_ANSWER_SIZE         10

/** Maximum ATR ucSize in bytes.*/
#define MAX_ATR_SIZE            55

/** USB states */
/// Use for power management
#define STATE_IDLE    0
/// The USB device is in suspend state
#define STATE_SUSPEND 4
/// The USB device is in resume state
#define STATE_RESUME  5

extern volatile uint8_t timeout_occured;

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
/** USB state: suspend, resume, idle */
unsigned char USBState = STATE_IDLE;

/** ISO7816 pins */
static const Pin pinsISO7816_PHONE[] = { PINS_ISO7816_PHONE };

/** Bus switch pins */

#if DEBUG_PHONE_SNIFF
#warning "Debug phone sniff via logic analyzer is enabled"
// Logic analyzer probes are easier to attach to the SIM card slot
static const Pin pins_bus[] = { PINS_BUS_SNIFF };
#else
static const Pin pins_bus[] = { PINS_BUS_DEFAULT };
#endif

/** ISO7816 RST pin */
static uint8_t sim_inserted = 0;

static const Pin pPwr[] = {
	/* Enable power converter 4.5-6V to 3.3V; low: off */
	{SIM_PWEN, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT},

	/* Enable second power converter: VCC_PHONE to VCC_SIM; high: off */
	{VCC_FWD, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
};

const Pin pinPhoneRST = PIN_ISO7816_RST_PHONE;

static struct Usart_info usart_info = {
	.base = USART_PHONE,
	.id = ID_USART_PHONE,
	.state = USART_RCV,
};

/* ===================================================*/
/*                      Taken from iso7816_4.c        */
/* ===================================================*/
/** Flip flop for send and receive char */
#define USART_SEND 0
#define USART_RCV  1

/*-----------------------------------------------------------------------------
 *          Internal variables
 *-----------------------------------------------------------------------------*/
static uint8_t host_to_sim_buf[BUFLEN];
static bool change_fidi = false;

static void receive_from_host(void);
static void sendResponse_to_phone(uint8_t * pArg, uint8_t status,
				  uint32_t transferred, uint32_t remaining)
{
	if (status != USBD_STATUS_SUCCESS) {
		TRACE_ERROR("USB err status: %d (%s)\n", __FUNCTION__, status);
		return;
	}
	TRACE_DEBUG("sendResp, stat: %X, trnsf: %x, rem: %x\n\r", status,
		    transferred, remaining);
	TRACE_DEBUG("Resp: %x %x %x .. %x\n", host_to_sim_buf[0],
		    host_to_sim_buf[1], host_to_sim_buf[2],
		    host_to_sim_buf[transferred - 1]);

	USART_SetReceiverEnabled(USART_PHONE, 0);
	USART_SetTransmitterEnabled(USART_PHONE, 1);
	uint32_t i = 0;
	if (host_to_sim_buf[0] == 0xff) {
		printf("Change FIDI detected\n");
		// PTS command, change FIDI after command
		i = 2;
		change_fidi = true;
	}
	for (; i < transferred; i++) {
		ISO7816_SendChar(host_to_sim_buf[i], &usart_info);
	}
	USART_SetTransmitterEnabled(USART_PHONE, 0);
	USART_SetReceiverEnabled(USART_PHONE, 1);

	if (change_fidi == true) {
		printf("Change FIDI: %x\n", host_to_sim_buf[2]);
		update_fidi(host_to_sim_buf[2]);
		change_fidi = false;
	}

	receive_from_host();
}

static void receive_from_host()
{
	int ret;
	if ((ret = USBD_Read(PHONE_DATAOUT, &host_to_sim_buf,
			     sizeof(host_to_sim_buf),
			     (TransferCallback) &sendResponse_to_phone,
			     0)) == USBD_STATUS_SUCCESS) {
	} else {
		TRACE_ERROR("USB Err: %X\n", ret);
	}
}

void Phone_configure(void)
{
	PIO_ConfigureIt(&pinPhoneRST, ISR_PhoneRST);
	NVIC_EnableIRQ(PIOA_IRQn);
}

void Phone_exit(void)
{
	PIO_DisableIt(&pinPhoneRST);
	NVIC_DisableIRQ(USART1_IRQn);
	USART_DisableIt(USART_PHONE, US_IER_RXRDY);
	USART_SetTransmitterEnabled(USART_PHONE, 0);
	USART_SetReceiverEnabled(USART_PHONE, 0);
}

void Phone_init(void)
{
	PIO_Configure(pinsISO7816_PHONE, PIO_LISTSIZE(pinsISO7816_PHONE));
	PIO_Configure(pins_bus, PIO_LISTSIZE(pins_bus));

	PIO_Configure(&pinPhoneRST, 1);

	PIO_EnableIt(&pinPhoneRST);
	ISO7816_Init(&usart_info, CLK_SLAVE);

	USART_SetTransmitterEnabled(USART_PHONE, 0);
	USART_SetReceiverEnabled(USART_PHONE, 1);

	USART_EnableIt(USART_PHONE, US_IER_RXRDY);
	NVIC_EnableIRQ(USART1_IRQn);

	receive_from_host();
}

void Phone_run(void)
{
	check_data_from_phone();
}
