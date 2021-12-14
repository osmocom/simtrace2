/* Code to switch between local (physical) and remote (emulated) SIM
 *
 * (C) 2021 by Harald Welte <hwelte@hmw-consulting.de>
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
#include "sim_switch.h"

int sim_switch_use_physical(unsigned int nr, int physical)
{
	if (physical) {
		TRACE_ERROR("%u: Use local/physical SIM - UNSUPPORTED!\r\n", nr);
	} else {
		TRACE_INFO("%u: Use remote/emulated SIM\r\n", nr);
	}

	return 0;
}

int sim_switch_init(void)
{
	return 1; // SIMtrace hardware has only one switchable interface
}
