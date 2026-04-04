#include "../test_framework.h"
#include "parser.h"

TEST(parse_simple_descr) {
    const char *src = "descr Shape { Circle { double radius; } };";
    ParseResult result = parse(src);
    ASSERT_EQ(result.error, 0);
    ASSERT_EQ(result.program.descr_count, 1);

    DescrDecl *d = &result.program.descrs[0];
    ASSERT_STR_EQ(d->name, "Shape");
    ASSERT_EQ(d->variant_count, 1);
    ASSERT_STR_EQ(d->variants[0].name, "Circle");
    ASSERT_EQ(d->variants[0].field_count, 1);
    ASSERT_STR_EQ(d->variants[0].fields[0].type_name, "double");
    ASSERT_STR_EQ(d->variants[0].fields[0].field_name, "radius");

    parse_result_free(&result);
}

TEST(parse_multi_variant) {
    const char *src =
        "descr Shape {\n"
        "    Circle { double radius; },\n"
        "    Rectangle { double width; double height; }\n"
        "};";
    ParseResult result = parse(src);
    ASSERT_EQ(result.error, 0);
    ASSERT_EQ(result.program.descr_count, 1);

    DescrDecl *d = &result.program.descrs[0];
    ASSERT_STR_EQ(d->name, "Shape");
    ASSERT_EQ(d->variant_count, 2);

    ASSERT_STR_EQ(d->variants[0].name, "Circle");
    ASSERT_EQ(d->variants[0].field_count, 1);

    ASSERT_STR_EQ(d->variants[1].name, "Rectangle");
    ASSERT_EQ(d->variants[1].field_count, 2);
    ASSERT_STR_EQ(d->variants[1].fields[0].type_name, "double");
    ASSERT_STR_EQ(d->variants[1].fields[0].field_name, "width");
    ASSERT_STR_EQ(d->variants[1].fields[1].type_name, "double");
    ASSERT_STR_EQ(d->variants[1].fields[1].field_name, "height");

    parse_result_free(&result);
}

TEST(parse_empty_variant) {
    const char *src = "descr Option { Some { int value; }, None {} };";
    ParseResult result = parse(src);
    ASSERT_EQ(result.error, 0);

    DescrDecl *d = &result.program.descrs[0];
    ASSERT_EQ(d->variant_count, 2);
    ASSERT_STR_EQ(d->variants[1].name, "None");
    ASSERT_EQ(d->variants[1].field_count, 0);

    parse_result_free(&result);
}

TEST(parse_passthrough_code) {
    const char *src = "#include <stdio.h>\nint main(void) { return 0; }\n";
    ParseResult result = parse(src);
    ASSERT_EQ(result.error, 0);
    ASSERT_EQ(result.program.descr_count, 0);
    /* passthrough chunks should be preserved */
    ASSERT(result.program.chunk_count > 0);

    parse_result_free(&result);
}

TEST(parse_descr_between_code) {
    const char *src =
        "#include <stdio.h>\n"
        "\n"
        "descr Result { Ok { int val; }, Err { int code; } };\n"
        "\n"
        "int main(void) { return 0; }\n";
    ParseResult result = parse(src);
    ASSERT_EQ(result.error, 0);
    ASSERT_EQ(result.program.descr_count, 1);
    ASSERT_STR_EQ(result.program.descrs[0].name, "Result");

    parse_result_free(&result);
}

TEST(parse_multi_word_type) {
    const char *src = "descr Foo { Bar { unsigned int x; } };";
    ParseResult result = parse(src);
    ASSERT_EQ(result.error, 0);

    DescrDecl *d = &result.program.descrs[0];
    ASSERT_EQ(d->variants[0].field_count, 1);
    ASSERT_STR_EQ(d->variants[0].fields[0].type_name, "unsigned int");
    ASSERT_STR_EQ(d->variants[0].fields[0].field_name, "x");

    parse_result_free(&result);
}

TEST(parse_pointer_type) {
    const char *src = "descr Foo { Bar { char *name; } };";
    ParseResult result = parse(src);
    ASSERT_EQ(result.error, 0);

    DescrDecl *d = &result.program.descrs[0];
    ASSERT_EQ(d->variants[0].field_count, 1);
    ASSERT_STR_EQ(d->variants[0].fields[0].type_name, "char *");
    ASSERT_STR_EQ(d->variants[0].fields[0].field_name, "name");

    parse_result_free(&result);
}

/* --- Error cases: parser must reject malformed input --- */

TEST(parse_error_missing_name) {
    const char *src = "descr { X { int a; } };";
    ParseResult result = parse(src);
    ASSERT(result.error != 0);
    ASSERT_NOT_NULL(result.error_message);
    parse_result_free(&result);
}

TEST(parse_error_missing_open_brace) {
    const char *src = "descr Shape Circle { double radius; } };";
    ParseResult result = parse(src);
    ASSERT(result.error != 0);
    parse_result_free(&result);
}

TEST(parse_error_missing_semicolon) {
    /* Missing trailing semicolon after closing brace */
    const char *src = "descr Shape { Circle { double radius; } }";
    ParseResult result = parse(src);
    ASSERT(result.error != 0);
    parse_result_free(&result);
}

TEST(parse_error_empty_descr) {
    /* descr with no variants */
    const char *src = "descr Empty {};";
    ParseResult result = parse(src);
    ASSERT(result.error != 0);
    parse_result_free(&result);
}

