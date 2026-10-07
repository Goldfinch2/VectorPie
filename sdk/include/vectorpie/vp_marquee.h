/* vp_marquee.h — the game's marquee on the cabinet's HDMI screen.
 *
 * While a game draws on the vector board, the cabinet's HDMI screen (the
 * marquee) would stay black. vp_run shows the game's marquee there by
 * itself when the output is the board: the image `marquee` names in the
 * package's game.ini (marquee.png when there is no game.ini line), as
 * marquee.<screen height>.png when that exists, fitted the way advmame.rc's
 * vector_aux_marquee_display_mode says (fit, zoom, stretch, fit_width,
 * fit_height), and cleared when the game ends. Nothing to do in the game.
 *
 * A game can also change it, e.g. per level:
 *
 *   char path[1024];
 *   vp_marquee_show(vp_asset_path("art/level2.png", path, sizeof path));
 *
 * PNG files. On Linux (the cabinet: KMS/DRM, the HDMI screen); elsewhere,
 * and with the window output, there is no marquee screen and the calls do
 * nothing (vp_marquee_show returns VP_ERR_NODEV). Needs the SDL build.
 */
#ifndef VECTORPIE_VP_MARQUEE_H
#define VECTORPIE_VP_MARQUEE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Show a PNG on the marquee screen. VP_OK, or VP_ERR_NOTFOUND (the file
 * cannot be read or decoded) or VP_ERR_NODEV (no marquee screen). */
int  vp_marquee_show(const char *png_path);

/* Black, and the screen released. */
void vp_marquee_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* VECTORPIE_VP_MARQUEE_H */
