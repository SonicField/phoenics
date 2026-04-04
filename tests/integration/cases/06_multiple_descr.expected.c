#include <assert.h>
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

typedef struct {
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
    assert(v.tag == Color_Red && "Color: expected Red");
    return v.Red;
}

static inline Color_Green_t Color_as_Green(Color v) {
    assert(v.tag == Color_Green && "Color: expected Green");
    return v.Green;
}

static inline Color_Blue_t Color_as_Blue(Color v) {
    assert(v.tag == Color_Blue && "Color: expected Blue");
    return v.Blue;
}

typedef enum {
    Value_Integer,
    Value_Float,
    Value_String,
    Value__COUNT
} Value_Tag;

typedef struct {
    int n;
} Value_Integer_t;

typedef struct {
    double f;
} Value_Float_t;

typedef struct {
    const char *s;
} Value_String_t;

typedef struct {
    Value_Tag tag;
    union {
        Value_Integer_t Integer;
        Value_Float_t Float;
        Value_String_t String;
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

static inline Value_Integer_t Value_as_Integer(Value v) {
    assert(v.tag == Value_Integer && "Value: expected Integer");
    return v.Integer;
}

static inline Value_Float_t Value_as_Float(Value v) {
    assert(v.tag == Value_Float && "Value: expected Float");
    return v.Float;
}

static inline Value_String_t Value_as_String(Value v) {
    assert(v.tag == Value_String && "Value: expected String");
    return v.String;
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
