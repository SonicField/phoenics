#include "semantic.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static void sem_error(SemanticResult *sr, const char *fmt, ...) {
    if (sr->error) return;
    sr->error = 1;
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    sr->error_message = strdup(buf);
}

SemanticResult analyse(const Program *prog) {
    SemanticResult sr;
    memset(&sr, 0, sizeof(sr));

    /* Build type table from descr declarations */
    sr.descr_type_count = prog->descr_count;
    if (sr.descr_type_count > 0) {
        sr.descr_types = calloc((size_t)sr.descr_type_count, sizeof(DescrType));
    }

    for (int i = 0; i < prog->descr_count; i++) {
        const DescrDecl *d = &prog->descrs[i];
        sr.descr_types[i].name = d->name;
        sr.descr_types[i].variant_count = d->variant_count;
        sr.descr_types[i].variant_names =
            calloc((size_t)d->variant_count, sizeof(const char *));
        for (int j = 0; j < d->variant_count; j++) {
            sr.descr_types[i].variant_names[j] = d->variants[j].name;
        }
    }

    /* Check for duplicate descr names */
    for (int i = 0; i < prog->descr_count && !sr.error; i++) {
        for (int j = i + 1; j < prog->descr_count && !sr.error; j++) {
            if (strcmp(prog->descrs[i].name, prog->descrs[j].name) == 0) {
                sem_error(&sr, "duplicate descr type '%s'", prog->descrs[i].name);
            }
        }
    }

    /* Check for duplicate variant names within each descr */
    for (int i = 0; i < prog->descr_count && !sr.error; i++) {
        const DescrDecl *d = &prog->descrs[i];
        for (int j = 0; j < d->variant_count && !sr.error; j++) {
            for (int k = j + 1; k < d->variant_count && !sr.error; k++) {
                if (strcmp(d->variants[j].name, d->variants[k].name) == 0) {
                    sem_error(&sr, "duplicate variant '%s' in descr '%s'",
                              d->variants[j].name, d->name);
                }
            }
        }
    }

    /* Validate match_descr constructs */
    for (int ci = 0; ci < prog->chunk_count && !sr.error; ci++) {
        if (prog->chunks[ci].type != CHUNK_MATCH_DESCR) continue;
        const MatchDescr *m = &prog->chunks[ci].match_descr;

        /* Find the descr type */
        const DescrDecl *d = NULL;
        for (int di = 0; di < prog->descr_count; di++) {
            if (strcmp(prog->descrs[di].name, m->type_name) == 0) {
                d = &prog->descrs[di];
                break;
            }
        }
        if (!d) {
            sem_error(&sr, "unknown descr type '%s'", m->type_name);
            break;
        }

        /* Check for duplicate cases */
        for (int i = 0; i < m->case_count && !sr.error; i++) {
            for (int j = i + 1; j < m->case_count && !sr.error; j++) {
                if (strcmp(m->cases[i].variant_name, m->cases[j].variant_name) == 0) {
                    sem_error(&sr, "duplicate case '%s' in match_descr on '%s'",
                              m->cases[i].variant_name, m->type_name);
                }
            }
        }

        /* Check for unknown variants in cases */
        for (int i = 0; i < m->case_count && !sr.error; i++) {
            int found = 0;
            for (int vi = 0; vi < d->variant_count; vi++) {
                if (strcmp(m->cases[i].variant_name, d->variants[vi].name) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                sem_error(&sr, "unknown variant '%s' in match_descr on '%s'",
                          m->cases[i].variant_name, m->type_name);
            }
        }

        /* Check exhaustiveness */
        for (int vi = 0; vi < d->variant_count && !sr.error; vi++) {
            int found = 0;
            for (int cj = 0; cj < m->case_count; cj++) {
                if (strcmp(d->variants[vi].name, m->cases[cj].variant_name) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                sem_error(&sr,
                          "non-exhaustive match_descr on '%s': missing variant '%s'",
                          m->type_name, d->variants[vi].name);
            }
        }
    }

    return sr;
}

void semantic_result_free(SemanticResult *sr) {
    free(sr->error_message);
    for (int i = 0; i < sr->descr_type_count; i++) {
        free(sr->descr_types[i].variant_names);
    }
    free(sr->descr_types);
    memset(sr, 0, sizeof(*sr));
}
