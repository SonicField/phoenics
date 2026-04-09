# Phoenics Language Reference

Phoenics adds discriminated unions and enhanced enums to C11. `phc` translates Phoenics-extended C into standard C11. The output compiles with clang, gcc, or MSVC.

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

Reserved keywords: `phc_descr`, `phc_enum`, `phc_match`, `phc_defer`, `phc_defer_cancel`, `phc_free`. All other C identifiers are unreserved.

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

## `phc_enum` — Enhanced Enum

C enums are integers with names. phc_enum adds what C forgot: string conversion and exhaustive matching.

### Syntax

```
phc_enum <TypeName> {
    <Name>,
    <Name> = <integer>,
    ...
};
```

At least one value. Values separated by commas. Explicit integer assignment is optional — without it, values auto-increment from the previous value (starting at 0), matching C enum behaviour.

### Example

```c
phc_enum Color {
    Red,
    Green,
    Blue
};
```

With explicit values (protocol/wire formats):

```c
phc_enum HttpStatus {
    Ok = 200,
    NotFound = 404,
    ServerError = 500
};
```

Mixed auto-increment and explicit:

```c
phc_enum Priority {
    Low,           /* 0 */
    Medium = 10,   /* 10 */
    High,          /* 11 */
    Critical = 20  /* 20 */
};
```

### What phc generates

Three things.

**Enum typedef:**
```c
typedef enum {
    Color_Red,
    Color_Green,
    Color_Blue,
    Color__COUNT = 3
} Color;
```

`__COUNT` is always the number of declared values, explicitly assigned. For contiguous 0-based enums this equals the next sequential value. For non-contiguous enums (HttpStatus), `__COUNT = 3` — the count of values, not one past the last.

**String conversion (to_string):**
```c
static inline const char *Color_to_string(Color c) {
    switch (c) {
        case Color_Red: return "Red";
        case Color_Green: return "Green";
        case Color_Blue: return "Blue";
        default: return "(unknown)";
    }
}
```

The strings match the source names exactly. No allocation. The default case handles values outside the enum range — this happens in C because enums are integers.

**String parsing (from_string):**
```c
static inline int Color_from_string(const char *s, Color *out) {
    if (strcmp(s, "Red") == 0) { *out = Color_Red; return 1; }
    if (strcmp(s, "Green") == 0) { *out = Color_Green; return 1; }
    if (strcmp(s, "Blue") == 0) { *out = Color_Blue; return 1; }
    return 0;
}
```

Returns 1 on match, 0 on failure. phc emits `extern int strcmp(const char *, const char *);` in the preamble.

### Naming

| What | Pattern | Example |
|------|---------|---------|
| Enum type | `<Type>` | `Color` |
| Value | `<Type>_<Name>` | `Color_Red` |
| Count | `<Type>__COUNT` | `Color__COUNT` |
| To string | `<Type>_to_string` | `Color_to_string` |
| From string | `<Type>_from_string` | `Color_from_string` |

### Exhaustive matching

phc_enum types work with `phc_match`. Every value must be covered:

```c
phc_match(Color, c) {
    case Red: { printf("red\n"); } break;
    case Green: { printf("green\n"); } break;
    case Blue: { printf("blue\n"); } break;
}
```

No destructuring — enum values carry no fields. Missing a value is a phc error. Adding a new value to a phc_enum breaks every phc_match on that type until updated.

Generated C for enum matching uses `switch(expr)` directly (not `switch(expr.tag)` as with phc_descr, since enums are values, not structs).

### Manifest support

phc_enum types appear in type manifests (`--emit-types`). Cross-file exhaustiveness checking works the same as phc_descr.

---

## `phc_flags` — Type-Safe Bitflags

C flag enums use manual power-of-2 constants. Programmers miscalculate, duplicate values, and debug-printing flags is always manual. phc_flags generates the constants and the helpers.

### Syntax

```
phc_flags <TypeName> {
    <Name>,
    <Name>,
    ...
};
```

At least one flag. Flags separated by commas. Two modes, not mixed:

- **Auto-assigned:** omit values. The first flag is `(1u << 0)`, the second `(1u << 1)`, and so on.
- **Explicit masks:** assign every flag. For hardware registers and protocol definitions where bit positions are externally specified.

If some flags have explicit values and others do not, phc rejects the declaration. All-auto or all-explicit — no mixing.

### Example

```c
phc_flags Permissions {
    Read,
    Write,
    Execute
};
```

With explicit masks (hardware registers):

```c
phc_flags GpioPin {
    Input  = 0x01,
    Output = 0x02,
    PullUp = 0x10,
    PullDown = 0x20
};
```

### What phc generates

Four things.

