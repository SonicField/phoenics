#include <stdio.h>
#include <stdlib.h>

typedef unsigned int Options;
#define Options_Fast (1u << 0)
#define Options_Safe (1u << 1)
#define Options_Quiet (1u << 2)
#define Options__ALL (Options_Fast | Options_Safe | Options_Quiet)
#define Options__COUNT 3
#define Options__MAX_STRING 16

static inline int Options_has(Options flags, Options flag) {
    return (flags & flag) == flag;
}

static inline Options Options_set(Options flags, Options flag) {
    return flags | flag;
}

static inline Options Options_clear(Options flags, Options flag) {
    return flags & ~flag;
}

static inline const char *Options_to_string(Options p, char *buf, unsigned long len) {
    if (len == 0) return buf;
    char *pos = buf;
    char *end = buf + len - 1;
    if (p & Options_Fast) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Fast"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (p & Options_Safe) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Safe"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (p & Options_Quiet) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Quiet"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (pos == buf) {
        const char *s = "(none)"; while (*s && pos < end) *pos++ = *s++;
    }
    *pos = '\0';
    return buf;
}
#line 9

const char *describe(Options opts) {
    char *buf = malloc(128);
    Options_to_string(opts, buf, 128);
    if (Options_has(opts, Options_Fast)) {
        {  free(buf); return "fast mode"; }
    }
    {  free(buf); return "normal mode"; }
 free(buf);
}

int main(void) {
    Options o = Options_Fast | Options_Safe;
    printf("%s\n", describe(o));
    return 0;
}
