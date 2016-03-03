#include "ringbuffer.h"
#include "trace.h"

void rbuf_reset(volatile ringbuf * rb)
{
	rb->ird = 0;
	rb->iwr = 0;
}

uint8_t rbuf_read(volatile ringbuf * rb)
{
	uint8_t val = rb->buf[rb->ird];
	rb->ird = (rb->ird + 1) % RING_BUFLEN;
	return val;
}

uint8_t rbuf_peek(volatile ringbuf * rb)
{
	return rb->buf[rb->ird];
}

void rbuf_write(volatile volatile ringbuf * rb, uint8_t item)
{
	if (!rbuf_is_full(rb)) {
		rb->buf[rb->iwr] = item;
		rb->iwr = (rb->iwr + 1) % RING_BUFLEN;
	} else {
		TRACE_ERROR("Ringbuffer full, losing bytes!");
	}
}

bool rbuf_is_empty(volatile ringbuf * rb)
{
	return rb->ird == rb->iwr;
}

bool rbuf_is_full(volatile ringbuf * rb)
{
	return rb->ird == (rb->iwr + 1) % RING_BUFLEN;
}
