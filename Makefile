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

.PHONY: all clean test test-unit test-integration test-selfhost test-roundtrip

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

test: test-unit test-integration test-selfhost

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

# Self-hosting: phc must pass through its own source unchanged
test-selfhost: $(BUILDDIR)/phc
	@echo "=== Self-Hosting Round-Trip Test ==="
	@$(TESTDIR)/integration/roundtrip_test.sh $(BUILDDIR)/phc $(SRCDIR)

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

clean:
	rm -rf $(BUILDDIR)
