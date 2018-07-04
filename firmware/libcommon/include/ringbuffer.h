#ifndef SIMTRACE_RINGBUF_H
#define SIMTRACE_RINGBUF_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

#define RING_BUFLEN 256

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

#endif /* end of include guard: SIMTRACE_RINGBUF_H */
