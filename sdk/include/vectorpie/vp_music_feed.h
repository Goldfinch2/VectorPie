/* vp_music_feed.h — the cabinet's music feed: what the menu is playing,
 * shared with a screensaver app (vp_saver.h reads it for you).
 *
 * The menu keeps playing its music while a screensaver app draws, and
 * publishes, about 60 times a second, one vp_music_feed in shared memory (the
 * file VP_MUSIC_FEED names, /dev/shm/vectorpie-music): the track, its beats
 * (from the track's beat map, <track>.beats) and its sound (a spectrum, VU
 * meters and the waveform, all as heard). The app asks for the next track by
 * adding 1 to skip_requests.
 *
 * Writing: seq goes odd, the fields change, seq goes even again. Reading:
 * copy, and keep the copy only if seq was even and the same before and after
 * (vp_music_feed_read).
 *
 * The spectrum analysis is here too (vp_music_analyze), so the menu and the
 * SDK's stand-in feed (no menu: a game on a PC) measure sound the same way.
 * The bars' top is the track's own loudest, measured once over the whole
 * track (vp_music_track_top_db, kept in its beat map as "# top_db <dB>"),
 * else the loudest heard lately. The VU meters and peaks are measured
 * against the same top.
 *
 * Plain C, header only, no dependencies: the menu includes a copy of it.
 */
#ifndef VECTORPIE_VP_MUSIC_FEED_H
#define VECTORPIE_VP_MUSIC_FEED_H

#include <math.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VP_MUSIC_FEED_PATH    "/dev/shm/vectorpie-music"
#define VP_MUSIC_FEED_MAGIC   0x5650464du       /* 'VPFM' */
#define VP_MUSIC_FEED_VERSION 2
#define VP_MUSIC_BANDS        32
#define VP_MUSIC_TEXT         96
#define VP_MUSIC_WAVE         512             /* frames of waveform */

/* What is playing. */
enum { VP_MUSIC_NONE = 0, VP_MUSIC_TRACK, VP_MUSIC_CLIP, VP_MUSIC_THEME };

typedef struct vp_music_feed {
    uint32_t magic, version, size;
    volatile uint32_t seq;          /* odd while being written */
    uint32_t heartbeat;             /* +1 each write: the menu is alive */

    /* the track */
    int32_t  kind;                  /* VP_MUSIC_TRACK (the playlist), _CLIP (a DJ clip), _THEME, _NONE */
    uint32_t track;                 /* changes with each new track */
    float    position;              /* s into it, as heard */
    float    length;                /* s, its whole length; 0 unknown */
    char     title[VP_MUSIC_TEXT];  /* "" when it has none (the theme) */
    char     next[VP_MUSIC_TEXT];

    /* its beats: from its beat map (has_beats 0: none, the rest 0) */
    int32_t  has_beats;
    uint32_t beats;                 /* beats heard so far: +1 on each */
    int32_t  beat_in_bar;           /* of the last one: 1 the downbeat .. 4; 0 unknown */
    float    since_beat;            /* s since the last beat */
    float    to_beat;               /* s to the next */
    float    bpm;

    /* its sound */
    float    level;                 /* 0 .. 1 */
    float    bands[VP_MUSIC_BANDS]; /* 0 .. 1, low (40 Hz) to high (16 kHz), log spaced */
    uint32_t onsets;                /* +1 on each hit heard (drums, attacks): beats without a map */
    float    vu[2];                 /* left, right: VU meters in dB, 0 = the track's loud parts
                                       (a VU face reads -20 .. +3); as slow as a VU needle */
    float    peak[2];               /* left, right: the peaks in dB, 0 = the track's loudest;
                                       held a second, then falling */
    int32_t  wave_rate;             /* the waveform's sample rate (Hz); 0: no waveform */
    int16_t  wave[VP_MUSIC_WAVE][2];/* the last VP_MUSIC_WAVE frames heard, left and right,
                                       oldest first (-32768 .. 32767): a scope, an XY figure */

    /* the app's requests */
    volatile uint32_t skip_requests; /* the app adds 1: the menu plays the next track */
} vp_music_feed;

