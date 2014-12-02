#include "board.h"
#include "spi.h"
#include "pio.h"
#include "pmc.h"
#include "usart.h"

#define BUFFER_SIZE         20

/** Pins to configure for the application.*/
const Pin pins[] = { BOARD_PIN_USART_RXD,
                     BOARD_PIN_USART_TXD
//                     PIN_USART1_SCK,
//                     BOARD_PIN_USART_RTS,
//                     BOARD_PIN_USART_CTS,
//                    PINS_SPI,
//                    PIN_SPI_NPCS0_PA11
                    };

char Buffer[BUFFER_SIZE] = "Hello World";
/**
 * \brief Configures USART in synchronous mode,8N1
 *  * \param mode 1 for  master, 0 for slave
 *   */
static void _ConfigureUsart(uint32_t freq )
{
    uint32_t mode;
/* MASTER; mode  = US_MR_USART_MODE_NORMAL
                    | US_MR_USCLKS_MCK
                    | US_MR_CHMODE_NORMAL 
                    | US_MR_CLKO
                    | US_MR_SYNC    // FIXME: sync or not sync? 
                    | US_MR_MSBF
                    | US_MR_CHRL_8_BIT
                    | US_MR_NBSTOP_1_BIT
                    | US_MR_PAR_NO ;
*/


// Slave mode:
        mode = US_MR_USART_MODE_NORMAL   
//                | US_MR_USCLKS_SCK    // we don't have serial clock, do we?
                | US_MR_CHMODE_NORMAL
//                | US_MR_SYNC 
                | US_MR_MSBF
                | US_MR_CHRL_8_BIT 
                | US_MR_NBSTOP_1_BIT
                | US_MR_PAR_NO;
    
    /* Enable the peripheral clock in the PMC */
    PMC_EnablePeripheral( BOARD_ID_USART ) ;

    /* Configure the USART in the desired mode @USART_SPI_CLK bauds*/
    USART_Configure( BOARD_USART_BASE, mode, freq, BOARD_MCK ) ;

    /* enable USART1 interrupt */ 
//    NVIC_EnableIRQ( USART1_IRQn ) ;

    /* Enable receiver & transmitter */
    USART_SetTransmitterEnabled( BOARD_USART_BASE, 1 ) ;
    USART_SetReceiverEnabled( BOARD_USART_BASE, 1 ) ;
}

int main() {
    /*  Configure pins */
    PIO_Configure( pins, PIO_LISTSIZE( pins ) ) ;

    _ConfigureUsart( 1000000UL);

    USART_WriteBuffer(BOARD_USART_BASE, Buffer, BUFFER_SIZE);

// FIXME: do we need to call USARTEnable?


// FIXME: do we need to call USARTDisable?

    return 0;
}
