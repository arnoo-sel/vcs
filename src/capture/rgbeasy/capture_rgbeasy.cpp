/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <atomic>
#include <cmath>
#include "common/globals.h"
#include "common/propagate/vcs_event.h"
#include "capture/capture.h"
#include "capture/alias.h"

#if _WIN32
    #include <windows.h>
    #include <rgb.h>
    #include <rgbapi.h>
    #include <rgberror.h>
#else
    #include "capture/rgbeasy/null_rgbeasy.h"
#endif

static HRGB CAPTURE_HANDLE = 0;

static HRGBDLL API_HANDLE = 0;

// Set to 1 if we're currently capturing.
static bool IS_CAPTURE_ACTIVE = false;

// The corresponding flag will be set to true when the RGBEasy callback
// thread notifies us of such an event; and reset back to false when we've
// handled that event.
static bool CAPTURE_EVENT_FLAGS[static_cast<int>(capture_event_e::num_enumerators)] = {false};

static std::atomic<unsigned int> NUM_FRAMES_PROCESSED(0);
static std::atomic<unsigned int> NUM_FRAMES_RECEIVED(0);

// The pixel format in which the capture device sends captured frames.
static capture_pixel_format_e CAPTURE_PIXEL_FORMAT = capture_pixel_format_e::rgb_888;

// Frames sent by the capture hardware will be stored here for processing.
static captured_frame_s FRAME_BUFFER;

// The number of frames the capture hardware has sent which VCS was too busy to
// receive and so which we had to skip.
static std::atomic<unsigned int> NUM_NEW_FRAME_EVENTS_SKIPPED(0);

static bool IS_RECEIVING_A_SIGNAL = true;

static bool IS_SIGNAL_INVALID = false;

static bool IS_SIGNAL_DIGITAL = false;

// The maximum image depth that the capture device can handle.
static const unsigned MAX_BIT_DEPTH = 32;

static resolution_s INPUT_RESOLUTION = {640, 480, MAX_BIT_DEPTH};

static bool apicall_succeeded(long callReturnValue)
{
    if (callReturnValue != RGBERROR_NO_ERROR)
    {
        DEBUG(("A call to the RGBEasy API returned with error code (0x%x).", callReturnValue));
        return false;
    }

    return true;
}

// Marks the given capture event as having occurred.
static void push_capture_event(capture_event_e event)
{
    CAPTURE_EVENT_FLAGS[static_cast<int>(event)] = true;

    return;
}

// Removes the given capture event from the 'queue', and returns its value.
static bool pop_capture_event(capture_event_e event)
{
    const bool eventOccurred = CAPTURE_EVENT_FLAGS[static_cast<int>(event)];
    CAPTURE_EVENT_FLAGS[static_cast<int>(event)] = false;

    return eventOccurred;
}

static resolution_s get_current_input_resolution_from_api(void)
{
    resolution_s r = INPUT_RESOLUTION;

    if (!apicall_succeeded(RGBGetCaptureWidth(CAPTURE_HANDLE, &r.w)) ||
        !apicall_succeeded(RGBGetCaptureHeight(CAPTURE_HANDLE, &r.h)))
    {
        DEBUG(("The capture device didn't report an input resolution. Using a default."));
        return {640, 480, MAX_BIT_DEPTH};
    }

    r.bpp = kc_get_capture_color_depth();

    return r;
}

