# CI toolchain image (see .gitlab-ci.yml and .github/workflows/build.yml). Every
# input is pinned to a fixed version and checksum, so a rebuild cannot silently
# pick up a different one:
#   - base image by digest
#   - Arch packages from the Arch Linux Archive snapshot of 2025-09-08, whose
#     repo databases are pinned by sha256
#   - XC32 from Microchip's official installer, by URL and sha256
#   - the PIC32MK-GP device pack by URL and sha256
# Bump XC32_VERSION/DFP_VERSION together with pic32.X/nbproject/configurations.xml
# (languageToolchainVersion, PIC32MK-GP_DFP pack version) and
# pic32.X/nbproject/Makefile-local-MK_GPK.mk (MP_*_DIR, DFP_DIR).
# This image carries XC32 6.00, not the 4.35 of the image both CI files pin
# today (sha256:cdbbfd3b...), so its firmware differs from that image's. Test
# the built hex on hardware before switching CI to an image built from this file.
FROM archlinux:base-20250622.0.370030@sha256:a921b6864609280975d8269df323c2f4f478cdc9d0f5479e70c3fb5e710d1b11

ARG XC32_VERSION=6.00
ARG XC32_SHA256=18e28f71a2ce3ff4ced7fe76cef8bf22a8e65b968ed48481c296c82adffc806d
ARG DFP_VERSION=1.10.264
ARG DFP_SHA256=f8a5dfcc7ba39e928f0ef6a029930438434691574c0c7ed7fd2e0d2bf408094d
# DFP_DIR as baked into the generated Makefile-local-MK_GPK.mk
ARG DFP_DIR=/opt/microchip/mplabx/v6.25/packs/Microchip/PIC32MK-GP_DFP/${DFP_VERSION}

# Package signatures are not checked: some packager keys that signed the
# snapshot have since expired, so pacman rejects them. Integrity comes from the
# pinned database hashes instead; pacman checks every package against the
# sha256 recorded in those databases.
ARG ALA=https://archive.archlinux.org/repos/2025/09/08
RUN echo "Server = $ALA/\$repo/os/\$arch" > /etc/pacman.d/mirrorlist && \
    sed -i 's/^SigLevel .*/SigLevel = Never/' /etc/pacman.conf && \
    curl -fsSLo /var/lib/pacman/sync/core.db $ALA/core/os/x86_64/core.db && \
    curl -fsSLo /var/lib/pacman/sync/extra.db $ALA/extra/os/x86_64/extra.db && \
    printf '%s  %s\n' \
        0435a17092307a40b0ce1a3997dd2a36a4dfa7046d5c0d7bf0d0a2ba9d125f3e /var/lib/pacman/sync/core.db \
        704d3644e4a2738de7af6dadcea509161187ee6dbc1630c8a840028743e77bdc /var/lib/pacman/sync/extra.db \
        | sha256sum -c - && \
    pacman -Su --noconfirm && \
    pacman -S --noconfirm --needed make && \
    pacman -Scc --noconfirm

# Official Microchip installer; needs root, which the image build has.
# XC32 >= 6.00 has no license check, so -O3 needs no PRO key.
RUN curl -fsSL -o /tmp/xc32.run \
      "https://ww1.microchip.com/downloads/aemDocuments/documents/DEV/ProductDocuments/SoftwareTools/xc32-v${XC32_VERSION}-full-install-linux-x64-installer.run" && \
    echo "${XC32_SHA256}  /tmp/xc32.run" | sha256sum -c - && \
    chmod +x /tmp/xc32.run && \
    /tmp/xc32.run --mode unattended --unattendedmodeui none \
      --prefix /opt/microchip/xc32/v${XC32_VERSION} && \
    rm /tmp/xc32.run && \
    /opt/microchip/xc32/v${XC32_VERSION}/bin/xc32-gcc --version

# XC32 >= 5.00 ships no device support; it comes only from the DFP.
RUN curl -fsSL -o /tmp/dfp.atpack \
      "https://packs.download.microchip.com/Microchip.PIC32MK-GP_DFP.${DFP_VERSION}.atpack" && \
    echo "${DFP_SHA256}  /tmp/dfp.atpack" | sha256sum -c - && \
    mkdir -p "${DFP_DIR}" && \
    bsdtar -xf /tmp/dfp.atpack -C "${DFP_DIR}" && \
    rm /tmp/dfp.atpack && \
    test -f "${DFP_DIR}/xc32/32MK0512GPK064/p32MK0512GPK064.ld"
