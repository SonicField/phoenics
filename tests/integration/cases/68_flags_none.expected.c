#include <stdio.h>

typedef unsigned int Status;
#define Status_Active (1u << 0)
#define Status_Pending (1u << 1)
#define Status_Closed (1u << 2)
#define Status__ALL (Status_Active | Status_Pending | Status_Closed)
#define Status__COUNT 3
#define Status__MAX_STRING 22

static inline int Status_has(Status flags, Status flag) {
    return (flags & flag) == flag;
}

static inline Status Status_set(Status flags, Status flag) {
    return flags | flag;
}

static inline Status Status_clear(Status flags, Status flag) {
    return flags & ~flag;
}

static inline const char *Status_to_string(Status p, char *buf, unsigned long len) {
    if (len == 0) return buf;
    char *pos = buf;
    char *end = buf + len - 1;
    if (p & Status_Active) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Active"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (p & Status_Pending) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Pending"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (p & Status_Closed) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Closed"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (pos == buf) {
        const char *s = "(none)"; while (*s && pos < end) *pos++ = *s++;
    }
    *pos = '\0';
    return buf;
}
#line 8

int main(void) {
    Status s = 0;
    char buf[64];
    printf("%s\n", Status_to_string(s, buf, sizeof(buf)));
    return 0;
}
