#ifndef PHC_LEXER_H
#define PHC_LEXER_H

#include <stddef.h>

typedef enum {
    TOK_EOF = 0,
    TOK_DESCR,      /* 'descr' keyword */
    TOK_IDENT,      /* identifier */
    TOK_LBRACE,     /* { */
    TOK_RBRACE,     /* } */
    TOK_SEMICOLON,  /* ; */
    TOK_COMMA,      /* , */
    TOK_STAR,       /* * */
    TOK_OTHER       /* anything that isn't part of descr syntax */
} TokenType;

typedef struct {
    TokenType type;
    const char *value;   /* points into source or into token's own buf */
    size_t length;
    int line;
    int col;
} Token;

typedef struct {
    const char *src;     /* full source text */
    size_t pos;          /* current position */
    size_t len;          /* total length */
    int line;
    int col;
} Lexer;

void  lexer_init(Lexer *lex, const char *source);
Token lexer_next(Lexer *lex);
Token lexer_peek(Lexer *lex);

#endif /* PHC_LEXER_H */
