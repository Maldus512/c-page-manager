#ifndef PMAN_H_INCLUDED
#define PMAN_H_INCLUDED

#include <stdint.h>
#include "page_manager_conf.h"
#include "stack.h"
#ifndef PMAN_EXCLUDE_LVGL
#include "lvgl.h"
#endif


#define PMAN_REGISTER_TIMER_ID(handle, period, id) pman_timer_create(handle, period, ((void *)(uintptr_t)id))


typedef void (*pman_user_msg_cb_t)(pman_handle_t, void *);


/**
 * @brief Page manager structure
 *
 */
typedef struct {
    // Page stack
    pman_page_stack_t page_stack;

#ifndef PMAN_EXCLUDE_LVGL
    // Reference to the touch input device; used to reset the touch state when changing page
    lv_indev_t *touch_indev;
#endif

    // Callback to process user messages (i.e. system commands)
    pman_user_msg_cb_t user_msg_cb;

    // If present, called every time a page is closed
    void (*close_global_cb)(void *handle);

    // If present, called every an event is fired
    uint8_t (*event_global_cb)(void *handle, pman_event_t event);

    // User pointer
    void *user_data;
} pman_t;


void pman_init(pman_t *pman, void *user_data,
#ifndef PMAN_EXCLUDE_LVGL
               lv_indev_t *indev,
#endif
               pman_user_msg_cb_t user_msg_cb, void (*close_global_cb)(void *handle),
               uint8_t (*event_global_cb)(void *handle, pman_event_t event));
void    pman_change_page(pman_t *pman, pman_page_t page);
void    pman_change_page_extra(pman_t *pman, pman_page_t newpage, void *extra);
void    pman_back(pman_t *pman);
void    pman_rebase_page(pman_t *pman, pman_page_t newpage);
void    pman_rebase_page_extra(pman_t *pman, pman_page_t newpage, void *extra);
void    pman_swap_page(pman_t *pman, pman_page_t newpage);
void    pman_swap_page_extra(pman_t *pman, pman_page_t newpage, void *extra);
void    pman_reset_to_page_id(pman_t *pman, int id, uint8_t *found);
void    pman_event(pman_t *pman, pman_event_t event);
void    pman_destroy_all(void *state, void *extra);
void    pman_close_all(void *state);
void   *pman_get_user_data(pman_handle_t handle);
uint8_t pman_is_current_page_id(pman_t *pman, int id);
int     pman_get_current_page_id(pman_t *pman);
#ifndef PMAN_EXCLUDE_LVGL
void pman_register_obj_event(pman_handle_t handle, lv_obj_t *obj, lv_event_code_t event);
void pman_unregister_obj_event(lv_obj_t *obj);
void pman_set_obj_self_destruct(lv_obj_t *obj);
void pman_register_obj_id_and_number(pman_handle_t handle, lv_obj_t *obj, int id, int number);

void         *pman_timer_get_user_data(pman_timer_t *timer);
pman_timer_t *pman_timer_create(pman_handle_t handle, uint32_t period, void *user_data);
void          pman_timer_delete(pman_timer_t *timer);
void          pman_timer_ready(pman_timer_t *timer);
void          pman_timer_resume(pman_timer_t *timer);
void          pman_timer_reset(pman_timer_t *timer);
void          pman_timer_pause(pman_timer_t *timer);
void          pman_timer_set_period(pman_timer_t *timer, uint32_t arg);
#endif

#endif
