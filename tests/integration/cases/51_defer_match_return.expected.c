extern void abort(void);
#define phc_free(pp) do { free(*(pp)); *(pp) = ((void*)0); } while(0)
#include <stdio.h>
#include <stdlib.h>

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
} Result_Err_t;

typedef struct Result {
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

static inline Result Result_mk_Err(int code) {
    Result _v;
    _v.tag = Result_Err;
    _v.Err.code = code;
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
#line 8

int process(Result r) {
    int *log = malloc(sizeof(int));
    *log = 0;
    switch (r.tag) {
    case Result_Ok: { int value = r.Ok.value; {
            printf("ok: %d\n", value);
            {  free(log); printf("log freed\n"); return value; }
        } break; }
    case Result_Err: { int code = r.Err.code; {
            printf("err: %d\n", code);
            {  free(log); printf("log freed\n"); return code; }
        } break; }
    default: break;
}
#line 24
    {  free(log); printf("log freed\n"); return -99; }
 free(log); printf("log freed\n");
}

int main(void) {
    Result ok = Result_mk_Ok(42);
    Result err = Result_mk_Err(-1);
    printf("ret=%d\n", process(ok));
    printf("ret=%d\n", process(err));
    return 0;
}
