// FIXME: Copyright license here
/*------------------------------------------------------------------------------
 *          Headers
 *------------------------------------------------------------------------------*/

#include "board.h"

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
typedef struct {
    void (* init) ( void );
    void (* run) ( void );
} conf_func;

conf_func config_func_ptrs[] = {
    {Sniffer_Init, Sniffer_run},  /*  CFG_NUM_SNIFF */
    {CCID_init, CCID_run},  /*  CFG_NUM_CCID */
    {Phone_Master_Init, Phone_run},  /* CFG_NUM_PHONE */
    {MITM_init, MITM_run},  /* CFG_NUM_MITM */
};


/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
uint8_t simtrace_config = CFG_NUM_SNIFF;
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

// FIXME: why don't we get any interrupts with this line?:
//    while(USBD_GetState() < USBD_STATE_CONFIGURED);

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

//        for (int i=0; i <10000; i++);

/*  FIXME: Or should we move the while loop into every case, and break out
    in case the config changes? */
        if (conf_changed) {
            config_func_ptrs[simtrace_config-1].init();
            conf_changed = 0;
        } else {
            config_func_ptrs[simtrace_config-1].run();
        }
    }
}
