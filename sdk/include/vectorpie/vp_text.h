/* vp_text.h — text, drawn as strokes.
 *
 *   vp_color(d, 255, 255, 0);
 *   vp_text(d, "GAME OVER", 2048, 2000, 200, VP_TEXT_CENTER);
 *
 * Two fonts (vp_text_font):
 *   VP_FONT_HERSHEY    Hershey Simplex: upper and lower case, punctuation,
 *                      round letters (the default)
 *   VP_FONT_ASTEROIDS  Atari's Asteroids lettering: capitals only (lowercase
 *                      is drawn as capitals), few strokes per letter, the
 *                      classic arcade look
 *
 * Text is drawn with the current color through vp_moveto / vp_lineto, each
 * letter in an order chosen to keep the beam's invisible moves short.
 *
 * `height` is the height of a capital letter in board units. (x, y) is on
 * the baseline: the left end, the center or the right end per `align`.
 * Printable ASCII only; anything else is drawn as a space.
 *
 * Needs no SDL: available in every build.
 */
#ifndef VECTORPIE_VP_TEXT_H
#define VECTORPIE_VP_TEXT_H

#include "vp_dvg.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    VP_TEXT_LEFT = 0,
    VP_TEXT_CENTER,
    VP_TEXT_RIGHT
};

typedef enum vp_font {
    VP_FONT_HERSHEY = 0,
    VP_FONT_ASTEROIDS
} vp_font;

/* The font the following vp_text / vp_text_width calls use (this thread). */
void    vp_text_font(vp_font font);
vp_font vp_text_get_font(void);

void  vp_text(vp_dvg *d, const char *s, float x, float y, float height, int align);

/* The same, kept within `width` board units: a line that would be wider is
 * drawn with narrower letters (the same height), not smaller ones; a shorter
 * one as vp_text draws it. For text that must fill a given box (a re-created
 * game's labels). width <= 0: no limit. */
void  vp_text_fit(vp_dvg *d, const char *s, float x, float y, float height, float width, int align);
/* How wide `s` is at `height`, in board units (letter spacing included). */
float vp_text_width(const char *s, float height);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_TEXT_H */
