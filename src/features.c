#include "ovl_features.h"
#include "ovl_config.h"
#include "ovl_draw.h"
#include "ovl_util.h"

static uint64_t g_start_us = 0;
static uint64_t g_last_us = 0;
static uint64_t g_win_us = 0;
static uint64_t g_ft_us = 0;
static uint32_t g_frames = 0;
static uint32_t g_fps = 0;
static uint32_t g_total = 0;

static int32_t g_w = 1920, g_h = 1080;
static uint32_t g_fmt = 0;
static int32_t g_tiled = 0;
static int32_t g_buffers = 0;
static uint32_t g_hooks = 0;

static const ovl_color_t WHITE = {255, 255, 255, 255};
static const ovl_color_t BLACK = {0, 0, 0, 255};

void features_init(void) {
    g_start_us = g_last_us = g_win_us = g_ft_us = 0;
    g_frames = g_fps = g_total = 0;
}

void features_set_screen(int32_t w, int32_t h, uint32_t fmt, int32_t tiled, int32_t buffers) {
    if (w > 0 && h > 0) {
        g_w = w;
        g_h = h;
    }
    g_fmt = fmt;
    g_tiled = tiled;
    g_buffers = buffers;
}

void features_screen_size(int32_t *w, int32_t *h) {
    if (w != NULL) *w = g_w;
    if (h != NULL) *h = g_h;
}

void features_set_hook_flags(uint32_t flags) {
    g_hooks = flags;
}

void features_frame_tick(void) {
    uint64_t now = ovl_time_us();
    if (g_start_us == 0) {
        g_start_us = now;
        g_win_us = now;
    }
    if (g_last_us != 0 && now > g_last_us) {
        uint64_t dt = now - g_last_us;
        g_ft_us = g_ft_us ? (g_ft_us * 7 + dt) / 8 : dt;
    }
    g_last_us = now;
    g_frames++;
    g_total++;
    uint64_t el = now - g_win_us;
    if (el >= 1000000) {
        g_fps = (uint32_t)(((uint64_t)g_frames * 1000000u + el / 2) / el);
        g_frames = 0;
        g_win_us = now;
    }
}

uint32_t features_fps(void) {
    return g_fps;
}

static void cat_frametime(char *buf, size_t n) {
    uint32_t tenths = (uint32_t)((g_ft_us + 50) / 100);
    ovl_cat_int(buf, n, tenths / 10);
    ovl_cat(buf, n, ".");
    ovl_cat_int(buf, n, tenths % 10);
}

static void cat_uptime(char *buf, size_t n) {
    uint64_t secs = g_start_us ? (g_last_us - g_start_us) / 1000000u : 0;
    ovl_cat_2d(buf, n, (int32_t)(secs / 3600));
    ovl_cat(buf, n, ":");
    ovl_cat_2d(buf, n, (int32_t)((secs / 60) % 60));
    ovl_cat(buf, n, ":");
    ovl_cat_2d(buf, n, (int32_t)(secs % 60));
}

static const ovl_color_t PALETTE[8] = {
    {255, 255, 255, 255}, {0, 0, 0, 255},     {255, 50, 50, 255}, {50, 255, 80, 255},
    {60, 120, 255, 255},  {255, 230, 40, 255}, {40, 230, 255, 255}, {255, 80, 200, 255},
};

