/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/unsharp_mask/filter_unsharp_mask.h"
#include "common/globals.h"
#include <opencv2/imgproc/imgproc.hpp>

void filter_unsharp_mask_c::apply(image_s *const image)
{
    this->assert_input_validity(image);

    static uint8_t *const tmpBuf = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

    const double str = this->parameter(PARAM_STRENGTH);
    const double rad = this->parameter(PARAM_RADIUS);
    cv::Mat tmp = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, tmpBuf);
    cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
    
    cv::GaussianBlur(output, tmp, cv::Size(0, 0), rad);
    cv::addWeighted(output, 1 + str, tmp, -str, 0, output);

    return;
}
