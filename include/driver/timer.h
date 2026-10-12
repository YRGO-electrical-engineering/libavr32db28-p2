/**
 * @file AVR32DB28 timer driver.
 */
#ifndef TIMER_H_
#define TIMER_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Enumeration of timer IDs.
 */
typedef enum
{
    TIMER_ID_0,    ///< ID for timer 0.
    TIMER_ID_1,    ///< ID for timer 1.
    TIMER_ID_2,    ///< ID for timer 2.
    TIMER_ID_NONE, ///< No timer available.
} timer_id_t;

/**
 * @brief Reserve a timer circuit.
 *
 * @param[in] timeout_ms Timeout in milliseconds. Must be greater than 0.
 *
 * @return ID of the reserved timer instance.
 *
 *         TIMER_ID_NONE is returned if no timer is available, or if the timeout is invalid.
 *
 * @note The reserved timer is stopped. Call timer_start() to start it.
 */
timer_id_t timer_init(uint16_t timeout_ms);

/**
 * @brief Release timer circuit.
 *
 * @param[in] id ID of the timer instance.
 */
void timer_deinit(timer_id_t id);

/**
 * @brief Check if the timer is running.
 *
 * @param[in] id ID of the timer instance.
 *
 * @return True if the timer is running, false otherwise.
 */
bool timer_running(timer_id_t id);

/**
 * @brief Start timer.
 *
 * @param[in] id ID of the timer instance.
 */
void timer_start(timer_id_t id);

/**
 * @brief Stop timer.
 *
 * @param[in] id ID of the timer instance.
 */
void timer_stop(timer_id_t id);

/**
 * @brief Toggle timer.
 *
 * @param[in] id ID of the timer instance.
 */
void timer_toggle(timer_id_t id);

/**
 * @brief Check if timer has elapsed.
 *
 * @param[in] id ID of the timer instance.
 *
 * @return True if the timer has elapsed, false otherwise.
 *
 * @note Call this function often, at least once every millisecond, for example in the main
 *       loop. The timers only keep track of the time while the program is asking them to, so
 *       slow code makes every timer run slow.
 */
bool timer_elapsed(timer_id_t id);

#endif /** TIMER_H_ */
