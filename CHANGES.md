
### Next release

● Release Notes

  - Made the network connection UI more intuitive and less confusing.
  - Added the ability to enable or disable Wi-Fi or Ethernet.
  - Hostname is now shown in the network settings.

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
