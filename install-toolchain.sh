#!/bin/sh
# Installs the PIC32 firmware toolchain that pic32.X/nbproject/Makefile-local-MK_GPK.mk
# expects: XC32 from Microchip's official installer and the PIC32MK-GP device pack,
# both pinned by URL and sha256. Used by CI (.gitlab-ci.yml, .github/workflows/build.yml);
# needs root to write /opt, plus curl, unzip and sha256sum.
# Bump XC32_VERSION/DFP_VERSION together with pic32.X/nbproject/configurations.xml
# (languageToolchainVersion, PIC32MK-GP_DFP pack version) and Makefile-local-MK_GPK.mk
# (MP_*_DIR, DFP_DIR).
set -eu

XC32_VERSION=6.00
XC32_SHA256=18e28f71a2ce3ff4ced7fe76cef8bf22a8e65b968ed48481c296c82adffc806d
DFP_VERSION=1.10.264
DFP_SHA256=f8a5dfcc7ba39e928f0ef6a029930438434691574c0c7ed7fd2e0d2bf408094d

XC32_DIR=/opt/microchip/xc32/v$XC32_VERSION
DFP_DIR=/opt/microchip/mplabx/v6.25/packs/Microchip/PIC32MK-GP_DFP/$DFP_VERSION

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# XC32 >= 6.00 has no license check, so -O3 needs no PRO key.
curl -fsSL -o "$tmp/xc32.run" \
  "https://ww1.microchip.com/downloads/aemDocuments/documents/DEV/ProductDocuments/SoftwareTools/xc32-v$XC32_VERSION-full-install-linux-x64-installer.run"
echo "$XC32_SHA256  $tmp/xc32.run" | sha256sum -c -
chmod +x "$tmp/xc32.run"
"$tmp/xc32.run" --mode unattended --unattendedmodeui none --prefix "$XC32_DIR"
rm "$tmp/xc32.run"
"$XC32_DIR/bin/xc32-gcc" --version

# XC32 >= 5.00 ships no device support; it comes only from the DFP.
curl -fsSL -o "$tmp/dfp.atpack" \
  "https://packs.download.microchip.com/Microchip.PIC32MK-GP_DFP.$DFP_VERSION.atpack"
echo "$DFP_SHA256  $tmp/dfp.atpack" | sha256sum -c -
mkdir -p "$DFP_DIR"
unzip -q "$tmp/dfp.atpack" -d "$DFP_DIR"
test -f "$DFP_DIR/xc32/32MK0512GPK064/p32MK0512GPK064.ld"
