#include <stdio.h>

int main(void) {
    printf("start\n");

    printf("before return\n");
    {  printf("defer 3\n");  printf("defer 2\n");  printf("defer 1\n"); return 0; }
 printf("defer 3\n");
 printf("defer 2\n");
 printf("defer 1\n");
}
