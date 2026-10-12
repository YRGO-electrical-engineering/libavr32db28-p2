/**
 * @file AVR32DB28 7-segment display driver implementation details.
 */
#include <stdbool.h>
#include <stdint.h>

#include "arch/avr/hw_platform.h"
#include "driver/display.h"
#include "driver/timer.h"

#define PORT PORTA // Port register for the display.

#define BCD_MASK (PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm) // BCD pins (PORTA0 - PORTA3).
#define DP_MASK PIN4_bm                                  // Decimal point pin (PORTA4).
#define DIGIT1_MASK PIN5_bm                              // Digit 1 select pin (PORTA5).
#define DIGIT2_MASK PIN6_bm                              // Digit 2 select pin (PORTA6).
#define DIGIT3_MASK PIN7_bm                              // Digit 3 select pin (PORTA7).

#define DIGIT_MASK (DIGIT1_MASK | DIGIT2_MASK | DIGIT3_MASK) // Every digit select.
#define DISPLAY_MASK (BCD_MASK | DP_MASK | DIGIT_MASK)       // Every pin the display uses.

#define BCD_BLANK 15U     // BCD value blanking a digit; the decoder only knows 0 - 9.
#define VALUE_MAX 999U    // Largest value three digits can show.
#define TENS_MIN 10U      // Smallest value with a digit in the tens.
#define HUNDREDS_MIN 100U // Smallest value with a digit in the hundreds.
#define BASE 10U          // Number base, i.e. how the value is split between the digits.
#define NO_VALUE (-1)     // Returned when the display is blank and has no value to report.

#define REFRESH_MS 3U // Time each digit is lit, giving the display 111 refreshes per second.

/**
 * @brief Enumeration of digits.
 */
typedef enum
{
    DIGIT1,      ///< The first digit, i.e. the hundreds.
    DIGIT2,      ///< The second digit, i.e. the tens.
    DIGIT3,      ///< The third digit, i.e. the ones.
    DIGIT_COUNT, ///< Number of digits.
} digit_t;

#define DP_DIGIT DIGIT2 // Digit the decimal point is lit with, placing it before the ones.

/** Value being displayed (0 - 999). */
static uint16_t display_value = 0U;

/** True if the display is blank, false otherwise. */
static bool blanked = true;

/** Whether the decimal point is shown. */
static bool dp_shown = false;

/** Current digit. */
static digit_t current_digit = DIGIT1;

/** Timer deciding when to move on to the next digit. */
static timer_id_t refresh_timer = TIMER_ID_NONE;

// -----------------------------------------------------------------------------
static inline uint8_t get_hundreds(void) { return (uint8_t)(display_value / HUNDREDS_MIN); }

// -----------------------------------------------------------------------------
static inline uint8_t get_tens(void) { return (uint8_t)((display_value / BASE) % BASE); }

// -----------------------------------------------------------------------------
static inline uint8_t get_ones(void) { return (uint8_t)(display_value % BASE); }

// -----------------------------------------------------------------------------
static inline bool value_valid(const uint16_t value) { return VALUE_MAX >= value; }

// -----------------------------------------------------------------------------
static inline bool hundreds_blanked(void)
{
    // The hundreds are blank when the value has none.
    return HUNDREDS_MIN > display_value;
}

// -----------------------------------------------------------------------------
static inline bool tens_blanked(void)
{
    // The tens are blank when the value is below ten. The zero is kept when the decimal point
    // is shown, since 0.7 reads better than .7.
    return (TENS_MIN > display_value) && !dp_shown;
}

// -----------------------------------------------------------------------------
static inline bool dp_lit(const digit_t digit)
{
    // The decimal point is a segment like any other, so it belongs to whichever digit happens
    // to be lit. Lighting it with the second digit places it before the ones.
    return dp_shown && !blanked && (DP_DIGIT == digit);
}

// -----------------------------------------------------------------------------
static inline digit_t next_digit(const digit_t digit)
{
    // Return the digit after the given one, starting over after the last.
    return (digit_t)((digit + 1U) % DIGIT_COUNT);
}

