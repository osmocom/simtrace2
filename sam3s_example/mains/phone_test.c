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
/** ISO7816 RST pin */
static const Pin pinIso7816RstMC  = PIN_ISO7816_RST_PHONE;
static uint8_t sim_inserted = 0;

static const Pin pPwr[] = {
    /* Enable power converter 4.5-6V to 3.3V; low: off */
    {SIM_PWEN, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT},
    
    /* Enable second power converter: VCC_PHONE to VCC_SIM; high: off */
    {VCC_FWD, PIOA, ID_PIOA, PIO_OUTPUT_1, PIO_DEFAULT}
};

// SuperSIM ATR: 3B 9A 94 00 92 02 75 93 11 00 01 02 02 19
static const ATR[] = {0x3B, 0x9A, 0x94, 0x00, 0x92, 0x02, 0x75, 0x93, 0x11, 0x00, 0x01, 0x02, 0x02, 0x19};
#define NONE        0
#define SEND_ATR    1
#define AFTER_ATR   2
static uint8_t phone_state = NONE;


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
/** Pin reset master card */
static Pin st_pinIso7816RstMC;

static uint8_t char_avail = 0;
 

/*-----------------------------------------------------------------------------
 *          Interrupt routines
 *-----------------------------------------------------------------------------*/
void USART1_IrqHandler( void )
{
    uint32_t stat;
    
    stat = USART1->US_CSR;    
    TRACE_DEBUG("stat: 0x%x, imask: 0x%x", stat, USART1->US_IMR);

    // Rcv buf full
/*    if((stat & US_CSR_RXBUFF) == US_CSR_RXBUFF) {
        TRACE_DEBUG("Rcv buf full");
        USART_DisableIt(USART1, US_IDR_RXBUFF);
    }
*/
    char_avail = 1; 
}

static void ISR_PhoneRST( const Pin *pPin)
{
    TRACE_DEBUG("+++++++++ Interrupt!! ISR:0x%x, CSR:0x%x\n\r", pinPhoneRST.pio->PIO_ISR, USART1->US_CSR);
    phone_state = SEND_ATR;
}

static void Config_PhoneRST_IrqHandler()
{
    PIO_Configure( &pinPhoneRST, 1);
//    PIO_Configure( &pinPhoneClk, 1);
    NVIC_EnableIRQ( PIOA_IRQn );
    PIO_ConfigureIt( &pinPhoneRST, ISR_PhoneRST ) ;
//    PIO_ConfigureIt( &pinPhoneClk, ISR_PhoneRST ) ;
    PIO_EnableIt( &pinPhoneRST ) ;
//    PIO_EnableIt( &pinPhoneClk ) ;
}

/**
 * Get a character from ISO7816
 * \param pCharToReceive Pointer for store the received char
 * \return 0: if timeout else status of US_CSR
 */
static uint32_t ISO7816_GetChar( uint8_t *pCharToReceive )
{
    uint32_t status;
    uint32_t timeout=0;

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
static uint32_t ISO7816_SendChar( uint8_t CharToSend )
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
        TRACE_DEBUG("******* status: 0x%X (Overrun: %d, NACK: %d, Timeout: %d, underrun: %d)\n\r",
                    status, ((status & US_CSR_OVRE)>> 5), ((status & US_CSR_NACK) >> 13),
                    ((status & US_CSR_TIMEOUT) >> 8), ((status & (1 << 10)) >> 10));

        TRACE_DEBUG("E (USART CSR reg):0x%X\n\r", USART_PHONE->US_CSR);
        TRACE_DEBUG("Nb (Number of errors):0x%X\n\r", USART_PHONE->US_NER );
        USART_PHONE->US_CR = US_CR_RSTSTA;
    }

    /* Return status */
    return( status );
}


/** Initializes a ISO driver
 *  \param pPinIso7816RstMC Pin ISO 7816 Rst MC
 */
void _ISO7816_Init( const Pin pPinIso7816RstMC )
{
    TRACE_DEBUG("ISO_Init\n\r");

    /* Pin ISO7816 initialize */
    st_pinIso7816RstMC  = pPinIso7816RstMC;

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
//    USART_PHONE->US_IDR = (uint32_t) -1;
    USART_PHONE->US_BRGR = US_BRGR_CD(1);
//    USART_PHONE->US_BRGR = BOARD_MCK / (372*9600);
    USART_PHONE->US_TTGR = 5;

    /* Configure USART */
    PMC_EnablePeripheral(ID_USART_PHONE);

    /* enable USART1 interrupt */
    NVIC_EnableIRQ( USART1_IRQn ) ;
    
//    USART_PHONE->US_IER = US_IER_RXRDY | US_IER_OVRE | US_IER_FRAME | US_IER_PARE | US_IER_NACK | US_IER_ITER;


    USART_SetTransmitterEnabled(USART_PHONE, 1);
//    USART_SetReceiverEnabled(USART_PHONE, 1);

}

