/**
 * @file Tests for the digital GPIO driver.
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

/** Number of pins the driver supports, and the first pin number that does not exist. */
constexpr std::uint8_t PinCount{21U};
constexpr std::uint8_t InvalidPin{PinCount};

/** Number of the last pin, the pins being numbered from zero. */
constexpr std::uint8_t LastPin{PinCount - 1U};

/** Pin configuration left by the analog driver: digital input disabled, here with a pull-up. */
constexpr std::uint8_t AnalogWithPullUp{PORT_ISC_INPUT_DISABLE_gc | PORT_PULLUPEN_bm};

/** DAC control value with the converter enabled and its output connected to the pin. */
constexpr std::uint8_t DacEnabled{DAC_ENABLE_bm | DAC_OUTEN_bm};

/** An arbitrary analog output value. */
constexpr std::uint16_t SomeValue{1234U};

/** A mode outside the documented ones. */
constexpr gpio_mode_t InvalidMode{static_cast<gpio_mode_t>(GPIO_MODE_OUT + 1)};

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
};

/** Every pin of the driver, along with the hardware it is expected to use. */
const PinHw Pins[PinCount]{
    {GPIO_PIN_PA0, PORTA, PIN0_bm, PORTA.PIN0CTRL}, {GPIO_PIN_PA1, PORTA, PIN1_bm, PORTA.PIN1CTRL},
    {GPIO_PIN_PA2, PORTA, PIN2_bm, PORTA.PIN2CTRL}, {GPIO_PIN_PA3, PORTA, PIN3_bm, PORTA.PIN3CTRL},
    {GPIO_PIN_PA4, PORTA, PIN4_bm, PORTA.PIN4CTRL}, {GPIO_PIN_PA5, PORTA, PIN5_bm, PORTA.PIN5CTRL},
    {GPIO_PIN_PA6, PORTA, PIN6_bm, PORTA.PIN6CTRL}, {GPIO_PIN_PA7, PORTA, PIN7_bm, PORTA.PIN7CTRL},
    {GPIO_PIN_PC0, PORTC, PIN0_bm, PORTC.PIN0CTRL}, {GPIO_PIN_PC1, PORTC, PIN1_bm, PORTC.PIN1CTRL},
    {GPIO_PIN_PC2, PORTC, PIN2_bm, PORTC.PIN2CTRL}, {GPIO_PIN_PC3, PORTC, PIN3_bm, PORTC.PIN3CTRL},
    {GPIO_PIN_PD1, PORTD, PIN1_bm, PORTD.PIN1CTRL}, {GPIO_PIN_PD2, PORTD, PIN2_bm, PORTD.PIN2CTRL},
    {GPIO_PIN_PD3, PORTD, PIN3_bm, PORTD.PIN3CTRL}, {GPIO_PIN_PD4, PORTD, PIN4_bm, PORTD.PIN4CTRL},
    {GPIO_PIN_PD5, PORTD, PIN5_bm, PORTD.PIN5CTRL}, {GPIO_PIN_PD6, PORTD, PIN6_bm, PORTD.PIN6CTRL},
    {GPIO_PIN_PD7, PORTD, PIN7_bm, PORTD.PIN7CTRL}, {GPIO_PIN_PF0, PORTF, PIN0_bm, PORTF.PIN0CTRL},
    {GPIO_PIN_PF1, PORTF, PIN1_bm, PORTF.PIN1CTRL},
};

/**
 * @brief Test that initializing a pin as an output sets its direction bit, and only that bit.
 */
TEST(Digital, InitAsOutputSetsTheDirectionBit)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        EXPECT_TRUE(digital_init(hw.pin, GPIO_MODE_OUT));

        EXPECT_EQ(hw.port.DIR, hw.mask);
        const auto directions =
            static_cast<std::uint8_t>(PORTA.DIR | PORTC.DIR | PORTD.DIR | PORTF.DIR);
        EXPECT_EQ(directions, hw.mask);
    }
}

