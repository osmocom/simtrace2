typedef struct ringbuf {
    uint8_t buf[BUFLEN];
    uint8_t *buf_end;
    uint8_t *reader;
    uint8_t *writer;
} ringbuf;

void rbuf_init(ringbuf *rb)
{
    rb->buf_end = buf[BUFLEN-1];
    rb->buf = {0};
    rb->reader = rb->buf[0];
    rb->writer = rb->buf[0];
}

uint8_t rbuf_read(ringbuf *rb)
{
    uint8_t val = *(rb->reader);
    if (rb->reader == rb->buf_end) {
        rb->reader = rb->buf;
    } else{
        rb->reader++;
    }
    return val;
}

void rbuf_write(ringbuf *rb, uint8_t item) {
    *(rb->writer) = item;
    if (rb->writer == rb->buf_end) {
        rb->writer = rb->buf;
    } else{
        rb->writer++;
    }
}
