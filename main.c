/**
 * @file Application entry point.
 */
#include <stdint.h>

#include "arch/avr/hw_platform.h"
#include "driver/digital.h"

#define LED GPIO_PIN_PC0    // Pin the LED is connected to.
#define BLINK_SPEED_MS 200U // Blink speed in ms.

/**
 * @brief Run the application.
 *
 *        Blink the LED every 200 ms.
 *
 * @return -1 if the LED could not be initialized; otherwise this function never returns.
 */
int main(void)
{
    // Initialize the LED, terminate on failure.
    if (!digital_init(LED, GPIO_MODE_OUT)) { return -1; }

    while (1)
    {
        digital_toggle(LED);
        delay_ms(BLINK_SPEED_MS);
    }
}