**Type and constants:**
```c
typedef unsigned int Permissions;
#define Permissions_Read    (1u << 0)
#define Permissions_Write   (1u << 1)
#define Permissions_Execute (1u << 2)
#define Permissions__ALL    (Permissions_Read | Permissions_Write | Permissions_Execute)
#define Permissions__COUNT  3
#define Permissions__MAX_STRING 21  /* "Read|Write|Execute" + null */
```

The type is `unsigned int`, not `enum` — bitwise OR of enum values is implementation-defined in some compilers. `__COUNT` is the number of declared flags. `__MAX_STRING` is the buffer size needed for `to_string` with all flags set.

**Helpers:**
```c
static inline int Permissions_has(Permissions flags, Permissions flag) {
    return (flags & flag) == flag;
}
static inline Permissions Permissions_set(Permissions flags, Permissions flag) {
    return flags | flag;
}
static inline Permissions Permissions_clear(Permissions flags, Permissions flag) {
    return flags & ~flag;
}
```

**Debug printing (to_string):**
```c
static inline const char *Permissions_to_string(Permissions p, char *buf, size_t len) {
    buf[0] = '\0';
    if (p & Permissions_Read) snprintf(buf + strlen(buf), len - strlen(buf), "%sRead", buf[0] ? "|" : "");
    if (p & Permissions_Write) snprintf(buf + strlen(buf), len - strlen(buf), "%sWrite", buf[0] ? "|" : "");
    if (p & Permissions_Execute) snprintf(buf + strlen(buf), len - strlen(buf), "%sExecute", buf[0] ? "|" : "");
    if (buf[0] == '\0') snprintf(buf, len, "(none)");
    return buf;
}
```

Caller provides the buffer. Use `__MAX_STRING` for sizing:

```c
char buf[Permissions__MAX_STRING];
printf("%s\n", Permissions_to_string(perms, buf, sizeof(buf)));
```

No allocation. Thread-safe. The caller owns the buffer.

### Naming

| What | Pattern | Example |
|------|---------|---------|
| Type | `<Type>` | `Permissions` |
| Flag constant | `<Type>_<Name>` | `Permissions_Read` |
| All flags | `<Type>__ALL` | `Permissions__ALL` |
| Flag count | `<Type>__COUNT` | `Permissions__COUNT` |
| Max string size | `<Type>__MAX_STRING` | `Permissions__MAX_STRING` |
| Has flag | `<Type>_has` | `Permissions_has` |
| Set flag | `<Type>_set` | `Permissions_set` |
| Clear flag | `<Type>_clear` | `Permissions_clear` |
| To string | `<Type>_to_string` | `Permissions_to_string` |

### Constraints

- Maximum 32 flags per type (fits `unsigned int`). More than 32 is a phc error.
- Explicit mask values must be powers of 2. Non-power-of-2 masks are a phc error.
- phc_flags types do NOT work with `phc_match` — flags are combinable, not exclusive. Use bitwise tests instead.

### Manifest support

phc_flags types appear in type manifests (`--emit-types`).

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

The type must be a `phc_descr` or `phc_enum` declared in the same file or loaded via `--type-manifest`. Destructuring (binding fields) is available for `phc_descr` types only — `phc_enum` values carry no fields.

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

### Emitting type manifests and headers

```bash
phc --emit-types=shapes.phc-types < shapes.phc > shapes.c
```

Generates two files:

1. **`shapes.phc-types`** — type manifest for exhaustiveness checking (variant names, field types).
2. **`shapes.phc.h`** — generated C header containing the full type definitions: typedef, constructors, accessors, to_string/from_string, helpers. Self-contained with include guard.

The manifest carries metadata. The header carries code. Both are machine-generated.

### Using shared types

A consuming file needs both: the manifest for phc's exhaustiveness checker, and the header for the C compiler.

```bash
phc --type-manifest=shapes.phc-types < main.phc > main.c
```

In the `.phc` source:

```c
#include "shapes.phc.h"   /* C preprocessor pulls in the generated types */

void draw(Shape s) {
    phc_match(Shape, s) {
        case Circle(radius): { /* ... */ } break;
        case Rectangle(width, height): { /* ... */ } break;
    }
}
```

The `#include` gives the C compiler the type definitions. The `--type-manifest` gives phc the type metadata for exhaustiveness checking and destructuring. Both are required for cross-file matching.

### Generated header format

The `.phc.h` file contains all types declared in the source — `phc_descr`, `phc_enum`, and `phc_flags` — wrapped in an include guard:

```c
#ifndef PHC_SHAPES_PHC_H
#define PHC_SHAPES_PHC_H

extern void abort(void);

typedef enum { Shape_Circle, Shape_Rectangle, Shape__COUNT = 2 } Shape_Tag;
/* ... full generated C for all types ... */

#endif /* PHC_SHAPES_PHC_H */
```

Multiple inclusion is safe. Static inline functions (constructors, accessors) are per-translation-unit — no linker conflicts.

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

## `phc_require` / `phc_check` / `phc_invariant` — Assertions

