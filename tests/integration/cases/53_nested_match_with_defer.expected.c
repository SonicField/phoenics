#ifndef abort
extern void abort(void);
#endif
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

typedef enum {
    Action_Process,
    Action_Skip,
    Action__COUNT
} Action_Tag;

typedef struct {
    int input;
} Action_Process_t;

typedef struct {
    char _empty;
} Action_Skip_t;

typedef struct Action {
    Action_Tag tag;
    union {
        Action_Process_t Process;
        Action_Skip_t Skip;
    };
} Action;

static inline Action Action_mk_Process(int input) {
    Action _v;
    _v.tag = Action_Process;
    _v.Process.input = input;
    return _v;
}

static inline Action Action_mk_Skip(void) {
    Action _v;
    _v.tag = Action_Skip;
    return _v;
}

static inline Action_Process_t Action_as_Process(Action v) {
    if (v.tag != Action_Process) abort();
    return v.Process;
}

static inline Action_Skip_t Action_as_Skip(Action v) {
    if (v.tag != Action_Skip) abort();
    return v.Skip;
}
#line 13

int handle(Action act, Result res) {
    int *log = malloc(sizeof(int));
    *log = 0;
    switch (act.tag) {
    case Action_Process: { int input = act.Process.input; {
            switch (res.tag) {
    case Result_Ok: { int value = res.Ok.value; {
                    printf("process ok: %d + %d = %d\n", input, value, input + value);
                    {  free(log); printf("log freed\n"); return input + value; }
                } break; }
    case Result_Err: { int code = res.Err.code; {
                    printf("process err: %d\n", code);
                    {  free(log); printf("log freed\n"); return code; }
                } break; }
    default: break;
}
        } break; }
    case Action_Skip: {
            printf("skip\n");
            {  free(log); printf("log freed\n"); return 0; }
        } break;
    default: break;
}
#line 37
    {  free(log); printf("log freed\n"); return -99; }
 free(log); printf("log freed\n");
}

int main(void) {
    Action proc = Action_mk_Process(10);
    Action skip = Action_mk_Skip();
    Result ok = Result_mk_Ok(5);
    Result err = Result_mk_Err(-1);

    printf("ret=%d\n", handle(proc, ok));
    printf("ret=%d\n", handle(proc, err));
    printf("ret=%d\n", handle(skip, ok));
    return 0;
}
