#include <stdio.h>
#include <string.h>
#include "ovl_features.h"
#include "ovl_config.h"
#include "ovl_draw.h"

#define FEAT_MAX_PROVIDERS 8
#define FEAT_WINDOW_MS 1000

static uint32_t g_frames = 0;
static uint32_t g_fps = 0;
static uint64_t g_window_start = 0;
static ovl_clock_fn g_clock = NULL;
static int32_t g_screen_w = 1920;
static int32_t g_screen_h = 1080;

static const ovl_info_provider_t *g_providers[FEAT_MAX_PROVIDERS];
static size_t g_provider_count = 0;

static bool info_title_read(char *buf, size_t len) {
    const char *t = ovl_config_get_string("game.title_id", "");
    if (t[0] == '\0') {
        return false;
    }
    snprintf(buf, len, "%s", t);
    return true;
}

static const ovl_info_provider_t g_title_provider = {"Title ID", info_title_read};

static bool info_entity_read(char *buf, size_t len) {
    (void)buf;
    (void)len;
    return false;
}

static const ovl_info_provider_t g_entity_provider = {"Entity data", info_entity_read};

void features_init(void) {
    g_frames = 0;
    g_fps = 0;
    g_window_start = 0;
    g_provider_count = 0;
    features_register_info(&g_title_provider);
    features_register_info(&g_entity_provider);
}

void features_set_clock(ovl_clock_fn fn) {
    g_clock = fn;
}

void features_set_screen_size(int32_t w, int32_t h) {
    if (w > 0 && h > 0) {
        g_screen_w = w;
        g_screen_h = h;
    }
}

void features_screen_size(int32_t *w, int32_t *h) {
    if (w != NULL) {
        *w = g_screen_w;
    }
    if (h != NULL) {
        *h = g_screen_h;
    }
}

void features_frame_tick(void) {
    g_frames++;
    if (g_clock == NULL) {
        return;
    }
    uint64_t now = g_clock();
    if (g_window_start == 0) {
        g_window_start = now;
        return;
    }
    uint64_t elapsed = now - g_window_start;
    if (elapsed >= FEAT_WINDOW_MS && elapsed > 0) {
        g_fps = (uint32_t)((uint64_t)g_frames * 1000u / elapsed);
        g_frames = 0;
        g_window_start = now;
    }
}

uint32_t features_fps(void) {
    return g_fps;
}

void features_draw_overlay(void) {
    ovl_color_t white = OVL_COLOR(255, 255, 255, 255);
    ovl_color_t cross = OVL_COLOR(g_ovl_settings.crosshair_r,
                                  g_ovl_settings.crosshair_g,
                                  g_ovl_settings.crosshair_b, 255);

    if (g_ovl_settings.show_fps) {
        char buf[24];
        if (g_fps == 0) {
            snprintf(buf, sizeof buf, "FPS: --");
        } else {
            snprintf(buf, sizeof buf, "FPS: %u", (unsigned)g_fps);
        }
        draw_text(10, 10, buf, white);
    }

    if (g_ovl_settings.show_crosshair && g_ovl_settings.crosshair_size > 0) {
        int32_t cx = g_screen_w / 2;
        int32_t cy = g_screen_h / 2;
        int32_t s = g_ovl_settings.crosshair_size;
        draw_line(cx - s, cy, 2 * s, 0, cross);
        draw_line(cx, cy - s, 0, 2 * s, cross);
    }
}

ovl_status_t features_register_info(const ovl_info_provider_t *provider) {
    if (provider == NULL || provider->read == NULL) {
        return OVL_ERR_NULL;
    }
    if (g_provider_count >= FEAT_MAX_PROVIDERS) {
        return OVL_ERR_FULL;
    }
    g_providers[g_provider_count++] = provider;
    return OVL_OK;
}

size_t features_info_count(void) {
    return g_provider_count;
}

const char *features_info_line(size_t index, char *buf, size_t len) {
    if (buf == NULL || len == 0) {
        return "";
    }
    if (index >= g_provider_count) {
        buf[0] = '\0';
        return buf;
    }
    const ovl_info_provider_t *p = g_providers[index];
    char value[48];
    if (p->read(value, sizeof value)) {
        snprintf(buf, len, "%s: %s", p->name, value);
    } else {
        snprintf(buf, len, "%s: unavailable", p->name);
    }
    return buf;
}
