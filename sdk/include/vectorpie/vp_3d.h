/* vp_3d.h — 3D: transforms, a camera, meshes, solid and wireframe drawing.
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
 *   vp3d_draw_mesh(d, &view, tank, &place, VP3D_SOLID);
 *
 * UNITS. World units are the game's own (meters, feet...). Camera space:
 * +x right, +y up, +z forward, into the screen. Angles in radians, except the
 * field of view in degrees.
 *
 * THE SCREEN SHAPE. The board's square 0..4095 space is stretched by the
 * monitor to 4:3 (or 3:4 when vertical). The view compensates (`aspect`), so a
 * circle in the world is round on the tube.
 *
 * SOLID (the default) draws a mesh as an object: only its outline (the
 * silhouette, where a face turned to the camera meets one turned away) and
 * its sharp creases (where faces meet at a sharper angle than the mesh's
 * crease angle). WIREFRAME draws every edge, see-through. Either way each
 * edge is drawn once, in as few connected strokes as the shape allows, and
 * lines crossing the near plane are cut there. One object hiding another:
 * draw through a SCENE (below).
 *
 * HANDEDNESS. This world is left-handed: from the default camera, +x is
 * right, +y up, +z forward. OBJ files are right-handed (Blender and most
 * tools), so the loader flips z: models look as they were authored.
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
    /* fog: full brightness up to fog_start, fog_floor at fog_end and beyond,
     * by distance along the view; a line spanning depths fades along its
     * length (drawn in up to 12 steps). fog_end <= fog_start: no fog (the
     * default). */
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
/* A face's own color (face: its index, in the order faces were added). Its
 * edges are drawn in it instead of the current color; an edge between two
 * colored faces takes the first face's. Can be changed at any time, also
 * after finishing. Faces without one, and lone lines, use the current color. */
int  vp3d_mesh_face_color(vp3d_mesh *m, int face, uint8_t r, uint8_t g, uint8_t b);
/* A face's material name in its OBJ file (its usemtl), or NULL: to color
 * faces by their material yourself (vp3d_mesh_face_color), e.g. from a
 * palette that changes while the game runs. Read even with no_materials. */
const char *vp3d_mesh_face_material(const vp3d_mesh *m, int face);
/* Builds the edges, the face normals and the strokes. Call once, after the
 * last vertex/face/line. Faces must all be wound the same way round (either
 * way: the SDK works out which side is outside). */
int  vp3d_mesh_finish(vp3d_mesh *m);
void vp3d_mesh_free(vp3d_mesh *m);

/* Solid drawing: faces meeting at a sharper angle than this keep their edge
 * (default 40 degrees; 0 = every edge between seen faces, 180 = outline
 * only). Can be changed at any time. */
void vp3d_mesh_set_crease(vp3d_mesh *m, float degrees);
/* Faces seen from both sides: for open shapes (a flag, a plate) that have no
 * inside. Off by default. */
void vp3d_mesh_set_two_sided(vp3d_mesh *m, int two_sided);

/* Loading a Wavefront OBJ file. */
typedef struct vp3d_obj_options {
    float scale;            /* 0 = 1 */
    int   center;           /* 1 = move the model's bounding-box center to the origin */
    int   z_up;             /* 1 = the file is Z-up (some tools): turn it Y-up */
    float crease_deg;       /* 0 = the default (40), see vp3d_mesh_set_crease */
    int   no_materials;     /* 1 = ignore the file's colors (mtllib); the faces' material
                               names are kept (vp3d_mesh_face_material) */
} vp3d_obj_options;

/* Reads v, f (all index forms, negative indices) and l, and the faces'
 * colors: "mtllib" material libraries (next to the file) give each material
 * its diffuse color (Kd), "usemtl" gives it to the faces that follow (see
 * vp3d_mesh_face_color). A missing library is not an error: those faces use
 * the current color. Ignores the rest. `options` may be NULL. Errors name the
 * file and line (vp_last_error). */
int  vp3d_mesh_load_obj(const char *path, const vp3d_obj_options *options, vp3d_mesh **out);

