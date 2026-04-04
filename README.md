# Phoenics (phc)

A C-to-C source preprocessor that adds first-class discriminated unions (sum types) to C11.

Phoenics sits between your source files and the C compiler, transforming `phc_descr` declarations and `phc_match` exhaustive matching into standard C11 that compiles with clang and gcc.

Designed for AI-assisted C development: unambiguous syntax, explicit semantics, zero collision with existing C code.

## Features

- **`phc_descr`** — Discriminated union declarations with typed variants
- **`phc_match`** — Exhaustive pattern matching, enforced at transpile time
- **Safe accessors** — `Type_as_Variant(v)` macros with runtime tag assertions
- **Passthrough fidelity** — Standard C passes through byte-for-byte unchanged
- **Validated at scale** — SQLite amalgamation (256K lines) passes through unmodified

## Quick Example

```c
#include <stdio.h>
#include <assert.h>

phc_descr Result {
    Ok { int value; },
    Err { const char *message; }
};

int main(void) {
    Result r = Result_mk_Ok(42);

    phc_match(Result, r) {
        case Ok: {
            printf("success: %d\n", Result_as_Ok(r).value);
        } break;
        case Err: {
            printf("error: %s\n", Result_as_Err(r).message);
        } break;
    }
    return 0;
}
```

## Build

Requirements: C11 compiler (clang or gcc), make, POSIX environment.

```bash
make          # builds build/phc
make test     # runs unit, integration, and self-hosting tests
make clean    # removes build artifacts
```

## Usage

```bash
# Transpile a .phc file to standard C
build/phc < input.phc > output.c

# Full pipeline
build/phc < myfile.phc | clang -std=c11 -x c - -o myfile
```

## Generated C

A `phc_descr` declaration:

```c
phc_descr Shape {
    Circle { double radius; },
    Rectangle { double width; double height; }
};
```

Generates:

1. **Tag enum** — `Shape_Tag` with `Shape_Circle`, `Shape_Rectangle`, `Shape__COUNT`
2. **Tagged union struct** — `Shape` with `Shape_Tag tag` and anonymous union
3. **Constructor functions** — `Shape_mk_Circle(double radius)`, etc.
4. **Safe accessor macros** — `Shape_as_Circle(v)` with runtime tag assertion

## Enforcement

**Compile-time (by phc):**
- Missing variant in `phc_match` → transpile error
- Duplicate case → error
- Unknown type → error

**Runtime (by generated C):**
- Safe accessors assert correct tag before field access
- `__COUNT` sentinel enables tag range validation

## Documentation

- [Phoenics Language Reference](docs/phoenics-reference.md) — Full specification
- [Architecture](docs/architecture.md) — Design decisions and internals
- [Design Notes](docs/design-notes.md) — Development history

## Known Limitations (v1)

- Single-file only: `phc_match` can only reference `phc_descr` types declared in the same file
- No pattern destructuring in `phc_match` (use `Type_as_Variant(v).field`)
- No `#line` directive emission (compiler errors reference generated output, not source)
- Field types limited to simple types, pointers, and multi-word qualifiers (no function pointers or arrays)

See [Architecture: Known Limitations](docs/architecture.md) for details and v2 plans.

## License

See [LICENSE](LICENSE).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## Code of Conduct

See [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).
