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

Supports all C field types: pointers, function pointers (`void (*cb)(int)`), arrays (`int data[4]`), multi-word types (`unsigned long long`).

### Forward declare (for recursive types)

```c
phc_descr List;

phc_descr List {
    Cons { int head; List *tail; },
    Nil {}
};
```

### Construct

```c
TypeName v = TypeName_mk_Variant1(value1, value2);
TypeName w = TypeName_mk_Variant3();
```

### Access safely

```c
TypeName_Variant1_t data = TypeName_as_Variant1(v);  // aborts if wrong tag
field_type x = data.field1;
```

### Match with destructuring

```c
phc_match(TypeName, v) {
    case Variant1(field1, field2): {
        // field1 and field2 are local variables
    } break;
    case Variant2(field3): {
        use(field3);
    } break;
    case Variant3: {
        // no fields to bind
    } break;
}
```

Every variant must be covered. Missing one is a compile-time error. Binding names must match field names.

### Raw tag access

```c
if (v.tag == TypeName_Variant1) { ... }
```

## Build

```bash
phc < input.phc > output.c                         # direct mode
cc -E input.phc | phc | cc -x c -                   # pipeline mode
phc --emit-types=types.phc-types < lib.phc > lib.c   # emit manifest
phc --type-manifest=types.phc-types < main.phc       # use manifest
make                                                  # build phc itself
make test                                              # 182 checks, 7 suites
make test-asan                                         # AddressSanitizer build
```

## Rules

1. `phc_match` requires the type to be declared in the same file or loaded via `--type-manifest`.
2. Accessor functions call `abort()` on tag mismatch.
3. Recursive types need a forward declaration and pointer fields.
4. Every `phc_match` covers all variants. No `default`.
5. Binding names must match field names exactly. Partial binding is allowed.
6. Array fields bind as pointers in destructuring.
7. Nested `phc_descr` is not supported — define types at file scope.

## Reference

Full spec: `docs/phoenics-reference.md`.
