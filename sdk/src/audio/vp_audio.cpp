// Sound effects and streamed music, mixed in float on SDL2's audio device.
//
// Threads:
//   game thread    play / stop / volume calls: change voices under the audio
//                  device lock
//   audio callback mixes the voices (SDL calls it with the device lock held)
//   feeder thread  reads streamed WAV or Ogg Vorbis files into each stream's
//                  ring buffer
//                  (single producer: feeder, single consumer: callback), and
//                  frees streams whose voice has ended
// The callback never touches a file or allocates.

#include "../core/vp_internal.h"

#include <SDL.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <new>
#include <thread>
#include <vector>

// Ogg Vorbis: stb_vorbis (Sean Barrett, public domain or MIT), compiled on its
// own in src/third_party/stb; here its declarations only.
#define STB_VORBIS_HEADER_ONLY
#include "../third_party/stb/stb_vorbis.c"

namespace {

const int   DEVICE_RATE      = 48000;
const int   DEVICE_FRAMES    = 1024;      // ~21 ms per callback at 48 kHz
const float CLICK_FREE_S     = 0.005f;    // every volume change takes at least this long
const uint32_t RING_FRAMES   = 1u << 16;  // per stream: ~1.4 s at 48 kHz
const int   FEED_CHUNK       = 4096;      // frames read from disk at a time
const int   FEED_PERIOD_MS   = 10;
const uint32_t RING_GUARD    = 4;         // frames the feeder leaves free behind the
                                          // reader (the resampler looks one back)

// ---------------------------------------------------------------------------
// WAV files
// ---------------------------------------------------------------------------

struct WavInfo {
    int      channels = 0;
    int      rate = 0;
    int      bits = 0;
    bool     isFloat = false;
    long     dataOffset = 0;
    uint32_t dataBytes = 0;
    int bytesPerFrame() const { return channels * bits / 8; }
};

uint32_t le32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

int readWavHeader(FILE *f, const char *path, WavInfo *w)
{
    uint8_t h[12];
    if (fread(h, 1, 12, f) != 12 || memcmp(h, "RIFF", 4) != 0 || memcmp(h + 8, "WAVE", 4) != 0)
        return vp_fail(VP_ERR_INVALID, "%s is not a WAV file", path);
    bool haveFmt = false;
    for (;;) {
        uint8_t c[8];
        if (fread(c, 1, 8, f) != 8)
            return vp_fail(VP_ERR_INVALID, "%s: no audio data in the file", path);
        uint32_t size = le32(c + 4);
        if (memcmp(c, "fmt ", 4) == 0) {
            uint8_t fmt[40] = {0};
            uint32_t n = size < sizeof(fmt) ? size : (uint32_t)sizeof(fmt);
            if (fread(fmt, 1, n, f) != n)
                return vp_fail(VP_ERR_INVALID, "%s: truncated format chunk", path);
            if (size > n) fseek(f, (long)(size - n), SEEK_CUR);
            uint16_t tag = le16(fmt);
            w->channels = le16(fmt + 2);
            w->rate = (int)le32(fmt + 4);
            w->bits = le16(fmt + 14);
            if (tag == 0xFFFE && size >= 26) tag = le16(fmt + 24);   // WAVE_FORMAT_EXTENSIBLE
            w->isFloat = (tag == 3);
            if (!(tag == 1 || tag == 3))
                return vp_fail(VP_ERR_INVALID, "%s: compressed WAV (format %u) not supported", path, tag);
            if (w->isFloat ? w->bits != 32 : !(w->bits == 8 || w->bits == 16 || w->bits == 24 || w->bits == 32))
                return vp_fail(VP_ERR_INVALID, "%s: %d-bit samples not supported", path, w->bits);
            if (w->channels < 1 || w->rate < 1000)
                return vp_fail(VP_ERR_INVALID, "%s: bad channel count or sample rate", path);
            haveFmt = true;
        } else if (memcmp(c, "data", 4) == 0) {
            if (!haveFmt)
                return vp_fail(VP_ERR_INVALID, "%s: audio data before its format", path);
            w->dataOffset = ftell(f);
            w->dataBytes = size;
            // Some tools write a bogus size (0xFFFFFFFF, or 2 GB for a 16 KB
            // file): never more than the file holds, or loading it allocates
            // gigabytes (bad_alloc on a 1 GB Pi).
            if (fseek(f, 0, SEEK_END) == 0) {
                long end = ftell(f);
                if (end > w->dataOffset && (uint64_t)(end - w->dataOffset) < size)
                    w->dataBytes = (uint32_t)(end - w->dataOffset);
                fseek(f, w->dataOffset, SEEK_SET);
            }
            return VP_OK;
        } else {
            fseek(f, (long)(size + (size & 1)), SEEK_CUR);   // chunks are word-aligned
        }
    }
}

// Convert `frames` frames of raw WAV data to stereo float.
void toStereoFloat(const WavInfo &w, const uint8_t *in, int frames, float *out)
{
    const int bps = w.bits / 8;
    for (int i = 0; i < frames; i++) {
        float ch[2] = {0, 0};
        for (int c = 0; c < 2 && c < w.channels; c++) {
            const uint8_t *s = in + (size_t)i * w.bytesPerFrame() + (size_t)c * bps;
            float v;
            if (w.isFloat) { float f; memcpy(&f, s, 4); v = f; }
            else if (w.bits == 8)  v = (s[0] - 128) / 128.0f;
            else if (w.bits == 16) v = (int16_t)le16(s) / 32768.0f;
            else if (w.bits == 24) v = (int32_t)(((uint32_t)s[0] << 8) | ((uint32_t)s[1] << 16) | ((uint32_t)s[2] << 24)) / 2147483648.0f;
            else                   v = (int32_t)le32(s) / 2147483648.0f;
            ch[c] = v;
        }
        if (w.channels == 1) ch[1] = ch[0];
        out[i * 2] = ch[0];
        out[i * 2 + 1] = ch[1];
    }
}

// An Ogg file starts with "OggS"; anything else is taken for a WAV.
bool isOgg(const char *path)
{
    char m[4] = { 0 };
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    size_t n = fread(m, 1, 4, f);
    fclose(f);
    return n == 4 && memcmp(m, "OggS", 4) == 0;
}

// ---------------------------------------------------------------------------
// Streams: a WAV or Ogg Vorbis file read ahead into a ring of stereo float
// frames
// ---------------------------------------------------------------------------

struct Stream {
    FILE    *file = nullptr;            // WAV
    stb_vorbis *ogg = nullptr;          // or Ogg Vorbis
    int         oggChannels = 2;        // its channels as decoded: 1 or 2
    WavInfo  wav;                       // WAV: its format; Ogg: rate only
    bool     loop = false;
    uint32_t bytesLeft = 0;             // in the data chunk (feeder only)
    std::vector<float> ring;            // RING_FRAMES x 2
    std::atomic<uint32_t> writeFrames{0};   // total frames written (feeder)
    std::atomic<uint32_t> readFrames{0};    // total frames consumed (callback)
    std::atomic<bool> eof{false};           // nothing more will be written
    bool     released = false;          // its voice is gone: free it (feeder)
};

void closeStream(Stream *s)
{
    if (s->file) fclose(s->file);
    if (s->ogg) stb_vorbis_close(s->ogg);
    delete s;
}

// Ogg Vorbis: decoded to float at the file's own channel count (stb_vorbis's
// float call does NOT fold mono to stereo: asked for 2 channels of a mono
// file it fills the second with silence), a mono file copied to both sides;
// beyond stereo, the first two channels. Looping by seeking back to the start.
void feedOgg(Stream *s)
{
    static thread_local std::vector<float> conv;
    bool restarted = false;
    for (;;) {
        uint32_t w = s->writeFrames.load(std::memory_order_relaxed);
        uint32_t r = s->readFrames.load(std::memory_order_acquire);
        uint32_t space = RING_FRAMES - RING_GUARD - (w - r);
        if (space < 2) return;
        uint32_t frames = space < (uint32_t)FEED_CHUNK ? space : (uint32_t)FEED_CHUNK;
        const int ch = s->oggChannels;
        conv.resize((size_t)frames * 2);
        int got = stb_vorbis_get_samples_float_interleaved(s->ogg, ch, conv.data(), (int)frames * ch);
        if (got <= 0) {
            // the end: again from the start, unless that gives nothing either
            if (!s->loop || restarted || !stb_vorbis_seek_start(s->ogg)) {
                s->eof.store(true, std::memory_order_release);
                return;
            }
            restarted = true;
            continue;
        }
        restarted = false;
        for (int i = 0; i < got; i++) {
            uint32_t at = ((w + (uint32_t)i) & (RING_FRAMES - 1)) * 2;
            s->ring[at] = conv[i * ch];
            s->ring[at + 1] = conv[i * ch + ch - 1];
        }
        s->writeFrames.store(w + (uint32_t)got, std::memory_order_release);
    }
}

// Read as much as fits into the ring. Feeder thread (and once at start, before
// the stream is visible to the callback).
void feed(Stream *s)
{
    if (s->eof.load(std::memory_order_relaxed)) return;
    if (s->ogg) { feedOgg(s); return; }
    static thread_local std::vector<uint8_t> raw;
    static thread_local std::vector<float> conv;
    const int bpf = s->wav.bytesPerFrame();
    for (;;) {
        uint32_t w = s->writeFrames.load(std::memory_order_relaxed);
        uint32_t r = s->readFrames.load(std::memory_order_acquire);
        uint32_t space = RING_FRAMES - RING_GUARD - (w - r);
        if (space < 2) return;
        if (s->bytesLeft < (uint32_t)bpf) {
            if (!s->loop) { s->eof.store(true, std::memory_order_release); return; }
            fseek(s->file, s->wav.dataOffset, SEEK_SET);
            s->bytesLeft = s->wav.dataBytes;
            if (s->bytesLeft < (uint32_t)bpf) { s->eof.store(true, std::memory_order_release); return; }
        }
        uint32_t frames = space;
        if (frames > (uint32_t)FEED_CHUNK) frames = FEED_CHUNK;
        if (frames > s->bytesLeft / bpf) frames = s->bytesLeft / bpf;
        raw.resize((size_t)frames * bpf);
        size_t got = fread(raw.data(), 1, raw.size(), s->file);
        uint32_t gotFrames = (uint32_t)(got / bpf);
        if (gotFrames == 0) { s->bytesLeft = 0; continue; }   // short file: treat as its end
        s->bytesLeft -= gotFrames * bpf;
        conv.resize((size_t)gotFrames * 2);
        toStereoFloat(s->wav, raw.data(), (int)gotFrames, conv.data());
        for (uint32_t i = 0; i < gotFrames; i++) {
            uint32_t at = ((w + i) & (RING_FRAMES - 1)) * 2;
            s->ring[at] = conv[i * 2];
            s->ring[at + 1] = conv[i * 2 + 1];
        }
        s->writeFrames.store(w + gotFrames, std::memory_order_release);
    }
}

// ---------------------------------------------------------------------------
// Voices
// ---------------------------------------------------------------------------

struct Voice {
    bool      active = false;
    uint32_t  generation = 0;
    vp_sound *sound = nullptr;          // an effect, or
    Stream   *stream = nullptr;         // a stream
    double    pos = 0;                  // effect: frame position; stream: fraction past readFrames
    float     pitch = 1;
    double    step = 1;                 // source frames per device frame
    bool      loop = false;
    float     vol = 0, target = 0, volStep = 0;
    bool      stopAtTarget = false;     // ends when the volume reaches 0
    bool      finished = false;         // stream voice ended: the feeder detaches it
};

}  // namespace

