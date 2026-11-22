#ifndef OVL_OFFSETS_H
#define OVL_OFFSETS_H

#include <stdint.h>
#include "ovl_types.h"

typedef struct {
    uintptr_t module_base;
    uintptr_t render_present;
    uintptr_t pad_read;
} ovl_offsets_t;

ovl_status_t ovl_offsets_load(ovl_offsets_t *out);

#endif
