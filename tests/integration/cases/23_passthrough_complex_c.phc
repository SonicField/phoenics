/* Complex C file — no Phoenics constructs.
   Passthrough must be byte-for-byte identical. */

#include <stdio.h>
#include <stdlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MULTI_LINE_MACRO(x, y) \
    do { \
        int _tmp = (x); \
        (x) = (y); \
        (y) = _tmp; \
    } while (0)

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

/* Nested comment-like patterns: // inside block comment */
/* Another: "string inside comment" */

typedef struct {
    int (*callback)(void *, size_t);
    const char *name;
    union {
        struct { int x, y; } point;
        struct { double r; } radius;
    };
} complex_t;

static UNUSED int tricky_strings(void) {
    const char *a = "phc_descr inside string";
    const char *b = "phc_match(Foo, bar) { case X: {} }";
    const char *c = "backslash\\end";
    const char *d = "embedded\nnewline";
    const char *e = "";
    (void)a; (void)b; (void)c; (void)d; (void)e;
    return 0;
}

int main(void) {
    int a = 3, b = 7;
    MULTI_LINE_MACRO(a, b);
    printf("%d\n", MAX(a, b));
    return 0;
}
