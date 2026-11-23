#include "ovl_ui.h"
#include "ovl_config.h"
#include "ovl_draw.h"
#include "ovl_features.h"
#include "ovl_util.h"

/* Layout is expressed in "font pixels"; everything is multiplied by the UI
 * scale (1..3) when drawn. */
#define PANEL_W   232
#define TITLE_H   20
#define TAB_H     16
#define ROW_H     14
#define MAX_ROWS  8
#define FOOT_H    14
#define PAD_X     8
#define PANEL_H   (TITLE_H + TAB_H + 6 + MAX_ROWS * ROW_H + 4 + FOOT_H)

#define REPEAT_DELAY 18
#define REPEAT_RATE  3

typedef enum { IT_TOGGLE, IT_SLIDER, IT_CHOICE, IT_ACTION } item_kind_t;
typedef enum { ACT_SAVE, ACT_RESET, ACT_RELOAD } action_t;

typedef struct {
    const char *label;
    item_kind_t kind;
    int32_t *value;
    int32_t min, max;
    const char *const *choices;
    action_t action;
} ui_item_t;

typedef struct {
    const char *name;
    const ui_item_t *items;
    size_t count;
} ui_tab_t;

static const char *const CH_CORNER[] = {"Top L", "Top R", "Bot L", "Bot R"};
static const char *const CH_STYLE[]  = {"Cross", "Plus", "Dot", "Box"};
static const char *const CH_COLOR[]  = {"White", "Black", "Red", "Green", "Blue", "Yellow", "Cyan", "Pink"};
static const char *const CH_THEME[]  = {"Dark", "Light"};
static const char *const CH_COMBO[]  = {"Circle+Tri", "L3+R3", "L1+R1+Sq", "Pad+Opt"};
static const char *const CH_LAYOUT[] = {"Auto", "Linear", "Tiled"};

#define S(f) (&g_ovl_settings.f)

static const ui_item_t ITEMS_HUD[] = {
    {"FPS counter",  IT_TOGGLE, S(show_fps),       0, 1, NULL, 0},
    {"Frame time",   IT_TOGGLE, S(show_frametime), 0, 1, NULL, 0},
    {"Resolution",   IT_TOGGLE, S(show_res),       0, 1, NULL, 0},
    {"Uptime",       IT_TOGGLE, S(show_uptime),    0, 1, NULL, 0},
    {"Corner",       IT_CHOICE, S(hud_corner),     0, 3, CH_CORNER, 0},
    {"HUD scale",    IT_SLIDER, S(hud_scale),      1, 3, NULL, 0},
};

static const ui_item_t ITEMS_SIGHT[] = {
    {"Crosshair",    IT_TOGGLE, S(xh_on),      0, 1,  NULL, 0},
    {"Style",        IT_CHOICE, S(xh_style),   0, 3,  CH_STYLE, 0},
    {"Size",         IT_SLIDER, S(xh_size),    1, 64, NULL, 0},
    {"Thickness",    IT_SLIDER, S(xh_thick),   1, 8,  NULL, 0},
    {"Gap",          IT_SLIDER, S(xh_gap),     0, 32, NULL, 0},
    {"Color",        IT_CHOICE, S(xh_color),   0, 7,  CH_COLOR, 0},
    {"Outline",      IT_TOGGLE, S(xh_outline), 0, 1,  NULL, 0},
    {"Center dot",   IT_TOGGLE, S(xh_dot),     0, 1,  NULL, 0},
};

static const ui_item_t ITEMS_MENU[] = {
    {"Theme",        IT_CHOICE, S(theme),       0, 1,   CH_THEME, 0},
    {"Menu scale",   IT_SLIDER, S(ui_scale),    1, 3,   NULL, 0},
    {"Position X",   IT_SLIDER, S(menu_x),      0, 100, NULL, 0},
    {"Position Y",   IT_SLIDER, S(menu_y),      0, 100, NULL, 0},
    {"Open combo",   IT_CHOICE, S(combo),       0, 3,   CH_COMBO, 0},
    {"Block input",  IT_TOGGLE, S(block_input), 0, 1,   NULL, 0},
    {"Hints",        IT_TOGGLE, S(show_hints),  0, 1,   NULL, 0},
};

