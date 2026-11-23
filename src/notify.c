#include "ovl_notify.h"
#include "ovl_config.h"
#include "ovl_util.h"

extern int sceKernelDlsym(int handle, const char *symbol, void **addr);

/* Layout used by sceKernelSendNotificationRequest for plain text toasts. */
typedef struct {
    char unused[45];
    char message[3075];
} notify_request_t;

typedef int (*send_notification_fn)(int device, notify_request_t *req, size_t size, int blocking);

#define LIBKERNEL_HANDLE 0x2001

static send_notification_fn g_send = NULL;
static bool g_resolved = false;
static notify_request_t g_req;

void notify_init(void) {
    if (g_resolved) {
        return;
    }
    g_resolved = true;
    void *fn = NULL;
    if (sceKernelDlsym(LIBKERNEL_HANDLE, "sceKernelSendNotificationRequest", &fn) == 0 && fn != NULL) {
        g_send = (send_notification_fn)fn;
        ovl_log("notification call resolved");
    } else {
        ovl_log("sceKernelSendNotificationRequest not found - notifications disabled");
    }
}

bool notify_send(const char *message) {
    if (message == NULL || !g_ovl_settings.notify) {
        return false;
    }
    notify_init();
    if (g_send == NULL) {
        return false;
    }
    for (size_t i = 0; i < sizeof g_req; i++) {
        ((volatile char *)&g_req)[i] = 0;
    }
    ovl_copy(g_req.message, sizeof g_req.message, message);
    int r = g_send(0, &g_req, sizeof g_req, 0);
    if (r != 0) {
        ovl_log_int("notification send returned", r);
    }
    return r == 0;
}
