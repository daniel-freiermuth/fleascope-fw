# Flea-Scope command reference

The firmware is a line-oriented command line on a USB CDC ACM serial port
(`/dev/ttyACM*`, `COM*`; the baud rate setting is ignored). This page describes
every command of the firmware after 3.1.0 without the StickOS interpreter
(not yet released; its `ver` still reports `3.1.0`, as did the 3.1.0 release
with the interpreter, so the two cannot be told apart by version) and its
replies byte for byte, for people
writing host software and for use in a terminal.

## The command line

**Input.** A line ends with CR, LF, CRLF or LFCR and holds at most 71
characters. Commands and keywords are lower case. Numbers are decimal or
`0x` hex with lower-case digits; spaces separate arguments. The line editor
drops tabs and other control characters, so a tab does not separate.

**Output.** Every line the firmware prints ends in CRLF. After each command
line it prints the prompt `> ` (no line end), or `done` + CRLF with
`prompt off`. After the first command line since reset, the welcome banner
(`help about`) comes before that prompt.

**Echo.** With `echo on` (the default) typed characters are echoed, with line
editing (arrow keys, Home/End, Delete, Backspace, Ctrl-U) and a history
(arrow up/down); the echo of a line comes before its output. Hosts usually
send `echo off` first, so replies contain only output.

**Ctrl-C** (`0x03`) stops a running `scope`, `stream` or `digital_stream`
and clears the line being typed. While one of these runs, everything else
the host sends is discarded.

**Errors.** A line that does not parse is answered with a caret under the
offending character, counted from the start of the line plus 2 for the prompt:

```
> scope -5
        ^ - error
```

If the caret would be in column 10 or later, the line starts with `error -`
and the caret follows in its column. Commands that parse but cannot run
print a message (below) instead.

A typical host session right after reset (echo is on until `echo off` has run,
and the first line since reset is followed by the banner):

```
→ echo off
← echo off
← Welcome to the Flea-Scope firmware v3.1.0!
← https://github.com/daniel-freiermuth/fleascope-fw
← > 
→ ver
← 3.1.0 (checksum 0x2e47)
← > 
```

Hosts that connect to a running device cannot know which of these come first,
so they usually send Ctrl-C and `echo off`, then read until the prompt.

## Capture and signal generation

### `scope <interval> [+|-|~](<level>|0x<bits>) [<mask> [<delay>]]`

Captures one block of 2000 samples of the analog input and the 9 digital
inputs, starting at the trigger, and prints them.

| Argument | Range | Default | Meaning |
|---|---|---|---|
| `interval` | 1–31457 | 10 | Sample period in units of 1/18 µs: 1 → 18 Msps, 10 → 1.8 Msps, 18000 → 1 ksps. Five ADCs sample interleaved. |
| `+`, `-`, `~` | | (none) | Trigger mode: `+` rising edge, `-` falling edge, `~` auto, none: level. |
| `level` | 0–1023 | | Analog trigger: comparator threshold on the analog input, in 1/1024 of the 3.3 V supply, i.e. a sample value / 4. |
| `0x<bits>` | 0x0–0x3ff | | Digital trigger instead: the pattern b8…b0 to wait for. A `0x` here selects the digital trigger. |
| `mask` | 0–1023 (0x3ff) | all bits | Digital trigger only: which of b8…b0 the pattern compares; bits above b8 are ignored. Any number in this position is the mask, decimal or hex, and it is required before a `delay`: with an analog trigger pass any value, e.g. `0`. |
| `delay` | 0–1000000 | 0 | Samples to skip after the trigger before the 2000 printed ones. |

Trigger modes:

| Mode | Analog | Digital |
|---|---|---|
| level (none) | waits until the input is above `level` | waits until the masked inputs equal `bits` |
| `+` | waits for the input to rise through `level` | waits for the masked inputs to start matching `bits` |
| `-` | waits for the input to fall through `level` | waits for the masked inputs to stop matching `bits` |
| `~` | like level, but captures anyway after a timeout of about 1,000,000 polls | same |

Level and edge triggers wait until they fire or Ctrl-C. Hosts always pass a
trigger; without one, the analog threshold is set to the top of its range.

**Reply:** 2000 lines `<analog>,<digital>`. `analog` is the raw 12-bit ADC
value in decimal (0–4095; the host converts it to volts with its
calibration, see below), `digital` the inputs b8…b0 as lower-case hex
without `0x` (`0`–`1ff`):

```
2149,1ff
2148,1ff
…
> 
```

After Ctrl-C nothing is printed but the prompt. If `interval` is above the
maximum the reply is `sample interval too large (max 31457)`.

