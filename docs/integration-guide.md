# phc Integration Guide

How to add phc to your C project's build system and use Phoenics constructs in production code.

## Build System Integration

### Makefile Setup

Add phc as a build step: `.phc` sources are transpiled to `.c` before compilation.

```makefile
PHC ?= path/to/phc
CC ?= clang
CFLAGS = -std=c11 -Wall -Wextra -Werror

# phc sources → generated C
%.c: %.phc
	$(PHC) < $< > $@

# Compile generated C normally
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
```

### Pipeline Mode (Recommended for projects with `#include`)

Pipeline mode runs phc after the preprocessor, so `#include`, `#ifdef`, and macros work naturally:

```makefile
%.o: %.phc
	$(CC) -E $< | $(PHC) | $(CC) $(CFLAGS) -x c - -c -o $@
```

phc emits `#line` directives so compiler errors reference the original `.phc` source file and line.

### Multi-File Projects with Type Manifests

When `phc_descr` types are shared across files, use type manifests:

```makefile
# Step 1: Transpile library and emit manifest
lib.c lib.phc-types: lib.phc
	$(PHC) --emit-types=lib.phc-types < $< > lib.c

# Step 2: Transpile consumer with manifest
main.c: main.phc lib.phc-types
	$(PHC) --type-manifest=lib.phc-types < $< > $@
```

Multiple manifests can be loaded:
```makefile
main.c: main.phc types_a.phc-types types_b.phc-types
	$(PHC) --type-manifest=types_a.phc-types --type-manifest=types_b.phc-types < $< > $@
```

### File Organization

```
project/
├── src/
│   ├── types.phc          # shared phc_descr declarations
│   ├── parser.phc          # uses phc_match + phc_defer
│   ├── renderer.phc        # uses phc_match
│   └── main.phc
├── build/
│   ├── types.c             # generated
│   ├── types.phc-types     # manifest
│   ├── parser.c            # generated
│   └── ...
└── Makefile
```

### Header Files

phc_descr generates typedefs, constructors, and accessors. For multi-file projects, put `phc_descr` in a `.phc` file, transpile it, and include the generated `.c` as a header (or extract the type definitions into a `.h`).

Pattern: define types in `types.phc`, transpile to `types.h` (rename output), include from other files:

```makefile
types.h: types.phc
	$(PHC) < $< > $@
```

```c
// parser.phc
#include "types.h"   // includes generated phc_descr types

int parse(Token t) {
    phc_match(Token, t) { ... }
}
```

## Usage Patterns for Terminal Emulators

phc is particularly well-suited for terminal emulator internals. Here are concrete patterns for a VT parser / terminal emulator project.

### VT Parser State Machine

The VT parser processes escape sequences through states. Each state carries different context:

```c
phc_descr VtState {
    Ground {},
    Escape {},
    CsiEntry {},
    CsiParam { int params[16]; int param_count; },
    CsiIntermediate { int params[16]; int param_count; char intermediates[4]; int inter_count; },
    OscString { char buf[256]; int len; },
    DcsEntry {},
    DcsPassthrough { char buf[1024]; int len; }
};
```

State transitions with exhaustive matching:

```c
VtState vt_feed(VtState state, uint8_t byte) {
    phc_match(VtState, state) {
        case Ground: {
            if (byte == 0x1B) return VtState_mk_Escape();
            if (byte >= 0x20) { emit_char(byte); }
            return state;
        } break;
        case Escape: {
            if (byte == '[') return VtState_mk_CsiEntry();
            if (byte == ']') return VtState_mk_OscString(/* empty */);
            return VtState_mk_Ground();
        } break;
        case CsiParam(params, param_count): {
            // Process CSI parameters...
        } break;
        // ... all states must be handled
    }
    return state;
}
```

**Why phc helps:** Adding a new parser state (e.g., for Sixel graphics) requires handling it in every `phc_match`. The compiler catches any switch that doesn't cover the new state.

### Screen Buffer Cells

Each cell can be a character, wide character continuation, or empty:

```c
phc_descr Cell {
    Char { uint32_t codepoint; uint16_t attrs; uint8_t fg; uint8_t bg; },
    WideCont {},           // continuation of a wide character
    Empty { uint16_t attrs; uint8_t fg; uint8_t bg; }
};
```

### SGR Attributes

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

### Input Encoding

```c
phc_descr KeyEvent {
    Char { uint32_t codepoint; },
    Function { int fkey; },          // F1-F12
    Arrow { int direction; },         // Up/Down/Left/Right
    Special { int key; },             // Home/End/Insert/Delete/PageUp/PageDown
    Mouse { int button; int col; int row; }
};
```

### Resource Cleanup with phc_defer

Terminal emulators allocate buffers, file descriptors, and rendering contexts. `phc_defer` ensures cleanup on all exit paths:

```c
int terminal_init(Terminal *term, int rows, int cols) {
    term->cells = calloc(rows * cols, sizeof(Cell));
    if (!term->cells) return -1;
    phc_defer { if (failed) free(term->cells); }

    term->scrollback = scrollback_alloc(1000);
    if (!term->scrollback) return -1;
    phc_defer { if (failed) scrollback_free(term->scrollback); }

    term->alt_screen = calloc(rows * cols, sizeof(Cell));
    if (!term->alt_screen) return -1;

    // All resources allocated — clear the failure flag
    int failed = 0;
    return 0;
}
```

## Testing with phc

phc's test infrastructure can be reused:

```makefile
# Test phc output compiles and runs
test: build/my_program
	./build/my_program

# ASan gate
test-asan:
	$(MAKE) test CFLAGS="$(CFLAGS) -fsanitize=address -fno-omit-frame-pointer"
```

## Known Limitations

- `phc_defer` must be at function scope (not inside loops or blocks)
- `return` inside `phc_match` case bodies is intercepted for defer cleanup, but nested `phc_match` inside case bodies is not yet supported
- Cross-file destructuring requires v2 manifests (regenerate with current phc)
- `longjmp` bypasses `phc_defer` cleanup (documented, same as Go's defer)
