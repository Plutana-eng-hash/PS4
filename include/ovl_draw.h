#ifndef OVL_DRAW_H
#define OVL_DRAW_H

#include <stdint.h>
#include <stddef.h>
#include "ovl_types.h"

#define DRAW_MAX_CMDS 1024
#define DRAW_TEXT_MAX 64

typedef enum {
    OVL_CMD_RECT_FILL,
    OVL_CMD_LINE,
    OVL_CMD_TEXT
} ovl_cmd_kind_t;

typedef struct {
    ovl_cmd_kind_t kind;
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    ovl_color_t color;
    char text[DRAW_TEXT_MAX];
} ovl_cmd_t;

typedef void (*ovl_draw_backend_fn)(const ovl_cmd_t *cmds, size_t count);

void draw_set_backend(ovl_draw_backend_fn fn);
void draw_begin(void);
void draw_rect_fill(int32_t x, int32_t y, int32_t w, int32_t h, ovl_color_t color);
void draw_line(int32_t x, int32_t y, int32_t dx, int32_t dy, ovl_color_t color);
void draw_text(int32_t x, int32_t y, const char *text, ovl_color_t color);
void draw_flush(void);

#endif
