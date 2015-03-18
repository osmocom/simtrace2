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
#define     WTX_req     0b10000011
#define     WTX_req     0b10100011
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
/** ISO7816 RST pin */
static const Pin pinIso7816RstMC  = PIN_ISO7816_RST_PHONE;
static uint8_t sim_inserted = 0;

static const Pin pPwr[] = {
    /* Enable power converter 4.5-6V to 3.3V; low: off */
    {SIM_PWEN, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT},
    
    /* Enable second power converter: VCC_PHONE to VCC_SIM; high: off */
    {VCC_FWD, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
};


#define ISO7816_PHONE_RST   {PIO_PA24, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP | PIO_DEBOUNCE | PIO_DEGLITCH | PIO_IT_EDGE }
static const Pin pinPhoneRST = ISO7816_PHONE_RST;
//#define ISO7816_PHONE_CLK   {PIO_PA23A_SCK1, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP | PIO_DEBOUNCE | PIO_DEGLITCH | PIO_IT_EDGE }
//static const Pin pinPhoneClk = ISO7816_PHONE_CLK;

/* ===================================================*/
/*                      Taken from iso7816_4.c        */
/* ===================================================*/
/** Flip flop for send and receive char */
#define USART_SEND 0
#define USART_RCV  1

#define NONE            9
#define RST_RCVD        10
#define WAIT_CMD_PHONE  11
#define WAIT_CMD_PC     12
#define WAIT_ATR        13
/*-----------------------------------------------------------------------------
 *          Internal variables
 *-----------------------------------------------------------------------------*/
/** Variable for state of send and receive froom USART */
static uint8_t StateUsartGlobal = USART_RCV;

static uint32_t state;
extern uint8_t rcvdChar;

/*-----------------------------------------------------------------------------
 *          Interrupt routines
 *-----------------------------------------------------------------------------*/
#define     RESET   'R'
static void ISR_PhoneRST( const Pin *pPin)
{
    printf("+++ Int!!\n\r");
    if (state == NONE) {
        state = RST_RCVD;
    }
    // FIXME: What to do on reset?
    // FIXME: It seems like the phone is constantly sending a lot of these RSTs
//    PIO_DisableIt( &pinPhoneRST ) ;
}

static void Config_PhoneRST_IrqHandler()
{
    PIO_Configure( &pinPhoneRST, 1);
//    PIO_Configure( &pinPhoneClk, 1);
    PIO_ConfigureIt( &pinPhoneRST, ISR_PhoneRST ) ;
//    PIO_ConfigureIt( &pinPhoneClk, ISR_PhoneRST ) ;
    PIO_EnableIt( &pinPhoneRST ) ;
//    PIO_EnableIt( &pinPhoneClk ) ;
    NVIC_EnableIRQ( PIOA_IRQn );
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

void Phone_Master_Init( void ) {
    
    PIO_Configure( pinsISO7816_PHONE, PIO_LISTSIZE( pinsISO7816_PHONE ) ) ;
    PIO_Configure( pins_bus, PIO_LISTSIZE( pins_bus) ) ;

    Config_PhoneRST_IrqHandler();

    _ISO7816_Init();

    USART_SetTransmitterEnabled(USART_PHONE, 1);
    USART_SetReceiverEnabled(USART_PHONE, 1);

    /*  Configure ISO7816 driver */
    // FIXME:    PIO_Configure(pPwr, PIO_LISTSIZE( pPwr ));

    state = NONE;


// FIXME: Or do I need to call VBUS_CONFIGURE() here instead, which will call USBD_Connect() later?
//    USBD_Connect();
// FIXME: USB clock? USB PMC?
//    NVIC_EnableIRQ( UDP_IRQn );

   USART_EnableIt( USART_PHONE, US_IER_RXRDY) ;

   // FIXME: At some point USBD_IrqHandler() should get called and set USBD_STATE_CONFIGURED
  /*  while (USBD_GetState() < USBD_STATE_CONFIGURED) {
        int i = 1; 
        if ((i%10000) == 0) {
            TRACE_DEBUG("%d: USB State: %x\n\r", i, USBD_GetState());
        }
        i++;
    }
*/

}

void send_ATR(uint8_t *ATR, uint8_t status, uint32_t transferred, uint32_t remaining)
{
    int i;
    TRACE_INFO("Send %x %x %x.. %x", ATR[0], ATR[1], ATR[2], ATR[transferred-1]);
    for ( i = 0; i < transferred; i++ ) {
        _ISO7816_SendChar(*(ATR++));
    }
    state = WAIT_CMD_PHONE;
}

void sendResponse( uint8_t *pArg, uint8_t status, uint32_t transferred, uint32_t remaining)
{
    int i;
    TRACE_INFO("sendResponse, stat: %X, transf: %x, remain: %x", status, transferred, remaining);
    TRACE_INFO("Resp: %x %x %x .. %x", pArg[0], pArg[1], pArg[2], pArg[transferred-1]);

    for ( i = 0; i < transferred; i++ ) {
        _ISO7816_SendChar(*(pArg++));
    }
    state = WAIT_CMD_PHONE;
}

extern ring_buffer buf;
#define     MAX_MSG_LEN     64
#define     PR              printf

void wait_for_response(uint8_t pBuffer[]) {
    int ret = 0;
//    uint8_t msg[] = {0xa0, 0xa4, 0x0, 0x0, 0x2};
    if (rcvdChar != 0) {
        printf(" rr ");
        /*  DATA_IN for host side is data_out for simtrace side   */
        /* FIXME: Performancewise sending a USB packet for every byte is a disaster */
        PR("b:%x %x %x %x %x.\n\r", buf.buf[0], buf.buf[1],buf.buf[2], buf.buf[3], buf.buf[4]);
        USBD_Write( DATAIN, buf.buf, BUFLEN, 0, 0 );
        //USBD_Write( DATAIN, msg, BUFLEN, 0, 0 );

        rcvdChar = 0;

        if ((ret = USBD_Read(DATAOUT, pBuffer, MAX_MSG_LEN, (TransferCallback)&sendResponse, pBuffer)) == USBD_STATUS_SUCCESS) {
            TRACE_INFO("Reading started sucessfully (wait_resp)");
            state = WAIT_CMD_PC;
        } else {
            TRACE_INFO("USB Error: %X", ret);
        }
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
            USBD_Write( INT, &msg, 1, 0, 0 );
            // send_ATR sets state to WAIT_CMD
            if ((ret = USBD_Read(DATAOUT, pBuffer, MAX_MSG_LEN, (TransferCallback)&send_ATR, pBuffer)) == USBD_STATUS_SUCCESS) {
                TRACE_INFO("Reading started sucessfully (ATR)");
                state = WAIT_ATR;
            } else {
                TRACE_INFO("USB Error: %X", ret);
//FIXME:                 state = ERR;
            }
            break;
        case WAIT_CMD_PHONE:
            wait_for_response(pBuffer);
            break;
        default:
            break;
    }

    // FIXME: Function Phone_run not implemented yet

    /* Send and receive chars */
    // ISO7816_GetChar(&rcv_char);
    // ISO7816_SendChar(char_to_send);
}
