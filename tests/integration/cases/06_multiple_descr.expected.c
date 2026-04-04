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

#define Color_as_Red(v) \
    (assert((v).tag == Color_Red && "Color: expected Red"), \
     (v).Red)

#define Color_as_Green(v) \
    (assert((v).tag == Color_Green && "Color: expected Green"), \
     (v).Green)

#define Color_as_Blue(v) \
    (assert((v).tag == Color_Blue && "Color: expected Blue"), \
     (v).Blue)


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

#define Value_as_Integer(v) \
    (assert((v).tag == Value_Integer && "Value: expected Integer"), \
     (v).Integer)

#define Value_as_Float(v) \
    (assert((v).tag == Value_Float && "Value: expected Float"), \
     (v).Float)

#define Value_as_String(v) \
    (assert((v).tag == Value_String && "Value: expected String"), \
     (v).String)


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
