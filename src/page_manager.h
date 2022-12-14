#ifndef LV_PMAN_H_INCLUDED
#define LV_PMAN_H_INCLUDED

#include <stdint.h>
#include "lvgl.h"
#include "lv_page_manager_conf.h"
#include "stack.h"


typedef struct {
    lv_pman_page_stack_t page_stack;
    lv_indev_t          *touch_indev;
    void (*controller_cb)(void *, lv_pman_controller_msg_t);
    void *args;
} lv_pman_t;


void    lv_pman_init(lv_pman_t *pman, void *args, lv_indev_t *indev,
                     void (*controller_cb)(void *, lv_pman_controller_msg_t));
void    lv_pman_change_page(lv_pman_t *pman, void *args, lv_pman_page_t page);
void    lv_pman_change_page_extra(lv_pman_t *pman, void *args, lv_pman_page_t newpage, void *extra);
void    lv_pman_back(lv_pman_t *pman, void *args);
void    lv_pman_rebase_page(lv_pman_t *pman, void *args, lv_pman_page_t newpage);
void    lv_pman_rebase_page_extra(lv_pman_t *pman, void *args, lv_pman_page_t newpage, void *extra);
void    lv_pman_swap_page(lv_pman_t *pman, void *args, lv_pman_page_t newpage);
void    lv_pman_swap_page_extra(lv_pman_t *pman, void *args, lv_pman_page_t newpage, void *extra);
void    lv_pman_reset_to_page(lv_pman_t *pman, void *args, int id, uint8_t *found);
void    lv_pman_event(lv_pman_t *pman, lv_pman_event_t event);
void    lv_pman_register_obj_id(lv_pman_page_handle_t handle, lv_obj_t *obj, int id);
void    lv_pman_register_obj_id_and_number(lv_pman_page_handle_t handle, lv_obj_t *obj, int id, int number);
void    lv_pman_destroy_all(void *data, void *extra);
void    lv_pman_close_all(void *data);
uint8_t lv_pman_is_page_open(lv_pman_page_handle_t handle);



#endif