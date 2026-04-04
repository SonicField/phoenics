extern void abort(void);
#include <stdio.h>

typedef enum {
    Result_Ok,
    Result_Err,
    Result__COUNT
} Result_Tag;

typedef struct {
    int value;
} Result_Ok_t;

typedef struct {
    int code;
    const char *message;
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

static inline Result Result_mk_Err(int code, const char *message) {
    Result _v;
    _v.tag = Result_Err;
    _v.Err.code = code;
    _v.Err.message = message;
    return _v;
}

static inline Result_Ok_t Result_as_Ok(Result v) {
    if (v.tag != Result_Ok) abort();
    return v.Ok;
}

static inline Result_Err_t Result_as_Err(Result v) {
    if (v.tag != Result_Err) abort();
    return v.Err;
}
#line 6


int main(void) {
    Result r = Result_mk_Ok(42);
    switch (r.tag) {
        case Result_Ok: {
            printf("ok: %d\n", r.Ok.value);
        } break;
        case Result_Err: {
            printf("err %d: %s\n", r.Err.code, r.Err.message);
        } break;
            default: break;
    }
#line 18

    return 0;
}
