# Flea-Scope firmware build (PIC32MK0512GPK064, MPLAB XC32).
#
#   make                  build out/pic32.X.production.{elf,hex,map}
#   make fetch-dfp        download the pinned PIC32MK-GP device pack
#   make test             host unit tests (Unity, test/unit/test_*.c)
#   make lint             clang-tidy and cppcheck over the firmware sources
#   make clean
#
# Override XC32_ROOT / DFP_DIR on the command line or in the environment if
# the toolchain or pack live elsewhere.
#
# Sources are discovered by wildcard, so a new .c file under src/app/ or
# src/platform/ is built without editing this file.

# Pinned toolchain. A different XC32 or DFP version produces a different hex.
XC32_VERSION := v6.00
DFP_VERSION  := 1.10.264
DFP_SHA256   := f8a5dfcc7ba39e928f0ef6a029930438434691574c0c7ed7fd2e0d2bf408094d
DFP_URL      := https://packs.download.microchip.com/Microchip.PIC32MK-GP_DFP.$(DFP_VERSION).atpack

XC32_ROOT ?= /opt/microchip/xc32/$(XC32_VERSION)
XC32_CC   := $(XC32_ROOT)/bin/xc32-gcc
BIN2HEX   := $(XC32_ROOT)/bin/xc32-bin2hex

# Pack location: the MPLAB Tools for VS Code / `make fetch-dfp` directory, else an
# MPLAB X install, else the fetch directory again (so `make fetch-dfp all` works).
# Never empty: CFLAGS/LDFLAGS expand it once, at parse time.
DFP_FETCH_DIR := $(HOME)/.mchp_packs/Microchip/PIC32MK-GP_DFP/$(DFP_VERSION)
DFP_DIR ?= $(firstword $(wildcard $(DFP_FETCH_DIR) \
                                  /opt/microchip/mplabx/v*/packs/Microchip/PIC32MK-GP_DFP/$(DFP_VERSION)) \
                       $(DFP_FETCH_DIR))

BUILD_DIR ?= out
TARGET    := $(BUILD_DIR)/pic32.X.production

# `make SODEBUG=1` builds with debug assert()s and checks (main.h); CI builds
# both configurations, so the warnings stay clean in each
SODEBUG_FLAG := $(if $(SODEBUG),-DSODEBUG=$(SODEBUG))

PROCESSOR := 32MK0512GPK064