/*------------------------------------------------------------------------------
 *        USB 
 *------------------------------------------------------------------------------*/

// FIXME: Implement those two functions
//------------------------------------------------------------------------------
/// Put the CPU in 32kHz, disable PLL, main oscillator
/// Put voltage regulator in standby mode
//------------------------------------------------------------------------------
void LowPowerMode(void)
{
// FIXME: Implement low power consumption mode?
//    PMC_CPUInIdleMode();
}
//------------------------------------------------------------------------------
/// Put voltage regulator in normal mode
/// Return the CPU to normal speed 48MHz, enable PLL, main oscillator
//------------------------------------------------------------------------------
void NormalPowerMode(void)
{
}


/*------------------------------------------------------------------------------
 *        Main 
 *------------------------------------------------------------------------------*/

/**
 * Initializes the DBGU and ISO7816 driver, and starts some tests.
 * \return Unused (ANSI-C compatibility)
 */
extern int main( void )
{
    uint8_t pAtr[MAX_ATR_SIZE] ;
    uint8_t ucSize ;
    static uint8_t buff[100];

    LED_Configure(LED_NUM_GREEN);
    LED_Set(LED_NUM_GREEN);

    /* Disable watchdog*/
    WDT_Disable( WDT ) ;

    PIO_InitializeInterrupts(0);    

    /*  Configure ISO7816 driver */
    PIO_Configure( pinsISO7816_PHONE, PIO_LISTSIZE( pinsISO7816_PHONE ) ) ;
    PIO_Configure(pPwr, PIO_LISTSIZE(pPwr));

// FIXME: RST seems to be allways high
    TRACE_DEBUG("******* Reset State4 (1 if the Pin RstMC is high): 0x%X", ISO7816_StatusReset());
    //SendReceiveCommands() ;
    _ISO7816_Init(pinIso7816RstMC);

// FIXME: Or do I need to call VBUS_CONFIGURE() here instead, which will call USBD_Connect() later?
//    USBD_Connect();
// FIXME: USB clock? USB PMC?
    NVIC_EnableIRQ( UDP_IRQn );

    Config_PhoneRST_IrqHandler();


//    USART_EnableIt( USART_PHONE, US_IER_TXRDY) ;

    //USART_EnableIt( USART1, 0xffff) ;
   // FIXME: At some point USBD_IrqHandler() should get called and set USBD_STATE_CONFIGURED
  /*  while (USBD_GetState() < USBD_STATE_CONFIGURED) {
        int i = 1; 
        if ((i%10000) == 0) {
            TRACE_DEBUG("%d: USB State: %x\n\r", i, USBD_GetState());
        }
        i++;
    }
*/
    printf("***** START \n\r");
    TRACE_DEBUG("%s", "Start while loop\n\r");
    while (1) {

/*        if( USBState == STATE_SUSPEND ) {
            TRACE_DEBUG("suspend  !\n\r");
            LowPowerMode();
            USBState = STATE_IDLE;
        }
        if( USBState == STATE_RESUME ) {
            // Return in normal MODE
            TRACE_DEBUG("resume !\n\r");
            NormalPowerMode();
            USBState = STATE_IDLE;
        }
        CCID_SmartCardRequest();
*/
       int j;
        for( j=0; j < 1000; j++ ) {
        //    ISO7816_GetChar(&buff[j++]);
        }
        for( j=0; j < 1000; j++ ) {
        /*    printf("0x%x ", buff[j++]);
            if ((j % 40)==0) {
                printf("\n\r");    
            }
        */
        }
        //ISO7816_GetChar(&buff[0]);
        //printf("buf: 0x%x\n\r", buff[0]);

        uint32_t ret=-1;        

        if (phone_state == SEND_ATR) {
            printf("*");
            TRACE_DEBUG("Send char %x %x %x ..", ATR[0], ATR[1], ATR[2]);
            for (j=0; j < sizeof(ATR)/sizeof(ATR[0]); j++) {
                ret = ISO7816_SendChar(ATR[j]);
                TRACE_DEBUG("ret: 0x%x\n\r", ret);
            }
            phone_state = AFTER_ATR;
        }
    }
    return 0 ;
}
