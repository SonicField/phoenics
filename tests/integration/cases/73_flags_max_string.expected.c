#include <stdio.h>

typedef unsigned int Opts;
#define Opts_Alpha (1u << 0)
#define Opts_Beta (1u << 1)
#define Opts_Gamma (1u << 2)
#define Opts__ALL (Opts_Alpha | Opts_Beta | Opts_Gamma)
#define Opts__COUNT 3
#define Opts__MAX_STRING 17

static inline int Opts_has(Opts flags, Opts flag) {
    return (flags & flag) == flag;
}

static inline Opts Opts_set(Opts flags, Opts flag) {
    return flags | flag;
}

static inline Opts Opts_clear(Opts flags, Opts flag) {
    return flags & ~flag;
}

static inline const char *Opts_to_string(Opts p, char *buf, unsigned long len) {
    if (len == 0) return buf;
    char *pos = buf;
    char *end = buf + len - 1;
    if (p & Opts_Alpha) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Alpha"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (p & Opts_Beta) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Beta"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (p & Opts_Gamma) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Gamma"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (pos == buf) {
        const char *s = "(none)"; while (*s && pos < end) *pos++ = *s++;
    }
    *pos = '\0';
    return buf;
}
#line 8

int main(void) {
    /* __MAX_STRING is large enough for all flag names joined by '|' */
    char buf[Opts__MAX_STRING];
    Opts all = Opts__ALL;
    printf("%s\n", Opts_to_string(all, buf, sizeof(buf)));
    return 0;
}
