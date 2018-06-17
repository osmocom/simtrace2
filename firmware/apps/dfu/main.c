#include "board.h"
#include "utils.h"
#include "usb/device/dfu/dfu.h"
#include "usb/common/dfu/usb_dfu.h"
#include "manifest.h"

#include <osmocom/core/timer.h>

#define ALTIF_RAM 0
#define ALTIF_FLASH 1

unsigned int g_unique_id[4];
/* remember if the watchdog has been configured in the main loop so we can kick it in the ISR */
static bool watchdog_configured = false;

/*----------------------------------------------------------------------------
 *       Callbacks
 *----------------------------------------------------------------------------*/

#define RAM_ADDR(offset) (IRAM_ADDR + BOARD_DFU_RAM_SIZE + offset)
#define FLASH_ADDR(offset) (IFLASH_ADDR + BOARD_DFU_BOOT_SIZE + offset)

#define IFLASH_END	((uint8_t *)IFLASH_ADDR + IFLASH_SIZE)
#define IRAM_END	((uint8_t *)IRAM_ADDR + IRAM_SIZE)

/* incoming call-back: Host has transferred 'len' bytes (stored at
 * 'data'), which we shall write to 'offset' into the partition
 * associated with 'altif'.  Guaranted to be less than
 * BOARD_DFU_PAGE_SIZE */
int USBDFU_handle_dnload(uint8_t altif, unsigned int offset,
			 uint8_t *data, unsigned int len)
{
	uint32_t addr;
	int rc;
	/* address of the last allocated variable on the stack */
	uint32_t stack_addr = (uint32_t)&rc;
	/* kick the dog to have enough time to flash */
	if (watchdog_configured) {
		WDT_Restart(WDT);
	}

	printf("dnload(altif=%u, offset=%u, len=%u)\n\r", altif, offset, len);

	switch (altif) {
	case ALTIF_RAM:
		addr = RAM_ADDR(offset);
		if (addr < IRAM_ADDR || addr + len >= IRAM_ADDR + IRAM_SIZE || addr + len >= stack_addr) {
			g_dfu->state = DFU_STATE_dfuERROR;
			g_dfu->status = DFU_STATUS_errADDRESS;
			return DFU_RET_STALL;
		}
		memcpy((void *)addr, data, len);
		return DFU_RET_ZLP;
	case ALTIF_FLASH:
		addr = FLASH_ADDR(offset);
		if (addr < IFLASH_ADDR || addr + len >= IFLASH_ADDR + IFLASH_SIZE) {
			g_dfu->state = DFU_STATE_dfuERROR;
			g_dfu->status = DFU_STATUS_errADDRESS;
			return DFU_RET_STALL;
		}
		rc = FLASHD_Unlock(addr, addr + len, 0, 0);
		if (rc != 0) {
			TRACE_ERROR("DFU download flash unlock failed\n\r");
			/* FIXME: set error codes */
			return DFU_RET_STALL;
		}
		rc = FLASHD_Write(addr, data, len);
		if (rc != 0) {
			TRACE_ERROR("DFU download flash erase failed\n\r");
			/* FIXME: set error codes */
			return DFU_RET_STALL;
		}
		for (unsigned int i=0; i<len; i++) {
			if (((uint8_t*)addr)[i]!=data[i]) {
				TRACE_ERROR("DFU download flash data written not correct\n\r");
				return DFU_RET_STALL;
			}
		}
		rc = FLASHD_Lock(addr, addr + len, 0, 0);
		if (rc != 0) {
			TRACE_ERROR("DFU download flash lock failed\n\r");
			/* FIXME: set error codes */
			return DFU_RET_STALL;
		}
		return DFU_RET_ZLP;
	default:
		/* FIXME: set error codes */
		TRACE_ERROR("DFU download for unknown AltIf %d\n\r", altif);
		return DFU_RET_STALL;
	}
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
		TRACE_ERROR("DFU upload for unknown AltIf %d\n\r", altif);
		/* FIXME: set error codes */
		return -1;
	}
	printf("=%u\n\r", req_len);
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

	/* If the loopback jumper is set, we enter DFU mode */
	if (board_override_enter_dfu())
		return 1;

	/* if the first word of the application partition doesn't look
	 * like a stack pointer (i.e. point to RAM), enter DFU mode */
	if ((app_part[0] < IRAM_ADDR) ||
	    ((uint8_t *)app_part[0] > IRAM_END))
		return 1;

	/* if the second word of the application partition doesn't look
	 * like a function from flash (reset vector), enter DFU mode */
	if (((uint32_t *)app_part[1] < app_part) ||
	    ((uint8_t *)app_part[1] > IFLASH_END))
		return 1;

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

