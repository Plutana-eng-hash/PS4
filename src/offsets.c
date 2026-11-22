#include "ovl_offsets.h"
#include "ovl_config.h"

ovl_status_t ovl_offsets_load(ovl_offsets_t *out) {
    if (out == NULL) {
        return OVL_ERR_NULL;
    }

    out->module_base = (uintptr_t)ovl_config_get_int("offset.module_base", 0);
    out->render_present = (uintptr_t)ovl_config_get_int("offset.render_present", 0);
    out->pad_read = (uintptr_t)ovl_config_get_int("offset.pad_read", 0);
    return OVL_OK;
}
