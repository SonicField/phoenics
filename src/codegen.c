#include "codegen.h"
#include "buffer.h"
#include <string.h>
#include <stdio.h>

/* --- descr codegen --- */

/* Helper: emit "type name" with correct spacing for pointer types */
static void emit_type_and_name(Buffer *buf, const char *type, const char *name) {
    buf_append(buf, type);
    size_t tlen = strlen(type);
    if (tlen > 0 && type[tlen - 1] != '*') {
        buf_append(buf, " ");
    }
    buf_append(buf, name);
}

static void emit_descr(Buffer *buf, const DescrDecl *d) {
    /* Tag enum */
    buf_printf(buf, "typedef enum {\n");
    for (int i = 0; i < d->variant_count; i++) {
        buf_printf(buf, "    %s_%s,\n", d->name, d->variants[i].name);
    }
    buf_printf(buf, "    %s__COUNT\n", d->name);
    buf_printf(buf, "} %s_Tag;\n", d->name);

    /* Named variant struct typedefs */
    for (int i = 0; i < d->variant_count; i++) {
        const Variant *v = &d->variants[i];
        buf_printf(buf, "\ntypedef struct {\n");
        if (v->field_count == 0) {
            buf_append(buf, "    char _empty;\n");
        } else {
            for (int j = 0; j < v->field_count; j++) {
                buf_append(buf, "    ");
                emit_type_and_name(buf, v->fields[j].type_name, v->fields[j].field_name);
                buf_append(buf, ";\n");
            }
        }
        buf_printf(buf, "} %s_%s_t;\n", d->name, v->name);
    }

    /* Main struct with anonymous union using named variant types */
    buf_printf(buf, "\ntypedef struct {\n");
    buf_printf(buf, "    %s_Tag tag;\n", d->name);
    buf_append(buf, "    union {\n");
    for (int i = 0; i < d->variant_count; i++) {
        buf_printf(buf, "        %s_%s_t %s;\n", d->name, d->variants[i].name,
                   d->variants[i].name);
    }
    buf_append(buf, "    };\n");
    buf_printf(buf, "} %s;\n", d->name);

    /* Constructor functions */
    for (int i = 0; i < d->variant_count; i++) {
        const Variant *v = &d->variants[i];
        buf_append(buf, "\n");
        buf_printf(buf, "static inline %s %s_mk_%s(", d->name, d->name, v->name);

        if (v->field_count == 0) {
            buf_append(buf, "void");
        } else {
            for (int j = 0; j < v->field_count; j++) {
                if (j > 0) buf_append(buf, ", ");
                emit_type_and_name(buf, v->fields[j].type_name, v->fields[j].field_name);
            }
        }

        buf_append(buf, ") {\n");
        buf_printf(buf, "    %s _v;\n", d->name);
        buf_printf(buf, "    _v.tag = %s_%s;\n", d->name, v->name);
        for (int j = 0; j < v->field_count; j++) {
            buf_printf(buf, "    _v.%s.%s = %s;\n",
                       v->name, v->fields[j].field_name, v->fields[j].field_name);
        }
        buf_append(buf, "    return _v;\n");
        buf_append(buf, "}");
        if (i < d->variant_count - 1) {
            buf_append(buf, "\n");
        }
    }

    /* Safe accessor functions — assert tag, return variant struct */
    for (int i = 0; i < d->variant_count; i++) {
        const Variant *v = &d->variants[i];
        buf_append(buf, "\n\n");
        buf_printf(buf, "static inline %s_%s_t %s_as_%s(%s v) {\n",
                   d->name, v->name, d->name, v->name, d->name);
        buf_printf(buf, "    if (v.tag != %s_%s) abort();\n",
                   d->name, v->name);
        buf_printf(buf, "    return v.%s;\n", v->name);
        buf_append(buf, "}");
    }
}

/* --- match_descr codegen --- */

