# Changelog — Flea-Scope Firmware

All notable changes to the firmware are documented here.  

---

## [Unreleased]

### Changed
- **`wave` refuses out-of-range frequencies with `bad frequency`** and leaves the running
  waveform untouched; before, any nonzero frequency up to 10 MHz but outside the shape's range
  (square 10 Hz – 4 MHz, sine/triangle/ekg 2 Hz – 40 kHz) silently stopped the generator.
  `wave <shape> 0` still stops it.
- GitLab `test:host` runs on the `gcc:16` image (was `gcc:14`).
- **CI:** the GitHub `build`, `test` and `release` workflows run on `ubuntu-26.04` and
  GitLab's `build:pic32` on `ubuntu:26.04` (Resolute Raccoon), up from 24.04.

### Removed
- Unreachable pin modes in `src/platform/pin.c`: `pin_declare`, `pin_set` and `pin_get` handle
  only digital input and output (every caller passes one of those); the analog, servo,
  frequency and UART branches, the servo-rate and timer-sharing state and the unused
  `pin_type_names`, `pin_qual_names` and `pin_qual_mask` tables are gone. So is the code only
  those branches called: the `pin_uart_*` helpers and their state, `adc_get_value()` and the
  `pin_analog` level loaded from the analog parameter. Digital pin behaviour is unchanged.

---

## [4.0.0] — 2026-11-23

### Breaking
- **The StickOS BASIC interpreter is removed.** The firmware is a plain command line
  (`src/app/commands.c`) with `scope`, `stream`, `digital_stream`, `wave`, `ver`, `hostname`,
  `reset`, `upgrade`, `help`, `echo` and `prompt`, replying exactly as before. Hostname and
  other parameters keep their flash locations (`src/platform/params.c`). Everything else BASIC
  (programs, `run`, `list`, flash and pin variables, `connect`/`nodeid`, `uptime`,
  `help <topic>`, autorun) is gone; the welcome banner changed.
- **Probe calibration is no longer stored on the device.** The BASIC flash-variable statements
  hosts used for it — `dim <var>[[n]] as flash`, `print <var>, …`, `[let] <var> = <n>` — now
  get the syntax-error reply; hosts must keep calibration themselves. `upgrade` already erases
  the parameter pages, so calibration stored by earlier firmware is gone after installing this
  one; the first boot after a reflash no longer clears those words to 0. The original web GUI
  is affected: it reads calibration with `print` on connect and takes the error reply
  `  ^ - error` for values, which leaves its x10 calibration at NaN, so with the x10 probe
  setting all its analog captures are rejected; x1 keeps working with its default calibration.
- **`scope -<n>` (magnification) is removed.** It repeated each sample n times on the device;
  the original web GUI uses it for its 2×/5×/10× time bases.
- **`stream` takes arguments and its default output changed:**
  `stream [<ticks> [<averaging> [<bitwidth>]]]` (see [docs/commands.md](docs/commands.md)).
  The defaults (2333 ticks ≈ 51.4 ksps, averaging 1, 16 bits) keep the rate and the
  two-byte little-endian samples, but a plain `stream` now sends one 12-bit reading shifted
  left by 4 instead of the rounded mean of 4 of the 5 interleaved readings: values are 16×
  larger and unaveraged. Samples narrower than 8 bits are packed LSB-first across bytes.
- Removed with BASIC: ZigFlea wireless support, the UART console, the I2C and QSPI drivers,
  the help-text compressor, the STICK_GUEST host build with its golden-output tests and
  `make guest`/`make help`, `code.txt`/`qa.txt` and the BASIC documents in `doc/`.
  `make test` now runs the Unity unit tests only.

### Added
- **Command reference** ([docs/commands.md](docs/commands.md)): every command, its arguments
  and its replies byte for byte.
- **`make test`** runs the host unit tests; GitLab (`test:host`) and GitHub
  (`.github/workflows/test.yml`) both run exactly this. It replaces `test/testcalc.sh` and the
  `calc-tests`/`host-tests` workflows.
- **Unity** (v2.7.0, vendored in `test/unity/`) for host unit tests; the
  `fleascope-calc.h` test moved to `test/unit/test_fleascope_calc.c`.