// Callback functions for the RGBEasy API, through which the API communicates
// with VCS. RGBEasy isn't supported on platforms other than Windows, hence the
// #if - on other platforms, we load in empty placeholder functions (elsewhere
// in the code).
namespace rgbeasy_callbacks_n
{
#if _WIN32
    // Called by the capture hardware when a new frame has been captured. The
    // captured RGBA data is in frameData.
    void RGBCBKAPI frame_captured(HWND, HRGB, LPBITMAPINFOHEADER frameInfo, void *frameData, ULONG_PTR)
    {
        // If the hardware is sending us a new frame while we're still unfinished
        // with processing the previous frame. In that case, we'll need to skip
        // this new frame.
        if (NUM_FRAMES_RECEIVED != NUM_FRAMES_PROCESSED)
        {
            NUM_NEW_FRAME_EVENTS_SKIPPED++;
            return;
        }

        std::lock_guard<std::mutex> lock(kc_capture_mutex());

        // Ignore new callback events if the user has signaled to quit the program.
        if (PROGRAM_EXIT_REQUESTED)
        {
            goto done;
        }

        // Verify that the frame appears validly formed.
        {
            // This could happen e.g. if direct DMA transfer is enabled.
            if (frameData == nullptr ||
                frameInfo == nullptr)
            {
                goto done;
            }

            // The capture device sent in a frame before the local buffer into which
            // VCS wants to copy the frame had been allocated.
            if (FRAME_BUFFER.pixels == nullptr)
            {
                goto done;
            }

            // The capture device sent in a frame that has an unsupported bit depth.
            if (frameInfo->biBitCount > MAX_BIT_DEPTH)
            {
                goto done;
            }

            if ((frameInfo->biWidth < MIN_CAPTURE_WIDTH) ||
                (frameInfo->biWidth > MAX_CAPTURE_WIDTH) ||
                (std::abs(frameInfo->biHeight) < MIN_CAPTURE_HEIGHT) ||
                (std::abs(frameInfo->biHeight) > MAX_CAPTURE_HEIGHT))
            {
                IS_SIGNAL_INVALID = true;
                push_capture_event(capture_event_e::invalid_signal);
                goto done;
            }

            if ((frameInfo->biWidth != FRAME_BUFFER.r.w) ||
                (std::abs(frameInfo->biHeight) != FRAME_BUFFER.r.h))
            {
                push_capture_event(capture_event_e::new_video_mode);
            }
        }

        FRAME_BUFFER.r.w = frameInfo->biWidth;
        FRAME_BUFFER.r.h = abs(frameInfo->biHeight);
        FRAME_BUFFER.r.bpp = frameInfo->biBitCount;
        FRAME_BUFFER.pixelFormat = CAPTURE_PIXEL_FORMAT;

        // Copy the frame's data into our local buffer so we can work on it.
        std::memcpy(
            FRAME_BUFFER.pixels,
            (u8*)frameData,
            FRAME_BUFFER.r.w * FRAME_BUFFER.r.h * (FRAME_BUFFER.r.bpp / 8)
        );

        push_capture_event(capture_event_e::new_frame);

        done:
        NUM_FRAMES_RECEIVED++;
        return;
    }

    // Called by the capture hardware when the input video mode changes.
    void RGBCBKAPI video_mode_changed(HWND, HRGB, PRGBMODECHANGEDINFO modeInfo, ULONG_PTR)
    {
        std::lock_guard<std::mutex> lock(kc_capture_mutex());

        // Ignore new callback events if the user has signaled to quit the program.
        if (PROGRAM_EXIT_REQUESTED)
        {
            goto done;
        }

        IS_SIGNAL_DIGITAL = modeInfo->BDVI;
        IS_SIGNAL_INVALID = false;
        if (!IS_RECEIVING_A_SIGNAL)
        {
            push_capture_event(capture_event_e::signal_gained);
        }
        IS_RECEIVING_A_SIGNAL = true;
        push_capture_event(capture_event_e::new_video_mode);

        done:
        return;
    }

    // Called by the capture hardware when it's given a signal it can't handle.
    void RGBCBKAPI invalid_signal(HWND, HRGB, unsigned long horClock, unsigned long verClock, ULONG_PTR)
    {
        std::lock_guard<std::mutex> lock(kc_capture_mutex());

        // Ignore new callback events if the user has signaled to quit the program.
        if (PROGRAM_EXIT_REQUESTED)
        {
            goto done;
        }

        // Let the card apply its own no signal handler as well, just in case.
        RGBInvalidSignal(CAPTURE_HANDLE, horClock, verClock);

        IS_SIGNAL_INVALID = true;
        push_capture_event(capture_event_e::invalid_signal);

        done:
        return;
    }

