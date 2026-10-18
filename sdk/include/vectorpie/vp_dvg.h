/* vp_dvg.h — the device layer: frames to a vector board or to a window.
 *
 * OUTPUT. A handle sends its frames to ONE output: the vector board (USB-DVG
 * or BeamForge, same protocol), or a window (SDL2) on a PC or on the Pi. The
 * window shows exactly what the board would be sent: the same coordinates,
 * colors and clipping, as plain lines. vp_dvg_open_auto picks the output the
 * way a VectorPie game should.
 *
 * DRAWING is the beam's own model: three calls.
 *   vp_color(d, r, g, b)   the color of the lines that follow
 *   vp_moveto(d, x, y)     move the beam, off
 *   vp_lineto(d, x, y)     draw a line from the beam to (x, y)
 * The SDK draws exactly what is asked, in the order asked: nothing is
 * reordered or merged. Draw connected lines one after the other: a lineto
 * that starts where the beam already is costs one word; every jump costs an
 * extra invisible move.
 *
 * COORDINATES. 0..VP_DVG_COORD_MAX on both axes, origin bottom-left, Y up.
 * The monitor stretches this square to its tube (4:3, or 3:4 vertical).
 * Lines are clipped to the clip region (the full range by default).
 *
 * COLOR. 8-bit R, G, B. There is no separate intensity: a dimmer line is a
 * darker color. A black lineto draws nothing (it is a move).
 *
 * A FRAME is vp_dvg_begin_frame, the drawing calls, vp_dvg_end_frame. The
 * board redraws the last frame until the next one arrives, but it goes back to
 * its home screen if no data arrives for its BURN-IN time (a board menu
 * setting, 2 seconds by default): keep sending frames.
 *
 * Fitting a frame on the hardware is the game's job: the board draws an
 * overloaded frame faster and coarser. The counters (vp_dvg_get_stats) show
 * what each frame costs.
 *
 * A handle is not thread-safe: use it from one thread.
 */
#ifndef VECTORPIE_VP_DVG_H
#define VECTORPIE_VP_DVG_H

#include <stddef.h>
#include <stdint.h>
#include "vp_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VP_DVG_COORD_MAX    4095    /* 12 bits per axis, both boards */
#define VP_DVG_MAX_VECTORS  65536   /* lines per frame; past it, counted in `dropped` */

typedef struct vp_dvg vp_dvg;

typedef enum vp_dvg_output {
    VP_DVG_OUTPUT_BOARD = 1,   /* the vector board */
    VP_DVG_OUTPUT_WINDOW       /* a window (SDL2) */
} vp_dvg_output;

/* What is drawing (vp_dvg_hardware). */
typedef enum vp_hardware {
    VP_HW_WINDOW = 0,
    VP_HW_USB_DVG,             /* also the answer when a board does not say */
    VP_HW_BEAMFORGE
} vp_hardware;

/* How to open a handle (vp_dvg_open_ex). Zero-initialize, then set what you
 * need: `vp_dvg_options o = {0}; o.output = VP_DVG_OUTPUT_WINDOW;`. */
typedef struct vp_dvg_options {
    vp_dvg_output output;
    /* board */
    const char *port;           /* NULL = vp_dvg_resolve_port */
    /* window */
    const char *window_title;   /* NULL = "VectorPie" */
    int         fullscreen;     /* 1 = fill the screen (shape kept, letterboxed) */
    int         window_width;   /* 0 = 1024 (768 when vertical) */
    int         window_height;  /* 0 = 768 (1024 when vertical) */
    int         vertical;       /* 1 = a vertically mounted monitor (3:4) */
    int         show_blanks;    /* 1 = draw the invisible moves, dimly */
    int         maximized;      /* 1 = a normal window, opened maximized (not fullscreen) */
} vp_dvg_options;

/* Counters. The "last frame" fields describe the most recent vp_dvg_end_frame;
 * the totals run from opening. They count the same on every output. */
typedef struct vp_dvg_stats {
    /* last frame */
    uint32_t vectors;        /* lines drawn (after clipping) */
    uint32_t blank_moves;    /* invisible moves between lines */
    uint32_t words;          /* 4-byte words encoded, end-of-frame included */
    uint32_t clipped_away;   /* lines entirely outside the clip region */
    uint32_t dropped;        /* lines over VP_DVG_MAX_VECTORS: NOT drawn */
    uint32_t lit_length;     /* total length of the lines, board units */
    uint32_t blank_length;   /* total length of the invisible moves, board units */
    /* totals */
    uint64_t frames_sent;
    uint64_t frames_with_drops;
    uint64_t bytes_sent;     /* board only */
    uint32_t write_errors;   /* board only */
} vp_dvg_stats;

