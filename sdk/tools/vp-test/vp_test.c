/* vp-test — check that the SDK reaches the screen, and what it read.
 *
 * Draws a test pattern on the output a game would use (the USB-DVG, or a
 * fullscreen window when the DVG is disabled), or on the one you name. It
 * prints which output was chosen and why, the DVG port and where it came
 * from, and the frame counters once a second. With --rc it also lists what
 * advmame.rc says: the DVG settings and every input_map binding.
 *
 *   vp-test                      the output a game would use, for 10 seconds
 *   vp-test --dvg [PORT]         the USB-DVG
 *   vp-test --window             a fullscreen window
 *   vp-test --windowed           a normal window
 *   vp-test --seconds 0          run until the window is closed or Ctrl-C
 *
 * Other options: --fps N (default 40), --vertical, --blanks (draw the
 * invisible moves in the window), --rc, --version.
 */
#include <vectorpie/vp.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI_F 3.14159265f
#define MAXC VP_DVG_COORD_MAX

static void usage(void)
{
    fprintf(stderr,
            "usage: vp-test [--dvg [PORT] | --window | --windowed] [--seconds N] [--fps N]\n"
            "               [--vertical] [--blanks] [--rc] [--version]\n");
}

static const char *binding_type_name(vp_binding_type t)
{
    switch (t) {
    case VP_BINDING_KEY:          return "key";
    case VP_BINDING_JOY_BUTTON:   return "joystick button";
    case VP_BINDING_JOY_AXIS_NEG: return "joystick axis -";
    case VP_BINDING_JOY_AXIS_POS: return "joystick axis +";
    case VP_BINDING_MOUSE_BUTTON: return "mouse button";
    case VP_BINDING_MOUSE_AXIS:   return "mouse axis";
    }
    return "?";
}

static void print_rc(void)
{
    vp_rc *rc = NULL;
    int r = vp_rc_load(NULL, &rc);
    if (r != VP_OK) {
        printf("advmame.rc: %s\n", vp_last_error());
        return;
    }
    printf("advmame.rc: %s\n", vp_rc_path(rc));
    static const char *keys[] = {
        "vector_aux_renderer", "vector_aux_renderer_port",
        "vector_aux_renderer_sort_vectors", "input_adstick_device", NULL
    };
    for (int i = 0; keys[i]; i++) {
        const char *v = vp_rc_get(rc, keys[i]);
        printf("  %-34s %s\n", keys[i], v ? v : "(not set)");
    }
    printf("  input_map (%d actions):\n", vp_rc_action_count(rc));
    for (int i = 0; i < vp_rc_action_count(rc); i++) {
        const char *name = vp_rc_action_name(rc, i);
        printf("    %-14s", name);
        for (int j = 0; j < vp_rc_binding_count(rc, name); j++) {
            vp_binding b;
            vp_rc_binding(rc, name, j, &b);
            printf("%s%s dev %s ", j ? " | " : "", binding_type_name(b.type), b.device);
            if (b.type == VP_BINDING_KEY) printf("%s", b.key);
            else printf("#%d", b.index);
        }
        printf("\n");
    }
    vp_rc_free(rc);
}

static void line(vp_dvg *d, int x0, int y0, int x1, int y1)
{
    vp_moveto(d, x0, y0);
    vp_lineto(d, x1, y1);
}

/* One stroke: four linetos from the corner the beam moves to. */
static void rect(vp_dvg *d, int x0, int y0, int x1, int y1)
{
    vp_moveto(d, x0, y0);
    vp_lineto(d, x1, y0);
    vp_lineto(d, x1, y1);
    vp_lineto(d, x0, y1);
    vp_lineto(d, x0, y0);
}

static void circle(vp_dvg *d, int cx, int cy, int radius, int segments)
{
    vp_moveto(d, cx + radius, cy);
    for (int i = 1; i <= segments; i++) {
        float a = 2.0f * PI_F * (float)i / (float)segments;
        vp_lineto(d, cx + (int)lroundf(radius * cosf(a)), cy + (int)lroundf(radius * sinf(a)));
    }
}

/* The pattern: the full border (shows the edges of the board space), a
 * center cross, a circle (shows the aspect: round on a calibrated monitor),
 * color and brightness bars, corner ticks, and a turning spoke so a frozen
 * output is obvious. */
static void draw_pattern(vp_dvg *d, float t)
{
    const int c = MAXC / 2;

    vp_color(d, 255, 255, 255);
    rect(d, 0, 0, MAXC, MAXC);
    line(d, c - 200, c, c + 200, c);
    line(d, c, c - 200, c, c + 200);

    vp_color(d, 0, 255, 0);
    circle(d, c, c, 1200, 64);

    /* corner ticks, 45 degrees in */
    vp_color(d, 255, 255, 0);
    line(d, 0, 0, 300, 300);
    line(d, MAXC, 0, MAXC - 300, 300);
    line(d, 0, MAXC, 300, MAXC - 300);
    line(d, MAXC, MAXC, MAXC - 300, MAXC - 300);

    /* color bars: red, green, blue, white */
    static const uint8_t colors[4][3] = { {255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255} };
    for (int i = 0; i < 4; i++) {
        vp_color(d, colors[i][0], colors[i][1], colors[i][2]);
        rect(d, 600 + i * 750, 3300, 1200 + i * 750, 3700);
    }

    /* brightness steps, white at 1/8 .. 8/8 */
    for (int i = 0; i < 8; i++) {
        uint8_t v = (uint8_t)(32 * (i + 1) - 1);
        int x = 500 + i * 400;
        vp_color(d, v, v, v);
        rect(d, x, 400, x + 250, 700);
    }

    /* turning spoke */
    float a = t * 2.0f * PI_F / 4.0f;   /* one turn every 4 seconds */
    vp_color(d, 0, 255, 255);
    line(d, c, c, c + (int)lroundf(1000 * cosf(a)), c + (int)lroundf(1000 * sinf(a)));
}