    // Called by the capture hardware when the input signal is lost.
    void RGBCBKAPI lost_signal(HWND, HRGB, ULONG_PTR)
    {
        std::lock_guard<std::mutex> lock(kc_capture_mutex());

        // Let the card apply its own 'no signal' handler as well, just in case.
        RGBNoSignal(CAPTURE_HANDLE);

        IS_RECEIVING_A_SIGNAL = false;
        push_capture_event(capture_event_e::signal_lost);

        return;
    }

    void RGBCBKAPI error(HWND, HRGB, unsigned long, ULONG_PTR, unsigned long*)
    {
        std::lock_guard<std::mutex> lock(kc_capture_mutex());

        push_capture_event(capture_event_e::unrecoverable_error);

        return;
    }
#endif
}

static bool release_hardware(void)
{
    if (!apicall_succeeded(RGBCloseInput(CAPTURE_HANDLE)) ||
        !apicall_succeeded(RGBFree(API_HANDLE)))
    {
        return false;
    }

    return true;
}

static PIXELFORMAT pixel_format_to_rgbeasy_pixel_format(capture_pixel_format_e fmt)
{
    switch (fmt)
    {
        case capture_pixel_format_e::rgb_555: return RGB_PIXELFORMAT_555;
        case capture_pixel_format_e::rgb_565: return RGB_PIXELFORMAT_565;
        case capture_pixel_format_e::rgb_888: return RGB_PIXELFORMAT_888;
        default: k_assert(0, "Unknown pixel format."); return RGB_PIXELFORMAT_888;
    }
}

static bool initialize_hardware(void)
{
    if (!apicall_succeeded(RGBLoad(&API_HANDLE)))
    {
        goto fail;
    }

    if (!apicall_succeeded(RGBOpenInput(INPUT_CHANNEL_IDX,      &CAPTURE_HANDLE)) ||
        !apicall_succeeded(RGBSetFrameDropping(CAPTURE_HANDLE,   FRAME_SKIP)) ||
        !apicall_succeeded(RGBSetDMADirect(CAPTURE_HANDLE,       FALSE)) ||
        !apicall_succeeded(RGBSetPixelFormat(CAPTURE_HANDLE,     pixel_format_to_rgbeasy_pixel_format(CAPTURE_PIXEL_FORMAT))) ||
        !apicall_succeeded(RGBUseOutputBuffers(CAPTURE_HANDLE,   FALSE)) ||
        !apicall_succeeded(RGBSetFrameCapturedFn(CAPTURE_HANDLE, rgbeasy_callbacks_n::frame_captured, 0)) ||
        !apicall_succeeded(RGBSetModeChangedFn(CAPTURE_HANDLE,   rgbeasy_callbacks_n::video_mode_changed, 0)) ||
        !apicall_succeeded(RGBSetInvalidSignalFn(CAPTURE_HANDLE, rgbeasy_callbacks_n::invalid_signal, 0)) ||
        !apicall_succeeded(RGBSetErrorFn(CAPTURE_HANDLE,         rgbeasy_callbacks_n::error, 0)) ||
        !apicall_succeeded(RGBSetNoSignalFn(CAPTURE_HANDLE,      rgbeasy_callbacks_n::lost_signal, 0)))
    {
        NBENE(("Failed to initialize the capture hardware."));
        goto fail;
    }

    return true;

fail:
    return false;
}

static bool start_capture(void)
{
    INFO(("Starting capture on input channel %d.", (INPUT_CHANNEL_IDX + 1)));

    if (!apicall_succeeded(RGBStartCapture(CAPTURE_HANDLE)))
    {
        NBENE(("Failed to start capture on input channel %u.", (INPUT_CHANNEL_IDX + 1)));
        goto fail;
    }
    else
    {
        IS_CAPTURE_ACTIVE = true;
    }

    return true;

    fail:
    return false;
}

