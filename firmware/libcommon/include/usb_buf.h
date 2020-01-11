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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA
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

struct usb_if {
	uint8_t if_num;		/* interface number */
	uint8_t ep_out;		/* OUT endpoint (0 if none) */
	uint8_t ep_in;		/* IN endpint (0 if none) */
	uint8_t ep_int;		/* INT endpoint (0 if none) */
	void *data;		/* opaque data, passed through */
	struct {
		/* call-back to be called for inclming messages on OUT EP */
		void (*rx_out)(struct msgb *msg, const struct usb_if *usb_if);
	} ops;
};
void usb_process(const struct usb_if *usb_if);