/*------------------------------------------------------------------------------
 *        Main
 *------------------------------------------------------------------------------*/
#define MAX_USB_ITER BOARD_MCK/72	// This should be around a second
extern int main(void)
{
	uint8_t isUsbConnected = 0;
	unsigned int i = 0;
	uint32_t reset_cause = (RSTC->RSTC_SR & RSTC_SR_RSTTYP_Msk) >> RSTC_SR_RSTTYP_Pos;

#if 0
	led_init();
	led_blink(LED_GREEN, BLINK_3O_30F);
	led_blink(LED_RED, BLINK_3O_30F);
#endif

	/* Enable watchdog for 2000ms, with no window */
	WDT_Enable(WDT, WDT_MR_WDRSTEN | WDT_MR_WDDBGHLT | WDT_MR_WDIDLEHLT |
		   (WDT_GetPeriod(2000) << 16) | WDT_GetPeriod(2000));
	watchdog_configured = true;

	PIO_InitializeInterrupts(0);

	EEFC_ReadUniqueID(g_unique_id);

        printf("\n\r\n\r"
		"=============================================================================\n\r"
		"DFU bootloader %s for board %s (C) 2010-2017 by Harald Welte\n\r"
		"=============================================================================\n\r",
		manifest_revision, manifest_board);

	TRACE_INFO("Chip ID: 0x%08x (Ext 0x%08x)\n\r", CHIPID->CHIPID_CIDR, CHIPID->CHIPID_EXID);
	TRACE_INFO("Serial Nr. %08x-%08x-%08x-%08x\n\r",
		   g_unique_id[0], g_unique_id[1],
		   g_unique_id[2], g_unique_id[3]);
	TRACE_INFO("Reset Cause: 0x%x\n\r", reset_cause);

	/* clear g_dfu on power-up reset */
	if (reset_cause == 0)
		memset(g_dfu, 0, sizeof(*g_dfu));

	board_main_top();

	TRACE_INFO("USB init...\n\r");
	/* Signal USB reset by disabling the pull-up on USB D+ for at least 10 ms */
	const Pin usb_dp_pullup = PIN_USB_PULLUP;
	PIO_Configure(&usb_dp_pullup, 1);
	PIO_Set(&usb_dp_pullup);
	mdelay(15);
	PIO_Clear(&usb_dp_pullup);
	USBDFU_Initialize(&dfu_descriptors);

	while (USBD_GetState() < USBD_STATE_CONFIGURED) {
		WDT_Restart(WDT);
		check_exec_dbg_cmd();
#if 1
		if (i >= MAX_USB_ITER * 3) {
			TRACE_ERROR("Resetting board (USB could "
				    "not be configured)\n\r");
			USBD_Disconnect();
			NVIC_SystemReset();
		}
#endif
		i++;
	}

	/* Initialize the flash to be able to write it, using the IAP ROM code */
	FLASHD_Initialize(BOARD_MCK, 1);

	TRACE_INFO("entering main loop...\n\r");
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
			TRACE_INFO("USB is now configured\n\r");

			isUsbConnected = 1;
		}
	}
}
