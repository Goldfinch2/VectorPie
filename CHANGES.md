### vectorpie-1.0-NN (upcoming)

● Release Notes

  - **Final reflash — atomic in-place updates begin next release.** This is the last time you'll write a VectorPie image to your SD card. The on-disk layout changes to A/B base partitions plus a separate persistent partition for user state; from the next release onward, updates apply in place without reflashing and auto-roll-back if anything goes wrong. Before flashing this image, run **Settings → BACKUP → SAVE ⏏** on your existing card; after flashing, run **Settings → BACKUP → RESTORE ⏏** to bring your settings, Wi-Fi, high scores, music, and artwork onto the new layout. Minimum SD size is still 32 GB.
  - **Removed the AdvanceMAME OverlayFS.** `/usr/local/share/advance/` is now a plain directory on the rootfs. Streamlines updates (writes go directly to the live tree) and removes a frequent source of "I installed it but the change didn't take effect" confusion.
  - **Tightened backup contents.** Backups now capture only the locked critical-file set (settings, configs, high scores, Wi-Fi/SSH credentials, etc.) — not ROMs, samples, or customized artwork. Backup tarballs are now KB-sized instead of GB. Older backups still restore correctly: only the critical subset is applied; extra content in the archive is ignored.
  - Animated screensaver background — `vectorpie.mp4` plays on loop after the attract video; falls back to the still PNG if not present.
  - Per-resolution artwork — `<image>.<height>.png` (e.g. `pacman.360.png`) is picked automatically when it matches the screen height; plain `<image>.png` remains the fallback.
  - Settings menu polish — dark-purple outlined section headers, slightly larger; SETTINGS title lowered; column block centered as a unit.

● Bug Fixes

  - Galaga high scores in the hint bar were garbage; now decoded correctly.



● Release Notes

  - **New vector game — Vector Galaga**: a vectorised rendering of Galaga, joining Vector Kong in the Other section of the game list.
  - Added missing sample file for Vector Kong.
  - **Screensaver video on marquee displays**: the screensaver video now scales to fill the screen width on wide-aspect marquees (e.g. 1920×360), keeping the picture full-width and cropping the height instead of letterboxing.
  - **Now Playing readability on marquees**: on wide-aspect marquee displays, the NOW PLAYING / NEXT block uses a larger font with a thick black outline so titles stay readable against the screensaver video. Layout is anchored from the bottom so both NOW PLAYING and NEXT fit on short-aspect screens.
  - **New MARQUEE scaling modes**: Settings → MARQUEE now offers **FIT WIDTH** (scale to screen width, crop/letterbox height) and **FIT HEIGHT** (scale to screen height, crop/pillarbox width) alongside the existing FIT / STRETCH / ZOOM. The new modes are honored everywhere marquee artwork is shown — the menu, AdvanceMAME, and the native games (Battle Zone II, Geometry Wars).
  - **MAME Sound Quality preset**: new **Settings → AUDIO → MAME SOUND QUALITY** chooses **HIGH** / **MEDIUM** / **LOW**, applying a coordinated set of AdvanceMAME audio settings (sample rate, latency, normalization) to reduce crackling and boost performance on demanding games. **LOW** (11 kHz) is the most performant but makes speech games (Star Trek, Zektor, Space Fury) sound muffled; **HIGH** (48 kHz) is the default.

● Bug Fixes

  - **VectorPie Commercial title flicker**: the NOW PLAYING / NEXT labels no longer briefly swap to the next playlist track around a commercial break. Transitions are now monotonic: current track → "VectorPie Commercial" → next track, with no intermediate state visible.


### vectorpie-1.0-33-g867bdbc.img.gz

● Release Notes

  - New game: vector kong.
  - Made the network connection UI more intuitive and less confusing.
  - Added the ability to enable or disable Wi-Fi or Ethernet.
  - Hostname is now shown in the network settings.
  - **VectorPie Radio**: each time the music playlist starts, a random DJ-announcement clip from the new `radio/` folder plays before the first track. Drop your own `.mp3`/`.ogg` files into `sounds/radio/` to customize.
  - **Backup export safe-eject**: when saving a backup to USB, the menu now flushes caches, unmounts, and powers down the USB device before showing `SUCCESS` — the activity LED is off when you see the result, so it's truly safe to remove. An `EJECTING…` indicator is shown during the flush/detach phase.

