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

/* WTX (Wait time extension):
*   R-block PCB begins with (msb) 10 , ends with 000011 for WTX req, 100011 for WTX resp
*
* The standard says:
*   Rule 3 — If the card requires more than BWT to process the previously received I-block, it transmits S(WTX
*   request) where INF conveys one byte encoding an integer multiplier of the BWT value. The interface device
*   shall acknowledge by S(WTX response) with the same INF.
*   The time allocated starts at the leading edge of the last character of S(WTX response).
*/
// FIXME: Two times the same name for the define, which one is right?
//#define     WTX_req     0b10000011
//#define     WTX_req     0b10100011
// Alternatively:
/* For T = 0 Protocol: The firmware on receiving the NULL (0x60) Procedure byte from the card, notifies
it to the driver using the RDR_to_PC_DataBlock response. During this period, the reception of bytes
from the smart card is still in progress and hence the device cannot indefinitely wait for IN tokens on
the USB bulk-in endpoint. Hence, it is required of the driver to readily supply ‘IN’ tokens on the USB
bulk-in endpoint. On failure to do so, some of the wait time extension responses, will not be queued to
the driver.
*/
extern volatile uint8_t timeout_occured;

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
/** USB state: suspend, resume, idle */
unsigned char USBState = STATE_IDLE;

/** ISO7816 pins */
static const Pin pinsISO7816_PHONE[]    = {PINS_ISO7816_PHONE};
/** Bus switch pins */

#if DEBUG_PHONE_SNIFF
# warning "Debug phone sniff via logic analyzer is enabled"
// Logic analyzer probes are easier to attach to the SIM card slot
static const Pin pins_bus[]    = {PINS_BUS_SNIFF};
#else
static const Pin pins_bus[]    = {PINS_BUS_DEFAULT};
#endif

/** ISO7816 RST pin */
static const Pin pinIso7816RstMC  = PIN_ISO7816_RST_PHONE;
static uint8_t sim_inserted = 0;

