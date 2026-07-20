### vectorpie-1.1.4


● Release Notes

  - **Battlezone II — proportional single-stick control**: the analog stick now drives the tank proportionally — partial deflection gives partial speed and gentle turns, full deflection gives full speed and the familiar hard turn — instead of acting as on/off switches. The old dual-stick (Xbox-style) input model and the `single_stick` config option are gone; one stick is simply how it works now.

  - **Star Wars — reticle centered at every boot**: the original ROM deliberately loosens its learned yoke calibration at every power-on, which is why the reticle always started off-center until you swept the yoke to its corners. That boot-time relaxation is now patched out when the game loads (Star Wars, its early revision, and The Empire Strikes Back), and the yoke's analog sensitivity default was raised from 70/50 to 100 on both axes to make the calibration an exact pass-through. The reticle is centered from the first frame of every boot — no corner sweep, no per-game configuration.

  - **Settings layout polish**: the label/control divider now sits exactly under the up/down scroll arrows with section headers centered on the same line, labels have more breathing room from their controls, sliders are narrower, and the slider cycle arrows are vertically centered on the bar.



### vectorpie-1.1.3


● Release Notes

  - **Edit USB-DVG board settings from the menu**: a new **Settings → USB-DVG → DVG SETTINGS** row opens its own page showing the USB-DVG board's own settings menu on the vector display, navigated with the cabinet controls — Up/Down/Left/Right/Select as usual, Cancel goes back in the board menu (closing it at its top level saves to the board), and Config closes the board menu (saving) and leaves the page back to VectorPie Settings. The board menu also closes by itself after 30 seconds without a press. Requires USB-DVG firmware 1.14R2 or later — the row is grayed out on older firmware.

  - **Joystick calibration**: a new **Settings → CONTROLS → CALIBRATE** row opens a live joystick axis view (axis bars on both HDMI and the vector display). Calibrating is one step: move all axes to their extremes, release them to rest, and press Select to save — the resting position becomes the new center. Fixes off-center rest positions and limited range on analog controllers such as the Star Wars yoke. With more than one joystick connected, Left/Right on the row picks which one to work on. Calibration is saved and reapplied automatically at every startup; games pick it up on their next launch.

  - **Long setting values scroll**: a settings value too wide for its row — a long Wi-Fi network name, track title, or joystick name — now scrolls horizontally while the row is highlighted instead of overflowing, on both HDMI and the vector display. AdvanceMAME's mirrored in-game menus benefit too.

  - **Settings layout polish**: labels sit closer to their controls, the whole label/control block is better centered, and sliders are slightly shorter. Rows that open their own page (CALIBRATE, DVG SETTINGS) are marked with a trailing "..." to set them apart from in-place settings.

  - **New USB-DVG settings section**: everything USB-DVG-related now lives under its own **USB-DVG** header — DVG, DVG TYPE, USB-DVG UPDATE (moved from SYSTEM, which keeps VECTORPIE UPDATE and EXIT TO SHELL), DVG SETTINGS, CRT CALIBRATION, OVERLAY, and AUTOSTART GAME. DISPLAY keeps the display-generic rows — MARQUEE, MENU ON HDMI, SHOW GAME PREVIEW, MENU FADE — and absorbs the former HINT BAR section (HINTS, HIGH SCORES).

  - **CRT calibration moved into Settings**: the vector-display alignment test patterns now live at **Settings → USB-DVG → CRT CALIBRATION** with their own page (Left/Right cycles patterns, Cancel exits). Pressing Coin in the game menu no longer opens calibration.



### vectorpie-1.1.2


● Release Notes

  - **New vector game — Vector Millipede**: a vectorised rendering of Atari's Millipede joins the vector line-up (Vector Kong, Vector Galaga, Vector Galaxian, Vector Centipede) in the game list, under Atari.

  - **Expanded Vectrex library**: a large batch of new Vectrex titles has been added to the game list, spanning both Vectrex Official releases and Vectrex Homebrew games.

  - **Game preview video returns after the screensaver**: dismissing the screensaver — or coming back from Settings — now brings the selected game's preview video right back, instead of leaving the marquee static until you moved to a different game.

  - **Snappier idle dimming**: the menu text now begins fading the moment you stop pressing keys and is fully hidden after 2.5 seconds (it used to wait 15 seconds, then take another 15 to fade). The fade duration is configurable in Settings → DISPLAY → MENU FADE (1 s to 15 s, default 2.5 s).

  - **Alphabetical game list**: manufacturers and the games within each are now always listed alphabetically by title, so the menu stays tidy even though newly-added games are inserted at the top of the underlying list on update.

  - **New games survive an update**: when you OTA-update, games added to the built-in list since your installed version (such as the new vector games) are merged into your game list on restore — without bringing back games you've removed.

  - **USB-DVG firmware updates from the menu**: a new **Settings → SYSTEM → USB-DVG UPDATE** row checks online for new USB-DVG firmware and — when an update is available — a second press downloads, verifies, and flashes the board right from the menu, then reboots the Pi. The installed version is read from the board itself, and only newer releases are offered (the menu never downgrades).

  - **Firmware type detected automatically**: a new **DISPLAY → DVG TYPE** row shows whether the board runs the STANDARD or ARCADE CONTROL firmware, detected from the board itself at startup. Changing it makes USB-DVG UPDATE offer the other variant — even at the same version — so a board flashed with the wrong variant can be corrected from the menu.

  - **Update rows renamed**: the former CHECK FOR UPDATES row is now **VECTORPIE UPDATE**, sitting alongside the new USB-DVG UPDATE row — each checks with one press and applies with a second.

  - **Music playlist moved to the persistent partition**: playlist tracks now live at `/persistent/vector_pie_menu_dir/playlist`, which survives software updates in place. Existing tracks are migrated automatically on first start. As a result, playlist music is no longer part of USB backups or update snapshots — when moving to a new SD card, copy the playlist folder over the network share separately.