#if defined(_MSC_VER)
#include <intrin.h>
#define VP_MUSIC_FENCE() _ReadWriteBarrier()
#else
#define VP_MUSIC_FENCE() __sync_synchronize()
#endif

/* A consistent copy of *f into *out: 1, else 0 (being written: try again). */
static inline int vp_music_feed_read(const volatile vp_music_feed *f, vp_music_feed *out)
{
    uint32_t a = f->seq;
    if (a & 1) return 0;
    VP_MUSIC_FENCE();
    memcpy(out, (const void *)f, sizeof *out);
    VP_MUSIC_FENCE();
    return f->seq == a && out->magic == VP_MUSIC_FEED_MAGIC && out->version == VP_MUSIC_FEED_VERSION;
}

/* ---- the beats: where a position falls in a beat map ----------------------- */

/* From a beat map (n beats: times in s, ascending; in_bar 1..4, 0 unknown),
 * at `position` s: fills f's beats, beat_in_bar, since_beat, to_beat, bpm and
 * has_beats. */
static inline void vp_music_beats_at(const float *times, const int *in_bar, int n, float position,
                                     vp_music_feed *f)
{
    f->has_beats = n > 1;
    f->beats = 0; f->beat_in_bar = 0; f->since_beat = 0; f->to_beat = 0; f->bpm = 0;
    if (n < 2) return;
    int lo = 0, hi = n;                     /* the first beat after position */
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (times[mid] <= position) lo = mid + 1; else hi = mid;
    }
    f->beats = (uint32_t)lo;
    if (lo > 0) {
        f->beat_in_bar = in_bar ? in_bar[lo - 1] : 0;
        f->since_beat = position - times[lo - 1];
    }
    if (lo < n) f->to_beat = times[lo] - position;
    int a = lo > 0 ? lo - 1 : 0, b = a + 1 < n ? a + 1 : a;
    if (b > a && times[b] > times[a]) f->bpm = 60.0f / (times[b] - times[a]);
}

/* ---- the analysis: a spectrum and hits from the latest samples ------------- */

#define VP_MUSIC_FFT 1024           /* frames per look */

typedef struct vp_music_analyzer {
    float bands[VP_MUSIC_BANDS];    /* shown: rising at once, falling slowly */
    float level;
    float flux_avg;                 /* the usual change, for spotting a hit */
    float raw_prev[VP_MUSIC_BANDS];
    float top_db;                   /* the bars' top, in dB (0: not yet heard) */
    float track_top_db;             /* the track's loudest, as heard (vp_music_use_top) */
    int   has_track_top;            /* 0: none, the top follows the loudest lately */
    float since_onset;
    uint32_t onsets;
    float vu_pow[2];                /* VU: the mean square, as slow as a needle */
    float vu[2], peak[2];           /* in dB, as in vp_music_feed */
    float peak_hold[2];             /* s the peak stays */
} vp_music_analyzer;

/* In-place radix-2 FFT of n (a power of 2) complex values. */
static inline void vp_music_fft(float *re, float *im, int n)
{
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            float t = re[i]; re[i] = re[j]; re[j] = t;
            t = im[i]; im[i] = im[j]; im[j] = t;
        }
    }
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -6.28318530f / (float)len;
        float wr = cosf(ang), wi = sinf(ang);
        for (int i = 0; i < n; i += len) {
            float cr = 1, ci = 0;
            for (int k = 0; k < len / 2; k++) {
                int a = i + k, b = i + k + len / 2;
                float xr = re[b] * cr - im[b] * ci, xi = re[b] * ci + im[b] * cr;
                re[b] = re[a] - xr; im[b] = im[a] - xi;
                re[a] += xr; im[a] += xi;
                float t = cr * wr - ci * wi;
                ci = cr * wi + ci * wr;
                cr = t;
            }
        }
    }
}

/* The bands of VP_MUSIC_FFT mono samples (-1..1) at `rate` Hz, in dB (the
 * highs lifted: they are quieter) into db[VP_MUSIC_BANDS]; returns the
 * loudest, and their RMS in *rms. */
