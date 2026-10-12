/**
 * @file Tests for the analog GPIO driver.
 */
#include <cstdint>

#include "arch/avr/hw_platform.h"
#include "yrgo/test/test.h"

extern "C"
{
#include "driver/analog.h"
#include "driver/digital.h"
} // extern "C"

namespace
{
constexpr std::uint8_t Zero{0U};

/** Value of a register in which every bit is set. */
constexpr std::uint8_t AllPinsHigh{0xFFU};

/** Number of pins the driver supports, and of pins that are digital only. */
constexpr std::uint8_t PinCount{9U};
constexpr std::uint8_t DigitalOnlyPinCount{4U};

/** The first pin number that does not exist. */
constexpr std::uint8_t InvalidPin{PinCount + DigitalOnlyPinCount};

/** A mode outside the documented ones. */
constexpr gpio_mode_t InvalidMode{static_cast<gpio_mode_t>(GPIO_MODE_OUT + 1)};

/** Largest raw value, and an arbitrary one in between. */
constexpr std::uint16_t FullScale{4095U};
constexpr std::uint16_t SomeValue{1234U};

/** Sampling time mirroring SAMPLE_LENGTH in analog.c. */
constexpr std::uint8_t SampleLength{8U};

/** DAC control value with the converter enabled and its output connected to the pin. */
constexpr std::uint8_t DacEnabled{DAC_ENABLE_bm | DAC_OUTEN_bm};

/** ADC control value with the converter enabled at its full resolution. */
constexpr std::uint8_t AdcEnabled{static_cast<std::uint8_t>(ADC_ENABLE_bm | ADC_RESSEL_12BIT_gc)};

/** DAC data register holding the largest output value, its ten bits being the upper ones. */
constexpr std::uint16_t FullScaleData{0xFFC0U};

/**
 * @brief Hardware a pin is expected to map to.
 */
struct PinHw
{
    /** Pin as named by the driver. */
    gpio_pin_t pin;

    /** I/O port the pin belongs to. */
    PORT_t& port;

    /** Bit mask of the pin within the port. */
    std::uint8_t mask;

    /** Control register of the pin. */
    volatile std::uint8_t& pinctrl;

    /** Analog input the pin is connected to. */
    std::uint8_t muxpos;
};

/** Every pin of the driver, along with the hardware it is expected to use. */
const PinHw Pins[PinCount]{
    {GPIO_PIN_PD1, PORTD, PIN1_bm, PORTD.PIN1CTRL, ADC_MUXPOS_AIN1_gc},
    {GPIO_PIN_PD2, PORTD, PIN2_bm, PORTD.PIN2CTRL, ADC_MUXPOS_AIN2_gc},
    {GPIO_PIN_PD3, PORTD, PIN3_bm, PORTD.PIN3CTRL, ADC_MUXPOS_AIN3_gc},
    {GPIO_PIN_PD4, PORTD, PIN4_bm, PORTD.PIN4CTRL, ADC_MUXPOS_AIN4_gc},
    {GPIO_PIN_PD5, PORTD, PIN5_bm, PORTD.PIN5CTRL, ADC_MUXPOS_AIN5_gc},
    {GPIO_PIN_PD6, PORTD, PIN6_bm, PORTD.PIN6CTRL, ADC_MUXPOS_AIN6_gc},
    {GPIO_PIN_PD7, PORTD, PIN7_bm, PORTD.PIN7CTRL, ADC_MUXPOS_AIN7_gc},
    {GPIO_PIN_PF0, PORTF, PIN0_bm, PORTF.PIN0CTRL, ADC_MUXPOS_AIN16_gc},
    {GPIO_PIN_PF1, PORTF, PIN1_bm, PORTF.PIN1CTRL, ADC_MUXPOS_AIN17_gc},
};

/** The pins that exist but are not connected to the ADC or the DAC, i.e. I/O port C. */
constexpr gpio_pin_t DigitalOnlyPins[DigitalOnlyPinCount]{
    GPIO_PIN_PC0,
    GPIO_PIN_PC1,
    GPIO_PIN_PC2,
    GPIO_PIN_PC3,
};

/**
 * @brief Hand the driver a conversion result.
 *
 * @param[in] result Value the conversion is to produce.
 */
void prepareConversion(const std::uint16_t result)
{
    ADC0.RES      = result;
    ADC0.INTFLAGS = ADC_RESRDY_bm;
}

/**
 * @brief Test that initializing an input releases the pin and disables its digital input buffer.
 */
TEST(Analog, InitAsInputConfiguresThePin)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        // Every direction bit set except the pin's own.
        const auto input = static_cast<std::uint8_t>(AllPinsHigh & ~hw.mask);

