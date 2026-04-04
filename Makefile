CC ?= clang
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -g
SRCDIR = src
TESTDIR = tests
BUILDDIR = build

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))

# Exclude main.o for test builds
LIB_OBJS = $(filter-out $(BUILDDIR)/main.o,$(OBJS))

UNIT_SRCS = $(wildcard $(TESTDIR)/unit/*.c)
UNIT_BINS = $(patsubst $(TESTDIR)/unit/%.c,$(BUILDDIR)/test_%,$(UNIT_SRCS))

.PHONY: all clean test test-unit test-integration

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

test: test-unit test-integration

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
	@echo "=== All integration tests passed ==="

clean:
	rm -rf $(BUILDDIR)
