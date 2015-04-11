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

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
/** USB state: suspend, resume, idle */
unsigned char USBState = STATE_IDLE;

/** ISO7816 pins */
static const Pin pinsISO7816_PHONE[]    = {PINS_ISO7816_PHONE};
/** Bus switch pins */
static const Pin pins_bus[]    = {PINS_BUS_DEFAULT};
// FIXME: temporary enable bus switch 
//static const Pin pins_bus[]    = {PINS_BUS_SNIFF};

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

enum states{
    WAIT_FOR_RST            = 9,
    RST_RCVD        = 10,
    WAIT_CMD_PHONE  = 11,
    WAIT_CMD_PC     = 12,
    WAIT_ATR        = 13,
};

/*-----------------------------------------------------------------------------
 *          Internal variables
 *-----------------------------------------------------------------------------*/
/** Variable for state of send and receive froom USART */
static uint8_t StateUsartGlobal = USART_RCV;

static enum states state;

extern volatile uint8_t timeout_occured;

/*-----------------------------------------------------------------------------
 *          Interrupt routines
 *-----------------------------------------------------------------------------*/
#define     RESET   'R'
static void ISR_PhoneRST( const Pin *pPin)
{
    printf("+++ Int!! %x\n\r", pinPhoneRST.pio->PIO_ISR);
    if ( ((pinPhoneRST.pio->PIO_ISR & pinPhoneRST.mask) != 0)  )
    {
        if(PIO_Get( &pinPhoneRST ) == 0) {
            printf(" 0 ");
        } else {
            printf(" 1 ");
        }
    }
    state = RST_RCVD;

    /* Interrupt enabled after ATR is sent to phone */
    PIO_DisableIt( &pinPhoneRST ) ;
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

    state = WAIT_FOR_RST;


// FIXME: Or do I need to call VBUS_CONFIGURE() here instead, which will call USBD_Connect() later?
//    USBD_Connect();

    USART_EnableIt( USART_PHONE, US_IER_RXRDY) ;

    Timer_Init();
}

void send_ATR(uint8_t *ATR, uint8_t status, uint32_t transferred, uint32_t remaining)
{
    uint32_t i;

    if (status != USBD_STATUS_SUCCESS) {
        TRACE_ERROR("USB err status: %d (%s)\n", __FUNCTION__, status);
        return;
    }
    PR("Send %x %x .. %x (tr: %d, st: %x)", ATR[0], ATR[1], ATR[transferred-1], transferred, status);
    for ( i = 0; i < transferred; i++ ) {
        _ISO7816_SendChar(*(ATR++));
    }
    state = WAIT_CMD_PHONE;
    PIO_EnableIt( &pinPhoneRST ) ;
}

void sendResponse( uint8_t *pArg, uint8_t status, uint32_t transferred, uint32_t remaining)
{
    uint32_t i;

    if (status != USBD_STATUS_SUCCESS) {
        TRACE_ERROR("USB err status: %d (%s)\n", __FUNCTION__, status);
        return;
    }
    PR("sendResp, stat: %X, trnsf: %x, rem: %x\n\r", status, transferred, remaining);
    PR("Resp: %x %x %x .. %x", pArg[0], pArg[1], pArg[2], pArg[transferred-1]);

    for ( i = 0; i < transferred; i++ ) {
        _ISO7816_SendChar(*(pArg++));
    }
/*
    if (*(pArg-1) == 0x8A) {
        for (i=0; i<20000; i++) ;
        _ISO7816_SendChar(0x90);
        _ISO7816_SendChar(0x00);
    }
*/
    state = WAIT_CMD_PHONE;
}

#define     MAX_MSG_LEN     64

void wait_for_response(uint8_t pBuffer[]) {
    int ret = 0;
    if (rcvdChar != 0) {
        printf(" rr ");

        /*  DATA_IN for host side is data_out for simtrace side   */
        ret = USBD_Write( PHONE_DATAIN, (void *)buf.buf, BUFLEN, 0, 0 );
        if (ret != USBD_STATUS_SUCCESS) {
            TRACE_ERROR("USB err status: %d (%s)\n", ret, __FUNCTION__);
            return;
        }
        PR("b:%x %x %x %x %x.\n\r", buf.buf[0], buf.buf[1],buf.buf[2], buf.buf[3], buf.buf[4]);

        rcvdChar = 0;
    } else if (timeout_occured && buf.idx != 0) {
        printf(" to ");

        ret = USBD_Write( PHONE_DATAIN, (void *) buf.buf, buf.idx, 0, 0 );
        if (ret != USBD_STATUS_SUCCESS) {
            TRACE_ERROR("USB err status: %d (%s)\n", ret, __FUNCTION__);
            return;
        }

        timeout_occured = 0;
        buf.idx = 0;
        rcvdChar = 0;
        PR("b:%x %x %x %x %x.\n\r", buf.buf[0], buf.buf[1],buf.buf[2], buf.buf[3], buf.buf[4]);
    } else {
        return;
    }
    if ((ret = USBD_Read(PHONE_DATAOUT, pBuffer, MAX_MSG_LEN,
                (TransferCallback)&sendResponse, pBuffer)) == USBD_STATUS_SUCCESS) {
        PR("wait_rsp\n\r");
//        state = WAIT_CMD_PC;
        buf.idx = 0;
        TC0_Counter_Reset();
    } else {
        PR("USB Err: %X", ret);
        return;
    }
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
    int ret;
    uint8_t pBuffer[MAX_MSG_LEN];
    int msg = RESET;
// FIXME: remove:
//    uint8_t ATR[] = {0x3B, 0x9A, 0x94, 0x00, 0x92, 0x02, 0x75, 0x93, 0x11, 0x00, 0x01, 0x02, 0x02, 0x19}; 
//    send_ATR(ATR, (sizeof(ATR)/sizeof(ATR[0])));
    switch (state) {
        case RST_RCVD:
            if ((ret = USBD_Write( PHONE_INT, &msg, 1, 0, 0 )) != USBD_STATUS_SUCCESS) {
                TRACE_ERROR("USB err status: %d (%s)\n", ret, __FUNCTION__);
                return;
            }
            //buf.idx = 0;
            //rcvdChar = 0;
//            TC0_Counter_Reset();
            // send_ATR sets state to WAIT_CMD
            if ((ret = USBD_Read(PHONE_DATAOUT, pBuffer, MAX_MSG_LEN, (TransferCallback)&send_ATR, pBuffer)) == USBD_STATUS_SUCCESS) {
                PR("Reading started sucessfully (ATR)");
                state = WAIT_ATR;
            } else {
                TRACE_ERROR("USB err status: %d (%s)\n", ret, __FUNCTION__);
                return;
            }
            break;
        case WAIT_CMD_PHONE:
// FIXME:            TC0_Counter_Reset();
            wait_for_response(pBuffer);
            break;
        case WAIT_FOR_RST:
            break;
        default:
//            PR(":(");
            break;
    }
}
