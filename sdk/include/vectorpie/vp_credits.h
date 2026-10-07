/* vp_credits.h — coins, credits, and the START buttons' lamps.
 *
 *   static vp_credits credits;              // vp_credits_init(&credits) in init
 *
 *   update():
 *     if (vp_credits_update(&credits, in))  // a coin went in
 *         vp_sound_play(audio, coinSound, 1, 1, 0);
 *     if (attract) {
 *         vp_credits_blink(&credits, vp_game_time(g));   // START 1 / 2 blink
 *         if (vp_input_pressed(in, "start1") && vp_credits_take(&credits, 1)) start(1);
 *         if (vp_input_pressed(in, "start2") && vp_credits_take(&credits, 2)) start(2);
 *     } else {
 *         vp_lamps_set(0);
 *     }
 *
 * COINS: coin1 and coin2 (advmame.rc actions) each add a credit, up to
 * `max` (99 by default). A game for N players takes N credits.
 *
 * LAMPS: the cabinet's START buttons light through the keyboard LEDs, as in
 * AdvanceMAME and the VectorPie menu: NUM LOCK is START 1, CAPS LOCK is
 * START 2 (the menu passes them to the panel's lamps while a game runs).
 * vp_lamps_set writes only when the state changes; the LEDs are put back as
 * they were when the game ends (vp_run does it; otherwise vp_lamps_close).
 * On Linux they are the keyboards' LEDs (/dev/input, the user needs write
 * access, as the cabinet's pi user has); elsewhere the calls do nothing.
 *
 * vp_credits_update needs the controls (vp_input), so the SDL build; the
 * lamps need no SDL.
 */
#ifndef VECTORPIE_VP_CREDITS_H
#define VECTORPIE_VP_CREDITS_H

#include "vp_input.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the START lamps ------------------------------------------------------ */

enum {
    VP_LAMP_START1 = 1,     /* NUM LOCK */
    VP_LAMP_START2 = 2      /* CAPS LOCK */
};

void vp_lamps_set(unsigned mask);   /* VP_LAMP_* together; 0 = both off */
void vp_lamps_close(void);          /* back as they were */

/* ---- credits ------------------------------------------------------------- */

typedef struct vp_credits {
    int credits;            /* credits in the machine */
    int max;                /* coins beyond this are not counted (99) */
    int free_play;          /* 1: START needs no credit (and none is taken) */
} vp_credits;

void vp_credits_init(vp_credits *c);

/* Read coin1 / coin2: returns how many coins went in this step (0 mostly). */
int vp_credits_update(vp_credits *c, vp_input *in);

/* Can a game for `players` start? */
int vp_credits_enough(const vp_credits *c, int players);

/* Start a game for `players`: takes the credits and returns 1, or returns 0
 * when there are not enough. */
int vp_credits_take(vp_credits *c, int players);

/* The lamps for a game waiting to start: START 1 blinks with a credit,
 * START 2 too with two (both, in free play); off without. `time` in
 * seconds (the game time), twice a second. */
void vp_credits_blink(const vp_credits *c, double time);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_CREDITS_H */
