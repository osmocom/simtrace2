#include "board.h"
#include "utils.h"
#include "usb/device/dfu/dfu.h"
#include "usb/common/dfu/usb_dfu.h"
#include "manifest.h"

#define ALTIF_RAM 0
#define ALTIF_FLASH 1

unsigned int g_unique_id[4];

/*----------------------------------------------------------------------------
 *       Callbacks
 *----------------------------------------------------------------------------*/

#define RAM_ADDR(offset) (IRAM_ADDR + BOARD_DFU_RAM_SIZE + offset)
#define FLASH_ADDR(offset) (IFLASH_ADDR + BOARD_DFU_BOOT_SIZE + offset)

#define IFLASH_END	((uint8_t *)IFLASH_ADDR + IFLASH_SIZE)
#define IRAM_END	((uint8_t *)IRAM_ADDR + IRAM_SIZE)

/* incoming call-back: Host has transfered 'len' bytes (stored at
 * 'data'), which we shall write to 'offset' into the partition
 * associated with 'altif'.  Guaranted to be les than
 * BOARD_DFU_PAGE_SIZE */
int USBDFU_handle_dnload(uint8_t altif, unsigned int offset,
			 uint8_t *data, unsigned int len)
{
	uint32_t addr;
	int rc;

	printf("dnload(altif=%u, offset=%u, len=%u\n\r", altif, offset, len);

	switch (altif) {
	case ALTIF_RAM:
		addr = RAM_ADDR(offset);
		if (addr > IRAM_ADDR + IRAM_SIZE) {
			g_dfu.state = DFU_STATE_dfuERROR;
			g_dfu.status = DFU_STATUS_errADDRESS;
			return DFU_RET_STALL;
		}
		memcpy((void *)addr, data, len);
		return DFU_RET_ZLP;
	case ALTIF_FLASH:
		addr = FLASH_ADDR(offset);
		if (addr > IFLASH_ADDR + IFLASH_SIZE) {
			g_dfu.state = DFU_STATE_dfuERROR;
			g_dfu.status = DFU_STATUS_errADDRESS;
			return DFU_RET_STALL;
		}
		rc = FLASHD_Write(addr, data, len);
		if (rc != 0) {
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
			g_dfu.state = DFU_STATE_dfuERROR;
			g_dfu.status = DFU_STATUS_errADDRESS;
			return -1;
		}
		if ((uint8_t *)addr + req_len > IRAM_END)
			req_len = IRAM_END - (uint8_t *)addr;
		memcpy(data, (void *)addr, req_len);
		break;
	case ALTIF_FLASH:
		addr = FLASH_ADDR(offset);
		if (addr > IFLASH_ADDR + IFLASH_SIZE) {
			g_dfu.state = DFU_STATE_dfuERROR;
			g_dfu.status = DFU_STATUS_errADDRESS;
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
	printf("=%u\r\n", req_len);
	return req_len;
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
#if 1
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
