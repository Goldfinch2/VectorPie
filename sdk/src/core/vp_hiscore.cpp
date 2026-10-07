// The high-score table: save/highscores.txt, in the format the VectorPie
// menu reads ("AAA 12345" lines, best first).

#include "vp_internal.h"
#include <vectorpie/vp_hiscore.h>

#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace {

// Upper case, 1..3 characters the menu's "%3s" reads back whole.
void cleanName(const char *in, char out[4])
{
    if (!in) in = "";
    size_t len = strlen(in);
    while (len > 0 && isspace((unsigned char)in[len - 1])) len--;   // padding
    int n = 0;
    for (size_t i = 0; i < len && n < 3; i++) {
        int c = toupper((unsigned char)in[i]);
        if (!(isalnum(c) || c == '.' || c == '-')) c = '.';
        out[n++] = (char)c;
    }
    if (n == 0) { strcpy(out, "---"); return; }
    out[n] = '\0';
}

int loadTable(vp_score out[VP_HISCORE_MAX])
{
    FILE *f = fopen(vp_hiscore_path(), "r");
    if (!f) return 0;
    int n = 0;
    char line[128];
    while (n < VP_HISCORE_MAX && fgets(line, sizeof line, f)) {
        char name[4];
        int score;
        if (sscanf(line, "%3s %d", name, &score) != 2 || score <= 0) continue;
        cleanName(name, out[n].name);
        out[n].score = score;
        n++;
    }
    fclose(f);
    // Best first, whatever wrote the file (stable: equal scores keep their order).
    for (int i = 1; i < n; i++)
        for (int j = i; j > 0 && out[j].score > out[j - 1].score; j--) {
            vp_score t = out[j]; out[j] = out[j - 1]; out[j - 1] = t;
        }
    return n;
}

int rankIn(const vp_score *t, int n, int score)
{
    if (score <= 0) return -1;
    for (int i = 0; i < n; i++)
        if (score > t[i].score) return i;
    return n < VP_HISCORE_MAX ? n : -1;
}

// Write to a temporary file, flush it to the disk, rename it over the table.
int saveTable(const vp_score *t, int n)
{
    const char *path = vp_hiscore_path();
    char tmp[1300];                  // the path (up to 1199) + ".new"
    snprintf(tmp, sizeof tmp, "%s.new", path);
    FILE *f = fopen(tmp, "w");
    if (!f) return vp_fail(VP_ERR_IO, "cannot write %s: %s", tmp, strerror(errno));
    bool ok = true;
    for (int i = 0; i < n; i++)
        if (fprintf(f, "%s %d\n", t[i].name, t[i].score) < 0) ok = false;
    if (fflush(f) != 0) ok = false;
#ifndef _WIN32
    if (ok && fsync(fileno(f)) != 0) ok = false;
#endif
    if (fclose(f) != 0) ok = false;
    if (!ok) {
        remove(tmp);
        return vp_fail(VP_ERR_IO, "cannot write %s", tmp);
    }
#ifdef _WIN32
    if (!MoveFileExA(tmp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        remove(tmp);
        return vp_fail(VP_ERR_IO, "cannot replace %s", path);
    }
#else
    if (rename(tmp, path) != 0) {
        int e = errno;
        remove(tmp);
        return vp_fail(VP_ERR_IO, "cannot replace %s: %s", path, strerror(e));
    }
#endif
    return VP_OK;
}

}  // namespace

const char *vp_hiscore_path(void)
{
    static char path[1200];
    if (!path[0]) snprintf(path, sizeof path, "%s/highscores.txt", vp_save_dir());
    return path;
}

int vp_hiscore_load(vp_score out[VP_HISCORE_MAX])
{
    if (!out) return 0;
    return loadTable(out);
}

int vp_hiscore_rank(int score)
{
    vp_score t[VP_HISCORE_MAX];
    int n = loadTable(t);
    return rankIn(t, n, score);
}

int vp_hiscore_insert(const char *name, int score)
{
    vp_score t[VP_HISCORE_MAX];
    int n = loadTable(t);
    int r = rankIn(t, n, score);
    if (r < 0) return -1;
    if (n < VP_HISCORE_MAX) n++;
    for (int i = n - 1; i > r; i--) t[i] = t[i - 1];
    cleanName(name, t[r].name);
    t[r].score = score;
    int s = saveTable(t, n);
    return s == VP_OK ? r : s;
}

void vp_hiscore_draw(vp_dvg *d, const vp_score *table, int count,
                     float x, float y, float height, int highlight, double time)
{
    if (!d || !table) return;
    // Columns: the rank right-aligned, the name, the score right-aligned,
    // so the table stays straight whatever the widths.
    const float rankW = vp_text_width("10.", height);
    const float nameW = vp_text_width("WWW", height);
    const float gap = height;
    const float scoreW = vp_text_width("0000000", height);
    const float left = x - (rankW + gap + nameW + gap + scoreW) / 2;
    for (int i = 0; i < count && i < VP_HISCORE_MAX; i++) {
        if (i == highlight && std::fmod(time, 0.5) >= 0.35) continue;    // blinking
        float ly = y - i * height * 1.6f;
        char buf[16];
        snprintf(buf, sizeof buf, "%d.", i + 1);
        vp_text(d, buf, left + rankW, ly, height, VP_TEXT_RIGHT);
        vp_text(d, table[i].name, left + rankW + gap, ly, height, VP_TEXT_LEFT);
        snprintf(buf, sizeof buf, "%d", table[i].score);
        vp_text(d, buf, left + rankW + gap + nameW + gap + scoreW, ly, height, VP_TEXT_RIGHT);
    }
}
