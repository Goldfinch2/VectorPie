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
