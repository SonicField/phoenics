/* Adversarial keyword isolation test.
 * Every occurrence of phc_descr and phc_match below is in a context
 * where it must NOT be interpreted as a Phoenics keyword. */
#include <stdio.h>

/* phc_descr in a multi-line
   block comment that spans
   three lines phc_match */

// phc_descr in a line comment
// phc_match(Shape, s) { case Circle: {} }

int main(void) {
    /* String literals with complete keyword syntax */
    const char *a = "phc_descr Shape { Circle { double r; } };";
    const char *b = "phc_match(Shape, s) { case Circle: {} }";
    const char *c = "nested \"phc_descr\" in escaped string";

    /* Keywords as substrings of longer identifiers */
    int xphc_descr = 10;
    int phc_descrx = 20;
    int xphc_matchx = 30;
    int my_phc_descr_thing = 40;
    int phc_match_extra = 50;

    /* Keywords in adjacent string concatenation */
    const char *d = "phc_" "descr";
    const char *e = "phc" "_match";

    printf("%s\n%s\n%s\n%d %d %d %d %d\n%s\n%s\n",
           a, b, c,
           xphc_descr, phc_descrx, xphc_matchx,
           my_phc_descr_thing, phc_match_extra,
           d, e);
    return 0;
}
