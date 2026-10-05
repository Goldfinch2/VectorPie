// The device layer. Clipping and the wire encoding come from dvg_utils'
// DvgCmd (proven on the cabinet); what changed:
//   - drawing is the beam's model: color / moveto / lineto, drawn in the order
//     given (no sorting, no merging);
//   - one serial port per handle instead of a process-global one;
//   - the command buffer is sized for the worst case, so the end-of-frame word
//     can never be lost, and lines over the cap are counted, not hidden;
//   - reads and writes time out instead of blocking forever;
//   - frames can go to a window instead of the board;
//   - the board is asked once what it is (USB-DVG or BeamForge).

#include "vp_serial.h"
#include "vp_window.h"
#include "../core/vp_internal.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

// ---------------------------------------------------------------------------
// Wire protocol: 4-byte big-endian words, (FLAG << 29) | payload
// ---------------------------------------------------------------------------

#define DVG_FLAG_COMPLETE            0x0u
#define DVG_FLAG_RGB                 0x1u
#define DVG_FLAG_XY                  0x2u
#define DVG_FLAG_CMD                 0x5u
#define DVG_FLAG_EXIT                0x7u
#define DVG_CMD_GET_DVG_INFO         0x1u
#define DVG_COMPLETE_MONOCHROME      (1u << 28)

// Worst case per line: a blank move, a color change and the line itself.
static const uint32_t WORDS_PER_VECTOR_MAX = 3;
static const uint32_t CMD_BUF_WORDS = VP_DVG_MAX_VECTORS * WORDS_PER_VECTOR_MAX + 1;

// Frames are written in 512-byte pieces followed by a drain, as dvg_utils
// does on the cabinet.
static const int WRITE_CHUNK = 512;
static const int WRITE_TIMEOUT_MS = 2000;

// GET_DVG_INFO: the firmware's send_dvg_info() builds the JSON in a 512-byte
// buffer; anything much larger is not a reply.
static const int INFO_TIMEOUT_MS = 1000;
static const uint32_t INFO_MAX_LEN = 4096;

// ---------------------------------------------------------------------------
// The handle
// ---------------------------------------------------------------------------

struct vp_dvg {
    vp_dvg_output output;
    vp_hardware   hardware;
    vp_serial    *serial;
    vp_window    *window;
    char          port[256];

    uint8_t      *cmd;
    uint32_t      cmdLen;           // bytes

    vp_vec       *vecs;
    uint32_t      vecCnt;

    int32_t       clipL, clipB, clipR, clipT;

    // The drawing state the game sees.
    int32_t       penX, penY;
    uint8_t       penR, penG, penB;

    // The encoder's state.
    int32_t       beamX, beamY;     // where the previous frame left the beam
    int           lastR, lastG, lastB;

    vp_dvg_stats  stats;
};

// ---------------------------------------------------------------------------
// Encoding
// ---------------------------------------------------------------------------

static void cmdAppend(vp_dvg *d, uint32_t w)
{
    // Cannot overflow: the buffer holds the worst case of a full frame.
    uint8_t *p = d->cmd + d->cmdLen;
    p[0] = (uint8_t)(w >> 24);
    p[1] = (uint8_t)(w >> 16);
    p[2] = (uint8_t)(w >> 8);
    p[3] = (uint8_t)(w);
    d->cmdLen += 4;
}

