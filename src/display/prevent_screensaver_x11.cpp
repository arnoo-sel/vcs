/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
#include "display/display.h"
#include "common/timer/timer.h"
#include "common/globals.h"

subsystem_releaser_t kd_prevent_screensaver(void)
{
    // Note: We're using kludgy X-specific methods rather than inhibiting via D-Bus,
    // since cross-distro support for the D-Bus way looks sketchy to me. These methods
    // should be more portable across the X ecosystem, and less bother to implement.
    Display *display = XOpenDisplay(nullptr);

    if (display)
    {
        CARD16 powerLevel = 0;
        BOOL wasDpmsEnabled = false;
        if (
            DPMSCapable(display) &&
            DPMSInfo(display, &powerLevel, &wasDpmsEnabled) &&
            wasDpmsEnabled
        )
        {
            DPMSDisable(display);
            XSync(display, false);
        }

        kt_timer(50000, [display](const unsigned)
        {
            XResetScreenSaver(display);
        });

        return [display, wasDpmsEnabled]{
            if (display && wasDpmsEnabled)
            {
                DPMSEnable(display);
                XSync(display, false);
            }
        };
    }
    else
    {
        NBENE(("Could not access the X display for preventing the screensaver."));
        return []{};
    }
}
