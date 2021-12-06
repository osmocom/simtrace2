/* SIMtrace 2 firmware crc stub
 *
 * (C) 2021 by sysmocom -s.f.m.c. GmbH, Author: Eric Wild <ewild@sysmocom.de>
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
 */

#include <stdint.h>
#include "board.h"
#include "core_cm3.h"
#include "usb/device/dfu/dfu.h"

/*
* This file is a bit special, everything has to go to specific sections, and no globals are available.
* No external functions may be called, unless inlining is enforced!
*/

static void crc_check_stub();
__attribute__((section(".crcstub_table"))) volatile uint32_t crcstub_dummy_table[] = {
	(uint32_t)0xdeadc0de, /* deliberately choose invalid value so unpatched image will not be started */
	(uint32_t)crc_check_stub, /* must be valid flash addr */
	(uint32_t)0xf1, /* crc value calculated by the host */
	(uint32_t)0xf2, /* crc calc start address */
	(uint32_t)0xf3 /* crc calc length (byte) */
};

__attribute__((section(".crcstub_code"))) static void do_crc32(int8_t c, uint32_t *crc_reg)
{
	int32_t i, mask;
	*crc_reg ^= c;

	for (unsigned int j = 0; j < 8; j++)
		if (*crc_reg & 1)
			*crc_reg = (*crc_reg >> 1) ^ 0xEDB88320;
		else
			*crc_reg = *crc_reg >> 1;
}

__attribute__((section(".crcstub_code"), noinline)) static void crc_check_stub()
{
	uint32_t crc_reg = 0xffffffff;
	uint32_t expected_crc_val = crcstub_dummy_table[2];
	uint8_t *crc_calc_startaddr = (uint8_t *)crcstub_dummy_table[3];
	volatile uint32_t *actual_exc_tbl = (volatile uint32_t *)crc_calc_startaddr;
	uint32_t crc_len = crcstub_dummy_table[4];

	/* 4000ms wdt tickling */
	WDT->WDT_MR = WDT_MR_WDRSTEN | WDT_MR_WDDBGHLT | WDT_MR_WDIDLEHLT | (((4000UL << 8) / 1000) << 16) |
		      ((4000UL << 8) / 1000);

	for (uint8_t *i = crc_calc_startaddr; i < crc_calc_startaddr + crc_len; i++)
		do_crc32(*i, &crc_reg);

	crc_reg = ~crc_reg;

	if (crc_reg == expected_crc_val) {
		/* this looks a bit awkward because we have to ensure the bx does not require a sp-relative load */
		__asm__ volatile("\
		mov r0, %0;\n\
		mov r1, %1;\n\
		MSR msp,r0;\n\
		bx r1;"
				 :
				 : "r"(actual_exc_tbl[0]), "r"(actual_exc_tbl[1]));
	} else {
		/* no globals ! */
		((struct dfudata *)0x20000000)->magic = USB_DFU_MAGIC;
		__DSB();
		for (;;) {
			/* no functon call, since NVIC_SystemReset() might not be inlined! */
			SCB->AIRCR = ((0x5FA << SCB_AIRCR_VECTKEY_Pos) | (SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) |
				      SCB_AIRCR_SYSRESETREQ_Msk);
			__DSB();
			while (1)
				;
		}
	}
}
