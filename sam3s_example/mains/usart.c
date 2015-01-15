#include "board.h"
#include "spi.h"
#include "pio.h"
#include "pmc.h"
#include "usart.h"

#define BUFFER_SIZE         20

const Pin statusled = {LED_GREEN, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};

/*
 *  Describes the type and attribute of one PIO pin or a group of similar pins.
 *  The #type# field can have the following values:
 *     - PIO_PERIPH_A
 *     - PIO_PERIPH_B
 *     - PIO_OUTPUT_0
 *     - PIO_OUTPUT_1
 *     - PIO_INPUT
 *
 *  The #attribute# field is a bitmask that can either be set to PIO_DEFAULt,
 *  or combine (using bitwise OR '|') any number of the following constants:
 *     - PIO_PULLUP
 *     - PIO_DEGLITCH
 *     - PIO_DEBOUNCE
 *     - PIO_OPENDRAIN
 *     - PIO_IT_LOW_LEVEL
 *     - PIO_IT_HIGH_LEVEL
 *     - PIO_IT_FALL_EDGE
 *     - PIO_IT_RISE_EDGE
 */
#if 0
typedef struct _Pin
{
    /*  Bitmask indicating which pin(s) to configure. */
    uint32_t mask;
    /*  Pointer to the PIO controller which has the pin(s). */
    Pio    *pio;
    /*  Peripheral ID of the PIO controller which has the pin(s). */
    uint8_t id;
    /*  Pin type. */
    uint8_t type;
    /*  Pin attribute. */
    uint8_t attribute;
} Pin ; */
 #endif          



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
                | US_MR_USCLKS_SCK    // we don't have serial clock, do we?
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
    UART_Configure(9600, BOARD_MCK);

    /* enable USART1 interrupt */ 
//    NVIC_EnableIRQ( USART1_IRQn ) ;

    /* Enable receiver & transmitter */
    USART_SetTransmitterEnabled( BOARD_USART_BASE, 1 ) ;
    USART_SetReceiverEnabled( BOARD_USART_BASE, 1 ) ;
}

int main() {
    register int i = 0;
    register int state = 0;
    /*  Configure pins */

// FIXME: initialize system clock done in lowlevelinit?

    PIO_Configure(&statusled, PIO_LISTSIZE(statusled));
    PIO_Clear(&statusled);

    PIO_Configure( pins, PIO_LISTSIZE( pins ) ) ;

    _ConfigureUsart( 1000000UL);

    while(1) {
        USART_WriteBuffer(BOARD_USART_BASE, Buffer, BUFFER_SIZE);

        i = i+1;
        if ((i%500000) == 0) {
            switch(state) {
                case 0:
                    PIO_Set(&statusled);
                    state=1;
                    break;
                case 1:
                    PIO_Clear(&statusled);
                    state = 2;
                    break;
                default:
                    state = 0;
            }
        }
    }

// FIXME: do we need to call USARTEnable?


// FIXME: do we need to call USARTDisable?

    return 0;
}
