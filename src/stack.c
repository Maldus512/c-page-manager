#include <stdint.h>
#include "stack.h"


#ifdef LV_PMAN_PAGE_STACK_DEPTH
#define ARRAY_LENGTH(stack) LV_PMAN_PAGE_STACK_DEPTH
#else
#define ARRAY_LENGTH(stack) 0
#endif


void lv_pman_page_stack_init(lv_pman_page_stack_t *pstack) {
    pstack->index = 0;
}


lv_pman_page_t *lv_pman_page_stack_push(lv_pman_page_stack_t *pstack, lv_pman_page_t *ppage) {
    if (pstack->index == ARRAY_LENGTH(pstack)) {
        return NULL;
    }

    size_t page_index         = pstack->index++;
    pstack->items[page_index] = *ppage;

    return &pstack->items[page_index];
}


int lv_pman_page_stack_pop(lv_pman_page_stack_t *pstack, lv_pman_page_t *ppage) {
    if (pstack->index == 0) {
        return -1;
    }

    if (ppage) {
        *ppage = pstack->items[pstack->index - 1];
    }
    pstack->index--;

    return 0;
}


lv_pman_page_t *lv_pman_page_stack_top(lv_pman_page_stack_t *pstack) {
    if (pstack->index == 0) {
        return NULL;
    }

    return &pstack->items[pstack->index - 1];
}


void lv_pman_page_stack_dequeue(lv_pman_page_stack_t *pstack) {
    if (pstack->index > 0) {
        for (size_t i = 0; i < pstack->index - 1; i++) {
            pstack->items[i] = pstack->items[i + 1];
        }

        pstack->index--;
    }
}


uint8_t lv_pman_page_stack_is_empty(lv_pman_page_stack_t *pstack) {
    return pstack->index == 0;
}


uint8_t lv_pman_page_stack_is_full(lv_pman_page_stack_t *pstack) {
    size_t const capacity = ARRAY_LENGTH(pstack->items);
    return pstack->index == capacity;
}