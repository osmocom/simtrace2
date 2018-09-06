/* Card simulator specific functions
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

#include "chip.h"
#include "board.h"
#include "utils.h"

static const Pin pins_cardsim[] = PINS_CARDSIM;

void board_exec_dbg_cmd(int ch)
{
	switch (ch) {
	case '?':
		printf("\t?\thelp\n\r");
		printf("\tR\treset SAM3\n\r");
		break;
	case 'R':
		printf("Asking NVIC to reset us\n\r");
		USBD_Disconnect();
		NVIC_SystemReset();
		break;
	default:
		printf("Unknown command '%c'\n\r", ch);
		break;
	}
}

void cardsim_set_simpres(uint8_t slot, int present)
{
	if (slot > 1)
		return;

	if (present)
		PIO_Set(&pins_cardsim[slot]);
	else
		PIO_Clear(&pins_cardsim[slot]);
}

void cardsim_gpio_init(void)
{
	PIO_Configure(pins_cardsim, ARRAY_SIZE(pins_cardsim));
}
