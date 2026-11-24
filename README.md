Flea-Scope™ is a very low-cost ($13 BoM) and easy-to-use 18 million sample-per-second (Msps) USB oscilloscope and
mixed-signal logic analyzer with a built-in waveform generator. This is its firmware.

The firmware started as Rich Testardi's StickOS BASIC port to the Flea-Scope. It no longer contains the
BASIC interpreter: it is a plain command line over USB (CDC ACM) with the commands hosts use — `scope`, `stream`,
`digital_stream`, `wave`, `ver`, `hostname`, `reset`, `upgrade`, `echo`, `prompt`. Their replies are unchanged. Probe
calibration is no longer stored on the device: the BASIC flash-variable statements (`dim … as flash`, `print`,
assignment) that hosts such as [fleascope-rs](https://github.com/daniel-freiermuth/fleascope-rs) used for that are
gone, so hosts keep calibration themselves.
Every command and its replies are described in [docs/commands.md](docs/commands.md).

See the Flea Scope™ User's Guide: [https://rtestardi.github.io/usbte/flea-scope.pdf](https://rtestardi.github.io/usbte/flea-scope.pdf)

See the Web USB Flea Scope™ GUI repository: [https://github.com/rtestardi/usbte](https://github.com/rtestardi/usbte)
(its 2×/5×/10× zoom time bases need the `scope -<n>` magnification this firmware no longer has, and with the x10
probe setting its analog captures fail, since it reads calibration from the device on connect).

## Repository layout

| Path | Contents |
|---|---|
| `src/` | `main.c` (startup, clocks, `#pragma config`), `config.h`, `main.h` |
| `src/app/` | The commands: command line, `scope`, `stream`, `digital_stream`, `wave` |
| `src/platform/` | USB and CDC ACM, terminal, `printf`, flash and `upgrade`, parameter pages, ADC, timers, pins |
| `test/` | Unity unit tests (`test/unit/`), the host stand-in for `main.h` (`test/host/`), Unity (`test/unity/`) |
| `tools/` | HEX normalization and the `upgrade` compatibility check, run by the build |
| `pic32.X/` | MPLAB X prebuilt project for flashing and debugging |
| `docs/` | [Command reference](docs/commands.md); Rich Testardi's guides and articles (`.docx`) and release notes (`history/`) |
| `hardware/` | Board design files, wiring diagram, prebuilt StickOS `.hex` files |

## Building the firmware

Requires MPLAB XC32 v6.00 (default location `/opt/microchip/xc32/v6.00`), the PIC32MK-GP device pack 1.10.264, and
Python 3. `sudo sh install-toolchain.sh` installs XC32 and the pack, pinned by sha256, as CI does.

```sh
make fetch-dfp   # once, if the pack is not installed: downloads it to ~/.mchp_packs (sha256-checked)
make            # -> out/pic32.X.production.hex (.elf, .map alongside)
```

The `.hex` is what the device's `upgrade` command installs (paste it into the terminal after `upgrade`); for devices
without a programmer that is the only way to update. The build rewrites xc32-bin2hex's output into whole 32-bit words
(`tools/normalize-hex.py`), since `upgrade` rejects anything else, and fails unless the file passes
`tools/check-upgrade-hex.py`: upgrade's record and address rules, room left for the next upgrade's staging area, and
the pinned configuration words (a wrong oscillator or USB setting would leave the device unreachable over USB).

Override `XC32_ROOT=` / `DFP_DIR=` if they are installed elsewhere; an MPLAB X install under
`/opt/microchip/mplabx/v*/packs` is found automatically. The root `Makefile` is the only build description and picks
up every `.c` file under `src/app/` and `src/platform/`, linked in sorted order per directory, so adding a source file needs
no build edits.

`pic32.X/` is an MPLAB X *prebuilt* project for flashing and debugging: it loads `out/pic32.X.production.elf`
(run `make` first) and has no source list of its own.

## Testing

```sh
make test        # Unity unit tests (test/unit/test_*.c); needs only host GCC (HOST_CC=), not XC32
```

Unit tests use [Unity](https://github.com/ThrowTheSwitch/Unity) (vendored in `test/unity/`, v2.7.0). Each
`test/unit/test_*.c` is built into its own executable and picked up automatically. Firmware modules without SFR
access or I/O (`src/app/fleascope-calc.h`, `src/platform/params.c`, `src/platform/cdcacm.c`) are compiled for the
host and linked into the tests from an archive; a test provides fakes for what they call (e.g. a simulated flash,
the usb driver), and `test/host/main.h` stands in for the firmware's `main.h`. `test/unit/test_flash_upgrade.c`
instead includes `src/platform/flash.c` with the registers it uses simulated, and maps program flash at
`FLASH_START` and its KSEG1 alias (Linux only). Development is on Linux; on Windows, use WSL.

## Static checks

Firmware and test sources (not the vendored Unity) are compiled with a strict warning set and `-Werror` (`WARNINGS` in
the `Makefile`), among others `-Wextra -Wpedantic -Wconversion` (implicit narrowing and sign changes), `-Wshadow`,
`-Wcast-qual`, `-Wcast-align=strict`, `-Wstrict-prototypes`, `-Wmissing-prototypes`, `-Wformat=2 -Wformat-signedness`
(the firmware's own `printf` family carries format attributes) and `-Wswitch-enum`. The firmware adds GCC's static
analyzer (`-fanalyzer`) and `-Wframe-larger-than=512` (no stack frame over 512 bytes of the 3 KiB stack; `-Wvla` and
`-Walloca` keep frames fixed-size). The linker runs with `--fatal-warnings`.

```sh
make lint        # clang-tidy (.clang-tidy, plus clang's own warnings) and cppcheck; needs XC32 and uv
```

`make lint` checks the firmware sources as XC32 compiles them: clang parses them with XC32's and the device pack's
headers, and both tools get XC32's target macros. Both are pinned PyPI wheels (`CLANG_TIDY_VERSION`, `CPPCHECK_WHEEL`)
run with [`uvx`](https://docs.astral.sh/uv/); `CLANG_TIDY=` / `CPPCHECK=` select other binaries. CI runs it after the
firmware build. Findings are fixed, not silenced; the few deliberate exceptions are suppressed at the line with the
reason (`NOLINT(check): …`, `cppcheck-suppress id ; …`, `#pragma GCC diagnostic`).

## License

[Parity Public License 7.0.0](LICENSE.md): free to use and share, also commercially, as long as software built on it
is shared alike. For other terms, contact [daniel-freiermuth](https://github.com/daniel-freiermuth). The code's origins
in StickOS and the public-domain parts are described in [NOTICE.md](NOTICE.md).
