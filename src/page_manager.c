#include <assert.h>
#include "page_manager.h"
#include "stack.h"


#define PTR_TO_U32(x) ((uint32_t)(uintptr_t)x)


static void clear_page_stack(lv_pman_t *pman);
static void wait_release(lv_pman_t *pman);
static void reset_page(lv_pman_t *pman);
static void free_user_data_callback(lv_event_t *event);
static void page_subscription_cb(void *s, lv_msg_t *msg);
static void controller_subscription_cb(void *s, lv_msg_t *lv_msg);
static void event_callback(lv_event_t *event);


void lv_pman_init(lv_pman_t *pman, void *args, lv_indev_t *indev,
                  void (*controller_cb)(void *, lv_pman_controller_msg_t)) {
    pman->touch_indev   = indev;
    pman->args          = args;
    pman->controller_cb = controller_cb;

    lv_msg_subsribe(LV_PMAN_CONTROLLER_MSG_ID, controller_subscription_cb, pman);
    lv_pman_page_stack_init(&pman->page_stack);
}


/*
 * Page stack management
 */


void lv_pman_swap_page_extra(lv_pman_t *pman, void *args, lv_pman_page_t newpage, void *extra) {
    lv_pman_page_t *current = lv_pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    if (current->close) {
        current->close(current->data);
    }
    if (current->destroy) {
        current->destroy(current->data, current->extra);
    }

    lv_pman_page_stack_pop(&pman->page_stack, NULL);

    current = lv_pman_page_stack_push(&pman->page_stack, &newpage);
    assert(current != NULL);

    current->extra = extra;
    // Create the newpage
    if (current->create) {
        current->data = current->create(current, args, current->extra);
    } else {
        current->data = NULL;
    }

    if (current->open) {
        current->open(current, args, current->data);
    }
    reset_page(pman);
}


void lv_pman_swap_page(lv_pman_t *pman, void *args, lv_pman_page_t newpage) {
    lv_pman_swap_page_extra(pman, args, newpage, NULL);
}


void lv_pman_reset_to_page(lv_pman_t *pman, void *args, int id, uint8_t *found) {
    if (found) {
        *found = 0;
    }

    lv_pman_page_t *current = lv_pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    if (current->close) {
        current->close(current->data);
    }

    do {
        if (current->id == id) {
            if (found) {
                *found = 1;
            }
            if (current->open) {
                current->open(current, args, current->data);
            }
            reset_page(pman);
            break;
        } else if (current->destroy) {
            current->destroy(current->data, current->extra);
        }

        lv_pman_page_stack_pop(&pman->page_stack, NULL);
    } while ((current = lv_pman_page_stack_top(&pman->page_stack)) != NULL);
}


void lv_pman_rebase_page_extra(lv_pman_t *pman, void *args, lv_pman_page_t newpage, void *extra) {
    lv_pman_page_t *current = lv_pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    if (current->close) {
        current->close(current->data);
    }
    if (current->destroy) {
        current->destroy(current->data, current->extra);
    }

    clear_page_stack(pman);

    current = lv_pman_page_stack_push(&pman->page_stack, &newpage);
    assert(current != NULL);

    current->extra = extra;
    // Create the newpage
    if (current->create) {
        current->data = current->create(current, args, current->extra);
    } else {
        current->data = NULL;
    }

    // Open the page
    if (current->open) {
        current->open(current, args, current->data);
    }
    reset_page(pman);
}


void lv_pman_rebase_page(lv_pman_t *pman, void *args, lv_pman_page_t newpage) {
    lv_pman_rebase_page_extra(pman, args, newpage, NULL);
}


void lv_pman_change_page_extra(lv_pman_t *pman, void *args, lv_pman_page_t newpage, void *extra) {
    lv_pman_page_t *current = lv_pman_page_stack_top(&pman->page_stack);
    if (current != NULL) {
        if (current->close) {
            current->close(current->data);
        }
    }

    current = lv_pman_page_stack_push(&pman->page_stack, &newpage);
    assert(current != NULL);

    current->extra = extra;

    // Create the newpage
    if (current->create) {
        current->data = current->create(current, args, extra);
    } else {
        current->data = NULL;
    }

    // Open the page
    if (current->open) {
        current->open(current, args, current->data);
    }
    reset_page(pman);
}


void lv_pman_change_page(lv_pman_t *pman, void *args, lv_pman_page_t page) {
    lv_pman_change_page_extra(pman, args, page, NULL);
}


void lv_pman_back(lv_pman_t *pman, void *args) {
    lv_pman_page_t page;

    if (lv_pman_page_stack_pop(&pman->page_stack, &page) == 0) {
        if (page.close) {
            page.close(page.data);
        }
        if (page.destroy) {
            page.destroy(page.data, page.extra);
        }

        lv_pman_page_t *current = lv_pman_page_stack_top(&pman->page_stack);
        assert(current != NULL);
        if (current->open) {
            current->open(current, args, current->data);
        }
        reset_page(pman);
    }
}


/*
 *  Event management
 */


