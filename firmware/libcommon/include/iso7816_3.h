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
/* this library provides utilities to handle the ISO-7816 part 3 communication aspects (e.g. related to F and D) */
#pragma once

#include <stdint.h>
#include <stdbool.h>

/** default clock rate conversion integer Fd
 *  @implements ISO/IEC 7816-3:2006(E) section 8.1
 */
#define ISO7816_3_DEFAULT_FD 372
/** default baud rate adjustment factor Dd
 *  @implements ISO/IEC 7816-3:2006(E) section 8.1
 */
#define ISO7816_3_DEFAULT_DD 1
/** default clock rate conversion integer Fi
 *  @implements ISO/IEC 7816-3:2006(E) section 8.3
 *  @note non-default value is optionally specified in TA1
 */
#define ISO7816_3_DEFAULT_FI 372
/** default baud rate adjustment factor Di
 *  @implements ISO/IEC 7816-3:2006(E) section 8.3
 *  @note non-default value is optionally specified in TA1
 */
#define ISO7816_3_DEFAULT_DI 1
/** default maximum clock frequency, in Hz
 *  @implements ISO/IEC 7816-3:2006(E) section 8.3
 *  @note non-default value is optionally specified in TA1
 */
#define ISO7816_3_DEFAULT_FMAX 5000000UL
/** default Waiting Integer (WI) value for T=0
 *  @implements ISO/IEC 7816-3:2006(E) section 10.2
 *  @note non-default value is optionally specified in TC2
 */
#define ISO7816_3_DEFAULT_WI 10
/** default Waiting Time (WT) value, in ETU
 *  @implements ISO/IEC 7816-3:2006(E) section 8.1
 *  @note depends on Fi, Di, and WI if protocol T=0 is selected
 */
#define ISO7816_3_DEFAULT_WT 9600

/** Table encoding the clock rate conversion integer Fi
 *  @note Fi is indicated in TA1, but the same table is used for F and Fn during PPS
 *  @implements ISO/IEC 7816-3:2006(E) table 7
 */
extern const uint16_t iso7816_3_fi_table[];

/** Table encoding the maximum clock frequency f_max in Hz
 *  @implements ISO/IEC 7816-3:2006(E) table 7
 *  @note f_max is indicated in TA1, but the same table is used for F and Fn during PPS
 */
extern const uint32_t iso7816_3_fmax_table[];

/** Table encoding the baud rate adjust integer Di
 *  @implements ISO/IEC 7816-3:2006(E) table 8
 *  @note Di is indicated in TA1, but the same table is used for D and Dn during PPS
 */
extern const uint8_t iso7816_3_di_table[];

/* verify if the clock rate conversion integer F value is valid
 * @param[in] f F value to be validated
 * @return if F value is valid
 * @note only values in ISO/IEC 7816-3:2006(E) table 7 are valid
 */
bool iso7816_3_valid_f(uint16_t f);
/* verify if the baud rate adjustment factor D value is valid
 * @param[in] d D value to be validated
 * @return if D value is valid
 * @note only values in ISO/IEC 7816-3:2006(E) table 8 are valid
 */
bool iso7816_3_valid_d(uint8_t d);
/** calculate Waiting Time (WT)
 *  @param[in] wi Waiting Integer
 *  @param[in] fi clock rate conversion integer Fi value
 *  @param[in] di baud rate adjustment factor Di value
 *  @param[in] f clock rate conversion integer F value
 *  @param[in] d baud rate adjustment factor D value
 *  @return Waiting Time WT, in ETU, or < 0 on error (see code for return codes)
 *  @note this should happen after reset and T=0 protocol select (through PPS or implicit)
 *  @implements ISO/IEC 7816-3:2006(E) section 8.1 and 10.2
 */
int32_t iso7816_3_calculate_wt(uint8_t wi, uint16_t fi, uint8_t di, uint16_t f, uint8_t d);
