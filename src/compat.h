/*
 * compat.h — MSVC / Windows compatibility shims
 *
 * On POSIX systems this header is a no-op.  On MSVC it provides
 * implementations of POSIX functions that phc relies on:
 *   getline, strtok_r, strdup, strndup, ssize_t
 */
#ifndef PHC_COMPAT_H
#define PHC_COMPAT_H

#ifdef _MSC_VER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ssize_t — MSVC doesn't define it in the C standard headers */
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;

/* strtok_r — MSVC calls it strtok_s (same signature) */
#define strtok_r(str, delim, saveptr) strtok_s(str, delim, saveptr)

/* strdup — MSVC prefixes with underscore */
#define strdup _strdup

/* strndup — MSVC doesn't have it at all */
static inline char *strndup(const char *s, size_t n) {
    size_t len = strlen(s);
    if (len > n) len = n;
    char *copy = (char *)malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}

/* getline — POSIX function, not available on MSVC */
static inline ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
    if (!lineptr || !n || !stream) return -1;

    size_t pos = 0;
    int c;

    if (!*lineptr || *n == 0) {
        *n = 128;
        *lineptr = (char *)malloc(*n);
        if (!*lineptr) return -1;
    }

    while ((c = fgetc(stream)) != EOF) {
        if (pos + 1 >= *n) {
            *n *= 2;
            char *tmp = (char *)realloc(*lineptr, *n);
            if (!tmp) return -1;
            *lineptr = tmp;
        }
        (*lineptr)[pos++] = (char)c;
        if (c == '\n') break;
    }

    if (pos == 0 && c == EOF) return -1;

    (*lineptr)[pos] = '\0';
    return (ssize_t)pos;
}

#endif /* _MSC_VER */

#endif /* PHC_COMPAT_H */
