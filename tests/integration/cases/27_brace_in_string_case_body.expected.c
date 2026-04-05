extern void abort(void);
#include <stdio.h>

typedef enum {
    Val_A,
    Val_B,
    Val__COUNT
} Val_Tag;

typedef struct {
    int x;
} Val_A_t;

typedef struct {
    int y;
} Val_B_t;

typedef struct Val {
    Val_Tag tag;
    union {
        Val_A_t A;
        Val_B_t B;
    };
} Val;

static inline Val Val_mk_A(int x) {
    Val _v;
    _v.tag = Val_A;
    _v.A.x = x;
    return _v;
}

static inline Val Val_mk_B(int y) {
    Val _v;
    _v.tag = Val_B;
    _v.B.y = y;
    return _v;
}

static inline Val_A_t Val_as_A(Val v) {
    if (v.tag != Val_A) abort();
    return v.A;
}

static inline Val_B_t Val_as_B(Val v) {
    if (v.tag != Val_B) abort();
    return v.B;
}
#line 7

int main(void) {
    Val v = Val_mk_A(42);
    switch (v.tag) {
    case Val_A: {
            printf("brace in string: }\n");
            printf("another: { }\n");
        } break;
    case Val_B: {
            printf("B\n");
        } break;
    default: break;
}
#line 19
    return 0;
}
