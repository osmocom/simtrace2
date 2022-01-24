/* SIMtrace 2 firmware USB DFU bootloader
 *
 * (C) 2015-2017 by Harald Welte <hwelte@hmw-consulting.de>
 * (C) 2018-2019 by sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "board.h"
#include "utils.h"
#include "usb/device/dfu/dfu.h"
#include "usb/common/dfu/usb_dfu.h"
#include "manifest.h"
#include "USBD_HAL.h"

#include <osmocom/core/timer.h>

/* USB alternate interface index used to identify which partition to flash */
/** USB alternate interface index indicating RAM partition */
#define ALTIF_RAM 0
/** USB alternate interface index indicating flash partition */
#if defined(ENVIRONMENT_flash)
#define ALTIF_FLASH 1
#elif defined(ENVIRONMENT_dfu)
#define ALTIF_FLASH 2
#endif

unsigned int g_unique_id[4];
/* remember if the watchdog has been configured in the main loop so we can kick it in the ISR */
static bool watchdog_configured = false;

/* There is not enough space in the 16 KiB DFU bootloader to include led.h functions */
#ifdef PINS_LEDS
/** LED pin configurations */
static const Pin pinsLeds[] = { PINS_LEDS } ;
#endif

/*----------------------------------------------------------------------------
 *       Callbacks
 *----------------------------------------------------------------------------*/

#define RAM_ADDR(offset) (IRAM_ADDR + BOARD_DFU_RAM_SIZE + offset)
#if defined(ENVIRONMENT_flash)
#define FLASH_ADDR(offset) (IFLASH_ADDR + BOARD_DFU_BOOT_SIZE + offset)
#elif defined(ENVIRONMENT_dfu)
#define FLASH_ADDR(offset) (IFLASH_ADDR + offset)
#endif

#define IRAM_END ((uint8_t *)IRAM_ADDR + IRAM_SIZE)
#if defined(ENVIRONMENT_flash)
#define IFLASH_END ((uint8_t *)IFLASH_ADDR + IFLASH_SIZE)
#elif defined(ENVIRONMENT_dfu)
#define IFLASH_END ((uint8_t *)IFLASH_ADDR + BOARD_DFU_BOOT_SIZE)
#endif

/* incoming call-back: Host has transferred 'len' bytes (stored at
 * 'data'), which we shall write to 'offset' into the partition
 * associated with 'altif'.  Guaranted to be less than
 * BOARD_DFU_PAGE_SIZE */