        hw.port.DIR = AllPinsHigh;
        EXPECT_TRUE(analog_init(hw.pin, GPIO_MODE_IN));

        EXPECT_EQ(hw.port.DIR, input);
        EXPECT_EQ(hw.pinctrl, PORT_ISC_INPUT_DISABLE_gc);
    }
}

/**
 * @brief Test that initializing an input switches off the pull-up.
 */
TEST(Analog, InitAsInputSwitchesOffThePullUp)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        hw.pinctrl = PORT_PULLUPEN_bm;
        EXPECT_TRUE(analog_init(hw.pin, GPIO_MODE_IN));

        EXPECT_EQ(hw.pinctrl, PORT_ISC_INPUT_DISABLE_gc);
    }
}

/**
 * @brief Test that the digital-only pins are refused in both modes.
 */
TEST(Analog, InitIsRefusedForDigitalOnlyPins)
{
    testHwPlatformReset();
    PORTC.DIR = AllPinsHigh;

    for (const auto pin : DigitalOnlyPins)
    {
        EXPECT_FALSE(analog_init(pin, GPIO_MODE_IN));
        EXPECT_FALSE(analog_init(pin, GPIO_MODE_OUT));
    }
    EXPECT_EQ(PORTC.DIR, AllPinsHigh);
    EXPECT_EQ(PORTC.PIN0CTRL, Zero);
    EXPECT_EQ(ADC0.CTRLA, Zero);
    EXPECT_EQ(DAC0.CTRLA, Zero);
}

/**
 * @brief Test that the digital-only pins can be neither read nor written.
 */
TEST(Analog, ReadAndWriteAreRefusedForDigitalOnlyPins)
{
    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD1, GPIO_MODE_IN));
    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT));
    PORTC.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
    prepareConversion(SomeValue);

    for (const auto pin : DigitalOnlyPins)
    {
        EXPECT_EQ(analog_read(pin), ANALOG_READ_ERROR);
        EXPECT_FALSE(analog_write(pin, SomeValue));
    }
    EXPECT_EQ(ADC0.COMMAND, Zero);
    EXPECT_EQ(DAC0.DATA, Zero);
}

/**
 * @brief Test that initializing an input prepares the ADC.
 */
TEST(Analog, InitAsInputPreparesTheAdc)
{
    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD1, GPIO_MODE_IN));

    EXPECT_EQ(VREF.ADC0REF, VREF_REFSEL_VDD_gc);
    EXPECT_EQ(ADC0.CTRLC, ADC_PRESC_DIV4_gc);
    EXPECT_EQ(ADC0.SAMPCTRL, SampleLength);
    EXPECT_EQ(ADC0.CTRLA, AdcEnabled);
}

/**
 * @brief Test that initializing an input leaves the other pins and the DAC untouched.
 */
TEST(Analog, InitAsInputLeavesTheRestUntouched)
{
    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD4, GPIO_MODE_IN));

    EXPECT_EQ(PORTD.PIN3CTRL, Zero);
    EXPECT_EQ(PORTD.PIN5CTRL, Zero);
    EXPECT_EQ(PORTF.PIN0CTRL, Zero);
    const auto directions =
        static_cast<std::uint8_t>(PORTA.DIR | PORTC.DIR | PORTD.DIR | PORTF.DIR);
    EXPECT_EQ(directions, Zero);
    EXPECT_EQ(DAC0.CTRLA, Zero);
    EXPECT_EQ(VREF.DAC0REF, Zero);
}

