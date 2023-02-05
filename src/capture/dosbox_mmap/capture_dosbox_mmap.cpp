/*
 * 2021-2022 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 * Captures DOSBox's graphical output in Linux via a shared memory interface.
 *
 * Note: DOSBox must be modified using the provided patch to enable two-way
 * communication via the memory interface.
 *
 * Note: DOSBox must be set to render using OpenGL.
 *
 */

#include <atomic>
#include <future>
#include <cstring>
#include "common/globals.h"
#include "capture/capture.h"

// For shared memory.
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

static bool CAPTURE_EVENT_FLAGS[(int)capture_event_e::num_enumerators];

static bool IS_VALID_SIGNAL = true;

static double CURRENT_REFRESH_RATE = 0;

static captured_frame_s FRAME_BUFFER;

static const char MMAP_STATUS_BUF_FILENAME[] = "vcs_dosbox_mmap_status";
static const char MMAP_SCREEN_BUF_FILENAME[] = "vcs_dosbox_mmap_screen";
static const unsigned MMAP_STATUS_BUF_SIZE = 256;
static const unsigned MMAP_SCREEN_BUF_SIZE = MAX_NUM_BYTES_IN_CAPTURED_FRAME;
static uint8_t *MMAP_STATUS_BUF = nullptr;
static uint8_t *MMAP_SCREEN_BUF = nullptr;

static std::atomic<bool> RUN_CAPTURE_THREAD = {false};
static std::future<int> CAPTURE_THREAD_FUTURE;

enum class screen_buffer_value_e : unsigned
{
    // Horizontal resolution of the frame whose data is currently in the buffer.
    width,

    // Vertical resolution of the frame whose data is currently in the buffer.
    height,

    // The screen buffer's pixels (BGRA/8888).
    pixels_ptr,
};

enum class status_buffer_value_e : unsigned
{
    // Set to 1 by DOSBox whenever it's finished copying a new frame into the
    // shared screen buffer; and to 0 by VCS whenever it finishes fetching a
    // frame from the shared screen buffer.
    is_new_frame_available,
};

static intptr_t get_screen_buffer_value(const screen_buffer_value_e valueEnum)
{
    k_assert(MMAP_SCREEN_BUF, "Attempting to access the screen buffer before its initialization.");

    switch (valueEnum)
    {
        case screen_buffer_value_e::width:
        {
            return *((uint16_t*)(&MMAP_SCREEN_BUF[0]));
        }
        case screen_buffer_value_e::height:
        {
            return *((uint16_t*)(&MMAP_SCREEN_BUF[2]));
        }
        case screen_buffer_value_e::pixels_ptr:
        {
            return (intptr_t)&MMAP_SCREEN_BUF[4];
        }
        default: k_assert(0, "Unrecognized enum value for querying the screen buffer.");
    }

    // This is expected to never be reached - prior to this, either a proper value
    // has been returned or an assertion thrown.
    return 0;
}

static intptr_t get_status_buffer_value(const status_buffer_value_e valueEnum)
{
    k_assert(MMAP_STATUS_BUF, "Attempting to access the status buffer before its initialization.");

    switch (valueEnum)
    {
        case status_buffer_value_e::is_new_frame_available:
        {
            return bool(MMAP_STATUS_BUF[0]);
        }
        default: k_assert(0, "Unrecognized enum value for querying the status buffer.");
    }

    // This is expected to never be reached - prior to this, either a proper value
    // has been returned or an assertion thrown.
    return 0;
}

