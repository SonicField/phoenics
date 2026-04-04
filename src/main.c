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

int main(void) {
    char *source = read_stdin();

    ParseResult result = parse(source);
    if (result.error) {
        fprintf(stderr, "phc: error: %s (line %d)\n",
                result.error_message, result.error_line);
        parse_result_free(&result);
        free(source);
        return 1;
    }

    SemanticResult sem = analyse(&result.program);
    if (sem.error) {
        fprintf(stderr, "phc: error: %s\n", sem.error_message);
        semantic_result_free(&sem);
        parse_result_free(&result);
        free(source);
        return 1;
    }

    char *output = codegen(&result.program);
    semantic_result_free(&sem);
    if (!output) {
        parse_result_free(&result);
        free(source);
        return 1;
    }

    fputs(output, stdout);

    free(output);
    parse_result_free(&result);
    free(source);
    return 0;
}
