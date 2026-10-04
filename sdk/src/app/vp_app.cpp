// The game loop: fixed-step update, paced frames, a clean exit to the menu.

#include "../core/vp_internal.h"

#include <SDL.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

struct vp_game {
    const vp_game_desc *desc;
    vp_dvg   *out;
    vp_input *in;
    double    time;
    bool      quit;
};

namespace {

volatile std::sig_atomic_t g_signal = 0;

void onSignal(int)
{
    g_signal = 1;
}

// A late frame catches up with at most this many update steps; beyond that
// the game slows down instead of freezing to catch up.
const int MAX_STEPS_PER_FRAME = 5;

// --dvg [PORT], --window, --windowed, --vertical. Returns false on anything
// else (the usage is printed).
bool parseArgs(int argc, char **argv, const char *title, vp_dvg_options *o, const char **reason)
{
    int forced = 0, fullscreen = 0, vertical = 0;
    const char *port = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--dvg")) {
            forced = VP_DVG_OUTPUT_BOARD;
            if (i + 1 < argc && argv[i + 1][0] != '-') port = argv[++i];
        } else if (!strcmp(argv[i], "--window")) {
            forced = VP_DVG_OUTPUT_WINDOW;
            fullscreen = 1;
        } else if (!strcmp(argv[i], "--windowed")) {
            forced = VP_DVG_OUTPUT_WINDOW;
            fullscreen = 0;
        } else if (!strcmp(argv[i], "--vertical")) {
            vertical = 1;
        } else {
            fprintf(stderr, "usage: %s [--dvg [PORT] | --window | --windowed] [--vertical]\n",
                    argc > 0 ? argv[0] : "game");
            return false;
        }
    }
    if (forced) {
        memset(o, 0, sizeof(*o));
        o->output = (vp_dvg_output)forced;
        o->port = port;
        o->fullscreen = fullscreen;
        o->window_title = title;
        *reason = "command line";
    } else {
        vp_dvg_auto_options(o, title, reason);
    }
    if (vertical) o->vertical = 1;
    return true;
}

}  // namespace

int vp_run(const vp_game_desc *desc, int argc, char **argv)
{
    if (!desc) {
        fprintf(stderr, "vp_run: no game description\n");
        return 1;
    }
    const char *title = desc->title ? desc->title : "VectorPie";
    const int updateHz = desc->update_hz > 0 ? desc->update_hz : 60;
    const int frameHz  = desc->frame_hz  > 0 ? desc->frame_hz  : 40;
    const double dt = 1.0 / updateHz;

    vp_dvg_options o;
    const char *reason = "";
    if (!parseArgs(argc, argv, title, &o, &reason))
        return 1;

    vp_game g;
    memset(&g, 0, sizeof(g));
    g.desc = desc;

    int r = vp_dvg_open_ex(&o, &g.out);
    if (r != VP_OK) {
        fprintf(stderr, "%s: cannot open the output: %s\n", title, vp_last_error());
        return 1;
    }
    r = vp_input_open(&g.in);
    if (r != VP_OK) {
        fprintf(stderr, "%s: cannot open the controls: %s\n", title, vp_last_error());
        vp_dvg_close(g.out);
        return 1;
    }
    printf("%s: VectorPie SDK %s, %s%s%s (%s), %d joystick(s)\n", title, vp_version(),
           vp_hardware_name(vp_dvg_hardware(g.out)),
           vp_dvg_get_output(g.out) == VP_DVG_OUTPUT_BOARD ? " on " : "",
           vp_dvg_port(g.out), reason, vp_input_joystick_count(g.in));
    fflush(stdout);

    // After SDL is up, so these handlers replace SDL's own.
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    int status = 0;
    if (desc->init && desc->init(&g) != VP_OK) {
        fprintf(stderr, "%s: init failed: %s\n", title, vp_last_error());
        status = 1;
        g.quit = true;
    }

    const uint64_t frameMs = (uint64_t)(1000 / frameHz);
    uint64_t last = vp_ticks_ms();
    uint64_t nextFrame = last;
    double acc = 0.0;

    while (!g.quit && !g_signal) {
        // The loop owns SDL's queue: look for a closed window, then drop the
        // rest (the controls are read as state by vp_input).
        SDL_PumpEvents();
        if (SDL_HasEvent(SDL_QUIT)) break;
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

        uint64_t now = vp_ticks_ms();
        acc += (now - last) / 1000.0;
        last = now;
        int steps = 0;
        while (acc >= dt && !g.quit) {
            if (steps == MAX_STEPS_PER_FRAME) {
                acc = 0.0;      // too far behind: let it go
                break;
            }
            vp_input_update(g.in);
            if (vp_input_pressed(g.in, "ui_cancel")) {
                g.quit = true;
                break;
            }
            if (desc->update) desc->update(&g, (float)dt);
            g.time += dt;
            acc -= dt;
            steps++;
        }
        if (g.quit) break;

        vp_dvg_begin_frame(g.out);
        if (desc->draw) desc->draw(&g, g.out);
        r = vp_dvg_end_frame(g.out, 0);
        if (r != VP_OK) {
            fprintf(stderr, "%s: frame not sent: %s\n", title, vp_last_error());
            status = 1;
            break;
        }

        nextFrame += frameMs;
        now = vp_ticks_ms();
        if (nextFrame > now) vp_sleep_ms((int)(nextFrame - now));
        else nextFrame = now;   // running late: do not try to catch up frames
    }

    if (desc->shutdown) desc->shutdown(&g);
    vp_input_close(g.in);
    vp_dvg_close(g.out);
    return status;
}

void *vp_game_user(vp_game *g)
{
    return g ? g->desc->user : nullptr;
}

vp_input *vp_game_input(vp_game *g)
{
    return g ? g->in : nullptr;
}

vp_dvg *vp_game_output(vp_game *g)
{
    return g ? g->out : nullptr;
}

double vp_game_time(vp_game *g)
{
    return g ? g->time : 0.0;
}

void vp_game_quit(vp_game *g)
{
    if (g) g->quit = true;
}
