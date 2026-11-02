/* vp_saver.h — screensaver apps: games the cabinet runs instead of its own
 * screensaver, drawing to the cabinet's music.
 *
 *   vp_music_feed m;
 *   if (vp_saver_music(&m)) {                   // in update() or draw()
 *       for (int b = 0; b < VP_MUSIC_BANDS; b++) draw_bar(b, m.bands[b]);
 *       if (m.beats != last_beats) { flash(m.beat_in_bar == 1); last_beats = m.beats; }
 *   }
 *
 * A screensaver is a package like a game, made with `kind = screensaver` in
 * its game.ini: vp-pack then makes a .vps (not a .vpk), which the cabinet
 * installs apart from its games and offers in its SCREENSAVER setting.
 *
 * ON THE CABINET the menu keeps playing its music (its playlist, DJ clips and
 * light show go on) and shares, about 60 times a second, what is playing: the
 * track, its beats from the track's beat map and a spectrum of its sound
 * (vp_music_feed.h). Any control wakes the cabinet: vp_run quits; START 2
 * asks for the next track instead (vp_saver_skip). vp_run keeps the picture
 * moving, so no line burns in: all of it, a little smaller, wanders slowly
 * round the screen (vp_dvg_set_view).
 *
 * ON A PC (no menu) the SDK stands in: give the game a music file (Ogg or
 * WAV) or a folder of them on its command line, or in VP_SAVER_MUSIC; it
 * plays them in turn and analyses what it plays, with the beat map beside
 * each track (<track>.beats) when there is one. Esc quits.
 */
#ifndef VECTORPIE_VP_SAVER_H
#define VECTORPIE_VP_SAVER_H

#include "vp_music_feed.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 1 when the cabinet started this game as its screensaver (VP_SAVER=1). */
int  vp_saver_active(void);

/* What is playing now (vp_music_feed.h): 1, else 0 (nothing playing, no
 * feed). Call it once per update or draw. */
int  vp_saver_music(vp_music_feed *out);

/* Ask for the next track. */
void vp_saver_skip(void);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_SAVER_H */
