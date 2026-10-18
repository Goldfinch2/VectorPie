// The marquee: a PNG on the cabinet's HDMI screen while the game draws on
// the board, as the cabinet's own native games show theirs (fit modes from
// advmame.rc's vector_aux_marquee_display_mode).

#include "../core/vp_internal.h"
#include "../dvg/vp_window.h"
#include <vectorpie/vp_marquee.h>

#include <SDL.h>

#ifdef __linux__
#include "../third_party/lodepng/lodepng.h"
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <fstream>
#include <string>
#include <vector>
#endif

#ifdef __linux__

namespace {

SDL_Window   *g_window = nullptr;
SDL_Renderer *g_renderer = nullptr;
SDL_Texture  *g_texture = nullptr;
bool          g_videoUp = false;

enum class Fit { FIT, ZOOM, STRETCH, FIT_WIDTH, FIT_HEIGHT };

Fit fitMode()
{
    Fit fit = Fit::FIT;
    vp_rc *rc = nullptr;
    if (vp_rc_load(nullptr, &rc) == VP_OK) {
        const char *m = vp_rc_get(rc, "vector_aux_marquee_display_mode");
        if (m) {
            if      (!strcmp(m, "zoom"))       fit = Fit::ZOOM;
            else if (!strcmp(m, "stretch"))    fit = Fit::STRETCH;
            else if (!strcmp(m, "fit_width"))  fit = Fit::FIT_WIDTH;
            else if (!strcmp(m, "fit_height")) fit = Fit::FIT_HEIGHT;
        }
        vp_rc_free(rc);
    }
    return fit;
}

SDL_Rect place(Fit fit, int winW, int winH, int texW, int texH)
{
    float sx = (float)winW / texW, sy = (float)winH / texH;
    switch (fit) {
    case Fit::STRETCH:
        return { 0, 0, winW, winH };
    case Fit::FIT_WIDTH: {
        int h = (int)(texH * sx);
        return { 0, (winH - h) / 2, winW, h };
    }
    case Fit::FIT_HEIGHT: {
        int w = (int)(texW * sy);
        return { (winW - w) / 2, 0, w, winH };
    }
    default: {
        float s = fit == Fit::ZOOM ? (sx > sy ? sx : sy) : (sx < sy ? sx : sy);
        int w = (int)(texW * s), h = (int)(texH * s);
        return { (winW - w) / 2, (winH - h) / 2, w, h };
    }
    }
}

bool openScreen()
{
    if (g_renderer) return true;
    if (!g_videoUp) {
        if (vp_sdl_video_init() != VP_OK) return false;
        g_videoUp = true;
    }
    // Only on KMS/DRM is there a marquee screen of its own; under X11 or
    // Wayland (a Linux PC) a fullscreen window would cover the desktop.
    const char *driver = SDL_GetCurrentVideoDriver();
    if (!driver || (strcmp(driver, "KMSDRM") != 0 && strcmp(driver, "kmsdrm") != 0)) {
        vp_fail(VP_ERR_NODEV, "no marquee screen (video driver %s)", driver ? driver : "none");
        return false;
    }
    SDL_DisplayMode dm;
    int w = 640, h = 480;
    if (SDL_GetCurrentDisplayMode(0, &dm) == 0) { w = dm.w; h = dm.h; }
    g_window = SDL_CreateWindow("marquee", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                w, h, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);
    if (!g_window) {
        vp_fail(VP_ERR_NODEV, "marquee window: %s", SDL_GetError());
        return false;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
    if (!g_renderer) g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    if (!g_renderer) {
        vp_fail(VP_ERR_NODEV, "marquee renderer: %s", SDL_GetError());
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
        return false;
    }
    SDL_ShowCursor(SDL_DISABLE);
    return true;
}

// Both buffers (KMS/DRM double-buffers), then nothing more: the picture
// stays on the screen until it is replaced.
void present(const SDL_Rect *dst)
{
    for (int i = 0; i < 2; i++) {
        SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
        SDL_RenderClear(g_renderer);
        if (g_texture && dst) SDL_RenderCopy(g_renderer, g_texture, nullptr, dst);
        SDL_RenderPresent(g_renderer);
    }
}

bool fileExists(const std::string &p)
{
    std::ifstream f(p);
    return (bool)f;
}

}  // namespace

int vp_marquee_show(const char *png_path)
{
    if (!png_path || !*png_path) return vp_fail(VP_ERR_INVALID, "no marquee file");
    std::vector<unsigned char> rgba;
    unsigned w = 0, h = 0;
    unsigned e = lodepng::decode(rgba, w, h, std::string(png_path));
    if (e || w == 0 || h == 0)
        return vp_fail(VP_ERR_NOTFOUND, "marquee %s: %s", png_path, e ? lodepng_error_text(e) : "empty");
    if (!openScreen()) return VP_ERR_NODEV;

    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom(rgba.data(), (int)w, (int)h, 32, (int)w * 4,
                                                        SDL_PIXELFORMAT_RGBA32);
    if (!s) return vp_fail(VP_ERR_NOMEM, "marquee surface: %s", SDL_GetError());
    if (g_texture) SDL_DestroyTexture(g_texture);
    g_texture = SDL_CreateTextureFromSurface(g_renderer, s);
    SDL_FreeSurface(s);
    if (!g_texture) return vp_fail(VP_ERR_NOMEM, "marquee texture: %s", SDL_GetError());

    int winW = 0, winH = 0;
    SDL_GetRendererOutputSize(g_renderer, &winW, &winH);
    SDL_Rect dst = place(fitMode(), winW, winH, (int)w, (int)h);
    present(&dst);
    return VP_OK;
}

void vp_marquee_clear(void)
{
    if (g_texture) { SDL_DestroyTexture(g_texture); g_texture = nullptr; }
    if (g_renderer) {
        present(nullptr);                       // black
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }
    if (g_window) { SDL_DestroyWindow(g_window); g_window = nullptr; }
    if (g_videoUp) { vp_sdl_video_quit(); g_videoUp = false; }
}

// vp_run, when the output is the board: the package's marquee, if it has one.
void vp_marquee_auto(void)
{
    std::string base = vp_manifest_value("marquee");
    if (base.empty()) base = "marquee.png";
    if (base.size() > 4 && strcasecmp(base.c_str() + base.size() - 4, ".png") == 0)
        base.erase(base.size() - 4);

    // marquee.<screen height>.png first (an image made for this screen)
    std::vector<std::string> tries;
    if (vp_sdl_video_init() == VP_OK) {
        SDL_DisplayMode dm;
        if (SDL_GetCurrentDisplayMode(0, &dm) == 0)
            tries.push_back(base + "." + std::to_string(dm.h) + ".png");
        vp_sdl_video_quit();
    }
    tries.push_back(base + ".png");

    char path[1100];
    for (const std::string &t : tries) {
        vp_asset_path(t.c_str(), path, sizeof path);
        if (!fileExists(path)) continue;
        if (vp_marquee_show(path) != VP_OK)
            fprintf(stderr, "marquee: %s\n", vp_last_error());
        return;
    }
}

#else  // no marquee screen

int vp_marquee_show(const char *png_path)
{
    (void)png_path;
    return vp_fail(VP_ERR_NODEV, "no marquee screen on this system");
}

void vp_marquee_clear(void) {}
void vp_marquee_auto(void) {}

#endif
