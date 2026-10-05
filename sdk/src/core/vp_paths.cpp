// Where a game's files are: next to its executable.

#include "vp_internal.h"

#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

const char *vp_package_dir(void)
{
    static char dir[1024];
    if (dir[0]) return dir;

#ifdef _WIN32
    DWORD n = GetModuleFileNameA(nullptr, dir, sizeof(dir) - 1);
    if (n == 0 || n >= sizeof(dir) - 1) { strcpy(dir, "."); return dir; }
    dir[n] = '\0';
    for (char *p = dir; *p; p++) if (*p == '\\') *p = '/';
#else
    ssize_t n = readlink("/proc/self/exe", dir, sizeof(dir) - 1);
    if (n <= 0) { strcpy(dir, "."); return dir; }
    dir[n] = '\0';
#endif
    char *slash = strrchr(dir, '/');
    if (slash && slash != dir) *slash = '\0';
    else strcpy(dir, ".");
    return dir;
}

const char *vp_asset_path(const char *relative, char *buf, size_t size)
{
    if (!buf || size == 0) return "";
    snprintf(buf, size, "%s/%s", vp_package_dir(), relative ? relative : "");
    return buf;
}