struct vp_sound {
    std::vector<float> frames;          // stereo float
    int rate = 0;
    uint32_t count() const { return (uint32_t)(frames.size() / 2); }
};

struct vp_audio {
    SDL_AudioDeviceID dev = 0;
    int    rate = DEVICE_RATE;
    float  master = 1.0f;
    Voice  voices[VP_AUDIO_VOICES];
    uint32_t nextGeneration = 1;
    uint32_t refused = 0;
    std::atomic<uint32_t> underruns{0};
    std::vector<vp_sound *> sounds;     // all loaded, freed at close

    std::mutex streamsMutex;            // the streams list (game thread, feeder)
    std::vector<Stream *> streams;
    std::thread feeder;
    std::atomic<bool> stopFeeder{false};
};

namespace {

vp_voice handleOf(int slot, uint32_t generation)
{
    return (generation << 8) | (uint32_t)(slot + 1);
}

Voice *voiceOf(vp_audio *a, vp_voice v)
{
    int slot = (int)(v & 0xff) - 1;
    if (slot < 0 || slot >= VP_AUDIO_VOICES) return nullptr;
    Voice &vc = a->voices[slot];
    return (vc.active && vc.generation == (v >> 8)) ? &vc : nullptr;
}

void setTarget(vp_audio *a, Voice &v, float target, float seconds)
{
    if (seconds < CLICK_FREE_S) seconds = CLICK_FREE_S;
    v.target = target;
    v.volStep = std::fabs(target - v.vol) / (seconds * a->rate);
    if (v.volStep <= 0) v.volStep = 1e-6f;
}

// A free voice slot, or -1. Under the device lock.
int freeSlot(vp_audio *a)
{
    for (int i = 0; i < VP_AUDIO_VOICES; i++)
        if (!a->voices[i].active) return i;
    return -1;
}

void endVoice(Voice &v)
{
    if (v.stream) {
        v.finished = true;              // the feeder detaches and frees the stream
        v.vol = v.target = 0;
    } else {
        v.active = false;
        v.sound = nullptr;
    }
}

// ---------------------------------------------------------------------------
// The mixer (SDL's audio thread, device lock held)
// ---------------------------------------------------------------------------

// Resampling: 4-point cubic (Catmull-Rom) between y1 and y2, t in 0..1.
// Straight-line interpolation dulled the highs (-1.5 dB at 10 kHz, 44.1 to
// 48 kHz) and added a harsh edge; this keeps them.
inline float cubic(float y0, float y1, float y2, float y3, float t)
{
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * t + c2) * t + c1) * t + y1;
}

