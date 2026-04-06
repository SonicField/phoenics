#include "compat.h"
#include "parser.h"
#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* --- Dynamic array helpers --- */

#define DA_INIT_CAP 8

#define DA_PUSH(arr, count, cap, item) do { \
    if ((count) >= (cap)) { \
        (cap) = (cap) ? (cap) * 2 : DA_INIT_CAP; \
        (arr) = realloc((arr), sizeof(*(arr)) * (size_t)(cap)); \
    } \
    (arr)[(count)++] = (item); \
} while(0)

/* --- Parser state --- */

typedef struct {
    Lexer lex;
    Token cur;
    int error;
    char error_msg[512];
    int error_line;
    const char *source;
    size_t source_len;

    DescrDecl *descrs;
    int descr_count;
    int descr_cap;

    Chunk *chunks;
    int chunk_count;
    int chunk_cap;

    DeferBlock *defers;
    int defer_count;
    int defer_cap;
    int active_defer_count; /* defers active in current function scope */

    size_t passthrough_start;
} Parser;

static void parser_init(Parser *p, const char *source) {
    memset(p, 0, sizeof(*p));
    p->source = source;
    p->source_len = strlen(source);
    lexer_init(&p->lex, source);
    p->cur = lexer_next(&p->lex);
    p->passthrough_start = 0;
}

static void parser_error(Parser *p, const char *fmt, ...) {
    if (p->error) return;
    p->error = 1;
    p->error_line = p->cur.line;
    va_list args;
    va_start(args, fmt);
    vsnprintf(p->error_msg, sizeof(p->error_msg), fmt, args);
    va_end(args);
}

static void next_token(Parser *p) {
    p->cur = lexer_next(&p->lex);
}

static int expect(Parser *p, TokenType type) {
    if (p->cur.type != type) {
        parser_error(p, "expected token type %d, got %d", type, p->cur.type);
        return 0;
    }
    next_token(p);
    return 1;
}

static void add_passthrough(Parser *p, size_t start, size_t end) {
    if (start >= end) return;
    Chunk c;
    memset(&c, 0, sizeof(c));
    c.type = CHUNK_PASSTHROUGH;
    c.passthrough.start = start;
    c.passthrough.end = end;
    DA_PUSH(p->chunks, p->chunk_count, p->chunk_cap, c);
}

/* --- Field parsing --- */

static int parse_field(Parser *p, Field *f) {
    size_t field_start = p->cur.pos;
    Token tokens[64];
    int count = 0;

    while (p->cur.type != TOK_SEMICOLON && p->cur.type != TOK_EOF &&
           p->cur.type != TOK_RBRACE) {
        if (count < 64) tokens[count] = p->cur;
        count++;
        next_token(p);
    }
    if (count > 64) count = 64;

    if (count < 2) {
        parser_error(p, "invalid field declaration: need at least type and name");
        return 0;
    }
    if (p->cur.type != TOK_SEMICOLON) {
        parser_error(p, "expected ';' after field declaration");
        return 0;
    }

    /* Capture raw declaration text (field_start to just before ;) */
    size_t field_end = p->cur.pos;
    while (field_end > field_start &&
           (p->source[field_end - 1] == ' ' || p->source[field_end - 1] == '\t'))
        field_end--;
    f->raw_decl = strndup(p->source + field_start, field_end - field_start);

    next_token(p);

    /* Extract field name with heuristic */
    int name_idx = -1;
    f->is_array = 0;

    /* 1. Function pointer: ( * IDENT ) */
    for (int i = 0; i < count - 2; i++) {
        if (tokens[i].type == TOK_LPAREN && tokens[i + 1].type == TOK_STAR &&
            tokens[i + 2].type == TOK_IDENT) {
            name_idx = i + 2;
            break;
        }
    }

    /* 2. Array: IDENT followed by '[' (TOK_OTHER '[') */
    if (name_idx < 0) {
        for (int i = 0; i < count - 1; i++) {
            if (tokens[i].type == TOK_IDENT && tokens[i + 1].type == TOK_OTHER &&
                tokens[i + 1].length == 1 && tokens[i + 1].value[0] == '[') {
                name_idx = i;
                f->is_array = 1;
                break;
            }
        }
    }

    /* 3. Simple: last IDENT token */
    if (name_idx < 0) {
        for (int i = count - 1; i >= 0; i--) {
            if (tokens[i].type == TOK_IDENT) { name_idx = i; break; }
        }
    }

    if (name_idx < 0) {
        parser_error(p, "cannot find field name in declaration");
        free(f->raw_decl);
        return 0;
    }
    f->field_name = strndup(tokens[name_idx].value, tokens[name_idx].length);

    /* Build type_name for simple types (IDENT/STAR only before name) */
    int simple = 1;
    for (int i = 0; i < name_idx; i++) {
        if (tokens[i].type != TOK_IDENT && tokens[i].type != TOK_STAR) {
            simple = 0;
            break;
        }
    }
    if (simple) {
        size_t type_len = 0;
        char *type_buf = malloc(256);
        for (int i = 0; i < name_idx; i++) {
            if (tokens[i].type == TOK_STAR) {
                if (type_len > 0 && type_buf[type_len - 1] != ' ')
                    type_buf[type_len++] = ' ';
                type_buf[type_len++] = '*';
            } else {
                if (type_len > 0 && type_buf[type_len - 1] != '*')
                    type_buf[type_len++] = ' ';
                memcpy(type_buf + type_len, tokens[i].value, tokens[i].length);
                type_len += tokens[i].length;
            }
        }
        type_buf[type_len] = '\0';
        f->type_name = type_buf;
    } else {
        f->type_name = NULL;
    }

    return 1;
}