● Bug Fixes

  - **Software update could fail half-way with a large music library**: the update's settings-restore staged its working copy on the system partition, which could fill the disk mid-restore and leave settings (including the game list merge) only partially applied. The restore now checks free space before writing anything, stages on the persistent partition instead, and cleans up after itself on failure.

  - **Settings menu auto-closed during long operations**: the 60-second idle timeout no longer triggers while a backup, restore, software update, or firmware flash is running — and once the operation finishes, the timeout restarts so the result stays on screen long enough to read.


### vectorpie-1.1.1


● Release Notes

  - **New vector games — Vector Galaxian and Vector Centipede**: two more vectorised arcade renderings join Vector Kong and Vector Galaga in the game list — Vector Galaxian under Midway and Vector Centipede under Atari.

  - **AdvanceMAME's in-game menus mirror on the vector display**: when the USB-DVG is connected, opening AdvanceMAME's configuration menu (Tab) — including the **input configuration** screen — now shows the same menu on the vector monitor in parallel with HDMI, styled like the game menu. It works over any game (vector or raster) and holds a steady, flicker-free refresh. Control bindings are shown in a compact, all-caps form (e.g. `J:BTN1`, `LCTRL`, `P:1`); the HDMI menu still shows the full names. The in-game menu labels were also shortened to fit the vector screen.

  - **Editable hostname**: **Settings → NETWORK → HOSTNAME** is now editable (it used to be read-only). Press Select to edit it, then use the on-screen character editor (Up/Down spin the character, Left/Right move between them, Right past the end adds one, Select picks it, a **DEL** choice removes one; **Cancel** saves and exits). The change applies **live** — no reboot — so `<name>.local` and the network-share name update right away. The hostname is included in backups and restored onto a new image.

  - **On-screen text editor improvements**: the character editor used for the hostname and Wi-Fi password now has a **DEL** choice to delete characters, scrolls when the text is long, and is finished with your **Cancel** control.

  - **Settings menu mirrored on the vector display**: while Settings is open, the USB-DVG now shows the same menu — styled like the game menu (red title and section headers, the highlighted row in a selection box, choices and sliders with cycle arrows) — instead of the screensaver. You can navigate and change settings while watching the vector monitor.

  - **Rebind the in-game configuration control**: a new **UI CONFIG** entry at the top of **Settings → CONTROLS** rebinds AdvanceMAME's configuration-menu control (default Tab). It always keeps Tab so the in-game menu stays reachable. CONTROLS now lists UI CONFIG and UI CANCEL first, ahead of the player controls.

  - **Rebind without a keyboard**: a control rebind is now saved by pressing your **Cancel** control, so the whole flow works on a cabinet with no keyboard. Rebinding the Cancel control itself is saved with Escape (it can't use itself). Escape still works everywhere as a fallback, and the abandoned-capture auto-save now triggers after 20 seconds (was 30).

  - **Configure controls from the menu**: a new **Settings → CONTROLS** section lets you rebind the common player controls — P1/P2 directions, fire, start, coin, pause, and cancel — directly from the VectorPie menu, without launching AdvanceMAME. Highlight a control, press Select, then press the key/button/joystick direction you want; the binding builds up live (e.g. `p` then `6` → `p or 6`). Press several inputs to add "or" alternatives, then Escape to save; Left/Right resets to the default. Bindings write to the shared AdvanceMAME configuration, so they apply to the menu, AdvanceMAME games, and the native Pi games alike. The cancel control always keeps Escape so you can't lock yourself out.

  - **Game preview videos play instantly**: the preview video now starts the moment you land on a game selection, instead of after 15 seconds of idle.


### vectorpie-1.1.0


● Release Notes

  - Support for OTA updates. Settings → SYSTEM → CHECK FOR UPDATES, press twice, the Pi reboots into the new version. Your Wi-Fi, settings, high scores, and music are restored after the update.

  - **Game preview videos**: after 15 seconds of idle on a game selection, the game's preview video plays over the marquee. Ships with ~40 vector-arcade preview MP4s at 1920×360 (no audio). New **Settings → DISPLAY → SHOW GAME PREVIEW** toggle (default on).

  - **Hint bar visible without the pie menu**: when **MENU ON HDMI** is off, the hint bar still appears on the marquee provided **SHOW BUTTON HINTS** or **SHOW HIGH SCORES** is enabled.

  - **Hint bar text legibility**: every non-pill hint segment now renders with a 2-pixel black outline so titles stay readable against bright marquee backgrounds.

  - **HDMI-1 overlay display removed**: the second HDMI output is no longer used for per-game color overlay artwork. ~171 MB of overlay PNGs removed from the image.

  - **Tightened backup contents.** Backups now capture only the locked critical-file set (settings, configs, high scores, Wi-Fi/SSH credentials, etc.) — not ROMs, samples, or customized artwork. Older backups still restore correctly: only the critical subset is applied; extra content in the archive is ignored.

  - Animated screensaver background — `vectorpie.mp4` plays on loop after the attract video; falls back to the still PNG if not present.

  - Per-resolution artwork — `<image>.<height>.png` (e.g. `pacman.360.png`) is picked automatically when it matches the screen height; plain `<image>.png` remains the fallback.

● Bug Fixes

  - Galaga high scores in the hint bar were garbage; now decoded correctly.

### vectorpie-1.0-39-g92d0753

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
