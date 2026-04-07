extern int strcmp(const char *, const char *);
#include <stdio.h>

/* Auto-increment from last explicit value */
typedef enum {
    Priority_Critical = 10,
    Priority_High,
    Priority_Medium = 20,
    Priority_Low,
    Priority__COUNT = 4
} Priority;

static inline const char *Priority_to_string(Priority c) {
    switch (c) {
        case Priority_Critical: return "Critical";
        case Priority_High: return "High";
        case Priority_Medium: return "Medium";
        case Priority_Low: return "Low";
        default: return "(unknown)";
    }
}

static inline int Priority_from_string(const char *s, Priority *out) {
    if (strcmp(s, "Critical") == 0) { *out = Priority_Critical; return 1; }
    if (strcmp(s, "High") == 0) { *out = Priority_High; return 1; }
    if (strcmp(s, "Medium") == 0) { *out = Priority_Medium; return 1; }
    if (strcmp(s, "Low") == 0) { *out = Priority_Low; return 1; }
    return 0;
}
#line 10

int main(void) {
    printf("%d %d %d %d\n", Priority_Critical, Priority_High, Priority_Medium, Priority_Low);
    printf("%s\n", Priority_to_string(Priority_High));
    return 0;
}
