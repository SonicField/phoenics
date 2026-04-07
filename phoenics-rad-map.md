# Phoenics Feature Roadmap

Priority-ordered feature list for phoenics, produced by team brainstorm with
architectural review (theologian), BS-detection (testkeeper), and practicality
gating (gatekeeper). Only features that passed all gates are included.

Current state: V9 (fee4b39). Features: phc_descr, phc_match, phc_defer,
phc_defer_cancel, phc_free. 266 tests, ASan clean. Pushed to SonicField/phoenics.

---

## Tier 1 — Build Next (high value, low risk, gatekeeper PASS)

### 1. phc_enum — Enhanced Enums

**Status:** Unanimous PASS. Three independent proposals converged on this.

**Problem:** C enums are weak ints. No exhaustive matching, no string conversion,
no count. Every C project reinvents enum-to-string tables that go stale when
someone adds a value.

**Syntax:**
```c
phc_enum Color {
    Red = 0,
    Green = 1,
    Blue = 2
};
```

**Generated C:**
- `typedef enum { Color_Red = 0, Color_Green = 1, Color_Blue = 2, Color__COUNT } Color;`
- `static inline const char *Color_to_string(Color c)` — switch-based, returns string literal
- `static inline int Color_from_string(const char *s, Color *out)` — strcmp chain
- Integration with `phc_match` for exhaustive matching (case labels, no destructuring)

**Design requirements:**
- Explicit value assignment (`Red = 5`) is mandatory — protocol/wire format code needs it
- Auto-incrementing from last explicit value when not specified (matching C enum behaviour)
- phc_match exhaustiveness checking: every variant must be covered
- Exhaustiveness checker must share the same code path as phc_descr (check "are all
  variants covered?" regardless of type kind). Only codegen differs (no destructuring
  for enums). This keeps the phc_match interaction surface small.

**Implementation:** Structurally simpler than phc_descr (no fields per variant). Parser
recognises `phc_enum` keyword, parses comma-separated names with optional `= integer`,
generates typedef enum + inline functions. Reuses existing codegen patterns.

**Why C needs this:** Enum-to-string is the single most duplicated boilerplate in C
projects. Stale string tables are a common bug class. phc already solves exhaustive
matching for discriminated unions — extending to plain enums is the natural next step.

---

### 2. phc_flags — Type-Safe Bitflags

**Status:** PASS. Same convergence signal as phc_enum.

**Problem:** C flag enums use manual power-of-2 constants. Programmers miscalculate,
duplicate values, forget to update combination masks. Debug-printing flags is always
manual.

**Syntax:**
```c
phc_flags Permissions {
    Read,
    Write,
    Execute
};
```

**Generated C:**
- `typedef unsigned int Permissions;` (NOT enum — bitwise OR of enum values is implementation-defined)
- `#define Permissions_Read   (1u << 0)`
- `#define Permissions_Write  (1u << 1)`
- `#define Permissions_Execute (1u << 2)`
- `#define Permissions__ALL   (Permissions_Read | Permissions_Write | Permissions_Execute)`
- `static inline int Permissions_has(Permissions flags, Permissions flag)`
- `static inline Permissions Permissions_set(Permissions flags, Permissions flag)`
- `static inline Permissions Permissions_clear(Permissions flags, Permissions flag)`
- `static inline const char *Permissions_to_string(Permissions p, char *buf, size_t len)` — "Read|Write" format

**Design requirements:**
- Hard error if flag count exceeds 32 (or 64 with `unsigned long long` variant)
- Explicit value assignment supported for hardware registers. Values are masks, not
  bit indices: `Read = 0x04` means the flag IS 0x04, not `1u << 4`. Using masks
  directly is familiar to C programmers and matches existing codebases.
- Separate keyword from phc_enum — flags and enums have different semantics (combinable vs exclusive)

**Implementation:** Same parser pattern as phc_enum. Different codegen (unsigned int +
defines vs typedef enum). Bounded.

---

## Tier 2 — Build Soon (high value, moderate effort)

### 3. phc_export / phc_import — Auto-Generated Headers

**Status:** PASS from testkeeper and gatekeeper. Fixes real workflow pain.

**Problem:** Cross-file type sharing requires manual .phc-types manifests. Manual
maintenance is fragile and universally hated.

**Syntax:**
```c
/* lib.phc */
phc_export phc_descr Shape {
    Circle { double radius; },
    Rectangle { double width; double height; }
};

/* main.phc */
phc_import("lib.phc")
```