static bool stop_capture(void)
{
    INFO(("Stopping capture on input channel %d.", (INPUT_CHANNEL_IDX + 1)));

    if (IS_CAPTURE_ACTIVE)
    {
        if (!apicall_succeeded(RGBStopCapture(CAPTURE_HANDLE)))
        {
            NBENE(("Failed to stop capture on input channel %d.", (INPUT_CHANNEL_IDX + 1)));
            goto fail;
        }
    }
    else
    {
        NBENE(("Was asked to stop the capture even though it hadn't been started."));
        goto fail;
    }

    IS_CAPTURE_ACTIVE = false;

    DEBUG(("Restoring default callback handlers."));
    RGBSetFrameCapturedFn(CAPTURE_HANDLE, NULL, 0);
    RGBSetModeChangedFn(CAPTURE_HANDLE, NULL, 0);
    RGBSetInvalidSignalFn(CAPTURE_HANDLE, NULL, 0);
    RGBSetNoSignalFn(CAPTURE_HANDLE, NULL, 0);
    RGBSetErrorFn(CAPTURE_HANDLE, NULL, 0);

    return true;

    fail:
    return false;
}

bool kc_initialize_device(void)
{
    DEBUG(("Initializing the RGBEASY capture device."));
    k_assert(!FRAME_BUFFER.pixels, "Attempting to doubly initialize the capture device.");

    FRAME_BUFFER.pixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

    // Open an input on the capture hardware, and have it start sending in frames.
    if (!initialize_hardware() ||
        !start_capture())
    {
        NBENE(("Failed to initialize the capture device."));

        PROGRAM_EXIT_REQUESTED = 1;
        goto done;
    }

    INPUT_RESOLUTION = get_current_input_resolution_from_api();

    done:
    // We can successfully exit if the initialization didn't trigger a request
    // to terminate the program.
    return !PROGRAM_EXIT_REQUESTED;
}

bool kc_release_device(void)
{
    DEBUG(("Releasing the capture device."));

    delete [] FRAME_BUFFER.pixels;

    if (stop_capture() &&
        release_hardware())
    {
        DEBUG(("The capture device has been released."));

        return true;
    }
    else
    {
        NBENE(("Failed to release the capture device."));

        return false;
    }
}

bool kc_device_supports_component_capture(void)
{
    const int numInputChannels = kc_get_device_maximum_input_count();

    for (int i = 0; i < numInputChannels; i++)
    {
        long isSupported = 0;

        if (!apicall_succeeded(RGBInputIsComponentSupported(i, &isSupported)))
        {
            return false;
        }

        if (isSupported)
        {
            return true;
        }
    }

    return false;
}

bool kc_device_supports_composite_capture(void)
{
    const int numInputChannels = kc_get_device_maximum_input_count();
    long isSupported = 0;

    for (int i = 0; i < numInputChannels; i++)
    {
        if (!apicall_succeeded(RGBInputIsCompositeSupported(i, &isSupported)))
        {
            return false;
        }

        if (isSupported)
        {
            return true;
        }
    }

    return false;
}

bool kc_device_supports_deinterlacing(void)
{
    long isSupported = 0;

    if (!apicall_succeeded(RGBIsDeinterlaceSupported(&isSupported)))
    {
        return false;
    }

    return bool(isSupported);
}

bool kc_device_supports_svideo(void)
{
    const int numInputChannels = kc_get_device_maximum_input_count();

    for (int i = 0; i < numInputChannels; i++)
    {
        long isSupported = 0;

        if (!apicall_succeeded(RGBInputIsSVideoSupported(i, &isSupported)))
        {
            return false;
        }

        if (isSupported)
        {
            return true;
        }
    }

    return false;
}

bool kc_device_supports_dma(void)
{
    long isSupported = 0;

    if (!apicall_succeeded(RGBIsDirectDMASupported(&isSupported)))
    {
        return false;
    }

    return bool(isSupported);
}

bool kc_device_supports_dvi(void)
{
    const int numInputChannels = kc_get_device_maximum_input_count();

    for (int i = 0; i < numInputChannels; i++)
    {
        long isSupported = 0;

        if (!apicall_succeeded(RGBInputIsDVISupported(i, &isSupported)))
        {
            return false;
        }

        if (isSupported)
        {
            return true;
        }
    }

    return false;
}

bool kc_device_supports_vga(void)
{
    const int numInputChannels = kc_get_device_maximum_input_count();

    for (int i = 0; i < numInputChannels; i++)
    {
        long isSupported = 0;

        if (!apicall_succeeded(RGBInputIsVGASupported(i, &isSupported)))
        {
            return false;
        }

        if (isSupported)
        {
            return true;
        }
    }

    return false;
}

