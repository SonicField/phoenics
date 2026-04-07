extern int strcmp(const char *, const char *);
typedef enum {
    HttpStatus_OK = 200,
    HttpStatus_NotFound = 404,
    HttpStatus_ServerError = 500,
    HttpStatus__COUNT = 3
} HttpStatus;

static inline const char *HttpStatus_to_string(HttpStatus c) {
    switch (c) {
        case HttpStatus_OK: return "OK";
        case HttpStatus_NotFound: return "NotFound";
        case HttpStatus_ServerError: return "ServerError";
        default: return "(unknown)";
    }
}

static inline int HttpStatus_from_string(const char *s, HttpStatus *out) {
    if (strcmp(s, "OK") == 0) { *out = HttpStatus_OK; return 1; }
    if (strcmp(s, "NotFound") == 0) { *out = HttpStatus_NotFound; return 1; }
    if (strcmp(s, "ServerError") == 0) { *out = HttpStatus_ServerError; return 1; }
    return 0;
}
#line 6
