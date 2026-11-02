/* saver-vu — a screensaver: the music on two analog VU meters.
 *
 * Left and right, side by side, as on a tape deck: the classic scale from
 * -20 to +3 VU (spaced as the needle swings, by voltage, so 0 sits well to
 * the right; numbered without signs, as on the real ones), red from 0 up. The needles have a little weight: they swing
 * past and settle. A lamp over each meter lights on the peaks, and the
 * faces glow a little brighter on each beat. The track's title scrolls
 * along the bottom.
 *
 * On the cabinet: chosen in SETTINGS -> SCREENSAVERS -> SELECT once installed (its
 * game.ini says kind = screensaver; vp-pack makes a .vps). Any control wakes
 * the cabinet, START 2 skips to the next track.
 * On a PC: saver-vu <a .ogg or a folder of them>. Esc quits.
 */
#include <vectorpie/vp.h>

#include <math.h>
#include <string.h>

#define W VP_DVG_COORD_MAX
#define PI 3.14159265f
#define SWING (42 * PI / 180)   /* the needle's travel each side of upright */

static struct {
    vp_music_feed m;
    int   have;
    float needle[2], speed[2];  /* where each needle is on its scale (0 at -20, 1 at +3) */
    unsigned last_beat, last_onset, last_track;
    float glow;                 /* 1 at a beat, fading */
    float scroll;               /* how far the title has moved, board units */
} G;

/* Where a VU reading sits on the scale: 0 at -20, 1 at +3, by voltage. */
static float scale_at(float vu)
{
    const float lo = powf(10, -20 / 20.0f), hi = powf(10, 3 / 20.0f);
    return (powf(10, vu / 20.0f) - lo) / (hi - lo);
}

static void update(vp_game *g, float dt)
{
    (void)g;
    G.have = vp_saver_music(&G.m);
    if (G.have && G.m.track != G.last_track) {  /* a new track: no glow for the jump */
        G.last_track = G.m.track;
        G.last_beat = G.m.beats;
        G.last_onset = G.m.onsets;
        G.scroll = 0;
    }
    G.scroll += dt * 700;
    if (G.have && (G.m.has_beats ? G.m.beats != G.last_beat : G.m.onsets != G.last_onset))
        G.glow = G.m.has_beats && G.m.beat_in_bar == 1 ? 1.0f : 0.6f;
    G.last_beat = G.m.beats;
    G.last_onset = G.m.onsets;
    G.glow -= dt * 3;
    if (G.glow < 0) G.glow = 0;

    /* the needles: a spring with a little give, so they swing past and settle */
    const float w = 18, damp = 0.55f;
    for (int c = 0; c < 2; c++) {
        float target = G.have ? scale_at(G.m.vu[c]) : -0.04f;
        if (target < -0.04f) target = -0.04f;   /* resting on the left pin */
        for (int step = 0; step < 4; step++) {  /* in short steps: steady at any frame rate */
            float h = dt / 4;
            G.speed[c] += (w * w * (target - G.needle[c]) - 2 * damp * w * G.speed[c]) * h;
            G.needle[c] += G.speed[c] * h;
        }
        if (G.needle[c] < -0.04f) { G.needle[c] = -0.04f; G.speed[c] = 0; }   /* the pins */
        if (G.needle[c] > 1.06f)  { G.needle[c] = 1.06f;  G.speed[c] = 0; }
    }
}

/* A point at radius r from (px, py), at angle a from upright (right +). */
static void at(float px, float py, float r, float a, int *x, int *y)
{
    *x = (int)(px + r * sinf(a));
    *y = (int)(py + r * cosf(a));
}

static void arc(vp_dvg *d, float px, float py, float r, float a0, float a1)
{
    const int n = 16;
    int x, y;
    at(px, py, r, a0, &x, &y);
    vp_moveto(d, x, y);
    for (int i = 1; i <= n; i++) {
        at(px, py, r, a0 + (a1 - a0) * i / n, &x, &y);
        vp_lineto(d, x, y);
    }
}

static float angle_of(float s) { return -SWING + 2 * SWING * s; }

