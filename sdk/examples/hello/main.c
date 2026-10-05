/* hello — a small complete game on the VectorPie SDK.
 *
 * Shoot the falling diamonds before they reach the ground. It shows the
 * pieces a game is made of:
 *   vp_run     the loop: update() at a fixed 60 Hz, draw() once per frame
 *   vp_input   the cabinet's controls by their advmame.rc names
 *   vp_text    text, drawn as strokes
 *   vp_audio   effects, and a streamed loop
 *
 * Sounds: if assets/sounds/ next to the executable holds shipfire.wav and
 * explosn.wav (Rip Off's, from a MAME sample set: see README.md), they are
 * used; music.wav (any track), or else bg1.wav, is streamed as the
 * background loop. Otherwise the game makes its own sounds in code.
 *
 * Controls (AdvanceMAME's defaults unless the cabinet's advmame.rc says
 * otherwise): P1 stick or arrows to move, P1 button 1 (left Ctrl) to fire,
 * START 1 (key 1) to play, the cabinet's exit button (Esc) to go back to the
 * menu.
 */
#include <vectorpie/vp.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define W VP_DVG_COORD_MAX           /* the board space is 0..4095 */
#define MAX_SHOTS    8
#define MAX_TARGETS  6
#define SHIP_SPEED   2200.0f        /* board units per second */
#define SHOT_SPEED   3600.0f
#define START_LIVES  3

typedef struct { float x, y; int live; } thing;

enum { TITLE, PLAYING, GAME_OVER };

static struct {
    int      state;
    float    state_time;            /* seconds in the current state */
    float    ship_x, ship_y;
    thing    shots[MAX_SHOTS];
    thing    targets[MAX_TARGETS];
    float    spawn_timer;
    int      score, best, lives;
    unsigned rng;
    vp_sound *zap, *boom, *hum;
    vp_voice  music;
} G;

/* One small deterministic random generator: the same inputs, the same game. */
static float rnd(void)
{
    G.rng = G.rng * 1664525u + 1013904223u;
    return (float)(G.rng >> 8) / 16777216.0f;
}

static void start_game(void)
{
    G.state = PLAYING;
    G.state_time = 0;
    G.ship_x = W / 2.0f;
    G.ship_y = 400;
    for (int i = 0; i < MAX_SHOTS; i++) G.shots[i].live = 0;
    for (int i = 0; i < MAX_TARGETS; i++) G.targets[i].live = 0;
    G.spawn_timer = 0;
    G.score = 0;
    G.lives = START_LIVES;
}

/* ---- sounds -------------------------------------------------------------- */

#define SYNTH_RATE 22050

/* A sound made in code: `seconds` long, each sample from gen(t, i). */
static vp_sound *synth(vp_audio *a, float seconds, float (*gen)(float t, int i))
{
    int n = (int)(seconds * SYNTH_RATE);
    float *buf = malloc(sizeof(float) * (size_t)n);
    vp_sound *s = NULL;
    if (!buf) return NULL;
    for (int i = 0; i < n; i++) buf[i] = gen((float)i / SYNTH_RATE, i);
    vp_sound_from_samples(a, buf, n, 1, SYNTH_RATE, &s);
    free(buf);
    return s;
}

static float gen_zap(float t, int i)       /* a falling chirp */
{
    (void)i;
    float f = 1800.0f - 9000.0f * t;
    return 0.5f * sinf(6.2831853f * f * t) * (1.0f - t / 0.15f);
}

static float gen_boom(float t, int i)      /* noise, falling away */
{
    unsigned x = (unsigned)i * 2654435761u;
    x ^= x >> 13;
    float noise = (float)(x & 0xffff) / 32768.0f - 1.0f;
    return 0.7f * noise * expf(-6.0f * t);
}

static float gen_hum(float t, int i)       /* a low two-tone hum, loops cleanly */
{
    (void)i;
    return 0.15f * sinf(6.2831853f * 55.0f * t) + 0.08f * sinf(6.2831853f * 110.0f * t);
}

static int file_exists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) fclose(f);
    return f != NULL;
}

