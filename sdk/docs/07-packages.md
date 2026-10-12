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

## Making a `.vpk`

```sh
tools/vp-pack/vp-pack.sh <folder> [<output dir>]    # -> <id>-<version>.vpk
```

A `.vpk` is a `.tar.gz` of one `<id>/` folder. `vp-pack` checks the
manifest, that the executable is a 64-bit ARM program (warns if not), and
that there are no links; it leaves `save/` out.

To get it on the cabinet, either:

* put it in a USB drive's `games/` folder (or its top folder) and plug the
  drive in: INSTALL GAMES copies it to `/persistent/games/packages/` first;
* or copy it straight to `/persistent/games/packages/` (scp, the network
  share), no USB drive needed.

INSTALL GAMES lists every `.vpk` in that folder with NEW, UPDATE, SAME or
OLDER against what is installed, or why it cannot be installed. The files
stay there after installing (a reinstall needs no USB drive). REMOVE GAME takes a game off,
keeping its high scores for a reinstall unless told otherwise.

GLtron's `vpk` build target is an example: it stages the folder and runs
`vp-pack` (`cmake --build --preset pi --target vpk`).
