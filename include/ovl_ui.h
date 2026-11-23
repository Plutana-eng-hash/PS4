#ifndef OVL_UI_H
#define OVL_UI_H

#include <stdbool.h>
#include "ovl_input.h"

void ui_init(void);
void ui_update(const ovl_pad_state_t *pad);
void ui_draw(void);
bool ui_is_visible(void);

#endif
