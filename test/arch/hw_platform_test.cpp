/**
 * @brief Tests for the mocked hardware platform.
 */
#include <cstdint>

#include "arch/avr/hw_platform.h"
#include "yrgo/test/test.h"

namespace
{
constexpr std::uint8_t Zero{0U};

/** Durations used to check that delays are recorded in the order they were requested. */
constexpr std::uint16_t FirstDelayMs{5U};
constexpr std::uint16_t SecondDelayMs{7U};

/** A delay far longer than the test suite could afford to actually sleep for. */
constexpr std::uint16_t LongDelayMs{60000U};

/** Positions in the list of recorded delays, and one far beyond anything the platform keeps. */
constexpr std::uint16_t FirstDelay{0U};
constexpr std::uint16_t SecondDelay{1U};
constexpr std::uint16_t UnrecordedDelay{1000U};

/** Numbers of recorded delays the tests expect to find. */
constexpr std::uint16_t OneDelay{1U};
constexpr std::uint16_t TwoDelays{2U};

/** Counter top value producing a one millisecond tick at 4 MHz with a /2 prescaler. */
constexpr std::uint16_t TimerPeriod{1999U};

/** An arbitrary non-zero conversion, written to check that a reset clears the register. */
constexpr std::uint16_t SomeConversion{1234U};
} // namespace

/**
 * @brief Test that a reset clears the mocked registers.
 */
TEST(HwPlatform, ResetClearsRegisters)
{
    PORTA.DIR      = PIN7_bm;
    PORTC.OUTSET   = PIN0_bm;
    PORTD.IN       = PIN1_bm;
    PORTF.PIN0CTRL = PORT_PULLUPEN_bm;

    testHwPlatformReset();
    EXPECT_EQ(PORTA.DIR, Zero);
    EXPECT_EQ(PORTC.OUTSET, Zero);
    EXPECT_EQ(PORTD.IN, Zero);
    EXPECT_EQ(PORTF.PIN0CTRL, Zero);
}

/**
 * @brief Test that the port registers behave as independent storage.
 */
TEST(HwPlatform, PortRegistersAreIndependent)
{
    testHwPlatformReset();
    PORTA.OUTSET = PIN7_bm;
    PORTA.OUTCLR = PIN0_bm;

    EXPECT_EQ(PORTA.OUTSET, PIN7_bm);
    EXPECT_EQ(PORTA.OUTCLR, PIN0_bm);
    EXPECT_EQ(PORTA.OUT, Zero);
    EXPECT_EQ(PORTA.IN, Zero);
}

/**
 * @brief Test that the ports are distinct objects.
 */
TEST(HwPlatform, PortsAreDistinct)
{
    testHwPlatformReset();
    PORTC.PIN0CTRL = PORT_PULLUPEN_bm;

    EXPECT_EQ(PORTC.PIN0CTRL, PORT_PULLUPEN_bm);
    EXPECT_EQ(PORTA.PIN0CTRL, Zero);
    EXPECT_EQ(PORTD.PIN0CTRL, Zero);
    EXPECT_EQ(PORTF.PIN0CTRL, Zero);
}

/**
 * @brief Test that delays are recorded in the order they were requested.
 */
TEST(HwPlatform, DelaysAreRecorded)
{
    testHwPlatformReset();
    delay_ms(FirstDelayMs);
    delay_ms(SecondDelayMs);

    EXPECT_EQ(testDelayCount(), TwoDelays);
    EXPECT_EQ(testDelayAt(FirstDelay), FirstDelayMs);
    EXPECT_EQ(testDelayAt(SecondDelay), SecondDelayMs);
}

/**
 * @brief Test that a delay returns immediately instead of sleeping.
 */
TEST(HwPlatform, DelaysDoNotSleep)
{
    testHwPlatformReset();
    delay_ms(LongDelayMs);
    EXPECT_EQ(testDelayCount(), OneDelay);
    EXPECT_EQ(testDelayAt(FirstDelay), LongDelayMs);
}

/**
 * @brief Test that resetting the platform discards the recorded delays.
 */
TEST(HwPlatform, ResetClearsRecordedDelays)
{
    testHwPlatformReset();
    delay_ms(FirstDelayMs);
    testHwPlatformReset();

    EXPECT_EQ(testDelayCount(), Zero);
    EXPECT_EQ(testDelayAt(FirstDelay), Zero);
}

/**
 * @brief Test that reading a delay that was never recorded is harmless.
 */
TEST(HwPlatform, ReadingAnUnrecordedDelayReturnsZero)
{
    testHwPlatformReset();
    delay_ms(FirstDelayMs);
    EXPECT_EQ(testDelayAt(SecondDelay), Zero);
    EXPECT_EQ(testDelayAt(UnrecordedDelay), Zero);
}

/**
 * @brief Test that resetting the platform clears the analog and timer peripherals too.
 */
