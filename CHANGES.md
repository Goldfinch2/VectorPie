
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
