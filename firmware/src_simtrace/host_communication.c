//#define TRACE_LEVEL 6

#include "board.h"
#include "req_ctx.h"
#include "linuxlist.h"

static volatile uint32_t usbep_in_progress[BOARD_USB_NUMENDPOINTS];

/* call-back after (successful?) transfer of a buffer */
static void usb_write_cb(uint8_t *arg, uint8_t status, uint32_t transferred,
			 uint32_t remaining)
{
	struct req_ctx *rctx = (struct req_ctx *) arg;

	TRACE_DEBUG("%s (EP=%u)\r\n", __func__, rctx->ep);

	__disable_irq();
	usbep_in_progress[rctx->ep]--;
	__enable_irq();
	TRACE_DEBUG("%u: in_progress=%d\n", rctx->ep, usbep_in_progress[rctx->ep]);

	if (status != USBD_STATUS_SUCCESS)
		TRACE_ERROR("%s error, status=%d\n", __func__, status);

	/* release request contxt to pool */
	req_ctx_set_state(rctx, RCTX_S_FREE);
}

int usb_refill_to_host(struct llist_head *queue, uint32_t ep)
{
	struct req_ctx *rctx;
	int rc;

	__disable_irq();
	if (usbep_in_progress[ep]) {
		__enable_irq();
		return 0;
	}

	if (llist_empty(queue)) {
		__enable_irq();
		return 0;
	}

	usbep_in_progress[ep]++;

	rctx = llist_entry(queue->next, struct req_ctx, list);
	llist_del(&rctx->list);

	__enable_irq();

	TRACE_DEBUG("%u: in_progress=%d\n", ep, usbep_in_progress[ep]);
	TRACE_DEBUG("%s (EP=%u)\r\n", __func__, ep);

	req_ctx_set_state(rctx, RCTX_S_USB_TX_BUSY);
	rctx->ep = ep;

	rc = USBD_Write(ep, rctx->data, rctx->tot_len,
			(TransferCallback) &usb_write_cb, rctx);
	if (rc != USBD_STATUS_SUCCESS) {
		TRACE_ERROR("%s error %x\n", __func__, rc);
		req_ctx_set_state(rctx, RCTX_S_USB_TX_PENDING);
		__disable_irq();
		usbep_in_progress[ep]--;
		__enable_irq();
		TRACE_DEBUG("%u: in_progress=%d\n", ep, usbep_in_progress[ep]);
		return 0;
	}

	return 1;
}

static void usb_read_cb(uint8_t *arg, uint8_t status, uint32_t transferred,
			uint32_t remaining)
{
	struct req_ctx *rctx = (struct req_ctx *) arg;
	struct llist_head *queue = (struct llist_head *) usbep_in_progress[rctx->ep];

	TRACE_DEBUG("%s (EP=%u)\r\n", __func__, rctx->ep);

	usbep_in_progress[rctx->ep] = 0;

	if (status != USBD_STATUS_SUCCESS) {
		TRACE_ERROR("%s error, status=%d\n", __func__, status);
		/* release request contxt to pool */
		req_ctx_put(rctx);
		return;
	}
	rctx->tot_len = transferred;
	req_ctx_set_state(rctx, RCTX_S_MAIN_PROCESSING);
	llist_add_tail(&rctx->list, queue);
}

int usb_refill_from_host(struct llist_head *queue, int ep)
{
	struct req_ctx *rctx;
	int rc;

	if (usbep_in_progress[ep])
		return 0;

	TRACE_DEBUG("%s (EP=%u)\r\n", __func__, ep);

	rctx = req_ctx_find_get(0, RCTX_S_FREE, RCTX_S_USB_RX_BUSY);
	rctx->ep = ep;

	rc = USBD_Read(ep, rctx->data, rctx->size,
			(TransferCallback) &usb_read_cb, rctx);

	if (rc != USBD_STATUS_SUCCESS) {
		TRACE_ERROR("%s error %x\n", __func__, rc);
		req_ctx_put(rctx);
		return 0;
	}

	usbep_in_progress[ep] = (uint32_t) queue;

	return 1;
}
