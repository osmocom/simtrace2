// FIXME: Copyright license here
/*------------------------------------------------------------------------------
 *          Headers
 *------------------------------------------------------------------------------*/

#include "board.h"

/*------------------------------------------------------------------------------
 *         Internal definitions
 *------------------------------------------------------------------------------*/
#define CONF_NONE           0
#define CONF_SNIFFER        1
#define CONF_CCID_READER    2
#define CONF_SIMCARD_EMUL   3
#define CONF_MITM           4


/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
uint8_t simtrace_config = CONF_NONE;
uint8_t conf_changed = 1;

uint8_t rcvdChar = 0;
uint32_t char_stat = 0;

/*------------------------------------------------------------------------------
 *        Main 
 *------------------------------------------------------------------------------*/

extern int main( void )
{
    uint8_t isUsbConnected = 0;

    LED_Configure(LED_NUM_GREEN);
    LED_Set(LED_NUM_GREEN);

    /* Disable watchdog*/
    WDT_Disable( WDT ) ;

    PIO_InitializeInterrupts(0);

    SIMtrace_USB_Initialize();

    printf("%s", "USB init\n\r");

    while(USBD_GetState() < USBD_STATE_CONFIGURED);

    TRACE_DEBUG("%s", "Start\n\r");
    printf("%s", "Start\n\r");
    while(1) {

            /* Device is not configured */
        if (USBD_GetState() < USBD_STATE_CONFIGURED) {

            if (isUsbConnected) {
                isUsbConnected = 0;
//                TC_Stop(TC0, 0);
            }
        }
        else if (isUsbConnected == 0) {
            printf("USB is now configured\n\r");

            isUsbConnected = 1;
//            TC_Start(TC0, 0);
        }    
    

/*  FIXME: Or should we move the while loop into every case, and break out
    in case the config changes? */
        switch(simtrace_config) {
            case CONF_SNIFFER:
                if (conf_changed) {
                    Sniffer_Init();
                    conf_changed = 0;
                } else {
                    if (rcvdChar != 0) {
                        TRACE_DEBUG("Rcvd char _%x_ \n\r", rcvdChar);
                        rcvdChar = 0;
                    }
                }
                break;
            case CONF_CCID_READER:
                if (conf_changed) {
                    // Init
                    conf_changed = 0;
                } else {
                    // Receive char
                }
                break;
            case CONF_SIMCARD_EMUL:
                if (conf_changed) {
                    Phone_Master_Init();
                    conf_changed = 0;
                    /*  Configure ISO7816 driver */
                    // FIXME:    PIO_Configure(pPwr, PIO_LISTSIZE( pPwr ));
                } else {
                    /* Send and receive chars */
                    // ISO7816_GetChar(&rcv_char);
                    // ISO7816_SendChar(char_to_send);
                }
                break;
            case CONF_MITM:
                if (conf_changed) {
                    // Init
                    conf_changed = 0;
                } else {
                    // Receive char
                }
                break;
            default:
                break;
        }
    }
}
