// 3D: vectors, placements, the view (camera, lens, fog), projection, lines,
// meshes and wireframe drawing. Float math; everything ends as vp_moveto /
// vp_lineto on the device layer.

#include "vp_3d_internal.h"

#include <cmath>
#include <new>
#include <unordered_map>

// ---------------------------------------------------------------------------
// Vectors
// ---------------------------------------------------------------------------

vp_vec3 vp_v3(float x, float y, float z) { vp_vec3 v = { x, y, z }; return v; }
vp_vec3 vp_v3_add(vp_vec3 a, vp_vec3 b) { return vp_v3(a.x + b.x, a.y + b.y, a.z + b.z); }
vp_vec3 vp_v3_sub(vp_vec3 a, vp_vec3 b) { return vp_v3(a.x - b.x, a.y - b.y, a.z - b.z); }
vp_vec3 vp_v3_scale(vp_vec3 a, float s) { return vp_v3(a.x * s, a.y * s, a.z * s); }
float   vp_v3_dot(vp_vec3 a, vp_vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
vp_vec3 vp_v3_cross(vp_vec3 a, vp_vec3 b)
{
    return vp_v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
float   vp_v3_length(vp_vec3 a) { return std::sqrt(vp_v3_dot(a, a)); }
vp_vec3 vp_v3_normalize(vp_vec3 a)
{
    float l = vp_v3_length(a);
    return l > 0.0f ? vp_v3_scale(a, 1.0f / l) : a;
}

// ---------------------------------------------------------------------------
// Placements
// ---------------------------------------------------------------------------

static vp_xf xfFromRows(float a, float b, float c, float d, float e, float f,
                        float g, float h, float i)
{
    vp_xf x;
    x.m[0] = a; x.m[1] = b; x.m[2] = c;
    x.m[3] = d; x.m[4] = e; x.m[5] = f;
    x.m[6] = g; x.m[7] = h; x.m[8] = i;
    x.t[0] = x.t[1] = x.t[2] = 0.0f;
    return x;
}

vp_xf vp_xf_identity(void) { return xfFromRows(1, 0, 0, 0, 1, 0, 0, 0, 1); }

vp_xf vp_xf_translate(float x, float y, float z)
{
    vp_xf r = vp_xf_identity();
    r.t[0] = x; r.t[1] = y; r.t[2] = z;
    return r;
}

vp_xf vp_xf_scale(float sx, float sy, float sz) { return xfFromRows(sx, 0, 0, 0, sy, 0, 0, 0, sz); }

vp_xf vp_xf_rot_x(float a)
{
    float c = std::cos(a), s = std::sin(a);
    return xfFromRows(1, 0, 0, 0, c, -s, 0, s, c);
}

vp_xf vp_xf_rot_y(float a)
{
    float c = std::cos(a), s = std::sin(a);
    return xfFromRows(c, 0, s, 0, 1, 0, -s, 0, c);
}

vp_xf vp_xf_rot_z(float a)
{
    float c = std::cos(a), s = std::sin(a);
    return xfFromRows(c, -s, 0, s, c, 0, 0, 0, 1);
}

vp_xf vp_xf_mul2(vp_xf a, vp_xf b)
{
    vp_xf r;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            r.m[i * 3 + j] = a.m[i * 3] * b.m[j] + a.m[i * 3 + 1] * b.m[3 + j] + a.m[i * 3 + 2] * b.m[6 + j];
        r.t[i] = a.m[i * 3] * b.t[0] + a.m[i * 3 + 1] * b.t[1] + a.m[i * 3 + 2] * b.t[2] + a.t[i];
    }
    return r;
}

vp_xf vp_xf_inverse(vp_xf a)
{
    const float *m = a.m;
    float c00 = m[4] * m[8] - m[5] * m[7];
    float c01 = m[5] * m[6] - m[3] * m[8];
    float c02 = m[3] * m[7] - m[4] * m[6];
    float det = m[0] * c00 + m[1] * c01 + m[2] * c02;
    if (det == 0.0f) return vp_xf_identity();
    float k = 1.0f / det;
    vp_xf r;
    r.m[0] = c00 * k;
    r.m[1] = (m[2] * m[7] - m[1] * m[8]) * k;
    r.m[2] = (m[1] * m[5] - m[2] * m[4]) * k;
    r.m[3] = c01 * k;
    r.m[4] = (m[0] * m[8] - m[2] * m[6]) * k;
    r.m[5] = (m[2] * m[3] - m[0] * m[5]) * k;
    r.m[6] = c02 * k;
    r.m[7] = (m[1] * m[6] - m[0] * m[7]) * k;
    r.m[8] = (m[0] * m[4] - m[1] * m[3]) * k;
    for (int i = 0; i < 3; i++)
        r.t[i] = -(r.m[i * 3] * a.t[0] + r.m[i * 3 + 1] * a.t[1] + r.m[i * 3 + 2] * a.t[2]);
    return r;
}

vp_vec3 vp_xf_point(const vp_xf *a, vp_vec3 p)
{
    const float *m = a->m;
    return vp_v3(m[0] * p.x + m[1] * p.y + m[2] * p.z + a->t[0],
                 m[3] * p.x + m[4] * p.y + m[5] * p.z + a->t[1],
                 m[6] * p.x + m[7] * p.y + m[8] * p.z + a->t[2]);
}

vp_vec3 vp_xf_dir(const vp_xf *a, vp_vec3 v)
{
    const float *m = a->m;
    return vp_v3(m[0] * v.x + m[1] * v.y + m[2] * v.z,
                 m[3] * v.x + m[4] * v.y + m[5] * v.z,
                 m[6] * v.x + m[7] * v.y + m[8] * v.z);
}

// ---------------------------------------------------------------------------
// The view
// ---------------------------------------------------------------------------

void vp3d_view_init(vp3d_view *v, float fov_deg)
{
    v->world_to_camera = vp_xf_identity();
    v->fov_deg = fov_deg > 1.0f && fov_deg < 179.0f ? fov_deg : 60.0f;
    v->near_z = 0.1f;
    v->far_z = 10000.0f;
    v->aspect = 4.0f / 3.0f;
    v->fog_start = v->fog_end = 0.0f;
    v->fog_floor = 0.0f;
}

int vp3d_look_at(vp3d_view *v, vp_vec3 eye, vp_vec3 target, vp_vec3 up)
{
    vp_vec3 f = vp_v3_sub(target, eye);
    if (vp_v3_length(f) == 0.0f) return 0;
    f = vp_v3_normalize(f);
    vp_vec3 r = vp_v3_cross(up, f);         // camera x: right
    if (vp_v3_length(r) < 1e-6f) return 0;
    r = vp_v3_normalize(r);
    vp_vec3 u = vp_v3_cross(f, r);          // camera y: up
    vp_xf w = xfFromRows(r.x, r.y, r.z, u.x, u.y, u.z, f.x, f.y, f.z);
    vp_vec3 t = vp_xf_dir(&w, eye);
    w.t[0] = -t.x; w.t[1] = -t.y; w.t[2] = -t.z;
    v->world_to_camera = w;
    return 1;
}

void vp3d_set_pose(vp3d_view *v, vp_vec3 pos, float heading, float pitch, float roll)
{
    // Camera to world: move to pos, turn by heading, tilt by pitch, roll.
    // A positive pitch looks up: rot_x turns +z down, so it takes -pitch.
    vp_xf c = vp_xf_mul2(vp_xf_translate(pos.x, pos.y, pos.z),
              vp_xf_mul2(vp_xf_rot_y(heading),
              vp_xf_mul2(vp_xf_rot_x(-pitch), vp_xf_rot_z(roll))));
    v->world_to_camera = vp_xf_inverse(c);
}

vp_vec3 vp3d_eye(const vp3d_view *v)
{
    vp_xf c = vp_xf_inverse(v->world_to_camera);
    return vp_v3(c.t[0], c.t[1], c.t[2]);
}

namespace vp3d_internal {

Lens lensOf(const vp3d_view *v)
{
    Lens l;
    const float half = (float)(VP_DVG_COORD_MAX + 1) / 2.0f;
    l.fx = half / std::tan(v->fov_deg * 3.14159265f / 360.0f);
    // The monitor stretches the square board space to its shape: vertical
    // board units are physically shorter on a 4:3 tube, so more of them.
    l.fy = l.fx * (v->aspect > 0.0f ? v->aspect : 4.0f / 3.0f);
    l.cx = half;
    l.cy = half;
    return l;
}

float fogFactor(const vp3d_view *v, float z)
{
    if (v->fog_end <= v->fog_start) return 1.0f;
    if (z <= v->fog_start) return 1.0f;
    float floor = v->fog_floor < 0.0f ? 0.0f : (v->fog_floor > 1.0f ? 1.0f : v->fog_floor);
    if (z >= v->fog_end) return floor;
    float t = (z - v->fog_start) / (v->fog_end - v->fog_start);
    return 1.0f - t * (1.0f - floor);
}

// Liang-Barsky against the board square, in float (projected points can be
// far off the board; converting those to int first would overflow).
static bool clip2d(float &x0, float &y0, float &x1, float &y1)
{
    const float lo = 0.0f, hi = (float)VP_DVG_COORD_MAX;
    float t0 = 0.0f, t1 = 1.0f, dx = x1 - x0, dy = y1 - y0;
    const float p[4] = { -dx, dx, -dy, dy };
    const float q[4] = { x0 - lo, hi - x0, y0 - lo, hi - y0 };
    for (int i = 0; i < 4; i++) {
        if (p[i] == 0.0f) {
            if (q[i] < 0.0f) return false;
        } else {
            float r = q[i] / p[i];
            if (p[i] < 0.0f) { if (r > t1) return false; if (r > t0) t0 = r; }
            else             { if (r < t0) return false; if (r < t1) t1 = r; }
        }
    }
    float nx0 = x0 + t0 * dx, ny0 = y0 + t0 * dy;
    x1 = x0 + t1 * dx; y1 = y0 + t1 * dy;
    x0 = nx0; y0 = ny0;
    return true;
}

void drawCameraSegment(vp_dvg *d, const vp3d_view *v, const Lens &l, vp_vec3 a, vp_vec3 b,
                       uint8_t r, uint8_t g, uint8_t bl)
{
    const float n = v->near_z > 0.0f ? v->near_z : 0.1f;
    if (a.z < n && b.z < n) return;
    if (a.z > v->far_z && b.z > v->far_z) return;
    // Cut at the near plane.
    if (a.z < n) { float t = (n - a.z) / (b.z - a.z); a = vp_v3_add(a, vp_v3_scale(vp_v3_sub(b, a), t)); a.z = n; }
    if (b.z < n) { float t = (n - b.z) / (a.z - b.z); b = vp_v3_add(b, vp_v3_scale(vp_v3_sub(a, b), t)); b.z = n; }

    // Fog: a line whose ends differ in brightness is drawn in steps (about a
    // twelfth of full brightness each), so a long line fades along its length.
    float fa = fogFactor(v, a.z), fb = fogFactor(v, b.z);
    int steps = (int)std::ceil(std::fabs(fa - fb) * 12.0f);
    if (steps < 1) steps = 1;
    if (steps > 12) steps = 12;
    vp_vec3 ab = vp_v3_sub(b, a);
    for (int i = 0; i < steps; i++) {
        vp_vec3 pa = vp_v3_add(a, vp_v3_scale(ab, (float)i / steps));
        vp_vec3 pb = i + 1 == steps ? b : vp_v3_add(a, vp_v3_scale(ab, (float)(i + 1) / steps));
        float x0 = l.cx + pa.x / pa.z * l.fx, y0 = l.cy + pa.y / pa.z * l.fy;
        float x1 = l.cx + pb.x / pb.z * l.fx, y1 = l.cy + pb.y / pb.z * l.fy;
        if (!clip2d(x0, y0, x1, y1)) continue;

        float f = fogFactor(v, (pa.z + pb.z) * 0.5f);
        if (f < 1.0f) vp_color(d, (uint8_t)(r * f + 0.5f), (uint8_t)(g * f + 0.5f), (uint8_t)(bl * f + 0.5f));
        else          vp_color(d, r, g, bl);
        // Consecutive segments that share an end chain on the board by themselves:
        // a moveto to where the beam already is costs nothing.
        vp_moveto(d, (int)std::lround(x0), (int)std::lround(y0));
        vp_lineto(d, (int)std::lround(x1), (int)std::lround(y1));
    }
}

}  // namespace vp3d_internal

using namespace vp3d_internal;

int vp3d_project(const vp3d_view *v, vp_vec3 world, float *sx, float *sy)
{
    vp_vec3 c = vp_xf_point(&v->world_to_camera, world);
    if (c.z < (v->near_z > 0.0f ? v->near_z : 0.1f)) return 0;
    Lens l = lensOf(v);
    if (sx) *sx = l.cx + c.x / c.z * l.fx;
    if (sy) *sy = l.cy + c.y / c.z * l.fy;
    return 1;
}

void vp3d_line(vp_dvg *d, const vp3d_view *v, vp_vec3 a, vp_vec3 b)
{
    uint8_t r, g, bl;
    vp_get_color(d, &r, &g, &bl);
    Lens l = lensOf(v);
    drawCameraSegment(d, v, l, vp_xf_point(&v->world_to_camera, a),
                      vp_xf_point(&v->world_to_camera, b), r, g, bl);
    vp_color(d, r, g, bl);
}

// ---------------------------------------------------------------------------
// Meshes
// ---------------------------------------------------------------------------

int vp3d_mesh_create(vp3d_mesh **out)
{
    if (!out) return vp_fail(VP_ERR_INVALID, "vp3d_mesh_create: NULL argument");
    *out = new (std::nothrow) vp3d_mesh;
    return *out ? VP_OK : vp_fail(VP_ERR_NOMEM, "out of memory");
}

int vp3d_mesh_vertex(vp3d_mesh *m, float x, float y, float z)
{
    if (!m || m->finished) return -1;
    m->verts.push_back(vp_v3(x, y, z));
    return (int)m->verts.size() - 1;
}

int vp3d_mesh_face(vp3d_mesh *m, const int *idx, int count)
{
    if (!m || m->finished || !idx || count < 3)
        return vp_fail(VP_ERR_INVALID, "vp3d_mesh_face: a face needs 3 or more vertices");
    for (int i = 0; i < count; i++)
        if (idx[i] < 0 || idx[i] >= (int)m->verts.size())
            return vp_fail(VP_ERR_INVALID, "vp3d_mesh_face: vertex %d does not exist", idx[i]);
    m->faces.push_back(std::vector<int>(idx, idx + count));
    m->faceColor.push_back(-1);
    m->faceMaterial.push_back(-1);
    return VP_OK;
}

const char *vp3d_mesh_face_material(const vp3d_mesh *m, int face)
{
    if (!m || face < 0 || face >= (int)m->faces.size() || m->faceMaterial[face] < 0) return nullptr;
    return m->materials[m->faceMaterial[face]].c_str();
}

int vp3d_mesh_face_color(vp3d_mesh *m, int face, uint8_t r, uint8_t g, uint8_t b)
{
    if (!m || face < 0 || face >= (int)m->faces.size())
        return vp_fail(VP_ERR_INVALID, "vp3d_mesh_face_color: face %d does not exist", face);
    m->faceColor[face] = (int32_t)r << 16 | (int32_t)g << 8 | b;
    m->hasColors = true;
    return VP_OK;
}

int vp3d_mesh_line(vp3d_mesh *m, int a, int b)
{
    if (!m || m->finished) return vp_fail(VP_ERR_INVALID, "vp3d_mesh_line: mesh finished");
    int n = (int)m->verts.size();
    if (a < 0 || a >= n || b < 0 || b >= n || a == b)
        return vp_fail(VP_ERR_INVALID, "vp3d_mesh_line: bad vertices %d, %d", a, b);
    m->lines.push_back(std::make_pair(a, b));
    return VP_OK;
}

void vp3d_build_strokes(int nv, const std::vector<std::pair<int, int>> &edges,
                        const std::vector<int> &subset, std::vector<std::vector<int>> &out)
{
    // Scratch kept between calls: this runs every frame for solid meshes.
    static thread_local std::vector<std::vector<int>> adj;     // subset positions per vertex
    static thread_local std::vector<int> left;
    static thread_local std::vector<char> used;
    out.clear();
    if (subset.empty()) return;
    if ((int)adj.size() < nv) adj.resize(nv);
    left.assign(nv, 0);
    for (int i = 0; i < nv; i++) adj[i].clear();
    for (int k = 0; k < (int)subset.size(); k++) {
        const std::pair<int, int> &e = edges[subset[k]];
        adj[e.first].push_back(k);
        adj[e.second].push_back(k);
        left[e.first]++;
        left[e.second]++;
    }
    used.assign(subset.size(), 0);

    size_t remaining = subset.size();
    int scanOdd = 0, scanAny = 0;          // where the next start searches resume
    while (remaining > 0) {
        int start = -1;
        for (; scanOdd < nv && start < 0; scanOdd++)
            if (left[scanOdd] % 2 == 1) start = scanOdd;
        if (start >= 0) scanOdd = start;   // it may still be odd after this stroke
        for (; start < 0 && scanAny < nv; scanAny++)
            if (left[scanAny] > 0) start = scanAny;
        if (start < 0) break;              // cannot happen: remaining > 0

        std::vector<int> stroke(1, start);
        int at = start;
        for (;;) {
            int next = -1;
            for (int k : adj[at]) {
                if (used[k]) continue;
                used[k] = 1;
                remaining--;
                const std::pair<int, int> &e = edges[subset[k]];
                next = e.first == at ? e.second : e.first;
                left[at]--;
                left[next]--;
                break;
            }
            if (next < 0) break;
            stroke.push_back(next);
            at = next;
        }
        out.push_back(std::move(stroke));
    }
}

void vp3d_update_creases(vp3d_mesh *m)
{
    const float c = std::cos(m->creaseDeg * 3.14159265f / 180.0f);
    m->edgeCrease.assign(m->edges.size(), 0);
    for (size_t e = 0; e < m->edges.size(); e++) {
        int f0 = m->edgeFace0[e], f1 = m->edgeFace1[e];
        // 0 degrees: every edge, also between faces in one plane (the
        // pieces of a flat ring).
        if (f0 < 0 || f1 < 0 || m->edgeMany[e] || m->creaseDeg <= 0.0f) { m->edgeCrease[e] = 1; continue; }
        // Faces meeting at a sharper angle than the crease angle keep the edge.
        m->edgeCrease[e] = vp_v3_dot(m->faceNormal[f0], m->faceNormal[f1]) < c;
    }
}

// Unique edges from the faces and lines; for each edge the faces on its two
// sides; face normals, turned to point out of the solid; then the wireframe
// strokes.
int vp3d_mesh_finish(vp3d_mesh *m)
{
    if (!m) return vp_fail(VP_ERR_INVALID, "vp3d_mesh_finish: NULL mesh");
    if (m->finished) return VP_OK;

    std::unordered_map<uint64_t, int> &edgeIndex = m->edgeLookup;
    auto addEdge = [&](int a, int b, int face) {
        if (a == b) return;
        int lo = a < b ? a : b, hi = a < b ? b : a;
        uint64_t key = vp3d_edge_key(lo, hi);
        auto it = edgeIndex.find(key);
        int e;
        if (it == edgeIndex.end()) {
            e = (int)m->edges.size();
            edgeIndex.emplace(key, e);
            m->edges.push_back(std::make_pair(lo, hi));
            m->edgeFace0.push_back(-1);
            m->edgeFace1.push_back(-1);
            m->edgeMany.push_back(0);
        } else {
            e = it->second;
        }
        if (face < 0) return;
        if (m->edgeFace0[e] < 0)      m->edgeFace0[e] = face;
        else if (m->edgeFace1[e] < 0) m->edgeFace1[e] = face;
        else                          m->edgeMany[e] = 1;
    };
    for (int fi = 0; fi < (int)m->faces.size(); fi++) {
        const std::vector<int> &f = m->faces[fi];
        for (size_t i = 0; i < f.size(); i++)
            addEdge(f[i], f[(i + 1) % f.size()], fi);
    }
    for (const std::pair<int, int> &l : m->lines)
        addEdge(l.first, l.second, -1);

    // Face normals (Newell's method: right for any planar polygon, and steady
    // for slightly bent ones) and centroids.
    vp_vec3 meshCenter = vp_v3(0, 0, 0);
    for (const vp_vec3 &p : m->verts) meshCenter = vp_v3_add(meshCenter, p);
    if (!m->verts.empty()) meshCenter = vp_v3_scale(meshCenter, 1.0f / m->verts.size());
    float volume = 0.0f;
    m->faceNormal.resize(m->faces.size());
    m->facePoint.resize(m->faces.size());
    for (size_t fi = 0; fi < m->faces.size(); fi++) {
        const std::vector<int> &f = m->faces[fi];
        vp_vec3 n = vp_v3(0, 0, 0), c = vp_v3(0, 0, 0);
        for (size_t i = 0; i < f.size(); i++) {
            const vp_vec3 &a = m->verts[f[i]], &b = m->verts[f[(i + 1) % f.size()]];
            n.x += (a.y - b.y) * (a.z + b.z);
            n.y += (a.z - b.z) * (a.x + b.x);
            n.z += (a.x - b.x) * (a.y + b.y);
            c = vp_v3_add(c, a);
        }
        c = vp_v3_scale(c, 1.0f / f.size());
        volume += vp_v3_dot(vp_v3_sub(c, meshCenter), n);     // signed: outward normals give > 0
        m->faceNormal[fi] = vp_v3_normalize(n);
        m->facePoint[fi] = c;
    }
    // Tools wind faces either way round: whichever it is, make the normals
    // point out of the solid (faces must at least be wound consistently).
    if (volume < 0.0f)
        for (vp_vec3 &n : m->faceNormal) n = vp_v3_scale(n, -1.0f);
    vp3d_update_creases(m);

    // Faces as triangle fans, for hiding what is behind them (faces are taken
    // to be convex, as exporters write them).
    m->triVerts.clear();
    m->triFace.clear();
    for (int fi = 0; fi < (int)m->faces.size(); fi++) {
        const std::vector<int> &f = m->faces[fi];
        for (size_t i = 1; i + 1 < f.size(); i++) {
            m->triVerts.push_back(f[0]);
            m->triVerts.push_back(f[i]);
            m->triVerts.push_back(f[i + 1]);
            m->triFace.push_back(fi);
        }
    }

    std::vector<int> all(m->edges.size());
    for (size_t i = 0; i < all.size(); i++) all[i] = (int)i;
    vp3d_build_strokes((int)m->verts.size(), m->edges, all, m->strokes);

    float r2 = 0.0f;
    for (const vp_vec3 &p : m->verts) {
        float d = vp_v3_dot(p, p);
        if (d > r2) r2 = d;
    }
    m->radius = std::sqrt(r2);
    m->finished = true;
    return VP_OK;
}

void vp3d_mesh_set_crease(vp3d_mesh *m, float degrees)
{
    if (!m) return;
    m->creaseDeg = degrees < 0.0f ? 0.0f : (degrees > 180.0f ? 180.0f : degrees);
    if (m->finished) vp3d_update_creases(m);
}

void vp3d_mesh_set_two_sided(vp3d_mesh *m, int two_sided)
{
    if (m) m->twoSided = two_sided != 0;
}

void vp3d_mesh_free(vp3d_mesh *m)
{
    delete m;
}

int vp3d_mesh_vertex_count(const vp3d_mesh *m) { return m ? (int)m->verts.size() : 0; }
int vp3d_mesh_face_count(const vp3d_mesh *m)   { return m ? (int)m->faces.size() : 0; }
int vp3d_mesh_edge_count(const vp3d_mesh *m)   { return m ? (int)m->edges.size() : 0; }
int vp3d_mesh_stroke_count(const vp3d_mesh *m) { return m ? (int)m->strokes.size() : 0; }
float vp3d_mesh_radius(const vp3d_mesh *m)     { return m ? m->radius : 0.0f; }

void vp3d_mesh_bounds(const vp3d_mesh *m, vp_vec3 *min, vp_vec3 *max)
{
    vp_vec3 lo = vp_v3(0, 0, 0), hi = vp_v3(0, 0, 0);
    if (m && !m->verts.empty()) {
        lo = hi = m->verts[0];
        for (const vp_vec3 &v : m->verts) {
            lo.x = std::fmin(lo.x, v.x); lo.y = std::fmin(lo.y, v.y); lo.z = std::fmin(lo.z, v.z);
            hi.x = std::fmax(hi.x, v.x); hi.y = std::fmax(hi.y, v.y); hi.z = std::fmax(hi.z, v.z);
        }
    }
    if (min) *min = lo;
    if (max) *max = hi;
}

void vp3d_visible(const vp3d_mesh *m, const vp_xf &toCam, vp3d_mode mode,
                  std::vector<char> &seen, std::vector<int> &show)
{
    // A face is seen when the eye is on its outer side. In the mesh's own
    // space: the eye moved back through the placement.
    vp_xf toModel = vp_xf_inverse(toCam);
    vp_vec3 eye = vp_v3(toModel.t[0], toModel.t[1], toModel.t[2]);
    seen.resize(m->faces.size());
    for (size_t f = 0; f < m->faces.size(); f++)
        seen[f] = m->twoSided || vp_v3_dot(m->faceNormal[f], vp_v3_sub(eye, m->facePoint[f])) > 0.0f;

    show.clear();
    for (size_t e = 0; e < m->edges.size(); e++) {
        if (mode != VP3D_SOLID || m->faces.empty()) { show.push_back((int)e); continue; }
        // An edge shows on the silhouette (one side seen, the other not), as
        // a crease between seen faces, on the rim of a seen face, or alone.
        int f0 = m->edgeFace0[e], f1 = m->edgeFace1[e];
        bool visible;
        if (f0 < 0)                    visible = true;                    // a lone line
        else if (m->edgeMany[e])       visible = true;                    // odd geometry: keep it
        else if (f1 < 0)               visible = seen[f0];                // a rim
        else if (seen[f0] != seen[f1]) visible = true;                    // silhouette
        else                           visible = seen[f0] && m->edgeCrease[e];
        if (visible) show.push_back((int)e);
    }
}

void vp3d_draw_mesh(vp_dvg *d, const vp3d_view *v, const vp3d_mesh *m, const vp_xf *place,
                    vp3d_mode mode)
{
    if (!d || !v || !m || !m->finished) return;
    vp_xf toCam = place ? vp_xf_mul2(v->world_to_camera, *place) : v->world_to_camera;

    // Whole mesh outside the view's depth range: nothing to do.
    vp_vec3 c = vp_v3(toCam.t[0], toCam.t[1], toCam.t[2]);
    float scale = 0.0f;
    for (int i = 0; i < 9; i++) scale = std::fmax(scale, std::fabs(toCam.m[i]));
    float reach = m->radius * scale * 1.7320508f;
    if (c.z + reach < v->near_z || c.z - reach > v->far_z) return;

    static thread_local std::vector<vp_vec3> cam;
    cam.resize(m->verts.size());
    for (size_t i = 0; i < m->verts.size(); i++)
        cam[i] = vp_xf_point(&toCam, m->verts[i]);

    const std::vector<std::vector<int>> *strokes = &m->strokes;
    static thread_local std::vector<std::vector<int>> visibleStrokes;
    static thread_local std::vector<char> seen;
    static thread_local std::vector<int> show;
    if ((mode == VP3D_SOLID || m->textured) && !m->faces.empty())
        vp3d_visible(m, toCam, mode, seen, show);
    if (mode == VP3D_SOLID && !m->faces.empty()) {
        vp3d_build_strokes((int)m->verts.size(), m->edges, show, visibleStrokes);
        strokes = &visibleStrokes;
    }

    uint8_t r, g, bl;
    vp_get_color(d, &r, &g, &bl);
    Lens l = lensOf(v);
    for (const std::vector<int> &s : *strokes)
        for (size_t i = 1; i < s.size(); i++) {
            int32_t c = -1;
            if (m->hasColors) {
                auto it = m->edgeLookup.find(vp3d_edge_key(s[i - 1], s[i]));
                c = vp3d_edge_color(m, it == m->edgeLookup.end() ? -1 : it->second);
            }
            if (c < 0) drawCameraSegment(d, v, l, cam[s[i - 1]], cam[s[i]], r, g, bl);
            else       drawCameraSegment(d, v, l, cam[s[i - 1]], cam[s[i]], c >> 16 & 0xff, c >> 8 & 0xff, c & 0xff);
        }
    // Its textures, on the faces seen (all of them in wireframe).
    if (m->textured) {
        struct Emit { vp_dvg *d; const vp3d_view *v; Lens l; uint8_t r, g, bl; } e = { d, v, l, r, g, bl };
        for (size_t f = 0; f < m->faces.size(); f++) {
            if (f >= m->faceTexture.size() || !m->faceTexture[f]) continue;
            if (mode == VP3D_SOLID && !seen[f]) continue;
            int32_t fc = m->faceColor[f];
            e.r = fc < 0 ? r : (uint8_t)(fc >> 16 & 0xff);
            e.g = fc < 0 ? g : (uint8_t)(fc >> 8 & 0xff);
            e.bl = fc < 0 ? bl : (uint8_t)(fc & 0xff);
            vp3d_texture_segments(m, (int)f, place, cam.data(),
                [](void *user, vp_vec3 a, vp_vec3 b, int32_t rgb, float dim) {
                    Emit &e = *(Emit *)user;
                    if (rgb >= 0) drawCameraSegment(e.d, e.v, e.l, a, b, rgb >> 16 & 0xff, rgb >> 8 & 0xff, rgb & 0xff);
                    else drawCameraSegment(e.d, e.v, e.l, a, b, (uint8_t)(e.r * dim), (uint8_t)(e.g * dim), (uint8_t)(e.bl * dim));
                }, &e);
        }
    }
    vp_color(d, r, g, bl);
}

void vp3d_draw_mesh_wire(vp_dvg *d, const vp3d_view *v, const vp3d_mesh *m, const vp_xf *place)
{
    vp3d_draw_mesh(d, v, m, place, VP3D_WIREFRAME);
}
