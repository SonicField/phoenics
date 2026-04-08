#ifndef abort
extern void abort(void);
#endif
#define phc_free(pp) do { free(*(pp)); *(pp) = ((void*)0); } while(0)
#include <stdio.h>

typedef enum {
    Color_Red,
    Color_Green,
    Color_Blue,
    Color__COUNT
} Color_Tag;

typedef struct {
    char _empty;
} Color_Red_t;

typedef struct {
    char _empty;
} Color_Green_t;

typedef struct {
    char _empty;
} Color_Blue_t;

typedef struct Color {
    Color_Tag tag;
    union {
        Color_Red_t Red;
        Color_Green_t Green;
        Color_Blue_t Blue;
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

static inline Color_Red_t Color_as_Red(Color v) {
    if (v.tag != Color_Red) abort();
    return v.Red;
}

static inline Color_Green_t Color_as_Green(Color v) {
    if (v.tag != Color_Green) abort();
    return v.Green;
}

static inline Color_Blue_t Color_as_Blue(Color v) {
    if (v.tag != Color_Blue) abort();
    return v.Blue;
}
#line 8

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