/* --- cleanup helpers --- */

static void free_variant_contents(Variant *v) {
    for (int i = 0; i < v->field_count; i++) {
        free(v->fields[i].type_name);
        free(v->fields[i].field_name);
        free(v->fields[i].raw_decl);
    }
    free(v->fields);
    free(v->name);
}

static void free_descr_contents(DescrDecl *d) {
    for (int i = 0; i < d->variant_count; i++)
        free_variant_contents(&d->variants[i]);
    free(d->variants);
    free(d->name);
}

static void free_match_contents(MatchDescr *m) {
    for (int i = 0; i < m->case_count; i++) {
        free(m->cases[i].variant_name);
        free(m->cases[i].body_text);
        /* Free recursively parsed body chunks */
        if (m->cases[i].body_chunks) {
            for (int j = 0; j < m->cases[i].body_chunk_count; j++) {
                Chunk *bc = &m->cases[i].body_chunks[j];
                if (bc->type == CHUNK_MATCH_DESCR) {
                    free(bc->match.type_name);
                    free(bc->match.expr_text);
                    free(bc->match.end_file);
                    /* Recursive: cases in nested match also need freeing */
                    for (int k = 0; k < bc->match.case_count; k++) {
                        free(bc->match.cases[k].variant_name);
                        free(bc->match.cases[k].body_text);
                        free(bc->match.cases[k].body_chunks);
                        for (int b = 0; b < bc->match.cases[k].binding_count; b++)
                            free(bc->match.cases[k].bindings[b]);
                        free(bc->match.cases[k].bindings);
                    }
                    free(bc->match.cases);
                } else if (bc->type == CHUNK_RETURN) {
                    free(bc->ret.expr);
                } else if (bc->type == CHUNK_DEFER) {
                    free(bc->defer.body_text);
                }
            }
            free(m->cases[i].body_chunks);
        }
        for (int b = 0; b < m->cases[i].binding_count; b++)
            free(m->cases[i].bindings[b]);
        free(m->cases[i].bindings);
    }
    free(m->cases);
    free(m->type_name);
    free(m->expr_text);
}

/* --- descr parsing --- */

static int parse_variant(Parser *p, Variant *v) {
    if (p->cur.type != TOK_IDENT) {
        parser_error(p, "expected variant name");
        return 0;
    }
    v->name = strndup(p->cur.value, p->cur.length);
    v->fields = NULL;
    v->field_count = 0;
    next_token(p);

    if (!expect(p, TOK_LBRACE)) { free(v->name); return 0; }

    int field_cap = 0;
    while (p->cur.type != TOK_RBRACE && p->cur.type != TOK_EOF) {
        Field f;
        if (!parse_field(p, &f)) { free_variant_contents(v); return 0; }
        DA_PUSH(v->fields, v->field_count, field_cap, f);
    }

    if (!expect(p, TOK_RBRACE)) { free_variant_contents(v); return 0; }
    return 1;
}

