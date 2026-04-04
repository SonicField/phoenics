# Phoenics (phc) Architecture

## Terminal Goal

Build `phc` — a source-to-source C translator that adds first-class discriminated unions (sum types) to C11 via the `phc_descr` keyword. AI-optimised syntax. Emits standard C11 that compiles with clang and gcc.

## Pipeline Position

```
source.phc  -->  cc -E (preprocess)  -->  phc  -->  cc -c (compile)
```

phc operates AFTER the C preprocessor. It sees fully expanded source — macros resolved, includes inlined. It transforms Phoenics extensions into standard C and passes everything else through unchanged.

**Rationale:** Running after the preprocessor allows macros to generate Phoenics constructs. For example, `#define OPTION(T) phc_descr Option_##T { Some { T value; }, None {} };` expands before phc processes it. The `phc_` prefix guarantees no system header contains Phoenics keywords, so expanded headers pass through as TOK_OTHER without issue.

**Falsifier:** A source file with millions of lines of expanded headers could cause memory pressure. The island-parsing approach (TOK_OTHER passthrough) handles this efficiently — phc does not parse header content, only scans for keywords. Validated against SQLite (256K lines, 9MB) with byte-for-byte fidelity.

## Phoenics Language Extensions

### 1. `phc_descr` Declaration

**Syntax:**

```
phc_descr <TypeName> {
    <VariantName> { <field-type> <field-name>; ... },
    <VariantName> { <field-type> <field-name>; ... },
    ...
};
```

**Rules:**
- `TypeName` MUST be a valid C identifier
- EXACTLY ONE or more variants MUST be present (zero variants is an error)
- Each `VariantName` MUST be a valid C identifier, unique within the phc_descr
- Each variant CONTAINS ZERO OR MORE fields
- Fields follow C declaration syntax: type followed by name followed by semicolon
- Variants are separated by commas; the last variant MAY omit the trailing comma
- The phc_descr block ends with `};`

**Generated C11 output for `phc_descr Shape { Circle { double radius; }, Rectangle { double width; double height; } };`:**

```c
extern void abort(void);
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

typedef struct {
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
```

**Design choices:**

| Choice | Rationale |
|--------|-----------|
| `Shape_Tag` enum name | Avoids collision with the struct name. Consistent naming: `<Type>_Tag`. |
| `Shape_Circle` enum values | `<Type>_<Variant>` is unambiguous, grep-friendly, AI-readable. |
| `Shape__COUNT` sentinel | Enables runtime tag validation (`assert(v.tag < Shape__COUNT)`). Double underscore signals "not a real variant." |
| Named variant typedefs (`Shape_Circle_t`) | Enables typed accessor functions. Each variant struct gets its own typedef. |
| Anonymous union | C11 feature. Allows `v.Circle.radius` instead of `v.u.Circle.radius`. |
| `static inline` constructors | Works across translation units without linker issues. Compiler optimises away the function call. |
| `static inline` safe accessors | `Shape_as_Circle(v)` returns the variant struct by value. Calls `abort()` if tag is wrong — declared via `extern void abort(void);` to avoid double-inclusion in post-preprocessor mode. C11 portable. |
| `_v` local name | Unlikely to collide. Prefixed underscore + lowercase is reserved in file scope but legal in block scope. |
| Empty variants get `struct { char _empty; }` | C forbids zero-size structs. The `_empty` field occupies 1 byte and is never accessed. |

### 2. `phc_match` — Exhaustive Discrimination

This is the hardest problem. Two options exist.

**Option A: Enhance standard `switch`**

phc recognises `switch (expr.tag)` where the tag belongs to a known phc_descr type, parses the case labels, and errors if any variant is missing.

- **Pro:** No new syntax. Standard C switch idiom.
- **Con:** Requires phc to know the type of `expr`. This means tracking variable declarations, function parameters, struct members, and scope — essentially building a type inference engine. This is 80% of a C compiler's semantic analysis. Enormous effort for v1.
- **Falsifier:** `switch (get_shape().tag)` — phc would need to know the return type of `get_shape()`. This requires parsing function declarations, which requires parsing headers, which requires a full C front-end.

**Option B: Explicit `phc_match` keyword**

```
phc_match(<TypeName>, <expr>) {
    case <VariantName>: { ... } break;
    case <VariantName>: { ... } break;
    ...
}
```

