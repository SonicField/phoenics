#ifndef PHC_PARSER_H
#define PHC_PARSER_H

#include "ast.h"

typedef struct {
    int error;
    char *error_message;
    int error_line;
    Program program;
} ParseResult;

ParseResult parse(const char *source);
void parse_result_free(ParseResult *result);

#endif /* PHC_PARSER_H */