TEST(HwPlatform, ResetClearsTheAnalogAndTimerPeripherals)
{
    ADC0.MUXPOS    = ADC_MUXPOS_AIN4_gc;
    ADC0.RES       = SomeConversion;
    VREF.ADC0REF   = VREF_REFSEL_VDD_gc;
    DAC0.CTRLA     = DAC_ENABLE_bm;
    DAC0.DATA      = SomeConversion;
    TCB0.CCMP      = TimerPeriod;
    TCB2.CTRLA     = TCB_ENABLE_bm;
    PORTD.PIN7CTRL = PORT_PULLUPEN_bm;

    testHwPlatformReset();
    EXPECT_EQ(ADC0.MUXPOS, Zero);
    EXPECT_EQ(ADC0.RES, Zero);
    EXPECT_EQ(VREF.ADC0REF, Zero);
    EXPECT_EQ(DAC0.CTRLA, Zero);
    EXPECT_EQ(DAC0.DATA, Zero);
    EXPECT_EQ(TCB0.CCMP, Zero);
    EXPECT_EQ(TCB2.CTRLA, Zero);
    EXPECT_EQ(PORTD.PIN7CTRL, Zero);
}

/**
 * @brief Test that every pin of a port has its own control register.
 */
TEST(HwPlatform, EveryPinHasItsOwnControlRegister)
{
    testHwPlatformReset();
    PORTD.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
    PORTD.PIN7CTRL = PORT_PULLUPEN_bm;

    EXPECT_EQ(PORTD.PIN4CTRL, PORT_ISC_INPUT_DISABLE_gc);
    EXPECT_EQ(PORTD.PIN7CTRL, PORT_PULLUPEN_bm);
    EXPECT_EQ(PORTD.PIN5CTRL, Zero);
    EXPECT_EQ(PORTD.PIN6CTRL, Zero);
}

/**
 * @brief Test that the pin control registers lie one after another, starting with pin 0.
 */
TEST(HwPlatform, PinControlRegistersAreConsecutive)
{
    const volatile std::uint8_t* const first{&PORTD.PIN0CTRL};
    const volatile std::uint8_t* const pin1Ctrl{first + PIN1_bp};
    const volatile std::uint8_t* const pin4Ctrl{first + PIN4_bp};
    const volatile std::uint8_t* const pin7Ctrl{first + PIN7_bp};

    EXPECT_EQ(pin1Ctrl, &PORTD.PIN1CTRL);
    EXPECT_EQ(pin4Ctrl, &PORTD.PIN4CTRL);
    EXPECT_EQ(pin7Ctrl, &PORTD.PIN7CTRL);
}

/**
 * @brief Test that every pin's bit position and bit mask describe the same bit.
 */
TEST(HwPlatform, PinPositionsMatchPinMasks)
{
    constexpr std::uint8_t one{1U};
    constexpr std::uint8_t mask0{one << PIN0_bp};
    constexpr std::uint8_t mask1{one << PIN1_bp};
    constexpr std::uint8_t mask2{one << PIN2_bp};
    constexpr std::uint8_t mask3{one << PIN3_bp};
    constexpr std::uint8_t mask4{one << PIN4_bp};
    constexpr std::uint8_t mask5{one << PIN5_bp};
    constexpr std::uint8_t mask6{one << PIN6_bp};
    constexpr std::uint8_t mask7{one << PIN7_bp};

    EXPECT_EQ(mask0, PIN0_bm);
    EXPECT_EQ(mask1, PIN1_bm);
    EXPECT_EQ(mask2, PIN2_bm);
    EXPECT_EQ(mask3, PIN3_bm);
    EXPECT_EQ(mask4, PIN4_bm);
    EXPECT_EQ(mask5, PIN5_bm);
    EXPECT_EQ(mask6, PIN6_bm);
    EXPECT_EQ(mask7, PIN7_bm);
}

/**
 * @brief Test that the three timer circuits are distinct objects.
 */
TEST(HwPlatform, TimerCircuitsAreDistinct)
{
    testHwPlatformReset();
    TCB1.CCMP = TimerPeriod;

    EXPECT_EQ(TCB1.CCMP, TimerPeriod);
    EXPECT_EQ(TCB0.CCMP, Zero);
    EXPECT_EQ(TCB2.CCMP, Zero);
}

/**
 * @brief Test that the ADC result register holds a full twelve bit conversion.
 */
TEST(HwPlatform, AdcResultHoldsATwelveBitValue)
{
    constexpr std::uint16_t fullScale{4095U};
    testHwPlatformReset();
    ADC0.RES = fullScale;
    EXPECT_EQ(ADC0.RES, fullScale);
}

/**
 * @brief Test that the DAC data register holds a full sixteen bit value.
 */
TEST(HwPlatform, DacDataHoldsASixteenBitValue)
{
    constexpr std::uint16_t fullScale{0xFFC0U};
    testHwPlatformReset();
    DAC0.DATA = fullScale;
    EXPECT_EQ(DAC0.DATA, fullScale);
}

/**
 * @brief Test that the interrupt flags are plain storage rather than write-one-to-clear.
 */
TEST(HwPlatform, InterruptFlagsKeepWhateverIsWritten)
{
    testHwPlatformReset();
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    ADC0.INTFLAGS = ADC_RESRDY_bm;
    EXPECT_EQ(ADC0.INTFLAGS, ADC_RESRDY_bm);

    ADC0.INTFLAGS = Zero;
    EXPECT_EQ(ADC0.INTFLAGS, Zero);
}
