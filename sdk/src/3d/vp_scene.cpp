// Scenes: lines cut by the solid faces in front of them.
//
// For every line segment (camera space, cut at the near plane) and every
// solid face triangle whose screen box it crosses:
//   1. the part of the segment that projects inside the triangle (clipped
//      in screen space, then taken back to the segment's own parameter:
//      perspective makes the two differ);
//   2. the part that lies on the far side of the triangle's plane, seen from
//      the eye;
// their overlap is hidden. What is left of [0, 1] is drawn. A screen grid
// keeps each segment to the triangles near it.

#include "vp_3d_internal.h"

#include <algorithm>
#include <cmath>
#include <new>

using namespace vp3d_internal;

namespace {

const int   GRID = 32;                               // cells per axis
const float CELL = (VP_DVG_COORD_MAX + 1) / (float)GRID;
const float DEPTH_EPS = 1e-3f;                       // relative: lines on a face are not behind it
const float MIN_PIECE = 1.5f;                        // board units: shorter pieces are dropped
const float FOLD_SIN = 0.0872f;                      // sin 5 degrees: see addCorners

struct Segment {
    vp_vec3 a, b;                                    // camera space
    uint8_t r, g, bl;
    int faceA, faceB;                                // global ids of the faces it lies on, or -1
    int va, vb;                                      // global ids of its ends (mesh vertices), or -1
};

struct Tri {
    float x[3], y[3];                                // screen, anticlockwise
    float minX, maxX, minY, maxY;
    vp_vec3 n;                                       // camera-space plane: n . p = k
    float k;
    int face;                                        // global face id
};

}  // namespace

struct vp3d_scene {
    vp_dvg *d = nullptr;
    vp3d_view view;
    Lens lens;
    std::vector<Segment> segs;
    std::vector<Tri> tris;
    int nextFace = 0;                                // global face ids, per scene
    int nextVertex = 0;                              // global vertex ids, per scene
    std::vector<int> cornerStart;                    // per mesh face (global id): its corners in corners
    std::vector<int> corners;                        // global vertex ids
    std::vector<std::vector<int>> grid;              // triangle indices per cell
    std::vector<uint32_t> stamp;                     // per triangle: last segment that looked at it
    vp3d_scene_stats stats;
};

namespace {

void project(const Lens &l, vp_vec3 p, float &x, float &y)
{
    x = l.cx + p.x / p.z * l.fx;
    y = l.cy + p.y / p.z * l.fy;
}

// Clip a camera-space polygon to z >= near (Sutherland-Hodgman).
void clipNear(const vp_vec3 *in, int n, float near, std::vector<vp_vec3> &out)
{
    out.clear();
    for (int i = 0; i < n; i++) {
        const vp_vec3 &a = in[i], &b = in[(i + 1) % n];
        bool ain = a.z >= near, bin = b.z >= near;
        if (ain) out.push_back(a);
        if (ain != bin) {
            float t = (near - a.z) / (b.z - a.z);
            vp_vec3 p = vp_v3_add(a, vp_v3_scale(vp_v3_sub(b, a), t));
            p.z = near;
            out.push_back(p);
        }
    }
}

void addTri(vp3d_scene *s, const vp_vec3 &a, const vp_vec3 &b, const vp_vec3 &c,
            vp_vec3 n, float k, int face)
{
    Tri t;
    const vp_vec3 *p[3] = { &a, &b, &c };
    for (int i = 0; i < 3; i++) project(s->lens, *p[i], t.x[i], t.y[i]);
    float area = (t.x[1] - t.x[0]) * (t.y[2] - t.y[0]) - (t.x[2] - t.x[0]) * (t.y[1] - t.y[0]);
    // Seen exactly edge-on: hides nothing. Only that: a detailed model's
    // small triangles (a kart down the road) still hide what is behind them.
    if (std::fabs(area) < 1e-4f) return;
    if (area < 0.0f) { std::swap(t.x[1], t.x[2]); std::swap(t.y[1], t.y[2]); }
    t.minX = std::min({ t.x[0], t.x[1], t.x[2] });
    t.maxX = std::max({ t.x[0], t.x[1], t.x[2] });
    t.minY = std::min({ t.y[0], t.y[1], t.y[2] });
    t.maxY = std::max({ t.y[0], t.y[1], t.y[2] });
    if (t.maxX < 0 || t.minX > VP_DVG_COORD_MAX || t.maxY < 0 || t.minY > VP_DVG_COORD_MAX) return;
    t.n = n;
    t.k = k;
    t.face = face;
    s->tris.push_back(t);
}

int cellOf(float v)
{
    int c = (int)(v / CELL);
    return c < 0 ? 0 : (c >= GRID ? GRID - 1 : c);
}

// The screen-parameter interval [s0, s1] of segment P0->P1 inside triangle t.
bool insideInterval(const Tri &t, float x0, float y0, float x1, float y1, float &s0, float &s1)
{
    s0 = 0.0f; s1 = 1.0f;
    float dx = x1 - x0, dy = y1 - y0;
    for (int i = 0; i < 3; i++) {
        int j = (i + 1) % 3;
        // inward normal of an anticlockwise edge
        float nx = -(t.y[j] - t.y[i]), ny = t.x[j] - t.x[i];
        float num = nx * (x0 - t.x[i]) + ny * (y0 - t.y[i]);
        float den = nx * dx + ny * dy;
        if (den == 0.0f) {
            if (num < 0.0f) return false;
        } else {
            float r = -num / den;
            if (den > 0.0f) { if (r > s0) s0 = r; }
            else            { if (r < s1) s1 = r; }
        }
        if (s0 >= s1) return false;
    }
    return true;
}

}  // namespace

