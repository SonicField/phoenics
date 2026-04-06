extern void abort(void);
#define phc_free(pp) do { free(*(pp)); *(pp) = ((void*)0); } while(0)
#include <stdio.h>

#define HAVE_SHAPE 1

#ifdef HAVE_SHAPE
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
#line 10
#endif

int main(void) {
#ifdef HAVE_SHAPE
    Shape s = Shape_mk_Circle(2.0);
    switch (s.tag) {
    case Shape_Circle: { double radius = s.Circle.radius; { printf("r=%g\n", radius); } break; }
    case Shape_Rectangle: { printf("rect\n"); } break;
    default: break;
}
#line 19
#else
    printf("no shape\n");
#endif
    return 0;
}
