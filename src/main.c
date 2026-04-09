#include "compat.h"
#include "parser.h"
#include "semantic.h"
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_stdin(void) {
    size_t cap = 4096;
    size_t len = 0;
    char *buf = malloc(cap);

    while (1) {
        size_t n = fread(buf + len, 1, cap - len, stdin);
        len += n;
        if (n == 0) break;
        if (len == cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
    }

    buf[len] = '\0';
    return buf;
}

/* --- Manifest I/O --- */

static int emit_type_manifest(const char *path, const Program *prog) {
    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "phc: error: cannot open '%s' for writing\n", path);
        return 0;
    }
    fprintf(f, "# phc type manifest v2 — machine generated, do not edit\n");
    /* Emit enum types */
    for (int i = 0; i < prog->enum_count; i++) {
        const EnumDecl *e = &prog->enums[i];
        fprintf(f, "enum %s", e->name);
        for (int j = 0; j < e->value_count; j++) {
            fprintf(f, " %s", e->values[j].name);
        }
        fprintf(f, "\n");
    }
    /* Emit descr types */
    for (int i = 0; i < prog->descr_count; i++) {
        const DescrDecl *d = &prog->descrs[i];
        fprintf(f, "descr %s", d->name);
        for (int j = 0; j < d->variant_count; j++) {
            fprintf(f, " %s", d->variants[j].name);
        }
        fprintf(f, "\n");
        /* Emit field lines for each variant */
        for (int j = 0; j < d->variant_count; j++) {
            const Variant *v = &d->variants[j];
            for (int k = 0; k < v->field_count; k++) {
                const Field *fld = &v->fields[k];
                /* Format: field TypeName VariantName field_name type_text */
                if (fld->type_name) {
                    fprintf(f, "field %s %s %s %s\n",
                            d->name, v->name, fld->field_name, fld->type_name);
                } else {
                    /* Complex types: extract type from raw_decl by removing field_name */
                    fprintf(f, "field %s %s %s %s\n",
                            d->name, v->name, fld->field_name, fld->raw_decl);
                }
            }
        }
    }
    fclose(f);

    /* Generate companion .phc.h header file.
     * Derive header path: replace .phc-types with .phc.h */
    size_t path_len = strlen(path);
    const char *suffix = ".phc-types";
    size_t suffix_len = strlen(suffix);
    char *header_path = NULL;
    if (path_len > suffix_len &&
        strcmp(path + path_len - suffix_len, suffix) == 0) {
        header_path = malloc(path_len - suffix_len + 7); /* .phc.h + null */
        memcpy(header_path, path, path_len - suffix_len);
        strcpy(header_path + path_len - suffix_len, ".phc.h");
    } else {
        /* Fallback: append .h */
        header_path = malloc(path_len + 3);
        sprintf(header_path, "%s.h", path);
    }

    /* Build include guard name from header filename.
     * Safe chars [A-Za-z0-9_] pass through (lowercase uppercased).
     * Dash maps to double-underscore to avoid collisions:
     *   foo-bar.phc.h -> PHC_FOO__BAR_PHC_H
     *   foo_bar.phc.h -> PHC_FOO_BAR_PHC_H
     * Dot maps to single underscore.
     * Non-ASCII (UTF-8) → C11 universal character names (\uXXXX).
     * Control chars and special chars → hex-encoded as X_XX_. */
    const char *base = header_path;
    for (const char *p = header_path; *p; p++) {
        if (*p == '/') base = p + 1;
    }
    char guard[512];
    char *g = guard;
    char *gend = guard + sizeof(guard) - 1;
    static const char hex[] = "0123456789ABCDEF";
    const char *prefix = "PHC_";
    while (*prefix && g < gend) *g++ = *prefix++;
    for (const char *s = base; *s && g < gend; s++) {
        unsigned char ch = (unsigned char)*s;
        if (ch == '-') {
            if (g + 1 < gend) { *g++ = '_'; *g++ = '_'; }
        } else if (ch == '.') {
            *g++ = '_';
        } else if (ch >= 'a' && ch <= 'z') {
            *g++ = (char)(ch - 'a' + 'A');
        } else if (ch == '_' || (ch >= 'A' && ch <= 'Z') ||
                   (ch >= '0' && ch <= '9')) {
            *g++ = (char)ch;
        } else if (ch >= 0x80) {
            /* UTF-8 multi-byte → C11 universal character name */
            unsigned long cp = 0;
            int trail = 0;
            if ((ch & 0xE0) == 0xC0)      { cp = ch & 0x1F; trail = 1; }
            else if ((ch & 0xF0) == 0xE0)  { cp = ch & 0x0F; trail = 2; }
            else if ((ch & 0xF8) == 0xF0)  { cp = ch & 0x07; trail = 3; }
            else {
                if (g + 4 < gend) {
                    *g++ = 'X'; *g++ = '_';
                    *g++ = hex[ch >> 4]; *g++ = hex[ch & 0x0F];
                    *g++ = '_';
                }
                continue;
            }
            int valid = 1;
            const char *start = s;
            for (int i = 0; i < trail; i++) {
                if (!s[1] || ((unsigned char)s[1] & 0xC0) != 0x80) {
                    valid = 0; break;
                }
                s++;
                cp = (cp << 6) | ((unsigned char)*s & 0x3F);
            }
            if (!valid) {
                s = start; /* reset — hex-encode the lead byte only */
                if (g + 4 < gend) {
                    *g++ = 'X'; *g++ = '_';
                    *g++ = hex[ch >> 4]; *g++ = hex[ch & 0x0F];
                    *g++ = '_';
                }
            } else if (cp <= 0xFFFF) {
                if (g + 5 < gend) {
                    *g++ = '\\'; *g++ = 'u';
                    *g++ = hex[(cp >> 12) & 0x0F];
                    *g++ = hex[(cp >> 8) & 0x0F];
                    *g++ = hex[(cp >> 4) & 0x0F];
                    *g++ = hex[cp & 0x0F];
                }
            } else {
                if (g + 9 < gend) {
                    *g++ = '\\'; *g++ = 'U';
                    *g++ = hex[(cp >> 28) & 0x0F];
                    *g++ = hex[(cp >> 24) & 0x0F];
                    *g++ = hex[(cp >> 20) & 0x0F];
                    *g++ = hex[(cp >> 16) & 0x0F];
                    *g++ = hex[(cp >> 12) & 0x0F];
                    *g++ = hex[(cp >> 8) & 0x0F];
                    *g++ = hex[(cp >> 4) & 0x0F];
                    *g++ = hex[cp & 0x0F];
                }
            }
        } else {
            /* Control chars and special chars → hex-encode as X_XX_ */
            if (g + 4 < gend) {
                *g++ = 'X'; *g++ = '_';
                *g++ = hex[ch >> 4]; *g++ = hex[ch & 0x0F];
                *g++ = '_';
            }
        }
    }
    *g = '\0';

    char *header = codegen_header(prog, guard);
    if (header) {
        FILE *hf = fopen(header_path, "w");
        if (hf) {
            fputs(header, hf);
            fclose(hf);
        } else {
            fprintf(stderr, "phc: warning: cannot write header '%s'\n", header_path);
        }
        free(header);
    }
    free(header_path);

    return 1;
}

