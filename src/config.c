#include "ovl_config.h"
#include "ovl_util.h"

#define CFG_MAX_ENTRIES 64
#define CFG_KEY_LEN 32
#define CFG_VAL_LEN 24
#define CFG_FILE_MAX 4096

#define CFG_PATH OVL_DATA_DIR "ovl_config.ini"

typedef struct {
    char key[CFG_KEY_LEN];
    char val[CFG_VAL_LEN];
} cfg_entry_t;

static cfg_entry_t g_entries[CFG_MAX_ENTRIES];
static size_t g_count = 0;
static char g_file[CFG_FILE_MAX];

ovl_settings_t g_ovl_settings;

typedef struct {
    const char *key;
    int32_t *p;
    int32_t def;
    int32_t min;
    int32_t max;
} setting_def_t;

static const setting_def_t g_defs[] = {
    {"hud.fps",        &g_ovl_settings.show_fps,       1, 0, 1},
    {"hud.frametime",  &g_ovl_settings.show_frametime, 0, 0, 1},
    {"hud.resolution", &g_ovl_settings.show_res,       0, 0, 1},
    {"hud.uptime",     &g_ovl_settings.show_uptime,    0, 0, 1},
    {"hud.corner",     &g_ovl_settings.hud_corner,     0, 0, 3},
    {"hud.scale",      &g_ovl_settings.hud_scale,      2, 1, 3},
    {"sight.enabled",  &g_ovl_settings.xh_on,          0, 0, 1},
    {"sight.style",    &g_ovl_settings.xh_style,       0, 0, 3},
    {"sight.size",     &g_ovl_settings.xh_size,       12, 1, 64},
    {"sight.thickness",&g_ovl_settings.xh_thick,       2, 1, 8},
    {"sight.gap",      &g_ovl_settings.xh_gap,         4, 0, 32},
    {"sight.color",    &g_ovl_settings.xh_color,       3, 0, 7},
    {"sight.outline",  &g_ovl_settings.xh_outline,     1, 0, 1},
    {"sight.dot",      &g_ovl_settings.xh_dot,         0, 0, 1},
    {"menu.theme",     &g_ovl_settings.theme,          0, 0, 1},
    {"menu.scale",     &g_ovl_settings.ui_scale,       2, 1, 3},
    {"menu.x",         &g_ovl_settings.menu_x,         4, 0, 100},
    {"menu.y",         &g_ovl_settings.menu_y,        35, 0, 100},
    {"menu.combo",     &g_ovl_settings.combo,          0, 0, 3},
    {"menu.hints",     &g_ovl_settings.show_hints,     1, 0, 1},
    {"menu.block_input",&g_ovl_settings.block_input,   1, 0, 1},
    {"render.layout",  &g_ovl_settings.layout,         0, 0, 2},
};

#define DEF_COUNT (sizeof g_defs / sizeof g_defs[0])

static bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static char *trim(char *s) {
    while (is_space(*s)) {
        s++;
    }
    size_t n = ovl_strlen(s);
    while (n > 0 && is_space(s[n - 1])) {
        s[--n] = '\0';
    }
    return s;
}

static void store(const char *key, const char *val) {
    for (size_t i = 0; i < g_count; i++) {
        if (ovl_strcmp(g_entries[i].key, key) == 0) {
            ovl_copy(g_entries[i].val, CFG_VAL_LEN, val);
            return;
        }
    }
    if (g_count < CFG_MAX_ENTRIES) {
        ovl_copy(g_entries[g_count].key, CFG_KEY_LEN, key);
        ovl_copy(g_entries[g_count].val, CFG_VAL_LEN, val);
        g_count++;
    }
}

ovl_status_t ovl_config_load(const char *path) {
    int n = ovl_file_read(path, g_file, sizeof g_file);
    if (n < 0) {
        return OVL_ERR_IO;
    }
    char *line = g_file;
    while (*line != '\0') {
        char *end = line;
        while (*end != '\0' && *end != '\n') {
            end++;
        }
        char saved = *end;
        *end = '\0';

        char *l = trim(line);
        if (l[0] != '\0' && l[0] != '#' && l[0] != ';' && l[0] != '[') {
            char *eq = l;
            while (*eq != '\0' && *eq != '=') {
                eq++;
            }
            if (*eq == '=') {
                *eq = '\0';
                char *k = trim(l);
                char *v = trim(eq + 1);
                if (k[0] != '\0') {
                    store(k, v);
                }
            }
        }
        if (saved == '\0') {
            break;
        }
        line = end + 1;
    }
    return OVL_OK;
}

int32_t ovl_config_get_int(const char *key, int32_t def) {
    for (size_t i = 0; i < g_count; i++) {
        if (ovl_strcmp(g_entries[i].key, key) == 0) {
            const char *v = g_entries[i].val;
            if (v[0] == '\0') {
                return def;
            }
            return ovl_atoi(v);
        }
    }
    return def;
}

static int32_t clamp(int32_t v, int32_t lo, int32_t hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void ovl_settings_defaults(void) {
    for (size_t i = 0; i < DEF_COUNT; i++) {
        *g_defs[i].p = g_defs[i].def;
    }
}

ovl_status_t ovl_settings_load(void) {
    g_count = 0;
    ovl_settings_defaults();
    ovl_status_t st = ovl_config_load(CFG_PATH);
    for (size_t i = 0; i < DEF_COUNT; i++) {
        int32_t v = ovl_config_get_int(g_defs[i].key, g_defs[i].def);
        *g_defs[i].p = clamp(v, g_defs[i].min, g_defs[i].max);
    }
    return st;
}

ovl_status_t ovl_settings_save(void) {
    g_file[0] = '\0';
    ovl_cat(g_file, sizeof g_file, "# PS4 Overlay settings (written by the in-game menu)\n");
    for (size_t i = 0; i < DEF_COUNT; i++) {
        ovl_cat(g_file, sizeof g_file, g_defs[i].key);
        ovl_cat(g_file, sizeof g_file, " = ");
        ovl_cat_int(g_file, sizeof g_file, *g_defs[i].p);
        ovl_cat(g_file, sizeof g_file, "\n");
    }
    return ovl_file_write(CFG_PATH, g_file, ovl_strlen(g_file), false) == 0 ? OVL_OK : OVL_ERR_IO;
}
