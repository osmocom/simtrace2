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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "board.h"
#include "core_cm3.h"
#include "flashd.h"
#include "utils.h"
#include "usb/device/dfu/dfu.h"
#include "usb/common/dfu/usb_dfu.h"
#include "manifest.h"
#include "USBD_HAL.h"

#include <osmocom/core/timer.h>

/* actual section content must be replaced with the padded bootloader by running objcopy! */
const uint32_t bl_update_data[BOARD_DFU_BOOT_SIZE / sizeof(uint32_t)] __attribute__((section(".fwupdate"))) = { 0xFF };

unsigned int g_unique_id[4];
/* remember if the watchdog has been configured in the main loop so we can kick it in the ISR */
static bool watchdog_configured = false;

extern uint32_t _end;
extern uint32_t _srelocate;
extern uint32_t _etext;

void DFURT_SwitchToDFU(void)
{
}
void USBDFU_Runtime_RequestHandler(const USBGenericRequest *request)
{
}
int USBDFU_handle_dnload(uint8_t altif, unsigned int offset, uint8_t *data, unsigned int len)
{
	return 0;
}
int USBDFU_handle_upload(uint8_t altif, unsigned int offset, uint8_t *data, unsigned int req_len)
{
	return 0;
}
int USBDFU_OverrideEnterDFU(void)
{
	return 0;
}

__attribute__((section(".ramfunc"), noinline)) static uint32_t flash_wait_ready()
{
	Efc *efc = EFC;
	uint32_t dwStatus;

	do {
		dwStatus = efc->EEFC_FSR;
	} while ((dwStatus & EEFC_FSR_FRDY) != EEFC_FSR_FRDY);
	return (dwStatus & (EEFC_FSR_FLOCKE | EEFC_FSR_FCMDE));
}

__attribute__((section(".ramfunc"), noinline)) static void flash_cmd(uint32_t dwCommand, uint32_t dwArgument)
{
	Efc *efc = EFC;
	uint32_t dwStatus;
	efc->EEFC_FCR = EEFC_FCR_FKEY(0x5A) | EEFC_FCR_FARG(dwArgument) | EEFC_FCR_FCMD(dwCommand);
}

__attribute__((section(".ramfunc"), noinline, noreturn)) static void erase_first_app_sector()
{
	/* page 64 */
	uint32_t first_app_page = (BOARD_DFU_BOOT_SIZE / IFLASH_PAGE_SIZE);
	uint32_t *first_app_address = (uint32_t *)(IFLASH_ADDR + first_app_page * IFLASH_PAGE_SIZE + 0);

#if 1
	/* overwrite first app sector so we don't keep booting this */
	for (int i = 0; i < IFLASH_PAGE_SIZE / 4; i++)
		first_app_address[i] = 0xffffffff;

	flash_cmd(EFC_FCMD_EWP, first_app_page);
#else
	/* why does erasing the whole flash with a protected bootloader not work at all? */
	flash_cmd(EFC_FCMD_EA, 0);
#endif
	flash_wait_ready();
	for (;;)
		NVIC_SystemReset();
}

#define MAX_USB_ITER BOARD_MCK / 72 // This should be around a second
extern int main(void)
{
	uint8_t isUsbConnected = 0;
	unsigned int i = 0;
	uint32_t reset_cause = (RSTC->RSTC_SR & RSTC_SR_RSTTYP_Msk) >> RSTC_SR_RSTTYP_Pos;

	/* Enable watchdog for 2000ms, with no window */
	WDT_Enable(WDT, WDT_MR_WDRSTEN | WDT_MR_WDDBGHLT | WDT_MR_WDIDLEHLT | (WDT_GetPeriod(2000) << 16) |
				WDT_GetPeriod(2000));
	watchdog_configured = true;

	EEFC_ReadUniqueID(g_unique_id);

	printf("\n\r\n\r");
	printf("bootloader updater %s for board %s\n\r"
	       "(C) 2010-2017 by Harald Welte, 2018-2019 by Kevin Redon\n\r",
	       manifest_revision, manifest_board);

	/* clear g_dfu on power-up reset */
	memset(g_dfu, 0, sizeof(*g_dfu));

	TRACE_INFO("USB init...\n\r");
	/* Signal USB reset by disabling the pull-up on USB D+ for at least 10 ms */
	USBD_Disconnect();

	/* Initialize the flash to be able to write it, using the IAP ROM code */
	FLASHD_Initialize(BOARD_MCK, 1);

	__disable_irq();
	FLASHD_Unlock(IFLASH_ADDR, IFLASH_ADDR + IFLASH_SIZE - 1, 0, 0);
	FLASHD_Write(IFLASH_ADDR, bl_update_data, BOARD_DFU_BOOT_SIZE);

	erase_first_app_sector();
}
