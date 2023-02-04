/*
 * 2020 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <QColor>
#include <chrono>
#include "common/globals.h"
#include "common/propagate/vcs_event.h"
#include "common/timer/timer.h"
#include "capture/capture.h"

// We'll try to redraw the on-screen test pattern this often.
static const double TARGET_REFRESH_RATE = 60;

// Keep track of the actual achieved refresh rate.
static unsigned NUM_FRAMES_PER_SECOND = 0;
static double CURRENT_REFRESH_RATE = 0;

static bool CAPTURE_EVENT_FLAGS[(int)capture_event_e::num_enumerators];

static bool IS_VALID_SIGNAL = true;

static const resolution_s MAX_RESOLUTION = {MAX_CAPTURE_WIDTH, MAX_CAPTURE_HEIGHT, 32};
static const resolution_s MIN_RESOLUTION = {320, 200, 32};

static const capture_pixel_format_e DEFAULT_PIXEL_FORMAT = capture_pixel_format_e::rgb_888;

static captured_frame_s FRAME_BUFFER;

static unsigned CUR_INPUT_CHANNEL_IDX = 0;

static void refresh_test_pattern(void)
{
    static unsigned numTicks = 0;

    numTicks++;
    NUM_FRAMES_PER_SECOND++;

    for (unsigned y = 0; y < FRAME_BUFFER.r.h; y++)
    {
        for (unsigned x = 0; x < FRAME_BUFFER.r.w; x++)
        {
            /// TODO: Remove the dependency on Qt for the RGB -> HSL -> RGB conversion.
            QColor rgbGradient;
            const double widthRatio = (((x + numTicks) % FRAME_BUFFER.r.w) / double(FRAME_BUFFER.r.w));
            const double heightRatio = (y / double(FRAME_BUFFER.r.h));
            rgbGradient.setHsl((widthRatio * 359), 255, (255 - (heightRatio * 255)));
            rgbGradient = rgbGradient.toRgb();

            const u8 red = rgbGradient.red();
            const u8 green = rgbGradient.green();
            const u8 blue = rgbGradient.blue();
            const u8 alpha = 255;

            switch (FRAME_BUFFER.pixelFormat)
            {
                case capture_pixel_format_e::rgb_888:
                {
                    const unsigned idx = ((x + y * FRAME_BUFFER.r.w) * (FRAME_BUFFER.r.bpp / 8));

                    FRAME_BUFFER.pixels[idx + 0] = blue;
                    FRAME_BUFFER.pixels[idx + 1] = green;
                    FRAME_BUFFER.pixels[idx + 2] = red;
                    FRAME_BUFFER.pixels[idx + 3] = alpha;

                    break;
                }
                case capture_pixel_format_e::rgb_565:
                {
                    const unsigned idx = (x + y * FRAME_BUFFER.r.w);

                    ((uint16_t*)FRAME_BUFFER.pixels)[idx] = (
                        ((red   & 0xf8) << 8) |
                        ((green & 0xfc) << 3) |
                        (blue           >> 3)
                    );

                    break;
                }
                case capture_pixel_format_e::rgb_555:
                {
                    const unsigned idx = (x + y * FRAME_BUFFER.r.w);

                    ((uint16_t*)FRAME_BUFFER.pixels)[idx] = (
                        ((alpha & 0x80) << 8) |
                        ((red   & 0xf8) << 7) |
                        ((green & 0xf8) << 2) |
                        (blue           >> 3)
                    );

                    break;
                }
            }
        }
    }

    return;
}

static void push_capture_event(const capture_event_e event)
{
    CAPTURE_EVENT_FLAGS[(int)event] = true;

    return;
}

static bool pop_capture_event(const capture_event_e event)
{
    const bool oldFlagValue = CAPTURE_EVENT_FLAGS[(int)event];

    CAPTURE_EVENT_FLAGS[(int)event] = false;

    return oldFlagValue;
}

bool kc_initialize_device(void)
{
    DEBUG(("Initializing the virtual capture device."));

    FRAME_BUFFER.r = {640, 480, 32};
    FRAME_BUFFER.pixelFormat = capture_pixel_format_e::rgb_888;
    FRAME_BUFFER.pixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

    // Simulate the capturing of a new frame.
    kt_timer(std::round(1000 / TARGET_REFRESH_RATE), [](const unsigned)
    {
        if ((kc_get_capture_resolution().bpp > MAX_CAPTURE_BPP) ||
            (kc_get_capture_resolution().w > MAX_CAPTURE_WIDTH) ||
            (kc_get_capture_resolution().h > MAX_CAPTURE_HEIGHT))
        {
            IS_VALID_SIGNAL = false;
            push_capture_event(capture_event_e::invalid_signal);
        }
        else
        {
            IS_VALID_SIGNAL = true;
            refresh_test_pattern();
            push_capture_event(capture_event_e::new_frame);
        }
    });

    kt_timer(1000, [](const unsigned elapsedMs)
    {
        const double newRefreshRate = (NUM_FRAMES_PER_SECOND * (1000.0 / elapsedMs));

        NUM_FRAMES_PER_SECOND = 0;

        if (CURRENT_REFRESH_RATE != newRefreshRate)
        {
            CURRENT_REFRESH_RATE = newRefreshRate;
            push_capture_event(capture_event_e::new_video_mode);
        }
    });

    return true;
}

bool kc_release_device(void)
{
    delete [] FRAME_BUFFER.pixels;

    return true;
}

bool kc_set_capture_pixel_format(const capture_pixel_format_e pf)
{
    switch (pf)
    {
        case capture_pixel_format_e::rgb_888:
        {
            FRAME_BUFFER.r.bpp = 32;
            break;
        }
        case capture_pixel_format_e::rgb_565:
        case capture_pixel_format_e::rgb_555:
        {
            FRAME_BUFFER.r.bpp = 16;
            break;
        }
    }

    FRAME_BUFFER.pixelFormat = pf;
    push_capture_event(capture_event_e::new_video_mode);

    return true;
}

bool kc_set_capture_resolution(const resolution_s &r)
{
    if ((r.w > MAX_RESOLUTION.w) ||
        (r.w < MIN_RESOLUTION.w) ||
        (r.h > MAX_RESOLUTION.h) ||
        (r.h < MIN_RESOLUTION.h))
    {
        return false;
    }

    FRAME_BUFFER.r.w = r.w;
    FRAME_BUFFER.r.h = r.h;
    push_capture_event(capture_event_e::new_video_mode);

    return true;
}

refresh_rate_s kc_get_capture_refresh_rate(void)
{
    return refresh_rate_s(CURRENT_REFRESH_RATE);
}

bool kc_set_capture_input_channel(const unsigned idx)
{
    CUR_INPUT_CHANNEL_IDX = idx;

    ks_evInputChannelChanged.fire();

    return true;
}

const captured_frame_s& kc_get_frame_buffer(void)
{
    return FRAME_BUFFER;
}

capture_event_e kc_pop_capture_event_queue(void)
{
    if (pop_capture_event(capture_event_e::invalid_signal))
    {
        return capture_event_e::invalid_signal;
    }
    else if (pop_capture_event(capture_event_e::new_video_mode))
    {
        return capture_event_e::new_video_mode;
    }
    else if (pop_capture_event(capture_event_e::new_frame))
    {
        return capture_event_e::new_frame;
    }

    return capture_event_e::none;
}

bool kc_device_supports_component_capture(void)
{
    return true;
}

bool kc_device_supports_composite_capture(void)
{
    return true;
}

bool kc_device_supports_deinterlacing(void)
{
    return true;
}

bool kc_device_supports_svideo(void)
{
    return true;
}

bool kc_device_supports_dma(void)
{
    return true;
}

bool kc_device_supports_dvi(void)
{
    return true;
}

bool kc_device_supports_vga(void)
{
    return true;
}

bool kc_device_supports_yuv(void)
{
    return true;
}

bool kc_has_digital_signal(void)
{
    return true;
}

bool kc_has_valid_device(void)
{
    return true;
}

capture_pixel_format_e kc_get_capture_pixel_format(void)
{
    return FRAME_BUFFER.pixelFormat;
}

uint kc_get_capture_color_depth(void)
{
    return (unsigned)FRAME_BUFFER.r.bpp;
}

uint kc_get_missed_frames_count(void)
{
    return 0;
}

uint kc_get_device_input_channel_idx(void)
{
    return CUR_INPUT_CHANNEL_IDX;
}

resolution_s kc_get_capture_resolution(void)
{
    return FRAME_BUFFER.r;
}

resolution_s kc_get_device_minimum_resolution(void)
{
    return MIN_RESOLUTION;
}

resolution_s kc_get_device_maximum_resolution(void)
{
    return MAX_RESOLUTION;
}

std::string kc_get_device_name(void)
{
    return "Virtual capture device";
}

std::string kc_get_device_api_name(void)
{
    return "Virtual";
}

std::string kc_get_device_driver_version(void)
{
    return "Unknown";
}

std::string kc_get_device_firmware_version(void)
{
    return "Unknown";
}

int kc_get_device_maximum_input_count(void)
{
    return 2;
}

video_signal_parameters_s kc_get_device_video_parameters(void)
{
    return video_signal_parameters_s{};
}

video_signal_parameters_s kc_get_device_video_parameter_defaults(void)
{
    return video_signal_parameters_s{};
}

video_signal_parameters_s kc_get_device_video_parameter_minimums(void)
{
    return video_signal_parameters_s{};
}

video_signal_parameters_s kc_get_device_video_parameter_maximums(void)
{
    return video_signal_parameters_s{};
}

bool kc_set_deinterlacing_mode(const capture_deinterlacing_mode_e mode)
{
    (void)mode;

    return false;
}

bool kc_mark_frame_buffer_as_processed(void)
{
    return false;
}

bool kc_has_valid_signal(void)
{
    return IS_VALID_SIGNAL;
}

bool kc_is_receiving_signal(void)
{
    return true;
}

bool kc_set_video_signal_parameters(const video_signal_parameters_s &p)
{
    (void)p;

    return false;
}
