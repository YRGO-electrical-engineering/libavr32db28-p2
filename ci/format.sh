#!/usr/bin/env bash
#
# Format (or check the formatting of) all C/C++ files using clang-format.
#
# Usage:
#   ci/format.sh          Format files in place.
#   ci/format.sh --check  Fail if any file is not already formatted.
set -euo pipefail

# Root directory.
ROOT_DIR="$(dirname "${BASH_SOURCE[0]}")/.."

################################################################################
# Terminate the script if clang-format is not installed.
# Globals:
#   None
# Arguments:
#   None
################################################################################
check_clang_format() {
    if ! command -v clang-format &> /dev/null
    then
        echo "error: clang-format not found. Install it, e.g. 'sudo apt -y install clang-format'." >&2
        exit 1
    fi
}

################################################################################
# Find C/C++ files and store them in the given array. Two directories are skipped: libs, which
# holds submodules that carry their own formatting configuration, and dfp, which holds the
# vendor device family pack downloaded by the CI pipeline.
# Globals:
#   None
# Arguments:
#   $1 - Name of the array variable to populate with file paths.
################################################################################
select_files() {
    local -n out=$1
    mapfile -t out < <(find * -path "libs/*" -prune -o -path "dfp/*" -prune -o \
        \( -name "*.h" -o -name "*.hpp" -o -name "*.c" -o -name "*.cpp" \) -print)
}

################################################################################
# Format the given files in place or check their formatting.
# Globals:
#   None
# Arguments:
#   $1 - Run command, e.g. --check. Empty string formats files in place.
#   $2 - Name of the array variable containing files to format.
################################################################################
format_files() {
    local arg="$1"
    local -n files=$2

    # Nothing to do if no files were found.
    if [[ ${#files[@]} -eq 0 ]]
    then
        echo "No C/C++ files found."
        return
    fi

    # Format selected files.
    if [[ "${arg:-}" == "--check" ]]
    then
        echo "Checking ${#files[@]} file(s)."
        clang-format --dry-run --Werror "${files[@]}"
    else
        local count=0
        for file in "${files[@]}"
        do
            before=$(md5sum "$file")
            clang-format -i "$file"
            after=$(md5sum "$file")
            if [[ "$before" != "$after" ]]
            then
                echo "Formatted: $file"
                ((++count))
            fi
        done
        echo "Formatted $count file(s)."
    fi
}

# Navigate to the root directory.
cd "$ROOT_DIR"

# Check if clang-format is installed.
check_clang_format

# Select files to format.
select_files FILES

# Format selected files.
format_files "${1:-}" FILES
