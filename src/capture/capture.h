/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

/*! @file
 *
 * @brief
 * The interface to VCS's capture subsystem.
 *
 * The capture subsystem is responsible for mediating exchange between VCS and
 * the capture device; including initializing the device and providing VCS with
 * access to the frame buffer(s) associated with the device.
 * 
 * The capture subsystem is an exceptional subsystem in that it's allowed to run
 * outside of the main VCS thread. This freedom -- in what is otherwise a
 * single-threaded application -- is granted because some capture devices require
 * out-of-thread callbacks or the like.
 * 
 * The multithreaded capture subsystem will typically have an idle monitoring
 * thread that waits for the capture device to send in data. When data comes in,
 * the thread will copy it to a local memory buffer to be operated on in the
 * main VCS thread when it's ready to do so.
 * 
 * Because of the out-of-thread nature of this subsystem, it's important that
 * accesses to its memory buffers are synchronized using the capture mutex (see
 * kc_capture_mutex()).
 *
 * Usage:
 *
 *   1. Call kc_initialize_capture() to initialize the subsystem. This is VCS's
 *      default startup behavior.
 *
 *   2. Use the interface functions to interact with the subsyste. For instance,
 *      kc_get_frame_buffer() returns the most recent captured frame's data.
 *
 *   3. Call kc_release_capture() to release the subsystem. This is VCS's default
 *      exit behavior.
 * 
 * ## Implementing support for new capture devices
 * 
 * The capture subsystem interface (this file, capture.h) provides a declaration
 * of the functions used by VCS to interact with the capture subsystem. Some of
 * the functions -- e.g. kc_initialize_capture() -- are agnostic to the capture
 * device being used, while others -- e.g. kc_initialize_device() -- are specific
 * to a particular type of capture device.
 * 
 * The universal functions like kc_initialize_capture() are defined in the
 * base interface source file (@a capture.cpp), whereas the device-specific
 * functions like kc_initialize_device() are implemented in a separate,
 * device-specific source file. For instance, the device source file
 * @a capture_rgbeasy.cpp implements support for RGBEasy capture devices under
 * Windows, while @a capture_vision_v4l.cpp adds support for Vision Linux devices.
 * Both files simply implement the device-specific functions of the interface for
 * the corresponding device, and depending on which device is to be supported by
 * VCS, only one of the files gets included in the compiled executable (see @a vcs.pro
 * for the logic that decides which implementation is included in the build).
 * 
 * You can add support for a new capture device by creating implementations for
 * the device of all of the device-specific interface functions, namely those
 * declared in capture.h prefixed with @a kc_ but which aren't already defined in
 * @a capture.cpp. You can look at the existing device source files for hands-on
 * examples.
 * 
 * As you'll see from @a capture_virtual.cpp, the capture device doesn't need
 * to be an actual device. It can be any source of image data -- something
 * that reads images from @a stdin, for example. It only needs to implement the
 * interface functions and to output its data in the form dictated by the interface.
 */

#ifndef VCS_CAPTURE_CAPTURE_H
#define VCS_CAPTURE_CAPTURE_H

#include <vector>
#include <mutex>
#include "display/display.h"
#include "common/globals.h"
#include "scaler/scaler.h"
#include "common/refresh_rate.h"
#include "common/memory/memory.h"
#include "common/types.h"
#include "common/propagate/app_events.h"

// VCS has received a new frame from the capture device. (The frame's data is
// available from kc_get_frame_buffer().)
extern app_event_c<void> kcEvent_frameCaptured;

// The capture device has received a new video mode. We treat it as a proposal,
// since we might e.g. not want this video mode to be used, and in that case
// would tell the capture device to use some other mode.
extern app_event_c<void> kcEvent_newProposedVideoMode;

// The capture device has received a new video mode that we've approved of
// (cf. newProposedVideoMode).
extern app_event_c<void> kcEvent_newVideoMode;

// The active input channel index has changed.
extern app_event_c<void> kcEvent_newInputChannel;

// The current capture device is invalid.
extern app_event_c<void> kcEvent_invalidDevice;

extern app_event_c<void> kcEvent_signalLost;
extern app_event_c<void> kcEvent_signalGained;
extern app_event_c<void> kcEvent_invalidSignal;
extern app_event_c<void> kcEvent_unrecoverableError;

// The capture subsystem has had to ignore frames coming from the capture
// device because VCS was busy with something else (e.g. with processing
// a previous frame). Provides the count of missed frames (generally, the
// capture subsystem might fire this event at regular intervals and pass
// the count of missed frames during that interval).
extern app_event_c<unsigned> kcEvent_missedFramesCount;

