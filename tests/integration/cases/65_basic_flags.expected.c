#include <stdio.h>

typedef unsigned int Permissions;
#define Permissions_Read (1u << 0)
#define Permissions_Write (1u << 1)
#define Permissions_Execute (1u << 2)
#define Permissions__ALL (Permissions_Read | Permissions_Write | Permissions_Execute)
#define Permissions__COUNT 3
#define Permissions__MAX_STRING 19

static inline int Permissions_has(Permissions flags, Permissions flag) {
    return (flags & flag) == flag;
}

static inline Permissions Permissions_set(Permissions flags, Permissions flag) {
    return flags | flag;
}

static inline Permissions Permissions_clear(Permissions flags, Permissions flag) {
    return flags & ~flag;
}

static inline const char *Permissions_to_string(Permissions p, char *buf, unsigned long len) {
    if (len == 0) return buf;
    char *pos = buf;
    char *end = buf + len - 1;
    if (p & Permissions_Read) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Read"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (p & Permissions_Write) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Write"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (p & Permissions_Execute) {
        if (pos != buf && pos < end) *pos++ = '|';
        { const char *s = "Execute"; while (*s && pos < end) *pos++ = *s++; }
    }
    if (pos == buf) {
        const char *s = "(none)"; while (*s && pos < end) *pos++ = *s++;
    }
    *pos = '\0';
    return buf;
}
#line 8

int main(void) {
    Permissions p = Permissions_Read | Permissions_Write;
    char buf[64];
    printf("%s\n", Permissions_to_string(p, buf, sizeof(buf)));
    printf("has read: %d\n", Permissions_has(p, Permissions_Read));
    printf("has exec: %d\n", Permissions_has(p, Permissions_Execute));
    return 0;
}
