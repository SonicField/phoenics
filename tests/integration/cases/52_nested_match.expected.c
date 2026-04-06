extern void abort(void);
#define phc_free(pp) do { free(*(pp)); *(pp) = ((void*)0); } while(0)
#include <stdio.h>

typedef enum {
    Outer_A,
    Outer_B,
    Outer__COUNT
} Outer_Tag;

typedef struct {
    int x;
} Outer_A_t;

typedef struct {
    int y;
} Outer_B_t;

typedef struct Outer {
    Outer_Tag tag;
    union {
        Outer_A_t A;
        Outer_B_t B;
    };
} Outer;

static inline Outer Outer_mk_A(int x) {
    Outer _v;
    _v.tag = Outer_A;
    _v.A.x = x;
    return _v;
}

static inline Outer Outer_mk_B(int y) {
    Outer _v;
    _v.tag = Outer_B;
    _v.B.y = y;
    return _v;
}

static inline Outer_A_t Outer_as_A(Outer v) {
    if (v.tag != Outer_A) abort();
    return v.A;
}

static inline Outer_B_t Outer_as_B(Outer v) {
    if (v.tag != Outer_B) abort();
    return v.B;
}
#line 7

typedef enum {
    Inner_P,
    Inner_Q,
    Inner__COUNT
} Inner_Tag;

typedef struct {
    int p;
} Inner_P_t;

typedef struct {
    int q;
} Inner_Q_t;

typedef struct Inner {
    Inner_Tag tag;
    union {
        Inner_P_t P;
        Inner_Q_t Q;
    };
} Inner;

static inline Inner Inner_mk_P(int p) {
    Inner _v;
    _v.tag = Inner_P;
    _v.P.p = p;
    return _v;
}

static inline Inner Inner_mk_Q(int q) {
    Inner _v;
    _v.tag = Inner_Q;
    _v.Q.q = q;
    return _v;
}

static inline Inner_P_t Inner_as_P(Inner v) {
    if (v.tag != Inner_P) abort();
    return v.P;
}

static inline Inner_Q_t Inner_as_Q(Inner v) {
    if (v.tag != Inner_Q) abort();
    return v.Q;
}
#line 12

void process(Outer o, Inner i) {
    switch (o.tag) {
    case Outer_A: { int x = o.A.x; {
            switch (i.tag) {
    case Inner_P: { int p = i.P.p; {
                    printf("A.P: %d %d\n", x, p);
                } break; }
    case Inner_Q: { int q = i.Q.q; {
                    printf("A.Q: %d %d\n", x, q);
                } break; }
    default: break;
}
        } break; }
    case Outer_B: { int y = o.B.y; {
            printf("B: %d\n", y);
        } break; }
    default: break;
}
#line 29
}

int main(void) {
    Outer a = Outer_mk_A(10);
    Inner p = Inner_mk_P(20);
    Inner q = Inner_mk_Q(30);
    Outer b = Outer_mk_B(99);

    process(a, p);
    process(a, q);
    process(b, p);
    return 0;
}