static int parse_descr(Parser *p) {
    if (p->cur.type != TOK_IDENT) {
        parser_error(p, "expected type name after 'descr'");
        return 0;
    }

    DescrDecl d;
    memset(&d, 0, sizeof(d));
    d.name = strndup(p->cur.value, p->cur.length);
    next_token(p);

    /* Forward declaration: phc_descr Name; */
    if (p->cur.type == TOK_SEMICOLON) {
        size_t end_pos = p->cur.pos + 1;
        next_token(p);

        d.variant_count = -1; /* sentinel: forward declaration */
        int idx = p->descr_count;
        DA_PUSH(p->descrs, p->descr_count, p->descr_cap, d);

        Chunk c;
        memset(&c, 0, sizeof(c));
        c.type = CHUNK_DESCR;
        c.descr_index = idx;
        DA_PUSH(p->chunks, p->chunk_count, p->chunk_cap, c);

        p->passthrough_start = end_pos;
        return 1;
    }

    if (p->cur.type != TOK_LBRACE) {
        parser_error(p, "expected '{' after descr type name");
        free(d.name);
        return 0;
    }
    next_token(p);

    int variant_cap = 0;
    int first = 1;
    while (p->cur.type != TOK_RBRACE && p->cur.type != TOK_EOF) {
        if (!first) {
            if (p->cur.type == TOK_COMMA) {
                next_token(p);
                if (p->cur.type == TOK_RBRACE) break;
            }
        }
        first = 0;
        Variant v;
        if (!parse_variant(p, &v)) { free_descr_contents(&d); return 0; }
        DA_PUSH(d.variants, d.variant_count, variant_cap, v);
    }

    if (d.variant_count == 0) {
        parser_error(p, "descr '%s' must have at least one variant", d.name);
        free_descr_contents(&d);
        return 0;
    }

    if (!expect(p, TOK_RBRACE)) { free_descr_contents(&d); return 0; }
    if (p->cur.type != TOK_SEMICOLON) {
        parser_error(p, "expected ';' after descr declaration");
        free_descr_contents(&d);
        return 0;
    }
    size_t end_pos = p->cur.pos + 1;
    next_token(p);
    if (p->lex.marker_seen) {
        d.end_line = p->cur.orig_line + 1;
        d.end_file = strndup(p->lex.orig_file, p->lex.orig_file_len);
    } else {
        d.end_line = p->cur.line + 1;
        d.end_file = NULL;
    }

    int idx = p->descr_count;
    DA_PUSH(p->descrs, p->descr_count, p->descr_cap, d);

    Chunk c;
    memset(&c, 0, sizeof(c));
    c.type = CHUNK_DESCR;
    c.descr_index = idx;
    DA_PUSH(p->chunks, p->chunk_count, p->chunk_cap, c);

    p->passthrough_start = end_pos;
    return 1;
}

/* --- match_descr parsing --- */

