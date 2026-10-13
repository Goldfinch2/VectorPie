/* vp_input.h — the cabinet's controls, as AdvanceMAME names them.
 *
 * A game asks for ACTIONS by their advmame.rc names, not for keys or buttons:
 *
 *   if (vp_input_pressed(in, "p1_button1")) fire();
 *   float steer = vp_input_axis(in, "p1_stickx");
 *
 * Each action's bindings come from the cabinet's advmame.rc (the same file the
 * menu edits and AdvanceMAME reads), so a game follows the owner's controls
 * with no settings screen of its own. Where advmame.rc says "auto", or has no
 * line for an action, or there is no advmame.rc at all (a PC), the action gets
 * AdvanceMAME's own default: see the table in docs/03-game-layer.md.
 *
 * ACTIONS
 *   digital  p1_up p1_down p1_left p1_right p1_button1 .. p1_button6
 *            the same for p2_ (p2 defaults: R F D G, A S Q W, joystick 2)
 *            start1 start2 coin1 coin2 ui_cancel
 *   analog   p1_stickx p1_sticky p2_stickx p2_sticky (sticks, -1..1)
 *   relative p1_dialx p1_trackballx p1_trackbally (spinner / trackball counts)
 * Any other input_map action in advmame.rc works too, by its name.
 *
 * TIMING. vp_input_update reads every device once; pressed/released compare
 * with the previous update. vp_run (vp_app.h) calls it before each game step.
 *
 * KEYBOARD. Keys need SDL's video subsystem, which vp_input_open starts
 * (on the Pi: KMS/DRM). On a PC driving a board with no window, Windows sends
 * keys only to a focused window: use a gamepad there, or the window output.
 *
 * Requires the SDK built with SDL2 (the default).
 */
#ifndef VECTORPIE_VP_INPUT_H
#define VECTORPIE_VP_INPUT_H

#include "vp_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vp_input vp_input;

/* Load the bindings (advmame.rc from vp_rc_default_path, or none) and open
 * the joysticks. */
int  vp_input_open(vp_input **out);
void vp_input_close(vp_input *in);

/* Read every device. Once per game step. Joysticks plugged in or out are
 * picked up here. */
void vp_input_update(vp_input *in);

/* Digital actions. 1 or 0. An unknown action is never held. */
int vp_input_held(const vp_input *in, const char *action);
int vp_input_pressed(const vp_input *in, const char *action);    /* down since the last update */
int vp_input_released(const vp_input *in, const char *action);   /* up since the last update */

/* Analog actions: -1 .. 1, centered 0, with the dead zone applied (left/up
 * negative, as the stick reports it). Several bindings add up, clamped. */
float vp_input_axis(const vp_input *in, const char *action);

/* Relative actions (spinner, trackball, mouse): counts moved since the last
 * update, signed. */
int vp_input_delta(const vp_input *in, const char *action);

/* Capture the mouse (SDL relative mode): the pointer is hidden and held,
 * so mouse, spinner and trackball movement keeps counting however far it
 * goes (otherwise it stops at the screen's edge). For spinner and
 * trackball games. Released by vp_input_close. 0 releases it. */
void vp_input_grab_mouse(vp_input *in, int grab);

/* The stick dead zone, as a fraction of full travel. Default 0.15. */
void vp_input_set_deadzone(vp_input *in, float fraction);

/* Joysticks currently open. */
int vp_input_joystick_count(const vp_input *in);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_INPUT_H */
