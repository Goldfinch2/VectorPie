// Window output, SDL2: a window on the PC, the whole HDMI screen on the Pi.
// Plain lines, no glow: it shows exactly what the board would be sent.

#include "vp_window.h"
#include "../core/vp_internal.h"

#include <SDL.h>
#include <new>

struct vp_window {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    int           aspectW, aspectH;   // the monitor's shape, e.g. 1024 x 768
    int           showBlanks;
};

// Multisampling: the renderer's lines are not anti-aliased; 4x MSAA on a GL
// renderer smooths their edges. If the driver refuses it, the window opens
// without it.
static const int MSAA_SAMPLES = 4;
#if defined(_WIN32)
static const char *const GL_RENDER_DRIVER = "opengl";
#else
static const char *const GL_RENDER_DRIVER = "opengles2";
#endif

// Brightness of an invisible move when show_blanks is on.
static const Uint8 BLANK_GRAY = 60;

int vp_sdl_video_init()
{
    // The SDK has no main() of its own to hand to SDL; tell SDL so (needed on
    // Windows when the program does not link SDL2main, harmless elsewhere).
    SDL_SetMainReady();
#if defined(__linux__)
    // The Pi runs no windowing system: with no X11 or Wayland display to open
    // a window on, go straight to KMS/DRM, which draws on the HDMI display the
    // way the menu does. An SDL_VIDEODRIVER the user set always wins.
    if (!SDL_getenv("SDL_VIDEODRIVER") && !SDL_getenv("DISPLAY") && !SDL_getenv("WAYLAND_DISPLAY"))
        SDL_SetHint(SDL_HINT_VIDEODRIVER, "kmsdrm");
#endif
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) == 0)
        return VP_OK;
    return vp_fail(VP_ERR_NODEV, "SDL video init failed: %s", SDL_GetError());
}

void vp_sdl_video_quit()
{
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

int vp_window_open(const char *title, int width, int height, int fullscreen,
                   int showBlanks, vp_window **out)
{
    int r = vp_sdl_video_init();
    if (r != VP_OK)
        return r;

    // KMS/DRM has no windows: it is always the whole screen.
    const char *driver = SDL_GetCurrentVideoDriver();
    if (driver && SDL_strcasecmp(driver, "KMSDRM") == 0)
        fullscreen = 1;

    Uint32 flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_RESIZABLE;
    SDL_Window *win = nullptr;
    SDL_Renderer *ren = nullptr;

    // First try: a GL renderer with multisampling.
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, MSAA_SAMPLES);
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, GL_RENDER_DRIVER);
    win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           width, height, flags | SDL_WINDOW_OPENGL);
    if (win) {
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
        if (!ren) {
            SDL_DestroyWindow(win);
            win = nullptr;
        }
    }

    // Otherwise: whatever renderer SDL picks, no multisampling.
    if (!win) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
        SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
        win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width, height, flags);
        if (!win) {
            r = vp_fail(VP_ERR_NODEV, "cannot create the window: %s", SDL_GetError());
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            return r;
        }
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
        if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
        if (!ren) {
            r = vp_fail(VP_ERR_NODEV, "cannot create a renderer: %s", SDL_GetError());
            SDL_DestroyWindow(win);
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            return r;
        }
    }
    // Leave SDL's renderer choice as it was, for anything else the program
    // opens itself.
    SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
    if (fullscreen)
        SDL_ShowCursor(SDL_DISABLE);

    vp_window *w = new (std::nothrow) vp_window;
    if (!w) {
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return vp_fail(VP_ERR_NOMEM, "out of memory");
    }
    w->window = win;
    w->renderer = ren;
    w->aspectW = width;
    w->aspectH = height;
    w->showBlanks = showBlanks;
    *out = w;
    return VP_OK;
}

void vp_window_close(vp_window *w)
{
    if (!w) return;
    SDL_DestroyRenderer(w->renderer);
    SDL_DestroyWindow(w->window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    delete w;
}

void vp_window_frame(vp_window *w, const vp_vec *vecs, uint32_t count,
                     int32_t beamX, int32_t beamY)
{
    // Draw at the screen's real resolution (a fixed logical size would be
    // rasterized small and scaled up: blocky lines). The 0..4095 board space
    // is stretched to the monitor's shape, the way the monitor stretches it
    // to the tube, and centered with black bars where the screen's shape
    // differs.
    int ow = 0, oh = 0;
    SDL_GetRendererOutputSize(w->renderer, &ow, &oh);
    const float aspect = (float)w->aspectW / (float)w->aspectH;
    float vw = (float)ow, vh = (float)oh;
    if (vw / vh > aspect) vw = vh * aspect;
    else                  vh = vw / aspect;
    const float ox = ((float)ow - vw) * 0.5f;
    const float oy = ((float)oh - vh) * 0.5f;

    // Board Y points up, screen Y points down. +0.5: the center of a pixel.
    const float sx = vw / (VP_DVG_COORD_MAX + 1);
    const float sy = vh / (VP_DVG_COORD_MAX + 1);
#define WX(x) (ox + ((float)(x) + 0.5f) * sx)
#define WY(y) (oy + ((float)(VP_DVG_COORD_MAX - (y)) + 0.5f) * sy)

    SDL_SetRenderDrawColor(w->renderer, 0, 0, 0, 255);
    SDL_RenderClear(w->renderer);

    for (uint32_t i = 0; i < count; i++) {
        const vp_vec &v = vecs[i];
        const bool blank = (v.r | v.g | v.b) == 0;

        if (w->showBlanks && (beamX != v.x0 || beamY != v.y0)) {
            SDL_SetRenderDrawColor(w->renderer, BLANK_GRAY, BLANK_GRAY, BLANK_GRAY, 255);
            SDL_RenderDrawLineF(w->renderer, WX(beamX), WY(beamY), WX(v.x0), WY(v.y0));
        }
        if (blank) {
            if (w->showBlanks) {
                SDL_SetRenderDrawColor(w->renderer, BLANK_GRAY, BLANK_GRAY, BLANK_GRAY, 255);
                SDL_RenderDrawLineF(w->renderer, WX(v.x0), WY(v.y0), WX(v.x1), WY(v.y1));
            }
        } else if (v.x0 == v.x1 && v.y0 == v.y1) {
            SDL_SetRenderDrawColor(w->renderer, v.r, v.g, v.b, 255);
            SDL_RenderDrawPointF(w->renderer, WX(v.x0), WY(v.y0));
        } else {
            SDL_SetRenderDrawColor(w->renderer, v.r, v.g, v.b, 255);
            SDL_RenderDrawLineF(w->renderer, WX(v.x0), WY(v.y0), WX(v.x1), WY(v.y1));
        }
        beamX = v.x1;
        beamY = v.y1;
    }
#undef WX
#undef WY

    SDL_RenderPresent(w->renderer);
}

int vp_window_closed(vp_window *w)
{
    (void)w;
    // Pump, then look without removing: the program's own event loop (if it
    // has one) must still see every event. SDL_QUIT is what SDL posts when
    // the last window is closed.
    SDL_PumpEvents();
    return SDL_HasEvent(SDL_QUIT) ? 1 : 0;
}