**Generated output:**
- `phc_export` causes phc to emit a standalone `.h` file containing the typedef,
  constructors, and accessor functions for the marked type
- `phc_import` reads from a `.phc` source file and generates the corresponding
  extern declarations and type definitions

**Design requirements:**
- Static inline functions (constructors, accessors) are per-TU in C11 — no linker issues
- **v1: manifest-based (low risk).** phc_export generates a companion .h file from the
  phc_descr declaration. phc_import reads pre-generated manifests/headers.
- **Future extension:** Reading .phc source files directly requires circular import
  detection — design work needed before attempting.
- No C parsing needed — phc already generates the C code, extracting it into a header
  is bounded text generation

**Why C needs this:** Eliminates the manual manifest step from phc's multi-file workflow.
A veteran C programmer would say "finally."

---

### 4. phc_generic / phc_stamp — Type Stamping

**Status:** PASS (with permanent hard limits). Right concept, real slippery-slope
risk mitigated by hard limits that ARE the feature.

**Problem:** Type-safe parameterized containers in C require X-macros or token pasting,
which produce unreadable preprocessor output and cannot integrate with phc_match
exhaustiveness checking.

**Syntax:**
```c
phc_generic(Option, T) {
    phc_descr Option_T {
        Some { T val; },
        None {}
    };
}

phc_stamp(Option, int)    /* generates Option_int with Some/None */
phc_stamp(Option, float)  /* generates Option_float */
```

**Generated C:** The template body with `T` replaced by the concrete type. Stamped
types are full phc_descr types with exhaustiveness checking and destructuring via
phc_match.

**PERMANENT hard limits (these are the feature, not "v1 limits"):**
1. **Single type parameter only.** No `phc_generic(Name, T, U)`.
2. **No specialization.** Every stamp produces identical text with T replaced.
3. **No recursion.** Template bodies cannot contain phc_stamp for the same or
   mutually-referencing templates.
4. **File-scoped definitions only.** No nested generics, no generics inside functions.
5. **Single-identifier type arguments.** `phc_stamp(List, int)` YES.
   `phc_stamp(List, unsigned long long)` NO — use a typedef first.
6. **Same-file only (v1).** phc_generic and phc_stamp must appear in the same
   translation unit. Cross-file via manifests is a future extension.

**Implementation notes:**
- Template bodies stored as raw text during scanning. At stamp sites, substituted
  text is prepended to the lexer's input buffer (same technique as C preprocessor
  macro expansion). Single-pass architecture holds.
- Interaction matrix with other phc constructs (descr, match, defer) must be
  thoroughly tested.
- **#line markers:** Prepended template text has no original source position.
  Stamped output MUST emit `#line` directives pointing back to the phc_generic
  definition site, not the stamp site. Without this, compiler errors inside
  stamped bodies report wrong file:line — breaking phc's debuggable-output
  guarantee. This is a design requirement, not optional.
- **Interaction with V7 recursive parser:** The lexer-prepend mechanism and the
  recursive body parser both manipulate the lexer input. Their interaction must
  be prototyped and tested before committing to this architecture.

**No "standard library" shipped.** phc does NOT ship blessed Optional/Result/Slice
templates. Users write their own. Documentation provides examples. This is C
philosophy — build primitives, let users compose.

**Why C needs this:** Defines patterns once, stamps for each type, with full phc_match
exhaustiveness. This capability does not exist in C or the preprocessor.

---

## Tier 3 — Consider Later (value exists but significant effort or unresolved debate)

### 5. phc_assert — Multi-Level Assertions

**Status:** PASS (testkeeper proposal, theologian endorsed). Solves the contract
motivation without requiring function signature parsing.

**Problem:** C's `assert()` is controlled by NDEBUG — all-or-nothing. Contracts
(preconditions, invariants) are specifications, not debugging aids. You want to keep
preconditions active in production while disabling verbose invariant checks.

**Syntax:**
```c
phc_precondition(b != 0, "divisor must be nonzero");
phc_invariant(list->count >= 0, "count must be non-negative");
```

**Generated C:** Conditional assertions controlled by compile-time level, independent
of NDEBUG. phc does compile-time stripping (like -DPHC_TEST) rather than runtime level
checks — this is what differentiates it from a macro.

