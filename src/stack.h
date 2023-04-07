#ifndef PMAN_STACK_H_INCLUDED
#define PMAN_STACK_H_INCLUDED


#include <stdlib.h>
#include "page_manager_conf.h"
#include "page.h"


typedef struct {
    size_t index;
    size_t num;
#ifdef PMAN_PAGE_STACK_DEPTH
    pman_page_t items[PMAN_PAGE_STACK_DEPTH];
#else
#error "TODO"
    pman_page_t *items;
#endif
} pman_page_stack_t;


void            pman_page_stack_init(pman_page_stack_t *pstack);
pman_page_t *pman_page_stack_push(pman_page_stack_t *pstack, pman_page_t *ppage);
int             pman_page_stack_pop(pman_page_stack_t *pstack, pman_page_t *ppage);
pman_page_t *pman_page_stack_top(pman_page_stack_t *pstack);
void            pman_page_stack_dequeue(pman_page_stack_t *pstack);
uint8_t         pman_page_stack_is_empty(pman_page_stack_t *pstack);
uint8_t         pman_page_stack_is_full(pman_page_stack_t *pstack);


#endif
