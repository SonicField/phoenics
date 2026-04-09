# Phoenics Examples — Build and run all demos
#
# Usage:
#   make              — build and run all examples
#   make 01           — build and run descr basics
#   make 06_strip     — build assert levels with stripping variants
#   make clean        — remove build artifacts

PHC ?= ../build/phc
CC ?= cc
CFLAGS := -std=c11 -Wall -Wextra -Werror -Wno-unused-function
BUILDDIR := build

.PHONY: all clean 01 02 03 04 05 06 06_strip 07 08

all: 01 02 03 04 05 06 06_strip 07 08

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# --- 01: Discriminated unions (phc_descr) ---
01: $(BUILDDIR)
	@echo "=== 01: phc_descr basics ==="
	$(PHC) < 01_descr_basics.phc | $(CC) $(CFLAGS) -x c - -o $(BUILDDIR)/01_descr_basics
	@$(BUILDDIR)/01_descr_basics
	@echo ""

# --- 02: Exhaustive matching (phc_match) ---
02: $(BUILDDIR)
	@echo "=== 02: phc_match patterns ==="
	$(PHC) < 02_match_patterns.phc | $(CC) $(CFLAGS) -x c - -o $(BUILDDIR)/02_match_patterns
	@$(BUILDDIR)/02_match_patterns
	@echo ""

# --- 03: Enhanced enums (phc_enum) ---
03: $(BUILDDIR)
	@echo "=== 03: phc_enum colors ==="
	$(PHC) < 03_enum_colors.phc | $(CC) $(CFLAGS) -x c - -o $(BUILDDIR)/03_enum_colors
	@$(BUILDDIR)/03_enum_colors
	@echo ""

# --- 04: Type-safe bitflags (phc_flags) ---
04: $(BUILDDIR)
	@echo "=== 04: phc_flags permissions ==="
	$(PHC) < 04_flags_permissions.phc | $(CC) $(CFLAGS) -x c - -o $(BUILDDIR)/04_flags_permissions
	@$(BUILDDIR)/04_flags_permissions
	@echo ""

# --- 05: Automatic cleanup (phc_defer) ---
05: $(BUILDDIR)
	@echo "=== 05: phc_defer cleanup ==="
	$(PHC) < 05_defer_cleanup.phc | $(CC) $(CFLAGS) -x c - -o $(BUILDDIR)/05_defer_cleanup
	@$(BUILDDIR)/05_defer_cleanup
	@echo ""

# --- 06: Assertions (phc_require/phc_check/phc_invariant) ---
06: $(BUILDDIR)
	@echo "=== 06: phc_assert levels (all active) ==="
	$(PHC) < 06_assert_levels.phc | $(CC) $(CFLAGS) -x c - -o $(BUILDDIR)/06_assert_all
	@$(BUILDDIR)/06_assert_all 5
	@echo ""

# Build three stripping variants to show compile-time control
06_strip: $(BUILDDIR)
	@echo "=== 06: stripping variants ==="
	@echo "--- all active ---"
	$(PHC) < 06_assert_levels.phc > $(BUILDDIR)/06_all.c
	@grep -c 'abort()' $(BUILDDIR)/06_all.c | xargs printf "  abort() calls: %s\n"
	@echo "--- --strip-invariant ---"
	$(PHC) --strip-invariant < 06_assert_levels.phc > $(BUILDDIR)/06_no_inv.c
	@grep -c 'abort()' $(BUILDDIR)/06_no_inv.c | xargs printf "  abort() calls: %s\n"
	@grep 'stripped' $(BUILDDIR)/06_no_inv.c || true
	@echo "--- --strip-check --strip-invariant ---"
	$(PHC) --strip-check --strip-invariant < 06_assert_levels.phc > $(BUILDDIR)/06_prod.c
	@grep -c 'abort()' $(BUILDDIR)/06_prod.c | xargs printf "  abort() calls: %s\n"
	@grep 'stripped' $(BUILDDIR)/06_prod.c || true
	@echo ""

# --- 07: Cross-file type sharing (--emit-types + .phc.h) ---
07: $(BUILDDIR)
	@echo "=== 07: multi-file type sharing ==="
	$(PHC) --emit-types=$(BUILDDIR)/shapes.phc-types < 07_multi_file/shapes.phc > $(BUILDDIR)/shapes.c
	$(PHC) --type-manifest=$(BUILDDIR)/shapes.phc-types < 07_multi_file/main.phc > $(BUILDDIR)/main.c
	$(CC) $(CFLAGS) -I$(BUILDDIR) -x c $(BUILDDIR)/main.c -o $(BUILDDIR)/07_multi_file
	@$(BUILDDIR)/07_multi_file
	@echo ""

# --- 08: Pipeline mode (cc -E | phc | cc) ---
08: $(BUILDDIR)
	@echo "=== 08: pipeline mode ==="
	$(CC) -x c -E 08_pipeline.phc | $(PHC) | $(CC) $(CFLAGS) -x c - -o $(BUILDDIR)/08_pipeline
	@$(BUILDDIR)/08_pipeline
	@echo ""

clean:
	rm -rf $(BUILDDIR)
