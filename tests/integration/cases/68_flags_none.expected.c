extern int snprintf(char *, unsigned long, const char *, ...);
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
    buf[0] = '\0';
    unsigned long pos = 0;
    if (p & Status_Active) { pos += snprintf(buf + pos, len - pos, "%sActive", pos ? "|" : ""); }
    if (p & Status_Pending) { pos += snprintf(buf + pos, len - pos, "%sPending", pos ? "|" : ""); }
    if (p & Status_Closed) { pos += snprintf(buf + pos, len - pos, "%sClosed", pos ? "|" : ""); }
    if (pos == 0) { snprintf(buf, len, "(none)"); }
    return buf;
}
#line 8

int main(void) {
    Status s = 0;
    char buf[64];
    printf("%s\n", Status_to_string(s, buf, sizeof(buf)));
    return 0;
}
