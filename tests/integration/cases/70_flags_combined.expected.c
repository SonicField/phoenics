extern int snprintf(char *, unsigned long, const char *, ...);
#include <stdio.h>

typedef unsigned int Color;
#define Color_Red (1u << 0)
#define Color_Green (1u << 1)
#define Color_Blue (1u << 2)
#define Color__ALL (Color_Red | Color_Green | Color_Blue)
#define Color__COUNT 3
#define Color__MAX_STRING 15

static inline int Color_has(Color flags, Color flag) {
    return (flags & flag) == flag;
}

static inline Color Color_set(Color flags, Color flag) {
    return flags | flag;
}

static inline Color Color_clear(Color flags, Color flag) {
    return flags & ~flag;
}

static inline const char *Color_to_string(Color p, char *buf, unsigned long len) {
    buf[0] = '\0';
    unsigned long pos = 0;
    if (p & Color_Red) { pos += snprintf(buf + pos, len - pos, "%sRed", pos ? "|" : ""); }
    if (p & Color_Green) { pos += snprintf(buf + pos, len - pos, "%sGreen", pos ? "|" : ""); }
    if (p & Color_Blue) { pos += snprintf(buf + pos, len - pos, "%sBlue", pos ? "|" : ""); }
    if (pos == 0) { snprintf(buf, len, "(none)"); }
    return buf;
}
#line 8

int main(void) {
    Color white = Color__ALL;
    Color magenta = Color_Red | Color_Blue;
    char buf[64];

    printf("white: %s\n", Color_to_string(white, buf, sizeof(buf)));
    printf("magenta: %s\n", Color_to_string(magenta, buf, sizeof(buf)));
    printf("count: %d\n", Color__COUNT);
    return 0;
}
