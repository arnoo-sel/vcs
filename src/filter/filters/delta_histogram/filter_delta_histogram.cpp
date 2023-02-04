/*
 * 2021-2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#include <cmath>
#include "filter/filters/output_scaler/filter_output_scaler.h"
#include "filter/filters/delta_histogram/filter_delta_histogram.h"
#include "common/globals.h"

// Draws a histogram indicating the amount by which pixel values differ between two consecutive
// frames. A single tall bar at the middle of the graph means that pixels don't differ at all,
// while spread toward the edges of the graph means pixels differ - the more the closer the
// bars get to an edge of the graph.
void filter_delta_histogram_c::apply(image_s *const image)
{
    this->assert_input_validity(image);

    static const unsigned numBins = 511; // Representing the range [-255,255].
    static const unsigned graphHeight = 256;
    static const unsigned graphByteSize = (numBins * graphHeight * 4);
    static uint8_t *const prevFramePixels = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();
    static uint8_t *const scaledGraph = new uint8_t[MAX_NUM_BYTES_IN_CAPTURED_FRAME]();
    static uint8_t *const graph = new uint8_t[graphByteSize]();
    static unsigned redBin[numBins];
    static unsigned greenBin[numBins];
    static unsigned blueBin[numBins];

    const unsigned numColorChannels = (image->resolution.bpp / 8);

    // For each RGB channel in a given pixel coordinate, count how many times a particular
    // delta value occurred.
    {
        for (unsigned *const binToReset: {redBin, greenBin, blueBin})
        {
            std::memset(binToReset, 0, (numBins * sizeof(binToReset[0])));
        }

        for (unsigned i = 0; i < (image->resolution.w * image->resolution.h * numColorChannels); i += numColorChannels)
        {
            const unsigned deltaBlue  = ((image->pixels[i+0] + 255) - prevFramePixels[i+0]);
            const unsigned deltaGreen = ((image->pixels[i+1] + 255) - prevFramePixels[i+1]);
            const unsigned deltaRed   = ((image->pixels[i+2] + 255) - prevFramePixels[i+2]);

            blueBin[deltaBlue]++;
            greenBin[deltaGreen]++;
            redBin[deltaRed]++;
        }

        std::memcpy(
            prevFramePixels,
            image->pixels,
            (image->resolution.w * image->resolution.h * numColorChannels)
        );
    }

    // Draw the bins into the histogram graph.
    {
        memset(graph, 0, graphByteSize);

        for (unsigned x = 0; x < numBins; x++)
        {
            const double maxBinHeight = (image->resolution.w * image->resolution.h);
            static const auto draw_bin = [&](const unsigned *const srcBin, std::array<uint8_t, 3> color)
            {
                const unsigned binHeight = std::round((srcBin[x] / maxBinHeight) * (graphHeight - 1));
                for (unsigned y = 0; y < binHeight; y++)
                {
                    const unsigned idx = ((x + (y + (graphHeight / 2) - (binHeight / 2)) * numBins) * numColorChannels);
                    graph[idx+0] = color[0];
                    graph[idx+1] = color[1];
                    graph[idx+2] = color[2];
                    graph[idx+3] = 255;
                }
            };

            draw_bin(blueBin, {255, 150, 0});
            draw_bin(greenBin, {0, 255, 0});
            draw_bin(redBin, {0, 0, 255});
        }
    }

    // Copy the histogram graph into the output image.
    {
        const image_s graphImage = image_s(graph, {numBins, graphHeight, 32});
        image_s scaledGraphImage = image_s(scaledGraph, image->resolution);

        filter_output_scaler_c::nearest(graphImage, &scaledGraphImage);

        for (unsigned y = 0; y < image->resolution.h; y++)
        {
            for (unsigned x = 0; x < image->resolution.w; x++)
            {
                const unsigned idx = ((x + y * image->resolution.w) * 4);

                // Foreground.
                if (scaledGraph[idx+3])
                {
                    image->pixels[idx+0] = scaledGraph[idx+0];
                    image->pixels[idx+1] = scaledGraph[idx+1];
                    image->pixels[idx+2] = scaledGraph[idx+2];
                }
                // Background.
                else
                {
                    image->pixels[idx+0] = LERP(0, image->pixels[idx+0], 0.25);
                    image->pixels[idx+1] = LERP(0, image->pixels[idx+1], 0.25);
                    image->pixels[idx+2] = LERP(0, image->pixels[idx+2], 0.25);
                }
            }
        }
    }

    return;
}
