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
    lex->defer_active = 0;
    lex->scan_brace_depth = 0;
    lex->orig_line = 1;
    lex->orig_file = NULL;
    lex->orig_file_len = 0;
    lex->marker_seen = 0;
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
            if (lex->marker_seen) lex->orig_line++;
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
                        size_t start_pos, int line, int col, int orig_line) {
    Token tok;
    tok.type = type;
    tok.value = value;
    tok.length = len;
    tok.line = line;
    tok.col = col;
    tok.pos = start_pos;
    tok.orig_line = orig_line;
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
    int sorig = lex->orig_line;

    while (lex->pos < lex->len) {
        char c = peek(lex);

        /* Preprocessor line marker:
         *   GCC/Clang: # <number> "file" [flags]
         *   MSVC:      #line <number> "file"
         */
        if (c == '#' && lex->col == 1) {
            size_t p = lex->pos + 1;
            while (p < lex->len && lex->src[p] == ' ') p++;
            /* MSVC format: skip 'line' keyword if present */
            if (p + 4 <= lex->len &&
                lex->src[p] == 'l' && lex->src[p+1] == 'i' &&
                lex->src[p+2] == 'n' && lex->src[p+3] == 'e' &&
                (p + 4 >= lex->len || lex->src[p+4] == ' ' || lex->src[p+4] == '\t')) {
                p += 4;
                while (p < lex->len && lex->src[p] == ' ') p++;
            }
            if (p < lex->len && isdigit((unsigned char)lex->src[p])) {
                int line_num = 0;
                while (p < lex->len && isdigit((unsigned char)lex->src[p])) {
                    line_num = line_num * 10 + (lex->src[p] - '0');
                    p++;
                }
                /* Optional filename in quotes */
                while (p < lex->len && lex->src[p] == ' ') p++;
                if (p < lex->len && lex->src[p] == '"') {
                    p++;
                    lex->orig_file = lex->src + p;
                    while (p < lex->len && lex->src[p] != '"') p++;
                    lex->orig_file_len = (size_t)(lex->src + p - lex->orig_file);
                }
                /* Set orig_line to line_num - 1: the \n at end of marker
                 * will increment it to line_num via advance() */
                lex->orig_line = line_num - 1;
                lex->marker_seen = 1;
                /* Advance to end of marker line (stays as passthrough) */
                while (lex->pos < lex->len && peek(lex) != '\n') advance(lex);
                continue;
            }
        }

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

            /* Check for Phoenics keywords (phc_ prefixed — no collision risk) */
            if (check_keyword(lex, id_start, id_len, "phc_descr", 9) ||
                check_keyword(lex, id_start, id_len, "phc_enum", 8) ||
                check_keyword(lex, id_start, id_len, "phc_match", 9) ||
                check_keyword(lex, id_start, id_len, "phc_defer", 9) ||
                check_keyword(lex, id_start, id_len, "phc_defer_cancel", 16)) {
                /* Keyword found! Emit TOK_OTHER for text before it, if any */
                if (id_start > start) {
                    return make_token(TOK_OTHER, lex->src + start,
                                      id_start - start, start, sline, scol, sorig);
                }
                /* Emit the keyword token */
                int kline = lex->line;
                int kcol = lex->col;
                int korig = lex->orig_line;
                while (lex->pos < id_end) advance(lex);

                lex->mode = LEXER_STRUCT;
                lex->brace_depth = 0;
                lex->depth_zero_seen = 0;

                if (id_len == 16 && check_keyword(lex, id_start, id_len, "phc_defer_cancel", 16)) {
                    /* phc_defer_cancel — not a struct-mode keyword */
                    lex->mode = LEXER_SCAN;
                    return make_token(TOK_PHC_DEFER_CANCEL, lex->src + id_start,
                                      id_len, id_start, kline, kcol, korig);
                } else if (id_len == 9 && lex->src[id_start + 4] == 'd' &&
                    lex->src[id_start + 6] == 'f') {
                    /* phc_defer */
                    return make_token(TOK_PHC_DEFER, lex->src + id_start,
                                      id_len, id_start, kline, kcol, korig);
                } else if (id_len == 8 && check_keyword(lex, id_start, id_len, "phc_enum", 8)) {
                    /* phc_enum */
                    return make_token(TOK_ENUM, lex->src + id_start,
                                      id_len, id_start, kline, kcol, korig);
                } else if (id_len == 9 && lex->src[id_start + 4] == 'd') {
                    return make_token(TOK_DESCR, lex->src + id_start,
                                      id_len, id_start, kline, kcol, korig);
                } else {
                    return make_token(TOK_MATCH_DESCR, lex->src + id_start,
                                      id_len, id_start, kline, kcol, korig);
                }
            }

            /* Check for 'return' keyword when defer is active */
            if (lex->defer_active &&
                check_keyword(lex, id_start, id_len, "return", 6)) {
                if (id_start > start) {
                    return make_token(TOK_OTHER, lex->src + start,
                                      id_start - start, start, sline, scol, sorig);
                }
                int kline = lex->line;
                int kcol = lex->col;
                int korig = lex->orig_line;
                while (lex->pos < id_end) advance(lex);
                return make_token(TOK_RETURN, lex->src + id_start,
                                  id_len, id_start, kline, kcol, korig);
            }

            /* Not a keyword — skip the identifier and continue scanning */
            while (lex->pos < id_end) advance(lex);
            continue;
        }

        /* Track brace depth in scan mode for defer function boundary */
        if (c == '{') lex->scan_brace_depth++;
        else if (c == '}') {
            lex->scan_brace_depth--;
            if (lex->scan_brace_depth <= 0 && lex->defer_active) {
                /* Function closing brace — emit text before it, then the brace */
                if (lex->pos > start) {
                    /* Return accumulated text first; } will be emitted next call */
                    return make_token(TOK_OTHER, lex->src + start,
                                      lex->pos - start, start, sline, scol, sorig);
                }
                /* Return the closing brace as TOK_RBRACE */
                lex->defer_active = 0;
                int kline = lex->line;
                int kcol = lex->col;
                int korig = lex->orig_line;
                advance(lex);
                return make_token(TOK_RBRACE, lex->src + lex->pos - 1,
                                  1, lex->pos - 1, kline, kcol, korig);
            }
        }

        /* Any other character — just advance */
        advance(lex);
    }

    /* Reached EOF — emit remaining text as TOK_OTHER, or EOF */
    if (lex->pos > start) {
        return make_token(TOK_OTHER, lex->src + start,
                          lex->pos - start, start, sline, scol, sorig);
    }
    return make_token(TOK_EOF, NULL, 0, lex->pos, lex->line, lex->col, lex->orig_line);
}