lv_pman_controller_msg_t lv_pman_process_page_event(lv_pman_t *pman, void *args, lv_pman_event_t event) {
    lv_pman_page_t *current = lv_pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    lv_pman_msg_t msg = current->process_event(current, args, current->data, event);

    switch (msg.vmsg.tag) {
        case LV_PMAN_VIEW_MSG_TAG_CHANGE_PAGE:
            lv_pman_change_page(pman, args, *((lv_pman_page_t *)msg.vmsg.page));
            break;

        case LV_PMAN_VIEW_MSG_TAG_CHANGE_PAGE_EXTRA:
            lv_pman_change_page_extra(pman, args, *((lv_pman_page_t *)msg.vmsg.page), msg.vmsg.extra);
            break;

        case LV_PMAN_VIEW_MSG_TAG_BACK:
            lv_pman_back(pman, args);
            break;

        case LV_PMAN_VIEW_MSG_TAG_REBASE:
            lv_pman_rebase_page(pman, args, *((lv_pman_page_t *)msg.vmsg.page));
            break;

        case LV_PMAN_VIEW_MSG_TAG_SWAP:
            lv_pman_swap_page(pman, args, *((lv_pman_page_t *)msg.vmsg.page));
            break;

        case LV_PMAN_VIEW_MSG_TAG_SWAP_EXTRA:
            lv_pman_swap_page_extra(pman, args, *((lv_pman_page_t *)msg.vmsg.page), msg.vmsg.extra);
            break;

        case LV_PMAN_VIEW_MSG_TAG_RESET_TO:
            lv_pman_reset_to_page(pman, args, msg.vmsg.id, NULL);
            break;

        case LV_PMAN_VIEW_MSG_TAG_NOTHING:
            break;
    }

    return msg.cmsg;
}


void lv_pman_register_obj_id_and_number(lv_pman_page_handle_t handle, lv_obj_t *obj, int id, int number) {
    lv_pman_obj_data_t *data = lv_obj_get_user_data(obj);
    if (data == NULL) {
        data = lv_mem_alloc(sizeof(lv_pman_obj_data_t));
        assert(data != NULL);
    }
    data->handle = handle;
    data->id     = id;
    data->number = number;
    lv_obj_set_user_data(obj, data);
    lv_obj_remove_event_cb(obj, free_user_data_callback);
    lv_obj_remove_event_cb(obj, event_callback);
    lv_obj_add_event_cb(obj, free_user_data_callback, LV_EVENT_DELETE, NULL);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_LONG_PRESSED_REPEAT, NULL);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_CANCEL, NULL);
    lv_obj_add_event_cb(obj, event_callback, LV_EVENT_READY, NULL);
}


void lv_pman_register_obj_id(lv_pman_page_handle_t handle, lv_obj_t *obj, int id) {
    lv_pman_register_obj_id_and_number(handle, obj, id, 0);
}


void lv_pman_event(lv_pman_t *pman, lv_pman_event_t event) {
    lv_pman_event_t *event_copy = lv_mem_alloc(sizeof(lv_pman_event_t));
    assert(event_copy != NULL);
    memcpy(event_copy, &event, sizeof(lv_pman_event_t));

    lv_msg_send(PTR_TO_U32(lv_pman_page_stack_top(&pman->page_stack)), event_copy);
}


void lv_pman_destroy_all(void *data, void *extra) {
    (void)extra;
    lv_mem_free(data);
}


void lv_pman_close_all(void *data) {
    (void)data;
    lv_obj_clean(lv_scr_act());
}


/*
 * Private functions
 */


static void reset_page(lv_pman_t *pman) {
    lv_pman_page_t *current = lv_pman_page_stack_top(&pman->page_stack);
    assert(current != NULL);

    lv_msg_subsribe(PTR_TO_U32(current), page_subscription_cb, pman);

    lv_pman_event(pman, (lv_pman_event_t){.tag = LV_PMAN_EVENT_TAG_OPEN});
    wait_release(pman);
}


static void clear_page_stack(lv_pman_t *pman) {
    lv_pman_page_t page;

    while (lv_pman_page_stack_pop(&pman->page_stack, &page) == 0) {
        if (page.destroy) {
            page.destroy(page.data, page.extra);
        }
    }
}


static void wait_release(lv_pman_t *pman) {
    if (pman->touch_indev != NULL) {
        lv_indev_wait_release(pman->touch_indev);
    }
}


static void free_user_data_callback(lv_event_t *event) {
    if (lv_event_get_code(event) == LV_EVENT_DELETE) {
        lv_obj_t           *obj  = lv_event_get_current_target(event);
        lv_pman_obj_data_t *data = lv_obj_get_user_data(obj);
        lv_mem_free(data);
    }
}


static void page_subscription_cb(void *s, lv_msg_t *lv_msg) {
    (void)s;
    const lv_pman_event_t *event = lv_msg_get_payload(lv_msg);
    lv_pman_t             *pman  = lv_msg_get_user_data(lv_msg);

    lv_pman_controller_msg_t *cmsg = lv_mem_alloc(sizeof(lv_pman_controller_msg_t));
    assert(cmsg != NULL);

    *cmsg = lv_pman_process_page_event(pman, pman->args, *event);
    lv_mem_free((void *)event);

    lv_msg_send(LV_PMAN_CONTROLLER_MSG_ID, cmsg);
}


static void controller_subscription_cb(void *s, lv_msg_t *lv_msg) {
    (void)s;
    const lv_pman_controller_msg_t *cmsg = lv_msg_get_payload(lv_msg);
    lv_pman_t                      *pman = lv_msg_get_user_data(lv_msg);

    pman->controller_cb(pman->args, *cmsg);
    lv_mem_free((void *)cmsg);
}


static void event_callback(lv_event_t *event) {
    lv_obj_t *target = lv_event_get_current_target(event);

    lv_pman_obj_data_t *data       = lv_obj_get_user_data(target);
    lv_pman_event_t    *pman_event = lv_mem_alloc(sizeof(lv_pman_event_t));
    assert(pman_event != NULL);
    pman_event->tag         = LV_PMAN_EVENT_TAG_LVGL;
    pman_event->lvgl.event  = lv_event_get_code(event);
    pman_event->lvgl.id     = data->id;
    pman_event->lvgl.number = data->number;
    pman_event->lvgl.target = target;

    lv_msg_send(PTR_TO_U32(data->handle), pman_event);
}