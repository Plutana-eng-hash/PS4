#ifndef OVL_HOOKS_H
#define OVL_HOOKS_H

#include <stdint.h>
#include "ovl_types.h"

/* Resolves the video-out and pad APIs and installs the hooks. */
ovl_status_t hooks_install(void);
void hooks_remove(void);

#endif
