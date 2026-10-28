#!/usr/bin/env python3
"""make_comic_font.py: VP_FONT_COMIC's letters (src/text/vp_font_comic.inc)
from a TrueType file. From the SDK's folder:

    python3 tools/make-comic-font/make_comic_font.py \
        tools/make-comic-font/fonts/TitanOne-Regular.ttf src/text/vp_font_comic.inc

Each letter's outline is read from the font (its quadratic curves flattened
to lines, then simplified), and a second outline is made a little outside it:
the letter's border. Coordinates are in 1/1000 of the font's height (em),
y up, from the baseline and the letter's left side. No modules needed.
"""
import math
import struct
import sys

CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!?'-.:,+/()&*=<>#"
TOLERANCE = 7.0     # em/1000: how far the simplified outline may stray
BORDER = 85.0       # em/1000: the border's distance outside the letter (clear of it on the beam)


# ---- reading the font ------------------------------------------------------

class Font:
    def __init__(self, path):
        self.d = open(path, "rb").read()
        n = struct.unpack(">H", self.d[4:6])[0]
        self.tables = {}
        for i in range(n):
            tag, _, off, length = struct.unpack(">4sIII", self.d[12 + 16 * i:28 + 16 * i])
            self.tables[tag.decode("latin-1")] = (off, length)
        head = self.tables["head"][0]
        self.em = struct.unpack(">H", self.d[head + 18:head + 20])[0]
        self.long_loca = struct.unpack(">h", self.d[head + 50:head + 52])[0] == 1
        self.num_glyphs = struct.unpack(">H", self.d[self.tables["maxp"][0] + 4:self.tables["maxp"][0] + 6])[0]
        hhea = self.tables["hhea"][0]
        self.num_hmetrics = struct.unpack(">H", self.d[hhea + 34:hhea + 36])[0]
        self.cmap = self._cmap()

    def u16(self, o): return struct.unpack(">H", self.d[o:o + 2])[0]
    def i16(self, o): return struct.unpack(">h", self.d[o:o + 2])[0]

    def _cmap(self):
        base = self.tables["cmap"][0]
        n = self.u16(base + 2)
        for i in range(n):
            plat, enc, off = struct.unpack(">HHI", self.d[base + 4 + 8 * i:base + 12 + 8 * i])
            sub = base + off
            if self.u16(sub) == 4 and (plat, enc) in ((3, 1), (0, 3), (0, 4)):
                seg2 = self.u16(sub + 6)
                ends = sub + 14
                starts = ends + seg2 + 2
                deltas = starts + seg2
                ranges = deltas + seg2
                out = {}
                for s in range(seg2 // 2):
                    end, start = self.u16(ends + 2 * s), self.u16(starts + 2 * s)
                    delta, roff = self.i16(deltas + 2 * s), self.u16(ranges + 2 * s)
                    for c in range(start, end + 1):
                        if c == 0xffff:
                            continue
                        if roff == 0:
                            g = (c + delta) & 0xffff
                        else:
                            g = self.u16(ranges + 2 * s + roff + 2 * (c - start))
                            if g:
                                g = (g + delta) & 0xffff
                        out[c] = g
                return out
        raise SystemExit("no usable cmap")

    def advance(self, g):
        hmtx = self.tables["hmtx"][0]
        i = min(g, self.num_hmetrics - 1)
        return self.u16(hmtx + 4 * i)

    def glyph_range(self, g):
        loca = self.tables["loca"][0]
        if self.long_loca:
            a, b = struct.unpack(">II", self.d[loca + 4 * g:loca + 4 * g + 8])
        else:
            a, b = (2 * x for x in struct.unpack(">HH", self.d[loca + 2 * g:loca + 2 * g + 4]))
        return self.tables["glyf"][0] + a, b - a

    def contours(self, g, dx=0, dy=0):
        """The glyph's contours: lists of (x, y, on_curve)."""
        off, length = self.glyph_range(g)
        if length == 0:
            return []
        nc = self.i16(off)
        if nc >= 0:
            return self._simple(off, nc, dx, dy)
        out, p = [], off + 10
        while True:
            flags, gi = self.u16(p), self.u16(p + 2)
            p += 4
            if flags & 1:
                a, b = self.i16(p), self.i16(p + 2); p += 4
            else:
                a, b = struct.unpack(">bb", self.d[p:p + 2]); p += 2
            if flags & 8: p += 2
            elif flags & 0x40: p += 4
            elif flags & 0x80: p += 8
            out += self.contours(gi, dx + a, dy + b)      # offsets only (no scaled parts expected)
            if not flags & 0x20:
                break
        return out

    def _simple(self, off, nc, dx, dy):
        ends = [self.u16(off + 10 + 2 * i) for i in range(nc)]
        npts = ends[-1] + 1 if ends else 0
        p = off + 10 + 2 * nc
        p += 2 + self.u16(p)                                # instructions
        flags = []
        while len(flags) < npts:
            f = self.d[p]; p += 1
            flags.append(f)
            if f & 8:
                r = self.d[p]; p += 1
                flags += [f] * r
        xs, x = [], 0
        for f in flags:
            if f & 2:
                v = self.d[p]; p += 1
                x += v if f & 16 else -v
            elif not f & 16:
                x += self.i16(p); p += 2
            xs.append(x)
        ys, y = [], 0
        for f in flags:
            if f & 4:
                v = self.d[p]; p += 1
                y += v if f & 32 else -v
            elif not f & 32:
                y += self.i16(p); p += 2
            ys.append(y)
        out, start = [], 0
        for e in ends:
            out.append([(xs[i] + dx, ys[i] + dy, bool(flags[i] & 1)) for i in range(start, e + 1)])
            start = e + 1
        return out


# ---- curves to lines ------------------------------------------------------------

def flatten(contour):
    """A closed contour's points, its quadratic curves cut into lines."""
    pts = contour
    n = len(pts)
    if n == 0:
        return []
    # start on an on-curve point (or the middle of two off-curve ones)
    k = next((i for i, p in enumerate(pts) if p[2]), None)
    if k is None:
        a, b = pts[0], pts[1]
        pts = [((a[0] + b[0]) / 2, (a[1] + b[1]) / 2, True)] + pts[1:] + pts[:1]
        k = 0
    pts = pts[k:] + pts[:k]
    out = [(pts[0][0], pts[0][1])]
    cur = (pts[0][0], pts[0][1])
    i = 1
    seq = pts[1:] + [pts[0]]
    while i <= len(seq):
        p = seq[i - 1]
        if p[2]:
            cur = (p[0], p[1]); out.append(cur); i += 1
            continue
        ctrl = (p[0], p[1])
        nxt = seq[i] if i < len(seq) else seq[0]
        end = (nxt[0], nxt[1]) if nxt[2] else ((ctrl[0] + nxt[0]) / 2, (ctrl[1] + nxt[1]) / 2)
        length = math.dist(cur, ctrl) + math.dist(ctrl, end)
        steps = max(2, min(10, int(length / 40)))
        for s in range(1, steps + 1):
            t = s / steps
            x = (1 - t) ** 2 * cur[0] + 2 * (1 - t) * t * ctrl[0] + t * t * end[0]
            y = (1 - t) ** 2 * cur[1] + 2 * (1 - t) * t * ctrl[1] + t * t * end[1]
            out.append((x, y))
        cur = end
        i += 1 if not nxt[2] else 2
    if math.dist(out[0], out[-1]) < 1e-6:
        out.pop()
    return out


def simplify(pts, tol):
    """Douglas-Peucker on a closed polygon."""
    if len(pts) < 5:
        return pts
    # split at the point farthest from the first
    far = max(range(len(pts)), key=lambda i: math.dist(pts[0], pts[i]))
    def dp(seg):
        if len(seg) < 3:
            return seg
        a, b = seg[0], seg[-1]
        dx, dy = b[0] - a[0], b[1] - a[1]
        L = math.hypot(dx, dy) or 1e-9
        best, bi = -1, 0
        for i in range(1, len(seg) - 1):
            p = seg[i]
            d = abs((p[0] - a[0]) * dy - (p[1] - a[1]) * dx) / L
            if d > best:
                best, bi = d, i
        if best <= tol:
            return [a, b]
        return dp(seg[:bi + 1])[:-1] + dp(seg[bi:])
    first = dp(pts[:far + 1])
    second = dp(pts[far:] + [pts[0]])
    return first[:-1] + second[:-1]


def area(pts):
    return sum(pts[i][0] * pts[(i + 1) % len(pts)][1] - pts[(i + 1) % len(pts)][0] * pts[i][1]
               for i in range(len(pts))) / 2


def offset(pts, d):
    """Each corner moved d away from the ink (TrueType: ink on the right of
    the way round), mitered, the miter capped."""
    n = len(pts)
    out = []
    for i in range(n):
        p0, p1, p2 = pts[i - 1], pts[i], pts[(i + 1) % n]
        def left(a, b):
            dx, dy = b[0] - a[0], b[1] - a[1]
            L = math.hypot(dx, dy) or 1e-9
            return (-dy / L, dx / L)
        n1, n2 = left(p0, p1), left(p1, p2)
        mx, my = n1[0] + n2[0], n1[1] + n2[1]
        L = math.hypot(mx, my)
        if L < 1e-6:                                # a hairpin: straight out
            mx, my, L = n1[0], n1[1], 1.0
        # the miter's length: d / cos(half the turn), capped at 2.9 x d
        half = max(1e-3, math.sqrt(max(0.0, (1 + n1[0] * n2[0] + n1[1] * n2[1]) / 2)))
        k = min(d / half, d * 2.9)
        out.append((p1[0] + mx / L * k, p1[1] + my / L * k))
    return out


def triangulate(poly):
    """Ear clipping: a simple polygon (either way round) into triangles, as
    index triples. None if it does not come apart (it crosses itself)."""
    n = len(poly)
    idx = list(range(n))
    if area(poly) < 0:
        idx.reverse()                               # counter-clockwise from here
    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    def inside(p, a, b, c):
        return cross(a, b, p) >= 0 and cross(b, c, p) >= 0 and cross(c, a, p) >= 0
    tris, guard = [], 0
    while len(idx) > 3 and guard < 10000:
        guard += 1
        for k in range(len(idx)):
            i0, i1, i2 = idx[k - 1], idx[k], idx[(k + 1) % len(idx)]
            a, b, c = poly[i0], poly[i1], poly[i2]
            if cross(a, b, c) <= 1e-9:
                continue                            # a reflex corner: not an ear
            if any(inside(poly[j], a, b, c) for j in idx if j not in (i0, i1, i2)):
                continue
            tris.append((i0, i1, i2))
            idx.pop(k)
            break
        else:
            return None
    if len(idx) == 3:
        tris.append(tuple(idx))
    return tris


# ---- writing ---------------------------------------------------------------------

def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    f = Font(sys.argv[1])
    scale = 1000.0 / f.em
    glyphs = []
    for ch in CHARS:
        g = f.cmap.get(ord(ch))
        if g is None:
            raise SystemExit("no glyph for %r" % ch)
        inner, outer = [], []
        for c in f.contours(g):
            pts = [(x * scale, y * scale) for x, y in flatten(c)]
            pts = simplify(pts, TOLERANCE)
            if len(pts) < 3:
                continue
            inner.append(pts)
            o = offset(pts, BORDER)
            # a counter too small for the border closes up: leave it out
            if area(o) * area(pts) > 0 and abs(area(o)) > 400:
                outer.append(simplify(o, TOLERANCE))
        glyphs.append((ch, round(f.advance(g) * scale), inner, outer))

    # The letters' shapes as triangles, to hide what is behind them: each
    # outside border (the same way round as the biggest), counters filled.
    def shapes(inner, outer):
        if not outer:
            return []
        sign = 1 if area(max(outer, key=lambda c: abs(area(c)))) > 0 else -1
        return [c for c in outer if area(c) * sign > 0]

    out = open(sys.argv[2], "w")
    out.write("// vp_font_comic.inc: VP_FONT_COMIC, made by tools/make-comic-font/make_comic_font.py\n"
              "// from %s (SIL Open Font License: tools/make-comic-font/fonts/OFL.txt).\n"
              "// Do not edit: run the tool again.\n\n" % sys.argv[1].split("/")[-1])
    pts_all, contours, tris = [], [], []
    for ch, adv, inner, outer in glyphs:
        solid = shapes(inner, outer)
        starts = {}
        for kind, cs in ((0, inner), (1, outer)):
            for i, c in enumerate(cs):
                starts[(kind, i)] = len(pts_all)
                contours.append((ch, kind, len(pts_all), len(c)))
                pts_all += [(round(x), round(y)) for x, y in c]
        for i, c in enumerate(outer):
            if c not in solid:
                continue
            first, t = starts[(1, i)], triangulate(c)
            if t is None:
                # a border crossing itself (a tight inner corner): the letter's
                # own outline there instead, the one it was made from
                j = min(range(len(inner)), key=lambda j: abs(abs(area(inner[j])) - abs(area(c))))
                first, t = starts[(0, j)], triangulate(inner[j])
                if t is None:
                    print("warning: %r does not triangulate" % ch)
                    continue
            tris += [(ch, first + a, first + b, first + cc) for a, b, cc in t]
    out.write("static const short COMIC_PTS[][2] = {\n")
    for i in range(0, len(pts_all), 8):
        out.write("    " + " ".join("{%d,%d}," % p for p in pts_all[i:i + 8]) + "\n")
    out.write("};\n\n")
    out.write("/* a contour: its first point, its count, 0 the letter / 1 its border */\n")
    out.write("typedef struct { char ch; unsigned char border; unsigned short first, count; } ComicContour;\n")
    out.write("static const ComicContour COMIC_CONTOURS[] = {\n")
    for ch, kind, first, count in contours:
        out.write("    { %s, %d, %d, %d },\n" % (repr(ch) if ch != "'" else "'\\''", kind, first, count))
    out.write("};\n\n")
    out.write("/* the letters' shapes (their borders, counters filled) as triangles */\n")
    out.write("typedef struct { char ch; unsigned short a, b, c; } ComicTriangle;\n")
    out.write("static const ComicTriangle COMIC_TRIANGLES[] = {\n")
    for ch, a, b, c in tris:
        out.write("    { %s, %d, %d, %d },\n" % (repr(ch) if ch != "'" else "'\\''", a, b, c))
    out.write("};\n\n")
    out.write("typedef struct { char ch; short advance; } ComicGlyph;\n")
    out.write("static const ComicGlyph COMIC_GLYPHS[] = {\n")
    for ch, adv, inner, outer in glyphs:
        out.write("    { %s, %d },\n" % (repr(ch) if ch != "'" else "'\\''", adv))
    out.write("};\n")
    out.close()
    print("%d letters, %d contours, %d points, %d triangles" % (len(glyphs), len(contours), len(pts_all), len(tris)))


if __name__ == "__main__":
    main()
