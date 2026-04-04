#include "lexer.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

void lexer_init(Lexer *lex, const char *source) {
    lex->src = source;
    lex->pos = 0;
    lex->len = strlen(source);
    lex->line = 1;
    lex->col = 1;
    lex->mode = LEXER_SCAN;
    lex->brace_depth = 0;
    lex->depth_zero_seen = 0;
}

static char peek(const Lexer *lex) {
    if (lex->pos >= lex->len) return '\0';
    return lex->src[lex->pos];
}

static char peek_at(const Lexer *lex, size_t offset) {
    size_t p = lex->pos + offset;
    if (p >= lex->len) return '\0';
    return lex->src[p];
}

static void advance(Lexer *lex) {
    if (lex->pos < lex->len) {
        if (lex->src[lex->pos] == '\n') {
            lex->line++;
            lex->col = 1;
        } else {
            lex->col++;
        }
        lex->pos++;
    }
}

static void skip_whitespace(Lexer *lex) {
    while (lex->pos < lex->len && isspace((unsigned char)peek(lex))) {
        advance(lex);
    }
}

static int is_ident_start(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

static int is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static Token make_token(TokenType type, const char *value, size_t len,
                        size_t start_pos, int line, int col) {
    Token tok;
    tok.type = type;
    tok.value = value;
    tok.length = len;
    tok.line = line;
    tok.col = col;
    tok.pos = start_pos;
    return tok;
}

/* Skip over a string literal starting at " (advances past closing ") */
static void skip_string(Lexer *lex) {
    advance(lex); /* skip opening " */
    while (lex->pos < lex->len) {
        char c = peek(lex);
        if (c == '\\') { advance(lex); advance(lex); }
        else if (c == '"') { advance(lex); return; }
        else { advance(lex); }
    }
}

/* Skip over a character literal starting at ' */
static void skip_char_lit(Lexer *lex) {
    advance(lex); /* skip opening ' */
    while (lex->pos < lex->len) {
        char c = peek(lex);
        if (c == '\\') { advance(lex); advance(lex); }
        else if (c == '\'') { advance(lex); return; }
        else { advance(lex); }
    }
}

/* Skip over a block comment starting at / * */
static void skip_block_comment(Lexer *lex) {
    advance(lex); advance(lex); /* skip / * */
    while (lex->pos < lex->len) {
        if (peek(lex) == '*' && peek_at(lex, 1) == '/') {
            advance(lex); advance(lex);
            return;
        }
        advance(lex);
    }
}

/* Skip over a line comment starting at // */
static void skip_line_comment(Lexer *lex) {
    advance(lex); advance(lex); /* skip // */
    while (lex->pos < lex->len && peek(lex) != '\n') {
        advance(lex);
    }
}

/* Check if an identifier at `start` of length `ident_len` matches `keyword` exactly,
 * with word boundary after it. */
static int check_keyword(const Lexer *lex, size_t start, size_t ident_len,
                          const char *keyword, size_t kw_len) {
    if (ident_len != kw_len) return 0;
    if (memcmp(lex->src + start, keyword, kw_len) != 0) return 0;
    /* Check word boundary after */
    size_t after = start + kw_len;
    if (after >= lex->len) return 1;
    return !is_ident_char(lex->src[after]);
}

/* ===== SCAN MODE ===== */

static Token lexer_next_scan(Lexer *lex) {
    size_t start = lex->pos;
    int sline = lex->line;
    int scol = lex->col;

    while (lex->pos < lex->len) {
        char c = peek(lex);

        /* String literal — consume without checking for keywords inside */
        if (c == '"') { skip_string(lex); continue; }

        /* Character literal */
        if (c == '\'') { skip_char_lit(lex); continue; }

        /* Block comment */
        if (c == '/' && peek_at(lex, 1) == '*') { skip_block_comment(lex); continue; }

        /* Line comment */
        if (c == '/' && peek_at(lex, 1) == '/') { skip_line_comment(lex); continue; }

        /* Check for identifier / keyword */
        if (is_ident_start(c)) {
            size_t id_start = lex->pos;
            /* Peek ahead to measure the full identifier length */
            size_t id_end = id_start;
            while (id_end < lex->len && is_ident_char(lex->src[id_end])) {
                id_end++;
            }
            size_t id_len = id_end - id_start;

            /* Check for Phoenics keywords */
            if (check_keyword(lex, id_start, id_len, "descr", 5) ||
                check_keyword(lex, id_start, id_len, "match_descr", 11)) {
                /* Keyword found! Emit TOK_OTHER for text before it, if any */
                if (id_start > start) {
                    return make_token(TOK_OTHER, lex->src + start,
                                      id_start - start, start, sline, scol);
                }
                /* Emit the keyword token */
                /* Advance past the keyword */
                int kline = lex->line;
                int kcol = lex->col;
                while (lex->pos < id_end) advance(lex);

                lex->mode = LEXER_STRUCT;
                lex->brace_depth = 0;
                lex->depth_zero_seen = 0;

                if (id_len == 5) {
                    return make_token(TOK_DESCR, lex->src + id_start,
                                      id_len, id_start, kline, kcol);
                } else {
                    return make_token(TOK_MATCH_DESCR, lex->src + id_start,
                                      id_len, id_start, kline, kcol);
                }
            }

            /* Not a keyword — skip the identifier and continue scanning */
            while (lex->pos < id_end) advance(lex);
            continue;
        }

        /* Any other character — just advance */
        advance(lex);
    }

    /* Reached EOF — emit remaining text as TOK_OTHER, or EOF */
    if (lex->pos > start) {
        return make_token(TOK_OTHER, lex->src + start,
                          lex->pos - start, start, sline, scol);
    }
    return make_token(TOK_EOF, NULL, 0, lex->pos, lex->line, lex->col);
}

/* ===== STRUCT MODE ===== */

static Token lexer_next_struct(Lexer *lex) {
    skip_whitespace(lex);

    if (lex->pos >= lex->len) {
        return make_token(TOK_EOF, NULL, 0, lex->pos, lex->line, lex->col);
    }

    /* If depth returned to 0 on the previous }, check for trailing ; then go to SCAN */
    if (lex->depth_zero_seen) {
        lex->depth_zero_seen = 0;
        if (peek(lex) == ';') {
            size_t start = lex->pos;
            int sline = lex->line;
            int scol = lex->col;
            advance(lex);
            lex->mode = LEXER_SCAN;
            return make_token(TOK_SEMICOLON, NULL, 1, start, sline, scol);
        }
        /* No semicolon — return to SCAN */
        lex->mode = LEXER_SCAN;
        return lexer_next_scan(lex);
    }

    size_t start = lex->pos;
    int sline = lex->line;
    int scol = lex->col;
    char c = peek(lex);

    /* String literal in struct mode → TOK_OTHER */
    if (c == '"') {
        skip_string(lex);
        return make_token(TOK_OTHER, lex->src + start, lex->pos - start,
                          start, sline, scol);
    }

    /* Character literal → TOK_OTHER */
    if (c == '\'') {
        skip_char_lit(lex);
        return make_token(TOK_OTHER, lex->src + start, lex->pos - start,
                          start, sline, scol);
    }

    /* Comments → TOK_OTHER */
    if (c == '/' && peek_at(lex, 1) == '*') {
        skip_block_comment(lex);
        return make_token(TOK_OTHER, lex->src + start, lex->pos - start,
                          start, sline, scol);
    }
    if (c == '/' && peek_at(lex, 1) == '/') {
        skip_line_comment(lex);
        return make_token(TOK_OTHER, lex->src + start, lex->pos - start,
                          start, sline, scol);
    }

    /* Structural tokens */
    switch (c) {
    case '{':
        advance(lex);
        lex->brace_depth++;
        return make_token(TOK_LBRACE, NULL, 1, start, sline, scol);
    case '}':
        advance(lex);
        lex->brace_depth--;
        if (lex->brace_depth <= 0) {
            lex->depth_zero_seen = 1;
        }
        return make_token(TOK_RBRACE, NULL, 1, start, sline, scol);
    case '(':
        advance(lex);
        return make_token(TOK_LPAREN, NULL, 1, start, sline, scol);
    case ')':
        advance(lex);
        return make_token(TOK_RPAREN, NULL, 1, start, sline, scol);
    case ';':
        advance(lex);
        return make_token(TOK_SEMICOLON, NULL, 1, start, sline, scol);
    case ',':
        advance(lex);
        return make_token(TOK_COMMA, NULL, 1, start, sline, scol);
    case '*':
        advance(lex);
        return make_token(TOK_STAR, NULL, 1, start, sline, scol);
    case ':':
        advance(lex);
        return make_token(TOK_COLON, NULL, 1, start, sline, scol);
    default:
        break;
    }

    /* Identifiers and keywords */
    if (is_ident_start(c)) {
        while (lex->pos < lex->len && is_ident_char(peek(lex))) {
            advance(lex);
        }
        size_t len = lex->pos - start;
        const char *val = lex->src + start;

        if (len == 5 && memcmp(val, "descr", 5) == 0)
            return make_token(TOK_DESCR, val, len, start, sline, scol);
        if (len == 11 && memcmp(val, "match_descr", 11) == 0)
            return make_token(TOK_MATCH_DESCR, val, len, start, sline, scol);
        if (len == 4 && memcmp(val, "case", 4) == 0)
            return make_token(TOK_CASE, val, len, start, sline, scol);
        if (len == 5 && memcmp(val, "break", 5) == 0)
            return make_token(TOK_BREAK, val, len, start, sline, scol);

        return make_token(TOK_IDENT, val, len, start, sline, scol);
    }

    /* Everything else in struct mode → single char TOK_OTHER */
    advance(lex);
    return make_token(TOK_OTHER, lex->src + start, 1, start, sline, scol);
}

/* ===== Public API ===== */

Token lexer_next(Lexer *lex) {
    if (lex->mode == LEXER_SCAN) {
        return lexer_next_scan(lex);
    } else {
        return lexer_next_struct(lex);
    }
}

Token lexer_peek(Lexer *lex) {
    /* Save full state */
    size_t saved_pos = lex->pos;
    int saved_line = lex->line;
    int saved_col = lex->col;
    LexerMode saved_mode = lex->mode;
    int saved_depth = lex->brace_depth;
    int saved_dzs = lex->depth_zero_seen;

    Token tok = lexer_next(lex);

    /* Restore state */
    lex->pos = saved_pos;
    lex->line = saved_line;
    lex->col = saved_col;
    lex->mode = saved_mode;
    lex->brace_depth = saved_depth;
    lex->depth_zero_seen = saved_dzs;

    return tok;
}
