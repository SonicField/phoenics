#ifndef abort
extern void abort(void);
#endif
#define phc_free(pp) do { free(*(pp)); *(pp) = ((void*)0); } while(0)
#include <stdio.h>

typedef enum {
    Wrapper_Value,
    Wrapper__COUNT
} Wrapper_Tag;

typedef struct {
    int x;
} Wrapper_Value_t;

typedef struct Wrapper {
    Wrapper_Tag tag;
    union {
        Wrapper_Value_t Value;
    };
} Wrapper;

static inline Wrapper Wrapper_mk_Value(int x) {
    Wrapper _v;
    _v.tag = Wrapper_Value;
    _v.Value.x = x;
    return _v;
}

static inline Wrapper_Value_t Wrapper_as_Value(Wrapper v) {
    if (v.tag != Wrapper_Value) abort();
    return v.Value;
}
#line 6

int main(void) {
    Wrapper w = Wrapper_mk_Value(42);
    switch (w.tag) {
    case Wrapper_Value: { int x = w.Value.x; {
            printf("value: %d\n", x);
        } break; }
    default: break;
}
#line 14
    return 0;
}
