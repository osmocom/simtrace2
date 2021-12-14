/* AT91SAM3 USB string descriptor builder 
 * (C) 2006-2017 by Harald Welte <laforge@gnumonks.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by 
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

/* Based on existing utf8_to_utf16le() function,
 * Copyright (C) 2003 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if 0
static int utf8_to_utf16le(const char *s, uint16_t *cp, unsigned len)
{
	int	count = 0;
	uint8_t	c;
	uint16_t	uchar;

	/* this insists on correct encodings, though not minimal ones.
	 * BUT it currently rejects legit 4-byte UTF-8 code points,
	 * which need surrogate pairs.  (Unicode 3.1 can use them.)
	 */
	while (len != 0 && (c = (uint8_t) *s++) != 0) {
		if (c & 0x80) {
			// 2-byte sequence:
			// 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
			if ((c & 0xe0) == 0xc0) {
				uchar = (c & 0x1f) << 6;

				c = (uint8_t) *s++;
				if ((c & 0xc0) != 0xc0)
					goto fail;
				c &= 0x3f;
				uchar |= c;

			// 3-byte sequence (most CJKV characters):
			// zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
			} else if ((c & 0xf0) == 0xe0) {
				uchar = (c & 0x0f) << 12;

				c = (uint8_t) *s++;
				if ((c & 0xc0) != 0xc0)
					goto fail;
				c &= 0x3f;
				uchar |= c << 6;

				c = (uint8_t) *s++;
				if ((c & 0xc0) != 0xc0)
					goto fail;
				c &= 0x3f;
				uchar |= c;

				/* no bogus surrogates */
				if (0xd800 <= uchar && uchar <= 0xdfff)
					goto fail;

			// 4-byte sequence (surrogate pairs, currently rare):
			// 11101110wwwwzzzzyy + 110111yyyyxxxxxx
			//     = 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
			// (uuuuu = wwww + 1)
			// FIXME accept the surrogate code points (only)

			} else
				goto fail;
		} else
			uchar = c;

		*cp++ = uchar;
		count++;
		len--;
	}
	return count;
fail:
	return -1;
}

#define COLUMNS		6
static int print_array16(uint16_t *buf, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		int mod = i % COLUMNS;
		char *suffix;
		char *prefix;

		switch (mod) {
		case 0:
			if (i == 0)
				prefix = "\t";
			else
				prefix= "\t\t\t";
			suffix = ", ";
			break;
		case COLUMNS-1:
			prefix = "";
			suffix = ",\n";
			break;
		default:
			prefix = "";
			suffix = ", ";
			break;
		}
			
		printf("%s0x%04x%s", prefix, buf[i], suffix);
	}
}
#endif

static void print_structhdr(int i, int size)
{
	printf("static const unsigned char string%d[] = {\n", i);
	printf("\tUSBStringDescriptor_LENGTH(%d),\n", size);
	printf("\tUSBGenericDescriptor_STRING,\n");
}
static void print_structftr(void)
{
	printf("};\n\n");
}

int main(int argc, char **argv)
{
	char asciibuf[512+1];
	uint16_t utf16buf[1024+1];
	int len;
	int j, i = 1;

	printf("#pragma once\n\n");
	printf("/* THIS FILE IS AUTOGENERATED, DO NOT MODIFY MANUALLY */\n\n");
	printf("#include \"usb/include/USBDescriptors.h\"\n\n");

	print_structhdr(0, 1);
	printf("\tUSBStringDescriptor_ENGLISH_US\n");
	print_structftr();
#if 0	
	printf("static const struct usb_string_descriptor string0 = {\n"
	       "\t.bLength = sizeof(string0) + 1 * sizeof(uint16_t),\n"
	       "\t.bDescriptorType = USB_DT_STRING,\n"
	       "\t.wData[0] = 0x0409, /* English */\n"
	       "};\n\n");
#endif

	while (scanf("%512[^\n]\n", asciibuf) != EOF) {
		len = strlen(asciibuf);
		printf("/* String %u \"%s\" */\n", i, asciibuf);
		print_structhdr(i, len);

		for (j = 0; j < len; j++)
			printf("\tUSBStringDescriptor_UNICODE('%c'),\n", asciibuf[j]);

		print_structftr();

		i++;
	}

	printf("const unsigned char *usb_strings[] = {\n");
	for (j = 0; j < i; j++) 
		printf("\tstring%d,\n", j);
	printf("};\n\n");

	exit(0);
}
