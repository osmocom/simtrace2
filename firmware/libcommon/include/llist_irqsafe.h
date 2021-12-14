/* IRQ-safe linked lists
 *
 * (C) 2015-2017 by Harald Welte <laforge@gnumonks.org>
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
#include "utils.h"

static inline void llist_add_irqsafe(struct llist_head *_new,
					  struct llist_head *head)
{
	unsigned long x;

	local_irq_save(x);
	llist_add(_new, head);
	local_irq_restore(x);
}

static inline void llist_add_tail_irqsafe(struct llist_head *_new,
					  struct llist_head *head)
{
	unsigned long x;

	local_irq_save(x);
	llist_add_tail(_new, head);
	__enable_irq();
}

static inline struct llist_head *llist_head_dequeue_irqsafe(struct llist_head *head)
{
	struct llist_head *lh;
	unsigned long x;

	local_irq_save(x);
	if (llist_empty(head)) {
		lh = NULL;
	} else {
		lh = head->next;
		llist_del(lh);
	}
	local_irq_restore(x);

	return lh;
}
