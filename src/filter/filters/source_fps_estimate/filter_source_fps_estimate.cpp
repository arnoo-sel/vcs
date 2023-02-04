/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/source_fps_estimate/filter_source_fps_estimate.h"
#include "filter/filters/render_text/filter_render_text.h"
#include "filter/filters/render_text/font_fraps.h"
#include "capture/capture.h"

static const auto FONT = font_fraps_c();
static const unsigned FONT_SIZE = 2;

// Counts the number of unique frames per second, i.e. frames in which the pixels change
// between frames by less than a set threshold (which is to account for analog capture
// artefacts).
void filter_frame_rate_c::apply(image_s *const image)
{
    this->assert_input_validity(image);

    static uint8_t *const prevPixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();
    static auto timeOfLastUpdate = std::chrono::system_clock::now();
    static unsigned numUniqueFrames = 0;
    static unsigned estimatedFPS = 0;

    const unsigned threshold = this->parameter(PARAM_THRESHOLD);
    const unsigned cornerId = this->parameter(PARAM_CORNER);
    const unsigned fgColorId = this->parameter(PARAM_TEXT_COLOR);
    const uint8_t bgAlpha = this->parameter(PARAM_BG_ALPHA);
    const bool isSingleRow = this->parameter(PARAM_IS_SINGLE_ROW_ENABLED);
    const unsigned rowNumber = this->parameter(PARAM_SINGLE_ROW_NUMBER);

    // Find out whether any pixel in the current frame differs from the previous frame
    // by more than the threshold.
    {
        const unsigned imageBitsPerPixel = (image->resolution.bpp / 8);
        const unsigned imageByteSize = (image->resolution.w * image->resolution.h * imageBitsPerPixel);
        const unsigned start = (
            isSingleRow
                ? (rowNumber * image->resolution.w * imageBitsPerPixel)
                : 0
        );
        const unsigned end = std::min(imageByteSize,
            isSingleRow
                ? unsigned(start + (image->resolution.w * imageBitsPerPixel))
                : imageByteSize
        );

        for (unsigned i = start; i < end; i += imageBitsPerPixel)
        {
            if (
                (std::abs(int(image->pixels[i + 0]) - int(prevPixels[i + 0])) >= threshold) ||
                (std::abs(int(image->pixels[i + 1]) - int(prevPixels[i + 1])) >= threshold) ||
                (std::abs(int(image->pixels[i + 2]) - int(prevPixels[i + 2])) >= threshold)
            ){
                numUniqueFrames++;
                memcpy(prevPixels, image->pixels, imageByteSize);
                break;
            }
        }
    }

    // Update the FPS reading.
    {
        const auto timeNow = std::chrono::system_clock::now();
        const double timeElapsed = (std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - timeOfLastUpdate).count() / 500.0);

        if (timeElapsed >= 1)
        {
            estimatedFPS = std::round(2 * (numUniqueFrames / timeElapsed));
            numUniqueFrames = 0;
            timeOfLastUpdate = timeNow;
        }
    }

    // Draw the FPS counter into the image.
    {
        const unsigned signalRefreshRate = kc_current_capture_state().input.refreshRate.value<unsigned>();
        const std::string outputString = (std::to_string(std::min(estimatedFPS, signalRefreshRate)) + ((estimatedFPS >= signalRefreshRate)? "+" : ""));

        const std::pair<unsigned, unsigned> screenCoords = ([cornerId, &outputString, image]()->std::pair<unsigned, unsigned>
        {
            const unsigned textWidth = (FONT_SIZE * FONT.width_of(outputString));
            const unsigned textHeight = (FONT_SIZE * FONT.height_of(outputString));

            switch (cornerId)
            {
                default:
                case TOP_LEFT: return {0, 0};
                case TOP_RIGHT: return {(image->resolution.w - textWidth), 0};
                case BOTTOM_RIGHT: return {(image->resolution.w - textWidth), (image->resolution.h - textHeight)};
                case BOTTOM_LEFT: return {0, (image->resolution.h - textHeight)};
            }
        })();

        const auto fgColor = ([fgColorId]()->std::vector<uint8_t>
        {
            switch (fgColorId)
            {
                default:
                case TEXT_GREEN: return {0, 255, 0};
                case TEXT_PURPLE: return {255, 0, 255};
                case TEXT_RED: return {0, 0, 255};
                case TEXT_YELLOW: return {0, 255, 255};
                case TEXT_WHITE: return {255, 255, 255};
            }
        })();

        FONT.render(outputString, image, screenCoords.first, screenCoords.second, FONT_SIZE, fgColor, {0, 0, 0, bgAlpha});
    }

    return;
}
