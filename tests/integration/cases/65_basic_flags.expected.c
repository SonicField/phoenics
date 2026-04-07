extern int snprintf(char *, unsigned long, const char *, ...);
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
    buf[0] = '\0';
    unsigned long pos = 0;
    if (p & Permissions_Read) { pos += snprintf(buf + pos, len - pos, "%sRead", pos ? "|" : ""); }
    if (p & Permissions_Write) { pos += snprintf(buf + pos, len - pos, "%sWrite", pos ? "|" : ""); }
    if (p & Permissions_Execute) { pos += snprintf(buf + pos, len - pos, "%sExecute", pos ? "|" : ""); }
    if (pos == 0) { snprintf(buf, len, "(none)"); }
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