- **Compile-time checks.** Firmware and test sources (not the vendored Unity) build with a
  strict GCC warning set and `-Werror` (`-Wextra -Wpedantic -Wconversion -Wshadow -Wcast-qual
  -Wcast-align=strict -Wstrict-prototypes -Wmissing-prototypes -Wformat=2`, …); the
  firmware adds GCC's static analyzer (`-fanalyzer`), `-Wframe-larger-than=512` and
  linker `--fatal-warnings`. The firmware's `printf` family has format attributes, so
  every format string is checked. `make lint` runs clang-tidy (`.clang-tidy`, with
  clang's own warnings) and cppcheck on the firmware sources with XC32's headers and
  macros, both pinned PyPI wheels run with `uvx`; both CIs run it after the build.
- **GitHub releases.** When `VERSION` in `src/main.h` changes on `main`, the `release`
  workflow builds and checks the firmware like CI, tags the commit `v<VERSION>` and publishes
  a release with `fleascope-fw-<VERSION>.hex` (plus `.elf`, `.map`, `SHA256SUMS`), the
  version's changelog section and the checksum `ver` reports. `tools/flash-checksum.py`
  computes that checksum from the `.hex` and `.elf`.

### Changed
- **Repository layout**: the firmware sources are in `src/` (`src/app/`, was `cpustick/`;
  `src/platform/`, was `sources/`), the board files in `hardware/` (was `board/`), the
  documents in `docs/` (was `doc/`). `python/scope.py`, `README.png` and `.nojekyll` are
  removed.
- Toolchain moved from XC32 4.35 to XC32 6.00 (GCC 13 base) and
  PIC32MK-GP_DFP 1.8.253 to 1.10.264. The old CI image used an AUR package
  that patched out XC32's license check to get `-O3`. XC32 6.00 provides
  `-O3` without a license, so CI no longer needs a prebuilt Docker image:
  `install-toolchain.sh` installs both from Microchip's official downloads,
  pinned by sha256, and both CI files run it on a stock Ubuntu 24.04.
  `Dockerfile` and the `compile-image` workflow are removed.
- **Build:** the firmware is built with a hand-written root `Makefile` (`make fetch-dfp && make`, output in
  `out/`) instead of the committed MPLAB X generated makefiles. Sources are picked up by wildcard. CI artifacts
  moved from `pic32.X/dist/MK_GPK/production/` to `out/`.
- MPLAB X generated files (`pic32.X/nbproject/Makefile-*.mk`, `pic32.X/.generated_files/`) are no longer committed.
- `main.h` takes `bool` and the fixed-width types from `<stdbool.h>`/`<stdint.h>`/`<stddef.h>`
  instead of hand-written typedefs, so the sources compile as C17 and C23; the firmware
  keeps XC32's default (`gnu17`).
- **MPLAB X:** `pic32.X` is now a prebuilt project that loads `out/pic32.X.production.elf` for
  flashing and debugging. It has no source list to keep in sync, so `make check-mplabx` is removed.
- Only the PIC32MK0512GPK064 is supported: conditionals for the PIC32MK0512MCM064 and
  other parts are removed. Behavior is unchanged; the `.hex` differs only in the source
  line numbers that `ASSERT` embeds.
- **`stream` is about 9× faster:** up to ~1.15 MB/s (USB full speed, 18 packets per 1 ms
  frame) instead of ~120 kB/s. It writes 64-byte chunks instead of one `printf` per byte and
  does no division or calls per sample. The USB serial port sends full 64-byte packets (ending
  a transfer that fills its last packet with a zero-length packet), keeps two IN packets armed
  (ping-pong) and polls for a free buffer instead of sleeping 1 ms. Replies are unchanged.

### Fixed
- **The firmware `.hex` can be installed with `upgrade` again.** Since the switch to XC32 6.00
  and the root `Makefile`, xc32-bin2hex output had data records of 2, 9 and 11 bytes, which
  `upgrade` rejects ("bad count"; it then aborts before writing anything). The build now
  normalizes the file to whole words and checks it (`tools/normalize-hex.py`,
  `tools/check-upgrade-hex.py`), including the configuration words.
- **`scope` with an analog trigger no longer hangs.** Since 2025-09 it stopped the timer
  interrupt before setting up the comparator, whose `delay()` waits on that interrupt, so the
  device stopped responding, Ctrl-C included, until replugged.
- **The device no longer comes up as WinUSB instead of a serial port after `scope` or
  `stream`.** Both drive pin a0 low; at the next reset or `upgrade` the boot check read a0
  before its pull-up had pulled it back up and chose the vendor-class (WinUSB) interface. The
  check now waits 1 ms after enabling the pull-up.
- `stream` stopped sending data after its sample counter overflowed (after ~4 minutes at
  70 ticks, ~2.3 hours at the default), and it ignored Ctrl-C while it was behind with sending.
- `digital_stream`: an `n_samples_per_packet` that does not fit one 63-byte packet overflowed
  a stack buffer; it is now rejected with the maximum in the reply. Periods above ~8.7 ms
  were silently clamped to 8.7 ms; Timer3's prescalers up to 1:256 now reach 139808 µs, and
  longer periods are rejected with that maximum instead of overflowing.
- `scope` with an interval above 31457 programmed a truncated Timer3 period and sampled at a
  rate unrelated to the request; such intervals are now rejected with the maximum.
- `scope` cleared pin e3 (waiting for trigger) only when stopped with Ctrl-C, not when the
  trigger fired.
- USB setup packet fields (`wValue`, `wIndex`, `wLength`) were read as signed 16-bit values,
  so values of 0x8000 and above were negative.
- The command-error caret could write past its buffer for long lines in release builds.
- Stream: with `bitwidth` below the sample's bit count, near-full-scale readings rounded up
  to `1 << bitwidth` and were sent as 0, with the carry bit leaking into the next sample.
  The rounded sample now saturates at `(1 << bitwidth) - 1`.
- `printf` with a conversion the firmware doesn't implement (e.g. `%i`) used a garbage
  length; it now prints nothing for it.
- `memcmp()` returned the wrong sign for byte differences beyond ±127.
- `upgrade`: a data record before the file's first extended address (`:04`) record was
  placed using an uninitialized upper address and could be installed at an arbitrary
  flash address. Such records now count as address 0 and are rejected. Files from the
  build start with a `:04` record and install as before.

### Removed
- Unreferenced legacy build files: `build/Makefile.sourcery`, `cpustick/Makefile.cpustick`,
  `stickos/basic.vcproj`.
- `build.sh`, `build/` (including `build/Makefile.windows`) and the Visual Studio solution,
  the multi-target builds of the original StickOS; the root `Makefile` replaces them. Windows
  users build in WSL.
- `readme.txt` (CodeWarrior/MPLAB 8 project list) and `review.sh` (Cygwin patch review).

---

## [3.1.0] — 2026-06-23

### Added
- **Digital stream command** (`digital_stream 0x<mask> <us_per_sample> <n_samples_per_packet>`):
  streams continuous digital samples over USB as compact binary packets.
  - Samples up to 9 virtual channels (b0–b8), mapped from PORTB pins rb4–rb6,
    rb10–rb15 (same physical-to-virtual mapping as `scope`).
  - Samples are packed LSB-first, lowest virtual channel first, into
    `ceil(n_samples_per_packet × N / 8)` bytes per packet — no inline header.
  - Host chooses `n_samples_per_packet` to balance throughput vs. latency
    (max: `floor(63 × 8 / N)` samples fills one USB FS bulk packet).
  - Uses a timer for accurate per-sample timing; does not require DMA.

---

## [3.0.0] — 2025-08-25

First git-tracked release, picking up where the hand-tracked 2.27k left off.

### Added
- **Analog stream command** (`stream`): continuous ADC capture streamed over USB.
  - Up to 18 Msps using 5× interleaved ADC DMA on ra0, ra1, rb0, rb1, ra11.
  - Configurable sample interval (1–20 000), trigger level/slew, delay samples,
    and magnification (−10×, −5×, −2×, 1×, …).
  - RC10 used as waveform output (CDAC2 or OC11).
  - Data transmitted little-endian.
- **CI pipeline**: GitLab CI build added (`.gitlab-ci.yml`, Dockerfile,
  `build-distrobox.sh`).
- **Wiring diagram**: `fleascope.drawio` added to document board wiring.
- **`wave` promoted from command to statement**: `wave` can now appear anywhere
  a statement is valid (previously command-only); parser and runner updated
  accordingly. Tests added to `test/basic3.sh`.
- `Versions` file added to track the hand-release history.

### Changed
- Fleascope code split into HAL (`fleascope-hal.c/.h`) and per-feature command
  files (`scope-command.c/.h`, `stream-command.c/.h`) for better readability.
- XC32 compiler parameters tuned.
- Digital stream no longer uses DMA channels (timer-only path is sufficient).
- Chunked USB sending reverted; little-endian wire encoding retained.

### Fixed
- Stream: double-increment of sample index.
- Stream: redundant `\n` removed from packet output.
- Stream: reset-once semantics — hardware state reset only once per command
  invocation, not per packet.
- Stream: integer rounding applied to sample-interval calculation.

---

## [2.27k] — July 2024

*(No detailed change notes; listed in `Versions` file.)*

---

## [2.27j] — May 2024 — checksum `0x39ca`

### Fixed
- **Bug #5**: Some Samsung phones claim Flea-Scope via the serial driver but do
  not expose Web Serial, leaving the device inaccessible. Addressed in USB
  descriptor / enumeration path.

### Added
- USB serial number embedded in device descriptor.

### Notes
- **Do not use pin a0 during boot** except to force a non-CDC/ACM USB endpoint.

---

## [2.27h] — January 2024 — checksum `0x990f`

### Fixed
- **Bug #2**: UART read/write of a BASIC array variable garbled or duplicated
  data.
- **Bug #3**: Weak pull-up resistors (7 kΩ–22 kΩ) were enabled by default on
  logic-analyzer inputs, interfering with I²C bus signals.

### Board
- Experimental QFP board revision (unintentional solder paste under QFP pad —
  see 2.27i board errata).

---

## [2.27g] — January 2024 — checksum `0x7c8e` — **production release**

### Changed
- Release preparation and final QA pass.

### Board
- Elecrow layout: BNC and USB connector positions updated (USB change later
  reverted on subsequent board spins).

---

## [2.27f] — January 2024 — checksum `0x55c1`

### Added
- `ver` command: prints firmware version string and flash checksum
  (e.g. `2.27f (checksum 0x55c1)`).

### Board
- Minewing production run with ENIG surface finish and adjusted pad spacing.

---

## [2.27e] — January 2024 — checksum `0x2a37`

### Fixed
- Pull-ups disabled on analog pins a0–a8; the enabled pull-ups were
  incompatible with ADC scanning and produced incorrect readings.

---

## [2.27d] — December 2023 — checksum `0x423c`

### Changed
- ADC switched from 10-bit to **12-bit** conversion mode.
- Flash calibration reference values scaled up by 4× to match the new ADC
  resolution.

---

## [2.27c] — November 2023 — checksum `0x50c0`

### Fixed
- JTAG controller was stealing pin E3/TDO after a flash upgrade, corrupting
  subsequent GPIO use of that pin.

---

## [2.27b] — November 2023 — checksum `0x45bf`

### Fixed
- Flash upgrade procedure (previously unreliable).

### Added
- UART pin variables: pins can now be declared as `uart input` / `uart output`
  in BASIC `dim` statements.

---

## [2.27a] — November 2023 — checksum `0x4eec`

### Added
- Flash checksum computed on boot and exposed via the `ver` command (landed
  fully in 2.27f, infrastructure added here).

---

## [2.27] — initial release — checksum `0x0`

First firmware release for the Flea-Scope hardware (PIC32MK0512GPK064 /
PIC32MK0512MCM064). Implements the StickOS BASIC interpreter over USB CDC/ACM,
analog oscilloscope capture (`scope` command), and basic GPIO/UART/I²C/SPI/PWM
pin control.
