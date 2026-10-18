// Wavefront OBJ loading, through the same builder as a mesh made in code.

#include "vp_3d_internal.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <tuple>

namespace {

// "12", "12/3", "12//7", "12/3/7"; negative counts back from the last vertex.
bool parseIndex(const char *tok, int vertexCount, int *out)
{
    char *end = nullptr;
    long i = strtol(tok, &end, 10);
    if (end == tok || i == 0) return false;
    long idx = i > 0 ? i - 1 : vertexCount + i;
    if (idx < 0 || idx >= vertexCount) return false;
    *out = (int)idx;
    return true;
}

// Whitespace-separated tokens, in place (strtok_r is not on every compiler).
char *nextToken(char *start)
{
    static thread_local char *rest = nullptr;
    char *p = start ? start : rest;
    if (!p) return nullptr;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (!*p) { rest = nullptr; return nullptr; }
    char *tok = p;
    while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') p++;
    if (*p) { *p = '\0'; rest = p + 1; }
    else    rest = nullptr;
    return tok;
}

// The rest of a line, trimmed: a name ("mtllib", "usemtl", "newmtl").
std::string restOf(const char *p)
{
    while (*p == ' ' || *p == '\t') p++;
    std::string s(p);
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '\t'))
        s.pop_back();
    return s;
}

// A material library: each material's diffuse color (Kd, 0..1) as 0xRRGGBB.
// A missing or unreadable file adds nothing (the faces keep the current color).
void loadMaterials(const std::string &path, std::map<std::string, int32_t> &out)
{
    FILE *f = fopen(path.c_str(), "r");
    if (!f) return;
    char buf[1024];
    std::string name;
    while (fgets(buf, sizeof(buf), f)) {
        char *p = buf;
        while (*p == ' ' || *p == '\t') p++;
        if (!strncmp(p, "newmtl", 6) && (p[6] == ' ' || p[6] == '\t')) {
            name = restOf(p + 6);
        } else if (p[0] == 'K' && p[1] == 'd' && (p[2] == ' ' || p[2] == '\t') && !name.empty()) {
            float c[3];
            if (sscanf(p + 3, "%f %f %f", &c[0], &c[1], &c[2]) != 3) continue;
            int32_t rgb = 0;
            for (float v : c) {
                int b = (int)std::lround((v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v)) * 255.0f);
                rgb = rgb << 8 | b;
            }
            out[name] = rgb;
        }
    }
    fclose(f);
}

}  // namespace

