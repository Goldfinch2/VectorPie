# VectorPie SDK

Write vector games for **VectorPie**: a Raspberry Pi driving a real vector
monitor through a **USB-DVG** (or the coming **BeamForge**) board. Develop
and test on your PC, in a window; run the same code on the cabinet.

> Status: **0.1.0, in development**: the device layer (frames to the board or
> to a window), the three drawing calls, the advmame.rc reader, the game loop,
> the controls, text, `vp-test`, and two examples (`poc`, `hello`). Audio, 3D,
> high scores and game packages come next.

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

## Try it

On the PC (from the Debian shell; the `.exe` runs in Windows):

```sh
build-pc/poc.exe --windowed          # a turning cube in a window
build-pc/poc.exe --dvg COM5          # on a USB-DVG plugged into the PC
build-pc/hello.exe --windowed        # a small game: arrows, Left Ctrl, 1 to start, Esc
build-pc/vp-test.exe --windowed      # the test pattern
```

On the cabinet: copy `build-pi/poc` over (`scp build-pi/poc pi@<cabinet>:`),
quit the menu (only one program can hold the board), then:

```sh
./poc                     # the board, if advmame.rc enables the DVG
VP_OUTPUT=window ./poc    # the HDMI screen
```

Ctrl+C or closing the window stops it; the board returns to its home screen.
`file build-pi/poc` should say `ARM aarch64`.

## Where a frame goes

| | |
|---|---|
| `VP_OUTPUT=dvg` | the board |
| `VP_OUTPUT=window` / `windowed` | a fullscreen / normal window (on the Pi, always the whole HDMI screen) |
| otherwise, advmame.rc `vector_aux_renderer dvg` | the board |
| otherwise | a fullscreen window (the DVG is disabled, or this is a PC) |

The board's port: `VP_DVG_PORT`, else advmame.rc `vector_aux_renderer_port`,
else `/dev/ttyACM0`. advmame.rc: `VP_ADVMAME_RC`, else `~/.advance/advmame.rc`,
else `/home/pi/.advance/advmame.rc`.

## Docs

Start at [docs/00-index.md](docs/00-index.md).

## License

MIT, see [LICENSE](LICENSE).
