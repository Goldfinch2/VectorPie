# The PC build: 64-bit Windows, cross-compiled from WSL2 Debian with MinGW-w64.
# Used by the `pc` preset (CMakePresets.json); tools/setup-wsl.sh installs the
# compiler (posix thread model) and SDL2's MinGW package in /opt/sdl2-mingw.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(MINGW_PREFIX x86_64-w64-mingw32)
set(CMAKE_C_COMPILER   ${MINGW_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_PREFIX}-g++)
set(CMAKE_RC_COMPILER  ${MINGW_PREFIX}-windres)

set(CMAKE_FIND_ROOT_PATH /usr/${MINGW_PREFIX})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
# SDL2's package lives outside the MinGW root (/opt/sdl2-mingw).
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
