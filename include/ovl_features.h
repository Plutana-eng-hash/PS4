#ifndef OVL_FEATURES_H
#define OVL_FEATURES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void features_init(void);
void features_set_screen(int32_t w, int32_t h, uint32_t fmt, int32_t tiled, int32_t buffers);
void features_screen_size(int32_t *w, int32_t *h);
void features_set_hook_flags(uint32_t flags);

void features_frame_tick(void);
uint32_t features_fps(void);

/* HUD + crosshair, drawn every frame. */
void features_draw_overlay(void);

/* Lines for the INFO tab. Returns false past the last line. */
bool features_info_line(size_t index, char *buf, size_t len);

#endif
