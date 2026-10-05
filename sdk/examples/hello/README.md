# hello

A small complete game on the VectorPie SDK: shoot the falling diamonds.
It uses the game loop (`vp_run`), the cabinet's controls (`vp_input`),
stroke text (`vp_text`) and sound (`vp_audio`).

Controls (AdvanceMAME's defaults, or your advmame.rc): P1 stick or arrows,
P1 button 1 (Left Ctrl) to fire, START 1 (key 1) to play, the exit button
(Esc) to quit.

## Sounds

Out of the box, `hello` makes its own sounds in code (`vp_sound_from_samples`).

To try real WAV files instead, including a **streamed** music loop, put three
files in `examples/hello/assets/sounds/`:

| file | used for |
|---|---|
| `shipfire.wav` | firing |
| `explosn.wav` | a hit (and, lower, a miss) |
| `music.wav` | the background loop, streamed from disk (`vp_music_play`): any WAV track, the longer the better |
| `bg1.wav` | the background loop when there is no `music.wav` |

These names match Rip Off's MAME sample set. From a cabinet that has it
(`/usr/local/share/advance/sample/ripoff.zip`):

```sh
mkdir -p examples/hello/assets/sounds
unzip -j -o /mnt/x/usr/local/share/advance/sample/ripoff.zip \
      shipfire.wav explosn.wav bg1.wav -d examples/hello/assets/sounds
cmake --preset pc       # configure again: the build copies assets/ next to the executable
cmake --build --preset pc
```

The folder is ignored by git: arcade sample sets are not ours to publish.
On the cabinet, copy `assets/` next to the `hello` executable.
