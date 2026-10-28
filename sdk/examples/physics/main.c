/* physics — vp_physics on the vector screen.
 *
 * A playground: a stack of crates, a seesaw on a hinge, a chain of balls
 * hanging from ball joints, and a bar the game turns (a "moved" body) that
 * sweeps the floor. Balls and boxes drop in now and then; where two things
 * meet hard, a spark. Everything is drawn through a vp3d scene, so solid
 * things hide what is behind them.
 *
 * Controls: P1 button 1 drops a ball over the stack, button 2 a box; START 1
 * builds the playground again; the stick turns the camera. Exit: the
 * cabinet's exit button (Esc).
 */
#include <vectorpie/vp.h>

#include <math.h>
#include <stdio.h>

#define MAX_THINGS 96
#define MAX_SPARKS 24

typedef struct {
    vp_body   *body;
    vp3d_mesh *mesh;
    vp_vec3    size;            /* the mesh's scale: half sizes, or the radius */
    uint8_t    r, g, b;
} thing;

typedef struct { vp_vec3 at; float life; } spark;

static struct {
    vp_phys    *world;
    vp3d_mesh  *cube, *ball;
    vp3d_scene *scene;
    vp3d_view   view;
    thing       things[MAX_THINGS];
    int         count;
    vp_body    *sweeper;
    spark       sparks[MAX_SPARKS];
    float       cam_angle, drop_timer, time;
    unsigned    rng;
} P;

static float rnd(void)
{
    P.rng = P.rng * 1664525u + 1013904223u;
    return (float)(P.rng >> 8) / 16777216.0f;
}

/* ---- meshes: a cube (corners at +-1) and a ball (radius 1) ----------------- */

static vp3d_mesh *make_cube(void)
{
    vp3d_mesh *m;
    if (vp3d_mesh_create(&m) != VP_OK) return NULL;
    for (int i = 0; i < 8; i++)
        vp3d_mesh_vertex(m, (i & 1) ? 1.0f : -1.0f, (i & 2) ? 1.0f : -1.0f, (i & 4) ? 1.0f : -1.0f);
    static const int faces[6][4] = { { 0, 2, 3, 1 }, { 4, 5, 7, 6 }, { 0, 1, 5, 4 },
                                     { 2, 6, 7, 3 }, { 0, 4, 6, 2 }, { 1, 3, 7, 5 } };
    for (int f = 0; f < 6; f++) vp3d_mesh_face(m, faces[f], 4);
    vp3d_mesh_finish(m);
    return m;
}

static vp3d_mesh *make_ball(void)
{
    enum { SIDES = 10, RINGS = 6 };
    vp3d_mesh *m;
    if (vp3d_mesh_create(&m) != VP_OK) return NULL;
    int south = vp3d_mesh_vertex(m, 0, -1, 0), first = -1;
    for (int r = 1; r < RINGS; r++) {
        float la = -1.5708f + 3.14159f * r / RINGS;
        for (int s = 0; s < SIDES; s++) {
            float a = 6.28318f * s / SIDES;
            int v = vp3d_mesh_vertex(m, cosf(la) * sinf(a), sinf(la), cosf(la) * cosf(a));
            if (first < 0) first = v;
        }
    }
    int north = vp3d_mesh_vertex(m, 0, 1, 0);
    for (int s = 0; s < SIDES; s++) {
        int s1 = (s + 1) % SIDES;
        int f[3] = { south, first + s1, first + s };
        vp3d_mesh_face(m, f, 3);
        for (int r = 0; r + 1 < RINGS - 1; r++) {
            int q[4] = { first + r * SIDES + s, first + r * SIDES + s1,
                         first + (r + 1) * SIDES + s1, first + (r + 1) * SIDES + s };
            vp3d_mesh_face(m, q, 4);
        }
        int t[3] = { north, first + (RINGS - 2) * SIDES + s, first + (RINGS - 2) * SIDES + s1 };
        vp3d_mesh_face(m, t, 3);
    }
    /* a ring round it: lone lines, so it shows turning */
    int ring[12];
    for (int i = 0; i < 12; i++) {
        float a = 6.28318f * i / 12;
        ring[i] = vp3d_mesh_vertex(m, 1.02f * cosf(a), 1.02f * sinf(a), 0);
    }
    for (int i = 0; i < 12; i++) vp3d_mesh_line(m, ring[i], ring[(i + 1) % 12]);
    vp3d_mesh_set_crease(m, 180);
    vp3d_mesh_finish(m);
    return m;
}

/* ---- the playground ------------------------------------------------------------- */

static thing *add_thing(vp_body *b, vp3d_mesh *mesh, vp_vec3 size, uint8_t r, uint8_t g, uint8_t bl)
{
    if (!b || P.count >= MAX_THINGS) return NULL;
    thing *t = &P.things[P.count++];
    t->body = b;
    t->mesh = mesh;
    t->size = size;
    t->r = r; t->g = g; t->b = bl;
    vp_body_set_user(b, t);
    return t;
}

