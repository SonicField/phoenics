# Phoenics Language Reference

Phoenics adds discriminated unions to C11. `phc` translates Phoenics-extended C into standard C11. The output compiles with clang or gcc.

Phoenics is for AI agents writing C. The syntax is unambiguous, explicit, and mechanically verifiable. Human ergonomics are secondary.

## Pipeline

### Direct mode
```
source.phc  -->  phc  -->  source.c  -->  cc (clang/gcc)
```

### Pipeline mode (post-preprocessor)
```
source.phc  -->  cc -E  -->  phc  -->  cc -x c -
```

phc runs **after** the preprocessor in pipeline mode. `#include`, `#ifdef`, and macros are resolved before phc sees the source. phc emits `#line` directives to restore correct source file and line references in compiler error messages.

Everything that is not a Phoenics construct passes through byte-for-byte unchanged.

Two reserved keywords: `phc_descr` and `phc_match`. All other C identifiers are unreserved.

---

## `phc_descr` — Discriminated Union

### Syntax

```
phc_descr <TypeName> {
    <Variant> { <type> <field>; ... },
    <Variant> { <type> <field>; ... },
    ...
};
```

At least one variant. Variants separated by commas. Fields follow C declaration syntax. Supported field types: identifiers, multi-word qualifiers (`unsigned long long`), pointers (`char *`), function pointers (`void (*callback)(int)`), and arrays (`int data[4]`).

### Forward declarations

```c
phc_descr List;  /* forward declaration */

phc_descr List {
    Cons { int head; List *tail; },
    Nil {}
};
```

Forward declarations enable recursive types. A forward-declared type cannot be used in `phc_match` until fully defined.

### Example

```c
phc_descr Shape {
    Circle { double radius; },
    Rectangle { double width; double height; },
    Triangle { double base; double height; }
};
```

### What phc generates

Four things. No magic — the output is readable C.

**Tag enum:**
```c
typedef enum {
    Shape_Circle,
    Shape_Rectangle,
    Shape_Triangle,
    Shape__COUNT
} Shape_Tag;
```

**Named variant struct typedefs:**
```c
typedef struct {
    double radius;
} Shape_Circle_t;

typedef struct {
    double width;
    double height;
} Shape_Rectangle_t;
```

**Main struct with named tag:**
```c
typedef struct Shape {
    Shape_Tag tag;
    union {
        Shape_Circle_t Circle;
        Shape_Rectangle_t Rectangle;
        Shape_Triangle_t Triangle;
    };
} Shape;
```

Access fields directly: `v.Circle.radius`. Empty variants get `char _empty;` because C forbids zero-size structs.

**Constructors:**
```c
static inline Shape Shape_mk_Circle(double radius) {
    Shape _v;
    _v.tag = Shape_Circle;
    _v.Circle.radius = radius;
    return _v;
}
```

One per variant. Empty variants take `void`. Array fields are skipped in constructor parameters (assign manually after construction).

**Safe accessors:**
```c
static inline Shape_Circle_t Shape_as_Circle(Shape v) {
    if (v.tag != Shape_Circle) abort();
    return v.Circle;
}
```

Asserts the tag at runtime via `abort()`, then returns the variant struct. Usage: `Shape_as_Circle(s).radius`.

### Naming

| What | Pattern | Example |
|------|---------|---------|
| Tag type | `<Type>_Tag` | `Shape_Tag` |
| Tag value | `<Type>_<Variant>` | `Shape_Circle` |
| Count sentinel | `<Type>__COUNT` | `Shape__COUNT` |
| Struct type | `<Type>` | `Shape` |
| Variant typedef | `<Type>_<Variant>_t` | `Shape_Circle_t` |
| Constructor | `<Type>_mk_<Variant>` | `Shape_mk_Circle` |
| Accessor | `<Type>_as_<Variant>` | `Shape_as_Circle` |

---

## `phc_match` — Exhaustive Match

### Syntax

```
phc_match(<TypeName>, <expr>) {
    case <Variant>: { <body> } break;
    case <Variant>: { <body> } break;
    ...
}
```

Every variant must appear. Missing one is a phc error. Duplicates are an error. Unknown variant names are an error. No `default` — exhaustiveness is the point.

The type must be a `phc_descr` declared in the same file or loaded via `--type-manifest`.

### Pattern destructuring

Bind variant fields to local variables in the case label:

```c
phc_match(Shape, s) {
    case Circle(radius): {
        printf("r=%g\n", radius);
    } break;
    case Rectangle(width, height): {
        printf("%gx%g\n", width, height);
    } break;
    case Triangle: {
        printf("triangle\n");
    } break;
}
```

