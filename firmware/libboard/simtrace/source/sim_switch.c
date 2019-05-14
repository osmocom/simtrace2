/* Code to switch between local (physical) and remote (emulated) SIM
 *
 * (C) 2015-2017 by Harald Welte <hwelte@hmw-consulting.de>
 * (C) 2018, sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>
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
#include "trace.h"
#include "led.h"
#include "sim_switch.h"

int sim_switch_use_physical(unsigned int nr, int physical)
{
	const Pin pin_sc = PIN_SC_SW_DEFAULT; // pin to control bus switch for VCC/RST/CLK signals
	const Pin pin_io = PIN_IO_SW_DEFAULT; // pin to control bus switch for I/O signal

	if (nr > 0) {
		TRACE_ERROR("SIM interface for Modem %d can't be switched\r\n", nr);
		return -1;
	}

	TRACE_INFO("Modem %u: %s SIM\n\r", nr, physical ? "physical" : "virtual");

	if (physical) {
		TRACE_INFO("%u: Use local/physical SIM\r\n", nr);
		PIO_Set(&pin_sc);
		PIO_Set(&pin_io);
	} else {
		TRACE_INFO("%u: Use remote/emulated SIM\r\n", nr);
		PIO_Clear(&pin_sc);
		PIO_Clear(&pin_io);
	}

	return 0;
}

int sim_switch_init(void)
{
	// the bus switch is already initialised
	return 1; // SIMtrace hardware has only one switchable interface
}
