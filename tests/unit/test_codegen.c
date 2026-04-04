#include "../test_framework.h"
#include "codegen.h"
#include "parser.h"
#include <string.h>

/* Helper: check that a substring appears in the output */
static int contains(const char *haystack, const char *needle) {
    return strstr(haystack, needle) != NULL;
}

TEST(codegen_tag_enum) {
    const char *src = "phc_descr Shape { Circle { double radius; }, Rect { int w; } };";
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
    const char *src = "phc_descr Shape { Circle { double radius; } };";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    ASSERT(contains(out, "typedef struct {"));
    ASSERT(contains(out, "Shape_Tag tag;"));
    ASSERT(contains(out, "union {"));
    ASSERT(contains(out, "Shape_Circle_t Circle;"));
    ASSERT(contains(out, "} Shape;"));

    free(out);
    parse_result_free(&pr);
}

TEST(codegen_constructor_functions) {
    const char *src = "phc_descr Shape { Circle { double radius; } };";
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
    const char *src = "phc_descr Option { Some { int value; }, None {} };";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    ASSERT(contains(out, "static inline Option Option_mk_None(void)"));
    ASSERT(contains(out, "Option_None_t None;"));

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

TEST(codegen_multiple_descrs) {
    const char *src =
        "phc_descr A { X { int a; } };\n"
        "phc_descr B { Y { int b; } };\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    ASSERT(contains(out, "A_Tag"));
    ASSERT(contains(out, "B_Tag"));
    ASSERT(contains(out, "A_mk_X"));
    ASSERT(contains(out, "B_mk_Y"));

    free(out);
    parse_result_free(&pr);
}

TEST(codegen_multi_field_constructor) {
    const char *src = "phc_descr P { Point { int x; int y; int z; } };";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    ASSERT(contains(out, "P_mk_Point(int x, int y, int z)"));
    ASSERT(contains(out, "_v.Point.x = x;"));
    ASSERT(contains(out, "_v.Point.y = y;"));
    ASSERT(contains(out, "_v.Point.z = z;"));

    free(out);
    parse_result_free(&pr);
}

TEST(codegen_descr_between_code_ordering) {
    /* Verify that codegen preserves ordering: header, descr expansion, footer */
    const char *src =
        "#include <stdio.h>\n"
        "phc_descr V { A { int x; } };\n"
        "int main(void) { return 0; }\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);

    /* #include must appear before the typedef */
    const char *inc_pos = strstr(out, "#include <stdio.h>");
    const char *td_pos = strstr(out, "typedef enum");
    const char *main_pos = strstr(out, "int main");
    ASSERT_NOT_NULL(inc_pos);
    ASSERT_NOT_NULL(td_pos);
    ASSERT_NOT_NULL(main_pos);
    ASSERT(inc_pos < td_pos);
    ASSERT(td_pos < main_pos);

    free(out);
    parse_result_free(&pr);
}

/* --- __COUNT sentinel --- */

TEST(codegen_count_sentinel) {
    const char *src = "phc_descr Shape { Circle { double r; }, Rect { int w; } };";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    ASSERT(contains(out, "Shape__COUNT"));

    free(out);
    parse_result_free(&pr);
}

/* --- match_descr codegen --- */

TEST(codegen_match_descr_generates_switch) {
    const char *src =
        "phc_descr Shape { Circle { double r; }, Rect { int w; } };\n"
        "phc_match(Shape, s) {\n"
        "    case Circle: { foo(); } break;\n"
        "    case Rect: { bar(); } break;\n"
        "}\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    /* match_descr should generate a standard switch on .tag */
    ASSERT(contains(out, "switch (s.tag)"));
    /* Case labels should be prefixed with type name */
    ASSERT(contains(out, "case Shape_Circle:"));
    ASSERT(contains(out, "case Shape_Rect:"));
    /* Should include default: break; for __COUNT sentinel */
    ASSERT(contains(out, "default:"));

    free(out);
    parse_result_free(&pr);
}

TEST(codegen_match_descr_preserves_body) {
    const char *src =
        "phc_descr AB { A { int x; }, B { int y; } };\n"
        "phc_match(AB, v) {\n"
        "    case A: { printf(\"%d\", v.A.x); } break;\n"
        "    case B: { printf(\"%d\", v.B.y); } break;\n"
        "}\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    /* Case bodies should be preserved in the output */
    ASSERT(contains(out, "v.A.x"));
    ASSERT(contains(out, "v.B.y"));

    free(out);
    parse_result_free(&pr);
}

/* --- Safe accessor functions --- */

TEST(codegen_safe_accessor_generated) {
    const char *src = "phc_descr Shape { Circle { double radius; }, Rect { int w; } };";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    /* Should generate Shape_as_Circle accessor */
    ASSERT(contains(out, "Shape_as_Circle"));
    ASSERT(contains(out, "Shape_as_Rect"));
    /* Accessor should check the tag */
    ASSERT(contains(out, "__builtin_trap()"));
    ASSERT(contains(out, "Shape_Circle"));

    free(out);
    parse_result_free(&pr);
}

TEST(codegen_safe_accessor_empty_variant) {
    const char *src = "phc_descr Option { Some { int value; }, None {} };";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    /* Even empty variants get accessors (for tag checking) */
    ASSERT(contains(out, "Option_as_Some"));
    ASSERT(contains(out, "Option_as_None"));

    free(out);
    parse_result_free(&pr);
}

TEST(codegen_safe_accessor_returns_pointer) {
    const char *src = "phc_descr V { A { int x; int y; } };";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    char *out = codegen(&pr.program);
    ASSERT_NOT_NULL(out);
    /* Accessor function checks tag then returns variant struct */
    ASSERT(contains(out, "V_A_t V_as_A("));
    ASSERT(contains(out, "__builtin_trap()"));
    ASSERT(contains(out, "return v.A;"));

    free(out);
    parse_result_free(&pr);
}

TEST_MAIN(
    /* descr codegen */
    RUN_TEST(codegen_tag_enum);
    RUN_TEST(codegen_struct_with_union);
    RUN_TEST(codegen_constructor_functions);
    RUN_TEST(codegen_empty_variant_constructor);
    RUN_TEST(codegen_passthrough_preserved);
    RUN_TEST(codegen_multiple_descrs);
    RUN_TEST(codegen_multi_field_constructor);
    RUN_TEST(codegen_descr_between_code_ordering);
    /* __COUNT sentinel */
    RUN_TEST(codegen_count_sentinel);
    /* match_descr codegen */
    RUN_TEST(codegen_match_descr_generates_switch);
    RUN_TEST(codegen_match_descr_preserves_body);
    /* safe accessor functions */
    RUN_TEST(codegen_safe_accessor_generated);
    RUN_TEST(codegen_safe_accessor_empty_variant);
    RUN_TEST(codegen_safe_accessor_returns_pointer);
)
