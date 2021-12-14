/* UART print output
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
#include <stdio.h>
#include "uart_console.h"

int fputc(int c, FILE *stream)
{
	UART_PutChar(c);
	return c;
}

int fputs(const char *s, FILE *stream)
{
	while (*s != '\0')
		fputc(*s++, stream);
	return 0;
}

int fputc_sync(int c, FILE *stream)
{
	UART_PutChar_Sync(c);
	return c;
}

int fputs_sync(const char *s, FILE *stream)
{
	while (*s != '\0')
		fputc_sync(*s++, stream);
	return 0;
}