`scope` (like `stream`) makes the general-purpose pins a0–a8 low outputs, and
they stay so after the command ends, until reset.
If pin s1 is held low, each of them is pulsed once after the trigger (a test
pattern for a loopback from a* to b*). Pin e3 is high while waiting for the
trigger, pin e2 high once triggered.

### `stream [<ticks> [<averaging> [<bitwidth>]]]`

Streams the analog input continuously as raw binary until Ctrl-C.

| Argument | Range | Default | Meaning |
|---|---|---|---|
| `ticks` | 2–65535 | 2333 | Output sample period in 120 MHz bus clock ticks: rate = 120 MHz / `ticks` (default ≈ 51.4 ksps). |
| `averaging` | 1–5 | 1 | Each period yields 5 interleaved ADC readings; the first `averaging` of them are summed into one output sample. |
| `bitwidth` | 1–16 | 16 | Bits per output sample. |

**Reply:** a raw byte stream without header or framing, then the prompt
after Ctrl-C.

- The sum of `averaging` 12-bit readings has 12 (1), 13 (2), 14 (3–4) or
  15 (5) significant bits. It is scaled to `bitwidth` bits: shifted right
  with rounding (half up, saturating at the largest `bitwidth`-bit value) or
  shifted left.
- Samples are packed back to back, LSB first: sample *n* occupies bits
  [*n*·`bitwidth`, (*n*+1)·`bitwidth`) of the stream, bit 0 of each byte
  first. With `bitwidth` 16 every sample is a little-endian 16-bit word.
- Bits that do not fill a byte when the stream stops are dropped.
- Like `scope`, `stream` leaves pins a0–a8 as low outputs until reset.

USB Full Speed bulk transfers carry at most 1216 kB/s by the specification
(19 packets of 64 bytes per 1 ms frame); measured with a Linux host, `stream`
gets 1152 kB/s (18 packets per frame). The data rate is
120 MHz / `ticks` × `bitwidth` / 8 bytes per second, so with `bitwidth` 16 it
fits that measurement for `ticks` ≥ 209 (574 ksps, 1148 kB/s) and with
`bitwidth` 8 for `ticks` ≥ 105 (1.14 Msps). `stream 200 1 16` needs 1200 kB/s
and delivered 1152 kB/s in that measurement, so it loses samples there; a
host that polls faster may keep up.
Above what the link carries, the firmware falls behind its sample buffer
without noticing: samples are overwritten before they are sent, with no
marker in the stream. Since the data is binary, a host should find the end of
the stream by the prompt that follows its Ctrl-C, not by scanning for it while
streaming.

### `digital_stream 0x<mask> [<us_per_sample> [<samples_per_packet>]]`

Streams the digital inputs selected by `mask` as raw binary packets until
Ctrl-C.

| Argument | Range | Default | Meaning |
|---|---|---|---|
| `mask` | 0x1–0x1ff | (required) | Channels b8…b0 to sample, N = number of set bits. Must be written in hex. |
| `us_per_sample` | 0–139808 | 0 | Sample period in µs; 0 samples as fast as the firmware can. |
| `samples_per_packet` | 1–⌊504/N⌋ | ⌊504/N⌋ | Samples per packet; a packet is at most 63 bytes, which the default fills. |

**Reply:** packets of ⌈`samples_per_packet`·N/8⌉ bytes, no header. They are
back to back in the byte stream and not aligned with USB packets (the firmware
fills each 64-byte USB packet completely), so a host counts bytes, also when
it reads raw bulk transfers in WinUSB mode. Each
sample is N bits, lowest selected channel first, packed LSB first and back
to back within the packet. When the stream stops, a partial last packet is
padded with zero bits to full size; then the prompt follows. Errors:
`us_per_sample too large (max 139808)`,
`n_samples_per_packet too large (max …)`.

### `wave (square|sine|triangle|ekg) <hz>`

Starts a waveform on the wave output and returns; it keeps running, also
during captures, until the next `wave` command or reset.

| Waveform | Frequency | Output |
|---|---|---|
| `square` | 10 Hz – 4 MHz | digital square wave |
| `sine`, `triangle`, `ekg` | 2 Hz – 40 kHz | 12-bit DAC (0–3.3 V), 100 points per period |

The command waits about 0.5 s before it starts the waveform. A frequency
above 10 MHz is answered with `bad frequency`; any other frequency outside the
range stops the current waveform without a message.

## Device

