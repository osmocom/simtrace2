/* USB Device Firmware Update Implementation for OpenPCD, OpenPICC SIMtrace
 * (C) 2006-2017 by Harald Welte <hwelte@hmw-consulting.de>
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

#include <unistd.h>

#include <board.h>
#include <core_cm3.h>

#include "trace.h"

#include <usb/include/USBDescriptors.h>
#include <usb/include/USBRequests.h>
#include <usb/include/USBD.h>
#include <usb/common/dfu/usb_dfu.h>
#include <usb/device/dfu/dfu.h>

/* FIXME: this was used for a special ELF section which then got called
 * by DFU code and Application code, across flash partitions */
#define __dfudata __attribute__ ((section (".dfudata")))
#define __dfufunc

/// Standard device driver instance.
static USBDDriver usbdDriver;
static unsigned char if_altsettings[1];

__dfudata struct dfudata _g_dfu = {
  	.state = DFU_STATE_appIDLE,
	.past_manifest = 0,
	.total_bytes = 0,
};
struct dfudata *g_dfu = &_g_dfu;

WEAK void dfu_drv_updstatus(void)
{
	TRACE_INFO("DFU: updstatus()\n\r");

	/* we transition immediately from MANIFEST_SYNC to MANIFEST,
	 * as the flash-writing is not asynchronous in this
	 * implementation */
	if (g_dfu->state == DFU_STATE_dfuMANIFEST_SYNC)
		g_dfu->state = DFU_STATE_dfuMANIFEST;
}

