#include <assert.h>
#include "page_manager.h"
#include "stack.h"


static void clear_page_stack(lv_pman_t *pman);
static void wait_release(lv_pman_t *pman);
static void reset_page(lv_pman_t *pman);
static void free_user_data_callback(lv_event_t *event);
static void page_subscription_cb(lv_pman_t *pman, lv_pman_event_t event);
static void event_callback(lv_event_t *event);
static void open_page(lv_pman_handle_t handle, lv_pman_page_t *page);
static void close_page(lv_pman_page_t *page);
static void destroy_page(lv_pman_page_t *page);


/**
 * @brief Initialize the page manager instance
 *
 * @param pman Pointer to the page manager instance
 * @param user_data user pointer
 * @param indev optional input device reference
 * @param user_msg_cb function to handle user messages
 */
void lv_pman_init(lv_pman_t *pman, void *user_data, lv_indev_t *indev, lv_pman_user_msg_cb_t user_msg_cb) {
    pman->touch_indev = indev;
    pman->user_data   = user_data;
    pman->user_msg_cb = user_msg_cb;

    lv_pman_page_stack_init(&pman->page_stack);
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
void lv_pman_swap_page_extra(lv_pman_t *pman, lv_pman_page_t newpage, void *extra) {
    lv_pman_page_t *current = lv_pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    close_page(current);
    destroy_page(current);

    lv_pman_page_stack_pop(&pman->page_stack, NULL);

    current = lv_pman_page_stack_push(&pman->page_stack, &newpage);
    assert(current != NULL);

    current->extra = extra;
    // Create the newpage
    if (current->create) {
        current->state = current->create(pman->user_data, current->extra);
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
void lv_pman_swap_page(lv_pman_t *pman, lv_pman_page_t newpage) {
    lv_pman_swap_page_extra(pman, newpage, NULL);
}


/**
 * @brief Reset the page stack to the highest instance of page with the corresponding id. All pages until the target are
 * closed and destroyed. If no such page is found, clears the whole stack.
 *
 * @param pman
 * @param id
 * @param found whether the target page was found or not
 */
void lv_pman_reset_to_page_id(lv_pman_t *pman, int id, uint8_t *found) {
    if (found) {
        *found = 0;
    }

    lv_pman_page_t *current = lv_pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    close_page(current);

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

        lv_pman_page_stack_pop(&pman->page_stack, NULL);
    } while ((current = lv_pman_page_stack_top(&pman->page_stack)) != NULL);
}


/**
 * @brief Clears the whole stack and adds a new page, passing also the extra argument. All previous pages are closed and
 * destroyed
 *
 * @param pman
 * @param newpage
 * @param extra
 */
void lv_pman_rebase_page_extra(lv_pman_t *pman, lv_pman_page_t newpage, void *extra) {
    lv_pman_page_t *current = lv_pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    close_page(current);
    clear_page_stack(pman);

    current = lv_pman_page_stack_push(&pman->page_stack, &newpage);
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
void lv_pman_rebase_page(lv_pman_t *pman, lv_pman_page_t newpage) {
    lv_pman_rebase_page_extra(pman, newpage, NULL);
}


/**
 * @brief Changes the current page passing also the extra argument, adding it on top of the stack. The previous page is
 * closed.
 *
 * @param pman
 * @param newpage
 * @param extra
 */
void lv_pman_change_page_extra(lv_pman_t *pman, lv_pman_page_t newpage, void *extra) {
    lv_pman_page_t *current = lv_pman_page_stack_top(&pman->page_stack);
    if (current != NULL) {
        close_page(current);
    }

    current = lv_pman_page_stack_push(&pman->page_stack, &newpage);
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
void lv_pman_change_page(lv_pman_t *pman, lv_pman_page_t page) {
    lv_pman_change_page_extra(pman, page, NULL);
}


void lv_pman_back(lv_pman_t *pman) {
    lv_pman_page_t page;

    if (lv_pman_page_stack_pop(&pman->page_stack, &page) == 0) {
        close_page(&page);
        destroy_page(&page);

        lv_pman_page_t *current = lv_pman_page_stack_top(&pman->page_stack);
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
void *lv_pman_process_page_event(lv_pman_t *pman, lv_pman_event_t event) {
    lv_pman_page_t *current = lv_pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    lv_pman_msg_t msg = current->process_event(pman, current->state, event);

    switch (msg.vmsg.tag) {
        case LV_PMAN_VIEW_MSG_TAG_CHANGE_PAGE:
            lv_pman_change_page(pman, *((lv_pman_page_t *)msg.vmsg.page));
            break;

        case LV_PMAN_VIEW_MSG_TAG_CHANGE_PAGE_EXTRA:
            lv_pman_change_page_extra(pman, *((lv_pman_page_t *)msg.vmsg.page), msg.vmsg.extra);
            break;

        case LV_PMAN_VIEW_MSG_TAG_BACK:
            lv_pman_back(pman);
            break;

        case LV_PMAN_VIEW_MSG_TAG_REBASE:
            lv_pman_rebase_page(pman, *((lv_pman_page_t *)msg.vmsg.page));
            break;

        case LV_PMAN_VIEW_MSG_TAG_SWAP:
            lv_pman_swap_page(pman, *((lv_pman_page_t *)msg.vmsg.page));
            break;

        case LV_PMAN_VIEW_MSG_TAG_SWAP_EXTRA:
            lv_pman_swap_page_extra(pman, *((lv_pman_page_t *)msg.vmsg.page), msg.vmsg.extra);
            break;

        case LV_PMAN_VIEW_MSG_TAG_RESET_TO:
            lv_pman_reset_to_page_id(pman, msg.vmsg.id, NULL);
            break;

        case LV_PMAN_VIEW_MSG_TAG_NOTHING:
            break;
    }

    return msg.user_msg;
}


/**
 * @brief Register an lvgl object in the default event management system. Events sent by the objects will be forwarded
 * to the page. It includes an ID code and a number.
 *
 * @param handle Handle for event management, passed to page callbacks
 * @param obj
 * @param id
 * @param number
 */
void lv_pman_register_obj_id_and_number(lv_pman_handle_t handle, lv_obj_t *obj, int id, int number) {
    lv_pman_obj_data_t *data = lv_obj_get_user_data(obj);
    if (data == NULL) {
        data = lv_mem_alloc(sizeof(lv_pman_obj_data_t));
        assert(data != NULL);
    }
    data->id     = id;
    data->number = number;

    lv_obj_set_user_data(obj, data);
    lv_obj_remove_event_cb(obj, free_user_data_callback);
    lv_obj_remove_event_cb(obj, event_callback);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_CLICKED, handle);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_VALUE_CHANGED, handle);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_RELEASED, handle);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_PRESSED, handle);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_PRESSING, handle);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_LONG_PRESSED, handle);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_LONG_PRESSED_REPEAT, handle);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_CANCEL, handle);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_READY, handle);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_DELETE, handle);
    lv_obj_add_event_cb(obj, free_user_data_callback, LV_EVENT_DELETE, handle);
}