| Command | Reply |
|---|---|
| `ver` | `3.1.0 (checksum 0x2e47)`: firmware version and a checksum of the program flash, which identifies the exact build. |
| `hostname` | The device name, default `Flea-Scope`. |
| `hostname <name>` | Stores the name (up to 31 characters) in flash. It is also the USB product name, from the next connection on. |
| `reset` | Restarts the device; the serial port goes away and comes back. |
| `help` | A one-line summary per command. |
| `help about` | The welcome banner: version and project URL. |
| `echo [on\|off]`, `prompt [on\|off]` | Without an argument prints `on` or `off`. |

If pin a0 is held low while the device starts, it comes up as a vendor-class
(WinUSB) device instead of a serial port.

### `upgrade`

Replaces the firmware with an Intel HEX file pasted line by line, the
`out/pic32.X.production.hex` the build produces. For devices without a
programmer this is the only way to update; see the README for the checks the
build runs on the file.

```
> upgrade
paste HEX upgrade file now...
........
paste done!
programming flash for upgrade...
wait for heartbeat LED to blink!
```

After `paste HEX upgrade file now...` echo is off; the firmware prints a `.`
every 4 records, and after the end-of-file record it writes the new firmware
and restarts. Errors:

- A malformed record (`bad record`, `bad count`, `bad address`, `bad type`,
  `bad data`, `bad checksum`, `bad checksum 0x…`) ends the upgrade at once,
  so `:01 record not found` follows (and `:04 record not found` if no `:04`
  record had come yet). The rest of the pasted file then arrives at the
  command line, which answers each remaining line with an error caret; a host
  should stop sending.
- Exception: the end-of-file record counts as read before its checksum is
  checked. If only its checksum is missing or wrong, the device prints
  `bad checksum` (or `bad checksum 0x…`) and then installs the firmware as if
  the upgrade had succeeded (`paste done!` …), unless an address error came
  earlier. All data records have been checked by then.
- An address the upgrade cannot use (`duplicate address 0x…`,
  `bad address 0x…`) is reported and the paste goes on to the end-of-file
  record; then the upgrade fails.

`:04 record not found` / `:01 record not found` are printed whenever the
upgrade has not seen those records; then the device prints `upgrade failed`, switches echo on (whatever it was
before) and keeps running the old firmware. If the running firmware is too large to
leave room for the staging area, `upgrade` only prints
`code exceeds half of flash`.

`upgrade` erases the parameter pages before it reads the file, so the
flash variables (calibration) are reset to 0 and the hostname to `Flea-Scope`,
whether it succeeds or fails. After a failed `upgrade` the device keeps running but has
forgotten every `dim`: `print` and assignments report `var '…' undefined`
(see below for what follows the message) until the host declares its
variables again.

## Flash variables (calibration)

Hosts store calibration in flash with three statements from StickOS BASIC,
which the firmware keeps for compatibility.

| Statement | Effect |
|---|---|
| `dim <name> as flash[, …]` | Declares a variable. |
| `dim <name>[<n>] as flash[, …]` | Declares an array of `n` elements. |
| `print <name>[[<i>]][, …]` | Prints the values separated by spaces, e.g. `1050 2000`. As in StickOS, the line is cut after 78 characters without a marker. |
| `[let] <name>[[<i>]] = <value>[, …]` | Stores 32-bit signed values (decimal, `-`, or `0x` hex) in flash. |

Declarations are held in RAM until reset or a failed `upgrade`, which also
drops them: each takes the next free words of
the parameter page in the order declared, and the names are not stored. A
host that declares the same variables in the same order after each reset
finds its values again. On the first start after a firmware update, and
after `upgrade`, all variables read 0.

Names are case-sensitive identifiers of up to 31 characters; there are up to
32 declarations and 511 words. A line is checked completely before any part
of it runs. A failing item reports a message and the statement goes on, as in
StickOS: `print` sends each message on its own line first, then its value line,
in which a failed item without an index shows nothing and a failed indexed
item shows `0`; an assignment skips the failing item. For example, after
`dim a as flash, b as flash`:

```
> print nope
var 'nope' undefined

> print a, nope, b
var 'nope' undefined
0  0
> print nope[2]
var 'nope' undefined
0
```

Messages: `var '<name>' undefined`,
`var '<name>' index <i> out of range`,
`var '<name>' already declared at this scope`, `declared 0 length array`,
`out of variables`, `out of parameter flash`.

The original web GUI and Python client use this layout, which other hosts
follow to share calibration:

```
dim unused[8] as flash
dim cal_zero_x1 as flash, cal_3v3_x1 as flash
dim cal_zero_x10 as flash, cal_3v3_x10 as flash
```

`cal_zero_*` and `cal_3v3_*` hold the ADC readings for 0 V and 3.3 V in the
x1 and x10 probe settings; 0 means not calibrated.
