/*
 * Semantic analyser unit tests for PHC (Phoenics).
 *
 * The semantic analyser operates on the parsed AST and is responsible for:
 *   1. Building a descr type table (mapping type names to variant lists)
 *   2. Validating descr declarations (no duplicate variant names, at least one variant)
 *   3. Validating match_descr constructs (type exists, exhaustive cases, no duplicates)
 *
 * It does NOT do type checking, scope analysis, or type inference.
 * Those are the C compiler's job.
 *
 * These tests require both parser.h and semantic.h.
 */

#include "../test_framework.h"
#include "parser.h"
#include "semantic.h"

/* === Type table building === */

TEST(sem_builds_type_table) {
    const char *src = "descr Shape { Circle { double r; }, Rect { int w; } };";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    SemanticResult sr = analyse(&pr.program);
    ASSERT_EQ(sr.error, 0);
    ASSERT_EQ(sr.descr_type_count, 1);
    ASSERT_STR_EQ(sr.descr_types[0].name, "Shape");
    ASSERT_EQ(sr.descr_types[0].variant_count, 2);

    semantic_result_free(&sr);
    parse_result_free(&pr);
}

TEST(sem_builds_multiple_types) {
    const char *src =
        "descr A { X { int a; } };\n"
        "descr B { Y { int b; }, Z { int c; } };\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    SemanticResult sr = analyse(&pr.program);
    ASSERT_EQ(sr.error, 0);
    ASSERT_EQ(sr.descr_type_count, 2);

    semantic_result_free(&sr);
    parse_result_free(&pr);
}

/* === Descr validation === */

TEST(sem_error_duplicate_variant_name) {
    const char *src = "descr Bad { A { int x; }, A { int y; } };";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    SemanticResult sr = analyse(&pr.program);
    ASSERT(sr.error != 0);
    ASSERT_NOT_NULL(sr.error_message);
    ASSERT_STR_CONTAINS(sr.error_message, "duplicate variant");

    semantic_result_free(&sr);
    parse_result_free(&pr);
}

TEST(sem_error_duplicate_descr_name) {
    const char *src =
        "descr Dup { A { int x; } };\n"
        "descr Dup { B { int y; } };\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    SemanticResult sr = analyse(&pr.program);
    ASSERT(sr.error != 0);
    ASSERT_NOT_NULL(sr.error_message);
    ASSERT_STR_CONTAINS(sr.error_message, "duplicate descr");

    semantic_result_free(&sr);
    parse_result_free(&pr);
}

/* === match_descr exhaustiveness checking === */

TEST(sem_match_descr_exhaustive_pass) {
    const char *src =
        "descr Shape { Circle { double r; }, Rect { int w; } };\n"
        "match_descr(Shape, s) {\n"
        "    case Circle: { break; }\n"
        "    case Rect: { break; }\n"
        "}\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    SemanticResult sr = analyse(&pr.program);
    ASSERT_EQ(sr.error, 0);

    semantic_result_free(&sr);
    parse_result_free(&pr);
}

TEST(sem_match_descr_missing_variant) {
    const char *src =
        "descr Shape { Circle { double r; }, Rect { int w; }, Tri { double b; } };\n"
        "match_descr(Shape, s) {\n"
        "    case Circle: { break; }\n"
        "    case Rect: { break; }\n"
        "}\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    SemanticResult sr = analyse(&pr.program);
    ASSERT(sr.error != 0);
    ASSERT_NOT_NULL(sr.error_message);
    ASSERT_STR_CONTAINS(sr.error_message, "Tri");

    semantic_result_free(&sr);
    parse_result_free(&pr);
}

