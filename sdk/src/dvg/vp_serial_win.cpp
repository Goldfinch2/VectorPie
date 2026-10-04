#ifdef _WIN32

#include "vp_serial.h"
#include "../core/vp_internal.h"

#include <cstdio>
#include <cstring>
#include <new>
#include <windows.h>

struct vp_serial {
    HANDLE h;
};

static const char *winErrorText(DWORD err)
{
    static thread_local char buf[160];
    DWORD n = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                             nullptr, err, 0, buf, sizeof(buf), nullptr);
    while (n > 0 && (buf[n - 1] == '\r' || buf[n - 1] == '\n' || buf[n - 1] == '.'))
        buf[--n] = '\0';
    if (n == 0)
        snprintf(buf, sizeof(buf), "error %lu", (unsigned long)err);
    return buf;
}

int vp_serial_open(const char *port, vp_serial **out)
{
    // COM10 and above only open through the \\.\ prefix; it is harmless below.
    char path[64];
    if (strncmp(port, "\\\\.\\", 4) == 0)
        snprintf(path, sizeof(path), "%s", port);
    else
        snprintf(path, sizeof(path), "\\\\.\\%s", port);

    HANDLE h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return vp_fail(VP_ERR_NODEV, "cannot open %s: %s", port, winErrorText(GetLastError()));

    DCB dcb;
    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) {
        DWORD e = GetLastError();
        CloseHandle(h);
        return vp_fail(VP_ERR_NODEV, "%s is not a serial port: %s", port, winErrorText(e));
    }
    dcb.BaudRate        = 2000000;
    dcb.ByteSize        = 8;
    dcb.Parity          = NOPARITY;
    dcb.StopBits        = ONESTOPBIT;
    dcb.fBinary         = TRUE;
    dcb.fOutX           = FALSE;
    dcb.fInX            = FALSE;
    dcb.fOutxCtsFlow    = FALSE;
    dcb.fOutxDsrFlow    = FALSE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fRtsControl     = RTS_CONTROL_ENABLE;
    dcb.fDtrControl     = DTR_CONTROL_ENABLE;
    if (!SetCommState(h, &dcb)) {
        DWORD e = GetLastError();
        CloseHandle(h);
        return vp_fail(VP_ERR_NODEV, "cannot configure %s: %s", port, winErrorText(e));
    }

    vp_serial *s = new (std::nothrow) vp_serial;
    if (!s) {
        CloseHandle(h);
        return vp_fail(VP_ERR_NOMEM, "out of memory");
    }
    s->h = h;
    *out = s;
    return VP_OK;
}

void vp_serial_close(vp_serial *s)
{
    if (!s) return;
    FlushFileBuffers(s->h);
    CloseHandle(s->h);
    delete s;
}

int vp_serial_write(vp_serial *s, const void *data, int count, int timeout_ms)
{
    COMMTIMEOUTS t;
    memset(&t, 0, sizeof(t));
    t.WriteTotalTimeoutConstant = (DWORD)timeout_ms;
    SetCommTimeouts(s->h, &t);

    const uint8_t *p = static_cast<const uint8_t *>(data);
    while (count > 0) {
        DWORD n = 0;
        if (!WriteFile(s->h, p, (DWORD)count, &n, nullptr))
            return vp_fail(VP_ERR_IO, "write failed: %s", winErrorText(GetLastError()));
        if (n == 0)
            return vp_fail(VP_ERR_TIMEOUT, "the port did not accept data for %d ms", timeout_ms);
        p += n;
        count -= (int)n;
    }
    return VP_OK;
}

int vp_serial_read(vp_serial *s, void *data, int count, int timeout_ms)
{
    uint8_t *p = static_cast<uint8_t *>(data);
    const uint64_t deadline = vp_ticks_ms() + (uint64_t)(timeout_ms > 0 ? timeout_ms : 0);
    while (count > 0) {
        uint64_t now = vp_ticks_ms();
        if (now >= deadline)
            return vp_fail(VP_ERR_TIMEOUT, "no answer from the board within %d ms", timeout_ms);
        // MAXDWORD / MAXDWORD / constant: return as soon as any byte is there,
        // or after the constant with nothing.
        COMMTIMEOUTS t;
        memset(&t, 0, sizeof(t));
        t.ReadIntervalTimeout = MAXDWORD;
        t.ReadTotalTimeoutMultiplier = MAXDWORD;
        t.ReadTotalTimeoutConstant = (DWORD)(deadline - now);
        t.WriteTotalTimeoutConstant = 2000;
        SetCommTimeouts(s->h, &t);

        DWORD n = 0;
        if (!ReadFile(s->h, p, (DWORD)count, &n, nullptr))
            return vp_fail(VP_ERR_IO, "read failed: %s", winErrorText(GetLastError()));
        p += n;
        count -= (int)n;
    }
    return VP_OK;
}

void vp_serial_flush_input(vp_serial *s)
{
    PurgeComm(s->h, PURGE_RXCLEAR);
}

void vp_serial_drain(vp_serial *s)
{
    FlushFileBuffers(s->h);
}

uint64_t vp_ticks_ms(void)
{
    return (uint64_t)GetTickCount64();
}

void vp_sleep_ms(int ms)
{
    if (ms > 0) Sleep((DWORD)ms);
}

#endif // _WIN32