/**
 * @brief Test that initializing a pin as an input clears its direction bit, and only that bit.
 */
TEST(Digital, InitAsInputClearsTheDirectionBit)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();

        // Every direction bit set except the pin's own.
        const auto input = static_cast<std::uint8_t>(AllPinsHigh & ~hw.mask);
        hw.port.DIR      = AllPinsHigh;
        EXPECT_TRUE(digital_init(hw.pin, GPIO_MODE_IN));
        EXPECT_EQ(hw.port.DIR, input);
    }
}

/**
 * @brief Test that initializing a pin never drives it.
 */
TEST(Digital, InitDoesNotDriveThePin)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        EXPECT_TRUE(digital_init(hw.pin, GPIO_MODE_OUT));

        EXPECT_EQ(hw.port.OUT, Zero);
        EXPECT_EQ(hw.port.OUTSET, Zero);
        EXPECT_EQ(hw.port.OUTCLR, Zero);
        EXPECT_EQ(hw.port.OUTTGL, Zero);
    }
}

/**
 * @brief Test that initializing a pin enables its digital input and keeps its pull-up setting.
 */
TEST(Digital, InitEnablesTheDigitalInput)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        hw.pinctrl = AnalogWithPullUp;
        EXPECT_TRUE(digital_init(hw.pin, GPIO_MODE_IN));
        EXPECT_EQ(hw.pinctrl, PORT_PULLUPEN_bm);

        hw.pinctrl = AnalogWithPullUp;
        EXPECT_TRUE(digital_init(hw.pin, GPIO_MODE_OUT));
        EXPECT_EQ(hw.pinctrl, PORT_PULLUPEN_bm);
    }
}

/**
 * @brief Test that initializing a pin leaves the configuration of every other pin alone.
 */
TEST(Digital, InitLeavesOtherPinConfigurationsUntouched)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        for (const auto& other : Pins)
        {
            other.pinctrl = AnalogWithPullUp;
        }
        EXPECT_TRUE(digital_init(hw.pin, GPIO_MODE_IN));

        for (const auto& other : Pins)
        {
            if (other.pin == hw.pin) { continue; }
            EXPECT_EQ(other.pinctrl, AnalogWithPullUp);
        }
    }
}

/**
 * @brief Test that initializing PD6 disconnects the DAC from it, and that no other pin does.
 */
TEST(Digital, InitDisconnectsTheDacFromItsPinOnly)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        DAC0.CTRLA = DacEnabled;
        EXPECT_TRUE(digital_init(hw.pin, GPIO_MODE_OUT));

        const std::uint8_t expected{(GPIO_PIN_PD6 == hw.pin) ? Zero : DacEnabled};
        EXPECT_EQ(DAC0.CTRLA, expected);
    }
}

/**
 * @brief Test that a pin used as an analog input can be taken back as a digital pin.
 */
TEST(Digital, InitTakesThePinFromTheAnalogInput)
{
    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD1, GPIO_MODE_IN));
    EXPECT_TRUE(digital_init(GPIO_PIN_PD1, GPIO_MODE_IN));

    PORTD.IN = PIN1_bm;
    EXPECT_TRUE(digital_read(GPIO_PIN_PD1));
    EXPECT_EQ(PORTD.PIN1CTRL, Zero);
    EXPECT_EQ(analog_read(GPIO_PIN_PD1), ANALOG_READ_ERROR);
}

/**
 * @brief Test that the analog output can be taken back as a digital pin.
 */
TEST(Digital, InitTakesThePinFromTheAnalogOutput)
{
    testHwPlatformReset();
    EXPECT_TRUE(analog_init(GPIO_PIN_PD6, GPIO_MODE_OUT));
    EXPECT_TRUE(digital_init(GPIO_PIN_PD6, GPIO_MODE_OUT));

    EXPECT_EQ(DAC0.CTRLA, Zero);
    EXPECT_EQ(PORTD.PIN6CTRL, Zero);
    EXPECT_TRUE(digital_write(GPIO_PIN_PD6, true));
    EXPECT_EQ(PORTD.OUTSET, PIN6_bm);
    EXPECT_FALSE(analog_write(GPIO_PIN_PD6, SomeValue));
}

