#ifndef PHC_CODEGEN_H
#define PHC_CODEGEN_H

#include "ast.h"
#include "semantic.h"

/*
 * Generate C11 source from the parsed program.
 * Returns a malloc'd string; caller must free.
 * Returns NULL on error (writes to stderr).
 */
char *codegen(const Program *program,
              const DescrType *external_types, int external_type_count);

#endif /* PHC_CODEGEN_H */
