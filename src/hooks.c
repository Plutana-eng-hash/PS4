#include "ovl_hooks.h"
#include "ovl_platform.h"
#include "ovl_config.h"
#include "ovl_draw.h"
#include "ovl_features.h"
#include "ovl_input.h"
#include "ovl_notify.h"
#include "ovl_ui.h"
#include "ovl_util.h"

#define LIB_VIDEOOUT "/system/common/lib/libSceVideoOut.sprx"
#define LIB_PAD      "/system/common/lib/libScePad.sprx"
#define LIB_USER     "/system/common/lib/libSceUserService.sprx"
#define MAX_BUFFERS  16

typedef int (*submit_flip_fn)(int handle, int index, uint32_t mode, int64_t arg);
typedef int (*register_fn)(int handle, int start, void *const *addrs, int num, const void *attr);
typedef int (*pad_state_fn)(int handle, void *data);
typedef int (*pad_handle_fn)(int user, int type, int index);
typedef int (*user_fn)(int *user);

static submit_flip_fn o_submit = NULL;
static register_fn o_register = NULL;
static pad_state_fn o_pad_state = NULL;
static pad_handle_fn f_pad_handle = NULL;
static user_fn f_initial_user = NULL;

static struct {
    void *addr[MAX_BUFFERS];
    int32_t count;
    uint32_t fmt;
    int32_t tiling;
    uint32_t w, h, pitch;
} g_fb;

static uint32_t g_frame = 0;
static uint32_t g_pad_frame = 0;
static int g_pad_handle = 0;
static uint32_t g_flags = 0;
static uint8_t g_poll_buf[256] __attribute__((aligned(16)));
static bool g_warned_fmt = false;

/* ScePadData: buttons u32 @0, left stick @4/5, right stick @6/7, L2/R2 @8/9 */
static void pad_parse(const uint8_t *d, ovl_pad_state_t *out) {
    out->buttons = (uint32_t)d[0] | ((uint32_t)d[1] << 8) | ((uint32_t)d[2] << 16) |
                   ((uint32_t)d[3] << 24);
    out->lx = d[4];
    out->ly = d[5];
    out->rx = d[6];
    out->ry = d[7];
    out->l2 = d[8];
    out->r2 = d[9];
}

static void pad_neutralise(uint8_t *d) {
    d[0] = d[1] = d[2] = d[3] = 0;
    d[4] = d[5] = d[6] = d[7] = 0x80;
    d[8] = d[9] = 0;
}

static int h_pad_state(int handle, void *data) {
    int r = o_pad_state(handle, data);
    if (r == 0 && data != NULL) {
        ovl_pad_state_t s;
        pad_parse((const uint8_t *)data, &s);
        g_pad_handle = handle;
        g_pad_frame = g_frame;
        input_feed(&s);
        /* Hide the open combo from the game, and everything while the menu is up. */
        if (input_combo_active() || (ui_is_visible() && g_ovl_settings.block_input)) {
            pad_neutralise((uint8_t *)data);
        }
    }
    return r;
}

static int h_register(int handle, int start, void *const *addrs, int num, const void *attr) {
    if (addrs != NULL && attr != NULL && start >= 0) {
        const uint32_t *a = (const uint32_t *)attr;
        g_fb.fmt = a[0];
        g_fb.tiling = (int32_t)a[1];
        g_fb.w = a[3];
        g_fb.h = a[4];
        g_fb.pitch = a[5];
        for (int i = 0; i < num && start + i < MAX_BUFFERS; i++) {
            g_fb.addr[start + i] = addrs[i];
            if (start + i + 1 > g_fb.count) {
                g_fb.count = start + i + 1;
            }
        }
    }
    return o_register(handle, start, addrs, num, attr);
}

static void poll_pad_fallback(void) {
    if (o_pad_state == NULL || g_frame - g_pad_frame <= 3) {
        return;
    }
    int handle = g_pad_handle;
    if (handle <= 0 && f_pad_handle != NULL && f_initial_user != NULL) {
        int user = 0;
        if (f_initial_user(&user) == 0) {
            handle = f_pad_handle(user, 0, 0);
        }
    }
    if (handle > 0 && o_pad_state(handle, g_poll_buf) == 0) {
        ovl_pad_state_t s;
        pad_parse(g_poll_buf, &s);
        input_feed(&s);
    }
}

static void on_present(int index) {
    g_frame++;
    if (g_frame == 1) {
        ovl_log("first frame presented - overlay is live");
        features_show_banner();
        notify_send("PS4 Overlay\nReady");
    }
    features_frame_tick();
    poll_pad_fallback();
    ui_update(input_state());

    if (index < 0 || index >= MAX_BUFFERS || g_fb.addr[index] == NULL) {
        return;
    }
    int32_t tiled = (g_ovl_settings.layout == 1) ? 0
                  : (g_ovl_settings.layout == 2) ? 1
                  : (g_fb.tiling == 0 ? 1 : 0);
    draw_set_surface(g_fb.addr[index], (int32_t)g_fb.w, (int32_t)g_fb.h,
                     (int32_t)g_fb.pitch, g_fb.fmt, tiled);
    features_set_screen((int32_t)g_fb.w, (int32_t)g_fb.h, g_fb.fmt, tiled, g_fb.count);
    features_set_hook_flags(g_flags);

    if (!draw_surface_ok()) {
        if (!g_warned_fmt) {
            g_warned_fmt = true;
            ovl_log_int("unsupported pixel format", g_fb.fmt);
        }
        return;
    }
    draw_begin();
    features_draw_overlay();
    ui_draw();
    draw_flush();
}

static int h_submit(int handle, int index, uint32_t mode, int64_t arg) {
    on_present(index);
    return o_submit(handle, index, mode, arg);
}

ovl_status_t hooks_install(void) {
    void *t_submit = ovl_resolve(LIB_VIDEOOUT, "sceVideoOutSubmitFlip");
    void *t_register = ovl_resolve(LIB_VIDEOOUT, "sceVideoOutRegisterBuffers");
    void *t_pad = ovl_resolve(LIB_PAD, "scePadReadState");
    f_pad_handle = (pad_handle_fn)ovl_resolve(LIB_PAD, "scePadGetHandle");
    f_initial_user = (user_fn)ovl_resolve(LIB_USER, "sceUserServiceGetInitialUser");

    ovl_status_t st = OVL_ERR_NULL;
    if (t_register != NULL && ovl_hook_install(t_register, (void *)h_register, (void **)&o_register) == OVL_OK) {
        g_flags |= 2;
        ovl_log("hooked sceVideoOutRegisterBuffers");
    }
    if (t_pad != NULL && ovl_hook_install(t_pad, (void *)h_pad_state, (void **)&o_pad_state) == OVL_OK) {
        g_flags |= 4;
        ovl_log("hooked scePadReadState");
    }
    if (t_submit != NULL && ovl_hook_install(t_submit, (void *)h_submit, (void **)&o_submit) == OVL_OK) {
        g_flags |= 1;
        ovl_log("hooked sceVideoOutSubmitFlip");
        st = OVL_OK;
    }
    ovl_log_int("hook flags (1=flip 2=register 4=pad)", g_flags);
    return st;
}

void hooks_remove(void) {
    /* Hooks stay in place for the life of the process; the detours are
     * harmless if the module is idle. */
}
