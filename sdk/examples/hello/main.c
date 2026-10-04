/* hello — a small complete game on the VectorPie SDK.
 *
 * Shoot the falling diamonds before they reach the ground. It shows the
 * three pieces a game is made of:
 *   vp_run     the loop: update() at a fixed 60 Hz, draw() once per frame
 *   vp_input   the cabinet's controls by their advmame.rc names
 *   vp_text    text, drawn as strokes
 *
 * Controls (AdvanceMAME's defaults unless the cabinet's advmame.rc says
 * otherwise): P1 stick or arrows to move, P1 button 1 (left Ctrl) to fire,
 * START 1 (key 1) to play, the cabinet's exit button (Esc) to go back to the
 * menu.
 */
#include <vectorpie/vp.h>

#include <stdio.h>

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
                break;
            }
        }
        if (tg->live && tg->y < 150) {
            tg->live = 0;
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
        vp_color(d, 0, 255, 255);
        vp_text(d, "VECTORPIE", W / 2.0f, 2600, 400, VP_TEXT_CENTER);
        vp_color(d, 255, 255, 255);
        vp_text(d, "HELLO, SDK", W / 2.0f, 2100, 180, VP_TEXT_CENTER);
        if ((int)(G.state_time * 2) % 2 == 0) {
            vp_color(d, 255, 255, 0);
            vp_text(d, "PRESS START", W / 2.0f, 1300, 160, VP_TEXT_CENTER);
        }
        return;
    }

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
    desc.update = update;
    desc.draw   = draw;
    return vp_run(&desc, argc, argv);
}
