// Internal helpers shared by the SDK's modules. Not installed.
#pragma once

#include <vectorpie/vp.h>
#include <cstddef>
#include <string>
#include <vector>

// Record the detail text for vp_last_error() and return `code`, so a failing
// path reads: return vp_fail(VP_ERR_IO, "write to %s failed", port);
int vp_fail(int code, const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

// Copy a C string into a fixed buffer, always NUL-terminated, truncating.
void vp_strlcpy(char *dst, const char *src, size_t size);

// Parse one advmame.rc binding value ("keyboard[0,up] or joystick_button[0,button1]")
// the way vp_rc does. The input layer writes its defaults in this syntax.
std::vector<vp_binding> vp_rc_parse_bindings(const std::string &value);

// vp_run, when the output is the board: show the package's marquee on the
// HDMI screen (vp_marquee.h), if it has one. Nothing elsewhere.
void vp_marquee_auto(void);

// The game package's game.ini: a key's value (comments after ';' or '#'
// dropped, spaces trimmed), or empty when there is none. `section`: the
// lines after "[section]" (one per board: dvg, beamforge), or NULL for the
// lines before any section (id, name, marquee...).
std::string vp_manifest_value(const char *key, const char *section = nullptr);

// The current font's letters as board-space triangles, laid out as vp_text
// lays them out (VP_FONT_COMIC; nothing for the other fonts): what
// vp3d_scene_text puts in a scene to hide what is behind the text.
void vp_text_screens(const char *s, float x, float y, float height, int align,
                     void (*triangle)(void *user, const float xy[6]), void *user);