- **Pro:** phc knows the type from the keyword syntax — no type inference needed. Trivially parseable. AI-friendly: explicit is better than implicit. Consistent with the "AI-optimised" design directive.
- **Con:** New syntax that doesn't exist in standard C. But that's the entire point of Phoenics.
- **Falsifier:** If the type name is wrong (doesn't match a known phc_descr), phc can detect and report it at transpile time.

**Recommendation: Option B.** The AI-optimised design directive says "explicit over terse." Type inference is the single largest source of complexity in C tooling. Avoiding it keeps phc implementable and maintainable.

**Generated C for `phc_match(Shape, s) { case Circle: { ... } break; case Rectangle: { ... } break; }`:**

```c
switch (s.tag) {
    case Shape_Circle: { ... } break;
    case Shape_Rectangle: { ... } break;
    default: break;
}
```

The `default: break;` is required because the generated enum includes `Shape__COUNT`. Without it, `-Wswitch` warns about the unhandled `__COUNT` value, violating invariant #2 (no warnings with `-Werror`).

**Exhaustiveness checking:** phc looks up the phc_descr definition for `Shape`, collects all variant names from the case labels, and reports an error listing the missing variants. This check happens at transpile time — it is a phc error, not a C compiler error.

**Future extension:** `phc_match` could support destructuring (binding variant fields to local variables), but this is not required for v1. The v1 form requires the programmer to access fields via `expr.VariantName.field`.

### 3. Construction Syntax

For v1, use the generated constructor functions:

```c
Shape s = Shape_mk_Circle(42.0);
```

This is explicit, unambiguous, and requires no new syntax. The AI can write it without difficulty.

### 4. Tag Access and Field Access

For v1, direct member access:

```c
s.tag           /* Shape_Tag enum value */
s.Circle.radius /* field access (only valid when s.tag == Shape_Circle) */
```

phc does NOT enforce safe access in v1 — that would require data-flow analysis (knowing which branch of a switch we're in). The generated C lets the programmer access any variant's fields regardless of tag. This matches C's existing safety model. Runtime assertions can be added by the programmer:

```c
assert(s.tag == Shape_Circle);
double r = s.Circle.radius;
```

**Future extension:** phc could generate accessor functions with built-in tag assertions, e.g., `Shape_as_Circle(s)` that asserts the tag and returns a pointer to the Circle struct.

## Internal Architecture

### Components

```
Input (.phc)
    |
    v
+--------+     +--------+     +----------+     +---------+
| Lexer  | --> | Parser | --> | Semantic | --> | Codegen |
+--------+     +--------+     +----------+     +---------+
                                                    |
                                                    v
                                              Output (.c)
```

### Lexer

Scans input text and produces a token stream.

**Token types (v1):**

| Token | Matches |
|-------|---------|
| `TOK_PHC_DESCR` | `phc_descr` keyword |
| `TOK_PHC_MATCH` | `phc_match` keyword |
| `TOK_IDENT` | C identifiers |
| `TOK_LBRACE` / `TOK_RBRACE` | `{` / `}` |
| `TOK_LPAREN` / `TOK_RPAREN` | `(` / `)` |
| `TOK_SEMICOLON` | `;` |
| `TOK_COMMA` | `,` |
| `TOK_STAR` | `*` |
| `TOK_COLON` | `:` |
| `TOK_CASE` | `case` keyword |
| `TOK_BREAK` | `break` keyword |
| `TOK_OTHER` | Passthrough text chunks |
| `TOK_EOF` | End of input |

**Dual-mode operation:**

The lexer operates in two modes:

1. **Passthrough mode** (default): Scans forward through the source, consuming text verbatim as a single `TOK_OTHER` token. Respects string literal, character literal, and comment boundaries (does not scan for keywords inside them). Stops when it encounters a Phoenics keyword (`phc_descr` or `phc_match`) at a valid position.

2. **Structured mode** (after encountering a keyword): Produces individual tokens (`TOK_IDENT`, `TOK_LBRACE`, `TOK_SEMICOLON`, etc.). Tracks brace depth. Returns to passthrough mode when the Phoenics construct ends (`};` for phc_descr, closing `}` at depth 0 for phc_match).

Structural token types (`TOK_LBRACE`, `TOK_CASE`, etc.) are ONLY produced in structured mode. In passthrough mode, braces, case labels, and all other C syntax are absorbed into `TOK_OTHER` verbatim. This guarantees passthrough fidelity: passthrough text is never tokenized and reassembled.

**Critical invariant:** The lexer MUST NOT match keywords inside string literals, character literals, or comments. The lexer must consume these correctly to avoid false matches. In passthrough mode, the lexer must also correctly skip these constructs to avoid prematurely ending a `TOK_OTHER` chunk.

**Falsifier:** Input containing `char *s = "phc_descr Shape { ... }";` — the lexer must NOT treat the string contents as a phc_descr declaration.

### Parser

Consumes the token stream and produces a Program AST.

**AST structure:**

```
Program
    |
    +-- chunks[] (interleaved, ordered)
            |
            +-- PassthroughChunk { text }
            +-- DescrDecl { name, variants[] }
            |       |
            |       +-- Variant { name, fields[] }
            |               |
            |               +-- Field { type_name, field_name }
            +-- MatchDescr { type_name, expr_text, cases[] }
                    |
                    +-- MatchCase { variant_name, body_text }
```

The Program is an ordered sequence of chunks. Each chunk is either passthrough text, a phc_descr declaration, or a match_descr construct. Ordering preserves the original source structure — this is how codegen reconstructs the output.

**Parser modes:**
1. **Scanning mode:** Consume TOK_OTHER chunks. When a TOK_PHC_DESCR or TOK_PHC_MATCH is encountered, switch to structured parsing.
2. **Descr parsing:** Parse the full `phc_descr TypeName { ... };` grammar.
3. **Match parsing:** Parse `phc_match(TypeName, expr) { case Variant: { ... } break; ... }`.

### Semantic Analyser

Operates on the parsed AST.

**Responsibilities:**
1. **Build phc_descr type table:** Map type names to their variant lists.
2. **Validate phc_descr declarations:** No duplicate variant names. At least one variant.
3. **Validate phc_match:** The type name must reference a known phc_descr. Check exhaustiveness: every variant in the phc_descr must have a corresponding case. Report missing variants by name.
4. **Duplicate case detection:** A match_descr with the same variant twice is an error.

**What it does NOT do:**
- Type checking of field types (the C compiler handles this)
- Scope analysis (phc doesn't track variable scope)
- Type inference (phc doesn't know variable types)

### Code Generator

Walks the AST and emits C11.

**For each chunk type:**
- **PassthroughChunk:** Emit text verbatim.
- **DescrDecl:** Emit the tag enum, the struct with union, and the constructor functions (as specified above).
- **MatchDescr:** Emit a standard `switch` on `expr.tag` with case labels prefixed by the type name.

## Key Invariants

These MUST hold for the system to be correct:

1. **Passthrough fidelity:** Non-Phoenics code MUST pass through byte-for-byte identical. `phc(standard_c) == standard_c`.
2. **Valid C11 output:** The output MUST compile with both `clang -std=c11 -Wall -Werror` and `gcc -std=c11 -Wall -Werror` without warnings.
3. **Exhaustiveness completeness:** If a `phc_match` is missing a variant, phc MUST report an error. If all variants are present, phc MUST NOT report an error. No false positives, no false negatives.
4. **Tag-value correspondence:** The generated enum values MUST correspond 1:1 with the variant names. The constructors MUST set the tag to the correct enum value.
5. **Name uniqueness:** Within a phc_descr, all variant names MUST be unique. Across a translation unit, all phc_descr type names MUST be unique (or phc reports a duplicate error).
6. **Keyword isolation:** `phc_descr` and `phc_match` MUST NOT be recognised inside string literals, character literals, or C comments.

## Risks

| Risk | Impact | Falsifier | Mitigation |
|------|--------|-----------|------------|
| C type syntax is complex (multi-word types, function pointers, arrays) | Parser fails on valid field declarations | `phc_descr Foo { Bar { void (*callback)(int); } };` | v1: support only simple types, pointers, and multi-word qualifiers. Document unsupported forms. |
| Nested phc_phc_descr types | Code generator produces invalid C | `phc_descr Outer { A { phc_descr Inner { X {}; } inner; } };` | v1: prohibit nested phc_descr. Declarations at file scope only. |
| Recursive phc_phc_descr types | Struct contains itself (infinite size) | `phc_descr List { Cons { int head; List tail; } };` | Must use pointer: `List *tail`. phc does not check this — the C compiler will error on infinite-size struct. Document the pattern. |
| `phc_descr` used as an identifier in existing code | phc incorrectly treats it as a keyword | `int phc_descr = 42;` | Unlikely collision due to `phc_` prefix. Phoenics reserves `phc_descr` and `phc_match`. |
| Large passthrough chunks exceed buffer | Memory exhaustion | 100MB C file with one phc_descr at the end | Use streaming/chunked passthrough emission. Don't buffer the entire output. |
| `#line` directives absent | Compiler errors reference wrong line numbers | Any non-trivial input | v1: no `#line` support (accepted limitation). v2: emit `#line` at each chunk boundary. |
| Multi-file phc_descr usage | `phc_match` fails with "unknown phc_descr type" when the phc_descr is defined in another file | File B has `#include "shapes.h"` and `phc_match(Shape, s)` where Shape is defined in shapes.phc | v1: single-file constraint — `phc_match` only works when the phc_descr declaration is in the same file. See "Known Limitations" section below. |
| Position-based codegen limits extensibility | Adding phc_match features that transform body content (destructuring, field binding) requires abandoning source-offset splicing for AST-based emission | Any v2 feature that rewrites case body text | Accepted structural cost. v2 codegen rewrite scoped to phc_match emission only — phc_descr and passthrough codegen are unaffected. |

## Resolved Design Decisions

1. **Variant separator:** Comma-separated. Variants are alternatives, not declarations. *(Decided by supervisor)*
2. **`phc_match` vs standard switch:** Explicit `phc_match` keyword. Avoids type inference complexity. AI-optimised: explicit over terse. *(Decided by supervisor)*
3. **Header generation:** Implicit in transform pipeline. `.phc` → `.h` works but cross-file `phc_match` is a known limitation (see above). *(Decided by supervisor)*
4. **Default in match_descr:** No default in v1. Exhaustive matching is the purpose — use raw `switch` for catch-all patterns. *(Decided by supervisor)*

## Known Limitations (v1)

### Single-File Constraint

phc processes one file at a time without expanding `#include` directives. The semantic analyser resolves phc_descr types only within the current translation unit's source. This means:

- `phc_descr` declarations in file A can generate `.h` output that other files include
- Other files can use the generated types (structs, enums, constructors) normally — these are standard C
- **`phc_match` in file B CANNOT reference a phc_descr defined in file A** — phc has no visibility into included headers

**Consequence:** Exhaustive matching — the core safety feature — is only available when the phc_descr and the phc_match are in the same `.phc` file. Cross-file usage falls back to raw `switch` without exhaustiveness checking.

**v2 remediation options (ranked by invasiveness):**

1. **Type manifest files** — phc emits a `.phc-types` manifest alongside each `.h` output, listing phc_descr names and their variants. When processing a file, phc reads manifests for any `#include`d `.h` files that have a corresponding `.phc-types`. Least invasive: no new syntax, no preprocessor interaction.

2. **Explicit import directive** — `phc_import "shapes.phc"` tells phc to parse another `.phc` file for type definitions before processing the current file. Simple to implement but introduces a new directive that duplicates `#include`.

3. **Post-preprocessor position** — Run phc after `cpp` so `#include` is expanded. Gains full type visibility but requires parsing expanded system headers (enormous complexity increase).

4. **Two-pass build** — First pass extracts phc_descr declarations from all `.phc` files into a shared type database. Second pass validates `phc_match` against the database. Requires build system integration.

## v1 Scope Boundary

**In scope:**
- `phc_descr` declarations with simple field types (identifiers, `*` pointers, multi-word qualifiers)
- Generated tag enum, struct with anonymous union, constructor functions
- `phc_match` with exhaustiveness checking
- Passthrough of all non-Phoenics code
- Error reporting with line numbers

**Out of scope for v1:**
- Cross-file `phc_match` (see Known Limitations above)
- `#line` directive emission
- Nested phc_descr
- Pattern destructuring in phc_match
- ~~Safe accessor functions with tag assertions~~ *(implemented in v1.5)*
- Conditional compilation interaction (`#ifdef` around phc_descr)
- Function pointer or array field types
- Forward declarations of phc_descr types
