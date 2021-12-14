/* Code to control the PERST lines of attached modems
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

int wwan_perst_set(int modem_nr, int active);
int wwan_perst_do_reset_pulse(int modem_nr, unsigned int duration_ms);
int wwan_perst_init(void);
