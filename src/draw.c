#include <string.h>
#include "ovl_draw.h"

static ovl_cmd_t g_cmds[DRAW_MAX_CMDS];
static size_t g_cmd_count = 0;
static ovl_draw_backend_fn g_backend = NULL;

static void draw_push(const ovl_cmd_t *cmd) {
    if (g_cmd_count < DRAW_MAX_CMDS) {
        g_cmds[g_cmd_count++] = *cmd;
    }
}

void draw_set_backend(ovl_draw_backend_fn fn) {
    g_backend = fn;
}

void draw_begin(void) {
    g_cmd_count = 0;
}

void draw_rect_fill(int32_t x, int32_t y, int32_t w, int32_t h, ovl_color_t color) {
    ovl_cmd_t c;
    memset(&c, 0, sizeof c);
    c.kind = OVL_CMD_RECT_FILL;
    c.x = x;
    c.y = y;
    c.w = w;
    c.h = h;
    c.color = color;
    draw_push(&c);
}

void draw_line(int32_t x, int32_t y, int32_t dx, int32_t dy, ovl_color_t color) {
    ovl_cmd_t c;
    memset(&c, 0, sizeof c);
    c.kind = OVL_CMD_LINE;
    c.x = x;
    c.y = y;
    c.w = dx;
    c.h = dy;
    c.color = color;
    draw_push(&c);
}

void draw_text(int32_t x, int32_t y, const char *text, ovl_color_t color) {
    if (text == NULL) {
        return;
    }
    ovl_cmd_t c;
    memset(&c, 0, sizeof c);
    c.kind = OVL_CMD_TEXT;
    c.x = x;
    c.y = y;
    c.color = color;
    strncpy(c.text, text, DRAW_TEXT_MAX - 1);
    c.text[DRAW_TEXT_MAX - 1] = '\0';
    draw_push(&c);
}

void draw_flush(void) {

    if (g_backend != NULL && g_cmd_count > 0) {
        g_backend(g_cmds, g_cmd_count);
    }
    g_cmd_count = 0;
}
