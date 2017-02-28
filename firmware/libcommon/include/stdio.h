#pragma once
#include <stddef.h>
#include <stdarg.h>


#ifndef EOF
#define EOF (-1)
#endif

struct File;
typedef struct File FILE;

extern FILE* const stdin;
extern FILE* const stdout;
extern FILE* const stderr;

signed int vsnprintf(char *pStr, size_t length, const char *pFormat, va_list ap);
signed int snprintf(char *pString, size_t length, const char *pFormat, ...);
signed int vsprintf(char *pString, const char *pFormat, va_list ap);
signed int vfprintf(FILE *pStream, const char *pFormat, va_list ap);
signed int vprintf(const char *pFormat, va_list ap);
signed int fprintf(FILE *pStream, const char *pFormat, ...);
signed int printf(const char *pFormat, ...);
signed int sprintf(char *pStr, const char *pFormat, ...);
signed int puts(const char *pStr);


int fputc(int c, FILE *stream);
int fputs(const char *s, FILE *stream);

#define putc(c, stream) fputc(c, stream)
#define putchar(c) fputc(c, stdout)
