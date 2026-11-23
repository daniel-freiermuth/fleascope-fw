#!/usr/bin/env python3
# SPDX-License-Identifier: Parity-7.0.0
"""Prints the checksum that `ver` reports for a firmware image.

src/main.c sums the bytes of program flash from FLASH_START up to the
_on_bootstrap symbol into a 16-bit counter. This repeats that over the HEX
file (erased flash reads 0xFF) with the end address taken from the ELF's
symbol table.

Usage: flash-checksum.py out/pic32.X.production.hex out/pic32.X.production.elf
"""

import struct
import sys

FLASH_START = 0x9D000000             # src/config.h
FLASH_PA = FLASH_START & 0x1FFFFFFF


def symbol_address(elf_path, name):
    with open(elf_path, "rb") as f:
        elf = f.read()
    if elf[:4] != b"\x7fELF" or elf[4] != 1 or elf[5] != 1:
        sys.exit(f"{elf_path}: not a 32-bit little-endian ELF")
    shoff, = struct.unpack_from("<I", elf, 0x20)
    shentsize, shnum = struct.unpack_from("<HH", elf, 0x2E)
    sections = [struct.unpack_from("<IIIIIIIIII", elf, shoff + i * shentsize) for i in range(shnum)]
    for _, sh_type, _, _, off, size, link, _, _, entsize in sections:
        if sh_type != 2:                                         # SHT_SYMTAB
            continue
        stroff = sections[link][4]
        for pos in range(off, off + size, entsize):
            st_name, st_value = struct.unpack_from("<II", elf, pos)
            end = elf.index(b"\0", stroff + st_name)
            if elf[stroff + st_name:end].decode() == name:
                return st_value
    sys.exit(f"{elf_path}: no symbol {name}")


def flash_bytes(hex_path):
    data = {}
    upper = 0
    with open(hex_path) as f:
        for line in f:
            rec = bytes.fromhex(line.strip()[1:])
            count, addr, rtype = rec[0], (rec[1] << 8) | rec[2], rec[3]
            payload = rec[4:4 + count]
            if rtype == 0x04:
                upper = (payload[0] << 24) | (payload[1] << 16)
            elif rtype == 0x00:
                for i, b in enumerate(payload):
                    data[upper + addr + i] = b
    return data


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    end = symbol_address(sys.argv[2], "_on_bootstrap")
    if not FLASH_START < end <= FLASH_START + 512 * 1024:
        sys.exit(f"_on_bootstrap at 0x{end:08x} is outside program flash")
    data = flash_bytes(sys.argv[1])
    total = sum(data.get(pa, 0xFF) for pa in range(FLASH_PA, FLASH_PA + end - FLASH_START))
    print(f"0x{total & 0xFFFF:x}")


if __name__ == "__main__":
    main()
