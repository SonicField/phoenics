#include <assert.h>
typedef enum {
    Maybe_int_Just,
    Maybe_int_Nothing,
    Maybe_int__COUNT
} Maybe_int_Tag;

typedef struct {
    int value;
} Maybe_int_Just_t;

typedef struct {
    char _empty;
} Maybe_int_Nothing_t;

typedef struct {
    Maybe_int_Tag tag;
    union {
        Maybe_int_Just_t Just;
        Maybe_int_Nothing_t Nothing;
    };
} Maybe_int;

static inline Maybe_int Maybe_int_mk_Just(int value) {
    Maybe_int _v;
    _v.tag = Maybe_int_Just;
    _v.Just.value = value;
    return _v;
}

static inline Maybe_int Maybe_int_mk_Nothing(void) {
    Maybe_int _v;
    _v.tag = Maybe_int_Nothing;
    return _v;
}

static inline Maybe_int_Just_t Maybe_int_as_Just(Maybe_int v) {
    assert(v.tag == Maybe_int_Just && "Maybe_int: expected Just");
    return v.Just;
}

static inline Maybe_int_Nothing_t Maybe_int_as_Nothing(Maybe_int v) {
    assert(v.tag == Maybe_int_Nothing && "Maybe_int: expected Nothing");
    return v.Nothing;
}
