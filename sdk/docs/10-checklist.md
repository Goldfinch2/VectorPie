# 10 — A complete VectorPie app: the checklist

A game that only draws and reads a joystick runs, but it doesn't feel
like part of the cabinet. On a VectorPie cabinet players expect lit
buttons, a spoken control guide, a marquee on the HDMI screen, coins and
credits, and high scores in the menu. Each of these takes a few lines.
This chapter lists them all, with the chapter that explains each one.

Making a screensaver app? Its own, shorter list is at the end:
[Screensaver apps](#screensaver-apps).

**Building your app with an AI assistant?** Give it this chapter with your
request, and ask it to go through the list before calling the app
finished. These are the parts it most often forgets.

## The list

| | what | how | chapter |
|---|---|---|---|
| 1 | The game loop | `vp_run`, never a loop of your own | [03](03-game-layer.md) |
| 2 | The cabinet's controls | `vp_input` actions, never raw keys | [03](03-game-layer.md) |
| 3 | Its own control layout | `vp_controls` and a CONTROLS page | [03](03-game-layer.md) |
| 4 | Lamp colors | `leds.cfg`: `color_map` | below, [07](07-packages.md) |
| 5 | The voice guide | `leds.cfg`: `voice_map`, and `guide/*.wav` for new words | below, [07](07-packages.md) |
| 6 | The marquee | `marquee.png` in the package | below, [07](07-packages.md) |
| 7 | Coins, credits, START lamps | `vp_credits` | [06](06-hiscores.md) |
| 8 | High scores | `hiscores = yes`, `vp_hiscore`, initials | [06](06-hiscores.md) |
| 9 | Both boards | `[dvg]` and `[beamforge]` frame rates, lighter detail on the USB-DVG | below |
| 10 | Its files | `vp_asset_path`, `vp_save_dir` | [04](04-audio.md) |
| 11 | Attract mode | title, high scores, a demo, round and round | below |
| 12 | The package | `game.ini`, `vp-pack`, tested on the PC first | [07](07-packages.md) |

## 1–3. The loop and the controls

* **`vp_run`** runs the loop at a fixed update rate, handles quitting
  (`ui_cancel`, Esc on a PC) and SIGTERM, and shows the marquee. Don't write
  your own loop with `SDL_PollEvent`.
* **Read actions, not keys**: `p1_up` .. `p1_right`, `p1_button1` ..
  `p1_button6`, the `p2_` ones, `start1`, `start2`, `coin1`, `coin2`,
  `ui_pause`, the sticks `p1_stickx` / `p1_sticky`, the spinner and
  trackball. They follow the owner's `advmame.rc`, so the game works with
  any panel wiring: keyboard encoder, joysticks, or both.
* **A typical cabinet has two joysticks** and up to six buttons per player.
  A twin-stick game should use joystick 1 to move and joystick 2 to aim.
* **Name the game's own functions** (`fire`, `thrust`, `bomb`) with
  `vp_input_define`, each on a standard action, and give the player a
  CONTROLS page (`vp_capture`) that saves to `controls.ini`. See
  `examples/` and GLtron.
* **Pause** on `ui_pause`.

## 4. Lamp colors (`leds.cfg`)

A cabinet with LED-lit controls lights each game's controls in that game's
colors. Ship a `leds.cfg` in the package with a line for each control the
game uses, prefixed with the game's `id`:

```
mygame/color_map[p1_joystick1] #50C0FF
mygame/color_map[p1_button1]   #FF2000
mygame/color_map[p1_button2]   #FFB000
```

* Light only the controls the game uses. The others stay dark, so players
  see at once what the game needs.
* Choose colors that mean something: the color of the shot, the shield, the
  ship.
* START and COIN are lit by the cabinet (and by `vp_credits`), so leave them
  out.
* Control names: `p1_joystick1`, `p1_joystick2`, `p2_joystick1`,
  `p1_button1` .. `p1_button6`, `p1_trackball`, `p1_dial`, and the same for
  `p2_`.

## 5. The voice guide (`leds.cfg`)

Holding the select button on a game in the menu starts its **control
guide**: each control lights in turn while a voice says what it does. Add
one `voice_map` line per step, in the order the guide should go:

```
mygame/voice_map[p1_joystick1] move
mygame/voice_map[p2_joystick1] aim
mygame/voice_map[p1_button1]   fire
mygame/voice_map[p1_button2]   bomb
```

* Controls given the same word light together (two buttons that both fire).
* The label on the screen is the word, with `_` shown as a space
  (`hyper_space` shows HYPER SPACE).
* The guide ends with INSERT COIN and PRESS START on its own. Don't add them.

**Words that already have a voice.** The cabinet has a clip for each of
these words, so use them as they are written:

| | | | |
|---|---|---|---|
| `abort` | `add_ship` | `aim` | `armor_piercing` |
| `beginner` | `bomb` | `expert` | `fast` |
| `fire` | `fire_jump` | `forward` | `high_explosive` |
| `hyper_space` | `intermediate` | `jump` | `left_tread` |
| `machine_gun` | `magnification` | `medium` | `move` |
| `nova` | `panic` | `panic_bomb` | `photons` |
| `punch` | `radar` | `right_tread` | `rocket` |
| `rotate` | `shield` | `slow` | `steer` |
| `super_zapper` | `sword` | `thrust` | `tow_missiles` |
| `tractor_shield` | `warp` | | |

(`insert_coin` and `press_start` exist too; the guide uses them by itself.)

**A word that isn't in the table** needs its own clip in the package:
`guide/<word>.wav`, for example `guide/dash.wav`. The file is named
exactly as the word in `voice_map`: lowercase, `_` between words, never a
space (`guide/charge_shot.wav`). Make it WAV, mono, under
about 3 seconds, with peaks near -3 dB like the cabinet's clips.
So it sounds like the rest of the guide, use the cabinet's voice: the
[ElevenLabs](https://elevenlabs.io) voice **Guy - Serious TV Radio
Announcer**, saying the word as it is shown (DASH, CHARGE SHOT). A word
with no clip anywhere is shown without a voice. `vp-pack` notes each
guide word that has no clip of its own in the package.

## 6. The marquee

When a game runs on the vector board, the cabinet's HDMI screen would stay
black, so `vp_run` shows the package's marquee there.

* Put a **`marquee.png`** in the package (or name the file in `game.ini`:
  `marquee = art.png`): the game's logo or artwork.
* For a screen of a particular shape, add **`marquee.<screen height>.png`**,
  for example `marquee.360.png` for a 1920x360 bar marquee. It is used when
  the screen is that tall.
* The owner's `advmame.rc` decides how it fits (fit, zoom, stretch...).
* The menu shows the same picture when the game is selected.
* A game can change it while it plays with `vp_marquee_show(path)`, for
  example per level.
* A screensaver gets the HDMI screen only with `hdmi = app` in `game.ini`
  ([09](09-screensavers.md)).

## 7–8. Coins, credits and high scores

* **`vp_credits`**: coins add credits, START takes them, the START lamps
  blink when a game can start, and the owner's free-play setting is
  honored. Show INSERT COIN, CREDITS n and PRESS START on the attract
  screens.
* **`hiscores = yes`** in `game.ini`, a top-10 table with `vp_hiscore`, and
  initials entered with `vp_initials` when a score ranks. The menu then
  shows the game's best score.

## 9. Both boards

The same package runs on a USB-DVG and on BeamForge. BeamForge draws far
more per frame:

```ini
[dvg]
frame_hz = 30
[beamforge]
frame_hz = 60
```

* Draw lighter on the USB-DVG: fewer particles, a coarser grid, no trails.
  Check `vp_dvg_hardware()` once at the start.
* Watch the counters in a PC window ([01](01-device-layer.md)): a frame with
  too many lines flickers on the tube.
* Draw shapes in the order they lie on the screen. A long jump across the
  screen shifts the strokes that follow it.
* Use bright, saturated colors. Very dim lines disappear on a real tube.

## 10. Its files

* Read the game's data through **`vp_asset_path`**, never a path relative
  to the current folder or one written into the program.
* Write settings, layouts and scores only in **`vp_save_dir()`**. The
  package folder is replaced on every update; the save folder is kept.

## 11. Attract mode

A cabinet stands on and idle for hours. Without credits, cycle through
the title, the high scores, a short how-to-play, and a self-playing demo,
then start again. Keep the picture moving so no line burns into the tube,
and keep the demo silent.

## 12. The package and testing

* `game.ini`: `id`, `name`, `manufacturer`, `executable`, `version`, and
  `author` and `description` so the menu can show them. Raise `version` on
  every release.
* Build and play the PC version first (`--windowed`), then the Pi build.
* `vp-pack` makes the `.vpk` and checks `game.ini`, the executable and
  `leds.cfg`. Read what it prints.
* On the cabinet, check: the lamps light in the game's colors, the guide
  speaks every step, the marquee shows, coins and START work, a high score
  asks for initials and shows in the menu, and EXIT returns to the menu.

## Screensaver apps

A screensaver app ([09](09-screensavers.md)) is an SDK app too, but the
cabinet does more of the work, so its list is different.

| | what | how |
|---|---|---|
| 1 | The kind | `kind = screensaver` in `game.ini`: `vp-pack` makes a `.vps` |
| 2 | The loop | `vp_run`, as for a game |
| 3 | The music | `vp_saver_music` every frame |
| 4 | Tracks without a beat map | pulse on `onsets` when `has_beats` is 0 |
| 5 | Between tracks | something that still looks right when nothing plays (`vp_saver_music` returns 0) and during a DJ clip (`kind` is `VP_MUSIC_CLIP`: speech, no beat) |
| 6 | The HDMI screen | leave it to the menu, or `hdmi = app` and a marquee |
| 7 | Both boards | `[dvg]` and `[beamforge]` frame rates, lighter on the USB-DVG |
| 8 | Hours of running | no memory growth, no picture that slowly fills up |
| 9 | Testing | on a PC with a music file or folder on the command line |

**What a screensaver leaves out:**

* **Lamps and the voice guide**: no `leds.cfg`. The menu keeps running its
  own light show on the beat while the app draws.
* **Controls**: none of its own, and no CONTROLS page. Any control wakes the
  cabinet, and START 2 plays the next track. `vp_run` and the menu do both.
* **Credits, high scores, attract mode**: none. The screensaver is the
  attract mode.
* **Keeping the picture moving to save the tube**: `vp_run` already moves
  the whole picture slowly around the screen. Draw for the full screen as
  usual.

**The music:**

* Count beats with `beats` (it goes up by one on each beat), not by watching
  `since_beat`, so no beat is missed between frames. `beat_in_bar == 1` is
  the downbeat, the place for the big moment.
* Use `to_beat` to time a movement so it lands on the next beat.
* The bars, VU meters and peaks are already scaled to the track's
  loudest. Don't scale them again.
* The waveform shrinks with the volume: scale it to its own largest sample.
  On the board, draw 128 to 256 of its points, not all 512.

**The HDMI screen:**

* Without `hdmi = app`, the menu shows its screensaver video with the
  titles playing, and the app has no marquee.
* With `hdmi = app`, the app has the screen. Show a `marquee.png`, or make a
  picture per track with `vp_marquee_show` (the track's name is `title`).
  The menu's NOW PLAYING titles are gone then, so show the title yourself
  if it matters.

**On the cabinet, check:** it starts after the idle delay from SCREENSAVERS →
SELECT, it moves on the beat (and on a track without a beat map), START 2
skips one track, any other control brings the menu back, and it still runs
cleanly after a night.
