#include "ovl_platform.h"
#include "ovl_util.h"

extern int sceKernelLoadStartModule(const char *name, size_t argc, const void *argv,
                                    uint32_t flags, const void *opt, int *res);
extern int sceKernelDlsym(int handle, const char *symbol, void **addr);
extern int sceKernelMprotect(void *addr, size_t len, int prot);
extern int sceKernelMmap(void *addr, size_t len, int prot, int flags, int fd,
                         int64_t off, void **res);

#define OVL_PAGE 0x4000u
#define PROT_RX  5
#define PROT_RWX 7
#define ABS_JMP_LEN 14

static uint8_t g_static_pool[OVL_PAGE] __attribute__((aligned(OVL_PAGE)));
static uint8_t *g_pool = NULL;
static size_t g_pool_used = 0;
static size_t g_pool_cap = 0;

void *ovl_resolve(const char *module_path, const char *symbol) {
    int h = sceKernelLoadStartModule(module_path, 0, NULL, 0, NULL, NULL);
    if (h < 0) {
        ovl_log_int("module load failed", h);
        ovl_log(module_path);
        return NULL;
    }
    void *addr = NULL;
    if (sceKernelDlsym(h, symbol, &addr) != 0 || addr == NULL) {
        ovl_log("symbol not found:");
        ovl_log(symbol);
        return NULL;
    }
    return addr;
}

/* ---- minimal x86-64 length decoder for function prologues ---------------
 * Handles the common integer/SSE-move subset. Anything position dependent
 * (rip-relative operands, relative jumps/calls) or unknown returns 0 and the
 * hook is refused rather than risking a corrupt trampoline. */
static int modrm_len(const uint8_t *m) {
    uint8_t mod = m[0] >> 6;
    uint8_t rm = m[0] & 7;
    int len = 1;
    if (mod == 0 && rm == 5) {
        return 0;
    }
    if (mod != 3 && rm == 4) {
        len++;
        if (mod == 0 && (m[1] & 7) == 5) {
            len += 4;
        }
    }
    if (mod == 1) {
        len += 1;
    } else if (mod == 2) {
        len += 4;
    }
    return len;
}

static int insn_len(const uint8_t *p) {
    int i = 0;
    bool opsz = false;
    bool rexw = false;
    while (i < 4) {
        uint8_t b = p[i];
        if (b == 0x66) {
            opsz = true;
        } else if (b != 0xF2 && b != 0xF3 && b != 0x2E && b != 0x3E && b != 0x64 && b != 0x65) {
            break;
        }
        i++;
    }
    if ((p[i] & 0xF0) == 0x40) {
        rexw = (p[i] & 8) != 0;
        i++;
    }
    uint8_t op = p[i++];
    int m;

    if ((op >= 0x50 && op <= 0x5F) || op == 0x90 || op == 0x98 || op == 0x99) {
        return i;
    }
    if (op == 0x6A) {
        return i + 1;
    }
    if (op == 0x68) {
        return i + 4;
    }
    if (op < 0x40 && op != 0x0F) {
        uint8_t low = op & 7;
        if (low < 4) {
            m = modrm_len(p + i);
            return m ? i + m : 0;
        }
        if (low == 4) {
            return i + 1;
        }
        if (low == 5) {
            return i + 4;
        }
        return 0;
    }
    switch (op) {
    case 0x63: case 0x84: case 0x85: case 0x86: case 0x87: case 0x88:
    case 0x89: case 0x8A: case 0x8B: case 0x8D: case 0xFE: case 0xFF:
        m = modrm_len(p + i);
        return m ? i + m : 0;
    case 0x69:
        m = modrm_len(p + i);
        return m ? i + m + 4 : 0;
    case 0x6B: case 0x80: case 0x83: case 0xC6:
        m = modrm_len(p + i);
        return m ? i + m + 1 : 0;
    case 0x81:
        m = modrm_len(p + i);
        return m ? i + m + (opsz ? 2 : 4) : 0;
    case 0xC7:
        m = modrm_len(p + i);
        return m ? i + m + (opsz ? 2 : 4) : 0;
    case 0xF6:
        m = modrm_len(p + i);
        return m ? i + m + ((((p[i] >> 3) & 7) < 2) ? 1 : 0) : 0;
    case 0xF7:
        m = modrm_len(p + i);
        return m ? i + m + ((((p[i] >> 3) & 7) < 2) ? 4 : 0) : 0;
    case 0xA8:
        return i + 1;
    case 0xA9:
        return i + 4;
    case 0x0F: {
        uint8_t o2 = p[i++];
        if (o2 == 0x05) {
            return i;
        }
        if (o2 == 0x1E || o2 == 0x1F || o2 == 0xB6 || o2 == 0xB7 || o2 == 0xBE ||
            o2 == 0xBF || o2 == 0xAF || (o2 >= 0x40 && o2 <= 0x4F) ||
            (o2 >= 0x90 && o2 <= 0x9F) || o2 == 0x10 || o2 == 0x11 || o2 == 0x28 ||
            o2 == 0x29 || o2 == 0x6F || o2 == 0x7F || o2 == 0xEF || o2 == 0xD6 ||
            o2 == 0x6E || o2 == 0x7E) {
            m = modrm_len(p + i);
            return m ? i + m : 0;
        }
        return 0;
    }
    default:
        break;
    }
    if (op >= 0xB0 && op <= 0xB7) {
        return i + 1;
    }
    if (op >= 0xB8 && op <= 0xBF) {
        return i + (rexw ? 8 : (opsz ? 2 : 4));
    }
    return 0;
}

