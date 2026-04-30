# VectorPie User Manual

![VectorPie Logo](vector_pie_logo.png)

## Overview

VectorPie is a pre-configured Raspberry Pi image designed to be used with the USB-DVG vector generator board. It can also be used without USB-DVG, displaying on a standard HDMI monitor instead. It boots directly into the game menu and all navigation is done entirely through your arcade controls — no keyboard, mouse, or desktop environment required.

Key features:

- **Headless operation** — runs at boot with no monitor or desktop required on the Pi
- **HDMI-0 marquee display** — the primary HDMI output automatically shows the marquee artwork for the game currently highlighted in the menu
- **USB-DVG support** — drive a real vector monitor via the USB-DVG vector generator board; HDMI output is also supported for use without USB-DVG
- **Unified input mapping** — controls are configured once in AdvanceMAME and apply to the menu and every game, including non-emulated titles
- **High scores** — scrolling hint bar displays high scores parsed directly from MAME save files for the selected game
- **Wi-Fi configuration** — connect to a wireless network directly from the menu
- **Remote access** — built-in SSH server and Windows network share for easy file management from a PC

VectorPie is compatible with the **Raspberry Pi 4** and **Raspberry Pi 5**.

---

## Supported Hardware

- Raspberry Pi 4 (4 GB or 8 GB) or Raspberry Pi 5 (4 GB or 8 GB)
- 32 GB SD card or larger

---

## Download & Installation

### VectorPie Image

