# Cross-compile the AVR32DB28 firmware.
build:
	@bash ci/build.sh

# Build and run the host test suite against the mocked hardware platform.
test:
	@bash ci/test.sh

# Flash the firmware to the board over UPDI. The port defaults to COM3 on Windows and
# /dev/ttyUSB0 elsewhere; override it with e.g. 'make flash PORT=COM4'.
flash: build
	@bash ci/flash.sh

# Format all C/C++ files in place.
format:
	@bash ci/format.sh

# Check formatting without modifying any files; fails if something isn't formatted.
format-check:
	@bash ci/format.sh --check

# Remove all build artifacts.
clean:
	@rm -rf build
	@$(MAKE) -C test clean

.PHONY: build test flash format format-check clean
