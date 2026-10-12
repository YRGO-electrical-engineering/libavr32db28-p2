/**
 * @file AVR32DB28 timer driver implementation details.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "arch/avr/hw_platform.h"
#include "driver/timer.h"

#define TIMER_COUNT TIMER_ID_NONE                                     // Number of timer circuits.
#define TIMER_PRESCALER 2UL                                           // Clock divisor.
#define TICKS_PER_SEC 1000UL                                          // Ticks per second.
#define TIMER_FREQ (F_CPU / TIMER_PRESCALER)                          // Timer frequency in Hz.
#define TIMER_PERIOD ((uint16_t)((TIMER_FREQ / TICKS_PER_SEC) - 1UL)) // Counter top value.

/** Timeout of each timer measured in milliseconds. */
static uint16_t timeouts_ms[TIMER_COUNT] = {0U, 0U, 0U};

/** Internal millisecond counter of each timer. */
static uint16_t counters_ms[TIMER_COUNT] = {0U, 0U, 0U};

/** Indicates whether each timer has been reserved, i.e. is associated with a circuit. */
static bool reserved[TIMER_COUNT] = {false, false, false};

// -----------------------------------------------------------------------------
static inline TCB_t* get_circuit(const timer_id_t id)
{
    // Return a pointer to the circuit associated with the given ID, or NULL if invalid.
    switch (id)
    {
        case TIMER_ID_0:
            return &TCB0;
        case TIMER_ID_1:
            return &TCB1;
        case TIMER_ID_2:
            return &TCB2;
        default:
            return NULL;
    }
}

// -----------------------------------------------------------------------------
static inline bool timer_reserved(const timer_id_t id)
{
    // A timer is usable once its ID is in range and the timer has been reserved.
    return (TIMER_ID_NONE > id) && reserved[id];
}

// -----------------------------------------------------------------------------
static inline bool timeout_valid(const uint16_t timeout_ms) { return 0U != timeout_ms; }

// -----------------------------------------------------------------------------
static inline timer_id_t find_free_timer(void)
{
    // Return the ID of the first timer that hasn't been reserved yet.
    for (uint8_t id = 0U; id < TIMER_COUNT; ++id)
    {
        if (!reserved[id]) { return (timer_id_t)(id); }
    }
    return TIMER_ID_NONE;
}

// -----------------------------------------------------------------------------
static inline void reserve_timer(const timer_id_t id, const uint16_t timeout_ms)
{
    // Store the timeout and mark the timer as reserved.
    timeouts_ms[id] = timeout_ms;
    counters_ms[id] = 0U;
    reserved[id]    = true;
}

// -----------------------------------------------------------------------------
static inline void release_timer(const timer_id_t id)
{
    // Forget the timeout and mark the timer as free.
    timeouts_ms[id] = 0U;
    counters_ms[id] = 0U;
    reserved[id]    = false;
}

// -----------------------------------------------------------------------------
static inline void init_circuit(TCB_t* circuit)
{
    // Initialize the circuit to count a single millisecond, with its interrupt disabled.
    circuit->CCMP     = TIMER_PERIOD;
    circuit->CTRLB    = TCB_CNTMODE_INT_gc;
    circuit->INTCTRL  = 0U;
    circuit->INTFLAGS = TCB_CAPT_bm;
    circuit->CTRLA    = TCB_CLKSEL_DIV2_gc;
}

// -----------------------------------------------------------------------------
static inline void start_circuit(TCB_t* circuit)
{
    // Start counting from zero, with the flag of the previous millisecond lowered.
    circuit->CNT      = 0U;
    circuit->INTFLAGS = TCB_CAPT_bm;
    circuit->CTRLA    = TCB_CLKSEL_DIV2_gc | TCB_ENABLE_bm;
}

// -----------------------------------------------------------------------------
static inline void stop_circuit(TCB_t* circuit)
{
    // Stop the circuit, keeping the clock source selected so it can be restarted.
    circuit->CTRLA = TCB_CLKSEL_DIV2_gc;
}

