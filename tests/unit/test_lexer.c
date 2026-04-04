#include "../test_framework.h"
#include "lexer.h"

TEST(lex_descr_keyword) {
    Lexer lex;
    lexer_init(&lex, "descr");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_DESCR);
    ASSERT_STR_EQ(tok.value, "descr");
}

TEST(lex_identifier) {
    Lexer lex;
    lexer_init(&lex, "Shape");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_IDENT);
    ASSERT_STR_EQ(tok.value, "Shape");
}

TEST(lex_braces) {
    Lexer lex;
    lexer_init(&lex, "{ }");
    Token t1 = lexer_next(&lex);
    Token t2 = lexer_next(&lex);
    ASSERT_EQ(t1.type, TOK_LBRACE);
    ASSERT_EQ(t2.type, TOK_RBRACE);
}

TEST(lex_semicolon_comma) {
    Lexer lex;
    lexer_init(&lex, ";,");
    Token t1 = lexer_next(&lex);
    Token t2 = lexer_next(&lex);
    ASSERT_EQ(t1.type, TOK_SEMICOLON);
    ASSERT_EQ(t2.type, TOK_COMMA);
}

TEST(lex_eof) {
    Lexer lex;
    lexer_init(&lex, "");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_EOF);
}

TEST(lex_descr_declaration) {
    Lexer lex;
    lexer_init(&lex, "descr Shape { Circle { double radius; } };");

    Token tok;
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_DESCR);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT);
    ASSERT_STR_EQ(tok.value, "Shape");
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT);
    ASSERT_STR_EQ(tok.value, "Circle");
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT);
    ASSERT_STR_EQ(tok.value, "double");
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT);
    ASSERT_STR_EQ(tok.value, "radius");
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_SEMICOLON);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_RBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_RBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_SEMICOLON);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_EOF);
}

TEST(lex_passthrough_text) {
    Lexer lex;
    lexer_init(&lex, "#include <stdio.h>\nint main() { return 0; }");
    /* When not in descr context, the lexer should still tokenize */
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
}

TEST(lex_tracks_line_numbers) {
    Lexer lex;
    lexer_init(&lex, "descr\nShape");
    Token t1 = lexer_next(&lex);
    Token t2 = lexer_next(&lex);
    ASSERT_EQ(t1.line, 1);
    ASSERT_EQ(t2.line, 2);
}

TEST_MAIN(
    RUN_TEST(lex_descr_keyword);
    RUN_TEST(lex_identifier);
    RUN_TEST(lex_braces);
    RUN_TEST(lex_semicolon_comma);
    RUN_TEST(lex_eof);
    RUN_TEST(lex_descr_declaration);
    RUN_TEST(lex_passthrough_text);
    RUN_TEST(lex_tracks_line_numbers);
)
