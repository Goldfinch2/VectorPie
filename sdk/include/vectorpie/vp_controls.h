/* vp_controls.h — a game's own controls, and laying them out in its menu.
 *
 * A game may name its own functions ("fire", "smart_bomb", "thrust") and let
 * the player lay out the controls for each. Each function has a STANDARD:
 * controls written by the game, usually the cabinet's advmame.rc actions, so
 * out of the box the game follows the owner's setup:
 *
 *   vp_input_define(in, "fire",  "p1_button1");
 *   vp_input_define(in, "smart", "p1_button2+key_Space");
 *   vp_input_load(in, NULL);                     (the player's own, if saved)
 *   ...
 *   if (vp_input_pressed(in, "fire")) fire();    (as for any action)
 *
 * CONTROLS AS TEXT. A function's controls are text, '+' between them, at
 * most VP_CONTROLS_MAX of the player's own:
 *
 *   key_Left_Ctrl        a key: "key_" and SDL's name, '_' for spaces
 *   joy1_button3         a joystick's button (joysticks and buttons from 1)
 *   yoke_button3         the same on the yoke: the first joystick whose
 *                        name says "yoke", whatever its number
 *   joy1_hat1_up         a hat's way: up, down, left, right
 *   joy1_axis2_neg       an axis past half its travel: neg or pos
 *   mouse_left           a mouse button: left, middle, right, x1, x2
 *   p1_button1           an advmame.rc action: the controls the cabinet
 *                        gives it (advmame.rc, else AdvanceMAME's default)
 *   standard             the function's standard
 *   none                 no control
 *
 * The cabinet's own controls stay the cabinet's: START, COIN and the way out
 * (ui_cancel) are never laid out, and neither are F2, Esc, Enter and Delete.
 *
 * MENUS never go by what is laid out: they have fixed controls that work
 * whatever a function has (the arrow keys, any joystick's hat or stick,
 * START 1, Enter and the keypad's Enter: vp_input_menu_*), so a player can
 * never lock themselves out.
 *
 * LAYING OUT (vp_capture): START 1 or Enter on a function's row, then one
 * control. The first since the cursor came to the row replaces what the
 * function had; every further one on the row is added (the oldest goes past
 * VP_CONTROLS_MAX). While laying out, START 1 / Enter leave it as it was,
 * START 2 makes it none, Delete (also the keypad's) its standard. Left and
 * right on the row switch between none and the standard. A control may be
 * on several functions at once.
 *
 * Requires the SDK built with SDL2 (the default).
 */
#ifndef VECTORPIE_VP_CONTROLS_H
#define VECTORPIE_VP_CONTROLS_H

#include <stddef.h>
#include "vp_input.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VP_CONTROLS_LENGTH 128      /* a function's controls as text, at most */
#define VP_CONTROLS_MAX    4        /* controls one function may have */

/* ---- a game's functions ------------------------------------------------- */

/* Name a function and its standard controls; it starts on its standard.
 * Read it with vp_input_held / _pressed / _released like any action.
 * Defining it again replaces it. */
int vp_input_define(vp_input *in, const char *action, const char *standard);

/* The player's controls for a function: text as above, "none", or NULL (or
 * "standard") for its standard. VP_ERR_NOTFOUND if the game did not define
 * it. Controls the SDK does not know are skipped. */
int vp_input_set(vp_input *in, const char *action, const char *controls);

/* A function's controls as the player set them ("standard", "none", or
 * controls), or NULL if the game did not define it. Valid until the next
 * vp_input_set or vp_input_define of it. */
const char *vp_input_controls(const vp_input *in, const char *action);

/* "LEFT CTRL + JOY1 BUTTON 3": controls for the screen. advmame.rc actions
 * and "standard" (of `action`, which may be NULL) are spelled as the
 * controls they stand for. "NONE" when there are none. Returns buf. */
const char *vp_input_spelled(const vp_input *in, const char *action, const char *controls,
                             char *buf, size_t size);

