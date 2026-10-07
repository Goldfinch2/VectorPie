/* vp_paths.h — where a game's files are.
 *
 * A game's files (sounds, models...) live next to its executable: in the
 * build folder while developing, in its package folder on the cabinet. Build
 * paths from vp_package_dir, never from the current directory or a fixed
 * /persistent/games path.
 *
 *   char path[1024];
 *   vp_sound_load(audio, vp_asset_path("sounds/zap.wav", path, sizeof path), &zap);
 */
#ifndef VECTORPIE_VP_PATHS_H
#define VECTORPIE_VP_PATHS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The directory holding the running executable, without a trailing slash. */
const char *vp_package_dir(void);

/* vp_package_dir() + "/" + relative, written into buf; returns buf. Forward
 * slashes work on Windows too. */
const char *vp_asset_path(const char *relative, char *buf, size_t size);

/* vp_package_dir() + "/save": where the game writes what it keeps (high
 * scores, settings). Created the first time it is asked for; an installed
 * package keeps it across upgrades. */
const char *vp_save_dir(void);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_PATHS_H */
