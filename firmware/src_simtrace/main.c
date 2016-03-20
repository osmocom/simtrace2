// FIXME: Copyright license here
/*------------------------------------------------------------------------------
 *          Headers
 *------------------------------------------------------------------------------*/

#include "board.h"
#include "simtrace.h"
#include "utils.h"
#include "req_ctx.h"

uint32_t g_unique_id[4];

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
typedef struct {
	/* static initialization, called whether or not the usb config is active */
	void (*configure) (void);
	/* initialization function after the config was selected */
	void (*init) (void);
	/* de-initialization before selecting new config */
	void (*exit) (void);
	/* main loop content for given configuration */
	void (*run) (void);
	/* Interrupt handler for USART1 */
	void (*usart0_irq) (void);
	/* Interrupt handler for USART1 */
	void (*usart1_irq) (void);
} conf_func;

static const conf_func config_func_ptrs[] = {
	/* array slot 0 is empty, usb configs start at 1 */
#ifdef HAVE_SNIFFER
	[CFG_NUM_SNIFF] = {
		.configure = Sniffer_configure,
		.init = Sniffer_init,
		.exit = Sniffer_exit,
		.run = Sniffer_run,
	},
#endif
#ifdef HAVE_CCID
	[CFG_NUM_CCID] = {
		.configure = CCID_configure,
		.init = CCID_init,
		.exit = CCID_exit,
		.run = CCID_run,
	},
#endif
#ifdef HAVE_CARDEM
	[CFG_NUM_PHONE] = {
		.configure = mode_cardemu_configure,
		.init = mode_cardemu_init,
		.exit = mode_cardemu_exit,
		.run = mode_cardemu_run,
		.usart0_irq = mode_cardemu_usart0_irq,
		.usart1_irq = mode_cardemu_usart1_irq,
	},
#endif
#ifdef HAVE_MITM
	[CFG_NUM_MITM] = {
		.configure = MITM_configure,
		.init = MITM_init,
		.exit = MITM_exit,
		.run = MITM_run,
	},
#endif
};

/*------------------------------------------------------------------------------
 *         Internal variables
 *------------------------------------------------------------------------------*/
#if defined(HAVE_SNIFFER)
static volatile enum confNum simtrace_config = CFG_NUM_SNIFF;
#elif defined(HAVE_CARDEM)
static volatile enum confNum simtrace_config = CFG_NUM_PHONE;
#elif defined(HAVE_CCID)
static volatile enum confNum simtrace_config = CFG_NUM_CCID;
#endif

/*----------------------------------------------------------------------------
 *       Callbacks
 *----------------------------------------------------------------------------*/

void USBDDriverCallbacks_ConfigurationChanged(uint8_t cfgnum)
{
	TRACE_INFO_WP("cfgChanged%d ", cfgnum);
	simtrace_config = cfgnum;
}

void USART1_IrqHandler(void)
{
	config_func_ptrs[simtrace_config].usart1_irq();
}

void USART0_IrqHandler(void)
{
	config_func_ptrs[simtrace_config].usart0_irq();
}

/*------------------------------------------------------------------------------
 *        Main
 *------------------------------------------------------------------------------*/
#define MAX_USB_ITER BOARD_MCK/72	// This should be around a second
extern int main(void)
{
	uint8_t isUsbConnected = 0;
	enum confNum last_simtrace_config = simtrace_config;
	unsigned int i = 0;

	LED_Configure(LED_NUM_RED);
	LED_Configure(LED_NUM_GREEN);
	LED_Set(LED_NUM_RED);

	/* Disable watchdog */
	WDT_Disable(WDT);

	req_ctx_init();

	PIO_InitializeInterrupts(0);

	SIMtrace_USB_Initialize();

	EEFC_ReadUniqueID(g_unique_id);

        printf("\r\n\r\n"
		"=============================================================================\r\n"
		"SIMtrace2 firmware " GIT_VERSION " (C) 2010-2016 by Harald Welte\r\n"
		"=============================================================================\r\n");

	TRACE_INFO("Serial Nr. %08x-%08x-%08x-%08x\r\n",
		   g_unique_id[0], g_unique_id[1],
		   g_unique_id[2], g_unique_id[3]);

	TRACE_INFO("USB init...\r\n");
	while (USBD_GetState() < USBD_STATE_CONFIGURED) {
		if (i >= MAX_USB_ITER * 3) {
			TRACE_ERROR("Resetting board (USB could "
				    "not be configured)\r\n");
			NVIC_SystemReset();
		}
		i++;
	}

	TRACE_INFO("calling configure of all configurations...\r\n");
	for (i = 1; i < sizeof(config_func_ptrs) / sizeof(config_func_ptrs[0]);
	     ++i) {
		if (config_func_ptrs[i].configure)
			config_func_ptrs[i].configure();
	}

	TRACE_INFO("calling init of config %u...\r\n", simtrace_config);
	config_func_ptrs[simtrace_config].init();
	last_simtrace_config = simtrace_config;

	TRACE_INFO("entering main loop...\r\n");
	while (1) {
#if TRACE_LEVEL >= TRACE_LEVEL_DEBUG
		const char rotor[] = { '-', '\\', '|', '/' };
		putchar('\b');
		putchar(rotor[i++ % ARRAY_SIZE(rotor)]);
#endif

		if (USBD_GetState() < USBD_STATE_CONFIGURED) {

			if (isUsbConnected) {
				isUsbConnected = 0;
			}
		} else if (isUsbConnected == 0) {
			TRACE_INFO("USB is now configured\r\n");
			LED_Set(LED_NUM_GREEN);
			LED_Clear(LED_NUM_RED);

			isUsbConnected = 1;
		}
		if (last_simtrace_config != simtrace_config) {
			TRACE_INFO("USB config chg %u -> %u\r\n",
				   last_simtrace_config, simtrace_config);
			config_func_ptrs[last_simtrace_config].exit();
			config_func_ptrs[simtrace_config].init();
			last_simtrace_config = simtrace_config;
		} else {
			config_func_ptrs[simtrace_config].run();
		}
	}
}