/* ===== STRUCT MODE ===== */

static Token lexer_next_struct(Lexer *lex) {
    skip_whitespace(lex);

    if (lex->pos >= lex->len) {
        return make_token(TOK_EOF, NULL, 0, lex->pos, lex->line, lex->col, lex->orig_line);
    }

    /* If depth returned to 0 on the previous }, check for trailing ; then go to SCAN */
    if (lex->depth_zero_seen) {
        lex->depth_zero_seen = 0;
        if (peek(lex) == ';') {
            size_t start = lex->pos;
            int sline = lex->line;
            int scol = lex->col;
            int sorig = lex->orig_line;
            advance(lex);
            lex->mode = LEXER_SCAN;
            return make_token(TOK_SEMICOLON, NULL, 1, start, sline, scol, sorig);
        }
        /* No semicolon — return to SCAN */
        lex->mode = LEXER_SCAN;
        return lexer_next_scan(lex);
    }

    size_t start = lex->pos;
    int sline = lex->line;
    int scol = lex->col;
    int sorig = lex->orig_line;
    char c = peek(lex);

    /* String literal in struct mode → TOK_OTHER */
    if (c == '"') {
        skip_string(lex);
        return make_token(TOK_OTHER, lex->src + start, lex->pos - start,
                          start, sline, scol, sorig);
    }

    /* Character literal → TOK_OTHER */
    if (c == '\'') {
        skip_char_lit(lex);
        return make_token(TOK_OTHER, lex->src + start, lex->pos - start,
                          start, sline, scol, sorig);
    }

    /* Comments → TOK_OTHER */
    if (c == '/' && peek_at(lex, 1) == '*') {
        skip_block_comment(lex);
        return make_token(TOK_OTHER, lex->src + start, lex->pos - start,
                          start, sline, scol, sorig);
    }
    if (c == '/' && peek_at(lex, 1) == '/') {
        skip_line_comment(lex);
        return make_token(TOK_OTHER, lex->src + start, lex->pos - start,
                          start, sline, scol, sorig);
    }

    /* Structural tokens */
    switch (c) {
    case '{':
        advance(lex);
        lex->brace_depth++;
        return make_token(TOK_LBRACE, NULL, 1, start, sline, scol, sorig);
    case '}':
        advance(lex);
        lex->brace_depth--;
        if (lex->brace_depth <= 0) {
            lex->depth_zero_seen = 1;
        }
        return make_token(TOK_RBRACE, NULL, 1, start, sline, scol, sorig);
    case '(':
        advance(lex);
        return make_token(TOK_LPAREN, NULL, 1, start, sline, scol, sorig);
    case ')':
        advance(lex);
        return make_token(TOK_RPAREN, NULL, 1, start, sline, scol, sorig);
    case ';':
        advance(lex);
        return make_token(TOK_SEMICOLON, NULL, 1, start, sline, scol, sorig);
    case ',':
        advance(lex);
        return make_token(TOK_COMMA, NULL, 1, start, sline, scol, sorig);
    case '*':
        advance(lex);
        return make_token(TOK_STAR, NULL, 1, start, sline, scol, sorig);
    case ':':
        advance(lex);
        return make_token(TOK_COLON, NULL, 1, start, sline, scol, sorig);
    default:
        break;
    }

    /* Equals sign (for explicit enum values) */
    if (c == '=') {
        advance(lex);
        return make_token(TOK_EQUALS, lex->src + start, 1, start, sline, scol, sorig);
    }

    /* Number literals */
    if (isdigit((unsigned char)c) || (c == '-' && isdigit((unsigned char)peek_at(lex, 1)))) {
        /* Consume optional sign + digits (decimal) or 0x prefix (hex) */
        if (c == '-') advance(lex);
        if (peek(lex) == '0' && (peek_at(lex, 1) == 'x' || peek_at(lex, 1) == 'X')) {
            advance(lex); advance(lex); /* skip 0x */
            while (lex->pos < lex->len && isxdigit((unsigned char)peek(lex))) advance(lex);
        } else {
            while (lex->pos < lex->len && isdigit((unsigned char)peek(lex))) advance(lex);
        }
        return make_token(TOK_NUMBER, lex->src + start, lex->pos - start,
                          start, sline, scol, sorig);
    }

    /* Identifiers and keywords */
    if (is_ident_start(c)) {
        while (lex->pos < lex->len && is_ident_char(peek(lex))) {
            advance(lex);
        }
        size_t len = lex->pos - start;
        const char *val = lex->src + start;

        if (len == 9 && memcmp(val, "phc_descr", 9) == 0)
            return make_token(TOK_DESCR, val, len, start, sline, scol, sorig);
        if (len == 9 && memcmp(val, "phc_match", 9) == 0)
            return make_token(TOK_MATCH_DESCR, val, len, start, sline, scol, sorig);
        if (len == 4 && memcmp(val, "case", 4) == 0)
            return make_token(TOK_CASE, val, len, start, sline, scol, sorig);
        if (len == 5 && memcmp(val, "break", 5) == 0)
            return make_token(TOK_BREAK, val, len, start, sline, scol, sorig);

        return make_token(TOK_IDENT, val, len, start, sline, scol, sorig);
    }

    /* Everything else in struct mode → single char TOK_OTHER */
    advance(lex);
    return make_token(TOK_OTHER, lex->src + start, 1, start, sline, scol, sorig);
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
