// Textures: line drawings on a mesh's faces (vp_3d.h). A texture is a set of
// strokes in its square (u across, v up, 0..1), tiled over a face by the
// face's texture coordinates (its own, or world mapping), each stroke clipped
// to the face and handed out in camera space, on that face.

#include "vp_3d_internal.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

int vp3d_texture_create(vp3d_texture **out)
{
    if (!out) return vp_fail(VP_ERR_INVALID, "vp3d_texture_create: NULL argument");
    *out = new (std::nothrow) vp3d_texture;
    return *out ? VP_OK : vp_fail(VP_ERR_NOMEM, "out of memory");
}

void vp3d_texture_free(vp3d_texture *t)
{
    delete t;
}

int vp3d_texture_color(vp3d_texture *t, int32_t rgb)
{
    if (!t) return vp_fail(VP_ERR_INVALID, "vp3d_texture_color: NULL texture");
    t->color = rgb < 0 ? -1 : (rgb & 0xffffff);
    return VP_OK;
}

void vp3d_texture_set_dim(vp3d_texture *t, float dim)
{
    if (t) t->dim = dim < 0.0f ? 0.0f : (dim > 1.0f ? 1.0f : dim);
}

int vp3d_texture_line(vp3d_texture *t, float u0, float v0, float u1, float v1)
{
    if (!t) return vp_fail(VP_ERR_INVALID, "vp3d_texture_line: NULL texture");
    t->strokes.push_back({ u0, v0, u1, v1, t->color });
    return VP_OK;
}

int vp3d_texture_path(vp3d_texture *t, const float *uv, int count, int closed)
{
    if (!t || !uv || count < 2) return vp_fail(VP_ERR_INVALID, "vp3d_texture_path: 2 points or more");
    for (int i = 0; i + 1 < count; i++)
        t->strokes.push_back({ uv[i * 2], uv[i * 2 + 1], uv[i * 2 + 2], uv[i * 2 + 3], t->color });
    if (closed && count > 2)
        t->strokes.push_back({ uv[count * 2 - 2], uv[count * 2 - 1], uv[0], uv[1], t->color });
    return VP_OK;
}

int vp3d_texture_stroke_count(const vp3d_texture *t)
{
    return t ? (int)t->strokes.size() : 0;
}

// A texture file: one command a line, '#' starts a comment.
//   l u0 v0 u1 v1        a line
//   p u v u v ...        a path through the points
//   q u v u v ...        a closed shape (back to its first point)
//   c RRGGBB | c -       the following strokes' own color, or the face's
//   d 0.5                the face-colored strokes' brightness
int vp3d_texture_load(const char *path, vp3d_texture **out)
{
    if (!path || !out) return vp_fail(VP_ERR_INVALID, "vp3d_texture_load: NULL argument");
    FILE *f = fopen(path, "r");
    if (!f) return vp_fail(VP_ERR_NOTFOUND, "%s: cannot open", path);
    vp3d_texture *t;
    int r = vp3d_texture_create(&t);
    if (r != VP_OK) { fclose(f); return r; }
    char line[4096];
    int lineNo = 0;
    std::vector<float> pts;
    while (fgets(line, sizeof line, f)) {
        lineNo++;
        char *hash = strchr(line, '#');
        if (hash) *hash = 0;
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (!*p || *p == '\n' || *p == '\r') continue;
        char cmd = *p++;
        if (cmd == 'c') {
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '-') vp3d_texture_color(t, -1);
            else vp3d_texture_color(t, (int32_t)strtol(p, nullptr, 16));
            continue;
        }
        pts.clear();
        for (;;) {
            char *end;
            float v = strtof(p, &end);
            if (end == p) break;
            pts.push_back(v);
            p = end;
        }
        if (cmd == 'd' && pts.size() == 1) {
            vp3d_texture_set_dim(t, pts[0]);
        } else if (cmd == 'l' && pts.size() == 4) {
            vp3d_texture_line(t, pts[0], pts[1], pts[2], pts[3]);
        } else if ((cmd == 'p' || cmd == 'q') && pts.size() >= 4 && pts.size() % 2 == 0) {
            vp3d_texture_path(t, pts.data(), (int)pts.size() / 2, cmd == 'q');
        } else {
            vp3d_texture_free(t);
            fclose(f);
            return vp_fail(VP_ERR_INVALID, "%s:%d: not a texture command", path, lineNo);
        }
    }
    fclose(f);
    t->color = -1;
    *out = t;
    return VP_OK;
}