static int parse_match_descr(Parser *p, size_t keyword_pos) {
    MatchDescr m;
    memset(&m, 0, sizeof(m));
    (void)keyword_pos;

    if (p->cur.type != TOK_LPAREN) {
        parser_error(p, "expected '(' after 'match_descr'");
        return 0;
    }
    next_token(p);

    if (p->cur.type != TOK_IDENT) {
        parser_error(p, "expected type name in match_descr");
        return 0;
    }
    m.type_name = strndup(p->cur.value, p->cur.length);
    next_token(p);

    if (p->cur.type != TOK_COMMA) {
        parser_error(p, "expected ',' after type name in match_descr");
        free(m.type_name);
        return 0;
    }
    next_token(p);

    /* Capture expression text until matching ) */
    size_t expr_start = p->cur.pos;
    int paren_depth = 0;
    while (p->cur.type != TOK_EOF) {
        if (p->cur.type == TOK_LPAREN) paren_depth++;
        if (p->cur.type == TOK_RPAREN) {
            if (paren_depth == 0) break;
            paren_depth--;
        }
        next_token(p);
    }
    size_t expr_end = p->cur.pos;
    while (expr_end > expr_start &&
           (p->source[expr_end - 1] == ' ' || p->source[expr_end - 1] == '\t')) {
        expr_end--;
    }
    m.expr_text = strndup(p->source + expr_start, expr_end - expr_start);

    if (p->cur.type != TOK_RPAREN) {
        parser_error(p, "expected ')' in match_descr");
        free(m.type_name);
        free(m.expr_text);
        return 0;
    }
    next_token(p);

    if (p->cur.type != TOK_LBRACE) {
        parser_error(p, "expected '{' in match_descr");
        free(m.type_name);
        free(m.expr_text);
        return 0;
    }
    next_token(p);

    /* Parse cases (skip comments/other tokens between cases) */
    int case_cap = 0;
    while (p->cur.type == TOK_OTHER) next_token(p);
    while (p->cur.type == TOK_CASE) {
        next_token(p); /* consume 'case' */

        if (p->cur.type != TOK_IDENT) {
            parser_error(p, "expected variant name after 'case' in match_descr");
            free_match_contents(&m);
            return 0;
        }

        MatchCase mc;
        memset(&mc, 0, sizeof(mc));
        mc.variant_name = strndup(p->cur.value, p->cur.length);
        next_token(p);

        /* Optional destructuring: case Variant(field1, field2): */
        if (p->cur.type == TOK_LPAREN) {
            next_token(p);
            int bind_cap = 0;
            while (p->cur.type != TOK_RPAREN && p->cur.type != TOK_EOF) {
                if (p->cur.type != TOK_IDENT) {
                    parser_error(p, "expected field name in destructuring");
                    free(mc.variant_name);
                    for (int b = 0; b < mc.binding_count; b++) free(mc.bindings[b]);
                    free(mc.bindings);
                    free_match_contents(&m);
                    return 0;
                }
                char *name = strndup(p->cur.value, p->cur.length);
                DA_PUSH(mc.bindings, mc.binding_count, bind_cap, name);
                next_token(p);
                if (p->cur.type == TOK_COMMA) next_token(p);
            }
            if (p->cur.type != TOK_RPAREN) {
                parser_error(p, "expected ')' after destructuring fields");
                free(mc.variant_name);
                for (int b = 0; b < mc.binding_count; b++) free(mc.bindings[b]);
                free(mc.bindings);
                free_match_contents(&m);
                return 0;
            }
            next_token(p);
        }

        if (p->cur.type != TOK_COLON) {
            parser_error(p, "expected ':' after variant name in match_descr case");
            free(mc.variant_name);
            for (int b = 0; b < mc.binding_count; b++) free(mc.bindings[b]);
            free(mc.bindings);
            free_match_contents(&m);
            return 0;
        }
        next_token(p);

        /* Capture case body text: from current pos to after } [break;] */
        size_t body_start = p->cur.pos;

        if (p->cur.type != TOK_LBRACE) {
            parser_error(p, "expected '{' to open case body in match_descr");
            free(mc.variant_name);
            free_match_contents(&m);
            return 0;
        }

        int depth = 0;
        while (p->cur.type != TOK_EOF) {
            if (p->cur.type == TOK_LBRACE) depth++;
            if (p->cur.type == TOK_RBRACE) {
                depth--;
                if (depth == 0) {
                    next_token(p);
                    break;
                }
            }
            next_token(p);
        }

        /* Optional break; after } */
        if (p->cur.type == TOK_BREAK) {
            next_token(p);
            if (p->cur.type == TOK_SEMICOLON) {
                next_token(p);
            }
        }

        size_t body_end = p->cur.pos;
        /* Trim trailing whitespace from body */
        while (body_end > body_start &&
               (p->source[body_end - 1] == ' ' || p->source[body_end - 1] == '\t' ||
                p->source[body_end - 1] == '\n' || p->source[body_end - 1] == '\r')) {
            body_end--;
        }
        mc.body_text = strndup(p->source + body_start, body_end - body_start);

        /* Re-parse body text to extract phc constructs (recursive body parsing) */
        if (mc.body_text) {
            ParseResult sub = parse(mc.body_text);
            if (!sub.error && sub.program.chunk_count > 0) {
                mc.body_chunks = sub.program.chunks;
                mc.body_chunk_count = sub.program.chunk_count;
                /* Transfer chunk ownership — NULL out before freeing */
                sub.program.chunks = NULL;
                sub.program.chunk_count = 0;
            }
            parse_result_free(&sub);
        }

        DA_PUSH(m.cases, m.case_count, case_cap, mc);
        while (p->cur.type == TOK_OTHER) next_token(p);
    }

    if (p->cur.type != TOK_RBRACE) {
        parser_error(p, "expected '}' to close match_descr");
        free_match_contents(&m);
        return 0;
    }
    m.end_pos = p->cur.pos + 1;
    next_token(p);
    if (p->lex.marker_seen) {
        m.end_line = p->cur.orig_line;
        m.end_file = strndup(p->lex.orig_file, p->lex.orig_file_len);
    } else {
        m.end_line = p->cur.line;
        m.end_file = NULL;
    }

    Chunk c;
    memset(&c, 0, sizeof(c));
    c.type = CHUNK_MATCH_DESCR;
    c.match = m;
    DA_PUSH(p->chunks, p->chunk_count, p->chunk_cap, c);

    p->passthrough_start = m.end_pos;
    return 1;
}

