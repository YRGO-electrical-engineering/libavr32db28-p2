/**
 * @file Tests for the AVR32DB28 7-segment display driver.
 */
#include <cstdint>

#include "arch/avr/hw_platform.h"
#include "yrgo/test/test.h"

extern "C"
{
#include "driver/display.h"
#include "driver/timer.h"
} // extern "C"

namespace
{
constexpr std::uint8_t Zero{0U};

/** Bit masks of the pins the display uses, which is all of I/O port A. */
constexpr std::uint8_t BcdMask{static_cast<std::uint8_t>(PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm)};
constexpr std::uint8_t DpMask{PIN4_bm};
constexpr std::uint8_t Digit1Mask{PIN5_bm};
constexpr std::uint8_t Digit2Mask{PIN6_bm};
constexpr std::uint8_t Digit3Mask{PIN7_bm};
constexpr std::uint8_t DigitMask{static_cast<std::uint8_t>(Digit1Mask | Digit2Mask | Digit3Mask)};
constexpr std::uint8_t DisplayMask{static_cast<std::uint8_t>(BcdMask | DpMask | DigitMask)};

/** Pins driven high while the display is blank: BCD blank and every digit deselected. */
constexpr std::uint8_t BlankMask{static_cast<std::uint8_t>(BcdMask | DigitMask)};

/** Number of digits, and the select pin of each one in the order they are lit. */
constexpr std::uint8_t DigitCount{3U};
constexpr std::uint8_t DigitMasks[DigitCount]{Digit1Mask, Digit2Mask, Digit3Mask};

/** Value display_read gives when the display is blank. */
constexpr std::int16_t NoValue{-1};

/** BCD value the decoder blanks a digit for, mirroring BCD_BLANK in display.c. */
constexpr std::uint8_t BcdBlank{15U};

/** Time each digit is lit in milliseconds, mirroring REFRESH_MS in display.c. */
constexpr std::uint16_t RefreshMs{3U};

/** Counter top value producing a one millisecond tick at 4 MHz with a /2 prescaler. */
constexpr std::uint16_t TimerPeriod{1999U};

/** Number of timer circuits the device provides. */
constexpr std::uint8_t TimerCount{static_cast<std::uint8_t>(TIMER_ID_NONE)};

/** Three digit value most tests write, along with the three digits it is shown as. */
constexpr std::uint16_t DisplayValue{425U};
constexpr std::uint8_t DisplayHundreds{4U};
constexpr std::uint8_t DisplayTens{2U};
constexpr std::uint8_t DisplayOnes{5U};

/** BCD pins cleared while the hundreds of DisplayValue are shown. */
constexpr std::uint8_t HundredsCleared{static_cast<std::uint8_t>(~DisplayHundreds & BcdMask)};

/** Two digit value, along with the two digits it is shown as. */
constexpr std::uint16_t TwoDigitValue{37U};
constexpr std::uint8_t TwoDigitTens{3U};
constexpr std::uint8_t TwoDigitOnes{7U};

/** Single digit value, i.e. one shown without leading zeros. */
constexpr std::uint16_t SingleDigitValue{7U};

/** Largest value the display can show, and the smallest one it cannot. */
constexpr std::uint16_t LargestValue{999U};
constexpr std::uint16_t TooLargeValue{1000U};

/** Number of refresh periods stepped through where several in a row are checked. */
constexpr std::uint8_t RefreshCycles{7U};

/**
 * @brief Release every timer and reset the mocked registers.
 */
void resetHardware()
{
    for (std::uint8_t id{Zero}; id < TimerCount; ++id)
    {
        const auto timer = static_cast<timer_id_t>(id);
        timer_deinit(timer);
    }
    testHwPlatformReset();
}

/**
 * @brief Reset the hardware and initialize the display.
 */
void reset()
{
    resetHardware();
    display_init();
}

/**
 * @brief Let one digit's time run out, so that the display moves on to the next digit.
 */
void elapseRefresh()
{
    for (std::uint16_t i{Zero}; i < RefreshMs; ++i)
    {
        TCB0.INTFLAGS = TCB_CAPT_bm;
        display_update();
        TCB0.INTFLAGS = Zero;
    }
}

/**
 * @brief Get the BCD value the decoder is being fed.
 *
 * @return Value on the four BCD pins.
 */
std::uint8_t shownBcd() { return static_cast<std::uint8_t>(PORTA.OUTSET & BcdMask); }

/**
 * @brief Check whether a digit is lit.
 *
 * @param[in] mask Bit mask of the digit's pin.
 *
 * @return True if the digit is lit, false otherwise.
 */
bool digitLit(const std::uint8_t mask)
{
    return ((PORTA.OUTCLR & mask) != Zero) && ((PORTA.OUTSET & mask) == Zero);
}

/**
 * @brief Count the digits that are lit.
 *
 * @return Number of lit digits.
 */
std::uint8_t litDigits()
{
    std::uint8_t count{};
    for (const auto mask : DigitMasks)
    {
        if (digitLit(mask)) { ++count; }
    }
    return count;
}

/**
 * @brief Check whether the decimal point is lit.
 *
 * @return True if the decimal point is lit, false otherwise.
 */
bool dpLit() { return ((PORTA.OUTSET & DpMask) != Zero) && ((PORTA.OUTCLR & DpMask) == Zero); }

/**
 * @brief Test that initializing the display configures every pin of I/O port A as an output.
 */
TEST(Display, InitConfiguresTheDisplayPinsAsOutputs)
{
    constexpr std::uint8_t wholePort{0xFFU};

    reset();
    EXPECT_EQ(PORTA.DIR, DisplayMask);
    EXPECT_EQ(DisplayMask, wholePort);
}

/**
 * @brief Test that the display starts out blank.
 */
TEST(Display, InitBlanksTheDisplay)
{
    constexpr std::uint8_t noDigits{0U};

    reset();

    EXPECT_EQ(PORTA.OUTSET, BlankMask);
    EXPECT_EQ(PORTA.OUTCLR, DpMask);
    EXPECT_EQ(litDigits(), noDigits);
}

/**
 * @brief Test that initializing the display again starts over: no value and no decimal point.
 */
TEST(Display, InitStartsOverFromABlankDisplay)
{
    reset();
    display_write(DisplayValue);
    display_show_dp(true);
    elapseRefresh();
    EXPECT_TRUE(display_init());

    EXPECT_EQ(display_read(), NoValue);
    EXPECT_EQ(PORTA.OUTSET, BlankMask);

    // The first digit has the first turn again, and the decimal point stays dark.
    display_write(DisplayValue);
    elapseRefresh();
    EXPECT_TRUE(digitLit(Digit1Mask));
    elapseRefresh();
    EXPECT_TRUE(digitLit(Digit2Mask));
    EXPECT_FALSE(dpLit());
}

/**
 * @brief Test that initializing the display reserves a running timer to refresh itself with.
 */
TEST(Display, InitReservesAndStartsARefreshTimer)
{
    reset();

    EXPECT_EQ(TCB0.CCMP, TimerPeriod);
    const auto timer0Enabled = static_cast<std::uint8_t>(TCB0.CTRLA & TCB_ENABLE_bm);
    EXPECT_NE(timer0Enabled, Zero);
}

/**
 * @brief Test that initializing the display twice doesn't reserve a second timer.
 */
TEST(Display, InitTwiceReservesOneTimerOnly)
{
    reset();
    EXPECT_TRUE(display_init());

    const auto timer0Enabled = static_cast<std::uint8_t>(TCB0.CTRLA & TCB_ENABLE_bm);
    EXPECT_NE(timer0Enabled, Zero);
    EXPECT_EQ(TCB1.CTRLA, Zero);
    EXPECT_EQ(TCB2.CTRLA, Zero);
}

/**
 * @brief Test that a digit stays lit until its time is up.
 */
TEST(Display, UpdateWaitsForTheRefreshTime)
{
    reset();
    display_write(DisplayValue);

    // One millisecond short of a digit's time, the display hasn't changed since initialization.
    for (std::uint16_t i{Zero}; i < RefreshMs - 1U; ++i)
    {
        TCB0.INTFLAGS = TCB_CAPT_bm;
        display_update();
        TCB0.INTFLAGS = Zero;
    }
    EXPECT_EQ(PORTA.OUTSET, BlankMask);

    // The final millisecond lights the first digit.
    TCB0.INTFLAGS = TCB_CAPT_bm;
    display_update();
    EXPECT_TRUE(digitLit(Digit1Mask));
}

/**
 * @brief Test that the three digits are lit in turn: hundreds, tens, ones, then hundreds again.
 */
TEST(Display, UpdateTakesTurnsBetweenTheDigits)
{
    reset();
    display_write(DisplayValue);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), DisplayHundreds);
    EXPECT_TRUE(digitLit(Digit1Mask));

    elapseRefresh();
    EXPECT_EQ(shownBcd(), DisplayTens);
    EXPECT_TRUE(digitLit(Digit2Mask));

    elapseRefresh();
    EXPECT_EQ(shownBcd(), DisplayOnes);
    EXPECT_TRUE(digitLit(Digit3Mask));

    elapseRefresh();
    EXPECT_EQ(shownBcd(), DisplayHundreds);
    EXPECT_TRUE(digitLit(Digit1Mask));
}

