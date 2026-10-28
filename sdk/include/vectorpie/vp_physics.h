/* vp_physics.h — rigid-body physics (ReactPhysics3D inside).
 *
 *   vp_phys *world;
 *   vp_phys_create(&world);                                   // gravity -9.81 m/s^2 along y
 *   vp_phys_add_box(world, VP_BODY_STATIC, vp_v3(0, -0.5f, 0), vp_v3(10, 0.5f, 10), 0);
 *   vp_body *crate = vp_phys_add_box(world, VP_BODY_DYNAMIC, vp_v3(0, 5, 0),
 *                                    vp_v3(0.5f, 0.5f, 0.5f), 10);
 *   ...
 *   update():  vp_phys_step(world, dt);
 *   draw():    vp_xf place = vp_body_place(crate);             // its rotation and position
 *              vp_xf drawn = vp_xf_mul2(place, vp_xf_scale(0.5f, 0.5f, 0.5f));
 *              vp3d_draw_mesh(d, &view, cube, &drawn, VP3D_SOLID);
 *
 * WHAT IT IS. Bodies that fall, bounce, slide, roll, tumble, stack, push each
 * other and come to rest (they then SLEEP and cost almost nothing, until
 * something touches them); joints that hold them together (ball, hinge,
 * slider, fixed), with limits and motors; rays; and the contacts that began
 * each step, with how hard they hit. It draws nothing: read a body's place
 * (vp_body_place) and draw it with vp_3d, or any way you like.
 *
 * UNITS. The game's world units, meters suiting the defaults (gravity -9.81
 * along y, and the engine's sleep and contact thresholds, made for objects
 * a few centimeters to tens of meters across). Seconds; kilograms (only the
 * ratios between bodies matter); angles in radians.
 *
 * BODIES. Three kinds:
 *   VP_BODY_STATIC   never moves (the ground, walls): its mass is ignored
 *   VP_BODY_MOVED    moved by the game (a platform, a spinning bar): set its
 *                    velocity or spin, or place it; it pushes the others and
 *                    nothing pushes it
 *   VP_BODY_DYNAMIC  moved by the physics: gravity, contacts, joints, impulses
 * Shapes: a box (half sizes), a sphere, a capsule (a character standing up),
 * a convex hull (the smallest convex shape round some points: rocks, wedges),
 * and, for static scenery only, a mesh of triangles.
 *
 * STEPS. vp_phys_step(world, dt) once per update, with vp_run's fixed dt: the
 * engine is made for a fixed step. The same calls give the same result.
 *
 * Needs no SDL: available in every build.
 */
#ifndef VECTORPIE_VP_PHYSICS_H
#define VECTORPIE_VP_PHYSICS_H

#include "vp_3d.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vp_phys  vp_phys;        /* a world */
typedef struct vp_body  vp_body;
typedef struct vp_joint vp_joint;

typedef enum vp_body_kind {
    VP_BODY_STATIC = 0,
    VP_BODY_MOVED,
    VP_BODY_DYNAMIC
} vp_body_kind;

/* ---- the world -------------------------------------------------------------- */

int  vp_phys_create(vp_phys **out);
void vp_phys_free(vp_phys *w);          /* its bodies and joints with it */
void vp_phys_set_gravity(vp_phys *w, vp_vec3 g);
/* Advance by dt seconds. The contacts that began during it can be read
 * until the next step (vp_phys_contact_count). */
void vp_phys_step(vp_phys *w, float dt);
/* More solver passes: steadier stacks and chains, more time. Defaults 6
 * (velocities) and 3 (positions). */
void vp_phys_set_iterations(vp_phys *w, int velocity, int position);

/* ---- bodies ------------------------------------------------------------------- */

/* Each returns the body (NULL on failure: vp_last_error says why). `pos` is
 * where its middle is (a capsule's: halfway between its ends); upright,
 * unturned. `mass` (kg) for a dynamic body; ignored for the others. */
vp_body *vp_phys_add_box(vp_phys *w, vp_body_kind kind, vp_vec3 pos, vp_vec3 half, float mass);
vp_body *vp_phys_add_sphere(vp_phys *w, vp_body_kind kind, vp_vec3 pos, float radius, float mass);
/* Standing along y: `height` between the centers of its two rounded ends
 * (the whole capsule is height + 2 x radius tall). */
vp_body *vp_phys_add_capsule(vp_phys *w, vp_body_kind kind, vp_vec3 pos, float radius,
                             float height, float mass);
/* The convex hull of `count` points (4 or more, not all in one plane),
 * relative to pos. */
vp_body *vp_phys_add_hull(vp_phys *w, vp_body_kind kind, vp_vec3 pos, const vp_vec3 *points,
                          int count, float mass);
/* Static scenery from triangles: `tris` holds 3 vertex indices each,
 * wound as vp3d meshes are (either way, but all the same way). The points
 * are relative to pos. Only VP_BODY_STATIC (or MOVED: a moving floor). */
vp_body *vp_phys_add_triangles(vp_phys *w, vp_body_kind kind, vp_vec3 pos,
                               const vp_vec3 *points, int npoints, const int *tris, int ntris);

void vp_body_remove(vp_body *b);        /* its joints go with it */

/* ---- reading a body -------------------------------------------------------------- */

