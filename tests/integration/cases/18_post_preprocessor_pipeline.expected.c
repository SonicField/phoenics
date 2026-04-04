#include <stdio.h>

/* Test: phc works correctly after cc -E (preprocessor).
 * Pipeline: clang -x c -E -P input.phc | build/phc | clang -x c - -o output */

typedef enum {
    Result_Ok,
    Result_Err,
    Result__COUNT
} Result_Tag;

typedef struct {
    int value;
} Result_Ok_t;

typedef struct {
    const char *msg;
} Result_Err_t;

typedef struct {
    Result_Tag tag;
    union {
        Result_Ok_t Ok;
        Result_Err_t Err;
    };
} Result;

static inline Result Result_mk_Ok(int value) {
    Result _v;
    _v.tag = Result_Ok;
    _v.Ok.value = value;
    return _v;
}

static inline Result Result_mk_Err(const char *msg) {
    Result _v;
    _v.tag = Result_Err;
    _v.Err.msg = msg;
    return _v;
}

static inline Result_Ok_t Result_as_Ok(Result v) {
    if (v.tag != Result_Ok) __builtin_trap();
    return v.Ok;
}

static inline Result_Err_t Result_as_Err(Result v) {
    if (v.tag != Result_Err) __builtin_trap();
    return v.Err;
}

int main(void) {
    Result r = Result_mk_Ok(42);

    /* Test accessor function (not macro) */
    Result_Ok_t ok = Result_as_Ok(r);
    printf("ok: %d\n", ok.value);

    /* Test exhaustive match */
    switch (r.tag) {
        case Result_Ok: { printf("match ok: %d\n", r.Ok.value); } break;
        case Result_Err: { printf("match err: %s\n", r.Err.msg); } break;
            default: break;
    }

    return 0;
}