/*!
 * Enumerates the de-interlacing modes recognized by the capture subsystem.
 *
 * @note
 * The capture subsystem itself doesn't apply de-interlacing, it just asks the
 * capture device to do so. The capture device in turn may support only some or
 * none of these modes, and/or might apply them only when receiving an
 * interlaced signal.
 */
enum class capture_deinterlacing_mode_e
{
    weave,
    bob,
    field_0,
    field_1,
};

/*!
 * Enumerates the pixel color formats recognized by the capture subsystem for
 * captured frames.
 *
 * @see
 * captured_frame_s
 */
enum class capture_pixel_format_e
{
    //! 16 bits per pixel: 5 bits for red, 5 bits for green, 5 bits for blue, and
    //! 1 bit of padding. No alpha.
    rgb_555,

    //! 16 bits per pixel: 5 bits for red, 6 bits for green, and 5 bits for blue.
    //! No alpha.
    rgb_565,

    //! 32 bits per pixel: 8 bits for red, 8 bits for green, 8 bits for blue, and
    //! 8 bits of padding. No alpha.
    rgb_888,
};

/*!
 * VCS will periodically query the capture subsystem for the latest capture events.
 * This enumerates the range of capture events that the capture subsystem can report
 * back.
 */
enum class capture_event_e
{
    //! No capture events to report. Although some capture events may have occurred,
    //! the capture subsystem chooses to not inform VCS of them.
    none,

    //! Same as 'none', but the capture subsystem also thinks VCS shouldn't send
    //! a new query for capture events for some short while (milliseconds); e.g.
    //! because the capture device is currently not receiving a signal and so isn't
    //! expected to produce events in the immediate future.
    sleep,

    //! The capture device has just lost its input signal.
    signal_lost,

    //! The capture device has sent a new frame, whose data is now available tp
    //! VCS via the capture device interface.
    new_frame,

    //! The capture device's input signal has changed in resolution or refresh rate.
    //! The new mode parameters can be queried via the capture device interface.
    new_video_mode,

    //! The capture device's current input signal is invalid (e.g. out of range).
    invalid_signal,

    //! The capture device isn't available for use.
    invalid_device,

    //! An error has occurred within the capture subsystem from which the subsystem
    //! can't recover.
    unrecoverable_error,

    //! Total enumerator count. Should remain the last item in the list.
    num_enumerators
};

/*!
 * @brief
 * Stores the data of an image received from a capture device.
 */
struct captured_frame_s
{
    resolution_s r;

    capture_pixel_format_e pixelFormat;

    heap_bytes_s<u8> pixels;

    // Will be set to true after the frame's data has been processed for
    // display and is no longer needed.
    bool processed = false;
};

struct signal_info_s
{
    resolution_s r;
    int refreshRate;
    bool isInterlaced;
    bool isDigital;
};

struct video_signal_parameters_s
{
    resolution_s r; // For legacy (VCS <= 1.6.5) support.

    long overallBrightness;
    long overallContrast;

    long redBrightness;
    long redContrast;

    long greenBrightness;
    long greenContrast;

    long blueBrightness;
    long blueContrast;

    unsigned long horizontalScale;
    long horizontalPosition;
    long verticalPosition;
    long phase;
    long blackLevel;
};

/*!
 * Returns a reference to a mutex which should be locked by the capture subsystem
 * while it's accessing data shared with the rest of VCS (e.g. capture event flags
 * or the capture frame buffer), and which the rest of VCS should lock while
 * accessing that data.
 *
 * Failure to observe this mutex when accessing capture subsystem data may result
 * in a race condition, as the capture subsystem is allowed to run outside of the
 * main VCS thread.
 *
 * @code
 * // Code running in the main VCS thread.
 * 
 * // Blocks until the capture mutex allows us to access the capture data.
 * std::lock_guard<std::mutex> lock(kc_capture_mutex());
 *
 * // Handle the most recent capture event (having locked the mutex prevents
 * // the capture subsystem from pushing new events while we're doing this).
 * switch (kc_pop_capture_event_queue())
 * {
 *     // ...
 * }
 * @endcode
 *
 * @note
 * If the capture subsystem finds the capture mutex locked when the capture device
 * sends in a new frame, the frame will be discarded rather than waiting for the
 * lock to be released.
 */
std::mutex& kc_capture_mutex(void);

/*!
 * Initializes the capture subsystem.
 *
 * By default, VCS will call this function on program startup.
 *
 * @note
 * Will trigger an assertion failure if the initialization fails.
 *
 * @see
 * kc_release_capture()
 */
