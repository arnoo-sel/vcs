/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

/*! @file
 *
 * @brief
 * The anti-tear subsystem interface.
 * 
 * The anti-tear subsystem is responsible for reducing temporal tearing artifacts
 * in captured frames.
 * 
 * Generally, tearing artifacts occur when the capture device samples an image of
 * the input signal while the signal source is still in the process of drawing the
 * image. The captured frame will then consists of parts of new image data and
 * parts of old image data, with tears occurring where new and old parts meet.
 * 
 * The anti-tear subsystem takes as input a series of images and produces as
 * output a second, reduced series of images in which the input images' new parts
 * have been combined into un-torn images. For instance, if you input two
 * captured frames in which the top half of the first frame contains one half
 * of an image and the bottom half of the second frame contains the second half
 * of the image, the output will be one whole image.
 * 
 * The gist of the anti-tearing algorithm is that it first compares pixels in the
 * current input image against the pixels in the previous input image (scanning
 * pixels row by row) to find which areas of the image have changed, then
 * copies the new areas' pixels into a back buffer. The algorithm repeats itself
 * across successive input images until the back buffer contains a full de-torn
 * image. A major source of uncertainty in the algorithm comes from the fact
 * that captured frames may contain extraneous analog noise etc., making it less
 * obvious which parts of the image have changed from the previous one.
 * 
 * ## Usage
 * 
 *   1. Call kat_initialize_anti_tear() to initialize the subsystem. This is VCS's
 *      default startup behavior.
 * 
 *   2. Call kat_anti_tear() with the image data on which you want to apply
 *      anti-tearing.
 * 
 *   3. Optionally, call any of the setter functions (@a kat_set_xxxx) to adjust the
 *      anti-tearer's operation.
 * 
 *   4. Call kat_release_anti_tear() to release the subsystem. This is VCS's default
 *      exit behavior.
 *
 */

#ifndef VCS_FILTER_ANTI_TEAR_H
#define VCS_FILTER_ANTI_TEAR_H

#include "common/globals.h"
#include "main.h"

struct resolution_s;
struct image_s;

//! @cond
// Default starting parameter values for the anti-tear engine.
#define KAT_DEFAULT_THRESHOLD 3
#define KAT_DEFAULT_WINDOW_LENGTH 8
#define KAT_DEFAULT_NUM_MATCHES_REQUIRED 11
#define KAT_DEFAULT_STEP_SIZE 1
#define KAT_DEFAULT_VISUALIZE_TEARS false
#define KAT_DEFAULT_VISUALIZE_SCAN_RANGE false
#define KAT_DEFAULT_SCAN_DIRECTION anti_tear_scan_direction_e::down
#define KAT_DEFAULT_SCAN_HINT anti_tear_scan_hint_e::look_for_one_tear
//! @endcond

/*!
 * Enumerates the tear-scanning hints which can be given to the anti-tear
 * subsystem (see kat_set_scan_hint()). These hints provide insight about the input
 * data which may improve the anti-tearer's performance.
 */
enum class anti_tear_scan_hint_e
{
    /*!
     * Tells the anti-tear subsystem that input images are expected to have at
     * most one tear. In other words, a single un-torn image will span no more
     * than two consecutive input images. This may allow the anti-tearer to apply
     * performance optimizations, but will lead to incorrect anti-tearing results
     * if any input image has more than one tear.
     */
    look_for_one_tear,

    /*!
     * Tells the anti-tear subsystem that input images may have several tears.
     * In other words, a single un-torn image may span any number of
     * consecutive input images. The anti-tearer may, however, choose to only
     * scan a certain maximum number of consecutive input images before
     * declaring the result to be a fully de-torn image.
     */
    look_for_multiple_tears,
};

/*!
 * Enumerates the directions in which the anti-tear subsystem can scan images
 * for tears.
 * 
 * The scan direction determines which parts of an image following and preceding
 * a tear are considered by the anti-tearer to be new or old relative to the
 * previous image. For instance, if the scan begins from the top and encounters a
 * tear in the middle, the upper half of the image would be considered new;
 * whereas the lower half would be considered new if the scan had started from
 * the bottom.
 * 
 * For correct anti-tearing results, the scan direction should match the direction
 * in which the capture source draws the image (which in turn may depend e.g. on
 * the video mode).
 */
enum class anti_tear_scan_direction_e
{
    /*! Scans input images from bottom to top.*/
    up,

    /*! Scans input images from top to bottom.*/
    down,
};

/*!
 * Initializes the anti-tear subsystem.
 * 
 * By default, VCS will call this function on program startup.
 *
 * Returns a function that releases the anti-tear subsystem.
 * 
 * @note
 * Will trigger an assertion failure if the initialization fails.
 * 
 * @see
 * kat_release_anti_tear()
 */
subsystem_releaser_t kat_initialize_anti_tear(void);

