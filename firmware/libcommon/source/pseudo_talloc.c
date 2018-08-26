/* Memory allocation library
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
#include <stdint.h>

#include "talloc.h"
#include "trace.h"
#include "utils.h"
#include <osmocom/core/utils.h>

#define NUM_RCTX_SMALL 10
#define RCTX_SIZE_SMALL 348

static uint8_t msgb_data[NUM_RCTX_SMALL][RCTX_SIZE_SMALL] __attribute__((aligned(sizeof(long))));
static uint8_t msgb_inuse[NUM_RCTX_SMALL];

void *_talloc_zero(const void *ctx, size_t size, const char *name)
{
	unsigned int i;
	unsigned long x;

	local_irq_save(x);
	if (size > RCTX_SIZE_SMALL) {
		local_irq_restore(x);
		TRACE_ERROR("%s() request too large(%d > %d)\r\n", __func__, size, RCTX_SIZE_SMALL);
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(msgb_inuse); i++) {
		if (!msgb_inuse[i]) {
			uint8_t *out = msgb_data[i];
			msgb_inuse[i] = 1;
			memset(out, 0, size);
			local_irq_restore(x);
			return out;
		}
	}
	local_irq_restore(x);
	TRACE_ERROR("%s() out of memory!\r\n", __func__);
	return NULL;
}

int _talloc_free(void *ptr, const char *location)
{
	unsigned int i;
	unsigned long x;

	local_irq_save(x);
	for (i = 0; i < ARRAY_SIZE(msgb_inuse); i++) {
		if (ptr == msgb_data[i]) {
			if (!msgb_inuse[i]) {
				TRACE_ERROR("%s: double_free by %s\r\n", __func__, location);
			} else {
				msgb_inuse[i] = 0;
			}
			local_irq_restore(x);
			return 0;
		}
	}

	local_irq_restore(x);
	TRACE_ERROR("%s: invalid pointer %p from %s\r\n", __func__, ptr, location);
	return -1;
}

void talloc_set_name_const(const void *ptr, const char *name)
{
	/* do nothing */
}

#if 0
void *talloc_named_const(const void *context, size_t size, const char *name)
{
	if (size)
		TRACE_ERROR("%s: called with size!=0 from %s\r\n", __func__, name);
	return NULL;
}

void *talloc_pool(const void *context, size_t size)
{
}
#endif

