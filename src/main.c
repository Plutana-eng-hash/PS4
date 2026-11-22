#include <stddef.h>
#include "ovl_config.h"
#include "ovl_input.h"
#include "ovl_ui.h"
#include "ovl_features.h"
#include "ovl_hooks.h"

#define OVL_DATA_DIR "/data/GoldHEN/plugins/ps4_overlay_plugin/"

int module_start(size_t argc, const void *argp) {
    (void)argc;
    (void)argp;

    ovl_config_load(OVL_DATA_DIR "ovl_config.ini");
    ovl_config_load(OVL_DATA_DIR "offsets.ini");
    ovl_settings_load();

    features_init();
    ui_init();
    input_set_reader(NULL);

    (void)hooks_install();
    return 0;
}

int module_stop(size_t argc, const void *argp) {
    (void)argc;
    (void)argp;
    hooks_remove();
    return 0;
}
