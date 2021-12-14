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
#pragma once

#include <osmocom/core/linuxlist.h>
#include <osmocom/core/msgb.h>

/* buffered USB endpoint (with queue of msgb) */
struct usb_buffered_ep {
	/* endpoint number */
	uint8_t ep;
	/* OUT endpoint (1) or IN/IRQ (0)? */
	uint8_t out_from_host;
	/* currently any transfer in progress? */
	volatile uint32_t in_progress;
	/* Tx queue (IN) / Rx queue (OUT) */
	struct llist_head queue;
	/* current length of queue */
	unsigned int queue_len;
};

struct msgb *usb_buf_alloc(uint8_t ep);
void usb_buf_free(struct msgb *msg);
int usb_buf_submit(struct msgb *msg);
struct llist_head *usb_get_queue(uint8_t ep);
int usb_drain_queue(uint8_t ep);

void usb_buf_init(void);
struct usb_buffered_ep *usb_get_buf_ep(uint8_t ep);

int usb_refill_to_host(uint8_t ep);
int usb_refill_from_host(uint8_t ep);
