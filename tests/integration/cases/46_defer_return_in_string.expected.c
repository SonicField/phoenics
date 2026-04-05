#include <stdio.h>

int main(void) {
    /* return inside string must NOT be treated as a return statement */
    printf("return 0;\n");
    printf("fake return\n");
    {  printf("cleanup\n"); return 0; }
}
