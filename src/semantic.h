#ifndef PHC_SEMANTIC_H
#define PHC_SEMANTIC_H

#include "ast.h"

typedef struct {
    const char *name;
    int variant_count;
    const char **variant_names;
} DescrType;

typedef struct {
    int error;
    char *error_message;
    DescrType *descr_types;
    int descr_type_count;
} SemanticResult;

SemanticResult analyse(const Program *program);
void semantic_result_free(SemanticResult *result);

#endif /* PHC_SEMANTIC_H */
