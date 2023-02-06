/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

/*! @file
 *
 * @brief
 * The scaler subsystem interface.
 * 
 * The scaler subsystem provides facilities to VCS for scaling captured frames.
 * 
 * When an input frame is scaled, its pixel data is copied into the subsystem's
 * frame buffer, from which callers can fetch it until the next image is scaled
 * (or until the buffer's pixel data is modified in some other way).
 * 
 * By default, it's the state of the scaler subsystem's frame buffer that gets
 * displayed to the end-user in VCS's capture window.
 * 
 * ## Usage
 * 
 *   1. Call ks_initialize_scaler() to initialize the subsystem. This is VCS's
 *      default startup behavior. Note that this function should be called only
 *      once per program execution.
 * 
 *   2. Use setter functions to customize scaler options:
 *      @code
 *      ks_set_scaling_multiplier(0.75);
 *      ks_set_downscaling_filter("Linear");
 *      ks_set_base_resolution({640, 480});
 *      @endcode
 * 
 *   3. Feed captured frames into ks_scale_frame(), then read the scaled output from
 *      ks_frame_buffer():
 *      @code
 *      ks_scale_frame(frame);
 *      const auto &scaledImage = ks_frame_buffer();
 *      @endcode
 * 
 *   4. You can automate the scaling of captured frames using event listeners:
 *      @code
 *      // Executed each time the capture subsystem reports a new captured frame.
 *      kc_evNewCapturedFrame.listen([](const captured_frame_s &frame)
 *      {
 *          ks_scale_frame(frame);
 *      });
 * 
 *      // Executed each time the scaler subsystem produces a new scaled image.
 *      ks_evNewScaledImage.listen([](const image_s &image)
 *      {
 *          printf("A frame was scaled to %lu x %lu.\n", image.resolution.w, image.resolution.h);
 *      });
 *      @endcode
 *
 *   5. VCS will automatically release the scaler subsystem on program exit.
 * 
 */

#ifndef VCS_SCALER_SCALER_H
#define VCS_SCALER_SCALER_H

#include "common/globals.h"
#include "common/propagate/vcs_event.h"
#include "filter/filters/render_text/font_5x3.h"
#include "main.h"

class abstract_filter_c;
struct captured_frame_s;
struct image_s;

/*!
 * Callable inside a filter's apply() function to inform the user of an error
 * related to the filter's operation. The error string will be printed onto the
 * current output frame.
 */
#define KS_PRINT_FILTER_ERROR(errorString) \
    font_5x3_c().render(("FILTER ERROR [" + (this->name()) + "]:\n" + errorString), image, 0, 0, 2, {0, 0, 255}, {0, 0, 0});

/*!
 * Basic container for an image scaler.
 */
struct image_scaler_s
{
     /*! The scaler's display name. Will be shown in the GUI etc.*/
    std::string name;

    /*! A function that executes the scaler on the given pixels.*/
    void (*apply)(const image_s &srcImage, image_s *const dstImage, const std::array<unsigned, 4> padding);
};

/*!
 * An event fired when the scaler subsystem scales a frame into a resolution
 * different from the previous frame's.
 * 
 * @code
 * ks_scale_frame(frame);
 * 
 * ks_set_scaling_multiplier(ks_scaling_multiplier() + 1);
 * 
 * // Fires ks_evNewOutputResolution.
 * ks_scale_frame(frame);
 * @endcode
 */
extern vcs_event_c<const resolution_s&> ks_evNewOutputResolution;

/*!
 * An event fired when the scaler subsystem has finished scaling a frame.
 * 
 * @code
 * ks_evNewScaledImage.listen([](const image_s &scaledImage)
 * {
 *     printf("Scaled a frame to %lu x %lu.\n", scaledImage.resolution.w, scaledImage.resolution.h);
 * });
 * @endcode
 * 
 * @see
 * ks_scale_frame()
 */
extern vcs_event_c<const image_s&> ks_evNewScaledImage;

/*!
 * An event fired once per second, giving the number of input frames the scaler
 * subsystem scaled during that time.
 * 
 * @code
 * ks_evFramesPerSecond.listen([](unsigned numFrames)
 * {
 *     printf("Scaled %u frames per second.\n", numFrames);
 * });
 * @endcode
 * 
 * @see
 * ks_scale_frame()
 */
extern vcs_event_c<unsigned> ks_evFramesPerSecond;

/*!
 * An event fired when an output scaling filter becomes active, i.e. when captured
 * frames begin being scaled using such a filter.
 */
extern vcs_event_c<void> ks_evCustomScalerEnabled;

/*!
 * An event fired when there's no longer an output scaling filter being used to
 * scale captured frames.
 */
extern vcs_event_c<void> ks_evCustomScalerDisabled;

/*!
 * Returns the resolution to which the scaler will scale input frames, prior to
 * the application of resolution-influencing modifiers like a scaling multiplier.
 * 
 * In most cases, you'd be interested in ks_output_resolution(), instead.
 * 
 * @see
 * ks_output_resolution(), ks_scale_frame()
 */
resolution_s ks_base_resolution(void);

/*!
 * Returns the resolution to which the scaler would currently scale an input frame.
 *
 * @see
 * ks_scale_frame(), ks_base_resolution
 */
resolution_s ks_output_resolution(void);

/*!
 * Returns true if a custom output scaling filter is currently active; false otherwise.
 */
bool ks_is_custom_scaler_active(void);

/*!
 * Initializes the scaler subsystem.
 *
 * By default, VCS will call this function automatically on program startup.
 *
 * Returns a function that releases the scaler subsystem.
 * 
 * @note
 * This function should be called before any other functions of the scaler interface.
 */
