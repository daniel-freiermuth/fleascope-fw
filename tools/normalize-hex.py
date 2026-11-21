#!/usr/bin/env python3
# SPDX-License-Identifier: Parity-7.0.0
"""Rewrites an Intel HEX file into whole, word-aligned 32-bit words.

The in-firmware `upgrade` command programs flash a word at a time and rejects
data records whose length is not a multiple of 4. xc32-bin2hex (XC32 6.00)
ends a section's record wherever the section ends, e.g. 11 bytes of string
constants, so its output would be refused. This keeps every byte, fills the
unused bytes of a partly used word with 0xFF (the erased flash value, which is
what that byte holds anyway), and writes 16-byte records with an :04 record
before each 64 KiB segment.

Usage: normalize-hex.py IN.hex OUT.hex   (IN and OUT may be the same file)
"""

import sys

RECORD_BYTES = 16


# Refuses anything upgrade would refuse for structure, so that normalizing
# cannot turn a broken file (e.g. a truncated one, which upgrade rejects for
# its missing :01 record) into one that passes check-upgrade-hex.py.
def read(path):
    image = {}
    extend = None
    done = False
    with open(path, newline="") as f:
        lines = f.read().splitlines()
    for number, line in enumerate(lines, 1):
        if not line.strip():
            continue
        if done:
            sys.exit(f"{path}:{number}: record after the :01 end-of-file record")
        try:
            record = bytes.fromhex(line[1:])
        except ValueError:
            record = b""
        if not line.startswith(":") or len(record) < 5 or len(record) != record[0] + 5 or sum(record) & 0xFF:
            sys.exit(f"{path}:{number}: malformed record")
        kind, data = record[3], record[4:-1]
        if kind == 0:
            if extend is None:
                sys.exit(f"{path}:{number}: data record before the first :04 record")
            address = extend | record[1] << 8 | record[2]
            for i, byte in enumerate(data):
                if image.get(address + i, byte) != byte:
                    sys.exit(f"{path}:{number}: conflicting data at 0x{address + i:08x}")
                image[address + i] = byte
        elif kind == 4 and len(data) == 2:
            extend = (data[0] << 8 | data[1]) << 16
        elif kind == 1 and not data:
            done = True
        elif kind != 5:  # start address; the firmware starts from the reset vector
            sys.exit(f"{path}:{number}: unsupported record type {kind:02x}")
    if not done:
        sys.exit(f"{path}: no :01 end-of-file record (truncated?)")
    return image


def record(kind, address, data):
    # lowercase, like xc32-bin2hex and the released StickOS files
    raw = bytes([len(data), address >> 8 & 0xFF, address & 0xFF, kind]) + data
    return ":" + (raw + bytes([-sum(raw) & 0xFF])).hex() + "\n"


def write(path, image):
    words = sorted({address & ~3 for address in image})
    lines = []
    segment = None
    i = 0
    while i < len(words):
        start = words[i]
        data = bytearray()
        # consecutive words in the same 64 KiB segment, up to RECORD_BYTES
        while (i < len(words) and words[i] == start + len(data)
               and len(data) < RECORD_BYTES and words[i] >> 16 == start >> 16):
            data += bytes(image.get(words[i] + j, 0xFF) for j in range(4))
            i += 1
        if start >> 16 != segment:
            segment = start >> 16
            lines.append(record(4, 0, bytes([segment >> 8, segment & 0xFF])))
        lines.append(record(0, start & 0xFFFF, bytes(data)))
    lines.append(record(1, 0, b""))
    with open(path, "w", newline="") as f:
        f.writelines(lines)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__.strip().splitlines()[-1])
    image = read(sys.argv[1])
    write(sys.argv[2], image)
    again = read(sys.argv[2])
    assert all(again[address] == byte for address, byte in image.items())
    assert all(byte == 0xFF for address, byte in again.items() if address not in image)


if __name__ == "__main__":
    main()
