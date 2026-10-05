/* vp_app.h — the game loop.
 *
 * vp_run owns main's loop so every game behaves the same on the cabinet:
 *
 *   static void update(vp_game *g, float dt) { ... move things by dt ... }
 *   static void draw(vp_game *g, vp_dvg *d)  { ... vp_color / vp_moveto / vp_lineto ... }
 *
 *   int main(int argc, char **argv)
 *   {
 *       vp_game_desc desc = {0};
 *       desc.title  = "My Game";
 *       desc.update = update;
 *       desc.draw   = draw;
 *       return vp_run(&desc, argc, argv);
 *   }
 *
 * WHAT IT DOES
 *   - opens the output (vp_dvg_open_auto: the board, or a window), the
 *     controls (vp_input) and the sound (vp_audio; a game still runs when
 *     there is no sound device: vp_game_audio is then NULL, which every
 *     vp_audio call accepts);
 *   - calls update at a FIXED rate (update_hz, default 60), reading the
 *     controls before each call, so game logic does not depend on the frame
 *     rate; a late frame is caught up with at most 5 steps;
 *   - calls draw once per frame (frame_hz, default 40) between
 *     vp_dvg_begin_frame and vp_dvg_end_frame;
 *   - stops on ui_cancel (back to the menu), on a closed window, on Ctrl+C,
 *     or when the game calls vp_game_quit; then calls shutdown and closes
 *     everything (the board gets its exit sequence).
 *
 * COMMAND LINE (for development; the menu passes none):
 *   --dvg [PORT]   the vector board      --window     a fullscreen window
 *   --windowed     a normal window       --vertical   a 3:4 monitor
 * Arguments that do not start with '-' are left to the game (read argv
 * yourself before or after vp_run).
 *
 * vp_run owns SDL's event queue: events are read and discarded each frame.
 * Read the controls through vp_input.
 */
#ifndef VECTORPIE_VP_APP_H
#define VECTORPIE_VP_APP_H

#include "vp_dvg.h"
#include "vp_input.h"
#include "vp_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vp_game vp_game;

typedef struct vp_game_desc {
    const char *title;          /* window title */
    void       *user;           /* yours: vp_game_user() returns it */

    /* All optional. init returning anything but VP_OK stops the game. */
    int  (*init)(vp_game *g);
    void (*update)(vp_game *g, float dt);   /* dt = 1 / update_hz, in seconds */
    void (*draw)(vp_game *g, vp_dvg *d);
    void (*shutdown)(vp_game *g);

    int update_hz;              /* 0 = 60 */
    int frame_hz;               /* 0 = 40 */
} vp_game_desc;

/* Run the game. Returns 0 on a normal exit, 1 if something failed (the
 * reason is printed to stderr). */
int vp_run(const vp_game_desc *desc, int argc, char **argv);

void     *vp_game_user(vp_game *g);
vp_input *vp_game_input(vp_game *g);
vp_audio *vp_game_audio(vp_game *g);   /* NULL when there is no sound device */
vp_dvg   *vp_game_output(vp_game *g);
double    vp_game_time(vp_game *g);     /* seconds of game time (update steps x dt) */
void      vp_game_quit(vp_game *g);     /* stop after this step */

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_APP_H */
