#include <stdint.h>
#include "stack.h"


#ifdef PMAN_PAGE_STACK_DEPTH
#define ARRAY_LENGTH(stack) PMAN_PAGE_STACK_DEPTH
#else
#define ARRAY_LENGTH(stack) 0
#endif


void pman_page_stack_init(pman_page_stack_t *pstack) {
    pstack->index = 0;
}


pman_page_t *pman_page_stack_push(pman_page_stack_t *pstack, pman_page_t *ppage) {
    if (pstack->index == ARRAY_LENGTH(pstack)) {
        return NULL;
    }

    size_t page_index         = pstack->index++;
    pstack->items[page_index] = *ppage;

    return &pstack->items[page_index];
}


int pman_page_stack_pop(pman_page_stack_t *pstack, pman_page_t *ppage) {
    if (pstack->index == 0) {
        return -1;
    }

    if (ppage) {
        *ppage = pstack->items[pstack->index - 1];
    }
    pstack->index--;

    return 0;
}


pman_page_t *pman_page_stack_top(pman_page_stack_t *pstack) {
    if (pstack->index == 0) {
        return NULL;
    }

    return &pstack->items[pstack->index - 1];
}


void pman_page_stack_dequeue(pman_page_stack_t *pstack) {
    if (pstack->index > 0) {
        for (size_t i = 0; i < pstack->index - 1; i++) {
            pstack->items[i] = pstack->items[i + 1];
        }

        pstack->index--;
    }
}


uint8_t pman_page_stack_is_empty(pman_page_stack_t *pstack) {
    return pstack->index == 0;
}


uint8_t pman_page_stack_is_full(pman_page_stack_t *pstack) {
    size_t const capacity = ARRAY_LENGTH(pstack->items);
    return pstack->index == capacity;
}
