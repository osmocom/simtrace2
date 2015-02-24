// FIXME: Copyright license here
/*------------------------------------------------------------------------------
 *          Headers
 *------------------------------------------------------------------------------*/

#include "board.h"
#include "simtrace.h"

/*------------------------------------------------------------------------------
 *         Internal definitions
 *------------------------------------------------------------------------------*/
#define CONF_SNIFFER        1
#define CONF_CCID_READER    2
#define CONF_SIMCARD_EMUL   3
#define CONF_MITM           4


/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
uint8_t simtrace_config = CONF_SNIFFER;
uint8_t conf_changed = 0;

/*------------------------------------------------------------------------------
 *        Main 
 *------------------------------------------------------------------------------*/

extern int main( void )
{
    LED_Configure(LED_NUM_GREEN);
    LED_Set(LED_NUM_GREEN);

    /* Disable watchdog*/
    WDT_Disable( WDT ) ;

    PIO_InitializeInterrupts(0);


    TRACE_DEBUG("%s", "Start\n\r");
    while(1) {
/*  FIXME: Or should we move the while loop into every case, and break out
    in case the config changes? */
        switch(simtrace_config) {
            case CONF_SNIFFER:
                break;
            case CONF_CCID_READER:
                break;
            case CONF_SIMCARD_EMUL:
                if (conf_changed) {
                    Phone_Master_Init();
                    /*  Configure ISO7816 driver */
                    // FIXME:    PIO_Configure(pPwr, PIO_LISTSIZE( pPwr ));
                } else {
                    /* Send and receive chars */
                    // ISO7816_GetChar(&rcv_char);
                    // ISO7816_SendChar(char_to_send);
                }
                break;
            case CONF_MITM:
                break;
            default:
                break;
        }
    }
}
