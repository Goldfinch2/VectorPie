# 03 — The game layer: loop, controls, text

`#include <vectorpie/vp.h>` (`vp_app.h`, `vp_input.h`, `vp_controls.h`, `vp_text.h`)

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
  frame (`vp_dvg_begin_frame` / `vp_dvg_end_frame` are done for you). On
  the board, `frame_hz_board` replaces it when set: a heavy game can give
  the board more time per frame (30 Hz) and still run at 40 in a window.
  The game logic keeps its own `update_hz` either way;
* stops on **`ui_cancel`** (the cabinet's exit, back to the menu), on a closed
  window, on Ctrl+C, or when the game calls `vp_game_quit`; then calls
  `shutdown` and closes everything, so the board gets its exit sequence.

Optional callbacks: `init` (return `VP_OK`, anything else stops the game)
and `shutdown`. `vp_game_user(g)` returns `desc.user`; `vp_game_time(g)` is
the game time in seconds.

For development, `vp_run` reads a few options (the menu passes none):
`--dvg [PORT]`, `--window`, `--windowed`, `--maximized` (a normal window,
maximized: unlike the fullscreen `--window`, screen recorders capture it),
`--vertical`.

**`VP_STATS=1`** prints what a frame costs, once a second: the frame rate,
then the average and the peak of the lines drawn, the blank moves, the
words sent, the lit and blank path lengths (board units), the time in
`draw` (game drawing plus the 3D scene) and the time sending the frame, and
how many frames had lines dropped. Use it to tune a game to the board:
drawing time on the board grows with the number of lines and the path
length, and a frame the board cannot finish in time is drawn faster and
dimmer.

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
AdvanceMAME reads and the menu edits, so a game follows the owner's controls
out of the box. A game may also name its own functions and let the player
lay them out in its menu: see [A game's own controls](#a-games-own-controls).

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
| `ui_pause` | P | |
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
  the last step, in counts. A spinner or trackball game calls
  `vp_input_grab_mouse(in, 1)` once: the mouse is captured (SDL's relative
  mode), so the counts keep coming however far it turns instead of stopping
  at the screen's edge. `vp_input_close` releases it.

Joysticks are matched as advmame.rc names them: by number (`0` is the
first) or by name (`atari_controls`). Joysticks plugged in later are picked
up.

### A game's own controls: `vp_controls`

A game names its functions and gives each a **standard**, usually the
cabinet's actions, so it still follows the owner's setup until the player
changes something:

```c
vp_input *in = vp_game_input(g);
vp_input_define(in, "fire",  "p1_button1");
vp_input_define(in, "smart", "p1_button2+key_Space");
vp_input_load(in, NULL);                    /* the player's own: save/controls.ini */

if (vp_input_pressed(in, "fire")) fire();   /* like any action */
```

**Controls as text**, `+` between them, at most `VP_CONTROLS_MAX` (4) of the
player's own:

| text | control |
|---|---|
| `key_Left_Ctrl` | a key: `key_` and SDL's name, `_` for spaces |
| `joy1_button3` | a joystick's button (joysticks and buttons from 1) |
| `yoke_button3` | the same on the yoke: the first joystick whose name says "yoke" |
| `joy1_hat1_up` | a hat's way: `up`, `down`, `left`, `right` |
| `joy1_axis2_neg` | an axis past half its travel: `neg` or `pos` |
| `mouse_left` | a mouse button: `left`, `middle`, `right`, `x1`, `x2` |
| `p1_button1` | an advmame.rc action: the controls the cabinet gives it |
| `standard` | the function's standard |
| `none` | no control |

`vp_input_set(in, "fire", "joy1_button2")` sets a function (`NULL` or
`"standard"`: back to its standard), `vp_input_controls` reads what it is
set to, `vp_input_spelled` writes it for the screen (`LEFT CTRL + JOY1
BUTTON 3`). `vp_input_save` / `vp_input_load` keep the player's choices,
one `function = controls` line each, in `save/controls.ini` by default: the
game's `save/` folder, which a VectorPie backup carries.

**The menu.** A game's control menu never goes by what is laid out: it reads
the fixed **menu controls**, `vp_input_menu_up` / `_down` / `_left` /
`_right` / `_confirm` (the arrow keys, any joystick's hat or stick, START 1,
Enter), so a player can never lock themselves out. **Laying out** a
function is `vp_capture`, the same on every VectorPie game:

* START 1 or Enter on a function's row (`vp_capture_begin`), then one
  control: the first since the cursor came to the row replaces what the
  function had, every further one on the row is added (the oldest goes past
  four);
* while laying out, START 1 / Enter leave it as it was, START 2 makes it
  none, Delete (also the keypad's) its standard;
* Left and right on the row switch between none and the standard
  (`vp_capture_toggle`);
* START, COIN, the way out, F2, Esc, Enter and Delete are never taken
  (`vp_input_reserve_key` adds the game's own, such as the key that opens
  its menu).

```c
static vp_capture cap;                       /* all zero */

/* each step, on the row of function `name` */
if (cap.capturing) {
    if (vp_capture_step(&cap, in) == VP_CAPTURE_CHANGED) vp_input_save(in, NULL);
} else if (vp_input_menu_confirm(in)) {
    vp_capture_begin(&cap, in, name);
} else if (vp_input_key_pressed(in, SDL_SCANCODE_DELETE)) {
    vp_capture_standard(&cap, in, name);
    vp_input_save(in, NULL);
}
/* moving to another row: vp_capture_moved(&cap) */
```

`vp_capture_help` gives the two help lines for the bottom of the screen,
`vp_capture_said` a message to show under the rows (a reserved control
pressed), `vp_capture_filter` keeps a control just laid out from also moving
the cursor.

For a game's own handling of a flight yoke or stick: `vp_input_yoke`,
`vp_input_stick` (the first joystick that is not the yoke and has a stick
resting in its middle, not pedals) and `vp_input_joy_axis` read the
joysticks as they are.

**Keyboard and the board output on a PC:** Windows only sends keys to a
focused window. Driving a USB-DVG from the PC with no window, use a gamepad,
or test with the window output.

## Text: `vp_text`

```c
vp_color(d, 255, 255, 0);
vp_text(d, "GAME OVER", 2048, 2000, 200, VP_TEXT_CENTER);
float w = vp_text_width("SCORE 100", 120);
```

Three stroke fonts, chosen with `vp_text_font` (it applies to the following
`vp_text` and `vp_text_width` calls):

| font | looks | strokes |
|---|---|---|
| `VP_FONT_HERSHEY` (default) | Hershey Simplex: round letters, upper and lower case, full punctuation | more per letter |
| `VP_FONT_ASTEROIDS` | Atari's Asteroids lettering: capitals only (lower case is drawn as capitals), `<` and `>` as triangles | few per letter: cheap on the beam |
| `VP_FONT_COMIC` | fat, rounded cartoon capitals (Titan One), each letter drawn as its outline: capitals, digits, `! ? ' - . : , + / ( ) & * = < > #` | many per letter: titles and big messages |

```c
vp_text_font(VP_FONT_ASTEROIDS);
vp_text(d, "SCORE 1500", 200, 3800, 120, VP_TEXT_LEFT);
```

**The comic font** has two settings of its own, for the following calls:
`vp_text_border(r, g, b)` draws a border round each letter in that color (the
letters take the current color; 0, 0, 0 for none), and `vp_text_wobble(amount,
time)` makes each letter bounce and tilt a little (0 still, 1 lively; pass
the game time). Over a 3D scene, `vp3d_scene_text` makes the lettering hide
what the scene draws behind it ([05 — 3D](05-3d.md)).

```c
vp_text_font(VP_FONT_COMIC);
vp_text_border(255, 40, 170);                /* white letters, a pink border */
vp_text_wobble(1.0f, vp_game_time(g));
vp_color(d, 255, 255, 255);
vp_text(d, "QUALIFIED!", 2048, 2600, 300, VP_TEXT_CENTER);
```

The letters come from `tools/make-comic-font` (Titan One, SIL Open Font
License): another font can be made the same way.

Printable ASCII. `height` is a capital
letter's height in board units; `(x, y)` is on the baseline, at the left
end, the center or the right end (`VP_TEXT_LEFT`, `VP_TEXT_CENTER`,
`VP_TEXT_RIGHT`). Text uses the current color and the same `vp_moveto` /
`vp_lineto` as everything else, so it costs what its strokes cost.
