extern int snprintf(char *, unsigned long, const char *, ...);
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
    buf[0] = '\0';
    unsigned long pos = 0;
    if (p & Options_Fast) { pos += snprintf(buf + pos, len - pos, "%sFast", pos ? "|" : ""); }
    if (p & Options_Safe) { pos += snprintf(buf + pos, len - pos, "%sSafe", pos ? "|" : ""); }
    if (p & Options_Quiet) { pos += snprintf(buf + pos, len - pos, "%sQuiet", pos ? "|" : ""); }
    if (pos == 0) { snprintf(buf, len, "(none)"); }
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
