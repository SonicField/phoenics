# Case Study: Hardening a Tcl Binding with phc_assert

How phc_require, phc_check, and phc_invariant prevented a 12-commit debugging
spiral in a real terminal emulator.

---

## The Project

nbs-term is a terminal emulator built as a Python/C/Tcl hybrid. The C extension
(written in Phoenics) handles terminal semantics — VT parsing, screen buffers,
input encoding. It renders to a Tk canvas by calling directly into the Tcl
interpreter via `Tcl_EvalObjv`.

The architecture has three trust boundaries:

```
Python (user input, SSH data)
    ↓ PyArg_ParseTuple
C extension (terminal logic)
    ↓ Tcl_EvalObjv
Tcl interpreter (canvas rendering)
```

Each boundary passes data the receiver cannot trust.

---

## The Problem

The Tcl binding proved brittle. The git log records 12 commits over several
days to solve one problem: Tcl version portability on macOS.

```
48685a0  Fix Mac Tcl linkage: auto-detect library name
256b1e1  Fix Mac build: add platform-specific Tcl linkage
4d412f7  Mac: use -framework Tcl instead of guessing
5502ff5  Fix Mac Tcl crash: link same Tcl library as Python's _tkinter
278df99  Derive Tcl linkage from Python's _tkinter
7323b11  Mac: link Tcl by full path
46f4802  Fix Mac Tcl linkage: fall back to Homebrew
a5a3cbc  Use Tcl stubs for runtime resolution (the actual fix)
9494b8e  Fix TclFreeObj: resolve via dlsym on Mac
f26487f  Eliminate all pre-stubs Tcl calls
ab97bf5  Fix USE_TCL_STUBS: define BEFORE tcl.h
677f206  Fix Tcl_InitStubs: use TCL_VERSION not "8.6"
```

Each commit fixed a symptom. The root cause — link-time Tcl resolution instead
of runtime stubs — was not diagnosed until commit 8 of 12.

Every crash occurred at a trust boundary where an assumption about Tcl state
was implicit: "the interpreter is valid," "stubs are initialised," "this symbol
exists." When the assumption was wrong, the programme segfaulted with no
diagnostic.

---

## The Diagnosis: Implicit Trust

The code treated external state as trusted:

```c
/* Before: implicit trust */
Tcl_Interp *interp = (Tcl_Interp *)(uintptr_t)interp_addr;
/* If interp_addr is wrong, this is a wild pointer.
   The next Tcl call segfaults. No diagnostic. */

Tcl_Obj *objv[11];
objv[0] = Tcl_NewStringObj(canvas_path, -1);
/* If canvas_path is NULL, this crashes inside Tcl.
   The error message is Tcl's, not ours. */

int rc = Tcl_EvalObjv(interp, 11, objv, 0);
/* If stubs aren't initialised, this calls through
   a NULL function pointer. Segfault. */
```

The pattern: data crosses a boundary, code assumes it is valid, the programme
crashes when it is not. The developer debugs the crash, fixes the symptom,
and moves to the next crash. Twelve times.

---

## The Fix: Explicit Falsification

phc_assert makes each assumption explicit and falsifiable:

```c
/* After: explicit falsification */
Tcl_Interp *interp = (Tcl_Interp *)(uintptr_t)interp_addr;
phc_require(interp != NULL, "Tcl interpreter must not be NULL");
phc_require(tcl_stubs_initialized, "Tcl stubs not initialised");

phc_require(canvas_path != NULL, "canvas path must not be NULL");
phc_require(canvas_path[0] != '\0', "canvas path must not be empty");

int rc = Tcl_EvalObjv(interp, 11, objv, 0);
phc_require(rc == TCL_OK, "Tcl_EvalObjv failed");
```

Each `phc_require` is a hypothesis test: "I hypothesise this state is correct."
The assertion attempts to disprove it. If it succeeds (the state is wrong),
the programme aborts with a message identifying the exact boundary violation.

