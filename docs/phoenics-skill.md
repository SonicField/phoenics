---
description: "Phoenics: Discriminated unions for C via phc"
allowed-tools: Bash, Read, Write, Edit, Glob, Grep
---

# Phoenics Skill

Discriminated unions in C. Use `phc_descr` when a value can be one of several distinct types — tagged unions without the boilerplate.

## When to use

- **Option types.** Present or absent, with different data.
- **Result types.** Success or failure, each carrying different payloads.
- **AST nodes.** Expression trees, command variants, message kinds.
- **State machines.** States with different associated data.
- **Protocol messages.** Different shapes, one type.

If you are writing a `struct` with an `enum` tag and a `union`, use `phc_descr` instead. Same generated code, but with exhaustiveness checking.

## Quick reference

### Declare

```c
phc_descr TypeName {
    Variant1 { type1 field1; type2 field2; },
    Variant2 { type3 field3; },
    Variant3 {}
};
```

### Construct

```c
TypeName v = TypeName_mk_Variant1(value1, value2);
TypeName w = TypeName_mk_Variant3();
```

### Access safely

```c
field_type x = TypeName_as_Variant1(v).field1;  // asserts tag at runtime
```

### Match exhaustively

```c
phc_match(TypeName, v) {
    case Variant1: {
        // use TypeName_as_Variant1(v).field1
    } break;
    case Variant2: {
        // use TypeName_as_Variant2(v).field3
    } break;
    case Variant3: {
        // no fields
    } break;
}
```

Every variant must be covered. Missing one is a compile-time error.

### Raw tag access

```c
if (v.tag == TypeName_Variant1) { ... }
assert(v.tag < TypeName__COUNT);
```

## Build

```bash
phc < input.phc > output.c        # transpile
make                                # build phc itself
make test                           # 98 checks
```

## Rules

1. `phc_descr` and `phc_match` must be in the same file (v1).
2. Accessor macros evaluate their argument twice. Simple lvalues only.
3. Recursive types use pointers: `Expr *left`, not `Expr left`.
4. Every `phc_match` covers all variants. No `default`.
5. Field types: identifiers, qualifiers, pointers. No function pointers or arrays.

## Reference

Full spec: `docs/phoenics-reference.md`.
