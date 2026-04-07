#ifndef PHC_SEMANTIC_H
#define PHC_SEMANTIC_H

#include "ast.h"

typedef struct {
    const char *name;
    int variant_count;
    const char **variant_names;
    /* Field info per variant (NULL if unavailable, e.g., v1 manifest) */
    Field **variant_fields;     /* array of Field arrays, one per variant */
    int *variant_field_counts;  /* field count per variant */
    int is_enum;                /* nonzero if this is a phc_enum type */
} DescrType;

typedef struct {
    int error;
    char *error_message;
    DescrType *descr_types;
    int descr_type_count;
} SemanticResult;

SemanticResult analyse(const Program *program,
                       const DescrType *external_types,
                       int external_type_count);
void semantic_result_free(SemanticResult *result);

#endif /* PHC_SEMANTIC_H */
