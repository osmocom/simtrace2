/* USB communication methods
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
#include "board.h"
#include "llist_irqsafe.h"
#include "usb_buf.h"
#include "utils.h"

#include <osmocom/core/linuxlist.h>
#include <osmocom/core/msgb.h>
#include <errno.h>

/***********************************************************************
 * USBD Integration API
 ***********************************************************************/

/* call-back after (successful?) transfer of a buffer */
static void usb_write_cb(uint8_t *arg, uint8_t status, uint32_t transferred,
			 uint32_t remaining)
{
	struct msgb *msg = (struct msgb *) arg;
	struct usb_buffered_ep *bep = msg->dst;
	unsigned long x;

	TRACE_DEBUG("%s (EP=0x%02x)\r\n", __func__, bep->ep);

	local_irq_save(x);
	bep->in_progress--;
	local_irq_restore(x);
	TRACE_DEBUG("%u: in_progress=%lu\n", bep->ep, bep->in_progress);

	if (status != USBD_STATUS_SUCCESS)
		TRACE_ERROR("%s error, status=%d\n", __func__, status);

	usb_buf_free(msg);
}

int usb_refill_to_host(uint8_t ep)
{
	struct usb_buffered_ep *bep = usb_get_buf_ep(ep);
	struct msgb *msg;
	unsigned long x;
	int rc;

#if 0
	if (bep->out_from_host) {
		TRACE_ERROR("EP 0x%02x is not IN\r\n", bep->ep);
		return -EINVAL;
	}
#endif

	local_irq_save(x);
	if (bep->in_progress) {
		local_irq_restore(x);
		return 0;
	}

	if (llist_empty(&bep->queue)) {
		local_irq_restore(x);
		return 0;
	}

	bep->in_progress++;

	msg = msgb_dequeue(&bep->queue);

	local_irq_restore(x);

	TRACE_DEBUG("%s (EP=0x%02x), in_progress=%lu\r\n", __func__, ep, bep->in_progress);

	msg->dst = bep;

	rc = USBD_Write(ep, msgb_data(msg), msgb_length(msg),
			(TransferCallback) &usb_write_cb, msg);
	if (rc != USBD_STATUS_SUCCESS) {
		TRACE_ERROR("%s error %x\n", __func__, rc);
		/* re-insert to head of queue */
		llist_add_irqsafe(&msg->list, &bep->queue);
		local_irq_save(x);
		bep->in_progress--;
		local_irq_restore(x);
		TRACE_DEBUG("%02x: in_progress=%lu\n", bep->ep, bep->in_progress);
		return 0;
	}

	return 1;
}

/* call-back after (successful?) transfer of a buffer */
static void usb_read_cb(uint8_t *arg, uint8_t status, uint32_t transferred,
			uint32_t remaining)
{
	struct msgb *msg = (struct msgb *) arg;
	struct usb_buffered_ep *bep = msg->dst;

	TRACE_DEBUG("%s (EP=%u, len=%lu, q=%p)\r\n", __func__,
			bep->ep, transferred, &bep->queue);

	bep->in_progress = 0;

	if (status != USBD_STATUS_SUCCESS) {
		TRACE_ERROR("%s error, status=%d\n", __func__, status);
		usb_buf_free(msg);
		return;
	}
	msgb_put(msg, transferred);
	llist_add_tail_irqsafe(&msg->list, &bep->queue);
}

int usb_refill_from_host(uint8_t ep)
{
	struct usb_buffered_ep *bep = usb_get_buf_ep(ep);
	struct msgb *msg;
	unsigned long x;
	int rc;

#if 0
	if (!bep->out_from_host) {
		TRACE_ERROR("EP 0x%02x is not OUT\r\n", bep->ep);
		return -EINVAL;
	}
#endif

	if (bep->in_progress)
		return 0;

	TRACE_DEBUG("%s (EP=0x%02x)\r\n", __func__, bep->ep);

	msg = usb_buf_alloc(bep->ep);
	if (!msg)
		return -ENOMEM;
	msg->dst = bep;
	msg->l1h = msg->head;

	bep->in_progress = 1;

	rc = USBD_Read(ep, msg->head, msgb_tailroom(msg),
			(TransferCallback) &usb_read_cb, msg);
	if (rc != USBD_STATUS_SUCCESS) {
		TRACE_ERROR("%s error %d\n", __func__, rc);
		usb_buf_free(msg);
		bep->in_progress = 0;
	}

	return 1;
}

int usb_drain_queue(uint8_t ep)
{
	struct usb_buffered_ep *bep = usb_get_buf_ep(ep);
	struct msgb *msg;
	unsigned long x;
	int ret = 0;

	/* wait until no transfers are in progress anymore and block
	 * further interrupts */
	while (1) {
		local_irq_save(x);
		if (!bep->in_progress) {
			break;
		}
		local_irq_restore(x);
		/* retry */
	}

	/* free all queued msgbs */
	while ((msg = msgb_dequeue(&bep->queue))) {
		usb_buf_free(msg);
		ret++;
	}

	/* re-enable interrupts and return number of free'd msgbs */
	local_irq_restore(x);

	return ret;
}
