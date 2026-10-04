// The PC window output of the device layer. vp_window_sdl.cpp implements it
// with SDL2; vp_window_none.cpp replaces it when the SDK is built without SDL2.
#pragma once

#include <cstdint>

struct vp_window;

// One vector as the device layer keeps it, after clipping.
struct vp_vec {
    int32_t x0, y0, x1, y1;
    uint8_t r, g, b;
};

// `width` x `height` is the logical size (the board space is drawn onto it);
// fullscreen fills the display and letterboxes to that shape.
int  vp_window_open(const char *title, int width, int height, int fullscreen,
                    int showBlanks, vp_window **out);
void vp_window_close(vp_window *w);

// Draw one frame (board coordinates, Y up) and present it. `beamX/beamY` is
// where the beam started, for drawing the first invisible move.
void vp_window_frame(vp_window *w, const vp_vec *vecs, uint32_t count,
                     int32_t beamX, int32_t beamY);

// 1 if SDL_QUIT is pending. Pumps events but never removes any.
int  vp_window_closed(vp_window *w);

// Start SDL's video subsystem the SDK's way (on a Pi with no X11/Wayland
// display: KMS/DRM). Reference-counted by SDL: pair each successful call with
// vp_sdl_video_quit(). The input layer needs it for the keyboard even when
// frames go to the board.
int  vp_sdl_video_init();
void vp_sdl_video_quit();