The 12-commit debugging spiral becomes one diagnostic: `"Tcl stubs not
initialised"` — pointing directly at the root cause.

---

## Three Levels of Falsification

Not every assertion has the same cost-benefit profile. phc_assert provides
three levels, distinguished by trust:

### phc_require — permanent boundary guards

Always active. Never stripped. For data you do not control.

```c
/* Python passes an interpreter address as an integer.
   We cannot trust it. */
phc_require(interp_addr != 0, "interpreter address is zero");

/* Tcl returns a result. We cannot trust the external library. */
phc_require(rc == TCL_OK, "Tcl command failed");

/* The caller passes dimensions. They come from user input. */
phc_require(rows > 0 && rows <= 500, "rows out of range");
```

These stay in production. Untrusted data can be wrong at any time.

### phc_check — development correctness checks

Active during development, stripped in production via `--strip-check`.
For output from your own code.

```c
/* Our render function should return a valid screen. */
phc_check(screen != NULL, "render_screen returned NULL");

/* Our buffer index should be 0 or 1.
   (phc_require if buf comes from external state;
    phc_check if it's our own logic.) */
phc_check(buf == 0 || buf == 1, "buffer index out of range");

/* Font measure should return a positive width. */
phc_check(width > 0, "font measure returned non-positive");
```

These validate your own logic. Once confident, strip them.

### phc_invariant — expensive structural checks

Active only during testing, stripped first via `--strip-invariant`.
For properties that are expensive to verify.

```c
/* After every byte: is the parser state still valid? */
phc_invariant(parser->state.tag < VTState__COUNT, "parser state corrupted");

/* After scroll: is the scroll region still valid? */
phc_invariant(scr->scroll_top <= scr->scroll_bottom, "scroll region inverted");
```

These are the heavy artillery — run them when hunting for bugs, strip them
when the cost exceeds the value.

---

## Build Profiles

```makefile
# Testing — all active
phc < extension.phc > extension.c

# Development — strip expensive invariants
phc --strip-invariant < extension.phc > extension.c

# Production — keep only boundary guards
phc --strip-check --strip-invariant < extension.phc > extension.c
```

Stripped assertions leave a visible comment:

```c
/* phc_check stripped: hypothesis accepted without verification */
```

A reader of the generated C knows a check was defined here and deliberately
removed. No invisible assumptions.

---

## Results: nbs-term Assertion Map

The analysis identified 41 assertion sites across 7 trust boundaries:

| Boundary | phc_require | phc_check | phc_invariant | Total |
|----------|-------------|-----------|---------------|-------|
| Tcl probe (tk_render.phc) | 5 | 2 | 1 | 8 |
| Tcl render pipeline (extension.phc) | 7 | 4 | 1 | 12 |
| Python C extension | 5 | 2 | 0 | 7 |
| Config | 5 | 1 | 0 | 6 |
| Selection | 2 | 0 | 0 | 2 |
| Colour utilities | 1 | 0 | 0 | 1 |
| Internal (screen/parser) | 0 | 2 | 3 | 5 |
| **Total** | **25** | **11** | **5** | **41** |

25 permanent boundary guards. 11 development checks. 5 structural invariants.
The Tcl rendering pipeline — the source of the original brittleness — has the
most assertions (12), concentrated on the functions that call `Tcl_EvalObjv`
hundreds of times per frame.

---

## The Principle

Each assertion is a falsification attempt: "I hypothesise this state is correct.
Here is the test that would disprove me."

- `phc_require`: the permanent falsifier. The hypothesis "this input is valid"
  must be tested on every call, in every build, forever.
- `phc_check`: the development falsifier. The hypothesis "my code is correct"
  is tested while building confidence and withdrawn once earned.
- `phc_invariant`: the structural falsifier. The hypothesis "this data structure
  is consistent" is tested when actively hunting and withdrawn when the cost
  exceeds the value.

You cannot prove code correct. You can prove it wrong. phc_assert is the
instrument.