- Binding names must match field names from the `phc_descr` declaration
- Partial destructuring: `Rectangle(width)` binds only width
- Empty parens `Circle()`: no bindings (backward compatible)
- No parens `Circle:`: no bindings (v1 syntax, still works)
- Array fields bind as pointers: `Buffer(data)` where `int data[4]` binds as `int *data`
- Function pointer fields bind directly: `Callback(func)` where `void (*func)(int)` works

### Generated C

A standard `switch` with binding declarations:

```c
switch (s.tag) {
    case Shape_Circle: { double radius = s.Circle.radius; {
        printf("r=%g\n", radius);
    } break; }
    case Shape_Rectangle: { double width = s.Rectangle.width; double height = s.Rectangle.height; {
        printf("%gx%g\n", width, height);
    } break; }
    case Shape_Triangle: {
        printf("triangle\n");
    } break;
    default: break;
}
```

The `default: break;` suppresses `-Wswitch` warnings from the `__COUNT` sentinel.

### What it enforces

Add a variant to a `phc_descr` and every `phc_match` on that type breaks until updated. Remove a variant and cases referencing it fail as "unknown variant." Typos are caught.

---

## Multi-file Support

### Emitting type manifests

```bash
phc --emit-types=shapes.phc-types < shapes.phc > shapes.c
```

Generates a manifest file listing all `phc_descr` types with their variants and field types (v2 format).

### Using type manifests

```bash
phc --type-manifest=shapes.phc-types < main.phc > main.c
```

Loads external type definitions for exhaustiveness checking and pattern destructuring in `phc_match`.

### Manifest v2 format

```
# phc type manifest v2 — machine generated, do not edit
descr Shape Circle Rectangle
field Shape Circle radius double
field Shape Rectangle width double
field Shape Rectangle height double
```

Field lines: `field <Type> <Variant> <field_name> <type_tokens...>`

Backward compatible: v1 manifests (no field lines) work for exhaustiveness checking. Destructuring on v1 manifests produces a clear error.

---

## Source Mapping (#line)

phc emits `#line` directives after generated code to restore correct source line references. In pipeline mode, `#line` includes the original filename:

```c
#line 7 "shapes.phc"
```

Compiler errors reference the original `.phc` source, not the generated output.

---

## Complex Field Types

### Function pointers

```c
phc_descr Handler {
    Callback { void (*func)(int); },
    Direct { int value; }
};
```

Function pointer fields work in constructors, accessors, and destructuring.

### Arrays

```c
phc_descr Buffer {
    Data { int values[4]; int count; },
    Empty {}
};
```

Array fields are emitted correctly in structs. Constructor parameters skip array fields (arrays can't be passed by value in C). Destructuring binds array fields as pointers (`int *values`).

---

## Patterns

### Option

```c
phc_descr Option_int {
    Some { int value; },
    None {}
};

Option_int maybe_divide(int a, int b) {
    if (b == 0) return Option_int_mk_None();
    return Option_int_mk_Some(a / b);
}
```

### Result

```c
phc_descr Result {
    Ok { int value; },
    Err { int code; const char *message; }
};
```

### Recursive (AST nodes)

```c
phc_descr Expr;  /* forward declaration for self-reference */

phc_descr Expr {
    Literal { int value; },
    BinOp { int op; Expr *left; Expr *right; },
    Unary { int op; Expr *operand; }
};
```

Recursive types require a forward declaration and pointer fields.

---

## Errors

| Message | Cause |
|---------|-------|
| `non-exhaustive phc_match on '<T>': missing variant '<V>'` | Match does not cover all variants |
| `duplicate case '<V>' in phc_match on '<T>'` | Same variant listed twice |
| `unknown phc_descr type '<T>'` | Type not declared or loaded via manifest |
| `unknown variant '<V>' in phc_match on '<T>'` | Variant does not exist in the type |
| `duplicate phc_descr type '<T>'` | Two full definitions share a name |
| `duplicate variant '<V>' in descr '<T>'` | Two variants share a name |
| `descr '<T>' must have at least one variant` | Empty declaration |
| `cannot match on forward-declared type '<T>'` | Type only forward-declared, no full definition |
| `unknown field '<F>' in variant '<V>' of '<T>'` | Destructuring names a nonexistent field |
| `duplicate binding '<F>' in case '<V>'` | Same field bound twice in destructuring |
| `cannot destructure external type '<T>' (field info not available)` | v1 manifest without field types |

---

## Design Decisions

- **Nested phc_descr:** Not supported. Define types at file scope instead — this is idiomatic C.
- **Conditional compilation:** Works in pipeline mode (`cc -E` resolves `#ifdef` before phc).
- **Field name binding:** Binding names must match field names exactly. No positional or renamed bindings.
