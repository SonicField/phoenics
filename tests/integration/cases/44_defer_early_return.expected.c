#include <stdio.h>
#include <stdlib.h>

int process(int fail) {
    char *buf = malloc(100);
    if (fail) {
        printf("early return\n");
        {  free(buf); printf("freed\n"); return -1; }
    }

    printf("normal path\n");
    {  free(buf); printf("freed\n"); return 0; }
}

int main(void) {
    printf("ret=%d\n", process(1));
    printf("ret=%d\n", process(0));
    return 0;
}
