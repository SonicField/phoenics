extern void abort(void);
#define phc_free(pp) do { free(*(pp)); *(pp) = ((void*)0); } while(0)
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    Dummy_X,
    Dummy__COUNT
} Dummy_Tag;

typedef struct {
    char _empty;
} Dummy_X_t;

typedef struct Dummy {
    Dummy_Tag tag;
    union {
        Dummy_X_t X;
    };
} Dummy;

static inline Dummy Dummy_mk_X(void) {
    Dummy _v;
    _v.tag = Dummy_X;
    return _v;
}

static inline Dummy_X_t Dummy_as_X(Dummy v) {
    if (v.tag != Dummy_X) abort();
    return v.X;
}
#line 5

int main(void) {
    char *p = malloc(10);
    printf("before: %s\n", p ? "not null" : "null");
    phc_free((void**)&p);
    printf("after: %s\n", p ? "not null" : "null");

    /* Double call — must not crash */
    phc_free((void**)&p);
    printf("double: %s\n", p ? "not null" : "null");

    /* NULL pointer — must not crash */
    char *q = NULL;
    phc_free((void**)&q);
    printf("null safe: %s\n", q ? "not null" : "null");

    return 0;
}