/* --- Main parse loop --- */

ParseResult parse(const char *source) {
    Parser p;
    parser_init(&p, source);

    while (p.cur.type != TOK_EOF && !p.error) {
        if (p.cur.type == TOK_DESCR) {
            add_passthrough(&p, p.passthrough_start, p.cur.pos);
            next_token(&p);
            parse_descr(&p);
        } else if (p.cur.type == TOK_MATCH_DESCR) {
            add_passthrough(&p, p.passthrough_start, p.cur.pos);
            size_t kw_pos = p.cur.pos;
            next_token(&p);
            parse_match_descr(&p, kw_pos);
        } else if (p.cur.type == TOK_PHC_DEFER) {
            /* phc_defer { body } */
            if (p.lex.scan_brace_depth > 1) {
                parser_error(&p, "phc_defer must be at function scope, not inside loops or blocks");
            }
            add_passthrough(&p, p.passthrough_start, p.cur.pos);
            next_token(&p); /* consume phc_defer — now in struct mode */
            if (!p.error && p.cur.type != TOK_LBRACE) {
                parser_error(&p, "expected '{' after phc_defer");
            } else if (!p.error) {
                size_t brace_pos = p.cur.pos;
                next_token(&p);
                /* Capture body by brace-depth counting */
                int depth = 1;
                while (p.cur.type != TOK_EOF && depth > 0) {
                    if (p.cur.type == TOK_LBRACE) depth++;
                    if (p.cur.type == TOK_RBRACE) { depth--; if (depth == 0) break; }
                    next_token(&p);
                }
                /* Body text is between opening { and closing } */
                size_t body_start = brace_pos + 1;
                size_t body_end = p.cur.pos;
                while (body_end > body_start &&
                       (p.source[body_end - 1] == ' ' || p.source[body_end - 1] == '\t' ||
                        p.source[body_end - 1] == '\n'))
                    body_end--;
                /* Set defer_active BEFORE consuming }, so the lexer's
                 * transition back to scan mode will detect 'return' */
                p.lex.defer_active = 1;
                next_token(&p); /* consume closing } — back to scan mode */

                DeferBlock db;
                db.body_text = strndup(p.source + body_start, body_end - body_start);
                db.defer_index = p.active_defer_count;
                DA_PUSH(p.defers, p.defer_count, p.defer_cap, db);
                p.active_defer_count++;

                Chunk c;
                memset(&c, 0, sizeof(c));
                c.type = CHUNK_DEFER;
                c.defer = db;
                DA_PUSH(p.chunks, p.chunk_count, p.chunk_cap, c);

                p.lex.defer_active = 1;
                p.passthrough_start = p.cur.pos;
            }
        } else if (p.cur.type == TOK_PHC_DEFER_CANCEL) {
            add_passthrough(&p, p.passthrough_start, p.cur.pos);
            /* Skip past 'phc_defer_cancel' and the following ';' */
            size_t after_kw = p.cur.pos + p.cur.length;
            while (after_kw < p.source_len && p.source[after_kw] != ';') after_kw++;
            if (after_kw < p.source_len) after_kw++; /* skip ; */

            Chunk c;
            memset(&c, 0, sizeof(c));
            c.type = CHUNK_DEFER_CANCEL;
            DA_PUSH(p.chunks, p.chunk_count, p.chunk_cap, c);

            /* Advance lexer past ; */
            while (p.lex.pos < after_kw && p.lex.pos < p.lex.len) {
                if (p.lex.src[p.lex.pos] == '\n') {
                    p.lex.line++; p.lex.col = 1;
                    if (p.lex.marker_seen) p.lex.orig_line++;
                } else { p.lex.col++; }
                p.lex.pos++;
            }
            p.passthrough_start = p.lex.pos;
            p.cur = lexer_next(&p.lex);
        } else if (p.cur.type == TOK_RETURN) {
            /* return <expr>; with active defer — rewrite to goto */
            add_passthrough(&p, p.passthrough_start, p.cur.pos);
            size_t ret_end = p.cur.pos + p.cur.length; /* after 'return' */

            /* Find the ; at depth 0 (skip ; inside braces/parens) */
            size_t semi = ret_end;
            int depth = 0;
            while (semi < p.source_len) {
                char ch = p.source[semi];
                if (ch == '(' || ch == '{') depth++;
                else if (ch == ')' || ch == '}') depth--;
                else if (ch == ';' && depth == 0) break;
                /* Skip string literals */
                else if (ch == '"') {
                    semi++;
                    while (semi < p.source_len && p.source[semi] != '"') {
                        if (p.source[semi] == '\\') semi++;
                        semi++;
                    }
                }
                semi++;
            }

            /* Extract expression (between 'return' and ';'), trimmed */
            size_t expr_s = ret_end;
            while (expr_s < semi && (p.source[expr_s] == ' ' || p.source[expr_s] == '\t'))
                expr_s++;
            size_t expr_e = semi;
            while (expr_e > expr_s && (p.source[expr_e - 1] == ' ' || p.source[expr_e - 1] == '\t'))
                expr_e--;

            Chunk c;
            memset(&c, 0, sizeof(c));
            c.type = CHUNK_RETURN;
            c.ret.expr = (expr_e > expr_s) ? strndup(p.source + expr_s, expr_e - expr_s) : NULL;
            c.ret.defer_count = p.active_defer_count;
            DA_PUSH(p.chunks, p.chunk_count, p.chunk_cap, c);

            /* Advance lexer past the ; */
            while (p.lex.pos <= semi && p.lex.pos < p.lex.len) {
                if (p.lex.src[p.lex.pos] == '\n') {
                    p.lex.line++; p.lex.col = 1;
                    if (p.lex.marker_seen) p.lex.orig_line++;
                } else { p.lex.col++; }
                p.lex.pos++;
            }
            p.passthrough_start = p.lex.pos;
            p.cur = lexer_next(&p.lex);
        } else if (p.cur.type == TOK_RBRACE && p.active_defer_count > 0) {
            /* Function closing brace with active defers */
            add_passthrough(&p, p.passthrough_start, p.cur.pos);

            Chunk c;
            memset(&c, 0, sizeof(c));
            c.type = CHUNK_FUNC_END;
            c.func_end.defer_count = p.active_defer_count;
            DA_PUSH(p.chunks, p.chunk_count, p.chunk_cap, c);

            p.active_defer_count = 0;
            p.lex.defer_active = 0;
            p.passthrough_start = p.cur.pos; /* } included in next passthrough */
            next_token(&p);
        } else {
            next_token(&p);
        }
    }

    if (!p.error) {
        add_passthrough(&p, p.passthrough_start, p.source_len);
    }

    ParseResult result;
    memset(&result, 0, sizeof(result));
    if (p.error) {
        result.error = 1;
        result.error_message = strdup(p.error_msg);
        result.error_line = p.error_line;
    }
    result.program.source = source;
    result.program.source_len = p.source_len;
    result.program.descrs = p.descrs;
    result.program.descr_count = p.descr_count;
    result.program.chunks = p.chunks;
    result.program.chunk_count = p.chunk_count;
    result.program.defers = p.defers;
    result.program.defer_count = p.defer_count;

    return result;
}

void parse_result_free(ParseResult *result) {
    free(result->error_message);
    for (int i = 0; i < result->program.descr_count; i++) {
        DescrDecl *d = &result->program.descrs[i];
        free(d->name);
        for (int j = 0; j < d->variant_count; j++) {
            free(d->variants[j].name);
            for (int k = 0; k < d->variants[j].field_count; k++) {
                free(d->variants[j].fields[k].type_name);
                free(d->variants[j].fields[k].field_name);
                free(d->variants[j].fields[k].raw_decl);
            }
            free(d->variants[j].fields);
        }
        free(d->variants);
        free(d->end_file);
    }
    free(result->program.descrs);
    for (int i = 0; i < result->program.chunk_count; i++) {
        Chunk *c = &result->program.chunks[i];
        if (c->type == CHUNK_MATCH_DESCR) {
            free_match_contents(&c->match);
        } else if (c->type == CHUNK_RETURN) {
            free(c->ret.expr);
        }
        /* Note: CHUNK_DEFER body_text is freed via program.defers below */
    }
    free(result->program.chunks);
    for (int i = 0; i < result->program.defer_count; i++)
        free(result->program.defers[i].body_text);
    free(result->program.defers);
    memset(result, 0, sizeof(*result));
}
