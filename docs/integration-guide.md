# phc Integration Guide

How to add phc to your C project's build system and use Phoenics constructs in production code.

## Build System Integration

### Direct Mode (Simple Projects)

For projects without `#include` or preprocessor dependencies in `.phc` files:

```makefile
PHC ?= path/to/phc
CC ?= clang
CFLAGS = -std=c11 -Wall -Wextra -Werror -Wno-unused-function

# phc sources → generated C
build/%.c: src/%.phc | build
	$(PHC) < $< > $@ || (rm -f $@; exit 1)

# Compile generated C normally
build/%.o: build/%.c
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build
```

Note: `-Wno-unused-function` suppresses warnings for generated `static inline` constructors/accessors that may not all be called in every file.

### Pipeline Mode (Recommended)

Pipeline mode runs phc after the C preprocessor. This is recommended for any project using `#include`, `#ifdef`, or macros:

```makefile
build/%.o: src/%.phc | build
	$(CC) $(CFLAGS) -E $< | $(PHC) | $(CC) $(CFLAGS) -x c - -c -o $@
```

phc emits `#line N "filename"` directives so compiler errors reference the original `.phc` source file and line number, not the preprocessed/transpiled output.

**Important:** In pipeline mode, `#include <stdio.h>` and other headers are expanded before phc sees the source. phc passes expanded headers through unchanged.

### Multi-File Projects with Type Manifests

When `phc_descr` types are shared across translation units, use type manifests:

```makefile
# Step 1: Transpile type definitions and emit manifest
build/types.c build/types.phc-types: src/types.phc | build
	$(PHC) --emit-types=build/types.phc-types < $< > build/types.c

# Step 2: Transpile consumers with manifest
build/parser.c: src/parser.phc build/types.phc-types
	$(PHC) --type-manifest=build/types.phc-types < $< > $@

build/renderer.c: src/renderer.phc build/types.phc-types
	$(PHC) --type-manifest=build/types.phc-types < $< > $@
```

Multiple manifests can be loaded:
```makefile
build/main.c: src/main.phc build/types.phc-types build/events.phc-types
	$(PHC) --type-manifest=build/types.phc-types \
	       --type-manifest=build/events.phc-types < $< > $@
```

### Sharing Types via Generated Headers

phc generates `static inline` constructors and accessors, so the generated output can be used as a header (multiple inclusion is safe for `static inline` functions):

```makefile
# Generate a header from type definitions
build/types.h: src/types.phc | build
	$(PHC) < $< > $@
```

```c
// src/parser.phc
#include "types.h"   // generated phc_descr types

int parse(Token t) {
    phc_match(Token, t) {
        case Ident(name): { /* ... */ } break;
        case Number(value): { /* ... */ } break;
    }
    return 0;
}
```

**Caveat:** The generated header includes `extern void abort(void);`. If included in multiple files alongside `<stdlib.h>`, the declarations are compatible (both declare `abort`). No link-time conflicts.

### Recommended File Organization

```
project/
├── src/
│   ├── types.phc          # shared phc_descr declarations
│   ├── parser.phc          # uses phc_match + phc_defer
│   ├── renderer.phc        # uses phc_match
│   └── main.phc
├── build/
│   ├── types.h             # generated header
│   ├── types.phc-types     # manifest for cross-file matching
│   ├── parser.c            # generated
│   ├── renderer.c          # generated
│   └── main.c              # generated
├── tests/
│   ├── test_parser.phc     # tests using phc constructs
│   └── run_tests.sh
└── Makefile
```

## Usage Patterns for Terminal Emulators

phc is particularly well-suited for terminal emulator internals. Here are concrete patterns for a VT parser / terminal emulator project.

### VT Parser State Machine

The VT parser processes bytes through states. Each state carries different context:

```c
#include <stdint.h>

phc_descr VtState;  /* forward declaration for self-referencing */

phc_descr VtState {
    Ground {},
    Escape {},
    CsiEntry {},
    CsiParam { int params[16]; int param_count; },
    CsiIntermediate { int params[16]; int param_count;
                      char intermediates[4]; int inter_count; },
    OscString { char buf[256]; int len; },
    DcsEntry {},
    DcsPassthrough { char buf[1024]; int len; }
};
```

State transitions with exhaustive matching:

