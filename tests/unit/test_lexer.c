/*
 * Lexer unit tests for PHC (Phoenics).
 *
 * The lexer operates in TWO MODES (per architecture doc):
 *
 * 1. PASSTHROUGH MODE (default): scans forward, consuming text as TOK_OTHER,
 *    respecting string/char/comment boundaries. Stops when it encounters a
 *    Phoenics keyword (descr, match_descr).
 *
 * 2. STRUCTURED MODE (after keyword): tokenizes into individual tokens
 *    (TOK_IDENT, TOK_LBRACE, etc.). Tracks brace depth. Returns to
 *    passthrough mode when the construct ends.
 *
 * Consequence: structural tokens (braces, parens, etc.) only appear in
 * structured mode. In passthrough mode, they are absorbed into TOK_OTHER.
 * Tests for structural tokens must provide keyword context to trigger
 * structured mode.
 *
 * Token.value is a pointer with a separate length field — tests use
 * ASSERT_STRN_EQ(tok.value, tok.length, "expected") for length-aware comparison.
 */

#include "../test_framework.h"
#include "lexer.h"

/* === Keyword recognition (triggers mode switch) === */

TEST(lex_descr_keyword) {
    Lexer lex;
    lexer_init(&lex, "phc_descr Shape");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_DESCR);
    ASSERT_STRN_EQ(tok.value, tok.length, "phc_descr");
}

TEST(lex_match_descr_keyword) {
    Lexer lex;
    lexer_init(&lex, "phc_match(X, v)");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_MATCH_DESCR);
    ASSERT_STRN_EQ(tok.value, tok.length, "phc_match");
}

/* === Structural tokens in descr context (structured mode) === */

TEST(lex_braces_in_descr) {
    Lexer lex;
    lexer_init(&lex, "phc_descr X { };");
    Token tok;
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_DESCR);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_RBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_SEMICOLON);
}

TEST(lex_semicolon_comma_in_descr) {
    Lexer lex;
    lexer_init(&lex, "phc_descr X { A { int a; }, B { int b; } };");
    Token tok;
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_DESCR);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* X */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* A */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* int */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* a */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_SEMICOLON);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_RBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_COMMA);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* B */
}

TEST(lex_star_in_descr) {
    /* Pointer field type — star tokenised in structured mode */
    Lexer lex;
    lexer_init(&lex, "phc_descr F { B { char *name; } };");
    Token tok;
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_DESCR);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* F */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* B */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* char */
    ASSERT_STRN_EQ(tok.value, tok.length, "char");
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_STAR);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* name */
    ASSERT_STRN_EQ(tok.value, tok.length, "name");
}

TEST(lex_identifier_in_descr) {
    Lexer lex;
    lexer_init(&lex, "phc_descr Shape");
    Token tok;
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_DESCR);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT);
    ASSERT_STRN_EQ(tok.value, tok.length, "Shape");
}

/* === Structural tokens in match_descr context === */

TEST(lex_parens_in_match_descr) {
    Lexer lex;
    lexer_init(&lex, "phc_match(X, v)");
    Token tok;
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_MATCH_DESCR);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LPAREN);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* X */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_COMMA);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* v */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_RPAREN);
}

TEST(lex_case_in_match_descr) {
    Lexer lex;
    lexer_init(&lex, "phc_match(X, v) { case A: break; }");
    Token tok;
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_MATCH_DESCR);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LPAREN);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* X */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_COMMA);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* v */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_RPAREN);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_CASE);
    ASSERT_STRN_EQ(tok.value, tok.length, "case");
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* A */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_COLON);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_BREAK);
    ASSERT_STRN_EQ(tok.value, tok.length, "break");
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_SEMICOLON);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_RBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_EOF);
}

TEST(lex_eof) {
    Lexer lex;
    lexer_init(&lex, "");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_EOF);
}

/* === Full compound sequences === */

TEST(lex_descr_declaration) {
    Lexer lex;
    lexer_init(&lex, "phc_descr Shape { Circle { double radius; } };");

    Token tok;
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_DESCR);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT);
    ASSERT_STRN_EQ(tok.value, tok.length, "Shape");
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT);
    ASSERT_STRN_EQ(tok.value, tok.length, "Circle");
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT);
    ASSERT_STRN_EQ(tok.value, tok.length, "double");
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT);
    ASSERT_STRN_EQ(tok.value, tok.length, "radius");
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_SEMICOLON);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_RBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_RBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_SEMICOLON);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_EOF);
}

