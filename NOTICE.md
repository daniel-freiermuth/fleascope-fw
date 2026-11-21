# Licensing and provenance

## This firmware

Daniel Freiermuth's contributions to this repository are licensed under the
[Parity Public License 7.0.0](LICENSE.md): you may use and share them for free,
also commercially, but you have to share software that builds on them alike.

For use without the Parity obligations (for example in a closed-source product),
ask for a separate license via https://github.com/daniel-freiermuth.

## Origins

The firmware started as Rich Testardi's StickOS BASIC for the Flea-Scope™
(https://github.com/rtestardi/StickOS2). His original notice is kept in
[license.txt](license.txt). In short:

- The StickOS interpreter sources (`stickos/basic.c`, `code.c`, `parse.c`,
  `run.c`, `vars.c`), © CPUStick.com and covered by U.S. Patent 8,117,587, are
  no longer part of this firmware; they were removed in the commit
  "remove the StickOS BASIC interpreter".
- The other source files of that release are in the public domain. Their
  original versions remain public domain (see the upstream repository and this
  repository's history); the changes made here are covered by the license above.
- The board design files in `board/` are in the public domain.
- The prebuilt `board/*.hex` files contain StickOS and keep the license given
  for them in `license.txt` (personal, educational, or non-commercial use).

New files written for this firmware carry an `SPDX-License-Identifier:
Parity-7.0.0` line.
