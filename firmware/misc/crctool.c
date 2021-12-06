/* SIMtrace 2 firmware crc tool
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
#include <stdio.h>
#include <stdlib.h>

static void do_crc32(int8_t c, uint32_t *crc_reg)
{
	int32_t i, mask;
	*crc_reg ^= c;

	for (unsigned int j = 0; j < 8; j++)
		if (*crc_reg & 1)
			*crc_reg = (*crc_reg >> 1) ^ 0xEDB88320;
		else
			*crc_reg = *crc_reg >> 1;
}

int main(int argc, char *argv[])
{
	uint32_t crc_reg = 0xffffffff;
	long fsize;
	uint8_t *buffer;
	uint32_t crc_start_offset;

	if (argc < 3) {
		perror("usage: crctool startoffset blupdate.bin");
		return -1;
	}

	crc_start_offset = strtoull(argv[1], 0, 10);

	FILE *blfile = fopen(argv[2], "rb+");
	if (blfile == NULL) {
		perror("error opening file!");
		return -1;
	}

	fseek(blfile, 0, SEEK_END);
	fsize = ftell(blfile);

	if (fsize <= crc_start_offset) {
		perror("file size?!");
		return -1;
	}

	fseek(blfile, 0, SEEK_SET);

	buffer = malloc(fsize);
	fread(buffer, 1, fsize, blfile);

	if (*(uint32_t *)buffer != 0xdeadc0de) {
		perror("weird magic, not a valid blupdate file?");
		free(buffer);
		return -1;
	}

	uint8_t *startaddr = buffer + crc_start_offset;
	uint8_t *endaddr = buffer + fsize;
	for (uint8_t *i = startaddr; i < endaddr; i++) {
		do_crc32(*i, &crc_reg);
	}
	crc_reg = ~crc_reg;

	fprintf(stderr, "len: %ld crc: %.8x\n", fsize - crc_start_offset, crc_reg);

	((uint32_t *)buffer)[0] = 0x2000baa0; /* fix magic to valid stack address, checked by BL */
	((uint32_t *)buffer)[2] = crc_reg;
	((uint32_t *)buffer)[3] = 0x00400000 + 0x4000 + crc_start_offset;
	((uint32_t *)buffer)[4] = fsize - crc_start_offset;

	fseek(blfile, 0, SEEK_SET);
	fwrite(buffer, 4, 5, blfile);

	fclose(blfile);
	free(buffer);
	return 0;
}
