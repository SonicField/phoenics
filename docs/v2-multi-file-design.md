# v2 Design Sketch: Multi-File descr Support

## Problem

v1 processes one file at a time. `match_descr` requires the descr declaration to be in the same file. This makes exhaustive matching unavailable across translation units — the core safety feature is confined to single-file programs.

## Recommended Approach: Type Manifest Files

When phc processes a `.phc` file containing descr declarations, it emits two outputs:

1. The standard `.c` (or `.h`) output (existing behaviour)
2. A `.phc-types` manifest file listing all descr types and their variants

When phc processes a file containing `match_descr`, it resolves types from:
1. descr declarations in the current file (existing behaviour)
2. Any `.phc-types` manifests referenced by the file

### Manifest Format

For `shapes.phc` containing:
```c
descr Shape {
    Circle { double radius; },
    Rectangle { double width; double height; }
};
```

phc emits `shapes.phc-types`:
```
# phc type manifest — machine generated, do not edit
# source: shapes.phc
descr Shape Circle Rectangle
```

One line per descr type: `descr <TypeName> <Variant1> <Variant2> ...`

The format is deliberately minimal — it contains only what the semantic analyser needs for exhaustiveness checking (type names and variant names). Field types and names are not needed.

### How phc Discovers Manifests

**Option A: Implicit discovery via `#include`**

phc scans passthrough text for `#include "..."` directives (not `#include <...>` — system headers never contain descr types). For each local include, it checks whether a corresponding `.phc-types` file exists alongside the `.h` file. If yes, it reads the manifest and adds the types to the semantic analyser's type table.

Example: `#include "shapes.h"` → look for `shapes.phc-types` in the same directory.

- **Pro:** No new syntax. Works with existing include patterns.
- **Con:** Relies on naming convention. If the `.h` was renamed or moved, discovery fails silently.
- **Falsifier:** `#include "path/to/shapes.h"` but `shapes.phc-types` is in a different directory.

**Option B: Explicit `phc_types` directive**

A new Phoenics directive:
```c
phc_types "shapes.phc-types"
```

phc recognises this directive and reads the referenced manifest. The directive is consumed (not emitted to output).

- **Pro:** Explicit, no naming convention dependency, no silent failures.
- **Con:** New syntax. Redundant with `#include` (the user must both `#include "shapes.h"` for the C types and `phc_types "shapes.phc-types"` for exhaustiveness checking).
- **Falsifier:** User forgets the `phc_types` directive — match_descr fails. But this is a clear error message, not silent breakage.

**Recommendation: Option A with fallback to Option B.**

Try implicit discovery first (check for `.phc-types` alongside included `.h` files). If the user needs explicit control (non-standard paths, multiple manifest sources), `phc_types` is available. This covers both the common case (zero friction) and the edge case (explicit control).

### Build System Integration

The Makefile rule for `.phc` → `.c` becomes:
```makefile
%.c %.phc-types: %.phc
	phc $< -o $*.c --emit-types=$*.phc-types

%.h %.phc-types: %.phc
	phc $< -o $*.h --emit-types=$*.phc-types
```

The `--emit-types` flag is new. When present, phc writes the manifest alongside the primary output.

**Dependency ordering:** Files containing `match_descr` on external types depend on the manifest being generated first. The Makefile must ensure header `.phc` files are processed before source `.phc` files. This is analogous to the standard C requirement that headers exist before compilation.

### Implementation Changes

| Component | Change |
|-----------|--------|
| Lexer | No change — manifests are loaded before lexing the main file |
| Parser | Add `#include` detection in passthrough scanning (for implicit discovery) |
| Semantic | Accept an external type list alongside the parsed Program |
| Codegen | No change — match_descr codegen is type-name-based, source of type doesn't matter |
| Main | Add `--emit-types` flag. Add manifest loading before `analyse()`. Add `phc_types` directive handling if Option B. |

### Invariants

1. **Manifest accuracy:** The manifest MUST reflect the descr declarations in the corresponding `.phc` file. If the `.phc` file changes and the manifest is stale, `match_descr` validation may accept or reject incorrectly. Mitigation: the build system regenerates manifests when source changes (standard make dependency tracking).
2. **Manifest-output consistency:** The `.phc-types` manifest and the `.h` output MUST be generated from the same `.phc` source in the same phc invocation.
3. **No false discovery:** Implicit manifest discovery MUST NOT load manifests for non-Phoenics headers. A standard C `.h` file without a `.phc-types` companion is silently ignored — no error.

### What This Does NOT Solve

- Cross-file **field access** validation (e.g., checking that `s.Circle.radius` is valid when Shape is defined elsewhere). This requires full type information, not just variant names. The C compiler handles this.
- **Forward declarations** of descr types. A manifest lists complete types only.
- **Conditional descr** (`#ifdef` around variants). The manifest is unconditional.

These are separate v2+ concerns.

## Falsifiers for This Design

1. **Circular dependencies:** File A imports types from file B and vice versa. The manifest approach handles this — manifests are generated independently from each file's descr declarations, not from match_descr usage.
2. **Stale manifest:** User modifies a descr (adds a variant), regenerates the `.h` but not the `.phc-types`. A `match_descr` in another file appears exhaustive against the stale manifest but is missing the new variant at runtime. Mitigation: build system must treat `.phc-types` as a primary output, not optional.
3. **Manifest without header:** A `.phc-types` exists but the corresponding `.h` was deleted. phc loads type definitions that don't exist in the compiled code. Mitigation: the C compiler will error when the `#include` fails — phc doesn't need to check this independently.