/**
 * @brief Test that initializing a pin keeps the other pins of the port as they were.
 */
TEST(Digital, InitKeepsTheOtherPinsOfThePort)
{
    constexpr std::uint8_t outputs{static_cast<std::uint8_t>(PIN0_bm | PIN7_bm)};

    testHwPlatformReset();
    EXPECT_TRUE(digital_init(GPIO_PIN_PA0, GPIO_MODE_OUT));
    EXPECT_TRUE(digital_init(GPIO_PIN_PA7, GPIO_MODE_OUT));
    EXPECT_TRUE(digital_init(GPIO_PIN_PA3, GPIO_MODE_IN));
    EXPECT_EQ(PORTA.DIR, outputs);

    EXPECT_TRUE(digital_init(GPIO_PIN_PA0, GPIO_MODE_IN));
    EXPECT_EQ(PORTA.DIR, PIN7_bm);
}

/**
 * @brief Test that initializing a pin again changes its mode.
 */
TEST(Digital, InitCanChangeTheMode)
{
    testHwPlatformReset();
    EXPECT_TRUE(digital_init(GPIO_PIN_PD4, GPIO_MODE_OUT));
    EXPECT_TRUE(digital_write(GPIO_PIN_PD4, true));

    EXPECT_TRUE(digital_init(GPIO_PIN_PD4, GPIO_MODE_IN));
    EXPECT_FALSE(digital_write(GPIO_PIN_PD4, true));

    EXPECT_TRUE(digital_init(GPIO_PIN_PD4, GPIO_MODE_OUT));
    EXPECT_TRUE(digital_toggle(GPIO_PIN_PD4));
}

/**
 * @brief Test that the pins are numbered without gaps, the last one being PF1.
 */
TEST(Digital, PinsAreNumberedWithoutGaps)
{
    std::uint8_t expected{};

    for (const auto& hw : Pins)
    {
        const auto id = static_cast<std::uint8_t>(hw.pin);
        EXPECT_EQ(id, expected);
        ++expected;
    }
    constexpr std::uint8_t lastId{static_cast<std::uint8_t>(GPIO_PIN_PF1)};
    EXPECT_EQ(lastId, LastPin);
}

/**
 * @brief Test that an invalid pin or mode is rejected without touching any register.
 */
TEST(Digital, InitRejectsInvalidArguments)
{
    testHwPlatformReset();
    EXPECT_FALSE(digital_init(InvalidPin, GPIO_MODE_OUT));
    EXPECT_FALSE(digital_init(AllPinsHigh, GPIO_MODE_OUT));
    EXPECT_FALSE(digital_init(GPIO_PIN_PA0, InvalidMode));

    const auto directions =
        static_cast<std::uint8_t>(PORTA.DIR | PORTC.DIR | PORTD.DIR | PORTF.DIR);
    EXPECT_EQ(directions, Zero);
}

/**
 * @brief Test that writing an output sets the corresponding bit in OUTSET or OUTCLR.
 */
TEST(Digital, WriteDrivesAnOutput)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        EXPECT_TRUE(digital_init(hw.pin, GPIO_MODE_OUT));

        EXPECT_TRUE(digital_write(hw.pin, true));
        EXPECT_EQ(hw.port.OUTSET, hw.mask);
        EXPECT_EQ(hw.port.OUTCLR, Zero);

        EXPECT_TRUE(digital_write(hw.pin, false));
        EXPECT_EQ(hw.port.OUTCLR, hw.mask);
    }
}

/**
 * @brief Test that toggling an output sets the corresponding bit in OUTTGL.
 */
