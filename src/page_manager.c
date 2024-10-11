#include <assert.h>
#include "page_manager_timer.h"
#include "page_manager.h"
#include "page.h"
#include "stack.h"


#define DEFINE_TIMER_WRAPPER(fun)                                                                                      \
    void pman_timer_##fun(pman_timer_t *timer) { lv_timer_##fun(timer->timer); }
#define DEFINE_TIMER_WRAPPER_ARG(fun, type)                                                                            \
    void pman_timer_##fun(pman_timer_t *timer, type arg) { lv_timer_##fun(timer->timer, arg); }



static void clear_page_stack(pman_t *pman);
static void wait_release(pman_t *pman);
static void reset_page(pman_t *pman);
static void page_subscription_cb(pman_t *pman, pman_event_t event);
static void open_page(pman_handle_t handle, pman_page_t *page);
static void close_page(pman_t *pman, pman_page_t *page);
static void destroy_page(pman_page_t *page);
#ifndef PMAN_EXCLUDE_LVGL
static void free_user_data_callback(lv_event_t *event);
static void event_callback(lv_event_t *event);
static void timer_callback(lv_timer_t *timer);

#if LVGL_VERSION_MAJOR >= 9
#define lv_mem_free  lv_free
#define lv_mem_alloc lv_malloc
#endif

#endif


/**
 * @brief Initialize the page manager instance
 *
 * @param pman Pointer to the page manager instance
 * @param user_data user pointer
 * @param indev optional input device reference
 * @param user_msg_cb function to handle user messages
 * @param close_global_cb if not NULL, called every time a page is closed
 */
void pman_init(pman_t *pman, void *user_data,
#ifndef PMAN_EXCLUDE_LVGL
               lv_indev_t *indev,
#endif
               pman_user_msg_cb_t user_msg_cb, void (*close_global_cb)(void *handle),
               uint8_t (*event_global_cb)(void *handle, pman_event_t event)) {
#ifndef PMAN_EXCLUDE_LVGL
    pman->touch_indev = indev;
#endif
    pman->user_data       = user_data;
    pman->user_msg_cb     = user_msg_cb;
    pman->close_global_cb = close_global_cb;
    pman->event_global_cb = event_global_cb;

    pman_page_stack_init(&pman->page_stack);
}


/*
 * Page stack management
 */


/**
 * @brief Swap the current page with another one, passing also the extra argument. The current page is closed and
 * destroyed and the new page takes its place on top of the stack
 *
 * @param pman
 * @param user_data
 * @param newpage
 * @param extra
 */
void pman_swap_page_extra(pman_t *pman, pman_page_t newpage, void *extra) {
    pman_page_t *current = pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    close_page(pman, current);
    destroy_page(current);

    pman_page_stack_pop(&pman->page_stack, NULL);

    current = pman_page_stack_push(&pman->page_stack, &newpage);
    assert(current != NULL);

    current->extra = extra;
    // Create the newpage
    if (current->create) {
        current->state = current->create(pman, current->extra);
    } else {
        current->state = NULL;
    }

    open_page(pman, current);
    reset_page(pman);
}


/**
 * @brief Swap the current page with another one. The current page is closed and
 * destroyed and the new page takes its place on top of the stack
 *
 * @param pman
 * @param newpage
 * @param extra
 */
void pman_swap_page(pman_t *pman, pman_page_t newpage) {
    pman_swap_page_extra(pman, newpage, NULL);
}


int pman_get_current_page_id(pman_t *pman) {
    pman_page_t *current = pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);
    return current->id;
}


/**
 * @brief Reset the page stack to the highest instance of page with the corresponding id. All pages until the target are
 * closed and destroyed. If no such page is found, clears the whole stack.
 *
 * @param pman
 * @param id
 * @param found whether the target page was found or not
 */
void pman_reset_to_page_id(pman_t *pman, int id, uint8_t *found) {
    if (found) {
        *found = 0;
    }

    pman_page_t *current = pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    close_page(pman, current);

    do {
        if (current->id == id) {
            if (found) {
                *found = 1;
            }

            open_page(pman, current);
            reset_page(pman);
            break;
        } else {
            destroy_page(current);
        }

        pman_page_stack_pop(&pman->page_stack, NULL);
    } while ((current = pman_page_stack_top(&pman->page_stack)) != NULL);
}


uint8_t pman_is_current_page_id(pman_t *pman, int id) {
    pman_page_t *current = pman_page_stack_top(&pman->page_stack);
    if (current == NULL) {
        return 0;
    } else {
        return current->id == id;
    }
}


/**
 * @brief Clears the whole stack and adds a new page, passing also the extra argument. All previous pages are closed
 * and destroyed
 *
 * @param pman
 * @param newpage
 * @param extra
 */
void pman_rebase_page_extra(pman_t *pman, pman_page_t newpage, void *extra) {
    pman_page_t *current = pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    close_page(pman, current);
    clear_page_stack(pman);

    current = pman_page_stack_push(&pman->page_stack, &newpage);
    assert(current != NULL);

    current->extra = extra;
    // Create the newpage
    if (current->create) {
        current->state = current->create(pman, current->extra);
    } else {
        current->state = NULL;
    }

    // Open the page
    open_page(pman, current);
    reset_page(pman);
}