static int load_type_manifest(const char *path,
                              DescrType **types, int *count, int *cap) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "phc: error: cannot open manifest '%s'\n", path);
        return 0;
    }
    int initial_count = *count;
    char *line = NULL;
    size_t line_cap = 0;
    while (getline(&line, &line_cap, f) != -1) {
        /* Skip comments and blank lines */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') continue;

        char *saveptr = NULL;
        char *tok = strtok_r(line, " \t\n", &saveptr);
        if (!tok) continue;

        if (strcmp(tok, "enum") == 0) {
            tok = strtok_r(NULL, " \t\n", &saveptr);
            if (!tok) {
                fprintf(stderr, "phc: error: missing type name in manifest '%s'\n", path);
                free(line);
                fclose(f);
                return 0;
            }

            DescrType dt;
            memset(&dt, 0, sizeof(dt));
            dt.name = strdup(tok);
            dt.is_enum = 1;

            int vcap = 0;
            while ((tok = strtok_r(NULL, " \t\n", &saveptr)) != NULL) {
                if (dt.variant_count >= vcap) {
                    vcap = vcap ? vcap * 2 : 8;
                    dt.variant_names = realloc(dt.variant_names,
                                               sizeof(const char *) * (size_t)vcap);
                }
                dt.variant_names[dt.variant_count++] = strdup(tok);
            }

            if (*count >= *cap) {
                *cap = *cap ? *cap * 2 : 8;
                *types = realloc(*types, sizeof(DescrType) * (size_t)*cap);
            }
            (*types)[(*count)++] = dt;

        } else if (strcmp(tok, "descr") == 0) {
            tok = strtok_r(NULL, " \t\n", &saveptr);
            if (!tok) {
                fprintf(stderr, "phc: error: missing type name in manifest '%s'\n", path);
                free(line);
                fclose(f);
                return 0;
            }

            DescrType dt;
            memset(&dt, 0, sizeof(dt));
            dt.name = strdup(tok);

            int vcap = 0;
            while ((tok = strtok_r(NULL, " \t\n", &saveptr)) != NULL) {
                if (dt.variant_count >= vcap) {
                    vcap = vcap ? vcap * 2 : 8;
                    dt.variant_names = realloc(dt.variant_names,
                                               sizeof(const char *) * (size_t)vcap);
                }
                dt.variant_names[dt.variant_count++] = strdup(tok);
            }

            if (dt.variant_count == 0) {
                fprintf(stderr, "phc: error: type '%s' has no variants in manifest '%s'\n",
                        dt.name, path);
                free((void *)dt.name);
                free(line);
                fclose(f);
                return 0;
            }

            if (*count >= *cap) {
                *cap = *cap ? *cap * 2 : 8;
                *types = realloc(*types, sizeof(DescrType) * (size_t)*cap);
            }
            (*types)[(*count)++] = dt;

        } else if (strcmp(tok, "field") == 0) {
            /* field TypeName VariantName field_name type_tokens... */
            char *type_name = strtok_r(NULL, " \t\n", &saveptr);
            char *variant_name = strtok_r(NULL, " \t\n", &saveptr);
            char *field_name = strtok_r(NULL, " \t\n", &saveptr);
            if (!type_name || !variant_name || !field_name) continue;

            /* Collect remaining tokens as type string */
            char **tokens = NULL;
            int ntok = 0;
            int tok_cap = 0;
            while ((tok = strtok_r(NULL, " \t\n", &saveptr)) != NULL) {
                if (ntok >= tok_cap) {
                    tok_cap = tok_cap ? tok_cap * 2 : 8;
                    tokens = realloc(tokens, sizeof(char *) * (size_t)tok_cap);
                }
                tokens[ntok++] = tok;
            }
            if (ntok < 1) { free(tokens); continue; } /* need at least one type token */

            /* Find the DescrType for this type_name */
            DescrType *dt = NULL;
            for (int i = *count - 1; i >= 0; i--) {
                if (strcmp((*types)[i].name, type_name) == 0) {
                    dt = &(*types)[i];
                    break;
                }
            }
            if (!dt) { free(tokens); continue; }

            /* Find variant index */
            int vi = -1;
            for (int i = 0; i < dt->variant_count; i++) {
                if (strcmp(dt->variant_names[i], variant_name) == 0) {
                    vi = i;
                    break;
                }
            }
            if (vi < 0) { free(tokens); continue; }

            /* Allocate variant_fields/counts arrays if needed */
            if (!dt->variant_fields) {
                dt->variant_fields = calloc((size_t)dt->variant_count, sizeof(Field *));
                dt->variant_field_counts = calloc((size_t)dt->variant_count, sizeof(int));
            }

            /* Build type string from all remaining tokens */
            size_t tlen = 0;
            for (int i = 0; i < ntok; i++) {
                tlen += strlen(tokens[i]) + 1;
            }
            char *type_buf = malloc(tlen + 1);
            type_buf[0] = '\0';
            for (int i = 0; i < ntok; i++) {
                if (i > 0) strcat(type_buf, " ");
                strcat(type_buf, tokens[i]);
            }

            int fc = dt->variant_field_counts[vi];
            dt->variant_fields[vi] = realloc(dt->variant_fields[vi],
                                              sizeof(Field) * (size_t)(fc + 1));
            memset(&dt->variant_fields[vi][fc], 0, sizeof(Field));
            dt->variant_fields[vi][fc].type_name = type_buf; /* takes ownership */
            dt->variant_fields[vi][fc].field_name = strdup(field_name);
            dt->variant_field_counts[vi] = fc + 1;
            free(tokens);

        } else {
            continue; /* Skip unknown keywords for forward-compatibility */
        }
    }
    free(line);
    fclose(f);
    if (*count == initial_count) {
        fprintf(stderr, "phc: warning: no types found in manifest '%s'\n", path);
    }
    return 1;
}

