#ifndef OVL_DRAW_H
#define OVL_DRAW_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "ovl_types.h"

#define DRAW_MAX_CMDS 640
#define DRAW_TEXT_MAX 40
#define DRAW_GLYPH_W 8

/* Pixel formats of the scan-out buffers we can write to. */
#define OVL_FMT_A8R8G8B8_SRGB   0x80000000u
#define OVL_FMT_A8B8G8R8_SRGB   0x80002200u
#define OVL_FMT_A2R10G10B10     0x88060000u
#define OVL_FMT_A2R10G10B10_SRGB 0x88000000u

/* Point the renderer at a frame buffer for this frame. tiled=1 selects the
 * 8x8 micro-tiled layout, 0 selects linear. */
void draw_set_surface(void *base, int32_t w, int32_t h, int32_t pitch_px,
                      uint32_t fmt, int32_t tiled);
bool draw_surface_ok(void);

void draw_begin(void);
void draw_rect(int32_t x, int32_t y, int32_t w, int32_t h, ovl_color_t color);
void draw_pill(int32_t x, int32_t y, int32_t w, int32_t h, ovl_color_t color);
void draw_text(int32_t x, int32_t y, const char *text, ovl_color_t color, int32_t scale);
int32_t draw_text_width(const char *text, int32_t scale);
void draw_flush(void);

#endif
