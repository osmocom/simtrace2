/* ISO7816-3 Fi/Di tables + computation
 *
 * (C) 2010-2015 by Harald Welte <laforge@gnumonks.org>
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
#include <stdint.h>
#include <errno.h>

#include "utils.h"
#include "iso7816_fidi.h"

/* Table 7 of ISO 7816-3:2006 */
const uint16_t iso7816_3_fi_table[] = {
	372, 372, 558, 744, 1116, 1488, 1860, 0,
	0, 512, 768, 1024, 1536, 2048, 0, 0
};

/* Table 8 from ISO 7816-3:2006 */
const uint8_t iso7816_3_di_table[] = {
	0, 1, 2, 4, 8, 16, 32, 64,
	12, 20, 2, 4, 8, 16, 32, 64,
};

/* compute the F/D ratio based on Fi and Di values */
int iso7816_3_compute_fd_ratio(uint8_t f_index, uint8_t d_index)
{
	uint16_t f, d;
	int ret;

	if (f_index >= ARRAY_SIZE(iso7816_3_fi_table) ||
	    d_index >= ARRAY_SIZE(iso7816_3_di_table))
		return -EINVAL;

	f = iso7816_3_fi_table[f_index];
	if (f == 0)
		return -EINVAL;

	d = iso7816_3_di_table[d_index];
	if (d == 0)
		return -EINVAL;

	/* DI defined in Table 8 of ISO/IEC 7816-3:2006
	 * has values 0001..1001 as div 1, 2, 4, 8, 16, 32, 64, 12, 20
	 * so indices 1..9 are all divisors and the ratio is F/D.
	 * But Indices 1010..1111 are RFU in the 2006 edition!
	 * 1997 used those for 1/2 .. 1/64, where dividing by 1/d equals multiplying by d.
	 * Keep that legacy interpretation for the RFU range only. */
	if (d_index < 10)
		ret = f / d;
	else
		ret = f * d;

	return ret;
}
