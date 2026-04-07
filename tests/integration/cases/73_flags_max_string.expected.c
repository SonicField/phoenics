extern int snprintf(char *, unsigned long, const char *, ...);
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
    buf[0] = '\0';
    unsigned long pos = 0;
    if (p & Opts_Alpha) { pos += snprintf(buf + pos, len - pos, "%sAlpha", pos ? "|" : ""); }
    if (p & Opts_Beta) { pos += snprintf(buf + pos, len - pos, "%sBeta", pos ? "|" : ""); }
    if (p & Opts_Gamma) { pos += snprintf(buf + pos, len - pos, "%sGamma", pos ? "|" : ""); }
    if (pos == 0) { snprintf(buf, len, "(none)"); }
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
