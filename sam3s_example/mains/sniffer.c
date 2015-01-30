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

/* Sniffer configuration */
    
#ifdef PIN_SC_SW
#undef PIN_SC_SW
#endif
#define PIN_SC_SW               {PIO_PA20, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}

#ifdef PIN_IO_SW
#undef PIN_IO_SW
#endif
#define PIN_IO_SW               {PIO_PA19, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}

#define PINS_BUS                PIN_SC_SW, PIN_IO_SW

#define PINS_SIM_SNIFF_SIM      PIN_PHONE_IO,  PIN_PHONE_CLK

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
/** USB state: suspend, resume, idle */
unsigned char USBState = STATE_IDLE;

/** ISO7816 pins */
static const Pin pinsISO7816_sniff[]    = {PINS_SIM_SNIFF_SIM};
static const Pin pins_bus[]    = {PINS_BUS};
static const Pin pPwr[] = {
    /* Enable power converter 4.5-6V to 3.3V; low: off */
    {SIM_PWEN, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT},
    
    /* Enable second power converter: VCC_PHONE to VCC_SIM; high: off */
    {VCC_FWD, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
};

uint32_t char_stat = 0;
uint8_t rcvdChar = 0;

/*-----------------------------------------------------------------------------
 *          Interrupt routines
 *-----------------------------------------------------------------------------*/
/* 
 *  char_stat is zero if no error occured. 
 *  Otherwise it is filled with the content of the status register.
 */
void USART1_IrqHandler( void )
{
    uint32_t stat;
    char_stat = 0;
    // Rcv buf full
/*    if((stat & US_CSR_RXBUFF) == US_CSR_RXBUFF) {
        TRACE_DEBUG("Rcv buf full");
        USART_DisableIt(USART1, US_IDR_RXBUFF);
    }
*/
    uint32_t csr = USART_PHONE->US_CSR;

    if (csr & US_CSR_TXRDY) {
        /* transmit buffer empty, nothing to transmit */
    } 
    if (csr & US_CSR_RXRDY) {
        stat = (csr&(US_CSR_OVRE|US_CSR_FRAME|
                        US_CSR_PARE|US_CSR_TIMEOUT|US_CSR_NACK|
                        (1<<10)));

        if (stat == 0 ) {
            /* Get a char */
            rcvdChar = ((USART_PHONE->US_RHR) & 0xFF);
        } /* else: error occured */
        char_stat = stat;
    }
}

/** Initializes a ISO driver
 *  \param pPinIso7816RstMC Pin ISO 7816 Rst MC
 */
void _ISO7816_Init( )
{
    TRACE_DEBUG("ISO_Init\n\r");

    USART_Configure( USART_PHONE,
                     US_MR_USART_MODE_IS07816_T_0
// Nope, we aren't master:   
 //                    | US_MR_USCLKS_MCK
                     | US_MR_USCLKS_SCK
                     | US_MR_NBSTOP_1_BIT
                     | US_MR_PAR_EVEN
                     | US_MR_CHRL_8_BIT
                     | US_MR_CLKO   /** TODO: This field was set in the original simtrace firmware..why? */
                     | (3<<24), /* MAX_ITERATION */
                     1,
                     0);
    /* 
    SYNC = 0 (async mode)
    OVER = 0 (oversampling by 8?)
    FIDI = 372 (default val on startup before other value is negotiated)
    USCLKS = 3 (Select SCK as input clock) --> US_MR_USCLKS_SCK
    CD = 1 ?    --> US_BRGR_CD(1)
    */
    USART_PHONE->US_FIDI = 372; 
    USART_PHONE->US_BRGR = US_BRGR_CD(1);
//    USART_PHONE->US_BRGR = BOARD_MCK / (372*9600);
    USART_PHONE->US_TTGR = 5;

    /* Configure USART */
    PMC_EnablePeripheral(ID_USART_PHONE);

    USART1->US_IDR = 0xffffffff;
    USART_EnableIt( USART1, US_IER_RXRDY) ;
    /* enable USART1 interrupt */
    NVIC_EnableIRQ( USART1_IRQn ) ;
    
//    USART_PHONE->US_IER = US_IER_RXRDY | US_IER_OVRE | US_IER_FRAME | US_IER_PARE | US_IER_NACK | US_IER_ITER;

    USART_SetReceiverEnabled(USART_PHONE, 1);

}

/*------------------------------------------------------------------------------
 *        Main 
 *------------------------------------------------------------------------------*/

extern int main( void )
{
    uint8_t ucSize ;
    static uint8_t buff[100];

    LED_Configure(LED_NUM_GREEN);
    LED_Set(LED_NUM_GREEN);

    /* Disable watchdog*/
    WDT_Disable( WDT ) ;

    PIO_InitializeInterrupts(0);    

    /*  Configure ISO7816 driver */
    PIO_Configure( pinsISO7816_sniff, PIO_LISTSIZE( pinsISO7816_sniff ) ) ;
    PIO_Configure( pins_bus, PIO_LISTSIZE( pins_bus) ) ;
    PIO_Configure(pPwr, PIO_LISTSIZE( pPwr ));

    _ISO7816_Init( );

    printf("***** START \n\r");
    while (1) {
//        printf(".\n\r");

        if (rcvdChar != 0) {
            printf("Received _%x_ \n\r", rcvdChar);
            rcvdChar = 0;
        }
    }
    return 0 ;
}
