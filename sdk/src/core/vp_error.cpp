// Result codes, the per-thread error detail, and the version string.

#include "vp_internal.h"

#include <cstdarg>
#include <cstdio>

static thread_local char t_lastError[256];

const char *vp_strerror(int code)
{
    switch (code) {
    case VP_OK:           return "ok";
    case VP_ERR_INVALID:  return "invalid argument";
    case VP_ERR_NOMEM:    return "out of memory";
    case VP_ERR_NODEV:    return "no device";
    case VP_ERR_IO:       return "I/O error";
    case VP_ERR_TIMEOUT:  return "timeout";
    case VP_ERR_PROTOCOL: return "protocol error";
    case VP_ERR_NOTFOUND: return "not found";
    default:              return "unknown error";
    }
}

const char *vp_last_error(void)
{
    return t_lastError;
}

int vp_fail(int code, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(t_lastError, sizeof(t_lastError), fmt, ap);
    va_end(ap);
    return code;
}

void vp_strlcpy(char *dst, const char *src, size_t size)
{
    if (size == 0) return;
    size_t i = 0;
    for (; src && src[i] && i + 1 < size; i++) dst[i] = src[i];
    dst[i] = '\0';
}

const char *vp_version(void)
{
    return VP_VERSION_STRING;
}
