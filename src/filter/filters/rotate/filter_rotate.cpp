/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/rotate/filter_rotate.h"
#include "common/globals.h"
#include <opencv2/imgproc/imgproc.hpp>

void filter_rotate_c::apply(image_s *const image)
{
    this->assert_input_validity(image);

    static uint8_t *const scratch = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();
    
    const double angle = this->parameter(PARAM_ROT);
    const double scale = this->parameter(PARAM_SCALE);
    cv::Mat output = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, image->pixels);
    cv::Mat temp = cv::Mat(image->resolution.h, image->resolution.w, CV_8UC4, scratch);
    cv::Mat transf = cv::getRotationMatrix2D(cv::Point2d((image->resolution.w / 2), (image->resolution.h / 2)), -angle, scale);
    
    cv::warpAffine(output, temp, transf, cv::Size(image->resolution.w, image->resolution.h));
    temp.copyTo(output);

    return;
}