```c
VtState vt_feed(VtState state, uint8_t byte, Terminal *term) {
    phc_match(VtState, state) {
        case Ground: {
            if (byte == 0x1B) return VtState_mk_Escape();
            if (byte >= 0x20 && byte < 0x7F) {
                terminal_emit_char(term, byte);
            }
            return state;
        } break;
        case Escape: {
            if (byte == '[') return VtState_mk_CsiEntry();
            if (byte == ']') return VtState_mk_OscString(0);
            /* Unrecognized escape — return to ground */
            return VtState_mk_Ground();
        } break;
        case CsiEntry: {
            if (byte >= '0' && byte <= '9') {
                VtState s = VtState_mk_CsiParam(0);
                s.CsiParam.params[0] = byte - '0';
                s.CsiParam.param_count = 1;
                return s;
            }
            /* CSI dispatch for parameterless sequences */
            csi_dispatch(term, NULL, 0, (char)byte);
            return VtState_mk_Ground();
        } break;
        case CsiParam(params, param_count): {
            /* params is int* (array binds as pointer) */
            if (byte >= '0' && byte <= '9') {
                /* Accumulate digit into current parameter */
                return state;
            }
            if (byte == ';') {
                /* Next parameter */
                return state;
            }
            /* Final byte — dispatch */
            csi_dispatch(term, params, param_count, (char)byte);
            return VtState_mk_Ground();
        } break;
        case CsiIntermediate: {
            return VtState_mk_Ground();
        } break;
        case OscString: {
            /* Accumulate until ST (String Terminator) */
            return state;
        } break;
        case DcsEntry: {
            return VtState_mk_Ground();
        } break;
        case DcsPassthrough: {
            return VtState_mk_Ground();
        } break;
    }
    return state;
}
```

**Why phc helps:** Adding a new parser state (e.g., SOS for Start of String) forces handling it in every `phc_match`. The compiler catches any switch that doesn't cover the new state — impossible to forget.

### Screen Buffer Cells

```c
phc_descr Cell {
    Char { uint32_t codepoint; uint16_t attrs; uint8_t fg; uint8_t bg; },
    WideCont {},
    Empty { uint16_t attrs; uint8_t fg; uint8_t bg; }
};
```

Rendering with exhaustive match:

```c
void render_cell(Cell cell, int row, int col) {
    phc_match(Cell, cell) {
        case Char(codepoint, attrs, fg, bg): {
            set_color(fg, bg);
            set_attrs(attrs);
            draw_glyph(row, col, codepoint);
        } break;
        case WideCont: {
            /* Skip — handled by the preceding wide char */
        } break;
        case Empty(attrs, fg, bg): {
            set_color(fg, bg);
            set_attrs(attrs);
            draw_space(row, col);
        } break;
    }
}
```

### SGR (Select Graphic Rendition) Parameters

```c
phc_descr SgrParam {
    Reset {},
    Bold {},
    Dim {},
    Italic {},
    Underline {},
    Blink {},
    Inverse {},
    Hidden {},
    Strikethrough {},
    FgColor { uint8_t color; },
    BgColor { uint8_t color; },
    FgRgb { uint8_t r; uint8_t g; uint8_t b; },
    BgRgb { uint8_t r; uint8_t g; uint8_t b; },
    FgDefault {},
    BgDefault {}
};
```

### Input Events

```c
phc_descr KeyEvent {
    Char { uint32_t codepoint; },
    Function { int fkey; },
    Arrow { int direction; },
    Special { int key; },
    Mouse { int button; int col; int row; }
};

/* Encode a key event to the byte sequence expected by the remote terminal */
int encode_key(KeyEvent ev, char *buf, int bufsize) {
    phc_match(KeyEvent, ev) {
        case Char(codepoint): {
            /* UTF-8 encode */
            return utf8_encode(codepoint, buf, bufsize);
        } break;
        case Function(fkey): {
            return snprintf(buf, bufsize, "\x1b[%d~", fkey + 10);
        } break;
        case Arrow(direction): {
            const char codes[] = "ABCD";
            return snprintf(buf, bufsize, "\x1b[%c", codes[direction]);
        } break;
        case Special(key): {
            return encode_special_key(key, buf, bufsize);
        } break;
        case Mouse(button, col, row): {
            return snprintf(buf, bufsize, "\x1b[M%c%c%c",
                           32 + button, 33 + col, 33 + row);
        } break;
    }
    return 0;
}
```

### Resource Cleanup with phc_defer

Terminal emulators allocate buffers, file descriptors, and rendering contexts. `phc_defer` ensures cleanup on all error paths:

```c
int terminal_init(Terminal *term, int rows, int cols) {
    term->cells = calloc((size_t)(rows * cols), sizeof(Cell));
    if (!term->cells) return -1;

    phc_defer { free(term->cells); }

    term->scrollback = scrollback_alloc(1000);
    if (!term->scrollback) return -1;

    phc_defer { scrollback_free(term->scrollback); }

    term->alt_screen = calloc((size_t)(rows * cols), sizeof(Cell));
    if (!term->alt_screen) return -1;

    /* Success — we don't want cleanup to fire.
     * Clear the pointers so the defers free(NULL) harmlessly. */
    Cell *cells = term->cells;
    term->cells = NULL;
    term->scrollback = NULL;
    /* Actually, a simpler pattern: */
    return 0;
    /* On error paths (the early returns above), defers fire and clean up.
     * On the success path (return 0), defers also fire — but at this point
     * we've already returned 0, so the cleanup code runs but the function
     * has already returned successfully. The generated code wraps each
     * return with cleanup inline, so this works correctly. */
}
```

