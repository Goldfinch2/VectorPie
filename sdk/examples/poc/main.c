/* poc — the VectorPie SDK proof of concept.
 *
 * One source, two builds: a Windows .exe (cmake --preset pc) and a Pi
 * executable (cmake --preset pi). It draws a border, a crosshair and a turning
 * wireframe cube with the three drawing calls (vp_color, vp_moveto,
 * vp_lineto), on whatever output a VectorPie game would use:
 *
 *   poc                    the USB-DVG if advmame.rc enables it, otherwise a
 *                          fullscreen window (or VP_OUTPUT=dvg|window|windowed)
 *   poc --windowed         a normal window (the whole screen on the Pi)
 *   poc --window           a fullscreen window
 *   poc --dvg [PORT]       the vector board, e.g. --dvg COM5 on a PC
 *   poc --seconds N        stop after N seconds (default: run until closed)
 *   poc --vertical         a vertically mounted monitor (3:4)
 *
 * It stops when the window is closed or on Ctrl+C; the board then gets its
 * exit sequence and returns to its home screen.
 */
#include <vectorpie/vp.h>

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FPS     40
#define CENTER  (VP_DVG_COORD_MAX / 2)

static volatile sig_atomic_t g_stop = 0;

static void on_signal(int sig)
{
    (void)sig;
    g_stop = 1;
}

/* ---- the cube ----------------------------------------------------------- */

static const float CUBE[8][3] = {
    {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1},   /* front */
    {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}    /* back  */
};

/* Rotate a vertex about Y then X, push it away from the camera, project. */
static void project(const float v[3], float ay, float ax, int *sx, int *sy)
{
    float x = v[0] * cosf(ay) + v[2] * sinf(ay);
    float z = -v[0] * sinf(ay) + v[2] * cosf(ay);
    float y = v[1] * cosf(ax) - z * sinf(ax);
    z = v[1] * sinf(ax) + z * cosf(ax);

    const float distance = 4.0f;   /* camera to cube center */
    const float scale = 2400.0f;   /* board units at unit depth */
    float d = z + distance;
    *sx = CENTER + (int)lroundf(scale * x / d);
    *sy = CENTER + (int)lroundf(scale * y / d);
}

/* A cube has 8 corners where three edges meet, so it cannot be drawn in one
 * stroke: four strokes is the minimum. One long stroke (front face, across,
 * back face) and three single edges. */
static void draw_cube(vp_dvg *d, float t)
{
    int px[8], py[8];
    for (int i = 0; i < 8; i++)
        project(CUBE[i], t * 0.9f, t * 0.6f, &px[i], &py[i]);

    static const int stroke[] = { 0, 1, 2, 3, 0, 4, 5, 6, 7, 4 };
    vp_color(d, 0, 255, 0);
    vp_moveto(d, px[stroke[0]], py[stroke[0]]);
    for (size_t i = 1; i < sizeof(stroke) / sizeof(stroke[0]); i++)
        vp_lineto(d, px[stroke[i]], py[stroke[i]]);

    static const int edges[3][2] = { {1, 5}, {2, 6}, {3, 7} };
    for (int e = 0; e < 3; e++) {
        vp_moveto(d, px[edges[e][0]], py[edges[e][0]]);
        vp_lineto(d, px[edges[e][1]], py[edges[e][1]]);
    }
}

/* ---- the frame ---------------------------------------------------------- */

static void draw_frame(vp_dvg *d, float t)
{
    const int m = VP_DVG_COORD_MAX;

    /* the border: the edges of the board space, one stroke */
    vp_color(d, 96, 96, 96);
    vp_moveto(d, 0, 0);
    vp_lineto(d, m, 0);
    vp_lineto(d, m, m);
    vp_lineto(d, 0, m);
    vp_lineto(d, 0, 0);

    /* the crosshair */
    vp_color(d, 255, 255, 0);
    vp_moveto(d, CENTER - 150, CENTER);
    vp_lineto(d, CENTER + 150, CENTER);
    vp_moveto(d, CENTER, CENTER - 150);
    vp_lineto(d, CENTER, CENTER + 150);

    draw_cube(d, t);
}

/* ---- main --------------------------------------------------------------- */

static void usage(void)
{
    fprintf(stderr, "usage: poc [--dvg [PORT] | --window | --windowed] [--seconds N] [--vertical]\n");
}

int main(int argc, char **argv)
{
    vp_dvg_options o;
    const char *reason = NULL;
    int forced = 0, fullscreen = 0, vertical = 0, seconds = 0;
    const char *port = NULL;

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
        } else if (!strcmp(argv[i], "--seconds") && i + 1 < argc) {
            seconds = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--vertical")) {
            vertical = 1;
        } else {
            usage();
            return 2;
        }
    }

    if (forced) {
        memset(&o, 0, sizeof(o));
        o.output = (vp_dvg_output)forced;
        o.port = port;
        o.fullscreen = fullscreen;
        o.window_title = "VectorPie POC";
        reason = "command line";
    } else {
        vp_dvg_auto_options(&o, "VectorPie POC", &reason);
    }
    if (vertical) o.vertical = 1;

    printf("VectorPie SDK %s\n", vp_version());

    vp_dvg *d = NULL;
    int r = vp_dvg_open_ex(&o, &d);
    if (r != VP_OK) {
        fprintf(stderr, "poc: %s (%s)\n", vp_last_error(), vp_strerror(r));
        return 1;
    }
    printf("output: %s%s%s (%s)\n", vp_hardware_name(vp_dvg_hardware(d)),
           vp_dvg_get_output(d) == VP_DVG_OUTPUT_BOARD ? " on " : "",
           vp_dvg_port(d), reason);
    fflush(stdout);

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    const uint64_t start = vp_ticks_ms();
    const uint64_t frame_ms = 1000 / FPS;
    uint64_t next = start, last_report = start;
    unsigned frames = 0;
    int status = 0;

    while (!g_stop && !vp_dvg_window_closed(d)) {
        uint64_t now = vp_ticks_ms();
        if (seconds > 0 && now - start >= (uint64_t)seconds * 1000) break;

        vp_dvg_begin_frame(d);
        draw_frame(d, (float)(now - start) / 1000.0f);
        r = vp_dvg_end_frame(d, 0);
        if (r != VP_OK) {
            fprintf(stderr, "poc: frame not sent: %s (%s)\n", vp_last_error(), vp_strerror(r));
            status = 1;
            break;
        }
        frames++;

        now = vp_ticks_ms();
        if (now - last_report >= 1000) {
            vp_dvg_stats s;
            vp_dvg_get_stats(d, &s);
            printf("%5.1f fps | lines %u  moves %u  words %u  lit %u  blank %u  dropped %u\n",
                   frames * 1000.0 / (double)(now - last_report),
                   s.vectors, s.blank_moves, s.words, s.lit_length, s.blank_length, s.dropped);
            fflush(stdout);
            frames = 0;
            last_report = now;
        }

        next += frame_ms;
        if (next > now) vp_sleep_ms((int)(next - now));
        else next = now;   /* running late: do not try to catch up */
    }

    vp_dvg_close(d);
    return status;
}
