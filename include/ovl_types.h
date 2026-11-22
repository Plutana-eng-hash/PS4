#ifndef OVL_TYPES_H
#define OVL_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    OVL_OK = 0,
    OVL_ERR_NULL = -1,
    OVL_ERR_IO = -2,
    OVL_ERR_FULL = -3
} ovl_status_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} ovl_color_t;

#define OVL_COLOR(r_, g_, b_, a_) ((ovl_color_t){(uint8_t)(r_), (uint8_t)(g_), (uint8_t)(b_), (uint8_t)(a_)})

#endif
