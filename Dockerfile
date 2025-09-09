FROM archlinux:latest

# Install essential base packages and sudo
RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm base-devel git sudo rustup

# Create non-root user for building AUR packages
RUN useradd -m auruser && \
    echo "auruser ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

RUN pacman -Scc --noconfirm

# Switch to the new user
USER auruser
WORKDIR /home/auruser

RUN rustup default stable
# Clone and build paru from AUR
RUN git clone https://aur.archlinux.org/paru.git
RUN cd paru && makepkg -si --noconfirm
RUN rm -rf paru

RUN paru -S --noconfirm microchip-mplabxc32-bin
