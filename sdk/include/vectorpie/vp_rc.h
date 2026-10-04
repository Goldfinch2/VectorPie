/* vp_rc.h — reading the cabinet's advmame.rc.
 *
 * On VectorPie, advmame.rc is the one place the cabinet's controls and DVG
 * port are configured: AdvanceMAME, the menu and the native games all read
 * it, so a game that reads it too follows whatever the owner has set up.
 *
 * This layer only reads. It gives the raw value of any key, and the
 * input_map[...] bindings decoded into vp_binding records. Turning bindings
 * into live input state is the input layer's job (vp_input, phase 3).
 *
 * FILE LOCATION (vp_rc_default_path):
 *   1. the VP_ADVMAME_RC environment variable
 *   2. $HOME/.advance/advmame.rc
 *   3. /home/pi/.advance/advmame.rc (VectorPie's standard location; Linux only)
 */
#ifndef VECTORPIE_VP_RC_H
#define VECTORPIE_VP_RC_H

#include <stddef.h>
#include "vp_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vp_rc vp_rc;

typedef enum vp_binding_type {
    VP_BINDING_KEY = 1,        /* keyboard[dev,key] */
    VP_BINDING_JOY_BUTTON,     /* joystick_button[dev,button] */
    VP_BINDING_JOY_AXIS_NEG,   /* joystick_digital[dev,stick,axis,up|left] */
    VP_BINDING_JOY_AXIS_POS,   /* joystick_digital[dev,stick,axis,down|right] */
    VP_BINDING_MOUSE_BUTTON,   /* mouse_button[dev,button] */
    VP_BINDING_MOUSE_AXIS      /* mouse[dev,x|y]: a spinner or trackball */
} vp_binding_type;

typedef struct vp_binding {
    vp_binding_type type;
    /* The device as advmame.rc names it: a number ("0") or a normalized
     * device name ("atari_controls"). Matched to a real device later. */
    char device[64];
    /* VP_BINDING_KEY: AdvanceMAME's key name ("lcontrol", "1_pad"). */
    char key[24];
    /* Buttons: 0-based button index (advmame "button1" = 0; joystick names
     * like "trigger" and mouse names "left", "right", "middle", "side",
     * "extra" map to 0..4).
     * Joystick axes: axis index. Mouse axes: 0 = x, 1 = y. */
    int index;
} vp_binding;

/* The path vp_rc_load(NULL, ...) reads. VP_ERR_NOTFOUND if none applies. */
int vp_rc_default_path(char *path, size_t path_size);

/* Load and parse. `path` NULL = vp_rc_default_path. VP_ERR_NOTFOUND if the
 * file does not exist. */
int vp_rc_load(const char *path, vp_rc **out);
void vp_rc_free(vp_rc *rc);

/* The path actually loaded. */
const char *vp_rc_path(const vp_rc *rc);

/* Raw value of a "key value" line, whitespace-trimmed, or NULL if absent.
 * Works for every key, input_map[...] included. When a key appears more than
 * once, the last line wins. */
const char *vp_rc_get(const vp_rc *rc, const char *key);
/* "yes"/"true"/"1" -> 1, "no"/"false"/"0" -> 0, anything else -> def. */
int vp_rc_get_bool(const vp_rc *rc, const char *key, int def);
int vp_rc_get_int(const vp_rc *rc, const char *key, int def);

/* The input_map actions present in the file ("p1_up", "coin1", ...), in file
 * order. Actions set to "auto" or left empty are not listed. */
int vp_rc_action_count(const vp_rc *rc);
const char *vp_rc_action_name(const vp_rc *rc, int i);

/* The bindings of one action, "or" alternatives in file order. 0 if the
 * action is absent. Parts the SDK does not understand are skipped; a
 * combination ("keyboard[0,a] keyboard[0,b]") is read as its first element. */
int vp_rc_binding_count(const vp_rc *rc, const char *action);
int vp_rc_binding(const vp_rc *rc, const char *action, int i, vp_binding *out);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_RC_H */
