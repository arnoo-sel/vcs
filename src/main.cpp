/*
 * 2018-2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#if (!defined(CAPTURE_BACKEND_VIRTUAL) &&\
     !defined(CAPTURE_BACKEND_VISION_V4L) &&\
     !defined(CAPTURE_BACKEND_RGBEASY) &&\
     !defined(CAPTURE_BACKEND_DOSBOX_MMAP))
    #error "Unrecognized value for the capture backend toggle"
#endif

#include <chrono>
#include <thread>
#include <mutex>
#include <deque>
#include "display/qt/windows/output_window.h"
#include "common/command_line/command_line.h"
#include "anti_tear/anti_tear.h"
#include "capture/capture.h"
#include "display/display.h"
#include "common/globals.h"
#include "capture/alias.h"
#include "scaler/scaler.h"
#include "filter/filter.h"
#include "capture/video_presets.h"
#include "common/disk/disk.h"
#include "common/timer/timer.h"
#include "main.h"

#ifdef __SANITIZE_ADDRESS__
    #include <sanitizer/lsan_interface.h>
    const char* __lsan_default_options()
    {
        return (
            // Prevent __lsan_do_leak_check() from aborting when leaks are found.
            "exitcode=0:"

            // File located in relation to the program executable that optionally
            // suppresses leak reports for certain sources. For more info, see
            // https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer#suppression.
            "suppressions=asan-suppress.txt:"
        );
    }
#endif

vcs_event_c<void> k_evEcoModeEnabled;
vcs_event_c<void> k_evEcoModeDisabled;

static std::deque<subsystem_releaser_t> SUBSYSTEM_RELEASERS;

// Set to true when we want to break out of the program's main loop and terminate.
/// TODO. Don't have this be global. Instead provide a function to request state changes on it.
bool PROGRAM_EXIT_REQUESTED = false;

static bool IS_ECO_MODE_ENABLED = false;
static auto ECO_REFERENCE_TIME = std::chrono::system_clock::now();

static void prepare_for_exit(void)
{
    INFO(("Received orders to exit. Initiating cleanup."));

    while (!SUBSYSTEM_RELEASERS.empty())
    {
        auto releaser_fn = SUBSYSTEM_RELEASERS.back();
        SUBSYSTEM_RELEASERS.pop_back();
        releaser_fn();
    }

    // Force the memory leak check to run now rather than on program exit, so that
    // it ignores allocations that remain on exit and which we let the OS clean up.
    #ifdef __SANITIZE_ADDRESS__
      __lsan_do_leak_check();
    #endif

    INFO(("Ready to exit."));

    return;
}

static bool initialize_all(void)
{
    // Listen for app events.
    {
        k_evEcoModeEnabled.listen([]
        {
            ECO_REFERENCE_TIME = std::chrono::system_clock::now();
        });

        kc_evUnrecoverableError.listen([]
        {
            PROGRAM_EXIT_REQUESTED = true;
        });

        kc_evNewVideoMode.listen([](const video_mode_s &videoMode)
        {
            INFO((
                "Video mode: %u x %u at %.3f Hz.",
                videoMode.resolution.w,
                videoMode.resolution.h,
                videoMode.refreshRate.value<double>()
            ));
        });

        // The capture device has received a new video mode. We'll inspect the
        // mode to see if we think it's acceptable, then allow news of it to
        // propagate to the rest of VCS.
        kc_evNewProposedVideoMode.listen([](const video_mode_s &videoMode)
        {
            // If there's an alias for this resolution, force that resolution
            // instead. Note that forcing the resolution is expected to automatically
            // fire a capture.newVideoMode event.
            if (ka_has_alias(videoMode.resolution))
            {
                const resolution_s aliasResolution = ka_aliased(videoMode.resolution);
                kc_force_capture_resolution(aliasResolution);
            }
            else
            {
                kc_evNewVideoMode.fire(videoMode);
            }
        });
    }

    // Initialize subsystems.
    {
        SUBSYSTEM_RELEASERS.push_back(kvideopreset_initialize());
        SUBSYSTEM_RELEASERS.push_back(ks_initialize_scaler());
        SUBSYSTEM_RELEASERS.push_back(kc_initialize_capture());
        SUBSYSTEM_RELEASERS.push_back(kat_initialize_anti_tear());
        SUBSYSTEM_RELEASERS.push_back(kf_initialize_filters());
        if (kcom_prevent_screensaver())
        {
            SUBSYSTEM_RELEASERS.push_back(kd_prevent_screensaver());
        }

        // The display subsystem should be initialized last.
        SUBSYSTEM_RELEASERS.push_back(kd_acquire_output_window());
    }

    return !PROGRAM_EXIT_REQUESTED;
}

static capture_event_e process_next_capture_event(void)
{
    std::lock_guard<std::mutex> lock(kc_capture_mutex());
    const capture_event_e e = kc_pop_capture_event_queue();

    switch (e)
    {
        case capture_event_e::unrecoverable_error:
        {
            NBENE(("The capture device has reported an unrecoverable error."));

            kc_evUnrecoverableError.fire();

            break;
        }
        case capture_event_e::new_frame:
        {
            if (kc_has_valid_signal())
            {
                const auto &frame = kc_get_frame_buffer();
                kc_evNewCapturedFrame.fire(frame);
            }

            kc_mark_frame_buffer_as_processed();

            break;
        }
        case capture_event_e::new_video_mode:
        {
            if (kc_has_valid_signal())
            {
                kc_evNewProposedVideoMode.fire({
                    kc_get_capture_resolution(),
                    kc_get_capture_refresh_rate(),
                });
            }

            break;
        }
        case capture_event_e::signal_lost:
        {
            kc_evSignalLost.fire();
            break;
        }
        case capture_event_e::signal_gained:
        {
            kc_evSignalGained.fire();
            break;
        }
        case capture_event_e::invalid_signal:
        {
            kc_evInvalidSignal.fire();
            break;
        }
        case capture_event_e::invalid_device:
        {
            kc_evInvalidDevice.fire();
            break;
        }
        case capture_event_e::sleep:
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(4)); /// TODO. Is 4 the best wait-time?
            break;
        }
        case capture_event_e::none:
        {
            break;
        }
        default:
        {
            k_assert(0, "Unhandled capture event.");
        }
    }

    return e;
}

// Load in any data files that the user requested via the command-line.
static void load_user_data(void)
{
    kd_load_video_presets(kcom_video_presets_file_name());
    kd_load_filter_graph(kcom_filter_graph_file_name());
    kd_load_aliases(kcom_aliases_file_name());

    return;
}

void k_set_eco_mode_enabled(const bool isEnabled)
{
    if (isEnabled == IS_ECO_MODE_ENABLED)
    {
        return;
    }

    IS_ECO_MODE_ENABLED = isEnabled;
    isEnabled? k_evEcoModeEnabled.fire() : k_evEcoModeDisabled.fire();

    return;
}

bool k_is_eco_mode_enabled(void)
{
    return IS_ECO_MODE_ENABLED;
}

// VCS's eco mode. Tries to figure out the average latency between capture events and
// put this main VCS thread to sleep for nearly that long between the capture events,
// rather than having the CPU spin its wheels in a busy loop for that duration.
//
// There's basically two categories in VCS capture events: something happened, or nothing
// happened. In a non-eco busy loop, the main VCS thread will keep polling the capture
// thread for new events, mostly receiving the 'nothing happened' event until a new frame
// comes in. This wastes CPU cycles but ensures low latency in reacting to capture events.
// In eco mode, the idea is that capture events generally occur at predictable intervals,
// so if one is happening e.g. every ~16 milliseconds (new captured frame at 60 Hz) and
// it's taking VCS ~4 ms to process each event, we can reasonably assume that we can sleep
// for at least 8 ms without missing the next one. So in that case we sleep for 8 ms (as
// an example), then return to the busy loop to be ready to immediately receive the next
// frame when it comes in.
static void eco_sleep(const capture_event_e event)
{
    if (!IS_ECO_MODE_ENABLED)
    {
        return;
    }

    const unsigned maxTimeToSleepMs = 10;

    if (!kc_is_receiving_signal())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(maxTimeToSleepMs));
        return;
    }
    else if (event == capture_event_e::none)
    {
        return;
    }
    else
    {
        static double timeToSleepMs = 0;
        static unsigned numDroppedFrames = kc_get_missed_frames_count();
        const double msSinceLastEvent = (0.85 * std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - ECO_REFERENCE_TIME).count());

        const unsigned numNewDroppedFrames = (kc_get_missed_frames_count() - numDroppedFrames);
        numDroppedFrames += numNewDroppedFrames;

        // If we drop frames, we shorten the sleep duration, and otherwise we creep toward
        // the maximum possible sleep time given the current rate of capture events.
        timeToSleepMs = LERP((timeToSleepMs / (numNewDroppedFrames? 1.5 : 1)), msSinceLastEvent, 0.01);

        // We have time to sleep only if we're not dropping frames.
        if (!numNewDroppedFrames)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(std::min(maxTimeToSleepMs, unsigned(timeToSleepMs))));
        }
    }

    ECO_REFERENCE_TIME = std::chrono::system_clock::now();

    return;
}

int main(int argc, char *argv[])
{
    printf("VCS %s\n", PROGRAM_VERSION_STRING);

    #ifndef VCS_RELEASE_BUILD
        printf("NON-RELEASE BUILD\n");
    #endif

    // We want to be sure that the capture hardware is released in a controlled
    // manner, if possible, in the event of a runtime failure. (Not releasing the
    // hardware on program termination may cause a degradation in its subsequent
    // performance until the computer is rebooted.)
    std::set_terminate([]
    {
        NBENE(("VCS has encountered a run-time error and will attempt to exit."));
        PROGRAM_EXIT_REQUESTED = true;
        prepare_for_exit();
    });

    DEBUG(("Parsing the command line."));
    if (!kcom_parse_command_line(argc, argv))
    {
        NBENE(("Malformed command line argument(s). Exiting."));
        prepare_for_exit();
        return EXIT_FAILURE;
    }

    INFO(("Initializing VCS."));
    if (!initialize_all())
    {
        kd_show_headless_error_message(
            "",
           "VCS has to exit because it encountered one or more "
           "unrecoverable errors while initializing itself. "
           "More information will have been printed into "
           "the console. If a console window was not already "
           "open, run VCS again from the command line."
        );

        prepare_for_exit();
        return EXIT_FAILURE;
    }
    else
    {
        load_user_data();
    }

    if (!kc_is_receiving_signal())
    {
        INFO(("No signal."));
    }

    DEBUG(("Entering the main loop."));
    while (!PROGRAM_EXIT_REQUESTED)
    {
        const capture_event_e e = process_next_capture_event();
        kt_update_timers();
        kd_spin_event_loop();
        eco_sleep(e);
    }

    prepare_for_exit();
    return EXIT_SUCCESS;
}
