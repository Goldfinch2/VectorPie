// The game loop: fixed-step update, paced frames, a clean exit to the menu.

#include "../core/vp_internal.h"

#include <SDL.h>

#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>

struct vp_game {
    const vp_game_desc *desc;
    vp_dvg   *out;
    vp_input *in;
    vp_audio *audio;
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

// --dvg [PORT], --window, --windowed, --maximized, --vertical. Returns false on anything
// else (the usage is printed).
bool parseArgs(int argc, char **argv, const char *title, vp_dvg_options *o, const char **reason)
{
    int forced = 0, fullscreen = 0, maximized = 0, vertical = 0;
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
        } else if (!strcmp(argv[i], "--maximized")) {
            forced = VP_DVG_OUTPUT_WINDOW;
            fullscreen = 0;
            maximized = 1;
        } else if (!strcmp(argv[i], "--vertical")) {
            vertical = 1;
        } else if (argv[i][0] != '-') {
            // Not an option: the game's own argument (a file to load...).
        } else {
            fprintf(stderr, "usage: %s [--dvg [PORT] | --window | --windowed | --maximized] [--vertical]\n",
                    argc > 0 ? argv[0] : "game");
            return false;
        }
    }
    if (forced) {
        memset(o, 0, sizeof(*o));
        o->output = (vp_dvg_output)forced;
        o->port = port;
        o->fullscreen = fullscreen;
        o->maximized = maximized;
        o->window_title = title;
        *reason = "command line";
    } else {
        vp_dvg_auto_options(o, title, reason);
    }
    if (vertical) o->vertical = 1;
    return true;
}

// VP_STATS=1: once a second, the average and the peak of what a frame cost:
// the output counters (vp_dvg_get_stats) and the time spent in draw() and in
// sending the frame. For tuning a game to the hardware.
struct FrameStats {
    bool     on = false;
    uint64_t since = 0;                  // ms, start of this second
    int      frames = 0, dropFrames = 0;
    double   sum[7] = {}, peak[7] = {};  // vectors, moves, words, lit, blank, draw ms, send ms

    void add(const vp_dvg_stats &s, double drawMs, double sendMs)
    {
        const double v[7] = { (double)s.vectors, (double)s.blank_moves, (double)s.words,
                              (double)s.lit_length, (double)s.blank_length, drawMs, sendMs };
        for (int i = 0; i < 7; i++) {
            sum[i] += v[i];
            if (v[i] > peak[i]) peak[i] = v[i];
        }
        frames++;
        if (s.dropped) dropFrames++;
    }

    void report(const char *title, uint64_t now)
    {
        if (now - since < 1000) return;
        if (frames > 0) {
            double n = frames, sec = (now - since) / 1000.0;
            printf("%s stats: %.1f fps | avg/peak: vectors %.0f/%.0f, moves %.0f/%.0f, "
                   "words %.0f/%.0f, lit length %.0f/%.0f, blank length %.0f/%.0f, "
                   "draw %.1f/%.1f ms, send %.1f/%.1f ms | frames with dropped lines %d\n",
                   title, frames / sec,
                   sum[0] / n, peak[0], sum[1] / n, peak[1], sum[2] / n, peak[2],
                   sum[3] / n, peak[3], sum[4] / n, peak[4], sum[5] / n, peak[5],
                   sum[6] / n, peak[6], dropFrames);
            fflush(stdout);
        }
        *this = FrameStats();
        on = true;
        since = now;
    }
};

// A control used: a button, a direction, a coin, the stick or the spinner
// moved well past rest. START 2 is left out (the next track).
bool saverWoken(vp_input *in)
{
    static const char *const actions[] = {
        "p1_up", "p1_down", "p1_left", "p1_right", "p2_up", "p2_down", "p2_left", "p2_right",
        "p1_button1", "p1_button2", "p1_button3", "p1_button4", "p1_button5", "p1_button6",
        "p2_button1", "p2_button2", "p2_button3", "p2_button4", "p2_button5", "p2_button6",
        "start1", "coin1", "coin2", "ui_cancel",
    };
    for (const char *a : actions)
        if (vp_input_pressed(in, a)) return true;
    static const char *const axes[] = { "p1_stickx", "p1_sticky", "p2_stickx", "p2_sticky" };
    for (const char *a : axes)
        if (vp_input_axis(in, a) > 0.5f || vp_input_axis(in, a) < -0.5f) return true;
    static const char *const counts[] = { "p1_dialx", "p1_trackballx", "p1_trackbally" };
    for (const char *a : counts) {
        int d = vp_input_delta(in, a);
        if (d > 20 || d < -20) return true;
    }
    return false;
}

double msSince(uint64_t counter)
{
    return (double)(SDL_GetPerformanceCounter() - counter) * 1000.0 / (double)SDL_GetPerformanceFrequency();
}

}  // namespace

std::string vp_manifest_value(const char *key, const char *section)
{
    char path[1100];
    std::ifstream f(vp_asset_path("game.ini", path, sizeof path));
    std::string line, in;                   // in: the section the line is in ("": none yet)
    const std::string want = section ? section : "";
    auto trim = [](const std::string &s) {
        size_t b = s.find_first_not_of(" \t\r"), e = s.find_last_not_of(" \t\r");
        return b == std::string::npos ? std::string() : s.substr(b, e - b + 1);
    };
    while (std::getline(f, line)) {
        size_t c = line.find_first_of(";#");
        if (c != std::string::npos) line.erase(c);
        line = trim(line);
        if (line.size() >= 2 && line.front() == '[' && line.back() == ']') {
            in = trim(line.substr(1, line.size() - 2));
            continue;
        }
        size_t eq = line.find('=');
        if (in == want && eq != std::string::npos && trim(line.substr(0, eq)) == key) return trim(line.substr(eq + 1));
    }
    return {};
}