**Simpler pattern** — use a success flag:

```c
int terminal_init(Terminal *term, int rows, int cols) {
    int ok = 0;

    term->cells = calloc((size_t)(rows * cols), sizeof(Cell));
    if (!term->cells) return -1;
    phc_defer { if (!ok) free(term->cells); }

    term->scrollback = scrollback_alloc(1000);
    if (!term->scrollback) return -1;
    phc_defer { if (!ok) scrollback_free(term->scrollback); }

    term->alt_screen = calloc((size_t)(rows * cols), sizeof(Cell));
    if (!term->alt_screen) return -1;

    ok = 1;  /* All allocations succeeded — skip cleanup */
    return 0;
}
```

### Python C Extension Pattern

For Python C extensions (like nbs-term), combine phc_defer with Python reference counting:

```c
static PyObject *terminal_feed(TerminalObject *self, PyObject *args) {
    const char *data;
    Py_ssize_t len;
    if (!PyArg_ParseTuple(args, "s#", &data, &len))
        return NULL;

    /* No phc_defer needed here — no resources to clean up.
     * But for functions that create temporary Python objects: */
    PyObject *result = PyList_New(0);
    if (!result) return NULL;
    phc_defer { Py_XDECREF(result); }

    for (Py_ssize_t i = 0; i < len; i++) {
        int err = feed_byte(self->term, (uint8_t)data[i]);
        if (err) return NULL;  /* defer fires, decrefs result */
    }

    /* Success — transfer ownership to caller */
    PyObject *ret = result;
    result = NULL;  /* prevent defer from decrefing */
    return ret;
}
```

## Testing Strategy

### Unit Testing phc-generated Code

Test that phc output compiles, links, and produces correct runtime behavior:

```makefile
# Compile and run a test
test: build/test_parser
	./build/test_parser

# Build test from .phc source
build/test_parser: build/test_parser.c build/types.h
	$(CC) $(CFLAGS) -o $@ $<

build/test_parser.c: tests/test_parser.phc
	$(PHC) < $< > $@
```

### AddressSanitizer Gate

Catch memory errors in generated code:

```makefile
test-asan:
	$(MAKE) test CC=clang \
		CFLAGS="-std=c11 -Wall -Wextra -Werror -Wno-unused-function \
		        -fsanitize=address -fno-omit-frame-pointer"
```

### Integration Testing with Recorded Sessions

For terminal emulators, replay recorded byte streams and verify output:

```c
void test_vt_clear_screen(void) {
    Terminal *term = terminal_new(24, 80);
    /* Feed: clear screen sequence */
    terminal_feed_str(term, "\x1b[2J");
    /* Verify all cells are empty */
    for (int r = 0; r < 24; r++)
        for (int c = 0; c < 80; c++)
            assert(term->grid[r * 80 + c].tag == Cell_Empty);
    terminal_free(term);
}
```

### Property-Based Testing

Verify invariants across random inputs:

```bash
# Random valid C passes through phc unchanged
for i in $(seq 1000); do
    random_c_file > /tmp/test.phc
    diff <(cat /tmp/test.phc) <(phc < /tmp/test.phc) || echo "FAIL: fidelity"
done
```

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `unknown phc_descr type` | Type defined in another file | Use `--type-manifest` |
| `cannot destructure external type` | v1 manifest (no field types) | Regenerate manifest with current phc |
| `non-exhaustive phc_match` | Missing variant in switch | Add the missing case |
| Compiler error in generated code | phc bug or unsupported C pattern | File an issue with the `.phc` input |
| `phc_defer must be at function scope` | Defer inside loop or if block | Move defer to function scope |

## Known Limitations

- **Nested `phc_match`** inside case bodies: case body text is opaque. Nested matching requires defining the inner match in a separate function.
- **`phc_defer` + `longjmp`:** Defer cleanup is not invoked on `longjmp`. Same limitation as Go's `defer`. Document for users.
- **`phc_defer` scope:** Function scope only. Cannot defer inside loops or blocks. Use a separate function if per-iteration cleanup is needed.
- **Nested `phc_descr`:** Not supported inside variant bodies. Define types at file scope.
- **Cross-file destructuring of complex types:** Function pointer fields in manifests require v2 format (current phc generates v2 by default).
