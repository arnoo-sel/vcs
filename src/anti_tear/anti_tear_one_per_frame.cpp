/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#include <cstring>
#include <cmath>
#include "anti_tear/anti_tear_frame.h"
#include "anti_tear/anti_tear_one_per_frame.h"
#include "anti_tear/anti_tearer.h"
#include "capture/capture.h"

void anti_tear_one_per_frame_c::initialize(anti_tearer_c *const parent)
{
    this->base = parent;

    return;
}

void anti_tear_one_per_frame_c::process(const anti_tear_frame_s *const frame)
{
    bool nonTornFrameAlreadyCopied = false;

    switch (this->nextAction)
    {
        // If we found a tear last frame, finish copying its data from this new frame
        // to generate a finished front buffer.
        case next_action_e::copy_rest_of_pixel_data:
        {
            this->base->copy_frame_pixel_rows(frame, this->base->backBuffer, 0, this->latestTearRow);
            this->base->tornRowIndices.clear();
            std::swap(this->base->backBuffer, this->base->frontBuffer);

            this->nextAction = next_action_e::scan_for_tear;
            nonTornFrameAlreadyCopied = true;

            // Fall through to scan for the next tear.
        }
        // Look for a new tear and populate the back buffer with its data.
        case next_action_e::scan_for_tear:
        {
            this->latestTearRow = this->find_first_new_row_idx(frame, this->base->scanStartRow, this->base->scanEndRow);

            k_assert((this->latestTearRow <= int(frame->resolution.h)), "Tear row overflows frame's height.");

            // If we found a tear.
            if (this->latestTearRow >= 0)
            {
                this->base->copy_frame_pixel_rows(frame, this->base->backBuffer, this->latestTearRow, frame->resolution.h);
                this->nextAction = next_action_e::copy_rest_of_pixel_data;
                this->base->tornRowIndices.push_back(this->latestTearRow);
            }
            // Otherwise, we assume the frame is not torn and can be presented as-is.
            else if (!nonTornFrameAlreadyCopied)
            {
                this->base->copy_frame_pixel_rows(frame, this->base->frontBuffer, 0, frame->resolution.h);
                this->base->tornRowIndices.clear();
            }

            break;
        }
    }

    return;
}

int anti_tear_one_per_frame_c::find_first_new_row_idx(const anti_tear_frame_s *const frame,
                                                      const unsigned startRow,
                                                      const unsigned endRow)
{
    k_assert((startRow < frame->resolution.h), "The starting row overflows the target resolution.");

    // The image is expected to be filled in from bottom to top, so if
    // the first row is new, there can be no tear.
    if (this->base->has_pixel_row_changed(startRow, frame->pixels, this->base->frontBuffer, frame->resolution))
    {
        return -1;
    }

    int firstNewRow = (endRow + 1);
    int prevRow = startRow;
    int rowDelta = ((endRow - startRow) / 2);

    // Scan the vertical range's horizontal pixel rows by iteratively splitting
    // the vertical range in half in the direction of the tear.
    for (int curRow = (startRow + rowDelta); curRow != firstNewRow; curRow += rowDelta)
    {
        const bool isNewRow = this->base->has_pixel_row_changed(curRow, frame->pixels, this->base->frontBuffer, frame->resolution);
        const unsigned numRowsSkipped = std::abs(curRow - prevRow);
        const int sign = (isNewRow? -1 : 1);

        firstNewRow = (isNewRow? curRow : firstNewRow);
        rowDelta = (sign * std::max(1u, (numRowsSkipped / 2)));
        prevRow = curRow;
    }

    return ((unsigned(firstNewRow) > endRow)? -1 : firstNewRow);
}

void anti_tear_one_per_frame_c::visualize_current_tear(void)
{
    anti_tear_frame_s &frame = this->base->presentBuffer;

    if (this->latestTearRow >= 0)
    {
        const unsigned bpp = (frame.resolution.bpp / 8);
        const unsigned idx = (this->latestTearRow * frame.resolution.w * bpp);
        std::memset((this->base->presentBuffer.pixels + idx), 255, (frame.resolution.w * bpp));
    }

    return;
}