static void set_status_buffer_value(const status_buffer_value_e valueEnum, const intptr_t value)
{
    k_assert(MMAP_STATUS_BUF, "Attempting to access the status buffer before its initialization.");

    switch (valueEnum)
    {
        case status_buffer_value_e::is_new_frame_available:
        {
            MMAP_STATUS_BUF[0] = value;
            break;
        }
        default: k_assert(0, "Unrecognized enum value for querying the status buffer.");
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

// Runs in its own thread, polling the memory shared with DOSBox for new frames.
// Returns 1 on successful exit; 0 otherwise.
static int capture_thread(void)
{
    while (RUN_CAPTURE_THREAD)
    {
        if (get_status_buffer_value(status_buffer_value_e::is_new_frame_available))
        {
            std::lock_guard<std::mutex> lock(kc_capture_mutex());

            IS_VALID_SIGNAL = true;
            
            const unsigned frameWidth = get_screen_buffer_value(screen_buffer_value_e::width);
            const unsigned frameHeight = get_screen_buffer_value(screen_buffer_value_e::height);

            if ((frameWidth > MAX_CAPTURE_WIDTH) ||
                (frameHeight > MAX_CAPTURE_HEIGHT) ||
                (frameWidth < MIN_CAPTURE_WIDTH) ||
                (frameHeight < MIN_CAPTURE_HEIGHT))
            {
                IS_VALID_SIGNAL = false;
                push_capture_event(capture_event_e::invalid_signal);
                continue;
            }

            if ((frameWidth != FRAME_BUFFER.r.w) ||
                (frameHeight != FRAME_BUFFER.r.h))
            {
                FRAME_BUFFER.r = {frameWidth, frameHeight, FRAME_BUFFER.r.bpp};
                push_capture_event(capture_event_e::new_video_mode);
            }

            memcpy(
                FRAME_BUFFER.pixels,
                (char*)get_screen_buffer_value(screen_buffer_value_e::pixels_ptr),
                (frameWidth * frameHeight * 4)
            );

            push_capture_event(capture_event_e::new_frame);

            set_status_buffer_value(status_buffer_value_e::is_new_frame_available, false);
        }
    }

    return 1;
}

bool kc_initialize_device(void)
{
    DEBUG(("Initializing the virtual capture device."));
    k_assert(!FRAME_BUFFER.pixels, "Attempting to doubly initialize the capture device.");

    FRAME_BUFFER.r = {640, 480, 32};
    FRAME_BUFFER.pixelFormat = capture_pixel_format_e::rgb_888;
    FRAME_BUFFER.pixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

    // Initialize the shared memory interface.
    {
        int fd = shm_open(MMAP_STATUS_BUF_FILENAME, (O_RDWR | O_CREAT), 0666);
        k_assert((fd >= 0), "Failed to create the shared memory file (status).");
        int ftrerr = ftruncate(fd, MMAP_STATUS_BUF_SIZE);
        k_assert((ftrerr == 0), "Failed to initialize the shared memory file (status).");
        MMAP_STATUS_BUF = (uint8_t*)mmap(nullptr, MMAP_STATUS_BUF_SIZE, 0666, MAP_SHARED, fd, 0);
        k_assert(MMAP_STATUS_BUF, "Failed to MMAP into the shared memory file (status).");

        fd = shm_open(MMAP_SCREEN_BUF_FILENAME, (O_RDWR | O_CREAT), 0666);
        k_assert((fd >= 0), "Failed to create the shared memory file (screen).");
        ftrerr = ftruncate(fd, MMAP_SCREEN_BUF_SIZE);
        k_assert((ftrerr == 0), "Failed to initialize the shared memory file (screen).");
        MMAP_SCREEN_BUF = (uint8_t*)mmap(nullptr, MMAP_SCREEN_BUF_SIZE, 0666, MAP_SHARED, fd, 0);
        k_assert(MMAP_SCREEN_BUF, "Failed to MMAP into the shared memory file (screen).");
    }

    // Start the capture thread.
    {
        RUN_CAPTURE_THREAD = true;
        CAPTURE_THREAD_FUTURE = std::async(std::launch::async, capture_thread);
        if (!CAPTURE_THREAD_FUTURE.valid())
        {
            goto fail;
        }
    }

    return true;

    fail:
    return false;
}

bool kc_release_device(void)
{
    delete [] FRAME_BUFFER.pixels;
    
    RUN_CAPTURE_THREAD = false;
    CAPTURE_THREAD_FUTURE.wait();

    return true;
}

bool kc_set_capture_pixel_format(const capture_pixel_format_e pf)
{
    // Not supported.

    (void)pf;

    return false;
}

bool kc_set_capture_resolution(const resolution_s &r)
{
    // Not supported.

    (void)r;

    return false;
}

refresh_rate_s kc_get_capture_refresh_rate(void)
{
    return refresh_rate_s(CURRENT_REFRESH_RATE);
}

bool kc_set_capture_input_channel(const unsigned idx)
{
    // Not supported.

    (void)idx;

    return false;
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
    return false;
}

bool kc_device_supports_composite_capture(void)
{
    return false;
}

bool kc_device_supports_deinterlacing(void)
{
    return false;
}

bool kc_device_supports_svideo(void)
{
    return false;
}

bool kc_device_supports_dma(void)
{
    return false;
}

bool kc_device_supports_dvi(void)
{
    return false;
}

bool kc_device_supports_vga(void)
{
    return false;
}

bool kc_device_supports_yuv(void)
{
    return false;
}

bool kc_has_valid_device(void)
{
    return (MMAP_STATUS_BUF && MMAP_SCREEN_BUF);
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
    // Not supported.

    return 0;
}

uint kc_get_device_input_channel_idx(void)
{
    return 0;
}

resolution_s kc_get_capture_resolution(void)
{
    return FRAME_BUFFER.r;
}

resolution_s kc_get_device_minimum_resolution(void)
{
    return {MIN_CAPTURE_WIDTH, MIN_CAPTURE_HEIGHT, MAX_CAPTURE_BPP};
}

resolution_s kc_get_device_maximum_resolution(void)
{
    return {MAX_CAPTURE_WIDTH, MAX_CAPTURE_HEIGHT, MAX_CAPTURE_BPP};
}

std::string kc_get_device_name(void)
{
    return "MMAP";
}

std::string kc_get_device_api_name(void)
{
    return "DOSBox MMAP";
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
    return 1;
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
    // Not supported.

    (void)mode;

    return false;
}

bool kc_mark_frame_buffer_as_processed(void)
{
    // Not supported.
    
    return false;
}

bool kc_has_digital_signal(void)
{
    return true;
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
    // Not supported.
    
    (void)p;

    return false;
}
