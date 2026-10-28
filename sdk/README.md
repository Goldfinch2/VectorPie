# VectorPie SDK

Write vector games for **VectorPie**: a Raspberry Pi driving a real vector
monitor through a **USB-DVG** (or the coming **BeamForge**) board. Develop
and test on your PC, in a window; run the same code on the cabinet.

> Status: **0.9.0, in development**: the device layer (frames to the board or
> to a window), the three drawing calls, the advmame.rc reader, the game loop,
> the controls, text (three fonts, one of them fat comic capitals), sound,
> 3D (solid with hidden lines, and wireframe), physics (rigid bodies, joints,
> rays), high scores, credits and START lamps, game packages, `vp-test`, and
> five examples (`poc`, `hello`, `models`, `lamps`, `physics`).

## Set up (once)

Everything is built in **WSL2 running Debian 13 (trixie)**, the same release
as the cabinet's Pi OS. From Windows PowerShell:

```powershell
wsl --install -d Debian
```

Then, inside Debian, in this directory:

```sh
cat /etc/debian_version        # 13.x expected
bash tools/setup-wsl.sh
```

`setup-wsl.sh` installs the Windows cross compiler (MinGW-w64), the Pi cross
compiler (arm64) with the Pi's own library versions, CMake and Ninja, and
SDL2 for the PC build (`/opt/sdl2-mingw`). Keep your projects in the Debian
file system (`~/`) rather than under `/mnt/c`: builds are much faster there.

## Build

```sh
cmake --preset pc && cmake --build --preset pc     # Windows: build-pc/poc.exe
cmake --preset pi && cmake --build --preset pi     # Pi:      build-pi/poc
```

The same source gives both. `SDL2.dll` is copied next to the `.exe`; nothing
else needs to travel with it. The library itself is `libvectorpie.a`: a game
links it into its own executable.

## Install, and use it from a game

`cmake --install` puts the built SDK in one folder per platform (the
presets' install prefix):

```sh
cmake --install build-pc        # ~/VectorPie/dist/pc
cmake --install build-pi        # ~/VectorPie/dist/pi
```

Each holds `lib/libvectorpie.a`, `include/vectorpie/`, a CMake package
(`lib/cmake/VectorPie/`, with the toolchain files), a pkg-config file,
`bin/vp-pack` and the docs. A game uses it with:

```cmake
find_package(VectorPie 0.1 REQUIRED CONFIG)    # CMAKE_PREFIX_PATH = the platform's folder
add_executable(mygame src/main.c)
vp_program(mygame)                             # links VectorPie::vectorpie (+ SDL2, threads)
# the package: sh ${VECTORPIE_PACK} <staged folder> <output dir>
```

with presets setting `CMAKE_PREFIX_PATH` to `~/VectorPie/dist/pc` or
`/pi`, and the toolchain file from `lib/cmake/VectorPie/`. The library is
static: a game's executable carries its SDK, and a cabinet needs nothing
installed besides SDL2. The `0.1` asks for 0.1 or newer: within 0.x the
SDK only adds, so a game asking for an older 0.x builds with a newer one
(1.0 may change the API).

GLtron, Neverball and Torus Trooper are built this way: after changing the
SDK, install it again, then rebuild the game. (A game can also build the SDK
from its source tree, `add_subdirectory(<sdk> vectorpie-sdk)` and the same
`vp_program`.)

## Try it

On the PC (from the Debian shell; the `.exe` runs in Windows):

```sh
build-pc/poc.exe --windowed          # a turning cube in a window
build-pc/poc.exe --dvg COM5          # on a USB-DVG plugged into the PC
build-pc/hello.exe --windowed        # a small game: arrows, Left Ctrl, 1 to start, Esc
build-pc/models.exe --windowed       # 3D wireframe; add .obj files to look at them
build-pc/lamps.exe --windowed        # START lamps: patterns, coins and credits (lamps light on the Pi)
build-pc/physics.exe --windowed      # physics: crates, a seesaw, a chain; buttons 1 and 2 drop more
build-pc/vp-test.exe --windowed      # the test pattern
```

On the cabinet: copy `build-pi/poc` over (`scp build-pi/poc pi@<cabinet>:`),
quit the menu (only one program can hold the board), then:

```sh
./poc                     # the board, if advmame.rc enables the DVG
VP_OUTPUT=sdl ./poc       # the HDMI screen
```

Ctrl+C or closing the window stops it; the board returns to its home screen.
`file build-pi/poc` should say `ARM aarch64`.

## Where a frame goes

| | |
|---|---|
| `VP_OUTPUT=dvg` | the board |
| `VP_OUTPUT=sdl` | a fullscreen window (on the Pi, the HDMI screen) |
| otherwise, advmame.rc `vector_aux_renderer dvg` | the board |
| otherwise | a fullscreen window (the DVG is disabled, or this is a PC) |

The board's port: `VP_DVG_PORT`, else advmame.rc `vector_aux_renderer_port`,
else `/dev/ttyACM0`. advmame.rc: `VP_ADVMAME_RC`, else `~/.advance/advmame.rc`,
else `/home/pi/.advance/advmame.rc`.

`VP_STATS=1` makes a game built on `vp_run` print its frame cost once a
second (lines, moves, path lengths, draw and send time): see
[docs/03-game-layer.md](docs/03-game-layer.md).

## Docs

Start at [docs/00-index.md](docs/00-index.md).

## License

MIT, see [LICENSE](LICENSE).

Third-party code, under its own license: [LodePNG](src/third_party/lodepng/)
(Lode Vandevenne, zlib license, in its header), which reads the marquee
PNG; [stb_vorbis](src/third_party/stb/) (Sean Barrett, public domain or MIT,
at its end), which plays Ogg Vorbis sounds and music;
[ReactPhysics3D](src/third_party/reactphysics3d/) 0.10.2 (Daniel Chappuis,
zlib license, its LICENSE; one change, listed in its VECTORPIE-CHANGES.txt),
the physics engine under `vp_physics`. The comic font's letters are made
from [Titan One](tools/make-comic-font/fonts/) (SIL Open Font License, its
OFL.txt).
