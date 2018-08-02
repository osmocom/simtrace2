/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support 
 * ----------------------------------------------------------------------------
 * Copyright (c) 2008, Atmel Corporation
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 */
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

signed int vfprintf_sync(FILE *pStream, const char *pFormat, va_list ap);
signed int vprintf_sync(const char *pFormat, va_list ap);
signed int printf_sync(const char *pFormat, ...);
int fputc_sync(int c, FILE *stream);
int fputs_sync(const char *s, FILE *stream);
