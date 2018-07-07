/* Quad-modem speciic application code */
/* (C) 2016-2016 by Harald Welte <laforge@gnumonks.org> */

#include "board.h"
#include "simtrace.h"
#include "utils.h"
#include "wwan_led.h"
#include "wwan_perst.h"
#include "sim_switch.h"
#include "boardver_adc.h"
#include "card_pres.h"
#include <osmocom/core/timer.h>
#include "usb_buf.h"

static const Pin pin_hubpwr_override = PIN_PRTPWR_OVERRIDE;
static const Pin pin_hub_rst = {PIO_PA13, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
static const Pin pin_1234_detect = {PIO_PA14, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP};
static const Pin pin_peer_rst = {PIO_PA0, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
static const Pin pin_peer_erase = {PIO_PA11, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};

/* array of generated USB Strings */
extern unsigned char *usb_strings[];

static int qmod_sam3_is_12(void)
{
	if (PIO_Get(&pin_1234_detect) == 0)
		return 1;
	else
		return 0;
}

const unsigned char __eeprom_bin[256] = {
	USB_VENDOR_OPENMOKO & 0xff,
	USB_VENDOR_OPENMOKO >> 8,
	USB_PRODUCT_QMOD_HUB & 0xff,
	USB_PRODUCT_QMOD_HUB >> 8,
	                        0x00, 0x00, 0x9b, 0x20, 0x09, 0x00, 0x00, 0x00, 0x32, 0x32, 0x32, 0x32, /* 0x00 - 0x0f */
	0x32, 0x04, 0x09, 0x18, 0x0d, 0x00, 0x73, 0x00, 0x79, 0x00, 0x73, 0x00, 0x6d, 0x00, 0x6f, 0x00, /* 0x10 - 0x1f */
	0x63, 0x00, 0x6f, 0x00, 0x6d, 0x00, 0x20, 0x00, 0x2d, 0x00, 0x20, 0x00, 0x73, 0x00, 0x2e, 0x00, /* 0x20 - 0x2f */
	0x66, 0x00, 0x2e, 0x00, 0x6d, 0x00, 0x2e, 0x00, 0x63, 0x00, 0x2e, 0x00, 0x20, 0x00, 0x47, 0x00, /* 0x30 - 0x3f */
	0x6d, 0x00, 0x62, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x40 - 0x4f */
	0x00, 0x00, 0x00, 0x00, 0x71, 0x00, 0x75, 0x00, 0x61, 0x00, 0x64, 0x00, 0x20, 0x00, 0x6d, 0x00, /* 0x50 - 0x5f */
	0x6f, 0x00, 0x64, 0x00, 0x65, 0x00, 0x6d, 0x00, 0x20, 0x00, 0x76, 0x00, 0x32, 0x00, 0x00, 0x00, /* 0x60 - 0x6f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x70 - 0x7f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x80 - 0x8f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x90 - 0x9f */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa0 - 0xaf */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xb0 - 0xbf */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xc0 - 0xcf */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xd0 - 0xdf */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xe0 - 0xef */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x56, 0x23, 0x71, 0x04, 0x00, /* 0xf0 - 0xff */
};

#include "i2c.h"
static int write_hub_eeprom(void)
{
	const unsigned int __eeprom_bin_len = 256;

	int i;

	/* wait */
	mdelay(100);

	TRACE_INFO("Writing EEPROM...\n\r");
	/* write the EEPROM once */
	for (i = 0; i < 256; i++) {
		int rc = eeprom_write_byte(0x50, i, __eeprom_bin[i]);
		/* if the result was negative, repeat that write */
		if (rc < 0)
			i--;
	}

	/* then pursue re-reading it again and again */
	TRACE_INFO("Verifying EEPROM...\n\r");
	for (i = 0; i < 256; i++) {
		int byte = eeprom_read_byte(0x50, i);
		TRACE_INFO("0x%02x: %02x\n\r", i, byte);
		if (byte != __eeprom_bin[i])
			TRACE_ERROR("Byte %u is wrong, expected 0x%02x, found 0x%02x\n\r",
					i, __eeprom_bin[i], byte);
	}

	/* FIXME: Release PIN_PRTPWR_OVERRIDE after we know the hub is
	 * again powering us up */

	return 0;
}

static void board_exec_dbg_cmd_st12only(int ch)
{
	uint32_t addr, val;

	/* functions below only work on primary (ST12) */
	if (!qmod_sam3_is_12())
		return;

	switch (ch) {
	case 'E':
		write_hub_eeprom();
		break;
	case 'O':
		printf("Setting PRTPWR_OVERRIDE\n\r");
		PIO_Set(&pin_hubpwr_override);
		break;
	case 'o':
		printf("Clearing PRTPWR_OVERRIDE\n\r");
		PIO_Clear(&pin_hubpwr_override);
		break;
	case 'H':
		printf("Clearing _HUB_RESET -> HUB_RESET high (inactive)\n\r");
		PIO_Clear(&pin_hub_rst);
		break;
	case 'h':
		/* high level drives transistor -> HUB_RESET low */
		printf("Asserting _HUB_RESET -> HUB_RESET low (active)\n\r");
		PIO_Set(&pin_hub_rst);
		break;
	case 'w':
		if (PIO_GetOutputDataStatus(&pin_hub_rst) == 0)
			printf("WARNING: attempting EEPROM access while HUB not in reset\n\r");
		printf("Please enter EEPROM offset:\n\r");
		UART_GetIntegerMinMax(&addr, 0, 255);
		printf("Please enter EEPROM value:\n\r");
		UART_GetIntegerMinMax(&val, 0, 255);
		printf("Writing value 0x%02x to EEPROM offset 0x%02x\n\r", val, addr);
		eeprom_write_byte(0x50, addr, val);
		break;
	case 'r':
		printf("Please enter EEPROM offset:\n\r");
		UART_GetIntegerMinMax(&addr, 0, 255);
		printf("EEPROM[0x%02x] = 0x%02x\n\r", addr, eeprom_read_byte(0x50, addr));
		break;
	default:
		printf("Unknown command '%c'\n\r", ch);
		break;
	}
}

/* returns '1' in case we should break any endless loop */
void board_exec_dbg_cmd(int ch)
{
	switch (ch) {
	case '?':
		printf("\t?\thelp\n\r");
		printf("\tR\treset SAM3\n\r");
		if (qmod_sam3_is_12()) {
			printf("\tE\tprogram EEPROM\n\r");
			printf("\tO\tEnable PRTPWR_OVERRIDE\n\r");
			printf("\to\tDisable PRTPWR_OVERRIDE\n\r");
			printf("\tH\tRelease HUB RESET (high)\n\r");
			printf("\th\tAssert HUB RESET (low)\n\r");
			printf("\tw\tWrite single byte in EEPROM\n\r");
			printf("\tr\tRead single byte from EEPROM\n\r");
		}
		printf("\tX\tRelease peer SAM3 from reset\n\r");
		printf("\tx\tAssert peer SAM3 reset\n\r");
		printf("\tY\tRelease peer SAM3 ERASE signal\n\r");
		printf("\ty\tAssert peer SAM3 ERASE signal\n\r");
		printf("\tU\tProceed to USB Initialization\n\r");
		printf("\t1\tGenerate 1ms reset pulse on WWAN1\n\r");
		printf("\t2\tGenerate 1ms reset pulse on WWAN2\n\r");
		break;
	case 'R':
		printf("Asking NVIC to reset us\n\r");
		USBD_Disconnect();
		NVIC_SystemReset();
		break;
	case 'X':
		printf("Clearing _SIMTRACExx_RST -> SIMTRACExx_RST high (inactive)\n\r");
		PIO_Clear(&pin_peer_rst);
		break;
	case 'x':
		printf("Setting _SIMTRACExx_RST -> SIMTRACExx_RST low (active)\n\r");
		PIO_Set(&pin_peer_rst);
		break;
	case 'Y':
		printf("Clearing SIMTRACExx_ERASE (inactive)\n\r");
		PIO_Clear(&pin_peer_erase);
		break;
	case 'y':
		printf("Seetting SIMTRACExx_ERASE (active)\n\r");
		PIO_Set(&pin_peer_erase);
		break;
	case '1':
		printf("Resetting Modem 1 (of this SAM3)\n\r");
		wwan_perst_do_reset_pulse(0, 300);
		break;
	case '2':
		printf("Resetting Modem 2 (of this SAM3)\n\r");
		wwan_perst_do_reset_pulse(1, 300);
		break;
	case '!':
		sim_switch_use_physical(0, 0);
		break;
	case '@':
		sim_switch_use_physical(0, 0);
		break;
	default:
		if (!qmod_sam3_is_12())
			printf("Unknown command '%c'\n\r", ch);
		else
			board_exec_dbg_cmd_st12only(ch);
		break;
	}
}

void board_main_top(void)
{
#ifndef APPLICATION_dfu
	usb_buf_init();

	wwan_led_init();
	wwan_perst_init();
	sim_switch_init();
#endif

	/* make sure we can detect whether running in ST12 or ST34 */
	PIO_Configure(&pin_1234_detect, 1);

	if (qmod_sam3_is_12()) {
		/* set PIN_PRTPWR_OVERRIDE to output-low to avoid the internal
		* pull-up on the input to keep SIMTRACE12 alive */
		PIO_Configure(&pin_hubpwr_override, 1);
		PIO_Configure(&pin_hub_rst, 1);
	}
	PIO_Configure(&pin_peer_rst, 1);
	PIO_Configure(&pin_peer_erase, 1);

#ifndef APPLICATION_dfu
	i2c_pin_init();
#endif

	if (qmod_sam3_is_12()) {
		TRACE_INFO("Detected Quad-Modem ST12\n\r");
	} else {
		TRACE_INFO("Detected Quad-Modem ST34\n\r");
		/* make sure we use the second set of USB Strings
		 * calling the interfaces "Modem 3" and "Modem 4" rather
		 * than 1+2 */
		usb_strings[7] = usb_strings[9];
		usb_strings[8] = usb_strings[10];
	}

	/* Obtain the circuit board version (currently just prints voltage */
	get_board_version_adc();
#ifndef APPLICATION_dfu
	/* Initialize checking for card insert/remove events */
	card_present_init();
#endif
}

static int uart_has_loopback_jumper(void)
{
	unsigned int i;
	const Pin uart_loopback_pins[] = {
		{PIO_PA9A_URXD0, PIOA, ID_PIOA, PIO_INPUT, PIO_DEFAULT},
		{PIO_PA10A_UTXD0, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT}
	};

	/* Configure UART pins as I/O */
	PIO_Configure(uart_loopback_pins, PIO_LISTSIZE(uart_loopback_pins));

	/* Send pattern over UART TX and check if it is received on RX
	 * If the loop doesn't get interrupted, RxD always follows TxD and thus a
	 * loopback jumper has been placed on RxD/TxD, and we will boot
	 * into DFU unconditionally
	 */
	int has_loopback_jumper = 1;
	for (i = 0; i < 10; i++) {
		/* Set TxD high; abort if RxD doesn't go high either */
		PIO_Set(&uart_loopback_pins[1]);
		if (!PIO_Get(&uart_loopback_pins[0])) {
			has_loopback_jumper = 0;
			break;
		}
		/* Set TxD low, abort if RxD doesn't go low either */
		PIO_Clear(&uart_loopback_pins[1]);
		if (PIO_Get(&uart_loopback_pins[0])) {
			has_loopback_jumper = 0;
			break;
		}
	}

	/* Put pins back to UART mode */
	const Pin uart_pins[] = {PINS_UART};
	PIO_Configure(uart_pins, PIO_LISTSIZE(uart_pins));

	return has_loopback_jumper;
}

int board_override_enter_dfu(void)
{
	/* If the loopback jumper is set, we enter DFU mode */
	if (uart_has_loopback_jumper())
		return 1;

	return 0;
}
