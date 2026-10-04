# 01 — The device layer

`#include <vectorpie/vp_dvg.h>`

The device layer puts lines on one output: the **vector board** (USB-DVG or
BeamForge, which speak the same protocol) or a **window** (a window on the
PC, the whole HDMI screen on the Pi). The window shows exactly what the board
would be sent, so a game can be written and checked on a PC.

## A complete program

```c
#include <vectorpie/vp.h>
#include <stdio.h>

int main(void)
{
    vp_dvg *d;
    if (vp_dvg_open_auto("hello", &d) != VP_OK) {
        fprintf(stderr, "%s\n", vp_last_error());
        return 1;
    }
    while (!vp_dvg_window_closed(d)) {
        vp_dvg_begin_frame(d);
        vp_color(d, 0, 255, 0);
        vp_moveto(d, 1000, 1000);          /* a green triangle, one stroke */
        vp_lineto(d, 3000, 1000);
        vp_lineto(d, 2000, 3000);
        vp_lineto(d, 1000, 1000);
        vp_dvg_end_frame(d, 0);
        vp_sleep_ms(25);                   /* about 40 frames a second */
    }
    vp_dvg_close(d);
    return 0;
}
```

On a board the loop runs until the program is stopped; a real game exits on
its own input. `examples/poc/main.c` is a fuller version (a turning cube,
command-line options, Ctrl+C handling).

## Drawing: the beam's three calls

| call | what the beam does |
|---|---|
| `vp_color(d, r, g, b)` | the color of the lines that follow |
| `vp_moveto(d, x, y)` | moves to (x, y), off |
| `vp_lineto(d, x, y)` | draws a line from where it is to (x, y) |

This is the board's own model, and it is all there is. The SDK draws exactly
what is asked, **in the order asked**: it never reorders or merges lines.
Order is the game's job, and it matters: a `lineto` that continues from the
last point costs one 4-byte word; every jump adds an invisible move. Draw
connected lines one after the other, and nearby things together.

A black `lineto` draws nothing (it is a move). A `lineto` to the point the
beam is on draws a dot.

## Coordinates and color

* **0..4095** on both axes, origin **bottom-left**, **Y up**. The monitor
  stretches this square to its tube (4:3, or 3:4 when mounted vertically), and
  the window stretches it the same way.
* Lines are clipped to the **clip region** (`vp_dvg_set_clip`; the full range
  by default). A line entirely outside counts in `clipped_away`.
* **8-bit R, G, B.** There is no separate intensity: a dimmer line is a darker
  color.

## Choosing the output

`vp_dvg_open_auto` decides the way every VectorPie game should:

1. `VP_OUTPUT` in the environment, which the menu sets when it launches a
   game: `dvg`, `window` (fullscreen), `windowed`;
2. otherwise advmame.rc: `vector_aux_renderer dvg` means the board;
3. otherwise a **fullscreen window**: the DVG is disabled in the cabinet's
   settings, or there is no advmame.rc (a PC).

`VP_VERTICAL=1` gives the window the vertical 3:4 shape. `vp_dvg_open_ex`
takes the same choices explicitly (`vp_dvg_options`), and
`vp_dvg_auto_options` fills the options without opening anything, with the
reason.

**On the Pi there is no windowing system** (no X11, no Wayland). The window
output then draws through SDL's KMS/DRM driver, as the menu does: always the
whole HDMI screen, letterboxed to the monitor's shape, no cursor.

**The board's port**: `VP_DVG_PORT`, else advmame.rc `vector_aux_renderer_port`,
else `/dev/ttyACM0`. On Windows there is no default: give `COMx`.

## Which board

`vp_dvg_hardware(d)` says what is drawing: `VP_HW_WINDOW`, `VP_HW_USB_DVG` or
`VP_HW_BEAMFORGE` (`vp_hardware_name` gives the text). The SDK asks the board
once, when it is opened (its `productName`); a board that does not answer is
taken to be a USB-DVG. It is for the game's information, e.g. to choose its
level of detail: the SDK drives both boards the same way.

## Frames

`vp_dvg_begin_frame` → the drawing calls → `vp_dvg_end_frame`.

`vp_dvg_end_frame` turns the lines into the board's 4-byte commands: an
invisible move wherever a line does not start where the beam is, a color word
only when the color changes, the line's end point, and the end-of-frame word.

The board **keeps redrawing the last frame** until a new one arrives, but
returns to its home screen if nothing arrives for its **BURN-IN** time (a
board menu setting, 2 seconds by default). Keep sending frames, even
unchanged ones.

## Fitting the hardware is the game's job

The board draws with a variable step length: when a frame takes too long to
draw, it draws the next one faster, so an overloaded frame comes out coarser
and dimmer rather than failing. The SDK does not budget or simplify anything.
The counters show what a frame costs, so the designer can tune it on the
monitor. The BeamForge draws about 6.7 times as much as the USB-DVG
(about 20 million against 3 million points a second).

The one hard limit: **65,536 lines per frame** (`VP_DVG_MAX_VECTORS`). Past
it, lines are **not drawn** and counted in `dropped` (and the frame in
`frames_with_drops`). The end-of-frame word is always sent.

## Counters

`vp_dvg_get_stats` fills `vp_dvg_stats`. For the last frame: `vectors`
(lines), `blank_moves`, `words`, `clipped_away`, `dropped`, and `lit_length` /
`blank_length` (total distance in board units, lit and invisible). In total:
`frames_sent`, `frames_with_drops`, and for the board `bytes_sent` and
`write_errors`. **If `dropped` is not zero, the screen is not showing the
whole frame.**

## Closing

`vp_dvg_close` sends the board its exit sequence (16 end-of-frame words, then
EXIT), so the board returns to its home screen, then closes the port. A window
just closes. Only one program can hold the board at a time: on the cabinet,
the menu releases it while a game runs.

## Time

`vp_ticks_ms()` (a millisecond clock that never goes backwards) and
`vp_sleep_ms(ms)` work the same on the PC and the Pi (`vp_time.h`).

## Errors

| code | when |
|---|---|
| `VP_ERR_NODEV` | no port configured (Windows), the port cannot be opened, or no window possible |
| `VP_ERR_TIMEOUT` | the board stopped accepting data for 2 seconds |
| `VP_ERR_IO` | the port failed or was unplugged |

`vp_last_error()` has the specific text, e.g. `cannot open /dev/ttyACM0:
Permission denied`.