/**
 * @brief Test that exactly one digit is lit at a time.
 */
TEST(Display, OnlyOneDigitIsLitAtATime)
{
    constexpr std::uint8_t oneDigit{1U};

    reset();
    display_write(DisplayValue);

    for (std::uint8_t i{Zero}; i < RefreshCycles; ++i)
    {
        elapseRefresh();
        EXPECT_EQ(litDigits(), oneDigit);
    }
}

/**
 * @brief Test that the BCD pins carry the complement of the value as well.
 */
TEST(Display, UpdateDrivesEveryBcdPin)
{
    reset();
    display_write(DisplayValue);
    elapseRefresh();

    const auto bcdSet = static_cast<std::uint8_t>(PORTA.OUTSET & BcdMask);
    EXPECT_EQ(bcdSet, DisplayHundreds);
    const auto bcdCleared = static_cast<std::uint8_t>(PORTA.OUTCLR & BcdMask);
    EXPECT_EQ(bcdCleared, HundredsCleared);
}

/**
 * @brief Test that every pin of the port is driven one way or the other on each refresh.
 */
TEST(Display, UpdateDrivesEveryPin)
{
    reset();
    display_write(DisplayValue);

    for (std::uint8_t i{Zero}; i < RefreshCycles; ++i)
    {
        elapseRefresh();
        const auto driven = static_cast<std::uint8_t>(PORTA.OUTSET | PORTA.OUTCLR);
        EXPECT_EQ(driven, DisplayMask);
        const auto drivenBothWays = static_cast<std::uint8_t>(PORTA.OUTSET & PORTA.OUTCLR);
        EXPECT_EQ(drivenBothWays, Zero);
    }
}

