# 04 — Sound: `vp_audio`

`#include <vectorpie/vp.h>` (`vp_audio.h`, `vp_paths.h`)

```c
vp_audio *a = vp_game_audio(g);            /* opened by vp_run */
char path[1024];
vp_sound *zap;
vp_sound_load(a, vp_asset_path("sounds/zap.wav", path, sizeof path), &zap);

vp_sound_play(a, zap, 1.0f, 1.0f, 0);      /* volume, pitch, loop */
vp_voice music = vp_music_play(a, vp_asset_path("music/theme.wav", path, sizeof path),
                               0.6f, 1, 2.0f);        /* looped, 2 s fade in */
vp_voice_stop(a, music, 1.5f);             /* 1.5 s fade out */
```

## Effects and music

* **Effects** (`vp_sound`) are loaded into memory once (`vp_sound_load`) and
  played as often as needed, many at once. A sound can also be made in code:
  `vp_sound_from_samples` takes float samples (-1..1), mono or stereo, at any
  rate.
* **Music and speech** (`vp_music_play`) are **streamed from disk** while they
  play: a long track costs no memory. A background thread reads ahead about a
  second and a half; if it ever falls behind, the stream goes quiet briefly and
  `vp_audio_stats.underruns` counts it.

Files are **WAV** (PCM 8, 16, 24 or 32-bit, or 32-bit float; compressed WAV
is refused with a message) or **Ogg Vorbis** (`.ogg`, about a tenth of the
size: the choice for music). The SDK tells them apart by their contents, not
their names. Mono or stereo (Ogg surround is folded to stereo), any sample
rate (converted as it plays). An effect loaded from Ogg is decoded once, at
load; Ogg music is decoded as it streams, on the feeder thread.

## Voices

Every play returns a **voice** (`vp_voice`). With it: `vp_voice_stop` (with a
fade-out), `vp_voice_set_volume` (with a fade), `vp_voice_set_pitch`,
`vp_voice_playing`. A voice handle stays valid until the sound ends; after
that, calls with it do nothing. `vp_audio_stop_all` and `vp_audio_set_master`
act on everything.

Up to **64 voices** play at once (`VP_AUDIO_VOICES`), effects and music
together. When all are busy a play returns 0 and is counted in
`vp_audio_stats.refused`.

* **Volume**: 0..1 (above 1 amplifies; the mix is clipped at full scale).
* **Pitch**: 1 is the recorded speed, 2 an octave up and twice as fast.
* Every volume change is smoothed over a few milliseconds, so stops and
  changes never click.

## No sound device

`vp_run` opens the audio. If there is no sound device, the game runs silent:
`vp_game_audio` returns NULL, and every `vp_audio` call accepts NULL and does
nothing. A game needs no special case.

## Where the files are

`vp_asset_path("sounds/zap.wav", buf, size)` gives the path next to the
running executable: the build folder while developing, the game's package
folder on the cabinet. Never rely on the current directory.

## The device

48 kHz stereo (or what the device offers), through SDL2: Windows audio on
the PC, ALSA or PipeWire on the Pi, as the cabinet is set up. The menu
releases the audio device while a game runs.
