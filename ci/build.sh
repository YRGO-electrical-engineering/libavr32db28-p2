#!/usr/bin/env bash
#
# Cross-compile the AVR32DB28 firmware.
#
# The AVR-Dx device family pack (DFP) supplies the device specs and the <avr/io.h> header for
# this part; avr-libc does not ship them. Point DFP_DIR at an unpacked pack, or let the script
# fall back to the local Atmel Studio installation.
#
# Usage:
#   ci/build.sh
#   DFP_DIR=/path/to/AVR-Dx_DFP ci/build.sh
set -euo pipefail

# Root directory (resolved to an absolute path, since this script cd's around).
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Target device.
MCU="avr32db28"

# Output directory and ELF file.
BUILD_DIR="$ROOT_DIR/build"
TARGET="$BUILD_DIR/main.elf"

# CPU frequency in Hz. The OSCHF reset default; see avr32db28-registers.md section 2.
F_CPU=4000000UL

# Fallback locations of the device family pack on a local Atmel/Microchip Studio installation.
# Both the WSL and the Git Bash spelling of the Windows path are tried, and any pack version, so
# an installed Studio is found whichever shell this runs in.
LOCAL_DFPS=(
    "/mnt/c/Program Files (x86)/Atmel/Studio/7.0/packs/atmel/AVR-Dx_DFP"/*
    "/c/Program Files (x86)/Atmel/Studio/7.0/packs/atmel/AVR-Dx_DFP"/*
)

################################################################################
# Print the directory holding the AVR-Dx device family pack, or terminate the
# script if no pack can be found.
# Globals:
#   ROOT_DIR
#   LOCAL_DFPS
# Arguments:
#   None
################################################################################
resolve_dfp_dir() {
    local candidate

    for candidate in "${DFP_DIR:-}" "$ROOT_DIR/dfp" "${LOCAL_DFPS[@]}"
    do
        if [[ -n "$candidate" && -d "$candidate/gcc/dev/$MCU" ]]
        then
            echo "$candidate"
            return 0
        fi
    done

    echo "error: AVR-Dx device family pack not found." >&2
    echo "       Set DFP_DIR to an unpacked pack, or download one with:" >&2
    echo "       wget http://packs.download.atmel.com/Atmel.AVR-Dx_DFP.1.10.114.atpack" >&2
    echo "       unzip -q -d dfp Atmel.AVR-Dx_DFP.1.10.114.atpack" >&2
    exit 1
}

################################################################################
# Terminate the script if avr-gcc is not installed.
# Globals:
#   None
# Arguments:
#   None
################################################################################
check_avr_gcc() {
    if ! command -v avr-gcc &> /dev/null
    then
        echo "error: avr-gcc not found. Install it, e.g. 'sudo apt -y install gcc-avr'." >&2
        exit 1
    fi
}

################################################################################
# Find the firmware source files and store them in the given array. The mocked
# hardware platform in source/arch/test is excluded: it exists only for the host
# test suite and does not compile for the target.
# Globals:
#   ROOT_DIR
# Arguments:
#   $1 - Name of the array variable to populate with file paths.
################################################################################
select_sources() {
    local -n out=$1
    mapfile -t out < <(find source -name "*.c" -not -path "*/arch/test/*" | sort)
    out+=("main.c")
}

# Navigate to the root directory.
cd "$ROOT_DIR"

# Check that the toolchain and the device pack are available.
check_avr_gcc
DFP="$(resolve_dfp_dir)"

# Select the firmware sources.
select_sources SOURCES

# Build the firmware.
mkdir -p "$BUILD_DIR"
echo "Building $MCU firmware with DFP $DFP"
avr-gcc -mmcu="$MCU" -B "$DFP/gcc/dev/$MCU" -I "$DFP/include" -I include \
    -Os -std=c11 -Wall -Wextra -Werror -DF_CPU="$F_CPU" \
    -o "$TARGET" "${SOURCES[@]}"

# Generate a flashable hex file and report the size.
avr-objcopy -O ihex -R .eeprom "$TARGET" "$BUILD_DIR/main.hex"
avr-size "$TARGET"