static void cmdAddPoint(vp_dvg *d, int32_t x, int32_t y, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t blank = (r == 0 && g == 0 && b == 0) ? 1u : 0u;
    if (!blank && (r != d->lastR || g != d->lastG || b != d->lastB)) {
        d->lastR = r; d->lastG = g; d->lastB = b;
        cmdAppend(d, (DVG_FLAG_RGB << 29) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
    }
    cmdAppend(d, (DVG_FLAG_XY << 29) | (blank << 28)
                 | (((uint32_t)x & 0x3fff) << 14) | ((uint32_t)y & 0x3fff));
}

static uint32_t segLength(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    double dx = (double)x1 - x0, dy = (double)y1 - y0;
    return (uint32_t)lround(std::sqrt(dx * dx + dy * dy));
}

// ---------------------------------------------------------------------------
// Clipping (Cohen-Sutherland)
// ---------------------------------------------------------------------------

static int csCode(const vp_dvg *d, int32_t x, int32_t y)
{
    int code = 0;
    if (x < d->clipL)      code |= 1;
    else if (x > d->clipR) code |= 2;
    if (y < d->clipB)      code |= 4;
    else if (y > d->clipT) code |= 8;
    return code;
}

static bool clipLine(const vp_dvg *d, int32_t *x0, int32_t *y0, int32_t *x1, int32_t *y1)
{
    int code0 = csCode(d, *x0, *y0);
    int code1 = csCode(d, *x1, *y1);

    for (;;) {
        if (!(code0 | code1)) return true;
        if (code0 & code1) return false;

        int code = code0 ? code0 : code1;
        int32_t x, y;
        if (code & 8) {
            x = *x0 + (int32_t)((int64_t)(*x1 - *x0) * (d->clipT - *y0) / (*y1 - *y0));
            y = d->clipT;
        } else if (code & 4) {
            x = *x0 + (int32_t)((int64_t)(*x1 - *x0) * (d->clipB - *y0) / (*y1 - *y0));
            y = d->clipB;
        } else if (code & 2) {
            y = *y0 + (int32_t)((int64_t)(*y1 - *y0) * (d->clipR - *x0) / (*x1 - *x0));
            x = d->clipR;
        } else {
            y = *y0 + (int32_t)((int64_t)(*y1 - *y0) * (d->clipL - *x0) / (*x1 - *x0));
            x = d->clipL;
        }

        if (code == code0) { *x0 = x; *y0 = y; code0 = csCode(d, *x0, *y0); }
        else               { *x1 = x; *y1 = y; code1 = csCode(d, *x1, *y1); }
    }
}

// ---------------------------------------------------------------------------
// Asking the board what it is
// ---------------------------------------------------------------------------

// The string value of "key" in the board's JSON, or "" (the firmware writes a
// flat object of simple values; no general parser needed).
static void jsonString(const char *json, const char *key, char *out, size_t size)
{
    out[0] = '\0';
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *p = strstr(json, pattern);
    if (!p) return;
    p += strlen(pattern);
    while (*p == ' ' || *p == '\t' || *p == ':') p++;
    if (*p != '"') return;
    p++;
    size_t i = 0;
    while (*p && *p != '"' && i + 1 < size) out[i++] = *p++;
    out[i] = '\0';
}

static bool containsNoCase(const char *s, const char *needle)
{
    size_t n = strlen(needle);
    for (; *s; s++) {
        size_t i = 0;
        while (i < n && s[i] && tolower((unsigned char)s[i]) == tolower((unsigned char)needle[i])) i++;
        if (i == n) return true;
    }
    return false;
}

// GET_DVG_INFO, as the firmware's send_dvg_info() answers it: the command
// word echoed (raw uint32, little-endian), the JSON length (little-endian),
// then the JSON, NUL-terminated and padded to 4 bytes.
static vp_hardware queryHardware(vp_dvg *d)
{
    const uint32_t cmd = (DVG_FLAG_CMD << 29) | DVG_CMD_GET_DVG_INFO;
    const uint8_t word[4] = { (uint8_t)(cmd >> 24), (uint8_t)(cmd >> 16),
                              (uint8_t)(cmd >> 8), (uint8_t)cmd };

    vp_serial_flush_input(d->serial);
    if (vp_serial_write(d->serial, word, 4, WRITE_TIMEOUT_MS) != VP_OK)
        return VP_HW_USB_DVG;

    uint8_t le[4];
    if (vp_serial_read(d->serial, le, 4, INFO_TIMEOUT_MS) != VP_OK)
        return VP_HW_USB_DVG;
    uint32_t ack = (uint32_t)le[0] | ((uint32_t)le[1] << 8) | ((uint32_t)le[2] << 16) | ((uint32_t)le[3] << 24);
    if (ack != cmd)
        return VP_HW_USB_DVG;

    if (vp_serial_read(d->serial, le, 4, INFO_TIMEOUT_MS) != VP_OK)
        return VP_HW_USB_DVG;
    uint32_t len = (uint32_t)le[0] | ((uint32_t)le[1] << 8) | ((uint32_t)le[2] << 16) | ((uint32_t)le[3] << 24);
    if (len == 0 || len > INFO_MAX_LEN)
        return VP_HW_USB_DVG;

    char *json = static_cast<char *>(malloc(len + 1));
    if (!json)
        return VP_HW_USB_DVG;
    vp_hardware hw = VP_HW_USB_DVG;
    if (vp_serial_read(d->serial, json, (int)len, INFO_TIMEOUT_MS) == VP_OK) {
        json[len] = '\0';
        char product[64];
        jsonString(json, "productName", product, sizeof(product));
        if (containsNoCase(product, "beamforge"))
            hw = VP_HW_BEAMFORGE;
    }
    free(json);
    return hw;
}

// ---------------------------------------------------------------------------
// Output selection
// ---------------------------------------------------------------------------

int vp_dvg_resolve_port(char *port, size_t port_size, const char **source)
{
    if (!port || port_size == 0)
        return vp_fail(VP_ERR_INVALID, "no buffer for the port");

    const char *env = getenv("VP_DVG_PORT");
    if (env && *env) {
        vp_strlcpy(port, env, port_size);
        if (source) *source = "VP_DVG_PORT";
        return VP_OK;
    }

    vp_rc *rc = nullptr;
    if (vp_rc_load(nullptr, &rc) == VP_OK) {
        const char *p = vp_rc_get(rc, "vector_aux_renderer_port");
        if (p && *p) {
            vp_strlcpy(port, p, port_size);
            vp_rc_free(rc);
            if (source) *source = "advmame.rc";
            return VP_OK;
        }
        vp_rc_free(rc);
    }

#ifdef _WIN32
    port[0] = '\0';
    return vp_fail(VP_ERR_NODEV,
                   "no DVG port: set VP_DVG_PORT (e.g. COM5) or pass the port");
#else
    vp_strlcpy(port, "/dev/ttyACM0", port_size);
    if (source) *source = "default";
    return VP_OK;
#endif
}

void vp_dvg_auto_options(vp_dvg_options *o, const char *title, const char **reason)
{
    memset(o, 0, sizeof(*o));
    o->window_title = title;
    const char *vert = getenv("VP_VERTICAL");
    o->vertical = (vert && strcmp(vert, "1") == 0) ? 1 : 0;

    const char *env = getenv("VP_OUTPUT");
    if (env && *env) {
        if (strcmp(env, "dvg") == 0) {
            o->output = VP_DVG_OUTPUT_BOARD;
            if (reason) *reason = "VP_OUTPUT=dvg";
            return;
        }
        if (strcmp(env, "windowed") == 0) {
            o->output = VP_DVG_OUTPUT_WINDOW;
            if (reason) *reason = "VP_OUTPUT=windowed";
            return;
        }
        if (strcmp(env, "window") == 0) {
            o->output = VP_DVG_OUTPUT_WINDOW;
            o->fullscreen = 1;
            if (reason) *reason = "VP_OUTPUT=window";
            return;
        }
        // Unknown value: fall through to the normal rules.
    }

    vp_rc *rc = nullptr;
    if (vp_rc_load(nullptr, &rc) == VP_OK) {
        const char *r = vp_rc_get(rc, "vector_aux_renderer");
        bool dvg = r && strcmp(r, "dvg") == 0;
        vp_rc_free(rc);
        if (dvg) {
            o->output = VP_DVG_OUTPUT_BOARD;
            if (reason) *reason = "advmame.rc: vector_aux_renderer dvg";
            return;
        }
        o->output = VP_DVG_OUTPUT_WINDOW;
        o->fullscreen = 1;
        if (reason) *reason = "advmame.rc: the DVG is disabled";
        return;
    }

    o->output = VP_DVG_OUTPUT_WINDOW;
    o->fullscreen = 1;
    if (reason) *reason = "no advmame.rc";
}

// ---------------------------------------------------------------------------
// Open / close
// ---------------------------------------------------------------------------

static vp_dvg *allocHandle()
{
    vp_dvg *d = static_cast<vp_dvg *>(calloc(1, sizeof(vp_dvg)));
    if (!d) return nullptr;
    d->vecs = static_cast<vp_vec *>(malloc(VP_DVG_MAX_VECTORS * sizeof(vp_vec)));
    d->cmd  = static_cast<uint8_t *>(malloc(CMD_BUF_WORDS * 4));
    if (!d->vecs || !d->cmd) {
        free(d->vecs);
        free(d->cmd);
        free(d);
        return nullptr;
    }
    d->clipL = 0; d->clipB = 0;
    d->clipR = VP_DVG_COORD_MAX; d->clipT = VP_DVG_COORD_MAX;
    d->penR = d->penG = d->penB = 255;
    d->lastR = d->lastG = d->lastB = -1;
    return d;
}

static void freeHandle(vp_dvg *d)
{
    free(d->vecs);
    free(d->cmd);
    free(d);
}

int vp_dvg_open_ex(const vp_dvg_options *o, vp_dvg **out)
{
    if (!o || !out)
        return vp_fail(VP_ERR_INVALID, "vp_dvg_open_ex: NULL argument");
    *out = nullptr;
    if (o->output != VP_DVG_OUTPUT_BOARD && o->output != VP_DVG_OUTPUT_WINDOW)
        return vp_fail(VP_ERR_INVALID, "vp_dvg_open_ex: no output chosen");

    vp_dvg *d = allocHandle();
    if (!d)
        return vp_fail(VP_ERR_NOMEM, "out of memory for the frame buffers");
    d->output = o->output;

    if (o->output == VP_DVG_OUTPUT_BOARD) {
        if (o->port && *o->port) {
            vp_strlcpy(d->port, o->port, sizeof(d->port));
        } else {
            int r = vp_dvg_resolve_port(d->port, sizeof(d->port), nullptr);
            if (r != VP_OK) { freeHandle(d); return r; }
        }
        int r = vp_serial_open(d->port, &d->serial);
        if (r != VP_OK) { freeHandle(d); return r; }
        d->hardware = queryHardware(d);
    } else {
        int w = o->window_width, h = o->window_height;
        if (w <= 0 || h <= 0) {
            w = o->vertical ? 768 : 1024;
            h = o->vertical ? 1024 : 768;
        }
        const char *title = (o->window_title && *o->window_title) ? o->window_title : "VectorPie";
        int r = vp_window_open(title, w, h, o->fullscreen, o->show_blanks, &d->window);
        if (r != VP_OK) { freeHandle(d); return r; }
        d->hardware = VP_HW_WINDOW;
    }

    *out = d;
    return VP_OK;
}

int vp_dvg_open(const char *port, vp_dvg **out)
{
    vp_dvg_options o;
    memset(&o, 0, sizeof(o));
    o.output = VP_DVG_OUTPUT_BOARD;
    o.port = port;
    return vp_dvg_open_ex(&o, out);
}

int vp_dvg_open_auto(const char *title, vp_dvg **out)
{
    vp_dvg_options o;
    vp_dvg_auto_options(&o, title, nullptr);
    return vp_dvg_open_ex(&o, out);
}

void vp_dvg_close(vp_dvg *d)
{
    if (!d) return;
    if (d->serial) {
        // Exit sequence: 16 end-of-frame words, then EXIT; the board returns
        // to its home screen. The pauses around it are the ones the games'
        // DVG renderers use.
        vp_serial_drain(d->serial);
        vp_sleep_ms(100);
        d->cmdLen = 0;
        for (int i = 0; i < 16; i++)
            cmdAppend(d, DVG_FLAG_COMPLETE << 29);
        cmdAppend(d, DVG_FLAG_EXIT << 29);
        vp_serial_write(d->serial, d->cmd, (int)d->cmdLen, WRITE_TIMEOUT_MS);
        vp_serial_drain(d->serial);
        vp_sleep_ms(50);
        vp_serial_close(d->serial);
    }
    vp_window_close(d->window);
    freeHandle(d);
}

vp_dvg_output vp_dvg_get_output(const vp_dvg *d)
{
    return d->output;
}

vp_hardware vp_dvg_hardware(const vp_dvg *d)
{
    return d->hardware;
}

const char *vp_hardware_name(vp_hardware hw)
{
    switch (hw) {
    case VP_HW_WINDOW:    return "window";
    case VP_HW_USB_DVG:   return "USB-DVG";
    case VP_HW_BEAMFORGE: return "BeamForge";
    }
    return "unknown";
}

const char *vp_dvg_port(const vp_dvg *d)
{
    return d->port;
}

int vp_dvg_window_closed(vp_dvg *d)
{
    return d->window ? vp_window_closed(d->window) : 0;
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void vp_dvg_set_clip(vp_dvg *d, int x0, int y0, int x1, int y1)
{
    d->clipL = x0 < x1 ? x0 : x1;
    d->clipR = x0 < x1 ? x1 : x0;
    d->clipB = y0 < y1 ? y0 : y1;
    d->clipT = y0 < y1 ? y1 : y0;
    if (d->clipL < 0) d->clipL = 0;
    if (d->clipB < 0) d->clipB = 0;
    if (d->clipR > VP_DVG_COORD_MAX) d->clipR = VP_DVG_COORD_MAX;
    if (d->clipT > VP_DVG_COORD_MAX) d->clipT = VP_DVG_COORD_MAX;
}

void vp_dvg_reset_clip(vp_dvg *d)
{
    d->clipL = 0; d->clipB = 0;
    d->clipR = VP_DVG_COORD_MAX; d->clipT = VP_DVG_COORD_MAX;
}

void vp_dvg_begin_frame(vp_dvg *d)
{
    d->vecCnt = 0;
    d->stats.clipped_away = 0;
    d->stats.dropped = 0;
}

void vp_color(vp_dvg *d, uint8_t r, uint8_t g, uint8_t b)
{
    d->penR = r; d->penG = g; d->penB = b;
}

void vp_get_color(const vp_dvg *d, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (r) *r = d->penR;
    if (g) *g = d->penG;
    if (b) *b = d->penB;
}

void vp_moveto(vp_dvg *d, int x, int y)
{
    // Nothing is encoded here: the encoder inserts a blank move only when the
    // next line does not start where the beam is.
    d->penX = x;
    d->penY = y;
}

void vp_lineto(vp_dvg *d, int x, int y)
{
    int32_t x0 = d->penX, y0 = d->penY, x1 = x, y1 = y;
    d->penX = x;
    d->penY = y;

    // Black draws nothing: it is a move.
    if (d->penR == 0 && d->penG == 0 && d->penB == 0) return;

    if (!clipLine(d, &x0, &y0, &x1, &y1)) {
        d->stats.clipped_away++;
        return;
    }
    if (d->vecCnt >= VP_DVG_MAX_VECTORS) {
        d->stats.dropped++;
        return;
    }
    vp_vec &v = d->vecs[d->vecCnt++];
    v.x0 = x0; v.y0 = y0; v.x1 = x1; v.y1 = y1;
    v.r = d->penR; v.g = d->penG; v.b = d->penB;
}

int vp_dvg_end_frame(vp_dvg *d, int monochrome)
{
    const int32_t startX = d->beamX, startY = d->beamY;

    d->cmdLen = 0;
    d->lastR = d->lastG = d->lastB = -1;
    uint32_t blanks = 0;
    uint64_t litLen = 0, blankLen = 0;
    int32_t bx = d->beamX, by = d->beamY;
    for (uint32_t i = 0; i < d->vecCnt; i++) {
        const vp_vec &v = d->vecs[i];
        if (bx != v.x0 || by != v.y0) {
            cmdAddPoint(d, v.x0, v.y0, 0, 0, 0);
            blanks++;
            blankLen += segLength(bx, by, v.x0, v.y0);
        }
        cmdAddPoint(d, v.x1, v.y1, v.r, v.g, v.b);
        litLen += segLength(v.x0, v.y0, v.x1, v.y1);
        bx = v.x1;
        by = v.y1;
    }
    d->beamX = bx;
    d->beamY = by;
    cmdAppend(d, (DVG_FLAG_COMPLETE << 29) | (monochrome ? DVG_COMPLETE_MONOCHROME : 0u));

    d->stats.vectors = d->vecCnt;
    d->stats.blank_moves = blanks;
    d->stats.words = d->cmdLen / 4;
    d->stats.lit_length = litLen > UINT32_MAX ? UINT32_MAX : (uint32_t)litLen;
    d->stats.blank_length = blankLen > UINT32_MAX ? UINT32_MAX : (uint32_t)blankLen;
    d->stats.frames_sent++;
    if (d->stats.dropped) d->stats.frames_with_drops++;

    if (d->window) {
        vp_window_frame(d->window, d->vecs, d->vecCnt, startX, startY);
        return VP_OK;
    }

    for (uint32_t off = 0; off < d->cmdLen; off += WRITE_CHUNK) {
        uint32_t n = d->cmdLen - off;
        if (n > (uint32_t)WRITE_CHUNK) n = WRITE_CHUNK;
        int r = vp_serial_write(d->serial, d->cmd + off, (int)n, WRITE_TIMEOUT_MS);
        if (r != VP_OK) {
            d->stats.write_errors++;
            return r;
        }
        d->stats.bytes_sent += n;
    }
    vp_serial_drain(d->serial);
    return VP_OK;
}

void vp_dvg_get_stats(const vp_dvg *d, vp_dvg_stats *stats)
{
    *stats = d->stats;
}
