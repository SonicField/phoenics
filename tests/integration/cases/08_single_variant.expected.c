typedef enum {
    Wrapper_Value,
    Wrapper__COUNT
} Wrapper_Tag;

typedef struct {
    Wrapper_Tag tag;
    union {
        struct { int x; } Value;
    };
} Wrapper;

static inline Wrapper Wrapper_mk_Value(int x) {
    Wrapper _v;
    _v.tag = Wrapper_Value;
    _v.Value.x = x;
    return _v;
}

#define Wrapper_as_Value(v) \
    (assert((v).tag == Wrapper_Value && "Wrapper: expected Value"), \
     (v).Value)