int  vp3d_mesh_vertex_count(const vp3d_mesh *m);
int  vp3d_mesh_face_count(const vp3d_mesh *m);
int  vp3d_mesh_edge_count(const vp3d_mesh *m);
int  vp3d_mesh_stroke_count(const vp3d_mesh *m);   /* connected strokes in wireframe */
/* The radius of the smallest origin-centered sphere holding the mesh. */
float vp3d_mesh_radius(const vp3d_mesh *m);
/* The mesh's bounding box (to stand a model on the floor, center it...). */
void  vp3d_mesh_bounds(const vp3d_mesh *m, vp_vec3 *min, vp_vec3 *max);

/* ---- textures: line drawings on faces ----------------------------------------------
 * A texture is a small vector drawing in its own square (u across, v up,
 * 0..1): lines and paths. On a face it is laid by the face's texture
 * coordinates (an OBJ file's vt, or vp3d_mesh_face_uv), or by world mapping
 * (vp3d_mesh_texture_world); coordinates beyond 0..1 tile it. Its strokes are
 * clipped to each face, drawn on the faces seen (all of them in wireframe),
 * hidden in a scene like any line, never by their own face. A stroke takes
 * its own color, else its face's (or the current color) at the texture's dim.
 *   vp3d_texture *grid;
 *   vp3d_texture_create(&grid);
 *   vp3d_texture_line(grid, 0, 0, 1, 0);           // a tile's bottom and left side:
 *   vp3d_texture_line(grid, 0, 0, 0, 1);           // tiled, a grid
 *   vp3d_mesh_face_texture(floor, -1, grid);       // every face
 *   vp3d_mesh_texture_world(floor, 2.0f);          // 2 m squares, whatever the box's size
 * Faces are taken to be flat and their coordinates affine (exporters write
 * them so): the first three corners set the mapping. */
typedef struct vp3d_texture vp3d_texture;

int  vp3d_texture_create(vp3d_texture **out);
void vp3d_texture_free(vp3d_texture *t);          /* not while a mesh still uses it */
int  vp3d_texture_line(vp3d_texture *t, float u0, float v0, float u1, float v1);
/* `count` points (u, v pairs) joined; closed: back to the first. */
int  vp3d_texture_path(vp3d_texture *t, const float *uv, int count, int closed);
/* The strokes added from now on in this color (0xRRGGBB), or -1: the face's
 * own color, at the texture's dim (vp3d_texture_set_dim, default 0.5). */
int  vp3d_texture_color(vp3d_texture *t, int32_t rgb);
void vp3d_texture_set_dim(vp3d_texture *t, float dim);
int  vp3d_texture_stroke_count(const vp3d_texture *t);
/* A texture file, one command a line ('#' starts a comment):
 *   l u0 v0 u1 v1    a line          p u v u v ...   a path
 *   q u v u v ...    a closed shape  c RRGGBB | c -  own color, or the face's
 *   d 0.5            the face-colored strokes' brightness */
int  vp3d_texture_load(const char *path, vp3d_texture **out);

/* A face's texture coordinates: a u, v pair per corner, in its corners'
 * order (an OBJ file's vt give them too). */
int  vp3d_mesh_face_uv(vp3d_mesh *m, int face, const float *uv);
/* The texture on a face (face -1: every face; t NULL: none). The mesh keeps
 * a pointer: the texture must outlive it. */
int  vp3d_mesh_face_texture(vp3d_mesh *m, int face, const vp3d_texture *t);
/* World mapping: instead of the faces' coordinates, the texture laid flat
 * on each face (on its main axis), one tile `tile` world units across, by
 * the mesh's size as placed (a box scaled to a long platform gets more
 * tiles, not stretched ones), moving with it. 0: the faces' coordinates. */
void vp3d_mesh_texture_world(vp3d_mesh *m, float tile);

typedef enum vp3d_mode {
    VP3D_SOLID = 0,         /* outline and sharp creases: an object */
    VP3D_WIREFRAME          /* every edge, see-through */
} vp3d_mode;

/* Draw a mesh in the current color, or its faces' own colors (dimmed by
 * fog). `place` puts it in the world (NULL = as is). */