TEST(parse_single_variant_no_trailing_comma) {
    /* Single variant with no trailing comma — should be valid */
    const char *src = "descr Wrap { Val { int x; } };";
    ParseResult result = parse(src);
    ASSERT_EQ(result.error, 0);
    ASSERT_EQ(result.program.descr_count, 1);
    ASSERT_EQ(result.program.descrs[0].variant_count, 1);
    parse_result_free(&result);
}

TEST(parse_const_pointer_type) {
    const char *src = "descr Foo { Bar { const char *name; } };";
    ParseResult result = parse(src);
    ASSERT_EQ(result.error, 0);

    DescrDecl *d = &result.program.descrs[0];
    ASSERT_EQ(d->variants[0].field_count, 1);
    ASSERT_STR_EQ(d->variants[0].fields[0].type_name, "const char *");
    ASSERT_STR_EQ(d->variants[0].fields[0].field_name, "name");

    parse_result_free(&result);
}

/* --- match_descr parsing --- */

TEST(parse_match_descr_basic) {
    const char *src =
        "descr Shape { Circle { double r; }, Rect { int w; } };\n"
        "match_descr(Shape, s) {\n"
        "    case Circle: { foo(); } break;\n"
        "    case Rect: { bar(); } break;\n"
        "}\n";

    ParseResult result = parse(src);
    ASSERT_EQ(result.error, 0);
    ASSERT_EQ(result.program.descr_count, 1);

    /* Find the match_descr chunk */
    int found_match = 0;
    for (int i = 0; i < result.program.chunk_count; i++) {
        if (result.program.chunks[i].type == CHUNK_MATCH_DESCR) {
            MatchDescr *m = &result.program.chunks[i].match_descr;
            ASSERT_STR_EQ(m->type_name, "Shape");
            ASSERT_STR_EQ(m->expr_text, "s");
            ASSERT_EQ(m->case_count, 2);
            ASSERT_STR_EQ(m->cases[0].variant_name, "Circle");
            ASSERT_STR_EQ(m->cases[1].variant_name, "Rect");
            found_match = 1;
            break;
        }
    }
    ASSERT(found_match);

    parse_result_free(&result);
}

TEST(parse_match_descr_complex_expr) {
    /* match_descr with a more complex expression */
    const char *src =
        "descr AB { A { int x; }, B { int y; } };\n"
        "match_descr(AB, arr[i]) {\n"
        "    case A: { break; }\n"
        "    case B: { break; }\n"
        "}\n";

    ParseResult result = parse(src);
    ASSERT_EQ(result.error, 0);

    int found_match = 0;
    for (int i = 0; i < result.program.chunk_count; i++) {
        if (result.program.chunks[i].type == CHUNK_MATCH_DESCR) {
            MatchDescr *m = &result.program.chunks[i].match_descr;
            ASSERT_STR_EQ(m->type_name, "AB");
            ASSERT_STR_EQ(m->expr_text, "arr[i]");
            found_match = 1;
            break;
        }
    }
    ASSERT(found_match);

    parse_result_free(&result);
}

TEST(parse_match_descr_nested_braces) {
    /* Case body with nested braces — parser must track depth */
    const char *src =
        "descr X { A { int v; } };\n"
        "match_descr(X, x) {\n"
        "    case A: { if (x.A.v > 0) { do_thing(); } } break;\n"
        "}\n";

    ParseResult result = parse(src);
    ASSERT_EQ(result.error, 0);

    int found_match = 0;
    for (int i = 0; i < result.program.chunk_count; i++) {
        if (result.program.chunks[i].type == CHUNK_MATCH_DESCR) {
            MatchDescr *m = &result.program.chunks[i].match_descr;
            ASSERT_EQ(m->case_count, 1);
            ASSERT_STR_EQ(m->cases[0].variant_name, "A");
            /* The body text should contain the nested braces */
            ASSERT_NOT_NULL(m->cases[0].body_text);
            found_match = 1;
            break;
        }
    }
    ASSERT(found_match);

    parse_result_free(&result);
}

TEST(parse_match_descr_error_missing_lparen) {
    const char *src =
        "descr X { A { int v; } };\n"
        "match_descr X, x) { case A: { break; } }\n";

    ParseResult result = parse(src);
    ASSERT(result.error != 0);
    parse_result_free(&result);
}

TEST(parse_match_descr_error_missing_rparen) {
    const char *src =
        "descr X { A { int v; } };\n"
        "match_descr(X, x { case A: { break; } }\n";

    ParseResult result = parse(src);
    ASSERT(result.error != 0);
    parse_result_free(&result);
}

TEST_MAIN(
    /* descr parsing */
    RUN_TEST(parse_simple_descr);
    RUN_TEST(parse_multi_variant);
    RUN_TEST(parse_empty_variant);
    RUN_TEST(parse_passthrough_code);
    RUN_TEST(parse_descr_between_code);
    RUN_TEST(parse_multi_word_type);
    RUN_TEST(parse_pointer_type);
    /* descr error cases */
    RUN_TEST(parse_error_missing_name);
    RUN_TEST(parse_error_missing_open_brace);
    RUN_TEST(parse_error_missing_semicolon);
    RUN_TEST(parse_error_empty_descr);
    RUN_TEST(parse_single_variant_no_trailing_comma);
    RUN_TEST(parse_const_pointer_type);
    /* match_descr parsing */
    RUN_TEST(parse_match_descr_basic);
    RUN_TEST(parse_match_descr_complex_expr);
    RUN_TEST(parse_match_descr_nested_braces);
    RUN_TEST(parse_match_descr_error_missing_lparen);
    RUN_TEST(parse_match_descr_error_missing_rparen);
)