TEST(lex_match_descr_full_syntax) {
    Lexer lex;
    lexer_init(&lex, "phc_match(Shape, s) { case Circle: { x(); } break; case Rect: { y(); } break; }");

    Token tok;
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_MATCH_DESCR);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LPAREN);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT);
    ASSERT_STRN_EQ(tok.value, tok.length, "Shape");
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_COMMA);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT);
    ASSERT_STRN_EQ(tok.value, tok.length, "s");
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_RPAREN);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LBRACE);
    /* First case */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_CASE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT);
    ASSERT_STRN_EQ(tok.value, tok.length, "Circle");
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_COLON);
}

/* === Passthrough mode === */

TEST(lex_passthrough_pure_c) {
    /* Pure C with no Phoenics keywords — everything is TOK_OTHER */
    Lexer lex;
    lexer_init(&lex, "#include <stdio.h>\nint main() { return 0; }");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
}

TEST(lex_passthrough_structural_tokens) {
    /* Structural tokens in passthrough mode are NOT individually tokenised */
    Lexer lex;
    lexer_init(&lex, "{ } ( ) ; , * :");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
    tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_EOF);
}

TEST(lex_passthrough_case_break) {
    /* case and break outside Phoenics context are passthrough */
    Lexer lex;
    lexer_init(&lex, "switch (x) { case 1: break; }");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
}

TEST(lex_passthrough_before_descr) {
    Lexer lex;
    lexer_init(&lex, "#include <stdio.h>\n\nphc_descr Shape { };");
    Token t1 = lexer_next(&lex);
    ASSERT_EQ(t1.type, TOK_OTHER);
    Token t2 = lexer_next(&lex);
    ASSERT_EQ(t2.type, TOK_DESCR);
}

TEST(lex_passthrough_between_constructs) {
    /* Code between two descr declarations is passthrough */
    Lexer lex;
    lexer_init(&lex, "phc_descr A { X { int a; } };\nint gap;\nphc_descr B { Y { int b; } };");
    Token tok;
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_DESCR); /* descr A */
    /* Skip through A's tokens until we're past the construct */
    while (tok.type != TOK_EOF) {
        tok = lexer_next(&lex);
        if (tok.type == TOK_OTHER) break;
        if (tok.type == TOK_DESCR) break; /* hit next descr */
    }
    /* Should have passthrough text or next descr */
    ASSERT(tok.type == TOK_OTHER || tok.type == TOK_DESCR);
}

/* === Line tracking === */

TEST(lex_tracks_line_numbers) {
    Lexer lex;
    lexer_init(&lex, "phc_descr\nShape");
    Token t1 = lexer_next(&lex);
    Token t2 = lexer_next(&lex);
    ASSERT_EQ(t1.line, 1);
    ASSERT_EQ(t2.line, 2);
}

/* === Keyword isolation: MUST NOT match inside strings/comments === */

TEST(lex_descr_in_string_literal) {
    Lexer lex;
    lexer_init(&lex, "char *s = \"phc_descr Shape { };\";");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
    tok = lexer_next(&lex);
    ASSERT(tok.type == TOK_EOF || tok.type == TOK_OTHER);
}

TEST(lex_descr_in_char_literal) {
    Lexer lex;
    lexer_init(&lex, "char c = 'd';");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
}

TEST(lex_descr_in_line_comment) {
    Lexer lex;
    lexer_init(&lex, "// phc_descr Shape { }\nint x;");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
}

TEST(lex_descr_in_block_comment) {
    Lexer lex;
    lexer_init(&lex, "/* phc_descr Shape { } */\nint x;");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
}

TEST(lex_match_descr_in_string) {
    Lexer lex;
    lexer_init(&lex, "printf(\"phc_match test\");");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
}

/* === Edge cases === */

TEST(lex_descr_as_prefix) {
    /* "phc_descr_extended" starts with "phc_descr" — must NOT trigger */
    Lexer lex;
    lexer_init(&lex, "phc_descr_extended");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
}

TEST(lex_match_descr_as_prefix) {
    /* "phc_matcher" starts with "phc_match" — must NOT trigger */
    Lexer lex;
    lexer_init(&lex, "phc_matcher");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
}

