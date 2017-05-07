#pragma once

#include "osmocom/core/linuxlist.h"

static inline void llist_add_irqsafe(struct llist_head *_new,
					  struct llist_head *head)
{
	__disable_irq();
	llist_add(_new, head);
	__enable_irq();
}

static inline void llist_add_tail_irqsafe(struct llist_head *_new,
					  struct llist_head *head)
{
	__disable_irq();
	llist_add_tail(_new, head);
	__enable_irq();
}

static inline struct llist_head *llist_head_dequeue_irqsafe(struct llist_head *head)
{
	struct llist_head *lh;

	__disable_irq();
	if (llist_empty(head)) {
		lh = NULL;
	} else {
		lh = head->next;
		llist_del(lh);
	}
	__enable_irq();

	return lh;
}