int vp3d_scene_create(vp3d_scene **out)
{
    if (!out) return vp_fail(VP_ERR_INVALID, "vp3d_scene_create: NULL argument");
    *out = new (std::nothrow) vp3d_scene;
    if (!*out) return vp_fail(VP_ERR_NOMEM, "out of memory");
    (*out)->grid.resize(GRID * GRID);
    return VP_OK;
}

void vp3d_scene_free(vp3d_scene *s)
{
    delete s;
}

void vp3d_scene_begin(vp3d_scene *s, vp_dvg *d, const vp3d_view *v)
{
    if (!s || !v) return;
    s->d = d;
    s->view = *v;
    s->lens = lensOf(v);
    s->segs.clear();
    s->tris.clear();
    s->nextFace = 0;
    s->nextVertex = 0;
    s->cornerStart.clear();
    s->corners.clear();
    s->stats = vp3d_scene_stats();
}

void vp3d_scene_line(vp3d_scene *s, vp_vec3 a, vp_vec3 b)
{
    if (!s || !s->d) return;
    Segment seg;
    seg.a = vp_xf_point(&s->view.world_to_camera, a);
    seg.b = vp_xf_point(&s->view.world_to_camera, b);
    vp_get_color(s->d, &seg.r, &seg.g, &seg.bl);
    seg.faceA = seg.faceB = -1;
    seg.va = seg.vb = -1;
    s->segs.push_back(seg);
}

// A face's corners, by global face id. A line between two corners of a face
// lies on it (a diagonal of a quad not quite flat): that face never hides it.
// A line from one of its corners, running within FOLD_SIN of its plane, is a
// fold between neighboring faces (overlapping patches of a model, a quad
// split in two triangles the other way), not something behind it.
static void addCorners(vp3d_scene *s, int face, const int *verts, int count, int vertexBase)
{
    while ((int)s->cornerStart.size() < face) s->cornerStart.push_back((int)s->corners.size());
    s->cornerStart.push_back((int)s->corners.size());
    for (int i = 0; i < count; i++) s->corners.push_back(vertexBase + verts[i]);
}

static bool isCorner(const vp3d_scene *s, int face, int v)
{
    if (v < 0 || face < 0 || face >= (int)s->cornerStart.size()) return false;
    int end = face + 1 < (int)s->cornerStart.size() ? s->cornerStart[face + 1] : (int)s->corners.size();
    for (int i = s->cornerStart[face]; i < end; i++)
        if (s->corners[i] == v) return true;
    return false;
}

