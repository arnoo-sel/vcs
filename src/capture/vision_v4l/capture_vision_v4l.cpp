/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 * Implements capturing under Linux via the Datapath's Vision driver and the
 * Vision4Linux 2 API.
 *
 * Initialization happens in ::initialize(). The capture thread runs in
 * capture_function().
 *
 */

#include <unordered_map>
#include <cmath>
#include <atomic>
#include <vector>
#include <iostream>
#include <thread>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <cstring>
#include <chrono>
#include <poll.h>
#include "capture/vision_v4l/input_channel_v4l.h"
#include "capture/video_presets.h"
#include "capture/vision_v4l/ic_v4l_video_parameters.h"
#include "common/propagate/vcs_event.h"

#define INCLUDE_VISION
#include <visionrgb/include/rgb133control.h>
#include <visionrgb/include/rgb133v4l2.h>

// The input channel (/dev/videoX device) we're currently capturing from.
static input_channel_v4l_c *CUR_INPUT_CHANNEL = nullptr;

// The latest frame we've received from the capture device.
static captured_frame_s FRAME_BUFFER;

// The numeric index of the currently-active input channel. This would be 0 for
// /dev/video0, 4 for /dev/video4, etc.
static unsigned CUR_INPUT_CHANNEL_IDX = 0;

// Cumulative count of frames that were sent to us by the capture device but which
// VCS was too busy to process. Note that this count doesn't account for the missed
// frames on the current input channel, only on previous ones. The total number of
// missed frames over the program's execution is thus this value + the current input
// channel's value.
static unsigned NUM_MISSED_FRAMES = 0;

static bool reset_input_channel(void)
{
    return kc_set_capture_input_channel(kc_get_device_input_channel_idx());
}

capture_event_e kc_pop_capture_event_queue(void)
{
    if (!CUR_INPUT_CHANNEL)
    {
        return capture_event_e::unrecoverable_error;
    }
    else if (CUR_INPUT_CHANNEL->pop_capture_event(capture_event_e::unrecoverable_error))
    {
        CUR_INPUT_CHANNEL->captureStatus.invalidDevice = true;

        return capture_event_e::unrecoverable_error;
    }
    else if (CUR_INPUT_CHANNEL->pop_capture_event(capture_event_e::new_video_mode))
    {
        // Re-create the input channel for the new video mode.
        reset_input_channel();

        return capture_event_e::none;
    }
    else if (CUR_INPUT_CHANNEL->pop_capture_event(capture_event_e::signal_lost))
    {
        return capture_event_e::signal_lost;
    }
    else if (CUR_INPUT_CHANNEL->pop_capture_event(capture_event_e::signal_gained))
    {
        return capture_event_e::signal_gained;
    }
    else if (CUR_INPUT_CHANNEL->pop_capture_event(capture_event_e::invalid_signal))
    {
        return capture_event_e::invalid_signal;
    }
    else if (CUR_INPUT_CHANNEL->pop_capture_event(capture_event_e::invalid_device))
    {
        return capture_event_e::invalid_device;
    }
    else if (CUR_INPUT_CHANNEL->pop_capture_event(capture_event_e::new_frame))
    {
        return capture_event_e::new_frame;
    }
    else if (CUR_INPUT_CHANNEL->pop_capture_event(capture_event_e::sleep))
    {
        return capture_event_e::sleep;
    }

    return capture_event_e::none;
}

resolution_s kc_get_capture_resolution(void)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    return CUR_INPUT_CHANNEL->captureStatus.resolution;
}

resolution_s kc_get_device_minimum_resolution(void)
{
    /// TODO: Query actual hardware parameters for this.
    return resolution_s{
        MIN_OUTPUT_WIDTH,
        MIN_OUTPUT_HEIGHT,
        32
    };
}

resolution_s kc_get_device_maximum_resolution(void)
{
    /// TODO: Query actual hardware parameters for this.

    return resolution_s{
        std::min(2048u, MAX_CAPTURE_WIDTH),
        std::min(1536u, MAX_CAPTURE_HEIGHT),
        std::min(32u, MAX_CAPTURE_BPP)
    };
}

