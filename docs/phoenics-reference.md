# Phoenics Language Reference

Phoenics adds discriminated unions to C11. `phc` translates Phoenics-extended C into standard C11. The output compiles with clang or gcc.

Phoenics is for AI agents writing C. The syntax is unambiguous, explicit, and mechanically verifiable. Human ergonomics are secondary.

## Pipeline

```
source.phc  -->  phc  -->  source.c  -->  cc (clang/gcc)
```

phc runs before the preprocessor. Everything that is not a Phoenics construct passes through byte-for-byte unchanged. `#include`, comments, string literals, preprocessor directives — untouched.

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

At least one variant. Variants separated by commas. Fields follow C declaration syntax. Supported field types: identifiers, multi-word qualifiers (`unsigned int`), pointers (`char *`). No function pointers or arrays in v1.

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

`__COUNT` is a sentinel for runtime tag validation: `assert(v.tag < Shape__COUNT)`.

**Struct with anonymous union:**
```c
typedef struct {
    Shape_Tag tag;
    union {
        struct { double radius; } Circle;
        struct { double width; double height; } Rectangle;
        struct { double base; double height; } Triangle;
    };
} Shape;
```

Anonymous unions are C11. Access fields directly: `v.Circle.radius`. Empty variants get `struct { char _empty; }` because C forbids zero-size structs.

**Constructors:**
```c
static inline Shape Shape_mk_Circle(double radius) {
    Shape _v;
    _v.tag = Shape_Circle;
    _v.Circle.radius = radius;
    return _v;
}
```

One per variant. Empty variants take `void`.

**Safe accessors:**
```c
#define Shape_as_Circle(v) \
    (assert((v).tag == Shape_Circle && "Shape: expected Circle"), \
     (v).Circle)
```

Asserts the tag at runtime, then returns the variant struct. Usage: `Shape_as_Circle(s).radius`. The macro evaluates `v` twice — use simple lvalues, not function calls.

### Naming

| What | Pattern | Example |
|------|---------|---------|
| Tag type | `<Type>_Tag` | `Shape_Tag` |
| Tag value | `<Type>_<Variant>` | `Shape_Circle` |
| Count sentinel | `<Type>__COUNT` | `Shape__COUNT` |
| Struct type | `<Type>` | `Shape` |
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

The type must be a `phc_descr` declared earlier in the same file.

### Example

```c
phc_match(Shape, s) {
    case Circle: {
        printf("r=%g\n", Shape_as_Circle(s).radius);
    } break;
    case Rectangle: {
        printf("%gx%g\n", Shape_as_Rectangle(s).width,
                           Shape_as_Rectangle(s).height);
    } break;
    case Triangle: {
        printf("triangle\n");
    } break;
}
```

### Generated C

A standard `switch`:

```c
switch (s.tag) {
    case Shape_Circle: { ... } break;
    case Shape_Rectangle: { ... } break;
    case Shape_Triangle: { ... } break;
    default: break;
}
```

The `default: break;` suppresses `-Wswitch` warnings from the `__COUNT` sentinel.

### What it enforces

Add a variant to a `phc_descr` and every `phc_match` on that type breaks until updated. Remove a variant and cases referencing it fail as "unknown variant." Typos are caught. This is the same safety as Pascal's exhaustive case checking — applied to C.

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
phc_descr Expr {
    Literal { int value; },
    BinOp { int op; Expr *left; Expr *right; },
    Unary { int op; Expr *operand; }
};
```

Recursive types must use pointers. The C compiler rejects a struct that contains itself by value.

---

## Errors

| Message | Cause |
|---------|-------|
| `non-exhaustive phc_match on '<T>': missing variant '<V>'` | Match does not cover all variants |
| `duplicate case '<V>' in phc_match on '<T>'` | Same variant listed twice |
| `unknown descr type '<T>'` | Type not declared in this file |
| `unknown variant '<V>' in phc_match on '<T>'` | Variant does not exist in the type |
| `duplicate variant '<V>' in descr '<T>'` | Two variants share a name |
| `descr '<T>' must have at least one variant` | Empty declaration |

---

## Limitations (v1)

1. **Single-file.** `phc_match` only sees `phc_descr` types from the same file. Cross-file usage falls back to raw `switch`.
2. **No `#line`.** Compiler errors reference phc output, not original source.
3. **No destructuring.** Access fields via `Type_as_Variant(v).field`, not bound variables.
4. **No `#ifdef` interaction.** Conditional variants are not supported.
5. **Field types.** Identifiers, qualifiers, pointers. No function pointers or arrays.