void vp3d_scene_mesh(vp3d_scene *s, const vp3d_mesh *m, const vp_xf *place, vp3d_mode mode)
{
    if (!s || !s->d || !m || !m->finished) return;
    const vp3d_view *v = &s->view;
    vp_xf toCam = place ? vp_xf_mul2(v->world_to_camera, *place) : v->world_to_camera;

    vp_vec3 c = vp_v3(toCam.t[0], toCam.t[1], toCam.t[2]);
    float scale = 0.0f;
    for (int i = 0; i < 9; i++) scale = std::fmax(scale, std::fabs(toCam.m[i]));
    float reach = m->radius * scale * 1.7320508f;
    if (c.z + reach < v->near_z || c.z - reach > v->far_z) return;

    const int faceBase = s->nextFace;
    s->nextFace += (int)m->faces.size();
    const int vertexBase = s->nextVertex;
    s->nextVertex += (int)m->verts.size();

    static thread_local std::vector<vp_vec3> cam;
    cam.resize(m->verts.size());
    for (size_t i = 0; i < m->verts.size(); i++) cam[i] = vp_xf_point(&toCam, m->verts[i]);

    static thread_local std::vector<char> seen;
    static thread_local std::vector<int> show;
    static thread_local std::vector<std::vector<int>> strokes;
    vp3d_visible(m, toCam, mode, seen, show);
    vp3d_build_strokes((int)m->verts.size(), m->edges, show, strokes);

    uint8_t r, g, bl;
    vp_get_color(s->d, &r, &g, &bl);
    for (const std::vector<int> &st : strokes) {
        for (size_t i = 1; i < st.size(); i++) {
            Segment seg;
            seg.a = cam[st[i - 1]];
            seg.b = cam[st[i]];
            auto it = m->edgeLookup.find(vp3d_edge_key(st[i - 1], st[i]));
            int e = it == m->edgeLookup.end() ? -1 : it->second;
            int32_t c = m->hasColors ? vp3d_edge_color(m, e) : -1;
            if (c < 0) { seg.r = r; seg.g = g; seg.bl = bl; }
            else       { seg.r = c >> 16 & 0xff; seg.g = c >> 8 & 0xff; seg.bl = c & 0xff; }
            seg.faceA = (e >= 0 && m->edgeFace0[e] >= 0) ? faceBase + m->edgeFace0[e] : -1;
            seg.faceB = (e >= 0 && m->edgeFace1[e] >= 0) ? faceBase + m->edgeFace1[e] : -1;
            seg.va = vertexBase + st[i - 1];
            seg.vb = vertexBase + st[i];
            s->segs.push_back(seg);
        }
    }

    if (mode != VP3D_SOLID || m->faces.empty()) return;
    for (size_t f = 0; f < m->faces.size(); f++)
        addCorners(s, faceBase + (int)f, m->faces[f].data(), (int)m->faces[f].size(), vertexBase);

    // The faces turned to the camera hide what is behind them.
    const float near = v->near_z > 0.0f ? v->near_z : 0.1f;
    static thread_local std::vector<vp_vec3> poly;
    for (size_t t = 0; t < m->triFace.size(); t++) {
        int f = m->triFace[t];
        if (!seen[f]) continue;
        vp_vec3 tri[3] = { cam[m->triVerts[t * 3]], cam[m->triVerts[t * 3 + 1]], cam[m->triVerts[t * 3 + 2]] };
        if (tri[0].z < near && tri[1].z < near && tri[2].z < near) continue;
        vp_vec3 n = vp_v3_normalize(vp_v3_cross(vp_v3_sub(tri[1], tri[0]), vp_v3_sub(tri[2], tri[0])));
        float k = vp_v3_dot(n, tri[0]);
        if (std::fabs(k) < 1e-6f) continue;          // its plane goes through the eye
        clipNear(tri, 3, near, poly);
        for (size_t i = 1; i + 1 < poly.size(); i++)
            addTri(s, poly[0], poly[i], poly[i + 1], n, k, faceBase + f);
    }
}

