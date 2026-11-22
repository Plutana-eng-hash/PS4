#include <stddef.h>
#include "ovl_hooks.h"
#include "ovl_offsets.h"
#include "ovl_features.h"
#include "ovl_input.h"
#include "ovl_ui.h"
#include "ovl_draw.h"

static ovl_hook_backend_fn g_backend = NULL;
static bool g_installed = false;

void hooks_set_backend(ovl_hook_backend_fn fn) {
    g_backend = fn;
}

void hooks_on_present(void) {

    features_frame_tick();
    input_update();
    ui_update(input_state());

    draw_begin();
    features_draw_overlay();
    ui_draw();
    draw_flush();
}

static void hook_detour_present(void) {
    hooks_on_present();
}

ovl_status_t hooks_install(void) {
    ovl_offsets_t off;
    if (ovl_offsets_load(&off) != OVL_OK) {
        return OVL_ERR_IO;
    }

    if (off.module_base == 0 || off.render_present == 0) {
        return OVL_ERR_NULL;
    }

    if (g_backend == NULL) {
        return OVL_ERR_NULL;
    }
    ovl_status_t s = g_backend(off.module_base + off.render_present, (void *)hook_detour_present);
    g_installed = (s == OVL_OK);
    return s;
}

void hooks_remove(void) {

    g_installed = false;
}
