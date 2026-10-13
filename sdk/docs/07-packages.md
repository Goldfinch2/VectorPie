# 07 — Game packages (`game.ini`, `.vpk`)

A finished game goes to a cabinet as a **package**: one folder, installed
by the VectorPie menu (Settings -> GAMES -> INSTALL GAMES), listed under its
manufacturer, launched like any other game.

## The folder

```
gltron/                   named after the id
  game.ini                the manifest
  gltron                  the executable (the Pi build)
  data/ ...               anything the game reads (vp_asset_path)
  marquee.png             optional: the menu's marquee for it
  leds.cfg                optional: its lamp colors and control guide
  guide/*.wav             optional: voice clips for its control guide
```

On the cabinet it becomes `/persistent/games/<id>/`, plus a `save/` folder
for what the game writes (`vp_save_dir()`: high scores and settings).
`save/` is kept when the game is upgraded.

## `game.ini`

```ini
id           = gltron          ; a-z 0-9 _ : the folder's name, unique on the cabinet
name         = GLtron          ; the menu's title
manufacturer = Other           ; the menu group
executable   = gltron          ; relative to the folder
version      = 0.1.0           ; compared part by part: 1.10 > 1.9
sdk          = 0.1             ; optional
author       = ...             ; optional
description  = ...             ; optional: one line, shown by the menu's GAMES page
marquee      = marquee.png     ; optional (marquee.<screen height>.png too)
hiscores     = yes             ; yes = save/highscores.txt (vp_hiscore), shown in
                               ; the menu's hint bar; or a relative path; or no
```

No value may hold `:` or `|` (the menu's own separators). `;` or `#`
starts a comment.

## Running

The menu runs the executable from its folder with **`VP_OUTPUT=dvg`** or
**`VP_OUTPUT=sdl`**, the menu's own choice (as for its `_dvg` / `_sdl`
games). `vp_run` reads it: nothing to do in the game.

**The marquee while playing.** On the board, the cabinet's HDMI screen
would stay black; `vp_run` shows the package's marquee there instead (the
`marquee` file of `game.ini`, else `marquee.png`; `marquee.<screen
height>.png` first when it exists), fitted as advmame.rc's
`vector_aux_marquee_display_mode` says (fit, zoom, stretch, fit_width,
fit_height), and clears it at the end. A game can change it with
`vp_marquee_show(path)` (`vp_marquee.h`), e.g. per level.

## Lamps and the control guide (`leds.cfg`)

A cabinet with LED-lit controls colors each game's buttons, and its
**control guide** (hold the select button on a game) lights each control
in turn while a voice says what it does. A package brings its own, in a
`leds.cfg` with the same syntax as the cabinet's
`/usr/local/share/advance/leds.cfg`, holding lines for the game's id only:

```
# Torus Trooper: its lamp colors and its control guide
torustrooper/color_map[p1_joystick1] #00C0FF
torustrooper/color_map[p1_button1]   #FF2000
torustrooper/color_map[p1_button2]   #FFC000

torustrooper/voice_map[p1_joystick1] steer
torustrooper/voice_map[p1_button1]   fire
torustrooper/voice_map[p1_button2]   charge_shot
```

* **`color_map[<control>] #RRGGBB`**: a control's lamp color in the game.
* **`voice_map[<control>] <word>`**: a step of the guide. The lines'
  order is the guide's; controls given the same word light together; a
  later line for a control replaces an earlier one, and `-` takes it out.
  The label shown is the word, `_` as a space (CHARGE SHOT).
* Controls are the panel's: `p1_joystick1`, `p1_button1` .. `p1_button6`,
  `p1_trackball`, `p1_dial`... as the cabinet's `leds.cfg` names them.
  INSERT COIN and PRESS START close every guide on their own.

**The voice** for a word is the package's `guide/<word>.wav` when it has
one, else the cabinet's shared clip
(`/usr/local/share/advance/guide/<word>.wav`: `fire`, `thrust`, `rotate`,
`steer`, `jump`, `shield`, `hyper_space`, `move`, `aim`... about forty),
else the label alone. Ship only the words the cabinet lacks: WAV, mono,
under about 3 seconds, peaks near -3 dB like the shared clips.

**The owner's file wins**: for each kind of line (`color_map`,
`voice_map`), the package's are used only while the cabinet's own
`leds.cfg` has none for the game. Changing a game's colors in Settings
writes them to the cabinet's file, which then takes over. Installing
writes nothing into it; removing the game leaves nothing behind.

## Making a `.vpk`

```sh
tools/vp-pack/vp-pack.sh <folder> [<output dir>]    # -> <id>-<version>.vpk
```

A `.vpk` is a `.tar.gz` of one `<id>/` folder. `vp-pack` checks the
manifest, that the executable is a 64-bit ARM program (warns if not), that
there are no links, and `leds.cfg` if there is one (every line for the
game's id; a note for each guide word without its own clip); it leaves
`save/` out.

To get it on the cabinet, either:

* put it in a USB drive's `games/` folder (or its top folder) and plug the
  drive in: INSTALL GAMES copies it to `/persistent/games/packages/` first;
* or copy it straight to `/persistent/games/packages/` (scp, the network
  share), no USB drive needed.

INSTALL GAMES offers the `.vpk` files in that folder that are new games or
newer versions of installed ones, and asks before installing; a file is
deleted once installed (as are files of versions already installed).
REMOVE GAMES takes a game off, its high scores with it.

GLtron's `vpk` build target is an example: it stages the folder and runs
`vp-pack` (`cmake --build --preset pi --target vpk`).