static void add_box(vp_body_kind kind, vp_vec3 pos, vp_vec3 half, float mass, uint8_t r, uint8_t g, uint8_t b)
{
    add_thing(vp_phys_add_box(P.world, kind, pos, half, mass), P.cube, half, r, g, b);
}

static void add_ball(vp_vec3 pos, float radius, float mass, uint8_t r, uint8_t g, uint8_t b)
{
    vp_body *body = vp_phys_add_sphere(P.world, VP_BODY_DYNAMIC, pos, radius, mass);
    vp_body_set_material(body, 0.5f, 0.4f);           /* a bouncy ball */
    add_thing(body, P.ball, vp_v3(radius, radius, radius), r, g, b);
}

static void build(void)
{
    if (P.world) vp_phys_free(P.world);
    vp_phys_create(&P.world);
    P.count = 0;

    /* the floor, and walls round it so nothing rolls away */
    add_box(VP_BODY_STATIC, vp_v3(0, -0.5f, 0), vp_v3(9, 0.5f, 9), 0, 0, 90, 160);
    add_box(VP_BODY_STATIC, vp_v3(-9.25f, 0.5f, 0), vp_v3(0.25f, 0.5f, 9), 0, 0, 90, 160);
    add_box(VP_BODY_STATIC, vp_v3(9.25f, 0.5f, 0), vp_v3(0.25f, 0.5f, 9), 0, 0, 90, 160);
    add_box(VP_BODY_STATIC, vp_v3(0, 0.5f, 9.25f), vp_v3(9.5f, 0.5f, 0.25f), 0, 0, 90, 160);
    add_box(VP_BODY_STATIC, vp_v3(0, 0.5f, -9.25f), vp_v3(9.5f, 0.5f, 0.25f), 0, 0, 90, 160);

    /* a stack of crates */
    for (int i = 0; i < 6; i++)
        add_box(VP_BODY_DYNAMIC, vp_v3(-4.0f + (i & 1) * 0.15f, 0.5f + i * 1.0f, 3.0f),
                vp_v3(0.5f, 0.5f, 0.5f), 20, 255, 180, 60);

    /* a seesaw: a plank on a hinge over a post (lower than the plank: room
     * to rock) */
    add_box(VP_BODY_STATIC, vp_v3(3.5f, 0.3f, 3.0f), vp_v3(0.15f, 0.3f, 0.8f), 0, 160, 160, 160);
    vp_body *plank = vp_phys_add_box(P.world, VP_BODY_DYNAMIC, vp_v3(3.5f, 0.9f, 3.0f),
                                     vp_v3(3.0f, 0.1f, 0.6f), 30);
    add_thing(plank, P.cube, vp_v3(3.0f, 0.1f, 0.6f), 0, 255, 120);
    vp_phys_hinge(P.world, plank, NULL, vp_v3(3.5f, 0.9f, 3.0f), vp_v3(0, 0, 1));
    add_box(VP_BODY_DYNAMIC, vp_v3(1.2f, 1.6f, 3.0f), vp_v3(0.4f, 0.4f, 0.4f), 15, 255, 255, 255);

    /* a chain of balls from a ball joint in the air, swinging */
    vp_body *prev = NULL;
    for (int i = 0; i < 5; i++) {
        vp_vec3 at = vp_v3(0.0f + i * 0.9f, 7.0f, -3.0f);
        vp_body *link = vp_phys_add_sphere(P.world, VP_BODY_DYNAMIC, at, 0.4f, 5);
        add_thing(link, P.ball, vp_v3(0.4f, 0.4f, 0.4f), 255, 60, 200);
        vp_phys_ball_joint(P.world, link, prev, prev ? vp_v3(at.x - 0.45f, at.y, at.z) : vp_v3(-0.45f, 7.0f, -3.0f));
        prev = link;
    }

    /* a bar the game turns: it sweeps whatever is on the floor */
    P.sweeper = vp_phys_add_box(P.world, VP_BODY_MOVED, vp_v3(0, 0.45f, -3.0f), vp_v3(4.0f, 0.25f, 0.25f), 0);
    add_thing(P.sweeper, P.cube, vp_v3(4.0f, 0.25f, 0.25f), 255, 60, 40);
    vp_body_set_spin(P.sweeper, vp_v3(0, 0.8f, 0));
}

static int init(vp_game *g)
{
    (void)g;
    P.rng = 777;
    P.cube = make_cube();
    P.ball = make_ball();
    if (!P.cube || !P.ball || vp3d_scene_create(&P.scene) != VP_OK) return VP_ERR_NOMEM;
    vp3d_view_init(&P.view, 60.0f);
    P.view.fog_start = 14;
    P.view.fog_end = 40;
    P.view.fog_floor = 0.3f;
    P.cam_angle = -0.5f;
    build();
    return VP_OK;
}