static void draw_hud(void) {
    char lines[4][32];
    int n = 0;
    if (g_ovl_settings.show_fps) {
        lines[n][0] = '\0';
        ovl_cat(lines[n], 32, "FPS ");
        if (g_fps == 0) ovl_cat(lines[n], 32, "--"); else ovl_cat_int(lines[n], 32, g_fps);
        n++;
    }
    if (g_ovl_settings.show_frametime) {
        lines[n][0] = '\0';
        ovl_cat(lines[n], 32, "FRAME ");
        cat_frametime(lines[n], 32);
        ovl_cat(lines[n], 32, " ms");
        n++;
    }
    if (g_ovl_settings.show_res) {
        lines[n][0] = '\0';
        ovl_cat(lines[n], 32, "RES ");
        ovl_cat_int(lines[n], 32, g_w);
        ovl_cat(lines[n], 32, "x");
        ovl_cat_int(lines[n], 32, g_h);
        n++;
    }
    if (g_ovl_settings.show_uptime) {
        lines[n][0] = '\0';
        ovl_cat(lines[n], 32, "UP ");
        cat_uptime(lines[n], 32);
        n++;
    }
    if (n == 0) {
        return;
    }
    int32_t s = g_ovl_settings.hud_scale;
    int32_t pad = 3 * s;
    int32_t line_h = 10 * s;
    int32_t tw = 0;
    for (int i = 0; i < n; i++) {
        int32_t w = draw_text_width(lines[i], s);
        if (w > tw) tw = w;
    }
    int32_t bw = tw + pad * 2;
    int32_t bh = n * line_h + pad * 2 - 2 * s;
    int32_t margin = 12 * s;
    int32_t x = (g_ovl_settings.hud_corner & 1) ? g_w - bw - margin : margin;
    int32_t y = (g_ovl_settings.hud_corner & 2) ? g_h - bh - margin : margin;

    draw_rect(x - 1, y - 1, bw + 2, bh + 2, WHITE);
    draw_rect(x, y, bw, bh, BLACK);
    for (int i = 0; i < n; i++) {
        draw_text(x + pad, y + pad + i * line_h - s, lines[i], WHITE, s);
    }
}

typedef struct { int32_t x, y, w, h; } rect_t;

static void draw_crosshair(void) {
    if (!g_ovl_settings.xh_on) {
        return;
    }
    int32_t cx = g_w / 2;
    int32_t cy = g_h / 2;
    int32_t s = g_ovl_settings.xh_size;
    int32_t t = g_ovl_settings.xh_thick;
    int32_t gap = g_ovl_settings.xh_gap;
    int32_t ht = t / 2;
    rect_t r[10];
    int n = 0;

    switch (g_ovl_settings.xh_style) {
    case 0:
        r[n++] = (rect_t){cx - s, cy - ht, 2 * s + t, t};
        r[n++] = (rect_t){cx - ht, cy - s, t, 2 * s + t};
        break;
    case 1:
        if (s > gap) {
            r[n++] = (rect_t){cx - s, cy - ht, s - gap, t};
            r[n++] = (rect_t){cx + gap + t, cy - ht, s - gap, t};
            r[n++] = (rect_t){cx - ht, cy - s, t, s - gap};
            r[n++] = (rect_t){cx - ht, cy + gap + t, t, s - gap};
        }
        break;
    case 2: {
        int32_t d = t * 2 > 2 ? t * 2 : 2;
        r[n++] = (rect_t){cx - d / 2, cy - d / 2, d, d};
        break;
    }
    default:
        r[n++] = (rect_t){cx - s, cy - s, 2 * s + t, t};
        r[n++] = (rect_t){cx - s, cy + s, 2 * s + t, t};
        r[n++] = (rect_t){cx - s, cy - s + t, t, 2 * s - t};
        r[n++] = (rect_t){cx + s, cy - s + t, t, 2 * s - t};
        break;
    }
    if (g_ovl_settings.xh_dot && g_ovl_settings.xh_style != 2) {
        int32_t d = t + 1;
        r[n++] = (rect_t){cx - d / 2, cy - d / 2, d, d};
    }

    ovl_color_t col = PALETTE[g_ovl_settings.xh_color & 7];
    ovl_color_t out = (g_ovl_settings.xh_color == 1) ? WHITE : BLACK;
    if (g_ovl_settings.xh_outline) {
        for (int i = 0; i < n; i++) {
            draw_rect(r[i].x - 1, r[i].y - 1, r[i].w + 2, r[i].h + 2, out);
        }
    }
    for (int i = 0; i < n; i++) {
        draw_rect(r[i].x, r[i].y, r[i].w, r[i].h, col);
    }
}

