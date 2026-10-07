/* vp_hiscore.h — the high-score table, and entering initials.
 *
 *   int rank = vp_hiscore_rank(score);           // where it would go, or -1
 *   if (rank >= 0) vp_initials_begin(&entry, 1, 30.0f);
 *   ...
 *   if (vp_initials_update(&entry, in, dt))       // each step, until done
 *       vp_hiscore_insert(entry.name, score);     // saved at once
 *
 * THE FILE is save/highscores.txt in the game's folder (vp_save_dir), the
 * format the VectorPie menu shows in its hint bar: up to 10 lines, best
 * first, each a name of 1 to 3 characters and a score above zero:
 *
 *   MJM 125000
 *   AAA 98000
 *
 * Saving writes a new file and renames it over the old one, so a cabinet
 * switched off while saving keeps the previous table. A missing or unreadable
 * file is an empty table.
 *
 * NAMES are stored upper case, at most 3 characters, no spaces (a space or
 * any character outside A-Z 0-9 and . - becomes '.'); an empty name is "---".
 * Equal scores: the one already in the table stays ahead.
 *
 * vp_hiscore_load / rank / insert and vp_hiscore_draw need no SDL;
 * vp_initials_* read the controls (vp_input), which need the SDL build.
 */
#ifndef VECTORPIE_VP_HISCORE_H
#define VECTORPIE_VP_HISCORE_H

#include "vp_dvg.h"
#include "vp_input.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VP_HISCORE_MAX 10

typedef struct vp_score {
    char name[4];       /* NUL-terminated */
    int  score;
} vp_score;

/* The table, best first, into out[VP_HISCORE_MAX]. Returns how many (0..10). */
int vp_hiscore_load(vp_score out[VP_HISCORE_MAX]);

/* Where `score` would go in the table: 0 (the best) .. 9, or -1 if it does
 * not make it (or is not above zero). */
int vp_hiscore_rank(int score);

/* Put the score in the table and save it. Returns its rank (0..9), -1 if it
 * did not make the table, or a VP_ERR_* code below -1 if saving failed
 * (vp_last_error() says why). */
int vp_hiscore_insert(const char *name, int score);

/* The full path of the file (save/highscores.txt), for messages. */
const char *vp_hiscore_path(void);

/* Draw a table in the current font and color: one line per entry, "1. MJM
 * 125000", the first at (x, y) going down, `height` letters, lines
 * 1.6 x height apart, centered on x. Entry `highlight` (a rank, -1 none) is
 * drawn blinking, by `time` (seconds; pass the game time). */
void vp_hiscore_draw(vp_dvg *d, const vp_score *table, int count,
                     float x, float y, float height, int highlight, double time);

/* ---- entering initials: three letters, chosen with the stick ------------ */
/*
 * The player's stick up / down changes the letter (held: it repeats),
 * button 1 or right takes it and goes to the next, button 2 or left goes
 * back; after the third letter it is done. START takes the letters as they
 * are. With a time limit, the letters as they are when it runs out.
 * Letters: A-Z, 0-9, '.', '-'.
 */
typedef struct vp_initials {
    char   name[4];     /* the letters chosen so far, and the one being chosen */
    int    pos;         /* the letter being chosen: 0..2, 3 when done */
    int    player;      /* 1 or 2: whose controls */
    float  time;        /* seconds since vp_initials_begin */
    float  timeout;     /* seconds; 0 = no time limit */
    float  repeat;      /* internal: up/down auto-repeat */
} vp_initials;

/* Start entering for player 1 or 2, the first letter at A, with a time
 * limit in seconds (0 = none; arcade games use about 30). */
void vp_initials_begin(vp_initials *e, int player, float timeout_s);

/* One game step: reads the controls. Returns 1 once the initials are done
 * (e->name holds them, trailing unchosen letters dropped), else 0. */
int vp_initials_update(vp_initials *e, vp_input *in, float dt);

/* Draw the three letters centered on (x, y) with a line under each, the one
 * being chosen blinking; current font and color. */
void vp_initials_draw(const vp_initials *e, vp_dvg *d, float x, float y, float height);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_HISCORE_H */
