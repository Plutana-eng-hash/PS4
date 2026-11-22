#include <stdio.h>
#include <string.h>
#include "ovl_ui.h"
#include "ovl_config.h"
#include "ovl_draw.h"
#include "ovl_features.h"
#include "ovl_input.h"

#define UI_PANEL_W 380
#define UI_PANEL_H 280
#define UI_TITLE_H 28
#define UI_TAB_H 24
#define UI_ROW_H 22
#define UI_SCROLL_LIMIT 400
#define UI_ADJUST_LIMIT 2000
#define UI_BOOL_THRESH 64

typedef struct {
    const char *label;
    int32_t *value;
    int32_t min;
    int32_t max;
    bool is_bool;
} ui_item_t;

typedef struct {
    const char *name;
    ui_item_t *items;
    size_t count;
} ui_tab_t;

static ui_item_t g_items_visuals[] = {
    {"Show FPS",       &g_ovl_settings.show_fps,       0, 1,   true},
    {"Crosshair",      &g_ovl_settings.show_crosshair, 0, 1,   true},
    {"Crosshair size", &g_ovl_settings.crosshair_size, 1, 64,  false},
    {"Crosshair R",    &g_ovl_settings.crosshair_r,    0, 255, false},
    {"Crosshair G",    &g_ovl_settings.crosshair_g,    0, 255, false},
    {"Crosshair B",    &g_ovl_settings.crosshair_b,    0, 255, false},
};

static ui_item_t g_items_aim[] = {
    {"Reserved: no aim modules", NULL, 0, 0, false},
};

static ui_item_t g_items_misc[] = {
    {"Menu alpha",        &g_ovl_settings.menu_alpha,         0, 255, false},
    {"Stick move speed",  &g_ovl_settings.stick_move_speed,   1, 32,  false},
    {"Stick adjust speed",&g_ovl_settings.stick_adjust_speed, 1, 8,   false},
};

static ui_item_t g_items_settings[] = {
    {"Menu R", &g_ovl_settings.menu_r, 0, 255, false},
    {"Menu G", &g_ovl_settings.menu_g, 0, 255, false},
    {"Menu B", &g_ovl_settings.menu_b, 0, 255, false},
};

static ui_tab_t g_tabs[OVL_TAB_COUNT] = {
    {"Visuals",  g_items_visuals,  sizeof g_items_visuals  / sizeof g_items_visuals[0]},
    {"Aim",      g_items_aim,      sizeof g_items_aim      / sizeof g_items_aim[0]},
    {"Misc",     g_items_misc,     sizeof g_items_misc     / sizeof g_items_misc[0]},
    {"Info",     NULL,             0},
    {"Settings", g_items_settings, sizeof g_items_settings / sizeof g_items_settings[0]},
};

static int g_tab = 0;
static size_t g_sel = 0;
static int32_t g_acc_y = 0;
static int32_t g_acc_x = 0;
static bool g_bool_latch = false;
static bool g_latch_l1 = false;
static bool g_latch_r1 = false;
static bool g_visible = false;

