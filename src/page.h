#ifndef LV_PMAN_PAGE_H_INCLUDED
#define LV_PMAN_PAGE_H_INCLUDED


#include "lv_page_manager_conf.h"
#include "lv_pman_timer.h"
#include "lvgl.h"


#define LV_PMAN_MSG_NULL        ((lv_pman_msg_t){.user_msg = NULL, .vmsg = {.tag = LV_PMAN_STACK_MSG_TAG_NOTHING}})
#define LV_PMAN_USER_EVENT(ptr) ((lv_pman_event_t){.tag = LV_PMAN_EVENT_TAG_USER, .as = {.user = ptr}})


/**
 * @brief Tags for view messages (i.e. commands that act on the page stack)
 *
 */
typedef enum {
    LV_PMAN_STACK_MSG_TAG_NOTHING = 0,           // Do nothing
    LV_PMAN_STACK_MSG_TAG_BACK,                  // Go back to the previous page
    LV_PMAN_STACK_MSG_TAG_REBASE,                // Rebase to a new page
    LV_PMAN_STACK_MSG_TAG_RESET_TO,              // Reset to a previous page
    LV_PMAN_STACK_MSG_TAG_CHANGE_PAGE,           // Change to a new page
    LV_PMAN_STACK_MSG_TAG_CHANGE_PAGE_EXTRA,     // Change to a new page, with an extra argument
    LV_PMAN_STACK_MSG_TAG_SWAP,                  // Swap with a new page
    LV_PMAN_STACK_MSG_TAG_SWAP_EXTRA,            // Swap with a new page, with an extra argument
} lv_pman_stack_msg_tag_t;


/**
 * @brief View message
 *
 */
typedef struct {
    lv_pman_stack_msg_tag_t tag;

    union {
        struct {
            const void *page;      // Page to move to
            void       *extra;     // Extra argument
        } destination;
        int id;     // ID to reset to
    } as;
} lv_pman_stack_msg_t;


/**
 * @brief Message returned by a page event handler
 *
 */
typedef struct {
    void               *user_msg;
    lv_pman_stack_msg_t vmsg;
} lv_pman_msg_t;


/**
 * @brief Handle to use to register object subscriptions
 *
 */
typedef void *lv_pman_handle_t;


/**
 * @brief Event tag
 *
 */
typedef enum {
    LV_PMAN_EVENT_TAG_LVGL = 0,
    LV_PMAN_EVENT_TAG_OPEN,
    LV_PMAN_EVENT_TAG_TIMER,
    LV_PMAN_EVENT_TAG_USER,
} lv_pman_event_tag_t;


/**
 * @brief Page event
 *
 */
typedef struct {
    lv_pman_event_tag_t tag;
    union {
        lv_event_t      *lvgl;
        lv_pman_timer_t *timer;
        void            *user;
    } as;
} lv_pman_event_t;


typedef struct {
    int   id;
    void *state;
    void *extra;

    // Called when the page is first created; it initializes and returns the state structures used by the page
    void *(*create)(lv_pman_handle_t handle, void *extra);
    // Called when the page definitively exits the scenes; should free all used memory
    void (*destroy)(void *state, void *extra);

    // Called when the page enters view
    void (*open)(lv_pman_handle_t handle, void *state);
    // Called when the page exits view
    void (*close)(void *state);

    // Called to process an event
    lv_pman_msg_t (*process_event)(lv_pman_handle_t handle, void *state, lv_pman_event_t event);
} lv_pman_page_t;


#endif