refresh_rate_s kc_get_capture_refresh_rate(void)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    return CUR_INPUT_CHANNEL->captureStatus.refreshRate;
}

uint kc_get_missed_frames_count(void)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    return (NUM_MISSED_FRAMES + CUR_INPUT_CHANNEL->captureStatus.numNewFrameEventsSkipped);
}

bool kc_has_valid_signal(void)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    return !CUR_INPUT_CHANNEL->captureStatus.invalidSignal;
}

bool kc_has_digital_signal(void)
{
    k_assert(CUR_INPUT_CHANNEL, "Attempting to query input channel parameters on a null channel.");

    auto videoParams = CUR_INPUT_CHANNEL->captureStatus.videoParameters;

    /// TODO: Have a way to update only the specific control we're interested in,
    /// rather than having to update them all.
    videoParams.update();

    // I'm assuming the following values for the "signal_type" V4L control:
    //
    // 0: No Signal
    // 1: DVI
    // 2: DVI Dual Link
    // 3: SDI
    // 4: Video
    // 5: 3-Wire Sync On Green
    // 6: 4-Wire Composite Sync
    // 7: 5-Wire Separate Syncs
    // 8: YPRPB
    // 9: CVBS
    // 10: YC
    // 11: Unknown
    const bool isDigital = (
        kc_is_receiving_signal() &&
        (videoParams.value(ic_v4l_device_controls_c::control_type_e::signal_type) <= 3)
    );

    return isDigital;
}

bool kc_has_valid_device(void)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    return !CUR_INPUT_CHANNEL->captureStatus.invalidDevice;
}

bool kc_is_receiving_signal(void)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    return !CUR_INPUT_CHANNEL->captureStatus.noSignal;
}

capture_pixel_format_e kc_get_capture_pixel_format(void)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    return CUR_INPUT_CHANNEL->captureStatus.pixelFormat;
}

bool kc_initialize_device(void)
{
    DEBUG(("Initializing the Vision/V4L capture device."));

    kc_evSignalGained.listen([]
    {
        CUR_INPUT_CHANNEL->captureStatus.videoParameters.update();
        kc_evNewProposedVideoMode.fire({
            kc_get_capture_resolution(),
            kc_get_capture_refresh_rate(),
        });
    });

    FRAME_BUFFER.r = {640, 480, 32};
    FRAME_BUFFER.pixelFormat = capture_pixel_format_e::rgb_888;
    FRAME_BUFFER.pixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

    kc_set_capture_input_channel(INPUT_CHANNEL_IDX);

    return true;
}

bool kc_release_device(void)
{
    delete CUR_INPUT_CHANNEL;
    delete [] FRAME_BUFFER.pixels;

    return true;
}

std::string kc_get_device_name(void)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    v4l2_capability caps = {};

    if (!CUR_INPUT_CHANNEL->device_ioctl(VIDIOC_QUERYCAP, &caps))
    {
        NBENE(("Failed to query capture device capabilities."));

        return "Unknown";
    }

    return (char*)caps.card;
}

std::string kc_get_device_driver_version(void)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    v4l2_capability caps = {};

    if (!CUR_INPUT_CHANNEL->device_ioctl(VIDIOC_QUERYCAP, &caps))
    {
        NBENE(("Failed to query capture device capabilities."));

        return "Unknown";
    }

    return (std::string((char*)caps.driver)             + " " +
            std::to_string((caps.version >> 16) & 0xff) + "." +
            std::to_string((caps.version >> 8) & 0xff)  + "." +
            std::to_string((caps.version >> 0) & 0xff));
}

std::string kc_get_device_firmware_version(void)
{
    return "Unknown";
}