/**
 * @brief Register an lvgl object in the default event management system. Events sent by the objects will be forwarded
 * to the page.
 *
 * @param handle Handle for event management, passed to page callbacks
 * @param obj
 * @param id
 */
void lv_pman_register_obj_id(lv_pman_handle_t handle, lv_obj_t *obj, int id) {
    lv_pman_register_obj_id_and_number(handle, obj, id, 0);
}


/**
 * @brief Get the user pointer registered for the page manager instance
 *
 * @param handle
 * @return void*
 */
void *lv_pman_get_user_data(lv_pman_handle_t handle) {
    lv_pman_t *pman = handle;
    return pman->user_data;
}


/**
 * @brief Send an event to the current page
 *
 * @param pman
 * @param event
 */
void lv_pman_event(lv_pman_t *pman, lv_pman_event_t event) {
    page_subscription_cb(pman, event);
}


/**
 * @brief Utility function to be assigned to the "destroy" page callback. It clears all page state (attempting to free
 * it)
 *
 * @param state
 * @param extra
 */
void lv_pman_destroy_all(void *state, void *extra) {
    (void)extra;
    lv_mem_free(state);
}


/**
 * @brief Utility function to be assigned to the "close" page callback. It clears all LVGL objects on screen.
 *
 * @param state
 * @param extra
 */
void lv_pman_close_all(void *state) {
    (void)state;
    lv_obj_clean(lv_scr_act());
}


/*
 * Private functions
 */


/**
 * @brief Open a page that finds itself on top of the stack
 *
 * @param pman
 */
static void reset_page(lv_pman_t *pman) {
    lv_pman_page_t *current = lv_pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    lv_pman_event(pman, (lv_pman_event_t){.tag = LV_PMAN_EVENT_TAG_OPEN});
    wait_release(pman);
}


/**
 * @brief Clears the whole page stack, assuming all pages have already been closed
 *
 * @param pman
 */
static void clear_page_stack(lv_pman_t *pman) {
    lv_pman_page_t page;

    while (lv_pman_page_stack_pop(&pman->page_stack, &page) == 0) {
        destroy_page(&page);
    }
}


/**
 * @brief Signal the input device to wait for realease before sending new events
 *
 * @param pman
 */
static void wait_release(lv_pman_t *pman) {
    if (pman->touch_indev != NULL) {
        lv_indev_wait_release(pman->touch_indev);
    }
}


/**
 * @brief Callback that frees the user data associated with an object. To be tied to the LV_EVENT_DELETE event.
 *
 * @param event
 */
static void free_user_data_callback(lv_event_t *event) {
    if (lv_event_get_code(event) == LV_EVENT_DELETE) {
        lv_obj_t           *obj  = lv_event_get_current_target(event);
        lv_pman_obj_data_t *data = lv_obj_get_user_data(obj);
        lv_mem_free(data);
    }
}


/**
 * @brief Page subscription to events
 *
 * @param pman
 * @param event
 */
static void page_subscription_cb(lv_pman_t *pman, lv_pman_event_t event) {
    void *user_msg = lv_pman_process_page_event(pman, event);
    pman->user_msg_cb(pman, user_msg);
}


/**
 * @brief LVGL events callback
 *
 * @param event
 */
static void event_callback(lv_event_t *event) {
    lv_obj_t *target = lv_event_get_current_target(event);

    lv_pman_obj_data_t *data       = lv_obj_get_user_data(target);
    lv_pman_event_t     pman_event = {
            .tag = LV_PMAN_EVENT_TAG_LVGL,
            .lvgl =
            {
                    .event  = lv_event_get_code(event),
                    .id     = data->id,
                    .number = data->number,
                    .target = target,
            },
    };

    lv_pman_handle_t handle = lv_event_get_user_data(event);

    page_subscription_cb(handle, pman_event);
}


/**
 * @brief Destroys a page
 *
 * @param page
 */
static void destroy_page(lv_pman_page_t *page) {
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
static void open_page(lv_pman_handle_t handle, lv_pman_page_t *page) {
    if (page->open) {
        page->open(handle, page->state);
    }
}


/**
 * @brief Closes a page
 *
 * @param page
 */
static void close_page(lv_pman_page_t *page) {
    if (page->close) {
        page->close(page->state);
    }
}