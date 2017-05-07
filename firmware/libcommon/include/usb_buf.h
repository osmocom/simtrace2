#pragma once

#include "osmocom/core/linuxlist.h"
#include "osmocom/core/msgb.h"

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