bool kc_device_supports_yuv(void)
{
    long isSupported = 0;

    if (!apicall_succeeded(RGBIsYUVSupported(&isSupported)))
    {
        return false;
    }

    return bool(isSupported);
}

std::string kc_get_device_firmware_version(void)
{
    RGBINPUTINFO ii = {0};
    ii.Size = sizeof(ii);

    if (!apicall_succeeded(RGBGetInputInfo(INPUT_CHANNEL_IDX, &ii)))
    {
        return "Unknown";
    }

    return std::to_string(ii.FirmWare);
}

std::string kc_get_device_driver_version(void)
{
    RGBINPUTINFO ii = {0};
    ii.Size = sizeof(ii);

    if (!apicall_succeeded(RGBGetInputInfo(INPUT_CHANNEL_IDX, &ii)))
    {
        return "Unknown";
    }

    return std::string(std::to_string(ii.Driver.Major) + "." +
                       std::to_string(ii.Driver.Minor) + "." +
                       std::to_string(ii.Driver.Micro) + "/" +
                       std::to_string(ii.Driver.Revision));
}

std::string kc_get_device_name(void)
{
    const std::string unknownName = "Unknown capture device";

    CAPTURECARD card = RGB_CAPTURECARD_DGC103;

    if (!apicall_succeeded(RGBGetCaptureCard(&card)))
    {
        return unknownName;
    }

    switch (card)
    {
        case RGB_CAPTURECARD_DGC103: return "Datapath DGC103 Series";
        case RGB_CAPTURECARD_DGC133: return "Datapath DGC133 Series";
        default:                     return unknownName;
    }
}

std::string kc_get_device_api_name(void)
{
    return "RGBEasy";
}

int kc_get_device_maximum_input_count(void)
{
    unsigned long numInputs = 0;

    if (!apicall_succeeded(RGBGetNumberOfInputs(&numInputs)))
    {
        return 0;
    }

    return numInputs;
}

