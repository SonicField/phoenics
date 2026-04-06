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

void test_cancel(int do_cancel) {
    int *a = malloc(sizeof(int));
    int _phc_dg_1 = 1;
int *b = malloc(sizeof(int));
    int _phc_dg_2 = 1;
int *c = malloc(sizeof(int));
    int _phc_dg_3 = 1;
if (do_cancel) {
        _phc_dg_3 = 0; _phc_dg_2 = 0; _phc_dg_1 = 0; 

        printf("cancelled all\n");
        /* Must manually free since defers are cancelled */
        free(a); free(b); free(c);
    } else {
        printf("not cancelled\n");
    }
if (_phc_dg_3) {  free(c); printf("defer c\n"); }
if (_phc_dg_2) {  free(b); printf("defer b\n"); }
if (_phc_dg_1) {  free(a); printf("defer a\n"); }
}

int main(void) {
    printf("--- with cancel ---\n");
    test_cancel(1);
    printf("--- without cancel ---\n");
    test_cancel(0);
    return 0;
}
