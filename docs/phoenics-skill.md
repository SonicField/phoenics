---
description: "Phoenics: Discriminated unions for C via phc"
allowed-tools: Bash, Read, Write, Edit, Glob, Grep
---

# Phoenics Skill

Use discriminated unions (sum types) in C via the Phoenics language extension.

## When to Use

Use `phc_descr` when you need a value that can be one of several distinct types — the "tagged union" or "sum type" pattern. Common cases:

- **Option types:** A value that may or may not be present (`Some`/`None`)
- **Result types:** Success or failure with different data (`Ok`/`Err`)
- **AST nodes:** Expression trees, command types, message kinds
- **State machines:** States with different associated data
- **Protocol messages:** Different message types with different payloads

If you find yourself writing a `struct` with an `enum` tag and a `union`, use `phc_descr` instead — it generates the same code but with compile-time exhaustiveness checking.

## Quick Reference

### Declaration

```c
phc_descr TypeName {
    Variant1 { type1 field1; type2 field2; },
    Variant2 { type3 field3; },
    Variant3 {}
};
```

### Construction

```c
TypeName v = TypeName_mk_Variant1(value1, value2);
TypeName w = TypeName_mk_Variant3();  // empty variant
```

### Safe Access

```c
// Asserts tag at runtime, then returns the variant struct
field_type x = TypeName_as_Variant1(v).field1;
```

### Exhaustive Match

```c
phc_match(TypeName, v) {
    case Variant1: {
        // use TypeName_as_Variant1(v).field1
    } break;
    case Variant2: {
        // use TypeName_as_Variant2(v).field3
    } break;
    case Variant3: {
        // no fields to access
    } break;
}
```

Every variant MUST be covered. Missing a variant is a phc error.

### Raw Tag Access (when needed)

```c
if (v.tag == TypeName_Variant1) { ... }
assert(v.tag < TypeName__COUNT);  // runtime tag validation
```

## Build

```bash
# Transpile a single file
phc < input.phc > output.c

# In a Makefile
%.c: %.phc
	phc < $< > $@
```

## Rules

1. `phc_descr` and `phc_match` MUST be in the same file (v1 limitation)
2. Accessor macros evaluate their argument twice — use simple lvalues only
3. Recursive types MUST use pointers: `Expr *left`, not `Expr left`
4. Every `phc_match` MUST cover all variants — no `default` allowed
5. Field types: identifiers, multi-word qualifiers, pointers. No function pointers or arrays.

## Full Reference

See `docs/phoenics-reference.md` for complete syntax, generated C mapping, error messages, and known limitations.
