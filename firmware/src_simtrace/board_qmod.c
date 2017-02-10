/* Quad-modem speciic application code */
/* (C) 2016-2016 by Harald Welte <laforge@gnumonks.org> */

#include "board.h"
#include "simtrace.h"
#include "utils.h"
#include "req_ctx.h"
#include "wwan_led.h"
#include "wwan_perst.h"
#include "boardver_adc.h"
#include "osmocom/core/timer.h"

static const Pin pin_hubpwr_override = PIN_PRTPWR_OVERRIDE;
static const Pin pin_hub_rst = {PIO_PA13, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
static const Pin pin_1234_detect = {PIO_PA14, PIOA, ID_PIOA, PIO_INPUT, PIO_PULLUP};
static const Pin pin_peer_rst = {PIO_PA0, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
static const Pin pin_peer_erase = {PIO_PA11, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};


static int qmod_sam3_is_12(void)
{
	if (PIO_Get(&pin_1234_detect) == 0)
		return 1;
	else
		return 0;
}

const unsigned char __eeprom_bin[256] = {
	0x23, 0x42, 0x17, 0x25, 0x00, 0x00, 0x9b, 0x20, 0x01, 0x00, 0x00, 0x00, 0x32, 0x32, 0x32, 0x32, /* 0x00 - 0x0f */
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
	volatile int v;
	/* 440ns per cycle here */
	for (i = 0; i < 1000000; i++) {
		v = 0;
	}

	TRACE_INFO("Writing EEPROM...\r\n");
	/* write the EEPROM once */
	for (i = 0; i < 256; i++) {
		int rc = eeprom_write_byte(0x50, i, __eeprom_bin[i]);
		/* if the result was negative, repeat that write */
		if (rc < 0)
			i--;
	}

	/* then pursue re-reading it again and again */
	TRACE_INFO("Verifying EEPROM...\r\n");
	for (i = 0; i < 256; i++) {
		int byte = eeprom_read_byte(0x50, i);
		TRACE_INFO("0x%02x: %02x\r\n", i, byte);
		if (byte != __eeprom_bin[i])
			TRACE_ERROR("Byte %u is wrong, expected 0x%02x, found 0x%02x\r\n",
					i, __eeprom_bin[i], byte);
	}

	/* FIXME: Release PIN_PRTPWR_OVERRIDE after we know the hub is
	 * again powering us up */

	return 0;
}

/* returns '1' in case we should break any endless loop */
void board_exec_dbg_cmd(int ch)
{
	uint32_t addr, val;

	switch (ch) {
	case '?':
		printf("\t?\thelp\r\n");
		printf("\tE\tprogram EEPROM\r\n");
		printf("\tR\treset SAM3\r\n");
		printf("\tO\tEnable PRTPWR_OVERRIDE\r\n");
		printf("\to\tDisable PRTPWR_OVERRIDE\r\n");
		printf("\tH\tRelease HUB RESET (high)\r\n");
		printf("\th\tAssert HUB RESET (low)\r\n");
		printf("\tw\tWrite single byte in EEPROM\r\n");
		printf("\tr\tRead single byte from EEPROM\r\n");
		printf("\tX\tRelease peer SAM3 from reset\r\n");
		printf("\tx\tAssert peer SAM3 reset\r\n");
		printf("\tY\tRelease peer SAM3 ERASE signal\r\n");
		printf("\ty\tAssert peer SAM3 ERASE signal\r\n");
		printf("\tU\tProceed to USB Initialization\r\n");
		printf("\t1\tGenerate 1ms reset pulse on WWAN1\r\n");
		printf("\t2\tGenerate 1ms reset pulse on WWAN2\r\n");
		break;
	case 'E':
		write_hub_eeprom();
		break;
	case 'R':
		printf("Asking NVIC to reset us\r\n");
		NVIC_SystemReset();
		break;
	case 'O':
		printf("Setting PRTPWR_OVERRIDE\r\n");
		PIO_Set(&pin_hubpwr_override);
		break;
	case 'o':
		printf("Clearing PRTPWR_OVERRIDE\r\n");
		PIO_Clear(&pin_hubpwr_override);
		break;
	case 'H':
		printf("Clearing _HUB_RESET -> HUB_RESET high (inactive)\r\n");
		PIO_Clear(&pin_hub_rst);
		break;
	case 'h':
		/* high level drives transistor -> HUB_RESET low */
		printf("Asserting _HUB_RESET -> HUB_RESET low (active)\r\n");
		PIO_Set(&pin_hub_rst);
		break;
	case 'w':
		if (PIO_GetOutputDataStatus(&pin_hub_rst) == 0)
			printf("WARNING: attempting EEPROM access while HUB not in reset\r\n");
		printf("Please enter EEPROM offset:\r\n");
		UART_GetIntegerMinMax(&addr, 0, 255);
		printf("Please enter EEPROM value:\r\n");
		UART_GetIntegerMinMax(&val, 0, 255);
		printf("Writing value 0x%02x to EEPROM offset 0x%02x\r\n", val, addr);
		eeprom_write_byte(0x50, addr, val);
		break;
	case 'r':
		printf("Please enter EEPROM offset:\r\n");
		UART_GetIntegerMinMax(&addr, 0, 255);
		printf("EEPROM[0x%02x] = 0x%02x\r\n", addr, eeprom_read_byte(0x50, addr));
		break;
	case 'X':
		printf("Clearing _SIMTRACExx_RST -> SIMTRACExx_RST high (inactive)\r\n");
		PIO_Clear(&pin_peer_rst);
		break;
	case 'x':
		printf("Setting _SIMTRACExx_RST -> SIMTRACExx_RST low (active)\r\n");
		PIO_Set(&pin_peer_rst);
		break;
	case 'Y':
		printf("Clearing SIMTRACExx_ERASE (inactive)\r\n");
		PIO_Clear(&pin_peer_erase);
		break;
	case 'y':
		printf("Seetting SIMTRACExx_ERASE (active)\r\n");
		PIO_Set(&pin_peer_erase);
		break;
	case '1':
		printf("Resetting Modem 1 (of this SAM3)\r\n");
		wwan_perst_do_reset(1);
		break;
	case '2':
		printf("Resetting Modem 2 (of this SAM3)\r\n");
		wwan_perst_do_reset(2);
		break;
	default:
		printf("Unknown command '%c'\r\n", ch);
		break;
	}
}

void board_main_top(void)
{
	wwan_led_init();
	wwan_perst_init();

	/* set PIN_PRTPWR_OVERRIDE to output-low to avoid the internal
	 * pull-up on the input to keep SIMTRACE12 alive */
	PIO_Configure(&pin_hubpwr_override, 1);
	PIO_Configure(&pin_hub_rst, 1);
	PIO_Configure(&pin_1234_detect, 1);
	PIO_Configure(&pin_peer_rst, 1);
	PIO_Configure(&pin_peer_erase, 1);
	i2c_pin_init();

	if (qmod_sam3_is_12()) {
		TRACE_INFO("Detected Quad-Modem ST12\r\n");
	} else {
		TRACE_INFO("Detected Quad-Modem ST34\r\n");
	}

	/* Obtain the circuit board version (currently just prints voltage */
	get_board_version_adc();
}
