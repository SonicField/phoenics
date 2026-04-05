extern void abort(void);
#include <stdio.h>
#include <stdlib.h>

typedef struct List List;

typedef enum {
    List_Cons,
    List_Nil,
    List__COUNT
} List_Tag;

typedef struct {
    int head;
    List *tail;
} List_Cons_t;

typedef struct {
    char _empty;
} List_Nil_t;

typedef struct List {
    List_Tag tag;
    union {
        List_Cons_t Cons;
        List_Nil_t Nil;
    };
} List;

static inline List List_mk_Cons(int head, List *tail) {
    List _v;
    _v.tag = List_Cons;
    _v.Cons.head = head;
    _v.Cons.tail = tail;
    return _v;
}

static inline List List_mk_Nil(void) {
    List _v;
    _v.tag = List_Nil;
    return _v;
}

static inline List_Cons_t List_as_Cons(List v) {
    if (v.tag != List_Cons) abort();
    return v.Cons;
}

static inline List_Nil_t List_as_Nil(List v) {
    if (v.tag != List_Nil) abort();
    return v.Nil;
}
#line 10

int main(void) {
    List nil = List_mk_Nil();
    List one = List_mk_Cons(1, &nil);
    List two = List_mk_Cons(2, &one);

    List *cur = &two;
    while (cur->tag == List_Cons) {
        printf("%d ", cur->Cons.head);
        cur = cur->Cons.tail;
    }
    printf("\n");
    return 0;
}