/**
 * @brief Test that initializing PD6 as an output enables the DAC against the supply voltage.
 */
TEST(Analog, InitAsOutputEnablesTheDac)
{
    constexpr std::uint8_t released{static_cast<std::uint8_t>(AllPinsHigh & ~PIN6_bm)};

    testHwPlatformReset();
    PORTD.DIR = AllPinsHigh;
    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT));

    EXPECT_EQ(VREF.DAC0REF, VREF_REFSEL_VDD_gc);
    EXPECT_EQ(DAC0.CTRLA, DacEnabled);
    EXPECT_EQ(DAC0.DATA, Zero);
    EXPECT_EQ(PORTD.DIR, released);
    EXPECT_EQ(PORTD.PIN6CTRL, PORT_ISC_INPUT_DISABLE_gc);
}

/**
 * @brief Test that initializing the output again keeps the value it had.
 */
TEST(Analog, InitAsOutputKeepsTheOutputValue)
{
    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT));
    EXPECT_TRUE(analog_write(GPIO_PIN_PD6, SomeValue));
    const std::uint16_t data{DAC0.DATA};

    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT));
    EXPECT_EQ(DAC0.DATA, data);
    EXPECT_EQ(DAC0.CTRLA, DacEnabled);
}

/**
 * @brief Test that initializing the output leaves the ADC and the other pins untouched.
 */
TEST(Analog, InitAsOutputLeavesTheRestUntouched)
{
    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT));

    EXPECT_EQ(ADC0.CTRLA, Zero);
    EXPECT_EQ(VREF.ADC0REF, Zero);
    EXPECT_EQ(PORTD.PIN5CTRL, Zero);
    EXPECT_EQ(PORTD.PIN7CTRL, Zero);
    const auto directions =
        static_cast<std::uint8_t>(PORTA.DIR | PORTC.DIR | PORTD.DIR | PORTF.DIR);
    EXPECT_EQ(directions, Zero);
}

/**
 * @brief Test that only PD6 is accepted as an output, and that a refusal touches no register.
 */
TEST(Analog, InitAsOutputIsRefusedForOtherPins)
{
    for (const auto& hw : Pins)
    {
        if (GPIO_PIN_PD6 == hw.pin) { continue; }
        testHwPlatformReset();
        EXPECT_FALSE(analog_init(hw.pin, GPIO_MODE_OUT));

        EXPECT_EQ(hw.pinctrl, Zero);
        EXPECT_EQ(DAC0.CTRLA, Zero);
    }
}

/**
 * @brief Test that an invalid pin or mode is rejected without touching any register.
 */
TEST(Analog, InitRejectsInvalidArguments)
{
    testHwPlatformReset();

    EXPECT_FALSE(analog_init(InvalidPin, GPIO_MODE_IN));
    EXPECT_FALSE(analog_init(AllPinsHigh, GPIO_MODE_IN));
    EXPECT_FALSE(analog_init(GPIO_PIN_PD6, InvalidMode));

    EXPECT_EQ(ADC0.CTRLA, Zero);
    EXPECT_EQ(DAC0.CTRLA, Zero);
    EXPECT_EQ(PORTD.PIN6CTRL, Zero);
}

/**
 * @brief Test that reading an input converts the analog input the pin is connected to.
 */
TEST(Analog, ReadConvertsThePinsOwnInput)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        EXPECT_TRUE(analog_init(hw.pin, GPIO_MODE_IN));
        prepareConversion(SomeValue);

        EXPECT_EQ(analog_read(hw.pin), SomeValue);
        EXPECT_EQ(ADC0.MUXPOS, hw.muxpos);
        EXPECT_EQ(ADC0.COMMAND, ADC_STCONV_bm);
    }
}

/**
 * @brief Test that the whole 12-bit range is returned unchanged.
 */
TEST(Analog, ReadReturnsTheWholeRange)
{
    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PF0, GPIO_MODE_IN));

    prepareConversion(Zero);
    EXPECT_EQ(analog_read(GPIO_PIN_PF0), Zero);

    prepareConversion(FullScale);
    EXPECT_EQ(analog_read(GPIO_PIN_PF0), FullScale);
}

