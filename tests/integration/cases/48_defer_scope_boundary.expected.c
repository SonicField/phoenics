#include <stdio.h>

int check(int x) {
    /* Return BEFORE any defer — should NOT be converted to goto */
    if (x < 0) {
        printf("early exit (no defer)\n");
        return -1;
    }

    /* Return AFTER defer — should be converted to goto + cleanup */
    if (x == 0) {
        printf("zero path\n");
        {  printf("cleanup\n"); return 0; }
    }

    printf("normal path\n");
    {  printf("cleanup\n"); return x; }
}

int main(void) {
    printf("ret=%d\n", check(-1));
    printf("ret=%d\n", check(0));
    printf("ret=%d\n", check(5));
    return 0;
}
