/**
 * @brief Implementation details of the mocked hardware platform.
 */
#ifdef TESTSUITE

#include <string.h>

#include "arch/avr/hw_platform.h"

#define DELAY_CAPACITY 16U // Number of delay_ms calls kept for inspection.

/** Mocked peripheral instances. */
PORT_t PORTA;
PORT_t PORTC;
PORT_t PORTD;
PORT_t PORTF;
ADC_t ADC0;
DAC_t DAC0;
TCB_t TCB0;
TCB_t TCB1;
TCB_t TCB2;
VREF_t VREF;

/** Durations passed to delay_ms, and the number of calls made since the last reset. */
static uint16_t delays[DELAY_CAPACITY] = {0U};
static uint16_t delayCount             = 0U;

// -----------------------------------------------------------------------------
void testHwPlatformReset(void)
{
    memset((void*)&PORTA, 0, sizeof(PORTA));
    memset((void*)&PORTC, 0, sizeof(PORTC));
    memset((void*)&PORTD, 0, sizeof(PORTD));
    memset((void*)&PORTF, 0, sizeof(PORTF));

    memset((void*)&ADC0, 0, sizeof(ADC0));
    memset((void*)&DAC0, 0, sizeof(DAC0));
    memset((void*)&TCB0, 0, sizeof(TCB0));
    memset((void*)&TCB1, 0, sizeof(TCB1));
    memset((void*)&TCB2, 0, sizeof(TCB2));
    memset((void*)&VREF, 0, sizeof(VREF));

    memset(delays, 0, sizeof(delays));
    delayCount = 0U;
}

// -----------------------------------------------------------------------------
uint16_t testDelayCount(void) { return delayCount; }

// -----------------------------------------------------------------------------
uint16_t testDelayAt(const uint16_t index) { return (index < DELAY_CAPACITY) ? delays[index] : 0U; }

// -----------------------------------------------------------------------------
void delay_ms(const uint16_t ms)
{
    // Record the duration instead of sleeping, so that the test suite stays fast. Calls beyond
    // the capacity still count, they just aren't kept.
    if (DELAY_CAPACITY > delayCount) { delays[delayCount] = ms; }
    ++delayCount;
}

#endif // TESTSUITE
