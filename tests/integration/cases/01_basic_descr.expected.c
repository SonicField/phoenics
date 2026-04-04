typedef enum {
    Shape_Circle,
    Shape_Rectangle,
    Shape_Triangle
} Shape_Tag;

typedef struct {
    Shape_Tag tag;
    union {
        struct { double radius; } Circle;
        struct { double width; double height; } Rectangle;
        struct { double base; double height; } Triangle;
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
