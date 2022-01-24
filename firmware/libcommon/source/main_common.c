/* SIMtrace 2 firmware common main helpers
 *
 * (C) 2015-2019 by Harald Welte <hwelte@hmw-consulting.de>
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
#include "utils.h"

void print_banner(void)
{
	printf("\r\n\r\n"
		"=============================================================================\r\n"
		"SIMtrace2 firmware " GIT_VERSION ", BOARD=" BOARD ", APP=" APPLICATION "\r\n"
		"(C) 2010-2019 by Harald Welte, 2018-2019 by Kevin Redon\r\n"
		"=============================================================================\r\n");

#if (TRACE_LEVEL >= TRACE_LEVEL_INFO)
	/* print chip-unique ID */
	unsigned int unique_id[4];
	EEFC_ReadUniqueID(unique_id);
	TRACE_INFO("Chip ID: 0x%08lx (Ext 0x%08lx)\r\n", CHIPID->CHIPID_CIDR, CHIPID->CHIPID_EXID);
	TRACE_INFO("Serial Nr. %08x-%08x-%08x-%08x\r\n",
		   unique_id[0], unique_id[1], unique_id[2], unique_id[3]);

	/* print reset cause */
	uint8_t reset_cause = (RSTC->RSTC_SR & RSTC_SR_RSTTYP_Msk) >> RSTC_SR_RSTTYP_Pos;
	static const char* reset_causes[] = {
		"general reset (first power-up reset)",
		"backup reset (return from backup mode)",
		"watchdog reset (watchdog fault occurred)",
		"software reset (processor reset required by the software)",
		"user reset (NRST pin detected low)",
	};
	if (reset_cause < ARRAY_SIZE(reset_causes)) {
		TRACE_INFO("Reset Cause: %s\r\n", reset_causes[reset_cause]);
	} else {
		TRACE_INFO("Reset Cause: 0x%lx\r\n", (RSTC->RSTC_SR & RSTC_SR_RSTTYP_Msk) >> RSTC_SR_RSTTYP_Pos);
	}
#endif
}
