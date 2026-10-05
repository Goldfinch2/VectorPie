# 03 — The game layer: loop, controls, text

`#include <vectorpie/vp.h>` (`vp_app.h`, `vp_input.h`, `vp_text.h`)

`examples/hello/main.c` is a complete small game built on this chapter:
read it alongside.

## The loop: `vp_run`

```c
static void update(vp_game *g, float dt) { /* move things by dt */ }
static void draw(vp_game *g, vp_dvg *d)  { /* vp_color, vp_moveto, vp_lineto, vp_text */ }

int main(int argc, char **argv)
{
    vp_game_desc desc = {0};
    desc.title  = "My Game";
    desc.update = update;
    desc.draw   = draw;
    return vp_run(&desc, argc, argv);
}
```

`vp_run` does what every VectorPie game should do the same way:

* opens the output (`vp_dvg_open_auto`: the board, or a window) and the
  controls (`vp_input`);
* calls `update` at a **fixed rate** (`update_hz`, 60 by default), reading
  the controls before each call. Game logic sees the same `dt` every time, so
  it behaves the same at any frame rate. A late frame is caught up with at
  most 5 steps;
* calls `draw` **once per frame** (`frame_hz`, 40 by default), inside the
  frame (`vp_dvg_begin_frame` / `vp_dvg_end_frame` are done for you);
* stops on **`ui_cancel`** (the cabinet's exit, back to the menu), on a closed
  window, on Ctrl+C, or when the game calls `vp_game_quit`; then calls
  `shutdown` and closes everything, so the board gets its exit sequence.

Optional callbacks: `init` (return `VP_OK`, anything else stops the game)
and `shutdown`. `vp_game_user(g)` returns `desc.user`; `vp_game_time(g)` is
the game time in seconds.

For development, `vp_run` reads a few options (the menu passes none):
`--dvg [PORT]`, `--window`, `--windowed`, `--vertical`.

`vp_run` owns SDL's event queue: events are read and dropped each frame.
Read the controls through `vp_input`.

## The controls: `vp_input`

A game asks for **actions** by their advmame.rc names, never for keys:

```c
vp_input *in = vp_game_input(g);
if (vp_input_pressed(in, "p1_button1")) fire();       /* down since the last step */
if (vp_input_held(in, "p1_left"))       turn_left();
float steer = vp_input_axis(in, "p1_stickx");         /* -1 .. 1 */
int spin = vp_input_delta(in, "p1_dialx");            /* spinner counts */
```

Each action's bindings come from the cabinet's **advmame.rc**, the same file
AdvanceMAME reads and the menu edits. A game follows the owner's controls
without a settings screen of its own.

### Defaults

Where advmame.rc says `auto`, has no line for an action, or does not exist
(a PC), the action gets **AdvanceMAME's own default**:

| action | keyboard | joystick / mouse |
|---|---|---|
| `p1_up` `p1_down` `p1_left` `p1_right` | arrows | joystick 1, first stick |
| `p1_button1` .. `p1_button6` | Left Ctrl, Left Alt, Space, Left Shift, Z, X | joystick 1 buttons 1-6; mouse left, right, middle for 1-3 |
| `p2_up` `p2_down` `p2_left` `p2_right` | R, F, D, G | joystick 2, first stick |
| `p2_button1` .. `p2_button4` | A, S, Q, W | joystick 2 buttons 1-4 |
| `start1` `start2` | 1, 2 | |
| `coin1` `coin2` | 5, 6 | |
| `ui_cancel` | Esc | |
| `p1_stickx` `p1_sticky` (analog) | | joystick 1, first stick |
| `p2_stickx` `p2_sticky` (analog) | | joystick 2, first stick |
| `p1_dialx`, `p1_trackballx` / `p1_trackbally` (relative) | | mouse x / y |

Any other action listed in advmame.rc works too, by its name.

### Digital, analog, relative

* **Digital** (`vp_input_held`, `_pressed`, `_released`): keys, joystick
  buttons, a joystick direction past half travel, mouse buttons. Several
  bindings are "or" alternatives.
* **Analog** (`vp_input_axis`): a stick axis, -1 .. 1, left and up negative
  as the stick reports them, with a dead zone (`vp_input_set_deadzone`,
  default 0.15 of full travel). advmame.rc writes it `joystick[dev,stick,axis]`;
  a leading `-` reverses it; several analog inputs add up.
* **Relative** (`vp_input_delta`): mouse, spinner and trackball movement since
  the last step, in counts.

Joysticks are matched as advmame.rc names them: by number (`0` is the
first) or by name (`atari_controls`). Joysticks plugged in later are picked
up.

**Keyboard and the board output on a PC:** Windows only sends keys to a
focused window. Driving a USB-DVG from the PC with no window, use a gamepad,
or test with the window output.

## Text: `vp_text`

```c
vp_color(d, 255, 255, 0);
vp_text(d, "GAME OVER", 2048, 2000, 200, VP_TEXT_CENTER);
float w = vp_text_width("SCORE 100", 120);
```

Two stroke fonts, chosen with `vp_text_font` (it applies to the following
`vp_text` and `vp_text_width` calls):

| font | looks | strokes |
|---|---|---|
| `VP_FONT_HERSHEY` (default) | Hershey Simplex: round letters, upper and lower case, full punctuation | more per letter |
| `VP_FONT_ASTEROIDS` | Atari's Asteroids lettering: capitals only (lower case is drawn as capitals), `<` and `>` as triangles | few per letter: cheap on the beam |

```c
vp_text_font(VP_FONT_ASTEROIDS);
vp_text(d, "SCORE 1500", 200, 3800, 120, VP_TEXT_LEFT);
```

Printable ASCII. `height` is a capital
letter's height in board units; `(x, y)` is on the baseline, at the left
end, the center or the right end (`VP_TEXT_LEFT`, `VP_TEXT_CENTER`,
`VP_TEXT_RIGHT`). Text uses the current color and the same `vp_moveto` /
`vp_lineto` as everything else, so it costs what its strokes cost.
