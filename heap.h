// heap.h
#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

typedef struct {
    void **data;
    uint32_t size;
    uint32_t capacity;
} heap_t;

heap_t *heap_init(void);
int32_t heap_add(heap_t *heap, void *ptr);
void *heap_get(heap_t *heap, int32_t ref);
void heap_free(heap_t *heap);

#endif