static void update(vp_game *g, float dt)
{
    vp_input *in = vp_game_input(g);
    P.time += dt;
    if (vp_input_pressed(in, "start1")) build();
    if (vp_input_pressed(in, "p1_button1"))
        add_ball(vp_v3(-4.0f + rnd() - 0.5f, 9.0f, 3.0f), 0.45f, 8, 0, 255, 255);
    if (vp_input_pressed(in, "p1_button2"))
        add_box(VP_BODY_DYNAMIC, vp_v3(rnd() * 6 - 3, 9.0f, rnd() * 4 - 2), vp_v3(0.4f, 0.4f, 0.4f), 10,
                255, 255, 0);
    float turn = vp_input_axis(in, "p1_stickx");
    if (vp_input_held(in, "p1_left")) turn = -1;
    if (vp_input_held(in, "p1_right")) turn = 1;
    P.cam_angle += turn * dt * 1.2f;

    /* now and then, something drops in by itself */
    P.drop_timer -= dt;
    if (P.drop_timer <= 0 && P.count < MAX_THINGS - 8) {
        P.drop_timer = 2.5f;
        if (rnd() < 0.5f) add_ball(vp_v3(rnd() * 8 - 4, 10.0f, rnd() * 8 - 4), 0.35f + rnd() * 0.3f, 6, 0, 255, 255);
        else add_box(VP_BODY_DYNAMIC, vp_v3(rnd() * 8 - 4, 10.0f, rnd() * 8 - 4), vp_v3(0.4f, 0.4f, 0.4f), 10,
                     255, 255, 0);
    }

    vp_phys_step(P.world, dt);

    /* sparks where things met hard */
    for (int i = 0; i < MAX_SPARKS; i++) if (P.sparks[i].life > 0) P.sparks[i].life -= dt;
    for (int i = 0; i < vp_phys_contact_count(P.world); i++) {
        vp_contact c;
        if (!vp_phys_contact(P.world, i, &c) || c.speed < 3.0f) continue;
        for (int k = 0; k < MAX_SPARKS; k++)
            if (P.sparks[k].life <= 0) { P.sparks[k].at = c.point; P.sparks[k].life = 0.3f; break; }
    }
}

static void draw(vp_game *g, vp_dvg *d)
{
    (void)g;
    vp_vec3 eye = vp_v3(sinf(P.cam_angle) * 18.0f, 10.0f, -cosf(P.cam_angle) * 18.0f);
    vp3d_look_at(&P.view, eye, vp_v3(0, 1.5f, 0), vp_v3(0, 1, 0));

    vp3d_scene_begin(P.scene, d, &P.view);
    for (int i = 0; i < P.count; i++) {
        const thing *t = &P.things[i];
        vp_xf place = vp_body_place(t->body);
        vp_xf drawn = vp_xf_mul2(place, vp_xf_scale(t->size.x, t->size.y, t->size.z));
        /* sleeping things a little dimmer: the engine has stopped working on them */
        int dim = vp_body_sleeping(t->body) && vp_body_get_kind(t->body) == VP_BODY_DYNAMIC;
        vp_color(d, dim ? t->r * 2 / 3 : t->r, dim ? t->g * 2 / 3 : t->g, dim ? t->b * 2 / 3 : t->b);
        vp3d_scene_mesh(P.scene, t->mesh, &drawn, VP3D_SOLID);
    }
    vp3d_scene_end(P.scene);

    vp_color(d, 255, 255, 255);
    for (int i = 0; i < MAX_SPARKS; i++) {
        if (P.sparks[i].life <= 0) continue;
        float sx, sy, r = 40 + 200 * (0.3f - P.sparks[i].life);
        if (!vp3d_project(&P.view, P.sparks[i].at, &sx, &sy)) continue;
        for (int k = 0; k < 4; k++) {
            float a = 0.785f + 1.5708f * k;
            vp_moveto(d, (int)(sx + cosf(a) * r * 0.4f), (int)(sy + sinf(a) * r * 0.4f));
            vp_lineto(d, (int)(sx + cosf(a) * r), (int)(sy + sinf(a) * r));
        }
    }

    char line[64];
    vp_text_font(VP_FONT_ASTEROIDS);
    vp_color(d, 0, 200, 255);
    vp_text(d, "PHYSICS", 2048, 3850, 120, VP_TEXT_CENTER);
    vp_color(d, 160, 160, 160);
    snprintf(line, sizeof line, "%d BODIES  BUTTON 1 BALL  BUTTON 2 BOX  START RESET", P.count);
    vp_text(d, line, 2048, 200, 60, VP_TEXT_CENTER);
}

static void shutdown(vp_game *g)
{
    (void)g;
    vp_phys_free(P.world);
    vp3d_scene_free(P.scene);
    vp3d_mesh_free(P.cube);
    vp3d_mesh_free(P.ball);
}

int main(int argc, char **argv)
{
    vp_game_desc desc = {0};
    desc.title = "VectorPie physics";
    desc.init = init;
    desc.update = update;
    desc.draw = draw;
    desc.shutdown = shutdown;
    return vp_run(&desc, argc, argv);
}
