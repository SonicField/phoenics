extern void abort(void);
#define phc_free(pp) do { free(*(pp)); *(pp) = ((void*)0); } while(0)
typedef enum {
    Shape_Circle,
    Shape_Rectangle,
    Shape_Triangle,
    Shape__COUNT
} Shape_Tag;

typedef struct {
    double radius;
} Shape_Circle_t;

typedef struct {
    double width;
    double height;
} Shape_Rectangle_t;

typedef struct {
    double base;
    double height;
} Shape_Triangle_t;

typedef struct Shape {
    Shape_Tag tag;
    union {
        Shape_Circle_t Circle;
        Shape_Rectangle_t Rectangle;
        Shape_Triangle_t Triangle;
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

static inline Shape Shape_mk_Triangle(double base, double height) {
    Shape _v;
    _v.tag = Shape_Triangle;
    _v.Triangle.base = base;
    _v.Triangle.height = height;
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

static inline Shape_Triangle_t Shape_as_Triangle(Shape v) {
    if (v.tag != Shape_Triangle) abort();
    return v.Triangle;
}
#line 6
