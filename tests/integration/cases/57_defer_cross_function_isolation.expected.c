extern void abort(void);
#define phc_free(pp) do { free(*(pp)); *(pp) = ((void*)0); } while(0)
#include <stdio.h>

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
#line 4

int first(void) {
    printf("first body\n");
    {  printf("first cleanup\n"); return 1; }
 printf("first cleanup\n");
}

int second(void) {
    /* NO phc_defer here — return must NOT be rewritten with cleanup */
    printf("second body\n");
    return 2;
}

int third(void) {
    printf("third body\n");
    {  printf("third cleanup\n"); return 3; }
 printf("third cleanup\n");
}

int main(void) {
    printf("ret=%d\n", first());
    printf("ret=%d\n", second());
    printf("ret=%d\n", third());
    return 0;
}
