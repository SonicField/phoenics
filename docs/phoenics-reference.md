# Phoenics Language Reference

Version: 1.0 (v1)
Date: 2026-04-04

## 1. Overview

Phoenics is a language extension for C11 that adds discriminated unions (sum types) as a first-class type. It is implemented by `phc`, a source-to-source translator that accepts Phoenics-extended C and emits standard C11.

Phoenics is designed for AI agents writing C code. The syntax prioritises unambiguous parsing, explicit structure, and mechanical verifiability over human ergonomics.

### 1.1 Pipeline Position

```
source.phc  -->  phc  -->  source.c  -->  cc (clang/gcc)
```

phc operates on original source files BEFORE the C preprocessor. It transforms Phoenics extensions into standard C11 and passes all other code through byte-for-byte unchanged. The output `.c` file then enters the normal compile pipeline.

### 1.2 Reserved Keywords

Phoenics reserves two keywords: `phc_descr` and `phc_match`. These MUST NOT be used as identifiers in Phoenics source files. All other C identifiers (including `descr` and `match_descr`) are unreserved.

---

## 2. Discriminated Union Declaration (`phc_descr`)

### 2.1 Syntax

```
phc_descr <TypeName> {
    <VariantName> { <field-declarations> },
    <VariantName> { <field-declarations> },
    ...
};
```

### 2.2 Rules

- `TypeName` MUST be a valid C identifier.
- EXACTLY ONE or more variants MUST be present. Zero variants is an error.
- Each `VariantName` MUST be a valid C identifier, unique within the phc_descr.
- Each variant CONTAINS ZERO OR MORE field declarations.
- Each field declaration follows C syntax: type followed by name followed by semicolon.
- Supported field types: simple identifiers (`int`, `double`), multi-word qualifiers (`unsigned int`, `const char`), and pointer types (`char *`, `const char *`).
- Unsupported field types (v1): function pointers, arrays, nested phc_descr.
- Variants are separated by commas. The last variant MAY omit the trailing comma.
- The declaration ends with `};`.

### 2.3 Example

```c
phc_descr Shape {
    Circle { double radius; },
    Rectangle { double width; double height; },
    Triangle { double base; double height; }
};
```

### 2.4 Generated C11

For each `phc_descr`, phc generates four components:

**1. Tag enumeration:**
```c
typedef enum {
    Shape_Circle,
    Shape_Rectangle,
    Shape_Triangle,
    Shape__COUNT
} Shape_Tag;
```

The `<Type>__COUNT` sentinel enables runtime tag validation: `assert(v.tag < Shape__COUNT)`.

**2. Struct with anonymous union:**
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

The anonymous union (C11) allows direct field access: `v.Circle.radius`.

Empty variants generate `struct { char _empty; }` (C forbids zero-size structs).

**3. Constructor functions:**
```c
static inline Shape Shape_mk_Circle(double radius) {
    Shape _v;
    _v.tag = Shape_Circle;
    _v.Circle.radius = radius;
    return _v;
}
```

One constructor per variant. Empty variants take `void`: `Shape_mk_None(void)`.

**4. Safe accessor macros:**
```c
#define Shape_as_Circle(v) \
    (assert((v).tag == Shape_Circle && "Shape: expected Circle"), \
     (v).Circle)
```

One accessor per variant. Usage: `Shape_as_Circle(s).radius`. The assertion fires at runtime if the tag does not match.

**Caveat:** The accessor macro evaluates `v` twice. The argument MUST be a simple lvalue, not a function call or expression with side effects.

### 2.5 Naming Conventions

| Generated name | Pattern | Example |
|----------------|---------|---------|
| Tag enum type | `<Type>_Tag` | `Shape_Tag` |
| Tag enum value | `<Type>_<Variant>` | `Shape_Circle` |
| Variant count | `<Type>__COUNT` | `Shape__COUNT` |
| Struct type | `<Type>` | `Shape` |
| Constructor | `<Type>_mk_<Variant>` | `Shape_mk_Circle` |
| Safe accessor | `<Type>_as_<Variant>` | `Shape_as_Circle` |

---

## 3. Exhaustive Match (`phc_match`)

### 3.1 Syntax

```
phc_match(<TypeName>, <expr>) {
    case <VariantName>: { <body> } break;
    case <VariantName>: { <body> } break;
    ...
}
```

### 3.2 Rules

- `TypeName` MUST reference a `phc_descr` declared earlier in the same file.
- `expr` is any C expression that evaluates to a value of type `TypeName`.
- Every variant of `TypeName` MUST have a corresponding case. Missing variants cause a phc error.
- Duplicate cases cause a phc error.
- Unknown variant names cause a phc error.
- No `default` case is permitted. Exhaustive matching is mandatory.
- Each case body MUST be enclosed in braces.
- Each case MUST end with `break;`.