/**
 * @brief Test that reading an input leaves the pins and the DAC untouched.
 */
TEST(Analog, ReadDoesNotDriveAnyPin)
{
    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD3, GPIO_MODE_IN));
    prepareConversion(SomeValue);
    (void)analog_read(GPIO_PIN_PD3);

    for (const PORT_t* port : {&PORTA, &PORTC, &PORTD, &PORTF})
    {
        const auto pinState = static_cast<std::uint8_t>(port->DIR | port->OUT);
        EXPECT_EQ(pinState, Zero);
        const auto outputWrites =
            static_cast<std::uint8_t>(port->OUTSET | port->OUTCLR | port->OUTTGL);
        EXPECT_EQ(outputWrites, Zero);
    }
    EXPECT_EQ(DAC0.CTRLA, Zero);
    EXPECT_EQ(DAC0.DATA, Zero);
}

/**
 * @brief Test that the error value lies outside anything a conversion can produce.
 */
TEST(Analog, ReadErrorIsNotAValidReading) { EXPECT_TRUE(ANALOG_READ_ERROR > FullScale); }

/**
 * @brief Test that a conversion that never finishes reads as an error instead of hanging.
 */
TEST(Analog, ReadGivesUpOnASilentAdc)
{
    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD1, GPIO_MODE_IN));
    ADC0.RES = SomeValue;

    EXPECT_EQ(analog_read(GPIO_PIN_PD1), ANALOG_READ_ERROR);
}

/**
 * @brief Test that a pin that is not an analog input reads as an error, without a conversion.
 */
TEST(Analog, ReadIsRefusedForPinsThatAreNotInputs)
{
    testHwPlatformReset();
    prepareConversion(SomeValue);
    EXPECT_EQ(analog_read(GPIO_PIN_PD1), ANALOG_READ_ERROR);
    EXPECT_EQ(analog_read(InvalidPin), ANALOG_READ_ERROR);

    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT));
    EXPECT_EQ(analog_read(GPIO_PIN_PD6), ANALOG_READ_ERROR);

    EXPECT_EQ(ADC0.COMMAND, Zero);
}

/**
 * @brief Test that writing the output scales the 12-bit value to the ten bits of the DAC.
 */
TEST(Analog, WriteScalesTheValueToTheDac)
{
    constexpr std::uint16_t halfScale{2048U};
    constexpr std::uint16_t halfScaleData{0x8000U};

    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT));

    EXPECT_TRUE(analog_write(GPIO_PIN_PD6, FullScale));
    EXPECT_EQ(DAC0.DATA, FullScaleData);

    EXPECT_TRUE(analog_write(GPIO_PIN_PD6, halfScale));
    EXPECT_EQ(DAC0.DATA, halfScaleData);

    EXPECT_TRUE(analog_write(GPIO_PIN_PD6, Zero));
    EXPECT_EQ(DAC0.DATA, Zero);
}

/**
 * @brief Test that the two lowest bits of an output value are dropped.
 */
TEST(Analog, WriteDropsTheTwoLowestBits)
{
    constexpr std::uint16_t belowFirstStep{3U};
    constexpr std::uint16_t firstStep{4U};
    constexpr std::uint16_t belowSecondStep{7U};
    constexpr std::uint16_t firstStepData{0x0040U};

    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT));

    EXPECT_TRUE(analog_write(GPIO_PIN_PD6, belowFirstStep));
    EXPECT_EQ(DAC0.DATA, Zero);

    EXPECT_TRUE(analog_write(GPIO_PIN_PD6, firstStep));
    EXPECT_EQ(DAC0.DATA, firstStepData);

    EXPECT_TRUE(analog_write(GPIO_PIN_PD6, belowSecondStep));
    EXPECT_EQ(DAC0.DATA, firstStepData);
}

/**
 * @brief Test that writing the output leaves the ADC untouched.
 */
