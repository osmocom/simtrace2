/* SIMtrace 2 firmware card emulation application
 *
 * (C) 2015-2017 by Harald Welte <hwelte@hmw-consulting.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*------------------------------------------------------------------------------
 *          Headers
 *------------------------------------------------------------------------------*/

#include "board.h"
#include "simtrace.h"
#include "utils.h"
#include <osmocom/core/timer.h>

unsigned int g_unique_id[4];

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
	if (cfgnum < ARRAY_SIZE(config_func_ptrs)) {
		simtrace_config = cfgnum;
	} else {
		TRACE_ERROR("trying to set out of bounds config %u\r\n", cfgnum);
	}
}

void USART1_IrqHandler(void)
{
	if (config_func_ptrs[simtrace_config].usart1_irq)
		config_func_ptrs[simtrace_config].usart1_irq();
}

void USART0_IrqHandler(void)
{
	if (config_func_ptrs[simtrace_config].usart0_irq)
		config_func_ptrs[simtrace_config].usart0_irq();
}

/* returns '1' in case we should break any endless loop */
static void check_exec_dbg_cmd(void)
{
	int ch;

	if (!UART_IsRxReady())
		return;

	ch = UART_GetChar();
	/* We must echo the character to make python fdexpect happy, which we use in factory testing */
	fputc(ch, stdout);
	board_exec_dbg_cmd(ch);
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

	led_init();
	led_blink(LED_RED, BLINK_3O_5F);

	/* Enable watchdog for 2000ms, with no window */
	WDT_Enable(WDT, WDT_MR_WDRSTEN | WDT_MR_WDDBGHLT | WDT_MR_WDIDLEHLT |
		   (WDT_GetPeriod(2000) << 16) | WDT_GetPeriod(2000));

	PIO_InitializeInterrupts(0);

	EEFC_ReadUniqueID(g_unique_id);

		printf("\n\r\n\r"
		"=============================================================================\n\r"
		"SIMtrace2 firmware " GIT_VERSION " (C) 2010-2016 by Harald Welte\n\r"
		"=============================================================================\n\r");

#if (TRACE_LEVEL >= TRACE_LEVEL_INFO)
	TRACE_INFO("Chip ID: 0x%08lx (Ext 0x%08lx)\n\r", CHIPID->CHIPID_CIDR, CHIPID->CHIPID_EXID);
	TRACE_INFO("Serial Nr. %08x-%08x-%08x-%08x\n\r",
		   g_unique_id[0], g_unique_id[1],
		   g_unique_id[2], g_unique_id[3]);
	uint8_t reset_cause = (RSTC->RSTC_SR & RSTC_SR_RSTTYP_Msk) >> RSTC_SR_RSTTYP_Pos;
	static const char* reset_causes[] = {
		"general reset (first power-up reset)",
		"backup reset (return from backup mode)",
		"watchdog reset (watchdog fault occurred)",
		"software reset (processor reset required by the software)",
		"user reset (NRST pin detected low)",
	};
	if (reset_cause < ARRAY_SIZE(reset_causes)) {
		TRACE_INFO("Reset Cause: %s\n\r", reset_causes[reset_cause]);
	} else {
		TRACE_INFO("Reset Cause: 0x%lx\n\r", (RSTC->RSTC_SR & RSTC_SR_RSTTYP_Msk) >> RSTC_SR_RSTTYP_Pos);
	}
#endif

	board_main_top();

	TRACE_INFO("USB init...\n\r");
	SIMtrace_USB_Initialize();

	while (USBD_GetState() < USBD_STATE_CONFIGURED) {
		WDT_Restart(WDT);
		check_exec_dbg_cmd();
#if 0
		if (i >= MAX_USB_ITER * 3) {
			TRACE_ERROR("Resetting board (USB could "
				    "not be configured)\n\r");
			USBD_Disconnect();
			NVIC_SystemReset();
		}
#endif
		i++;
	}

	TRACE_INFO("calling configure of all configurations...\n\r");
	for (i = 1; i < sizeof(config_func_ptrs) / sizeof(config_func_ptrs[0]);
	     ++i) {
		if (config_func_ptrs[i].configure)
			config_func_ptrs[i].configure();
	}

	TRACE_INFO("calling init of config %u...\n\r", simtrace_config);
	if (config_func_ptrs[simtrace_config].init) {
		config_func_ptrs[simtrace_config].init();
	}
	last_simtrace_config = simtrace_config;

	TRACE_INFO("entering main loop...\n\r");
	while (1) {
		WDT_Restart(WDT);
#if TRACE_LEVEL >= TRACE_LEVEL_DEBUG
		const char rotor[] = { '-', '\\', '|', '/' };
		putchar('\b');
		putchar(rotor[i++ % ARRAY_SIZE(rotor)]);
#endif
		check_exec_dbg_cmd();
		osmo_timers_prepare();
		osmo_timers_update();

		if (USBD_GetState() < USBD_STATE_CONFIGURED) {

			if (isUsbConnected) {
				isUsbConnected = 0;
			}
		} else if (isUsbConnected == 0) {
			TRACE_INFO("USB is now configured\n\r");

			isUsbConnected = 1;
		}
		if (last_simtrace_config != simtrace_config) {
			TRACE_INFO("USB config chg %u -> %u\n\r",
				   last_simtrace_config, simtrace_config);
			if (config_func_ptrs[last_simtrace_config].exit) {
				config_func_ptrs[last_simtrace_config].exit();
			}
			if (config_func_ptrs[simtrace_config].init) {
				config_func_ptrs[simtrace_config].init();
			}
			last_simtrace_config = simtrace_config;
		} else {
			if (config_func_ptrs[simtrace_config].run) {
				config_func_ptrs[simtrace_config].run();
			}
		}
	}
}
