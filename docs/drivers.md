# Driver reference
Every function the P2 drivers expose, and what to know before using them. The headers in
`include/driver/` carry a short summary of each function.

---

## Conventions
* **Initialize each pin, then use it.** Call `digital_init()` or `analog_init()` once per pin.
  Calling it again changes the pin's mode.
* **One set of names.** Pins and modes are enumerations in `include/driver/gpio.h`, shared by both
  pin drivers.
* **The last driver to initialize a pin owns it.** The other driver refuses to use it.
* **Check what the `init` functions return.** A function that can be refused returns false and
  changes nothing. Functions returning nothing ignore a request they can't carry out.
* **Nothing uses interrupts.** The drivers are polled.

---

## The hardware

| Port  | Pins        | Names                           | Digital | Analog                        |
| ----- | ----------- | ------------------------------- | ------- | ----------------------------- |
| PORTC | PC0 - PC3   | `GPIO_PIN_PC0` - `GPIO_PIN_PC3` | Yes     | No                            |
| PORTD | PD1 - PD7   | `GPIO_PIN_PD1` - `GPIO_PIN_PD7` | Yes     | Input. PD6 is also the output |
| PORTF | PF0 - PF1   | `GPIO_PIN_PF0` - `GPIO_PIN_PF1` | Yes     | Input                         |

That is 13 pins. PD0 has no pin on the 28-pin package, and PF6 is the reset pin.

The display takes all of I/O port A: the BCD input of a 74HC4511 decoder on PA0 - PA3, the decimal
point on PA4 and the three digit select lines, active low, on PA5 - PA7.

The device runs at 5 V on its internal 4 MHz oscillator.

---

## Things worth knowing
* **Every pin starts as an input.** A pin that was never initialized can be read, but writing or
  toggling it is refused.
* **Inputs have no pull-up.** A button needs an external resistor holding its pin at the opposite
  level while it is released, or the pin floats and reads at random.
* **Use the pin names, never a raw number.** The IDs run from 0 to 12, so `GPIO_PIN_PD1` is 4
  rather than 1.
* **Not every pin is analog.** `analog_init(GPIO_PIN_PC0, GPIO_MODE_IN)` compiles, but returns
  false.
* **The display needs calling.** Without `display_update()` in the main loop it stays dark.
* **There are three timers, and the display takes one.** A program that uses the display has two
  left.
* **Blocking delays stop everything.** During `delay_ms()` no button is read and the display goes
  dark. `timer_elapsed()` is the way to wait without blocking.

---

## Digital
High is 5 V and low is ground. The [README](../README.md#usage) has a complete example.

```c
bool digital_init(uint8_t pin, gpio_mode_t mode);
```

Configures the pin as an input with `GPIO_MODE_IN` or an output with `GPIO_MODE_OUT`, taking it
back from the analog driver if needed. Returns false for an unknown pin or mode. An output keeps
the level it had, which after a reset is low.

```c
bool digital_read(uint8_t pin);
```

Returns true when the pin is high. For an output that is the level last written. An unknown pin
reads as false.

```c
bool digital_write(uint8_t pin, bool state);
```

Drives the pin high when true and low when false. Only outputs can be written. Anything else
returns false.

```c
bool digital_toggle(uint8_t pin);
```

Inverts the pin. Refuses the same pins `digital_write()` does.

---

## Analog
Values are 12 bits wide in both directions: 0 is 0 V and 4095 is just under the 5 V supply. One
step is about 1.2 mV, and a value converts to millivolts as `value * 5000 / 4096`, calculated in
32 bits.

```c
bool analog_init(uint8_t pin, gpio_mode_t mode);
```

Configures the pin as an analog input with `GPIO_MODE_IN` or an analog output with
`GPIO_MODE_OUT`, taking it over from the digital driver. Returns false for an unknown pin or mode,
for a pin on PORTC, and for an output on any pin but `GPIO_PIN_PD6`. The output keeps the value it
had, which after a reset is 0 V.

```c
uint16_t analog_read(uint8_t pin);
```

Measures the voltage on an input and returns it as a value from 0 to 4095. Any other pin returns
`ANALOG_READ_ERROR`, which is 65535. The call blocks for a few tens of microseconds, and returns
`ANALOG_READ_ERROR` if the conversion never finishes.

```c
bool analog_write(uint8_t pin, uint16_t value);
```

Sets the voltage on the output. Returns false for any pin that is not the output, and for a value
above 4095. The converter is 10 bits wide, so the two lowest bits are dropped and the voltage moves
in steps of four.

The output following a potentiometer on PD1:

```c
#include "driver/analog.h"

#define POT GPIO_PIN_PD1
#define OUTPUT GPIO_PIN_PD6

int main(void)
{
    if (!analog_init(POT, GPIO_MODE_IN)) { return -1; }
    if (!analog_init(OUTPUT, GPIO_MODE_OUT)) { return -1; }

    while (1)
    {
        const uint16_t value = analog_read(POT);
        if (ANALOG_READ_ERROR != value) { analog_write(OUTPUT, value); }
    }
}
```

---

## Display
Three digits, driven through a 74HC4511 decoder, so only digits can be shown. The digits share
their segment lines and are lit in turn, 3 ms each, which redraws the display 111 times a second.

```c
bool display_init(void);
```

Configures the display's pins, blanks it and reserves a timer. Returns false if all three timers
were already taken, in which case the display stays dark.

```c
void display_write(uint16_t value);
```

Shows a value from 0 to 999. A larger value is ignored. Leading zeros are not shown, except the
zero before the decimal point, so that 0.7 doesn't read as .7.

```c
int16_t display_read(void);
```

Returns the value the display was last given, or -1 when it is blank.

```c
void display_clear(void);
```

Blanks all three digits. The display stays blank until `display_write()` is called again.

```c
void display_show_dp(bool show);
```

Shows or hides the decimal point, which sits before the last digit. With it on, 372 reads as 37.2.

```c
void display_update(void);
```

Lights whichever digit's turn it is. **Call this every lap of the main loop.** It returns
immediately when the current digit's 3 ms are not yet up.

---

## Timer
Three timers, counting in milliseconds. A program reserves one, starts it, and then asks whether it
has elapsed.

**A timer only advances while the program is asking about one.** `timer_elapsed()` advances every
reserved timer, so call it at least once every millisecond, for example every lap of the main loop.
Slow code makes every timer run slow.

```c
timer_id_t timer_init(uint16_t timeout_ms);
```

Reserves a timer and returns its ID, or `TIMER_ID_NONE` if all three are taken or the timeout is
zero. The timer is stopped until `timer_start()` is called.

```c
void timer_deinit(timer_id_t id);
```

Stops the timer and releases it.

```c
void timer_start(timer_id_t id);
```

Starts the timer from zero.

```c
void timer_stop(timer_id_t id);
```

Stops the timer. A stopped timer never elapses.

```c
void timer_toggle(timer_id_t id);
```

Stops the timer if it is running, and starts it from zero otherwise.

```c
bool timer_running(timer_id_t id);
```

Returns whether the timer is running.

```c
bool timer_elapsed(timer_id_t id);
```

Returns true once the timeout has passed, and starts counting again from zero.

---

## Delays

```c
void delay_ms(uint16_t ms);
```

Declared in `include/arch/avr/hw_platform.h`. Waits the given number of milliseconds, during which
the processor does nothing else. In the host test suite it records the duration instead of
waiting.

---
