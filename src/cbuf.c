#include <stdint.h>
#include "cbuf.h"

int cbuf_empty(cbuf_t* b)
{
    return (b->head == b->tail);
}

int cbuf_full(cbuf_t* b)
{
    return ((b->tail + 1) % sizeof b->buffer) == b->head;
}

void cbuf_insert(cbuf_t* b, uint32_t data)
{
    if(!cbuf_full(b))
    {
        b->buffer[b->tail] = data;
        b->tail = (b->tail + 1) % sizeof b->buffer;
    }
}

uint32_t cbuf_dequeue(cbuf_t* b)
{
    uint32_t data = 0;

    if(!cbuf_empty(b))
    {
        data = b->buffer[b->head];
        b->head = (b->head + 1) % sizeof b->buffer;
    }

    return data;
}