### 3.3 Example

```c
phc_match(Shape, s) {
    case Circle: {
        printf("circle radius=%g\n", Shape_as_Circle(s).radius);
    } break;
    case Rectangle: {
        printf("rect %gx%g\n", Shape_as_Rectangle(s).width,
                                Shape_as_Rectangle(s).height);
    } break;
    case Triangle: {
        printf("triangle\n");
    } break;
}
```

### 3.4 Generated C11

phc transforms `phc_match` into a standard `switch` statement:

```c
switch (s.tag) {
    case Shape_Circle: {
        printf("circle radius=%g\n", Shape_as_Circle(s).radius);
    } break;
    case Shape_Rectangle: {
        printf("rect %gx%g\n", Shape_as_Rectangle(s).width,
                                Shape_as_Rectangle(s).height);
    } break;
    case Shape_Triangle: {
        printf("triangle\n");
    } break;
    default: break;
}
```

The `default: break;` handles the `__COUNT` sentinel to suppress `-Wswitch` warnings.

### 3.5 Enforcement

The value of `phc_match` is enforcement at transpile time:

- **Adding a variant** to a `phc_descr` causes every `phc_match` on that type (in the same file) to fail until updated. This prevents the "we thought this was X but it's Y" class of bugs.
- **Removing a variant** causes `phc_match` cases referencing it to fail as "unknown variant."
- **Typos in variant names** are caught as "unknown variant" errors.

This is analogous to Pascal's exhaustive case checking on variant records.

---

## 4. Common Patterns

### 4.1 Option Type

```c
phc_descr Option_int {
    Some { int value; },
    None {}
};

Option_int maybe_divide(int a, int b) {
    if (b == 0) return Option_int_mk_None();
    return Option_int_mk_Some(a / b);
}

void use_result(Option_int r) {
    phc_match(Option_int, r) {
        case Some: {
            printf("result: %d\n", Option_int_as_Some(r).value);
        } break;
        case None: {
            printf("division by zero\n");
        } break;
    }
}
```

### 4.2 Result Type

```c
phc_descr Result {
    Ok { int value; },
    Err { int code; const char *message; }
};

Result parse_int(const char *s) {
    char *end;
    long val = strtol(s, &end, 10);
    if (*end != '\0')
        return Result_mk_Err(1, "invalid character");
    if (val > INT_MAX || val < INT_MIN)
        return Result_mk_Err(2, "out of range");
    return Result_mk_Ok((int)val);
}
```

### 4.3 AST Node

```c
phc_descr Expr {
    Literal { int value; },
    BinOp { int op; Expr *left; Expr *right; },
    Unary { int op; Expr *operand; }
};
```

Note: recursive types use pointers (`Expr *`). The C compiler enforces this — a struct cannot contain itself by value.

---

## 5. Error Messages

| Error | Meaning |
|-------|---------|
| `non-exhaustive phc_match on '<Type>': missing variant '<Variant>'` | A phc_match does not cover all variants |
| `duplicate case '<Variant>' in phc_match on '<Type>'` | Same variant appears twice in phc_match |
| `unknown descr type '<Type>'` | phc_match references a type not declared in this file |
| `unknown variant '<Variant>' in phc_match on '<Type>'` | Case names a variant that doesn't exist in the phc_descr |
| `duplicate variant '<Variant>' in descr '<Type>'` | Two variants share a name within a phc_descr |
| `descr '<Type>' must have at least one variant` | Empty phc_descr |

---

## 6. Known Limitations (v1)

1. **Single-file constraint.** `phc_match` can only reference `phc_descr` types declared in the same file. Cross-file usage requires raw `switch` without exhaustiveness checking.
2. **No `#line` directives.** C compiler errors reference phc output line numbers, not original source.
3. **No pattern destructuring.** Case bodies access fields via `<Type>_as_<Variant>(expr).field`, not via bound variables.
4. **No conditional compilation interaction.** `#ifdef` around `phc_descr` variants is not supported.
5. **Field types limited to** identifiers, multi-word qualifiers, and pointer types. Function pointers and arrays are not supported as field types.

---

## 7. Passthrough Guarantee

All C code that is not part of a `phc_descr` or `phc_match` construct passes through phc byte-for-byte unchanged. This includes:

- Preprocessor directives (`#include`, `#define`, `#ifdef`)
- Comments (line and block)
- String and character literals (even those containing `phc_descr` or `phc_match`)
- All C syntax, operators, and expressions

phc will never modify, reformat, or reorder non-Phoenics code.
