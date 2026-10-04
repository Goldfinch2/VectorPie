// Stroke text with the Hershey Simplex font, drawn with vp_moveto / vp_lineto.

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

const float CAP_HEIGHT = 21.0f;   // a capital letter, in font units

const vp_glyph &glyphFor(char c)
{
    int ch = (unsigned char)c;
    if (ch < 32 || ch > 126) ch = 32;
    return HERSHEY_SIMPLEX[ch - 32];
}

}  // namespace

float vp_text_width(const char *s, float height)
{
    if (!s) return 0.0f;
    float units = 0.0f;
    for (const char *p = s; *p; p++)
        units += glyphFor(*p).width;
    return units * height / CAP_HEIGHT;
}

void vp_text(vp_dvg *d, const char *s, float x, float y, float height, int align)
{
    if (!d || !s) return;
    const float scale = height / CAP_HEIGHT;
    if (align == VP_TEXT_CENTER)     x -= vp_text_width(s, height) * 0.5f;
    else if (align == VP_TEXT_RIGHT) x -= vp_text_width(s, height);

    float cursor = x;
    for (const char *p = s; *p; p++) {
        const vp_glyph &g = glyphFor(*p);
        bool penDown = false;
        for (int i = 0; i < g.count; i++) {
            int gx = g.points[i * 2], gy = g.points[i * 2 + 1];
            if (gx == -1 && gy == -1) {
                penDown = false;
                continue;
            }
            int px = (int)std::lround(cursor + gx * scale);
            int py = (int)std::lround(y + gy * scale);
            if (penDown) vp_lineto(d, px, py);
            else         vp_moveto(d, px, py);
            penDown = true;
        }
        cursor += g.width * scale;
    }
}
