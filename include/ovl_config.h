#ifndef OVL_CONFIG_H
#define OVL_CONFIG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "ovl_types.h"

typedef struct {
    /* HUD */
    int32_t show_fps;
    int32_t show_frametime;
    int32_t show_res;
    int32_t show_uptime;
    int32_t hud_corner;
    int32_t hud_scale;
    /* Crosshair */
    int32_t xh_on;
    int32_t xh_style;
    int32_t xh_size;
    int32_t xh_thick;
    int32_t xh_gap;
    int32_t xh_color;
    int32_t xh_outline;
    int32_t xh_dot;
    /* Menu */
    int32_t theme;
    int32_t ui_scale;
    int32_t menu_x;
    int32_t menu_y;
    int32_t combo;
    int32_t show_hints;
    int32_t block_input;
    int32_t layout;
} ovl_settings_t;

extern ovl_settings_t g_ovl_settings;

ovl_status_t ovl_config_load(const char *path);
int32_t ovl_config_get_int(const char *key, int32_t def);

void ovl_settings_defaults(void);
ovl_status_t ovl_settings_load(void);
ovl_status_t ovl_settings_save(void);

#endif
