/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#include <unistd.h>
#include "common/globals.h"
#include "display/display.h"

/*
 * TODOS:
 *
 * - a few global variables need to be walled off.
 *
 */

// Which input channel on the capture hardware we want to receive frames from.
unsigned INPUT_CHANNEL_IDX = 0;

// How many frames the capture card should drop between captures.
unsigned FRAME_SKIP = 0;

// Whether VCS should attempt to prevent the screensaver from activating.
bool PREVENT_SCREENSAVER = true;

// The size of VCS's pre-allocated memory cache.
static unsigned MEM_CACHE_SIZE_MB = 256;

// Name of (and path to) the capture parameter file on disk.
static std::string VIDEO_PRESETS_FILE_NAME = "";

// Name of (and path to) the alias file on disk.
static std::string ALIAS_FILE_NAME = "";

// Name of (and path to) the filter set file on disk.
static std::string FILTER_GRAPH_FILE_NAME = "";

bool kcom_parse_command_line(const int argc, char *const argv[])
{
    const char parseFailMsg[] = "VCS has to exit because it found unexpected values "
                                "while parsing the command line. More information "
                                "will have been printed into the console. If a "
                                "console window was not already open, run VCS "
                                "again from the command line.";

    int c = 0;
    while ((c = getopt(argc, argv, "i:m:v:a:f:s")) != -1)
    {
        switch (c)
        {
            case 's':
            {
                PREVENT_SCREENSAVER = false;
                break;
            }
            case 'i':
            {
                INPUT_CHANNEL_IDX = strtol(optarg, NULL, 10);
                
                // Validate.
                {
                    const unsigned minChannelIdx = 1;    // Values must be 1-indexed.
                    const unsigned maxChannelIdx = 8192; // Sanity check.

                    if ((INPUT_CHANNEL_IDX < minChannelIdx) ||
                        (INPUT_CHANNEL_IDX > maxChannelIdx))
                    {
                        NBENE(("Input channel index (-i) is out of bounds. Expected range: %u-%u.",
                               minChannelIdx, maxChannelIdx));

                        kd_show_headless_error_message("", parseFailMsg);

                        return false;
                    }

                    // VCS expects the channel index to be 0-indexed, so let's convert.
                    INPUT_CHANNEL_IDX--;
                }

                break;
            }
            case 'm':
            {
                const int minSize = 1;
                const int maxSize = 8192;

                int size = strtol(optarg, NULL, 10);

                if ((size < minSize) ||
                    (size > maxSize))
                {
                    NBENE(("Memory cache size (-m) is out of bounds. Expected range: %d-%d.",
                           minSize, maxSize));

                    kd_show_headless_error_message("", parseFailMsg);

                    return false;
                }

                MEM_CACHE_SIZE_MB = unsigned(size);

                break;
            }
            case 'v':
            {
                VIDEO_PRESETS_FILE_NAME = optarg;
                break;
            }
            case 'a':
            {
                ALIAS_FILE_NAME = optarg;
                break;
            }
            case 'f':
            {
                FILTER_GRAPH_FILE_NAME = optarg;
                break;
            }
        }
    }

    return true;
}

void kcom_override_filter_graph_file_name(const std::string newFilename)
{
    FILTER_GRAPH_FILE_NAME = newFilename;

    return;
}

void kcom_override_aliases_file_name(const std::string newFilename)
{
    ALIAS_FILE_NAME = newFilename;

    return;
}

void kcom_override_video_presets_file_name(const std::string newFilename)
{
    VIDEO_PRESETS_FILE_NAME = newFilename;

    return;
}

bool kcom_prevent_screensaver(void)
{
    return PREVENT_SCREENSAVER;
}

unsigned kcom_mem_cache_size_mb(void)
{
    return MEM_CACHE_SIZE_MB;
}

const std::string& kcom_aliases_file_name(void)
{
    return ALIAS_FILE_NAME;
}

const std::string& kcom_filter_graph_file_name(void)
{
    return FILTER_GRAPH_FILE_NAME;
}

const std::string& kcom_video_presets_file_name(void)
{
    return VIDEO_PRESETS_FILE_NAME;
}
