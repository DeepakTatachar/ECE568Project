#ifndef CBUF_H_
#define CBUF_H_

typedef struct cbuffer {
    uint32_t buffer[16];
    int head;
    int tail;
} cbuf_t;


int cbuf_empty(cbuf_t* b);
int cbuf_full(cbuf_t* b);
void cbuf_insert(cbuf_t* b, uint32_t data);
uint32_t cbuf_dequeue(cbuf_t* b);


#endif /* CBUF_H_ */