static int32_t ui_clamp(int32_t v, int32_t lo, int32_t hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

void ui_init(void) {
    g_tab = 0;
    g_sel = 0;
    g_acc_x = 0;
    g_acc_y = 0;
    g_bool_latch = false;
    g_latch_l1 = false;
    g_latch_r1 = false;
    g_visible = false;
}

bool ui_is_visible(void) {
    return g_visible;
}

static void ui_move_window(const ovl_pad_state_t *pad) {
    int32_t sw = 0;
    int32_t sh = 0;
    features_screen_size(&sw, &sh);
    int32_t dx = (input_axis(pad->lx) * g_ovl_settings.stick_move_speed) / 127;
    int32_t dy = (input_axis(pad->ly) * g_ovl_settings.stick_move_speed) / 127;
    g_ovl_settings.win_x = ui_clamp(g_ovl_settings.win_x + dx, 0, sw - UI_PANEL_W);
    g_ovl_settings.win_y = ui_clamp(g_ovl_settings.win_y + dy, 0, sh - UI_PANEL_H);
}

static void ui_switch_tabs(const ovl_pad_state_t *pad) {
    bool l1 = (pad->buttons & OVL_BTN_L1) != 0;
    bool r1 = (pad->buttons & OVL_BTN_R1) != 0;
    if (l1 && !g_latch_l1) {
        g_tab = (g_tab + OVL_TAB_COUNT - 1) % OVL_TAB_COUNT;
        g_sel = 0;
    }
    if (r1 && !g_latch_r1) {
        g_tab = (g_tab + 1) % OVL_TAB_COUNT;
        g_sel = 0;
    }
    g_latch_l1 = l1;
    g_latch_r1 = r1;
}

static void ui_select_row(const ovl_pad_state_t *pad) {
    ui_tab_t *tab = &g_tabs[g_tab];
    if (tab->count == 0) {
        g_sel = 0;
        return;
    }
    g_acc_y += input_axis(pad->ry);
    if (g_acc_y >= UI_SCROLL_LIMIT) {
        g_acc_y = 0;
        g_sel = (g_sel + 1) % tab->count;
    } else if (g_acc_y <= -UI_SCROLL_LIMIT) {
        g_acc_y = 0;
        g_sel = (g_sel + tab->count - 1) % tab->count;
    }
}

static void ui_adjust_value(const ovl_pad_state_t *pad) {
    ui_tab_t *tab = &g_tabs[g_tab];
    if (tab->count == 0 || g_sel >= tab->count) {
        return;
    }
    ui_item_t *item = &tab->items[g_sel];
    if (item->value == NULL) {
        return;
    }
    int32_t rx = input_axis(pad->rx);

    if (item->is_bool) {
        if ((rx > UI_BOOL_THRESH || rx < -UI_BOOL_THRESH) && !g_bool_latch) {
            *item->value = (*item->value == 0) ? 1 : 0;
            g_bool_latch = true;
        } else if (rx < UI_BOOL_THRESH / 2 && rx > -UI_BOOL_THRESH / 2) {
            g_bool_latch = false;
        }
        return;
    }

    g_acc_x += rx * g_ovl_settings.stick_adjust_speed;
    if (g_acc_x >= UI_ADJUST_LIMIT) {
        g_acc_x = 0;
        *item->value = ui_clamp(*item->value + 1, item->min, item->max);
    } else if (g_acc_x <= -UI_ADJUST_LIMIT) {
        g_acc_x = 0;
        *item->value = ui_clamp(*item->value - 1, item->min, item->max);
    }
}

void ui_update(const ovl_pad_state_t *pad) {
    if (input_take_toggle()) {
        g_visible = !g_visible;
        g_acc_x = 0;
        g_acc_y = 0;
    }
    if (!g_visible || pad == NULL) {
        return;
    }
    ui_move_window(pad);
    ui_switch_tabs(pad);
    ui_select_row(pad);
    ui_adjust_value(pad);
}

void ui_draw(void) {
    if (!g_visible) {
        return;
    }
    int32_t x = g_ovl_settings.win_x;
    int32_t y = g_ovl_settings.win_y;
    ovl_color_t panel = OVL_COLOR(g_ovl_settings.menu_r, g_ovl_settings.menu_g,
                                  g_ovl_settings.menu_b, g_ovl_settings.menu_alpha);
    ovl_color_t title = OVL_COLOR(40, 90, 160, 240);
    ovl_color_t tab_on = OVL_COLOR(60, 110, 190, 255);
    ovl_color_t tab_off = OVL_COLOR(50, 50, 60, 255);
    ovl_color_t row_sel = OVL_COLOR(70, 70, 90, 255);
    ovl_color_t text = OVL_COLOR(255, 255, 255, 255);
    ovl_color_t dim = OVL_COLOR(170, 170, 180, 255);

    draw_rect_fill(x, y, UI_PANEL_W, UI_PANEL_H, panel);
    draw_rect_fill(x, y, UI_PANEL_W, UI_TITLE_H, title);
    draw_text(x + 8, y + 8, "PS4 Overlay Framework", text);

    int32_t tab_w = UI_PANEL_W / OVL_TAB_COUNT;
    for (int i = 0; i < OVL_TAB_COUNT; i++) {
        int32_t tx = x + i * tab_w;
        draw_rect_fill(tx, y + UI_TITLE_H, tab_w - 2, UI_TAB_H, (i == g_tab) ? tab_on : tab_off);
        draw_text(tx + 4, y + UI_TITLE_H + 6, g_tabs[i].name, text);
    }

    int32_t body_y = y + UI_TITLE_H + UI_TAB_H + 8;
    ui_tab_t *tab = &g_tabs[g_tab];

    if (g_tab == OVL_TAB_INFO) {

        size_t n = features_info_count();
        for (size_t i = 0; i < n && i < 8; i++) {
            char line[64];
            features_info_line(i, line, sizeof line);
            draw_text(x + 10, body_y + (int32_t)i * UI_ROW_H, line, text);
        }
    } else {
        for (size_t i = 0; i < tab->count; i++) {
            ui_item_t *item = &tab->items[i];
            int32_t ry = body_y + (int32_t)i * UI_ROW_H;
            if (i == g_sel) {
                draw_rect_fill(x + 4, ry - 2, UI_PANEL_W - 8, UI_ROW_H, row_sel);
            }
            draw_text(x + 10, ry + 2, item->label, text);

            if (item->value != NULL) {
                char val[16];
                if (item->is_bool) {
                    snprintf(val, sizeof val, "%s", (*item->value) ? "ON" : "OFF");
                } else {
                    snprintf(val, sizeof val, "%d", (int)*item->value);
                }
                draw_text(x + UI_PANEL_W - 70, ry + 2, val, dim);
            }
        }
    }

    draw_text(x + 10, y + UI_PANEL_H - 18, "L1/R1 tab  RS: select/adjust", dim);
}
