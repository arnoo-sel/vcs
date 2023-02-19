/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_COMMON_COMMAND_LINE_COMMAND_LINE_H
#define VCS_COMMON_COMMAND_LINE_COMMAND_LINE_H

#include <string>

bool kcom_parse_command_line(const int argc, char *const argv[]);

bool kcom_prevent_screensaver(void);
const std::string& kcom_aliases_file_name(void);
const std::string& kcom_filter_graph_file_name(void);
const std::string& kcom_video_presets_file_name(void);

void kcom_override_filter_graph_file_name(const std::string newFilename);
void kcom_override_video_presets_file_name(const std::string newFilename);

#endif
