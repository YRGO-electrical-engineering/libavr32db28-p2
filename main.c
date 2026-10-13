/**
 * @file Application entry point.
 */
#include <stdbool.h>
#include <stdint.h>

#include "driver/analog.h"
#include "driver/digital.h"
#include "driver/display.h"
#include "driver/timer.h"

#define JOYSTICK_Y GPIO_PIN_PD5   // Pin the joystick's vertical axis is connected to.
#define JOYSTICK_X GPIO_PIN_PD6   // Pin the joystick's horizontal axis is connected to.
#define RESET_BUTTON GPIO_PIN_PC3 // Pin the reset button (SW1) is connected to.

#define AXIS_LOW 1024U  // Below this reading, an axis counts as pushed down or left.
#define AXIS_HIGH 3072U // Above this reading, an axis counts as pushed up or right.

#define VALUE_MIN 0U   // Smallest value the display can show.
#define VALUE_MAX 999U // Largest value the display can show.
#define STEP_MS 200U   // Time the joystick has to be held for the value to change again.

/**
 * @brief Check whether an axis is pushed past the given limit.
 *
 * @param[in] pin Pin the axis is connected to.
 * @param[in] high True to check for a reading above AXIS_HIGH, false for one below AXIS_LOW.
 *
 * @return True if the axis is pushed past the limit, false otherwise.
 */
static bool axis_pushed(const uint8_t pin, const bool high)
{
    // Treat an axis that cannot be read as resting in the middle.
    const uint16_t reading = analog_read(pin);
    if (ANALOG_READ_ERROR == reading) { return false; }
    return high ? (AXIS_HIGH < reading) : (AXIS_LOW > reading);
}

/**
 * @brief Work out the value the joystick is asking for.
 *
 * @param[in] value Value shown at the moment.
 *
 * @return The value to show next.
 */
static uint16_t next_value(const uint16_t value)
{
    // Check if the reset button is pressed, reset the value if true.
    if (digital_read(RESET_BUTTON)) { return VALUE_MIN; }

    // Push the joystick left or up to count up, and right or down to count down.
    if (axis_pushed(JOYSTICK_Y, true) || axis_pushed(JOYSTICK_X, false))
    {
        return VALUE_MAX == value ? VALUE_MIN : value + 1U;
    }
    if (axis_pushed(JOYSTICK_Y, false) || axis_pushed(JOYSTICK_X, true))
    {
        return VALUE_MIN == value ? VALUE_MAX : value - 1U;
    }
    return value;
}

/**
 * @brief Run the application.
 *
 *        Counts on the display as the joystick is held: up or left counts up, down or right
 *        counts down, wrapping around at either end. Pressing SW1 starts over from zero.
 *
 * @return -1 if the hardware could not be set up; otherwise this function never returns.
 */
int main(void)
{
    // Set up the joystick, the reset button, the display and the step timer, terminate if any of
    // them fails.
    if (!analog_init(JOYSTICK_Y, GPIO_MODE_IN)) { return -1; }
    if (!analog_init(JOYSTICK_X, GPIO_MODE_IN)) { return -1; }
    if (!digital_init(RESET_BUTTON, GPIO_MODE_IN)) { return -1; }
    if (!display_init()) { return -1; }

    const timer_id_t timer = timer_init(STEP_MS);
    if (TIMER_ID_NONE == timer) { return -1; }
    timer_start(timer);
    uint16_t value = VALUE_MIN;

    while (1)
    {
        if (timer_elapsed(timer))
        {
            value = next_value(value);
            display_write(value);
        }
        display_update();
    }
    return 0;
}
