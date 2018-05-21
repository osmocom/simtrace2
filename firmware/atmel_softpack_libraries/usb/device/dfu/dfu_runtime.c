/* DFU related functions that are active at runtime, i.e. during the
 * normal operation of the device firmware, *not* during DFU update mode
 * (C) 2006 Harald Welte <hwelte@hmw-consulting.de>
 *
 * This ought to be compliant to the USB DFU Spec 1.0 as available from
 * http://www.usb.org/developers/devclass_docs/usbdfu10.pdf
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <board.h>
#include <assert.h>
#include <core_cm3.h>

#include <usb/include/USBD.h>
#include <usb/device/dfu/dfu.h>

#include "trace.h"

#include <usb/include/USBDescriptors.h>
#include <usb/include/USBRequests.h>
#include <usb/include/USBD.h>
#include <usb/common/dfu/usb_dfu.h>
#include <usb/device/dfu/dfu.h>

struct dfudata *g_dfu = (struct dfudata *) IRAM_ADDR;

/* FIXME: this was used for a special ELF section which then got called
 * by DFU code and Application code, across flash partitions */
#define __dfufunc

static __dfufunc void handle_getstatus(void)
{
	/* has to be static as USBD_Write is async ? */
	static struct dfu_status dstat;
	static const uint8_t poll_timeout_10ms[] = { 10, 0, 0 };

	/* send status response */
	dstat.bStatus = g_dfu->status;
	dstat.bState = g_dfu->state;
	dstat.iString = 0;
	memcpy(&dstat.bwPollTimeout, poll_timeout_10ms, sizeof(dstat.bwPollTimeout));

	TRACE_DEBUG("handle_getstatus(%u, %u)\n\r", dstat.bStatus, dstat.bState);

	USBD_Write(0, (char *)&dstat, sizeof(dstat), NULL, 0);
}

static void __dfufunc handle_getstate(void)
{
	uint8_t u8 = g_dfu->state;

	TRACE_DEBUG("handle_getstate(%u)\n\r", g_dfu->state);

	USBD_Write(0, (char *)&u8, sizeof(u8), NULL, 0);
}

static const uint8_t *get_dfu_func_desc(void)
{
	USBDDriver *usbdDriver = USBD_GetDriver();
	const USBConfigurationDescriptor *cfg_desc;
	const USBGenericDescriptor *gen_desc;

	if (USBD_IsHighSpeed())
		cfg_desc = &usbdDriver->pDescriptors->pHsConfiguration[usbdDriver->cfgnum];
	else
		cfg_desc = usbdDriver->pDescriptors->pFsConfiguration[usbdDriver->cfgnum];

	for (gen_desc = (const USBGenericDescriptor *) cfg_desc;
	     (const uint8_t *) gen_desc < (const uint8_t *) cfg_desc + cfg_desc->wTotalLength;
	     gen_desc = (const USBGenericDescriptor *) ((const uint8_t *)gen_desc + gen_desc->bLength)) {
		if (gen_desc->bDescriptorType == USB_DT_DFU)
			return (const uint8_t *) gen_desc;
	}
	return NULL;
}

static void TerminateCtrlInWithNull(void *pArg,
                                    unsigned char status,
                                    unsigned long int transferred,
                                    unsigned long int remaining)
{
    USBD_Write(0, // Endpoint #0
               0, // No data buffer
               0, // No data buffer
               (TransferCallback) 0,
               (void *)  0);
}

/* this function gets daisy-chained into processing EP0 requests */
void USBDFU_Runtime_RequestHandler(const USBGenericRequest *request)
{
	USBDDriver *usbdDriver = USBD_GetDriver();
	uint8_t req = USBGenericRequest_GetRequest(request);
	uint16_t len = USBGenericRequest_GetLength(request);
	uint16_t val = USBGenericRequest_GetValue(request);
	int rc, ret = DFU_RET_NOTHING;

	TRACE_DEBUG("type=0x%x, recipient=0x%x val=0x%x len=%u\n\r",
			USBGenericRequest_GetType(request),
			USBGenericRequest_GetRecipient(request),
			val, len);

	/* check for GET_DESCRIPTOR on DFU */
	if (USBGenericRequest_GetType(request) == USBGenericRequest_STANDARD &&
	    USBGenericRequest_GetRecipient(request) == USBGenericRequest_DEVICE &&
	    USBGenericRequest_GetRequest(request) == USBGenericRequest_GETDESCRIPTOR &&
	    USBGetDescriptorRequest_GetDescriptorType(request) == USB_DT_DFU) {
		uint16_t length = sizeof(struct usb_dfu_func_descriptor);
		const USBDeviceDescriptor *pDevice;
		const uint8_t *dfu_func_desc = get_dfu_func_desc();
		int terminateWithNull;

		ASSERT(dfu_func_desc);

		if (USBD_IsHighSpeed())
			pDevice = usbdDriver->pDescriptors->pHsDevice;
		else
			pDevice = usbdDriver->pDescriptors->pFsDevice;

		terminateWithNull = ((length % pDevice->bMaxPacketSize0) == 0);
		USBD_Write(0, dfu_func_desc, length,
			   terminateWithNull ? TerminateCtrlInWithNull : 0, 0);
		return;
	}

	/* forward all non-DFU specific messages to core handler*/
	if (USBGenericRequest_GetType(request) != USBGenericRequest_CLASS ||
	    USBGenericRequest_GetRecipient(request) != USBGenericRequest_INTERFACE) {
		TRACE_DEBUG("std_ho_usbd ");
		USBDCallbacks_RequestReceived(request);
		return;
	}

	switch (g_dfu->state) {
	case DFU_STATE_appIDLE:
		switch (req) {
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus();
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate();
			break;
		case USB_REQ_DFU_DETACH:
			/* we switch it DETACH state, send a ZLP and
			 * return.  The next USB reset in this state
			 * will then trigger DFURT_SwitchToDFU() below */
			TRACE_DEBUG("\r\n====dfu_detach\n\r");
			g_dfu->state = DFU_STATE_appDETACH;
			ret = DFU_RET_ZLP;
			goto out;
			break;
		default:
			ret = DFU_RET_STALL;
		}
		break;
	case DFU_STATE_appDETACH:
		switch (req) {
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus();
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate();
			break;
		default:
			g_dfu->state = DFU_STATE_appIDLE;
			ret = DFU_RET_STALL;
			goto out;
			break;
		}
		/* FIXME: implement timer to return to appIDLE */
		break;
	default:
		TRACE_ERROR("Invalid DFU State reached in Runtime Mode\r\n");
		ret = DFU_RET_STALL;
		break;
	}

out:
	switch (ret) {
	case DFU_RET_NOTHING:
		break;
	case DFU_RET_ZLP:
		USBD_Write(0, 0, 0, 0, 0);
		break;
	case DFU_RET_STALL:
		USBD_Stall(0);
		break;
	}
}

void DFURT_SwitchToDFU(void)
{
	/* store the magic value that the DFU loader can detect and
	 * activate itself, rather than boot into the application */
	g_dfu->magic = USB_DFU_MAGIC;

	/* Disconnect the USB by remoting the pull-up */
	USBD_Disconnect();
	__disable_irq();

	/* reset the processor, we will start execution with the
	 * ResetVector of the bootloader */
	NVIC_SystemReset();
}
