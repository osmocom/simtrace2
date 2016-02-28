#include "board.h"
#include "req_ctx.h"

/* call-back after (successful?) transfer of a buffer */
static void usb_write_cb(uint8_t *arg, uint8_t status, uint32_t transferred,
			 uint32_t remaining)
{
	struct req_ctx *rctx = (struct req_ctx *) arg;

	if (status != USBD_STATUS_SUCCESS)
		TRACE_ERROR("%s error, status=%d\n", __func__, status);

	/* release request contxt to pool */
	req_ctx_set_state(rctx, RCTX_S_FREE);
}

int usb_to_host(void)
{
	struct req_ctx *rctx;
	int rc;

	rctx = req_ctx_find_get(0, RCTX_S_USB_TX_PENDING, RCTX_S_USB_TX_BUSY);

	/* FIXME: obtain endpoint number from req_ctx! */
	rc = USBD_Write(PHONE_DATAIN, rctx->data, rctx->tot_len,
			(TransferCallback) &usb_write_cb, rctx);
	if (rc != USBD_STATUS_SUCCESS) {
		TRACE_ERROR("%s error %x\n", __func__, ret);
		req_ctx_set_state(rctx, RCTX_S_USB_TX_PENDING);
		return 0;
	}

	return 1;
}

static void usb_read_cb(uint8_t *arg, uint8_t status, uint32_t transferred,
			uint32_t remaining)
{
	struct req_ctx *rctx = (struct req_ctx *) arg;

	if (status != USBD_STATUS_SUCCESS) {
		TRACE_ERROR("%s error, status=%d\n", __func__, status);
		/* release request contxt to pool */
		req_ctx_put(rctx);
		return;
	}
	req_ctx_set_state(rctx, RCTX_S_UART_TX_PENDING);
}

int usb_from_host(int ep)
{
	struct req_ctx *rctx;
	int rc;

	rctx = req_ctx_find_get(0, RCTX_S_FREE, RCTX_S_USB_RX_BUSY);

	rc = USBD_Read(ep, rctx->data, rctx->size,
			(TransferCallback) &usb_read_cb, rctx);

	if (rc != USBD_STATUS_SUCCESS) {
		TRACE_ERROR("%s error %x\n", __func__, ret);
		req_ctx_put(rctx);
	}

	return 0;
}
