extern void abort(void);
#include <stdio.h>

typedef enum {
    Value_IntPtr,
    Value_ConstStr,
    Value_BigNum,
    Value_Pair,
    Value__COUNT
} Value_Tag;

typedef struct {
    int * *pp;
} Value_IntPtr_t;

typedef struct {
    const char *s;
} Value_ConstStr_t;

typedef struct {
    unsigned long long n;
} Value_BigNum_t;

typedef struct {
    int x;
    int y;
} Value_Pair_t;

typedef struct {
    Value_Tag tag;
    union {
        Value_IntPtr_t IntPtr;
        Value_ConstStr_t ConstStr;
        Value_BigNum_t BigNum;
        Value_Pair_t Pair;
    };
} Value;

static inline Value Value_mk_IntPtr(int * *pp) {
    Value _v;
    _v.tag = Value_IntPtr;
    _v.IntPtr.pp = pp;
    return _v;
}

static inline Value Value_mk_ConstStr(const char *s) {
    Value _v;
    _v.tag = Value_ConstStr;
    _v.ConstStr.s = s;
    return _v;
}

static inline Value Value_mk_BigNum(unsigned long long n) {
    Value _v;
    _v.tag = Value_BigNum;
    _v.BigNum.n = n;
    return _v;
}

static inline Value Value_mk_Pair(int x, int y) {
    Value _v;
    _v.tag = Value_Pair;
    _v.Pair.x = x;
    _v.Pair.y = y;
    return _v;
}

static inline Value_IntPtr_t Value_as_IntPtr(Value v) {
    if (v.tag != Value_IntPtr) abort();
    return v.IntPtr;
}

static inline Value_ConstStr_t Value_as_ConstStr(Value v) {
    if (v.tag != Value_ConstStr) abort();
    return v.ConstStr;
}

static inline Value_BigNum_t Value_as_BigNum(Value v) {
    if (v.tag != Value_BigNum) abort();
    return v.BigNum;
}

static inline Value_Pair_t Value_as_Pair(Value v) {
    if (v.tag != Value_Pair) abort();
    return v.Pair;
}
#line 9

int main(void) {
    /* Test pointer-to-pointer */
    int val = 42;
    int *p = &val;
    Value v1 = Value_mk_IntPtr(&p);
    switch (v1.tag) {
    case Value_IntPtr: { printf("intptr: %d\n", **v1.IntPtr.pp); } break;
    case Value_ConstStr: { printf("str: %s\n", v1.ConstStr.s); } break;
    case Value_BigNum: { printf("big: %llu\n", v1.BigNum.n); } break;
    case Value_Pair: { printf("pair: %d,%d\n", v1.Pair.x, v1.Pair.y); } break;
    default: break;
}
#line 22

    /* Test const char * */
    Value v2 = Value_mk_ConstStr("hello");
    switch (v2.tag) {
    case Value_IntPtr: { printf("intptr\n"); } break;
    case Value_ConstStr: { printf("str: %s\n", v2.ConstStr.s); } break;
    case Value_BigNum: { printf("big\n"); } break;
    case Value_Pair: { printf("pair\n"); } break;
    default: break;
}
#line 31

    /* Test unsigned long long */
    Value v3 = Value_mk_BigNum(18446744073709551615ULL);
    switch (v3.tag) {
    case Value_IntPtr: { printf("intptr\n"); } break;
    case Value_ConstStr: { printf("str\n"); } break;
    case Value_BigNum: { printf("big: %llu\n", v3.BigNum.n); } break;
    case Value_Pair: { printf("pair\n"); } break;
    default: break;
}
#line 40

    /* Test accessor on correct variant */
    Value_Pair_t pair = Value_as_Pair(Value_mk_Pair(10, 20));
    printf("pair accessor: %d,%d\n", pair.x, pair.y);

    return 0;
}
