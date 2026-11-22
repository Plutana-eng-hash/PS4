#ifndef OVL_FEATURES_H
#define OVL_FEATURES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "ovl_types.h"

typedef uint64_t (*ovl_clock_fn)(void);

typedef bool (*ovl_info_read_fn)(char *buf, size_t len);

typedef struct {
    const char *name;
    ovl_info_read_fn read;
} ovl_info_provider_t;

void features_init(void);
void features_set_clock(ovl_clock_fn fn);
void features_set_screen_size(int32_t w, int32_t h);
void features_screen_size(int32_t *w, int32_t *h);

void features_frame_tick(void);
uint32_t features_fps(void);

void features_draw_overlay(void);

ovl_status_t features_register_info(const ovl_info_provider_t *provider);
size_t features_info_count(void);
const char *features_info_line(size_t index, char *buf, size_t len);

#endif
