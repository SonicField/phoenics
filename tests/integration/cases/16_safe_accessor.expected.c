#include <stdio.h>
#include <assert.h>

typedef enum {
    Shape_Circle,
    Shape_Rectangle,
    Shape__COUNT
} Shape_Tag;

typedef struct {
    Shape_Tag tag;
    union {
        struct { double radius; } Circle;
        struct { double width; double height; } Rectangle;
    };
} Shape;

static inline Shape Shape_mk_Circle(double radius) {
    Shape _v;
    _v.tag = Shape_Circle;
    _v.Circle.radius = radius;
    return _v;
}

static inline Shape Shape_mk_Rectangle(double width, double height) {
    Shape _v;
    _v.tag = Shape_Rectangle;
    _v.Rectangle.width = width;
    _v.Rectangle.height = height;
    return _v;
}

#define Shape_as_Circle(v) \
    (assert((v).tag == Shape_Circle && "Shape: expected Circle"), \
     (v).Circle)

#define Shape_as_Rectangle(v) \
    (assert((v).tag == Shape_Rectangle && "Shape: expected Rectangle"), \
     (v).Rectangle)


int main(void) {
    Shape s = Shape_mk_Circle(3.14);

    /* Safe accessor macro — asserts tag is correct, returns variant struct */
    double r = Shape_as_Circle(s).radius;
    printf("radius: %g\n", r);

    Shape rect = Shape_mk_Rectangle(10.0, 5.0);
    double area = Shape_as_Rectangle(rect).width * Shape_as_Rectangle(rect).height;
    printf("area: %g\n", area);

    return 0;
}
