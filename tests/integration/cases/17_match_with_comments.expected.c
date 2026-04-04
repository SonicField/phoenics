#include <stdlib.h>
#include <stdio.h>

typedef enum {
    Val_Int,
    Val_Str,
    Val__COUNT
} Val_Tag;

typedef struct {
    int n;
} Val_Int_t;

typedef struct {
    const char *s;
} Val_Str_t;

typedef struct {
    Val_Tag tag;
    union {
        Val_Int_t Int;
        Val_Str_t Str;
    };
} Val;

static inline Val Val_mk_Int(int n) {
    Val _v;
    _v.tag = Val_Int;
    _v.Int.n = n;
    return _v;
}

static inline Val Val_mk_Str(const char *s) {
    Val _v;
    _v.tag = Val_Str;
    _v.Str.s = s;
    return _v;
}

static inline Val_Int_t Val_as_Int(Val v) {
    if (v.tag != Val_Int) abort();
    return v.Int;
}

static inline Val_Str_t Val_as_Str(Val v) {
    if (v.tag != Val_Str) abort();
    return v.Str;
}

int main(void) {
    Val v = Val_mk_Int(99);
    switch (v.tag) {
        /* Handle integer values */
        case Val_Int: {
            printf("int: %d\n", v.Int.n);
        } break;
        // Handle string values
        case Val_Str: {
            printf("str: %s\n", v.Str.s);
        } break;
            default: break;
    }
    return 0;
}
