/**
 * @file Tests for the AVR32DB28 timer driver.
 */
#include <cstdint>

#include "arch/avr/hw_platform.h"
#include "yrgo/test/test.h"

extern "C"
{
#include "driver/timer.h"
} // extern "C"

namespace
{
constexpr std::uint8_t Zero{0U};

/** Number of timer circuits the device provides. */
constexpr std::uint8_t TimerCount{static_cast<std::uint8_t>(TIMER_ID_NONE)};

/** Counter top value producing a one millisecond tick at 4 MHz with a /2 prescaler. */
constexpr std::uint16_t TimerPeriod{1999U};

/** Timeouts used by the tests, measured in milliseconds. */
constexpr std::uint16_t ShortTimeout{1U};
constexpr std::uint16_t Timeout{10U};
constexpr std::uint16_t LongTimeout{1000U};

/** Timeout value rejected by timer_init. */
constexpr std::uint16_t InvalidTimeout{0U};

/**
 * @brief Release every timer, so that no state leaks between test cases.
 */
void releaseAllTimers()
{
    for (std::uint8_t id{Zero}; id < TimerCount; ++id)
    {
        timer_deinit(static_cast<timer_id_t>(id));
    }
}

/**
 * @brief Reset the mocked registers and release every timer.
 */
void reset()
{
    releaseAllTimers();
    testHwPlatformReset();
}

/**
 * @brief Get the mocked circuit associated with the given timer ID.
 *
 * @param[in] id ID of the timer instance.
 *
 * @return Reference to the associated circuit.
 */
TCB_t& circuit(const timer_id_t id)
{
    if (TIMER_ID_1 == id) { return TCB1; }
    if (TIMER_ID_2 == id) { return TCB2; }
    return TCB0;
}

/**
 * @brief Simulate a single millisecond passing.
 *
 * @param[in] id ID of the timer instance.
 *
 * @return True if the timeout elapsed on this millisecond, false otherwise.
 */
bool tick(const timer_id_t id)
{
    circuit(id).INTFLAGS = TCB_CAPT_bm;
    const bool elapsed{timer_elapsed(id)};
    circuit(id).INTFLAGS = Zero;
    return elapsed;
}

/**
 * @brief Simulate a number of milliseconds passing.
 *
 * @param[in] id ID of the timer instance.
 * @param[in] count Number of milliseconds to simulate.
 */
void elapseMilliseconds(const timer_id_t id, const std::uint16_t count)
{
    for (std::uint16_t i{Zero}; i < count; ++i)
    {
        tick(id);
    }
}

/**
 * @brief Test that a timer is reserved and its circuit configured for a one millisecond tick.
 */
TEST(Timer, InitReservesTimerAndConfiguresCircuit)
{
    reset();

    const auto id = timer_init(Timeout);

    EXPECT_EQ(id, TIMER_ID_0);
    EXPECT_EQ(TCB0.CCMP, TimerPeriod);
    EXPECT_EQ(TCB0.CTRLB, TCB_CNTMODE_INT_gc);
    EXPECT_EQ(TCB0.INTCTRL, Zero);
}

/**
 * @brief Test that a newly reserved timer isn't running yet.
 */
TEST(Timer, InitLeavesTimerStopped)
{
    reset();

    const auto id = timer_init(Timeout);

    EXPECT_FALSE(timer_running(id));
    EXPECT_FALSE(timer_elapsed(id));
}

/**
 * @brief Test that a timeout of zero milliseconds is rejected.
 */
TEST(Timer, InitRejectsZeroTimeout)
{
    reset();

    EXPECT_EQ(timer_init(InvalidTimeout), TIMER_ID_NONE);
}

/**
 * @brief Test that each reservation hands out a different timer.
 */
TEST(Timer, InitHandsOutEachTimerOnce)
{
    reset();

    EXPECT_EQ(timer_init(Timeout), TIMER_ID_0);
    EXPECT_EQ(timer_init(Timeout), TIMER_ID_1);
    EXPECT_EQ(timer_init(Timeout), TIMER_ID_2);
}

/**
 * @brief Test that reserving more timers than the device provides fails.
 */
TEST(Timer, InitFailsWhenAllTimersAreBusy)
{
    reset();

    for (std::uint8_t i{Zero}; i < TimerCount; ++i)
    {
        EXPECT_NE(timer_init(Timeout), TIMER_ID_NONE);
    }

    EXPECT_EQ(timer_init(Timeout), TIMER_ID_NONE);
}

/**
 * @brief Test that a released timer can be reserved again.
 */
TEST(Timer, DeinitReleasesTimerForReuse)
{
    reset();

    const auto id = timer_init(Timeout);
    timer_start(id);
    timer_deinit(id);

    EXPECT_FALSE(timer_running(id));
    EXPECT_EQ(timer_init(Timeout), id);
}

/**
 * @brief Test that each timer ID drives its own circuit.
 */
TEST(Timer, EachTimerDrivesItsOwnCircuit)
{
    reset();
    const auto first  = timer_init(Timeout);
    const auto second = timer_init(Timeout);
    timer_start(second);

    EXPECT_FALSE(timer_running(first));
    EXPECT_TRUE(timer_running(second));
    const auto timer0Enabled = static_cast<std::uint8_t>(TCB0.CTRLA & TCB_ENABLE_bm);
    EXPECT_EQ(timer0Enabled, Zero);
    const auto timer1Enabled = static_cast<std::uint8_t>(TCB1.CTRLA & TCB_ENABLE_bm);
    EXPECT_NE(timer1Enabled, Zero);
    const auto timer2Enabled = static_cast<std::uint8_t>(TCB2.CTRLA & TCB_ENABLE_bm);
    EXPECT_EQ(timer2Enabled, Zero);
}

/**
 * @brief Test that starting and stopping a timer enables and disables its circuit.
 */
TEST(Timer, StartAndStopControlTheCircuit)
{
    reset();
    const auto id = timer_init(Timeout);

    timer_start(id);
    EXPECT_TRUE(timer_running(id));
    timer_stop(id);
    EXPECT_FALSE(timer_running(id));
}

/**
 * @brief Test that toggling a timer starts it when stopped and stops it when running.
 */
TEST(Timer, ToggleFlipsRunningState)
{
    reset();
    const auto id = timer_init(Timeout);

    timer_toggle(id);
    EXPECT_TRUE(timer_running(id));
    timer_toggle(id);
    EXPECT_FALSE(timer_running(id));
}

/**
 * @brief Test that the clock source survives a stop, so the timer can be restarted.
 */
TEST(Timer, StopKeepsTheClockSourceSelected)
{
    reset();
    const auto id = timer_init(Timeout);

    timer_start(id);
    timer_stop(id);
    EXPECT_EQ(TCB0.CTRLA, TCB_CLKSEL_DIV2_gc);
}

/**
 * @brief Test that a timer only elapses once its whole timeout has passed.
 */
TEST(Timer, ElapsesOnlyAfterTheFullTimeout)
{
    constexpr std::uint16_t beforeTimeout{static_cast<std::uint16_t>(Timeout - 1U)};
    reset();
    const auto id = timer_init(Timeout);
    timer_start(id);

    // One millisecond short of the timeout, nothing has elapsed yet.
    elapseMilliseconds(id, beforeTimeout);
    EXPECT_FALSE(timer_elapsed(id));

    // The final millisecond completes the timeout.
    EXPECT_TRUE(tick(id));
}

/**
 * @brief Test that a timer restarts by itself, so it elapses once per timeout.
 */
TEST(Timer, ElapsesRepeatedly)
{
    reset();
    const auto id = timer_init(ShortTimeout);
    timer_start(id);
    EXPECT_TRUE(tick(id));

    // The counter is cleared, so the next millisecond has to pass before it elapses again.
    EXPECT_FALSE(timer_elapsed(id));
    EXPECT_TRUE(tick(id));
}

/**
 * @brief Test that the driver clears the circuit's flag, so a millisecond isn't counted twice.
 */
TEST(Timer, ElapsedClearsTheCircuitFlag)
{
    reset();
    const auto id = timer_init(Timeout);

    timer_start(id);
    circuit(id).INTFLAGS = TCB_CAPT_bm | TCB_OVF_bm;
    timer_elapsed(id);
    EXPECT_EQ(TCB0.INTFLAGS, TCB_CAPT_bm);
}

/**
 * @brief Test that a stopped timer never elapses.
 */
TEST(Timer, StoppedTimerNeverElapses)
{
    reset();
    const auto id        = timer_init(ShortTimeout);
    circuit(id).INTFLAGS = TCB_CAPT_bm;
    EXPECT_FALSE(timer_elapsed(id));
}

/**
 * @brief Test that restarting a timer discards the milliseconds counted so far.
 */
TEST(Timer, StartDiscardsPreviouslyCountedMilliseconds)
{
    reset();
    const auto id = timer_init(Timeout);
    timer_start(id);
    elapseMilliseconds(id, static_cast<std::uint16_t>(Timeout - 1U));
    timer_start(id);
    // The counter was cleared, so a single further millisecond mustn't complete the timeout.
    circuit(id).INTFLAGS = TCB_CAPT_bm;
    EXPECT_FALSE(timer_elapsed(id));
}

/**
 * @brief Test that a timeout longer than a single circuit period is counted in software.
 */
TEST(Timer, LongTimeoutIsCountedInSoftware)
{
    constexpr std::uint16_t beforeTimeout{static_cast<std::uint16_t>(LongTimeout - 1U)};
    reset();
    const auto id = timer_init(LongTimeout);

    timer_start(id);
    elapseMilliseconds(id, beforeTimeout);
    EXPECT_FALSE(timer_elapsed(id));
    EXPECT_TRUE(tick(id));
}

/**
 * @brief Test that the millisecond counter saturates instead of wrapping around.
 */
TEST(Timer, CounterSaturatesInsteadOfWrapping)
{
    constexpr std::uint32_t millisecondsUntilWrap{65536UL};

    reset();
    const auto neglected = timer_init(Timeout);
    const auto polled    = timer_init(Timeout);
    timer_start(neglected);

    // Advance the neglected timer past the point where its counter would wrap, without ever
    // asking whether it has elapsed.
    for (std::uint32_t i{0UL}; i < millisecondsUntilWrap; ++i)
    {
        circuit(neglected).INTFLAGS = TCB_CAPT_bm;
        timer_elapsed(polled);
        circuit(neglected).INTFLAGS = Zero;
    }

    // Had the counter wrapped it would read zero here, making the timeout look unfinished.
    EXPECT_TRUE(timer_elapsed(neglected));
}

/**
 * @brief Test that a timer which hasn't been reserved is ignored by every function.
 */
TEST(Timer, UnreservedTimerIsIgnored)
{
    reset();
    timer_deinit(TIMER_ID_0);
    timer_start(TIMER_ID_1);
    timer_stop(TIMER_ID_2);
    timer_toggle(TIMER_ID_0);

    EXPECT_FALSE(timer_running(TIMER_ID_0));
    EXPECT_FALSE(timer_elapsed(TIMER_ID_1));
    EXPECT_EQ(TCB0.CTRLA, Zero);
    EXPECT_EQ(TCB1.CTRLA, Zero);
    EXPECT_EQ(TCB2.CTRLA, Zero);
}

/**
 * @brief Test that an invalid timer ID is ignored by every function.
 */
TEST(Timer, InvalidIdIsIgnored)
{
    reset();
    timer_deinit(TIMER_ID_NONE);
    timer_start(TIMER_ID_NONE);
    timer_stop(TIMER_ID_NONE);
    timer_toggle(TIMER_ID_NONE);

    EXPECT_FALSE(timer_running(TIMER_ID_NONE));
    EXPECT_FALSE(timer_elapsed(TIMER_ID_NONE));
    EXPECT_EQ(TCB0.CTRLA, Zero);
    EXPECT_EQ(TCB1.CTRLA, Zero);
    EXPECT_EQ(TCB2.CTRLA, Zero);
}
} // namespace