TEST(Digital, ToggleInvertsAnOutput)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        EXPECT_TRUE(digital_init(hw.pin, GPIO_MODE_OUT));
        EXPECT_TRUE(digital_toggle(hw.pin));

        EXPECT_EQ(hw.port.OUTTGL, hw.mask);
        EXPECT_EQ(hw.port.OUTSET, Zero);
        EXPECT_EQ(hw.port.OUTCLR, Zero);
    }
}

/**
 * @brief Test that driving one pin leaves the other I/O ports untouched.
 */
TEST(Digital, WriteAndToggleLeaveOtherPortsUntouched)
{
    testHwPlatformReset();
    EXPECT_TRUE(digital_init(GPIO_PIN_PC2, GPIO_MODE_OUT));
    EXPECT_TRUE(digital_write(GPIO_PIN_PC2, true));
    EXPECT_TRUE(digital_write(GPIO_PIN_PC2, false));
    EXPECT_TRUE(digital_toggle(GPIO_PIN_PC2));

    for (const PORT_t* port : {&PORTA, &PORTD, &PORTF})
    {
        const auto outputWrites =
            static_cast<std::uint8_t>(port->OUTSET | port->OUTCLR | port->OUTTGL);
        EXPECT_EQ(outputWrites, Zero);
    }
}

/**
 * @brief Test that writing and toggling are refused for pins that were never initialized.
 */
TEST(Digital, WriteAndToggleAreRefusedForUninitializedPins)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        EXPECT_FALSE(digital_write(hw.pin, true));
        EXPECT_FALSE(digital_toggle(hw.pin));

        const auto outputWrites =
            static_cast<std::uint8_t>(hw.port.OUTSET | hw.port.OUTCLR | hw.port.OUTTGL);
        EXPECT_EQ(outputWrites, Zero);
    }
}

/**
 * @brief Test that writing and toggling are refused for pins configured as inputs.
 */
TEST(Digital, WriteAndToggleAreRefusedForInputs)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();

        EXPECT_TRUE(digital_init(hw.pin, GPIO_MODE_IN));
        EXPECT_FALSE(digital_write(hw.pin, true));
        EXPECT_FALSE(digital_write(hw.pin, false));
        EXPECT_FALSE(digital_toggle(hw.pin));

        EXPECT_EQ(hw.port.OUTSET, Zero);
        EXPECT_EQ(hw.port.OUTCLR, Zero);
        EXPECT_EQ(hw.port.OUTTGL, Zero);
    }
}

/**
 * @brief Test that writing and toggling are refused for pins that do not exist.
 */
TEST(Digital, WriteAndToggleAreRefusedForInvalidPins)
{
    testHwPlatformReset();
    PORTA.DIR = AllPinsHigh;
    PORTC.DIR = AllPinsHigh;
    PORTD.DIR = AllPinsHigh;
    PORTF.DIR = AllPinsHigh;

    EXPECT_FALSE(digital_write(InvalidPin, true));
    EXPECT_FALSE(digital_toggle(InvalidPin));

    const auto setWrites =
        static_cast<std::uint8_t>(PORTA.OUTSET | PORTC.OUTSET | PORTD.OUTSET | PORTF.OUTSET);
    EXPECT_EQ(setWrites, Zero);
    const auto toggleWrites =
        static_cast<std::uint8_t>(PORTA.OUTTGL | PORTC.OUTTGL | PORTD.OUTTGL | PORTF.OUTTGL);
    EXPECT_EQ(toggleWrites, Zero);
}

/**
 * @brief Test that reading an input returns the level on its own pin.
 */
TEST(Digital, ReadReturnsTheLevelOfAnInput)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        EXPECT_TRUE(digital_init(hw.pin, GPIO_MODE_IN));
        EXPECT_FALSE(digital_read(hw.pin));

        hw.port.IN = hw.mask;
        EXPECT_TRUE(digital_read(hw.pin));

        // Every pin high except the one being read.
        const auto othersHigh = static_cast<std::uint8_t>(AllPinsHigh & ~hw.mask);
        hw.port.IN            = othersHigh;
        EXPECT_FALSE(digital_read(hw.pin));
    }
}

