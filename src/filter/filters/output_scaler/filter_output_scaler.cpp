/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/output_scaler/filter_output_scaler.h"
#include "common/memory/memory.h"
#include "capture/capture.h"
#include "scaler/scaler.h"

#if USE_OPENCV
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/core/core.hpp>
#endif

void filter_output_scaler_c::apply(u8 *const pixels, const resolution_s &r)
{
    assert_input_validity(pixels, r);

    const unsigned width = this->parameter(filter_output_scaler_c::PARAM_WIDTH);
    const unsigned height = this->parameter(filter_output_scaler_c::PARAM_HEIGHT);
    const unsigned padTop = this->parameter(filter_output_scaler_c::PARAM_PADDING_TOP);
    const unsigned padRight = this->parameter(filter_output_scaler_c::PARAM_PADDING_RIGHT);
    const unsigned padBottom = this->parameter(filter_output_scaler_c::PARAM_PADDING_BOTTOM);
    const unsigned padLeft = this->parameter(filter_output_scaler_c::PARAM_PADDING_LEFT);
    const unsigned scalerId = this->parameter(filter_output_scaler_c::PARAM_SCALER);

    const auto scaler_function = ([scalerId]
    {
        switch (scalerId)
        {
            case SCALER_NEAREST: return filter_output_scaler_c::nearest;
            case SCALER_LINEAR: return filter_output_scaler_c::linear;
            case SCALER_AREA: return filter_output_scaler_c::area;
            case SCALER_CUBIC: return filter_output_scaler_c::cubic;
            case SCALER_LANCZOS: return filter_output_scaler_c::lanczos;
            default: k_assert(0, "Unrecognized scaler id."); break;
        }
    })();

    const image_s srcImage = image_s(pixels, r);
    image_s dstImage = image_s(ks_frame_buffer().pixels.data(), {width, height, 32});
    scaler_function(srcImage, &dstImage, {padTop, padRight, padBottom, padLeft});

    return;
}

resolution_s filter_output_scaler_c::output_resolution() const
{
    const unsigned width = this->parameter(filter_output_scaler_c::PARAM_WIDTH);
    const unsigned height = this->parameter(filter_output_scaler_c::PARAM_HEIGHT);
    const unsigned padTop = this->parameter(filter_output_scaler_c::PARAM_PADDING_TOP);
    const unsigned padRight = this->parameter(filter_output_scaler_c::PARAM_PADDING_RIGHT);
    const unsigned padBottom = this->parameter(filter_output_scaler_c::PARAM_PADDING_BOTTOM);
    const unsigned padLeft = this->parameter(filter_output_scaler_c::PARAM_PADDING_LEFT);

    return {
        (width + padLeft + padRight),
        (height + padTop + padBottom),
        32
    };
}

#if USE_OPENCV
    static void scale_using_opencv(
        const image_s &srcImage,
        image_s *const dstImage,
        const std::array<unsigned, 4> &padding,
        const cv::InterpolationFlags interpolator
    ){
        static heap_mem<u8> scratch(MAX_NUM_BYTES_IN_OUTPUT_FRAME, "Output scaler scratch buffer");

        const unsigned padTop = padding[0];
        const unsigned padRight = padding[1];
        const unsigned padBottom = padding[2];
        const unsigned padLeft = padding[3];
        const cv::Mat src = cv::Mat(srcImage.resolution.h, srcImage.resolution.w, CV_8UC4, srcImage.pixels);

        if (padTop || padRight || padBottom || padLeft)
        {
            cv::Mat dstUnpadded = cv::Mat(dstImage->resolution.h, dstImage->resolution.w, CV_8UC4, scratch.data());
            cv::resize(src, dstUnpadded, dstUnpadded.size(), 0, 0, interpolator);

            const unsigned paddedWidth = (dstImage->resolution.w + padLeft + padRight);
            const unsigned paddedHeight = (dstImage->resolution.h + padTop + padBottom);
            cv::Mat dstPadded = cv::Mat(paddedHeight, paddedWidth, CV_8UC4, dstImage->pixels);
            cv::copyMakeBorder(dstUnpadded, dstPadded, padTop, padBottom, padLeft, padRight, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        }
        else
        {
            cv::Mat dst = cv::Mat(dstImage->resolution.h, dstImage->resolution.w, CV_8UC4, dstImage->pixels);
            cv::resize(src, dst, dst.size(), 0, 0, interpolator);
        }

        return;
    }
#endif

static void assert_scaler_input_validity(const image_s &srcImage, image_s *const dstImage)
{
    k_assert(
        (srcImage.resolution.bpp == 32) && (dstImage->resolution.bpp == 32),
        "This scaler can only operate on 32-bit images."
    );
}

void filter_output_scaler_c::nearest(const image_s &srcImage, image_s *const dstImage, const std::array<unsigned, 4> padding)
{
    assert_scaler_input_validity(srcImage, dstImage);

    #if USE_OPENCV
        scale_using_opencv(srcImage, dstImage, padding, cv::INTER_NEAREST);
    #else
        double deltaW = (srcImage.resolution.w / double(dstImage.resolution.w));
        double deltaH = (srcImage.resolution.h / double(dstImage.resolution.h));
        u8 *const dst = dstImage.pixels;

        for (uint y = 0; y < dstImage.resolution.h; y++)
        {
            for (uint x = 0; x < dstImage.resolution.w; x++)
            {
                const uint dstIdx = ((x + y * dstImage.resolution.w) * 4);
                const uint srcIdx = ((uint(x * deltaW) + uint(y * deltaH) * srcImage.resolution.w) * 4);
                memcpy(&dst[dstIdx], &srcImage.pixels[srcIdx], 4);
            }
        }
    #endif

    return;
}

void filter_output_scaler_c::linear(const image_s &srcImage, image_s *const dstImage, const std::array<unsigned, 4> padding)
{
    assert_scaler_input_validity(srcImage, dstImage);

    #if USE_OPENCV
        scale_using_opencv(srcImage, dstImage, padding, cv::INTER_LINEAR);
    #else
        k_assert(0, "Attempted to use a scaling filter that hasn't been implemented for non-OpenCV builds.");
    #endif

    return;
}

void filter_output_scaler_c::area(const image_s &srcImage, image_s *const dstImage, const std::array<unsigned, 4> padding)
{
    assert_scaler_input_validity(srcImage, dstImage);

    #if USE_OPENCV
        scale_using_opencv(srcImage, dstImage, padding, cv::INTER_AREA);
    #else
        k_assert(0, "Attempted to use a scaling filter that hasn't been implemented for non-OpenCV builds.");
    #endif

    return;
}

void filter_output_scaler_c::cubic(const image_s &srcImage, image_s *const dstImage, const std::array<unsigned, 4> padding)
{
    assert_scaler_input_validity(srcImage, dstImage);

    #if USE_OPENCV
        scale_using_opencv(srcImage, dstImage, padding, cv::INTER_CUBIC);
    #else
        k_assert(0, "Attempted to use a scaling filter that hasn't been implemented for non-OpenCV builds.");
    #endif

    return;
}

void filter_output_scaler_c::lanczos(const image_s &srcImage, image_s *const dstImage, const std::array<unsigned, 4> padding)
{
    assert_scaler_input_validity(srcImage, dstImage);

    #if USE_OPENCV
        scale_using_opencv(srcImage, dstImage, padding, cv::INTER_LANCZOS4);
    #else
        k_assert(0, "Attempted to use a scaling filter that hasn't been implemented for non-OpenCV builds.");
    #endif

    return;
}
