#include <stdio.h>

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

int main(void) {
    Shape s = Shape_mk_Circle(3.14);
    switch (s.tag) {
        case Shape_Circle: {
            printf("circle r=%g\n", s.Circle.radius);
        } break;
        case Shape_Rectangle: {
            printf("rect %gx%g\n", s.Rectangle.width, s.Rectangle.height);
        } break;
            default: break;
    }
    return 0;
}
