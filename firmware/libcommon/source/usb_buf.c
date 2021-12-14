/* USB buffer library
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "board.h"
#include "trace.h"
#include "usb_buf.h"
#include "simtrace_usb.h"

#include <osmocom/core/linuxlist.h>
#include <osmocom/core/msgb.h>
#include <errno.h>

#define USB_ALLOC_SIZE	280
#define USB_MAX_QLEN	3

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

	if (ep->queue_len >= USB_MAX_QLEN) {
		struct msgb *evict;
		/* free the first pending buffer in the queue */
		TRACE_INFO("EP%02x: dropping first queue element (qlen=%u)\r\n",
			   ep->ep, ep->queue_len);
		evict = msgb_dequeue_count(&ep->queue, &ep->queue_len);
		OSMO_ASSERT(evict);
		usb_buf_free(evict);
	}

	msgb_enqueue_count(&ep->queue, msg, &ep->queue_len);
	return 0;
}

void usb_buf_init(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(usb_buffered_ep); i++) {
		struct usb_buffered_ep *ep = &usb_buffered_ep[i];
		INIT_LLIST_HEAD(&ep->queue);
		ep->ep = i;
	}
}
