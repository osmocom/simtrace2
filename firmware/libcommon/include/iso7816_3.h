/*
 * Copyright (C) 2019 sysmocom -s.f.m.c. GmbH, Author: Kevin Redon <kredon@sysmocom.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/* this library provides utilities to handle the ISO-7816 part 3 communication aspects (e.g. related
 * to F and D) */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/*! default clock rate conversion integer Fd.
 *  ISO/IEC 7816-3:2006(E) section 8.1 */
#define ISO7816_3_DEFAULT_FD 372

/*! default baud rate adjustment factor Dd.
 *  ISO/IEC 7816-3:2006(E) section 8.1 */
#define ISO7816_3_DEFAULT_DD 1

/*! default clock rate conversion integer Fi.
 *  ISO/IEC 7816-3:2006(E) section 8.3
 *  \note non-default value is optionally specified in TA1 */
#define ISO7816_3_DEFAULT_FI 372

/*! default baud rate adjustment factor Di.
 *  ISO/IEC 7816-3:2006(E) section 8.3
 *  \note non-default value is optionally specified in TA1 */
#define ISO7816_3_DEFAULT_DI 1

/*! default maximum clock frequency, in Hz.
 *  ISO/IEC 7816-3:2006(E) section 8.3
 *  \note non-default value is optionally specified in TA1 */
#define ISO7816_3_DEFAULT_FMAX 5000000UL

/*! default Waiting Integer (WI) value for T=0.
 *  ISO/IEC 7816-3:2006(E) section 10.2
 *  \note non-default value is optionally specified in TC2 */
#define ISO7816_3_DEFAULT_WI 10

/*! default Waiting Time (WT) value, in ETU.
 *  ISO/IEC 7816-3:2006(E) section 8.1
 *  \note depends on Fi, Di, and WI if protocol T=0 is selected */
#define ISO7816_3_DEFAULT_WT 9600

extern const uint16_t iso7816_3_fi_table[];

extern const uint32_t iso7816_3_fmax_table[];

extern const uint8_t iso7816_3_di_table[];

bool iso7816_3_valid_f(uint16_t f);

bool iso7816_3_valid_d(uint8_t d);

int32_t iso7816_3_calculate_wt(uint8_t wi, uint16_t fi, uint8_t di, uint16_t f, uint8_t d);