static const ui_item_t ITEMS_SYS[] = {
    {"Save settings",  IT_ACTION, NULL, 0, 0, NULL, ACT_SAVE},
    {"Reload file",    IT_ACTION, NULL, 0, 0, NULL, ACT_RELOAD},
    {"Reset defaults", IT_ACTION, NULL, 0, 0, NULL, ACT_RESET},
    {"Buffer layout",  IT_CHOICE, S(layout), 0, 2, CH_LAYOUT, 0},
};

enum { TAB_HUD, TAB_SIGHT, TAB_MENU, TAB_INFO, TAB_SYS, TAB_COUNT };

static const ui_tab_t g_tabs[TAB_COUNT] = {
    {"HUD",   ITEMS_HUD,   sizeof ITEMS_HUD / sizeof ITEMS_HUD[0]},
    {"SIGHT", ITEMS_SIGHT, sizeof ITEMS_SIGHT / sizeof ITEMS_SIGHT[0]},
    {"MENU",  ITEMS_MENU,  sizeof ITEMS_MENU / sizeof ITEMS_MENU[0]},
    {"INFO",  NULL,        0},
    {"SYS",   ITEMS_SYS,   sizeof ITEMS_SYS / sizeof ITEMS_SYS[0]},
};

static int g_tab = 0;
static size_t g_sel = 0;
static size_t g_scroll = 0;
static bool g_visible = false;
static uint32_t g_prev = 0;
static int32_t g_hold[4];
static int32_t g_toast = 0;
static const char *g_toast_msg = "";

void ui_init(void) {
    g_tab = 0;
    g_sel = 0;
    g_scroll = 0;
    g_visible = false;
    g_prev = 0;
    for (int i = 0; i < 4; i++) g_hold[i] = 0;
    g_toast = 0;
}

bool ui_is_visible(void) {
    return g_visible;
}

