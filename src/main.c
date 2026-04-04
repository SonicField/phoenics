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
    fprintf(f, "# phc type manifest — machine generated, do not edit\n");
    for (int i = 0; i < prog->descr_count; i++) {
        const DescrDecl *d = &prog->descrs[i];
        fprintf(f, "descr %s", d->name);
        for (int j = 0; j < d->variant_count; j++) {
            fprintf(f, " %s", d->variants[j].name);
        }
        fprintf(f, "\n");
    }
    fclose(f);
    return 1;
}

static int load_type_manifest(const char *path,
                              DescrType **types, int *count, int *cap) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "phc: error: cannot open manifest '%s'\n", path);
        return 0;
    }
    char line[4096];
    while (fgets(line, (int)sizeof(line), f)) {
        /* Skip comments and blank lines */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') continue;

        /* Expect: descr TypeName Variant1 Variant2 ... */
        char *saveptr = NULL;
        char *tok = strtok_r(line, " \t\n", &saveptr);
        if (!tok || strcmp(tok, "descr") != 0) {
            continue; /* Skip unknown keywords for forward-compatibility */
        }

        tok = strtok_r(NULL, " \t\n", &saveptr);
        if (!tok) {
            fprintf(stderr, "phc: error: missing type name in manifest '%s'\n", path);
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
            fclose(f);
            return 0;
        }

        if (*count >= *cap) {
            *cap = *cap ? *cap * 2 : 8;
            *types = realloc(*types, sizeof(DescrType) * (size_t)*cap);
        }
        (*types)[(*count)++] = dt;
    }
    fclose(f);
    return 1;
}

static void free_external_types(DescrType *types, int count) {
    for (int i = 0; i < count; i++) {
        free((void *)types[i].name);
        for (int j = 0; j < types[i].variant_count; j++) {
            free((void *)types[i].variant_names[j]);
        }
        free(types[i].variant_names);
    }
    free(types);
}

/* --- Main --- */

int main(int argc, char **argv) {
    const char *emit_types_path = NULL;
    const char **manifest_paths = NULL;
    int manifest_count = 0;
    int manifest_cap = 0;

    /* Parse CLI arguments */
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--emit-types=", 13) == 0) {
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

    char *output = codegen(&result.program);
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
