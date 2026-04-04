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

typedef enum {
    Value_Integer,
    Value_Float,
    Value_String,
    Value__COUNT
} Value_Tag;

typedef struct {
    Value_Tag tag;
    union {
        struct { int n; } Integer;
        struct { double f; } Float;
        struct { const char *s; } String;
    };
} Value;

static inline Value Value_mk_Integer(int n) {
    Value _v;
    _v.tag = Value_Integer;
    _v.Integer.n = n;
    return _v;
}

static inline Value Value_mk_Float(double f) {
    Value _v;
    _v.tag = Value_Float;
    _v.Float.f = f;
    return _v;
}

static inline Value Value_mk_String(const char *s) {
    Value _v;
    _v.tag = Value_String;
    _v.String.s = s;
    return _v;
}

int main(void) {
    Color c = Color_mk_Red();
    Value v = Value_mk_Integer(42);
    switch (c.tag) {
        case Color_Red:   { printf("red\n"); } break;
        case Color_Green: { printf("green\n"); } break;
        case Color_Blue:  { printf("blue\n"); } break;
            default: break;
    }
    switch (v.tag) {
        case Value_Integer: { printf("int: %d\n", v.Integer.n); } break;
        case Value_Float:   { printf("float: %f\n", v.Float.f); } break;
        case Value_String:  { printf("string: %s\n", v.String.s); } break;
            default: break;
    }
    return 0;
}
