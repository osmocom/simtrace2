#include "board.h"

#include <stdio.h>
#include <string.h>

/*----------------------------------------------------------------------------
 *  *        Definitions
 *   *----------------------------------------------------------------------------*/

/** Console baudrate always using 115200. */
#define CONSOLE_BAUDRATE    115200
/** Usart Hw interface used by the console (UART0). */
#define CONSOLE_USART       UART0
/** Usart Hw ID used by the console (UART0). */
#define CONSOLE_ID          ID_UART0
/** Pins description corresponding to Rxd,Txd, (UART pins) */
#define CONSOLE_PINS        {PINS_UART}

/*----------------------------------------------------------------------------
 *  *        Variables
 *   *----------------------------------------------------------------------------*/

const Pin redled = {LED_RED, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
const Pin greenled = {LED_GREEN, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};

static const Pin *led;

/** Is Console Initialized. */
static uint8_t _ucIsConsoleInitialized=0 ;

/**
 * \brief Configures an USART peripheral with the specified parameters.
 *
 * \param baudrate  Baudrate at which the USART should operate (in Hz).
 * \param masterClock  Frequency of the system master clock (in Hz).
 */
extern void UART_Configure( uint32_t baudrate, uint32_t masterClock)
{
    const Pin pPins[] = CONSOLE_PINS;
    Uart *pUart = CONSOLE_USART;


    /* Configure PIO */
    PIO_Configure(pPins, PIO_LISTSIZE(pPins));

    /* Configure PMC */
/*     PMC_PCER0 : (PMC Offset: 0x0010) Peripheral Clock Enable Register 0 -------- */
    PMC->PMC_PCER0 = 1 << CONSOLE_ID;

    /* Reset and disable receiver & transmitter */
    pUart->UART_CR = UART_CR_RSTRX | UART_CR_RSTTX
                   | UART_CR_RXDIS | UART_CR_TXDIS;

    /* Configure mode */
    pUart->UART_MR =  UART_MR_PAR_NO;   // No parity

    /* Configure baudrate */
    /* Asynchronous, no oversampling */
    // BRG: Baud rate generator
    pUart->UART_BRGR = (masterClock / baudrate) / 16;

    /* Disable PDC channel */
    // PDC: Peripheral DMA
    // TCR: Transfer Control Register
    pUart->UART_PTCR = UART_PTCR_RXTDIS | UART_PTCR_TXTDIS;

    /* Enable receiver and transmitter */
    
    pUart->UART_CR = UART_CR_RXEN | UART_CR_TXEN;

    _ucIsConsoleInitialized=1 ;
}


/**
 * \brief Outputs a character on the UART line.
 *
 * \note This function is synchronous (i.e. uses polling).
 * \param c  Character to send.
 */
extern void UART_PutChar( uint8_t c )
{
    Uart *pUart=CONSOLE_USART ;

    if ( !_ucIsConsoleInitialized )
    {
        UART_Configure(CONSOLE_BAUDRATE, BOARD_MCK);
    }

    /* Wait for the transmitter to be ready */
    // UART_SR: Statzs register
    while ( (pUart->UART_SR & UART_SR_TXEMPTY) == 0 ) ;

    /* Send character */
    // THR: transmit holding register
    pUart->UART_THR=c ;

}



/**
 * \brief Input a character from the UART line.
 *
 * \note This function is synchronous
 * \return character received.
 */
extern uint32_t UART_GetChar( void )
{
    Uart *pUart=CONSOLE_USART ;

    if ( !_ucIsConsoleInitialized )
    {
        UART_Configure(CONSOLE_BAUDRATE, BOARD_MCK);
    }

    while ( (pUart->UART_SR & UART_SR_RXRDY) == 0 ) ;

    return pUart->UART_RHR ;
}

void Configure_LED() {
    PIO_Configure(&greenled, PIO_LISTSIZE(greenled));
    PIO_Configure(&redled, PIO_LISTSIZE(redled));
    PIO_Set(&redled);
    PIO_Set(&greenled);
    led = &redled;
}

void UART_PutString(char *str, int len) {
    int i;
    for (i=0; i<len; i++) {
        UART_PutChar(*str++);
    }
}

int main() {
    char *cmdp;
// FIXME: initialize system clock done in lowlevelinit?

    Configure_LED();

    size_t ret = asprintf(&cmdp, "Clockval: %d\r\n", BOARD_MCK);

    if (ret != strlen(cmdp)){
         PIO_Clear(&redled);
    } else {
         PIO_Clear(&greenled);
        while (1) {
            UART_PutString(cmdp, strlen(cmdp));
        }
    }

    return 0;
}
