
### Upcoming:

● Release Notes

  - Music playlist support — load `.mp3` / `.ogg` tracks via the new **MUSIC → IMPORT ⏏** option (see below) and the menu will play them sequentially or shuffled
  - New **MUSIC TYPE** setting selects between **THEME** (single looping track) and **PLAYLIST** (sequential or shuffled play of the music directory). Replaces the previous MUSIC on/off toggle.
  - New **PLAYLIST SHUFFLE** setting controls playlist ordering
  - New **IMPORT ⏏** option in the Music section copies `.mp3` / `.ogg` tracks from a USB drive — a single import handles both the playlist (from the drive's `playlist/` folder) and theme tracks (from `themes/`). Pick **ADD** to merge with your existing tracks, or **REPLACE** to wipe each destination first (REPLACE asks for a second press to confirm). The same drive can also carry a settings backup alongside the track folders.
  - Music keeps playing and advances to the next track while you are in the Settings menu (previously it would only resume after exiting Settings)
  - Theme music files now live in their own `themes/` directory, separate from the menu sound effects
  - Playlist position is preserved across game launches — the same track resumes from where it left off
  - Screensaver shows **NOW PLAYING** and **NEXT** track titles beneath the logo when the playlist is active. On the USB-DVG the titles are drawn larger and reposition periodically so they do not burn into the vector CRT
  - Settings menu redesigned — items are now grouped under bold cyan section headers (DISPLAY, HINT BAR, AUDIO, MUSIC, EFFECTS, CONTROLS, BACKUP, NETWORK, SYSTEM) for easier scanning, with shorter row labels
  - **AUDIO OUTPUT** in Settings now shows a friendly short name (`USB`, `HEADPHONE`, or `SOUND HAT`) instead of the full system device name; HifiBerry HATs and compatible clones (IQaudIO, JustBoom, Allo, etc.) are recognized too
  - Idle dimming retimed: menu text now begins fading at 15 seconds (was 65 s) and the screensaver activates at 60 seconds (was 90 s)
  - Button hints now stay visible during idle dimming instead of fading out with the rest of the menu text
  - Backup now also includes the playlist music directory and the new themes directory
  - DVG vector display now plays a decorative animation (drifting asteroids + logo + **SETTINGS** label) the entire time the Settings menu is open, instead of freezing on the last game-menu frame (LCD keeps showing the settings menu)
  - Game list navigation now wraps around — pressing **down** on the last game returns to the manufacturer header, and **up** on the header jumps to the last game; useful for quickly reaching games near the end of long lists
  - Backup now also captures SSH credentials and the pi-user password, so a restore onto a fresh SD image keeps you logged-in without reconfiguring:
      - `/home/pi/.ssh/` — the whole directory (private/public keys, `authorized_keys`, `known_hosts`, `config`). Restore wipes the destination's `.ssh` first so the result mirrors the backup byte-for-byte (anything added between backup and restore is dropped)
      - `/etc/ssh/ssh_host_*` — the system's SSH host keys, so SSH clients no longer get `REMOTE HOST IDENTIFICATION HAS CHANGED` warnings after reimaging. SSH service is restarted automatically on restore
      - The pi user's password hash, captured surgically from `/etc/shadow` (just the `pi:` line, not the whole file) so system-account entries on the destination are left untouched

● Bug Fixes

  - Native game: CANCEL (exit) can now be mapped to a joystick button — previously only keyboard/mouse bindings were honored because the main loop's exit check was missing the joystick handles
  - Menu: Vectrex manufacturer headers now show the `mfg_vectrex` marquee again — any manufacturer name containing "vectrex" (e.g. "Vectrex Official", "Vectrex Homebrew") collapses to the shared marquee
  - Backup restore: Wi-Fi connections now work again after restoring onto a fresh SD image. NetworkManager only loads `/etc/NetworkManager/system-connections/*` files owned by `root:root` with mode `0600`, but the previous backup archive forced `pi:pi` on every file. The archive is now built in two passes: system paths (NetworkManager, `/boot/firmware`, the overlay upper) keep their real on-disk ownership, while user content (`/home/pi/.advance/*`) is force-flipped to `pi:pi` for portability. Restore also runs a chown/chmod safety net so backups produced by older releases still work.
    To recover an already-restored setup without waiting for the new image: from the menu press **ESC** twice then **F1** to drop to a shell, and run:
    ```
    sudo chown -R root:root /etc/NetworkManager/system-connections
    sudo chmod 600 /etc/NetworkManager/system-connections/*
    sudo nmcli connection reload
    sudo reboot
    ```


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