// -----------------------------------------------------------------------------
static inline bool timer_reserved(void) { return TIMER_ID_NONE != refresh_timer; }

// -----------------------------------------------------------------------------
static inline uint8_t get_select_mask(const digit_t digit)
{
    // Return the select pin of the given digit.
    switch (digit)
    {
        case DIGIT1:
            return DIGIT1_MASK;
        case DIGIT2:
            return DIGIT2_MASK;
        default:
            return DIGIT3_MASK;
    }
}

// -----------------------------------------------------------------------------
static uint8_t get_bcd(const digit_t digit)
{
    // A blank display shows nothing on any digit.
    if (blanked) { return BCD_BLANK; }

    switch (digit)
    {
        case DIGIT1:
            // The first digit shows the hundreds.
            return hundreds_blanked() ? BCD_BLANK : get_hundreds();
        case DIGIT2:
            // The second digit shows the tens.
            return tens_blanked() ? BCD_BLANK : get_tens();
        default:
            // The third digit always shows the ones.
            return get_ones();
    }
}

// -----------------------------------------------------------------------------
static void show_digit(const digit_t digit)
{
    const uint8_t bcd    = get_bcd(digit);
    const uint8_t select = get_select_mask(digit);

    // Split the pins into the ones to drive high and the ones to drive low, starting with the
    // BCD value the decoder is to translate into segments.
    uint8_t high = (uint8_t)(bcd & BCD_MASK);
    uint8_t low  = (uint8_t)(~bcd & BCD_MASK);

    // The digits share their segment lines, so only one of them may be lit at a time. A digit
    // lights when its own pin is driven low and the other ones are left high.
    low |= select;
    high |= (uint8_t)(DIGIT_MASK & ~select);

    // Light the decimal point if it belongs to this digit.
    if (dp_lit(digit)) { high |= DP_MASK; }
    else { low |= DP_MASK; }

    // Drive the pins high first and low second, so that the previous digit is switched off by the
    // first write before this one is selected by the second.
    PORT.OUTSET = high;
    PORT.OUTCLR = low;
}

// -----------------------------------------------------------------------------
bool display_init(void)
{
    // Blank the display: every digit select high switches the digits off, and a BCD value above
    // nine leaves the decoder with nothing to show. The levels are set before the pins become
    // outputs, so that no digit is selected in between.
    PORT.OUTSET = DIGIT_MASK | BCD_MASK;
    PORT.OUTCLR = DP_MASK;

    // Configure every pin the display uses as an output.
    PORT.DIR |= DISPLAY_MASK;

    // Start over from a blank display.
    display_value = 0U;
    blanked       = true;
    dp_shown      = false;
    current_digit = DIGIT1;

    // Reserve a timer to take turns between the digits with.
    timer_deinit(refresh_timer);
    refresh_timer = timer_init(REFRESH_MS);
    timer_start(refresh_timer);

    // Return true if a timer was reserved, false otherwise.
    return timer_reserved();
}

// -----------------------------------------------------------------------------
void display_clear(void)
{
    // Blank the display. It stays blank until a new value is written.
    blanked = true;
}

// -----------------------------------------------------------------------------
int16_t display_read(void)
{
    // Return the current value if the display is not blank.
    return blanked ? NO_VALUE : (int16_t)(display_value);
}

// -----------------------------------------------------------------------------
void display_write(const uint16_t value)
{
    // Ignore a value three digits can't show.
    if (!value_valid(value)) { return; }
    display_value = value;
    blanked       = false;
}

// -----------------------------------------------------------------------------
void display_update(void)
{
    // Check if the refresh timer has elapsed, do nothing if not.
    if (!timer_elapsed(refresh_timer)) { return; }

    // Light the digit whose turn it is, then hand the next turn to the digit after it.
    show_digit(current_digit);
    current_digit = next_digit(current_digit);
}

// -----------------------------------------------------------------------------
void display_show_dp(const bool show)
{
    // Show or hide the decimal point from the next update onwards.
    dp_shown = show;
}
