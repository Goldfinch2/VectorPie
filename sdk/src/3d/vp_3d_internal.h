// Shared by the 3D sources. Not installed.
#pragma once

#include "../core/vp_internal.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct vp3d_mesh {
    std::vector<vp_vec3> verts;
    std::vector<std::vector<int>> faces;
    std::vector<std::pair<int, int>> lines;    // lone lines (OBJ "l", vp3d_mesh_line)
    std::vector<int32_t> faceColor;            // per face: 0xRRGGBB, or -1 (the current color)
    std::vector<int> faceMaterial;             // per face: index in materials, or -1
    std::vector<std::string> materials;        // OBJ material names (usemtl)
    bool hasColors = false;                    // some face has a color
    // Built by vp3d_mesh_finish:
    std::vector<std::pair<int, int>> edges;    // unique, low index first
    std::vector<std::vector<int>> strokes;     // vertex paths covering every edge once
    float radius = 0.0f;
    bool finished = false;

    // For solid drawing (also built by vp3d_mesh_finish):
    std::vector<vp_vec3> faceNormal;           // unit, pointing out of the solid
    std::vector<vp_vec3> facePoint;            // a point on the face (its centroid)
    std::vector<int> edgeFace0, edgeFace1;     // faces on each side of an edge, -1 = none
    std::vector<char> edgeMany;                // shared by more than two faces
    std::vector<char> edgeCrease;              // the faces meet sharper than the crease angle
    float creaseDeg = 40.0f;
    bool twoSided = false;                     // faces seen from both sides (plates)

    // For scenes (also built by vp3d_mesh_finish):
    std::unordered_map<uint64_t, int> edgeLookup;   // (low << 32 | high) -> edge index
    std::vector<int> triVerts;                 // faces as triangles (fans), 3 per triangle
    std::vector<int> triFace;                  // the face each triangle belongs to
};

// An edge's color: its first face's, else its second's; -1: the current color.
inline int32_t vp3d_edge_color(const vp3d_mesh *m, int e)
{
    if (e < 0) return -1;
    int f0 = m->edgeFace0[e], f1 = m->edgeFace1[e];
    if (f0 >= 0 && m->faceColor[f0] >= 0) return m->faceColor[f0];
    if (f1 >= 0 && m->faceColor[f1] >= 0) return m->faceColor[f1];
    return -1;
}

inline uint64_t vp3d_edge_key(int a, int b)
{
    int lo = a < b ? a : b, hi = a < b ? b : a;
    return ((uint64_t)(uint32_t)lo << 32) | (uint32_t)hi;
}

// Which faces are seen from the camera, and which edges show, for a mesh
// placed by toCam (model -> camera). Shared by vp3d_draw_mesh and scenes.
void vp3d_visible(const vp3d_mesh *m, const vp_xf &toCam, vp3d_mode mode,
                  std::vector<char> &seen, std::vector<int> &edges);

// Paths over a subset of edges (indices into `edges`), each edge once, as
// few as the shape allows: walks start where an odd number of edges is left.
void vp3d_build_strokes(int vertexCount, const std::vector<std::pair<int, int>> &edges,
                        const std::vector<int> &subset, std::vector<std::vector<int>> &out);

// Recompute edgeCrease from the face normals and creaseDeg.
void vp3d_update_creases(vp3d_mesh *m);

namespace vp3d_internal {

// The projection for a view: board = c + (x/z, y/z) * f.
struct Lens { float fx, fy, cx, cy; };
Lens lensOf(const vp3d_view *v);

// Brightness factor 0..1 at camera depth z.
float fogFactor(const vp3d_view *v, float z);

// One camera-space segment: near-plane cut, far reject, projection, board
// clip, fog on the color (r,g,b), then vp_moveto / vp_lineto.
void drawCameraSegment(vp_dvg *d, const vp3d_view *v, const Lens &l, vp_vec3 a, vp_vec3 b,
                       uint8_t r, uint8_t g, uint8_t bl);

}  // namespace vp3d_internal