static inline float vp_music_spectrum_db(const float *mono, int rate, float *db, float *rms)
{
    float re[VP_MUSIC_FFT], im[VP_MUSIC_FFT];
    float sum = 0;
    for (int i = 0; i < VP_MUSIC_FFT; i++) {
        float w = 0.5f - 0.5f * cosf(6.28318530f * (float)i / (VP_MUSIC_FFT - 1));   /* Hann */
        re[i] = mono[i] * w;
        im[i] = 0;
        sum += mono[i] * mono[i];
    }
    vp_music_fft(re, im, VP_MUSIC_FFT);
    if (rms) *rms = sqrtf(sum / VP_MUSIC_FFT);

    const float lo = 40.0f, hi = 16000.0f, binHz = (float)rate / VP_MUSIC_FFT;
    float loudest = -120.0f;
    for (int b = 0; b < VP_MUSIC_BANDS; b++) {
        float f0 = lo * powf(hi / lo, (float)b / VP_MUSIC_BANDS);
        float f1 = lo * powf(hi / lo, (float)(b + 1) / VP_MUSIC_BANDS);
        int k0 = (int)(f0 / binHz), k1 = (int)(f1 / binHz);
        if (k1 <= k0) k1 = k0 + 1;
        if (k1 > VP_MUSIC_FFT / 2) k1 = VP_MUSIC_FFT / 2;
        float m = 0;
        for (int k = k0; k < k1; k++) m = fmaxf(m, sqrtf(re[k] * re[k] + im[k] * im[k]));
        db[b] = 20.0f * log10f(m / (VP_MUSIC_FFT / 4) + 1e-6f) + 15.0f * (float)b / VP_MUSIC_BANDS;
        if (db[b] > loudest) loudest = db[b];
    }
    return loudest;
}

/* A whole track's loudest, for its bars' top: n mono samples (-1..1) at
 * `rate` Hz, looked at as vp_music_analyze does; the loudest band of the
 * loudest 1% of looks (a lone click does not set it). In dB as played at
 * full volume; -120 for silence. */
static inline float vp_music_track_top_db(const float *mono, long n, int rate)
{
    enum { BINS = 320 };                    /* half-dB steps, -120 .. +40 dB */
    unsigned hist[BINS] = {0}, looks = 0;
    float db[VP_MUSIC_BANDS];
    for (long at = 0; at + VP_MUSIC_FFT <= n; at += VP_MUSIC_FFT) {
        float l = vp_music_spectrum_db(mono + at, rate, db, 0);
        int k = (int)((l + 120.0f) * 2.0f);
        hist[k < 0 ? 0 : k >= BINS ? BINS - 1 : k]++;
        looks++;
    }
    unsigned above = 0;
    for (int k = BINS - 1; k > 0; k--) {
        above += hist[k];
        if (above * 100 >= looks) return (float)k / 2.0f - 120.0f;
    }
    return -120.0f;
}

/* The track now playing has this loudest (vp_music_track_top_db, plus
 * 20*log10 of the volume it is heard at); new_track: it just started (the
 * top moves to it at once). */
static inline void vp_music_use_top(vp_music_analyzer *a, float db, int new_track)
{
    a->track_top_db = db;
    a->has_track_top = 1;
    if (new_track) a->top_db = db;
}

/* The track playing has no measured loudest: the top follows what is heard. */
static inline void vp_music_no_top(vp_music_analyzer *a) { a->has_track_top = 0; }

/* One look at the latest VP_MUSIC_FFT frames heard (-1..1; left and right,
 * or right NULL: mono) at `rate` Hz, dt seconds after the last. Updates
 * bands, level, onsets, vu and peak. */