static __dfufunc void handle_getstatus(void)
{
	/* has to be static as USBD_Write is async ? */
	static struct dfu_status dstat;
	static const uint8_t poll_timeout_10ms[] = { 10, 0, 0 };

	dfu_drv_updstatus();

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

static uint8_t dfu_buf[BOARD_DFU_PAGE_SIZE];

/* download of a single page has completed */
static void dnload_cb(void *arg, unsigned char status, unsigned long int transferred,
		      unsigned long int remaining)
{
	int rc;

	TRACE_DEBUG("COMPLETE\n\r");

	if (status != USBD_STATUS_SUCCESS) {
		TRACE_ERROR("USBD download callback status %d\n\r", status);
		USBD_Stall(0);
		return;
	}

	rc = USBDFU_handle_dnload(if_altsettings[0], g_dfu->total_bytes, dfu_buf, transferred);
	switch (rc) {
	case DFU_RET_ZLP:
		g_dfu->total_bytes += transferred;
		g_dfu->state = DFU_STATE_dfuDNLOAD_IDLE;
		TerminateCtrlInWithNull(0,0,0,0);
		break;
	case DFU_RET_STALL:
		g_dfu->state = DFU_STATE_dfuERROR;
		USBD_Stall(0);
		break;
	case DFU_RET_NOTHING:
		break;
	}

}

static int handle_dnload(uint16_t val, uint16_t len, int first)
{
	int rc;

	if (len > BOARD_DFU_PAGE_SIZE) {
		TRACE_ERROR("DFU length exceeds flash page size\n\r");
		g_dfu->state = DFU_STATE_dfuERROR;
		g_dfu->status = DFU_STATUS_errADDRESS;
		return DFU_RET_STALL;
	}

	if (len & 0x03) {
		TRACE_ERROR("DFU length not four-byte-aligned\n\r");
		g_dfu->state = DFU_STATE_dfuERROR;
		g_dfu->status = DFU_STATUS_errADDRESS;
		return DFU_RET_STALL;
	}

	if (first)
		g_dfu->total_bytes = 0;

	if (len == 0) {
		TRACE_DEBUG("zero-size write -> MANIFEST_SYNC\n\r");
		g_dfu->state = DFU_STATE_dfuMANIFEST_SYNC;
		return DFU_RET_ZLP;
	}

	/* else: actually read data */
	rc = USBD_Read(0, dfu_buf, len, &dnload_cb, 0);
	if (rc == USBD_STATUS_SUCCESS)
		return DFU_RET_NOTHING;
	else
		return DFU_RET_STALL;
}

/* upload of a single page has completed */
static void upload_cb(void *arg, unsigned char status, unsigned long int transferred,
		      unsigned long int remaining)
{
	int rc;

	TRACE_DEBUG("COMPLETE\n\r");

	if (status != USBD_STATUS_SUCCESS) {
		TRACE_ERROR("USBD upload callback status %d\n\r", status);
		USBD_Stall(0);
		return;
	}

	g_dfu->total_bytes += transferred;
}

static int handle_upload(uint16_t val, uint16_t len, int first)
{
	int rc;

	if (first)
		g_dfu->total_bytes = 0;

	if (len > BOARD_DFU_PAGE_SIZE) {
		TRACE_ERROR("DFU length exceeds flash page size\n\r");
		g_dfu->state = DFU_STATE_dfuERROR;
		g_dfu->status = DFU_STATUS_errADDRESS;
		return DFU_RET_STALL;
	}

	rc = USBDFU_handle_upload(if_altsettings[0], g_dfu->total_bytes, dfu_buf, len);
	if (rc < 0) {
		TRACE_ERROR("application handle_upload() returned %d\n\r", rc);
		return DFU_RET_STALL;
	}

	if (USBD_Write(0, dfu_buf, rc, &upload_cb, 0) == USBD_STATUS_SUCCESS)
		return rc;

	return DFU_RET_STALL;
}

/* this function gets daisy-chained into processing EP0 requests */
void USBDFU_DFU_RequestHandler(const USBGenericRequest *request)
{
	uint8_t req = USBGenericRequest_GetRequest(request);
	uint16_t len = USBGenericRequest_GetLength(request);
	uint16_t val = USBGenericRequest_GetValue(request);
	int rc, ret;

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
		int terminateWithNull;

		if (USBD_IsHighSpeed())
			pDevice = usbdDriver.pDescriptors->pHsDevice;
		else
			pDevice = usbdDriver.pDescriptors->pFsDevice;

		terminateWithNull = ((length % pDevice->bMaxPacketSize0) == 0);
		USBD_Write(0, &dfu_cfg_descriptor.func_dfu, length,
			   terminateWithNull ? TerminateCtrlInWithNull : 0, 0);
		return;
	}

	/* forward all non-DFU specific messages to core handler*/
	if (USBGenericRequest_GetType(request) != USBGenericRequest_CLASS ||
	    USBGenericRequest_GetRecipient(request) != USBGenericRequest_INTERFACE) {
		TRACE_DEBUG("std_ho_usbd ");
		USBDDriver_RequestHandler(&usbdDriver, request);
		return;
	}

	switch (g_dfu->state) {
	case DFU_STATE_appIDLE:
	case DFU_STATE_appDETACH:
		TRACE_ERROR("Invalid DFU State reached in DFU mode\r\n");
		ret = DFU_RET_STALL;
		break;
	case DFU_STATE_dfuIDLE:
		switch (req) {
		case USB_REQ_DFU_DNLOAD:
			if (len == 0) {
				g_dfu->state = DFU_STATE_dfuERROR;
				ret = DFU_RET_STALL;
				goto out;
			}
			g_dfu->state = DFU_STATE_dfuDNLOAD_SYNC;
			ret = handle_dnload(val, len, 1);
			break;
		case USB_REQ_DFU_UPLOAD:
			g_dfu->state = DFU_STATE_dfuUPLOAD_IDLE;
			handle_upload(val, len, 1);
			break;
		case USB_REQ_DFU_ABORT:
			/* no zlp? */
			ret = DFU_RET_ZLP;
			break;
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus();
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate();
			break;
		default:
			g_dfu->state = DFU_STATE_dfuERROR;
			ret = DFU_RET_STALL;
			goto out;
			break;
		}
		break;
	case DFU_STATE_dfuDNLOAD_SYNC:
		switch (req) {
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus();
			/* FIXME: state transition depending on block completeness */
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate();
			break;
		default:
			g_dfu->state = DFU_STATE_dfuERROR;
			ret = DFU_RET_STALL;
			goto out;
		}
		break;
	case DFU_STATE_dfuDNBUSY:
		switch (req) {
		case USB_REQ_DFU_GETSTATUS:
			/* FIXME: only accept getstatus if bwPollTimeout
			 * has elapsed */
			handle_getstatus();
			break;
		default:
			g_dfu->state = DFU_STATE_dfuERROR;
			ret = DFU_RET_STALL;
			goto out;
		}
		break;
	case DFU_STATE_dfuDNLOAD_IDLE:
		switch (req) {
		case USB_REQ_DFU_DNLOAD:
			g_dfu->state = DFU_STATE_dfuDNLOAD_SYNC;
			ret = handle_dnload(val, len, 0);
			break;
		case USB_REQ_DFU_ABORT:
			g_dfu->state = DFU_STATE_dfuIDLE;
			ret = DFU_RET_ZLP;
			break;
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus();
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate();
			break;
		default:
			g_dfu->state = DFU_STATE_dfuERROR;
			ret = DFU_RET_STALL;
			break;
		}
		break;
	case DFU_STATE_dfuMANIFEST_SYNC:
		switch (req) {
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus();
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate();
			break;
		default:
			g_dfu->state = DFU_STATE_dfuERROR;
			ret = DFU_RET_STALL;
			break;
		}
		break;
	case DFU_STATE_dfuMANIFEST:
		switch (req) {
		case USB_REQ_DFU_GETSTATUS:
			/* we don't want to change to WAIT_RST, as it
			 * would mean that we can not support another
			 * DFU transaction before doing the actual
			 * reset.  Instead, we switch to idle and note
			 * that we've already been through MANIFST in
			 * the global variable 'past_manifest'.
			 */
			//g_dfu->state = DFU_STATE_dfuMANIFEST_WAIT_RST;
			g_dfu->state = DFU_STATE_dfuIDLE;
			g_dfu->past_manifest = 1;
			handle_getstatus();
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate();
			break;
		default:
			g_dfu->state = DFU_STATE_dfuERROR;
			ret = DFU_RET_STALL;
			break;
		}
		break;
	case DFU_STATE_dfuMANIFEST_WAIT_RST:
		/* we should never go here */
		break;
	case DFU_STATE_dfuUPLOAD_IDLE:
		switch (req) {
		case USB_REQ_DFU_UPLOAD:
			/* state transition if less data then requested */
			rc = handle_upload(val, len, 0);
			if (rc >= 0 && rc < len)
				g_dfu->state = DFU_STATE_dfuIDLE;
			break;
		case USB_REQ_DFU_ABORT:
			g_dfu->state = DFU_STATE_dfuIDLE;
			/* no zlp? */
			ret = DFU_RET_ZLP;
			break;
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus();
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate();
			break;
		default:
			g_dfu->state = DFU_STATE_dfuERROR;
			ret = DFU_RET_STALL;
			break;
		}
		break;
	case DFU_STATE_dfuERROR:
		switch (req) {
		case USB_REQ_DFU_GETSTATUS:
			handle_getstatus();
			break;
		case USB_REQ_DFU_GETSTATE:
			handle_getstate();
			break;
		case USB_REQ_DFU_CLRSTATUS:
			g_dfu->state = DFU_STATE_dfuIDLE;
			g_dfu->status = DFU_STATUS_OK;
			/* no zlp? */
			ret = DFU_RET_ZLP;
			break;
		default:
			g_dfu->state = DFU_STATE_dfuERROR;
			ret = DFU_RET_STALL;
			break;
		}
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

/* we assume the caller has enabled the required clock/PLL for USB */
void USBDFU_Initialize(const USBDDriverDescriptors *pDescriptors)
{
	/* We already start in DFU idle mode */
	g_dfu->state = DFU_STATE_dfuIDLE;

	USBDDriver_Initialize(&usbdDriver, pDescriptors, if_altsettings);
	USBD_Init();
	USBD_Connect();
	//USBD_ConfigureSpeed(1);

	NVIC_EnableIRQ(UDP_IRQn);
}

void USBDFU_SwitchToApp(void)
{
	/* make sure the MAGIC is not set to enter DFU again */
	g_dfu->magic = 0;

	printf("switching to app\r\n");

	/* disconnect from USB to ensure re-enumeration */
	USBD_Disconnect();

	/* disable any interrupts during transition */
	__disable_irq();

	/* Tell the hybrid to execute FTL JUMP! */
	NVIC_SystemReset();
}

/* A board can provide a function overriding this, enabling a
 * board-specific 'boot into DFU' override, like a specific GPIO that
 * needs to be pulled a certain way. */
WEAK int USBDFU_OverrideEnterDFU(void)
{
	return 0;
}

void USBDCallbacks_RequestReceived(const USBGenericRequest *request)
{
	USBDFU_DFU_RequestHandler(request);
}
