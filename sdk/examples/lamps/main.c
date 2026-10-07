/* lamps — drive the cabinet's START button lamps.
 *
 *   lamps                (with --windowed, --dvg ... as usual)
 *
 * The screen shows the two START buttons as the lamps are now. P1 left/right
 * picks a pattern:
 *
 *   CREDITS    what a game does: COIN adds a credit; START 1 blinks with a
 *              credit, START 2 too with two; START takes them and "plays"
 *              for 3 seconds (lamps off), then back to waiting
 *   OFF, ON, BLINK, ALTERNATE, START 1 ONLY, START 2 ONLY, CHASE
 *
 * Exit button to quit; the lamps go back as they were.
 *
 * It shows: vp_lamps_set (NUM LOCK = START 1, CAPS LOCK = START 2, the
 * keyboard LEDs the VectorPie menu passes to the panel), and vp_credits
 * (coins, credits, the blinking START lamps of a game waiting to start).
 * On Linux (the cabinet) the keyboard LEDs light; on Windows the lamp calls
 * do nothing, and the screen still shows what they would be.
 */
#include <vectorpie/vp.h>

#include <math.h>
#include <stdio.h>

enum { PAT_CREDITS, PAT_OFF, PAT_ON, PAT_BLINK, PAT_ALTERNATE, PAT_START1, PAT_START2,
       PAT_CHASE, PAT_COUNT };

static const char *const PATTERN_NAME[PAT_COUNT] = {
    "CREDITS", "OFF", "ON", "BLINK", "ALTERNATE", "START 1 ONLY", "START 2 ONLY", "CHASE"
};

static struct {
    int        pattern;
    vp_credits credits;
    float      playing;     /* CREDITS: seconds of the pretend game left */
    int        players;     /* ... for 1 or 2 players */
    unsigned   lamps;       /* what was last set, for the screen */
} L;

/* The lamps for this moment of a pattern. */
static unsigned patternLamps(int pattern, double t)
{
    int half = fmod(t, 1.0) < 0.5;                      /* twice a second */
    switch (pattern) {
    case PAT_OFF:       return 0;
    case PAT_ON:        return VP_LAMP_START1 | VP_LAMP_START2;
    case PAT_BLINK:     return half ? VP_LAMP_START1 | VP_LAMP_START2 : 0;
    case PAT_ALTERNATE: return half ? VP_LAMP_START1 : VP_LAMP_START2;
    case PAT_START1:    return half ? VP_LAMP_START1 : 0;
    case PAT_START2:    return half ? VP_LAMP_START2 : 0;
    case PAT_CHASE: {                                   /* 1, 1+2, 2, none, ... */
        static const unsigned step[4] = { VP_LAMP_START1, VP_LAMP_START1 | VP_LAMP_START2,
                                          VP_LAMP_START2, 0 };
        return step[(int)(t * 5) % 4];
    }
    default:            return 0;
    }
}

static void update(vp_game *g, float dt)
{
    vp_input *in = vp_game_input(g);
    double t = vp_game_time(g);

    if (vp_input_pressed(in, "p1_right")) L.pattern = (L.pattern + 1) % PAT_COUNT;
    if (vp_input_pressed(in, "p1_left"))  L.pattern = (L.pattern + PAT_COUNT - 1) % PAT_COUNT;

    vp_credits_update(&L.credits, in);    /* coins count in every pattern */

    if (L.pattern != PAT_CREDITS) {
        L.playing = 0;
        L.lamps = patternLamps(L.pattern, t);
        vp_lamps_set(L.lamps);
        return;
    }

    /* CREDITS: a game waiting for START, or "playing" (lamps off) */
    if (L.playing > 0) {
        L.playing -= dt;
        L.lamps = 0;
        vp_lamps_set(0);
        return;
    }
    if (vp_input_pressed(in, "start1") && vp_credits_take(&L.credits, 1)) {
        L.playing = 3.0f;
        L.players = 1;
    } else if (vp_input_pressed(in, "start2") && vp_credits_take(&L.credits, 2)) {
        L.playing = 3.0f;
        L.players = 2;
    }
    /* what vp_credits_blink sets, kept for the screen */
    L.lamps = fmod(t, 0.5) < 0.25
        ? (vp_credits_enough(&L.credits, 1) ? VP_LAMP_START1 : 0) |
          (vp_credits_enough(&L.credits, 2) ? VP_LAMP_START2 : 0)
        : 0;
    vp_credits_blink(&L.credits, t);
}

/* A START button: a ring, filled with rings when lit. */
static void drawButton(vp_dvg *d, float cx, float cy, int lit, const char *label)
{
    const int SEG = 36;
    int rings = lit ? 6 : 1;
    if (lit) vp_color(d, 255, 60, 40);
    else     vp_color(d, 90, 30, 25);
    for (int r = 0; r < rings; r++) {
        float rad = 380.0f - r * 60.0f;
        vp_moveto(d, (int)(cx + rad), (int)cy);
        for (int i = 1; i <= SEG; i++) {
            float a = 6.2831853f * i / SEG;
            vp_lineto(d, (int)(cx + rad * cosf(a)), (int)(cy + rad * sinf(a)));
        }
    }
    vp_color(d, 255, 255, 255);
    vp_text(d, label, cx, cy - 600, 110, VP_TEXT_CENTER);
}

static void draw(vp_game *g, vp_dvg *d)
{
    char text[64];
    (void)g;
    vp_text_font(VP_FONT_ASTEROIDS);

    vp_color(d, 0, 200, 255);
    vp_text(d, "START LAMPS", 2048, 3650, 170, VP_TEXT_CENTER);
    vp_color(d, 255, 255, 255);
    snprintf(text, sizeof text, "< %s >", PATTERN_NAME[L.pattern]);
    vp_text(d, text, 2048, 3300, 130, VP_TEXT_CENTER);

    drawButton(d, 1300, 2000, (L.lamps & VP_LAMP_START1) != 0, "1 PLAYER");
    drawButton(d, 2796, 2000, (L.lamps & VP_LAMP_START2) != 0, "2 PLAYERS");

    vp_color(d, 255, 255, 255);
    if (L.pattern == PAT_CREDITS) {
        if (L.playing > 0)
            snprintf(text, sizeof text, "%d PLAYER GAME ON", L.players);
        else
            snprintf(text, sizeof text, "%s", vp_credits_enough(&L.credits, 2) ? "PRESS 1 OR 2 PLAYER START"
                                         : vp_credits_enough(&L.credits, 1) ? "PRESS 1 PLAYER START"
                                         : "INSERT COIN");
        vp_text(d, text, 2048, 900, 110, VP_TEXT_CENTER);
    }
    snprintf(text, sizeof text, "CREDITS %d", L.credits.credits);
    vp_text(d, text, 2048, 650, 90, VP_TEXT_CENTER);

    vp_text_font(VP_FONT_HERSHEY);
    vp_color(d, 150, 150, 150);
    vp_text(d, "left/right: pattern   coin: credit   start: play   exit: quit",
            2048, 350, 70, VP_TEXT_CENTER);
    vp_text(d, "NUM LOCK = START 1, CAPS LOCK = START 2 (keyboard LEDs, Linux)",
            2048, 220, 60, VP_TEXT_CENTER);
}

static int init(vp_game *g)
{
    (void)g;
    vp_credits_init(&L.credits);
    return VP_OK;
}

int main(int argc, char **argv)
{
    vp_game_desc desc = { 0 };
    desc.title  = "lamps";
    desc.init   = init;
    desc.update = update;
    desc.draw   = draw;
    return vp_run(&desc, argc, argv);
}
