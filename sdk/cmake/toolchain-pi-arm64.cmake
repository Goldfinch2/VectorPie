# The Pi build: 64-bit ARM Linux, cross-compiled from WSL2 Debian 13 (trixie),
# the same release as the cabinet's Pi OS. The arm64 libraries come from
# Debian's multiarch packages (libsdl2-dev:arm64), so they are the cabinet's
# own versions. Used by the `pi` preset; tools/setup-wsl.sh installs it all.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CMAKE_LIBRARY_ARCHITECTURE aarch64-linux-gnu)

# pkg-config must see the arm64 .pc files only, never the PC's own.
set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
