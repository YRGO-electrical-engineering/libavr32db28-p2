/**
 * @file Digital GPIO driver implementation details.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arch/avr/hw_platform.h"
#include "driver/digital.h"
#include "driver/gpio.h"

/**
 * @brief Pin hardware structure.
 */
typedef struct
{
    /** I/O port the pin belongs to. */
    PORT_t* port;

    /** Bit number of the pin within the port. */
    const uint8_t bit;

    /** Bit mask of the pin within the port. */
    const uint8_t mask;

    /** True if the pin is the output of the DAC. */
    const bool dac;
} pin_hw_t;

// -----------------------------------------------------------------------------
static inline bool pin_valid(const uint8_t pin)
{
    const uint8_t max = (uint8_t)(GPIO_PIN_PF1);
    return max >= pin;
}

// -----------------------------------------------------------------------------
static inline bool mode_valid(const gpio_mode_t mode)
{
    return (GPIO_MODE_IN == mode) || (GPIO_MODE_OUT == mode);
}

// -----------------------------------------------------------------------------
static inline const pin_hw_t* get_hw(const uint8_t pin)
{
    static const pin_hw_t pins[] = {
        {&PORTC, PIN0_bp, PIN0_bm, false}, {&PORTC, PIN1_bp, PIN1_bm, false},
        {&PORTC, PIN2_bp, PIN2_bm, false}, {&PORTC, PIN3_bp, PIN3_bm, false},
        {&PORTD, PIN1_bp, PIN1_bm, false}, {&PORTD, PIN2_bp, PIN2_bm, false},
        {&PORTD, PIN3_bp, PIN3_bm, false}, {&PORTD, PIN4_bp, PIN4_bm, false},
        {&PORTD, PIN5_bp, PIN5_bm, false}, {&PORTD, PIN6_bp, PIN6_bm, true},
        {&PORTD, PIN7_bp, PIN7_bm, false}, {&PORTF, PIN0_bp, PIN0_bm, false},
        {&PORTF, PIN1_bp, PIN1_bm, false},
    };
    return pin_valid(pin) ? &pins[pin] : NULL;
}

// -----------------------------------------------------------------------------
static inline bool is_output(const pin_hw_t* hw) { return (bool)(hw->port->DIR & hw->mask); }

// -----------------------------------------------------------------------------
static inline bool read_output(const pin_hw_t* hw) { return (bool)(hw->port->OUT & hw->mask); }

// -----------------------------------------------------------------------------
static inline bool read_input(const pin_hw_t* hw) { return (bool)(hw->port->IN & hw->mask); }

// -----------------------------------------------------------------------------
static inline volatile uint8_t* get_pinctrl(const pin_hw_t* hw)
{
    // The pin control registers lie one after another, starting with the one for pin 0.
    return &hw->port->PIN0CTRL + hw->bit;
}

// -----------------------------------------------------------------------------
static inline void claim_pin(const pin_hw_t* hw)
{
    // Disconnect the DAC from the pin, in case the analog driver used the pin as an output.
    if (hw->dac) { DAC0.CTRLA = 0U; }

    // Enable the digital input buffer; the analog driver disables it.
    *get_pinctrl(hw) &= ~PORT_ISC_gm;
}

// -----------------------------------------------------------------------------
static inline void set_direction(const pin_hw_t* hw, const gpio_mode_t mode)
{
    // Configure the pin direction as specified.
    if (GPIO_MODE_OUT == mode) { hw->port->DIR |= hw->mask; }
    else { hw->port->DIR &= ~hw->mask; }
}

// -----------------------------------------------------------------------------
static inline void write_output(const pin_hw_t* hw, const bool state)
{
    // Set pin state as specified.
    if (state) { hw->port->OUTSET = hw->mask; }
    else { hw->port->OUTCLR = hw->mask; }
}

// -----------------------------------------------------------------------------
static inline void toggle_output(const pin_hw_t* hw) { hw->port->OUTTGL = hw->mask; }

// -----------------------------------------------------------------------------
bool digital_init(const uint8_t pin, const gpio_mode_t mode)
{
    if (!mode_valid(mode)) { return false; }
    const pin_hw_t* hw = get_hw(pin);
    if (NULL == hw) { return false; }

    // Take the pin from the analog driver, then configure its direction as specified.
    claim_pin(hw);
    set_direction(hw, mode);
    return true;
}

// -----------------------------------------------------------------------------
bool digital_read(const uint8_t pin)
{
    const pin_hw_t* hw = get_hw(pin);
    if (NULL == hw) { return false; }

    // Read the driven state of an output, and the level on the pin of an input.
    return is_output(hw) ? read_output(hw) : read_input(hw);
}

// -----------------------------------------------------------------------------
bool digital_write(const uint8_t pin, const bool state)
{
    const pin_hw_t* hw = get_hw(pin);
    if (NULL == hw) { return false; }
    if (!is_output(hw)) { return false; }

    write_output(hw, state);
    return true;
}

// -----------------------------------------------------------------------------
bool digital_toggle(const uint8_t pin)
{
    const pin_hw_t* hw = get_hw(pin);
    if (NULL == hw) { return false; }
    if (!is_output(hw)) { return false; }

    toggle_output(hw);
    return true;
}