void SDLCALL mix(void *userdata, Uint8 *stream, int len)
{
    vp_audio *a = static_cast<vp_audio *>(userdata);
    float *out = reinterpret_cast<float *>(stream);
    const int frames = len / (int)(2 * sizeof(float));
    memset(out, 0, (size_t)len);

    for (Voice &v : a->voices) {
        if (!v.active || v.finished) continue;
        for (int i = 0; i < frames; i++) {
            // Volume ramp
            if (v.vol != v.target) {
                if (v.vol < v.target) { v.vol += v.volStep; if (v.vol > v.target) v.vol = v.target; }
                else                  { v.vol -= v.volStep; if (v.vol < v.target) v.vol = v.target; }
            }
            if (v.stopAtTarget && v.vol <= 0.0f) { endVoice(v); break; }

            float l, r;
            if (v.sound) {
                const vp_sound *s = v.sound;
                uint32_t n = s->count();
                uint32_t i0 = (uint32_t)v.pos;
                if (i0 >= n) {
                    if (!v.loop) { endVoice(v); break; }
                    v.pos -= n * std::floor(v.pos / n);
                    i0 = (uint32_t)v.pos;
                }
                // the frames around the position (wrapping when looped,
                // held at the ends when not)
                uint32_t im = i0 > 0 ? i0 - 1 : (v.loop ? n - 1 : 0);
                uint32_t i1 = i0 + 1 < n ? i0 + 1 : (v.loop ? (i0 + 1) % n : i0);
                uint32_t i2 = i0 + 2 < n ? i0 + 2 : (v.loop ? (i0 + 2) % n : n - 1);
                float t = (float)(v.pos - i0);
                const float *f = s->frames.data();
                l = cubic(f[im * 2], f[i0 * 2], f[i1 * 2], f[i2 * 2], t);
                r = cubic(f[im * 2 + 1], f[i0 * 2 + 1], f[i1 * 2 + 1], f[i2 * 2 + 1], t);
                v.pos += v.step;
            } else {
                Stream *s = v.stream;
                uint32_t rd = s->readFrames.load(std::memory_order_relaxed);
                uint32_t avail = s->writeFrames.load(std::memory_order_acquire) - rd;
                if (avail < 3) {
                    if (s->eof.load(std::memory_order_acquire)) { endVoice(v); break; }
                    a->underruns.fetch_add(1, std::memory_order_relaxed);
                    break;      // silence for the rest of this buffer
                }
                // one frame back (still in the ring: the feeder keeps a
                // guard), the frame, and two ahead
                uint32_t im = ((rd - 1) & (RING_FRAMES - 1)) * 2;
                uint32_t i0 = (rd & (RING_FRAMES - 1)) * 2;
                uint32_t i1 = ((rd + 1) & (RING_FRAMES - 1)) * 2;
                uint32_t i2 = ((rd + 2) & (RING_FRAMES - 1)) * 2;
                float t = (float)v.pos;
                const float *f = s->ring.data();
                l = cubic(f[im], f[i0], f[i1], f[i2], t);
                r = cubic(f[im + 1], f[i0 + 1], f[i1 + 1], f[i2 + 1], t);
                v.pos += v.step;
                uint32_t whole = (uint32_t)v.pos;
                if (whole) {
                    if (whole > avail - 2) whole = avail - 2;
                    v.pos -= whole;
                    s->readFrames.store(rd + whole, std::memory_order_release);
                }
            }
            out[i * 2]     += l * v.vol;
            out[i * 2 + 1] += r * v.vol;
        }
    }

    // The master volume, then clipped at full scale: keep the voices'
    // volumes such that they do not add up past it (clipping is heard as a
    // pop or a burst of noise).
    for (int i = 0; i < frames; i++) {
        float l = out[i * 2] * a->master, r = out[i * 2 + 1] * a->master;
        out[i * 2]     = l > 1.0f ? 1.0f : (l < -1.0f ? -1.0f : l);
        out[i * 2 + 1] = r > 1.0f ? 1.0f : (r < -1.0f ? -1.0f : r);
    }
}

