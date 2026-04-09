# Phoenics (phc)

A C-to-C source preprocessor that adds discriminated unions and automatic cleanup to C11.

Phoenics sits between your source files and the C compiler, transforming `phc_descr`, `phc_match`, and `phc_defer` constructs into standard C11 that compiles with clang and gcc.

Designed for AI-assisted C development: unambiguous syntax, explicit semantics, zero collision with existing C code.

## Features

- **`phc_descr`** — Discriminated union declarations with typed variants
- **`phc_match`** — Exhaustive pattern matching with field destructuring
- **`phc_defer`** — Automatic cleanup blocks (Go/Zig-style defer)
- **Forward declarations** — Recursive types via `phc_descr Name;`
- **Multi-file support** — Type manifests for cross-file matching and destructuring
- **Pipeline mode** — Works post-preprocessor (`cc -E | phc | cc`) with correct `#line` directives
- **Complex field types** — Function pointers, arrays, multi-word qualifiers
- **Safe accessors** — `Type_as_Variant(v)` inline functions with `abort()` on tag mismatch
- **Passthrough fidelity** — Standard C passes through byte-for-byte unchanged

## Quick Example

```c
#include <stdio.h>
#include <stdlib.h>

phc_descr Result {
    Ok { int value; },
    Err { const char *message; }
};

int process(Result r) {
    char *log = malloc(100);
    phc_defer { free(log); }

    phc_match(Result, r) {
        case Ok(value): {
            printf("success: %d\n", value);
            return value;       // defer cleanup fires here
        } break;
        case Err(message): {
            printf("error: %s\n", message);
            return -1;          // defer cleanup fires here too
        } break;
    }
    return 0;                   // and here
}

int main(void) {
    Result r = Result_mk_Ok(42);
    printf("ret=%d\n", process(r));
    return 0;
}
```

## Build

Requirements: C11 compiler (clang or gcc), make, POSIX environment.

```bash
make              # builds build/phc
make test         # runs all 7 test suites (~199 checks)
make test-asan    # AddressSanitizer build + tests
make clean        # removes build artifacts
```

## Usage

```bash
# Direct mode
build/phc < input.phc > output.c

# Pipeline mode (post-preprocessor)
cc -E input.phc | build/phc | cc -x c - -o output

# Multi-file: emit type manifest
build/phc --emit-types=types.phc-types < lib.phc > lib.c

# Multi-file: use type manifest
build/phc --type-manifest=types.phc-types < main.phc > main.c
```

## Language Overview

### `phc_descr` — Discriminated Unions

```c
phc_descr Shape {
    Circle { double radius; },
    Rectangle { double width; double height; }
};
```

Generates: tag enum, named variant typedefs, tagged union struct, constructor functions, safe accessor functions.

Forward declarations enable recursive types:
```c
phc_descr List;
phc_descr List { Cons { int head; List *tail; }, Nil {} };
```

### `phc_match` — Exhaustive Pattern Matching

```c
phc_match(Shape, s) {
    case Circle(radius): {
        printf("r=%g\n", radius);    // field bound to local variable
    } break;
    case Rectangle(width, height): {
        printf("%gx%g\n", width, height);
    } break;
}
```

Every variant must be covered. Missing variants, duplicates, and unknown variants are compile-time errors. Binding names must match field names from the `phc_descr` declaration.

### `phc_defer` — Automatic Cleanup

```c
FILE *f = fopen(path, "r");
phc_defer { fclose(f); }

char *buf = malloc(4096);
phc_defer { free(buf); }

// Cleanup fires in reverse order at every return point
```

Multiple defers stack in LIFO order. Works with `phc_match` — cleanup fires on return inside case bodies.

## Enforcement

**Compile-time (by phc):**
- Missing variant in `phc_match` → error
- Duplicate case → error
- Unknown type or variant → error
- Unknown field in destructuring → error
- C keyword as field name → error
- `phc_defer` inside loops/blocks → error (function scope only)

**Runtime (by generated C):**
- Safe accessors call `abort()` on tag mismatch

## Test Suite

7 test suites, ~199 checks:
- Unit tests (lexer, parser, semantic, codegen)
- Integration tests (47 feature + adversarial cases)
- Passthrough fidelity tests
- Self-hosting round-trip tests
- Post-preprocessor pipeline tests
- Multi-file manifest tests
- `#line` directive tests (direct + pipeline mode)
- AddressSanitizer gate

## Documentation

- [Phoenics Language Reference](docs/phoenics-reference.md) — Full specification
- [Architecture](docs/architecture.md) — Design decisions and internals
- [Design Notes](docs/design-notes.md) — Development history
- [Known Gotchas](docs/known-gotchas.md) — Platform-specific issues and integration pitfalls

## Known Limitations

- Nested `phc_descr` inside variant bodies — use file-scope definitions instead
- Nested `phc_match` inside case bodies — case body text is opaque
- `phc_defer` does not protect against `longjmp` (documented, same as Go's defer)
- Cross-file destructuring requires v2 manifests (v1 manifests lack field types)

## License

See [LICENSE](LICENSE).
