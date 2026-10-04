/* vp_text.h — text, drawn as strokes (the Hershey Simplex font).
 *
 *   vp_color(d, 255, 255, 0);
 *   vp_text(d, "GAME OVER", 2048, 2000, 200, VP_TEXT_CENTER);
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

void  vp_text(vp_dvg *d, const char *s, float x, float y, float height, int align);

/* How wide `s` is at `height`, in board units (letter spacing included). */
float vp_text_width(const char *s, float height);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_TEXT_H */
