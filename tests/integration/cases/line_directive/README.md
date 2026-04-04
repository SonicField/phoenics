# #line Directive Tests

Tests 01-03: Direct mode (raw .phc → phc → cc). #line works correctly.

Test 04: Pipeline mode (cc -E | phc | cc). KNOWN LIMITATION.
In pipeline mode, phc's #line directives reference preprocessor-output
line numbers, not original source line numbers. phc counts lines in its
input; in pipeline mode, the input is expanded preprocessor output.
Fix requires parsing preprocessor line markers (Option A) — tracked for v3.

The .pipeline_expected_line file records the CORRECT expected line (8),
but is not checked by the test runner. When pipeline-mode #line is fixed,
rename to .expected_line and the test will enforce it.
