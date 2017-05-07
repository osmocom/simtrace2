#include "board.h"
#include "trace.h"
#include "usb_buf.h"

#include "osmocom/core/linuxlist.h"
#include "osmocom/core/msgb.h"
#include <errno.h>

#define USB_ALLOC_SIZE	280

static struct usb_buffered_ep usb_buffered_ep[BOARD_USB_NUMENDPOINTS];

struct usb_buffered_ep *usb_get_buf_ep(uint8_t ep)
{
	if (ep >= ARRAY_SIZE(usb_buffered_ep))
		return NULL;
	return &usb_buffered_ep[ep];
}


/***********************************************************************
 * User API
 ***********************************************************************/

struct llist_head *usb_get_queue(uint8_t ep)
{
	struct usb_buffered_ep *bep = usb_get_buf_ep(ep);
	if (!bep)
		return NULL;
	return &bep->queue;
}

/* allocate a USB buffer for use with given end-point */
struct msgb *usb_buf_alloc(uint8_t ep)
{
	struct msgb *msg;

	msg = msgb_alloc(USB_ALLOC_SIZE, "USB");
	if (!msg)
		return NULL;
	msg->dst = usb_get_buf_ep(ep);
	return msg;
}

/* release/return the USB buffer to the pool */
void usb_buf_free(struct msgb *msg)
{
	msgb_free(msg);
}

/* submit a USB buffer for transmission to host */
int usb_buf_submit(struct msgb *msg)
{
	struct usb_buffered_ep *ep = msg->dst;

	if (!msg->dst) {
		TRACE_ERROR("%s: msg without dst\r\n", __func__);
		usb_buf_free(msg);
		return -EINVAL;
	}

	/* no need for irqsafe operation, as the usb_tx_queue is
	 * processed only by the main loop context */
	msgb_enqueue(&ep->queue, msg);
	return 0;
}

void usb_buf_init(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(usb_buffered_ep); i++) {
		struct usb_buffered_ep *ep = &usb_buffered_ep[i];
		INIT_LLIST_HEAD(&ep->queue);
	}
}