void kc_initialize_capture(void);

/*!
 * Initializes the capture device.
 *
 * Returns true on success; false otherwise.
 *
 * @warning
 * Don't call this function directly. Instead, call kc_initialize_capture(),
 * which will initialize both the capture device and the capture subsystem
 * (they are interlinked).
 *
 * @see
 * kc_initialize_capture(), kc_release_device()
 */
bool kc_initialize_device(void);

/*!
 * Releases the capture subsystem.
 *
 * By default, VCS will call this function on program exit.
 *
 * @see
 * kc_initialize_capture()
 */
void kc_release_capture(void);

/*!
 * Releases the capture device.
 *
 * By default, this function will be called by kc_release_capture(), which
 * you should call if you want to release the capture device.
 *
 * Returns true on success; false otherwise.
 *
 * @warning
 * Don't call this function directly. Instead, call kc_release_capture(), which
 * will release both the capture device and the capture subsystem (they are
 * interlinked).
 *
 * @see
 * kc_initialize_capture(), kc_initialize_device()
 */
bool kc_release_device(void);

/*!
 * Asks the capture device to set its input resolution to the one given,
 * overriding the current input resolution.
 *
 * @note
 * If the resolution of the captured signal doesn't match this resolution, the
 * captured image may become corrupted until the proper resolution is set.
 */
bool kc_force_capture_resolution(const resolution_s &r);

/*!
 * Returns true if the capture device is capable of capturing from a
 * component video source; false otherwise.
 */
bool kc_device_supports_component_capture(void);

/*!
 * Returns true if the capture device is capable of capturing from a
 * composite video source; false otherwise.
 */
bool kc_device_supports_composite_capture(void);

/*!
 * Returns true if the capture device supports hardware de-interlacing;
 * false otherwise.
 */
bool kc_device_supports_deinterlacing(void);

/*!
 * Returns true if the capture device is capable of capturing from an
 * S-Video source; false otherwise.
 */
bool kc_device_supports_svideo(void);

/*!
 * Returns true if the capture device is capable of streaming frames via
 * direct memory access (DMA); false otherwise.
 */
bool kc_device_supports_dma(void);

/*!
 * Returns true if the capture device is capable of capturing from a
 * digital (DVI) source; false otherwise.
 */
bool kc_device_supports_dvi(void);

/*!
 * Returns true if the capture device is capable of capturing from an
 * analog (VGA) source; false otherwise.
 */
bool kc_device_supports_vga(void);

/*!
 * Returns true if the capture device is capable of capturing in YUV
 * color; false otherwise.
 */
bool kc_device_supports_yuv(void);

/*!
 * Returns the number of input channels on the capture device that're
 * available to the interface.
 *
 * The value returned is an integer in the range [1,n] such that if the
 * capture device has, for instance, 16 input channels and the interface
 * can use two of them, 2 is returned.
 *
 * @see
 * kc_get_device_input_channel_idx()
 */
int kc_get_device_maximum_input_count(void);

/*!
 * Returns a string that identifies the capture device's firmware version;
 * e.g. "14.12.3".
 *
 * Will return "Unknown" if the firmware version is not known.
 *
 * @see
 * kc_get_device_driver_version()
 */
std::string kc_get_device_firmware_version(void);

/*!
 * Returns a string that identifies the capture device's driver version;
 * e.g. "14.12.3".
 *
 * Will return "Unknown" if the firmware version is not known.
 *
 * @see
 * kc_get_device_firmware_version()
 */
std::string kc_get_device_driver_version(void);

/*!
 * Returns a string that identifies the capture device; e.g. "Datapath
 * VisionRGB-PRO2".
 *
 * Will return "Unknown" if no name is available.
 */
std::string kc_get_device_name(void);

/*!
 * Returns a string that identifies the interface; e.g. "RGBEasy".
 */
std::string kc_get_device_api_name(void);

/*!
 * Returns the capture device's current video signal parameters.
 *
 * @see
 * kc_set_video_signal_parameters(), kc_get_device_video_parameter_minimums(),
 * kc_get_device_video_parameter_maximums(),
 * kc_get_device_video_parameter_defaults()
 */
video_signal_parameters_s kc_get_device_video_parameters(void);

/*!
 * Returns the capture device's default video signal parameters.
 *
 * @see
 * kc_get_device_video_parameters(), kc_get_device_video_parameters(),
 * kc_get_device_video_parameter_minimums(),
 * kc_get_device_video_parameter_maximums()
 */
video_signal_parameters_s kc_get_device_video_parameter_defaults(void);