// ---------------------------------------------------------------------------
// The feeder thread
// ---------------------------------------------------------------------------

void feederLoop(vp_audio *a)
{
    while (!a->stopFeeder.load()) {
        // Detach the streams whose voice has ended, under the device lock so
        // the mixer is not using them.
        std::vector<Stream *> done;
        SDL_LockAudioDevice(a->dev);
        for (Voice &v : a->voices) {
            if (v.active && v.stream && v.finished) {
                done.push_back(v.stream);
                v.stream = nullptr;
                v.finished = false;
                v.active = false;
            }
        }
        SDL_UnlockAudioDevice(a->dev);

        {
            std::lock_guard<std::mutex> lock(a->streamsMutex);
            for (Stream *s : done) s->released = true;
            for (size_t i = 0; i < a->streams.size();) {
                Stream *s = a->streams[i];
                if (s->released) {
                    closeStream(s);
                    a->streams.erase(a->streams.begin() + (long)i);
                } else {
                    feed(s);
                    i++;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(FEED_PERIOD_MS));
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

int vp_audio_open(vp_audio **out)
{
    if (!out) return vp_fail(VP_ERR_INVALID, "vp_audio_open: NULL argument");
    *out = nullptr;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
        return vp_fail(VP_ERR_NODEV, "SDL audio init failed: %s", SDL_GetError());

    vp_audio *a = new (std::nothrow) vp_audio;
    if (!a) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return vp_fail(VP_ERR_NOMEM, "out of memory");
    }

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = DEVICE_RATE;
    want.format = AUDIO_F32SYS;
    want.channels = 2;
    want.samples = DEVICE_FRAMES;
    want.callback = mix;
    want.userdata = a;
    a->dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (a->dev == 0) {
        int r = vp_fail(VP_ERR_NODEV, "no audio device: %s", SDL_GetError());
        delete a;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return r;
    }
    a->rate = have.freq;
    a->feeder = std::thread(feederLoop, a);
    SDL_PauseAudioDevice(a->dev, 0);
    *out = a;
    return VP_OK;
}

void vp_audio_close(vp_audio *a)
{
    if (!a) return;
    SDL_CloseAudioDevice(a->dev);           // the mixer stops
    a->stopFeeder.store(true);
    if (a->feeder.joinable()) a->feeder.join();
    for (Stream *s : a->streams) closeStream(s);
    for (vp_sound *s : a->sounds) delete s;
    delete a;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

int vp_sound_load(vp_audio *a, const char *path, vp_sound **out)
{
    if (!out) return vp_fail(VP_ERR_INVALID, "vp_sound_load: NULL argument");
    *out = nullptr;
    if (!a || !path) return vp_fail(VP_ERR_INVALID, "vp_sound_load: no audio or no path");

    if (isOgg(path)) {                  // decoded whole, to stereo float
        int channels = 0, rate = 0;
        short *pcm = nullptr;
        int frames = stb_vorbis_decode_filename(path, &channels, &rate, &pcm);
        if (frames < 1 || channels < 1 || !pcm) {
            free(pcm);
            return vp_fail(VP_ERR_INVALID, "%s: not a readable Ogg Vorbis file", path);
        }
        vp_sound *s = new (std::nothrow) vp_sound;
        if (!s) { free(pcm); return vp_fail(VP_ERR_NOMEM, "out of memory"); }
        s->rate = rate;
        s->frames.resize((size_t)frames * 2);
        for (int i = 0; i < frames; i++) {
            s->frames[i * 2]     = pcm[i * channels] / 32768.0f;
            s->frames[i * 2 + 1] = pcm[i * channels + (channels > 1 ? 1 : 0)] / 32768.0f;
        }
        free(pcm);
        a->sounds.push_back(s);
        *out = s;
        return VP_OK;
    }

    FILE *f = fopen(path, "rb");
    if (!f) return vp_fail(VP_ERR_NOTFOUND, "cannot open %s", path);
    WavInfo w;
    int r = readWavHeader(f, path, &w);
    if (r != VP_OK) { fclose(f); return r; }

    std::vector<uint8_t> raw(w.dataBytes);
    size_t got = fread(raw.data(), 1, raw.size(), f);
    fclose(f);
    int frames = (int)(got / w.bytesPerFrame());
    if (frames < 1) return vp_fail(VP_ERR_INVALID, "%s holds no audio", path);

    vp_sound *s = new (std::nothrow) vp_sound;
    if (!s) return vp_fail(VP_ERR_NOMEM, "out of memory");
    s->rate = w.rate;
    s->frames.resize((size_t)frames * 2);
    toStereoFloat(w, raw.data(), frames, s->frames.data());
    a->sounds.push_back(s);
    *out = s;
    return VP_OK;
}

int vp_sound_from_samples(vp_audio *a, const float *samples, int frames, int channels,
                          int sample_rate, vp_sound **out)
{
    if (!out) return vp_fail(VP_ERR_INVALID, "vp_sound_from_samples: NULL argument");
    *out = nullptr;
    if (!a || !samples || frames < 1 || (channels != 1 && channels != 2) || sample_rate < 1000)
        return vp_fail(VP_ERR_INVALID, "vp_sound_from_samples: bad arguments");
    vp_sound *s = new (std::nothrow) vp_sound;
    if (!s) return vp_fail(VP_ERR_NOMEM, "out of memory");
    s->rate = sample_rate;
    s->frames.resize((size_t)frames * 2);
    for (int i = 0; i < frames; i++) {
        s->frames[i * 2]     = samples[i * channels];
        s->frames[i * 2 + 1] = samples[i * channels + (channels - 1)];
    }
    a->sounds.push_back(s);
    *out = s;
    return VP_OK;
}

void vp_sound_free(vp_audio *a, vp_sound *s)
{
    if (!a || !s) return;
    SDL_LockAudioDevice(a->dev);
    for (Voice &v : a->voices)
        if (v.active && v.sound == s) { v.active = false; v.sound = nullptr; }
    SDL_UnlockAudioDevice(a->dev);
    for (size_t i = 0; i < a->sounds.size(); i++)
        if (a->sounds[i] == s) { a->sounds.erase(a->sounds.begin() + (long)i); break; }
    delete s;
}

float vp_sound_seconds(const vp_sound *s)
{
    return s && s->rate ? (float)s->count() / s->rate : 0.0f;
}

vp_voice vp_sound_play(vp_audio *a, vp_sound *s, float volume, float pitch, int loop)
{
    if (!a || !s) return 0;
    if (pitch <= 0.0f) pitch = 1.0f;
    vp_voice h = 0;
    SDL_LockAudioDevice(a->dev);
    int slot = freeSlot(a);
    if (slot < 0) {
        a->refused++;
    } else {
        Voice &v = a->voices[slot];
        v = Voice();
        v.active = true;
        v.generation = a->nextGeneration++ & 0xffffff;
        if (v.generation == 0) v.generation = a->nextGeneration++ & 0xffffff;
        v.sound = s;
        v.pitch = pitch;
        v.step = (double)s->rate / a->rate * pitch;
        v.loop = loop != 0;
        v.vol = 0.0f;             // up from silence in CLICK_FREE_S: a file that
        setTarget(a, v, volume, 0.0f);   // starts mid-waveform would pop
        h = handleOf(slot, v.generation);
    }
    SDL_UnlockAudioDevice(a->dev);
    return h;
}

vp_voice vp_music_play(vp_audio *a, const char *path, float volume, int loop, float fade_in_s)
{
    if (!a || !path) return 0;
    Stream *s = new (std::nothrow) Stream;
    if (!s) { vp_fail(VP_ERR_NOMEM, "out of memory"); return 0; }
    if (isOgg(path)) {
        int err = 0;
        s->ogg = stb_vorbis_open_filename(path, &err, nullptr);
        if (!s->ogg) {
            delete s;
            vp_fail(VP_ERR_INVALID, "%s: not a readable Ogg Vorbis file (error %d)", path, err);
            return 0;
        }
        stb_vorbis_info info = stb_vorbis_get_info(s->ogg);
        s->wav.rate = (int)info.sample_rate;
        s->wav.channels = 2;
        s->oggChannels = info.channels == 1 ? 1 : 2;
    } else {
        FILE *f = fopen(path, "rb");
        if (!f) { delete s; vp_fail(VP_ERR_NOTFOUND, "cannot open %s", path); return 0; }
        s->file = f;
        if (readWavHeader(f, path, &s->wav) != VP_OK) { closeStream(s); return 0; }
        s->bytesLeft = s->wav.dataBytes;
    }
    s->loop = loop != 0;
    s->ring.assign((size_t)RING_FRAMES * 2, 0.0f);
    feed(s);    // fill the ring now, so it starts at once

    vp_voice h = 0;
    {
        std::lock_guard<std::mutex> lock(a->streamsMutex);
        SDL_LockAudioDevice(a->dev);
        int slot = freeSlot(a);
        if (slot < 0) {
            a->refused++;
        } else {
            Voice &v = a->voices[slot];
            v = Voice();
            v.active = true;
            v.generation = a->nextGeneration++ & 0xffffff;
            if (v.generation == 0) v.generation = a->nextGeneration++ & 0xffffff;
            v.stream = s;
            v.pitch = 1.0f;
            v.step = (double)s->wav.rate / a->rate;
            v.loop = s->loop;
            if (fade_in_s > 0) { v.vol = 0; setTarget(a, v, volume, fade_in_s); }
            else               { v.vol = v.target = volume; }
            a->streams.push_back(s);
            h = handleOf(slot, v.generation);
        }
        SDL_UnlockAudioDevice(a->dev);
    }
    if (!h) closeStream(s);
    return h;
}

void vp_voice_stop(vp_audio *a, vp_voice h, float fade_out_s)
{
    if (!a) return;
    SDL_LockAudioDevice(a->dev);
    Voice *v = voiceOf(a, h);
    if (v && !v->finished) {
        setTarget(a, *v, 0.0f, fade_out_s);
        v->stopAtTarget = true;
    }
    SDL_UnlockAudioDevice(a->dev);
}

void vp_voice_set_volume(vp_audio *a, vp_voice h, float volume, float fade_s)
{
    if (!a) return;
    SDL_LockAudioDevice(a->dev);
    Voice *v = voiceOf(a, h);
    if (v && !v->stopAtTarget) setTarget(a, *v, volume, fade_s);
    SDL_UnlockAudioDevice(a->dev);
}

void vp_voice_set_pitch(vp_audio *a, vp_voice h, float pitch)
{
    if (!a || pitch <= 0.0f) return;
    SDL_LockAudioDevice(a->dev);
    Voice *v = voiceOf(a, h);
    if (v) {
        int rate = v->sound ? v->sound->rate : (v->stream ? v->stream->wav.rate : a->rate);
        v->pitch = pitch;
        v->step = (double)rate / a->rate * pitch;
    }
    SDL_UnlockAudioDevice(a->dev);
}

int vp_voice_playing(vp_audio *a, vp_voice h)
{
    if (!a) return 0;
    SDL_LockAudioDevice(a->dev);
    Voice *v = voiceOf(a, h);
    int playing = v && !v->finished;
    SDL_UnlockAudioDevice(a->dev);
    return playing;
}

void vp_audio_stop_all(vp_audio *a, float fade_out_s)
{
    if (!a) return;
    SDL_LockAudioDevice(a->dev);
    for (Voice &v : a->voices) {
        if (v.active && !v.finished) {
            setTarget(a, v, 0.0f, fade_out_s);
            v.stopAtTarget = true;
        }
    }
    SDL_UnlockAudioDevice(a->dev);
}

void vp_audio_set_master(vp_audio *a, float volume)
{
    if (!a) return;
    SDL_LockAudioDevice(a->dev);
    a->master = volume < 0 ? 0 : volume;
    SDL_UnlockAudioDevice(a->dev);
}

void vp_audio_get_stats(vp_audio *a, vp_audio_stats *st)
{
    if (!st) return;
    memset(st, 0, sizeof(*st));
    if (!a) return;
    SDL_LockAudioDevice(a->dev);
    for (const Voice &v : a->voices)
        if (v.active && !v.finished) st->voices++;
    st->refused = a->refused;
    SDL_UnlockAudioDevice(a->dev);
    st->underruns = a->underruns.load();
    st->sample_rate = a->rate;
}
