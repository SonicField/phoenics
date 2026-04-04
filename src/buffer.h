#ifndef PHC_BUFFER_H
#define PHC_BUFFER_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Buffer;

static inline void buf_init(Buffer *b) {
    b->cap = 1024;
    b->data = malloc(b->cap);
    b->data[0] = '\0';
    b->len = 0;
}

static inline void buf_ensure(Buffer *b, size_t extra) {
    while (b->len + extra + 1 > b->cap) {
        b->cap *= 2;
        b->data = realloc(b->data, b->cap);
    }
}

static inline void buf_append(Buffer *b, const char *s) {
    size_t slen = strlen(s);
    buf_ensure(b, slen);
    memcpy(b->data + b->len, s, slen);
    b->len += slen;
    b->data[b->len] = '\0';
}

static inline void buf_append_n(Buffer *b, const char *s, size_t n) {
    buf_ensure(b, n);
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

static inline void buf_append_char(Buffer *b, char c) {
    buf_ensure(b, 1);
    b->data[b->len++] = c;
    b->data[b->len] = '\0';
}

static inline void buf_printf(Buffer *b, const char *fmt, ...) {
    va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    buf_ensure(b, (size_t)needed);
    vsnprintf(b->data + b->len, (size_t)needed + 1, fmt, args2);
    b->len += (size_t)needed;
    va_end(args2);
}

static inline char *buf_finish(Buffer *b) {
    return b->data; /* caller owns */
}

static inline void buf_free(Buffer *b) {
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

#endif /* PHC_BUFFER_H */
