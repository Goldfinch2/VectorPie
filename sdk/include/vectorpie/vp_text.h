/* vp_text.h — text, drawn as strokes.
 *
 *   vp_color(d, 255, 255, 0);
 *   vp_text(d, "GAME OVER", 2048, 2000, 200, VP_TEXT_CENTER);
 *
 * Three fonts (vp_text_font):
 *   VP_FONT_HERSHEY    Hershey Simplex: upper and lower case, punctuation,
 *                      round letters (the default)
 *   VP_FONT_ASTEROIDS  Atari's Asteroids lettering: capitals only (lowercase
 *                      is drawn as capitals), few strokes per letter, the
 *                      classic arcade look
 *   VP_FONT_COMIC      fat, rounded cartoon capitals (Titan One), each letter
 *                      drawn as its outline, with a border round it if
 *                      vp_text_border sets one, bouncing if vp_text_wobble
 *                      says so: titles and big messages. Capitals, digits and
 *                      ! ? ' - . : , + / ( ) & * = < > # (lowercase is drawn
 *                      as capitals). Many
 *                      strokes per letter: keep it big and short. In a 3D
 *                      scene, vp3d_scene_text makes it hide what is behind it.
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
    VP_FONT_ASTEROIDS,
    VP_FONT_COMIC
} vp_font;

/* The font the following vp_text / vp_text_width calls use (this thread). */
void    vp_text_font(vp_font font);
vp_font vp_text_get_font(void);

/* VP_FONT_COMIC: a border round each letter, in this color (the letters
 * themselves take the current color). 0, 0, 0: none  (the default). This
 * thread's following calls. */
void vp_text_border(uint8_t r, uint8_t g, uint8_t b);
/* VP_FONT_COMIC: each letter bounces and tilts a little, `amount` 0 (still,
 * the default) to 1 (lively), moving with `time` (seconds: pass the game
 * time). This thread's following calls. */
void vp_text_wobble(float amount, double time);

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
