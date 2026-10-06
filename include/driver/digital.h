/**
 * @file Digital GPIO driver.
 */
#ifndef DIGITAL_H_
#define DIGITAL_H_

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"

/**
 * @brief Initialize digital GPIO.
 *
 * @param[in] pin GPIO pin to initialize (see gpio_pin_t).
 * @param[in] mode Mode to configure.
 *
 * @return True if initialized, false otherwise.
 *
 * @note The pin is taken over from the analog driver if that driver was using it.
 */
bool digital_init(uint8_t pin, gpio_mode_t mode);

/**
 * @brief Get digital GPIO state.
 *
 * @param[in] pin GPIO pin to read.
 *
 * @return True if high, false if low.
 */
bool digital_read(uint8_t pin);

/**
 * @brief Set digital GPIO state.
 *
 * @param[in] pin GPIO pin to set.
 * @param[in] state GPIO state (true = high, false = low).
 *
 * @return True if set, false if not.
 *
 * @note This operation is only supported for GPIOs configured as outputs.
 */
bool digital_write(uint8_t pin, bool state);

/**
 * @brief Toggle digital GPIO state.
 *
 * @param[in] pin GPIO pin to toggle.
 *
 * @return True if toggled, false if not.
 *
 * @note This operation is only supported for GPIOs configured as outputs.
 */
bool digital_toggle(uint8_t pin);

#endif /** DIGITAL_H_ */
