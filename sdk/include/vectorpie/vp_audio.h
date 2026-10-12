/* vp_audio.h — sound effects and music.
 *
 *   vp_sound *zap;
 *   vp_sound_load(audio, vp_asset_path("sounds/zap.wav", buf, sizeof buf), &zap);
 *   vp_sound_play(audio, zap, 1.0f, 1.0f, 0);               // fire and forget
 *   vp_voice music = vp_music_play(audio, path, 0.6f, 1, 2.0f);   // looped, 2 s fade in
 *   vp_voice_stop(audio, music, 1.5f);                      // 1.5 s fade out
 *
 * EFFECTS (vp_sound) are loaded into memory once and played as often as
 * needed, many at once, each with its own volume, pitch and looping.
 * MUSIC and speech (vp_music_play) is streamed from disk while it plays, so
 * long tracks cost no memory. Both take WAV files (PCM 8, 16, 24 or 32-bit,
 * or 32-bit float) or Ogg Vorbis files (.ogg, about a tenth of the size),
 * told apart by their contents; mono or stereo (Ogg surround is folded to
 * stereo); any sample rate (converted as it plays).
 *
 * A playing sound is a VOICE, identified by the vp_voice handle the play call
 * returns. A handle stays valid until the voice ends; after that every call
 * with it does nothing. 0 is never a voice.
 *
 * Volumes are 0..1 (above 1 amplifies). Pitch 1 is the recorded speed; 2 plays
 * an octave up and twice as fast. Changes are smoothed over a few
 * milliseconds, so they never click.
 *
 * vp_run opens the audio for a game (vp_game_audio). Every call accepts a NULL
 * vp_audio and then does nothing, so a game runs the same with no sound device.
 *
 * Requires the SDK built with SDL2 (the default).
 */
#ifndef VECTORPIE_VP_AUDIO_H
#define VECTORPIE_VP_AUDIO_H

#include <stdint.h>
#include "vp_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vp_audio vp_audio;
typedef struct vp_sound vp_sound;
typedef uint32_t vp_voice;

#define VP_AUDIO_VOICES 64    /* voices playing at once, effects and music together */

int  vp_audio_open(vp_audio **out);
void vp_audio_close(vp_audio *a);      /* stops everything, frees every sound */

/* Effects ---------------------------------------------------------------- */

int  vp_sound_load(vp_audio *a, const char *wav_path, vp_sound **out);
/* A sound from samples made by the program (a synthesized zap). `samples`
 * holds frames x channels floats in -1..1, interleaved; channels 1 or 2. */
int  vp_sound_from_samples(vp_audio *a, const float *samples, int frames, int channels,
                           int sample_rate, vp_sound **out);
/* Stops its voices first. NULL is allowed. */
void vp_sound_free(vp_audio *a, vp_sound *s);
float vp_sound_seconds(const vp_sound *s);

/* Play an effect. Returns its voice, or 0 if all VP_AUDIO_VOICES are busy
 * (counted in vp_audio_stats.refused). */
vp_voice vp_sound_play(vp_audio *a, vp_sound *s, float volume, float pitch, int loop);

/* Music and speech ----------------------------------------------------------- */

/* Stream a WAV file from disk. fade_in_s 0 = start at full volume. Returns its
 * voice, or 0 (the file cannot be read: vp_last_error() says why; or all
 * voices busy). */
vp_voice vp_music_play(vp_audio *a, const char *wav_path, float volume, int loop,
                       float fade_in_s);

/* Any voice ----------------------------------------------------------------- */

/* fade_out_s 0 = stop now (after a few milliseconds, to avoid a click). */
void vp_voice_stop(vp_audio *a, vp_voice v, float fade_out_s);
void vp_voice_set_volume(vp_audio *a, vp_voice v, float volume, float fade_s);
void vp_voice_set_pitch(vp_audio *a, vp_voice v, float pitch);
int  vp_voice_playing(vp_audio *a, vp_voice v);

void vp_audio_stop_all(vp_audio *a, float fade_out_s);
void vp_audio_set_master(vp_audio *a, float volume);   /* default 1 */

typedef struct vp_audio_stats {
    int      voices;       /* playing now */
    uint32_t refused;      /* plays refused because every voice was busy (total) */
    uint32_t underruns;    /* times a stream was not read from disk in time (total) */
    int      sample_rate;  /* the device's */
} vp_audio_stats;
void vp_audio_get_stats(vp_audio *a, vp_audio_stats *stats);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_AUDIO_H */
