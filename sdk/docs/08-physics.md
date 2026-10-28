# 08 — Physics: `vp_physics`

`#include <vectorpie/vp.h>` (`vp_physics.h`). `examples/physics` shows all of it.

Bodies that fall, bounce, slide, roll, tumble, stack, push each other and
come to rest; joints that hold them together, with limits and motors; rays;
and the contacts that began each step. Inside is
[ReactPhysics3D](https://www.reactphysics3d.com) 0.10.2 (zlib license),
built into the library: a game needs nothing more.

It draws nothing. Read where each body is and draw it with `vp_3d`, or any
way you like.

## A world

```c
vp_phys *world;
vp_phys_create(&world);                         /* gravity: -9.81 m/s^2 along y */

vp_phys_add_box(world, VP_BODY_STATIC, vp_v3(0, -0.5f, 0), vp_v3(10, 0.5f, 10), 0);   /* the floor */
vp_body *crate = vp_phys_add_box(world, VP_BODY_DYNAMIC, vp_v3(0, 5, 0),
                                 vp_v3(0.5f, 0.5f, 0.5f), 10);                        /* 10 kg */

/* update(), once per step, with vp_run's fixed dt: */
vp_phys_step(world, dt);

/* draw(): the body's place, the mesh scaled to the box first */
vp_xf place = vp_body_place(crate);
vp_xf drawn = vp_xf_mul2(place, vp_xf_scale(0.5f, 0.5f, 0.5f));    /* a cube mesh with corners at +-1 */
vp3d_scene_mesh(scene, cube, &drawn, VP3D_SOLID);
```

`vp_phys_free` frees the world with its bodies and joints.

**Units.** The game's world units; meters suit the defaults (gravity, and
the engine's thresholds for contacts and sleep, made for objects from a few
centimeters to tens of meters). Seconds, kilograms (only the ratios between
bodies matter), radians.

**Steps.** The engine is made for a fixed step: call `vp_phys_step` from
`update` with its `dt`. The same calls give the same result.
`vp_phys_set_iterations` gives the solver more passes (default 6 and 3):
steadier tall stacks and long chains, for more time.

## Bodies

| kind | moved by | for |
|---|---|---|
| `VP_BODY_STATIC` | nothing | the ground, walls |
| `VP_BODY_MOVED` | the game: its velocity, its spin, or its place | platforms, spinning bars, doors on a schedule: they push the others, nothing pushes them |
| `VP_BODY_DYNAMIC` | the physics: gravity, contacts, joints, impulses | crates, balls, debris, ragdoll parts |

| shape | made with |
|---|---|
| box | `vp_phys_add_box(w, kind, pos, half_sizes, mass)` |
| sphere | `vp_phys_add_sphere(w, kind, pos, radius, mass)` |
| capsule | `vp_phys_add_capsule(w, kind, pos, radius, height, mass)`: standing along y, `height` between the centers of its rounded ends. A character |
| convex hull | `vp_phys_add_hull(w, kind, pos, points, count, mass)`: the smallest convex shape round the points (relative to `pos`). Rocks, wedges |
| triangles | `vp_phys_add_triangles(w, kind, pos, points, n, tris, ntris)`: any shape, for scenery only (static, or moved) |

`pos` is the body's middle; it starts upright. The mass sets the body's
inertia too (from its shape). Each returns the body, or NULL with the reason
in `vp_last_error()`. `vp_body_remove` takes a body out, its joints with it.

**Reading.** `vp_body_place` (rotation and position, a `vp_xf` ready for
drawing), `vp_body_pos`, `vp_body_vel`, `vp_body_spin`, `vp_body_sleeping`.

**Moving.** `vp_body_set_vel` and `vp_body_set_spin` (a moved body's way of
moving); `vp_body_set_place` and `vp_body_set_pos` (a teleport);
`vp_body_impulse` (a kick through its middle, kg m/s) and
`vp_body_impulse_at` (at a world point: it also spins); `vp_body_force` (a
push for the next step, N: thrusters, wind).

**Settings.** `vp_body_set_material(b, bounce, friction)` (defaults 0.1,
0.5); `vp_body_set_damping`; `vp_body_lock_rotation(b, x, y, z)` (a
character that must stay upright locks x and z); `vp_body_set_gravity`;
`vp_body_set_kind`, `vp_body_set_mass`; `vp_body_set_user` for the game's own
pointer.

**Sleep.** A body that has stayed still a moment goes to sleep and costs
almost nothing until something touches it (or `vp_body_wake`).
`vp_body_set_sleep(b, 0)` keeps one awake.

## Joints

```c
vp_joint *axle = vp_phys_hinge(world, plank, NULL, vp_v3(0, 1, 0), vp_v3(0, 0, 1));   /* a seesaw */
```

| joint | holds |
|---|---|
| `vp_phys_ball_joint(w, a, b, anchor)` | at a point, turning any way: chains, ropes, a wrecking ball |
| `vp_phys_hinge(w, a, b, anchor, axis)` | turning about one axis: doors, wheels, seesaws, flippers |
| `vp_phys_slider(w, a, b, anchor, axis)` | sliding along one axis: pistons, drawers, lifts |
| `vp_phys_fixed_joint(w, a, b, anchor)` | welded together as they are |

Anchors and axes are in world coordinates, where the bodies are when the joint
is made. `b` NULL joins `a` to the world itself. Two bodies joined pass
through each other (an arm at its shoulder, a plank on its post); each still
collides with everything else.

Hinges and sliders take **limits** (`vp_joint_limits(j, on, min, max)`:
radians or meters from where the joint was made) and a **motor**
(`vp_joint_motor(j, speed, strength)`: `a` turns or slides at `speed` along
the axis, rad/s or m/s, with at most `strength`, N m or N; 0 turns the motor
off). `vp_joint_position` says where a hinge is turned or a slider slid, the
way the motor counts. A positive turn about an axis follows the right-hand
rule (in this left-handed world it looks the other way round on the screen).

## Rays and contacts

```c
vp_hit hit;
if (vp_phys_raycast(world, eye, target, &hit)) { /* hit.body, hit.point, hit.normal, hit.fraction */ }

for (int i = 0; i < vp_phys_contact_count(world); i++) {
    vp_contact c;
    vp_phys_contact(world, i, &c);
    if (c.speed > 3) vp_sound_play(audio, thud, c.speed / 10, 1, 0);
}
```

A ray finds the first body between two points; `vp_phys_raycast_ignore`
passes through one body (a character looking for the ground from its own
middle). The contacts are those that
**began** during the last step (two bodies that were not touching now are):
the bodies, a point where they touch, the normal (from `a` toward `b`), and
how fast they met along it, in m/s: a thud or a tap.

## What it costs

The Pi's CPU, not the beam. A few dozen bodies, awake, take well under a
millisecond a step on a Pi 5; sleeping bodies almost nothing. Triangle
scenery costs by its triangles near the moving bodies.

## Checked

On the PC (`dt` 1/60 s): a ball dropped from 5 m lands after 1.03 s (free
fall 1.01 s), meeting the floor at 9.97 m/s; five stacked boxes stand and go
to sleep; a 2 m pendulum swings with a period of 2.842 s (2.837 s in theory);
a hinge motor at 2 rad/s turns at 2.000 rad/s and stops at its limit; a
slider motor at 1.5 m/s moves at 1.500 m/s; a hull and a ball on a triangle
ramp behave.

The SDK's copy of ReactPhysics3D has one change, the sign of the hinge
motor's push (it ran away): see `src/third_party/reactphysics3d/VECTORPIE-CHANGES.txt`.
