/* saver-spectrum — a screensaver: the music as a spectrum analyzer.
 *
 * Thirty-two bars, low notes on the left, mirrored above and below a line,
 * with caps that hang at each bar's peak and drift down. Each beat (from the
 * track's beat map, or a hit heard when it has none) bumps it all: taller
 * at once, a little wider, then springing back; more on the downbeat, which
 * also flashes a frame round the screen. The track's title scrolls along
 * the bottom.
 *
 * On the cabinet: chosen in SETTINGS -> SCREENSAVERS -> SELECT once installed (its
 * game.ini says kind = screensaver; vp-pack makes a .vps). Any control wakes
 * the cabinet, START 2 skips to the next track.
 * On a PC: saver-spectrum <a .ogg or a folder of them> (a <track>.beats
 * beside a track gives its beats). Esc quits.
 */
#include <vectorpie/vp.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#define W VP_DVG_COORD_MAX

static struct {
    vp_music_feed m;
    int   have;
    float caps[VP_MUSIC_BANDS];
    unsigned last_beat, last_onset, last_track;
    float flash;                /* 1 at a downbeat, fading */
    float scroll;               /* how far the title has moved, board units */
    float bump, since_bump;     /* the last bump's size, and s since it */
} G;

/* The bump now: all at once, then a spring back, a little under, and still. */
static float bump_now(void)
{
    return G.bump * expf(-G.since_bump / 0.07f) * cosf(6.2831853f * G.since_bump / 0.24f);
}

static void update(vp_game *g, float dt)
{
    (void)g;
    G.have = vp_saver_music(&G.m);
    if (!G.have) return;
    if (G.m.track != G.last_track) {            /* a new track: no flash for the jump */
        G.last_track = G.m.track;
        G.last_beat = G.m.beats;
        G.last_onset = G.m.onsets;
        G.scroll = 0;                           /* its title comes in from the right */
    }
    G.scroll += dt * 700;
    G.since_bump += dt;
    /* the beat map's beats; else the hits heard, not closer than 250 ms
     * (they come in bursts: the bars would shake, not bump) */
    int beat = G.m.has_beats ? G.m.beats != G.last_beat
                             : G.m.onsets != G.last_onset && G.since_bump > 0.25f;
    if (beat) {
        int down = G.m.has_beats && G.m.beat_in_bar == 1;
        G.bump = down ? 0.17f : 0.10f;
        G.since_bump = 0;
        if (down) G.flash = 1;
    }
    G.last_beat = G.m.beats;
    G.last_onset = G.m.onsets;
    G.flash -= dt * 3;
    if (G.flash < 0) G.flash = 0;
    for (int b = 0; b < VP_MUSIC_BANDS; b++) {
        G.caps[b] -= dt * 0.35f;
        if (G.m.bands[b] > G.caps[b]) G.caps[b] = G.m.bands[b];
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
    const float k = bump_now();                 /* taller by k, wider by a third of it */
    const float mid = W * 0.55f, half = W * 0.36f * (1 + k);
    const float span = W * 0.88f * (1 + k * 0.3f);
    const float x0 = (W - span) / 2, step = span / VP_MUSIC_BANDS, bar = step * 0.62f;
    if (!G.have) {
        vp_color(d, 120, 120, 140);
        vp_text(d, "NO MUSIC", W / 2.0f, mid, 140, VP_TEXT_CENTER);
        return;
    }
    vp_color(d, 70, 70, 110);
    vp_moveto(d, (int)x0, (int)mid);
    vp_lineto(d, (int)(x0 + step * VP_MUSIC_BANDS), (int)mid);
    for (int b = 0; b < VP_MUSIC_BANDS; b++) {
        float t = (float)b / (VP_MUSIC_BANDS - 1);
        /* red at the bass, through yellow and green, to blue at the top */
        uint8_t r = (uint8_t)(255 * fmaxf(0, 1 - 2 * t)), gr = (uint8_t)(255 * (1 - fabsf(2 * t - 1)));
        uint8_t bl = (uint8_t)(255 * fmaxf(0, 2 * t - 1));
        vp_color(d, r + (uint8_t)((255 - r) * 0.3f), gr, bl + 60 > 255 ? 255 : bl + 60);
        float x = x0 + step * b, h = G.m.bands[b] * half;
        if (h > 8) {
            vp_moveto(d, (int)x, (int)(mid - h));
            vp_lineto(d, (int)x, (int)(mid + h));
            vp_lineto(d, (int)(x + bar), (int)(mid + h));
            vp_lineto(d, (int)(x + bar), (int)(mid - h));
            vp_lineto(d, (int)x, (int)(mid - h));
        }
        float c = G.caps[b] * half + 30;
        vp_color(d, 230, 230, 255);
        vp_moveto(d, (int)x, (int)(mid + c));
        vp_lineto(d, (int)(x + bar), (int)(mid + c));
        vp_moveto(d, (int)x, (int)(mid - c));
        vp_lineto(d, (int)(x + bar), (int)(mid - c));
    }
    if (G.flash > 0) {                          /* the downbeat: a frame round it all */
        float f = G.flash;
        int m = (int)(60 + (1 - f) * 60);
        vp_color(d, (uint8_t)(255 * f), (uint8_t)(120 * f), (uint8_t)(255 * f));
        vp_moveto(d, m, m);
        vp_lineto(d, W - m, m);
        vp_lineto(d, W - m, W - m);
        vp_lineto(d, m, W - m);
        vp_lineto(d, m, m);
    }
    vp_text_font(VP_FONT_ASTEROIDS);
    vp_color(d, 160, 160, 190);
    if (G.m.title[0]) scroll_title(d, G.m.title, G.scroll, 300, 90);
}

int main(int argc, char **argv)
{
    vp_game_desc desc = {0};
    desc.title = "Spectrum screensaver";
    desc.update = update;
    desc.draw = draw;
    return vp_run(&desc, argc, argv);
}
