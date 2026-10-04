// PC window output when the SDK is built without SDL2 (VP_WITH_SDL2=OFF).

#include "vp_window.h"
#include "../core/vp_internal.h"

int vp_window_open(const char *, int, int, int, int, vp_window **)
{
    return vp_fail(VP_ERR_NODEV, "this SDK was built without SDL2: no window output");
}

void vp_window_close(vp_window *) {}

void vp_window_frame(vp_window *, const vp_vec *, uint32_t, int32_t, int32_t) {}

int vp_window_closed(vp_window *)
{
    return 0;
}

int vp_sdl_video_init()
{
    return vp_fail(VP_ERR_NODEV, "this SDK was built without SDL2");
}

void vp_sdl_video_quit() {}
