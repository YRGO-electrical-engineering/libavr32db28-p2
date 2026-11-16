/**
 * @file Analog GPIO driver implementation details.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arch/avr/hw_platform.h"
#include "driver/analog.h"

#define VALUE_MAX 4095U // Largest raw value, for inputs and outputs alike.
#define DAC_SHIFT 2U    // Bits dropped from an output value, the DAC being 10 bits wide.

#define SAMPLE_LENGTH 8U         // Extra cycles spent charging the sample capacitor.
#define CONVERSION_TIMEOUT 1000U // Times the result is checked for before giving up.

/**
 * @brief Pin hardware structure.
 */
typedef struct
{
    /** I/O port the pin belongs to. */
    PORT_t* port;

    /** Bit number of the pin within the port. */
    const uint8_t bit;

    /** Analog input the pin is connected to. */
    const uint8_t muxpos;

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
    // Only the pins listed here can be used by this driver; the port of every other pin is left
    // as NULL, which is how the digital-only pins are told apart.
    static const pin_hw_t pins[] = {
        [GPIO_PIN_PD1] = {&PORTD, PIN1_bp, ADC_MUXPOS_AIN1_gc, false},
        [GPIO_PIN_PD2] = {&PORTD, PIN2_bp, ADC_MUXPOS_AIN2_gc, false},
        [GPIO_PIN_PD3] = {&PORTD, PIN3_bp, ADC_MUXPOS_AIN3_gc, false},
        [GPIO_PIN_PD4] = {&PORTD, PIN4_bp, ADC_MUXPOS_AIN4_gc, false},
        [GPIO_PIN_PD5] = {&PORTD, PIN5_bp, ADC_MUXPOS_AIN5_gc, false},
        [GPIO_PIN_PD6] = {&PORTD, PIN6_bp, ADC_MUXPOS_AIN6_gc, true},
        [GPIO_PIN_PD7] = {&PORTD, PIN7_bp, ADC_MUXPOS_AIN7_gc, false},
        [GPIO_PIN_PF0] = {&PORTF, PIN0_bp, ADC_MUXPOS_AIN16_gc, false},
        [GPIO_PIN_PF1] = {&PORTF, PIN1_bp, ADC_MUXPOS_AIN17_gc, false},
    };
    if (!pin_valid(pin)) { return NULL; }
    return (NULL != pins[pin].port) ? &pins[pin] : NULL;
}

// -----------------------------------------------------------------------------
static inline volatile uint8_t* get_pinctrl(const pin_hw_t* hw)
{
    // The pin control registers lie one after another, starting with the one for pin 0.
    return &hw->port->PIN0CTRL + hw->bit;
}

// -----------------------------------------------------------------------------
static inline uint8_t get_mask(const pin_hw_t* hw) { return (uint8_t)(1U << hw->bit); }

// -----------------------------------------------------------------------------
static inline bool value_valid(const uint16_t value) { return VALUE_MAX >= value; }

// -----------------------------------------------------------------------------
static inline bool mode_supported(const pin_hw_t* hw, const gpio_mode_t mode)
{
    // Every pin can be an input, but only the pin connected to the DAC can be an output.
    return (GPIO_MODE_IN == mode) || hw->dac;
}

// -----------------------------------------------------------------------------
static inline bool dac_enabled(void) { return (bool)(DAC0.CTRLA & DAC_OUTEN_bm); }

// -----------------------------------------------------------------------------
static inline bool is_output(const pin_hw_t* hw) { return hw->dac && dac_enabled(); }

// -----------------------------------------------------------------------------
static inline bool is_input(const pin_hw_t* hw)
{
    // An analog pin has its digital input buffer disabled; it is an input unless the DAC drives it.
    const uint8_t mask = *get_pinctrl(hw) & PORT_ISC_gm;
    const bool analog  = PORT_ISC_INPUT_DISABLE_gc == mask;
    return analog && !is_output(hw);
}

// -----------------------------------------------------------------------------
static inline void init_pin(const pin_hw_t* hw)
{
    // Stop driving the pin, then disable its digital input buffer; it carries a voltage now.
    hw->port->DIR &= ~get_mask(hw);
    *get_pinctrl(hw) = PORT_ISC_INPUT_DISABLE_gc;
}