TEST(lex_old_keywords_are_passthrough) {
    /* Old 'descr' and 'match_descr' (without phc_ prefix) are NOT keywords */
    Lexer lex;
    lexer_init(&lex, "int descr = 42; match_descr();");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
}

TEST(lex_descr_after_passthrough_returns_to_passthrough) {
    /* After a descr construct ends, the lexer returns to passthrough mode */
    Lexer lex;
    lexer_init(&lex, "phc_descr X { A { int a; } };\nint y = 0;");
    Token tok;
    /* Consume the descr construct */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_DESCR);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* X */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* A */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_LBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* int */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_IDENT); /* a */
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_SEMICOLON);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_RBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_RBRACE);
    tok = lexer_next(&lex); ASSERT_EQ(tok.type, TOK_SEMICOLON);
    /* After the descr, "int y = 0;" should be passthrough */
    tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
}

/* === Preprocessor line marker formats === */

TEST(lex_gcc_line_marker) {
    /* GCC/Clang format: # N "file" [flags]
     * After consuming "# 42 ..." + "\n" + "int x;\n", orig_line
     * advances past the content line, so it ends at 43. */
    Lexer lex;
    lexer_init(&lex, "# 42 \"foo.c\"\nint x;\n");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
    ASSERT_EQ(lex.marker_seen, 1);
    /* orig_line = 42 at start of "int x;", 43 after its \n */
    ASSERT_EQ(lex.orig_line, 43);
    ASSERT(lex.orig_file != NULL);
    ASSERT(lex.orig_file_len == 5);
    ASSERT(memcmp(lex.orig_file, "foo.c", 5) == 0);
}

TEST(lex_msvc_line_marker) {
    /* MSVC format: #line N "file" */
    Lexer lex;
    lexer_init(&lex, "#line 99 \"bar.h\"\nint y;\n");
    Token tok = lexer_next(&lex);
    ASSERT_EQ(tok.type, TOK_OTHER);
    ASSERT_EQ(lex.marker_seen, 1);
    /* Same as GCC: orig_line 99 at content, 100 after \n */
    ASSERT_EQ(lex.orig_line, 100);
    ASSERT(lex.orig_file != NULL);
    ASSERT(lex.orig_file_len == 5);
    ASSERT(memcmp(lex.orig_file, "bar.h", 5) == 0);
}

TEST_MAIN(
    /* Keyword recognition */
    RUN_TEST(lex_descr_keyword);
    RUN_TEST(lex_match_descr_keyword);
    /* Structural tokens in descr context */
    RUN_TEST(lex_braces_in_descr);
    RUN_TEST(lex_semicolon_comma_in_descr);
    RUN_TEST(lex_star_in_descr);
    RUN_TEST(lex_identifier_in_descr);
    /* Structural tokens in match_descr context */
    RUN_TEST(lex_parens_in_match_descr);
    RUN_TEST(lex_case_in_match_descr);
    RUN_TEST(lex_eof);
    /* Full compound sequences */
    RUN_TEST(lex_descr_declaration);
    RUN_TEST(lex_match_descr_full_syntax);
    /* Passthrough mode */
    RUN_TEST(lex_passthrough_pure_c);
    RUN_TEST(lex_passthrough_structural_tokens);
    RUN_TEST(lex_passthrough_case_break);
    RUN_TEST(lex_passthrough_before_descr);
    RUN_TEST(lex_passthrough_between_constructs);
    /* Line tracking */
    RUN_TEST(lex_tracks_line_numbers);
    /* Preprocessor line markers */
    RUN_TEST(lex_gcc_line_marker);
    RUN_TEST(lex_msvc_line_marker);
    /* Keyword isolation */
    RUN_TEST(lex_descr_in_string_literal);
    RUN_TEST(lex_descr_in_char_literal);
    RUN_TEST(lex_descr_in_line_comment);
    RUN_TEST(lex_descr_in_block_comment);
    RUN_TEST(lex_match_descr_in_string);
    /* Edge cases */
    RUN_TEST(lex_descr_as_prefix);
    RUN_TEST(lex_match_descr_as_prefix);
    RUN_TEST(lex_old_keywords_are_passthrough);
    RUN_TEST(lex_descr_after_passthrough_returns_to_passthrough);
)
