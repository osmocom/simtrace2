// FIXME: Copyright license here
/*------------------------------------------------------------------------------
 *          Headers
 *------------------------------------------------------------------------------*/

#include "board.h"

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
typedef struct {
    void (* configure) ( void );
    void (* init) ( void );
    void (* exit) ( void );
    void (* run) ( void );
} conf_func;

conf_func config_func_ptrs[] = {
    {Sniffer_configure, Sniffer_init, Sniffer_exit, Sniffer_run},  /*  CFG_NUM_SNIFF */
    {CCID_configure, CCID_init, CCID_exit, CCID_run},  /*  CFG_NUM_CCID */
    {Phone_configure, Phone_init, Phone_exit, Phone_run},  /* CFG_NUM_PHONE */
    {MITM_configure, MITM_init, MITM_exit, MITM_run},  /* CFG_NUM_MITM */
};


/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
volatile enum confNum simtrace_config = CFG_NUM_SNIFF;

/*------------------------------------------------------------------------------
 *        Main
 *------------------------------------------------------------------------------*/
#define MAX_USB_ITER BOARD_MCK/72  // This should be around a second
extern int main( void )
{
    uint8_t isUsbConnected = 0;
    enum confNum last_simtrace_config = simtrace_config;
    unsigned int i = 0;

    LED_Configure(LED_NUM_RED);
    LED_Configure(LED_NUM_GREEN);
    LED_Set(LED_NUM_RED);

    /* Disable watchdog*/
    WDT_Disable( WDT ) ;

    PIO_InitializeInterrupts(0);

    SIMtrace_USB_Initialize();

    printf("%s", "USB init\n\r");
    while(USBD_GetState() < USBD_STATE_CONFIGURED){
        if(i >= MAX_USB_ITER*3) {
            TRACE_ERROR("Resetting board (USB could not be configured)\n");
            NVIC_SystemReset();
        }
        i++;
    }

    for (i = 0; i < sizeof(config_func_ptrs)/sizeof(config_func_ptrs[0]); ++i)
    {
        config_func_ptrs[i].configure();
    }

    config_func_ptrs[simtrace_config-1].init();
    last_simtrace_config = simtrace_config;

    printf("%s", "Start\n\r");
    while(1) {

        if (USBD_GetState() < USBD_STATE_CONFIGURED) {

            if (isUsbConnected) {
                isUsbConnected = 0;
            }
        }
        else if (isUsbConnected == 0) {
            printf("USB is now configured\n\r");
            LED_Set(LED_NUM_GREEN);
            LED_Clear(LED_NUM_RED);

            isUsbConnected = 1;
        }


        if (last_simtrace_config != simtrace_config) {
            config_func_ptrs[last_simtrace_config-1].exit();
            config_func_ptrs[simtrace_config-1].init();
            last_simtrace_config = simtrace_config;
        } else {
            config_func_ptrs[simtrace_config-1].run();
        }
    }
}
