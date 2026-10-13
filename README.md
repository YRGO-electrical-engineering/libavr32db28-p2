# AVR32DB28 Drivers for P2
Digital and analog I/O, display and timer drivers for the AVR32DB28.

## About
A small, plain set of drivers that students build on in the course *Programmeringsmetodik*. They
assume no previous experience with microcontrollers: each header exposes a handful of ordinary C
functions, with no registers, bit masks or pointers. The register work lives in the `.c` files,
commented for anyone curious enough to open them.

The drivers are kept simple on purpose:
* **No interrupts.** Everything is polled, so a program runs in the order of its statements.
* **No structs in the API.** A pin is an enumerated name.
* **No extras.** Pull-ups, alternate pin mappings and clock tuning are left out.

The [AVR32DB28 data sheet](https://www.microchip.com/en-us/product/AVR32DB28) covers what the
hardware can do beyond this.

---

## Drivers

| Driver  | Header                                                 | Reads                        | Writes                       |
| ------- | ------------------------------------------------------ | ---------------------------- | ---------------------------- |
| Digital | [include/driver/digital.h](./include/driver/digital.h) | Whether a pin is high        | High, low or inverted        |
| Analog  | [include/driver/analog.h](./include/driver/analog.h)   | A voltage, from 0 to 4095    | A voltage, from 0 to 4095    |
| Display | [include/driver/display.h](./include/driver/display.h) | The number being shown       | A number from 0 to 999       |
| Timer   | [include/driver/timer.h](./include/driver/timer.h)     | Whether a timeout has passed | Started, stopped or inverted |

Both pin drivers share the pin and mode names in
[include/driver/gpio.h](./include/driver/gpio.h):
* **Digital:** all 13 pins, `GPIO_PIN_PC0` - `GPIO_PIN_PC3`, `GPIO_PIN_PD1` - `GPIO_PIN_PD7` and
  `GPIO_PIN_PF0` - `GPIO_PIN_PF1`.
* **Analog:** inputs on `GPIO_PIN_PD1` - `GPIO_PIN_PD7`, `GPIO_PIN_PF0` and `GPIO_PIN_PF1`. Only
  `GPIO_PIN_PD6` can be an output.

I/O port A belongs to the three digit 7-segment display, which also reserves one of the three
timers.

[docs/drivers.md](./docs/drivers.md) describes every function.

---

## Usage
Each pin is initialized once, as an input or an output, and then used from the main loop. Lighting
an LED while a button is held down is a complete program:

```c
#include "driver/digital.h"

#define LED GPIO_PIN_PC0
#define BUTTON GPIO_PIN_PF0

int main(void)
{
    if (!digital_init(LED, GPIO_MODE_OUT)) { return -1; }
    if (!digital_init(BUTTON, GPIO_MODE_IN)) { return -1; }

    while (1)
    {
        digital_write(LED, digital_read(BUTTON));
    }
}
```

Inputs have no pull-up, so the button needs a resistor holding its pin low while it is released.

The analog driver works the same way, with values from 0 to 4095. An output voltage following a
potentiometer:

```c
if (!analog_init(GPIO_PIN_PD1, GPIO_MODE_IN)) { return -1; }
if (!analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT)) { return -1; }

while (1)
{
    analog_write(GPIO_PIN_PD6, analog_read(GPIO_PIN_PD1));
}
```

The display takes a number. Its digits are lit one at a time, so `display_update()` has to be
called every lap of the loop. This shows 37.2:

```c
if (!display_init()) { return -1; }
display_write(372U);
display_show_dp(true);

while (1)
{
    display_update();
}
```

The `main.c` in this repository counts on the display while the joystick is held: up or left
counts up, down or right counts down, five steps a second, wrapping around at 0 and 999. Pressing
SW1 starts over from zero.

---

## Getting started in Microchip Studio
Microchip Studio needs no toolchain setup and no command line.

1. **Get the code.** Press `Code > Download ZIP` on GitHub and extract it somewhere short, such as
   `C:\avr`. Or clone it:
   ```bash
   git clone https://github.com/YRGO-electrical-engineering/libavr32db28-p2.git
   ```
2. **Open the project.** Double-click `libavr32db28_p2.atsln`. Opening `main.c` on its own leaves
   the build commands greyed out.
3. **Build.** Press `F7`. The `Output` window should end with `Build succeeded`. If the device is
   unknown, install `AVR-Dx_DFP` under `Tools > Device Pack Manager` and build again. The project
   is set up for pack version 1.10.114.
4. **Program the board.** With an Atmel-ICE or a Curiosity board, open `Tools > Device
   Programming`, pick the tool, set `Device` to `AVR32DB28` and `Interface` to `UPDI`, and press
   `Apply`. Under `Memories`, check that `Flash` points at `Debug\libavr32db28_p2.hex` and press
   `Program`.

Studio only talks to Microchip's own programmers. With a plain serial UPDI cable, use `make flash`
as described under [Command line](#command-line).

The `test/` folder is in the project so it can be read, but Studio does not build it.

---

## Structure

```text
ci/                    Scripts for building, flashing, testing and formatting
docs/                  Driver reference and toolchain setup
include/               Driver headers, i.e. the API students use
libs/                  The yrgo::test framework, a git submodule
source/                Driver implementations and the mocked hardware platform
test/                  Unit tests, run on the host against the mocked hardware
main.c                 Application entry point
Makefile               Targets for building, flashing, testing and formatting
libavr32db28_p2.atsln  Microchip Studio solution
libavr32db28_p2.cproj  Microchip Studio project
```

`include/arch/` and `source/arch/` hold the hardware platform. It selects the real AVR registers
when building firmware and a mocked set of registers in RAM when building the tests, so the same
driver code compiles for both. It also provides `delay_ms()`.

---

## Command line
The `make` targets are an alternative to Microchip Studio.
[docs/toolchain.md](./docs/toolchain.md) lists what each one needs and how to install it, on
Windows as well as on Linux.

| Target              | Does                                                            |
| ------------------- | --------------------------------------------------------------- |
| `make build`        | Cross-compiles the firmware to `build/main.elf` and `main.hex`  |
| `make flash`        | Builds, then programs the board through a serial UPDI adapter   |
| `make test`         | Builds and runs the unit tests on the host                      |
| `make format`       | Formats all C/C++ files with `clang-format`                     |
| `make format-check` | Checks the formatting without changing any file                 |
| `make clean`        | Removes all build artifacts                                     |

### Building
The build needs `avr-gcc` and the AVR-Dx device family pack. The pack is looked for in `dfp/`
first and then in a local Microchip Studio installation. Set `DFP_DIR` to use one somewhere else.

### Flashing
The port defaults to `COM3` on Windows and `/dev/ttyUSB0` elsewhere:

```bash
make flash PORT=COM4
```

To program an existing hex file without building again, run `bash ci/flash.sh` directly. It uses
`build/main.hex` if there is one and Studio's `Debug/` output otherwise.

### Tests
The drivers are tested against a mocked AVR32DB28, so no board is needed. The mock keeps the
registers as ordinary variables in RAM, and each test checks what a driver wrote to them.

The tests use the [yrgo::test](https://github.com/yrgo-libs/yrgo-test) framework, a git submodule
that has to be checked out once:

```bash
git submodule update --init
make test
```

---

## Continuous Integration
Every push and pull request to `main` builds the firmware, runs the tests and checks the
formatting. See [.github/workflows/ci.yml](./.github/workflows/ci.yml).

---
