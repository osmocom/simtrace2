// FIXME: Copyright license here
/*------------------------------------------------------------------------------
 *          Headers
 *------------------------------------------------------------------------------*/

#include "board.h"

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
typedef struct {
    /* static initialization, called whether or not the usb config is active */
    void (* configure) ( void );
    /* initialization function after the config was selected */
    void (* init) ( void );
    /* de-initialization before selecting new config */
    void (* exit) ( void );
    /* main loop content for given configuration */
    void (* run) ( void );
} conf_func;

static const conf_func config_func_ptrs[] = {
	/* array slot 0 is empty, usb configs start at 1 */
	[CFG_NUM_SNIFF] = {
		.configure = Sniffer_configure,
		.init = Sniffer_init,
		.exit = Sniffer_exit,
		.run = Sniffer_run,
	},
	[CFG_NUM_CCID] = {
		.configure = CCID_configure,
		.init = CCID_init,
		.exit = CCID_exit,
		.run = CCID_run,
	},
	[CFG_NUM_PHONE] = {
		.configure = Phone_configure,
		.init = Phone_init,
		.exit = Phone_exit,
		.run = Phone_run,
	},
	[CFG_NUM_MITM] = {
		.configure = MITM_configure,
		.init = MITM_init,
		.exit = MITM_exit,
		.run = MITM_run
	},
};


/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
static volatile enum confNum simtrace_config = CFG_NUM_SNIFF;

/*----------------------------------------------------------------------------
 *       Callbacks
 *----------------------------------------------------------------------------*/

void USBDDriverCallbacks_ConfigurationChanged(uint8_t cfgnum)
{
    TRACE_INFO_WP("cfgChanged%d ", cfgnum);
    simtrace_config = cfgnum;
}

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

    for (i = 1; i < sizeof(config_func_ptrs)/sizeof(config_func_ptrs[0]); ++i)
    {
        config_func_ptrs[i].configure();
    }

    config_func_ptrs[simtrace_config].init();
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
            config_func_ptrs[last_simtrace_config].exit();
            config_func_ptrs[simtrace_config].init();
            last_simtrace_config = simtrace_config;
        } else {
            config_func_ptrs[simtrace_config].run();
        }
    }
}