/**
 * @brief Test that a two digit value is shown without a leading zero.
 */
TEST(Display, LeadingZeroIsBlankedForTwoDigits)
{
    reset();
    display_write(TwoDigitValue);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), BcdBlank);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), TwoDigitTens);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), TwoDigitOnes);
}

/**
 * @brief Test that a single digit value is shown without leading zeros.
 */
TEST(Display, LeadingZerosAreBlankedForOneDigit)
{
    reset();
    display_write(SingleDigitValue);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), BcdBlank);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), BcdBlank);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), SingleDigitValue);
}

/**
 * @brief Test that zero is shown as a single zero.
 */
TEST(Display, ZeroIsShownAsOneDigit)
{
    reset();
    display_write(Zero);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), BcdBlank);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), BcdBlank);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), Zero);
}

/**
 * @brief Test that a zero in the middle of a value is shown.
 */
TEST(Display, ZeroInsideAValueIsShown)
{
    constexpr std::uint16_t value{405U};

    reset();
    display_write(value);

    elapseRefresh();
    elapseRefresh();
    EXPECT_EQ(shownBcd(), Zero);
    EXPECT_TRUE(digitLit(Digit2Mask));
}

/**
 * @brief Test that the zero before the decimal point is kept, so that 7 reads as 0.7.
 */
TEST(Display, LeadingZeroIsKeptWithTheDecimalPoint)
{
    reset();
    display_write(SingleDigitValue);
    display_show_dp(true);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), BcdBlank);
    EXPECT_FALSE(dpLit());

    elapseRefresh();
    EXPECT_EQ(shownBcd(), Zero);
    EXPECT_TRUE(dpLit());

    elapseRefresh();
    EXPECT_EQ(shownBcd(), SingleDigitValue);
}

/**
 * @brief Test that a two digit value with the decimal point reads as 3.7, without a leading zero.
 */
TEST(Display, TwoDigitsWithTheDecimalPoint)
{
    reset();
    display_write(TwoDigitValue);
    display_show_dp(true);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), BcdBlank);
    EXPECT_FALSE(dpLit());

    elapseRefresh();
    EXPECT_EQ(shownBcd(), TwoDigitTens);
    EXPECT_TRUE(dpLit());

    elapseRefresh();
    EXPECT_EQ(shownBcd(), TwoDigitOnes);
    EXPECT_FALSE(dpLit());
}

/**
 * @brief Test that the decimal point can be asked for before a value is written.
 */
TEST(Display, DecimalPointWaitsForAValue)
{
    reset();
    display_show_dp(true);

    for (std::uint8_t i{Zero}; i < DigitCount; ++i)
    {
        elapseRefresh();
        EXPECT_FALSE(dpLit());
    }

    display_write(DisplayValue);
    elapseRefresh();
    elapseRefresh();
    EXPECT_TRUE(dpLit());
}

/**
 * @brief Test that the decimal point is lit with the second digit only.
 */
TEST(Display, DecimalPointBelongsToTheSecondDigit)
{
    reset();
    display_write(DisplayValue);
    display_show_dp(true);

    elapseRefresh();
    EXPECT_TRUE(digitLit(Digit1Mask));
    EXPECT_FALSE(dpLit());

    elapseRefresh();
    EXPECT_TRUE(digitLit(Digit2Mask));
    EXPECT_TRUE(dpLit());

    elapseRefresh();
    EXPECT_TRUE(digitLit(Digit3Mask));
    EXPECT_FALSE(dpLit());
}