// ---------------------------------------------------------------------------
// On a mesh
// ---------------------------------------------------------------------------

int vp3d_mesh_face_uv(vp3d_mesh *m, int face, const float *uv)
{
    if (!m || !uv || face < 0 || face >= (int)m->faces.size())
        return vp_fail(VP_ERR_INVALID, "vp3d_mesh_face_uv: face %d does not exist", face);
    if (m->faceUV.size() < m->faces.size()) m->faceUV.resize(m->faces.size());
    m->faceUV[face].assign(uv, uv + m->faces[face].size() * 2);
    return VP_OK;
}

int vp3d_mesh_face_texture(vp3d_mesh *m, int face, const vp3d_texture *t)
{
    if (!m || face < -1 || face >= (int)m->faces.size())
        return vp_fail(VP_ERR_INVALID, "vp3d_mesh_face_texture: face %d does not exist", face);
    if (m->faceTexture.size() < m->faces.size()) m->faceTexture.resize(m->faces.size(), nullptr);
    if (face < 0) {
        for (const vp3d_texture *&ft : m->faceTexture) ft = t;
    } else {
        m->faceTexture[face] = t;
    }
    m->textured = false;
    for (const vp3d_texture *ft : m->faceTexture) m->textured = m->textured || ft;
    return VP_OK;
}

void vp3d_mesh_texture_world(vp3d_mesh *m, float tile)
{
    if (m) m->texWorld = tile > 0.0f ? tile : 0.0f;
}

namespace {

const int MAX_TILES = 1024;                 // per face: a pattern far too small for its face stops here

// Cyrus-Beck: the part of (a, b) inside the convex polygon (anticlockwise).
bool clipConvex(const std::vector<float> &poly, float &ax, float &ay, float &bx, float &by)
{
    float t0 = 0.0f, t1 = 1.0f, dx = bx - ax, dy = by - ay;
    const int n = (int)poly.size() / 2;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        float ex = poly[j * 2] - poly[i * 2], ey = poly[j * 2 + 1] - poly[i * 2 + 1];
        // inside: to the left of the edge
        float num = ex * (ay - poly[i * 2 + 1]) - ey * (ax - poly[i * 2]);
        float den = ex * dy - ey * dx;
        if (den == 0.0f) {
            if (num < 0.0f) return false;
        } else {
            float t = -num / den;
            if (den > 0.0f) { if (t > t0) t0 = t; }
            else            { if (t < t1) t1 = t; }
        }
        if (t0 >= t1) return false;
    }
    float x0 = ax + dx * t0, y0 = ay + dy * t0, x1 = ax + dx * t1, y1 = ay + dy * t1;
    ax = x0; ay = y0; bx = x1; by = y1;
    return true;
}

}  // namespace