int main(int argc, char **argv)
{
    vp_dvg_options o;
    const char *reason = NULL;
    int forced = 0, vertical = 0, blanks = 0, show_rc = 0;
    int seconds = 10, fps = 40;
    const char *port = NULL;

    memset(&o, 0, sizeof(o));
    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "--dvg")) {
            forced = VP_DVG_OUTPUT_BOARD;
            if (i + 1 < argc && argv[i + 1][0] != '-') port = argv[++i];
        } else if (!strcmp(a, "--window")) {
            forced = VP_DVG_OUTPUT_WINDOW;
            o.fullscreen = 1;
        } else if (!strcmp(a, "--windowed")) {
            forced = VP_DVG_OUTPUT_WINDOW;
            o.fullscreen = 0;
        } else if (!strcmp(a, "--seconds") && i + 1 < argc) {
            seconds = atoi(argv[++i]);
        } else if (!strcmp(a, "--fps") && i + 1 < argc) {
            fps = atoi(argv[++i]);
        } else if (!strcmp(a, "--vertical")) {
            vertical = 1;
        } else if (!strcmp(a, "--blanks")) {
            blanks = 1;
        } else if (!strcmp(a, "--rc")) {
            show_rc = 1;
        } else if (!strcmp(a, "--version")) {
            printf("vp-test, VectorPie SDK %s\n", vp_version());
            return 0;
        } else {
            usage();
            return 2;
        }
    }
    if (fps < 1) fps = 1;
    if (fps > 120) fps = 120;

    printf("VectorPie SDK %s\n", vp_version());
    if (show_rc) print_rc();

    if (forced) {
        int fullscreen = o.fullscreen;
        memset(&o, 0, sizeof(o));
        o.output = (vp_dvg_output)forced;
        o.port = port;
        o.fullscreen = fullscreen;
        o.window_title = "vp-test";
        reason = "command line";
    } else {
        vp_dvg_auto_options(&o, "vp-test", &reason);
    }
    if (vertical) o.vertical = 1;
    o.show_blanks = blanks;

    char resolved[256];   /* outlives the block: o.port may point here */
    if (o.output == VP_DVG_OUTPUT_BOARD) {
        const char *source = "command line";
        if (!o.port) {
            if (vp_dvg_resolve_port(resolved, sizeof(resolved), &source) != VP_OK) {
                fprintf(stderr, "vp-test: %s\n", vp_last_error());
                return 1;
            }
            o.port = resolved;
        }
        printf("output: board on %s (port from %s; output from %s)\n", o.port, source, reason);
    } else {
        printf("output: %s window, %s (%s)\n", o.fullscreen ? "fullscreen" : "normal",
               o.vertical ? "vertical 3:4" : "horizontal 4:3", reason);
    }

    vp_dvg *d = NULL;
    int r = vp_dvg_open_ex(&o, &d);
    if (r != VP_OK) {
        fprintf(stderr, "vp-test: %s (%s)\n", vp_last_error(), vp_strerror(r));
        return 1;
    }
    printf("hardware: %s\n", vp_hardware_name(vp_dvg_hardware(d)));
    fflush(stdout);

    const unsigned long long start = vp_ticks_ms();
    const unsigned long long frame_ms = 1000ull / (unsigned long long)fps;
    unsigned long long next = start, last_report = start;
    unsigned frames_since_report = 0;
    int status = 0;

    for (;;) {
        unsigned long long now = vp_ticks_ms();
        if (seconds > 0 && now - start >= (unsigned long long)seconds * 1000ull) break;
        if (vp_dvg_window_closed(d)) break;

        vp_dvg_begin_frame(d);
        draw_pattern(d, (float)(now - start) / 1000.0f);
        r = vp_dvg_end_frame(d, 0);
        if (r != VP_OK) {
            fprintf(stderr, "vp-test: frame not sent: %s (%s)\n", vp_last_error(), vp_strerror(r));
            status = 1;
            break;
        }
        frames_since_report++;

        now = vp_ticks_ms();
        if (now - last_report >= 1000) {
            vp_dvg_stats s;
            vp_dvg_get_stats(d, &s);
            printf("%5.1f fps | vectors %u  blank moves %u  words %u  clipped %u  dropped %u"
                   " | frames %llu  with drops %llu  bytes %llu  write errors %u\n",
                   frames_since_report * 1000.0 / (double)(now - last_report),
                   s.vectors, s.blank_moves, s.words, s.clipped_away, s.dropped,
                   (unsigned long long)s.frames_sent, (unsigned long long)s.frames_with_drops,
                   (unsigned long long)s.bytes_sent, s.write_errors);
            fflush(stdout);
            frames_since_report = 0;
            last_report = now;
        }

        next += frame_ms;
        if (next > now) vp_sleep_ms((int)(next - now));
        else next = now;   /* running late: do not try to catch up */
    }

    vp_dvg_close(d);
    return status;
}
