/* models — look at 3D models in wireframe.
 *
 *   models                      built-in shapes
 *   models tank.obj ship.obj    your OBJ files (with --windowed, --dvg ... as usual)
 *
 * Controls: P1 left/right (or stick) turn around the model, up/down move in
 * and out, button 1 shows the next model. Exit button to quit.
 *
 * It shows: a view (vp3d_view), meshes built in code and loaded from OBJ
 * files, wireframe drawing (each edge once, in as few strokes as the shape
 * allows), free 3D lines (the floor grid), and the counters.
 */
#include <vectorpie/vp.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MODELS 16
#define FIT_RADIUS 5.0f          /* every model is scaled to this size */

static struct {
    vp3d_mesh  *mesh[MAX_MODELS];
    const char *name[MAX_MODELS];
    int         count, current;
    float       orbit, distance, spin;
    vp3d_view   view;
} M;

static vp3d_mesh *make_cube(void)
{
    static const float v[8][3] = { {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
                                   {-1,-1, 1}, {1,-1, 1}, {1,1, 1}, {-1,1, 1} };
    static const int f[6][4] = { {0,3,2,1}, {4,5,6,7}, {0,1,5,4}, {2,3,7,6}, {1,2,6,5}, {0,4,7,3} };
    vp3d_mesh *m;
    vp3d_mesh_create(&m);
    for (int i = 0; i < 8; i++) vp3d_mesh_vertex(m, v[i][0], v[i][1], v[i][2]);
    for (int i = 0; i < 6; i++) vp3d_mesh_face(m, f[i], 4);
    vp3d_mesh_finish(m);
    return m;
}

static vp3d_mesh *make_pyramid(void)
{
    vp3d_mesh *m;
    vp3d_mesh_create(&m);
    vp3d_mesh_vertex(m, -1, -1, -1);
    vp3d_mesh_vertex(m,  1, -1, -1);
    vp3d_mesh_vertex(m,  1, -1,  1);
    vp3d_mesh_vertex(m, -1, -1,  1);
    vp3d_mesh_vertex(m,  0,  1.2f, 0);
    static const int base[4] = { 0, 1, 2, 3 };
    static const int sides[4][3] = { {0,4,1}, {1,4,2}, {2,4,3}, {3,4,0} };
    vp3d_mesh_face(m, base, 4);
    for (int i = 0; i < 4; i++) vp3d_mesh_face(m, sides[i], 3);
    vp3d_mesh_finish(m);
    return m;
}

/* A ring of triangles round a hub: many edges, a good stroke test. */
static vp3d_mesh *make_wheel(int spokes)
{
    vp3d_mesh *m;
    vp3d_mesh_create(&m);
    int hub = vp3d_mesh_vertex(m, 0, 0, 0);
    for (int i = 0; i < spokes; i++) {
        float a = 6.2831853f * i / spokes;
        vp3d_mesh_vertex(m, cosf(a), sinf(a), 0);
    }
    for (int i = 0; i < spokes; i++) {
        int tri[3] = { hub, 1 + i, 1 + (i + 1) % spokes };
        vp3d_mesh_face(m, tri, 3);
    }
    vp3d_mesh_finish(m);
    return m;
}

static void add_model(vp3d_mesh *m, const char *name)
{
    if (!m || M.count == MAX_MODELS) return;
    M.mesh[M.count] = m;
    M.name[M.count] = name;
    M.count++;
    printf("models: %-24s %5d vertices %5d edges %4d strokes\n", name,
           vp3d_mesh_vertex_count(m), vp3d_mesh_edge_count(m), vp3d_mesh_stroke_count(m));
}

static void update(vp_game *g, float dt)
{
    vp_input *in = vp_game_input(g);
    float turn = vp_input_axis(in, "p1_stickx");
    float move = vp_input_axis(in, "p1_sticky");
    if (vp_input_held(in, "p1_left"))  turn = -1;
    if (vp_input_held(in, "p1_right")) turn = 1;
    if (vp_input_held(in, "p1_up"))    move = -1;
    if (vp_input_held(in, "p1_down"))  move = 1;
    M.orbit += turn * 1.5f * dt;
    M.distance += move * 15.0f * dt;
    if (M.distance < 8) M.distance = 8;
    if (M.distance > 80) M.distance = 80;
    M.spin += 0.5f * dt;

    if (vp_input_pressed(in, "p1_button1")) M.current = (M.current + 1) % M.count;

    vp_vec3 eye = vp_v3(sinf(M.orbit) * M.distance, 6.0f, -cosf(M.orbit) * M.distance);
    vp3d_look_at(&M.view, eye, vp_v3(0, 2, 0), vp_v3(0, 1, 0));
}

static void draw(vp_game *g, vp_dvg *d)
{
    char line[96];
    vp_dvg_stats st;
    (void)g;

    /* the floor: free lines */
    vp_color(d, 0, 140, 0);
    for (int i = -60; i <= 60; i += 6) {
        vp3d_line(d, &M.view, vp_v3((float)i, 0, -60), vp_v3((float)i, 0, 60));
        vp3d_line(d, &M.view, vp_v3(-60, 0, (float)i), vp_v3(60, 0, (float)i));
    }

    /* the model, scaled to FIT_RADIUS, standing on the floor, turning */
    vp3d_mesh *m = M.mesh[M.current];
    float s = FIT_RADIUS / (vp3d_mesh_radius(m) > 0 ? vp3d_mesh_radius(m) : 1);
    vp_xf place = vp_xf_mul2(vp_xf_translate(0, FIT_RADIUS, 0),
                  vp_xf_mul2(vp_xf_rot_y(M.spin), vp_xf_scale(s, s, s)));
    vp_color(d, 0, 255, 255);
    vp3d_draw_mesh_wire(d, &M.view, m, &place);

    /* the text */
    vp_dvg_get_stats(d, &st);         /* last frame's: one frame behind, fine for a readout */
    vp_color(d, 255, 255, 255);
    vp_text(d, M.name[M.current], 2048, 3850, 120, VP_TEXT_CENTER);
    snprintf(line, sizeof line, "%d EDGES  %d STROKES   FRAME: %u LINES %u MOVES",
             vp3d_mesh_edge_count(m), vp3d_mesh_stroke_count(m), st.vectors, st.blank_moves);
    vp_text(d, line, 2048, 150, 70, VP_TEXT_CENTER);
}

int main(int argc, char **argv)
{
    vp_game_desc desc = {0};
    vp3d_obj_options opt = {0};
    int vertical = getenv("VP_VERTICAL") != NULL;
    opt.center = 1;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {           /* vp_run's options (and --dvg PORT) */
            if (!strcmp(argv[i], "--dvg") && i + 1 < argc && argv[i + 1][0] != '-') i++;
            if (!strcmp(argv[i], "--vertical")) vertical = 1;
            continue;
        }
        vp3d_mesh *m;
        if (vp3d_mesh_load_obj(argv[i], &opt, &m) == VP_OK) {
            const char *base = strrchr(argv[i], '/');
            add_model(m, base ? base + 1 : argv[i]);
        } else {
            fprintf(stderr, "models: %s\n", vp_last_error());
        }
    }
    if (M.count == 0) {
        add_model(make_cube(), "CUBE");
        add_model(make_pyramid(), "PYRAMID");
        add_model(make_wheel(12), "WHEEL");
    }

    vp3d_view_init(&M.view, 60.0f);
    if (vertical) M.view.aspect = 3.0f / 4.0f;     /* the monitor's shape */
    M.distance = 25;

    desc.title  = "VectorPie models";
    desc.update = update;
    desc.draw   = draw;
    int status = vp_run(&desc, argc, argv);
    for (int i = 0; i < M.count; i++) vp3d_mesh_free(M.mesh[i]);
    return status;
}
