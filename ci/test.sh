#!/usr/bin/env bash
#
# Build and run the host test suite.
#
# The suite compiles the C drivers against the mocked hardware platform in source/arch/test
# instead of the real AVR registers, so it runs natively on the host.
#
# Usage:
#   ci/test.sh
set -euo pipefail

# Root directory (resolved to an absolute path, since this script cd's around).
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Directory holding the yrgo::test framework.
FRAMEWORK_DIR="$ROOT_DIR/libs/test"

# Test suite directory.
TEST_DIR="$ROOT_DIR/test"

# Terminate the script if the test framework submodule hasn't been checked out.
if [[ ! -f "$FRAMEWORK_DIR/Makefile" ]]
then
    echo "error: test framework not found in libs/test. Run 'git submodule update --init'." >&2
    exit 1
fi

# Build and run the test suite.
cd "$TEST_DIR"
make clean build run
