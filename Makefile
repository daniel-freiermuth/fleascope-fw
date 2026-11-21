# Flea-Scope firmware build (PIC32MK0512GPK064, MPLAB XC32).
#
#   make                  build out/pic32.X.production.{elf,hex,map}
#   make fetch-dfp        download the pinned PIC32MK-GP device pack
#   make test             host tests: unit-test, golden-test, help-check
#   make guest            STICK_GUEST interpreter: out/guest/{debug,release}/stickos
#   make help             regenerate cpustick/help.c from basic0.c
#   make clean
#
# Override XC32_ROOT / DFP_DIR on the command line or in the environment if
# the toolchain or pack live elsewhere.
#
# Sources are discovered by wildcard, so a new .c file under cpustick/,
# sources/ or stickos/ is built without editing this file.

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

PROCESSOR := 32MK0512GPK064

# cpustick/help.c is generated and #included by cpustick/basic0.c.
# stickos/guest-main.c is the STICK_GUEST host entry point (make guest).
SRCS ?= $(filter-out cpustick/help.c,$(sort $(wildcard cpustick/*.c))) \
        $(sort $(wildcard sources/*.c)) \
        $(filter-out stickos/guest-main.c,$(sort $(wildcard stickos/*.c))) \
        main.c myplib.c

OBJS := $(SRCS:%.c=$(BUILD_DIR)/obj/%.o)

CFLAGS := -g -mprocessor=$(PROCESSOR) -mmicromips -mno-float \
          -O3 -funroll-loops -fomit-frame-pointer -fno-common -fno-builtin \
          -DSTICKOS -I. -Icpustick -Isources -Istickos \
          -Wall -Waddress-attribute-use -mdfp="$(DFP_DIR)"

LDFLAGS := -mprocessor=$(PROCESSOR) -mdfp="$(DFP_DIR)" \
           -Wl,--defsym=__MPLAB_BUILD=1,--defsym=_min_heap_size=0,--defsym=_min_stack_size=3072 \
           -Wl,--gc-sections,--no-code-in-dinit,--no-dinit-in-serial-mem \
           -Wl,-Map=$(TARGET).map,--memorysummary,$(BUILD_DIR)/memoryfile.xml

.PHONY: all clean fetch-dfp check-dfp test unit-test guest golden-test help help-check FORCE
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

# ---- host tests (no XC32 needed) -----------------------------------------

HOST_CC     ?= cc
HOST_DIR    := $(BUILD_DIR)/host
UNITY_DIR   := test/unity
HOST_CFLAGS := -std=gnu99 -Wall -Wextra -Werror -I$(UNITY_DIR) -Icpustick

# Unity tests: each test/unit/test_*.c is one executable, linked with Unity.
# One .c per compiler invocation, so each -MMD .d file covers its own source.
UNIT_TESTS := $(sort $(wildcard test/unit/test_*.c))
UNIT_BINS  := $(UNIT_TESTS:test/unit/%.c=$(HOST_DIR)/%)
UNITY_OBJ  := $(HOST_DIR)/unity.o

$(UNITY_OBJ): $(UNITY_DIR)/unity.c
	@mkdir -p $(@D)
	$(HOST_CC) $(HOST_CFLAGS) -MMD -MP -c -o $@ $<

$(HOST_DIR)/%: test/unit/%.c $(UNITY_OBJ)
	@mkdir -p $(@D)
	$(HOST_CC) $(HOST_CFLAGS) -MMD -MP -o $@ $< $(UNITY_OBJ)

test: unit-test golden-test help-check

unit-test: $(UNIT_BINS)
	@set -e; for t in $^; do $$t; done

# STICK_GUEST: the interpreter as a host program, for the golden tests and for
# generating cpustick/help.c. DEBUG (SODEBUG, asserts on) and RELEASE (-O).
GUEST_SRCS := stickos/guest-main.c \
              cpustick/basic0.c cpustick/parse2.c cpustick/run2.c \
              stickos/basic.c stickos/code.c stickos/parse.c stickos/run.c stickos/vars.c \
              sources/adc.c sources/led.c sources/pin.c sources/printf.c \
              sources/text.c sources/timer.c sources/util.c
GUEST_CFLAGS := -g -std=gnu2x -Werror -fno-builtin -DSTICK_GUEST -DSTICKOS \
                -I. -Icpustick -Isources -Istickos
GUEST_DIR    := $(BUILD_DIR)/guest
GUEST        := $(GUEST_DIR)/debug/stickos
GUEST_REL    := $(GUEST_DIR)/release/stickos
GUEST_DEBUG_OBJS   := $(GUEST_SRCS:%.c=$(GUEST_DIR)/debug/obj/%.o)
GUEST_RELEASE_OBJS := $(GUEST_SRCS:%.c=$(GUEST_DIR)/release/obj/%.o)

$(GUEST_DIR)/debug/obj/%.o: %.c
	@mkdir -p $(@D)
	$(HOST_CC) $(GUEST_CFLAGS) -DSODEBUG=1 -MMD -MP -c -o $@ $<

$(GUEST_DIR)/release/obj/%.o: %.c
	@mkdir -p $(@D)
	$(HOST_CC) $(GUEST_CFLAGS) -O -MMD -MP -c -o $@ $<

$(GUEST): $(GUEST_DEBUG_OBJS)
	$(HOST_CC) -o $@ $^

$(GUEST_REL): $(GUEST_RELEASE_OBJS)
	$(HOST_CC) -o $@ $^

guest: $(GUEST) $(GUEST_REL)

# Golden-output regression suites; test.sh / test3.sh -u re-bless them.
golden-test: guest
	cd test && BASIC=$(abspath $(GUEST)) sh test.sh && BASIC=$(abspath $(GUEST)) sh test3.sh && \
	  BASIC=$(abspath $(GUEST_REL)) sh test.sh && BASIC=$(abspath $(GUEST_REL)) sh test3.sh

# cpustick/help.c is generated from basic0.c by the interpreter.
help: $(GUEST)
	cd cpustick && $(abspath $(GUEST)) help <basic0.c >help.c

help-check: $(GUEST)
	@cd cpustick && $(abspath $(GUEST)) help <basic0.c | cmp -s - help.c || \
	  { echo "cpustick/help.c is stale; run 'make help' and commit it" >&2; exit 1; }

clean:
	rm -rf $(BUILD_DIR)

-include $(OBJS:.o=.d) $(UNIT_BINS:=.d) $(UNITY_OBJ:.o=.d) \
         $(GUEST_DEBUG_OBJS:.o=.d) $(GUEST_RELEASE_OBJS:.o=.d)
