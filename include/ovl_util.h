#ifndef OVL_UTIL_H
#define OVL_UTIL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define OVL_DATA_DIR "/data/GoldHEN/plugins/ps4_overlay_plugin/"

size_t ovl_strlen(const char *s);
int ovl_strcmp(const char *a, const char *b);
void ovl_copy(char *dst, size_t cap, const char *src);
void ovl_cat(char *dst, size_t cap, const char *src);
void ovl_cat_int(char *dst, size_t cap, int64_t v);
void ovl_cat_2d(char *dst, size_t cap, int32_t v);
int32_t ovl_atoi(const char *s);

int ovl_file_read(const char *path, char *buf, size_t cap);
int ovl_file_write(const char *path, const char *buf, size_t len, bool append);

uint64_t ovl_time_us(void);

void ovl_log_reset(void);
void ovl_log(const char *msg);
void ovl_log_int(const char *label, int64_t v);

#endif
