#!/bin/bash
# =============================================================================
# Pi Image Cleanup Script
# Run this before creating a distributable SD card image with PiShrink
# =============================================================================

set -e

# Must be run as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo $0"
    exit 1
fi

echo "=============================="
echo "  Pi Pre-Image Cleanup Script"
echo "=============================="
echo ""

# Confirm before proceeding
read -p "This will permanently delete sensitive and build data. Continue? (y/N) " confirm
if [[ "$confirm" != "y" && "$confirm" != "Y" ]]; then
    echo "Aborted."
    exit 0
fi

HOME_DIR="/home/pi"
ROOT_HOME="/root"

echo ""
echo "[1/12] Clearing shell histories..."
# pi user
rm -f "$HOME_DIR/.bash_history"
rm -f "$HOME_DIR/.zsh_history"
rm -f "$HOME_DIR/.ash_history"
rm -f "$HOME_DIR/.python_history"
rm -f "$HOME_DIR/.node_repl_history"
rm -f "$HOME_DIR/.wget-hsts"
# root user
rm -f "$ROOT_HOME/.bash_history"
rm -f "$ROOT_HOME/.zsh_history"
rm -f "$ROOT_HOME/.python_history"
rm -f "$ROOT_HOME/.node_repl_history"
rm -f "$ROOT_HOME/.wget-hsts"
# Symlink .bash_history to /dev/null so bash's in-memory history (from this
# still-running session) gets discarded on logout rather than re-written to disk.
ln -sf /dev/null "$HOME_DIR/.bash_history"
ln -sf /dev/null "$ROOT_HOME/.bash_history"
# Also clear history from .bash_logout for login shells (belt-and-suspenders).
echo 'history -c' >> "$HOME_DIR/.bash_logout"
echo 'history -c' >> "$ROOT_HOME/.bash_logout"
echo "      Done."

echo "[2/12] Removing advmame.rc and regenerating default config..."
rm -f "$HOME_DIR/.advance/advmame.rc"
sudo -u pi /usr/local/bin/advmame &>/dev/null || true
chown pi:pi "$HOME_DIR/.advance/advmame.rc"
echo "      Done."

echo "[3/12] Removing Git credentials, .netrc, and config secrets..."
rm -f "$HOME_DIR/.git-credentials"
rm -f "$HOME_DIR/.netrc"
rm -f "$ROOT_HOME/.git-credentials"
rm -f "$ROOT_HOME/.netrc"
# Optionally strip credential helper from git config (leaves other settings intact)
if [ -f "$HOME_DIR/.gitconfig" ]; then
    git config --file "$HOME_DIR/.gitconfig" --unset credential.helper 2>/dev/null || true
fi
echo "      Done."

echo "[4/12] Removing SSH keys and known hosts..."
rm -f "$HOME_DIR/.ssh/id_rsa"
rm -f "$HOME_DIR/.ssh/id_rsa.pub"
rm -f "$HOME_DIR/.ssh/id_ed25519"
rm -f "$HOME_DIR/.ssh/id_ed25519.pub"
rm -f "$HOME_DIR/.ssh/known_hosts"
rm -f "$HOME_DIR/.ssh/authorized_keys"
rm -f "$ROOT_HOME/.ssh/id_rsa"
rm -f "$ROOT_HOME/.ssh/id_rsa.pub"
rm -f "$ROOT_HOME/.ssh/id_ed25519"
rm -f "$ROOT_HOME/.ssh/id_ed25519.pub"
rm -f "$ROOT_HOME/.ssh/known_hosts"
rm -f "$ROOT_HOME/.ssh/authorized_keys"
echo "      Done."

echo "[5/12] Cleaning build artifacts and temp files..."
# Remove common build dirs left in home
rm -rf "$HOME_DIR/build" "$HOME_DIR/tmp" "$HOME_DIR/src"
# APT cache
apt-get clean -y
rm -rf /var/lib/apt/lists/*
# Temp files
rm -rf /tmp/* /var/tmp/*
echo "      Done."

echo "[6/12] Removing logs..."
# System logs
find /var/log -type f -name "*.log" -exec truncate -s 0 {} \;
find /var/log -type f -name "*.gz" -delete
find /var/log -type f -name "*.1" -delete
journalctl --rotate --vacuum-time=1s 2>/dev/null || true
# Advance logs and dat files
rm -f /usr/local/share/advance/*.log
rm -f /usr/local/share/advance/*.dat
rm -f /usr/local/share/advance/category.ini
echo "      Done."

echo "[7/12] Clearing Python/pip cache and thumbnail cache..."
rm -rf "$HOME_DIR/.cache"
rm -rf "$ROOT_HOME/.cache"
echo "      Done."

echo "[8/12] Removing WiFi credentials and DHCP leases..."
# NetworkManager connections (Pi OS Bookworm)
rm -f /etc/NetworkManager/system-connections/*.nmconnection
# DHCP leases
rm -f /var/lib/dhcp/*.leases
rm -f /var/lib/dhcpcd5/*.lease
echo "      Done."

echo "[9/12] Resetting machine ID..."
# Empty machine-id so a new one is generated on first boot
truncate -s 0 /etc/machine-id
rm -f /var/lib/dbus/machine-id
ln -s /etc/machine-id /var/lib/dbus/machine-id 2>/dev/null || true
echo "      Done."

echo "[10/12] Fixing ownership on advancemame installed files..."
chown -R pi:pi /usr/local/bin/
chown -R pi:pi /usr/local/share/advance
chown -R pi:pi /usr/local/doc/advance
chown -R pi:pi /usr/local/man/man1/
echo "      Done."

echo "[11/12] Removing non-hidden subfolders under /home/pi..."
find "$HOME_DIR" -mindepth 1 -maxdepth 1 -type d -not -name '.*' -exec rm -rf {} +
echo "      Done."

echo "[12/12] Regenerate SSH host keys on first boot..."
# Remove host keys and create a one-shot service to regenerate them on first boot
rm -f /etc/ssh/ssh_host_*
cat > /etc/systemd/system/regenerate-ssh-keys.service <<'UNIT'
[Unit]
Description=Regenerate SSH host keys
Before=ssh.service
ConditionPathExistsGlob=!/etc/ssh/ssh_host_*

[Service]
Type=oneshot
ExecStart=/usr/bin/ssh-keygen -A

[Install]
WantedBy=multi-user.target
UNIT
systemctl enable regenerate-ssh-keys.service
echo "      Done."

echo ""
echo "=============================="
echo "  Cleanup complete!"
echo ""
echo "  Recommended next steps:"
echo "  1. Shut down:   sudo shutdown -h now"
echo "  2. On your PC:  sudo pishrink.sh -a -z pi.img pi_clean.img.gz"
echo "=============================="