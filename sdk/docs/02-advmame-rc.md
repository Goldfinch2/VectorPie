# 02 — advmame.rc

`#include <vectorpie/vp_rc.h>`

On VectorPie, `advmame.rc` is where the cabinet is configured: the DVG and its
port, and the controls (`input_map[...]`). AdvanceMAME, the menu and the
native games all read it. A game that reads it too uses the same controls as
everything else on the cabinet, and follows the owner's changes without a
settings screen of its own.

The SDK reads it; it never writes it. Controls are changed in the menu.

## Where it is

`VP_ADVMAME_RC` if set, else `~/.advance/advmame.rc`, else
`/home/pi/.advance/advmame.rc`. On a PC there is usually none, and the SDK
falls back to its defaults (a window, default keys).

## Settings

```c
vp_rc *rc;
if (vp_rc_load(NULL, &rc) == VP_OK) {
    const char *renderer = vp_rc_get(rc, "vector_aux_renderer");      /* "dvg" or NULL */
    int sort = vp_rc_get_bool(rc, "vector_aux_renderer_sort_vectors", 0);
    vp_rc_free(rc);
}
```

`vp_rc_get` returns the raw value of any `key value` line; the last
occurrence of a key wins.

## Control bindings

Each `input_map[action]` line is decoded into `vp_binding` records, one per
`or` alternative:

```
input_map[p1_button1] keyboard[0,lcontrol] or joystick_button[atari_controls,button1]
```

| `vp_binding.type` | from | `device` | `key` / `index` |
|---|---|---|---|
| `VP_BINDING_KEY` | `keyboard[0,lcontrol]` | `"0"` | `key = "lcontrol"` |
| `VP_BINDING_JOY_BUTTON` | `joystick_button[atari_controls,button1]` | `"atari_controls"` | `index = 0` |
| `VP_BINDING_JOY_AXIS_NEG` / `_POS` | `joystick_digital[0,0,1,up]` | `"0"` | `index` = axis |
| `VP_BINDING_MOUSE_BUTTON` | `mouse_button[0,left]` | `"0"` | `index = 0` |
| `VP_BINDING_MOUSE_AXIS` | `mouse[0,x]` (spinner, trackball) | `"0"` | `index` 0 = x, 1 = y |

* Buttons are **0-based** (`button1` is 0).
* `device` is kept as written (a number, or the device name normalized the way
  AdvanceMAME does); the input layer matches it to a connected device.
* `auto` and empty actions are not listed. A combination of several inputs is
  read as its first input; `not` parts are skipped.

```c
for (int i = 0; i < vp_rc_binding_count(rc, "p1_up"); i++) {
    vp_binding b;
    vp_rc_binding(rc, "p1_up", i, &b);
}
```

`vp-test --rc` prints every action and binding the SDK read.

The common actions are `p1_up`, `p1_down`, `p1_left`, `p1_right`,
`p1_button1`..`p1_buttonN`, the same for `p2_`, `start1`, `start2`, `coin1`,
`coin2`, and `ui_cancel` (leave the game). The input layer (next phases) turns
them into pressed/held/released states, with defaults when advmame.rc is
missing.
