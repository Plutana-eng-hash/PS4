#ifndef OVL_NOTIFY_H
#define OVL_NOTIFY_H

#include <stdbool.h>

/* Resolve the system notification call. Safe to call more than once. */
void notify_init(void);

/* Show a PlayStation system notification (top-right toast). Honors the
 * notify.enabled setting. Returns true if the request was sent. */
bool notify_send(const char *message);

#endif
