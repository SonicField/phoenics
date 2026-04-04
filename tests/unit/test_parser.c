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

TEST_MAIN(
    RUN_TEST(parse_simple_descr);
    RUN_TEST(parse_multi_variant);
    RUN_TEST(parse_empty_variant);
    RUN_TEST(parse_passthrough_code);
    RUN_TEST(parse_descr_between_code);
    RUN_TEST(parse_multi_word_type);
    RUN_TEST(parse_pointer_type);
)
