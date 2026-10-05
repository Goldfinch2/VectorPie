/* vp_3d.h — 3D: transforms, a camera, meshes, wireframe drawing.
 *
 *   vp3d_view view;
 *   vp3d_view_init(&view, 60.0f);                         // 60 degree field of view
 *   vp3d_look_at(&view, eye, target, vp_v3(0, 1, 0));
 *
 *   vp3d_mesh *tank;
 *   vp3d_mesh_load_obj(path, NULL, &tank);
 *
 *   vp_color(d, 0, 255, 0);
 *   vp_xf place = vp_xf_mul2(vp_xf_translate(0, 0, 20), vp_xf_rot_y(angle));
 *   vp3d_draw_mesh_wire(d, &view, tank, &place);
 *
 * UNITS. World units are the game's own (metres, feet...). Camera space:
 * +x right, +y up, +z forward, into the screen. Angles in radians, except the
 * field of view in degrees.
 *
 * THE SCREEN SHAPE. The board's square 0..4095 space is stretched by the
 * monitor to 4:3 (or 3:4 when vertical). The view compensates (`aspect`), so a
 * circle in the world is round on the tube.
 *
 * WIREFRAME draws every edge of a mesh, see-through, each edge once, in
 * connected strokes (a mesh is turned into as few strokes as its shape
 * allows). Lines crossing the near plane are cut there. Hidden-line removal
 * (solid objects) comes in a later version of this API.
 *
 * FOG (optional) dims lines with distance: on a vector monitor brightness is
 * the one depth cue there is.
 *
 * Needs no SDL: available in every build. Float math throughout.
 */
#ifndef VECTORPIE_VP_3D_H
#define VECTORPIE_VP_3D_H

#include "vp_dvg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- vectors and transforms ------------------------------------------------ */

typedef struct vp_vec3 { float x, y, z; } vp_vec3;

vp_vec3 vp_v3(float x, float y, float z);
vp_vec3 vp_v3_add(vp_vec3 a, vp_vec3 b);
vp_vec3 vp_v3_sub(vp_vec3 a, vp_vec3 b);
vp_vec3 vp_v3_scale(vp_vec3 a, float s);
float   vp_v3_dot(vp_vec3 a, vp_vec3 b);
vp_vec3 vp_v3_cross(vp_vec3 a, vp_vec3 b);
float   vp_v3_length(vp_vec3 a);
vp_vec3 vp_v3_normalize(vp_vec3 a);     /* the zero vector stays zero */

/* A placement: p' = m * p + t, with m a 3x3 matrix, ROW-MAJOR (m[row*3+col]).
 * Rotations, uniform or per-axis scale, and translation. */
typedef struct vp_xf { float m[9]; float t[3]; } vp_xf;

vp_xf   vp_xf_identity(void);
vp_xf   vp_xf_translate(float x, float y, float z);
vp_xf   vp_xf_scale(float sx, float sy, float sz);
vp_xf   vp_xf_rot_x(float radians);
vp_xf   vp_xf_rot_y(float radians);
vp_xf   vp_xf_rot_z(float radians);
/* a then b: applies b first, then a (like matrix product a * b). */
vp_xf   vp_xf_mul2(vp_xf a, vp_xf b);
vp_xf   vp_xf_inverse(vp_xf a);         /* a's matrix must be invertible */
vp_vec3 vp_xf_point(const vp_xf *a, vp_vec3 p);    /* m * p + t */
vp_vec3 vp_xf_dir(const vp_xf *a, vp_vec3 v);      /* m * v (no translation) */

/* ---- the view: camera, lens, fog --------------------------------------------- */

typedef struct vp3d_view {
    vp_xf world_to_camera;  /* where the camera is (set by vp3d_look_at / vp3d_set_pose) */
    float fov_deg;          /* horizontal field of view */
    float near_z;           /* nothing closer is drawn; lines crossing it are cut. Default 0.1 */
    float far_z;            /* nothing farther is drawn (whole lines). Default 10000 */
    float aspect;           /* the monitor's width / height: 4/3 (default), 3/4 vertical */
    /* fog: full brightness up to fog_start, fog_floor at fog_end and beyond.
     * fog_end <= fog_start: no fog (the default). */
    float fog_start, fog_end, fog_floor;
} vp3d_view;

/* Defaults, with this field of view; the camera at the origin looking along +z. */
void vp3d_view_init(vp3d_view *v, float fov_deg);

/* Put the camera at `eye`, looking at `target`; `up` is the world's up
 * direction, usually (0,1,0). Returns 0 (and changes nothing) if eye and
 * target coincide or up is parallel to the view. */
int  vp3d_look_at(vp3d_view *v, vp_vec3 eye, vp_vec3 target, vp_vec3 up);

/* Put the camera at `pos`, turned by heading (about y, positive turns
 * right), then pitch (about x, positive looks up), then roll (about z). */
void vp3d_set_pose(vp3d_view *v, vp_vec3 pos, float heading, float pitch, float roll);

/* Where the camera is, in world coordinates. */
vp_vec3 vp3d_eye(const vp3d_view *v);

/* World point -> board coordinates. 0 if behind the near plane (sx, sy then
 * untouched). The point may be off the board (outside 0..4095). */
int  vp3d_project(const vp3d_view *v, vp_vec3 world, float *sx, float *sy);

/* ---- drawing lines ------------------------------------------------------------- */

/* A line in world space, in the current color (dimmed by fog). */
void vp3d_line(vp_dvg *d, const vp3d_view *v, vp_vec3 a, vp_vec3 b);

/* ---- meshes ----------------------------------------------------------------------- */

typedef struct vp3d_mesh vp3d_mesh;

/* Build one in code: vertices, then faces (vertex indices, 3 or more, wound
 * anticlockwise seen from outside) and/or lone lines, then finish. */
int  vp3d_mesh_create(vp3d_mesh **out);
int  vp3d_mesh_vertex(vp3d_mesh *m, float x, float y, float z);    /* its index, or -1 */
int  vp3d_mesh_face(vp3d_mesh *m, const int *indices, int count);
int  vp3d_mesh_line(vp3d_mesh *m, int a, int b);
/* Builds the edges and strokes. Call once, after the last vertex/face/line. */
int  vp3d_mesh_finish(vp3d_mesh *m);
void vp3d_mesh_free(vp3d_mesh *m);

/* Loading a Wavefront OBJ file. */
typedef struct vp3d_obj_options {
    float scale;            /* 0 = 1 */
    int   center;           /* 1 = move the model's bounding-box center to the origin */
    int   z_up;             /* 1 = the file is Z-up (some tools): turn it Y-up */
} vp3d_obj_options;

/* Reads v, f (all index forms, negative indices) and l; ignores the rest.
 * `options` may be NULL. Errors name the file and line (vp_last_error). */
int  vp3d_mesh_load_obj(const char *path, const vp3d_obj_options *options, vp3d_mesh **out);

int  vp3d_mesh_vertex_count(const vp3d_mesh *m);
int  vp3d_mesh_face_count(const vp3d_mesh *m);
int  vp3d_mesh_edge_count(const vp3d_mesh *m);
int  vp3d_mesh_stroke_count(const vp3d_mesh *m);   /* connected strokes in wireframe */
/* The radius of the smallest origin-centered sphere holding the mesh. */
float vp3d_mesh_radius(const vp3d_mesh *m);

/* Every edge, see-through, in the current color (dimmed by fog). `place` puts
 * the mesh in the world (NULL = as is). */
void vp3d_draw_mesh_wire(vp_dvg *d, const vp3d_view *v, const vp3d_mesh *m, const vp_xf *place);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_3D_H */
