// Coins and credits.

#include "../core/vp_internal.h"
#include <vectorpie/vp_credits.h>

#include <cmath>
#include <cstring>

void vp_credits_init(vp_credits *c)
{
    if (!c) return;
    memset(c, 0, sizeof *c);
    c->max = 99;
}

int vp_credits_update(vp_credits *c, vp_input *in)
{
    if (!c || !in) return 0;
    int coins = vp_input_pressed(in, "coin1") + vp_input_pressed(in, "coin2");
    int max = c->max > 0 ? c->max : 99;
    c->credits += coins;
    if (c->credits > max) c->credits = max;
    return coins;
}

int vp_credits_enough(const vp_credits *c, int players)
{
    if (!c) return 0;
    return c->free_play || c->credits >= players;
}

int vp_credits_take(vp_credits *c, int players)
{
    if (!vp_credits_enough(c, players)) return 0;
    if (!c->free_play) c->credits -= players;
    return 1;
}

void vp_credits_blink(const vp_credits *c, double time)
{
    unsigned mask = 0;
    if (std::fmod(time, 0.5) < 0.25) {
        if (vp_credits_enough(c, 1)) mask |= VP_LAMP_START1;
        if (vp_credits_enough(c, 2)) mask |= VP_LAMP_START2;
    }
    vp_lamps_set(mask);
}
