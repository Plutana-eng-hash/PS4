#include <string.h>
#include "ovl_input.h"

#define AXIS_DEADZONE 16

static ovl_pad_state_t g_state;
static bool g_combo_latched = false;
static bool g_toggle_pending = false;

static bool input_default_reader(ovl_pad_state_t *out) {
    (void)out;
    return false;
}

static ovl_pad_read_fn g_reader = input_default_reader;

void input_set_reader(ovl_pad_read_fn fn) {
    g_reader = fn;
}

void input_update(void) {
    ovl_pad_state_t s;
    memset(&s, 0, sizeof s);

    if (g_reader == NULL || !g_reader(&s)) {

        memset(&g_state, 0, sizeof g_state);
        g_combo_latched = false;
        return;
    }
    g_state = s;

    bool combo = ((s.buttons & OVL_BTN_CIRCLE) != 0) && ((s.buttons & OVL_BTN_TRIANGLE) != 0);
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

bool input_held(uint32_t mask) {
    return (g_state.buttons & mask) == mask;
}

int32_t input_axis(uint8_t raw) {
    int32_t centered = (int32_t)raw - 128;
    if (centered > -AXIS_DEADZONE && centered < AXIS_DEADZONE) {
        return 0;
    }
    if (centered > 127) {
        centered = 127;
    }
    if (centered < -127) {
        centered = -127;
    }
    return centered;
}
