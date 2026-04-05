extern void abort(void);
#include <stdio.h>
#include <assert.h>

typedef enum {
    Shape_Circle,
    Shape_Rectangle,
    Shape__COUNT
} Shape_Tag;

typedef struct {
    double radius;
} Shape_Circle_t;

typedef struct {
    double width;
    double height;
} Shape_Rectangle_t;

typedef struct Shape {
    Shape_Tag tag;
    union {
        Shape_Circle_t Circle;
        Shape_Rectangle_t Rectangle;
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

static inline Shape_Circle_t Shape_as_Circle(Shape v) {
    if (v.tag != Shape_Circle) abort();
    return v.Circle;
}

static inline Shape_Rectangle_t Shape_as_Rectangle(Shape v) {
    if (v.tag != Shape_Rectangle) abort();
    return v.Rectangle;
}
#line 8

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