/**
 * @brief Clears the whole stack and adds a new page. All previous pages are closed and
 * destroyed
 *
 * @param pman
 * @param newpage
 */
void pman_rebase_page(pman_t *pman, pman_page_t newpage) {
    pman_rebase_page_extra(pman, newpage, NULL);
}


/**
 * @brief Changes the current page passing also the extra argument, adding it on top of the stack. The previous page
 * is closed.
 *
 * @param pman
 * @param newpage
 * @param extra
 */
void pman_change_page_extra(pman_t *pman, pman_page_t newpage, void *extra) {
    pman_page_t *current = pman_page_stack_top(&pman->page_stack);
    if (current != NULL) {
        close_page(pman, current);
    }

    current = pman_page_stack_push(&pman->page_stack, &newpage);
    assert(current != NULL);

    current->extra = extra;

    // Create the newpage
    if (current->create) {
        current->state = current->create(pman, extra);
    } else {
        current->state = NULL;
    }

    // Open the page
    open_page(pman, current);
    reset_page(pman);
}


/**
 * @brief Changes the current page, adding it on top of the stack. The previous page is
 * closed.
 *
 * @param pman
 * @param newpage
 */
void pman_change_page(pman_t *pman, pman_page_t page) {
    pman_change_page_extra(pman, page, NULL);
}


void pman_back(pman_t *pman) {
    pman_page_t page;

    if (pman_page_stack_pop(&pman->page_stack, &page) == 0) {
        close_page(pman, &page);
        destroy_page(&page);

        pman_page_t *current = pman_page_stack_top(&pman->page_stack);
        assert(current != NULL);

        open_page(pman, current);
        reset_page(pman);
    }
}


/*
 *  Event management
 */


/**
 * @brief Processes an event, sending it to the current page and returning a message from the page to the system.
 *
 * @param pman
 * @param event
 * @return void*
 */
void *pman_process_page_event(pman_t *pman, pman_event_t event) {
    pman_page_t *current = pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    pman_msg_t msg = current->process_event(pman, current->state, event);

    switch (msg.stack_msg.tag) {
        case PMAN_STACK_MSG_TAG_PUSH_PAGE:
            pman_change_page(pman, *((pman_page_t *)msg.stack_msg.as.destination.page));
            break;

        case PMAN_STACK_MSG_TAG_PUSH_PAGE_EXTRA:
            pman_change_page_extra(pman, *((pman_page_t *)msg.stack_msg.as.destination.page),
                                   msg.stack_msg.as.destination.extra);
            break;

        case PMAN_STACK_MSG_TAG_BACK:
            pman_back(pman);
            break;

        case PMAN_STACK_MSG_TAG_REBASE:
            pman_rebase_page(pman, *((pman_page_t *)msg.stack_msg.as.destination.page));
            break;

        case PMAN_STACK_MSG_TAG_SWAP:
            pman_swap_page(pman, *((pman_page_t *)msg.stack_msg.as.destination.page));
            break;

        case PMAN_STACK_MSG_TAG_SWAP_EXTRA:
            pman_swap_page_extra(pman, *((pman_page_t *)msg.stack_msg.as.destination.page),
                                 msg.stack_msg.as.destination.extra);
            break;

        case PMAN_STACK_MSG_TAG_RESET_TO:
            pman_reset_to_page_id(pman, msg.stack_msg.as.id, NULL);
            break;

        case PMAN_STACK_MSG_TAG_NOTHING:
            break;
    }

    return msg.user_msg;
}


#ifndef PMAN_EXCLUDE_LVGL
void pman_unregister_obj_event(lv_obj_t *obj) {
    lv_obj_remove_event_cb(obj, event_callback);
}


void pman_register_obj_event(pman_handle_t handle, lv_obj_t *obj, lv_event_code_t event) {
    lv_obj_add_event_cb(obj, event_callback, event, handle);
}


void pman_set_obj_self_destruct(lv_obj_t *obj) {
    lv_obj_remove_event_cb(obj, free_user_data_callback);
    lv_obj_add_event_cb(obj, free_user_data_callback, LV_EVENT_DELETE, NULL);
}
#endif


/**
 * @brief Get the user pointer registered for the page manager instance
 *
 * @param handle
 * @return void*
 */
void *pman_get_user_data(pman_handle_t handle) {
    pman_t *pman = handle;
    return pman->user_data;
}


/**
 * @brief Send an event to the current page
 *
 * @param pman
 * @param event
 */
void pman_event(pman_t *pman, pman_event_t event) {
    page_subscription_cb(pman, event);
}


/**
 * @brief Utility function to be assigned to the "destroy" page callback. It clears all page state (attempting to
 * free it)
 *
 * @param state
 * @param extra
 */