TEST(Analog, WriteDoesNotTouchTheAdc)
{
    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT));
    EXPECT_TRUE(analog_write(GPIO_PIN_PD6, SomeValue));

    EXPECT_EQ(ADC0.CTRLA, Zero);
    EXPECT_EQ(ADC0.MUXPOS, Zero);
    EXPECT_EQ(ADC0.COMMAND, Zero);
}

/**
 * @brief Test that a value above the 12-bit range is refused and the output keeps its value.
 */
TEST(Analog, WriteRefusesValuesOutOfRange)
{
    constexpr std::uint16_t aboveFullScale{FullScale + 1U};

    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT));
    EXPECT_TRUE(analog_write(GPIO_PIN_PD6, FullScale));
    const std::uint16_t data{DAC0.DATA};

    EXPECT_FALSE(analog_write(GPIO_PIN_PD6, aboveFullScale));
    EXPECT_FALSE(analog_write(GPIO_PIN_PD6, ANALOG_READ_ERROR));
    EXPECT_EQ(DAC0.DATA, data);
}

/**
 * @brief Test that writing is refused for every pin that is not configured as an output.
 */
TEST(Analog, WriteIsRefusedForPinsThatAreNotOutputs)
{
    testHwPlatformReset();
    EXPECT_FALSE(analog_write(GPIO_PIN_PD6, SomeValue));
    EXPECT_FALSE(analog_write(InvalidPin, SomeValue));

    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        EXPECT_TRUE(analog_init(hw.pin, GPIO_MODE_IN));
        EXPECT_FALSE(analog_write(hw.pin, SomeValue));
    }

    // The other pins stay refused even while the DAC is enabled.
    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT));
    EXPECT_FALSE(analog_write(GPIO_PIN_PD5, SomeValue));
    EXPECT_EQ(DAC0.DATA, Zero);
}

/**
 * @brief Test that PD6 can be turned back into an input, which disconnects the DAC from it.
 */
TEST(Analog, OutputCanBecomeAnInputAgain)
{
    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT));
    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_IN));
    EXPECT_EQ(DAC0.CTRLA, Zero);

    prepareConversion(SomeValue);
    EXPECT_EQ(analog_read(GPIO_PIN_PD6), SomeValue);
    EXPECT_FALSE(analog_write(GPIO_PIN_PD6, SomeValue));
}

/**
 * @brief Test that initializing a pin takes it from the digital driver.
 */
TEST(Analog, InitTakesThePinFromTheDigitalDriver)
{
    testHwPlatformReset();
    EXPECT_TRUE(digital_init(GPIO_PIN_PD1, GPIO_MODE_OUT));
    EXPECT_TRUE(digital_write(GPIO_PIN_PD1, true));

    EXPECT_TRUE(analog_init(GPIO_PIN_PD1, GPIO_MODE_IN));
    EXPECT_FALSE(digital_write(GPIO_PIN_PD1, false));
    EXPECT_FALSE(digital_toggle(GPIO_PIN_PD1));

    prepareConversion(SomeValue);
    EXPECT_EQ(analog_read(GPIO_PIN_PD1), SomeValue);
}

/**
 * @brief Test that a pin configured through the digital driver alone is not an analog input.
 */
TEST(Analog, DigitalPinsAreNotAnalogInputs)
{
    testHwPlatformReset();
    EXPECT_TRUE(digital_init(GPIO_PIN_PD1, GPIO_MODE_IN));
    prepareConversion(SomeValue);

    EXPECT_EQ(analog_read(GPIO_PIN_PD1), ANALOG_READ_ERROR);
}

/**
 * @brief Test that initializing another input leaves the output running.
 */
TEST(Analog, InputsAndTheOutputCoexist)
{
    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT));
    EXPECT_TRUE(analog_init(GPIO_PIN_PD1, GPIO_MODE_IN));
    EXPECT_EQ(DAC0.CTRLA, DacEnabled);

    // A full sequence: the output follows a potentiometer on PD1.
    prepareConversion(FullScale);
    EXPECT_TRUE(analog_write(GPIO_PIN_PD6, analog_read(GPIO_PIN_PD1)));
    EXPECT_EQ(DAC0.DATA, FullScaleData);
}
} // namespace