// -----------------------------------------------------------------------------
static inline bool circuit_running(const TCB_t* circuit)
{
    return (bool)(circuit->CTRLA & TCB_ENABLE_bm);
}

// -----------------------------------------------------------------------------
static inline bool ms_passed(const TCB_t* circuit)
{
    return (bool)(circuit->INTFLAGS & TCB_CAPT_bm);
}

// -----------------------------------------------------------------------------
static inline void clear_ms_flag(TCB_t* circuit) { circuit->INTFLAGS = TCB_CAPT_bm; }

// -----------------------------------------------------------------------------
static inline void count_ms(const timer_id_t id)
{
    // Count another millisecond, stopping at the largest value instead of wrapping around.
    if (UINT16_MAX > counters_ms[id]) { counters_ms[id]++; }
}

// -----------------------------------------------------------------------------
static inline bool timed_out(const timer_id_t id) { return timeouts_ms[id] <= counters_ms[id]; }

// -----------------------------------------------------------------------------
static void poll_circuits(void)
{
    for (uint8_t id = 0U; id < TIMER_COUNT; ++id)
    {
        if (!reserved[id]) { continue; }
        TCB_t* circuit = get_circuit((timer_id_t)(id));

        // Count the millisecond if one has passed, then lower the flag announcing it.
        if (ms_passed(circuit))
        {
            count_ms((timer_id_t)(id));
            clear_ms_flag(circuit);
        }
    }
}

// -----------------------------------------------------------------------------
timer_id_t timer_init(const uint16_t timeout_ms)
{
    // Check timeout, return TIMER_ID_NONE if invalid.
    if (!timeout_valid(timeout_ms)) { return TIMER_ID_NONE; }

    // Find the first free timer circuit, return TIMER_ID_NONE if none is available.
    const timer_id_t id = find_free_timer();
    if (TIMER_ID_NONE == id) { return TIMER_ID_NONE; }

    // Initialize the circuit and reserve the timer, then return its ID.
    init_circuit(get_circuit(id));
    reserve_timer(id, timeout_ms);
    return id;
}

// -----------------------------------------------------------------------------
void timer_deinit(const timer_id_t id)
{
    // Terminate if the timer isn't reserved.
    if (!timer_reserved(id)) { return; }

    // Stop the circuit, then release the timer.
    stop_circuit(get_circuit(id));
    release_timer(id);
}

// -----------------------------------------------------------------------------
bool timer_running(const timer_id_t id)
{
    // Return false if the timer isn't reserved.
    if (!timer_reserved(id)) { return false; }

    // Return true if the timer is running, false otherwise.
    return circuit_running(get_circuit(id));
}

// -----------------------------------------------------------------------------
void timer_start(const timer_id_t id)
{
    // Terminate if the timer isn't reserved.
    if (!timer_reserved(id)) { return; }

    // Clear the internal counter and start the timer.
    counters_ms[id] = 0U;
    start_circuit(get_circuit(id));
}

// -----------------------------------------------------------------------------
void timer_stop(const timer_id_t id)
{
    // Terminate if the timer isn't reserved.
    if (!timer_reserved(id)) { return; }

    stop_circuit(get_circuit(id));
}

// -----------------------------------------------------------------------------
void timer_toggle(const timer_id_t id)
{
    // Terminate if the timer isn't reserved.
    if (!timer_reserved(id)) { return; }

    // Stop the timer if it's running, else start it.
    if (timer_running(id)) { timer_stop(id); }
    else { timer_start(id); }
}

// -----------------------------------------------------------------------------
bool timer_elapsed(const timer_id_t id)
{
    poll_circuits();

    // Return false if the timer isn't reserved.
    if (!timer_reserved(id)) { return false; }

    // A stopped timer never elapses.
    if (!timer_running(id)) { return false; }

    // Return false if the timeout hasn't passed yet.
    if (!timed_out(id)) { return false; }

    // Clear the internal counter, so that the timer starts over.
    counters_ms[id] = 0U;
    return true;
}
