#ifndef PHC_CODEGEN_H
#define PHC_CODEGEN_H

#include "ast.h"

/*
 * Generate C11 source from the parsed program.
 * Returns a malloc'd string; caller must free.
 * Returns NULL on error (writes to stderr).
 */
char *codegen(const Program *program);

#endif /* PHC_CODEGEN_H */
