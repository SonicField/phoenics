#include <stdio.h>

typedef unsigned int HwReg;
#define HwReg_Enable (0x01u)
#define HwReg_Ready (0x02u)
#define HwReg_Error (0x80u)
#define HwReg__ALL (HwReg_Enable | HwReg_Ready | HwReg_Error)
#define HwReg__COUNT 3
#define HwReg__MAX_STRING 19

static inline int HwReg_has(HwReg flags, HwReg flag) {
    return (flags & flag) == flag;
}

static inline HwReg HwReg_set(HwReg flags, HwReg flag) {
    return flags | flag;
}

static inline HwReg HwReg_clear(HwReg flags, HwReg flag) {
    return flags & ~flag;
}

static inline const char *HwReg_to_string(HwReg p, char *buf, unsigned long len) {
    if (len == 0) return buf;
    char *pos = buf;
    char *end = buf + len - 1;
    if (p & HwReg_Enable) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Enable"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (p & HwReg_Ready) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Ready"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (p & HwReg_Error) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Error"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (pos == buf) {
        const char *s = "(none)"; while (*s && pos < end) *pos++ = *s++;
    }
    *pos = '\0';
    return buf;
}
#line 8
