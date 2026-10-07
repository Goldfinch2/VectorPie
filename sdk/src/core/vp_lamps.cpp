// The START lamps: keyboard LEDs (NUM LOCK = START 1, CAPS LOCK = START 2),
// which the VectorPie menu passes to the cabinet's panel. Linux evdev, as
// dvg_utils' KeyboardLeds; nothing on other systems.

#include "vp_internal.h"
#include <vectorpie/vp_credits.h>

#ifdef __linux__

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {

const int MAX_DEVICES = 8;
const int MAX_SCAN = 32;            // /dev/input/event0..31

struct Device {
    int fd;
    unsigned saved;                 // VP_LAMP_* as found
};

Device g_dev[MAX_DEVICES];
int g_count = 0;
bool g_open = false;
unsigned g_mask = 0;

bool testBit(unsigned bit, const unsigned char *bits)
{
    return (bits[bit / 8] >> (bit % 8)) & 1;
}

void writeLed(int fd, int led, bool on)
{
    input_event ev = {};
    ev.type = EV_LED;
    ev.code = (unsigned short)led;
    ev.value = on ? 1 : 0;
    if (write(fd, &ev, sizeof ev) < 0) { /* a keyboard unplugged: nothing to do */ }
    input_event syn = {};
    syn.type = EV_SYN;
    syn.code = SYN_REPORT;
    if (write(fd, &syn, sizeof syn) < 0) {}
}

void writeAll(unsigned mask)
{
    for (int i = 0; i < g_count; i++) {
        writeLed(g_dev[i].fd, LED_NUML, mask & VP_LAMP_START1);
        writeLed(g_dev[i].fd, LED_CAPSL, mask & VP_LAMP_START2);
    }
}

void openAll()
{
    g_open = true;
    for (int i = 0; i < MAX_SCAN && g_count < MAX_DEVICES; i++) {
        char path[64];
        snprintf(path, sizeof path, "/dev/input/event%d", i);
        int fd = open(path, O_RDWR | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) continue;
        unsigned char ev[(EV_MAX + 7) / 8] = {};
        if (ioctl(fd, EVIOCGBIT(0, sizeof ev), ev) < 0 || !testBit(EV_LED, ev)) {
            close(fd);
            continue;
        }
        unsigned char leds[(LED_MAX + 7) / 8] = {};
        unsigned saved = 0;
        if (ioctl(fd, EVIOCGLED(sizeof leds), leds) >= 0) {
            if (testBit(LED_NUML, leds)) saved |= VP_LAMP_START1;
            if (testBit(LED_CAPSL, leds)) saved |= VP_LAMP_START2;
        }
        g_dev[g_count].fd = fd;
        g_dev[g_count].saved = saved;
        g_count++;
    }
    writeAll(0);
    g_mask = 0;
    atexit(vp_lamps_close);          // a game not using vp_run
}

}  // namespace

void vp_lamps_set(unsigned mask)
{
    mask &= VP_LAMP_START1 | VP_LAMP_START2;
    if (!g_open) openAll();
    if (mask == g_mask) return;
    g_mask = mask;
    writeAll(mask);
}

void vp_lamps_close(void)
{
    if (!g_open) return;
    for (int i = 0; i < g_count; i++) {
        writeLed(g_dev[i].fd, LED_NUML, g_dev[i].saved & VP_LAMP_START1);
        writeLed(g_dev[i].fd, LED_CAPSL, g_dev[i].saved & VP_LAMP_START2);
        close(g_dev[i].fd);
    }
    g_count = 0;
    g_mask = 0;
    g_open = false;
}

#else  // not Linux: no lamps

void vp_lamps_set(unsigned mask) { (void)mask; }
void vp_lamps_close(void) {}

#endif