static int32_t clampi(int32_t v, int32_t lo, int32_t hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static void toast(const char *m) {
    g_toast_msg = m;
    g_toast = 120;
}

static void run_action(action_t a) {
    switch (a) {
    case ACT_SAVE:
        toast(ovl_settings_save() == OVL_OK ? "SAVED" : "SAVE FAILED");
        break;
    case ACT_RELOAD:
        toast(ovl_settings_load() == OVL_OK ? "RELOADED" : "NO FILE - DEFAULTS");
        break;
    case ACT_RESET:
        ovl_settings_defaults();
        toast("DEFAULTS RESTORED");
        break;
    }
}

static void change(const ui_item_t *it, int32_t dir) {
    if (it->value == NULL) {
        return;
    }
    if (it->kind == IT_TOGGLE) {
        *it->value = *it->value ? 0 : 1;
    } else if (it->kind == IT_SLIDER) {
        *it->value = clampi(*it->value + dir, it->min, it->max);
    } else if (it->kind == IT_CHOICE) {
        int32_t span = it->max - it->min + 1;
        *it->value = it->min + ((*it->value - it->min + dir + span) % span);
    }
}

static bool fire(int idx, bool held) {
    if (!held) {
        g_hold[idx] = 0;
        return false;
    }
    int32_t h = g_hold[idx]++;
    if (h == 0) {
        return true;
    }
    return h >= REPEAT_DELAY && ((h - REPEAT_DELAY) % REPEAT_RATE) == 0;
}

void ui_update(const ovl_pad_state_t *pad) {
    if (g_toast > 0) {
        g_toast--;
    }
    if (input_take_toggle()) {
        g_visible = !g_visible;
        g_prev = pad ? pad->buttons : 0;
        for (int i = 0; i < 4; i++) g_hold[i] = 1;
    }
    if (!g_visible || pad == NULL) {
        return;
    }
    uint32_t b = pad->buttons;
    uint32_t pressed = b & ~g_prev;
    g_prev = b;

    const ui_tab_t *tab = &g_tabs[g_tab];

    if (pressed & OVL_BTN_L1) {
        g_tab = (g_tab + TAB_COUNT - 1) % TAB_COUNT;
        g_sel = g_scroll = 0;
        return;
    }
    if (pressed & OVL_BTN_R1) {
        g_tab = (g_tab + 1) % TAB_COUNT;
        g_sel = g_scroll = 0;
        return;
    }
    if (pressed & OVL_BTN_CIRCLE) {
        g_visible = false;
        return;
    }

    bool up = fire(0, (b & OVL_BTN_UP) != 0);
    bool down = fire(1, (b & OVL_BTN_DOWN) != 0);
    bool left = fire(2, (b & OVL_BTN_LEFT) != 0);
    bool right = fire(3, (b & OVL_BTN_RIGHT) != 0);

    if (tab->count == 0) {
        return;
    }
    if (up) {
        g_sel = (g_sel + tab->count - 1) % tab->count;
    }
    if (down) {
        g_sel = (g_sel + 1) % tab->count;
    }
    if (g_sel < g_scroll) {
        g_scroll = g_sel;
    }
    if (g_sel >= g_scroll + MAX_ROWS) {
        g_scroll = g_sel - MAX_ROWS + 1;
    }

    const ui_item_t *it = &tab->items[g_sel];
    if (left) {
        change(it, -1);
    }
    if (right) {
        change(it, +1);
    }
    if (pressed & OVL_BTN_CROSS) {
        if (it->kind == IT_ACTION) {
            run_action(it->action);
        } else if (it->kind == IT_CHOICE) {
            change(it, +1);
        } else if (it->kind == IT_TOGGLE) {
            change(it, 0);
        }
    }
}

static void put_toggle(int32_t x, int32_t y, int32_t s, bool on, ovl_color_t fg, ovl_color_t bg) {
    int32_t w = 24 * s, h = 10 * s;
    if (on) {
        draw_pill(x, y, w, h, fg);
        draw_pill(x + w - 8 * s, y + 2 * s, 6 * s, 6 * s, bg);
    } else {
        draw_pill(x, y, w, h, fg);
        draw_pill(x + s, y + s, w - 2 * s, h - 2 * s, bg);
        draw_pill(x + 2 * s, y + 2 * s, 6 * s, 6 * s, fg);
    }
}

static void put_slider(int32_t right, int32_t y, int32_t s, const ui_item_t *it,
                       ovl_color_t fg, ovl_color_t bg) {
    char val[8];
    val[0] = '\0';
    ovl_cat_int(val, sizeof val, *it->value);
    int32_t tw = draw_text_width(val, s);
    draw_text(right - tw, y + 3 * s, val, fg, s);

    int32_t bw = 44 * s;
    int32_t bx = right - tw - 6 * s - bw;
    int32_t by = y + 4 * s;
    draw_rect(bx, by, bw, 5 * s, fg);
    draw_rect(bx + s, by + s, bw - 2 * s, 3 * s, bg);
    int32_t span = it->max - it->min;
    int32_t fill = span > 0 ? ((bw - 2 * s) * (*it->value - it->min)) / span : 0;
    draw_rect(bx + s, by + s, fill, 3 * s, fg);
}

static void put_info(int32_t x, int32_t y, int32_t s, ovl_color_t fg) {
    char line[48];
    for (size_t i = 0; i < MAX_ROWS; i++) {
        if (!features_info_line(i, line, sizeof line)) {
            break;
        }
        draw_text(x + PAD_X * s, y + (int32_t)i * ROW_H * s + 3 * s, line, fg, s);
    }
}

void ui_draw(void) {
    if (!g_visible) {
        return;
    }
    int32_t s = clampi(g_ovl_settings.ui_scale, 1, 3);
    int32_t sw = 1920, sh = 1080;
    features_screen_size(&sw, &sh);
    int32_t pw = PANEL_W * s, ph = PANEL_H * s;
    if (pw > sw - 4) pw = sw - 4;
    if (ph > sh - 4) ph = sh - 4;

    int32_t x = ((sw - PANEL_W * s) * g_ovl_settings.menu_x) / 100;
    int32_t y = ((sh - PANEL_H * s) * g_ovl_settings.menu_y) / 100;
    if (x < 2) x = 2;
    if (y < 2) y = 2;

    bool light = g_ovl_settings.theme == 1;
    ovl_color_t bg = light ? (ovl_color_t){255, 255, 255, 255} : (ovl_color_t){0, 0, 0, 255};
    ovl_color_t fg = light ? (ovl_color_t){0, 0, 0, 255} : (ovl_color_t){255, 255, 255, 255};
    ovl_color_t dim = light ? (ovl_color_t){110, 110, 110, 255} : (ovl_color_t){140, 140, 140, 255};

    /* frame + background */
    draw_rect(x - s, y - s, PANEL_W * s + 2 * s, PANEL_H * s + 2 * s, fg);
    draw_rect(x, y, PANEL_W * s, PANEL_H * s, bg);

    /* title */
    draw_text(x + PAD_X * s, y + 6 * s, "OVERLAY", fg, s);
    draw_text(x + (PANEL_W - PAD_X) * s - draw_text_width("v2", s), y + 6 * s, "v2", dim, s);
    draw_rect(x, y + (TITLE_H - 1) * s, PANEL_W * s, s, fg);

    /* tabs */
    int32_t tw = PANEL_W / TAB_COUNT;
    for (int i = 0; i < TAB_COUNT; i++) {
        int32_t tx = x + i * tw * s;
        int32_t textw = draw_text_width(g_tabs[i].name, s);
        int32_t cx = tx + (tw * s - textw) / 2;
        if (i == g_tab) {
            draw_rect(tx + s, y + TITLE_H * s, tw * s - 2 * s, (TAB_H - 1) * s, fg);
            draw_text(cx, y + (TITLE_H + 4) * s, g_tabs[i].name, bg, s);
        } else {
            draw_text(cx, y + (TITLE_H + 4) * s, g_tabs[i].name, dim, s);
        }
    }
    draw_rect(x, y + (TITLE_H + TAB_H - 1) * s, PANEL_W * s, s, dim);

    int32_t body_y = y + (TITLE_H + TAB_H + 6) * s;
    int32_t right = x + (PANEL_W - PAD_X) * s;
    const ui_tab_t *tab = &g_tabs[g_tab];

    if (g_tab == TAB_INFO) {
        put_info(x, body_y, s, fg);
    } else {
        for (size_t r = 0; r < MAX_ROWS && g_scroll + r < tab->count; r++) {
            size_t idx = g_scroll + r;
            const ui_item_t *it = &tab->items[idx];
            int32_t ry = body_y + (int32_t)r * ROW_H * s;
            bool sel = idx == g_sel;
            ovl_color_t rfg = sel ? bg : fg;
            ovl_color_t rbg = sel ? fg : bg;

            if (sel) {
                draw_rect(x + 4 * s, ry, (PANEL_W - 8) * s, (ROW_H - 1) * s, fg);
            }
            draw_text(x + PAD_X * s, ry + 3 * s, it->label, rfg, s);

            switch (it->kind) {
            case IT_TOGGLE:
                put_toggle(right - 24 * s, ry + 2 * s, s, *it->value != 0, rfg, rbg);
                break;
            case IT_SLIDER:
                put_slider(right, ry, s, it, rfg, rbg);
                break;
            case IT_CHOICE: {
                char v[24];
                v[0] = '\0';
                ovl_cat(v, sizeof v, "< ");
                ovl_cat(v, sizeof v, it->choices[*it->value - it->min]);
                ovl_cat(v, sizeof v, " >");
                draw_text(right - draw_text_width(v, s), ry + 3 * s, v, rfg, s);
                break;
            }
            case IT_ACTION:
                draw_text(right - draw_text_width("[X]", s), ry + 3 * s, "[X]", rfg, s);
                break;
            }
        }
        if (tab->count > MAX_ROWS) {
            draw_text(right - 8 * s, y + (TITLE_H + TAB_H + 6) * s - 8 * s, "...", dim, s);
        }
    }

    /* footer */
    int32_t fy = y + (PANEL_H - FOOT_H) * s;
    draw_rect(x, fy, PANEL_W * s, s, dim);
    if (g_toast > 0) {
        draw_text(x + PAD_X * s, fy + 4 * s, g_toast_msg, fg, s);
    } else if (g_ovl_settings.show_hints) {
        draw_text(x + PAD_X * s, fy + 4 * s, "L1/R1 TAB  X OK  O HIDE", dim, s);
    }
}
