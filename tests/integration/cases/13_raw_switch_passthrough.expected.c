#include <stdio.h>

typedef enum {
    Color_Red,
    Color_Green,
    Color_Blue,
    Color__COUNT
} Color_Tag;

typedef struct {
    Color_Tag tag;
    union {
        struct { char _empty; } Red;
        struct { char _empty; } Green;
        struct { char _empty; } Blue;
    };
} Color;

static inline Color Color_mk_Red(void) {
    Color _v;
    _v.tag = Color_Red;
    return _v;
}

static inline Color Color_mk_Green(void) {
    Color _v;
    _v.tag = Color_Green;
    return _v;
}

static inline Color Color_mk_Blue(void) {
    Color _v;
    _v.tag = Color_Blue;
    return _v;
}

/* Raw switch on phc_descr tag — NOT phc_match.
 * PHC must NOT enforce exhaustiveness here.
 * The switch passes through unchanged. */
int main(void) {
    Color c = Color_mk_Green();
    switch (c.tag) {
    case Color_Red:   printf("red\n"); break;
    case Color_Green: printf("green\n"); break;
    default: printf("other\n"); break;
    }
    return 0;
}
