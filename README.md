# VectorPie User Manual

![VectorPie Logo](vector_pie_logo.png)

## Overview

VectorPie is a Raspberry Pi image for the USB-DVG vector generator board. It boots straight into a game menu driven entirely from the arcade controls — no keyboard, mouse, or desktop. It also works without a USB-DVG, on an HDMI monitor.

- **Headless** — runs at boot, no monitor or desktop needed on the Pi
- **Marquee display** — the HDMI output shows the artwork for the highlighted game
- **USB-DVG** — drives a real vector monitor; HDMI works without it
- **One input mapping** — configured once in AdvanceMAME, used by the menu and every game
- **High scores** — read from the MAME save files and shown in the hint bar
- **LED lighting** — each game lights its own controls in its own colors, with a voice-guided tour and beat-synced light shows
- **Wi-Fi** — set up from the menu
- **Remote access** — SSH and a Windows network share
- **Updates from the menu** — with automatic rollback if the new version fails to boot

Runs on the **Raspberry Pi 4** and **Raspberry Pi 5**.

---

## Supported Hardware

- Raspberry Pi 4 or 5, 1 GB or more. Raspberry Pi 5 recommended for better performance.
- 32 GB SD card or larger. (Pi 5) M.2 NVMe HAT and M.2 NVMe SSD recommended for performance and reliability.

---

## Download & Installation

### VectorPie Image

