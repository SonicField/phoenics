#include <stdio.h>

int test(void) {
    {  printf("cleanup\n"); return sizeof(struct { int x; int y; }); }
 printf("cleanup\n");
}

int main(void) {
    printf("size=%d\n", test());
    return 0;
}