static int32_t g_banner = 0;

void features_show_banner(void) {
    g_banner = 360; /* roughly six seconds at 60 fps */
}

static const char *combo_hint(void) {
    switch (g_ovl_settings.combo) {
    case 1:  return "L3+R3";
    case 2:  return "L1+R1+SQUARE";
    case 3:  return "TOUCHPAD+OPTIONS";
    default: return "CIRCLE+TRIANGLE";
    }
}

static void draw_banner(void) {
    if (g_banner <= 0 || !g_ovl_settings.banner) {
        return;
    }
    g_banner--;
    char line[48];
    line[0] = '\0';
    ovl_cat(line, sizeof line, combo_hint());
    ovl_cat(line, sizeof line, " = MENU");

    const char *title = "OVERLAY ACTIVE";
    int32_t s = 2;
    int32_t w1 = draw_text_width(title, s);
    int32_t w2 = draw_text_width(line, s);
    int32_t tw = w1 > w2 ? w1 : w2;
    int32_t pad = 8 * s;
    int32_t bw = tw + pad * 2;
    int32_t bh = 2 * 10 * s + pad * 2 - 2 * s;
    int32_t x = (g_w - bw) / 2;
    int32_t y = g_h / 12;
    draw_rect(x - 1, y - 1, bw + 2, bh + 2, WHITE);
    draw_rect(x, y, bw, bh, BLACK);
    draw_text(x + (bw - w1) / 2, y + pad - s, title, WHITE, s);
    draw_text(x + (bw - w2) / 2, y + pad - s + 10 * s, line, WHITE, s);
}

void features_draw_overlay(void) {
    draw_crosshair();
    draw_hud();
    draw_banner();
}

static const char *fmt_name(void) {
    switch (g_fmt) {
    case OVL_FMT_A8R8G8B8_SRGB: return "A8R8G8B8";
    case OVL_FMT_A8B8G8R8_SRGB: return "A8B8G8R8";
    case OVL_FMT_A2R10G10B10:
    case OVL_FMT_A2R10G10B10_SRGB: return "A2R10G10B10";
    default: return "unsupported";
    }
}

bool features_info_line(size_t i, char *buf, size_t len) {
    buf[0] = '\0';
    switch (i) {
    case 0:
        ovl_cat(buf, len, "FPS: ");
        ovl_cat_int(buf, len, g_fps);
        return true;
    case 1:
        ovl_cat(buf, len, "Frame time: ");
        cat_frametime(buf, len);
        ovl_cat(buf, len, " ms");
        return true;
    case 2:
        ovl_cat(buf, len, "Resolution: ");
        ovl_cat_int(buf, len, g_w);
        ovl_cat(buf, len, "x");
        ovl_cat_int(buf, len, g_h);
        return true;
    case 3:
        ovl_cat(buf, len, "Format: ");
        ovl_cat(buf, len, fmt_name());
        return true;
    case 4:
        ovl_cat(buf, len, "Layout: ");
        ovl_cat(buf, len, g_tiled ? "tiled" : "linear");
        return true;
    case 5:
        ovl_cat(buf, len, "Buffers: ");
        ovl_cat_int(buf, len, g_buffers);
        return true;
    case 6:
        ovl_cat(buf, len, "Hooks flip/reg/pad: ");
        ovl_cat(buf, len, (g_hooks & 1) ? "1" : "0");
        ovl_cat(buf, len, (g_hooks & 2) ? "1" : "0");
        ovl_cat(buf, len, (g_hooks & 4) ? "1" : "0");
        return true;
    case 7:
        ovl_cat(buf, len, "Frames: ");
        ovl_cat_int(buf, len, g_total);
        return true;
    default:
        return false;
    }
}
