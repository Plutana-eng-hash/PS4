#ifndef OVL_INPUT_H
#define OVL_INPUT_H

#include <stdint.h>
#include <stdbool.h>

#define OVL_BTN_L3        0x00000002u
#define OVL_BTN_R3        0x00000004u
#define OVL_BTN_OPTIONS   0x00000008u
#define OVL_BTN_UP        0x00000010u
#define OVL_BTN_RIGHT     0x00000020u
#define OVL_BTN_DOWN      0x00000040u
#define OVL_BTN_LEFT      0x00000080u
#define OVL_BTN_L1        0x00000400u
#define OVL_BTN_R1        0x00000800u
#define OVL_BTN_TRIANGLE  0x00001000u
#define OVL_BTN_CIRCLE    0x00002000u
#define OVL_BTN_CROSS     0x00004000u
#define OVL_BTN_SQUARE    0x00008000u
#define OVL_BTN_TOUCHPAD  0x00100000u

typedef struct {
    uint32_t buttons;
    uint8_t lx, ly, rx, ry;
    uint8_t l2, r2;
} ovl_pad_state_t;

/* Feed a fresh pad sample (from the pad hook or the fallback poll). */
void input_feed(const ovl_pad_state_t *s);
const ovl_pad_state_t *input_state(void);
/* True once per press of the configured open/close combo. */
bool input_take_toggle(void);
/* True while the open/close combo is being held. The pad hook hides input from
 * the game during this so the combo never leaks into gameplay. */
bool input_combo_active(void);

#endif
