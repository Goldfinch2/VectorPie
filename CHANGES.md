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

