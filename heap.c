// heap.c
#include "heap.h"
#include <stdlib.h>
#include <assert.h>

heap_t *heap_init(void) {
    heap_t *h = malloc(sizeof(heap_t));
    h->data = NULL;
    h->size = 0;
    h->capacity = 0;
    return h;
}

int32_t heap_add(heap_t *heap, void *ptr) {
    if (heap->size == heap->capacity) {
        heap->capacity = heap->capacity ? heap->capacity * 2 : 4;
        heap->data = realloc(heap->data, heap->capacity * sizeof(void *));
    }
    heap->data[heap->size] = ptr;
    return heap->size++;
}

void *heap_get(heap_t *heap, int32_t ref) {
    assert(ref >= 0 && ref < (int32_t)heap->size);
    return heap->data[ref];
}

void heap_free(heap_t *heap) {
    for (uint32_t i = 0; i < heap->size; i++) {
        free(heap->data[i]);
    }
    free(heap->data);
    free(heap);
}