void vp3d_texture_segments(const vp3d_mesh *m, int face, const vp_xf *place, const vp_vec3 *cam,
                           void (*emit)(void *user, vp_vec3 a, vp_vec3 b, int32_t rgb, float dim),
                           void *user)
{
    if (face >= (int)m->faceTexture.size()) return;
    const vp3d_texture *t = m->faceTexture[face];
    if (!t || t->strokes.empty()) return;
    const std::vector<int> &f = m->faces[face];
    const int n = (int)f.size();

    // Its texture coordinates: world mapping (the placed face, flat on its
    // main axis, tile units), else its own.
    static thread_local std::vector<float> uv;
    uv.assign((size_t)n * 2, 0.0f);
    if (m->texWorld > 0.0f) {
        vp_xf turn = place ? *place : vp_xf_identity();
        turn.t[0] = turn.t[1] = turn.t[2] = 0.0f;   // relative to the mesh's origin: it moves with it
        static thread_local std::vector<vp_vec3> q;
        q.resize((size_t)n);
        for (int i = 0; i < n; i++) q[i] = vp_xf_point(&turn, m->verts[f[i]]);
        vp_vec3 nrm = vp_v3_cross(vp_v3_sub(q[1], q[0]), vp_v3_sub(q[2], q[0]));
        float ax = std::fabs(nrm.x), ay = std::fabs(nrm.y), az = std::fabs(nrm.z);
        for (int i = 0; i < n; i++) {
            float u, v;
            if (ay >= ax && ay >= az) { u = q[i].x; v = q[i].z; }
            else if (ax >= az)        { u = q[i].z; v = q[i].y; }
            else                      { u = q[i].x; v = q[i].y; }
            uv[i * 2] = u / m->texWorld;
            uv[i * 2 + 1] = v / m->texWorld;
        }
    } else if (face < (int)m->faceUV.size() && m->faceUV[face].size() == (size_t)n * 2) {
        uv.assign(m->faceUV[face].begin(), m->faceUV[face].end());
    } else {
        return;                                     // no coordinates: nothing to lay it by
    }

    // The texture square onto the face: affine, from its first three corners.
    float du1 = uv[2] - uv[0], dv1 = uv[3] - uv[1], du2 = uv[4] - uv[0], dv2 = uv[5] - uv[1];
    float det = du1 * dv2 - du2 * dv1;
    if (std::fabs(det) < 1e-9f) return;
    const vp_vec3 c0 = cam[f[0]], e1 = vp_v3_sub(cam[f[1]], c0), e2 = vp_v3_sub(cam[f[2]], c0);
    const vp_vec3 A = vp_v3_scale(vp_v3_sub(vp_v3_scale(e1, dv2), vp_v3_scale(e2, dv1)), 1.0f / det);
    const vp_vec3 B = vp_v3_scale(vp_v3_sub(vp_v3_scale(e2, du1), vp_v3_scale(e1, du2)), 1.0f / det);
    auto toCam = [&](float u, float v) {
        return vp_v3_add(c0, vp_v3_add(vp_v3_scale(A, u - uv[0]), vp_v3_scale(B, v - uv[1])));
    };

    // The face's outline in texture space, anticlockwise.
    static thread_local std::vector<float> poly;
    poly.assign(uv.begin(), uv.end());
    float area = 0.0f;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        area += poly[i * 2] * poly[j * 2 + 1] - poly[j * 2] * poly[i * 2 + 1];
    }
    if (area < 0.0f)
        for (int i = 0, k = n - 1; i < k; i++, k--) {
            std::swap(poly[i * 2], poly[k * 2]);
            std::swap(poly[i * 2 + 1], poly[k * 2 + 1]);
        }
    float minU = poly[0], maxU = poly[0], minV = poly[1], maxV = poly[1];
    for (int i = 1; i < n; i++) {
        minU = std::fmin(minU, poly[i * 2]); maxU = std::fmax(maxU, poly[i * 2]);
        minV = std::fmin(minV, poly[i * 2 + 1]); maxV = std::fmax(maxV, poly[i * 2 + 1]);
    }

    // Tile by tile, each stroke clipped to the face.
    int tu0 = (int)std::floor(minU), tu1 = (int)std::ceil(maxU), tv0 = (int)std::floor(minV), tv1 = (int)std::ceil(maxV);
    if ((long)(tu1 - tu0) * (tv1 - tv0) > MAX_TILES) return;
    for (int tu = tu0; tu < tu1; tu++)
        for (int tv = tv0; tv < tv1; tv++)
            for (const vp3d_texture::Stroke &s : t->strokes) {
                float ax = s.u0 + tu, ay = s.v0 + tv, bx = s.u1 + tu, by = s.v1 + tv;
                if (std::fmax(ax, bx) < minU || std::fmin(ax, bx) > maxU ||
                    std::fmax(ay, by) < minV || std::fmin(ay, by) > maxV) continue;
                if (!clipConvex(poly, ax, ay, bx, by)) continue;
                emit(user, toCam(ax, ay), toCam(bx, by), s.rgb, t->dim);
            }
}
