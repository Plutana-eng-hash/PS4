#ifndef OVL_INPUT_H
#define OVL_INPUT_H

#include <stdint.h>
#include <stdbool.h>
#include "ovl_types.h"

#define OVL_BTN_L1        0x00000400u
#define OVL_BTN_R1        0x00000800u
#define OVL_BTN_TRIANGLE  0x00001000u
#define OVL_BTN_CIRCLE    0x00002000u

typedef struct {
    uint32_t buttons;
    uint8_t lx;
    uint8_t ly;
    uint8_t rx;
    uint8_t ry;
} ovl_pad_state_t;

typedef bool (*ovl_pad_read_fn)(ovl_pad_state_t *out);

void input_set_reader(ovl_pad_read_fn fn);

void input_update(void);

const ovl_pad_state_t *input_state(void);

bool input_take_toggle(void);

bool input_held(uint32_t mask);

int32_t input_axis(uint8_t raw);

#endif