int vp_run(const vp_game_desc *desc, int argc, char **argv)
{
    if (!desc) {
        fprintf(stderr, "vp_run: no game description\n");
        return 1;
    }
    const char *title = desc->title ? desc->title : "VectorPie";
    const int updateHz = desc->update_hz > 0 ? desc->update_hz : 60;
    int frameHz = desc->frame_hz > 0 ? desc->frame_hz : 40;
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
    if (vp_audio_open(&g.audio) != VP_OK) {
        fprintf(stderr, "%s: no sound: %s\n", title, vp_last_error());
        g.audio = nullptr;      // the game runs silent
    }
    // A screensaver's music when it runs without the cabinet's menu: its
    // first non-option argument (vp_saver.h).
    const char *musicArg = nullptr;
    for (int i = 1; i < argc && !musicArg; i++) {
        if (!strcmp(argv[i], "--dvg") && i + 1 < argc && argv[i + 1][0] != '-') { i++; continue; }
        if (argv[i][0] != '-') musicArg = argv[i];
    }
    vp_saver_attach(g.audio, musicArg);
    const bool saver = vp_saver_active() != 0;
    // Under the cabinet's menu (its music feed given) the menu reads the
    // controls: it wakes the cabinet and skips tracks itself.
    const bool menuControls = saver && getenv("VP_MUSIC_FEED") && *getenv("VP_MUSIC_FEED");
    // A screensaver runs for hours: its whole picture, a little smaller,
    // wanders slowly round the screen and breathes in size, so no line stays
    // on the same spot of the tube. Slow, uneven periods (the path never
    // repeats exactly), from a different place each time.
    const double wanderPhase = saver ? (double)(SDL_GetPerformanceCounter() % 6283) / 1000.0 : 0;
    if (vp_dvg_get_output(g.out) == VP_DVG_OUTPUT_BOARD) {
        if (desc->frame_hz_board > 0) frameHz = desc->frame_hz_board;
        // game.ini can set it per board, without rebuilding the game: its
        // [dvg] (the USB-DVG) or [beamforge] section's frame_hz.
        const bool beamforge = vp_dvg_hardware(g.out) == VP_HW_BEAMFORGE;
        int hz = atoi(vp_manifest_value("frame_hz", beamforge ? "beamforge" : "dvg").c_str());
        if (hz >= 10 && hz <= 120) frameHz = hz;
    }
    // On the board, the cabinet's HDMI screen shows the game's marquee; a
    // screensaver leaves it to the menu (its screensaver picture stays).
    if (vp_dvg_get_output(g.out) == VP_DVG_OUTPUT_BOARD && !saver)
        vp_marquee_auto();
    printf("%s: VectorPie SDK %s, %s%s%s (%s), %d joystick(s), %s\n", title, vp_version(),
           vp_hardware_name(vp_dvg_hardware(g.out)),
           vp_dvg_get_output(g.out) == VP_DVG_OUTPUT_BOARD ? " on " : "",
           vp_dvg_port(g.out), reason, vp_input_joystick_count(g.in),
           g.audio ? "sound" : "no sound");
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

    FrameStats fs;
    const char *statsEnv = getenv("VP_STATS");
    fs.on = statsEnv && *statsEnv && strcmp(statsEnv, "0") != 0;
    fs.since = last;

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
            // The cabinet's screensaver: any control wakes the cabinet (the
            // menu comes back), START 2 plays the next track. Not in the
            // first moment: the control that was let go as it started.
            if (saver && !menuControls && g.time > 0.6) {
                if (vp_input_pressed(g.in, "start2")) {
                    vp_saver_skip();
                } else if (saverWoken(g.in)) {
                    g.quit = true;
                    break;
                }
            }
            if (desc->update) desc->update(&g, (float)dt);
            g.time += dt;
            acc -= dt;
            steps++;
        }
        if (g.quit) break;

        uint64_t t0 = SDL_GetPerformanceCounter();
        vp_dvg_begin_frame(g.out);
        if (saver) {
            const double t = g.time, two_pi = 6.283185307;
            float scale = (float)(0.90 + 0.04 * sin(two_pi * t / 89 + wanderPhase));
            float room = (1 - scale) / 2 * VP_DVG_COORD_MAX * 0.95f;   /* never off the screen */
            vp_dvg_set_view(g.out, scale, room * (float)sin(two_pi * t / 47 + wanderPhase * 2),
                            room * (float)sin(two_pi * t / 67 + wanderPhase * 3));
        }
        if (desc->draw) desc->draw(&g, g.out);
        double drawMs = msSince(t0);
        uint64_t t1 = SDL_GetPerformanceCounter();
        r = vp_dvg_end_frame(g.out, 0);
        if (r != VP_OK) {
            fprintf(stderr, "%s: frame not sent: %s\n", title, vp_last_error());
            status = 1;
            break;
        }
        if (fs.on) {
            vp_dvg_stats s;
            vp_dvg_get_stats(g.out, &s);
            fs.add(s, drawMs, msSince(t1));
            fs.report(title, vp_ticks_ms());
        }

        nextFrame += frameMs;
        now = vp_ticks_ms();
        if (nextFrame > now) vp_sleep_ms((int)(nextFrame - now));
        else nextFrame = now;   // running late: do not try to catch up frames
    }

    if (desc->shutdown) desc->shutdown(&g);
    vp_saver_detach();
    vp_lamps_close();           // the START lamps back as they were
    vp_marquee_clear();
    vp_audio_close(g.audio);
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

vp_audio *vp_game_audio(vp_game *g)
{
    return g ? g->audio : nullptr;
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
