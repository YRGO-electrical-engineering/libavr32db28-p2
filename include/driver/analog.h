/**
 * @file Analog GPIO driver.
 */
#ifndef ANALOG_H_
#define ANALOG_H_

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"

#define ANALOG_READ_ERROR 65535U // Analog read error.

/**
 * @brief Initialize analog GPIO.
 *
 * @param[in] pin GPIO pin to initialize (see gpio_pin_t).
 * @param[in] mode Mode to configure.
 *
 * @return True if initialized, false otherwise.
 *
 * @note Analog input is only supported for GPIO_PIN_PD1 - GPIO_PIN_PD7, GPIO_PIN_PF0 and
 *       GPIO_PIN_PF1. Analog output is only supported for GPIO_PIN_PD6.
 *
 * @note The pin is taken over from the digital driver if that driver was using it.
 */
bool analog_init(uint8_t pin, gpio_mode_t mode);

/**
 * @brief Get analog GPIO input value.
 *
 * @param[in] pin GPIO pin to read.
 *
 * @return Raw input value in range (0, 4095), or ANALOG_READ_ERROR if the pin could not be read.
 *
 * @note This operation is only supported for GPIOs configured as inputs.
 */
uint16_t analog_read(uint8_t pin);

/**
 * @brief Set analog GPIO output value.
 *
 * @param[in] pin GPIO pin to set.
 * @param[in] value Raw output value in range (0, 4095).
 *
 * @return True if set, false if not.
 *
 * @note This operation is only supported for GPIOs configured as outputs.
 */
bool analog_write(uint8_t pin, uint16_t value);

#endif /** ANALOG_H_ */