/*!
 * Returns the minimum value supported by the capture device for each video
 * signal parameter.
 *
 * @see
 * kc_set_video_signal_parameters(), kc_get_device_video_parameters(),
 * kc_get_device_video_parameter_maximums(),
 * kc_get_device_video_parameter_defaults()
 */
video_signal_parameters_s kc_get_device_video_parameter_minimums(void);

/*!
 * Returns the maximum value supported by the capture device for each video
 * signal parameter.
 *
 * @see
 * kc_set_video_signal_parameters(), kc_get_device_video_parameters(),
 * kc_get_device_video_parameter_minimums(),
 * kc_get_device_video_parameter_defaults()
 */
video_signal_parameters_s kc_get_device_video_parameter_maximums(void);

/*!
 * Returns the capture device's current input/output resolution.
 *
 * @note
 * The capture device's input and output resolutions are expected to always
 * be equal; any scaling of captured frames is expected to be done by VCS and
 * not the capture device.
 *
 * @warning
 * Don't take this to be the resolution of the latest captured frame
 * (returned from kc_get_frame_buffer()), as the capture device's resolution
 * may have changed since that frame was captured.
 *
 * @see
 * kc_set_capture_resolution(), kc_get_device_minimum_resolution(),
 * kc_get_device_maximum_resolution()
 */
resolution_s kc_get_capture_resolution(void);

/*!
 * Returns the minimum capture resolution supported by the interface.
 *
 * @note
 * This resolution may be larger - but not smaller - than the minimum
 * capture resolution supported by the capture device.
 *
 * @see
 * kc_get_device_maximum_resolution(), kc_get_capture_resolution(),
 * kc_set_capture_resolution()
 */
resolution_s kc_get_device_minimum_resolution(void);

/*!
 * Returns the maximum capture resolution supported by the interface.
 *
 * @note
 * This resolution may be smaller - but not larger - than the maximum
 * capture resolution supported by the capture device.
 *
 * @see
 * kc_get_device_minimum_resolution(), kc_get_capture_resolution(),
 * kc_set_capture_resolution()
 */
resolution_s kc_get_device_maximum_resolution(void);

/*!
 * Returns the number of frames the interface has received from the capture
 * device which VCS was too busy to process and display. These are, in
 * effect, dropped frames.
 *
 * If this value is above 0, it indicates that VCS is failing to process
 * and display captured frames as fast as the capture device is producing
 * them. This could be a symptom of e.g. an inadequately performant host
 * CPU.
 *
 * @note
 * This value must be cumulative over the lifetime of the program's
 * execution and must not decrease during that time.
 */
unsigned kc_get_missed_frames_count(void);

/*!
 * Returns the index value of the capture device's input channel on which the
 * device is currently listening for signals. The value is in the range [0,n-1],
 * where n = kc_get_device_maximum_input_count().
 *
 * If the capture device has more input channels than are supported by the
 * interface, the interface is expected to map the index to a consecutive range.
 * For example, if channels #1, #5, and #6 on the capture device are available
 * to the interface, capturing on channel #5 would correspond to an index value
 * of 1, with index 0 being channel #1 and index 2 channel #6.
 *
 * @see
 * kc_get_device_maximum_input_count()
 */
unsigned kc_get_device_input_channel_idx(void);

/*!
 * Returns the refresh rate of the current capture signal.
 */
refresh_rate_s kc_get_capture_refresh_rate(void);

/*!
 * Returns the color depth, in bits, that the interface currently expects
 * the capture device to send captured frames in.
 *
 * For RGB888 frames the color depth would be 32; 16 for RGB565 frames; etc.
 *
 * The color depth of a given frame as returned from kc_get_frame_buffer() may
 * be different from this value e.g. if the capture color depth was changed
 * just after the frame was captured.
 */
unsigned kc_get_capture_color_depth(void);

/*!
 * Returns the pixel format that the capture device is currently storing
 * its captured frames in.
 *
 * The pixel format of a given frame as returned from kc_get_frame_buffer() may
 * be different from this value e.g. if the capture pixel format was changed
 * just after the frame was captured.
 */
capture_pixel_format_e kc_get_capture_pixel_format(void);

/*!
 * Returns true if the current capture signal is invalid; false otherwise.
 *
 * @see
 * has_no_signal()
 */
bool kc_has_invalid_signal(void);

/*!
 * Returns true if the current capture signal is valid; false otherwise.
 *
 * @see
 * has_invalid_signal(), has_no_signal()
 */
bool kc_has_valid_signal(void);

