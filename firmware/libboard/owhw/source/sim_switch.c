/* Code to switch between local (physical) and remote (emulated) SIM
 *
 * (C) 2015-2017 by Harald Welte <hwelte@hmw-consulting.de>
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

#ifdef PIN_SIM_SWITCH1
static const Pin pin_conn_usim1 = {PIO_PA20, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
#endif
#ifdef PIN_SIM_SWITCH2
static const Pin pin_conn_usim2 = {PIO_PA28, PIOA, ID_PIOA, PIO_OUTPUT_0, PIO_DEFAULT};
#endif

static int initialized = 0;

int sim_switch_use_physical(unsigned int nr, int physical)
{
	const Pin *pin;
	enum led led;

	if (!initialized) {
		TRACE_ERROR("Somebody forgot to call sim_switch_init()\r\n");
		sim_switch_init();
	}

	TRACE_INFO("Modem %d: %s SIM\n\r", nr,
		   physical ? "physical" : "virtual");

	switch (nr) {
#ifdef PIN_SIM_SWITCH1
	case 0:
		pin = &pin_conn_usim1;
		led = LED_USIM1;
		break;
#endif
#ifdef PIN_SIM_SWITCH2
	case 1:
		pin = &pin_conn_usim2;
		led = LED_USIM2;
		break;
#endif
	default:
		TRACE_ERROR("Invalid SIM%u\n\r", nr);
		return -1;
	}

	if (physical) {
		TRACE_INFO("%u: Use local/physical SIM\r\n", nr);
		PIO_Clear(pin);
		led_blink(led, BLINK_ALWAYS_ON);
	} else {
		TRACE_INFO("%u: Use remote/emulated SIM\r\n", nr);
		PIO_Set(pin);
		led_blink(led, BLINK_ALWAYS_OFF);
	}

	return 0;
}

int sim_switch_init(void)
{
	int num_switch = 0;
#ifdef PIN_SIM_SWITCH1
	PIO_Configure(&pin_conn_usim1, 1);
	num_switch++;
#endif
#ifdef PIN_SIM_SWITCH2
	PIO_Configure(&pin_conn_usim2, 1);
	num_switch++;
#endif
	initialized = 1;
	return num_switch;
}
