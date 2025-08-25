# Changelog — Flea-Scope Firmware

All notable changes to the firmware are documented here.  

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
