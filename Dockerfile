# CI toolchain image (see .gitlab-ci.yml and .github/workflows/build.yml). Every
# input is pinned to a fixed version and checksum, so a rebuild cannot silently
# pick up a different one:
#   - base image by digest
#   - Arch packages from the Arch Linux Archive snapshot of 2025-09-08, whose
#     repo databases are pinned by sha256
#   - AUR packages by git commit; each PKGBUILD verifies its own sources
#     (microchip-mplabxc32-bin checks the XC32 v4.35 installer's sha256)
#   - the PIC32MK-GP device pack by URL and sha256
# Not yet verified: that an image built from this file compiles firmware
# byte-identical to the published image both CI files pin (sha256:cdbbfd3b...).
# That image also carries a full MPLAB X 6.25 install, which this file replaces
# with the standalone device pack. Compare `xc32-gcc --version` and the built
# hex against that image before switching CI to a rebuilt image.
FROM archlinux:base-20250622.0.370030@sha256:a921b6864609280975d8269df323c2f4f478cdc9d0f5479e70c3fb5e710d1b11

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
    pacman -S --noconfirm --needed base-devel git sudo

# Create non-root user for building AUR packages
RUN useradd -m auruser && \
    echo "auruser ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

RUN pacman -Scc --noconfirm

# Switch to the new user
USER auruser
WORKDIR /home/auruser

# AUR packages in dependency order, each pinned to a commit of its AUR repo
RUN set -e; \
    for pkg in \
        ucl=05e860626c00a73a260d2499b2b846ffe1e4760b \
        tclkit=cc7d83c4a5bf790f9607b149c9cba0314d067a10 \
        sdx=9347f2e264aa297437b9d61303009d1d7ca881b6 \
        bitrock-unpacker=73eeb7199997117b522afa8e9b9b460b0cea5cbc \
        microchip-mplabxc32-bin=c618d981bf0f065f41a97ca4b974ecae3e54265d; \
    do \
        name=${pkg%%=*}; \
        git clone https://aur.archlinux.org/$name.git; \
        git -C $name checkout --detach ${pkg#*=}; \
        (cd $name && makepkg -si --noconfirm); \
        rm -rf $name; \
    done

# Device family pack, at the path pic32.X/nbproject/Makefile-local-MK_GPK.mk expects
RUN curl -fsSLo dfp.atpack https://packs.download.microchip.com/Microchip.PIC32MK-GP_DFP.1.8.253.atpack && \
    echo "fc93177e89006834954da5ad26b2850bd82e61fac64152df3aadc80b1d096cf5  dfp.atpack" | sha256sum -c - && \
    sudo mkdir -p /opt/microchip/mplabx/v6.25/packs/Microchip/PIC32MK-GP_DFP/1.8.253 && \
    sudo bsdtar -xf dfp.atpack -C /opt/microchip/mplabx/v6.25/packs/Microchip/PIC32MK-GP_DFP/1.8.253 && \
    rm dfp.atpack
