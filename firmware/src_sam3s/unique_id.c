#include "chip.h"

#define EFC_FCMD_STUI	0x0E
#define EFC_FCMD_SPUI	0x0F

__attribute__ ((section(".ramfunc")))
void EEFC_ReadUniqueID(unsigned int *pdwUniqueID)
{
	unsigned int status;

	/* Errata / Workaround: Set bit 16 of EEFC Flash Mode Register
	 * to 1 */
	EFC->EEFC_FMR |= (1 << 16);

	/* Send the Start Read unique Identifier command (STUI) by
	 * writing the Flash Command Register with the STUI command. */
	EFC->EEFC_FCR = (0x5A << 24) | EFC_FCMD_STUI;

	/* Wait for the FRDY bit to fall */
	do {
		status = EFC->EEFC_FSR;
	} while ((status & EEFC_FSR_FRDY) == EEFC_FSR_FRDY);

	/* The Unique Identifier is located in the first 128 bits of the
	 * Flash memory mapping. So, at the address 0x400000-0x400003.
	 * */
	pdwUniqueID[0] = *(uint32_t *) IFLASH_ADDR;
	pdwUniqueID[1] = *(uint32_t *) (IFLASH_ADDR + 4);
	pdwUniqueID[2] = *(uint32_t *) (IFLASH_ADDR + 8);
	pdwUniqueID[3] = *(uint32_t *) (IFLASH_ADDR + 12);

	/* To stop the Unique Identifier mode, the user needs to send
	 * the Stop Read unique Identifier command (SPUI) by writing the
	 * Flash Command Register with the SPUI command. */
	EFC->EEFC_FCR = (0x5A << 24) | EFC_FCMD_SPUI;

	/* When the Stop read Unique Unique Identifier command (SPUI)
	 * has been performed, the FRDY bit in the Flash Programming
	 * Status Register (EEFC_FSR) rises. */
	do {
		status = EFC->EEFC_FSR;
	} while ((status & EEFC_FSR_FRDY) != EEFC_FSR_FRDY);
}
