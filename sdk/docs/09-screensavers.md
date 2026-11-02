# 09 — Screensavers: `vp_saver`

`#include <vectorpie/vp.h>` (`vp_saver.h`, `vp_music_feed.h`).
`examples/saver-spectrum`, `examples/saver-pulse` and `examples/saver-vu` show all of it.
Each has its `game.ini`; `cmake --build build-pi --target vps` packs them
(`build-pi/saver_<name>-<version>.vps`).

A screensaver app is a game the cabinet runs **instead of its own
screensaver**, drawing to the music the cabinet is playing. The cabinet's
menu keeps playing its music (its playlist, DJ clips, light show go on)
and tells the app what is playing, about 60 times a second: the track, its
beats and its sound.

## Making one

It is an SDK game like any other (`vp_run`), packaged with one more line in
its `game.ini`:

```ini
kind = screensaver
```

`vp-pack` then makes a **`.vps`** (not a `.vpk`). SETTINGS → SCREENSAVERS →
INSTALL installs it apart from the games; it appears in
**SETTINGS → SCREENSAVERS → SELECT** (next to the cabinet's own, VECTORPIE,
and RANDOM). The cabinet starts it after the idle delay, on the board; the
HDMI screen keeps the cabinet's screensaver picture and the titles playing
(a screensaver does not show a marquee).

## The music

```c
vp_music_feed m;
if (vp_saver_music(&m)) {               /* in update() or draw(): 1 when something plays */
    for (int b = 0; b < VP_MUSIC_BANDS; b++) bar(b, m.bands[b]);
    if (m.beats != last_beats) { boom(m.beat_in_bar == 1); last_beats = m.beats; }
}
```

| field | what |
|---|---|
| `kind` | `VP_MUSIC_TRACK` (the playlist), `VP_MUSIC_CLIP` (a DJ clip), `VP_MUSIC_THEME`, `VP_MUSIC_NONE` |
| `track` | changes with each new track |
| `position` | seconds into it, as heard |
| `length` | its whole length in seconds; 0 unknown |
| `title`, `next` | the track's name and the next one's (empty for the theme) |
| `has_beats` | 1 when the track has a beat map (`<track>.beats`, made by the menu's GENERATE BEAT MAPS) |
| `beats` | beats heard so far: it goes up by one on each, so none is missed between frames |
| `beat_in_bar` | the last beat's place in its bar: 1 the downbeat, 2, 3, 4; 0 unknown |
| `since_beat`, `to_beat` | seconds since the last beat, to the next: for movements timed to land on it |
| `bpm` | the tempo there |
| `level` | how loud, 0 .. 1 |
| `bands[32]` | the spectrum, 0 .. 1, from the bass (40 Hz) to the highs (16 kHz), log spaced; each rises at once and sinks back in about a second. 1 is the track's loudest (measured with its beat map), else the loudest lately, so the bars fill the screen at any volume |
| `onsets` | hits heard (drums, attacks): goes up by one on each. On a track without a beat map, the thing to pulse on |
| `vu[2]` | VU meters, left and right, in dB: 0 is the track's loud parts, so a classic VU face reads -20 .. +3. As slow as a VU needle (300 ms) |
| `peak[2]` | the peaks, left and right, in dB: 0 is the track's loudest. A peak stays a second, then falls |
| `wave[512][2]`, `wave_rate` | the waveform: the last 512 frames heard (about 11 ms), left and right, -32768 .. 32767, oldest first, at `wave_rate` Hz (0: none). For a scope, or left against right as an XY figure |

The VU meters and peaks, like the bars, are measured against the track's
loudest, so they swing the same at any volume. The waveform is the sound
as it leaves the cabinet, so it shrinks with the volume: scale it to its
own largest sample for a picture that fills the screen. On the board, draw
128 to 256 of its points (every 2nd to 4th) rather than all 512: the
picture stays steady.

## Saving the screen

A screensaver runs for hours, so `vp_run` keeps its picture moving: the
whole of it, drawn a little smaller (86 to 94%), wanders slowly round the
screen and breathes in size, on slow, uneven periods (about a minute), from a
different place each run. No line stays on the same spot of the tube, even
in a picture that stands still (a frame, a meter's scale). Draw for the full
screen as usual; nothing is cut at the edges. It is on whenever the game
runs as a screensaver (`VP_SAVER=1`); `vp_dvg_set_view` is the same
movement for a game of its own.

## The controls

**Any control wakes the cabinet** (the app ends, the menu comes back), and
**the NEXT TRACK control plays the next track** (the app goes on). On the
cabinet the menu reads the controls itself and ends the app (SIGTERM, which
`vp_run` answers by quitting); the app gets no keyboard there, as the menu
keeps the screen. Run as a screensaver without the menu (`VP_SAVER=1`, no
`VP_MUSIC_FEED`), `vp_run` does the same: START 2 is `vp_saver_skip()`.
The game needs no controls of its own.

## On a PC

With no cabinet menu, the SDK stands in for it: give the app a music file
(Ogg Vorbis or WAV), or a folder of them, on its command line (or in
`VP_SAVER_MUSIC`):

```
saver-spectrum music\track.ogg
```

It plays them in turn and analyses what it plays; a `track.beats` beside
`track.ogg` gives that track's beats. A beat map is a text file, a beat a
line: `<seconds><tab><beat in bar>`, and a line `# top_db <dB>` gives the
track's loudest (the top of the bars; `vp_music_track_top_db` measures it).
Esc quits.

## Under the hood

The menu publishes a `vp_music_feed` (`vp_music_feed.h`) in shared memory,
the file named by `VP_MUSIC_FEED` (`/dev/shm/vectorpie-music`), and starts
the app with `VP_SAVER=1`. The header has the layout, the read that never
sees a half-written copy, and the spectrum analysis both sides use. A game
can analyse its own sound the same way: `vp_audio_recent_stereo` and
`vp_music_analyze`.
