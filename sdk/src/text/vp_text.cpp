// Stroke text, drawn with vp_moveto / vp_lineto. Two fonts: Hershey Simplex
// and Asteroids.

#include "../core/vp_internal.h"

#include <cmath>
#include <cstdint>

namespace {

struct vp_glyph {
    uint8_t count;        // points, pen-up markers included
    uint8_t width;        // advance, spacing included
    int8_t  points[110];  // x,y pairs; (-1,-1) lifts the pen
};

#include "vp_font_hershey.inc"

struct AsteroidsGlyph {
    uint8_t points[8];    // AP(x, y), AF_UP or AF_LAST
};

#include "vp_font_asteroids.inc"

// A capital letter's height and the advance, in each font's own units.
const float HERSHEY_CAP = 21.0f;
const float ASTEROIDS_CAP = 12.0f;
const float ASTEROIDS_ADVANCE = 12.0f;   // 8 wide + 4 of spacing, as the firmware draws it

thread_local vp_font t_font = VP_FONT_HERSHEY;

const vp_glyph &hersheyGlyph(char c)
{
    int ch = (unsigned char)c;
    if (ch < 32 || ch > 126) ch = 32;
    return HERSHEY_SIMPLEX[ch - 32];
}

const AsteroidsGlyph &asteroidsGlyph(char c)
{
    int ch = (unsigned char)c;
    if (ch > 127) ch = 32;
    return ASTEROIDS[ch];
}

// Draw one glyph's strokes from (x, y) at `sx` (across) and `sy` (up) board
// units per font unit.
void drawHershey(vp_dvg *d, const vp_glyph &g, float x, float y, float sx, float sy)
{
    bool penDown = false;
    for (int i = 0; i < g.count; i++) {
        int gx = g.points[i * 2], gy = g.points[i * 2 + 1];
        if (gx == -1 && gy == -1) {
            penDown = false;
            continue;
        }
        int px = (int)std::lround(x + gx * sx);
        int py = (int)std::lround(y + gy * sy);
        if (penDown) vp_lineto(d, px, py);
        else         vp_moveto(d, px, py);
        penDown = true;
    }
}

void drawAsteroids(vp_dvg *d, const AsteroidsGlyph &g, float x, float y, float sx, float sy)
{
    bool penDown = false;
    for (int i = 0; i < 8; i++) {
        uint8_t p = g.points[i];
        if (p == AF_LAST) break;
        if (p == AF_UP) {
            penDown = false;
            continue;
        }
        int px = (int)std::lround(x + ((p >> 4) & 0xF) * sx);
        int py = (int)std::lround(y + (p & 0xF) * sy);
        if (penDown) vp_lineto(d, px, py);
        else         vp_moveto(d, px, py);
        penDown = true;
    }
}

}  // namespace

void vp_text_font(vp_font font)
{
    t_font = font == VP_FONT_ASTEROIDS ? VP_FONT_ASTEROIDS : VP_FONT_HERSHEY;
}

vp_font vp_text_get_font(void)
{
    return t_font;
}

float vp_text_width(const char *s, float height)
{
    if (!s) return 0.0f;
    float units = 0.0f;
    if (t_font == VP_FONT_ASTEROIDS) {
        for (const char *p = s; *p; p++) units += ASTEROIDS_ADVANCE;
        return units * height / ASTEROIDS_CAP;
    }
    for (const char *p = s; *p; p++)
        units += hersheyGlyph(*p).width;
    return units * height / HERSHEY_CAP;
}

// The string at `height`, its letters `squeeze` times their width (1: as
// designed), from x on the baseline y.
static void drawText(vp_dvg *d, const char *s, float x, float y, float height, float squeeze)
{
    float cursor = x;
    if (t_font == VP_FONT_ASTEROIDS) {
        const float scale = height / ASTEROIDS_CAP;
        for (const char *p = s; *p; p++) {
            drawAsteroids(d, asteroidsGlyph(*p), cursor, y, scale * squeeze, scale);
            cursor += ASTEROIDS_ADVANCE * scale * squeeze;
        }
        return;
    }
    const float scale = height / HERSHEY_CAP;
    for (const char *p = s; *p; p++) {
        const vp_glyph &g = hersheyGlyph(*p);
        drawHershey(d, g, cursor, y, scale * squeeze, scale);
        cursor += g.width * scale * squeeze;
    }
}

void vp_text(vp_dvg *d, const char *s, float x, float y, float height, int align)
{
    if (!d || !s) return;
    if (align == VP_TEXT_CENTER)     x -= vp_text_width(s, height) * 0.5f;
    else if (align == VP_TEXT_RIGHT) x -= vp_text_width(s, height);
    drawText(d, s, x, y, height, 1.0f);
}

void vp_text_fit(vp_dvg *d, const char *s, float x, float y, float height, float width, int align)
{
    if (!d || !s) return;
    const float natural = vp_text_width(s, height);
    const float squeeze = (width > 0.0f && natural > width) ? width / natural : 1.0f;
    const float drawn = natural * squeeze;
    if (align == VP_TEXT_CENTER)     x -= drawn * 0.5f;
    else if (align == VP_TEXT_RIGHT) x -= drawn;
    drawText(d, s, x, y, height, squeeze);
}
