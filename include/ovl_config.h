#ifndef OVL_CONFIG_H
#define OVL_CONFIG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "ovl_types.h"

typedef struct {
    int32_t win_x;
    int32_t win_y;
    int32_t show_fps;
    int32_t show_crosshair;
    int32_t crosshair_size;
    int32_t crosshair_r;
    int32_t crosshair_g;
    int32_t crosshair_b;
    int32_t menu_r;
    int32_t menu_g;
    int32_t menu_b;
    int32_t menu_alpha;
    int32_t stick_move_speed;
    int32_t stick_adjust_speed;
} ovl_settings_t;

extern ovl_settings_t g_ovl_settings;

ovl_status_t ovl_config_load(const char *path);

int32_t ovl_config_get_int(const char *key, int32_t def);
bool ovl_config_get_bool(const char *key, bool def);
const char *ovl_config_get_string(const char *key, const char *def);

ovl_status_t ovl_settings_load(void);

#endif
