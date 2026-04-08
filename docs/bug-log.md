# Phoenics Bug Log

Bugs reported and fixed in the phoenics (phc) project, in chronological order.

| # | Title | Date | Description | Fix |
|---|-------|------|-------------|-----|
| 1 | Test framework false PASS | 2026-04-04 | test_framework.h reported failing tests as PASS. `_tf_current_failed` flag not reset between tests — failing tests counted as passed. The tool that verifies correctness was itself incorrect. | Reset flag per test. tests/test_framework.h. |
| 2 | Codegen corruption on compact case blocks | 2026-04-04 | Backward newline search in position-based source splicing produced garbage on single-line phc_match case blocks. All existing tests passed only because they used multi-line formatting. | Clamped search, added single-line path (45423bc). Regression test case 14 added. |
| 3 | Keyword collision with C identifiers | 2026-04-04 | `descr` and `match_descr` were reserved words that collided with user code (struct fields, variables, macros). phc could not process its own source files. | Renamed to `phc_descr` and `phc_match` with `phc_` namespace prefix (5546fe9). |
| 4 | Comments between phc_match cases | 2026-04-04 | Comments in phc_match case bodies caused parse errors. | Fixed parser comment handling (b5c8a6d). |
| 5 | `__builtin_trap()` not portable | 2026-04-05 | Generated accessor functions used `__builtin_trap()` for tag-mismatch termination. GCC/Clang extension, not standard C11. | Replaced with `abort()` from `<stdlib.h>` (5c71366). |
| 6 | Pipeline `#include` stdlib.h redefinition | 2026-04-05 | Post-preprocessor mode: typedef redefinition errors from `#include <stdlib.h>` in generated output. | Emit `extern void abort(void)` instead of `#include` (676f99c). |
| 7 | Manifest fgets buffer overflow | 2026-04-05 | `fgets()` silently truncated lines >4096 bytes in manifest parser, corrupting the type table. | Switched to `getline()` (5189ea2). |
| 8 | Return expression semicolon splitting | 2026-04-06 | Parser split at wrong semicolon for complex return expressions containing `sizeof(struct{...})`. | Depth-0 semicolon scanning (0949d88). |
| 9 | C keyword as field name accepted | 2026-04-06 | phc_descr field names could be C keywords (e.g., `int`, `return`), generating uncompilable C. | Semantic validation rejects C keywords as field names (0949d88). |
| 10 | phc_defer inside loops | 2026-04-06 | Defer cleanup body emitted outside loop scope, causing incorrect behaviour. | Restricted defer to function scope (brace depth 1) (0949d88). |
| 11 | Body text scanner comment-skipping | 2026-04-06 | Defer cleanup code inserted inside C comments. | Scanner now skips `//` and `/* */` blocks (0e04af9). |
| 12 | Cross-function defer contamination | 2026-04-06 | Defer state leaked between functions in the same file. | Lexer resets `defer_active` at FUNC_END (0440f38, V8). |
| 13 | ASan leak: MatchCase.bindings | 2026-04-06 | `parse_result_free()` missing cleanup of `bindings[]` array. | Free bindings in match case loop (a4abf2d). |
| 14 | Manifest field parser buffer overflows | 2026-04-06 | `tokens[64]` stack array and `type_buf[256]` with unchecked `strcat` could overflow. | Switched to dynamic allocation (10e7ef0). |
| 15 | Manifest NULL type_name for complex fields | 2026-04-06 | Uninitialised `is_array` from `realloc` caused NULL `type_name` in manifest output. | `memset` before field assignment (7908fc0). |
| 16 | ASan leak: free_match_contents end_file | 2026-04-07 | `free_match_contents()` missing `free(m->end_file)`. Pre-existing bug exposed by V12's cross-file pipeline test. | Added `free(m->end_file)` (e1d4fd9, V12). |
| 17 | Cross-file phc_enum matching broken | 2026-04-07 | `is_enum_type()` only checked local `EnumDecl` array, not external types from manifests (with `is_enum=1`). Generated `switch(expr.tag)` instead of `switch(expr)` for external enums — compile failure. | `is_enum_type()` now checks `ext_types` for `is_enum=1` (86f6dec). Tests added: cross_file_enum_match, cross_file_flags_match. |
| 18 | Bare extern declarations conflict with _FORTIFY_SOURCE | 2026-04-08 | Generated code emitted bare `extern int snprintf(...)`, `extern int strcmp(...)`, and `extern void abort(...)`. These conflict with platforms where _FORTIFY_SOURCE redefines standard library functions as macros (e.g., macOS). 6 emission points in codegen.c. | Replace bare externs with guarded `#include` directives. Fix in progress. |
