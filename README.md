Flea-Scope™ is a very low-cost ($13 BoM) and easy-to-use 18 million sample-per-second (Msps) USB oscilloscope and
mixed-signal logic analyzer with a built-in waveform generator. This is its firmware.

The firmware started as Rich Testardi's StickOS BASIC port to the Flea-Scope. It no longer contains the
BASIC interpreter: it is a plain command line over USB (CDC ACM) with the commands hosts use — `scope`, `stream`,
`digital_stream`, `wave`, `ver`, `hostname`, `reset`, `upgrade`, `echo`, `prompt` — plus the BASIC flash-variable
statements (`dim … as flash`, `print`, assignment) that hosts use to keep probe calibration in flash. Replies are
unchanged, so existing hosts such as [fleascope-rs](https://github.com/daniel-freiermuth/fleascope-rs) keep working.

See the Flea Scope™ User's Guide: [https://rtestardi.github.io/usbte/flea-scope.pdf](https://rtestardi.github.io/usbte/flea-scope.pdf)

See the Web USB Flea Scope™ GUI repository: [https://github.com/rtestardi/usbte](https://github.com/rtestardi/usbte)
(its 2×/5×/10× zoom time bases need the `scope -<n>` magnification this firmware no longer has).

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
up every `.c` file under `cpustick/` and `sources/`, linked in sorted order per directory, so adding a source file needs
no build edits.

`pic32.X/` is an MPLAB X *prebuilt* project for flashing and debugging: it loads `out/pic32.X.production.elf`
(run `make` first) and has no source list of its own.

## Testing

```sh
make test        # Unity unit tests (test/unit/test_*.c); needs only a host C compiler, not XC32
```

Unit tests use [Unity](https://github.com/ThrowTheSwitch/Unity) (vendored in `test/unity/`, v2.7.0). Each
`test/unit/test_*.c` is built into its own executable and picked up automatically. Firmware modules without SFR
access or I/O (`cpustick/fleascope-calc.h`, `cpustick/flash-vars.c`) are compiled for the host and linked into the
tests from an archive; a test provides fakes for what they call. Development is on Linux; on Windows, use WSL.
