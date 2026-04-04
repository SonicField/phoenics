#include <stdio.h>

typedef enum {
    Result_Ok,
    Result_Err
} Result_Tag;

typedef struct {
    Result_Tag tag;
    union {
        struct { int value; } Ok;
        struct { int code; const char *message; } Err;
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

int main(void) {
    Result r = Result_mk_Ok(42);
    switch (r.tag) {
    case Result_Ok:
        printf("ok: %d\n", r.Ok.value);
        break;
    case Result_Err:
        printf("err %d: %s\n", r.Err.code, r.Err.message);
        break;
    }
    return 0;
}
