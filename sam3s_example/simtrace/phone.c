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

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
/** USB state: suspend, resume, idle */
unsigned char USBState = STATE_IDLE;

/** ISO7816 pins */
static const Pin pinsISO7816_PHONE[]    = {PINS_ISO7816_PHONE};
/** Bus switch pins */
static const Pin pinsBus[]    = {PINS_BUS_DEFAULT};
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
/*-----------------------------------------------------------------------------
 *          Internal variables
 *-----------------------------------------------------------------------------*/
/** Variable for state of send and receive froom USART */
static uint8_t StateUsartGlobal = USART_RCV;

extern uint32_t char_stat;
extern uint8_t rcvdChar;

/*-----------------------------------------------------------------------------
 *          Interrupt routines
 *-----------------------------------------------------------------------------*/

static void ISR_PhoneRST( const Pin *pPin)
{
    TRACE_DEBUG("+++++++++ Interrupt!! ISR:0x%x, CSR:0x%x\n\r", pinPhoneRST.pio->PIO_ISR, USART1->US_CSR);
    // FIXME: What to do on reset?
    // PIO_DisableIt( &pinPhoneRST ) ;
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




// FIXME: Or do I need to call VBUS_CONFIGURE() here instead, which will call USBD_Connect() later?
//    USBD_Connect();
// FIXME: USB clock? USB PMC?
//    NVIC_EnableIRQ( UDP_IRQn );

 //   USART_EnableIt( USART_PHONE, US_IER_RXRDY) ;

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

void Phone_run( void )
{
    // FIXME: Function Phone_run not implemented yet

    /* Send and receive chars */
    // ISO7816_GetChar(&rcv_char);
    // ISO7816_SendChar(char_to_send);
}