static void emit_match_descr(Buffer *buf, const Program *prog,
                              const MatchDescr *m) {
    const char *src = prog->source;

    /* Emit "switch (expr.tag)" replacing "match_descr(Type, expr)" */
    buf_append(buf, "switch (");
    buf_append(buf, m->expr_text);
    buf_append(buf, ".tag)");

    /* Copy body from source with variant name replacements.
     * Copy from header_end_pos to rbrace_pos, replacing case variant names. */
    size_t pos = m->header_end_pos;
    for (int i = 0; i < m->case_count; i++) {
        /* Copy from current pos to the variant name */
        buf_append_n(buf, src + pos, m->cases[i].name_pos - pos);
        /* Emit prefixed variant name */
        buf_printf(buf, "%s_%s", m->type_name, m->cases[i].variant_name);
        /* Skip past original variant name */
        pos = m->cases[i].name_pos + m->cases[i].name_len;
    }

    /* Copy from after last case variant name to just before the closing }.
     * Split at the last \n so we can insert default: break; before the
     * closing brace's indentation. Clamp searches to within the match body
     * (lbrace_pos+1) to avoid walking into preceding source on single-line input. */
    size_t body_lower = m->lbrace_pos + 1;
    size_t last_nl = m->rbrace_pos;
    while (last_nl > body_lower && src[last_nl - 1] != '\n') last_nl--;

    int has_newline = (last_nl > body_lower);

    if (has_newline) {
        /* Multi-line: copy body up to last newline */
        buf_append_n(buf, src + pos, last_nl - pos);
    } else {
        /* Single-line: copy all body text before closing } */
        buf_append_n(buf, src + pos, m->rbrace_pos - pos);
    }

    /* Insert "default: break;" with same indentation as case labels */
    if (m->case_count > 0) {
        size_t first_name = m->cases[0].name_pos;
        size_t case_kw = first_name;
        while (case_kw > body_lower && src[case_kw - 1] == ' ') case_kw--;
        if (case_kw >= 4) case_kw -= 4;

        /* Find line start before case keyword, clamped to body */
        size_t line_start = case_kw;
        while (line_start > body_lower && src[line_start - 1] != '\n') line_start--;

        if (has_newline) {
            buf_append_n(buf, src + line_start, case_kw - line_start);
            buf_append(buf, "    default: break;\n");
        } else {
            buf_append(buf, " default: break; ");
        }
    }

    /* Emit closing brace with its original indentation */
    if (has_newline) {
        size_t close_line = m->rbrace_pos;
        while (close_line > body_lower && src[close_line - 1] != '\n') close_line--;
        buf_append_n(buf, src + close_line, m->rbrace_pos - close_line);
    }
    buf_append(buf, "}");
}

/* --- Main codegen --- */

char *codegen(const Program *prog) {
    Buffer buf;
    buf_init(&buf);

    /* Declare abort() for safe accessor functions. Use a direct declaration
     * instead of #include <stdlib.h> to avoid double-inclusion when phc runs
     * post-preprocessor (cc -E | phc | cc — stdlib.h is already expanded). */
    if (prog->descr_count > 0) {
        buf_append(&buf, "extern void abort(void);\n");
    }

    for (int i = 0; i < prog->chunk_count; i++) {
        const Chunk *c = &prog->chunks[i];
        switch (c->type) {
        case CHUNK_PASSTHROUGH:
            buf_append_n(&buf, prog->source + c->passthrough.start,
                         c->passthrough.end - c->passthrough.start);
            break;
        case CHUNK_DESCR:
            emit_descr(&buf, &prog->descrs[c->descr_index]);
            buf_printf(&buf, "\n#line %d\n", prog->descrs[c->descr_index].end_line);
            break;
        case CHUNK_MATCH_DESCR:
            emit_match_descr(&buf, prog, &c->match);
            buf_printf(&buf, "\n#line %d\n", c->match.end_line);
            break;
        }
    }

    return buf_finish(&buf);
}
