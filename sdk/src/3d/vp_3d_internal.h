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
};

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
