// Shared by the 3D sources. Not installed.
#pragma once

#include "../core/vp_internal.h"

#include <cstdint>
#include <utility>
#include <vector>

struct vp3d_mesh {
    std::vector<vp_vec3> verts;
    std::vector<std::vector<int>> faces;
    std::vector<std::pair<int, int>> lines;    // lone lines (OBJ "l", vp3d_mesh_line)
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
};

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
