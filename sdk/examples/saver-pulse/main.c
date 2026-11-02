/* saver-pulse — a screensaver: shapes that breathe with the music.
 *
 * A star of 32 spokes, one per band of the spectrum, turning slowly; round
 * it, rings that leave the middle on each beat and widen as they fade, a
 * thick one on the downbeat. On a track without a beat map the hits heard
 * set the rings off instead. The colors move round the wheel bar by bar.
 * The track's title scrolls along the bottom.
 *
 * On the cabinet: chosen in SETTINGS -> SCREENSAVERS -> SELECT once installed (its
 * game.ini says kind = screensaver; vp-pack makes a .vps). Any control wakes
 * the cabinet, START 2 skips to the next track.
 * On a PC: saver-pulse <a .ogg or a folder of them>. Esc quits.
 */
#include <vectorpie/vp.h>

#include <math.h>
#include <string.h>

#define W VP_DVG_COORD_MAX
#define RINGS 8

typedef struct { float age; int strong; float hue; } ring;

static struct {
    vp_music_feed m;
    int   have;
    unsigned last_beat, last_onset, last_track;
    ring  rings[RINGS];
    float turn, hue, time;
    float scroll;               /* how far the title has moved, board units */
} G;

/* A hue (0..1 round the color wheel) as a bright color. */
static void hue_rgb(float h, uint8_t *r, uint8_t *g, uint8_t *b)
{
    static const float wheel[6][3] = { { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0, 1, 1 }, { 0, 0, 1 }, { 1, 0, 1 } };
    h = fmodf(h, 1.0f) * 6;
    if (h < 0) h += 6;
    int i = (int)h % 6, j = (i + 1) % 6;
    float f = h - (int)h, c[3];
    for (int k = 0; k < 3; k++) c[k] = wheel[i][k] + (wheel[j][k] - wheel[i][k]) * f;
    *r = (uint8_t)(70 + 185 * c[0]); *g = (uint8_t)(70 + 185 * c[1]); *b = (uint8_t)(70 + 185 * c[2]);
}

static void start_ring(int strong)
{
    for (int i = 0; i < RINGS; i++)
        if (G.rings[i].age <= 0 || G.rings[i].age > 1) {
            G.rings[i].age = 0.0001f;
            G.rings[i].strong = strong;
            G.rings[i].hue = G.hue;
            return;
        }
}

static void update(vp_game *g, float dt)
{
    (void)g;
    G.time += dt;
    G.have = vp_saver_music(&G.m);
    for (int i = 0; i < RINGS; i++)
        if (G.rings[i].age > 0) G.rings[i].age += dt * 0.7f;
    if (!G.have) return;
    if (G.m.track != G.last_track) {
        G.last_track = G.m.track;
        G.last_beat = G.m.beats;
        G.last_onset = G.m.onsets;
        G.scroll = 0;                           /* its title comes in from the right */
    }
    G.scroll += dt * 700;
    if (G.m.has_beats && G.m.beats != G.last_beat) {
        int down = G.m.beat_in_bar == 1;
        if (down) G.hue += 0.13f;
        start_ring(down);
    } else if (!G.m.has_beats && G.m.onsets != G.last_onset) {
        start_ring(0);
    }
    G.last_beat = G.m.beats;
    G.last_onset = G.m.onsets;
    G.turn += dt * (0.15f + G.m.level * 0.8f);
}

static void circle(vp_dvg *d, float r, int sides, float a0)
{
    for (int i = 0; i <= sides; i++) {
        float a = a0 + 6.2831853f * i / sides;
        int x = W / 2 + (int)(cosf(a) * r * 0.75f), y = W / 2 + (int)(sinf(a) * r);
        if (i == 0) vp_moveto(d, x, y); else vp_lineto(d, x, y);
    }
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
    uint8_t r, gr, b;
    for (int i = 0; i < RINGS; i++) {
        const ring *k = &G.rings[i];
        if (k->age <= 0 || k->age > 1) continue;
        float f = 1 - k->age;
        hue_rgb(k->hue, &r, &gr, &b);
        vp_color(d, (uint8_t)(r * f), (uint8_t)(gr * f), (uint8_t)(b * f));
        float rad = 350 + k->age * 1700;
        circle(d, rad, 24, G.turn);
        if (k->strong) circle(d, rad + 40, 24, G.turn);
    }
    if (!G.have) {
        vp_color(d, 100, 100, 130);
        circle(d, 400 + 40 * sinf(G.time * 2), 6, G.time * 0.3f);
        return;
    }
    /* the star: a spoke per band, its length the band's loudness */
    for (int s = 0; s < VP_MUSIC_BANDS; s++) {
        float a = G.turn + 6.2831853f * s / VP_MUSIC_BANDS;
        float in = 220, out = 260 + G.m.bands[s] * 1200;
        hue_rgb(G.hue + (float)s / VP_MUSIC_BANDS * 0.5f, &r, &gr, &b);
        vp_color(d, r, gr, b);
        vp_moveto(d, W / 2 + (int)(cosf(a) * in * 0.75f), W / 2 + (int)(sinf(a) * in));
        vp_lineto(d, W / 2 + (int)(cosf(a) * out * 0.75f), W / 2 + (int)(sinf(a) * out));
    }
    vp_color(d, 230, 230, 255);
    circle(d, 200 + G.m.level * 120, 16, -G.turn);
    vp_text_font(VP_FONT_ASTEROIDS);
    vp_color(d, 180, 180, 210);
    if (G.m.title[0]) scroll_title(d, G.m.title, G.scroll, 240, 90);
}

int main(int argc, char **argv)
{
    vp_game_desc desc = {0};
    desc.title = "Pulse screensaver";
    desc.update = update;
    desc.draw = draw;
    return vp_run(&desc, argc, argv);
}