void vp3d_scene_polygon(vp3d_scene *s, const vp_vec3 *pts, int count, unsigned edge_mask)
{
    if (!s || !s->d || !pts || count < 3 || count > 16) return;
    const vp3d_view *v = &s->view;
    const int face = s->nextFace++;

    vp_vec3 cam[16];
    for (int i = 0; i < count; i++) cam[i] = vp_xf_point(&v->world_to_camera, pts[i]);

    uint8_t r, g, bl;
    vp_get_color(s->d, &r, &g, &bl);
    for (int i = 0; i < count; i++) {
        if (!(edge_mask & (1u << i))) continue;
        Segment seg;
        seg.a = cam[i];
        seg.b = cam[(i + 1) % count];
        seg.r = r; seg.g = g; seg.bl = bl;
        seg.faceA = face;
        seg.faceB = -1;
        seg.va = seg.vb = -1;
        s->segs.push_back(seg);
    }

    // It hides from both sides: no facing test.
    const float near = v->near_z > 0.0f ? v->near_z : 0.1f;
    vp_vec3 n = vp_v3_normalize(vp_v3_cross(vp_v3_sub(cam[1], cam[0]), vp_v3_sub(cam[2], cam[0])));
    float k = vp_v3_dot(n, cam[0]);
    if (std::fabs(k) < 1e-6f || vp_v3_length(n) == 0.0f) return;
    static thread_local std::vector<vp_vec3> poly;
    clipNear(cam, count, near, poly);
    for (size_t i = 1; i + 1 < poly.size(); i++)
        addTri(s, poly[0], poly[i], poly[i + 1], n, k, face);
}

