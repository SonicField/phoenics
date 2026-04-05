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

/* Find a field's type in a variant by field name */
static const char *find_field_type(const Field *fields, int field_count,
                                   const char *field_name) {
    for (int i = 0; i < field_count; i++) {
        if (strcmp(fields[i].field_name, field_name) == 0)
            return fields[i].type_name;
    }
    return NULL;
}

/* Emit binding declarations for a destructured case.
 * Searches local descrs first, then external types. */
static void emit_bindings(Buffer *buf, const MatchDescr *m, const MatchCase *mc,
                           const DescrDecl *descrs, int descr_count,
                           const DescrType *ext_types, int ext_count) {
    /* Try local descrs first */
    for (int i = 0; i < descr_count; i++) {
        if (strcmp(descrs[i].name, m->type_name) != 0) continue;
        for (int vi = 0; vi < descrs[i].variant_count; vi++) {
            if (strcmp(descrs[i].variants[vi].name, mc->variant_name) != 0) continue;
            const Variant *v = &descrs[i].variants[vi];
            for (int b = 0; b < mc->binding_count; b++) {
                const char *type = find_field_type(v->fields, v->field_count, mc->bindings[b]);
                if (type)
                    buf_printf(buf, " %s %s = %s.%s.%s;",
                               type, mc->bindings[b], m->expr_text,
                               mc->variant_name, mc->bindings[b]);
            }
            return;
        }
    }
    /* Try external types */
    for (int i = 0; i < ext_count; i++) {
        if (strcmp(ext_types[i].name, m->type_name) != 0) continue;
        if (!ext_types[i].variant_fields) return;
        for (int vi = 0; vi < ext_types[i].variant_count; vi++) {
            if (strcmp(ext_types[i].variant_names[vi], mc->variant_name) != 0) continue;
            Field *fields = ext_types[i].variant_fields[vi];
            int fc = ext_types[i].variant_field_counts[vi];
            for (int b = 0; b < mc->binding_count; b++) {
                const char *type = find_field_type(fields, fc, mc->bindings[b]);
                if (type)
                    buf_printf(buf, " %s %s = %s.%s.%s;",
                               type, mc->bindings[b], m->expr_text,
                               mc->variant_name, mc->bindings[b]);
            }
            return;
        }
    }
}

static void emit_match_descr(Buffer *buf, const MatchDescr *m,
                              const DescrDecl *descrs, int descr_count,
                              const DescrType *ext_types, int ext_count) {
    buf_append(buf, "switch (");
    buf_append(buf, m->expr_text);
    buf_append(buf, ".tag) {\n");

    for (int i = 0; i < m->case_count; i++) {
        const MatchCase *mc = &m->cases[i];

        if (mc->binding_count > 0) {
            buf_printf(buf, "    case %s_%s: {", m->type_name, mc->variant_name);
            emit_bindings(buf, m, mc, descrs, descr_count, ext_types, ext_count);
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

char *codegen(const Program *prog,
              const DescrType *external_types, int external_type_count) {
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
        case CHUNK_DESCR: {
            const DescrDecl *d = &prog->descrs[c->descr_index];
            emit_descr(&buf, d);
            if (d->end_file)
                buf_printf(&buf, "\n#line %d \"%s\"", d->end_line, d->end_file);
            else
                buf_printf(&buf, "\n#line %d", d->end_line);
            break;
        }
        case CHUNK_MATCH_DESCR:
            emit_match_descr(&buf, &c->match, prog->descrs, prog->descr_count,
                            external_types, external_type_count);
            if (c->match.end_file)
                buf_printf(&buf, "\n#line %d \"%s\"", c->match.end_line, c->match.end_file);
            else
                buf_printf(&buf, "\n#line %d", c->match.end_line);
            break;
        }
    }

    return buf_finish(&buf);
}
