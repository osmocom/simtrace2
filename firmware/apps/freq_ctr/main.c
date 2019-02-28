
#include "board.h"
#include "utils.h"
#include "osmocom/core/timer.h"

extern void freq_ctr_init(void);

/* returns '1' in case we should break any endless loop */
static void check_exec_dbg_cmd(void)
{
	int ch;

	if (!UART_IsRxReady())
		return;

	ch = UART_GetChar();

	board_exec_dbg_cmd(ch);
}


extern int main(void)
{
	led_init();
	led_blink(LED_RED, BLINK_ALWAYS_ON);
	led_blink(LED_GREEN, BLINK_ALWAYS_ON);

	/* Enable watchdog for 2000 ms, with no window */
	WDT_Enable(WDT, WDT_MR_WDRSTEN | WDT_MR_WDDBGHLT | WDT_MR_WDIDLEHLT |
		   (WDT_GetPeriod(2000) << 16) | WDT_GetPeriod(2000));

	PIO_InitializeInterrupts(0);


	printf("\n\r\n\r"
		"=============================================================================\n\r"
		"Freq Ctr firmware " GIT_VERSION " (C) 2019 by Harald Welte\n\r"
		"=============================================================================\n\r");

	board_main_top();

	TRACE_INFO("starting frequency counter...\n\r");
	freq_ctr_init();

	TRACE_INFO("entering main loop...\n\r");
	while (1) {
		WDT_Restart(WDT);

		check_exec_dbg_cmd();
		osmo_timers_prepare();
		osmo_timers_update();
	}

}