void vp3d_draw_mesh(vp_dvg *d, const vp3d_view *v, const vp3d_mesh *m, const vp_xf *place,
                    vp3d_mode mode);
/* The same as vp3d_draw_mesh(..., VP3D_WIREFRAME). */
void vp3d_draw_mesh_wire(vp_dvg *d, const vp3d_view *v, const vp3d_mesh *m, const vp_xf *place);

/* ---- scenes: objects hiding each other ------------------------------------------
 *
 * vp3d_draw_mesh hides a mesh's own back, nothing else. A SCENE hides
 * everything behind solid objects:
 *
 *   vp3d_scene_begin(scene, d, &view);
 *   vp_color(d, 0, 140, 0);
 *   vp3d_scene_line(scene, a, b);                           // the floor...
 *   vp_color(d, 0, 255, 255);
 *   vp3d_scene_mesh(scene, ship, &place, VP3D_SOLID);       // ...hidden behind the ship
 *   vp3d_scene_end(scene);                                  // cut, then draw
 *
 * Between begin and end, meshes and lines are collected (each with the color
 * current when it was added, or a mesh's face colors); vp3d_scene_end cuts every line by every SOLID
 * face nearer the camera, then draws the visible pieces, in the order they
 * were added. Order does not matter for hiding: add things in any order.
 *   SOLID meshes hide what is behind them, and themselves (a wing behind the
 *     body is hidden too). A face never hides a line between two of its own
 *     corners (its edges, or a diagonal of a quad that is not quite flat),
 *     nor one from a corner of it that runs within 5 degrees of its plane (a
 *     slight fold between neighboring faces).
 *   WIREFRAME meshes and lines hide nothing; they are hidden by solids in
 *     front of them.
 * Faces are taken to be convex (exporters write triangles and quads). */
typedef struct vp3d_scene vp3d_scene;

typedef struct vp3d_scene_stats {
    uint32_t lines;          /* line segments added */
    uint32_t faces;          /* solid faces that could hide something */
    uint32_t cut;            /* segments partly or wholly hidden */
    uint32_t pieces;         /* visible pieces drawn */
} vp3d_scene_stats;

int  vp3d_scene_create(vp3d_scene **out);
void vp3d_scene_free(vp3d_scene *s);
void vp3d_scene_begin(vp3d_scene *s, vp_dvg *d, const vp3d_view *v);
void vp3d_scene_mesh(vp3d_scene *s, const vp3d_mesh *m, const vp_xf *place, vp3d_mode mode);
void vp3d_scene_line(vp3d_scene *s, vp_vec3 a, vp_vec3 b);
/* A solid, flat, convex polygon in world space, for geometry that changes
 * every frame (walls that grow, a door that opens) where building a mesh
 * would be wasteful. It hides what is behind it, from both sides. Of its
 * edges (edge i joins pts[i] and pts[i+1], the last one back to pts[0]),
 * only those whose bit is set in `edge_mask` are drawn, in the current color:
 * VP3D_ALL_EDGES draws its outline, 0 makes it an invisible screen. 3 to 16
 * points. */
#define VP3D_ALL_EDGES 0xffffu
void vp3d_scene_polygon(vp3d_scene *s, const vp_vec3 *pts, int count, unsigned edge_mask);
/* Text on the screen that hides what the scene draws behind it: the current
 * font's letters (vp_text_font; VP_FONT_COMIC only, with its wobble: the
 * other fonts' strokes have no inside) as invisible screens just in front of
 * the camera. Same arguments as vp_text, whose call draws the letters
 * themselves (before or after the scene). */
void vp3d_scene_text(vp3d_scene *s, const char *text, float x, float y, float height, int align);
void vp3d_scene_end(vp3d_scene *s);
/* Things resting on a surface sink into it a little (a body on the floor in
 * a physics world, a model placed by eye): their lowest lines then count as
 * behind it and are cut. A line less than `tolerance` world units behind a
 * face is not hidden by it. Default 0; a few centimeters suits a game in
 * meters. */
void vp3d_scene_set_contact(vp3d_scene *s, float tolerance);
void vp3d_scene_get_stats(const vp3d_scene *s, vp3d_scene_stats *stats);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_3D_H */
