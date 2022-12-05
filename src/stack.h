#ifndef LV_PMAN_STACK_H_INCLUDED
#define LV_PMAN_STACK_H_INCLUDED


#include <stdlib.h>
#include "lv_page_manager_conf.h"
#include "page.h"


typedef struct {
    size_t index;
    size_t num;
#ifdef LV_PMAN_PAGE_STACK_DEPTH
    lv_pman_page_t items[LV_PMAN_PAGE_STACK_DEPTH];
#else
#error "TODO"
    lv_pman_page_t *items;
#endif
} lv_pman_page_stack_t;


void            lv_pman_page_stack_init(lv_pman_page_stack_t *pstack);
lv_pman_page_t *lv_pman_page_stack_push(lv_pman_page_stack_t *pstack, lv_pman_page_t *ppage);
int             lv_pman_page_stack_pop(lv_pman_page_stack_t *pstack, lv_pman_page_t *ppage);
lv_pman_page_t *lv_pman_page_stack_top(lv_pman_page_stack_t *pstack);
void            lv_pman_page_stack_dequeue(lv_pman_page_stack_t *pstack);
uint8_t         lv_pman_page_stack_is_empty(lv_pman_page_stack_t *pstack);
uint8_t         lv_pman_page_stack_is_full(lv_pman_page_stack_t *pstack);


#endif