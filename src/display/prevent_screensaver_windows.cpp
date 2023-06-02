/*
 * 2023 Arnaud Sellier
 *
 * Software: VCS
 *
 */

#include <windows.h>
#include "main.h"

subsystem_releaser_t kd_prevent_screensaver(void)
{
    SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
    return []{}; // no need to unset, OS takes care of that
}
