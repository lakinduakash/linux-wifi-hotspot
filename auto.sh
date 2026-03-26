#!/usr/bin/env bash

set -euo pipefail

WIFI_IFACE="${1:-wlp61s0}"
SSID="${2:-MyAccessPoint}"
PASSPHRASE="${3:-12345678}"

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  cat <<'EOF'
Usage:
  start-hotspot-auto-channel.sh [wifi_iface] [ssid] [passphrase] [--dry-run]

Examples:
  ./start-hotspot-auto-channel.sh
  ./start-hotspot-auto-channel.sh wlp61s0 MyAccessPoint 12345678
  ./start-hotspot-auto-channel.sh wlp61s0 MyAccessPoint 12345678 --dry-run
EOF
  exit 0
fi

if [[ ${#PASSPHRASE} -lt 8 || ${#PASSPHRASE} -gt 63 ]]; then
  echo "Error: passphrase must be 8..63 characters."
  exit 1
fi

DRY_RUN="false"
for arg in "$@"; do
  if [[ "$arg" == "--dry-run" ]]; then
    DRY_RUN="true"
  fi
done

if ! command -v iw >/dev/null 2>&1; then
  echo "Error: 'iw' is required but not installed."
  exit 1
fi

if ! command -v create_ap >/dev/null 2>&1; then
  echo "Error: 'create_ap' is required but not installed/in PATH."
  exit 1
fi

CHANNEL="$(iw dev "$WIFI_IFACE" info 2>/dev/null | awk '/channel/ {print $2; exit}')"

if [[ -z "$CHANNEL" ]]; then
  echo "Error: Could not detect current channel from interface '$WIFI_IFACE'."
  echo "Make sure it is connected to Wi-Fi first."
  exit 1
fi

CMD=(
  sudo pkexec --user root
  create_ap
  -c "$CHANNEL"
  "$WIFI_IFACE" "$WIFI_IFACE"
  "$SSID" "$PASSPHRASE"
  -w 2
)

echo "Detected channel: $CHANNEL"
echo "Command: ${CMD[*]}"

if [[ "$DRY_RUN" == "true" ]]; then
  exit 0
fi

sudo create_ap --stop "$WIFI_IFACE" >/dev/null 2>&1 || true
"${CMD[@]}"
