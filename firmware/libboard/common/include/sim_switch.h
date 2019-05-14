/* This program is free software; you can redistribute it and/or modify
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
#pragma once

/** switch card lines to use physical or emulated card
 *  @param[in] nr card interface number (i.e. slot)
 *  @param[in] physical which physical interface to switch to (e.g. 0: physical, 1: virtual)
 *  @return 0 on success, negative else
 */
int sim_switch_use_physical(unsigned int nr, int physical);
/** initialise card switching capabilities
 *  @return number of switchable card interfaces
 */
int sim_switch_init(void);
