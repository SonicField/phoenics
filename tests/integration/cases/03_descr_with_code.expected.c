extern void abort(void);
#include <stdio.h>

typedef enum {
    Option_int_Some,
    Option_int_None,
    Option_int__COUNT
} Option_int_Tag;

typedef struct {
    int value;
} Option_int_Some_t;

typedef struct {
    char _empty;
} Option_int_None_t;

typedef struct {
    Option_int_Tag tag;
    union {
        Option_int_Some_t Some;
        Option_int_None_t None;
    };
} Option_int;

static inline Option_int Option_int_mk_Some(int value) {
    Option_int _v;
    _v.tag = Option_int_Some;
    _v.Some.value = value;
    return _v;
}

static inline Option_int Option_int_mk_None(void) {
    Option_int _v;
    _v.tag = Option_int_None;
    return _v;
}

static inline Option_int_Some_t Option_int_as_Some(Option_int v) {
    if (v.tag != Option_int_Some) abort();
    return v.Some;
}

static inline Option_int_None_t Option_int_as_None(Option_int v) {
    if (v.tag != Option_int_None) abort();
    return v.None;
}

int main(void) {
    Option_int x = Option_int_mk_Some(42);
    switch (x.tag) {
        case Option_int_Some: {
            printf("got %d\n", x.Some.value);
        } break;
        case Option_int_None: {
            printf("nothing\n");
        } break;
            default: break;
    }
    return 0;
}
