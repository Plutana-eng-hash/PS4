#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "ovl_config.h"

#define CFG_MAX_ENTRIES 128
#define CFG_KEY_LEN 48
#define CFG_VAL_LEN 64

typedef struct {
    char key[CFG_KEY_LEN];
    char val[CFG_VAL_LEN];
} cfg_entry_t;

static cfg_entry_t g_entries[CFG_MAX_ENTRIES];
static size_t g_count = 0;

ovl_settings_t g_ovl_settings;

static void cfg_trim(char *s) {
    if (s == NULL) {
        return;
    }
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' ||
                       s[len - 1] == ' ' || s[len - 1] == '\t')) {
        s[--len] = '\0';
    }
    size_t start = 0;
    while (s[start] == ' ' || s[start] == '\t') {
        start++;
    }
    if (start > 0) {
        memmove(s, s + start, strlen(s + start) + 1);
    }
}

static const char *cfg_find(const char *key) {
    for (size_t i = 0; i < g_count; i++) {
        if (strcmp(g_entries[i].key, key) == 0) {
            return g_entries[i].val;
        }
    }
    return NULL;
}

static void cfg_store(const char *key, const char *val) {
    for (size_t i = 0; i < g_count; i++) {
        if (strcmp(g_entries[i].key, key) == 0) {
            strncpy(g_entries[i].val, val, CFG_VAL_LEN - 1);
            g_entries[i].val[CFG_VAL_LEN - 1] = '\0';
            return;
        }
    }
    if (g_count >= CFG_MAX_ENTRIES) {
        return;
    }
    strncpy(g_entries[g_count].key, key, CFG_KEY_LEN - 1);
    g_entries[g_count].key[CFG_KEY_LEN - 1] = '\0';
    strncpy(g_entries[g_count].val, val, CFG_VAL_LEN - 1);
    g_entries[g_count].val[CFG_VAL_LEN - 1] = '\0';
    g_count++;
}

ovl_status_t ovl_config_load(const char *path) {
    if (path == NULL) {
        return OVL_ERR_NULL;
    }
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return OVL_ERR_IO;
    }
    char line[160];
    while (fgets(line, sizeof line, f) != NULL) {
        cfg_trim(line);

        if (line[0] == '\0' || line[0] == '#' || line[0] == ';' || line[0] == '[') {
            continue;
        }
        char *eq = strchr(line, '=');
        if (eq == NULL) {
            continue;
        }
        *eq = '\0';
        char *key = line;
        char *val = eq + 1;
        cfg_trim(key);
        cfg_trim(val);
        if (key[0] != '\0') {
            cfg_store(key, val);
        }
    }
    fclose(f);
    return OVL_OK;
}

int32_t ovl_config_get_int(const char *key, int32_t def) {
    if (key == NULL) {
        return def;
    }
    const char *v = cfg_find(key);
    if (v == NULL || v[0] == '\0') {
        return def;
    }
    char *end = NULL;
    long r = strtol(v, &end, 0);
    if (end == v) {
        return def;
    }
    return (int32_t)r;
}

bool ovl_config_get_bool(const char *key, bool def) {
    if (key == NULL) {
        return def;
    }
    const char *v = cfg_find(key);
    if (v == NULL) {
        return def;
    }
    if (strcmp(v, "1") == 0 || strcmp(v, "true") == 0 || strcmp(v, "on") == 0 || strcmp(v, "yes") == 0) {
        return true;
    }
    if (strcmp(v, "0") == 0 || strcmp(v, "false") == 0 || strcmp(v, "off") == 0 || strcmp(v, "no") == 0) {
        return false;
    }
    return def;
}

const char *ovl_config_get_string(const char *key, const char *def) {
    if (key == NULL) {
        return def;
    }
    const char *v = cfg_find(key);
    return (v != NULL) ? v : def;
}

ovl_status_t ovl_settings_load(void) {
    g_ovl_settings.win_x = ovl_config_get_int("ui.win_x", 40);
    g_ovl_settings.win_y = ovl_config_get_int("ui.win_y", 60);
    g_ovl_settings.show_fps = ovl_config_get_int("visual.show_fps", 1);
    g_ovl_settings.show_crosshair = ovl_config_get_int("visual.show_crosshair", 1);
    g_ovl_settings.crosshair_size = ovl_config_get_int("visual.crosshair_size", 10);
    g_ovl_settings.crosshair_r = ovl_config_get_int("visual.crosshair_r", 0);
    g_ovl_settings.crosshair_g = ovl_config_get_int("visual.crosshair_g", 255);
    g_ovl_settings.crosshair_b = ovl_config_get_int("visual.crosshair_b", 0);
    g_ovl_settings.menu_r = ovl_config_get_int("ui.menu_r", 24);
    g_ovl_settings.menu_g = ovl_config_get_int("ui.menu_g", 24);
    g_ovl_settings.menu_b = ovl_config_get_int("ui.menu_b", 32);
    g_ovl_settings.menu_alpha = ovl_config_get_int("ui.menu_alpha", 220);
    g_ovl_settings.stick_move_speed = ovl_config_get_int("ui.stick_move_speed", 8);
    g_ovl_settings.stick_adjust_speed = ovl_config_get_int("ui.stick_adjust_speed", 2);
    return OVL_OK;
}