int vp3d_mesh_load_obj(const char *path, const vp3d_obj_options *opt, vp3d_mesh **out)
{
    if (!out) return vp_fail(VP_ERR_INVALID, "vp3d_mesh_load_obj: NULL argument");
    *out = nullptr;
    if (!path) return vp_fail(VP_ERR_INVALID, "vp3d_mesh_load_obj: no path");
    FILE *f = fopen(path, "r");
    if (!f) return vp_fail(VP_ERR_NOTFOUND, "cannot open %s", path);

    vp3d_mesh *m = nullptr;
    int r = vp3d_mesh_create(&m);
    if (r != VP_OK) { fclose(f); return r; }

    const float scale = (opt && opt->scale > 0.0f) ? opt->scale : 1.0f;
    const bool zUp = opt && opt->z_up;

    const bool materials = !(opt && opt->no_materials);
    std::string dir(path);                  // material libraries are next to the file
    size_t slash = dir.find_last_of("/\\");
    dir = slash == std::string::npos ? std::string() : dir.substr(0, slash + 1);
    std::map<std::string, int32_t> library;
    int32_t color = -1;                     // the current material's, or -1
    int material = -1;                      // the current material's index in m->materials

    char buf[4096];
    int lineNo = 0;
    std::vector<int> idx;
    // Files often repeat a vertex (seams, separate parts): identical
    // positions are merged, or the faces on either side would not share an
    // edge and solid drawing would show it.
    std::vector<int> remap;                 // file vertex -> mesh vertex
    std::map<std::tuple<float, float, float>, int> byPosition;
    while (fgets(buf, sizeof(buf), f)) {
        lineNo++;
        char *p = buf;
        while (*p == ' ' || *p == '\t') p++;
        if (p[0] == 'v' && (p[1] == ' ' || p[1] == '\t')) {
            float x, y, z;
            if (sscanf(p + 2, "%f %f %f", &x, &y, &z) != 3) {
                vp3d_mesh_free(m); fclose(f);
                return vp_fail(VP_ERR_INVALID, "%s:%d: a vertex needs x y z", path, lineNo);
            }
            if (zUp) { float t = y; y = z; z = -t; }
            // OBJ is right-handed, this world left-handed: flip z, or the
            // model would come out mirrored.
            std::tuple<float, float, float> key(x * scale, y * scale, -z * scale);
            auto it = byPosition.find(key);
            if (it != byPosition.end()) {
                remap.push_back(it->second);
            } else {
                int i = vp3d_mesh_vertex(m, std::get<0>(key), std::get<1>(key), std::get<2>(key));
                byPosition.emplace(key, i);
                remap.push_back(i);
            }
        } else if ((p[0] == 'f' || p[0] == 'l') && (p[1] == ' ' || p[1] == '\t')) {
            const bool face = p[0] == 'f';
            idx.clear();
            for (char *tok = nextToken(p + 2); tok; tok = nextToken(nullptr)) {
                int i;
                if (!parseIndex(tok, (int)remap.size(), &i)) {
                    vp3d_mesh_free(m); fclose(f);
                    return vp_fail(VP_ERR_INVALID, "%s:%d: bad vertex index \"%s\"", path, lineNo, tok);
                }
                idx.push_back(remap[i]);
            }
            // Merged vertices can leave a corner repeated: drop the repeats.
            std::vector<int> clean;
            for (int i : idx)
                if (clean.empty() || clean.back() != i) clean.push_back(i);
            if (face && clean.size() > 1 && clean.front() == clean.back()) clean.pop_back();
            idx.swap(clean);
            if (face) {
                if (idx.size() >= 3 && vp3d_mesh_face(m, idx.data(), (int)idx.size()) == VP_OK) {
                    m->faceMaterial.back() = material;
                    if (color >= 0)
                        vp3d_mesh_face_color(m, (int)m->faces.size() - 1, color >> 16 & 0xff, color >> 8 & 0xff, color & 0xff);
                }
            } else {
                for (size_t i = 1; i < idx.size(); i++)
                    if (idx[i - 1] != idx[i]) vp3d_mesh_line(m, idx[i - 1], idx[i]);
            }
        } else if (materials && !strncmp(p, "mtllib", 6) && (p[6] == ' ' || p[6] == '\t')) {
            std::string name = restOf(p + 6);
            if (!name.empty()) loadMaterials(dir + name, library);
        } else if (!strncmp(p, "usemtl", 6) && (p[6] == ' ' || p[6] == '\t')) {
            std::string name = restOf(p + 6);
            material = -1;
            for (size_t i = 0; i < m->materials.size() && material < 0; i++)
                if (m->materials[i] == name) material = (int)i;
            if (material < 0) { material = (int)m->materials.size(); m->materials.push_back(name); }
            auto it = library.find(name);
            color = it == library.end() ? -1 : it->second;
        }
        // Everything else (vt, vn, s, g, o, comments) is not needed.
    }
    fclose(f);

    if (m->verts.empty()) {
        vp3d_mesh_free(m);
        return vp_fail(VP_ERR_INVALID, "%s: no vertices", path);
    }
    if (opt && opt->center) {
        vp_vec3 lo = m->verts[0], hi = m->verts[0];
        for (const vp_vec3 &v : m->verts) {
            lo.x = std::fmin(lo.x, v.x); lo.y = std::fmin(lo.y, v.y); lo.z = std::fmin(lo.z, v.z);
            hi.x = std::fmax(hi.x, v.x); hi.y = std::fmax(hi.y, v.y); hi.z = std::fmax(hi.z, v.z);
        }
        vp_vec3 c = vp_v3_scale(vp_v3_add(lo, hi), 0.5f);
        for (vp_vec3 &v : m->verts) v = vp_v3_sub(v, c);
    }
    if (opt && opt->crease_deg > 0.0f) vp3d_mesh_set_crease(m, opt->crease_deg);
    r = vp3d_mesh_finish(m);
    if (r != VP_OK) { vp3d_mesh_free(m); return r; }
    *out = m;
    return VP_OK;
}
