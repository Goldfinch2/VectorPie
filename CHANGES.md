
<!-- TODO: update filename -->
vectorpie-<VERSION>.img.gz:

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


vectorpie-1.0-15-gdd32520.img.gz:

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


Older versions:

# Pi 5 "mame" — Custom Changes vs Stock Raspberry Pi OS Trixie Lite

**Date:** April 4, 2026  
**Host:** mame  
**OS:** Raspberry Pi OS Trixie Lite 64-bit  
**Kernel:** 6.12.75+rpt-rpi-2712  

---

## /boot/firmware/config.txt

### Stock Trixie Lite 64-bit defaults (Pi 5 section)

```
dtparam=audio=on
auto_initramfs=1
camera_auto_detect=1
display_auto_detect=1
disable_fw_kms_setup=1
arm_64bit=1
disable_overscan=1
arm_boost=1
max_framebuffers=2

[cm4]
otg_mode=1

[cm5]
dtoverlay=dwc2,dr_mode=host

[pi4]
dtoverlay=vc4-kms-v3d

[pi5]
dtoverlay=vc4-kms-v3d
```

### Custom additions in [pi5] section

| Setting | Value | Purpose |
|---------|-------|---------|
| `noaudio` param on dtoverlay | `dtoverlay=vc4-kms-v3d,noaudio` | Disables HDMI audio via DRM driver (audio handled separately) |
| `usb_max_current_enable` | `1` | Allows USB ports to draw more current (up to 1.2A total) |

### Custom additions in [pi4] section

| Setting | Value | Purpose |
|---------|-------|---------|
| `noaudio` param on dtoverlay | `dtoverlay=vc4-kms-v3d,noaudio` | Disables HDMI audio via DRM driver |

### Custom additions in [all] section

| Setting | Value | Purpose |
|---------|-------|---------|
| `disable_splash` | `1` | Hides the rainbow splash screen at boot |
| `enable_uart` | `1` | Enables serial console on GPIO pins |
| `hdmi_force_hotplug` | `1` | Forces HDMI output even if no display detected |
| `hdmi_group` | `1` | CEA (TV) display mode group |
| `hdmi_mode` | `16` | 1080p @ 60Hz |
| `hdmi_ignore_edid_audio` | `1` | Ignores audio capabilities reported by display EDID |
| `dtparam=sd_poll_once` | `off` | Keeps SD card polling active (default is on) |

---

## /boot/firmware/cmdline.txt

### Stock Trixie Lite default

```
console=serial0,115200 console=tty1 root=PARTUUID=xxxxx-02 rootfstype=ext4 fsck.repair=yes rootwait quiet splash plymouth.ignore-serial-consoles
```

### Custom additions / changes

| Setting | Value | Purpose |
|---------|-------|---------|
| `loglevel` | `0` | Suppresses all kernel messages on boot (stock has no loglevel set) |
| `logo.nologo` | — | Hides the Raspberry Pi logo in top-left corner during boot |
| `video` | `HDMI-A-1:1920x1080@60D` | Forces specific HDMI resolution (stock relies on EDID autodetect) |
| `cpufreq.default_governor` | `performance` | Locks CPU to max frequency (stock uses ondemand/schedutil) |
| `usbcore.autosuspend` | `-1` | Disables USB autosuspend (prevents USB devices from sleeping) |
| `reboot` | `f,w` | Forces firmware-level hardware reset on reboot |

---

## EEPROM Configuration

### Stock Trixie Lite default (Pi 5)

```
[all]
BOOT_UART=0
POWER_OFF_ON_HALT=0
BOOT_ORDER=0xf461
```

Note: Stock defaults may vary depending on EEPROM version shipped with the board.

### Current custom configuration

```
[all]
BOOT_ORDER=0xf41
NET_INSTALL_ENABLED=0
```

All other EEPROM settings left at defaults.

### Changes from stock

| Setting | Stock | Custom | Purpose |
|---------|-------|--------|---------|
| `BOOT_ORDER` | `0xf461` | `0xf41` | Removed NVMe (6) from boot order; USB → SD → retry |
| `NET_INSTALL_ENABLED` | `1` (default) | `0` | **Disables network install recovery screen — fixes intermittent boot failure** |

---

