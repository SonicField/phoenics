extern void abort(void);
#include <stdio.h>

typedef enum {
    Expr_Num,
    Expr_Add,
    Expr_Neg,
    Expr__COUNT
} Expr_Tag;

typedef struct {
    int value;
} Expr_Num_t;

typedef struct {
    int left;
    int right;
} Expr_Add_t;

typedef struct {
    int inner;
} Expr_Neg_t;

typedef struct {
    Expr_Tag tag;
    union {
        Expr_Num_t Num;
        Expr_Add_t Add;
        Expr_Neg_t Neg;
    };
} Expr;

static inline Expr Expr_mk_Num(int value) {
    Expr _v;
    _v.tag = Expr_Num;
    _v.Num.value = value;
    return _v;
}

static inline Expr Expr_mk_Add(int left, int right) {
    Expr _v;
    _v.tag = Expr_Add;
    _v.Add.left = left;
    _v.Add.right = right;
    return _v;
}

static inline Expr Expr_mk_Neg(int inner) {
    Expr _v;
    _v.tag = Expr_Neg;
    _v.Neg.inner = inner;
    return _v;
}

static inline Expr_Num_t Expr_as_Num(Expr v) {
    if (v.tag != Expr_Num) abort();
    return v.Num;
}

static inline Expr_Add_t Expr_as_Add(Expr v) {
    if (v.tag != Expr_Add) abort();
    return v.Add;
}

static inline Expr_Neg_t Expr_as_Neg(Expr v) {
    if (v.tag != Expr_Neg) abort();
    return v.Neg;
}
#line 8

int main(void) {
    Expr e = Expr_mk_Add(3, 4);
    switch (e.tag) {
    case Expr_Num: { int value = e.Num.value; {
            printf("num %d\n", value);
        } break; }
    case Expr_Add: { int left = e.Add.left; int right = e.Add.right; {
            printf("add %d+%d=%d\n", left, right, left + right);
        } break; }
    case Expr_Neg: {
            printf("neg (no destructure)\n");
        } break;
    default: break;
}
#line 22
    return 0;
}