/**
 * @brief Test that reading an output returns the state held in the OUT register.
 */
TEST(Digital, ReadReturnsTheStateOfAnOutput)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        EXPECT_TRUE(digital_init(hw.pin, GPIO_MODE_OUT));
        EXPECT_FALSE(digital_read(hw.pin));

        hw.port.OUT = hw.mask;
        EXPECT_TRUE(digital_read(hw.pin));
    }
}

/**
 * @brief Test that an input is read from the IN register and an output from the OUT register.
 */
TEST(Digital, ReadUsesTheRegisterOfThePinsMode)
{
    for (const auto& hw : Pins)
    {
        testHwPlatformReset();
        EXPECT_TRUE(digital_init(hw.pin, GPIO_MODE_IN));
        hw.port.OUT = hw.mask;
        EXPECT_FALSE(digital_read(hw.pin));

        testHwPlatformReset();
        EXPECT_TRUE(digital_init(hw.pin, GPIO_MODE_OUT));
        hw.port.IN = hw.mask;
        EXPECT_FALSE(digital_read(hw.pin));
    }
}

/**
 * @brief Test that a pin that was never initialized can still be read as an input.
 */
TEST(Digital, ReadWorksForUninitializedPins)
{
    testHwPlatformReset();
    EXPECT_FALSE(digital_read(GPIO_PIN_PF1));
    PORTF.IN = PIN1_bm;
    EXPECT_TRUE(digital_read(GPIO_PIN_PF1));
}

/**
 * @brief Test that reading a pin doesn't disturb any register.
 */
TEST(Digital, ReadDoesNotWriteRegisters)
{
    testHwPlatformReset();
    for (const auto& hw : Pins)
    {
        (void)(digital_read(hw.pin));
    }

    for (const PORT_t* port : {&PORTA, &PORTC, &PORTD, &PORTF})
    {
        const auto pinState = static_cast<std::uint8_t>(port->DIR | port->OUT);
        EXPECT_EQ(pinState, Zero);
        const auto outputWrites =
            static_cast<std::uint8_t>(port->OUTSET | port->OUTCLR | port->OUTTGL);
        EXPECT_EQ(outputWrites, Zero);
    }
}

/**
 * @brief Test that a pin that does not exist reads low, whatever the levels on the ports.
 */
TEST(Digital, ReadReturnsFalseForInvalidPins)
{
    testHwPlatformReset();
    PORTA.IN = AllPinsHigh;
    PORTC.IN = AllPinsHigh;
    PORTD.IN = AllPinsHigh;
    PORTF.IN = AllPinsHigh;

    EXPECT_FALSE(digital_read(InvalidPin));
    EXPECT_FALSE(digital_read(AllPinsHigh));
}

/**
 * @brief Test a full sequence: a LED on PC0 driven from a switch on PF0.
 */
TEST(Digital, DigitalSequence)
{
    testHwPlatformReset();
    EXPECT_TRUE(digital_init(GPIO_PIN_PC0, GPIO_MODE_OUT));
    EXPECT_TRUE(digital_init(GPIO_PIN_PF0, GPIO_MODE_IN));

    PORTF.IN = PIN0_bm;
    EXPECT_TRUE(digital_write(GPIO_PIN_PC0, digital_read(GPIO_PIN_PF0)));
    EXPECT_EQ(PORTC.OUTSET, PIN0_bm);

    PORTF.IN = Zero;
    EXPECT_TRUE(digital_write(GPIO_PIN_PC0, digital_read(GPIO_PIN_PF0)));
    EXPECT_EQ(PORTC.OUTCLR, PIN0_bm);
}
} // namespace