1. Download the image (about 2 GB): [VectorPie Image](https://drive.google.com/file/d/1LUsRHiiyYHzvATRZeuvclLhPZ3dwjVR8/view?usp=sharing)
2. Write it to the SD card with **balenaEtcher** ([download](https://etcher.balena.io/)): **Flash from file** → the `.img.gz` → **Select target** → **Flash!**
3. Insert the card and power on. VectorPie boots into the game menu.

### USB-DVG Firmware

Two variants:

- **Standalone** — any cabinet or custom controls
- **Arcade Control** — for the adapter card that connects to the original harness of an Asteroids, Asteroids Deluxe, Space Duel, Tempest, or Black Widow cabinet, so its buttons, CRT and coin inputs work natively

| USB-DVG Version | Use |
|---|---|
| v1 / v2 | Standalone only |
| v3 or later (standalone) | Standalone |
| v3 or later (on adapter card) | Arcade Control |

> On VectorPie 1.1.2 or later the firmware is updated from the menu (Settings → USB-DVG → USB-DVG UPDATE). The manual procedure below is for older versions only.

Flash with **Teensy Loader** ([download](https://www.pjrc.com/teensy/loader.html)):

1. Download the `.hex`: [Standalone 1.14R1](https://drive.google.com/file/d/1goWkwykACHfnLVW0sk0REXXBPuLQ8ldB/view?usp=drive_link) or [Arcade Control 1.14R1](https://drive.google.com/file/d/1tx1JvYTLceOkbxAkWTCNXsJ22BJQq2xp/view?usp=drive_link)
2. **File → Open HEX File** in Teensy Loader
3. Press the reset button on the Teensy; it flashes and resets

---

## Menu Navigation

Keyboard or arcade controls, as mapped in AdvanceMAME. The hint bar at the bottom scrolls the available controls and the selected game's high scores.

> A keyboard may be needed once, to map the arcade controls inside AdvanceMAME.

| Action | Description |
|---|---|
| Up / Down | Move between games |
| Left / Right (on manufacturer header) | Previous / next manufacturer |
| Left / Right (on a game) | Cycle ROM variants (revisions, regions, prototypes) |
| Select (tap) | Launch the game |
| Select (hold ~1 s) | CONTROL GUIDE: each of the game's controls blinks alone on the panel while a voice names it. Ends with INSERT COIN and PRESS START; any input stops it. Needs a configured LED panel — without one, holding just launches |
| Player 2 Start | Next music track |
| Settings | Open the settings menu |
| Quit | Reboot (Select confirms) |

---

## Menu Layout

Games are listed by manufacturer:

- The **manufacturer header** is the top entry; Left/Right on it switches manufacturer.
- **Games** are listed below it. A game with several ROM variants shows arrows; Left/Right cycles them.
- The **selected item** is full-size in the center with a pulsing highlight; the others shrink toward the edges. Long names scroll.

---

## Marquee Display

The HDMI output shows the highlighted game's marquee, or the manufacturer's logo on a header. A game without artwork gets a default image. Scaling (Fit, Stretch, Zoom) is set in Settings → DISPLAY.

---

## Input Mapping

Controls are mapped **once, in AdvanceMAME**, and used everywhere: the menu, every AdvanceMAME game, and the games built natively for the Pi. To remap, launch any AdvanceMAME game and use its input configuration menu; the change applies on the next launch.

> With a USB-DVG, AdvanceMAME's in-game menus — including input configuration — are mirrored on the vector display, so you can rebind while watching the monitor. Bindings show there in short form (`J:BTN1`, `LCTRL`, `P:1` = keypad 1); HDMI shows the full names.

### Rebinding from the menu

**Settings → CONTROLS → CONFIGURE INPUTS...** rebinds the common controls without a keyboard or a game: UI CONFIG and UI CANCEL first, then the player controls (P1/P2 directions, fire, start, coin, pause), each with its binding.

- Highlight a control, press **Select**, then press the key, button, or stick direction. The binding builds up live (`p` then `6` gives `p or 6`).
- Press several inputs for "or" alternatives.
- Press **Cancel** to finish the capture (**Escape** when rebinding Cancel itself). A capture left alone finishes after 20 seconds.
- **Left / Right** resets a control to its default.
- UI CANCEL always keeps Escape and UI CONFIG always keeps Tab, so you can't lock yourself out.

Nothing is written until **SAVE AND EXIT**; leaving with unsaved changes asks first. The bindings go to the AdvanceMAME configuration, so they apply everywhere.

The menu and the native Pi games read these actions. **Read by** shows who honors each — the menu and Geometry Wars (`opengw`):

| AdvanceMAME Action | Menu Function | Default Key | Read by |
|---|---|---|---|
| `ui_up` | Up | Up Arrow | menu |
| `ui_down` | Down | Down Arrow | menu |
| `ui_left` | Left / previous manufacturer | Left Arrow | menu |
| `ui_right` | Right / next manufacturer | Right Arrow | menu |
| `ui_select` | Launch | 1, Enter, or Left Ctrl | menu |
| `ui_configure` | Settings | Tab | menu |
| `ui_pause` | (not used by menu) | P | opengw (pause) |
| `ui_cancel` | Quit / exit | Escape | menu, opengw |
| `p2_start` | Next track | 2 | menu |

### Game Controls

AdvanceMAME maps the connected hardware to each game's control types. **Read by** shows which native Pi games honor each action; AdvanceMAME games read all of them.

| Control Type | AdvanceMAME Action | Typical Input | Read by |
|---|---|---|---|
| Directional movement | `p1_up/down/left/right` | Joystick, arrows | opengw (move) |
| Twin-stick aim | `p2_up/down/left/right` | Second joystick, R/F/D/G | opengw (aim) |
| Fire / action | `p1_button1` | Left Ctrl, mouse button, joystick button | opengw |
| Player 1 start | `start1` | 1 | opengw |
| Insert coin | `coin1` | 5 | menu, opengw |

> A mouse and an analog joystick can both be connected. For analog-stick games (Star Wars, Empire Strikes Back, Lunar Lander, Tail Gunner, Red Baron), **CONTROLS → ADSTICK** picks which drives the control — JOYSTICK or MOUSE. Spinners and trackballs are unaffected.

---

## LED Lighting

VectorPie drives the control-panel LEDs itself: a game lights the controls it uses, in its own colors, and a voice guide tours them. Boards: **PacLED64**, **NanoLed**, **PacDrive**, **I-PAC Ultimate I/O**, **LED-Wiz 32** — several at once if needed. Setup is done once, on the cabinet.

### Connecting the board

1. Wire each lamp to the board. An RGB lamp uses three consecutive outputs — R, G, B on n, n+1, n+2.
2. Plug the board into a USB 2.0 port (black; the blue ones are for the USB-DVG).
3. The board is detected automatically and LED SETUP lists it. No drivers, no service.

### Configuring each output

**Settings → LEDS → LED SETUP** is one page. BOARD picks the board, OUTPUT walks its outputs — the one you land on lights on the panel. Per output:

- **MODE** — MONO or RGB. RGB takes the next two outputs, so it is offered only on a trio boundary (1, 4, 7…), and not on a PacDrive (on/off outputs). A MONO lamp on a board with levels shows any color as an intensity (0.3 R + 0.6 G + 0.1 B): red dim, green medium, white full.
- **LED STRIP SIZE** — NanoLed only: the number of LEDs in the strip or ring under the control (a spinner ring). The strip is one lamp, lit one LED after another.
- **BRIGHTNESS** — dims a lamp that outshines the others. Not offered on an on/off board.
- **TYPE** — what the control is:
  - **BUTTON, JOYSTICK, SPINNER, TRACKBALL** — game controls: lit per game, colored by the game, named by the guide
  - **COIN** — lit while a game is selected
  - **START** — driven by the running game (Space Duel lights its own)
  - **OTHER** — a cabinet lamp no game uses (a menu button); steady while the menu is up
- **INPUT** — press the control the lamp sits over. A joystick has one lamp and several inputs: press all its directions. SPINNER and TRACKBALL fill in their axis by themselves; turn the control to override it.

Nothing is written until **SAVE AND EXIT**; leaving with unsaved changes asks first. **CLEAR CONFIG** wipes the board's outputs and colors (press twice). A board that is configured but not connected shows in red; clearing it is all that page offers.

### Cabinet colors

**CABINET COLORS** colors the lamps no game owns — coins, starts, the menu button. OUTPUT picks the lamp (it lights alone); TYPE says what it is to the cabinet (coin1, start1, the menu button, other). The color follows that, so it survives rewiring. Default is red for all. Starts light only when the game drives them. **RESET TO DEFAULT** restores the defaults (press twice).

### Game colors

**GAME COLORS** recolors one game's controls — fire red, thrust white — for every revision of the title, however the panel is wired. Every game ships with colors and a voice tour. OUTPUT picks the lamp (it lights alone, in its color); TYPE is its wiring type, ROLE what it does in that game (fire, rotate…); **RESET TO DEFAULT** restores the shipped look (press twice).

Every color picker offers the same twelve colors, plus black for off.

### Light shows

**LED SHOW** animates the whole panel. **ANIMATION** picks one of four:

| Animation | Plays |
|---|---|
| **SCREENSAVER** | during the screensaver — on the beat of the music, or on a fixed interval |
| **STARTUP** | once, as the menu comes up |
| **LAUNCH** | once, as a game starts |
| **MANUFACTURER** | looping while the cursor is on a manufacturer |

An animation is a list of frames; a frame is which lamps are lit, in what color. **FRAMES** sets how many (up to 32), **INTERVAL** how long each is held. The screensaver animation also has a **MODE**: **BEAT** steps one frame per beat of the music (see [Beat maps](#beat-maps)), **INTERVAL** steps on the clock.

**EDIT FRAMES...** builds the frames lamp by lamp, with the panel showing the frame as you go. Pick the **FRAME** and its **EFFECT** — hold, strobe, fade in, fade out, or random — then:

- **LAMP** walks every lamp on the cabinet by address; while the cursor is on LAMP or COLOR the picked lamp shows alone on the panel (blinking white if it isn't lit), and `*` marks the ones already in the frame. Select puts it in, in **COLOR**, or takes it out. The other rows show the whole frame.
- **COLOR** is the twelve colors and **RANDOM** (a fresh color each time the frame plays). Changing it recolors the picked lamp; on a PacDrive lamp it just reads **ON**.
- **CLEAR FRAME** turns every lamp off.

Nothing is written until **SAVE AND EXIT**; leaving with unsaved changes asks first. **CLEAR ANIMATION** deletes every frame (press twice).

---

## Wi-Fi Configuration

1. Open Settings and go to **NETWORK** — networks are scanned in the background
2. Left/Right on **WIFI NETWORK** picks yours
3. Select on **WIFI PASSWORD** to edit: **Up/Down** cycles the character under the cursor (the last choice, **DEL**, deletes it), **Left/Right** moves along (Right past the end adds a character), **Select** confirms it, **Cancel** saves
4. Select **CONNECT**

Connection status and IP address are shown at the top of the section.

---

## Background Music

**Settings → MUSIC → TYPE** picks the mode:

- **THEME** — one looping track from the themes directory, chosen with **MUSIC → THEME**; it previews as you cycle.
- **PLAYLIST** — every track in the playlist directory, in alphabetical or shuffled order (**PLAYLIST SHUFFLE**). The next track starts when one ends, even with Settings open.

A launched game pauses the playlist; it resumes where it was when the game exits. The current and next titles show on the screensaver.

**Player 2 Start skips to the next track** — in the menu (and starts the screensaver), and during the screensaver without waking it. A DJ break can't be skipped.

### Beat maps

The screensaver light show follows the beat once each track has a **beat map**. **Settings → MUSIC → GENERATE BEAT MAPS...** makes them, about half a minute per track; **Cancel** stops it and a later run picks up where it left off. A track without a map gets the breathing show.

A map is a `.beats` file beside its track. Importing from USB copies any already on the drive.

### Importing from USB

**Settings → MUSIC → IMPORT ⏏** copies `.mp3` / `.ogg` tracks from a drive's `playlist/` folder into the playlist and from its `themes/` folder into the themes; a missing folder is skipped. Left/Right picks **ADD** (keep what's there, add the rest) or **REPLACE** (empty the destination first). REPLACE asks for a second press.

Prepare the drive:

1. Format it FAT32, exFAT, or NTFS
2. Create `playlist/` and/or `themes/` at the root
3. Copy the `.mp3` / `.ogg` files in; files elsewhere are ignored
4. Plug it in and run the import

The drive can carry a `vectorpie_backup.tar.gz` too. The row reports `ADDED 23/4` (playlist / themes), `NO playlist/ OR themes/`, or `NO TRACKS ON USB`.

| File type | Location |
|---|---|
| Theme tracks | `/usr/local/share/advance/themes` |
| Playlist tracks | `/persistent/vector_pie_menu_dir/playlist` |

---

## Idle Dimming & Screensaver

| Timeout | Behavior |
|---|---|
| Stop pressing keys | Menu text fades over the MENU FADE time (default 2.5 s); the marquee brightens; hints disappear |
| 60 seconds idle | Screensaver: a short attract video, then the screensaver video loops |

The HDMI screensaver shows the current and next track titles, and that Player 2 Start skips. The USB-DVG shows drifting asteroids and a bouncing VectorPie logo. With an LED panel, the SCREENSAVER light show plays (see [Light shows](#light-shows)). Any control other than Player 2 Start returns to the menu.

---

## Settings Menu

**Tab** opens Settings; Tab or **Escape** closes it. Up/Down moves, Left/Right adjusts, Select toggles or opens. With a USB-DVG, Settings is mirrored on the vector display.

Sections: DISPLAY, USB-DVG, AUDIO, MUSIC, EFFECTS, CONTROLS, LEDS, BACKUP, NETWORK, SYSTEM.

| Section | Setting | Description |
|---|---|---|
| DISPLAY  | MARQUEE        | Marquee scaling: FIT / STRETCH / ZOOM / FIT WIDTH / FIT HEIGHT |
| DISPLAY  | MENU ON HDMI   | With a USB-DVG, also show the game menu on HDMI. Off: HDMI shows only the marquee (and hint bar). |
| DISPLAY  | SHOW GAME PREVIEW | Play a game's preview video (`/usr/local/share/advance/video/<rom>.mp4`) over the marquee when you land on it. Default on. |
| DISPLAY  | MENU FADE      | Fade-out time for the menu text: 1.0–15.0 s. Default 2.5 s. |
| DISPLAY  | HINTS          | Control hints in the hint bar |
| DISPLAY  | HIGH SCORES    | High scores in the hint bar |
| USB-DVG  | DVG            | USB-DVG output on/off |
| USB-DVG  | DVG REFRESH    | Refresh rate, 30–60 Hz. Default 40. Applies as you adjust — raise for a steadier, brighter picture, lower if the display can't keep up. |
| USB-DVG  | DVG TYPE       | Firmware variant on the board: STANDARD or ARCADE CONTROL. Used by USB-DVG UPDATE. Newer firmware reports it and this follows. |
| USB-DVG  | USB-DVG UPDATE | Check online for newer firmware (for DVG TYPE). Second press downloads, verifies, flashes, and reboots. |
| USB-DVG  | DVG SETTINGS   | The board's own settings menu on the vector display: Up/Down/Left/Right navigate, Select confirms, Cancel goes back (at the top level, closes and saves), Config closes and saves from anywhere. Closes by itself after 30 s without a press. Needs firmware 1.14R2 or later. |
| USB-DVG  | CRT CALIBRATION | Alignment patterns on the vector display: Left/Right cycles them, Cancel exits. |
| USB-DVG  | OVERLAY        | Color overlays: DISABLED / COLORING / ARTWORK / BOTH (see below) |
| USB-DVG  | AUTOSTART GAME | Launch the USB-DVG default game at startup |
| AUDIO    | OUTPUT         | The audio device in use: `USB`, `HEADPHONE`, `SOUND HAT`, or `DEFAULT` |
| AUDIO    | MASTER VOLUME  | System volume |
| AUDIO    | MAME SOUND QUALITY | AdvanceMAME audio: **HIGH** (48 kHz), **MEDIUM** (22 kHz, smoother on demanding games), **LOW** (11 kHz, maximum performance; speech games sound muffled) |
| MUSIC    | VOLUME         | Music volume |
| MUSIC    | TYPE           | **THEME** or **PLAYLIST** |
| MUSIC    | THEME          | The looping theme track; previews as you cycle. Grayed with PLAYLIST. |
| MUSIC    | PLAYLIST SHUFFLE | Random or alphabetical order. Grayed with THEME. |
| MUSIC    | IMPORT ⏏       | Copy tracks from a USB drive — `playlist/` and `themes/` folders. Left/Right: **ADD** or **REPLACE**; REPLACE asks twice. |
| MUSIC    | GENERATE BEAT MAPS... | Make the beat maps the screensaver light show follows. Cancel stops and keeps what's done. See [Beat maps](#beat-maps). |
| EFFECTS  | ENABLED        | Navigation and select sounds |
| EFFECTS  | VOLUME         | Their volume |
| CONTROLS | ZERO DEADZONE  | Removes the joystick deadzone at the kernel level. Needed by some analog controllers (Alan-1 Star Wars yoke). |
| CONTROLS | ADSTICK        | Which device drives analog-stick games: JOYSTICK or MOUSE. |
| CONTROLS | CALIBRATE      | Left/Right picks the joystick, Select opens its calibration page. Move all axes to their extremes, let go, press Select — the resting position becomes center. Reapplied at every startup. Grayed with no joystick. |
| CONTROLS | CONFIGURE INPUTS... | Rebind the AdvanceMAME controls from the menu. See [Rebinding from the menu](#rebinding-from-the-menu). |
| LEDS     | LED SETUP      | Wire the LED panel. See [LED Lighting](#led-lighting). |
| LEDS     | CABINET COLORS | Colors for the coin, start and menu lamps. See [Cabinet colors](#cabinet-colors). |
| LEDS     | GAME COLORS    | Recolor a game's controls. See [Game colors](#game-colors). |
| LEDS     | LED SHOW...    | Light shows for the screensaver, startup, game launch and manufacturers. See [Light shows](#light-shows). |
| BACKUP   | SAVE ⏏         | Save settings to a USB drive. See [Backup Save / Restore](#backup-save--restore). |
| BACKUP   | RESTORE ⏏      | Restore them from the drive. |
| NETWORK  | ETHERNET       | Wired interface on/off. Off forces Wi-Fi even with a cable in. |
| NETWORK  | WIFI           | Wi-Fi radio on/off. |
| NETWORK  | HOSTNAME       | The Pi's name on the network, edited like the Wi-Fi password (lowercase, digits, hyphens). Applies live, including the `.local` name. |
| NETWORK  | CONNECTION     | Current connection and SSID |
| NETWORK  | IP ADDRESS     | Current IP address |
| NETWORK  | WIFI NETWORK   | Pick a network from the scan. |
| NETWORK  | WIFI PASSWORD  | Its password. See [Wi-Fi Configuration](#wi-fi-configuration). |
| NETWORK  | CONNECT        | Connect. Grayed when already on that network. |
| SYSTEM   | VECTORPIE UPDATE | Check online for a newer VectorPie. Second press downloads, verifies, applies, and reboots. See [Software Updates](#software-updates). |
| SYSTEM   | EXIT TO SHELL  | Leave the menu for a Linux prompt |

Changes are saved when Settings closes.

---

## Backup Save / Restore

Reflashing the SD card loses your settings. Save them to a USB drive first, then restore on the new image.

### What is backed up

- **advmame.rc** — input mappings, DVG and audio settings
- **vector_pie_menu.cfg** — menu preferences
- **gamelist.ini** — your game list
- **LED settings** — panel wiring, cabinet colors, game colors, light shows
- **Theme tracks** you've added
- **High scores** — `.hi` and NVRAM files, Geometry Wars saves
- **Wi-Fi connections**
- **Hostname** — reapplied live, so the `.local` and share names carry over
- **SSH host keys** — so SSH clients don't complain after reflashing
- **Pi user password**
- **`/boot/firmware/user-config.txt`** — your boot overrides (included from `config.txt`, so it never conflicts with updates)
- **`/boot/firmware/cmdline.txt`** — the `video=` token only

### What is *not* backed up

Bulky content added over the network share. Copy it off the Pi separately if it must survive a reflash:

- **ROMs** — `/usr/local/share/advance/rom/`
- **Samples** — `/usr/local/share/advance/sample/`
- **Artwork** — marquees, manufacturer logos, overlays, under `/usr/local/share/advance/artwork/`
- **Game preview videos** — `/usr/local/share/advance/video/`
- **Playlist tracks and beat maps** — `/persistent/vector_pie_menu_dir/playlist/`, which survives updates in place but is not in the backup

Shipped ROMs, samples, and artwork are part of the image.

### How to use

**Save:** plug in a USB drive, **BACKUP → SAVE ⏏**. It writes `vectorpie_backup.tar.gz`.

**Restore:** boot the new image, plug in the drive, **BACKUP → RESTORE ⏏**.

Any mounted USB drive works. The row reports the result.

---

## Software Updates

VectorPie updates itself over the internet — no reflashing.

### How to update

1. Connect to a network (see [Wi-Fi Configuration](#wi-fi-configuration))
2. **SYSTEM → VECTORPIE UPDATE**, press Select: **CHECKING**, then **UP TO DATE**, **UPDATE AVAILABLE: *version***, or **OFFLINE**
3. Press Select again to apply: **DOWNLOADING → VERIFYING → APPLYING**, then the Pi reboots into the new version

An update takes 10–15 minutes. Power loss during it is safe — the previous version stays until the new one boots — but let it finish.

### USB-DVG firmware updates

**USB-DVG → USB-DVG UPDATE** works the same way for the board's firmware (for the DVG TYPE variant): check, then download, verify and flash — the vector display goes dark during the flash — then reboot. Only newer firmware is offered, except that changing DVG TYPE offers the other variant at the same version, so a board flashed with the wrong one can be corrected. A failed flash is recoverable: the Teensy bootloader is separate, so try again.

### What carries over

Everything a USB backup holds — see [What is backed up](#what-is-backed-up). The playlist carries over too: it lives on the persistent partition, untouched by updates.

### What does *not* carry over

User-added ROMs, samples, artwork, and preview videos — see [What is *not* backed up](#what-is-not-backed-up). **Copy them off the Pi before updating** and put them back afterward.

### If an update fails

VectorPie keeps two copies of itself. If the new one fails to boot, the Pi reverts to the previous on the next reboot. Retry later or skip the release.

---

## USB-DVG Support

The USB-DVG renders the menu as vector text on a real vector monitor, with manufacturer logos and an asteroids screensaver, alongside the marquee on HDMI. It is detected automatically and can be turned off in Settings. It can also name a default game to launch at startup.

> Firmware 1.14R0 or later is required.

### Connecting the USB-DVG

Plug it into a **USB 3.0 port** (blue). Leave the other USB 3.0 port empty. On a **Pi 5**, a USB audio device goes in a USB 2.0 port (black). Joysticks, keyboards, mice and encoders go in the remaining USB 2.0 port, through a hub if needed.

### Color Overlays

Many black-and-white vector games shipped with a colored plastic overlay on the monitor. VectorPie tints the vectors instead:

| Game | Overlay |
|---|---|
| Armor Attack | Yellow border, green play field |
| Asteroids Deluxe | Blue |
| Barrier | Blue |
| Battle Zone | Green play field, red radar |
| Bradley Trainer | Green |
| Demon | Zone-based multi-color |
| Meteorites | Yellow |
| Omega Race | Amber |
| Red Baron | Cyan |
| Solar Quest | Red top, yellow center, blue elsewhere |
| Star Castle | Rings — blue outer, red, orange, yellow inner |
| Star Hawk | Blue at low intensity, white at high |
| Sundance | Yellow |
| Tail Gunner | Cyan |
| Warrior | Tinted |

Games with native color (Tempest, Major Havoc, Star Wars, Gravitar, Black Widow, Space Duel, Quantum, the Sega and Cinematronics color titles) use their own colors.

**Settings → OVERLAY**: DISABLED (all white), COLORING (tint only), ARTWORK (line artwork only), BOTH (default).

### Calibration Mode

**Settings → USB-DVG → CRT CALIBRATION** puts test patterns on the vector monitor for geometry adjustment. Left/Right cycles them, Cancel exits. The pattern name shows on the marquee.

| Pattern | For |
|---|---|
| DIAGONAL BOXES | Linearity and geometry |
| CONCENTRIC BOXES | Centering and aspect ratio |
| GRID | Linearity and convergence |
| COLOR INTENSITY BARS | Beam intensity and color balance |

---

## High Scores

The hint bar shows the selected game's high scores, read from its MAME `.hi` and `.nv` save files: the label in gold, ranks and scores in cyan, names in white, alternating with the control hints in gray. Both can be turned off in Settings → DISPLAY.

---

## Launching Games

Select fades the music, hands over the display, and launches the game. When it exits the menu resumes, and a playlist track picks up where it was. Mappings or settings changed inside AdvanceMAME apply at once.

---

## Troubleshooting

**Don't read or write the SD card from a PC or Mac**

Even if the computer can mount the Pi's Linux partitions, don't. Files written with the wrong ownership can break the menu or game launches. Use the supported paths instead:

- **Network share** — `\\<pi-ip-address>\pi` (see [Accessing the Pi from a Windows PC](#accessing-the-pi-from-a-windows-pc))
- **SSH** — `ssh pi@<pi-ip-address>`
- **USB drive** — music through **MUSIC → IMPORT ⏏**, settings through **BACKUP → SAVE ⏏ / RESTORE ⏏**

---

**DVG, OVERLAY, AUTOSTART GAME and MENU ON HDMI are grayed out**

The USB-DVG isn't detected. Check its USB connection.

---

**A mouse yoke and an analog joystick are both connected (Star Wars, ESB, Lunar Lander)**

Analog-stick games use the joystick by default. Set **CONTROLS → ADSTICK** to **MOUSE** to make the mouse drive them instead. Spinners, trackballs and everything else are unaffected.

---

**The Pi sometimes shows the Network Install screen instead of booting**

On Raspberry Pi OS Trixie Lite the red/white Network Install screen can appear at power-on even with a valid SD card. Disable it in the EEPROM:

```
sudo rpi-eeprom-config --edit
```

Add or set:

```
NET_INSTALL_ENABLED=0
```

Save and reboot.

---

## Appendix — Low-Level Access

Optional: normal use never needs a PC, a shell, or hand-edited files. This is for adding your own ROMs, artwork and music, and customizing the game list.

### Accessing the Pi from a Windows PC

VectorPie runs an SSH server and a Samba share. The Pi's IP address is in Settings → NETWORK.

| | |
|---|---|
| **Username** | `pi` |
| **Password** | `raspberry` |

#### Network Share (File Access)

In File Explorer's address bar type `\\<pi-ip-address>\pi` (e.g. `\\192.168.1.50\pi`) and log in. To keep it: right-click **This PC** → **Map network drive**, enter the path, check **Reconnect at sign-in**.

#### SSH (Command Line Access)

From Command Prompt or PowerShell: `ssh pi@<pi-ip-address>`, password `raspberry`. PuTTY works too.

### Game List — `gamelist.ini`

`/usr/local/share/advance/gamelist.ini`, one game per line, pipe-delimited:

```
MANUFACTURER|DISPLAY NAME|PARENT ROM|CLONE ROM[|COMMAND]
```

| Field | Description |
|---|---|
| MANUFACTURER | Groups games (`Atari`, `Sega`) |
| DISPLAY NAME | Shown in the menu |
| PARENT ROM | AdvanceMAME parent ROM |
| CLONE ROM | The variant to launch |
| COMMAND | Optional custom launcher for non-emulated games |

```
Atari|Asteroids (rev 2)|asteroid|asteroid
Atari|Tempest (rev 3)|tempest|tempest
Sega|Star Trek|startrek|startrek
```

Games sharing a parent ROM are variants, cycled with Left/Right. The menu sorts manufacturers and games alphabetically whatever the file order.

### Artwork, ROMs and Samples

| File type | Location |
|---|---|
| ROMs | `/usr/local/share/advance/rom` |
| Samples | `/usr/local/share/advance/sample` |
| Marquees | `/usr/local/share/advance/artwork/marquees` |
| Game preview videos | `/usr/local/share/advance/video` |

Artwork is PNG, named after the clone ROM (`asteroid.png`, case-insensitive), then the parent ROM, then `default.png`. Manufacturer logos are `mfg_<name>.png` (lowercase, underscores for spaces) in the marquees directory.

#### Per-resolution variants

Any image can have a variant per screen height: `<name>.<height>.png`. On a 1920×360 marquee panel `pacman.360.png` is used first, on a 1080p screen `pacman.1080.png`, otherwise `pacman.png`.

#### Vectrex per-cart marquees

Vectrex games load the cartridge named in the clone ROM field; the marquee is that name plus `.png` (`lunar.bin` → `lunar.bin.png`, case-insensitive), falling back to `default.png`.