**Design requirement:** phc must do compile-time stripping of assertion blocks (like
-DPHC_TEST strips test blocks), not runtime level checks. Compile-time stripping is
what differentiates this from a macro and justifies phc-level processing.

---

### 6. phc_test — Inline Test Blocks

**Status:** PASS from gatekeeper. Testkeeper calls it "mixed" (process problem vs
language feature). Debatable but real AI-workflow value.

**Problem:** C has no standard test framework. Every project reinvents test
infrastructure. AI agents writing test-first need zero-friction test syntax.

**Syntax:**
```c
phc_test("option none is safe") {
    Option_int x = Option_int_mk_None();
    /* ... assertions ... */
}
```

**Generated C:** Each phc_test becomes a function. phc emits a `main()` that calls
them all and reports results. Compile with `-DPHC_TEST` to include tests; without it,
test blocks are stripped entirely.

**Design note:** main() collision must be documented — test files should not define
their own main(), or user main() should be guarded with `#ifndef PHC_TEST`.

---

### 7. phc_loop / phc_break / phc_continue — Named Loop Control

**Status:** Gatekeeper PASS (practicality). Testkeeper MARGINAL (C has goto).
Theologian revised to MARGINAL after accepting parser-expansion concern.

**Problem:** Breaking out of nested loops requires goto with manual labels — error-prone
(wrong label, stale label after refactoring).

**Syntax:**
```c
phc_loop(outer) for (int i = 0; i < n; i++) {
    phc_loop(inner) for (int j = 0; j < m; j++) {
        if (found) phc_break(outer);
    }
}
```

**Generated C:** goto + generated labels. Bounded, C11 compatible, ABI-neutral.

**Concern:** Requires phc to recognise for/while/do keywords and track loop nesting.
Currently phc only recognises phc_* keywords. Disproportionate parser expansion for
the value delivered.

---

### 8. phc_scope — Block-Scoped Defer

**Status:** CONDITIONAL PASS. Feasible but complex interaction with function-scoped
defer model.

**Problem:** phc_defer is function-scoped. Lock/unlock patterns and other block-scoped
cleanup currently require helper functions.

**Syntax:**
```c
phc_scope {
    Mutex *m = acquire_lock();
    phc_defer { release_lock(m); }
    /* ... work ... */
}  /* defer fires here, not at function end */
```

**Concern:** Interaction semantics with function-scoped defers (nesting order, goto
escape) are unspecified. phc_defer already covers ~90% of cleanup patterns. Marginal
value for significant implementation complexity.

---

## Killed (with reason)

| Proposal | Verdict | Reason |
|----------|---------|--------|
| phc_optional(T) | BS | Sugar for 1-line phc_descr. Type parameter parsing blocker. |
| phc_result(T, E) | BS | Sugar for phc_descr + phc_try is infeasible. |
| phc_slice(T) | BS | Opt-in safety is false security. `slice.data[n]` bypasses bounds. Header macro equivalent. |
| phc_auto | BS | Requires type inference (infeasible). Violates C philosophy. |
| phc_foreach | BS | It's a C macro, not a phc feature. |
| phc_try | BLOCKED | Requires function return type knowledge. |
| phc_guard | BS | It's a C macro. |
| phc_auto_cleanup | BS | Redundant with phc_defer. |
| phc_contract | BLOCKED | phc_require is a macro; phc_ensure requires function signature parsing. |
| phc_serial | BS | Opinionated library concern (which JSON library?). |

---

## Notes

- **phc_optional and phc_slice as documentation examples:** Once phc_generic ships,
  Optional and Slice patterns should be documented as user-space phc_generic examples,
  not shipped as built-in templates. C philosophy: build primitives, let users compose.

- **phc_flags vs phc_enum mode:** Team considered `phc_enum(flags)` as a single keyword.
  Decision: separate keywords. Flags and enums have fundamentally different semantics
  (combinable vs exclusive) and different generated code.

- **phc_generic slippery slope:** The hard limits are PERMANENT. They are not "v1 limits"
  to be relaxed later. The distance from textual stamping to C++ templates is measured
  in feature requests. The answer to "can we add partial specialization?" is "no — write
  a phc_descr."

---

*Generated 2026-04-07 by team brainstorm: supervisor (coordination), theologian
(architecture), testkeeper (falsification), gatekeeper (practicality), generalist
(implementation). All proposals subjected to alexie's four criteria: extends C
philosophy, AI-usable from docs, no ABI change, "C really needed that" elegance.*