static void meter(vp_dvg *d, int side, float fx0, float fy0, float fx1, float fy1)
{
    static const struct { float vu; const char *label; } marks[] = {
        { -20, "20" }, { -10, "10" }, { -7, "7" }, { -5, "5" }, { -3, "3" },
        { -2, 0 }, { -1, 0 }, { 0, "0" }, { 1, "1" }, { 2, "2" }, { 3, "3" },
    };
    const float fh = fy1 - fy0, cx = (fx0 + fx1) / 2;
    const float px = cx, py = fy0 + fh * 0.10f, r = fh * 0.60f;   /* the needle's pivot, the scale */
    const float lit = 0.72f + 0.28f * G.glow;
    uint8_t a = (uint8_t)(255 * lit), b = (uint8_t)(185 * lit), c = (uint8_t)(90 * lit);
    int x, y, x2, y2;

    vp_color(d, 110, 80, 45);                   /* the case */
    vp_moveto(d, (int)fx0, (int)fy0);
    vp_lineto(d, (int)fx1, (int)fy0);
    vp_lineto(d, (int)fx1, (int)fy1);
    vp_lineto(d, (int)fx0, (int)fy1);
    vp_lineto(d, (int)fx0, (int)fy0);

    const float zero = angle_of(scale_at(0));
    vp_color(d, a, b, c);                       /* the scale: amber, then red from 0 */
    arc(d, px, py, r, -SWING, zero);
    vp_color(d, (uint8_t)(255 * lit), (uint8_t)(45 * lit), (uint8_t)(30 * lit));
    arc(d, px, py, r, zero, SWING);
    arc(d, px, py, r - 28, zero, SWING);        /* the red band is thick */
    arc(d, px, py, r - 56, zero, SWING);

    vp_text_font(VP_FONT_ASTEROIDS);
    for (size_t i = 0; i < sizeof marks / sizeof marks[0]; i++) {
        float t = angle_of(scale_at(marks[i].vu));
        if (marks[i].vu > 0) vp_color(d, (uint8_t)(255 * lit), (uint8_t)(45 * lit), (uint8_t)(30 * lit));
        else                 vp_color(d, a, b, c);
        at(px, py, r, t, &x, &y);
        at(px, py, r + (marks[i].label ? 70 : 40), t, &x2, &y2);
        vp_moveto(d, x, y);
        vp_lineto(d, x2, y2);
        if (marks[i].label) {
            at(px, py, r + 110, t, &x2, &y2);
            vp_text(d, marks[i].label, (float)x2, (float)y2, 60, VP_TEXT_CENTER);
        }
    }
    vp_color(d, a, b, c);
    vp_text(d, "VU", px, py + r * 0.45f, 150, VP_TEXT_CENTER);
    vp_text(d, side ? "RIGHT" : "LEFT", px, fy0 + fh * 0.04f, 60, VP_TEXT_CENTER);

    /* the peak lamp, top right: red while a peak is near the loudest */
    const float lx = fx1 - fh * 0.08f, ly = fy1 - fh * 0.08f, lr = fh * 0.025f;
    int on = G.have && G.m.peak[side] > -2.0f;
    if (on) vp_color(d, 255, 40, 30);
    else    vp_color(d, 80, 30, 25);
    arc(d, lx, ly, lr, 0, 2 * PI);
    if (on) arc(d, lx, ly, lr * 0.5f, 0, 2 * PI);

    /* the needle, from under its cover to just past the scale */
    float t = angle_of(G.needle[side]);
    vp_color(d, 255, 245, 225);
    at(px, py, r * 0.22f, t, &x, &y);
    at(px, py, r + 30, t, &x2, &y2);
    vp_moveto(d, x, y);
    vp_lineto(d, x2, y2);
    vp_color(d, 110, 80, 45);                   /* its cover */
    arc(d, px, py, r * 0.20f, -PI / 2, PI / 2);
}

/* The title, scrolling right to left along the bottom, coming back in at the
 * right once it has gone. Letter by letter, only those wholly between the
 * margins: a vector line cannot be cut at the edge. */
static void scroll_title(vp_dvg *d, const char *title, float scroll, float y, float height)
{
    const float x0 = W * 0.06f, x1 = W * 0.94f;
    char one[2] = { 0, 0 }, prefix[VP_MUSIC_TEXT];
    float w = vp_text_width(title, height);
    float x = x1 - fmodf(scroll, w + (x1 - x0));
    for (int i = 0; title[i] && i < VP_MUSIC_TEXT - 1; i++) {
        memcpy(prefix, title, (size_t)i);
        prefix[i] = 0;
        one[0] = title[i];
        float cx = x + vp_text_width(prefix, height), cw = vp_text_width(one, height);
        if (cx >= x0 && cx + cw <= x1) vp_text(d, one, cx, y, height, VP_TEXT_LEFT);
    }
}

static void draw(vp_game *g, vp_dvg *d)
{
    (void)g;
    const float y0 = W * 0.26f, y1 = W * 0.74f;
    meter(d, 0, W * 0.03f, y0, W * 0.485f, y1);
    meter(d, 1, W * 0.515f, y0, W * 0.97f, y1);
    vp_text_font(VP_FONT_ASTEROIDS);
    vp_color(d, 160, 160, 190);
    if (!G.have)           vp_text(d, "NO MUSIC", W / 2.0f, 300, 90, VP_TEXT_CENTER);
    else if (G.m.title[0]) scroll_title(d, G.m.title, G.scroll, 300, 90);
}

int main(int argc, char **argv)
{
    vp_game_desc desc = {0};
    desc.title = "VU meter screensaver";
    desc.update = update;
    desc.draw = draw;
    return vp_run(&desc, argc, argv);
}
