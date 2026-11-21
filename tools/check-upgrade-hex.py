#!/usr/bin/env python3
# SPDX-License-Identifier: Parity-7.0.0
"""Checks that a firmware HEX file can be installed with the in-firmware
`upgrade` command and is safe to install that way.

Devices without a programmer can only be updated through `upgrade`
(sources/flash.c: flash_upgrade()), and a bad image there cannot be undone.
This mirrors the rules flash_upgrade() applies while it parses the pasted
file, so a file that passes here is accepted by every firmware version with
that parser (StickOS 2.x/3.x and this firmware), and adds the checks the parser
cannot make:

- every line fits the 72-byte command line buffer the terminal collects;
- record types 00, 01, 04 only; an 04 before the first data record; data
  records hold whole, word-aligned 32-bit words;
- data only in program flash below FLASH_BYTES/2 - FLASH2_BYTES (the image
  must leave the upper half free for the next upgrade's staging area) or in
  boot flash; no address twice;
- the reset vector at the start of boot flash is present;
- the configuration words are exactly the pinned ones below. A wrong
  oscillator, USB PLL, or debug/protect setting can leave the device unable to
  enumerate, which without a programmer means bricked. Change PINNED_CONFIG only
  together with main.c's #pragma config lines, after testing on hardware with a
  programmer at hand.

Usage: check-upgrade-hex.py out/pic32.X.production.hex
"""

import sys

COMMAND_LINE_MAX = 72 - 1            # sources/terminal.c: command[BASIC_INPUT_LINE_SIZE]

FLASH_PA = 0x1D000000                # FLASH_START (0x9D000000) as a physical address
FLASH_BYTES = 512 * 1024
BOOT_PA = 0x1FC00000                 # FLASH2_START (0x9FC00000) physical
BOOT_BYTES = 20 * 1024               # FLASH2_BYTES
PROGRAM_END = FLASH_PA + FLASH_BYTES // 2 - BOOT_BYTES

# main.c #pragma config, as XC32 6.00 encodes it (DEVCFG3..0, DEVCP, SEQ)
PINNED_CONFIG = {
    0x1FC03FC0: 0x3F3FFFFF,
    0x1FC03FC4: 0x7FFAA728,
    0x1FC03FC8: 0x5F743EB9,
    0x1FC03FCC: 0xFFFFFFDB,
    0x1FC03FDC: 0xFFFFFFFF,
    0x1FC03FF0: 0xFFFF0000,
}
CONFIG_PA = (0x1FC03F00, 0x1FC04000)


def check(path):
    errors = []
    words = {}
    extend = None
    done = False

    with open(path, newline="") as f:
        lines = f.read().splitlines()

    for number, line in enumerate(lines, 1):
        def error(message):
            errors.append(f"{path}:{number}: {message}")

        if not line.strip():
            continue
        if done:
            error("record after the :01 end-of-file record")
            break
        if len(line) > COMMAND_LINE_MAX:
            error(f"line is {len(line)} characters; the terminal keeps only {COMMAND_LINE_MAX}")
        if not line.startswith(":"):
            error("bad record: no ':'")
            continue
        try:
            record = bytes.fromhex(line[1:])
        except ValueError:
            error("bad record: not hex")
            continue
        if len(record) < 5 or len(record) != record[0] + 5:
            error("bad count")
            continue
        if sum(record) & 0xFF:
            error("bad checksum")
            continue

        count, address, kind, data = record[0], record[1] << 8 | record[2], record[3], record[4:-1]
        if kind == 1 and count == 0:
            done = True
        elif kind == 4 and count == 2:
            extend = data[0] << 8 | data[1]
        elif kind == 0:
            if extend is None:
                error("data record before the first :04 record")
                continue
            if count % 4:
                error("data record count is not a multiple of 4")
                continue
            physical = ((extend << 16) | address) & 0x1FFFFFFF
            if physical % 4:
                error(f"data at 0x{physical:08x} is not word aligned")
            for i in range(0, count, 4):
                pa = physical + i
                if not (FLASH_PA <= pa < PROGRAM_END or BOOT_PA <= pa < BOOT_PA + BOOT_BYTES):
                    error(f"address 0x{pa:08x} outside the upgradable ranges")
                elif pa in words:
                    error(f"duplicate address 0x{pa:08x}")
                words[pa] = int.from_bytes(data[i:i + 4], "little")
        else:
            error(f"record type {kind:02x} with count {count} is not accepted by upgrade")

    if extend is None:
        errors.append(f"{path}: no :04 record")
    if not done:
        errors.append(f"{path}: no :01 end-of-file record")
    if BOOT_PA not in words:
        errors.append(f"{path}: no reset vector at 0x{BOOT_PA:08x}")

    config = {pa: value for pa, value in words.items() if CONFIG_PA[0] <= pa < CONFIG_PA[1]}
    if config != PINNED_CONFIG:
        for pa in sorted(set(config) | set(PINNED_CONFIG)):
            got, want = config.get(pa), PINNED_CONFIG.get(pa)
            if got != want:
                show = lambda v: "absent" if v is None else f"0x{v:08x}"
                errors.append(f"{path}: config word 0x{pa:08x} is {show(got)}, pinned {show(want)}")

    program = [pa for pa in words if pa < BOOT_PA]
    end = max(program) + 4 if program else FLASH_PA
    return errors, end


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__.strip().splitlines()[-1])
    errors, end = check(sys.argv[1])
    if errors:
        print("\n".join(errors), file=sys.stderr)
        sys.exit(f"{sys.argv[1]}: NOT safe to install with 'upgrade'")
    print(f"{sys.argv[1]}: upgrade-compatible; program flash used up to 0x{end:08x} "
          f"(limit 0x{PROGRAM_END:08x})")


if __name__ == "__main__":
    main()