/*!
 * Submits an image into the anti-tearing process. The parts of the image that are
 * found by the anti-tearer to be new relative to the previous submitted image(s)
 * will be integrated into the anti-tearer's back buffer.
 *
 * If anti-tearing is enabled, this function returns an image whose pixel data points
 * to a subsystem-managed pixel buffer containing a copy of the most recent fully de-
 * torn image.
 *
 * If anti-tearing is disabled, this function does nothing.
 * 
 * The input data won't be modified. 
 * 
 * @note
 * This function returns the most recent fully de-torn image; meaning e.g. that if
 * an image is torn into two consecutive frames, the effective frame rate of the
 * function's output is half of its input, as each output image requires two
 * input images (two consecutive calls will return the same output image until the
 * next fully de-torn image is available).
 * 
 * @warning
 * This function should not be called before the subsystem has been initialized
 * with kat_initialize_anti_tear().
 * 
 * @warning
 * The image returned by this function should be considered invalid after
 * kat_release_anti_tear() has been called.
 * 
 * @see
 * kat_set_anti_tear_enabled()
 */
image_s kat_anti_tear(const image_s &image);

/*!
 * Sets the current anti-tearing scan hint.
 */
void kat_set_scan_hint(const anti_tear_scan_hint_e newHint);

/*!
 * Sets the current anti-tearing scan direction.
 */
void kat_set_scan_direction(const anti_tear_scan_direction_e newDirection);

/*!
 * Sets the vertical range over which the anti-tearer will scan input images
 * for tears.
 * 
 * @p min is the vertical pixel row offset from the first pixel row in the image
 * toward the last row, while @p max is the vertical pixel row offset from the
 * last pixel row in the image toward the first row. If the current scan direction
 * is down, the first pixel row is at the top of the image and the last row is
 * at the bottom, whereas if the scan direction is up, the first pixel row is at
 * the bottom of the image and the last row is at the top.
 * 
 * For example, to scan the bottom half of a 640 x 480 image when the scan
 * direction is down, pass 240 for @p min (start at the 240th vertical pixel row
 * from the top) and 0 for @p max (scan down to the 480 - @p max = 480th vertical
 * pixel row). To scan the same region of the image when the scan direction is up,
 * pass 0 for @p min (start at the 480 - @p min = 480th pixel row) and 240 for @p
 * max (scan up to the 240th pixel row).
 * 
 * @see
 * kat_set_scan_direction()
 */
void kat_set_range(const u32 min, const u32 max);

/*!
 * Sets anti-tearing to enabled/disabled.
 * 
 * When disabled, kat_anti_tear() will act as a passive passthrough and no
 * anti-tearing processing will be performed.
 */
void kat_set_anti_tear_enabled(const bool state);

/*!
 * Toggles visualizing options.
 * 
 * The anti-tear subsystem provides some visual metaoutput of the anti-tearing
 * process.
 * 
 * @p visualizeTear sets whether detected tears are visually indicated in the
 * output image. If true, horizontal lines will be drawn at the locations of
 * the tears out of which the image was constructed.
 * 
 * @p visualizeRange sets whether the current scan range should be visually
 * indicated in the output image. If true, the region in the output image over
 * which the anti-tearer scanned for tears will be shaded.
 */
void kat_set_visualization(const bool visualizeTear, const bool visualizeRange);

/*!
 * Sets the threshold amount by which pixel color values are required to change
 * between two consecutive input images for the pixel to be considered to have
 * changed (accounting for analog noise etc.).
 * 
 * The less temporal pixel noise in the input images, the lower the value of
 * @p t can be.
 */
void kat_set_threshold(const u32 t);

/*!
 * Sets the size of the tear scanner's horizontal sampling window.
 * 
 * When scanning input images for tears, the anti-tearer averages together
 * a number of adjacent horizontal pixels' color values to reduce the negative
 * effect of temporal analog noise on the accuracy of the scan. The size of the
 * sampling window determines the number of adjacent pixels to average together.
 * 
 * With no temporal pixel noise present, smaller values of @p ds should result
 * in a more accurate tear scan. The more noise there is, the larger the value
 * of @p ds needs to be.
 */
void kat_set_domain_size(const u32 ds);

/*!
 * Sets the number of horizontal pixels to skip when scanning for tears.
 * 
 * The tear scanner will examine every @p s pixels in a given pixel row to
 * determine whether the row has changed between the current input image and the
 * image before that.
 * 
 * Smaller values of @p s will result in a more accurate scan, but higher values
 * will make the scan faster.
 * 
 * @note
 * A step size of 0 would result in an infinite loop, so that value will
 * be converted to 1. A step size larger than the width of the input image
 * will be interpreted as a step size equal to the width of the image, which
 * will result in the scanner only examining the the first pixel on each row.
 */
void kat_set_step_size(const u32 s);

/*!
 * Sets the number of pixels in the row of an input image that must pass the delta
 * value threshold for the row to be considered new data relative to the previous
 * input image.
 * 
 * Due to analog noise etc., the tear scanner is expected to generate some false
 * positives when comparing pixels between adjacent captured frames. This setting
 * reduces the scanner's sensitivity by allowing a greater number of false positives
 * without triggering a false detection of a tear.
 * 
 * The less temporal pixel noise in the input images, the lower the value of
 * @p mr can be.
 * 
 * @see
 * kat_set_threshold()
 */
void kat_set_matches_required(const u32 mr);

#endif
