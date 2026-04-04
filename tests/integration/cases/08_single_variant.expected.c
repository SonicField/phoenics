#include <assert.h>
typedef enum {
    Wrapper_Value,
    Wrapper__COUNT
} Wrapper_Tag;

typedef struct {
    int x;
} Wrapper_Value_t;

typedef struct {
    Wrapper_Tag tag;
    union {
        Wrapper_Value_t Value;
    };
} Wrapper;

static inline Wrapper Wrapper_mk_Value(int x) {
    Wrapper _v;
    _v.tag = Wrapper_Value;
    _v.Value.x = x;
    return _v;
}

static inline Wrapper_Value_t Wrapper_as_Value(Wrapper v) {
    assert(v.tag == Wrapper_Value && "Wrapper: expected Value");
    return v.Value;
}
