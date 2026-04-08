#ifndef strcmp
extern int strcmp(const char *, const char *);
#endif
#include <stdio.h>
#include <string.h>

typedef enum {
    Color_Red,
    Color_Green,
    Color_Blue,
    Color__COUNT = 3
} Color;

static inline const char *Color_to_string(Color c) {
    switch (c) {
        case Color_Red: return "Red";
        case Color_Green: return "Green";
        case Color_Blue: return "Blue";
        default: return "(unknown)";
    }
}

static inline int Color_from_string(const char *s, Color *out) {
    if (strcmp(s, "Red") == 0) { *out = Color_Red; return 1; }
    if (strcmp(s, "Green") == 0) { *out = Color_Green; return 1; }
    if (strcmp(s, "Blue") == 0) { *out = Color_Blue; return 1; }
    return 0;
}
#line 9

int main(void) {
    Color c = Color_Green;
    printf("%s\n", Color_to_string(c));

    Color parsed;
    if (Color_from_string("Blue", &parsed)) {
        printf("%s\n", Color_to_string(parsed));
    }
    return 0;
}
