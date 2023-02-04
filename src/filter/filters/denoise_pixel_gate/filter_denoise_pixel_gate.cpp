/*
 * 2021 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include "filter/filters/denoise_pixel_gate/filter_denoise_pixel_gate.h"
#include "common/globals.h"

// Reduces temporal image noise by requiring that pixels between frames vary by at
// least a threshold value before being updated on screen.
void filter_denoise_pixel_gate_c::apply(image_s *const image)
{
    this->assert_input_validity(image);

    const unsigned threshold = this->parameter(PARAM_THRESHOLD);
    static uint8_t *const prevPixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();

    for (unsigned i = 0; i < (image->resolution.h * image->resolution.w); i++)
    {
        const unsigned idx = (i * (image->resolution.bpp / 8));

        if (
            (std::abs(image->pixels[idx + 0] - prevPixels[idx + 0]) > threshold) ||
            (std::abs(image->pixels[idx + 1] - prevPixels[idx + 1]) > threshold) ||
            (std::abs(image->pixels[idx + 2] - prevPixels[idx + 2]) > threshold)
        ){
            prevPixels[idx + 0] = image->pixels[idx + 0];
            prevPixels[idx + 1] = image->pixels[idx + 1];
            prevPixels[idx + 2] = image->pixels[idx + 2];
        }
        else
        {
            image->pixels[idx + 0] = prevPixels[idx + 0];
            image->pixels[idx + 1] = prevPixels[idx + 1];
            image->pixels[idx + 2] = prevPixels[idx + 2];
        }
    }

    return;
}
