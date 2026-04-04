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
    char *body_text;    /* raw text of case body (for inspection/testing) */
    size_t name_pos;    /* byte offset of variant name in source */
    size_t name_len;    /* length of variant name */
} MatchCase;

typedef struct {
    char *type_name;
    char *expr_text;
    MatchCase *cases;
    int case_count;
    size_t start_pos;       /* position of 'match_descr' keyword */
    size_t header_end_pos;  /* position after ')' */
    size_t lbrace_pos;      /* position of opening '{' */
    size_t rbrace_pos;      /* position of closing '}' */
    size_t end_pos;         /* position after closing '}' */
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
