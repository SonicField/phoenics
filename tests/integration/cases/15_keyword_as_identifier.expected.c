/* Test that 'descr' and 'match_descr' (without phc_ prefix) pass through
 * unchanged as ordinary C identifiers. Only 'phc_descr' and 'phc_match'
 * are Phoenics keywords. */
#include <stdio.h>

struct foo {
    int descr;
    int match_descr;
};

int descr = 42;

int main(void) {
    struct foo f;
    f.descr = 1;
    f.match_descr = 2;
    printf("%d %d %d\n", f.descr, f.match_descr, descr);
    return 0;
}
