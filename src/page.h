#ifndef PMAN_PAGE_H_INCLUDED
#define PMAN_PAGE_H_INCLUDED


#include "page_manager_conf.h"
#include "page_manager_timer.h"
#include "lvgl.h"


#define PMAN_MSG_NULL        ((pman_msg_t){.user_msg = NULL, .stack_msg = {.tag = PMAN_STACK_MSG_TAG_NOTHING}})
#define PMAN_USER_EVENT(ptr) ((pman_event_t){.tag = PMAN_EVENT_TAG_USER, .as = {.user = ptr}})


/**
 * @brief Tags for view messages (i.e. commands that act on the page stack)
 *
 */
typedef enum {
    PMAN_STACK_MSG_TAG_NOTHING = 0,           // Do nothing
    PMAN_STACK_MSG_TAG_BACK,                  // Go back to the previous page
    PMAN_STACK_MSG_TAG_REBASE,                // Rebase to a new page
    PMAN_STACK_MSG_TAG_RESET_TO,              // Reset to a previous page
    PMAN_STACK_MSG_TAG_CHANGE_PAGE,           // Change to a new page
    PMAN_STACK_MSG_TAG_CHANGE_PAGE_EXTRA,     // Change to a new page, with an extra argument
    PMAN_STACK_MSG_TAG_SWAP,                  // Swap with a new page
    PMAN_STACK_MSG_TAG_SWAP_EXTRA,            // Swap with a new page, with an extra argument
} pman_stack_msg_tag_t;


/**
 * @brief View message
 *
 */
typedef struct {
    pman_stack_msg_tag_t tag;

    union {
        struct {
            const void *page;      // Page to move to
            void       *extra;     // Extra argument
        } destination;
        int id;     // ID to reset to
    } as;
} pman_stack_msg_t;


/**
 * @brief Message returned by a page event handler
 *
 */
typedef struct {
    void               *user_msg;
    pman_stack_msg_t stack_msg;
} pman_msg_t;


/**
 * @brief Handle to use to register object subscriptions
 *
 */
typedef void *pman_handle_t;


/**
 * @brief Event tag
 *
 */
typedef enum {
    PMAN_EVENT_TAG_LVGL = 0,
    PMAN_EVENT_TAG_OPEN,
    PMAN_EVENT_TAG_TIMER,
    PMAN_EVENT_TAG_USER,
} pman_event_tag_t;


/**
 * @brief Page event
 *
 */
typedef struct {
    pman_event_tag_t tag;
    union {
        lv_event_t      *lvgl;
        pman_timer_t *timer;
        void            *user;
    } as;
} pman_event_t;


typedef struct {
    int   id;
    void *state;
    void *extra;

    // Called when the page is first created; it initializes and returns the state structures used by the page
    void *(*create)(pman_handle_t handle, void *extra);
    // Called when the page definitively exits the scenes; should free all used memory
    void (*destroy)(void *state, void *extra);

    // Called when the page enters view
    void (*open)(pman_handle_t handle, void *state);
    // Called when the page exits view
    void (*close)(void *state);

    // Called to process an event
    pman_msg_t (*process_event)(pman_handle_t handle, void *state, pman_event_t event);
} pman_page_t;


#endif
