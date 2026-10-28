# VectorPie SDK documentation

Read in order the first time; each chapter is written to stand alone after that.

| chapter | what it answers |
|---|---|
| [01 — The device layer](01-device-layer.md) | Drawing (color / moveto / lineto), where a frame goes (board or window), which board, the counters |
| [02 — advmame.rc](02-advmame-rc.md) | Reading the cabinet's settings and control bindings |
| [03 — The game layer](03-game-layer.md) | The game loop (`vp_run`), the controls (`vp_input`), a game's own controls and its control menu (`vp_controls`), text (`vp_text`) |
| [04 — Sound](04-audio.md) | Effects, streamed music, voices (`vp_audio`); where a game's files are (`vp_paths`) |
| [05 — 3D](05-3d.md) | The view (camera, fog), placements, meshes and OBJ files, solid and wireframe drawing, scenes where objects hide each other (`vp_3d`) |
| [06 — High scores and credits](06-hiscores.md) | The top-10 table the menu shows, entering initials, drawing the table (`vp_hiscore`); coins, credits, the START lamps (`vp_credits`) |
| [07 — Game packages](07-packages.md) | The game's folder, `game.ini`, `VP_OUTPUT`, making a `.vpk` with `vp-pack`, installing from USB |
| [08 — Physics](08-physics.md) | Bodies that fall, bounce, stack and tumble; joints with limits and motors; rays and contacts (`vp_physics`) |

Coming next: installing games online (a signed catalog).

## The layers

```
your game
  |
  |  vp_run    the loop          vp_input  the controls     vp_text  text
  |  vp_audio  effects, music     vp_paths  the game's files
  |  vp_3d     3D: solid, wireframe, hidden lines
  |  vp_physics  rigid bodies, joints, rays (ReactPhysics3D inside)
  |  vp_hiscore  high scores, initials   vp_credits  coins, START lamps
  |  game packages: game.ini, vp-pack, installed from USB by the menu
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
