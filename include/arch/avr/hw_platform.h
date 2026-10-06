/**
 * @file Hardware platform for AVR32DB28.
 */
#pragma once

#include <stdint.h>

/** CPU frequency measured in Hz. Defined for both builds, so drivers can use it either way. */
#ifndef F_CPU
#define F_CPU 4000000UL // Default CPU frequency measured in Hz (OSCHF reset default).
#endif

/** When compiling for the actual AVR target, include the real AVR hardware libraries. */
#ifndef TESTSUITE

#include <avr/cpufunc.h>
#include <avr/io.h>
#include <util/delay.h>

/** When compiling for the test suite, include the test hardware platform header instead. */
#else
#include "arch/test/hw_platform.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Generate a blocking delay.
 *
 *        Provided by both builds: the target busy-waits, while the test suite records the
 *        requested duration instead of sleeping.
 *
 * @param[in] ms Duration in milliseconds.
 */
void delay_ms(uint16_t ms);

#ifdef __cplusplus
}
#endif
