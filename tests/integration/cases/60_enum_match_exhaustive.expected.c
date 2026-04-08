#ifndef strcmp
extern int strcmp(const char *, const char *);
#endif
#include <stdio.h>

typedef enum {
    Direction_North,
    Direction_South,
    Direction_East,
    Direction_West,
    Direction__COUNT = 4
} Direction;

static inline const char *Direction_to_string(Direction c) {
    switch (c) {
        case Direction_North: return "North";
        case Direction_South: return "South";
        case Direction_East: return "East";
        case Direction_West: return "West";
        default: return "(unknown)";
    }
}

static inline int Direction_from_string(const char *s, Direction *out) {
    if (strcmp(s, "North") == 0) { *out = Direction_North; return 1; }
    if (strcmp(s, "South") == 0) { *out = Direction_South; return 1; }
    if (strcmp(s, "East") == 0) { *out = Direction_East; return 1; }
    if (strcmp(s, "West") == 0) { *out = Direction_West; return 1; }
    return 0;
}
#line 9

int main(void) {
    Direction d = Direction_East;
    switch (d) {
    case Direction_North: { printf("north\n"); } break;
    case Direction_South: { printf("south\n"); } break;
    case Direction_East: { printf("east\n"); } break;
    case Direction_West: { printf("west\n"); } break;
    default: break;
}
#line 18
    return 0;
}