int kc_get_device_maximum_input_count(void)
{
    const char baseDevicePath[] = "/dev/video";
    int numInputs = 0;

    // Returns true if the given V4L capabilities appear to indicate a Vision
    // capture device.
    const auto is_vision_capture_device = [](const v4l2_capability *const caps)
    {
        const bool usesVisionDriver = (strcmp((const char*)caps->driver, "Vision") == 0);
        const bool isVisionControlDevice = (strcmp((const char*)caps->card, "Vision Control") == 0);

        return (usesVisionDriver && !isVisionControlDevice);
    };

    /// TODO: Make sure we only count inputs on the capture card that we're
    /// currently capturing on, rather than tallying up the number of inputs on
    /// all the Vision capture cards on the system.
    for (int i = 0; i < 64; i++)
    {
        v4l2_capability deviceCaps;

        const int deviceFile = open((baseDevicePath + std::to_string(i)).c_str(), O_RDONLY);

        if (deviceFile < 0)
        {
            continue;
        }

        if ((ioctl(deviceFile, VIDIOC_QUERYCAP, &deviceCaps) >= 0) &&
            is_vision_capture_device(&deviceCaps))
        {
            numInputs++;
        }

        close(deviceFile);
    }

    return numInputs;
}

uint kc_get_device_input_channel_idx(void)
{
    return CUR_INPUT_CHANNEL_IDX;
}

video_signal_parameters_s kc_get_device_video_parameters(void)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    if (!kc_is_receiving_signal())
    {
        return kc_get_device_video_parameter_defaults();
    }

    auto videoParams = CUR_INPUT_CHANNEL->captureStatus.videoParameters;

    videoParams.update();

    video_signal_parameters_s p;

    p.phase              = videoParams.value(ic_v4l_device_controls_c::control_type_e::phase);
    p.blackLevel         = videoParams.value(ic_v4l_device_controls_c::control_type_e::black_level);
    p.horizontalPosition = videoParams.value(ic_v4l_device_controls_c::control_type_e::horizontal_position);
    p.verticalPosition   = videoParams.value(ic_v4l_device_controls_c::control_type_e::vertical_position);
    p.horizontalScale    = videoParams.value(ic_v4l_device_controls_c::control_type_e::horizontal_size);
    p.overallBrightness  = videoParams.value(ic_v4l_device_controls_c::control_type_e::brightness);
    p.overallContrast    = videoParams.value(ic_v4l_device_controls_c::control_type_e::contrast);
    p.redBrightness      = videoParams.value(ic_v4l_device_controls_c::control_type_e::red_brightness);
    p.greenBrightness    = videoParams.value(ic_v4l_device_controls_c::control_type_e::green_brightness);
    p.blueBrightness     = videoParams.value(ic_v4l_device_controls_c::control_type_e::blue_brightness);
    p.redContrast        = videoParams.value(ic_v4l_device_controls_c::control_type_e::red_contrast);
    p.greenContrast      = videoParams.value(ic_v4l_device_controls_c::control_type_e::green_contrast);
    p.blueContrast       = videoParams.value(ic_v4l_device_controls_c::control_type_e::blue_contrast);

    return p;
}

video_signal_parameters_s kc_get_device_video_parameter_defaults(void)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    const auto videoParams = CUR_INPUT_CHANNEL->captureStatus.videoParameters;

    video_signal_parameters_s p;

    // The V4L API returns no parameter ranges while there's no signal - so let's
    // approximate them.
    if (!kc_is_receiving_signal())
    {
        p.phase              = 0;
        p.blackLevel         = 8;
        p.horizontalPosition = 112;
        p.verticalPosition   = 36;
        p.horizontalScale    = 900;
        p.overallBrightness  = 32;
        p.overallContrast    = 128;
        p.redBrightness      = 128;
        p.greenBrightness    = 256;
        p.blueBrightness     = 128;
        p.redContrast        = 256;
        p.greenContrast      = 128;
        p.blueContrast       = 256;
    }
    else
    {
        p.phase              = videoParams.default_value(ic_v4l_device_controls_c::control_type_e::phase);
        p.blackLevel         = videoParams.default_value(ic_v4l_device_controls_c::control_type_e::black_level);
        p.horizontalPosition = videoParams.default_value(ic_v4l_device_controls_c::control_type_e::horizontal_position);
        p.verticalPosition   = videoParams.default_value(ic_v4l_device_controls_c::control_type_e::vertical_position);
        p.horizontalScale    = videoParams.default_value(ic_v4l_device_controls_c::control_type_e::horizontal_size);
        p.overallBrightness  = videoParams.default_value(ic_v4l_device_controls_c::control_type_e::brightness);
        p.overallContrast    = videoParams.default_value(ic_v4l_device_controls_c::control_type_e::contrast);
        p.redBrightness      = videoParams.default_value(ic_v4l_device_controls_c::control_type_e::red_brightness);
        p.greenBrightness    = videoParams.default_value(ic_v4l_device_controls_c::control_type_e::green_brightness);
        p.blueBrightness     = videoParams.default_value(ic_v4l_device_controls_c::control_type_e::blue_brightness);
        p.redContrast        = videoParams.default_value(ic_v4l_device_controls_c::control_type_e::red_contrast);
        p.greenContrast      = videoParams.default_value(ic_v4l_device_controls_c::control_type_e::green_contrast);
        p.blueContrast       = videoParams.default_value(ic_v4l_device_controls_c::control_type_e::blue_contrast);
    }

    return p;
}

