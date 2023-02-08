/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <X11/Xlib.h>
#include "display/display.h"
#include "common/timer/timer.h"

void kd_prevent_screensaver(void)
{
    // Note: We're using the kludgy XResetScreenSaver() method rather than inhibiting
    // via D-Bus, since cross-distro support for the D-Bus way looks sketchy to me.
    // This method should be more portable across the X11 ecosystem, and less bother
    // to implement.
    Display *display = XOpenDisplay(nullptr);

    if (display)
    {
        kt_timer(50000, [display](const unsigned)
        {
            XResetScreenSaver(display);
        });
    }

    return;
}
