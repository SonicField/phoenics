#ifndef PHC_AST_H
#define PHC_AST_H

#include <stddef.h>

typedef struct {
    char *type_name;    /* reconstructed type (simple types) or NULL (complex) */
    char *field_name;
    char *raw_decl;     /* full declaration text from source (e.g., "void (*cb)(int)") */
    int is_array;       /* nonzero if field is an array type */
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
    char *end_file;     /* source filename (NULL in direct mode) */
} DescrDecl;

/* Forward declaration — Chunk is defined below */
typedef struct Chunk Chunk;

typedef struct {
    char *variant_name;
    char *body_text;        /* raw text (source for body_chunks passthrough offsets) */
    Chunk *body_chunks;     /* parsed sub-program chunks (NULL if no phc constructs) */
    int body_chunk_count;
    char **bindings;        /* field names to destructure (NULL if no parens) */
    int binding_count;
} MatchCase;

typedef struct {
    char *type_name;
    char *expr_text;
    MatchCase *cases;
    int case_count;
    size_t end_pos;         /* position after closing '}' (for passthrough tracking) */
    int end_line;           /* source line after closing '}' */
    char *end_file;         /* source filename (NULL in direct mode) */
} MatchDescr;

typedef struct {
    char *body_text;    /* cleanup code to execute */
    int defer_index;    /* sequential index within function (for label generation) */
} DeferBlock;

typedef struct {
    char *expr;         /* return expression text (NULL for void/bare return) */
    int defer_count;    /* number of active defers at this return point */
} ReturnStmt;

typedef enum {
    CHUNK_PASSTHROUGH,
    CHUNK_DESCR,
    CHUNK_MATCH_DESCR,
    CHUNK_DEFER,
    CHUNK_RETURN,
    CHUNK_FUNC_END      /* function closing brace with pending defers */
} ChunkType;

struct Chunk {
    ChunkType type;
    union {
        struct { size_t start; size_t end; } passthrough;
        int descr_index;
        MatchDescr match;
        DeferBlock defer;
        ReturnStmt ret;
        struct { int defer_count; } func_end;
    };
};

typedef struct {
    const char *source;     /* original source text (not owned) */
    size_t source_len;
    DescrDecl *descrs;
    int descr_count;
    Chunk *chunks;
    int chunk_count;
    DeferBlock *defers;     /* all defer blocks (referenced by chunks) */
    int defer_count;
} Program;

#endif /* PHC_AST_H */