video_signal_parameters_s kc_get_device_video_parameter_minimums(void)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    const auto videoParams = CUR_INPUT_CHANNEL->captureStatus.videoParameters;

    video_signal_parameters_s p;

    // The V4L API returns no parameter ranges while there's no signal - so let's
    // approximate them.
    //
    // Note: Querying the minimums from V4L seems to work a little unreliably (and
    // they vary from one video mode to another), so for now let's just always use
    // hard-coded values.
    if (1 || !kc_is_receiving_signal())
    {
        // Note: These values have been approximated with the VisionRGB-E1S in mind.
        // They may or may not be suitable for other models.
        p.phase              = 0;
        p.blackLevel         = 1;
        p.horizontalPosition = 1;
        p.verticalPosition   = 1;
        p.horizontalScale    = 100;
        p.overallBrightness  = 0;
        p.overallContrast    = 0;
        p.redBrightness      = 0;
        p.greenBrightness    = 0;
        p.blueBrightness     = 0;
        p.redContrast        = 0;
        p.greenContrast      = 0;
        p.blueContrast       = 0;
    }
    else
    {
        p.phase              = videoParams.minimum_value(ic_v4l_device_controls_c::control_type_e::phase);
        p.blackLevel         = videoParams.minimum_value(ic_v4l_device_controls_c::control_type_e::black_level);
        p.horizontalPosition = videoParams.minimum_value(ic_v4l_device_controls_c::control_type_e::horizontal_position);
        p.verticalPosition   = videoParams.minimum_value(ic_v4l_device_controls_c::control_type_e::vertical_position);
        p.horizontalScale    = videoParams.minimum_value(ic_v4l_device_controls_c::control_type_e::horizontal_size);
        p.overallBrightness  = videoParams.minimum_value(ic_v4l_device_controls_c::control_type_e::brightness);
        p.overallContrast    = videoParams.minimum_value(ic_v4l_device_controls_c::control_type_e::contrast);
        p.redBrightness      = videoParams.minimum_value(ic_v4l_device_controls_c::control_type_e::red_brightness);
        p.greenBrightness    = videoParams.minimum_value(ic_v4l_device_controls_c::control_type_e::green_brightness);
        p.blueBrightness     = videoParams.minimum_value(ic_v4l_device_controls_c::control_type_e::blue_brightness);
        p.redContrast        = videoParams.minimum_value(ic_v4l_device_controls_c::control_type_e::red_contrast);
        p.greenContrast      = videoParams.minimum_value(ic_v4l_device_controls_c::control_type_e::green_contrast);
        p.blueContrast       = videoParams.minimum_value(ic_v4l_device_controls_c::control_type_e::blue_contrast);
    }

    return p;
}

