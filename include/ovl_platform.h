#ifndef OVL_PLATFORM_H
#define OVL_PLATFORM_H

#include "ovl_types.h"

/* Resolve an exported symbol from a system module (loads it if needed). */
void *ovl_resolve(const char *module_path, const char *symbol);

/* Inline-hook target with detour. *orig receives a callable trampoline. */
ovl_status_t ovl_hook_install(void *target, void *detour, void **orig);

#endif
