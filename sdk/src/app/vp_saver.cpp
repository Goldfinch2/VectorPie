// Screensaver apps (vp_saver.h): the cabinet's music feed, or a stand-in for
// it on a PC (no menu): the SDK plays the music given and analyses it.

#include "../core/vp_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#define STB_VORBIS_HEADER_ONLY
#include "../third_party/stb/stb_vorbis.c"

namespace {

struct BeatMap {
    std::vector<float> times;
    std::vector<int> inBar;
    bool  hasTop = false;           // its "# top_db <dB>" line: the track's loudest
    float topDb = 0;
};

const float kStandInVolume = 0.8f;

// The stand-in feed: music from files, analysed here.
struct StandIn {
    bool started = false;
    std::vector<std::string> tracks;
    size_t index = 0;
    vp_voice voice = 0;
    uint64_t startedAt = 0, lastLook = 0;
    BeatMap beats;
    vp_music_analyzer an{};
    vp_music_feed feed{};
};

vp_audio  *g_audio = nullptr;
std::string g_musicArg;                     // the game's music argument (a file or a folder)
const volatile vp_music_feed *g_shared = nullptr;   // the menu's feed, mapped
vp_music_feed *g_sharedRw = nullptr;
bool g_triedShared = false;
StandIn g_standIn;

bool endsWith(const std::string &s, const char *tail)
{
    size_t n = strlen(tail);
    if (s.size() < n) return false;
    for (size_t i = 0; i < n; i++)
        if (tolower((unsigned char)s[s.size() - n + i]) != tail[i]) return false;
    return true;
}

bool isMusic(const std::string &p) { return endsWith(p, ".ogg") || endsWith(p, ".wav"); }

// A track's length in s; 0 unknown (a .wav).
float lengthOf(const std::string &track)
{
    if (!endsWith(track, ".ogg")) return 0;
    int err = 0;
    stb_vorbis *v = stb_vorbis_open_filename(track.c_str(), &err, nullptr);
    if (!v) return 0;
    float s = stb_vorbis_stream_length_in_seconds(v);
    stb_vorbis_close(v);
    return s;
}

// <track>.beats beside it: "<seconds>\t<beat in bar>" a line ('#': a
// comment), and "# top_db <dB>": the track's loudest (vp_music_feed.h).
BeatMap loadBeats(const std::string &track)
{
    BeatMap m;
    std::string path = track.substr(0, track.find_last_of('.')) + ".beats";
    std::ifstream f(path);
    if (!f) {
        f.open(track + ".beats");   // also <track.ogg>.beats
        if (!f) return m;
    }
    std::string line;
    while (std::getline(f, line)) {
        if (line.compare(0, 8, "# top_db") == 0)
            m.hasTop = sscanf(line.c_str() + 8, "%f", &m.topDb) == 1;
        if (line.empty() || line[0] == '#') continue;
        std::istringstream in(line);
        float t;
        int b = 0;
        if (!(in >> t)) continue;
        in >> b;
        m.times.push_back(t);
        m.inBar.push_back(b);
    }
    return m;
}

std::vector<std::string> findTracks(const std::string &arg)
{
    std::vector<std::string> out;
    if (arg.empty()) return out;
#ifndef _WIN32
    if (DIR *d = opendir(arg.c_str())) {
        while (dirent *e = readdir(d)) {
            std::string n = e->d_name;
            if (n[0] != '.' && isMusic(n)) out.push_back(arg + "/" + n);
        }
        closedir(d);
        std::sort(out.begin(), out.end());
        return out;
    }
#else
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((arg + "\\*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            std::string n = fd.cFileName;
            if (n[0] != '.' && isMusic(n)) out.push_back(arg + "\\" + n);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
        std::sort(out.begin(), out.end());
        if (!out.empty()) return out;
    }
#endif
    if (isMusic(arg)) out.push_back(arg);
    return out;
}

std::string titleOf(const std::string &path)
{
    size_t s = path.find_last_of("/\\");
    std::string t = path.substr(s == std::string::npos ? 0 : s + 1);
    t = t.substr(0, t.find_last_of('.'));
    std::replace(t.begin(), t.end(), '_', ' ');
    return t;
}

void standInPlay(StandIn &s, size_t index)
{
    if (s.voice) vp_voice_stop(g_audio, s.voice, 0.3f);
    s.index = index % s.tracks.size();
    s.voice = vp_music_play(g_audio, s.tracks[s.index].c_str(), kStandInVolume, 0, 0.0f);
    s.startedAt = vp_ticks_ms();
    s.beats = loadBeats(s.tracks[s.index]);
    if (s.beats.hasTop) vp_music_use_top(&s.an, s.beats.topDb + 20.0f * log10f(kStandInVolume), 1);
    else                vp_music_no_top(&s.an);
    s.feed.track++;
    s.feed.length = lengthOf(s.tracks[s.index]);
    vp_strlcpy(s.feed.title, titleOf(s.tracks[s.index]).c_str(), sizeof s.feed.title);
    vp_strlcpy(s.feed.next, titleOf(s.tracks[(s.index + 1) % s.tracks.size()]).c_str(), sizeof s.feed.next);
}

void standInStart(StandIn &s)
{
    s.started = true;
    s.feed.magic = VP_MUSIC_FEED_MAGIC;
    s.feed.version = VP_MUSIC_FEED_VERSION;
    s.feed.size = sizeof s.feed;
    std::string arg = g_musicArg;
    if (arg.empty() && getenv("VP_SAVER_MUSIC")) arg = getenv("VP_SAVER_MUSIC");
    s.tracks = findTracks(arg);
    s.lastLook = vp_ticks_ms();
    if (!s.tracks.empty() && g_audio) standInPlay(s, 0);
}

int standInMusic(vp_music_feed *out)
{
    StandIn &s = g_standIn;
    if (!s.started) standInStart(s);
    if (s.tracks.empty() || !g_audio) return 0;
    if (!vp_voice_playing(g_audio, s.voice)) standInPlay(s, s.index + 1);   // the next, in turn
    uint64_t now = vp_ticks_ms();
    float dt = (float)(now - s.lastLook) / 1000.0f;
    s.lastLook = now;
    float left[VP_MUSIC_FFT], right[VP_MUSIC_FFT];
    int rate = vp_audio_recent_stereo(g_audio, left, right, VP_MUSIC_FFT);
    if (rate > 0 && dt > 0) vp_music_analyze(&s.an, left, right, rate, dt);
    if (rate > 0) vp_music_set_wave(&s.feed, left, right, VP_MUSIC_FFT, rate);
    s.feed.kind = VP_MUSIC_TRACK;
    s.feed.position = (float)(now - s.startedAt) / 1000.0f;
    s.feed.heartbeat++;
    s.feed.level = s.an.level;
    memcpy(s.feed.bands, s.an.bands, sizeof s.feed.bands);
    s.feed.onsets = s.an.onsets;
    memcpy(s.feed.vu, s.an.vu, sizeof s.feed.vu);
    memcpy(s.feed.peak, s.an.peak, sizeof s.feed.peak);
    vp_music_beats_at(s.beats.times.data(), s.beats.inBar.data(), (int)s.beats.times.size(),
                      s.feed.position, &s.feed);
    *out = s.feed;
    return 1;
}

// The menu's feed, mapped once (it stays for the whole run).
void mapShared()
{
    g_triedShared = true;
#ifndef _WIN32
    const char *path = getenv("VP_MUSIC_FEED");
    if (!path || !*path) return;
    int fd = open(path, O_RDWR);
    if (fd < 0) return;
    void *p = mmap(nullptr, sizeof(vp_music_feed), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (p == MAP_FAILED) return;
    g_sharedRw = static_cast<vp_music_feed *>(p);
    g_shared = g_sharedRw;
#endif
}

}  // namespace

void vp_saver_attach(vp_audio *audio, const char *music_arg)
{
    g_audio = audio;
    g_musicArg = music_arg ? music_arg : "";
}

void vp_saver_detach(void)
{
    if (g_standIn.voice && g_audio) vp_voice_stop(g_audio, g_standIn.voice, 0.2f);
    g_standIn = StandIn();
    g_audio = nullptr;
}

int vp_saver_active(void)
{
    const char *s = getenv("VP_SAVER");
    return s && strcmp(s, "1") == 0;
}

int vp_saver_music(vp_music_feed *out)
{
    if (!out) return 0;
    if (!g_triedShared) mapShared();
    if (g_shared) {
        for (int tries = 0; tries < 100; tries++)
            if (vp_music_feed_read(g_shared, out)) return out->kind != VP_MUSIC_NONE;
        return 0;
    }
    return standInMusic(out);
}

void vp_saver_skip(void)
{
    if (!g_triedShared) mapShared();
    if (g_sharedRw) {
        g_sharedRw->skip_requests = g_sharedRw->skip_requests + 1;
        return;
    }
    StandIn &s = g_standIn;
    if (s.started && !s.tracks.empty() && g_audio) standInPlay(s, s.index + 1);
}
