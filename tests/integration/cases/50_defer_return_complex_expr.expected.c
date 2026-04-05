#include <stdio.h>

int test(void) {
    {  printf("cleanup\n"); return sizeof(struct { int x; int y; }); }
}

int main(void) {
    printf("size=%d\n", test());
    return 0;
}