video_signal_parameters_s kc_get_device_video_parameter_maximums(void)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to query input channel parameters on a null channel.");

    const auto videoParams = CUR_INPUT_CHANNEL->captureStatus.videoParameters;

    video_signal_parameters_s p;

    // The V4L API returns no parameter ranges while there's no signal - so let's
    // approximate them.
    //
    // Note: Querying the maximums from V4L seems to work a little unreliably (and
    // they vary from one video mode to another), so for now let's just always use
    // hard-coded values.
    if (1 || !kc_is_receiving_signal())
    {
        // Note: These values have been approximated with the VisionRGB-E1S in mind.
        // They may or may not be suitable for other models.
        p.phase              = 31;
        p.blackLevel         = 255;
        p.horizontalPosition = 1200;
        p.verticalPosition   = 63;
        p.horizontalScale    = 4095;
        p.overallBrightness  = 64;
        p.overallContrast    = 255;
        p.redBrightness      = 255;
        p.greenBrightness    = 511;
        p.blueBrightness     = 255;
        p.redContrast        = 511;
        p.greenContrast      = 255;
        p.blueContrast       = 511;
    }
    else
    {
        p.phase              = videoParams.maximum_value(ic_v4l_device_controls_c::control_type_e::phase);
        p.blackLevel         = videoParams.maximum_value(ic_v4l_device_controls_c::control_type_e::black_level);
        p.horizontalPosition = videoParams.maximum_value(ic_v4l_device_controls_c::control_type_e::horizontal_position);
        p.verticalPosition   = videoParams.maximum_value(ic_v4l_device_controls_c::control_type_e::vertical_position);
        p.horizontalScale    = videoParams.maximum_value(ic_v4l_device_controls_c::control_type_e::horizontal_size);
        p.overallBrightness  = videoParams.maximum_value(ic_v4l_device_controls_c::control_type_e::brightness);
        p.overallContrast    = videoParams.maximum_value(ic_v4l_device_controls_c::control_type_e::contrast);
        p.redBrightness      = videoParams.maximum_value(ic_v4l_device_controls_c::control_type_e::red_brightness);
        p.greenBrightness    = videoParams.maximum_value(ic_v4l_device_controls_c::control_type_e::green_brightness);
        p.blueBrightness     = videoParams.maximum_value(ic_v4l_device_controls_c::control_type_e::blue_brightness);
        p.redContrast        = videoParams.maximum_value(ic_v4l_device_controls_c::control_type_e::red_contrast);
        p.greenContrast      = videoParams.maximum_value(ic_v4l_device_controls_c::control_type_e::green_contrast);
        p.blueContrast       = videoParams.maximum_value(ic_v4l_device_controls_c::control_type_e::blue_contrast);
    }

    return p;
}

const captured_frame_s& kc_get_frame_buffer(void)
{
    return FRAME_BUFFER;
}

bool kc_mark_frame_buffer_as_processed(void)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to set input channel parameters on a null channel.");

    CUR_INPUT_CHANNEL->captureStatus.numFramesProcessed++;

    FRAME_BUFFER.processed = true;

    return true;
}

std::string kc_get_device_api_name(void)
{
    return "Vision/Video4Linux";
}

bool kc_set_capture_resolution(const resolution_s &r)
{
    k_assert(CUR_INPUT_CHANNEL, "Attempting to set the capture resolution on a null input channel.");

    // Validate the resolution.
    {
        const auto minRes = kc_get_device_minimum_resolution();
        const auto maxRes = kc_get_device_maximum_resolution();

        if (
            (r.w < minRes.w) ||
            (r.w > maxRes.w) ||
            (r.h < minRes.h) ||
            (r.h > maxRes.h)
        ){
            NBENE(("Failed to set the capture resolution: resolution out of bounds"));
            goto fail;
        }
    }

    // Set the resolution.
    {
        struct v4l2_control control = {0};

        control.id = RGB133_V4L2_CID_HOR_TIME;
        control.value = r.w;
        if (!CUR_INPUT_CHANNEL->device_ioctl(VIDIOC_S_CTRL, &control))
        {
            NBENE(("Failed to set the capture resolution: device error when setting width."));
            return false;
        }

        control.id = RGB133_V4L2_CID_VER_TIME;
        control.value = r.h;
        if (!CUR_INPUT_CHANNEL->device_ioctl(VIDIOC_S_CTRL, &control))
        {
            NBENE(("Failed to set the capture resolution: device error when setting height."));
            goto fail;
        }
    }

    return true;

    fail:
    return false;
}

/// TODO: Implement.
bool kc_set_capture_pixel_format(const capture_pixel_format_e pf)
{
    (void)pf;

    return false;
}