static inline void vp_music_analyze(vp_music_analyzer *a, const float *left, const float *right,
                                    int rate, float dt)
{
    float mono[VP_MUSIC_FFT];
    for (int i = 0; i < VP_MUSIC_FFT; i++) mono[i] = right ? (left[i] + right[i]) * 0.5f : left[i];
    float db[VP_MUSIC_BANDS], rms;
    float loudest = vp_music_spectrum_db(mono, rate, db, &rms);
    float level = fminf(1.0f, rms * 3.0f);
    float fall = expf(-dt * 3.0f);                      /* the bars sink back in about a second */

    /* the gain: the top is the track's loudest when known, else the
     * loudest lately, whatever the volume. Louder than the top lifts it at
     * once; it sinks back 4 dB a second, to the track's loudest or (not
     * known) never below -60 dB, so silence stays low. */
    if (a->top_db == 0 || loudest > a->top_db) a->top_db = loudest;
    else a->top_db -= dt * 4.0f;
    if (a->has_track_top && a->top_db < a->track_top_db) a->top_db = a->track_top_db;
    if (a->top_db < -60.0f) a->top_db = -60.0f;

    float flux = 0;
    for (int b = 0; b < VP_MUSIC_BANDS; b++) {
        /* the top 36 dB shown, curved a little so loud and soft stand apart */
        float v = fminf(1.0f, fmaxf(0.0f, (db[b] - a->top_db + 36.0f) / 36.0f));
        v = v * sqrtf(v);
        if (v > a->raw_prev[b]) flux += v - a->raw_prev[b];
        a->raw_prev[b] = v;
        a->bands[b] = v > a->bands[b] * fall ? v : a->bands[b] * fall;
    }
    a->level = level > a->level * fall ? level : a->level * fall;

    /* a hit: much more change than usual, not too soon after the last */
    a->since_onset += dt;
    if (flux > a->flux_avg * 1.8f + 0.6f && a->since_onset > 0.12f && level > 0.02f) {
        a->onsets++;
        a->since_onset = 0;
    }
    a->flux_avg += (flux - a->flux_avg) * fminf(1.0f, dt * 2.0f);

    /* the VU meters and peaks, per side, against the bars' top: music's
     * loud parts measure about 9 dB under it (0 VU), its peaks up to about
     * 6 dB over it (0 dB peak). A needle takes 300 ms to swing; a peak
     * stays a second, then falls 15 dB a second. */
    const float k = 1.0f - expf(-dt / 0.065f);
    for (int c = 0; c < 2; c++) {
        const float *s = c == 1 && right ? right : left;
        float sq = 0, pk = 0;
        for (int i = VP_MUSIC_FFT / 2; i < VP_MUSIC_FFT; i++) {    /* the latest half */
            sq += s[i] * s[i];
            pk = fmaxf(pk, fabsf(s[i]));
        }
        a->vu_pow[c] += (sq / (VP_MUSIC_FFT / 2) - a->vu_pow[c]) * k;
        a->vu[c] = fmaxf(-60.0f, 10.0f * log10f(a->vu_pow[c] + 1e-12f) - (a->top_db - 9.0f));
        float p = fmaxf(-60.0f, 20.0f * log10f(pk + 1e-6f) - (a->top_db + 6.0f));
        a->peak_hold[c] -= dt;
        if (p >= a->peak[c]) { a->peak[c] = p; a->peak_hold[c] = 1.0f; }
        else if (a->peak_hold[c] <= 0) a->peak[c] = fmaxf(p, a->peak[c] - dt * 15.0f);
    }
}

/* The waveform into f: the last VP_MUSIC_WAVE of n frames (n >=
 * VP_MUSIC_WAVE; right NULL: mono) at `rate` Hz. */
static inline void vp_music_set_wave(vp_music_feed *f, const float *left, const float *right, int n, int rate)
{
    const float *l = left + n - VP_MUSIC_WAVE, *r = right ? right + n - VP_MUSIC_WAVE : l;
    for (int i = 0; i < VP_MUSIC_WAVE; i++) {
        float a = fminf(1.0f, fmaxf(-1.0f, l[i])), b = fminf(1.0f, fmaxf(-1.0f, r[i]));
        f->wave[i][0] = (int16_t)(a * 32767.0f);
        f->wave[i][1] = (int16_t)(b * 32767.0f);
    }
    f->wave_rate = rate;
}

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_MUSIC_FEED_H */