TEST(sem_match_descr_multiple_missing) {
    const char *src =
        "descr ABCD { A {}, B {}, C {}, D {} };\n"
        "match_descr(ABCD, v) {\n"
        "    case A: { break; }\n"
        "}\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    SemanticResult sr = analyse(&pr.program);
    ASSERT(sr.error != 0);
    ASSERT_NOT_NULL(sr.error_message);
    /* Should mention at least one of the missing variants */
    ASSERT(strstr(sr.error_message, "B") != NULL ||
           strstr(sr.error_message, "C") != NULL ||
           strstr(sr.error_message, "D") != NULL);

    semantic_result_free(&sr);
    parse_result_free(&pr);
}

TEST(sem_match_descr_duplicate_case) {
    const char *src =
        "descr AB { A { int x; }, B { int y; } };\n"
        "match_descr(AB, v) {\n"
        "    case A: { break; }\n"
        "    case B: { break; }\n"
        "    case A: { break; }\n"
        "}\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    SemanticResult sr = analyse(&pr.program);
    ASSERT(sr.error != 0);
    ASSERT_NOT_NULL(sr.error_message);
    ASSERT_STR_CONTAINS(sr.error_message, "duplicate");

    semantic_result_free(&sr);
    parse_result_free(&pr);
}

TEST(sem_match_descr_unknown_type) {
    const char *src =
        "match_descr(NoSuchType, x) {\n"
        "    case Foo: { break; }\n"
        "}\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    SemanticResult sr = analyse(&pr.program);
    ASSERT(sr.error != 0);
    ASSERT_NOT_NULL(sr.error_message);
    ASSERT_STR_CONTAINS(sr.error_message, "NoSuchType");

    semantic_result_free(&sr);
    parse_result_free(&pr);
}

TEST(sem_match_descr_unknown_variant_in_case) {
    /* All variants present plus an extra that doesn't exist in the descr */
    const char *src =
        "descr AB { A { int x; }, B { int y; } };\n"
        "match_descr(AB, v) {\n"
        "    case A: { break; }\n"
        "    case B: { break; }\n"
        "    case C: { break; }\n"
        "}\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    SemanticResult sr = analyse(&pr.program);
    ASSERT(sr.error != 0);
    ASSERT_NOT_NULL(sr.error_message);
    /* Should flag 'C' as not a variant of 'AB' */
    ASSERT_STR_CONTAINS(sr.error_message, "C");

    semantic_result_free(&sr);
    parse_result_free(&pr);
}

/* === No match_descr — should pass cleanly === */

TEST(sem_no_match_descr_passes) {
    const char *src =
        "descr Shape { Circle { double r; } };\n"
        "int main(void) { return 0; }\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    SemanticResult sr = analyse(&pr.program);
    ASSERT_EQ(sr.error, 0);

    semantic_result_free(&sr);
    parse_result_free(&pr);
}

TEST(sem_passthrough_only) {
    const char *src = "#include <stdio.h>\nint main(void) { return 0; }\n";
    ParseResult pr = parse(src);
    ASSERT_EQ(pr.error, 0);

    SemanticResult sr = analyse(&pr.program);
    ASSERT_EQ(sr.error, 0);

    semantic_result_free(&sr);
    parse_result_free(&pr);
}

TEST_MAIN(
    /* Type table */
    RUN_TEST(sem_builds_type_table);
    RUN_TEST(sem_builds_multiple_types);
    /* Descr validation */
    RUN_TEST(sem_error_duplicate_variant_name);
    RUN_TEST(sem_error_duplicate_descr_name);
    /* match_descr exhaustiveness */
    RUN_TEST(sem_match_descr_exhaustive_pass);
    RUN_TEST(sem_match_descr_missing_variant);
    RUN_TEST(sem_match_descr_multiple_missing);
    RUN_TEST(sem_match_descr_duplicate_case);
    RUN_TEST(sem_match_descr_unknown_type);
    RUN_TEST(sem_match_descr_unknown_variant_in_case);
    /* Clean pass cases */
    RUN_TEST(sem_no_match_descr_passes);
    RUN_TEST(sem_passthrough_only);
)
