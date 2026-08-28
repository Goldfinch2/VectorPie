# VectorPie User Manual

![VectorPie Logo](vector_pie_logo.png)

## Overview

VectorPie is a pre-configured Raspberry Pi image designed to be used with the USB-DVG vector generator board. It can also be used without USB-DVG, displaying on a standard HDMI monitor instead. It boots directly into the game menu and all navigation is done entirely through your arcade controls — no keyboard, mouse, or desktop environment required.

Key features:

- **Headless operation** — runs at boot with no monitor or desktop required on the Pi
- **Marquee display** — the HDMI output automatically shows the marquee artwork for the game currently highlighted in the menu
- **USB-DVG support** — drive a real vector monitor via the USB-DVG vector generator board; HDMI output is also supported for use without USB-DVG
- **Unified input mapping** — controls are configured once in AdvanceMAME and apply to the menu and every game, including non-emulated titles
- **High scores** — scrolling hint bar displays high scores parsed directly from MAME save files for the selected game
- **Per-game LED lighting** — the control panel lights each game's controls in its own colors, with a voice-guided control tour
- **Wi-Fi configuration** — connect to a wireless network directly from the menu
- **Remote access** — built-in SSH server and Windows network share for easy file management from a PC
- **Software updates from the menu** — check for and apply new VectorPie versions directly from the Settings menu, with automatic rollback if an update fails to boot

VectorPie is compatible with the **Raspberry Pi 4** and **Raspberry Pi 5**.

---

## Supported Hardware

- Raspberry Pi 4 (1 GB+) or Raspberry Pi 5 (1 GB+)
- 32 GB SD card or larger.

---

## Download & Installation

### VectorPie Image

