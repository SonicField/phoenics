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

/* Find a variant in a DescrDecl by name, return NULL if not found */
static const Variant *find_variant(const DescrDecl *descrs, int descr_count,
                                   const char *type_name, const char *variant_name) {
    for (int i = 0; i < descr_count; i++) {
        if (strcmp(descrs[i].name, type_name) != 0) continue;
        for (int j = 0; j < descrs[i].variant_count; j++) {
            if (strcmp(descrs[i].variants[j].name, variant_name) == 0)
                return &descrs[i].variants[j];
        }
    }
    return NULL;
}

static void emit_match_descr(Buffer *buf, const MatchDescr *m,
                              const DescrDecl *descrs, int descr_count) {
    buf_append(buf, "switch (");
    buf_append(buf, m->expr_text);
    buf_append(buf, ".tag) {\n");

    for (int i = 0; i < m->case_count; i++) {
        const MatchCase *mc = &m->cases[i];

        if (mc->binding_count > 0) {
            const Variant *v = find_variant(descrs, descr_count,
                                            m->type_name, mc->variant_name);
            /* Emit: case Type_Variant: { bindings; body_text_content } break; */
            buf_printf(buf, "    case %s_%s: {", m->type_name, mc->variant_name);
            for (int b = 0; b < mc->binding_count; b++) {
                /* Find the field type for this binding */
                for (int fi = 0; fi < v->field_count; fi++) {
                    if (strcmp(v->fields[fi].field_name, mc->bindings[b]) == 0) {
                        buf_printf(buf, " %s %s = %s.%s.%s;",
                                   v->fields[fi].type_name, mc->bindings[b],
                                   m->expr_text, mc->variant_name, mc->bindings[b]);
                        break;
                    }
                }
            }
            /* Emit body_text (which starts with '{') inside the outer braces */
            buf_printf(buf, " %s }\n", mc->body_text);
        } else {
            buf_printf(buf, "    case %s_%s: %s\n",
                       m->type_name, mc->variant_name, mc->body_text);
        }
    }

    buf_append(buf, "    default: break;\n");
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
            buf_printf(&buf, "\n#line %d", prog->descrs[c->descr_index].end_line);
            break;
        case CHUNK_MATCH_DESCR:
            emit_match_descr(&buf, &c->match, prog->descrs, prog->descr_count);
            buf_printf(&buf, "\n#line %d", c->match.end_line);
            break;
        }
    }

    return buf_finish(&buf);
}
