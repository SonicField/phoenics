extern int strcmp(const char *, const char *);
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    Action_Start,
    Action_Stop,
    Action_Pause,
    Action__COUNT = 3
} Action;

static inline const char *Action_to_string(Action c) {
    switch (c) {
        case Action_Start: return "Start";
        case Action_Stop: return "Stop";
        case Action_Pause: return "Pause";
        default: return "(unknown)";
    }
}

static inline int Action_from_string(const char *s, Action *out) {
    if (strcmp(s, "Start") == 0) { *out = Action_Start; return 1; }
    if (strcmp(s, "Stop") == 0) { *out = Action_Stop; return 1; }
    if (strcmp(s, "Pause") == 0) { *out = Action_Pause; return 1; }
    return 0;
}
#line 9

const char *describe_action(Action a) {
    char *buf = malloc(64);
    switch (a) {
    case Action_Start: { {  free(buf); return "starting"; } } break;
    case Action_Stop: { {  free(buf); return "stopping"; } } break;
    case Action_Pause: { {  free(buf); return "pausing"; } } break;
    default: break;
}
#line 19
    {  free(buf); return "unknown"; }
 free(buf);
}

int main(void) {
    printf("%s\n", describe_action(Action_Stop));
    return 0;
}