static void free_external_types(DescrType *types, int count) {
    for (int i = 0; i < count; i++) {
        free((void *)types[i].name);
        for (int j = 0; j < types[i].variant_count; j++) {
            free((void *)types[i].variant_names[j]);
        }
        free(types[i].variant_names);
        if (types[i].variant_fields) {
            for (int j = 0; j < types[i].variant_count; j++) {
                for (int k = 0; k < types[i].variant_field_counts[j]; k++) {
                    free(types[i].variant_fields[j][k].type_name);
                    free(types[i].variant_fields[j][k].field_name);
                }
                free(types[i].variant_fields[j]);
            }
            free(types[i].variant_fields);
            free(types[i].variant_field_counts);
        }
    }
    free(types);
}

/* --- Main --- */

int main(int argc, char **argv) {
    const char *emit_types_path = NULL;
    const char **manifest_paths = NULL;
    int manifest_count = 0;
    int manifest_cap = 0;
    int strip_check = 0;
    int strip_invariant = 0;

    /* Parse CLI arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--strip-check") == 0) {
            strip_check = 1;
        } else if (strcmp(argv[i], "--strip-invariant") == 0) {
            strip_invariant = 1;
        } else if (strncmp(argv[i], "--emit-types=", 13) == 0) {
            emit_types_path = argv[i] + 13;
        } else if (strncmp(argv[i], "--type-manifest=", 16) == 0) {
            if (manifest_count >= manifest_cap) {
                manifest_cap = manifest_cap ? manifest_cap * 2 : 8;
                manifest_paths = realloc(manifest_paths,
                                         sizeof(const char *) * (size_t)manifest_cap);
            }
            manifest_paths[manifest_count++] = argv[i] + 16;
        } else {
            fprintf(stderr, "phc: error: unknown option '%s'\n", argv[i]);
            free(manifest_paths);
            return 1;
        }
    }

    char *source = read_stdin();

    ParseResult result = parse(source);
    if (result.error) {
        fprintf(stderr, "phc: error: %s (line %d)\n",
                result.error_message, result.error_line);
        parse_result_free(&result);
        free(source);
        free(manifest_paths);
        return 1;
    }

    /* Emit type manifest if requested */
    if (emit_types_path) {
        if (!emit_type_manifest(emit_types_path, &result.program)) {
            parse_result_free(&result);
            free(source);
            free(manifest_paths);
            return 1;
        }
    }

    /* Load external type manifests */
    DescrType *external_types = NULL;
    int external_count = 0;
    int external_cap = 0;
    for (int i = 0; i < manifest_count; i++) {
        if (!load_type_manifest(manifest_paths[i],
                                &external_types, &external_count, &external_cap)) {
            free_external_types(external_types, external_count);
            parse_result_free(&result);
            free(source);
            free(manifest_paths);
            return 1;
        }
    }

    SemanticResult sem = analyse(&result.program, external_types, external_count);
    if (sem.error) {
        fprintf(stderr, "phc: error: %s\n", sem.error_message);
        semantic_result_free(&sem);
        free_external_types(external_types, external_count);
        parse_result_free(&result);
        free(source);
        free(manifest_paths);
        return 1;
    }

    char *output = codegen(&result.program, external_types, external_count,
                           strip_check, strip_invariant);
    /* Free semantic result BEFORE external types: analyse() copies name
     * pointers (not strings) from external_types into its type table,
     * and semantic_result_free() frees the pointer arrays but not the
     * strings themselves — those are owned by external_types. */
    semantic_result_free(&sem);
    free_external_types(external_types, external_count);
    if (!output) {
        parse_result_free(&result);
        free(source);
        free(manifest_paths);
        return 1;
    }

    fputs(output, stdout);

    free(output);
    parse_result_free(&result);
    free(source);
    free(manifest_paths);
    return 0;
}
