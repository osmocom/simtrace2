/* Ring buffer
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
#ifndef SIMTRACE_RINGBUF_H
#define SIMTRACE_RINGBUF_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#define RING_BUFLEN 1024

typedef struct ringbuf {
	uint8_t buf[RING_BUFLEN];
	size_t ird;
	size_t iwr;
} ringbuf;

void rbuf_reset(volatile ringbuf * rb);
uint8_t rbuf_read(volatile ringbuf * rb);
uint8_t rbuf_peek(volatile ringbuf * rb);
int rbuf_write(volatile ringbuf * rb, uint8_t item);
bool rbuf_is_empty(volatile ringbuf * rb);
bool rbuf_is_full(volatile ringbuf * rb);


/* same as above but with 16bit values instead of 8bit */

#define RING16_BUFLEN 512

typedef struct ringbuf16 {
	uint16_t buf[RING16_BUFLEN];
	size_t ird;
	size_t iwr;
} ringbuf16;

void rbuf16_reset(volatile ringbuf16 * rb);
uint16_t rbuf16_read(volatile ringbuf16 * rb);
uint16_t rbuf16_peek(volatile ringbuf16 * rb);
int rbuf16_write(volatile ringbuf16 * rb, uint16_t item);
bool rbuf16_is_empty(volatile ringbuf16 * rb);
bool rbuf16_is_full(volatile ringbuf16 * rb);

#endif /* end of include guard: SIMTRACE_RINGBUF_H */