Assertions are executable hypotheses. Each one states "this must be true" and attempts to falsify it at runtime. A triggered assertion is proof of a bug — not a hint, not a warning, proof.

Three levels, distinguished by trust:

| Level | Keyword | Stripped? | Purpose |
|-------|---------|-----------|---------|
| Require | `phc_require` | Never | Untrusted boundaries — caller input, external return values, IPC results |
| Check | `phc_check` | `--strip-check` | Own code correctness — postconditions, output validation |
| Invariant | `phc_invariant` | `--strip-invariant` | Structural properties — sorted order, ref count balance, tree depth |

The distinction is trust. `phc_require` guards data you do not control — it stays in production because untrusted data can be wrong at any time. `phc_check` and `phc_invariant` guard your own code — they are falsifiers you deploy during development and withdraw when confidence is earned.

### Syntax

```
phc_require(<expr>, "<message>");
phc_check(<expr>, "<message>");
phc_invariant(<expr>, "<message>");
```

The expression is any C expression that evaluates to true (nonzero) or false (zero). The message is a string literal describing what was expected.

### Example

```c
int Cmd_SetColor(ClientData cd, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
    phc_require(objc == 3, "set_color requires exactly 2 args");
    phc_require(interp != NULL, "interpreter must not be null");

    Color c = parse_color(objv[1]);
    phc_check(c.tag != Color__COUNT, "parse_color returned invalid tag");

    apply_color(canvas, c);

    phc_invariant(googled_color_count >= 0, "color count went negative");
    return TCL_OK;
}
```

### Generated C

Each assertion becomes an `if` guard that calls `abort()` on failure. The message is preserved as a C comment:

```c
if (!(objc == 3)) abort(); /* REQUIRE: set_color requires exactly 2 args */
if (!(interp != NULL)) abort(); /* REQUIRE: interpreter must not be null */
if (!(c.tag != Color__COUNT)) abort(); /* CHECK: parse_color returned invalid tag */
if (!(color_count >= 0)) abort(); /* INVARIANT: color count went negative */
```

No `fprintf`, no `stdio.h` dependency. The message is visible in the source and in debuggers via the comment. The abort location is identified by the stack trace or core dump.

### Compile-Time Stripping

Stripping physically removes assertion code from the output. Not dead code — absent code. Zero overhead.

```bash
# Test build: all assertions active
phc < input.phc > output.c

# Debug build: invariants stripped
phc --strip-invariant < input.phc > output.c

# Release build: checks and invariants stripped, requires stay
phc --strip-check --strip-invariant < input.phc > output.c
```

Build profiles:

| Profile | `phc_require` | `phc_check` | `phc_invariant` |
|---------|--------------|-------------|-----------------|
| Test | active | active | active |
| Debug | active | active | **stripped** |
| Release | active | **stripped** | **stripped** |

`phc_require` has no strip flag. It cannot be disabled. If you do not want a require, do not write one.

### Stripped Comments

When an assertion is stripped, phc emits a comment marking the omission:

```c
/* phc_check stripped: hypothesis accepted without verification */
/* phc_invariant stripped: hypothesis accepted without verification */
```

This makes stripping visible. A reader of the generated C knows that a falsification was defined here and deliberately removed. The alternative — no trace — creates invisible assumptions.

### Naming

| What | Pattern | Example |
|------|---------|---------|
| Require | `phc_require(expr, msg)` | `phc_require(n > 0, "n must be positive")` |
| Check | `phc_check(expr, msg)` | `phc_check(result != NULL, "alloc failed")` |
| Invariant | `phc_invariant(expr, msg)` | `phc_invariant(sorted(list), "list unsorted")` |

### Epistemology

You cannot prove code correct. You can prove it wrong. Each assertion is a hypothesis test: "I claim this state is correct. Here is the test that would prove me wrong."

- `phc_require`: permanent falsifier. Active at all times because the hypothesis "this external data is valid" must be tested on every call.
- `phc_check`: deployable falsifier. Active during development to test "my code produces correct output." Withdrawn in production when confidence is earned — not because the hypothesis is proven, but because the cost of testing exceeds the expected value.
- `phc_invariant`: expensive falsifier. Active during testing to hunt for structural bugs. Withdrawn earliest because invariant checks are often O(n) or worse.

A stripped assertion is not a removed assertion. It is an accepted hypothesis — accepted without verification, at the developer's risk.

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
| `phc_enum '<T>' must have at least one value` | Empty enum declaration |
| `cannot destructure enum type '<T>'` | Destructuring attempted on a phc_enum |

---

## Design Decisions

- **Nested phc_descr:** Not supported. Define types at file scope instead — this is idiomatic C.
- **Conditional compilation:** Works in pipeline mode (`cc -E` resolves `#ifdef` before phc).
- **Field name binding:** Binding names must match field names exactly. No positional or renamed bindings.
