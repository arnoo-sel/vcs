/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 * A subsystem providing basic timer functionality for automatic execution of code
 * at given intervals.
 *
 * Usage:
 *
 *   1. Call kt_initialize_timers() to initialize the timer subsystem.
 *
 *   2. Call kt_timer() to add a new timer. The function takes in an interval and
 *      a function; the function will be executed at about the given intervals.
 *
 *   3. To stop and release all timers, call kt_release_timers(). Individual
 *      timers cannot currently be stopped.
 *
 */

#include <vector>
#include "common/timer/timer.h"

static std::vector<timer_c> ACTIVE_TIMERS;

void kt_timer(const unsigned intervalMs, std::function<void(const unsigned elapsedMs)> functionToRun)
{
    ACTIVE_TIMERS.push_back(timer_c(intervalMs, functionToRun));

    return;
}

void kt_update_timers(void)
{
    for (timer_c &timer: ACTIVE_TIMERS)
    {
        timer.update();
    }

    return;
}