/* Keep the player's controls: one "function = controls" line for each
 * function the game defined. `path` NULL = vp_save_dir() "/controls.ini".
 * Load sets each defined function the file names (others are ignored);
 * VP_ERR_NOTFOUND if the file does not exist. */
int vp_input_save(const vp_input *in, const char *path);
int vp_input_load(vp_input *in, const char *path);

/* ---- the menus' fixed controls ------------------------------------------ */

int vp_input_menu_confirm(const vp_input *in);       /* START 1, Enter, keypad Enter: down since the last update */
int vp_input_menu_confirm_held(const vp_input *in);
int vp_input_menu_up(const vp_input *in);            /* down since the last update */
int vp_input_menu_down(const vp_input *in);
int vp_input_menu_left(const vp_input *in);          /* held */
int vp_input_menu_right(const vp_input *in);
int vp_input_menu_left_pressed(const vp_input *in);  /* down since the last update */
int vp_input_menu_right_pressed(const vp_input *in);
int vp_input_menu_way_x(const vp_input *in);         /* held: -1 left, 1 right, 0 */
int vp_input_menu_way_y(const vp_input *in);         /* held: -1 up, 1 down, 0 */

/* A key by SDL scancode (SDL_SCANCODE_TAB is 43...). */
int vp_input_key_held(const vp_input *in, int scancode);
int vp_input_key_pressed(const vp_input *in, int scancode);   /* down since the last update */

/* A key the game keeps for itself besides the usual (TAB for its menu...):
 * never laid out. */
void vp_input_reserve_key(vp_input *in, int scancode);

/* ---- the joysticks as they are ------------------------------------------ */

/* The yoke's number (from 0), or -1. */
int vp_input_yoke(const vp_input *in);
/* The first joystick that is not the yoke and has a stick (axes that rest
 * in their middle, not pedals), or -1. */
int vp_input_stick(const vp_input *in);
/* A joystick's axis (both from 0), -1 .. 1, no dead zone. */
float vp_input_joy_axis(const vp_input *in, int joystick, int axis);

/* ---- laying out a function's controls ----------------------------------- */

typedef struct vp_capture {         /* all zero to begin with */
    int   capturing, armed, adding, let_go;
    char  action[48];
    char  taken[VP_CONTROLS_LENGTH];
    int   count;
    int   said_frames;              /* frames left of saying `said` */
    char  said[64];
    short rest[8][16];              /* the joysticks' axes when it began */
} vp_capture;

enum { VP_CAPTURE_WAITING, VP_CAPTURE_CHANGED, VP_CAPTURE_LEFT };

/* The cursor went to another row (or page): the next control replaces. */
void vp_capture_moved(vp_capture *c);
/* START 1 / Enter on a function's row (vp_input_menu_confirm). */
void vp_capture_begin(vp_capture *c, const vp_input *in, const char *action);
/* Each update while c->capturing: VP_CAPTURE_CHANGED when the function's
 * controls were set (vp_input_set done: save them), VP_CAPTURE_LEFT when it
 * was left as it was, else VP_CAPTURE_WAITING. */
int  vp_capture_step(vp_capture *c, vp_input *in);
/* Delete on the row: the function's standard. */
void vp_capture_standard(vp_capture *c, vp_input *in, const char *action);
/* Left or right on the row: none, or the function's standard. */
void vp_capture_toggle(vp_capture *c, vp_input *in, const char *action);
/* Left and right as the menu should take them: after a control was laid
 * out they count only once they have been let go. */
void vp_capture_filter(vp_capture *c, int *left, int *right);
/* What the menu should say under its rows now, or NULL. Once an update. */
const char *vp_capture_said(vp_capture *c);
/* Help for the bottom of the screen, two lines: while laying out, or on a
 * function's row. */
void vp_capture_help(const vp_capture *c, const char **one, const char **two);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_CONTROLS_H */
