// Internal helpers shared by the SDK's modules. Not installed.
#pragma once

#include <vectorpie/vp.h>
#include <cstddef>

// Record the detail text for vp_last_error() and return `code`, so a failing
// path reads: return vp_fail(VP_ERR_IO, "write to %s failed", port);
int vp_fail(int code, const char *fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

// Copy a C string into a fixed buffer, always NUL-terminated, truncating.
void vp_strlcpy(char *dst, const char *src, size_t size);
