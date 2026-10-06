/**
 * @file Hardware platform implementation details for AVR32DB28.
 *
 *        Compiled for the target only. The test suite links the mocked platform in
 *        source/arch/test instead, so this file is excluded from the host build.
 */
#ifndef TESTSUITE

#include "arch/avr/hw_platform.h"

#define TICK_MS 1U // Generate 1 ms tick.

// -----------------------------------------------------------------------------
void delay_ms(const uint16_t ms)
{
    // _delay_ms needs a compile time constant, so wait one fixed tick at a time.
    for (uint16_t i = 0U; i < ms; ++i)
    {
        _delay_ms(TICK_MS);
    }
}

#endif // TESTSUITE