int USBDFU_handle_dnload(uint8_t altif, unsigned int offset,
			 uint8_t *data, unsigned int len)
{
	uint32_t addr;
	unsigned int i;
	int rc;
	/* address of the last allocated variable on the stack */
	uint32_t stack_addr = (uint32_t)&rc;
	/* kick the dog to have enough time to flash */
	if (watchdog_configured) {
		WDT_Restart(WDT);
	}

#if TRACE_LEVEL >= TRACE_LEVEL_INFO
	TRACE_INFO("dnload(altif=%u, offset=%u, len=%u)\r\n", altif, offset, len);
#else
	printf("DL off=%u\r\n", offset);
#endif

#ifdef PINS_LEDS
	PIO_Clear(&pinsLeds[LED_NUM_RED]);
#endif

	switch (altif) {
	case ALTIF_RAM:
		addr = RAM_ADDR(offset);
		if (addr < IRAM_ADDR || addr + len >= IRAM_ADDR + IRAM_SIZE || addr + len >= stack_addr) {
			g_dfu->state = DFU_STATE_dfuERROR;
			g_dfu->status = DFU_STATUS_errADDRESS;
			rc = DFU_RET_STALL;
			break;
		}
		memcpy((void *)addr, data, len);
		rc = DFU_RET_ZLP;
		break;
	case ALTIF_FLASH:
		addr = FLASH_ADDR(offset);
#if defined(ENVIRONMENT_flash)
		if (addr < IFLASH_ADDR || addr + len >= IFLASH_ADDR + IFLASH_SIZE) {
#elif defined(ENVIRONMENT_dfu)
		if (addr < IFLASH_ADDR || addr + len >= IFLASH_ADDR + BOARD_DFU_BOOT_SIZE) {
#endif
			g_dfu->state = DFU_STATE_dfuERROR;
			g_dfu->status = DFU_STATUS_errADDRESS;
			rc = DFU_RET_STALL;
			break;
		}
		rc = FLASHD_Unlock(addr, addr + len, 0, 0);
		if (rc != 0) {
			TRACE_ERROR("DFU download flash unlock failed\r\n");
			rc =  DFU_RET_STALL;
			break;
		}
		rc = FLASHD_Write(addr, data, len);
		if (rc != 0) {
			TRACE_ERROR("DFU download flash erase failed\r\n");
			rc = DFU_RET_STALL;
			break;
		}
		for (i = 0; i < len; i++) {
			if (((uint8_t*)addr)[i]!=data[i]) {
				TRACE_ERROR("DFU download flash data written not correct\r\n");
				rc = DFU_RET_STALL;
				break;
			}
		}
		rc = DFU_RET_ZLP;
		break;
	default:
		TRACE_ERROR("DFU download for unknown AltIf %d\r\n", altif);
		rc = DFU_RET_STALL;
		break;
	}

#ifdef PINS_LEDS
	PIO_Set(&pinsLeds[LED_NUM_RED]);
#endif

	return rc;
}

/* incoming call-back: Host has requested to read back 'req_len' bytes
 * starting from 'offset' of the firmware * associated with partition
 * 'altif' */
int USBDFU_handle_upload(uint8_t altif, unsigned int offset,
			 uint8_t *data, unsigned int req_len)
{
	uint32_t addr;

	printf("upload(altif=%u, offset=%u, len=%u)", altif, offset, req_len);

	switch (altif) {
	case ALTIF_RAM:
		addr = RAM_ADDR(offset);
		if (addr > IRAM_ADDR + IRAM_SIZE) {
			g_dfu->state = DFU_STATE_dfuERROR;
			g_dfu->status = DFU_STATUS_errADDRESS;
			return -1;
		}
		if ((uint8_t *)addr + req_len > IRAM_END)
			req_len = IRAM_END - (uint8_t *)addr;
		memcpy(data, (void *)addr, req_len);
		break;
	case ALTIF_FLASH:
		addr = FLASH_ADDR(offset);
		if (addr > IFLASH_ADDR + IFLASH_SIZE) {
			g_dfu->state = DFU_STATE_dfuERROR;
			g_dfu->status = DFU_STATUS_errADDRESS;
			return -1;
		}
		if ((uint8_t *)addr + req_len > IFLASH_END)
			req_len = IFLASH_END - (uint8_t *)addr;
		memcpy(data, (void *)addr, req_len);
		break;
	default:
		TRACE_ERROR("DFU upload for unknown AltIf %d\r\n", altif);
		/* FIXME: set error codes */
		return -1;
	}
	printf("=%u\r\n", req_len);
	return req_len;
}

/* can be overridden by board specific code, e.g. by pushbutton */
WEAK int board_override_enter_dfu(void)
{
	return 0;
}

/* using this function we can determine if we should enter DFU mode
 * during boot, or if we should proceed towards the application/runtime */
int USBDFU_OverrideEnterDFU(void)
{
	uint32_t *app_part = (uint32_t *)FLASH_ADDR(0);
	/* at the first call we are before the text segment has been relocated,
	 * so g_dfu is not initialized yet */
	g_dfu = &_g_dfu;
	if (USB_DFU_MAGIC == g_dfu->magic) {
		return 1;
	}

	/* If the loopback jumper is set, we enter DFU mode */
	if (board_override_enter_dfu()) {
		return 2;
	}

	/* if the first word of the application partition doesn't look
	 * like a stack pointer (i.e. point to RAM), enter DFU mode */
	if ((app_part[0] < IRAM_ADDR) || ((uint8_t *)app_part[0] > IRAM_END)) {
		return 3;
	}

	/* if the second word of the application partition doesn't look
	 * like a function from flash (reset vector), enter DFU mode */
	if (((uint32_t *)app_part[1] < app_part) ||
	    ((uint8_t *)app_part[1] > IFLASH_END)) {
		return 4;
	}

	return 0;
}

/* returns '1' in case we should break any endless loop */
static void check_exec_dbg_cmd(void)
{
	int ch;

	if (!UART_IsRxReady())
		return;

	ch = UART_GetChar();

	//board_exec_dbg_cmd(ch);
}

/* print a horizontal line of '=' characters; Doing this in a loop vs. using a 'const'
 * string saves us ~60 bytes of executable size (matters particularly for DFU loader) */
static void print_line(void)
{
	int i;
	for (i = 0; i < 78; i++)
		fputc('=', stdout);
	fputc('\n', stdout);
	fputc('\r', stdout);
}

/*------------------------------------------------------------------------------
 *        Main
 *------------------------------------------------------------------------------*/
#define MAX_USB_ITER BOARD_MCK/72	// This should be around a second
extern int main(void)
{
	uint8_t isUsbConnected = 0;
	unsigned int i = 0;
	uint32_t reset_cause = (RSTC->RSTC_SR & RSTC_SR_RSTTYP_Msk) >> RSTC_SR_RSTTYP_Pos;

	/* Enable watchdog for 2000ms, with no window */
	WDT_Enable(WDT, WDT_MR_WDRSTEN | WDT_MR_WDDBGHLT | WDT_MR_WDIDLEHLT |
		   (WDT_GetPeriod(2000) << 16) | WDT_GetPeriod(2000));
	watchdog_configured = true;

#ifdef PINS_LEDS
	/* Configure LED */
	PIO_Configure(pinsLeds, PIO_LISTSIZE(pinsLeds));
	PIO_Set(&pinsLeds[LED_NUM_RED]);
	PIO_Clear(&pinsLeds[LED_NUM_GREEN]);
#endif

	EEFC_ReadUniqueID(g_unique_id);

	printf("\r\n\r\n");
	print_line();
	printf("DFU bootloader %s for board %s\r\n"
		"(C) 2010-2017 by Harald Welte, 2018-2019 by Kevin Redon\r\n",
		manifest_revision, manifest_board);
	print_line();

#if (TRACE_LEVEL >= TRACE_LEVEL_INFO)
	TRACE_INFO("Chip ID: 0x%08lx (Ext 0x%08lx)\r\n", CHIPID->CHIPID_CIDR, CHIPID->CHIPID_EXID);
	TRACE_INFO("Serial Nr. %08x-%08x-%08x-%08x\r\n",
		   g_unique_id[0], g_unique_id[1],
		   g_unique_id[2], g_unique_id[3]);
	static const char* reset_causes[] = {
		"general reset (first power-up reset)",
		"backup reset (return from backup mode)",
		"watchdog reset (watchdog fault occurred)",
		"software reset (processor reset required by the software)",
		"user reset (NRST pin detected low)",
	};
	if (reset_cause < ARRAY_SIZE(reset_causes)) {
		TRACE_INFO("Reset Cause: %s\r\n", reset_causes[reset_cause]);
	} else {
		TRACE_INFO("Reset Cause: 0x%lx\r\n", (RSTC->RSTC_SR & RSTC_SR_RSTTYP_Msk) >> RSTC_SR_RSTTYP_Pos);
	}
#endif

#if (TRACE_LEVEL >= TRACE_LEVEL_INFO)
	/* Find out why we are in the DFU bootloader, and not the main application */
	TRACE_INFO("DFU bootloader start reason: ");
	switch (USBDFU_OverrideEnterDFU()) {
	case 0:
		if (SCB->VTOR < IFLASH_ADDR + BOARD_DFU_BOOT_SIZE) {
			TRACE_INFO_WP("unknown\r\n");
		} else {
			TRACE_INFO_WP("DFU is the main application\r\n");
		}
		break;
	case 1:
		TRACE_INFO_WP("DFU switch requested by main application\r\n");
		break;
	case 2:
		TRACE_INFO_WP("bootloader forced (button pressed or jumper set)\r\n");
		break;
	case 3:
		TRACE_INFO_WP("stack pointer (first application word) does no point in RAM\r\n");
		break;
	case 4: // the is no reason
		TRACE_INFO_WP("reset vector (second application word) does no point in flash\r\n");
		break;
	default:
		TRACE_INFO_WP("unknown\r\n");
		break;
	}
#endif

	/* clear g_dfu on power-up reset */
	if (reset_cause == 0)
		memset(g_dfu, 0, sizeof(*g_dfu));

	board_main_top();

	TRACE_INFO("USB init...\r\n");
	/* Signal USB reset by disabling the pull-up on USB D+ for at least 10 ms */
	USBD_Disconnect();
	mdelay(500);
	USBDFU_Initialize(&dfu_descriptors);

	while (USBD_GetState() < USBD_STATE_CONFIGURED) {
		WDT_Restart(WDT);
		check_exec_dbg_cmd();
#if 1
		if (i >= MAX_USB_ITER * 3) {
			TRACE_ERROR("Resetting board (USB could not be configured)\r\n");
			g_dfu->magic = USB_DFU_MAGIC; // start the bootloader after reboot
			USBD_Disconnect();
			NVIC_SystemReset();
		}
#endif
		i++;
	}

	/* Initialize the flash to be able to write it, using the IAP ROM code */
	FLASHD_Initialize(BOARD_MCK, 1);

	TRACE_INFO("entering main loop...\r\n");
	while (1) {
		WDT_Restart(WDT);
#if TRACE_LEVEL >= TRACE_LEVEL_DEBUG
		const char rotor[] = { '-', '\\', '|', '/' };
		putchar('\b');
		putchar(rotor[i++ % ARRAY_SIZE(rotor)]);
#endif
		check_exec_dbg_cmd();
#if 0
		osmo_timers_prepare();
		osmo_timers_update();
#endif

		if (USBD_GetState() < USBD_STATE_CONFIGURED) {

			if (isUsbConnected) {
				isUsbConnected = 0;
			}
		} else if (isUsbConnected == 0) {
			TRACE_INFO("USB is now configured\r\n");

			isUsbConnected = 1;
		}
	}
}
