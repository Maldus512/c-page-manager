#ifndef LV_PMAN_H_INCLUDED
#define LV_PMAN_H_INCLUDED

#include <stdint.h>
#include "lvgl.h"
#include "lv_page_manager_conf.h"
#include "stack.h"


typedef void (*lv_pman_user_msg_cb_t)(lv_pman_handle_t, void *);


/**
 * @brief Page manager structure
 *
 */
typedef struct {
    // Page stack
    lv_pman_page_stack_t page_stack;

    // Reference to the touch input device; used to reset the touch state when changing page
    lv_indev_t *touch_indev;

    // Callback to process user messages (i.e. system commands)
    lv_pman_user_msg_cb_t user_msg_cb;

    // User pointer
    void *user_data;
} lv_pman_t;


void  lv_pman_init(lv_pman_t *pman, void *user_data, lv_indev_t *indev, lv_pman_user_msg_cb_t user_msg_cb);
void  lv_pman_change_page(lv_pman_t *pman, lv_pman_page_t page);
void  lv_pman_change_page_extra(lv_pman_t *pman, lv_pman_page_t newpage, void *extra);
void  lv_pman_back(lv_pman_t *pman);
void  lv_pman_rebase_page(lv_pman_t *pman, lv_pman_page_t newpage);
void  lv_pman_rebase_page_extra(lv_pman_t *pman, lv_pman_page_t newpage, void *extra);
void  lv_pman_swap_page(lv_pman_t *pman, lv_pman_page_t newpage);
void  lv_pman_swap_page_extra(lv_pman_t *pman, lv_pman_page_t newpage, void *extra);
void  lv_pman_reset_to_page_id(lv_pman_t *pman, int id, uint8_t *found);
void  lv_pman_event(lv_pman_t *pman, lv_pman_event_t event);
void  lv_pman_register_obj_id(lv_pman_handle_t handle, lv_obj_t *obj, int id);
void  lv_pman_register_obj_id_and_number(lv_pman_handle_t handle, lv_obj_t *obj, int id, int number);
void  lv_pman_destroy_all(void *state, void *extra);
void  lv_pman_close_all(void *state);
void *lv_pman_get_user_data(lv_pman_handle_t handle);


#endif