void vp3d_scene_end(vp3d_scene *s)
{
    if (!s || !s->d) return;
    vp_dvg *d = s->d;
    const vp3d_view *v = &s->view;
    const float near = v->near_z > 0.0f ? v->near_z : 0.1f;

    // Bin the triangles by screen box.
    for (std::vector<int> &cell : s->grid) cell.clear();
    for (int i = 0; i < (int)s->tris.size(); i++) {
        const Tri &t = s->tris[i];
        for (int cy = cellOf(t.minY); cy <= cellOf(t.maxY); cy++)
            for (int cx = cellOf(t.minX); cx <= cellOf(t.maxX); cx++)
                s->grid[cy * GRID + cx].push_back(i);
    }
    s->stamp.assign(s->tris.size(), 0);
    s->stats.lines = (uint32_t)s->segs.size();
    s->stats.faces = (uint32_t)s->tris.size();

    uint8_t keepR, keepG, keepB;
    vp_get_color(d, &keepR, &keepG, &keepB);
    std::vector<std::pair<float, float>> hidden;
    uint32_t segNo = 0;

    for (const Segment &seg : s->segs) {
        segNo++;
        vp_vec3 a = seg.a, b = seg.b;
        if (a.z < near && b.z < near) continue;
        if (a.z > v->far_z && b.z > v->far_z) continue;
        if (a.z < near) { float t = (near - a.z) / (b.z - a.z); a = vp_v3_add(a, vp_v3_scale(vp_v3_sub(b, a), t)); a.z = near; }
        if (b.z < near) { float t = (near - b.z) / (a.z - b.z); b = vp_v3_add(b, vp_v3_scale(vp_v3_sub(a, b), t)); b.z = near; }

        float x0, y0, x1, y1;
        project(s->lens, a, x0, y0);
        project(s->lens, b, x1, y1);
        float minX = std::min(x0, x1), maxX = std::max(x0, x1);
        float minY = std::min(y0, y1), maxY = std::max(y0, y1);
        if (maxX < 0 || minX > VP_DVG_COORD_MAX || maxY < 0 || minY > VP_DVG_COORD_MAX) continue;

        hidden.clear();
        for (int cy = cellOf(minY); cy <= cellOf(maxY); cy++) {
            for (int cx = cellOf(minX); cx <= cellOf(maxX); cx++) {
                for (int ti : s->grid[cy * GRID + cx]) {
                    if (s->stamp[ti] == segNo) continue;
                    s->stamp[ti] = segNo;
                    const Tri &t = s->tris[ti];
                    if (t.face == seg.faceA || t.face == seg.faceB) continue;   // its own faces
                    const bool cornerA = isCorner(s, t.face, seg.va), cornerB = isCorner(s, t.face, seg.vb);
                    if (cornerA && cornerB) continue;                           // on that face
                    if (t.maxX < minX || t.minX > maxX || t.maxY < minY || t.minY > maxY) continue;

                    // 1. where it projects inside the triangle, as segment parameters
                    float s0, s1;
                    if (!insideInterval(t, x0, y0, x1, y1, s0, s1)) continue;
                    float u0 = s0 * a.z / (b.z - s0 * (b.z - a.z));
                    float u1 = s1 * a.z / (b.z - s1 * (b.z - a.z));

                    // 2. where it lies beyond the triangle's plane, seen from the eye
                    float side = t.k > 0.0f ? -1.0f : 1.0f;                // the eye's side is positive
                    float g0 = (vp_v3_dot(t.n, a) - t.k) * side;
                    float g1 = (vp_v3_dot(t.n, b) - t.k) * side;
                    if ((cornerA || cornerB) &&
                        std::max(std::fabs(g0), std::fabs(g1)) < FOLD_SIN * vp_v3_length(vp_v3_sub(b, a)))
                        continue;                                               // a fold at its corner
                    float eps = DEPTH_EPS * std::fabs(t.k);
                    float b0, b1;
                    if (g0 < -eps && g1 < -eps)      { b0 = 0.0f; b1 = 1.0f; }
                    else if (g0 >= -eps && g1 >= -eps) continue;
                    else {
                        float uc = (-eps - g0) / (g1 - g0);
                        if (g0 < -eps) { b0 = 0.0f; b1 = uc; }
                        else           { b0 = uc;   b1 = 1.0f; }
                    }
                    float lo = std::max(u0, b0), hi = std::min(u1, b1);
                    if (lo < hi) hidden.push_back(std::make_pair(lo, hi));
                }
            }
        }

        if (!hidden.empty()) s->stats.cut++;
        std::sort(hidden.begin(), hidden.end());
        // Draw [0, 1] minus the hidden intervals.
        float at = 0.0f;
        size_t h = 0;
        while (at < 1.0f) {
            float next = 1.0f, resume = 1.0f;
            while (h < hidden.size() && hidden[h].second <= at) h++;
            if (h < hidden.size() && hidden[h].first <= at) {
                // inside a hidden run: skip to its (merged) end
                resume = hidden[h].second;
                for (size_t j = h + 1; j < hidden.size() && hidden[j].first <= resume; j++)
                    resume = std::max(resume, hidden[j].second);
                at = resume;
                continue;
            }
            if (h < hidden.size()) next = hidden[h].first;
            vp_vec3 pa = vp_v3_add(a, vp_v3_scale(vp_v3_sub(b, a), at));
            vp_vec3 pb = vp_v3_add(a, vp_v3_scale(vp_v3_sub(b, a), next));
            float qx0, qy0, qx1, qy1;
            project(s->lens, pa, qx0, qy0);
            project(s->lens, pb, qx1, qy1);
            if (std::hypot(qx1 - qx0, qy1 - qy0) >= MIN_PIECE || hidden.empty()) {
                drawCameraSegment(d, v, s->lens, pa, pb, seg.r, seg.g, seg.bl);
                s->stats.pieces++;
            }
            at = next;
        }
    }
    vp_color(d, keepR, keepG, keepB);
}

void vp3d_scene_get_stats(const vp3d_scene *s, vp3d_scene_stats *stats)
{
    if (!stats) return;
    *stats = s ? s->stats : vp3d_scene_stats();
}
