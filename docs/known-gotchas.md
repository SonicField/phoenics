# Known Gotchas

Platform-specific issues and integration pitfalls when using phc-generated code.

## PY_SSIZE_T_CLEAN and Python Extensions

**Affects:** Projects that compile phc-generated C as part of a CPython extension module (Python 3.10+).

**Symptom:** `SystemError: PY_SSIZE_T_CLEAN macro must be defined for '#' formats` at runtime when calling any function that uses `PyArg_ParseTuple` with `#` format codes.

**Cause:** CPython 3.10 made `PY_SSIZE_T_CLEAN` mandatory. This macro must be `#define`d *before* `#include <Python.h>`. Without it, `#` formats use `int` instead of `Py_ssize_t`, which Python 3.12+ rejects as a hard error.

phc-generated `.phc.h` headers include `<stdlib.h>` and `<string.h>` but never `<Python.h>`. The issue arises when a consumer's build system includes the phc header before `Python.h` without defining `PY_SSIZE_T_CLEAN` first:

```c
/* WRONG — PY_SSIZE_T_CLEAN not defined before Python.h */
#include "types.phc.h"
#include <Python.h>       /* too late — PY_SSIZE_T_CLEAN must precede this */
```

**This is the same class of problem as `USE_TCL_STUBS`:** a define that must precede an include. The ordering is a CPython design requirement, not a phc bug. See [case-study-nbs-term.md](case-study-nbs-term.md) for the TCL equivalent of this pattern.

**Workaround:** Define `PY_SSIZE_T_CLEAN` at the top of every translation unit that includes `Python.h`, or pass it as a compiler flag:

```c
/* Option 1: define in source (before any includes) */
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "types.phc.h"
```

```makefile
# Option 2: define via compiler flags (recommended for build systems)
CFLAGS += -DPY_SSIZE_T_CLEAN
```

**Why phc doesn't handle this automatically:** phc generates standard C11. It has no knowledge of CPython or its macro requirements. The `PY_SSIZE_T_CLEAN` define is a CPython-specific concern that belongs in the consumer's build configuration, not in generated type headers.

## Windows cp1252 Encoding

**Affects:** Projects reading phc source files on Windows with default locale encoding.

**Symptom:** `UnicodeDecodeError: 'charmap' codec can't decode byte 0x81` when Python code opens phc-generated files without specifying encoding.

**Cause:** Windows defaults to cp1252 encoding, which cannot decode all UTF-8 byte sequences. phc source and generated files are UTF-8.

**Workaround:** Always specify `encoding='utf-8'` when opening phc files in Python:

```python
with open("types.phc.h", encoding="utf-8") as f:
    content = f.read()
```

## Windows Path Separators

**Affects:** Build systems that construct paths by string concatenation on Windows.

**Symptom:** Path mismatches like `/fake/appdata\nbs` vs `/fake/appdata/nbs`.

**Workaround:** Use `os.path.join()` or `pathlib.Path` instead of string concatenation for all path construction.
