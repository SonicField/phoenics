CC ?= clang
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -pedantic -g
SRCDIR = src
TESTDIR = tests
BUILDDIR = build

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))

# Exclude main.o for test builds
LIB_OBJS = $(filter-out $(BUILDDIR)/main.o,$(OBJS))

UNIT_SRCS = $(wildcard $(TESTDIR)/unit/*.c)
UNIT_BINS = $(patsubst $(TESTDIR)/unit/%.c,$(BUILDDIR)/test_%,$(UNIT_SRCS))

.PHONY: all clean test test-unit test-integration test-multifile test-fidelity test-line test-selfhost test-pipeline test-roundtrip

all: $(BUILDDIR)/phc

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/phc: $(OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) $(OBJS) -o $@

# Unit test binaries
$(BUILDDIR)/test_%: $(TESTDIR)/unit/%.c $(LIB_OBJS) | $(BUILDDIR)
	$(CC) $(CFLAGS) -I$(SRCDIR) $< $(LIB_OBJS) -o $@

test: test-unit test-integration test-fidelity test-selfhost test-pipeline test-multifile test-line test-headergen test-stdlib-extern

test-unit: $(UNIT_BINS)
	@echo "=== Unit Tests ==="
	@fail=0; \
	for t in $(UNIT_BINS); do \
		echo "--- $$(basename $$t) ---"; \
		$$t || fail=1; \
	done; \
	if [ $$fail -eq 1 ]; then echo "UNIT TESTS FAILED"; exit 1; fi
	@echo "=== All unit tests passed ==="

test-integration: $(BUILDDIR)/phc
	@echo "=== Integration Tests ==="
	@$(TESTDIR)/integration/run_tests.sh $(BUILDDIR)/phc

# Passthrough fidelity edge cases (empty, CRLF, no-newline, etc.)
test-fidelity: $(BUILDDIR)/phc
	@echo "=== Passthrough Fidelity Tests ==="
	@$(TESTDIR)/integration/passthrough_fidelity_test.sh $(BUILDDIR)/phc

# #line directive tests (P2 — compiler error line number verification)
test-line: $(BUILDDIR)/phc
	@echo "=== #line Directive Tests ==="
	@$(TESTDIR)/integration/line_directive_test.sh $(BUILDDIR)/phc

# Multi-file integration tests (v2 type manifests)
test-multifile: $(BUILDDIR)/phc
	@echo "=== Multi-File Integration Tests ==="
	@$(TESTDIR)/integration/multifile_test.sh $(BUILDDIR)/phc

# Header generation tests (V12 .phc.h headers)
test-headergen: $(BUILDDIR)/phc
	@echo "=== Header Generation Tests ==="
	@$(TESTDIR)/integration/header_gen_test.sh $(BUILDDIR)/phc

# Standard library extern portability tests
test-stdlib-extern: $(BUILDDIR)/phc
	@echo "=== Standard Library Extern Tests ==="
	@$(TESTDIR)/integration/stdlib_extern_test.sh $(BUILDDIR)/phc

# Self-hosting: phc must pass through its own source unchanged
test-selfhost: $(BUILDDIR)/phc
	@echo "=== Self-Hosting Round-Trip Test ==="
	@$(TESTDIR)/integration/roundtrip_test.sh $(BUILDDIR)/phc $(SRCDIR)

# Post-preprocessor pipeline test: cc -E | phc | cc
test-pipeline: $(BUILDDIR)/phc
	@echo "=== Post-Preprocessor Pipeline Test ==="
	@$(TESTDIR)/integration/pipeline_test.sh $(BUILDDIR)/phc $(TESTDIR)/integration/cases

# Passthrough round-trip test against real C files
# Usage: make test-roundtrip ROUNDTRIP_DIR=tests/scale/
ROUNDTRIP_DIR ?= tests/scale
test-roundtrip: $(BUILDDIR)/phc
	@echo "=== Passthrough Round-Trip Test ==="
	@$(TESTDIR)/integration/roundtrip_test.sh $(BUILDDIR)/phc $(ROUNDTRIP_DIR)

# Verify the test framework itself works correctly
test-self:
	@echo "=== Test Framework Self-Test ==="
	@$(CC) $(CFLAGS) -Wno-unused-function -o $(BUILDDIR)/test_self $(TESTDIR)/test_self.c
	@$(BUILDDIR)/test_self

# AddressSanitizer build: catches use-after-free, buffer overflow, memory leaks
test-asan: clean
	@echo "=== ASan Build + Test ==="
	$(MAKE) test CFLAGS="-std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Werror -pedantic -g -fsanitize=address -fno-omit-frame-pointer"

# Valgrind: catches uninitialised reads, leaks, and subtle memory errors
# Stronger than ASan for uninitialised value tracking
# Depends on clean — valgrind and ASan binaries are incompatible
test-valgrind: clean $(BUILDDIR)/phc test-unit
	@echo "=== Valgrind Tests ==="
	@fail=0; \
	for t in $(BUILDDIR)/test_*; do \
		echo "--- valgrind $$(basename $$t) ---"; \
		valgrind --leak-check=full --errors-for-leak-kinds=all --error-exitcode=1 \
			--quiet $$t || fail=1; \
	done; \
	if [ $$fail -eq 1 ]; then echo "VALGRIND FAILED"; exit 1; fi
	@echo "=== Valgrind clean ==="

# MSVC compatibility: verify phc output compiles with strict C11 (no POSIX)
test-msvc-compat: $(BUILDDIR)/phc
	@echo "=== MSVC Compatibility Test ==="
	@$(TESTDIR)/integration/msvc_compat_test.sh $(BUILDDIR)/phc

clean:
	rm -rf $(BUILDDIR)
