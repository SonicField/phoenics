extern void abort(void);
#define phc_free(pp) do { free(*(pp)); *(pp) = ((void*)0); } while(0)
#include <stdio.h>

typedef enum {
    Handler_Callback,
    Handler_Direct,
    Handler__COUNT
} Handler_Tag;

typedef struct {
    void (*func)(int);
} Handler_Callback_t;

typedef struct {
    int value;
} Handler_Direct_t;

typedef struct Handler {
    Handler_Tag tag;
    union {
        Handler_Callback_t Callback;
        Handler_Direct_t Direct;
    };
} Handler;

static inline Handler Handler_mk_Callback(void (*func)(int)) {
    Handler _v;
    _v.tag = Handler_Callback;
    _v.Callback.func = func;
    return _v;
}

static inline Handler Handler_mk_Direct(int value) {
    Handler _v;
    _v.tag = Handler_Direct;
    _v.Direct.value = value;
    return _v;
}

static inline Handler_Callback_t Handler_as_Callback(Handler v) {
    if (v.tag != Handler_Callback) abort();
    return v.Callback;
}

static inline Handler_Direct_t Handler_as_Direct(Handler v) {
    if (v.tag != Handler_Direct) abort();
    return v.Direct;
}
#line 7

static void print_it(int x) { printf("called: %d\n", x); }

int main(void) {
    Handler h = Handler_mk_Callback(print_it);
    switch (h.tag) {
    case Handler_Callback: {
            h.Callback.func(42);
        } break;
    case Handler_Direct: {
            printf("direct: %d\n", h.Direct.value);
        } break;
    default: break;
}
#line 20
    return 0;
}