static int init(vp_game *g)
{
    vp_audio *a = vp_game_audio(g);
    char path[1024];
    if (!a) return VP_OK;                   /* no sound device: play silent */

    if (vp_sound_load(a, vp_asset_path("assets/sounds/shipfire.wav", path, sizeof path), &G.zap) != VP_OK)
        G.zap = synth(a, 0.15f, gen_zap);
    if (vp_sound_load(a, vp_asset_path("assets/sounds/explosn.wav", path, sizeof path), &G.boom) != VP_OK)
        G.boom = synth(a, 0.8f, gen_boom);

    /* Background: a music track if there is one, else Rip Off's engine hum,
     * else a hum made in code. A file is streamed from disk as it plays. */
    static const char *const backgrounds[] = { "assets/sounds/music.wav", "assets/sounds/bg1.wav" };
    for (int i = 0; i < 2 && !G.music; i++) {
        vp_asset_path(backgrounds[i], path, sizeof path);
        if (file_exists(path)) {
            G.music = vp_music_play(a, path, 0.5f, 1, 2.0f);   /* streamed, looped, fades in */
            if (G.music) printf("hello: streaming %s\n", backgrounds[i]);
        }
    }
    if (!G.music) {
        G.hum = synth(a, 1.0f, gen_hum);                        /* whole cycles: loops cleanly */
        G.music = vp_sound_play(a, G.hum, 0.0f, 1.0f, 1);
        vp_voice_set_volume(a, G.music, 1.0f, 2.0f);
        printf("hello: background made in code\n");
    }
    return VP_OK;
}

/* ---- update: game logic, 60 times a second --------------------------- */

static void update(vp_game *g, float dt)
{
    vp_input *in = vp_game_input(g);
    G.state_time += dt;

    if (G.state != PLAYING) {
        if (vp_input_pressed(in, "start1") || vp_input_pressed(in, "p1_button1"))
            if (G.state == TITLE || G.state_time > 1.0f) start_game();
        return;
    }

    /* Move: the analog stick if there is one, the digital directions too. */
    float mx = vp_input_axis(in, "p1_stickx");
    float my = -vp_input_axis(in, "p1_sticky");     /* stick up is negative */
    if (vp_input_held(in, "p1_left"))  mx = -1;
    if (vp_input_held(in, "p1_right")) mx = 1;
    if (vp_input_held(in, "p1_down"))  my = -1;
    if (vp_input_held(in, "p1_up"))    my = 1;
    G.ship_x += mx * SHIP_SPEED * dt;
    G.ship_y += my * SHIP_SPEED * dt;
    if (G.ship_x < 150) G.ship_x = 150;
    if (G.ship_x > W - 150) G.ship_x = W - 150;
    if (G.ship_y < 250) G.ship_y = 250;
    if (G.ship_y > 1600) G.ship_y = 1600;

    /* Fire */
    if (vp_input_pressed(in, "p1_button1")) {
        for (int i = 0; i < MAX_SHOTS; i++) {
            if (!G.shots[i].live) {
                G.shots[i].x = G.ship_x;
                G.shots[i].y = G.ship_y + 120;
                G.shots[i].live = 1;
                vp_sound_play(vp_game_audio(g), G.zap, 0.8f, 1.0f, 0);
                break;
            }
        }
    }
    for (int i = 0; i < MAX_SHOTS; i++) {
        if (!G.shots[i].live) continue;
        G.shots[i].y += SHOT_SPEED * dt;
        if (G.shots[i].y > W) G.shots[i].live = 0;
    }

    /* Targets fall faster as the score grows. */
    G.spawn_timer -= dt;
    if (G.spawn_timer <= 0) {
        G.spawn_timer = 1.2f - G.score / 1000.0f;
        if (G.spawn_timer < 0.35f) G.spawn_timer = 0.35f;
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (!G.targets[i].live) {
                G.targets[i].x = 300 + rnd() * (W - 600);
                G.targets[i].y = W - 300;
                G.targets[i].live = 1;
                break;
            }
        }
    }
    float fall = 500.0f + G.score * 2.0f;
    for (int t = 0; t < MAX_TARGETS; t++) {
        thing *tg = &G.targets[t];
        if (!tg->live) continue;
        tg->y -= fall * dt;
        for (int s = 0; s < MAX_SHOTS; s++) {
            thing *sh = &G.shots[s];
            if (sh->live && sh->x > tg->x - 130 && sh->x < tg->x + 130 &&
                sh->y > tg->y - 130 && sh->y < tg->y + 130) {
                sh->live = tg->live = 0;
                G.score += 10;
                /* a little higher each time: the pitch control */
                vp_sound_play(vp_game_audio(g), G.boom, 0.9f, 1.0f + (G.score % 50) / 100.0f, 0);
                break;
            }
        }
        if (tg->live && tg->y < 150) {
            tg->live = 0;
            vp_sound_play(vp_game_audio(g), G.boom, 1.0f, 0.5f, 0);   /* low: a miss */
            if (--G.lives == 0) {
                G.state = GAME_OVER;
                G.state_time = 0;
                if (G.score > G.best) G.best = G.score;
            }
        }
    }
}