/* Choose the output the way a VectorPie game should, filling `options`:
 *   1. the VP_OUTPUT environment variable (the menu sets it when it launches
 *      a game, from its own type, as for bzone2_dvg / bzone2_sdl): "dvg" (the
 *      board) or "sdl" (a fullscreen window: the HDMI screen on the Pi)
 *   2. otherwise advmame.rc: vector_aux_renderer "dvg" -> the board
 *   3. otherwise (the DVG is disabled, or there is no advmame.rc, as on a
 *      PC) -> a fullscreen window
 * VP_VERTICAL=1 in the environment selects the vertical (3:4) shape.
 * `reason`, if not NULL, receives a short text saying which rule applied.
 * `title` is the window title (kept by pointer, not copied). */
void vp_dvg_auto_options(vp_dvg_options *options, const char *title,
                         const char **reason);

/* vp_dvg_auto_options + vp_dvg_open_ex. */
int vp_dvg_open_auto(const char *title, vp_dvg **out);

/* Open with explicit options.
 * Window: on a Pi (Linux with no X11/Wayland display) it draws through SDL's
 * kmsdrm driver, always full screen. VP_ERR_NODEV if the SDK was built without
 * SDL2.
 * Board: asks the board what it is (GET_DVG_INFO, once, up to 1 s) before
 * the first frame; see vp_dvg_hardware. */
int vp_dvg_open_ex(const vp_dvg_options *options, vp_dvg **out);

/* Open the board. `port` is a device path ("/dev/ttyACM0") or a COM port name
 * ("COM5"); NULL resolves it as vp_dvg_resolve_port does. */
int vp_dvg_open(const char *port, vp_dvg **out);

/* Where the board is, without opening anything:
 *   1. the VP_DVG_PORT environment variable
 *   2. vector_aux_renderer_port in advmame.rc (see vp_rc.h for its location)
 *   3. "/dev/ttyACM0" on Linux; on Windows there is no default (VP_ERR_NODEV)
 * `source`, if not NULL, receives "VP_DVG_PORT", "advmame.rc" or "default". */
int vp_dvg_resolve_port(char *port, size_t port_size, const char **source);

/* Board: send the exit sequence (the board returns to its home screen).
 * Then close. NULL is allowed. */
void vp_dvg_close(vp_dvg *dvg);

vp_dvg_output vp_dvg_get_output(const vp_dvg *dvg);

/* What is drawing. For a board, from the productName it reported when opened
 * ("BeamForge" -> VP_HW_BEAMFORGE); a board that did not answer is taken to be
 * a USB-DVG. For the game's information only: the SDK drives both boards the
 * same way. */
vp_hardware vp_dvg_hardware(const vp_dvg *dvg);
const char *vp_hardware_name(vp_hardware hw);   /* "window", "USB-DVG", "BeamForge" */

/* The port of a board handle, "" for a window. */
const char *vp_dvg_port(const vp_dvg *dvg);

/* Window: 1 once the user has closed it (SDL_QUIT is pending). Board: 0.
 * The SDK never removes events from SDL's queue: a program running its own
 * SDL event loop still receives every event, SDL_QUIT included. */
int vp_dvg_window_closed(vp_dvg *dvg);

/* Clip region, inclusive, in board coordinates. Default: the full range. */
void vp_dvg_set_clip(vp_dvg *dvg, int x0, int y0, int x1, int y1);
void vp_dvg_reset_clip(vp_dvg *dvg);

void vp_dvg_begin_frame(vp_dvg *dvg);

void vp_color(vp_dvg *dvg, uint8_t r, uint8_t g, uint8_t b);
void vp_get_color(const vp_dvg *dvg, uint8_t *r, uint8_t *g, uint8_t *b);
void vp_moveto(vp_dvg *dvg, int x, int y);
void vp_lineto(vp_dvg *dvg, int x, int y);

/* Encode the frame and send it. `monochrome` sets the board's monochrome-frame
 * flag. Board: blocks until the data has been handed to the USB driver. */
int vp_dvg_end_frame(vp_dvg *dvg, int monochrome);

void vp_dvg_get_stats(const vp_dvg *dvg, vp_dvg_stats *stats);

/* Save the last frame sent (vp_dvg_end_frame) as a PNG picture, its lines on
 * black, the board square stretched to width x height (1024 x 768 for a 4:3
 * monitor). For a game's screenshots and for comparing frames; any output. */
int vp_dvg_save_png(const vp_dvg *dvg, const char *path, int width, int height);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_DVG_H */
