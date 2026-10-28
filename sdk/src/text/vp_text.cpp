// Stroke text, drawn with vp_moveto / vp_lineto. Three fonts: Hershey
// Simplex, Asteroids and the comic capitals (outlined letters, a border,
// a wobble; their shapes as triangles for vp3d_scene_text).

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

#include "vp_font_comic.inc"

// A capital letter's height and the advance, in each font's own units.
const float HERSHEY_CAP = 21.0f;
const float ASTEROIDS_CAP = 12.0f;
const float ASTEROIDS_ADVANCE = 12.0f;   // 8 wide + 4 of spacing, as the firmware draws it

// The comic font's units: em/1000, a capital 700 high; letters stand
// SPACING apart (their borders stay apart), turning about ORIGIN across.
const float COMIC_CAP = 700.0f;
const float COMIC_SPACING = 180.0f;
const float COMIC_SPACE = 300.0f;
const float COMIC_ORIGIN = 300.0f;

thread_local vp_font t_font = VP_FONT_HERSHEY;
thread_local uint8_t t_border[3] = { 0, 0, 0 };
thread_local float   t_wobble = 0.0f;
thread_local double  t_wobble_time = 0.0;

int comicIndex(char c)
{
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    for (int i = 0; i < (int)(sizeof COMIC_GLYPHS / sizeof COMIC_GLYPHS[0]); i++)
        if (COMIC_GLYPHS[i].ch == c) return i;
    return -1;
}

float comicAdvance(char c)
{
    int i = comicIndex(c);
    return i < 0 ? COMIC_SPACE : COMIC_GLYPHS[i].advance + COMIC_SPACING;
}

// Where letter number i stands: its foot's middle at (ox, oy), scale k,
// turned by (ca, sa), `squeeze` its width.
struct ComicPlace { float ox, oy, k, ca, sa, squeeze; };

ComicPlace comicPlace(float x, float y, float k, float squeeze, int i)
{
    const float t = (float)t_wobble_time;
    const float a = t_wobble * 0.07f * std::sin(t * 4.0f + i * 1.3f);
    const float lift = t_wobble * COMIC_CAP * k * 0.06f * std::sin(t * 5.0f + i * 0.9f);
    return { x + COMIC_ORIGIN * k * squeeze, y + lift, k, std::cos(a), std::sin(a), squeeze };
}

void comicPoint(const ComicPlace &p, const short *q, float *sx, float *sy)
{
    const float x = (q[0] - COMIC_ORIGIN) * p.k * p.squeeze, y = q[1] * p.k;
    *sx = p.ox + x * p.ca - y * p.sa;
    *sy = p.oy + x * p.sa + y * p.ca;
}

void drawComic(vp_dvg *d, char c, int border, const ComicPlace &p)
{
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    for (const ComicContour &cc : COMIC_CONTOURS) {
        if (cc.ch != c || cc.border != border) continue;
        for (int j = 0; j <= cc.count; j++) {
            float sx, sy;
            comicPoint(p, COMIC_PTS[cc.first + j % cc.count], &sx, &sy);
            if (j == 0) vp_moveto(d, (int)std::lround(sx), (int)std::lround(sy));
            else        vp_lineto(d, (int)std::lround(sx), (int)std::lround(sy));
        }
    }
}

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
    t_font = (font == VP_FONT_ASTEROIDS || font == VP_FONT_COMIC) ? font : VP_FONT_HERSHEY;
}

void vp_text_border(uint8_t r, uint8_t g, uint8_t b)
{
    t_border[0] = r;
    t_border[1] = g;
    t_border[2] = b;
}

void vp_text_wobble(float amount, double time)
{
    t_wobble = amount < 0.0f ? 0.0f : amount;
    t_wobble_time = time;
}

vp_font vp_text_get_font(void)
{
    return t_font;
}

float vp_text_width(const char *s, float height)
{
    if (!s) return 0.0f;
    float units = 0.0f;
    if (t_font == VP_FONT_COMIC) {
        for (const char *p = s; *p; p++) units += comicAdvance(*p);
        if (*s) units -= COMIC_SPACING;
        return units * height / COMIC_CAP;
    }
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
    if (t_font == VP_FONT_COMIC) {
        const float k = height / COMIC_CAP;
        const bool border = t_border[0] || t_border[1] || t_border[2];
        uint8_t r, g, b;
        vp_get_color(d, &r, &g, &b);
        // the letters, then their borders: one change of color, not one a
        // letter (the beam's color settles after a change, smearing one
        // outline's color into the other)
        for (int pass = 0; pass < (border ? 2 : 1); pass++) {
            if (pass) vp_color(d, t_border[0], t_border[1], t_border[2]);
            float at = cursor;
            int i = 0;
            for (const char *p = s; *p; p++, i++) {
                if (comicIndex(*p) >= 0) drawComic(d, *p, pass, comicPlace(at, y, k, squeeze, i));
                at += comicAdvance(*p) * k * squeeze;
            }
        }
        vp_color(d, r, g, b);
        return;
    }
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

// The comic font's letters as board-space triangles (vp3d_scene_text): laid
// out as vp_text lays them out; nothing for the other fonts.
void vp_text_screens(const char *s, float x, float y, float height, int align,
                     void (*triangle)(void *user, const float xy[6]), void *user)
{
    if (!s || !triangle || t_font != VP_FONT_COMIC) return;
    if (align == VP_TEXT_CENTER)     x -= vp_text_width(s, height) * 0.5f;
    else if (align == VP_TEXT_RIGHT) x -= vp_text_width(s, height);
    const float k = height / COMIC_CAP;
    int i = 0;
    for (const char *p = s; *p; p++, i++) {
        char c = *p;
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        if (comicIndex(c) >= 0) {
            const ComicPlace place = comicPlace(x, y, k, 1.0f, i);
            for (const ComicTriangle &t : COMIC_TRIANGLES) {
                if (t.ch != c) continue;
                float xy[6];
                comicPoint(place, COMIC_PTS[t.a], &xy[0], &xy[1]);
                comicPoint(place, COMIC_PTS[t.b], &xy[2], &xy[3]);
                comicPoint(place, COMIC_PTS[t.c], &xy[4], &xy[5]);
                triangle(user, xy);
            }
        }
        x += comicAdvance(*p) * k;
    }
}
