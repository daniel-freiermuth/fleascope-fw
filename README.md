Flea-Scope™ is a very low-cost ($13 BoM) and easy-to-use 18 million sample-per-second (Msps) USB oscilloscope and 
mixed-signal logic analyzer with a built-in waveform generator, built on the StickOS® BASIC platform!  Flea-Scope
can be controlled by any Chromium-based web browser that supports WebUSB API or Web Serial API running on a computer,
tablet, or phone (!!!), with no need for further software install –- just plug it in and open a web-page and you are
up and running!

StickOS® BASIC is an entirely MCU-resident patented interactive programming environment, which includes an easy-to-use
editor, transparent line-by-line compiler, interactive debugger, performance profiler, and flash filesystem, all running
entirely within the MCU and controlled thru an interactive command-line user interface.  In StickOS, external MCU pins
may be bound to “pin variables” for manipulation or examination, and internal MCU peripherals may be managed by BASIC
control statements and interrupt handlers.

A StickOS-capable MCU can be controlled by any Chromium-based web browser that supports WebUSB API or Web Serial API
running on a computer, tablet, or phone (!!!), with no need for further software install -– just plug it in and open a
web-page and you are up and running!

See the Flea Scope™ User's Guide: [https://rtestardi.github.io/usbte/flea-scope.pdf](https://rtestardi.github.io/usbte/flea-scope.pdf)

See the Web USB Flea Scope™ GUI repository: [https://github.com/rtestardi/usbte](https://github.com/rtestardi/usbte)

See the StickOS® BASIC website: [https://rtestardi.github.io/StickOS/](https://rtestardi.github.io/StickOS/)

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
up every `.c` file under `cpustick/`, `sources/` and `stickos/`, linked in sorted order per directory, so adding a
source file needs no build edits.

`pic32.X/` is an MPLAB X *prebuilt* project for flashing and debugging: it loads `out/pic32.X.production.elf`
(run `make` first) and has no source list of its own.

## Testing

```sh
make test        # all host tests; needs only a host C compiler, not XC32
make unit-test   # Unity unit tests: test/unit/test_*.c
make golden-test # STICK_GUEST interpreter against test/basic.txt, test/basic3.txt
make help-check  # cpustick/help.c matches basic0.c (regenerate with make help)
```

The STICK_GUEST build (`make guest`, output in `out/guest/{debug,release}/stickos`) is the StickOS interpreter as a
Linux program, so the BASIC side can be tested without hardware. Development is on Linux; on Windows, use WSL.

Unit tests use [Unity](https://github.com/ThrowTheSwitch/Unity) (vendored in `test/unity/`, v2.7.0). Each
`test/unit/test_*.c` is built into its own executable and picked up automatically. Unit-testable code has no SFR
access or I/O; see `cpustick/fleascope-calc.h`.