/* ---- draw: once per frame, in beam order ----------------------------- */

static void draw_ship(vp_dvg *d, float x, float y)
{
    vp_moveto(d, (int)x, (int)(y + 120));
    vp_lineto(d, (int)(x + 90), (int)(y - 80));
    vp_lineto(d, (int)x, (int)(y - 30));
    vp_lineto(d, (int)(x - 90), (int)(y - 80));
    vp_lineto(d, (int)x, (int)(y + 120));
}

static void draw_diamond(vp_dvg *d, float x, float y, float r)
{
    vp_moveto(d, (int)x, (int)(y + r));
    vp_lineto(d, (int)(x + r), (int)y);
    vp_lineto(d, (int)x, (int)(y - r));
    vp_lineto(d, (int)(x - r), (int)y);
    vp_lineto(d, (int)x, (int)(y + r));
}

static void draw(vp_game *g, vp_dvg *d)
{
    char line[64];
    (void)g;

    /* the ground */
    vp_color(d, 0, 90, 0);
    vp_moveto(d, 0, 150);
    vp_lineto(d, W, 150);

    if (G.state == TITLE) {
        vp_text_font(VP_FONT_ASTEROIDS);            /* the arcade lettering */
        vp_color(d, 0, 255, 255);
        vp_text(d, "VECTORPIE", W / 2.0f, 2600, 400, VP_TEXT_CENTER);
        vp_text_font(VP_FONT_HERSHEY);              /* round letters, lower case */
        vp_color(d, 255, 255, 255);
        vp_text(d, "Hello, SDK", W / 2.0f, 2100, 180, VP_TEXT_CENTER);
        vp_text_font(VP_FONT_ASTEROIDS);
        if ((int)(G.state_time * 2) % 2 == 0) {
            vp_color(d, 255, 255, 0);
            vp_text(d, "PRESS START", W / 2.0f, 1300, 160, VP_TEXT_CENTER);
        }
        return;
    }

    vp_text_font(VP_FONT_ASTEROIDS);
    vp_color(d, 255, 255, 255);
    snprintf(line, sizeof(line), "SCORE %d", G.score);
    vp_text(d, line, 200, W - 250, 120, VP_TEXT_LEFT);
    snprintf(line, sizeof(line), "LIVES %d", G.lives);
    vp_text(d, line, W - 200.0f, W - 250, 120, VP_TEXT_RIGHT);

    vp_color(d, 255, 60, 200);
    for (int i = 0; i < MAX_TARGETS; i++)
        if (G.targets[i].live) draw_diamond(d, G.targets[i].x, G.targets[i].y, 110);

    vp_color(d, 255, 255, 0);
    for (int i = 0; i < MAX_SHOTS; i++) {
        if (!G.shots[i].live) continue;
        vp_moveto(d, (int)G.shots[i].x, (int)G.shots[i].y);
        vp_lineto(d, (int)G.shots[i].x, (int)G.shots[i].y + 90);
    }

    if (G.state == PLAYING) {
        vp_color(d, 0, 255, 0);
        draw_ship(d, G.ship_x, G.ship_y);
    } else {
        vp_color(d, 255, 80, 80);
        vp_text(d, "GAME OVER", W / 2.0f, 2200, 300, VP_TEXT_CENTER);
        vp_color(d, 255, 255, 255);
        snprintf(line, sizeof(line), "BEST %d", G.best);
        vp_text(d, line, W / 2.0f, 1800, 140, VP_TEXT_CENTER);
    }
}

int main(int argc, char **argv)
{
    vp_game_desc desc = {0};
    G.state = TITLE;
    G.rng = 12345;

    desc.title  = "Hello VectorPie";
    desc.init   = init;
    desc.update = update;
    desc.draw   = draw;
    return vp_run(&desc, argc, argv);
}
