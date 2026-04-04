# PHC (Phoenics) — Design Notes

## What is Phoenics?

Phoenics is a C11 language extension designed for AI readability and writability.
PHC is the source-to-source preprocessor that transforms Phoenics (C + extensions)
into standard C11.

## Design Principles

1. **AI-optimized syntax** — unambiguous, explicit, consistent structure
2. **C11 compliant output** — PHC emits standard C11 that compiles with clang/gcc
3. **Sits between preprocessor and compiler** — operates on preprocessed C
4. **TDD development** — tests written before implementation

## Feature: Discriminated Unions (`descr`)

### Syntax (current proposal)

```
descr TypeName {
    VariantA { type1 field1; type2 field2; },
    VariantB { type3 field3; },
    VariantC {}
};
```

### Output

Each `descr` declaration expands to three C11 constructs:

**1. Tag enum:**
```c
typedef enum {
    TypeName_VariantA,
    TypeName_VariantB,
    TypeName_VariantC
} TypeName_Tag;
```

**2. Tagged union struct:**
```c
typedef struct {
    TypeName_Tag tag;
    union {
        struct { type1 field1; type2 field2; } VariantA;
        struct { type3 field3; } VariantB;
        struct { char _empty; } VariantC;  // empty variants get placeholder
    };
} TypeName;
```

**3. Constructor functions:**
```c
static inline TypeName TypeName_mk_VariantA(type1 field1, type2 field2) {
    TypeName _v;
    _v.tag = TypeName_VariantA;
    _v.VariantA.field1 = field1;
    _v.VariantA.field2 = field2;
    return _v;
}

static inline TypeName TypeName_mk_VariantC(void) {
    TypeName _v;
    _v.tag = TypeName_VariantC;
    return _v;
}
```

### Exhaustive switch enforcement

**Open question for @theologian:** How to enforce exhaustive matching at
transpile time. Options considered:

- A. PHC analyzes `switch(x.tag)` statements and errors if cases are missing
- B. A new `match` keyword that guarantees exhaustiveness
- C. Generate a macro that wraps switch and uses compiler warnings

## Architecture

```
Source (.phc) → Lexer → Parser → AST → Codegen → C11 (.c)
```

### Lexer
- Tokenizes into: TOK_DESCR, TOK_IDENT, TOK_LBRACE, TOK_RBRACE,
  TOK_SEMICOLON, TOK_COMMA, TOK_STAR, TOK_OTHER, TOK_EOF
- TOK_OTHER captures all non-descr C code as passthrough text
- Tracks line/col for error reporting

### Parser
- Produces a `Program` containing interleaved:
  - Passthrough chunks (verbatim C code)
  - `DescrDecl` nodes (parsed descr declarations)
- Each `DescrDecl` has a name and list of `Variant`s
- Each `Variant` has a name and list of typed `Field`s

### Codegen
- Walks the Program, emitting passthrough chunks verbatim
- Replaces each DescrDecl with the three C11 constructs above

## Decisions Log

| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-04-04 | Language extension named "Phoenics" | Per alexie |
| 2026-04-04 | AI-optimized syntax over human ergonomics | Per alexie — PHC is for AI-assisted C development |
| 2026-04-04 | Pure TDD approach | Per alexie — tests before implementation |