/*!
 * Returns true if the current capture device is invalid; false otherwise.
 * This could happen e.g. if the user (on Linux) opens a /dev/videoX that
 * doesn't provide a compatible capture device.
 */
bool kc_has_invalid_device(void);

/*!
 * Returns true if the current capture device is valid; false otherwise.
 */
bool kc_has_valid_device(void);

/*!
 * Returns true if there's currently no capture signal on the capture
 * device's active input channel; false otherwise.
 *
 * @see
 * has_signal(), kc_get_device_input_channel_idx(), kc_set_capture_input_channel()
 */
bool kc_has_no_signal(void);

/*!
 * Returns true if the capture device's active input channel is currently
 * receiving a signal; false otherwise.
 *
 * @see
 * has_no_signal(), kc_get_device_input_channel_idx(), kc_set_capture_input_channel()
 */
bool kc_has_signal(void);

/*!
 * Returns true if the device is currently capturing; false otherwise.
 */
bool kc_is_capturing(void);

/*!
 * Returns a reference to the most recent captured frame.
 *
 * @code
 * // The capture mutex should be locked first, to ensure that the frame buffer
 * // isn't modified by another thread while we're accessing it.
 * std::lock_guard<std::mutex> lock(kc_capture_mutex());
 *
 * const auto &frameBuffer = kc_get_frame_buffer();
 * // Access the frame buffer's data...
 * @endcode
 * 
 * @code
 * // Capture subsystem events guarantee that the capture mutex has been locked
 * // for the duration of the subscribed event handler.
 * kcEvent_frameCaptured.subscribe([]
 * {
 *     // kc_capture_mutex() == locked.
 * 
 *     const auto &frameBuffer = kc_get_frame_buffer();
 *     // Access the frame buffer's data...
 * });
 * @endcode
 * 
 * @warning
 * To ensure that the frame buffer's data isn't modified by another thread while
 * you're accessing it, acquire the capture mutex before calling this function.
 *
 * @see
 * kc_capture_mutex()
 */
const captured_frame_s& kc_get_frame_buffer(void);

/*!
 * Called by VCS to notify the interface that VCS has finished processing the
 * latest frame obtained via kc_get_frame_buffer(). The inteface is then free
 * to e.g. overwrite the frame's data.
 *
 * Returns true on success; false otherwise.
 */
bool kc_mark_frame_buffer_as_processed(void);

/*!
 * Returns the latest capture event and removes it from the interface's
 * event queue. The caller can then respond to the event; e.g. by calling
 * kc_get_frame_buffer() if the event is a new frame.
 */
capture_event_e kc_pop_capture_event_queue(void);

/*!
 * Assigns to the capture device the given video signal parameters.
 *
 * Returns true on success; false otherwise.
 *
 * @see
 * kc_get_device_video_parameters(), kc_get_device_video_parameter_minimums(),
 * kc_get_device_video_parameter_maximums(), kc_get_device_video_parameter_defaults()
 */
bool kc_set_video_signal_parameters(const video_signal_parameters_s &p);

/*!
 * Sets the capture device's deinterlacing mode.
 *
 * Returns true on success; false otherwise.
 */
bool kc_set_deinterlacing_mode(const capture_deinterlacing_mode_e mode);

/*!
 * Tells the capture device to start listening for signals on the given
 * input channel.
 *
 * Returns true on success; false otherwise.
 *
 * @see
 * kc_get_device_input_channel_idx(), kc_get_device_maximum_input_count()
 */
bool kc_set_capture_input_channel(const unsigned idx);

/*!
 * Tells the capture device to store its captured frames using the given
 * pixel format.
 *
 * Returns true on success; false otherwise.
 *
 * @see
 * kc_get_capture_pixel_format()
 */
bool kc_set_capture_pixel_format(const capture_pixel_format_e pf);

/*!
 * Tells the capture device to adopt the given resolution as its input and
 * output resolution.
 *
 * @note
 * The capture device must adopt this as both its input and output
 * resolution. For example, if this resolution is 800 x 600, the capture
 * device should interpret the video signal as if it were 800 x 600, rather
 * than scaling the frame to 800 x 600 after capturing.
 *
 * @warning
 * Since this will be set as the capture device's input resolution,
 * captured frames may exhibit artefacting if the resolution doesn't match
 * the video signal's true resolution.
 *
 * Returns true on success; false otherwise.
 *
 * @see
 * kc_get_capture_resolution(), kc_get_device_minimum_resolution(),
 * kc_get_device_maximum_resolution()
 */
bool kc_set_capture_resolution(const resolution_s &r);

#endif
