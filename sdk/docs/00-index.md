# VectorPie SDK documentation

Read in order the first time; each chapter is written to stand alone after that.

| chapter | what it answers |
|---|---|
| [01 — The device layer](01-device-layer.md) | Drawing (color / moveto / lineto), where a frame goes (board or window), which board, the counters |
| [02 — advmame.rc](02-advmame-rc.md) | Reading the cabinet's settings and control bindings |
| [03 — The game layer](03-game-layer.md) | The game loop (`vp_run`), the controls (`vp_input`), text (`vp_text`) |

Coming next: audio, 3D, high scores, and packaging a game for the VectorPie
menu.

## The layers

```
your game
  |
  |  vp_run    the loop          vp_input  the controls     vp_text  text
  |  vp_audio, vp_3d, vp_hiscore, ...                     next
  v
vp_dvg   vp_color / vp_moveto / vp_lineto
         frames -> vector board (USB-DVG, BeamForge)  or  -> window (SDL2)
vp_rc    advmame.rc: DVG settings, control bindings
vp_time  a clock and a sleep, same on PC and Pi
```

## Conventions

* Prefix `vp_`. Every call that can fail returns `VP_OK` (0) or a negative
  `VP_ERR_*`; `vp_last_error()` has the detail.
* The SDK draws exactly what the game asks, in the order asked. Fitting a
  frame on the hardware is the game's job; the counters show the cost.
* Nothing fails silently: when the SDK drops or clips something, a counter
  says so.
* The public API is C99; the library is C++ inside and links as a static
  library (`libvectorpie.a`).
* One source, two builds: the PC version (Windows .exe) and the Pi version
  (Linux arm64), both from WSL2 Debian trixie.
