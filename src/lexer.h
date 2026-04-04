#ifndef PHC_LEXER_H
#define PHC_LEXER_H

#include <stddef.h>

typedef enum {
    TOK_EOF = 0,
    TOK_DESCR,        /* 'descr' keyword */
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
} Lexer;

void  lexer_init(Lexer *lex, const char *source);
Token lexer_next(Lexer *lex);
Token lexer_peek(Lexer *lex);

#endif /* PHC_LEXER_H */
