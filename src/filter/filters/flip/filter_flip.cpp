/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/flip/filter_flip.h"
#include "common/globals.h"
#include <opencv2/imgproc/imgproc.hpp>

// Flips the frame horizontally and/or vertically.
void filter_flip_c::apply(image_s *const image)
{
    this->assert_input_validity(image);

    static uint8_t *const scratch = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

    int axis = this->parameter(PARAM_AXIS);

    // 0 = vertical, 1 = horizontal, -1 = both.
    axis = ((axis == 2)? -1 : axis);

    cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
    cv::Mat temp = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, scratch);

    cv::flip(output, temp, axis);
    temp.copyTo(output);

    return;
}