static const Pin pPwr[] = {
    /* Enable power converter 4.5-6V to 3.3V; low: off */
    {SIM_PWEN, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT},
    
    /* Enable second power converter: VCC_PHONE to VCC_SIM; high: off */
    {VCC_FWD, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
};


static const Pin pinPhoneRST = PIN_ISO7816_RST_PHONE;

#define PR  TRACE_INFO

/* ===================================================*/
/*                      Taken from iso7816_4.c        */
/* ===================================================*/
/** Flip flop for send and receive char */
#define USART_SEND 0
#define USART_RCV  1

// FIXME: Comments
/*-----------------------------------------------------------------------------
 *          Internal variables
 *-----------------------------------------------------------------------------*/
/** Variable for state of send and receive froom USART */
static uint8_t StateUsartGlobal = USART_RCV;

static uint8_t host_to_sim_buf[BUFLEN];

/*-----------------------------------------------------------------------------
 *          Interrupt routines
 *-----------------------------------------------------------------------------*/
static void ISR_PhoneRST( const Pin *pPin)
{
    int ret;
    // FIXME: no printfs in ISRs?
    printf("+++ Int!! %x\n\r", pinPhoneRST.pio->PIO_ISR);
    if ( ((pinPhoneRST.pio->PIO_ISR & pinPhoneRST.mask) != 0)  )
    {
        if(PIO_Get( &pinPhoneRST ) == 0) {
            printf(" 0 ");
        } else {
            printf(" 1 ");
        }
    }

    if ((ret = USBD_Write( PHONE_INT, "R", 1, 0, 0 )) != USBD_STATUS_SUCCESS) {
        TRACE_ERROR("USB err status: %d (%s)\n", ret, __FUNCTION__);
        return;
    }

    /* Interrupt enabled after ATR is sent to phone */
   // PIO_DisableIt( &pinPhoneRST ) ;
}

/**
 * Get a character from ISO7816
 * \param pCharToReceive Pointer for store the received char
 * \return 0: if timeout else status of US_CSR
 */
/* FIXME: This code is taken from cciddriver.c
        --> Reuse the code!!! */
uint32_t _ISO7816_GetChar( uint8_t *pCharToReceive )
{
    uint32_t status;
    uint32_t timeout=0;

    TRACE_DEBUG("--");

    if( StateUsartGlobal == USART_SEND ) {
        while((USART_PHONE->US_CSR & US_CSR_TXEMPTY) == 0) {}
        USART_PHONE->US_CR = US_CR_RSTSTA | US_CR_RSTIT | US_CR_RSTNACK;
        StateUsartGlobal = USART_RCV;
    }

    /* Wait USART ready for reception */
    while( ((USART_PHONE->US_CSR & US_CSR_RXRDY) == 0) ) {
        if(timeout++ > 12000 * (BOARD_MCK/1000000)) {
            TRACE_DEBUG("TimeOut\n\r");
            return( 0 );
        }
    }

    /* At least one complete character has been received and US_RHR has not yet been read. */

    /* Get a char */
    *pCharToReceive = ((USART_PHONE->US_RHR) & 0xFF);

    status = (USART_PHONE->US_CSR&(US_CSR_OVRE|US_CSR_FRAME|
                                      US_CSR_PARE|US_CSR_TIMEOUT|US_CSR_NACK|
                                      (1<<10)));

    if (status != 0 ) {
        TRACE_DEBUG("R:0x%X\n\r", status); 
        TRACE_DEBUG("R:0x%X\n\r", USART_PHONE->US_CSR);
        TRACE_DEBUG("Nb:0x%X\n\r", USART_PHONE->US_NER );
        USART_PHONE->US_CR = US_CR_RSTSTA;
    }

    /* Return status */
    return( status );
}

/**
 * Send a char to ISO7816
 * \param CharToSend char to be send
 * \return status of US_CSR
 */
uint32_t _ISO7816_SendChar( uint8_t CharToSend )
{
    uint32_t status;

    TRACE_DEBUG("********** Send char: %c (0x%X)\n\r", CharToSend, CharToSend);

    if( StateUsartGlobal == USART_RCV ) {
        USART_PHONE->US_CR = US_CR_RSTSTA | US_CR_RSTIT | US_CR_RSTNACK;
        StateUsartGlobal = USART_SEND;
    }

    /* Wait USART ready for transmit */
    while((USART_PHONE->US_CSR & US_CSR_TXRDY) == 0)  {}
    /* There is no character in the US_THR */

    /* Transmit a char */
    USART_PHONE->US_THR = CharToSend;

    status = (USART_PHONE->US_CSR&(US_CSR_OVRE|US_CSR_FRAME|
                                      US_CSR_PARE|US_CSR_TIMEOUT|US_CSR_NACK|
                                      (1<<10)));

    if (status != 0 ) {
        TRACE_DEBUG("******* status: 0x%X (Overrun: %lu, NACK: %lu, Timeout: %lu, underrun: %lu)\n\r",
                    status, ((status & US_CSR_OVRE)>> 5), ((status & US_CSR_NACK) >> 13),
                    ((status & US_CSR_TIMEOUT) >> 8), ((status & (1 << 10)) >> 10));

        TRACE_DEBUG("E (USART CSR reg):0x%X\n\r", USART_PHONE->US_CSR);
        TRACE_DEBUG("Nb (Number of errors):0x%X\n\r", USART_PHONE->US_NER );
        USART_PHONE->US_CR = US_CR_RSTSTA;
    }

    /* Return status */
    return( status );
}

void receive_from_host( void );
void sendResponse_to_phone( uint8_t *pArg, uint8_t status, uint32_t transferred, uint32_t remaining)
{
    if (status != USBD_STATUS_SUCCESS) {
        TRACE_ERROR("USB err status: %d (%s)\n", __FUNCTION__, status);
        return;
    }
    PR("sendResp, stat: %X, trnsf: %x, rem: %x\n\r", status, transferred, remaining);
    PR("Resp: %x %x %x .. %x\n", host_to_sim_buf[0], host_to_sim_buf[1], host_to_sim_buf[2], host_to_sim_buf[transferred-1]);

    for (uint32_t i = 0; i < transferred; i++ ) {
        _ISO7816_SendChar(host_to_sim_buf[i]);
    }

    receive_from_host();
}

void receive_from_host()
{
    int ret;
    if ((ret = USBD_Read(PHONE_DATAOUT, &host_to_sim_buf, sizeof(host_to_sim_buf),
                (TransferCallback)&sendResponse_to_phone, 0)) == USBD_STATUS_SUCCESS) {
    } else {
        TRACE_ERROR("USB Err: %X\n", ret);
    }
}
void Phone_configure( void ) {
    PIO_ConfigureIt( &pinPhoneRST, ISR_PhoneRST ) ;
    NVIC_EnableIRQ( PIOA_IRQn );
}

void Phone_exit( void ) {
    PIO_DisableIt( &pinPhoneRST ) ;
    USART_DisableIt( USART_PHONE, US_IER_RXRDY) ;
    USART_SetTransmitterEnabled(USART_PHONE, 0);
    USART_SetReceiverEnabled(USART_PHONE, 0);
}

void Phone_init( void ) {
    PIO_Configure( pinsISO7816_PHONE, PIO_LISTSIZE( pinsISO7816_PHONE ) ) ;
    PIO_Configure( pins_bus, PIO_LISTSIZE( pins_bus) ) ;

    PIO_Configure( &pinPhoneRST, 1);

    PIO_EnableIt( &pinPhoneRST ) ;
    _ISO7816_Init();

    USART_SetTransmitterEnabled(USART_PHONE, 1);
    USART_SetReceiverEnabled(USART_PHONE, 1);

    /*  Configure ISO7816 driver */
    // FIXME:    PIO_Configure(pPwr, PIO_LISTSIZE( pPwr ));

// FIXME: Or do I need to call VBUS_CONFIGURE() here instead, which will call USBD_Connect() later?
//    USBD_Connect();

    USART_EnableIt( USART_PHONE, US_IER_RXRDY) ;

    //Timer_Init();

    receive_from_host();
}


// Sniffed Phone to SIM card communication:
// phone > sim : RST
// phone < sim : ATR
// phone > sim : A0 A4 00 00 02 (Select File)
// phone < sim : A4 (INS repeated)
// phone > sim : 7F 02 (= ??)
// phone < sim : 9F 16 (9F: success, can deliver 0x16 (=22) byte)
// phone > sim : ?? (A0 C0 00 00 16)
// phone < sim : C0 (INS repeated)
// phone < sim : 00 00 00 00   7F 20 02 00   00 00 00 00   09 91 00 17   04 00 83 8A (data of length 22 -2)
// phone <? sim : 90 00 (OK, everything went fine)
// phone ? sim : 00 (??)

void Phone_run( void )
{
    check_data_from_phone();
}