subsystem_releaser_t ks_initialize_scaler(void);

/*!
 * Applies scaling to the given frame's pixels and stores the result in the scaler
 * subsystem's frame buffer. The input data are not modified.
 * 
 * After this call, the scaled image is available via ks_frame_buffer().
 * 
 * @code
 * ks_scale_frame(frame);
 * const auto &scaledFrame = ks_frame_buffer();
 * @endcode
 * 
 * @code
 * // Feed captured frames into the scaler using a VCS event listener.
 * kc_evNewCapturedFrame.listen([](const captured_frame_s &frame)
 * {
 *     ks_scale_frame(frame);
 * });
 * 
 * // Receive scaled frames from the scaler.
 * ks_evNewScaledImage.listen([](const image_s &image)
 * {
 *     printf("A frame was scaled to %lu x %lu.\n", image.resolution.w, image.resolution.h);
 * });
 * @endcode
 * 
 * @see
 * ks_frame_buffer(), ks_evNewScaledImage
 */
void ks_scale_frame(const captured_frame_s &frame);

void ks_set_scaling_multiplier(void);

/*!
 * Enables or disables the scaler subsystem's scaling multiplier.
 * 
 * @see
 * ks_set_scaling_multiplier()
 */
void ks_set_scaling_multiplier_enabled(const bool enabled);

/*!
 * Sets the resolution to which input frames are to be scaled, before applying
 * size modifiers like scaling multiplier. By default, the scaler will apply the
 * modifiers to each input frame's own resolution.
 * 
 * @code
 * // Frames will be scaled to 2x of their original resolution.
 * ks_set_base_resolution_enabled(false);
 * ks_set_scaling_multiplier(2);
 * 
 * // Frames will be scaled to 2x of 640 x 480.
 * ks_set_base_resolution_enabled(true);
 * ks_set_base_resolution({640, 480});
 * ks_set_scaling_multiplier(2);
 * @endcode
 * 
 * @see
 * ks_set_base_resolution_enabled()
 */
void ks_set_base_resolution(const resolution_s &r);

/*!
 * Enables or disables the scaler subsystem's overridable base resolution.
 * 
 * If disabled, the base resolution will be the resolution of the input frame.
 * 
 * @see
 * ks_set_base_resolution()
 */
void ks_set_base_resolution_enabled(const bool enabled);

/*!
 * Draws a "no signal" image into the scaler subsystem's frame buffer, erasing
 * any previous image there.
 * 
 * @note
 * A subsequent call to ks_scale_frame() will overwite the image.
 * 
 * @code
 * // Produce a "no signal" image when the capture device loses its signal.
 * kc_evSignalLost.listen(ks_indicate_no_signal);
 * 
 * // Note: The kc_evSignalLost event fires when the capture device loses its
 * // signal, but not in the case where the device already has no signal when
 * // VCS is launched.
 * @endcode
 * 
 * @see
 * kc_evSignalLost
 */
void ks_indicate_no_signal(void);

/*!
 * Draws an "invalid signal" image into the scaler subsystem's frame buffer,
 * erasing any previous image there.
 * 
 * @note
 * A subsequent call to ks_scale_frame() will overwite the image.
 * 
 * @code
 * // Produce an "invalid signal" image when the capture device loses its signal.
 * kc_evInvalidSignal.listen(ks_indicate_invalid_signal);
 * @endcode
 * 
 * @see
 * kc_evInvalidSignal
 */
void ks_indicate_invalid_signal(void);

/*!
 * Returns a reference to the scaler subsystem's frame buffer.
 * 
 * The frame buffer contains the most recent image produced by the subsystem.
 * This may be a scaled frame (produced by ks_scale_frame()) or some other type
 * of image (e.g. one produced by ks_indicate_no_signal()).
 * 
 * @see
 * ks_scale_frame(), ks_indicate_no_signal(), ks_indicate_invalid_signal()
 */
image_s ks_scaler_frame_buffer(void);

/*!
 * Returns a list of the names of the image scalers available in this build of VCS.
 * 
 * @code
 * const auto list = ks_scaler_names();
 * // list == {"Nearest", "Linear", "Area", ...}.
 * @endcode
 */
std::vector<std::string> ks_scaler_names(void);

/*!
 * Sets the multiplier by which input frames are scaled. The multiplier applies
 * to both the width and height of the frame.
 * 
 * @code
 * // Input frames will be upscaled by 2x using the current upscaling filter.
 * ks_set_scaling_multiplier(2);
 * 
 * // Input frames will be downscaled by half using the current downscaling filter.
 * ks_set_scaling_multiplier(0.5);
 * @endcode
 * 
 * @see
 * ks_scale_frame()
 */
void ks_set_scaling_multiplier(const double s);

/*!
 * Sets the scaler to be used when the resolution of captured frames doesn't match
 * the resolution of the output window, such that the frames are scaled to the size
 * of the window. The desired filter is identified by @p name.
 * 
 * @code
 * // Produce an image downscaled by 0.75x using a linearly sampling scaler.
 * ks_set_default_scaler("Linear");
 * ks_set_scaling_multiplier(0.75);
 * ks_scale_frame(frame);
 * const auto &scaledImage = ks_frame_buffer();
 * @endcode
 * 
 * @note
 * @p name must be one of the strings returned by ks_scaler_names().
 * 
 * @see
 * ks_scaler_names(), ks_set_scaling_multiplier()
 */
void ks_set_default_scaler(const std::string &name);

/*!
 * Returns a pointer to the currently-active default scaler.
 *
 * @see
 * ks_set_default_scaler()
 */
const image_scaler_s* ks_default_scaler(void);
#endif