/// TODO: Implement.
bool kc_set_deinterlacing_mode(const capture_deinterlacing_mode_e mode)
{
    (void)mode;

    return false;
}

/// TODO: Implement.
uint kc_get_capture_color_depth(void)
{
    return 32;
}

bool kc_set_capture_input_channel(const unsigned idx)
{
    if (CUR_INPUT_CHANNEL)
    {
        NUM_MISSED_FRAMES += CUR_INPUT_CHANNEL->captureStatus.numNewFrameEventsSkipped;

        delete CUR_INPUT_CHANNEL;
    }

    CUR_INPUT_CHANNEL = new input_channel_v4l_c(
        (std::string("/dev/video") + std::to_string(idx)),
        3,
        &FRAME_BUFFER
    );

    CUR_INPUT_CHANNEL_IDX = idx;

    ks_evInputChannelChanged.fire();

    return true;
}

bool kc_set_video_signal_parameters(const video_signal_parameters_s &p)
{
    k_assert(CUR_INPUT_CHANNEL,
             "Attempting to set input channel parameters on a null channel.");

    if (!kc_is_receiving_signal())
    {
        DEBUG(("Was asked to set capture video params while there was no signal. "
               "Ignoring the request."));

        return true;
    }

    auto videoParams = CUR_INPUT_CHANNEL->captureStatus.videoParameters;

    const auto kc_set_parameter = [&videoParams](const int value, const ic_v4l_device_controls_c::control_type_e parameterType)
    {
        return videoParams.set_value(value, parameterType);
    };

    kc_set_parameter(p.phase,              ic_v4l_device_controls_c::control_type_e::phase);
    kc_set_parameter(p.blackLevel,         ic_v4l_device_controls_c::control_type_e::black_level);
    kc_set_parameter(p.horizontalPosition, ic_v4l_device_controls_c::control_type_e::horizontal_position);
    kc_set_parameter(p.verticalPosition,   ic_v4l_device_controls_c::control_type_e::vertical_position);
    kc_set_parameter(p.horizontalScale,    ic_v4l_device_controls_c::control_type_e::horizontal_size);
    kc_set_parameter(p.overallBrightness,  ic_v4l_device_controls_c::control_type_e::brightness);
    kc_set_parameter(p.overallContrast,    ic_v4l_device_controls_c::control_type_e::contrast);
    kc_set_parameter(p.redBrightness,      ic_v4l_device_controls_c::control_type_e::red_brightness);
    kc_set_parameter(p.greenBrightness,    ic_v4l_device_controls_c::control_type_e::green_brightness);
    kc_set_parameter(p.blueBrightness,     ic_v4l_device_controls_c::control_type_e::blue_brightness);
    kc_set_parameter(p.redContrast,        ic_v4l_device_controls_c::control_type_e::red_contrast);
    kc_set_parameter(p.greenContrast,      ic_v4l_device_controls_c::control_type_e::green_contrast);
    kc_set_parameter(p.blueContrast,       ic_v4l_device_controls_c::control_type_e::blue_contrast);

    return true;
}

/// TODO: Can this capability be queried with Video4Linux?
bool kc_device_supports_component_capture(void)
{
    return false;
}

/// TODO: Can this capability be queried with Video4Linux?
bool kc_device_supports_composite_capture(void)
{
    return false;
}

/// TODO: Can this capability be queried with Video4Linux?
bool kc_device_supports_deinterlacing(void)
{
    return false;
}

/// TODO: Can this capability be queried with Video4Linux?
bool kc_device_supports_svideo(void)
{
    return false;
}

/// TODO: Can this capability be queried with Video4Linux?
bool kc_device_supports_dma(void)
{
    return false;
}

/// TODO: Can this capability be queried with Video4Linux?
bool kc_device_supports_dvi(void)
{
    return false;
}

/// TODO: Can this capability be queried with Video4Linux?
bool kc_device_supports_vga(void)
{
    return false;
}

/// TODO: Can this capability be queried with Video4Linux?
bool kc_device_supports_yuv(void)
{
    return false;
}
