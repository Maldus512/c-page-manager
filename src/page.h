#ifndef LV_PMAN_PAGE_H_INCLUDED
#define LV_PMAN_PAGE_H_INCLUDED


#include "lv_page_manager_conf.h"
#include "lvgl.h"


typedef void *lv_pman_page_handle_t;


typedef enum {
    LV_PMAN_VIEW_MSG_TAG_NOTHING = 0,
    LV_PMAN_VIEW_MSG_TAG_BACK,
    LV_PMAN_VIEW_MSG_TAG_REBASE,
    LV_PMAN_VIEW_MSG_TAG_RESET_TO,
    LV_PMAN_VIEW_MSG_TAG_CHANGE_PAGE,
    LV_PMAN_VIEW_MSG_TAG_CHANGE_PAGE_EXTRA,
    LV_PMAN_VIEW_MSG_TAG_SWAP,
    LV_PMAN_VIEW_MSG_TAG_SWAP_EXTRA,
} lv_pman_view_msg_tag_t;


typedef struct {
    lv_pman_view_msg_tag_t tag;

    union {
        struct {
            const void *page;
            void       *extra;
        };
        int id;
    };
} lv_pman_view_msg_t;


typedef struct {
    lv_pman_controller_msg_t cmsg;
    lv_pman_view_msg_t       vmsg;
} lv_pman_msg_t;


typedef enum {
    LV_PMAN_EVENT_TAG_LVGL = 0,
    LV_PMAN_EVENT_TAG_OPEN,
    LV_PMAN_EVENT_TAG_USER,
} lv_pman_event_tag_t;


typedef struct {
    int                   id;
    int                   number;
    lv_pman_page_handle_t handle;
} lv_pman_obj_data_t;


typedef struct {
    lv_pman_event_tag_t tag;
    union {
        struct {
            lv_event_code_t event;
            int             id;
            int             number;
            lv_obj_t       *target;
        } lvgl;
        lv_pman_user_event_t user_event;
    };
} lv_pman_event_t;


typedef struct {
    int   id;
    void *data;
    void *extra;

    // Called when the page is first created; it initializes and returns the data structures used by the page
    void *(*create)(lv_pman_page_handle_t page, void *args, void *extra);
    // Called when the page definitively exits the scenes; should free all used memory
    void (*destroy)(void *data, void *extra);

    // Called when the page enters view
    void (*open)(lv_pman_page_handle_t page, void *args, void *data);
    // Called when the page exits view
    void (*close)(void *data);

    // Called to process an event
    lv_pman_msg_t (*process_event)(lv_pman_page_handle_t page, void *args, void *data, lv_pman_event_t event);
} lv_pman_page_t;


#endif