extern void abort(void);
#include <stdio.h>

typedef enum {
    Data_Buffer,
    Data_Empty,
    Data__COUNT
} Data_Tag;

typedef struct {
    int values[4];
    int count;
} Data_Buffer_t;

typedef struct {
    char _empty;
} Data_Empty_t;

typedef struct Data {
    Data_Tag tag;
    union {
        Data_Buffer_t Buffer;
        Data_Empty_t Empty;
    };
} Data;

static inline Data Data_mk_Buffer(int count) {
    Data _v;
    _v.tag = Data_Buffer;
    _v.Buffer.count = count;
    return _v;
}

static inline Data Data_mk_Empty(void) {
    Data _v;
    _v.tag = Data_Empty;
    return _v;
}

static inline Data_Buffer_t Data_as_Buffer(Data v) {
    if (v.tag != Data_Buffer) abort();
    return v.Buffer;
}

static inline Data_Empty_t Data_as_Empty(Data v) {
    if (v.tag != Data_Empty) abort();
    return v.Empty;
}
#line 7

int main(void) {
    Data d;
    d.tag = Data_Buffer;
    d.Buffer.values[0] = 10;
    d.Buffer.values[1] = 20;
    d.Buffer.count = 2;
    switch (d.tag) {
    case Data_Buffer: {
            for (int i = 0; i < d.Buffer.count; i++)
                printf("%d ", d.Buffer.values[i]);
            printf("\n");
        } break;
    case Data_Empty: {
            printf("empty\n");
        } break;
    default: break;
}
#line 24
    return 0;
}
