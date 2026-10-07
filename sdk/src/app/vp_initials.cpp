// Entering initials for the high-score table, with the player's stick.

#include "../core/vp_internal.h"
#include <vectorpie/vp_hiscore.h>

#include <cmath>
#include <cstdio>
#include <cstring>

namespace {

const char LETTERS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-";
const int  NLETTERS = (int)sizeof(LETTERS) - 1;
const float REPEAT_FIRST = 0.4f;     // seconds held before up/down repeats
const float REPEAT_NEXT = 0.12f;     // then one letter every

int indexOf(char c)
{
    const char *p = strchr(LETTERS, c);
    return p && c ? (int)(p - LETTERS) : 0;
}

void step(vp_initials *e, int by)
{
    int i = (indexOf(e->name[e->pos]) + by + NLETTERS) % NLETTERS;
    e->name[e->pos] = LETTERS[i];
}

// Done: keep the letters chosen and the one being chosen, drop the rest.
int finish(vp_initials *e)
{
    int n = e->pos < 3 ? e->pos + 1 : 3;
    e->name[n] = '\0';
    e->pos = 3;
    return 1;
}

}  // namespace

void vp_initials_begin(vp_initials *e, int player, float timeout_s)
{
    if (!e) return;
    memset(e, 0, sizeof *e);
    e->player = player == 2 ? 2 : 1;
    e->timeout = timeout_s > 0 ? timeout_s : 0;
    e->name[0] = 'A';
}

int vp_initials_update(vp_initials *e, vp_input *in, float dt)
{
    if (!e) return 1;
    if (e->pos >= 3) return 1;
    e->time += dt;
    if (e->timeout > 0 && e->time >= e->timeout) return finish(e);
    if (!in) return 0;

    char a[32];
    const char *p = e->player == 2 ? "p2" : "p1";
#define ACT(s) (snprintf(a, sizeof a, "%s_%s", p, s), a)
    if (vp_input_pressed(in, "start1") || vp_input_pressed(in, "start2"))
        return finish(e);

    // Up / down: one letter on the press, then repeating while held.
    int dir = vp_input_held(in, ACT("up")) ? 1 : vp_input_held(in, ACT("down")) ? -1 : 0;
    int pressed = vp_input_pressed(in, ACT("up")) || vp_input_pressed(in, ACT("down"));
    if (dir == 0) {
        e->repeat = 0;
    } else if (pressed) {
        step(e, dir);
        e->repeat = REPEAT_FIRST;
    } else {
        e->repeat -= dt;
        if (e->repeat <= 0) {
            step(e, dir);
            e->repeat += REPEAT_NEXT;
        }
    }

    if (vp_input_pressed(in, ACT("button1")) || vp_input_pressed(in, ACT("right"))) {
        if (e->pos == 2) return finish(e);
        e->pos++;
        e->name[e->pos] = e->name[e->pos - 1];    // start from the last letter
        return 0;
    }
    if ((vp_input_pressed(in, ACT("button2")) || vp_input_pressed(in, ACT("left"))) && e->pos > 0) {
        e->name[e->pos] = '\0';
        e->pos--;
    }
#undef ACT
    return 0;
}

void vp_initials_draw(const vp_initials *e, vp_dvg *d, float x, float y, float height)
{
    if (!e || !d) return;
    const float cell = height * 1.6f;
    const float left = x - cell * 1.5f;
    for (int i = 0; i < 3; i++) {
        float cx = left + cell * (i + 0.5f);
        bool current = i == e->pos;
        bool blinkOff = current && std::fmod(e->time, 0.5f) >= 0.35f;
        if (e->name[i] && i <= e->pos && !blinkOff) {
            char s[2] = { e->name[i], '\0' };
            vp_text(d, s, cx, y, height, VP_TEXT_CENTER);
        }
        // the line under each letter
        vp_moveto(d, (int)(cx - height * 0.5f), (int)(y - height * 0.35f));
        vp_lineto(d, (int)(cx + height * 0.5f), (int)(y - height * 0.35f));
    }
}
