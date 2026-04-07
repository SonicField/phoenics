#include "compat.h"
#include "codegen.h"
#include "buffer.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* --- descr codegen --- */

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
                buf_append(buf, v->fields[j].raw_decl);
                buf_append(buf, ";\n");
            }
        }
        buf_printf(buf, "} %s_%s_t;\n", d->name, v->name);
    }

    /* Main struct with named tag (enables forward declarations) */
    buf_printf(buf, "\ntypedef struct %s {\n", d->name);
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

        /* Count non-array fields for constructor params */
        int param_count = 0;
        for (int j = 0; j < v->field_count; j++) {
            if (!v->fields[j].is_array) param_count++;
        }

        if (param_count == 0) {
            buf_append(buf, "void");
        } else {
            int first = 1;
            for (int j = 0; j < v->field_count; j++) {
                if (v->fields[j].is_array) continue;
                if (!first) buf_append(buf, ", ");
                first = 0;
                buf_append(buf, v->fields[j].raw_decl);
            }
        }

        buf_append(buf, ") {\n");
        buf_printf(buf, "    %s _v;\n", d->name);
        buf_printf(buf, "    _v.tag = %s_%s;\n", d->name, v->name);
        for (int j = 0; j < v->field_count; j++) {
            if (v->fields[j].is_array) continue;
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

/* --- enum codegen --- */

static void emit_enum(Buffer *buf, const EnumDecl *e) {
    /* Enum typedef */
    buf_printf(buf, "typedef enum {\n");
    for (int i = 0; i < e->value_count; i++) {
        buf_printf(buf, "    %s_%s", e->name, e->values[i].name);
        if (e->values[i].has_value) {
            buf_printf(buf, " = %d", e->values[i].value);
        }
        buf_append(buf, ",\n");
    }
    /* __COUNT = number of declared values (not auto-increment) */
    buf_printf(buf, "    %s__COUNT = %d\n", e->name, e->value_count);
    buf_printf(buf, "} %s;\n", e->name);

    /* to_string */
    buf_printf(buf, "\nstatic inline const char *%s_to_string(%s c) {\n", e->name, e->name);
    buf_append(buf, "    switch (c) {\n");
    for (int i = 0; i < e->value_count; i++) {
        buf_printf(buf, "        case %s_%s: return \"%s\";\n",
                   e->name, e->values[i].name, e->values[i].name);
    }
    buf_append(buf, "        default: return \"(unknown)\";\n");
    buf_append(buf, "    }\n");
    buf_append(buf, "}\n");

    /* from_string */
    buf_printf(buf, "\nstatic inline int %s_from_string(const char *s, %s *out) {\n",
               e->name, e->name);
    for (int i = 0; i < e->value_count; i++) {
        buf_printf(buf, "    if (strcmp(s, \"%s\") == 0) { *out = %s_%s; return 1; }\n",
                   e->values[i].name, e->name, e->values[i].name);
    }
    buf_append(buf, "    return 0;\n");
    buf_append(buf, "}");
}

/* Check if a type name is a phc_enum */
static int is_enum_type(const char *type_name,
                        const EnumDecl *enums, int enum_count) {
    for (int i = 0; i < enum_count; i++) {
        if (strcmp(enums[i].name, type_name) == 0) return 1;
    }
    return 0;
}

/* --- match_descr codegen --- */

/* Find a field in a variant by field name */
static const Field *find_field(const Field *fields, int field_count,
                               const char *field_name) {
    for (int i = 0; i < field_count; i++) {
        if (strcmp(fields[i].field_name, field_name) == 0)
            return &fields[i];
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
                const Field *f = find_field(v->fields, v->field_count, mc->bindings[b]);
                if (!f) continue;
                if (f->is_array)
                    buf_printf(buf, " %s *%s = %s.%s.%s;",
                               f->type_name, mc->bindings[b], m->expr_text,
                               mc->variant_name, mc->bindings[b]);
                else
                    buf_printf(buf, " %s %s = %s.%s.%s;",
                               f->type_name, mc->bindings[b], m->expr_text,
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
                const Field *f = find_field(fields, fc, mc->bindings[b]);
                if (!f || !f->type_name) continue;
                /* Function pointer types embed the name as (*name). */
                char needle[256];
                int is_fnptr = 0;
                if (snprintf(needle, sizeof(needle), "(*%s)", mc->bindings[b]) < (int)sizeof(needle)) {
                    is_fnptr = strstr(f->type_name, needle) != NULL;
                }
                if (is_fnptr)
                    buf_printf(buf, " %s = %s.%s.%s;",
                               f->type_name, m->expr_text,
                               mc->variant_name, mc->bindings[b]);
                else if (f->is_array)
                    buf_printf(buf, " %s *%s = %s.%s.%s;",
                               f->type_name, mc->bindings[b], m->expr_text,
                               mc->variant_name, mc->bindings[b]);
                else
                    buf_printf(buf, " %s %s = %s.%s.%s;",
                               f->type_name, mc->bindings[b], m->expr_text,
                               mc->variant_name, mc->bindings[b]);
            }
            return;
        }
    }
}

