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
    if (len == 0) return buf;
    char *pos = buf;
    char *end = buf + len - 1;
    if (p & Color_Red) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Red"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (p & Color_Green) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Green"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (p & Color_Blue) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Blue"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (pos == buf) {
        const char *s = "(none)"; while (*s && pos < end) *pos++ = *s++;
    }
    *pos = '\0';
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
