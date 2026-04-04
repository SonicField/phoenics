#include <stdio.h>

/* Pure C — no Phoenics extensions. Must pass through byte-for-byte. */
struct point { int x, y; };

int add(int a, int b) { return a + b; }

int main(void) {
    struct point p = {1, 2};
    int *ptr = &p.x;
    char c = ':';
    switch (p.x) {
    case 1: printf("hello\n"); break;
    default: break;
    }
    (void)ptr;
    (void)c;
    return 0;
}
