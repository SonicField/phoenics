#include "../test_framework.h"
#include "codegen.h"
#include "parser.h"
#include <string.h>

/* Helper: check that a substring appears in the output */
static int contains(const char *haystack, const char *needle) {
    return strstr(haystack, needle) != NULL;
}

TEST(codegen_tag_enum) {
    const char *src = "descr Shape { Circle { double radius; }, Rect { int w; } };";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    ASSERT(contains(out, "typedef enum {"));
    ASSERT(contains(out, "Shape_Circle"));
    ASSERT(contains(out, "Shape_Rect"));
    ASSERT(contains(out, "} Shape_Tag;"));

    free(out);
    parse_result_free(&pr);
}

TEST(codegen_struct_with_union) {
    const char *src = "descr Shape { Circle { double radius; } };";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    ASSERT(contains(out, "typedef struct {"));
    ASSERT(contains(out, "Shape_Tag tag;"));
    ASSERT(contains(out, "union {"));
    ASSERT(contains(out, "struct { double radius; } Circle;"));
    ASSERT(contains(out, "} Shape;"));

    free(out);
    parse_result_free(&pr);
}

TEST(codegen_constructor_functions) {
    const char *src = "descr Shape { Circle { double radius; } };";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    ASSERT(contains(out, "static inline Shape Shape_mk_Circle(double radius)"));
    ASSERT(contains(out, "_v.tag = Shape_Circle;"));
    ASSERT(contains(out, "_v.Circle.radius = radius;"));
    ASSERT(contains(out, "return _v;"));

    free(out);
    parse_result_free(&pr);
}

TEST(codegen_empty_variant_constructor) {
    const char *src = "descr Option { Some { int value; }, None {} };";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    ASSERT(contains(out, "static inline Option Option_mk_None(void)"));
    ASSERT(contains(out, "struct { char _empty; } None;"));

    free(out);
    parse_result_free(&pr);
}

TEST(codegen_passthrough_preserved) {
    const char *src = "#include <stdio.h>\nint main(void) { return 0; }\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    ASSERT(contains(out, "#include <stdio.h>"));
    ASSERT(contains(out, "int main(void) { return 0; }"));

    free(out);
    parse_result_free(&pr);
}

TEST_MAIN(
    RUN_TEST(codegen_tag_enum);
    RUN_TEST(codegen_struct_with_union);
    RUN_TEST(codegen_constructor_functions);
    RUN_TEST(codegen_empty_variant_constructor);
    RUN_TEST(codegen_passthrough_preserved);
)