/* Emit body text, rewriting return statements to include defer cleanup.
 * Scans for 'return' at word boundaries, skipping strings/comments. */
static void emit_body_with_defer(Buffer *buf, const char *body, int defer_count,
                                  const char **defer_bodies, int defer_body_count) {
    if (!body || defer_count == 0) {
        if (body) buf_append(buf, body);
        return;
    }
    size_t len = strlen(body);
    size_t i = 0;
    while (i < len) {
        /* Skip string literals */
        if (body[i] == '"') {
            buf_append_n(buf, body + i, 1); i++;
            while (i < len && body[i] != '"') {
                if (body[i] == '\\' && i + 1 < len) {
                    buf_append_n(buf, body + i, 2); i += 2;
                } else {
                    buf_append_n(buf, body + i, 1); i++;
                }
            }
            if (i < len) { buf_append_n(buf, body + i, 1); i++; }
            continue;
        }
        /* Skip line comments */
        if (body[i] == '/' && i + 1 < len && body[i + 1] == '/') {
            while (i < len && body[i] != '\n') {
                buf_append_n(buf, body + i, 1); i++;
            }
            continue;
        }
        /* Skip block comments */
        if (body[i] == '/' && i + 1 < len && body[i + 1] == '*') {
            buf_append_n(buf, body + i, 2); i += 2;
            while (i + 1 < len && !(body[i] == '*' && body[i + 1] == '/')) {
                buf_append_n(buf, body + i, 1); i++;
            }
            if (i + 1 < len) { buf_append_n(buf, body + i, 2); i += 2; }
            continue;
        }
        /* Skip char literals */
        if (body[i] == '\'') {
            buf_append_n(buf, body + i, 1); i++;
            while (i < len && body[i] != '\'') {
                if (body[i] == '\\' && i + 1 < len) {
                    buf_append_n(buf, body + i, 2); i += 2;
                } else {
                    buf_append_n(buf, body + i, 1); i++;
                }
            }
            if (i < len) { buf_append_n(buf, body + i, 1); i++; }
            continue;
        }
        /* Check for 'return' keyword at word boundary */
        if (i + 6 <= len && memcmp(body + i, "return", 6) == 0 &&
            (i == 0 || !isalnum((unsigned char)body[i - 1])) &&
            (i + 6 >= len || !isalnum((unsigned char)body[i + 6]))) {
            /* Found return — find the semicolon at depth 0 */
            size_t ret_end = i + 6;
            size_t semi = ret_end;
            int depth = 0;
            while (semi < len) {
                if (body[semi] == '(' || body[semi] == '{') depth++;
                else if (body[semi] == ')' || body[semi] == '}') depth--;
                else if (body[semi] == ';' && depth == 0) break;
                semi++;
            }
            /* Extract return expression */
            size_t es = ret_end;
            while (es < semi && (body[es] == ' ' || body[es] == '\t')) es++;
            size_t ee = semi;
            while (ee > es && (body[ee - 1] == ' ' || body[ee - 1] == '\t')) ee--;

            /* Emit cleanup + return */
            buf_append(buf, "{ ");
            for (int d = 0; d < defer_body_count; d++) {
                if (defer_bodies[d]) {
                    buf_append(buf, defer_bodies[d]);
                    buf_append(buf, " ");
                }
            }
            if (ee > es) {
                buf_append(buf, "return ");
                buf_append_n(buf, body + es, ee - es);
                buf_append(buf, "; }");
            } else {
                buf_append(buf, "return; }");
            }
            i = semi + 1; /* skip past ; */
            continue;
        }
        buf_append_n(buf, body + i, 1);
        i++;
    }
}

