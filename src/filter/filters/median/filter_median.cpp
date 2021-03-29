/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/median/filter_median.h"

#ifdef USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/photo/photo.hpp>
    #include <opencv2/core/core.hpp>
#endif

void filter_median_c::apply(FILTER_FUNC_PARAMS) const
{
    VALIDATE_FILTER_INPUT

    #ifdef USE_OPENCV
        const u8 kernelSize = this->parameter(PARAM_KERNEL_SIZE);

        cv::Mat output = cv::Mat(r.h, r.w, CV_8UC4, pixels);
        cv::medianBlur(output, output, kernelSize);
    #endif

    return;
}