1. Download the VectorPie image (approximately 2 GB): [VectorPie Image](https://drive.google.com/file/d/1hH61wM1KwL0k7ErPcdbFcr2jzHHjA6fq/view?usp=sharing)
2. Write the image to a 32 GB or larger micro SD card using **balenaEtcher** ([download](https://etcher.balena.io/)):
   - Open balenaEtcher and click **Flash from file**, then select the downloaded `.img.gz` file
   - Click **Select target** and choose your micro SD card
   - Click **Flash!** and wait for the write and verification to complete
3. Insert the micro SD card into the Raspberry Pi and power it on — VectorPie boots directly into the game menu

### USB-DVG Firmware

There are two USB-DVG firmware variants:

- **Standalone** — standard firmware for use with any cabinet or custom control setup
- **Arcade Control** — includes support for an adapter card that connects directly to the original wiring harness of an Asteroids, Asteroids Deluxe, Space Duel, Tempest, or Black Widow cabinet, allowing the cab's original buttons, CRT, and coin inputs to be used natively

**Which firmware should I use?**

| USB-DVG Version | Use |
|---|---|
| v1 / v2 | Standalone only |
| v3 or later (standalone) | Standalone |
| v3 or later (mounted on adapter card) | Arcade Control |

Flash the firmware that matches your hardware using **Teensy Loader** ([download](https://www.pjrc.com/teensy/loader.html)):

> **Note:** Skip this section if you are running VectorPie 1.1.2 or later — the USB-DVG firmware is updated directly from the menu (Settings → USB-DVG → USB-DVG UPDATE), and Teensy Loader is no longer needed. The manual procedure below is only for older VectorPie versions.

1. Download the appropriate USB-DVG firmware `.hex` file:
   - [Standalone Firmware 1.14R1](https://drive.google.com/file/d/1goWkwykACHfnLVW0sk0REXXBPuLQ8ldB/view?usp=drive_link)
   - [Arcade Control Firmware 1.14R1](https://drive.google.com/file/d/1tx1JvYTLceOkbxAkWTCNXsJ22BJQq2xp/view?usp=drive_link)
2. Open **Teensy Loader** and load the `.hex` file via **File → Open HEX File**
3. Press the reset button on the Teensy board — Teensy Loader will detect the board and automatically flash the firmware
4. Once flashing is complete the board resets and is ready to use

---

## Menu Navigation

All navigation can be done with either a keyboard or arcade controls. Mappings are read from the AdvanceMAME configuration. A scrolling hint bar along the bottom of the screen shows available controls and high scores for the selected game.

> **Note:** A keyboard may be needed initially to configure the button mappings for your arcade controls inside AdvanceMAME. Once mapped, the keyboard is no longer required.

| Action | Description |
|---|---|
| Up / Down | Move between games in the current manufacturer's list |
| Left / Right (on manufacturer header) | Switch to the previous or next manufacturer |
| Left / Right (on a game) | Cycle through ROM variants (revisions, regions, prototypes) |
| Select / Start (tap) | Launch the selected game |
| Select / Start (hold ~1 s) | CONTROL GUIDE: tour the game's controls — each control blinks alone on the panel while a voice names its function and the name shows on screen. Every tour ends with INSERT COIN and PRESS START. Any input ends the tour. Needs a configured LED panel (Settings → LEDS → LED SETUP); without one, holding simply launches the game |
| Settings | Open the settings menu |
| Quit | Request reboot (then press Select to confirm) |

---

## Menu Layout

The menu shows a vertically scrolling list of games grouped by manufacturer:

- The **manufacturer header** is the top-level entry for each manufacturer. Pressing Left/Right while it is selected switches manufacturers.
- **Game entries** are listed below the header. Games with multiple ROM variants show left/right arrows and can be cycled with Left/Right.
- The **selected item** is displayed full-size in the center with a pulsing highlight. Surrounding entries scale down toward the edges.
- Long game names scroll horizontally within the selection box.

---

## Marquee Display

The HDMI output automatically updates to show the marquee artwork for the currently highlighted game. Browsing at the manufacturer level shows the manufacturer's logo. If no artwork exists for a specific game a default image is shown.

The artwork scaling mode (Fit, Stretch, or Zoom) can be changed in the Settings menu.

---

## Input Mapping

Control mappings are configured **once inside AdvanceMAME** and apply automatically everywhere:

- The VectorPie menu navigation controls
- All AdvanceMAME games
- Games compiled natively for the Pi — these use SDL and read their input mappings directly from the same AdvanceMAME configuration file

To remap controls, launch any AdvanceMAME game and use its input configuration menu. The new mappings take effect in the menu and all games immediately on the next launch.

> When the USB-DVG is connected, AdvanceMAME's in-game menus — including the **input configuration** screen — are mirrored on the vector display as you navigate them, in parallel with HDMI, so you can rebind while watching the vector monitor. Bindings appear in a compact, all-caps form there (e.g. `J:BTN1` = joystick button 1, `LCTRL`, `P:1` = keypad 1); HDMI still shows the full names. This works over any game, vector or raster.

### Rebinding from the menu

You can also rebind the common controls **directly from the VectorPie menu** — no keyboard or AdvanceMAME game needed. Open **Settings → CONTROLS**, where the config-menu and cancel controls (**UI CONFIG**, **UI CANCEL**) are listed first, followed by the player controls (P1/P2 directions, fire, start, coin, pause), each with its current binding:

- Highlight a control and press **Select**, then press the key, button, or joystick direction you want. The binding builds up live (e.g. press `p` then `6` shows `p or 6`).
- Press **several** inputs to add "or" alternatives (e.g. a key *and* a joystick direction).
- Press your **Cancel** control to save — so a rebind can be finished entirely from the cabinet, with no keyboard. (An abandoned capture also auto-saves after 20 seconds.)
- When rebinding the **Cancel** control itself, save with **Escape** instead (it can't use itself to finish).
- **Left / Right** resets a control to its default.
- The **cancel** control always keeps Escape and the **config-menu** control always keeps Tab, so you can never lock yourself out of quitting or the in-game configuration menu.

These write to the same AdvanceMAME configuration as above, so the changes apply to the menu, AdvanceMAME games, and the native Pi games alike.

The menu and the natively-compiled Pi games read the following AdvanceMAME input actions. The **Read by** column shows which programs honor each action — the VectorPie menu and Geometry Wars (`opengw`):

| AdvanceMAME Action | Menu Function | Default Key | Read by |
|---|---|---|---|
| `ui_up` | Navigate up | Up Arrow | menu |
| `ui_down` | Navigate down | Down Arrow | menu |
| `ui_left` | Navigate left / previous manufacturer | Left Arrow | menu |
| `ui_right` | Navigate right / next manufacturer | Right Arrow | menu |
| `ui_select` | Launch selected game | 1, Enter, or Left Ctrl | menu |
| `ui_configure` | Open settings menu | Tab | menu |
| `ui_pause` | (not used by menu) | P | opengw (pause) |
| `ui_cancel` | Quit / exit | Escape | menu, opengw |

### Game Controls

VectorPie games use several types of analog and digital controls. AdvanceMAME automatically maps connected hardware to the appropriate control type for each game. The **Read by** column shows which natively-compiled Pi games honor each action — AdvanceMAME-driven games read all of these regardless.

| Control Type | AdvanceMAME Action | Typical Input | Read by |
|---|---|---|---|
| Directional movement | `p1_up/down/left/right` | Joystick, keyboard arrows | opengw (move) |
| Twin-stick aim | `p2_up/down/left/right` | Second joystick, keyboard (default R/F/D/G) | opengw (aim) |
| Fire / action | `p1_button1` | Left Ctrl, mouse button, joystick button | opengw |
| Player 1 start | `start1` | 1 key | opengw |
| Insert coin | `coin1` | 5 key | menu, opengw |

Two-player games that use twin sticks use both `p1_` and `p2_` actions for movement and aiming independently.

> **Note:** A mouse and an analog joystick can be used simultaneously. For games that use an analog stick (Star Wars, Empire Strikes Back, Lunar Lander, Tail Gunner, Red Baron), the **CONTROLS → ADSTICK** setting in the Settings menu selects which device drives the control — JOYSTICK or MOUSE. Spinners and trackballs are not affected by this setting.

---

## LED Lighting

VectorPie drives control-panel LEDs natively: a game lights exactly the controls it uses, in its own colors, and a voice guide tours them. Supported controllers: **PacLED64**, **NanoLed**, **PacDrive**, **I-PAC Ultimate I/O**, **LED-Wiz 32** — several at once if needed. Setup is done once, on the cabinet.

### Connecting the board

Using the PacLED64 as the reference:

1. Wire each lamp to the board. An RGB lamp uses three consecutive outputs — R, G, B on n, n+1, n+2.
2. Plug the board into a USB 2.0 port (the black connectors — the blue ones belong to the USB-DVG).
3. Done. The board is detected automatically and LED SETUP shows it as `PACLED64-1  CONFIGURE...`. No drivers, no background service.

### Configuring each output

Open **Settings → LEDS → LED SETUP** and pick the board. The highlighted output flashes on the panel, so you always know which lamp you're on. Per output:

- **MODE** — MONO or RGB (RGB claims the next two outputs).
- **INPUT** — press the control the lamp sits over. The input is what ties the lamp to what that control *does* in each game. A joystick usually has one light and several inputs: press all its directions and they build up as "or" alternatives on the one lamp.
- **TYPE** — what the control is. Get it right — it decides the lamp's family:
  - **BUTTON, JOYSTICK, SPINNER, TRACKBALL** — game controls: lit per game, colored by the game, named by the guide.
  - **COIN** — lit while a game is selected.
  - **START** — special: the running game controls them. A game that drives its own start lights (Space Duel) lights these lamps.
  - **OTHER** — a cabinet lamp no game uses (a menu button). Steady while the menu is up.

Changes apply when you leave the page. A disconnected board shows NOT DETECTED; its outputs page offers REMOVE CONFIG... (asks to confirm).

### Cabinet colors

**CABINET COLORS** colors the buttons no game owns — coins, starts, the menu button. The color attaches to the button's function, not its wiring, so it survives rewiring. Default is red, the lit-button look of the era. For starts the color is the ON color: they light only when the game drives them, dark otherwise — like the original machines.

### Game colors

**GAME COLORS** recolors one game's controls. The colors are statements about the game — fire red, thrust white — so they apply to every revision of the title and stay valid however the panel is wired. Every game ships with colors and a voice tour. The panel mirrors your edits live; RESET TO DEFAULT restores the shipped look.

---

## Wi-Fi Configuration

Wi-Fi is configured directly from the menu — no keyboard or SSH session required.

1. Press **Tab** to open Settings and scroll down to the **NETWORK** section — the Wi-Fi network list scans in the background and updates automatically
2. Use Left/Right on **WIFI NETWORK** to select your network from the scan results
3. Navigate to **WIFI PASSWORD** and press Select to enter edit mode, then enter your password: **Up/Down** cycle the character under the cursor (the last choice is **DEL**, to delete), **Left/Right** move between characters (pressing **Right** past the last character adds a new one), **Select** picks the character and advances to the next (or removes it on **DEL**), and **Cancel** saves and exits editing.
4. Navigate to **CONNECT** and press Select

The current connection status and IP address are shown at the top of the NETWORK section.

---

## Background Music

VectorPie can play background music in the menu in either of two modes, selected from **Settings → MUSIC → TYPE**:

- **THEME** — a single looping track chosen from `.mp3` / `.ogg` files placed in the themes directory. The track is selected from **MUSIC → THEME** in the Settings menu and previews live as you cycle through the choices.
- **PLAYLIST** — every `.mp3` / `.ogg` file found in the playlist directory is added to a playlist that plays in alphabetical order, or in random order when **MUSIC → PLAYLIST SHUFFLE** is enabled. The next track starts automatically when the current one ends — including while the Settings menu is open.

When a game is launched, the current playlist position is preserved. After the game exits the same track resumes from where it left off. The current and next playlist titles are shown beneath the logo on the screensaver.

You can also bulk-load tracks from a USB drive using **Settings → MUSIC → IMPORT ⏏**. A single import handles both the playlist (from the drive's `playlist/` folder) and theme tracks (from the drive's `themes/` folder); whichever folder is missing is simply skipped. Left/Right toggles between **ADD** (merge with the existing tracks) and **REPLACE** (wipe each destination whose source folder is present). REPLACE asks for a second press to confirm.

**Preparing the USB drive:**

1. Format the drive as FAT32, exFAT, or NTFS on your PC/Mac (any common removable-drive format works)
2. Create a `playlist/` folder at the root for playlist tracks, and/or a `themes/` folder at the root for theme tracks
3. Copy your `.mp3` and `.ogg` files into the matching folder — files elsewhere on the drive are ignored
4. Plug the drive into the Pi and run **Settings → MUSIC → IMPORT ⏏**

The same drive can carry a `vectorpie_backup.tar.gz`, a `playlist/` folder, and a `themes/` folder at the same time. A status message on the row reports the result as `ADDED` or `REPLACED` followed by two numbers — *playlist / themes*, e.g. `ADDED 23/4`. If neither folder is present on the drive you get `NO playlist/ OR themes/`; if the folders are present but empty you get `NO TRACKS ON USB`.

| File type | Location |
|---|---|
| Theme music tracks | `/usr/local/share/advance/themes` |
| Playlist tracks | `/persistent/vector_pie_menu_dir/playlist` |

---

## Idle Dimming & Screensaver

| Timeout | Behavior |
|---|---|
| Stop pressing keys | Menu text begins fading immediately and is fully hidden after the MENU FADE duration (default 2.5 seconds, configurable in Settings → DISPLAY); marquee image brightens. Button hints disappear. |
| 60 seconds idle | Screensaver activates: a short attract video plays, then the screensaver video loops as the background |

On the HDMI display the screensaver first plays a short attract video, then loops the screensaver video as the background. When the music playlist is active, the current and next track titles overlay on top. On the USB-DVG the screensaver shows drifting asteroids and a bouncing VectorPie logo. Any control input returns to the menu.

---

## Settings Menu

Press **Tab** to open the settings menu (Tab again, or **Escape**, returns to the game menu). Navigate with Up/Down; adjust values with Left/Right; press Select to toggle or activate.

When USB-DVG is enabled, the settings menu is mirrored on the vector display while it's open, styled to match the game menu (the screensaver no longer takes over the DVG during Settings).

The Settings page is organized into named sections (shown as bold purple headers) — DISPLAY, USB-DVG, AUDIO, MUSIC, EFFECTS, CONTROLS, LEDS, BACKUP, NETWORK, SYSTEM — with the rows below each header.

| Section | Setting | Description |
|---|---|---|
| DISPLAY  | MARQUEE        | Marquee artwork scaling mode: FIT / STRETCH / ZOOM / FIT WIDTH / FIT HEIGHT |
| DISPLAY  | MENU ON HDMI   | When USB-DVG is the primary view, show the game menu on HDMI alongside the vector display. When off, HDMI shows only the marquee (and hint bar if enabled). |
| DISPLAY  | SHOW GAME PREVIEW | When you land on a game, play its preview video (if available at `/usr/local/share/advance/video/<rom>.mp4`) over the marquee. Default on. |
| DISPLAY  | MENU FADE      | How long the menu text takes to fade out once you stop pressing keys: 1.0 s to 15.0 s in 0.5 s steps. Default 2.5 s. |
| DISPLAY  | HINTS          | Show control hints in the scrolling hint bar |
| DISPLAY  | HIGH SCORES    | Show high scores for the selected game in the scrolling hint bar |
| USB-DVG  | DVG            | Enable/disable USB-DVG output |
| USB-DVG  | DVG REFRESH    | Vector display refresh rate: 30–60 Hz in 1 Hz steps. Default 40. Applies immediately as you adjust, so you can watch the picture on the CRT while tuning — raise it for a steadier, brighter picture, lower it if the display can't keep up with complex scenes. |
| USB-DVG  | DVG TYPE       | USB-DVG firmware type fitted on this board: STANDARD (default) or ARCADE CONTROL. Used by USB-DVG UPDATE to check and flash the right firmware. Newer firmware reports its type, in which case this setting is updated automatically on every firmware check. |
| USB-DVG  | USB-DVG UPDATE | Check online for newer USB-DVG firmware (for the variant selected under USB-DVG → DVG TYPE). First press performs the check; when an update is available, a second press downloads, verifies, and flashes the board, then reboots the Pi to bring everything back up cleanly. |
| USB-DVG  | DVG SETTINGS   | Opens its own page and shows the USB-DVG board's own settings menu on the vector display, navigated with the cabinet controls: Up/Down/Left/Right navigate, Select confirms, Cancel goes back in the board menu (at the top level it closes the board menu and saves), Config closes the board menu from any depth (saving) and leaves the page back to VectorPie Settings. The board menu closes by itself after 30 seconds without a press (without saving), and the page follows. Requires USB-DVG firmware 1.14R2 or later — grayed out on older firmware. |
| USB-DVG  | CRT CALIBRATION | Opens its own page showing alignment test patterns on the vector display: Left/Right cycles the patterns, Cancel exits. (Previously entered by pressing Coin from the game menu.) |
| USB-DVG  | OVERLAY        | Controls line artwork and color overlays on supported games (see below). Options: DISABLED / COLORING / ARTWORK / BOTH |
| USB-DVG  | AUTOSTART GAME | Auto-launch the USB-DVG default game on startup |
| AUDIO    | OUTPUT         | Read-only short label of the audio output device in use (`USB`, `HEADPHONE`, `SOUND HAT`, or `DEFAULT` when the device cannot be identified — typically the headphone jack on a Pi 4) |
| AUDIO    | MASTER VOLUME  | System-wide volume (0–100%) |
| AUDIO    | MAME SOUND QUALITY | Preset for AdvanceMAME audio: **HIGH** (48 kHz, full quality), **MEDIUM** (22 kHz, smoother on demanding games), or **LOW** (11 kHz, maximum performance — speech games such as Star Trek sound muffled). Writes coordinated values for `sound_samplerate`, `sound_latency`, and `sound_normalize` in `advmame.rc`. |
| MUSIC    | VOLUME         | Background music volume |
| MUSIC    | TYPE           | Source for background music: **THEME** (single looping track) or **PLAYLIST** (sequential or shuffled play of every track in the music directory). Switching takes effect immediately. |
| MUSIC    | THEME          | Select the looping theme track — choices are populated from `.mp3` and `.ogg` files found in the themes directory. Selection previews live as you cycle. Grayed when TYPE is PLAYLIST. |
| MUSIC    | PLAYLIST SHUFFLE | When on, the playlist plays in random order; when off, in alphabetical order. Grayed when TYPE is THEME. |
| MUSIC    | IMPORT ⏏       | Copy `.mp3` / `.ogg` tracks from a USB drive — the drive's `playlist/` folder feeds the playlist and its `themes/` folder feeds the theme tracks, in a single action. Left/Right toggles between **ADD** (merge) and **REPLACE** (wipe existing tracks first, per side). REPLACE requires a second press to confirm. |
| EFFECTS  | ENABLED        | Enable/disable menu navigation and select sound effects |
| EFFECTS  | VOLUME         | Navigation/select effects volume |
| CONTROLS | ZERO DEADZONE  | Removes the joystick axis deadzone at the kernel level for maximum precision. Required for certain analog controllers such as the Alan-1 Star Wars yoke. |
| CONTROLS | ADSTICK        | Selects which device drives analog stick controls (Star Wars yoke, etc.): JOYSTICK (default) or MOUSE. See Input Mapping for details. |
| CONTROLS | CALIBRATE | The row's value shows which joystick to work on — Left/Right cycles the connected devices, Select opens the calibration page for it (live axis bars on HDMI and the vector display). Calibrating is one step: move all axes to their extremes (yellow ticks mark the sampled range), release everything so the axes rest at center, and press Select to save — the resting position becomes the new center. Fixes off-center rest positions and limited range on analog controllers (Star Wars yoke). Saved and reapplied automatically at every startup; running games pick it up on their next launch. Cancel leaves the page. Grayed when no joystick is connected. |
| CONTROLS | UI CONFIG, UI CANCEL, P1/P2 UP·DOWN·LEFT·RIGHT, P1 BUTTON 1/2, START 1, COIN 1, UI PAUSE | Rebind these AdvanceMAME controls without leaving the menu: highlight one, press Select, then press the key/button/joystick direction (press several to add "or" alternatives), then your Cancel control to save (Escape when rebinding Cancel itself); Left/Right resets to default. UI CONFIG always keeps Tab and UI CANCEL always keeps Escape. Writes to the shared `advmame.rc`, so it applies everywhere. See [Rebinding from the menu](#rebinding-from-the-menu). |
| LEDS     | LED SETUP      | Configure the LED panel: boards are detected, each output flashes on the panel while you set its mode, type, and input. See [LED Lighting](#led-lighting). |
| LEDS     | CABINET COLORS | Colors for the buttons no game owns — coins, starts, menu button. See [Cabinet colors](#cabinet-colors). |
| LEDS     | GAME COLORS    | Recolor a game's controls, live on the panel; applies to all the game's revisions. See [Game colors](#game-colors). |
| BACKUP   | SAVE ⏏         | Saves essential settings to a USB drive — see [Backup Save / Restore](#backup-save--restore) |
| BACKUP   | RESTORE ⏏      | Restores settings from a USB drive backup — see [Backup Save / Restore](#backup-save--restore) |
| NETWORK  | ETHERNET       | Enable or disable the wired Ethernet interface. Disabling forces the Pi to use Wi-Fi even when a cable is plugged in. State persists across reboots. |
| NETWORK  | WIFI           | Enable or disable the Wi-Fi radio. Disabling stops Wi-Fi scanning and disconnects from the current network. State persists across reboots. |
| NETWORK  | HOSTNAME       | The Pi's hostname — edit it to tell cabs apart when more than one is on the network. Press Select to start editing, then: **Up/Down** spin the character under the cursor (only lowercase letters, digits, and hyphens are offered, plus a **DEL** choice), **Left/Right** move between characters (pressing **Right** past the last character adds a new one), **Select** picks the character and advances to the next (or **DEL** to remove it), and **Cancel** saves and exits. The change takes effect **live** — no reboot — and updates the name used with `.local` to SSH or open the network share (e.g. `vectorpie.local`). |
| NETWORK  | CONNECTION     | Read-only — current connection type and SSID (e.g. `WiFi (MyNetwork)` or `Ethernet`) |
| NETWORK  | IP ADDRESS     | Read-only — the Pi's current IP address |
| NETWORK  | WIFI NETWORK   | Pick a Wi-Fi network from the scan results (auto-refreshes in the background). Defaults to the network you're currently on (or last used). |
| NETWORK  | WIFI PASSWORD  | Enter the password for the selected Wi-Fi network — see [Wi-Fi Configuration](#wi-fi-configuration) |
| NETWORK  | CONNECT        | Connect to the selected Wi-Fi network using the entered password. Grayed out when you're already on the highlighted network. |
| SYSTEM   | VECTORPIE UPDATE | Check online for a newer VectorPie version. First press performs the check; when an update is available, a second press downloads, verifies, and applies it, then reboots. See [Software Updates](#software-updates). |
| SYSTEM   | EXIT TO SHELL  | Exits the menu to a Linux command prompt (for advanced users) |

Changes are saved automatically when closing the settings menu.

---

## Backup Save / Restore

When reflashing the SD card with a new VectorPie image, all personalized settings are lost. The backup feature lets you save your settings to a USB drive before reflashing, then restore them on the new image.

### What is backed up

- **advmame.rc** — all input mappings, DVG settings, and audio configuration
- **vector_pie_menu.cfg** — menu preferences (volumes, display toggles, selected theme music, etc.)
- **gamelist.ini** — your customized game list
- **LED settings** — the panel wiring from LED SETUP, the cabinet colors, and the per-game colors
- **Theme music files** — any `.mp3` / `.ogg` tracks you've added to the themes directory

> **Note:** playlist music is *not* part of backups — it lives on the persistent partition (`/persistent/vector_pie_menu_dir/playlist`), which survives software updates in place. When moving to a new SD card, copy the playlist folder over the network share separately.
- **High scores** — all `.hi` and NVRAM files, plus Geometry Wars save files
- **Wi-Fi connections** — saved network credentials
- **Hostname** — the Pi's hostname is restored and re-applied live, so its `.local` name and network-share name carry over after reflashing
- **SSH host keys** — `/etc/ssh/ssh_host_*`, so SSH/PuTTY clients no longer complain about a changed host key after reflashing
- **Pi user password** — your `pi` account password is preserved so you don't need to reset it after reflashing
- **`/boot/firmware/user-config.txt`** — your personal Pi boot overrides (this file is included from `config.txt` so you can edit it freely without conflicting with image updates)
- **`/boot/firmware/cmdline.txt` (partial)** — only the `video=` display-mode token is restored from the backup; the live `root=PARTUUID=…` and other kernel arguments are preserved untouched.

### What is *not* backed up

Bulky content you've added or customized via the network share is **not** included in the backup. If you want this content to survive reflashing or a software update, copy it off the Pi separately:

- **ROMs** you've added — `/usr/local/share/advance/rom/`
- **Sound samples** you've added — `/usr/local/share/advance/sample/`
- **Artwork** you've added or replaced — marquees, manufacturer logos, and overlay artwork, all under `/usr/local/share/advance/artwork/`
- **Game preview videos** you've added — `/usr/local/share/advance/video/`

Shipped ROMs, samples, and artwork that come with VectorPie are part of the base image and are present automatically on a fresh install and after a software update.

### How to use

**Saving (before reflashing):**

1. Plug a USB drive into the Pi
2. Open Settings and select **BACKUP → SAVE ⏏**
3. A `vectorpie_backup.tar.gz` file is created on the USB drive
4. Remove the USB drive and reflash the SD card

**Restoring (after reflashing):**

1. Boot the new VectorPie image and plug in the USB drive
2. Open Settings and select **BACKUP → RESTORE ⏏**
3. Your settings are restored from the backup

The USB drive is auto-detected — any mounted USB drive will work. A status message confirms success or indicates if no USB drive or backup file was found.

---

## Software Updates

VectorPie can update itself in place over the internet — no SD card removal or reflashing needed for normal updates.

### How to update

1. Make sure the Pi is connected to a network (see [Wi-Fi Configuration](#wi-fi-configuration))
2. Open Settings and navigate to **SYSTEM → VECTORPIE UPDATE**
3. Press Select. The row updates to one of:
   - **CHECKING** — contacting the update server
   - **UP TO DATE (*version*)** — you're already on the latest release
   - **UPDATE AVAILABLE: *version*** — a newer version is available
   - **OFFLINE** — the Pi could not reach the update server
4. When an update is available, press Select again to apply it. The row progresses through **DOWNLOADING → VERIFYING → APPLYING**, then the Pi reboots into the new version automatically.

The full update typically takes a 10-15 minutes depending on your network speed and backup size. Keep the Pi powered throughout — the update is safe to interrupt with power loss (the previous version is preserved until the new one boots successfully), but waiting it out is simpler.

### USB-DVG firmware updates

The USB-DVG board's firmware can be updated the same way from **USB-DVG → USB-DVG UPDATE**. The first press checks online (for the variant selected under USB-DVG → DVG TYPE); when an update is available, a second press downloads, verifies, and flashes the board — the vector display goes dark during the flash, and the Pi then reboots automatically so the board comes back up cleanly. After the reboot, USB-DVG UPDATE shows the new version. Only releases newer than the board's installed firmware are offered — the menu never downgrades. Exception: changing USB-DVG → DVG TYPE to the other variant makes USB-DVG UPDATE offer the chosen variant even at the same version, so a board flashed with the wrong variant can be corrected from the menu. A failed flash is always recoverable: the Teensy's bootloader is separate from the firmware, so you can simply try again.

### What carries over

Updates preserve the same files as a USB backup — Wi-Fi credentials, all settings, input mappings, high scores, theme music, customized game list, SSH host keys, and the Pi user password. See [What is backed up](#what-is-backed-up) for the full list. Playlist music carries over too, but by a different mechanism: it lives on the persistent partition (`/persistent/vector_pie_menu_dir/playlist`), untouched by updates, so it doesn't need to be copied at all.

### What does *not* carry over

Updates do not carry over user-added ROMs, samples, artwork, or game preview videos — see [What is *not* backed up](#what-is-not-backed-up) for the full list and locations. **If you've added your own content via the network share, copy it off the Pi (or to another machine on your network) before applying an update.** You can re-add it via the network share afterward.

### If an update fails

VectorPie keeps two copies of itself on the SD card. If a new version fails to boot cleanly, the Pi automatically reverts to the previous version on the next reboot — nothing is lost, and you can retry the update later or skip the failed release.

---

## USB-DVG Support

VectorPie can drive a USB-DVG vector generator board to render the menu on a real vector monitor alongside the marquee display.

The USB-DVG renders the full game menu as vector text, shows manufacturer logos, and runs a drifting asteroids screensaver when idle. It can also query the hardware for a default game to auto-launch on startup.

The USB-DVG is detected automatically when connected and can be enabled or disabled from the Settings menu.

> **Note:** USB-DVG firmware version 1.14R0 or later is required.

### Connecting the USB-DVG

Plug the USB-DVG into one of the **USB 3.0 ports** (the blue connectors) on the Raspberry Pi. Leave the other USB 3.0 port unconnected.

On the **Raspberry Pi 5**, if you are using a USB audio device, connect it to one of the **USB 2.0 ports** (the black connectors) rather than a USB 3.0 port.

Joysticks, keyboards, mice, and zero delay encoders should be connected to the remaining USB 2.0 port, or through a USB hub plugged into it if multiple devices are needed.

### Color Overlays

Many classic black & white vector arcade games originally shipped with a physical plastic color overlay placed directly on the monitor glass to give the illusion of color. VectorPie simulates these overlays digitally by tinting the vectors sent to the USB-DVG, so the games appear on screen with colors faithful to the original arcade experience.

Color overlays are supported for the following games:

| Game | Overlay |
|---|---|
| Armor Attack | Yellow outer border, green play field |
| Asteroids Deluxe | Blue |
| Barrier | Blue |
| Battle Zone | Green play field, red radar screen |
| Bradley Trainer | Green |
| Demon | Zone-based multi-color |
| Meteorites | Yellow |
| Omega Race | Amber/gold |
| Red Baron | Cyan |
| Solar Quest | Red at top, yellow in center, blue elsewhere |
| Star Castle | Concentric rings — blue outer, red, orange, yellow inner |
| Star Hawk | Blue at low intensity, white at high intensity |
| Sundance | Yellow |
| Tail Gunner | Cyan |
| Warrior | Color tinted |

Games that natively output color vectors (Tempest, Major Havoc, Star Wars, Gravitar, Black Widow, Space Duel, Quantum, and all Sega and Cinematronics color titles) are rendered in their original colors and do not use overlays.

Overlays are controlled from **Settings → OVERLAY** with four options: DISABLED (all games render in white), COLORING (color tinting only), ARTWORK (line artwork only), or BOTH (default — full overlay with coloring and line artwork).

### Calibration Mode

Vector monitors may require geometric adjustment to correctly display the image. VectorPie includes a built-in calibration mode that outputs test patterns via the USB-DVG to help align and tune the monitor.

To enter calibration mode, press the Calibration button while the USB-DVG is enabled. Use Left/Right to cycle through the available patterns and press Quit to exit.

| Pattern | Description |
|---|---|
| DIAGONAL BOXES | Crossing diagonal lines across the full screen — useful for checking linearity and geometry |
| CONCENTRIC BOXES | Nested rectangular frames centered on the screen — useful for checking centering and aspect ratio |
| GRID | Evenly spaced horizontal and vertical lines — useful for checking overall linearity and convergence |
| COLOR INTENSITY BARS | Graduated brightness bars in each color channel — useful for checking beam intensity and color balance |

The current pattern name is shown on the marquee display during calibration.

---

## High Scores

The scrolling hint bar at the bottom of the screen displays high scores for the currently selected game. Scores are parsed directly from MAME high score (`.hi`) and NVRAM (`.nv`) save files.

High scores are color-coded for readability: the "HIGH SCORES:" label appears in gold, rank and score values in cyan, and player names in white. Button hints are shown in gray and alternate with the high scores in the scrolling ticker.

Both the high score display and button hints can be individually toggled on or off from the Settings menu.

---

## Launching Games

Pressing Select on a game fades out the music, suspends the display, and launches the game. After the game exits the menu resumes automatically and the music continues — a playlist track resumes from the position it was at when the game launched. Any control mappings or settings changed inside AdvanceMAME take effect immediately.

---

## Troubleshooting

**Do not read or write the SD card directly from a Windows PC or Mac**

Even if your computer can mount the Pi's Linux partitions (modern Windows builds and various third-party tools support ext4), don't edit VectorPie's SD card from a host machine. Files written with the wrong ownership/permissions can break the menu or game launches, and there's no good way to verify state changes without booting the Pi.

To add ROMs, replace artwork, edit `gamelist.ini`, copy music, etc., always go through one of the supported paths instead:

- **Network share** from your PC — `\\<pi-ip-address>\pi` (see [Accessing the Pi from a Windows PC](#accessing-the-pi-from-a-windows-pc))
- **SSH** — `ssh pi@<pi-ip-address>`
- **USB drive** — for music, use **Settings → MUSIC → IMPORT ⏏**; for backup/restore use **Settings → BACKUP → SAVE ⏏ / RESTORE ⏏**

---

**DVG, OVERLAY, AUTOSTART GAME, and MENU ON HDMI are grayed out in Settings**

These options are unavailable because the USB-DVG board is not being detected. Check that the USB-DVG is securely connected to a USB port on the Pi and try again. Once detected, the options will become active.

---

**Using a mouse yoke with an analog joystick also connected (Star Wars, ESB, Lunar Lander)**

Games that use an analog stick control (Star Wars yoke, Lunar Lander thruster) default to using the analog joystick. To use a mouse or yoke controller instead, change **CONTROLS → ADSTICK** to **MOUSE** in the Settings menu. This makes the mouse exclusively drive those controls and the analog joystick will have no effect on them. Spinners, trackballs, and all other controls are unaffected by this setting.

---

**Raspberry Pi intermittently shows Network Install screen instead of booting**

On Raspberry Pi OS Trixie Lite, the Pi 4 or Pi 5 may intermittently display the red/white Network Install recovery screen at power-on instead of booting from the SD card or USB device. This happens even when a valid boot device is present.

The fix is to disable the Network Install feature in the EEPROM configuration:

```
sudo rpi-eeprom-config --edit
```

Add or set:

```
NET_INSTALL_ENABLED=0
```

Save and reboot. The Pi will now boot directly from the local device every time.

---

## Appendix — Low-Level Access

Everything in this appendix is optional: normal use of VectorPie never needs a PC, a shell, or hand-edited files. It's here for adding your own ROMs, artwork, and music, and for customizing the game list.

### Accessing the Pi from a Windows PC

VectorPie includes a pre-configured SSH server and Samba network share, so you can manage files and settings from any Windows PC on the same network without needing a keyboard or monitor connected to the Pi.

To find the Pi's IP address, press **Tab** from the main menu to open Settings and scroll down to the **NETWORK** section — the current IP address is displayed there.

The default login credentials for both the network share and SSH are:

| | |
|---|---|
| **Username** | `pi` |
| **Password** | `raspberry` |

#### Network Share (File Access)

The Pi's files are accessible as a standard Windows network share named **pi**. To connect:

1. Open **File Explorer** on your Windows PC
2. In the address bar, type `\\<pi-ip-address>\pi` and press Enter (e.g. `\\192.168.1.50\pi`)
3. Log in with username `pi` and password `raspberry` when prompted
4. Use the share to add ROMs, replace artwork, or edit the game list

You can also map it as a persistent network drive:
1. Right-click **This PC** in File Explorer and choose **Map network drive**
2. Enter `\\<pi-ip-address>\pi` as the folder path and check **Reconnect at sign-in**

#### SSH (Command Line Access)

SSH lets you open a terminal on the Pi from your Windows PC. Windows 10 and 11 include a built-in SSH client.

1. Find the Pi's IP address in the **NETWORK** section of the VectorPie Settings menu
2. Open **Command Prompt** or **PowerShell** on your PC
3. Type: `ssh pi@<pi-ip-address>` and press Enter
4. Enter the password `raspberry` when prompted

Alternatively, use a free SSH client such as **PuTTY** if you prefer a dedicated application.

### Game List — `gamelist.ini`

The game list is a pipe-delimited text file located at `/usr/local/share/advance/gamelist.ini`. Each line defines one game:

```
MANUFACTURER|DISPLAY NAME|PARENT ROM|CLONE ROM[|COMMAND]
```

| Field | Description |
|---|---|
| MANUFACTURER | Groups games by manufacturer (e.g. `Atari`, `Sega`) |
| DISPLAY NAME | The name shown in the menu |
| PARENT ROM | The AdvanceMAME parent ROM name |
| CLONE ROM | The specific ROM variant to launch |
| COMMAND | (Optional) Custom launcher for non-emulated games |

Example:
```
Atari|Asteroids (rev 2)|asteroid|asteroid
Atari|Tempest (rev 3)|tempest|tempest
Sega|Star Trek|startrek|startrek
```

Games sharing the same parent ROM are grouped as variants and cycled with Left/Right in the menu.

The menu always displays manufacturers, and the games within each, sorted alphabetically by name — the order of lines in `gamelist.ini` doesn't affect the on-screen order. (This keeps the list tidy even though an update inserts newly-added games at the top of the file.)

### Artwork, ROMs and Samples

| File type | Location |
|---|---|
| ROMs | `/usr/local/share/advance/rom` |
| Samples | `/usr/local/share/advance/sample` |
| Marquee artwork | `/usr/local/share/advance/artwork/marquees` |
| Game preview videos | `/usr/local/share/advance/video` |

Artwork images are PNGs. The filename must match the clone ROM name (e.g. `asteroid.png`), with case-insensitive matching so `Asteroid.PNG` also works. If no match is found, the parent ROM name is tried. For marquees, `default.png` is used as a final fallback if neither is found.

Manufacturer logos follow the naming pattern `mfg_<name>.png` (lowercase, spaces as underscores), e.g. `mfg_atari.png`. These are stored in the `artwork/marquees` directory.

#### Per-resolution variants

Any image (marquee or the settings background) may ship a resolution-specific variant alongside the default by inserting the screen height before `.png`: `<name>.<height>.png`. On a 1920×360 marquee panel, `pacman.360.png` is picked first; on a 1080p screen, `pacman.1080.png` is picked first; if no suffixed variant exists, plain `pacman.png` is used. Useful when you want the same cab to drive a wide marquee panel and a normal HDMI monitor with different artwork on each.

#### Vectrex per-cart marquees

Vectrex games run in MESS with the cartridge image loaded from the **fourth field** of `gamelist.ini` (the clone ROM field). For these titles the marquee filename is that field verbatim plus `.png`. For example, a row with clone ROM `lunar.bin` looks up `lunar.bin.png`. Matching is case-insensitive, so `Lunar.BIN.png` also works. If no per-cart marquee is found, `default.png` is used.