// -----------------------------------------------------------------------------
static inline void init_adc(void)
{
    // Measure against the supply voltage, so that a full reading means 5 V.
    VREF.ADC0REF = VREF_REFSEL_VDD_gc;

    // Clock the ADC at 1 MHz and sample for longer than the default, for high-impedance sources.
    ADC0.CTRLC    = ADC_PRESC_DIV4_gc;
    ADC0.SAMPCTRL = SAMPLE_LENGTH;

    // Enable the ADC at the full resolution the device offers.
    ADC0.CTRLA = ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc;
}

// -----------------------------------------------------------------------------
static inline void init_dac(void)
{
    // Output against the supply voltage, so that a full value means 5 V.
    VREF.DAC0REF = VREF_REFSEL_VDD_gc;

    // Enable the DAC and connect its output to the pin.
    DAC0.CTRLA = DAC_ENABLE_bm | DAC_OUTEN_bm;
}

// -----------------------------------------------------------------------------
static inline void release_dac(const pin_hw_t* hw)
{
    // Disconnect the DAC from the pin, if this is the pin it drives.
    if (hw->dac) { DAC0.CTRLA = 0U; }
}

// -----------------------------------------------------------------------------
static inline void conf_pin(const pin_hw_t* hw, const gpio_mode_t mode)
{
    init_pin(hw);

    // Configure the pin as specified, releasing it from the DAC if it is to be an input.
    if (GPIO_MODE_OUT == mode) { init_dac(); }
    else
    {
        release_dac(hw);
        init_adc();
    }
}

// -----------------------------------------------------------------------------
static inline void start_conversion(const pin_hw_t* hw)
{
    // Select the input and start a conversion.
    ADC0.MUXPOS  = hw->muxpos;
    ADC0.COMMAND = ADC_STCONV_bm;
}

// -----------------------------------------------------------------------------
static inline bool conversion_done(void) { return (bool)(ADC0.INTFLAGS & ADC_RESRDY_bm); }

// -----------------------------------------------------------------------------
static inline uint16_t read_result(void)
{
    // Read the result before lowering the flag; reading is itself one way to clear it.
    const uint16_t result = ADC0.RES;
    ADC0.INTFLAGS         = ADC_RESRDY_bm;
    return result;
}

// -----------------------------------------------------------------------------
static inline uint16_t get_dac_data(const uint16_t value)
{
    // Scale the value to the 10 bits of the DAC, which are the upper bits of its data register.
    const uint16_t shifted = (uint16_t)(value >> DAC_SHIFT);
    return (uint16_t)(shifted << DAC_DATA_gp);
}

// -----------------------------------------------------------------------------
bool analog_init(const uint8_t pin, const gpio_mode_t mode)
{
    if (!mode_valid(mode)) { return false; }
    const pin_hw_t* hw = get_hw(pin);
    if (NULL == hw) { return false; }
    if (!mode_supported(hw, mode)) { return false; }

    conf_pin(hw, mode);
    return true;
}

// -----------------------------------------------------------------------------
uint16_t analog_read(const uint8_t pin)
{
    const pin_hw_t* hw = get_hw(pin);
    if (NULL == hw) { return ANALOG_READ_ERROR; }
    if (!is_input(hw)) { return ANALOG_READ_ERROR; }

    start_conversion(hw);

    // Wait for the result, but give up eventually so a silent ADC can't stop the program.
    for (uint16_t i = 0U; i < CONVERSION_TIMEOUT; ++i)
    {
        if (conversion_done()) { return read_result(); }
    }
    return ANALOG_READ_ERROR;
}

// -----------------------------------------------------------------------------
bool analog_write(const uint8_t pin, const uint16_t value)
{
    const pin_hw_t* hw = get_hw(pin);
    if (NULL == hw) { return false; }
    if (!is_output(hw)) { return false; }
    if (!value_valid(value)) { return false; }

    DAC0.DATA = get_dac_data(value);
    return true;
}