void pman_destroy_all(void *state, void *extra) {
    (void)extra;
#ifndef PMAN_EXCLUDE_LVGL
    lv_mem_free(state);
#endif
}


/**
 * @brief Utility function to be assigned to the "close" page callback. It clears all LVGL objects on screen.
 *
 * @param state
 * @param extra
 */
void pman_close_all(void *state) {
    (void)state;
#ifndef PMAN_EXCLUDE_LVGL
    lv_obj_clean(lv_scr_act());
#endif
}


#ifndef PMAN_EXCLUDE_LVGL
pman_timer_t *pman_timer_create(pman_handle_t handle, uint32_t period, void *user_data) {
    pman_timer_t *timer = lv_mem_alloc(sizeof(pman_timer_t));
    if (timer == NULL) {
        return NULL;
    }

    timer->handle    = handle;
    timer->user_data = user_data;
    timer->timer     = lv_timer_create(timer_callback, period, timer);
    lv_timer_set_repeat_count(timer->timer, -1);
    lv_timer_pause(timer->timer);

    return timer;
}


void pman_timer_delete(pman_timer_t *timer) {
    lv_timer_del(timer->timer);
    lv_mem_free(timer);
}


void *pman_timer_get_user_data(pman_timer_t *timer) {
    return timer->user_data;
}


DEFINE_TIMER_WRAPPER(ready)
DEFINE_TIMER_WRAPPER(resume)
DEFINE_TIMER_WRAPPER(reset)
DEFINE_TIMER_WRAPPER(pause)
DEFINE_TIMER_WRAPPER_ARG(set_period, uint32_t)
#endif


/*
 * Private functions
 */


/**
 * @brief Open a page that finds itself on top of the stack
 *
 * @param pman
 */
static void reset_page(pman_t *pman) {
    pman_page_t *current = pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    pman_event(pman, (pman_event_t){.tag = PMAN_EVENT_TAG_OPEN});
    wait_release(pman);
}


/**
 * @brief Clears the whole page stack, assuming all pages have already been closed
 *
 * @param pman
 */
static void clear_page_stack(pman_t *pman) {
    pman_page_t page;

    while (pman_page_stack_pop(&pman->page_stack, &page) == 0) {
        destroy_page(&page);
    }
}


/**
 * @brief Signal the input device to wait for realease before sending new events
 *
 * @param pman
 */
static void wait_release(pman_t *pman) {
#ifndef PMAN_EXCLUDE_LVGL
    if (pman->touch_indev != NULL) {
        lv_indev_wait_release(pman->touch_indev);
    }
#endif
}


#ifndef PMAN_EXCLUDE_LVGL
/**
 * @brief Callback that frees the user data associated with an object. To be tied to the LV_EVENT_DELETE event.
 *
 * @param event
 */
static void free_user_data_callback(lv_event_t *event) {
    if (lv_event_get_code(event) == LV_EVENT_DELETE) {
        lv_obj_t *obj  = lv_event_get_current_target(event);
        void     *data = lv_obj_get_user_data(obj);
        lv_mem_free(data);
    }
}
#endif


/**
 * @brief Page subscription to events
 *
 * @param pman
 * @param event
 */
static void page_subscription_cb(pman_t *pman, pman_event_t event) {
    void *user_msg = pman_process_page_event(pman, event);

    uint8_t override = 0;
    if (pman->event_global_cb != NULL) {
        override = pman->event_global_cb(pman, event);
    }

    if (!override && pman->user_msg_cb) {
        pman->user_msg_cb(pman, user_msg);
    }
}


#ifndef PMAN_EXCLUDE_LVGL
/**
 * @brief LVGL events callback
 *
 * @param event
 */
static void event_callback(lv_event_t *event) {
    pman_event_t pman_event = {
        .tag = PMAN_EVENT_TAG_LVGL,
        .as  = {.lvgl = event},
    };

    pman_handle_t handle = lv_event_get_user_data(event);
    page_subscription_cb(handle, pman_event);
}


/**
 * @brief LVGL timers callback
 *
 * @param timer
 */
static void timer_callback(lv_timer_t *timer) {
    pman_timer_t *pman_timer = lv_timer_get_user_data(timer);

    pman_event_t pman_event = {
        .tag = PMAN_EVENT_TAG_TIMER,
        .as  = {.timer = pman_timer},
    };

    page_subscription_cb(pman_timer->handle, pman_event);
}
#endif


/**
 * @brief Destroys a page
 *
 * @param page
 */
static void destroy_page(pman_page_t *page) {
    if (page->destroy) {
        page->destroy(page->state, page->extra);
    }
}


/**
 * @brief Opens a page
 *
 * @param handle
 * @param page
 */
static void open_page(pman_handle_t handle, pman_page_t *page) {
    if (page->open) {
        page->open(handle, page->state);
    }
}


/**
 * @brief Closes a page
 *
 * @param pman
 * @param page
 */
static void close_page(pman_t *pman, pman_page_t *page) {
    if (pman->close_global_cb != NULL) {
        pman->close_global_cb(pman);
    }
    if (page->close) {
        page->close(page->state);
    }
}