video_signal_parameters_s kc_get_device_video_parameters(void)
{
    video_signal_parameters_s p = {0};

    if (!apicall_succeeded(RGBGetPhase(CAPTURE_HANDLE,         &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevel(CAPTURE_HANDLE,    &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPosition(CAPTURE_HANDLE,   &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPosition(CAPTURE_HANDLE,   &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScale(CAPTURE_HANDLE,      &p.horizontalScale)) ||
        !apicall_succeeded(RGBGetBrightness(CAPTURE_HANDLE,    &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrast(CAPTURE_HANDLE,      &p.overallContrast)) ||
        !apicall_succeeded(RGBGetColourBalance(CAPTURE_HANDLE, &p.redBrightness,
                                                                    &p.greenBrightness,
                                                                    &p.blueBrightness,
                                                                    &p.redContrast,
                                                                    &p.greenContrast,
                                                                    &p.blueContrast)))
    {
        return {0};
    }

    return p;
}

video_signal_parameters_s kc_get_device_video_parameter_defaults(void)
{
    video_signal_parameters_s p = {0};

    if (!apicall_succeeded(RGBGetPhaseDefault(CAPTURE_HANDLE,         &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevelDefault(CAPTURE_HANDLE,    &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPositionDefault(CAPTURE_HANDLE,   &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPositionDefault(CAPTURE_HANDLE,   &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScaleDefault(CAPTURE_HANDLE,      &p.horizontalScale)) ||
        !apicall_succeeded(RGBGetBrightnessDefault(CAPTURE_HANDLE,    &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrastDefault(CAPTURE_HANDLE,      &p.overallContrast)) ||
        !apicall_succeeded(RGBGetColourBalanceDefault(CAPTURE_HANDLE, &p.redBrightness,
                                                                           &p.greenBrightness,
                                                                           &p.blueBrightness,
                                                                           &p.redContrast,
                                                                           &p.greenContrast,
                                                                           &p.blueContrast)))
    {
        return {0};
    }

    return p;
}

video_signal_parameters_s kc_get_device_video_parameter_minimums(void)
{
    video_signal_parameters_s p = {0};

    if (!apicall_succeeded(RGBGetPhaseMinimum(CAPTURE_HANDLE,         &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevelMinimum(CAPTURE_HANDLE,    &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPositionMinimum(CAPTURE_HANDLE,   &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPositionMinimum(CAPTURE_HANDLE,   &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScaleMinimum(CAPTURE_HANDLE,      &p.horizontalScale)) ||
        !apicall_succeeded(RGBGetBrightnessMinimum(CAPTURE_HANDLE,    &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrastMinimum(CAPTURE_HANDLE,      &p.overallContrast)) ||
        !apicall_succeeded(RGBGetColourBalanceMinimum(CAPTURE_HANDLE, &p.redBrightness,
                                                                           &p.greenBrightness,
                                                                           &p.blueBrightness,
                                                                           &p.redContrast,
                                                                           &p.greenContrast,
                                                                           &p.blueContrast)))
    {
        return {0};
    }

    return p;
}

video_signal_parameters_s kc_get_device_video_parameter_maximums(void)
{
    video_signal_parameters_s p = {0};

    if (!apicall_succeeded(RGBGetPhaseMaximum(CAPTURE_HANDLE,         &p.phase)) ||
        !apicall_succeeded(RGBGetBlackLevelMaximum(CAPTURE_HANDLE,    &p.blackLevel)) ||
        !apicall_succeeded(RGBGetHorPositionMaximum(CAPTURE_HANDLE,   &p.horizontalPosition)) ||
        !apicall_succeeded(RGBGetVerPositionMaximum(CAPTURE_HANDLE,   &p.verticalPosition)) ||
        !apicall_succeeded(RGBGetHorScaleMaximum(CAPTURE_HANDLE,      &p.horizontalScale)) ||
        !apicall_succeeded(RGBGetBrightnessMaximum(CAPTURE_HANDLE,    &p.overallBrightness)) ||
        !apicall_succeeded(RGBGetContrastMaximum(CAPTURE_HANDLE,      &p.overallContrast)) ||
        !apicall_succeeded(RGBGetColourBalanceMaximum(CAPTURE_HANDLE, &p.redBrightness,
                                                                           &p.greenBrightness,
                                                                           &p.blueBrightness,
                                                                           &p.redContrast,
                                                                           &p.greenContrast,
                                                                           &p.blueContrast)))
    {
        return {0};
    }

    return p;
}

resolution_s kc_get_capture_resolution(void)
{
    return INPUT_RESOLUTION;
}

resolution_s kc_get_device_minimum_resolution(void)
{
    resolution_s r = {640, 480, 32};

    if (!apicall_succeeded(RGBGetCaptureWidthMinimum(CAPTURE_HANDLE, &r.w)) ||
        !apicall_succeeded(RGBGetCaptureHeightMinimum(CAPTURE_HANDLE, &r.h)))
    {
        return {0};
    }

    // NOTE: It's assumed that 16 bits is the minimum capture color depth.
    r.bpp = 16;

    return r;
}

resolution_s kc_get_device_maximum_resolution(void)
{
    resolution_s r = {640, 480, 32};

    if (!apicall_succeeded(RGBGetCaptureWidthMaximum(CAPTURE_HANDLE, &r.w)) ||
        !apicall_succeeded(RGBGetCaptureHeightMaximum(CAPTURE_HANDLE, &r.h)))
    {
        return {0};
    }

    // NOTE: It's assumed that 32 bits is the maximum capture color depth.
    r.bpp = 32;

    return r;
}

const captured_frame_s& kc_get_frame_buffer(void)
{
    return FRAME_BUFFER;
}

bool kc_mark_frame_buffer_as_processed(void)
{
    NUM_FRAMES_PROCESSED = NUM_FRAMES_RECEIVED.load();

    FRAME_BUFFER.processed = true;

    return true;
}

capture_event_e kc_pop_capture_event_queue(void)
{
    if (pop_capture_event(capture_event_e::unrecoverable_error))
    {
        return capture_event_e::unrecoverable_error;
    }
    else if (pop_capture_event(capture_event_e::new_video_mode))
    {
        INPUT_RESOLUTION = get_current_input_resolution_from_api();

        return capture_event_e::new_video_mode;
    }
    else if (pop_capture_event(capture_event_e::signal_lost))
    {
        return capture_event_e::signal_lost;
    }
    else if (pop_capture_event(capture_event_e::signal_gained))
    {
        return capture_event_e::signal_gained;
    }
    else if (pop_capture_event(capture_event_e::invalid_signal))
    {
        return capture_event_e::invalid_signal;
    }
    else if (pop_capture_event(capture_event_e::new_frame))
    {
        return capture_event_e::new_frame;
    }

    // If there were no events we should notify the caller about.
    return (IS_RECEIVING_A_SIGNAL? capture_event_e::none : capture_event_e::sleep);
}

refresh_rate_s kc_get_capture_refresh_rate(void)
{
    if (!kc_is_receiving_signal())
    {
        DEBUG(("Tried to query the refresh rate while no signal was being received. Ignoring this."));
        return refresh_rate_s(0);
    }

    RGBMODEINFO mi = {0};
    mi.Size = sizeof(mi);

    if (apicall_succeeded(RGBGetModeInfo(CAPTURE_HANDLE, &mi)))
    {
        return refresh_rate_s(mi.RefreshRate / 1000.0);
    }
    else
    {
        return refresh_rate_s(0);
    }
}

bool kc_set_deinterlacing_mode(const capture_deinterlacing_mode_e mode)
{
    DEINTERLACE apiDeinterlacingMode = RGB_DEINTERLACE_BOB;

    (void)apiDeinterlacingMode;

    DEBUG(("Setting deinterlacing mode to %d.", static_cast<int>(mode)));

    switch (mode)
    {
        case capture_deinterlacing_mode_e::bob: apiDeinterlacingMode = RGB_DEINTERLACE_BOB; break;
        case capture_deinterlacing_mode_e::weave: apiDeinterlacingMode = RGB_DEINTERLACE_WEAVE; break;
        case capture_deinterlacing_mode_e::field_0: apiDeinterlacingMode = RGB_DEINTERLACE_FIELD_0; break;
        case capture_deinterlacing_mode_e::field_1: apiDeinterlacingMode = RGB_DEINTERLACE_FIELD_1; break;
        default: k_assert(0, "Unknown deinterlacing mode."); return false;
    }

    if (apicall_succeeded(RGBSetDeinterlace(CAPTURE_HANDLE, apiDeinterlacingMode)))
    {
        return true;
    }
    else
    {
        DEBUG(("The deinterlacing mode couldn't be set."));
        return false;
    }
}

bool kc_set_video_signal_parameters(const video_signal_parameters_s &p)
{
    if (!kc_is_receiving_signal())
    {
        DEBUG(("Was asked to set capture video params while there was no signal. "
               "Ignoring the request."));

        return true;
    }

    /// TODO: Add error checking.
    RGBSetPhase(CAPTURE_HANDLE,         p.phase);
    RGBSetBlackLevel(CAPTURE_HANDLE,    p.blackLevel);
    RGBSetHorPosition(CAPTURE_HANDLE,   p.horizontalPosition);
    RGBSetHorScale(CAPTURE_HANDLE,      p.horizontalScale);
    RGBSetVerPosition(CAPTURE_HANDLE,   p.verticalPosition);
    RGBSetBrightness(CAPTURE_HANDLE,    p.overallBrightness);
    RGBSetContrast(CAPTURE_HANDLE,      p.overallContrast);
    RGBSetColourBalance(CAPTURE_HANDLE, p.redBrightness,
                                        p.greenBrightness,
                                        p.blueBrightness,
                                        p.redContrast,
                                        p.greenContrast,
                                        p.blueContrast);

    return true;
}

bool kc_set_capture_input_channel(const unsigned idx)
{
    const int numInputChannels = kc_get_device_maximum_input_count();

    if (numInputChannels < 0)
    {
        NBENE(("Encountered an unexpected error while setting the input channel. Aborting the action."));

        goto fail;
    }

    if (idx >= (unsigned)numInputChannels)
    {
        INFO(("Was asked to set an input channel that is out of bounds. Ignoring the request."));

        goto fail;
    }

    if (apicall_succeeded(RGBSetInput(CAPTURE_HANDLE, idx)))
    {
        INFO(("Setting capture input channel to %u.", (idx + 1)));

        INPUT_CHANNEL_IDX = idx;
    }
    else
    {
        NBENE(("Failed to set capture input channel to %u.", (idx + 1)));

        goto fail;
    }

    ks_evInputChannelChanged.fire();

    return true;

    fail:
    return false;
}

bool kc_set_capture_pixel_format(const capture_pixel_format_e pf)
{
    if (apicall_succeeded(RGBSetPixelFormat(CAPTURE_HANDLE, pixel_format_to_rgbeasy_pixel_format(pf))))
    {
        CAPTURE_PIXEL_FORMAT = pf;

        return true;
    }

    return false;
}

bool kc_set_capture_resolution(const resolution_s &r)
{
    if (!IS_CAPTURE_ACTIVE)
    {
        INFO(("Was asked to set the capture resolution while capture was inactive. Ignoring this request."));
        return false;
    }

    unsigned long wd = 0, hd = 0;

    const auto currentInputRes = kc_get_capture_resolution();
    if (r.w == currentInputRes.w &&
        r.h == currentInputRes.h)
    {
        DEBUG(("Was asked to force a capture resolution that had already been set. Ignoring the request."));
        goto fail;
    }

    // Test whether the capture hardware can handle the given resolution.
    if (!apicall_succeeded(RGBTestCaptureWidth(CAPTURE_HANDLE, r.w)))
    {
        NBENE(("Failed to force the new input resolution (%u x %u). The capture hardware says the width "
               "is illegal.", r.w, r.h));
        goto fail;
    }

    // Set the new resolution.
    if (!apicall_succeeded(RGBSetCaptureWidth(CAPTURE_HANDLE, (unsigned long)r.w)) ||
        !apicall_succeeded(RGBSetCaptureHeight(CAPTURE_HANDLE, (unsigned long)r.h)) ||
        !apicall_succeeded(RGBSetOutputSize(CAPTURE_HANDLE, (unsigned long)r.w, (unsigned long)r.h)))
    {
        NBENE(("The capture hardware could not properly initialize the new input resolution (%u x %u).",
               r.w, r.h));
        goto fail;
    }

    /// Temp hack to test if the new resolution is valid.
    RGBGetOutputSize(CAPTURE_HANDLE, &wd, &hd);
    if (wd != r.w ||
        hd != r.h)
    {
        NBENE(("The capture hardware failed to set the desired resolution."));
        goto fail;
    }

    INPUT_RESOLUTION = get_current_input_resolution_from_api();

    return true;

    fail:
    return false;
}

uint kc_get_missed_frames_count(void)
{
    return NUM_NEW_FRAME_EVENTS_SKIPPED;
}

uint kc_get_device_input_channel_idx(void)
{
    return INPUT_CHANNEL_IDX;
}

uint kc_get_capture_color_depth(void)
{
    switch (CAPTURE_PIXEL_FORMAT)
    {
        case capture_pixel_format_e::rgb_888: return 32;
        case capture_pixel_format_e::rgb_565: return 16;
        case capture_pixel_format_e::rgb_555: return 16;
        default: k_assert(0, "Unknown capture pixel format."); return 0;
    }
}

bool is_capturing(void)
{
    return IS_CAPTURE_ACTIVE;
}

bool kc_has_digital_signal(void)
{
    return IS_SIGNAL_DIGITAL;
}

bool kc_has_valid_signal(void)
{
    return !IS_SIGNAL_INVALID;
}

bool kc_is_receiving_signal(void)
{
    return IS_RECEIVING_A_SIGNAL;
}

/// TODO: Implement, if it can be done with the RGBEASY API.
bool kc_has_valid_device(void)
{
    return true;
}

capture_pixel_format_e kc_get_capture_pixel_format(void)
{
    return CAPTURE_PIXEL_FORMAT;
}
