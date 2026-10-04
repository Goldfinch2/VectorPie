#ifndef _WIN32

#include "vp_serial.h"
#include "../core/vp_internal.h"

#include <cerrno>
#include <cstring>
#include <ctime>
#include <new>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

struct vp_serial {
    int fd;
};

int vp_serial_open(const char *port, vp_serial **out)
{
    // O_NONBLOCK on the open itself: a tty open BLOCKS waiting for carrier
    // detect until CLOCAL is set, and CLOCAL is only set below, once the port
    // is open. Termios state persists on the device between opens, so this
    // stalls whoever opens first after a boot (it held AdvanceMAME for
    // 30 s - 2 min) and nobody afterwards. Blocking mode is restored after
    // tcsetattr.
    int fd = ::open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
        return vp_fail(VP_ERR_NODEV, "cannot open %s: %s", port, strerror(errno));

    struct termios tios;
    memset(&tios, 0, sizeof(tios));
    if (tcgetattr(fd, &tios) < 0) {
        int e = errno;
        ::close(fd);
        return vp_fail(VP_ERR_NODEV, "%s is not a serial port: %s", port, strerror(e));
    }

    cfmakeraw(&tios);
    cfsetispeed(&tios, B2000000);
    cfsetospeed(&tios, B2000000);
    tios.c_cflag |= (CLOCAL | CREAD);
    tios.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tios) < 0) {
        int e = errno;
        ::close(fd);
        return vp_fail(VP_ERR_NODEV, "cannot configure %s: %s", port, strerror(e));
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

    vp_serial *s = new (std::nothrow) vp_serial;
    if (!s) {
        ::close(fd);
        return vp_fail(VP_ERR_NOMEM, "out of memory");
    }
    s->fd = fd;
    *out = s;
    return VP_OK;
}

void vp_serial_close(vp_serial *s)
{
    if (!s) return;
    tcdrain(s->fd);
    ::close(s->fd);
    delete s;
}

int vp_serial_write(vp_serial *s, const void *data, int count, int timeout_ms)
{
    const uint8_t *p = static_cast<const uint8_t *>(data);
    while (count > 0) {
        // Wait for room first, so a board that stopped reading turns into a
        // timeout instead of a write() that never returns.
        struct pollfd pfd = { s->fd, POLLOUT, 0 };
        int r = poll(&pfd, 1, timeout_ms);
        if (r == 0)
            return vp_fail(VP_ERR_TIMEOUT, "the port did not accept data for %d ms", timeout_ms);
        if (r < 0) {
            if (errno == EINTR) continue;
            return vp_fail(VP_ERR_IO, "poll failed: %s", strerror(errno));
        }
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
            return vp_fail(VP_ERR_IO, "the port was disconnected");

        ssize_t n = ::write(s->fd, p, (size_t)count);
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            return vp_fail(VP_ERR_IO, "write failed: %s", strerror(errno));
        }
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
        // The port is blocking with VMIN=1, so a read() with nothing to read
        // would wait forever (the hang dvg_utils had): poll first, always.
        uint64_t now = vp_ticks_ms();
        if (now >= deadline)
            return vp_fail(VP_ERR_TIMEOUT, "no answer from the board within %d ms", timeout_ms);
        struct pollfd pfd = { s->fd, POLLIN, 0 };
        int r = poll(&pfd, 1, (int)(deadline - now));
        if (r == 0)
            return vp_fail(VP_ERR_TIMEOUT, "no answer from the board within %d ms", timeout_ms);
        if (r < 0) {
            if (errno == EINTR) continue;
            return vp_fail(VP_ERR_IO, "poll failed: %s", strerror(errno));
        }
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
            return vp_fail(VP_ERR_IO, "the port was disconnected");

        ssize_t n = ::read(s->fd, p, (size_t)count);
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            return vp_fail(VP_ERR_IO, "read failed: %s", strerror(errno));
        }
        if (n == 0)
            return vp_fail(VP_ERR_IO, "the port was disconnected");
        p += n;
        count -= (int)n;
    }
    return VP_OK;
}

void vp_serial_flush_input(vp_serial *s)
{
    tcflush(s->fd, TCIFLUSH);
}

void vp_serial_drain(vp_serial *s)
{
    tcdrain(s->fd);
}

uint64_t vp_ticks_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

void vp_sleep_ms(int ms)
{
    if (ms <= 0) return;
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    while (nanosleep(&ts, &ts) < 0 && errno == EINTR) {}
}

#endif // !_WIN32
