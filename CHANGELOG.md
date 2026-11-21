# Changelog — Flea-Scope Firmware

All notable changes to the firmware are documented here.  

---

## [Unreleased]

### Breaking
- **The StickOS BASIC interpreter is removed.** The firmware is a plain command line
  (`cpustick/commands.c`) with `scope`, `stream`, `digital_stream`, `wave`, `ver`, `hostname`,
  `reset`, `upgrade`, `help`, `echo` and `prompt`, replying exactly as before. The BASIC
  statements hosts use for calibration — `dim <var>[[n]] as flash`, `print <var>, …`,
  `[let] <var> = <n>` — are emulated (`cpustick/flash-vars.c`) with the same flash layout:
  variables take the next parameter-page words in declaration order since reset, so stored
  calibrations are found again. Hostname and other parameters keep their flash locations
  (`sources/params.c`). Everything else BASIC (programs, `run`, `list`, pin variables,
  `connect`/`nodeid`, `uptime`, `help <topic>`, autorun) is gone; the welcome banner changed.
- **`scope -<n>` (magnification) is removed.** It repeated each sample n times on the device;
  the original web GUI uses it for its 2×/5×/10× time bases.
- Removed with BASIC: ZigFlea wireless support, the UART console, the I2C and QSPI drivers,
  the help-text compressor, the STICK_GUEST host build with its golden-output tests and
  `make guest`/`make help`, `code.txt`/`qa.txt` and the BASIC documents in `doc/`.
  `make test` now runs the Unity unit tests only, including new ones for the flash-variable
  emulation.

### Fixed
- **The firmware `.hex` can be installed with `upgrade` again.** Since the switch to XC32 6.00
  and the root `Makefile`, xc32-bin2hex output had data records of 2, 9 and 11 bytes, which
  `upgrade` rejects ("bad count"; it then aborts before writing anything). The build now
  normalizes the file to whole words and checks it (`tools/normalize-hex.py`,
  `tools/check-upgrade-hex.py`), including the configuration words.

### Added
- **Host interpreter tests in CI**: the STICK_GUEST interpreter now builds with GCC on
  Linux (`make guest`), and CI runs the golden-output suites `test/test.sh`
  and `test/test3.sh` plus a `help.c` staleness check on every pull request.
- **`make test`** runs all host tests (`unit-test`, `golden-test`, `help-check`); GitLab
  (`test:host`) and GitHub (`.github/workflows/test.yml`) both run exactly this. It replaces
  `test/testcalc.sh` and the `calc-tests`/`host-tests` workflows.
- **Unity** (v2.7.0, vendored in `test/unity/`) for host unit tests; the
  `fleascope-calc.h` test moved to `test/unit/test_fleascope_calc.c`.

### Changed
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
  instead of hand-written typedefs, so the sources compile as C17 and C23. The host
  interpreter is now built as C23 (`-std=gnu2x`) instead of
  `-std=gnu11`; the firmware keeps XC32's default (`gnu17`).
- **MPLAB X:** `pic32.X` is now a prebuilt project that loads `out/pic32.X.production.elf` for
  flashing and debugging. It has no source list to keep in sync, so `make check-mplabx` is removed.
- **STICK_GUEST build:** `make guest` builds the host interpreter into `out/guest/{debug,release}/`
  from the root `Makefile`, and `make help` regenerates `cpustick/help.c`. They replace `build.sh`,
  `build/` and `help.sh`, the multi-target build of the original StickOS; the golden tests take
  the interpreter from `$BASIC`. `stickos/w32main.c` is renamed `stickos/guest-main.c`.
- Only the PIC32MK0512GPK064 is supported: conditionals for the PIC32MK0512MCM064 and
  other parts are removed. Behavior is unchanged; the `.hex` differs only in the source
  line numbers that `ASSERT` embeds.

### Fixed
- Host interpreter: console input no longer uses `gets()` (removed from C11 and from the
  VS2015+ CRT); an overlong line is truncated instead of overflowing the buffer.
- Stream: with `bitwidth` below the sample's bit count, near-full-scale readings rounded up
  to `1 << bitwidth` and were sent as 0, with the carry bit leaking into the next sample.
  The rounded sample now saturates at `(1 << bitwidth) - 1`.
- `printf` with a conversion the firmware doesn't implement (e.g. `%i`) used a garbage
  length; it now prints nothing for it.

### Removed
- Unreferenced legacy build files: `build/Makefile.sourcery`, `cpustick/Makefile.cpustick`,
  `stickos/basic.vcproj`.
- Windows host support: the MSVC/Win32 code paths of the STICK_GUEST build,
  `build/Makefile.windows` and the Visual Studio solution. Windows users build in WSL.
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
