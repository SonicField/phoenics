#include <stdio.h>

typedef unsigned int Mode;
#define Mode_Verbose (1u << 0)
#define Mode_Debug (1u << 1)
#define Mode_Strict (1u << 2)
#define Mode__ALL (Mode_Verbose | Mode_Debug | Mode_Strict)
#define Mode__COUNT 3
#define Mode__MAX_STRING 21

static inline int Mode_has(Mode flags, Mode flag) {
    return (flags & flag) == flag;
}

static inline Mode Mode_set(Mode flags, Mode flag) {
    return flags | flag;
}

static inline Mode Mode_clear(Mode flags, Mode flag) {
    return flags & ~flag;
}

static inline const char *Mode_to_string(Mode p, char *buf, unsigned long len) {
    if (len == 0) return buf;
    char *pos = buf;
    char *end = buf + len - 1;
    if (p & Mode_Verbose) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Verbose"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (p & Mode_Debug) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Debug"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (p & Mode_Strict) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Strict"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (pos == buf) {
        const char *s = "(none)"; while (*s && pos < end) *pos++ = *s++;
    }
    *pos = '\0';
    return buf;
}
#line 8

int main(void) {
    Mode m = 0;
    m = Mode_set(m, Mode_Verbose);
    m = Mode_set(m, Mode_Debug);
    char buf[64];
    printf("%s\n", Mode_to_string(m, buf, sizeof(buf)));

    m = Mode_clear(m, Mode_Verbose);
    printf("%s\n", Mode_to_string(m, buf, sizeof(buf)));

    printf("all: 0x%x\n", Mode__ALL);
    return 0;
}
