/*
 * 2018 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 * Handles interactions with the capture hardware.
 *
 */

#include "common/propagate/vcs_event.h"
#include "capture/capture.h"
#include "common/timer/timer.h"
#include "main.h"

vcs_event_c<const captured_frame_s&> kc_evNewCapturedFrame;
vcs_event_c<const video_mode_s&> kc_evNewProposedVideoMode;
vcs_event_c<const video_mode_s&> kc_evNewVideoMode;
vcs_event_c<void> ks_evInputChannelChanged;
vcs_event_c<void> kc_evInvalidDevice;
vcs_event_c<void> kc_evSignalLost;
vcs_event_c<void> kc_evSignalGained;
vcs_event_c<void> kc_evInvalidSignal;
vcs_event_c<void> kc_evUnrecoverableError;
vcs_event_c<unsigned> kc_evMissedFramesCount;

// We'll keep a running count of the number of frames we've missed, in total,
// during capturing.
static unsigned LAST_KNOWN_MISSED_FRAMES_COUNT = 0;

static std::mutex CAPTURE_MUTEX;

static capture_state_s CAPTURE_STATUS;

std::mutex& kc_capture_mutex(void)
{
    return CAPTURE_MUTEX;
}

const capture_state_s& kc_current_capture_state(void)
{
    return CAPTURE_STATUS;
}

void kc_initialize_capture(void)
{
    DEBUG(("Initializing the capture subsystem."));

    kc_initialize_device();

    kt_timer(1000, [](const unsigned)
    {
        const unsigned numMissedCurrent = kc_get_missed_frames_count();
        const unsigned numMissedFrames = (numMissedCurrent- LAST_KNOWN_MISSED_FRAMES_COUNT);

        LAST_KNOWN_MISSED_FRAMES_COUNT = numMissedCurrent;

        kc_evMissedFramesCount.fire(numMissedFrames);
    });

    kc_evSignalLost.listen([]
    {
        CAPTURE_STATUS.input = {0};
        CAPTURE_STATUS.signalFormat = signal_format_e::none;
    });

    kc_evNewVideoMode.listen([](const video_mode_s &videoMode)
    {
        CAPTURE_STATUS.input = videoMode;
        CAPTURE_STATUS.signalFormat = (kc_has_digital_signal()? signal_format_e::digital : signal_format_e::analog);
    });

    ks_evNewOutputResolution.listen([](const resolution_s &resolution)
    {
        CAPTURE_STATUS.output.resolution = resolution;
    });

    ks_evFramesPerSecond.listen([](const unsigned fps)
    {
        CAPTURE_STATUS.output.refreshRate = fps;
    });

    kc_evMissedFramesCount.listen([](const unsigned numMissed)
    {
        CAPTURE_STATUS.areFramesBeingDropped = numMissed;
    });

    ks_evInputChannelChanged.listen([]
    {
        CAPTURE_STATUS.hardwareChannelIdx = kc_get_device_input_channel_idx();
    });

    k_register_subsystem_releaser([]{
        DEBUG(("Releasing the capture subsystem."));
        kc_release_device();
    });

    return;
}

bool kc_force_capture_resolution(const resolution_s &r)
{
    const resolution_s min = kc_get_device_minimum_resolution();
    const resolution_s max = kc_get_device_maximum_resolution();

    if (!kc_is_receiving_signal())
    {
        DEBUG(("Was asked to change the input resolution while the capture card was not receiving a signal. Ignoring the request."));
        return false;
    }

    if (r.w > max.w ||
        r.w < min.w ||
        r.h > max.h ||
        r.h < min.h)
    {
        NBENE(("Was asked to set an unsupported input resolution (%u x %u). Ignoring the request.",
               r.w, r.h));
        return false;
    }

    if (!kc_set_capture_resolution(r))
    {
        NBENE(("Failed to set the new input resolution (%u x %u).", r.w, r.h));
        return false;
    }

    kc_evNewVideoMode.fire({
        kc_get_capture_resolution(),
        kc_get_capture_refresh_rate(),
    });

    return true;
}