static void write_abs_jmp(uint8_t *dst, const void *target) {
    dst[0] = 0xFF;
    dst[1] = 0x25;
    dst[2] = dst[3] = dst[4] = dst[5] = 0;
    uint64_t a = (uint64_t)(uintptr_t)target;
    for (int i = 0; i < 8; i++) {
        dst[6 + i] = (uint8_t)(a >> (8 * i));
    }
}

static uint8_t *pool_alloc(size_t n) {
    if (g_pool == NULL) {
        void *p = NULL;
        if (sceKernelMmap(NULL, OVL_PAGE, PROT_RWX, 0x1002, -1, 0, &p) == 0 && p != NULL) {
            g_pool = (uint8_t *)p;
        } else if (sceKernelMprotect(g_static_pool, OVL_PAGE, PROT_RWX) == 0) {
            g_pool = g_static_pool;
        } else {
            ovl_log("no executable memory for trampolines");
            return NULL;
        }
        g_pool_cap = OVL_PAGE;
    }
    if (g_pool_used + n > g_pool_cap) {
        return NULL;
    }
    uint8_t *r = g_pool + g_pool_used;
    g_pool_used += (n + 15) & ~(size_t)15;
    return r;
}

static void log_bytes(const uint8_t *p) {
    static const char hex[] = "0123456789ABCDEF";
    char line[64];
    size_t n = 0;
    for (int i = 0; i < 16; i++) {
        line[n++] = hex[p[i] >> 4];
        line[n++] = hex[p[i] & 15];
        line[n++] = ' ';
    }
    line[n] = '\0';
    ovl_log(line);
}

ovl_status_t ovl_hook_install(void *target, void *detour, void **orig) {
    if (target == NULL || detour == NULL || orig == NULL) {
        return OVL_ERR_NULL;
    }
    uint8_t *t = (uint8_t *)target;

    /* If something already hooked this function, hook behind its jump. */
    for (int k = 0; k < 2; k++) {
        if (t[0] == 0xE9) {
            int32_t d = (int32_t)((uint32_t)t[1] | ((uint32_t)t[2] << 8) |
                                  ((uint32_t)t[3] << 16) | ((uint32_t)t[4] << 24));
            t = t + 5 + d;
        } else if (t[0] == 0xFF && t[1] == 0x25) {
            int32_t d = (int32_t)((uint32_t)t[2] | ((uint32_t)t[3] << 8) |
                                  ((uint32_t)t[4] << 16) | ((uint32_t)t[5] << 24));
            uint64_t a = 0;
            for (int i = 0; i < 8; i++) {
                a |= (uint64_t)t[6 + d + i] << (8 * i);
            }
            t = (uint8_t *)(uintptr_t)a;
        } else {
            break;
        }
    }

    int len = 0;
    while (len < ABS_JMP_LEN) {
        int l = insn_len(t + len);
        if (l <= 0 || l > 15) {
            ovl_log("hook refused: unsupported prologue bytes:");
            log_bytes(t);
            return OVL_ERR_IO;
        }
        len += l;
    }

    uint8_t *tr = pool_alloc((size_t)len + ABS_JMP_LEN);
    if (tr == NULL) {
        return OVL_ERR_FULL;
    }
    for (int i = 0; i < len; i++) {
        tr[i] = t[i];
    }
    write_abs_jmp(tr + len, t + len);

    uintptr_t pg = (uintptr_t)t & ~(uintptr_t)(OVL_PAGE - 1);
    size_t span = (((uintptr_t)t + ABS_JMP_LEN + OVL_PAGE - 1) & ~(uintptr_t)(OVL_PAGE - 1)) - pg;
    if (sceKernelMprotect((void *)pg, span, PROT_RWX) != 0) {
        ovl_log("mprotect RWX on target failed");
        return OVL_ERR_IO;
    }
    uint8_t patch[ABS_JMP_LEN];
    write_abs_jmp(patch, detour);
    volatile uint8_t *w = (volatile uint8_t *)t;
    for (int i = 0; i < ABS_JMP_LEN; i++) {
        w[i] = patch[i];
    }
    sceKernelMprotect((void *)pg, span, PROT_RX);

    *orig = tr;
    return OVL_OK;
}
