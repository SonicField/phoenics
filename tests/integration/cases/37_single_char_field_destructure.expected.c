#ifndef abort
extern void abort(void);
#endif
#define phc_free(pp) do { free(*(pp)); *(pp) = ((void*)0); } while(0)
#include <stdio.h>

typedef enum {
    Pair_I,
    Pair_C,
    Pair_P,
    Pair__COUNT
} Pair_Tag;

typedef struct {
    int i;
} Pair_I_t;

typedef struct {
    char c;
} Pair_C_t;

typedef struct {
    double *p;
} Pair_P_t;

typedef struct Pair {
    Pair_Tag tag;
    union {
        Pair_I_t I;
        Pair_C_t C;
        Pair_P_t P;
    };
} Pair;

static inline Pair Pair_mk_I(int i) {
    Pair _v;
    _v.tag = Pair_I;
    _v.I.i = i;
    return _v;
}

static inline Pair Pair_mk_C(char c) {
    Pair _v;
    _v.tag = Pair_C;
    _v.C.c = c;
    return _v;
}

static inline Pair Pair_mk_P(double *p) {
    Pair _v;
    _v.tag = Pair_P;
    _v.P.p = p;
    return _v;
}

static inline Pair_I_t Pair_as_I(Pair v) {
    if (v.tag != Pair_I) abort();
    return v.I;
}

static inline Pair_C_t Pair_as_C(Pair v) {
    if (v.tag != Pair_C) abort();
    return v.C;
}

static inline Pair_P_t Pair_as_P(Pair v) {
    if (v.tag != Pair_P) abort();
    return v.P;
}
#line 8

int main(void) {
    Pair a = Pair_mk_I(99);
    switch (a.tag) {
    case Pair_I: { int i = a.I.i; { printf("i=%d\n", i); } break; }
    case Pair_C: { char c = a.C.c; { printf("c=%c\n", c); } break; }
    case Pair_P: { printf("p\n"); } break;
    default: break;
}
#line 16
    return 0;
}
