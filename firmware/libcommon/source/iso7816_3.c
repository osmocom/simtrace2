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
#include <stdint.h>
#include <stddef.h>

#include "utils.h"
#include "iso7816_3.h"

const uint16_t iso7816_3_fi_table[16] = {
	372, 372, 558, 744, 1116, 1488, 1860, 0,
	0, 512, 768, 1024, 1536, 2048, 0, 0
};

const uint32_t iso7816_3_fmax_table[16] = {
	4000000, 5000000, 6000000, 8000000, 12000000, 16000000, 20000000, 0,
	0, 5000000, 7500000, 10000000, 15000000, 20000000, 0, 0
};

const uint8_t iso7816_3_di_table[16] = {
	0, 1, 2, 4, 8, 16, 32, 64,
	12, 20, 0, 0, 0, 0, 0, 0,
};

/* all values are based on the Elementary Time Unit (ETU), defined in ISO/IEC 7816-3 section 7.1
 * this is the time required to transmit a bit, and is calculated as follows: 1 ETU = (F / D) x (1 / f) where:
 * - F is the clock rate conversion integer
 * - D is the baud rate adjustment factor
 * - f is the clock frequency
 * the possible F, f(max), and D values are defined in ISO/IEC 7816-3 table 7 and 8
 * - the initial value for F (after reset) is Fd = 372
 * - the initial value for D (after reset) is Dd = 1
 * - the initial maximum frequency f(max) is 5 MHz
 * the card must measure the ETU based on the clock signal provided by the reader
 * one ETU (e.g. 1 bit) takes F/D clock cycles, which the card must count
 *
 * the card can indicate an alternative set of supported values Fi (with corresponding f(max)) and Di for higher baud rate in TA1 in the ATR (see ISO/IEC 7816-3 section 8.3)
 * these values are selected according to ISO/IEC 7816-3 section 6.3.1:
 * - card in specific mode: they are enforced if TA2 is present (the reader can deactivate the card if it does not support these values)
 * - card in negotiable mode:
 * -- they can be selected by the reader using the Protocol and Parameters Selection (PPS) procedure
 * -- the first offered protocol and default values are used when no PPS is started
 *
 * PPS is done with Fd and Dd (see ISO/IEC 7816-3 section 9)
 * the reader can propose any F and D values between from Fd to Fi, and from Dd to Di (Fi and Di are indicated in TA1)
 * the in PPS agreed values F and D are called Fn and Dn and are applied after a successful exchange, corresponding to PPS1_Response bit 5
 *
 * the F and D values must be provided to the SAM3S USART peripheral (after reset and PPS)
 */

bool iso7816_3_valid_f(uint16_t f)
{
	if (0 == f) {
		return false;
	}
	uint8_t i = 0;
	for (i = 0; i < ARRAY_SIZE(iso7816_3_fi_table) && iso7816_3_fi_table[i] != f; i++);
	return (i < ARRAY_SIZE(iso7816_3_fi_table) && iso7816_3_fi_table[i] == f);
}

bool iso7816_3_valid_d(uint8_t d)
{
	if (0 == d) {
		return false;
	}
	uint8_t i = 0;
	for (i = 0; i < ARRAY_SIZE(iso7816_3_di_table) && iso7816_3_di_table[i] != d; i++);
	return (i < ARRAY_SIZE(iso7816_3_di_table) && iso7816_3_di_table[i] == d);
}

/*
 * the ETU is not only used to define the baud rate, but also the Waiting Time (WT) (see ISO/IEC 7816-3 section 8.1)
 * when exceeding WT without card response, the reader flags the card as unresponsive, and resets it
 * this can be used by the card to indicate errors or unsupported operations
 * if the card requires more time to respond, it shall send a procedure byte to restart WT
 * WT is calculated as follows (for T=0, see ISO/IEC 7816-3 section 10.2): WT = WI x 960 x (Fi / f(max)) where
 * - WI is encoded in TC2 in the ATR (10 if absent)
 * - WI does not depend on D/Di (used for the ETU)
 * - after reset WT is 9600 ETU
 * - WI (e.g. the new WT) is applied when T=0 is used (after 6.3.1), even if Fi is not Fn (this WT extension is important to know for the reader so to have the right timeout)
 */

int32_t iso7816_3_calculate_wt(uint8_t wi, uint16_t fi, uint8_t di, uint16_t f, uint8_t d)
{
	// sanity checks
	if (0 == wi) {
		return -1;
	}
	if (!iso7816_3_valid_f(fi)) {
		return -2;
	}
	if (!iso7816_3_valid_d(di)) {
		return -3;
	}
	if (!iso7816_3_valid_f(f)) {
		return -4;
	}
	if (!iso7816_3_valid_d(d)) {
		return -5;
	}
	if (f > fi) {
		return -6;
	}
	if (d > di) {
		return -7;
	}

	return wi * 960UL * (fi/f) * (di/d); // calculate timeout value in ETU
}
