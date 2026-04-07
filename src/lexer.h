#ifndef PHC_LEXER_H
#define PHC_LEXER_H

#include <stddef.h>

typedef enum {
    TOK_EOF = 0,
    TOK_DESCR,        /* 'descr' keyword */
    TOK_ENUM,         /* 'enum' keyword (phc_enum) */
    TOK_MATCH_DESCR,  /* 'match_descr' keyword */
    TOK_IDENT,        /* identifier */
    TOK_LBRACE,       /* { */
    TOK_RBRACE,       /* } */
    TOK_LPAREN,       /* ( */
    TOK_RPAREN,       /* ) */
    TOK_SEMICOLON,    /* ; */
    TOK_COMMA,        /* , */
    TOK_STAR,         /* * */
    TOK_COLON,        /* : */
    TOK_CASE,         /* 'case' keyword */
    TOK_BREAK,        /* 'break' keyword */
    TOK_PHC_DEFER,    /* 'phc_defer' keyword */
    TOK_PHC_DEFER_CANCEL, /* 'phc_defer_cancel' keyword */
    TOK_RETURN,       /* 'return' keyword (scan mode, only when defer active) */
    TOK_EQUALS,       /* = (for explicit enum values) */
    TOK_NUMBER,       /* integer literal */
    TOK_OTHER         /* anything that isn't part of descr/match_descr syntax */
} TokenType;

typedef enum {
    LEXER_SCAN,       /* passthrough: accumulate into TOK_OTHER */
    LEXER_STRUCT      /* structured: tokenize into individual tokens */
} LexerMode;

typedef struct {
    TokenType type;
    const char *value;   /* pointer into source (may not be null-terminated) */
    size_t length;
    int line;
    int col;
    size_t pos;          /* byte offset in source */
    int orig_line;       /* original source line (from preprocessor markers) */
} Token;

typedef struct {
    const char *src;     /* full source text */
    size_t pos;          /* current position */
    size_t len;          /* total length */
    int line;
    int col;
    LexerMode mode;
    int brace_depth;     /* tracked in STRUCT mode */
    int depth_zero_seen; /* flag: closing } brought depth to 0 */
    /* Defer tracking */
    int defer_active;    /* nonzero when phc_defer is pending in current function */
    int scan_brace_depth; /* brace depth in scan mode (for function boundary tracking) */
    /* Preprocessor marker tracking (pipeline mode) */
    int orig_line;       /* original source line from last # marker */
    const char *orig_file; /* pointer into source for filename */
    size_t orig_file_len;
    int marker_seen;     /* nonzero if any preprocessor marker was seen */
} Lexer;

void  lexer_init(Lexer *lex, const char *source);
Token lexer_next(Lexer *lex);
Token lexer_peek(Lexer *lex);

#endif /* PHC_LEXER_H */
