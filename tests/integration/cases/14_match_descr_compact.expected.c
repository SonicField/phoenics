extern void abort(void);
#include <stdio.h>

typedef enum {
    X_A,
    X_B,
    X__COUNT
} X_Tag;

typedef struct {
    int v;
} X_A_t;

typedef struct {
    int w;
} X_B_t;

typedef struct {
    X_Tag tag;
    union {
        X_A_t A;
        X_B_t B;
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

static inline X_A_t X_as_A(X v) {
    if (v.tag != X_A) abort();
    return v.A;
}

static inline X_B_t X_as_B(X v) {
    if (v.tag != X_B) abort();
    return v.B;
}

int main(void) {
    X x = X_mk_A(42);
    switch (x.tag) { case X_A: { printf("%d\n", x.A.v); } break; case X_B: { printf("%d\n", x.B.w); } break;  default: break; }
    return 0;
}
