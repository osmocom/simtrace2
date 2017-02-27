#include "board.h"
#include "utils.h"
#include "usb/device/dfu/dfu.h"
#include "usb/common/dfu/usb_dfu.h"
#include "manifest.h"

unsigned int g_unique_id[4];

/*----------------------------------------------------------------------------
 *       Callbacks
 *----------------------------------------------------------------------------*/

#if 0
void USBDDriverCallbacks_ConfigurationChanged(uint8_t cfgnum)
{
	TRACE_INFO_WP("cfgChanged%d ", cfgnum);
	simtrace_config = cfgnum;
}
#endif

/* returns '1' in case we should break any endless loop */
static void check_exec_dbg_cmd(void)
{
	int ch;

	if (!UART_IsRxReady())
		return;

	ch = UART_GetChar();

	//board_exec_dbg_cmd(ch);
}

/*------------------------------------------------------------------------------
 *        Main
 *------------------------------------------------------------------------------*/
#define MAX_USB_ITER BOARD_MCK/72	// This should be around a second
extern int main(void)
{
	uint8_t isUsbConnected = 0;
	unsigned int i = 0;

	LED_Configure(LED_NUM_RED);
	LED_Configure(LED_NUM_GREEN);
	LED_Set(LED_NUM_RED);

	/* Disable watchdog */
	WDT_Disable(WDT);

	//req_ctx_init();

	PIO_InitializeInterrupts(0);

	EEFC_ReadUniqueID(g_unique_id);

        printf("\r\n\r\n"
		"=============================================================================\r\n"
		"DFU bootloader %s for board %s (C) 2010-2017 by Harald Welte\r\n"
		"=============================================================================\r\n",
		manifest_revision, manifest_board);

	TRACE_INFO("Serial Nr. %08x-%08x-%08x-%08x\r\n",
		   g_unique_id[0], g_unique_id[1],
		   g_unique_id[2], g_unique_id[3]);

	//board_main_top();

	TRACE_INFO("USB init...\r\n");
	USBDFU_Initialize(&dfu_descriptors);

	while (USBD_GetState() < USBD_STATE_CONFIGURED) {
		check_exec_dbg_cmd();
#if 0
		if (i >= MAX_USB_ITER * 3) {
			TRACE_ERROR("Resetting board (USB could "
				    "not be configured)\r\n");
			NVIC_SystemReset();
		}
#endif
		i++;
	}

	TRACE_INFO("entering main loop...\r\n");
	while (1) {
#if TRACE_LEVEL >= TRACE_LEVEL_DEBUG
		const char rotor[] = { '-', '\\', '|', '/' };
		putchar('\b');
		putchar(rotor[i++ % ARRAY_SIZE(rotor)]);
#endif
		check_exec_dbg_cmd();
		//osmo_timers_prepare();
		//osmo_timers_update();

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
	}
}
