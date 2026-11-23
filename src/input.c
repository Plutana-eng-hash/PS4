#include "ovl_input.h"
#include "ovl_config.h"

static ovl_pad_state_t g_state;
static bool g_combo_latched = false;
static volatile bool g_toggle_pending = false;

static uint32_t combo_mask(void) {
    switch (g_ovl_settings.combo) {
    case 1:  return OVL_BTN_L3 | OVL_BTN_R3;
    case 2:  return OVL_BTN_L1 | OVL_BTN_R1 | OVL_BTN_SQUARE;
    case 3:  return OVL_BTN_TOUCHPAD | OVL_BTN_OPTIONS;
    default: return OVL_BTN_CIRCLE | OVL_BTN_TRIANGLE;
    }
}

void input_feed(const ovl_pad_state_t *s) {
    if (s == NULL) {
        return;
    }
    g_state = *s;
    uint32_t mask = combo_mask();
    bool combo = (s->buttons & mask) == mask;
    if (combo && !g_combo_latched) {
        g_toggle_pending = true;
    }
    g_combo_latched = combo;
}

const ovl_pad_state_t *input_state(void) {
    return &g_state;
}

bool input_take_toggle(void) {
    bool t = g_toggle_pending;
    g_toggle_pending = false;
    return t;
}

bool input_combo_active(void) {
    uint32_t mask = combo_mask();
    /* Touchpad click is unused in most games, so holding it alone is enough. */
    if (g_ovl_settings.combo == 3) {
        return (g_state.buttons & OVL_BTN_TOUCHPAD) != 0;
    }
    return (g_state.buttons & mask) == mask;
}
