#ifndef OVL_UI_H
#define OVL_UI_H

#include <stdbool.h>
#include "ovl_input.h"

typedef enum {
    OVL_TAB_VISUALS = 0,
    OVL_TAB_AIM,
    OVL_TAB_MISC,
    OVL_TAB_INFO,
    OVL_TAB_SETTINGS,
    OVL_TAB_COUNT
} ovl_tab_t;

void ui_init(void);

void ui_update(const ovl_pad_state_t *pad);

void ui_draw(void);

bool ui_is_visible(void);

#endif
