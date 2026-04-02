#!/usr/bin/env bash
# =============================================================================
# install.sh – Set up the BLE-MQTT bridge on the Raspberry Pi gateway
# =============================================================================
# Run as a regular user (not root).  Sudo is invoked where needed.
#
# Usage:
#   chmod +x install.sh
#   ./install.sh
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVICE_NAME="ble-mqtt-bridge"
SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}.service"
PYTHON_BIN="$(which python3)"
BRIDGE_SCRIPT="${SCRIPT_DIR}/ble_mqtt_bridge.py"

echo "============================================="
echo "  QPG6200 BLE IoT Demo – Bridge Installer"
echo "============================================="
echo ""

# ---------------------------------------------------------------------------
# Step 1: Install Python dependencies
# ---------------------------------------------------------------------------
echo "[1/4] Installing Python dependencies ..."
pip3 install --user -r "${SCRIPT_DIR}/requirements.txt"
echo "      Done."
echo ""

# ---------------------------------------------------------------------------
# Step 2: Make the bridge script executable
# ---------------------------------------------------------------------------
echo "[2/4] Setting script permissions ..."
chmod +x "${BRIDGE_SCRIPT}"
echo "      Done."
echo ""

# ---------------------------------------------------------------------------
# Step 3: Install a systemd service so the bridge starts on boot
# ---------------------------------------------------------------------------
echo "[3/4] Installing systemd service (${SERVICE_NAME}) ..."

CURRENT_USER="$(whoami)"
PYTHON_USER_BIN="${HOME}/.local/bin"

sudo tee "${SERVICE_FILE}" > /dev/null <<EOF
[Unit]
Description=QPG6200 BLE IoT Demo – BLE to MQTT Gateway Bridge
After=network-online.target bluetooth.target mosquitto.service
Wants=network-online.target

[Service]
Type=simple
User=${CURRENT_USER}
Environment="PATH=${PYTHON_USER_BIN}:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
ExecStart=${PYTHON_BIN} ${BRIDGE_SCRIPT} --broker localhost --port 1883
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

sudo systemctl daemon-reload
sudo systemctl enable "${SERVICE_NAME}"
echo "      Service installed and enabled."
echo ""

# ---------------------------------------------------------------------------
# Step 4: Start the service now
# ---------------------------------------------------------------------------
echo "[4/4] Starting ${SERVICE_NAME} ..."
sudo systemctl restart "${SERVICE_NAME}"
sleep 2
sudo systemctl status "${SERVICE_NAME}" --no-pager
echo ""

echo "============================================="
echo "  Installation complete!"
echo ""
echo "  Useful commands:"
echo "    Check status : sudo systemctl status ${SERVICE_NAME}"
echo "    View logs    : journalctl -u ${SERVICE_NAME} -f"
echo "    Stop bridge  : sudo systemctl stop ${SERVICE_NAME}"
echo "    Restart      : sudo systemctl restart ${SERVICE_NAME}"
echo "============================================="
