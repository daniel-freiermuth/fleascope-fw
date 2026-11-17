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

Requires MPLAB XC32 v6.00 (default location `/opt/microchip/xc32/v6.00`) and the PIC32MK-GP device pack 1.10.264.
`sudo sh install-toolchain.sh` installs both, pinned by sha256, as CI does.

```sh
make fetch-dfp   # once, if the pack is not installed: downloads it to ~/.mchp_packs (sha256-checked)
make            # -> out/pic32.X.production.hex (.elf, .map alongside)
```

Override `XC32_ROOT=` / `DFP_DIR=` if they are installed elsewhere; an MPLAB X install under
`/opt/microchip/mplabx/v*/packs` is found automatically. The root `Makefile` is the authoritative build and picks up
every `.c` file under `cpustick/`, `sources/` and `stickos/`, linked in sorted order per directory. `pic32.X/` remains
an MPLAB X project for flashing and debugging. New source files must also be added to
`pic32.X/nbproject/configurations.xml` at the same sorted position, so that the IDE links the same image as CI;
`make check-mplabx` (run in CI) fails if the two lists differ.