/* Its rotation and position: ready to place its mesh (vp3d_draw_mesh,
 * vp3d_scene_mesh; scale the mesh first, e.g. by the box's half sizes). */
vp_xf    vp_body_place(const vp_body *b);
vp_vec3  vp_body_pos(const vp_body *b);
vp_vec3  vp_body_vel(const vp_body *b);     /* m/s */
vp_vec3  vp_body_spin(const vp_body *b);    /* rad/s about each axis */
int      vp_body_sleeping(const vp_body *b);
vp_body_kind vp_body_get_kind(const vp_body *b);

/* ---- moving a body -------------------------------------------------------------- */

/* Put it there (its rotation from place's matrix: a rotation, no scale). A
 * teleport: for a body that keeps moving, prefer its velocity. */
void vp_body_set_place(vp_body *b, const vp_xf *place);
void vp_body_set_pos(vp_body *b, vp_vec3 pos);
void vp_body_set_vel(vp_body *b, vp_vec3 v);
void vp_body_set_spin(vp_body *b, vp_vec3 w);
/* A kick: impulse in kg m/s, through its middle, or at a world point (that
 * also sets it spinning). Dynamic bodies only. */
void vp_body_impulse(vp_body *b, vp_vec3 impulse);
void vp_body_impulse_at(vp_body *b, vp_vec3 impulse, vp_vec3 point);
/* A push for the next step only (N), as a thruster or wind does. */
void vp_body_force(vp_body *b, vp_vec3 force);
void vp_body_set_kind(vp_body *b, vp_body_kind kind);
void vp_body_set_mass(vp_body *b, float mass);
/* bounce 0 (none) .. 1 (keeps all its speed); friction 0 (ice) .. 1 (rubber)
 * and beyond. Defaults 0.1 and 0.5. */
void vp_body_set_material(vp_body *b, float bounce, float friction);
/* Slowing down by itself, per second: 0 none (default 0 and 0.05). */
void vp_body_set_damping(vp_body *b, float linear, float angular);
/* Turning about x, y, z allowed (1) or not (0): a character that must stay
 * upright locks x and z. */
void vp_body_lock_rotation(vp_body *b, int x, int y, int z);
void vp_body_set_gravity(vp_body *b, int on);
void vp_body_set_sleep(vp_body *b, int allowed);   /* default allowed */
void vp_body_wake(vp_body *b);

/* The game's own pointer for the body (a bean, a crate...). */
void  vp_body_set_user(vp_body *b, void *user);
void *vp_body_user(const vp_body *b);

/* ---- joints --------------------------------------------------------------------- */

/* Two bodies held at a point (world coordinates, where they are now); `b`
 * NULL holds `a` to the world itself. NULL on failure. Two bodies joined do
 * not collide with each other (a limb at its shoulder, a plank on its post). */
vp_joint *vp_phys_ball_joint(vp_phys *w, vp_body *a, vp_body *b, vp_vec3 anchor);
/* Turning about one axis through the anchor (a door, a wheel, a seesaw). */
vp_joint *vp_phys_hinge(vp_phys *w, vp_body *a, vp_body *b, vp_vec3 anchor, vp_vec3 axis);
/* Sliding along one axis (a piston, a drawer). */
vp_joint *vp_phys_slider(vp_phys *w, vp_body *a, vp_body *b, vp_vec3 anchor, vp_vec3 axis);
/* Welded together as they are. */
vp_joint *vp_phys_fixed_joint(vp_phys *w, vp_body *a, vp_body *b, vp_vec3 anchor);
void vp_joint_remove(vp_joint *j);

/* Hinges and sliders: limits (radians, or meters along the axis, from where
 * the joint was made); a motor driving it at `speed` (rad/s or m/s) with at
 * most `strength` (N m or N); 0 strength turns the motor off. */
void  vp_joint_limits(vp_joint *j, int on, float min, float max);
void  vp_joint_motor(vp_joint *j, float speed, float strength);
/* Where a hinge is turned (radians) or a slider slid (meters). */
float vp_joint_position(const vp_joint *j);

/* ---- rays and contacts ------------------------------------------------------------ */

typedef struct vp_hit {
    vp_body *body;
    vp_vec3  point;          /* where the ray met it */
    vp_vec3  normal;         /* its surface there, facing the ray */
    float    fraction;       /* how far along, 0 (from) .. 1 (to) */
} vp_hit;

/* The first body along the ray from `from` to `to`: 1 and *hit, else 0. */
int vp_phys_raycast(vp_phys *w, vp_vec3 from, vp_vec3 to, vp_hit *hit);
/* The same, passing through `ignore` (a character looking for the ground
 * from its own middle). */
int vp_phys_raycast_ignore(vp_phys *w, vp_vec3 from, vp_vec3 to, const vp_body *ignore, vp_hit *hit);

typedef struct vp_contact {
    vp_body *a, *b;
    vp_vec3  point;          /* a point where they touch, in the world */
    vp_vec3  normal;         /* from a toward b */
    float    speed;          /* how fast they met along the normal (m/s): a thud or a tap */
} vp_contact;

/* The contacts that began during the last step (two bodies that were not
 * touching now are), for sounds and game events. */
int vp_phys_contact_count(const vp_phys *w);
int vp_phys_contact(const vp_phys *w, int i, vp_contact *out);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_PHYSICS_H */
