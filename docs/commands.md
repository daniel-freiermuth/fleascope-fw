# Flea-Scope command reference

The firmware is a line-oriented command line on a USB CDC ACM serial port
(`/dev/ttyACM*`, `COM*`; the baud rate setting is ignored). This page describes
every command of firmware 3.1.0 and its replies byte for byte, for people
writing host software and for use in a terminal.

## The command line

**Input.** A line ends with CR, LF, CRLF or LFCR and holds at most 71
characters. Commands and keywords are lower case. Numbers are decimal or
`0x` hex with lower-case digits; spaces and tabs separate arguments.

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

A typical host session (`echo off`, `prompt on`):

```
→ echo off
← > 
→ ver
← 3.1.0 (checksum 0x2e47)
← > 
```

## Capture and signal generation

### `scope <interval> [+|-|~](<level>|0x<bits>) [0x<mask>] [<delay>]`

Captures one block of 2000 samples of the analog input and the 9 digital
inputs, starting at the trigger, and prints them.

| Argument | Range | Default | Meaning |
|---|---|---|---|
| `interval` | 1–31457 | 10 | Sample period in units of 1/18 µs: 1 → 18 Msps, 10 → 1.8 Msps, 18000 → 1 ksps. Five ADCs sample interleaved. |
| `+`, `-`, `~` | | (none) | Trigger mode: `+` rising edge, `-` falling edge, `~` auto, none: level. |
| `level` | 0–1023 | | Analog trigger: comparator threshold on the analog input, in 1/1024 of the 3.3 V supply, i.e. a sample value / 4. |
| `0x<bits>` | 0x0–0x1ff | | Digital trigger instead: the pattern b8…b0 to wait for. A `0x` here selects the digital trigger. |
| `0x<mask>` | 0x0–0x1ff | all bits | Digital trigger only: which of b8…b0 the pattern compares. Accepted and ignored with an analog trigger. |
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

During a capture the general-purpose pins a0–a8 are driven low as outputs.
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

The firmware sends about 100 KB/s (measured with the defaults, which need
103 KB/s). It does not detect when it falls behind its sample buffer: at higher rates samples are overwritten
before they are sent, without any marker in the stream. Since the data is
binary, a host should find the end of the stream by the prompt that follows
its Ctrl-C, not by scanning for it while streaming.

### `digital_stream 0x<mask> [<us_per_sample> [<samples_per_packet>]]`

Streams the digital inputs selected by `mask` as raw binary packets until
Ctrl-C.

| Argument | Range | Default | Meaning |
|---|---|---|---|
| `mask` | 0x1–0x1ff | (required) | Channels b8…b0 to sample, N = number of set bits. Must be written in hex. |
| `us_per_sample` | 0–139808 | 0 | Sample period in µs; 0 samples as fast as the firmware can. |
| `samples_per_packet` | 1–⌊504/N⌋ | ⌊504/N⌋ | Samples per packet; the default fills one 63-byte USB packet. |

**Reply:** packets of ⌈`samples_per_packet`·N/8⌉ bytes, no header. Each
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
and restarts. A record it cannot accept ends the upgrade with a message
(`bad record`, `bad count`, `bad address`, `bad type`, `bad data`,
`bad checksum`, `duplicate address 0x…`), followed by `:04 record not found`
/ `:01 record not found` if those records were missing, and `upgrade failed`.
The old firmware then keeps running. If the running firmware is too large to
leave room for the staging area, `upgrade` only prints
`code exceeds half of flash`.

`upgrade` erases the parameter pages before it reads the file, so the
hostname and all flash variables (calibration) are reset, whether it
succeeds or fails.

## Flash variables (calibration)

Hosts store calibration in flash with three statements from StickOS BASIC,
which the firmware keeps for compatibility.

| Statement | Effect |
|---|---|
| `dim <name> as flash[, …]` | Declares a variable. |
| `dim <name>[<n>] as flash[, …]` | Declares an array of `n` elements. |
| `print <name>[[<i>]][, …]` | Prints the values separated by spaces, e.g. `1050 2000`. |
| `[let] <name>[[<i>]] = <value>[, …]` | Stores 32-bit signed values (decimal, `-`, or `0x` hex) in flash. |

Declarations are held in RAM until reset: each takes the next free words of
the parameter page in the order declared, and the names are not stored. A
host that declares the same variables in the same order after each reset
finds its values again. On the first start after a firmware update, and
after `upgrade`, all variables read 0.

Names are case-sensitive identifiers of up to 31 characters; there are up to
32 declarations and 511 words. A line is checked completely before any part
of it runs. Messages: `var '<name>' undefined`,
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