/**
 * @brief Test that the decimal point stays dark until it is asked for, and goes dark again.
 */
TEST(Display, DecimalPointIsHiddenByDefault)
{
    reset();
    display_write(DisplayValue);

    for (std::uint8_t i{Zero}; i < DigitCount; ++i)
    {
        elapseRefresh();
        EXPECT_FALSE(dpLit());
    }

    display_show_dp(true);
    elapseRefresh();
    elapseRefresh();
    EXPECT_TRUE(dpLit());

    display_show_dp(false);
    for (std::uint8_t i{Zero}; i < DigitCount; ++i)
    {
        elapseRefresh();
        EXPECT_FALSE(dpLit());
    }
}

/**
 * @brief Test that the largest value, 999, is shown.
 */
TEST(Display, WriteAcceptsTheLargestValue)
{
    constexpr std::uint8_t nine{9U};

    reset();
    display_write(LargestValue);

    for (std::uint8_t i{Zero}; i < DigitCount; ++i)
    {
        elapseRefresh();
        EXPECT_EQ(shownBcd(), nine);
    }
}

/**
 * @brief Test that a value too large for three digits is ignored.
 */
TEST(Display, WriteIgnoresValuesAboveTheLargest)
{
    constexpr std::uint16_t largestPossible{65535U};

    reset();
    display_write(DisplayValue);
    display_write(TooLargeValue);
    display_write(largestPossible);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), DisplayHundreds);
}

/**
 * @brief Test that clearing the display blanks every digit and the decimal point.
 */
TEST(Display, ClearBlanksEveryDigit)
{
    reset();
    display_write(DisplayValue);
    display_show_dp(true);
    display_clear();

    for (std::uint8_t i{Zero}; i < DigitCount; ++i)
    {
        elapseRefresh();
        EXPECT_EQ(shownBcd(), BcdBlank);
        EXPECT_FALSE(dpLit());
    }
}

/**
 * @brief Test that a cleared display shows a value written afterwards.
 */
TEST(Display, WriteAfterClearShowsTheValueAgain)
{
    reset();
    display_clear();
    display_write(DisplayValue);

    elapseRefresh();
    EXPECT_EQ(shownBcd(), DisplayHundreds);
}

/**
 * @brief Test that a display which has been given nothing to show reads as blank.
 */
TEST(Display, ReadReturnsBlankBeforeAnythingIsWritten)
{
    reset();

    EXPECT_EQ(display_read(), NoValue);
}

/**
 * @brief Test that reading the display gives back the value written to it, without a refresh.
 */
TEST(Display, ReadReturnsTheValueWritten)
{
    constexpr std::int16_t expected{425};
    constexpr std::int16_t expectedLargest{999};

    reset();
    display_write(DisplayValue);
    EXPECT_EQ(display_read(), expected);

    display_write(LargestValue);
    EXPECT_EQ(display_read(), expectedLargest);
}

/**
 * @brief Test that zero is a value like any other, and not mistaken for a blank display.
 */
TEST(Display, ReadTellsZeroApartFromBlank)
{
    constexpr std::int16_t expected{0};

    reset();
    display_write(Zero);

    EXPECT_EQ(display_read(), expected);
    EXPECT_NE(display_read(), NoValue);
}

/**
 * @brief Test that a cleared display reads as blank.
 */
TEST(Display, ReadReturnsBlankAfterClear)
{
    reset();
    display_write(DisplayValue);
    display_clear();

    EXPECT_EQ(display_read(), NoValue);
}

/**
 * @brief Test that writing after clearing reads back the new value.
 */
TEST(Display, ReadReturnsTheValueWrittenAfterClear)
{
    constexpr std::int16_t expected{37};

    reset();
    display_write(DisplayValue);
    display_clear();
    display_write(TwoDigitValue);

    EXPECT_EQ(display_read(), expected);
}

/**
 * @brief Test that a rejected value leaves the display reading what it had.
 */
TEST(Display, ReadIgnoresRejectedValues)
{
    constexpr std::int16_t expected{425};

    reset();
    display_write(DisplayValue);
    display_write(TooLargeValue);

    EXPECT_EQ(display_read(), expected);
}

/**
 * @brief Test that the display leaves the other I/O ports untouched.
 */
TEST(Display, DisplayLeavesOtherPortsUntouched)
{
    reset();
    display_write(DisplayValue);
    for (std::uint8_t i{Zero}; i < RefreshCycles; ++i)
    {
        elapseRefresh();
    }

    for (const PORT_t* port : {&PORTC, &PORTD, &PORTF})
    {
        const auto written = static_cast<std::uint8_t>(port->DIR | port->OUTSET | port->OUTCLR);
        EXPECT_EQ(written, Zero);
    }
}
} // namespace
