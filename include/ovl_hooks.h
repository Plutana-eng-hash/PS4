#ifndef OVL_HOOKS_H
#define OVL_HOOKS_H

#include <stdint.h>
#include "ovl_types.h"

typedef ovl_status_t (*ovl_hook_backend_fn)(uintptr_t target, void *detour);

void hooks_set_backend(ovl_hook_backend_fn fn);

ovl_status_t hooks_install(void);
void hooks_remove(void);

void hooks_on_present(void);

#endif
