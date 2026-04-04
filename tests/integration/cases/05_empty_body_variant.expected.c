typedef enum {
    Maybe_int_Just,
    Maybe_int_Nothing,
    Maybe_int__COUNT
} Maybe_int_Tag;

typedef struct {
    Maybe_int_Tag tag;
    union {
        struct { int value; } Just;
        struct { char _empty; } Nothing;
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
