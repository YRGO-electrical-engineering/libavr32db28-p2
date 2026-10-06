#!/usr/bin/env bash
#
# Flash the firmware to an AVR32DB28 over UPDI with avrdude.
#
# A serial UPDI adapter is assumed, i.e. a USB-to-serial cable wired to the UPDI pin. The port
# defaults to COM3 on Windows and /dev/ttyUSB0 elsewhere; set PORT to use another one. The hex
# file defaults to the one produced by ci/build.sh, falling back to a Microchip Studio build;
# set HEX_FILE to flash something else.
#
# Usage:
#   ci/flash.sh
#   PORT=COM4 ci/flash.sh
#   HEX_FILE=build/main.hex ci/flash.sh
set -euo pipefail

# Root directory (resolved to an absolute path, since this script cd's around).
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Target device, as avrdude names it.
MCU="avr32db28"

# Programmer and baud rate for a serial UPDI adapter.
PROGRAMMER="serialupdi"
BAUD_RATE="115200"

################################################################################
# Print the serial port to flash over: PORT if set, otherwise the usual first
# port on this operating system.
# Globals:
#   PORT
#   OSTYPE
# Arguments:
#   None
################################################################################
resolve_port() {
    if [[ -n "${PORT:-}" ]]
    then
        echo "$PORT"
    elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]
    then
        echo "COM3"
    else
        echo "/dev/ttyUSB0"
    fi
}

################################################################################
# Print the hex file to flash, or terminate the script if no hex file is found.
# HEX_FILE is used if set, otherwise the output of ci/build.sh is preferred over
# a Microchip Studio build.
# Globals:
#   HEX_FILE
# Arguments:
#   None
################################################################################
resolve_hex_file() {
    local candidate

    for candidate in "${HEX_FILE:-}" "build/main.hex" \
        "Debug/libavr32db28_p2.hex" "Release/libavr32db28_p2.hex"
    do
        if [[ -n "$candidate" && -f "$candidate" ]]
        then
            echo "$candidate"
            return 0
        fi
    done

    echo "error: no hex file found. Run 'make build' first, or set HEX_FILE." >&2
    exit 1
}

################################################################################
# Terminate the script if avrdude is not installed.
# Globals:
#   None
# Arguments:
#   None
################################################################################
check_avrdude() {
    if ! command -v avrdude &> /dev/null
    then
        echo "error: avrdude not found. Install it, e.g. 'sudo apt -y install avrdude'." >&2
        exit 1
    fi
}

# Navigate to the root directory.
cd "$ROOT_DIR"

# Check that avrdude is available, then select the port and the hex file.
check_avrdude
SERIAL_PORT="$(resolve_port)"
HEX="$(resolve_hex_file)"

# Flash the firmware.
echo "Flashing $HEX to $MCU on $SERIAL_PORT"
avrdude -c "$PROGRAMMER" -p "$MCU" -P "$SERIAL_PORT" -b "$BAUD_RATE" -U "flash:w:$HEX:i"
