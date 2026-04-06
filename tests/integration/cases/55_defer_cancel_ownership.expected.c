extern void abort(void);
#define phc_free(pp) do { free(*(pp)); *(pp) = ((void*)0); } while(0)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#line 6

char *make_string(int fail) {
    char *result = malloc(64);
    strcpy(result, "hello");
    int _phc_dg_1 = 1;
if (fail) {
        printf("error path\n");
        { if (_phc_dg_1) {  free(result); printf("defer fired\n"); } return NULL; }  /* defer fires: free(result) */
    }

    _phc_dg_1 = 0; 

    printf("success path\n");
    { if (_phc_dg_1) {  free(result); printf("defer fired\n"); } return result; }  /* defer does NOT fire: ownership transferred */
if (_phc_dg_1) {  free(result); printf("defer fired\n"); }
}

int main(void) {
    /* Error path — defer fires */
    char *r1 = make_string(1);
    printf("r1=%s\n", r1 ? r1 : "null");

    /* Success path — defer cancelled, ownership transferred */
    char *r2 = make_string(0);
    printf("r2=%s\n", r2);
    free(r2);

    return 0;
}
