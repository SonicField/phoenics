#include <stdio.h>

typedef enum {
    X_A,
    X_B,
    X__COUNT
} X_Tag;

typedef struct {
    X_Tag tag;
    union {
        struct { int v; } A;
        struct { int w; } B;
    };
} X;

static inline X X_mk_A(int v) {
    X _v;
    _v.tag = X_A;
    _v.A.v = v;
    return _v;
}

static inline X X_mk_B(int w) {
    X _v;
    _v.tag = X_B;
    _v.B.w = w;
    return _v;
}

#define X_as_A(v) \
    (assert((v).tag == X_A && "X: expected A"), \
     (v).A)

#define X_as_B(v) \
    (assert((v).tag == X_B && "X: expected B"), \
     (v).B)


int main(void) {
    X x = X_mk_A(42);
    switch (x.tag) { case X_A: { printf("%d\n", x.A.v); } break; case X_B: { printf("%d\n", x.B.w); } break;  default: break; }
    return 0;
}