● Bug Fixes

  - Diagnostic log (`vector_pie_menu.log`) is now written reliably.
  - Backup/restore now captures more of your system configuration, so settings like the Wi-Fi enabled state survive a restore.


### vectorpie-1.0-27-g6b95bb9.img.gz

● Release Notes

  - **Music playlist**: drop `.mp3` / `.ogg` files into the playlist and the menu will play them sequentially or shuffled. New **MUSIC TYPE** setting picks **THEME** (single looping track) or **PLAYLIST**; **PLAYLIST SHUFFLE** controls order. Music keeps playing while you are in Settings, and playlist position is preserved across game launches.
  - **Music import from USB**: new **MUSIC → IMPORT ⏏** option copies tracks from a USB drive — `playlist/` and `themes/` folders on the drive go to their matching destinations. Pick **ADD** to merge or **REPLACE** to wipe first.
  - **Now Playing on screensaver**: current and next track titles are shown beneath the logo when the playlist is active.
  - **Settings menu redesigned** — items are now grouped under section headers (DISPLAY, HINT BAR, AUDIO, MUSIC, EFFECTS, CONTROLS, BACKUP, NETWORK, SYSTEM) for easier scanning.
  - **Audio output** shows friendly names (`USB`, `HEADPHONE`, `SOUND HAT`); HifiBerry-style HATs are recognized too.
  - **DVG screensaver in Settings**: drifting asteroids + logo run on the vector display the whole time Settings is open.
  - **Game list wraps**: down on the last game returns to the manufacturer header, and up on the header jumps to the last game.
  - **Faster idle dimming**: text starts fading at 15 s (was 65), screensaver kicks in at 60 s (was 90). Button hints stay visible during dim.
  - Backup now also captures the playlist and themes directories.

● Bug Fixes

  - Native game: CANCEL (exit) can now be mapped to a joystick button.
  - Vectrex manufacturer headers show the marquee again — any name containing "vectrex" matches.
  - Backup restore: Wi-Fi reconnects properly after restoring onto a fresh image, and restored playlist/settings appear immediately without a reboot.


### vectorpie-1.0-21-gea18204.img.gz:

● Release Notes

  - Menu audio split into independent effects and music toggles (each with its own volume)
  - Theme music is now selectable from the Settings menu — any `.mp3` or `.ogg` dropped into the sounds directory appears in the list; selection previews live
  - Marquee and overlay artwork lookups are now case-insensitive (`Asteroid.PNG` matches `asteroid`)
  - Vectrex cartridge titles get per-cart marquees — the marquee filename is the clone-ROM field from `gamelist.ini` plus `.png` (e.g. `lunar.bin.png`)
  - Settings → Network: current connection and IP address now auto-refresh, so plugging in an Ethernet cable is reflected immediately
  - Backup now also includes customized artwork (marquees/overlays), added theme music files, and your `/boot/firmware/user-config.txt`
  - `/boot/firmware/user-config.txt` is a new user-editable file included from `config.txt` — edit it freely without conflicting with future image updates
  - Import from USB no longer fails on FAT drives (ownership restore is skipped)
  - Reboot confirmation changed: after the first ESC, press **Select** to reboot (previously a second ESC) — prevents accidental reboots from double-tapping ESC

● Bug Fixes

  - Backup export/import now shows a `WORKING…` progress indicator and no longer freezes the menu while the operation is in progress
  - Export/import errors (e.g. "NO USB DRIVE FOUND") are no longer shown on both Export and Import rows simultaneously — the message only appears on the action you triggered
  - High-score hint bar now refreshes after a game exits, so newly-set scores appear without having to reopen the menu
  - Optimized rendering of long high-score hint bars — scrolling is now smooth even with many entries


### vectorpie-1.0-15-gdd32520.img.gz:

● Release Notes

  - High scores displayed in scrolling hint bar with color-coded entries
  - Export/import settings, high scores, and Wi-Fi credentials to USB drive
  - Settings menu opens instantly (Wi-Fi scans in background)
  - ESC double-press now reboots instead of exiting
  - EXIT TO SHELL option in settings menu for developer access
  - F1 shortcut to exit to shell from reboot prompt
  - Button hints and high scores individually toggleable
  - Native game high scores supported (Battle Zone II, Geometry Wars)
  - Geometry Wars high score entry screen re-enabled with improved layout
  - USB-DVG firmware updated to 1.14R1
