#ifndef PHC_AST_H
#define PHC_AST_H

#include <stddef.h>

typedef struct {
    char *type_name;
    char *field_name;
} Field;

typedef struct {
    char *name;
    Field *fields;
    int field_count;
} Variant;

typedef struct {
    char *name;
    Variant *variants;
    int variant_count;
    int end_line;       /* source line after closing ';' */
} DescrDecl;

typedef struct {
    char *variant_name;
    char *body_text;    /* raw text of case body including braces and optional break */
    char **bindings;    /* field names to destructure (NULL if no parens) */
    int binding_count;  /* 0 = no destructuring, -1 not used */
} MatchCase;

typedef struct {
    char *type_name;
    char *expr_text;
    MatchCase *cases;
    int case_count;
    size_t end_pos;         /* position after closing '}' (for passthrough tracking) */
    int end_line;           /* source line after closing '}' */
} MatchDescr;

typedef enum {
    CHUNK_PASSTHROUGH,
    CHUNK_DESCR,
    CHUNK_MATCH_DESCR
} ChunkType;

typedef struct {
    ChunkType type;
    union {
        struct { size_t start; size_t end; } passthrough;
        int descr_index;
        MatchDescr match;
    };
} Chunk;

typedef struct {
    const char *source;     /* original source text (not owned) */
    size_t source_len;
    DescrDecl *descrs;
    int descr_count;
    Chunk *chunks;
    int chunk_count;
} Program;

#endif /* PHC_AST_H */