1. Download the VectorPie image (approximately 2 GB): [VectorPie Image](https://drive.google.com/file/d/18jwzszuoj5irWUBVCDcp_8WixD3oODGD/view?usp=sharing)
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

> **Note:** Skip this section if your USB-DVG board already has firmware version 1.14R1 or later installed.

1. Download the appropriate USB-DVG firmware `.hex` file:
   - [Standalone Firmware 1.14R1](https://drive.google.com/file/d/1goWkwykACHfnLVW0sk0REXXBPuLQ8ldB/view?usp=drive_link)
   - [Arcade Control Firmware 1.14R1](https://drive.google.com/file/d/1tx1JvYTLceOkbxAkWTCNXsJ22BJQq2xp/view?usp=drive_link)
2. Open **Teensy Loader** and load the `.hex` file via **File → Open HEX File**
3. Press the reset button on the Teensy board — Teensy Loader will detect the board and automatically flash the firmware
4. Once flashing is complete the board resets and is ready to use

---

## Accessing the Pi from a Windows PC

VectorPie includes a pre-configured SSH server and Samba network share, so you can manage files and settings from any Windows PC on the same network without needing a keyboard or monitor connected to the Pi.

To find the Pi's IP address, press **Tab** from the main menu to open Settings and navigate to **NETWORK** — the current IP address is displayed there.

The default login credentials for both the network share and SSH are:

| | |
|---|---|
| **Username** | `pi` |
| **Password** | `raspberry` |

### Network Share (File Access)

The Pi's files are accessible as a standard Windows network share named **pi**. To connect:

1. Open **File Explorer** on your Windows PC
2. In the address bar, type `\\<pi-ip-address>\pi` and press Enter (e.g. `\\192.168.1.50\pi`)
3. Log in with username `pi` and password `raspberry` when prompted
4. Use the share to add ROMs, replace artwork, or edit the game list

You can also map it as a persistent network drive:
1. Right-click **This PC** in File Explorer and choose **Map network drive**
2. Enter `\\<pi-ip-address>\pi` as the folder path and check **Reconnect at sign-in**

### SSH (Command Line Access)

SSH lets you open a terminal on the Pi from your Windows PC. Windows 10 and 11 include a built-in SSH client.

1. Find the Pi's IP address on the VectorPie **Settings → Network** page
2. Open **Command Prompt** or **PowerShell** on your PC
3. Type: `ssh pi@<pi-ip-address>` and press Enter
4. Enter the password `raspberry` when prompted

Alternatively, use a free SSH client such as **PuTTY** if you prefer a dedicated application.

---

## Menu Navigation

All navigation can be done with either a keyboard or arcade controls. Mappings are read from the AdvanceMAME configuration. A scrolling hint bar along the bottom of the screen shows available controls and high scores for the selected game.

> **Note:** A keyboard may be needed initially to configure the button mappings for your arcade controls inside AdvanceMAME. Once mapped, the keyboard is no longer required.

| Action | Description |
|---|---|
| Up / Down | Move between games in the current manufacturer's list |
| Left / Right (on manufacturer header) | Switch to the previous or next manufacturer |
| Left / Right (on a game) | Cycle through ROM variants (revisions, regions, prototypes) |
| Select / Start | Launch the selected game |
| Settings | Cycle through settings and network pages |
| Quit | Request reboot (then press Select to confirm) |
| Coin button | Enter USB-DVG calibration mode (USB-DVG only) |

---

## Menu Layout

The menu shows a vertically scrolling list of games grouped by manufacturer:

- The **manufacturer header** is the top-level entry for each manufacturer. Pressing Left/Right while it is selected switches manufacturers.
- **Game entries** are listed below the header. Games with multiple ROM variants show left/right arrows and can be cycled with Left/Right.
- The **selected item** is displayed full-size in the center with a pulsing highlight. Surrounding entries scale down toward the edges.
- Long game names scroll horizontally within the selection box.

---

## HDMI-0 Marquee Display

The primary HDMI output (HDMI-0) automatically updates to show the marquee artwork for the currently highlighted game. Browsing at the manufacturer level shows the manufacturer's logo. If no artwork exists for a specific game a default image is shown.

The artwork scaling mode (Fit, Stretch, or Zoom) can be changed in the Settings menu.

---

## HDMI-1 Overlay Display

VectorPie supports a second display on HDMI-1 configured to show overlay artwork alongside the vector CRT, so both are visible to the player simultaneously. The overlay image is driven by the Pi's second HDMI output and displays game-specific artwork that complements the vector display — replicating the color overlays used in the original arcade cabinets.

When a game is launched, VectorPie automatically loads the matching overlay image onto the second display. The image remains static for the duration of the session. When the game exits the overlay window is closed.

Overlay lookup follows this order:

1. The clone ROM name is tried first (e.g. `asteroid.png` for the `asteroid` ROM)
2. If not found, the parent ROM name is tried
3. If neither is found, no overlay is shown and the second display is left blank

The image is scaled to fit the display's resolution while preserving aspect ratio (letterboxed or pillarboxed as needed).

---

## Input Mapping

Control mappings are configured **once inside AdvanceMAME** and apply automatically everywhere:

- The VectorPie menu navigation controls
- All AdvanceMAME games
- Games compiled natively for the Pi — these use SDL and read their input mappings directly from the same AdvanceMAME configuration file

To remap controls, launch any AdvanceMAME game and use its input configuration menu. The new mappings take effect in the menu and all games immediately on the next launch.

The menu and the natively-compiled Pi games read the following AdvanceMAME input actions. The **Read by** column shows which programs honor each action — the VectorPie menu, Battle Zone II (`bzone2`), and Geometry Wars (`opengw`):

| AdvanceMAME Action | Menu Function | Default Key | Read by |
|---|---|---|---|
| `ui_up` | Navigate up | Up Arrow | menu |
| `ui_down` | Navigate down | Down Arrow | menu |
| `ui_left` | Navigate left / previous manufacturer | Left Arrow | menu |
| `ui_right` | Navigate right / next manufacturer | Right Arrow | menu |
| `ui_select` | Launch selected game | 1, Enter, or Left Ctrl | menu |
| `ui_configure` | Open settings menu | Tab | menu |
| `ui_pause` | (not used by menu) | P | opengw (pause) |
| `ui_cancel` | Quit / exit | Escape | menu, bzone2, opengw |

### Game Controls

VectorPie games use several types of analog and digital controls. AdvanceMAME automatically maps connected hardware to the appropriate control type for each game. The **Read by** column shows which natively-compiled Pi games honor each action — AdvanceMAME-driven games read all of these regardless.

| Control Type | AdvanceMAME Action | Typical Input | Read by |
|---|---|---|---|
| Directional movement | `p1_up/down/left/right` | Joystick, keyboard arrows | bzone2, opengw (move) |
| Twin-stick aim | `p2_up/down/left/right` | Second joystick, keyboard | opengw (aim) |
| Fire / action | `p1_button1` | Left Ctrl, mouse button, joystick button | bzone2, opengw |
| Player 1 start | `start1` | 1 key | bzone2, opengw |
| Insert coin | `coin1` | 5 key | menu, bzone2, opengw |

Two-player games that use twin sticks use both `p1_` and `p2_` actions for movement and aiming independently.

> **Note:** A mouse and an analog joystick can be used simultaneously. For games that use an analog stick (Star Wars, Empire Strikes Back, Lunar Lander, Tail Gunner, Red Baron), the **CONTROLS → ADSTICK** setting in the Settings menu selects which device drives the control — JOYSTICK or MOUSE. Spinners and trackballs are not affected by this setting.

---

## Wi-Fi Configuration

Wi-Fi is configured directly from the menu — no keyboard or SSH session required.

1. Press **Tab** twice to reach the network page (game menu → settings → network) — the Wi-Fi network list scans in the background and updates automatically
2. Use Left/Right on **WIFI** to select your network from the scan results
3. Navigate to **PASSWORD** and press Select to enter edit mode, then enter your password (Up/Down cycles characters, Left/Right moves the cursor; pressing Right at the end of the string appends a new character). Press Select again to commit, or the settings button to cancel.
4. Navigate to **CONNECT** and press Select

The current connection status and IP address are shown at the top of the network page.

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
| Playlist tracks | `/usr/local/share/advance/playlist` |

---

## Idle Dimming & Screensaver

| Timeout | Behavior |
|---|---|
| 15 seconds idle | Menu text fades out over the next 15 seconds; marquee image brightens. Button hints remain visible. |
| 60 seconds idle | Screensaver activates; VectorPie logo displayed as marquee |

On the HDMI display the screensaver shows a bouncing VectorPie logo. When the music playlist is active, the current and next track titles are displayed beneath the logo. On the USB-DVG the screensaver shows drifting asteroids and a bouncing VectorPie logo. Any control input returns to the menu.

---

## Settings Menu

Press **Tab** to cycle through the settings and network pages (game menu → settings → network → game menu). Press **Escape** from any page to return directly to the game menu. Navigate with Up/Down; adjust values with Left/Right; press Select to toggle or activate.

The Settings page is organized into named sections (shown as bold cyan headers) — DISPLAY, HINT BAR, AUDIO, MUSIC, EFFECTS, CONTROLS, BACKUP, NETWORK, SYSTEM — with the rows below each header.

| Section | Setting | Description |
|---|---|---|
| DISPLAY  | MARQUEE        | Marquee artwork scaling mode: FIT / STRETCH / ZOOM |
| DISPLAY  | DVG            | Enable/disable USB-DVG output |
| DISPLAY  | OVERLAY        | Controls line artwork and color overlays on supported games (see below). Options: DISABLED / COLORING / ARTWORK / BOTH |
| DISPLAY  | AUTOSTART GAME | Auto-launch the USB-DVG default game on startup |
| DISPLAY  | MENU ON HDMI   | When USB-DVG is the primary view, show the game menu on HDMI alongside the vector display. When off, HDMI shows only the marquee artwork. |
| HINT BAR | HINTS          | Show control hints in the scrolling hint bar |
| HINT BAR | HIGH SCORES    | Show high scores for the selected game in the scrolling hint bar |
| AUDIO    | OUTPUT         | Read-only short label of the audio output device in use (`USB`, `HEADPHONE`, or `SOUND HAT`) |
| AUDIO    | MASTER VOLUME  | System-wide volume (0–100%) |
| MUSIC    | VOLUME         | Background music volume |
| MUSIC    | TYPE           | Source for background music: **THEME** (single looping track) or **PLAYLIST** (sequential or shuffled play of every track in the music directory). Switching takes effect immediately. |
| MUSIC    | THEME          | Select the looping theme track — choices are populated from `.mp3` and `.ogg` files found in the themes directory. Selection previews live as you cycle. Grayed when TYPE is PLAYLIST. |
| MUSIC    | PLAYLIST SHUFFLE | When on, the playlist plays in random order; when off, in alphabetical order. Grayed when TYPE is THEME. |
| MUSIC    | IMPORT ⏏       | Copy `.mp3` / `.ogg` tracks from a USB drive — the drive's `playlist/` folder feeds the playlist and its `themes/` folder feeds the theme tracks, in a single action. Left/Right toggles between **ADD** (merge) and **REPLACE** (wipe existing tracks first, per side). REPLACE requires a second press to confirm. |
| EFFECTS  | ENABLED        | Enable/disable menu navigation and select sound effects |
| EFFECTS  | VOLUME         | Navigation/select effects volume |
| CONTROLS | ZERO DEADZONE  | Removes the joystick axis deadzone at the kernel level for maximum precision. Required for certain analog controllers such as the Alan-1 Star Wars yoke. |
| CONTROLS | ADSTICK        | Selects which device drives analog stick controls (Star Wars yoke, etc.): JOYSTICK (default) or MOUSE. See Input Mapping for details. |
| BACKUP   | SAVE ⏏         | Saves essential settings to a USB drive (see below) |
| BACKUP   | RESTORE ⏏      | Restores settings from a USB drive backup (see below) |
| NETWORK  | (page)         | Connection status, IP address, and Wi-Fi configuration — see [Wi-Fi Configuration](#wi-fi-configuration) |
| SYSTEM   | EXIT TO SHELL  | Exits the menu to a Linux command prompt (for advanced users) |

Changes are saved automatically when closing the settings menu.

---

## Backup Save / Restore

When reflashing the SD card with a new VectorPie image, all personalized settings are lost. The backup feature lets you save your settings to a USB drive before reflashing, then restore them on the new image.

### What is backed up

- **advmame.rc** — all input mappings, DVG settings, and audio configuration
- **vector_pie_menu.cfg** — menu preferences (volumes, display toggles, selected theme music, etc.)
- **gamelist.ini** — your customized game list
- **Artwork** — marquee and overlay PNGs (including any you've customized or added)
- **Theme music files** — any `.mp3` / `.ogg` tracks you've added to the themes directory
- **Playlist music files** — any `.mp3` / `.ogg` tracks you've added to the music directory
- **High scores** — all `.hi` and NVRAM files, plus Battle Zone II and Geometry Wars save files
- **Wi-Fi connections** — saved network credentials
- **SSH host keys** — `/etc/ssh/ssh_host_*`, so SSH/PuTTY clients no longer complain about a changed host key after reflashing
- **Pi user password** — your `pi` account password is preserved so you don't need to reset it after reflashing
- **`/boot/firmware/user-config.txt`** — your personal Pi boot overrides (this file is included from `config.txt` so you can edit it freely without conflicting with image updates)

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

## USB-DVG Support

VectorPie can drive a USB-DVG vector generator board to render the menu on a real vector monitor alongside the HDMI-0 marquee display.

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

The current pattern name is shown on the HDMI-0 marquee display during calibration.

---

## High Scores

The scrolling hint bar at the bottom of the screen displays high scores for the currently selected game. Scores are parsed directly from MAME high score (`.hi`) and NVRAM (`.nv`) save files.

High scores are color-coded for readability: the "HIGH SCORES:" label appears in gold, rank and score values in cyan, and player names in white. Button hints are shown in gray and alternate with the high scores in the scrolling ticker.

Both the high score display and button hints can be individually toggled on or off from the Settings menu.

---

## Launching Games

Pressing Select on a game fades out the music, suspends the display, and launches the game. After the game exits the menu resumes automatically and the music continues — a playlist track resumes from the position it was at when the game launched. Any control mappings or settings changed inside AdvanceMAME take effect immediately.

---

## Game List — `gamelist.ini`

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

---

## Artwork, ROMs and Samples

| File type | Location |
|---|---|
| ROMs | `/usr/local/share/advance/rom` |
| Samples | `/usr/local/share/advance/sample` |
| Marquee artwork | `/usr/local/share/advance/artwork/marquees` |
| Overlay artwork | `/usr/local/share/advance/artwork/overlays` |

Artwork images are PNGs. The filename must match the clone ROM name (e.g. `asteroid.png`), with case-insensitive matching so `Asteroid.PNG` also works. If no match is found, the parent ROM name is tried. For marquees, `default.png` is used as a final fallback if neither is found.

Manufacturer logos follow the naming pattern `mfg_<name>.png` (lowercase, spaces as underscores), e.g. `mfg_atari.png`. These are stored in the `artwork/marquees` directory.

### Vectrex per-cart marquees

Vectrex games run in MESS with the cartridge image loaded from the **fourth field** of `gamelist.ini` (the clone ROM field). For these titles the marquee filename is that field verbatim plus `.png`. For example, a row with clone ROM `lunar.bin` looks up `lunar.bin.png`. Matching is case-insensitive, so `Lunar.BIN.png` also works. If no per-cart marquee is found, `default.png` is used.

---

## Troubleshooting

**Do not read or write the SD card directly from a Windows PC or Mac**

Even if your computer can mount the Pi's Linux partitions (modern Windows builds and various third-party tools support ext4), don't edit VectorPie's SD card from a host machine. The running system uses a layered overlay filesystem on top of the base image, so changes made out-of-band may be invisible at runtime, get shadowed by the overlay, or leave the system in an inconsistent state. Files written with the wrong ownership/permissions can also break the menu or game launches.

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
