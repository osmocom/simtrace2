/* USB Request Context for OpenPCD / OpenPICC / SIMtrace
 * (C) 2006-2016 by Harald Welte <hwelte@hmw-consulting.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#include "utils.h"
#include "trace.h"
#include "req_ctx.h"

#define NUM_RCTX_SMALL 10
#define NUM_RCTX_LARGE 0

#define NUM_REQ_CTX	(NUM_RCTX_SMALL+NUM_RCTX_LARGE)

static uint8_t rctx_data[NUM_RCTX_SMALL][RCTX_SIZE_SMALL];
static uint8_t rctx_data_large[NUM_RCTX_LARGE][RCTX_SIZE_LARGE];

static struct req_ctx req_ctx[NUM_REQ_CTX];

struct req_ctx __ramfunc *
req_ctx_find_get(int large, uint32_t old_state, uint32_t new_state)
{
	struct req_ctx *toReturn = NULL;
	unsigned long flags;
	unsigned int i;

	if (old_state >= RCTX_STATE_COUNT || new_state >= RCTX_STATE_COUNT) {
		TRACE_DEBUG("Invalid parameters for req_ctx_find_get");
		return NULL;
	}

	local_irq_save(flags);
	for (i = 0; i < ARRAY_SIZE(req_ctx); i++) {
		if (req_ctx[i].state == old_state) {
			toReturn = &req_ctx[i];
			toReturn->state = new_state;
		}
	}
	local_irq_restore(flags);

	TRACE_DEBUG("%s(%u, %u, %u)=%p\n", __func__, large, old_state,
			new_state, toReturn);
	return toReturn;
}

uint8_t req_ctx_num(struct req_ctx *ctx)
{
	return ctx - req_ctx;
}

void req_ctx_set_state(struct req_ctx *ctx, uint32_t new_state)
{
	unsigned long flags;

	TRACE_DEBUG("%s(ctx=%p, new_state=%u)\n", __func__, ctx, new_state);

	if (new_state >= RCTX_STATE_COUNT) {
		TRACE_DEBUG("Invalid new_state for req_ctx_set_state\n");
		return;
	}
	local_irq_save(flags);
	ctx->state = new_state;
	local_irq_restore(flags);
}

#ifdef DEBUG_REQCTX
void req_print(int state) {
	int count = 0;
	struct req_ctx *ctx, *last = NULL;
	DEBUGP("State [%02i] start <==> ", state);
	ctx = req_ctx_queues[state];
	while (ctx) {
		if (last != ctx->prev)
			DEBUGP("*INV_PREV* ");
		DEBUGP("%08X => ", ctx);
		last = ctx;
		ctx = ctx->next;
		count++;
		if (count > NUM_REQ_CTX) {
		  DEBUGP("*WILD POINTER* => ");
		  break;
		}
	}
	TRACE_DEBUG("NULL");
	if (!req_ctx_queues[state] && req_ctx_tails[state]) {
		TRACE_DEBUG("NULL head, NON-NULL tail");
	}
	if (last != req_ctx_tails[state]) {
		TRACE_DEBUG("Tail does not match last element");
	}
}
#endif

void req_ctx_put(struct req_ctx *ctx)
{
	return req_ctx_set_state(ctx, RCTX_S_FREE);
}

void req_ctx_init(void)
{
	int i;
	for (i = 0; i < NUM_RCTX_SMALL; i++) {
		req_ctx[i].size = RCTX_SIZE_SMALL;
		req_ctx[i].tot_len = 0;
		req_ctx[i].data = rctx_data[i];
		req_ctx[i].state = RCTX_S_FREE;
		TRACE_DEBUG("SMALL req_ctx[%02i] initialized at %p, Data: %p => %p\n",
			i, req_ctx + i, req_ctx[i].data, req_ctx[i].data + RCTX_SIZE_SMALL);
	}

	for (; i < NUM_REQ_CTX; i++) {
		req_ctx[i].size = RCTX_SIZE_LARGE;
		req_ctx[i].tot_len = 0;
		req_ctx[i].data = rctx_data_large[i];
		req_ctx[i].state = RCTX_S_FREE;
		TRACE_DEBUG("LARGE req_ctx[%02i] initialized at %p, Data: %p => %p\n",
			i, req_ctx + i, req_ctx[i].data, req_ctx[i].data + RCTX_SIZE_LARGE);
	}
}