/* Forward declaration for recursive chunk emission */
static void emit_chunks(Buffer *buf, const Chunk *chunks, int chunk_count,
                         const char *source, const DescrDecl *descrs, int descr_count,
                         const DescrType *ext_types, int ext_count,
                         const EnumDecl *enums, int enum_count,
                         int active_defer_count,
                         const char **defer_bodies, int defer_body_count);

static void emit_match_descr(Buffer *buf, const MatchDescr *m,
                              const DescrDecl *descrs, int descr_count,
                              const DescrType *ext_types, int ext_count,
                              const EnumDecl *enums, int enum_count,
                              int active_defer_count,
                              const char **defer_bodies, int defer_body_count) {
    int match_is_enum = is_enum_type(m->type_name, enums, enum_count);
    buf_append(buf, "switch (");
    buf_append(buf, m->expr_text);
    if (!match_is_enum)
        buf_append(buf, ".tag");
    buf_append(buf, ") {\n");

    for (int i = 0; i < m->case_count; i++) {
        const MatchCase *mc = &m->cases[i];

        if (mc->body_chunks && mc->body_chunk_count > 0) {
            /* Recursive body parsing: emit parsed chunks */
            if (mc->binding_count > 0) {
                buf_printf(buf, "    case %s_%s: {", m->type_name, mc->variant_name);
                emit_bindings(buf, m, mc, descrs, descr_count, ext_types, ext_count);
                buf_append(buf, " ");
            } else {
                buf_printf(buf, "    case %s_%s: ", m->type_name, mc->variant_name);
            }
            emit_chunks(buf, (const Chunk *)mc->body_chunks, mc->body_chunk_count,
                        mc->body_text, descrs, descr_count,
                        ext_types, ext_count, enums, enum_count,
                        active_defer_count, defer_bodies, defer_body_count);
            if (mc->binding_count > 0)
                buf_append(buf, " }\n");
            else
                buf_append(buf, "\n");
        } else if (mc->binding_count > 0) {
            buf_printf(buf, "    case %s_%s: {", m->type_name, mc->variant_name);
            emit_bindings(buf, m, mc, descrs, descr_count, ext_types, ext_count);
            buf_append(buf, " ");
            emit_body_with_defer(buf, mc->body_text, active_defer_count,
                                 defer_bodies, defer_body_count);
            buf_append(buf, " }\n");
        } else {
            buf_printf(buf, "    case %s_%s: ", m->type_name, mc->variant_name);
            emit_body_with_defer(buf, mc->body_text, active_defer_count,
                                 defer_bodies, defer_body_count);
            buf_append(buf, "\n");
        }
    }

    buf_append(buf, "    default: break;\n");
    buf_append(buf, "}");
}

/* Emit an array of chunks (used for case body sub-programs) */
static void emit_chunks(Buffer *buf, const Chunk *chunks, int chunk_count,
                         const char *source, const DescrDecl *descrs, int descr_count,
                         const DescrType *ext_types, int ext_count,
                         const EnumDecl *enums, int enum_count,
                         int active_defer_count,
                         const char **defer_bodies, int defer_body_count) {
    for (int i = 0; i < chunk_count; i++) {
        const Chunk *c = &chunks[i];
        switch (c->type) {
        case CHUNK_PASSTHROUGH:
            if (active_defer_count > 0) {
                /* Scan passthrough for returns when defers are active */
                char *text = strndup(source + c->passthrough.start,
                                     c->passthrough.end - c->passthrough.start);
                emit_body_with_defer(buf, text, active_defer_count,
                                     defer_bodies, defer_body_count);
                free(text);
            } else {
                buf_append_n(buf, source + c->passthrough.start,
                             c->passthrough.end - c->passthrough.start);
            }
            break;
        case CHUNK_DESCR:
            /* Not expected in case bodies, but handle gracefully */
            break;
        case CHUNK_MATCH_DESCR:
            emit_match_descr(buf, &c->match, descrs, descr_count,
                            ext_types, ext_count, enums, enum_count,
                            active_defer_count, defer_bodies, defer_body_count);
            break;
        case CHUNK_ENUM:
            /* Not expected in case bodies, but handle gracefully */
            break;
        case CHUNK_DEFER:
            /* Track but don't emit — handled at CHUNK_RETURN/CHUNK_FUNC_END */
            active_defer_count++;
            defer_body_count++;
            break;
        case CHUNK_RETURN: {
            int n = c->ret.defer_count + active_defer_count;
            /* Collect ALL defer bodies (from enclosing + local) */
            const char **all_bodies = calloc((size_t)(n > 0 ? n : 1), sizeof(const char *));
            int found = 0;
            /* Local defers from this chunk array */
            for (int j = i - 1; j >= 0 && found < c->ret.defer_count; j--) {
                if (chunks[j].type == CHUNK_DEFER)
                    all_bodies[found++] = chunks[j].defer.body_text;
            }
            /* Enclosing defers */
            for (int d = 0; d < defer_body_count && found < n; d++)
                all_bodies[found++] = defer_bodies[d];

            buf_append(buf, "{ ");
            for (int d = 0; d < found; d++) {
                if (all_bodies[d]) {
                    buf_append(buf, all_bodies[d]);
                    buf_append(buf, " ");
                }
            }
            if (c->ret.expr)
                buf_printf(buf, "return %s; }", c->ret.expr);
            else
                buf_append(buf, "return; }");
            free(all_bodies);
            break;
        }
        case CHUNK_DEFER_CANCEL:
            /* Set enclosing defer guards to 0 */
            for (int d = 0; d < defer_body_count; d++)
                buf_printf(buf, "_phc_dg_%d = 0; ", d + 1);
            for (int j = i - 1; j >= 0; j--) {
                if (chunks[j].type == CHUNK_DEFER)
                    buf_printf(buf, "_phc_dg_%d = 0; ", chunks[j].defer.defer_index + 1);
            }
            buf_append(buf, "\n");
            break;
        case CHUNK_FUNC_END:
            /* Not expected in case bodies */
            break;
        }
    }
}

