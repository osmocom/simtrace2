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
		UART_PutChar(*s++);
	return 0;
}
