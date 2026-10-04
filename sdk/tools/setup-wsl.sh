#!/usr/bin/env bash
# setup-wsl.sh — prepare WSL2 Debian 13 (trixie) to build VectorPie games.
#
# One install builds both versions of a game:
#   the PC version   a Windows .exe (MinGW-w64), run straight from this shell
#   the Pi version   a Linux arm64 executable, against the same Debian release
#                    and library versions as the cabinet's Pi OS
#
# Run it once, as your normal user (it uses sudo):  tools/setup-wsl.sh
# Running it again is harmless: every step checks before it acts.

set -euo pipefail

SDL2_VERSION=2.32.4            # the cabinet's libsdl2 version (Pi OS trixie)
SDL2_PREFIX=/opt/sdl2-mingw    # where the PC build's SDL2 goes (CMakePresets.json)

say() { printf '\n== %s\n' "$*"; }
die() { printf '\nsetup-wsl: %s\n' "$*" >&2; exit 1; }

# --- 1. the right Debian --------------------------------------------------------

[ -r /etc/os-release ] || die "no /etc/os-release: this is not a Debian system"
. /etc/os-release
if [ "${ID:-}" != "debian" ]; then
    die "this is ${PRETTY_NAME:-unknown}, not Debian.
Install Debian next to it, from Windows PowerShell:   wsl --install -d Debian
then run this script inside the Debian distribution."
fi
if [ "${VERSION_CODENAME:-}" != "trixie" ]; then
    die "this is ${PRETTY_NAME:-Debian}; the cabinet runs Debian 13 (trixie).
Upgrade this distribution first:
    sudo sed -i 's/bookworm/trixie/g' /etc/apt/sources.list /etc/apt/sources.list.d/* 2>/dev/null
    sudo apt update && sudo apt full-upgrade
then run this script again."
fi
say "Debian ${VERSION_ID:-13} (trixie): good"

# --- 2. packages ----------------------------------------------------------------

say "Adding the arm64 architecture (for the Pi build's libraries)"
if ! dpkg --print-foreign-architectures | grep -qx arm64; then
    sudo dpkg --add-architecture arm64
fi

say "Installing the compilers and libraries"
sudo apt-get update
sudo apt-get install -y \
    build-essential cmake ninja-build pkg-config git python3 \
    rsync openssh-client curl file \
    crossbuild-essential-arm64 libsdl2-dev:arm64 \
    mingw-w64

# --- 3. MinGW with the posix thread model ---------------------------------------
# Debian ships two MinGW variants; only "posix" has std::thread, which the SDK
# uses (audio streaming).

say "Selecting MinGW's posix thread model"
for tool in gcc g++; do
    alt=/usr/bin/x86_64-w64-mingw32-${tool}-posix
    [ -x "$alt" ] || die "$alt is missing (mingw-w64 did not install?)"
    sudo update-alternatives --set "x86_64-w64-mingw32-${tool}" "$alt"
done

# --- 4. SDL2 for the PC build ----------------------------------------------------
# Debian has no MinGW build of SDL2: use SDL's own, same version as the cabinet.

if [ -f "$SDL2_PREFIX/cmake/sdl2-config.cmake" ]; then
    say "SDL2 for MinGW already in $SDL2_PREFIX"
else
    say "Downloading SDL2 $SDL2_VERSION for MinGW into $SDL2_PREFIX"
    tmp=$(mktemp -d)
    trap 'rm -rf "$tmp"' EXIT
    url="https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/SDL2-devel-${SDL2_VERSION}-mingw.tar.gz"
    curl -fL --retry 3 -o "$tmp/sdl2.tar.gz" "$url" || die "download failed: $url"
    sudo mkdir -p "$SDL2_PREFIX"
    sudo tar -xzf "$tmp/sdl2.tar.gz" -C "$SDL2_PREFIX" --strip-components=1
    [ -f "$SDL2_PREFIX/cmake/sdl2-config.cmake" ] || die "unexpected SDL2 package layout in $SDL2_PREFIX"
fi

# --- 5. report --------------------------------------------------------------------

say "Ready"
printf '  PC compiler : %s\n' "$(x86_64-w64-mingw32-g++ --version | head -1)"
printf '  Pi compiler : %s\n' "$(aarch64-linux-gnu-g++ --version | head -1)"
printf '  Pi SDL2     : %s\n' "$(dpkg-query -W -f='${Version}' libsdl2-dev:arm64)"
printf '  PC SDL2     : %s (%s)\n' "$SDL2_VERSION" "$SDL2_PREFIX"
cat <<'EOF'

Next:
  cmake --preset pc && cmake --build --preset pc     # build-pc/poc.exe
  cmake --preset pi && cmake --build --preset pi     # build-pi/poc (arm64)

To deploy to the cabinet over SSH without a password, once:
  ssh-copy-id pi@<cabinet>
EOF
