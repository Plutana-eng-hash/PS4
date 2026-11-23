#include "ovl_util.h"

/* Direct libkernel imports. The module deliberately avoids the C library so
 * it works even when the host process never initialised it for this module. */
extern int sceKernelOpen(const char *path, int flags, int mode);
extern int64_t sceKernelRead(int fd, void *buf, size_t n);
extern int64_t sceKernelWrite(int fd, const void *buf, size_t n);
extern int sceKernelClose(int fd);
extern uint64_t sceKernelGetProcessTime(void);

#define OVL_O_RDONLY 0x0000
#define OVL_O_WRONLY 0x0001
#define OVL_O_APPEND 0x0008
#define OVL_O_CREAT  0x0200
#define OVL_O_TRUNC  0x0400

/* Compiler-generated calls (struct copies, zero-init) land here. Volatile
 * keeps the optimiser from turning these loops back into calls to themselves. */
void *memcpy(void *d, const void *s, size_t n) {
    volatile uint8_t *a = (volatile uint8_t *)d;
    const uint8_t *b = (const uint8_t *)s;
    while (n--) {
        *a++ = *b++;
    }
    return d;
}

void *memmove(void *d, const void *s, size_t n) {
    volatile uint8_t *a = (volatile uint8_t *)d;
    const uint8_t *b = (const uint8_t *)s;
    if (a < b) {
        while (n--) {
            *a++ = *b++;
        }
    } else {
        a += n;
        b += n;
        while (n--) {
            *--a = *--b;
        }
    }
    return d;
}

void *memset(void *d, int c, size_t n) {
    volatile uint8_t *a = (volatile uint8_t *)d;
    while (n--) {
        *a++ = (uint8_t)c;
    }
    return d;
}

int memcmp(const void *x, const void *y, size_t n) {
    const uint8_t *a = (const uint8_t *)x;
    const uint8_t *b = (const uint8_t *)y;
    while (n--) {
        if (*a != *b) {
            return (int)*a - (int)*b;
        }
        a++;
        b++;
    }
    return 0;
}

size_t ovl_strlen(const char *s) {
    size_t n = 0;
    while (s != NULL && s[n] != '\0') {
        n++;
    }
    return n;
}

int ovl_strcmp(const char *a, const char *b) {
    while (*a != '\0' && *a == *b) {
        a++;
        b++;
    }
    return (int)(uint8_t)*a - (int)(uint8_t)*b;
}

void ovl_copy(char *dst, size_t cap, const char *src) {
    if (dst == NULL || cap == 0) {
        return;
    }
    size_t i = 0;
    while (src != NULL && src[i] != '\0' && i + 1 < cap) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

void ovl_cat(char *dst, size_t cap, const char *src) {
    size_t n = ovl_strlen(dst);
    if (n >= cap) {
        return;
    }
    ovl_copy(dst + n, cap - n, src);
}

void ovl_cat_int(char *dst, size_t cap, int64_t v) {
    char tmp[24];
    int pos = 23;
    bool neg = v < 0;
    uint64_t u = neg ? (uint64_t)(-v) : (uint64_t)v;
    tmp[pos] = '\0';
    do {
        tmp[--pos] = (char)('0' + (u % 10));
        u /= 10;
    } while (u != 0 && pos > 1);
    if (neg) {
        tmp[--pos] = '-';
    }
    ovl_cat(dst, cap, tmp + pos);
}

void ovl_cat_2d(char *dst, size_t cap, int32_t v) {
    if (v < 10) {
        ovl_cat(dst, cap, "0");
    }
    ovl_cat_int(dst, cap, v);
}

int32_t ovl_atoi(const char *s) {
    if (s == NULL) {
        return 0;
    }
    bool neg = false;
    if (*s == '-') {
        neg = true;
        s++;
    }
    int32_t v = 0;
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (*s - '0');
        s++;
    }
    return neg ? -v : v;
}

int ovl_file_read(const char *path, char *buf, size_t cap) {
    if (path == NULL || buf == NULL || cap < 2) {
        return -1;
    }
    int fd = sceKernelOpen(path, OVL_O_RDONLY, 0);
    if (fd < 0) {
        return -1;
    }
    size_t total = 0;
    while (total + 1 < cap) {
        int64_t r = sceKernelRead(fd, buf + total, cap - 1 - total);
        if (r <= 0) {
            break;
        }
        total += (size_t)r;
    }
    sceKernelClose(fd);
    buf[total] = '\0';
    return (int)total;
}

int ovl_file_write(const char *path, const char *buf, size_t len, bool append) {
    if (path == NULL || buf == NULL) {
        return -1;
    }
    int flags = OVL_O_WRONLY | OVL_O_CREAT | (append ? OVL_O_APPEND : OVL_O_TRUNC);
    int fd = sceKernelOpen(path, flags, 0777);
    if (fd < 0) {
        return -1;
    }
    size_t done = 0;
    while (done < len) {
        int64_t w = sceKernelWrite(fd, buf + done, len - done);
        if (w <= 0) {
            break;
        }
        done += (size_t)w;
    }
    sceKernelClose(fd);
    return (done == len) ? 0 : -1;
}

uint64_t ovl_time_us(void) {
    return sceKernelGetProcessTime();
}

#define OVL_LOG_PATH OVL_DATA_DIR "ovl.log"

void ovl_log_reset(void) {
    ovl_file_write(OVL_LOG_PATH, "", 0, false);
}

void ovl_log(const char *msg) {
    char line[200];
    line[0] = '\0';
    ovl_cat(line, sizeof line, msg);
    ovl_cat(line, sizeof line, "\n");
    ovl_file_write(OVL_LOG_PATH, line, ovl_strlen(line), true);
}

void ovl_log_int(const char *label, int64_t v) {
    char line[200];
    line[0] = '\0';
    ovl_cat(line, sizeof line, label);
    ovl_cat(line, sizeof line, ": ");
    ovl_cat_int(line, sizeof line, v);
    ovl_log(line);
}