/* --- Main codegen --- */

char *codegen(const Program *prog,
              const DescrType *external_types, int external_type_count) {
    Buffer buf;
    buf_init(&buf);

    /* Declare abort() for safe accessor functions. Use a direct declaration
     * instead of #include <stdlib.h> to avoid double-inclusion when phc runs
     * post-preprocessor (cc -E | phc | cc — stdlib.h is already expanded). */
    /* Emit abort() declaration only if there are full descr definitions
     * (not just forward declarations) */
    for (int i = 0; i < prog->descr_count; i++) {
        if (prog->descrs[i].variant_count >= 0) {
            buf_append(&buf, "extern void abort(void);\n");
            buf_append(&buf, "#define phc_free(pp) do { free(*(pp)); *(pp) = ((void*)0); } while(0)\n");
            break;
        }
    }
    /* Declare strcmp() for enum from_string functions */
    if (prog->enum_count > 0) {
        buf_append(&buf, "extern int strcmp(const char *, const char *);\n");
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
            if (d->variant_count < 0) {
                /* Forward declaration */
                buf_printf(&buf, "typedef struct %s %s;", d->name, d->name);
            } else {
                emit_descr(&buf, d);
                if (d->end_file)
                    buf_printf(&buf, "\n#line %d \"%s\"", d->end_line, d->end_file);
                else
                    buf_printf(&buf, "\n#line %d", d->end_line);
            }
            break;
        }
        case CHUNK_ENUM: {
            const EnumDecl *e = &prog->enums[c->enum_index];
            emit_enum(&buf, e);
            if (e->end_file)
                buf_printf(&buf, "\n#line %d \"%s\"", e->end_line, e->end_file);
            else
                buf_printf(&buf, "\n#line %d", e->end_line);
            break;
        }
        case CHUNK_MATCH_DESCR: {
            /* Collect active defer bodies for return interception inside case bodies */
            int active_defers = 0;
            const char **dbodies = NULL;
            int dbody_count = 0;
            for (int j = i - 1; j >= 0; j--) {
                if (prog->chunks[j].type == CHUNK_FUNC_END) break;
                if (prog->chunks[j].type == CHUNK_DEFER) {
                    active_defers++;
                    dbody_count++;
                }
            }
            if (dbody_count > 0) {
                dbodies = calloc((size_t)dbody_count, sizeof(const char *));
                int f = 0;
                for (int j = i - 1; j >= 0 && f < dbody_count; j--) {
                    if (prog->chunks[j].type == CHUNK_FUNC_END) break;
                    if (prog->chunks[j].type == CHUNK_DEFER)
                        dbodies[f++] = prog->chunks[j].defer.body_text;
                }
            }
            emit_match_descr(&buf, &c->match, prog->descrs, prog->descr_count,
                            external_types, external_type_count,
                            prog->enums, prog->enum_count,
                            active_defers, dbodies, dbody_count);
            free(dbodies);
            if (c->match.end_file)
                buf_printf(&buf, "\n#line %d \"%s\"", c->match.end_line, c->match.end_file);
            else
                buf_printf(&buf, "\n#line %d", c->match.end_line);
            break;
        }
        case CHUNK_DEFER: {
            /* Emit guard variable if any defer_cancel exists in this function */
            int has_cancel = 0;
            for (int j = i + 1; j < prog->chunk_count; j++) {
                if (prog->chunks[j].type == CHUNK_DEFER_CANCEL) { has_cancel = 1; break; }
                if (prog->chunks[j].type == CHUNK_FUNC_END) break;
            }
            if (has_cancel) {
                buf_printf(&buf, "int _phc_dg_%d = 1;\n", c->defer.defer_index + 1);
            }
            break;
        }
        case CHUNK_DEFER_CANCEL: {
            /* Set all active defer guards to 0 */
            for (int j = i - 1; j >= 0; j--) {
                if (prog->chunks[j].type == CHUNK_FUNC_END) break;
                if (prog->chunks[j].type == CHUNK_DEFER)
                    buf_printf(&buf, "_phc_dg_%d = 0; ", prog->chunks[j].defer.defer_index + 1);
                if (prog->chunks[j].type == CHUNK_FUNC_END) break;
            }
            buf_append(&buf, "\n");
            break;
        }
        case CHUNK_RETURN: {
            /* Emit defer cleanup inline (reverse order), then actual return */
            int n = c->ret.defer_count;
            const char **bodies = calloc((size_t)(n > 0 ? n : 1), sizeof(const char *));
            int *indices = calloc((size_t)(n > 0 ? n : 1), sizeof(int));
            int found = 0;
            int has_cancel = 0;
            for (int j = i - 1; j >= 0 && found < n; j--) {
                if (prog->chunks[j].type == CHUNK_FUNC_END) break;
                if (prog->chunks[j].type == CHUNK_DEFER) {
                    bodies[found] = prog->chunks[j].defer.body_text;
                    indices[found] = prog->chunks[j].defer.defer_index + 1;
                    found++;
                }
                if (prog->chunks[j].type == CHUNK_DEFER_CANCEL) has_cancel = 1;
            }
            /* Check if cancel exists ahead too */
            for (int j = i + 1; j < prog->chunk_count && !has_cancel; j++) {
                if (prog->chunks[j].type == CHUNK_DEFER_CANCEL) has_cancel = 1;
                if (prog->chunks[j].type == CHUNK_FUNC_END) break;
            }
            buf_append(&buf, "{ ");
            for (int d = 0; d < found; d++) {
                if (bodies[d]) {
                    if (has_cancel)
                        buf_printf(&buf, "if (_phc_dg_%d) { %s } ", indices[d], bodies[d]);
                    else {
                        buf_append(&buf, bodies[d]);
                        buf_append(&buf, " ");
                    }
                }
            }
            if (c->ret.expr)
                buf_printf(&buf, "return %s; }", c->ret.expr);
            else
                buf_append(&buf, "return; }");
            free(bodies);
            free(indices);
            break;
        }
        case CHUNK_FUNC_END: {
            int n = c->func_end.defer_count;
            const char **bodies = calloc((size_t)(n > 0 ? n : 1), sizeof(const char *));
            int *indices = calloc((size_t)(n > 0 ? n : 1), sizeof(int));
            int found = 0;
            int has_cancel = 0;
            for (int j = i - 1; j >= 0 && found < n; j--) {
                if (prog->chunks[j].type == CHUNK_FUNC_END) break;
                if (prog->chunks[j].type == CHUNK_DEFER) {
                    bodies[found] = prog->chunks[j].defer.body_text;
                    indices[found] = prog->chunks[j].defer.defer_index + 1;
                    found++;
                }
                if (prog->chunks[j].type == CHUNK_DEFER_CANCEL) has_cancel = 1;
            }
            for (int d = 0; d < found; d++) {
                if (bodies[d]) {
                    if (has_cancel)
                        buf_printf(&buf, "if (_phc_dg_%d) { %s }\n", indices[d], bodies[d]);
                    else {
                        buf_append(&buf, bodies[d]);
                        buf_append(&buf, "\n");
                    }
                }
            }
            free(bodies);
            free(indices);
            break;
        }
        }
    }

    return buf_finish(&buf);
}
