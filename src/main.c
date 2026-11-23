#include <stddef.h>
#include "ovl_config.h"
#include "ovl_features.h"
#include "ovl_hooks.h"
#include "ovl_notify.h"
#include "ovl_ui.h"
#include "ovl_util.h"

int module_start(size_t argc, const void *argp) {
    (void)argc;
    (void)argp;

    ovl_log_reset();
    ovl_log("ps4_overlay_plugin starting");

    if (ovl_settings_load() != OVL_OK) {
        ovl_log("ovl_config.ini not found, using defaults");
    }

    notify_send("PS4 Overlay\nStarting. . .");

    features_init();
    ui_init();

    if (hooks_install() != OVL_OK) {
        ovl_log("render hook NOT installed - see messages above");
        notify_send("PS4 Overlay\nFailed to start - see ovl.log");
    }
    return 0;
}

int module_stop(size_t argc, const void *argp) {
    (void)argc;
    (void)argp;
    hooks_remove();
    return 0;
}
