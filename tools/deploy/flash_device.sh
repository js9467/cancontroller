#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_PACKAGE="$SCRIPT_DIR/../../dist/latest"
PACKAGE_PATH="${PACKAGE_PATH:-$DEFAULT_PACKAGE}"
PORT="${PORT:-}"
BAUD="${BAUD:-921600}"
ESPTOOL_BIN="${ESPTOOL:-esptool.py}"
LIST_ONLY=false

resolve_path() {
    python3 - "$1" <<'PY'
import os, sys
print(os.path.abspath(sys.argv[1]))
PY
}

usage() {
    echo "Usage: $0 [-p PORT] [-d PACKAGE_DIR] [-l]" >&2
    exit 1
}

while getopts ":p:d:l" opt; do
    case "$opt" in
        p) PORT="$OPTARG" ;;
        d) PACKAGE_PATH="$OPTARG" ;;
        l) LIST_ONLY=true ;;
        *) usage ;;
    esac
done

PACKAGE_PATH="$(resolve_path "$PACKAGE_PATH")"

list_ports() {
    ls /dev/tty.usb* /dev/cu.usb* /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || true
}

detect_port() {
    local candidates
    mapfile -t candidates < <(list_ports)
    if [[ ${#candidates[@]} -gt 0 ]]; then
        echo "${candidates[0]}"
    fi
}

if [[ "$LIST_ONLY" == "true" ]]; then
    list_ports
    exit 0
fi

if [[ ! -d "$PACKAGE_PATH" ]]; then
    echo "Package directory '$PACKAGE_PATH' not found" >&2
    exit 2
}

BOOTLOADER="$PACKAGE_PATH/bootloader.bin"
PARTITIONS="$PACKAGE_PATH/partitions.bin"
BOOT_APP0="$PACKAGE_PATH/boot_app0.bin"
FIRMWARE="$PACKAGE_PATH/firmware.bin"

for artifact in "$BOOTLOADER" "$PARTITIONS" "$BOOT_APP0" "$FIRMWARE"; do
    if [[ ! -f "$artifact" ]]; then
        echo "Missing artifact: $artifact" >&2
        exit 3
    fi
done

if [[ -z "$PORT" ]]; then
    PORT="$(detect_port || true)"
fi

if [[ -z "$PORT" ]]; then
    echo "Unable to detect serial port. Pass -p /dev/ttyACM0" >&2
    exit 4
fi

echo "Using $PORT"
"$ESPTOOL_BIN" \
    --chip esp32s3 \
    --port "$PORT" \
    --baud "$BAUD" \
    --before default_reset \
    --after hard_reset \
    write_flash \
    --flash_mode qio \
    --flash_size 16MB \
    0x0 "$BOOTLOADER" \
    0x8000 "$PARTITIONS" \
    0xe000 "$BOOT_APP0" \
    0x10000 "$FIRMWARE"

echo "Flash complete"
