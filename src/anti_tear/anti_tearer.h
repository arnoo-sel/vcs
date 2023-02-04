/*
 * 2021 Tarpeeksi Hyvae Soft
 * 
 * Software: VCS
 *
 */

#ifndef VCS_FILTER_ANTI_TEARER_H
#define VCS_FILTER_ANTI_TEARER_H

#include <vector>
#include "capture/capture.h"
#include "common/types.h"
#include "anti_tear/anti_tear.h"
#include "anti_tear/anti_tear_frame.h"
#include "anti_tear/anti_tear_one_per_frame.h"
#include "anti_tear/anti_tear_multiple_per_frame.h"

class anti_tearer_c
{
    friend class anti_tear_one_per_frame_c;
    friend class anti_tear_multiple_per_frame_c;

public:
    void initialize(const resolution_s &maxResolution);

    void release(void);

    // Applies anti-tearing to a copy of the given pixels. Returns a pointer to the copy.
    u8* process(u8 *const pixels,
                const resolution_s &resolution);

    // Anti-tearing parameters.
    unsigned scanStartOffset = 0;
    unsigned scanEndOffset = 0; // Rows from the bottom up, i.e. (height - x).
    anti_tear_scan_direction_e scanDirection = KAT_DEFAULT_SCAN_DIRECTION;
    anti_tear_scan_hint_e scanHint = KAT_DEFAULT_SCAN_HINT;
    unsigned threshold = KAT_DEFAULT_THRESHOLD;
    unsigned stepSize = KAT_DEFAULT_STEP_SIZE;
    unsigned windowLength = KAT_DEFAULT_WINDOW_LENGTH;
    unsigned matchesRequired = KAT_DEFAULT_NUM_MATCHES_REQUIRED;
    bool visualizeTears = KAT_DEFAULT_VISUALIZE_TEARS;
    bool visualizeScanRange = KAT_DEFAULT_VISUALIZE_SCAN_RANGE;

protected:
    // Copies the front buffer's pixels into the present buffer and returns a pointer
    // to the present buffer.
    u8* present_front_buffer(const resolution_s &resolution);

    void copy_frame_pixel_rows(const anti_tear_frame_s *const srcFrame,
                               u8 *const dstBuffer,
                               const unsigned fromRow,
                               const unsigned toRow);

    // Scans the given frame's rowIdx'th row to find whether the row's pixels have
    // chaned since the last full frame.
    bool has_pixel_row_changed(const unsigned rowIdx,
                               const u8 *const newPixels,
                               const u8 *const prevPixels,
                               const resolution_s &resolution);

    // Scans the given frame for a tear within the given row range. If a tear is found,
    // returns the index of the pixel row on which the tear starts. Otherwise, returns
    // -1.
    int find_first_new_row_idx(const anti_tear_frame_s *const frame,
                               const unsigned scanStartOffset,
                               const unsigned scanEndOffset);

    // Draws a visual indicator of the current scan range into the given frame.
    void visualize_scan_range(const anti_tear_frame_s &frame);

    // Draws a visual indicator of the most recently found tears into the given frame.
    void visualize_tears(const anti_tear_frame_s &frame);

    anti_tear_one_per_frame_c onePerFrame;
    anti_tear_multiple_per_frame_c multiplePerFrame;

    // Buffers for constructing whole frames from torn ones. The back buffer
    // accumulates torn frame fragments, and the front buffer stores the latest
    // fully reconstructed frame.
    uint8_t *buffers[2];
    u8 *backBuffer = nullptr;
    u8 *frontBuffer = nullptr;

    // Holds the pixels the anti-tearer considers ready for display; e.g.
    // the latest de-torn frame. Note that this image might hold additional
    // things, like anti-tear parameter visualization.
    anti_tear_frame_s presentBuffer;

    // The maximum size of frames that we can anti-tear.
    resolution_s maximumResolution;

    // The frame row locations of the most recent tears. Used for e.g. visualization.
    std::vector<unsigned> tornRowIndices;

    // The most recent vertical range over which a frame was scanned for tears.
    unsigned scanStartRow = 0;
    unsigned scanEndRow = 0;
};

#endif