SRCS ?= $(sort $(wildcard src/app/*.c)) \
        $(sort $(wildcard src/platform/*.c)) \
        src/main.c src/myplib.c

OBJS := $(SRCS:%.c=$(BUILD_DIR)/obj/%.o)

# Warnings for all C we compile, firmware and host tests; any warning fails the
# build. In C, -Wconversion includes -Wsign-conversion.
WARNINGS := -Wall -Wextra -Wpedantic -Wconversion \
            -Wshadow -Wundef -Wcast-qual -Wcast-align=strict -Wpointer-arith -Wwrite-strings \
            -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations \
            -Wredundant-decls -Wnested-externs \
            -Wformat=2 -Wformat-signedness -Wformat-overflow=2 -Wformat-truncation=2 \
            -Wswitch-enum -Wswitch-default \
            -Wnull-dereference -Wduplicated-cond -Wduplicated-branches -Wlogical-op \
            -Wjump-misses-init -Winit-self -Wbad-function-cast -Wdouble-promotion -Wfloat-equal \
            -Wshift-overflow=2 -Wstringop-overflow=4 -Warray-bounds=2 -Walloc-zero -Wvla -Walloca \
            -Wsuggest-attribute=noreturn -Wsuggest-attribute=format -Wunused-macros -Wpacked \
            -Wtrampolines -Wdisabled-optimization -Wdate-time -Wmissing-include-dirs \
            -Werror

# Firmware only: no stack frame may exceed 512 bytes of the 3 KiB stack
# (_min_stack_size below; -Wvla/-Walloca keep frames fixed-size), not
# -Wstack-usage, which XC32 can't compute for interrupt handlers. -fanalyzer is
# GCC's interprocedural static analyzer (use after free, uninitialized reads,
# NULL dereferences, leaks).
CFLAGS := -g -std=gnu17 -mprocessor=$(PROCESSOR) -mmicromips -mno-float \
          -O3 -funroll-loops -fomit-frame-pointer -fno-common -fno-builtin \
          -Isrc -Isrc/app -Isrc/platform $(SODEBUG_FLAG) \
          $(WARNINGS) -Waddress-attribute-use -Wframe-larger-than=512 -fanalyzer \
          -mdfp="$(DFP_DIR)"

LDFLAGS := -mprocessor=$(PROCESSOR) -mdfp="$(DFP_DIR)" \
           -Wl,--defsym=__MPLAB_BUILD=1,--defsym=_min_heap_size=0,--defsym=_min_stack_size=3072 \
           -Wl,--gc-sections,--no-code-in-dinit,--no-dinit-in-serial-mem \
           -Wl,--fatal-warnings \
           -Wl,-Map=$(TARGET).map,--memorysummary,$(BUILD_DIR)/memoryfile.xml

.PHONY: all clean fetch-dfp check-dfp test unit-test lint tidy cppcheck FORCE
# a .hex that failed normalization or the upgrade check must not look up to date
.DELETE_ON_ERROR:
all: $(TARGET).hex

PYTHON ?= python3

# The HEX must be installable with the in-firmware `upgrade` command, the only
# update path for devices without a programmer: normalize it to whole words
# (xc32-bin2hex ends records mid-word, which upgrade rejects) and check it
# against upgrade's rules and the pinned configuration words.
$(TARGET).hex: $(TARGET).elf tools/normalize-hex.py tools/check-upgrade-hex.py
	$(BIN2HEX) $<
	$(PYTHON) tools/normalize-hex.py $@ $@
	$(PYTHON) tools/check-upgrade-hex.py $@

# Command stamps: each holds the effective command line and is rewritten only
# when it changes. Objects depend on the compile command and the image on the
# link command (which includes the object list), so editing flags, overriding
# XC32_ROOT / DFP_DIR, or deleting or renaming a .c file rebuilds what it
# affects instead of leaving a stale or mixed-toolchain image.
COMPILE_CMD   := $(XC32_CC) $(CFLAGS) -MMD -MP -c
LINK_CMD      := $(XC32_CC) -o $(TARGET).elf $(OBJS) $(LDFLAGS)
COMPILE_STAMP := $(BUILD_DIR)/compile.cmd
LINK_STAMP    := $(BUILD_DIR)/link.cmd

define write-stamp
	@mkdir -p $(@D)
	@echo '$(strip $(1))' | cmp -s - $@ || echo '$(strip $(1))' > $@
endef

$(COMPILE_STAMP): FORCE
	$(call write-stamp,$(COMPILE_CMD))

$(LINK_STAMP): FORCE
	$(call write-stamp,$(LINK_CMD))

$(TARGET).elf: $(OBJS) $(LINK_STAMP)
	$(LINK_CMD)

$(BUILD_DIR)/obj/%.o: %.c $(COMPILE_STAMP) | check-dfp
	@mkdir -p $(@D)
	$(COMPILE_CMD) -o $@ $<

# XC32 >= 5.00 ships no device support of its own; it comes only from the
# pack, so check for the pack up front instead of failing in every compile.
# Runs after fetch-dfp when both goals are given, including under -j.
check-dfp: $(filter fetch-dfp,$(MAKECMDGOALS))
	@test -f "$(DFP_DIR)/Microchip.PIC32MK-GP_DFP.pdsc" || \
	  { echo "PIC32MK-GP_DFP $(DFP_VERSION) not found (DFP_DIR='$(DFP_DIR)'); run 'make fetch-dfp' or set DFP_DIR" >&2; exit 1; }

# Download and extract next to DFP_FETCH_DIR (same filesystem), then install
# with a single rename, so the pdsc at DFP_FETCH_DIR only ever appears with a
# complete pack. A leftover directory without it is replaced.
fetch-dfp:
	@if [ -f "$(DFP_FETCH_DIR)/Microchip.PIC32MK-GP_DFP.pdsc" ]; then \
	  echo "$(DFP_FETCH_DIR) already present"; \
	else \
	  set -e; mkdir -p "$(dir $(DFP_FETCH_DIR))"; \
	  tmp=$$(mktemp -d "$(dir $(DFP_FETCH_DIR)).fetch.XXXXXX"); \
	  trap 'rm -rf "$$tmp"' EXIT; trap 'rm -rf "$$tmp"; exit 1' INT TERM; \
	  curl -fsSLo "$$tmp/dfp.atpack" "$(DFP_URL)"; \
	  echo "$(DFP_SHA256)  $$tmp/dfp.atpack" | sha256sum -c -; \
	  unzip -q "$$tmp/dfp.atpack" -d "$$tmp/x"; \
	  test -f "$$tmp/x/Microchip.PIC32MK-GP_DFP.pdsc"; \
	  rm -rf "$(DFP_FETCH_DIR)"; mv -T "$$tmp/x" "$(DFP_FETCH_DIR)"; \
	fi

# ---- static analysis (make lint) -----------------------------------------

# clang-tidy (checks in .clang-tidy, plus clang's own warnings) and cppcheck,
# over the firmware sources as XC32 sees them: clang parses with XC32's and the
# pack's headers and XC32's target macros. Both tools are pinned PyPI wheels,
# run with uvx; override CLANG_TIDY / CPPCHECK to use other binaries.
CLANG_TIDY_VERSION := 23.1.4
# the cppcheck wheel 1.5.3 is Cppcheck 2.22.0
CPPCHECK_WHEEL     := 1.5.3
UVX        ?= uvx
CLANG_TIDY ?= $(UVX) --from clang-tidy==$(CLANG_TIDY_VERSION) clang-tidy
CPPCHECK   ?= $(UVX) --from cppcheck==$(CPPCHECK_WHEEL) cppcheck

LINT_DIR     := $(BUILD_DIR)/lint
XC32_DEFINES := $(LINT_DIR)/xc32-defines

# Warnings clang has beyond WARNINGS. XC32's #pragma config and attributes
# (ramfunc, unique_section) are unknown to clang; GCC checks those.
CLANG_WARNINGS := -Wall -Wextra -Wconversion -Wshadow-all -Wundef -Wcast-qual \
                  -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes \
                  -Wmissing-variable-declarations -Wconditional-uninitialized -Wcomma \
                  -Wunreachable-code-aggressive -Wunused-macros -Wextra-semi-stmt \
                  -Wloop-analysis -Wshift-sign-overflow -Wformat=2 -Wformat-signedness \
                  -Wno-unknown-pragmas -Wno-unknown-attributes

# Recursive (=): expanded only by the lint recipes, after $(XC32_DEFINES) is
# made, and only they need XC32.
LINT_CPPFLAGS = -Isrc -Isrc/app -Isrc/platform $(SODEBUG_FLAG) $(shell cat $(XC32_DEFINES))
LINT_CFLAGS   = --target=mipsel-unknown-none-elf -march=mips32r2 -msoft-float \
                -ffreestanding -nostdlibinc -std=gnu17 $(LINT_CPPFLAGS) \
                -isystem "$(DFP_DIR)/include" -isystem "$(XC32_ROOT)/pic32m/include" \
                -isystem "$(XC32_ROOT)/pic32m/include/pic32m-libs" \
                -isystem "$(XC32_ROOT)/pic32m/include/musl" \
                -idirafter "$(shell $(XC32_CC) -print-file-name=include)" \
                $(CLANG_WARNINGS)

lint: tidy cppcheck

# -D flags for the macros XC32 predefines for the device, the toolchain and the
# ABI (all numeric); clang defines its own for the MIPS target.
$(XC32_DEFINES): $(COMPILE_STAMP) | check-dfp
	@mkdir -p $(@D)
	$(XC32_CC) $(CFLAGS) -dM -E -x c /dev/null | \
	  grep -E '^#define (_*PIC32|_*XC|_*C32|_*LANGUAGE_C|__32MK|_MCHP_SZ|__mchp_|__mips_(micromips|dsp|no_float))' | \
	  sed -E 's/^#define ([A-Za-z0-9_]+) ([0-9]+)$$/-D\1=\2/' > $@
	@! grep -v '^-D' $@

# One clang-tidy per source, so -j parallelizes; never up to date.
tidy: $(SRCS:%.c=$(LINT_DIR)/%.tidy)

$(LINT_DIR)/%.tidy: %.c $(XC32_DEFINES) .clang-tidy
	$(CLANG_TIDY) --quiet $< -- $(LINT_CFLAGS)

cppcheck: $(XC32_DEFINES)
	@mkdir -p $(LINT_DIR)/cppcheck
	$(CPPCHECK) --quiet --error-exitcode=1 --inline-suppr --std=c17 --platform=mips32 \
	  --enable=warning,style,performance,portability --check-level=exhaustive \
	  --suppress=missingIncludeSystem --suppress=variableScope \
	  --cppcheck-build-dir=$(LINT_DIR)/cppcheck $(LINT_CPPFLAGS) $(SRCS)

# ---- host tests (no XC32 needed) -----------------------------------------

# GCC: WARNINGS has GCC-only options (-Wcast-align=strict, -Wduplicated-cond, ...)
HOST_CC     ?= gcc
HOST_DIR    := $(BUILD_DIR)/host
UNITY_DIR   := test/unity
# test/host first: its main.h stands in for the firmware's when a module
# under test includes "main.h"
HOST_CFLAGS := -std=gnu11 $(WARNINGS) -Itest/host -I$(UNITY_DIR) -Isrc/app -Isrc/platform
# Unity is vendored: built as upstream ships it, not held to WARNINGS
UNITY_CFLAGS := -std=gnu11 -Wall -Wextra -Werror -I$(UNITY_DIR)

# Firmware modules with no SFR access or I/O, compiled for the host. Tests link
# them from an archive, so each test pulls in only what it calls and provides
# fakes for what those modules call (e.g. flash_write_words(), usb_device_enqueue()).
HOST_SRCS := src/platform/params.c src/platform/cdcacm.c src/platform/terminal.c
HOST_OBJS := $(HOST_SRCS:%.c=$(HOST_DIR)/obj/%.o) $(HOST_DIR)/obj/unity.o
HOST_LIB  := $(HOST_DIR)/libhost.a

# Unity tests: each test/unit/test_*.c is one executable.
# One .c per compiler invocation, so each -MMD .d file covers its own source.
UNIT_TESTS := $(sort $(wildcard test/unit/test_*.c))
UNIT_BINS  := $(UNIT_TESTS:test/unit/%.c=$(HOST_DIR)/%)

$(HOST_DIR)/obj/unity.o: $(UNITY_DIR)/unity.c
	@mkdir -p $(@D)
	$(HOST_CC) $(UNITY_CFLAGS) -MMD -MP -c -o $@ $<

$(HOST_DIR)/obj/%.o: %.c
	@mkdir -p $(@D)
	$(HOST_CC) $(HOST_CFLAGS) -MMD -MP -c -o $@ $<

$(HOST_LIB): $(HOST_OBJS)
	rm -f $@
	$(AR) rcs $@ $^

$(HOST_DIR)/%: test/unit/%.c $(HOST_LIB)
	@mkdir -p $(@D)
	$(HOST_CC) $(HOST_CFLAGS) -MMD -MP -o $@ $< $(HOST_LIB)

# usb.c keeps buffer addresses in 32-bit ints (PIC32 physical addresses); a
# position-dependent executable has its static data below 2 GiB
$(HOST_DIR)/test_usb: private HOST_CFLAGS += -fno-pie -no-pie

test: unit-test

unit-test: $(UNIT_BINS)
	@set -e; for t in $^; do $$t; done

clean:
	rm -rf $(BUILD_DIR)

-include $(OBJS:.o=.d) $(UNIT_BINS:=.d) $(HOST_OBJS:.o=.d)
