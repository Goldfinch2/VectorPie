# 06 — High scores and credits (`vp_hiscore`, `vp_credits`)

A top-10 table kept in the game's folder, an initials-entry screen, and a
table to show between games. `#include <vectorpie/vp.h>`.

## The file

`save/highscores.txt` next to the executable (`vp_save_dir()`, created on
first use; `vp_hiscore_path()` gives the full path). It is the format the
VectorPie menu reads for its hint bar, so a game's best scores show in the
menu with nothing else to do:

```
MJM 125000
AAA 98000
ZZ 4500
```

* Up to 10 lines, best first; a name of 1 to 3 characters, a space, a score
  above zero.
* Names are upper case, no spaces: anything outside `A-Z 0-9 . -` becomes
  `.`; an empty name is stored as `---`.
* Equal scores: the one already in the table stays ahead.
* Saving writes `highscores.txt.new`, flushes it to the disk and renames it
  over the table: a cabinet switched off mid-save keeps the old table.
* A missing or unreadable file is an empty table; malformed lines are
  skipped.

## The calls

```c
vp_score table[VP_HISCORE_MAX];
int n = vp_hiscore_load(table);          /* best first; 0..10 entries */

int r = vp_hiscore_rank(score);          /* 0..9 where it would go, -1: not in */
r = vp_hiscore_insert("MJM", score);     /* inserts and saves; its rank, -1,
                                            or VP_ERR_IO (vp_last_error) */

vp_hiscore_draw(d, table, n, 2048, 3000, 120, r, vp_game_time(g));
                                         /* "1.  MJM  125000" ... down from
                                            (x, y), entry r blinking */
```

`vp_hiscore_draw` uses the current font and color and lines up the columns
(rank, name, score) whatever the widths.

## Entering initials

Three letters chosen with the player's own controls, driven from the
game's `update` and `draw`:

```c
static vp_initials entry;
static int entering, newScore;

/* the game is over */
if (vp_hiscore_rank(score) >= 0) {
    vp_initials_begin(&entry, 1, 30.0f);  /* player 1, 30 s to finish */
    entering = 1;
    newScore = score;
}

/* update(): */
if (entering && vp_initials_update(&entry, vp_game_input(g), dt)) {
    vp_hiscore_insert(entry.name, newScore);
    entering = 0;
}

/* draw(): */
if (entering) {
    vp_text(d, "ENTER YOUR INITIALS", 2048, 2600, 110, VP_TEXT_CENTER);
    vp_initials_draw(&entry, d, 2048, 2000, 220);
}
```

| control | does |
|---|---|
| `pN_up` / `pN_down` | next / previous letter; held, it repeats |
| `pN_button1` or `pN_right` | take the letter, go to the next |
| `pN_button2` or `pN_left` | back one letter |
| `start1` / `start2` | done, with the letters so far |

Letters: `A-Z`, `0-9`, `.`, `-`. Each new letter starts on the previous
one's. With a time limit, the letters so far are taken when it runs out.
`vp_initials_draw` shows the chosen letters and the current one blinking,
each over a short line.

The file calls and `vp_hiscore_draw` need no SDL; `vp_initials_*` reads the
controls, so it needs the SDL build (the default).

## Coins, credits and the START lamps (`vp_credits`)

```c
static vp_credits credits;          /* vp_credits_init(&credits) in init */

/* update(), every step: */
if (vp_credits_update(&credits, in))           /* coin1 / coin2: +1 credit each */
    vp_sound_play(audio, coinSound, 1, 1, 0);

if (waitingForStart) {
    vp_credits_blink(&credits, vp_game_time(g));    /* START 1 blinks with a credit,
                                                       START 2 too with two */
    if (vp_input_pressed(in, "start1") && vp_credits_take(&credits, 1)) begin(1);
    if (vp_input_pressed(in, "start2") && vp_credits_take(&credits, 2)) begin(2);
} else {
    vp_lamps_set(0);
}
```

* A game for N players takes N credits; credits stop counting at `max`
  (99). `free_play = 1`: START needs no credit.
* `vp_credits_enough(&credits, n)` tells the attract screen what to say:
  INSERT COIN, PRESS 1 PLAYER START, PRESS 1 OR 2 PLAYER START.

**The lamps** are the keyboard LEDs, the cabinet convention AdvanceMAME and
the VectorPie menu share: NUM LOCK lights START 1, CAPS LOCK lights START 2
(the menu passes them to the panel while a game runs). `vp_lamps_set(mask)`
(`VP_LAMP_START1 | VP_LAMP_START2`) writes only on a change; when the game
ends the LEDs are put back as they were (`vp_run` calls `vp_lamps_close`).
Linux only (the keyboards under `/dev/input`); on Windows the calls do
nothing.
